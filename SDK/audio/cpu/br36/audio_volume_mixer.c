/*
 ******************************************************************************************
 *							Volume Mixer
 *
 * Discription: 音量合成器
 *
 * Notes:
 *  (1)音量合成器框图如下：
 *    [bt]----->[app volume]---+
 *                             |
 *    [tone]--->[app volume]---+---->[sys volume]--->[Device(DAC/IIS/...)]
 *                             |
 *    [music]-->[app volume]---+
 *
 *	(2)app volume：应用音量，控制每个音频数据流独立的音量，互不影响
 * 				   对应windows音量合成器中的应用程序音量(Applications)
 *	(3)sys volume：系统音量，控制整个系统的全局音量
 *				   对应windows音量合成器中的设备音量(Device)
 ******************************************************************************************
 */
#include "system/includes.h"
#include "media/includes.h"
#include "user_cfg.h"
#include "app_config.h"
#include "app_action.h"
#include "app_main.h"
#include "jlstream.h"
#include "update.h"
#include "audio_config.h"
/*软件数字音量*/
#include "audio_dvol.h"
/*硬件数字音量*/
#include "audio_dac_digital_volume.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "volume_node.h"

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif/*TCFG_AUDIO_ANC_ENABLE*/

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#ifdef SUPPORT_MS_EXTENSIONS
#pragma const_seg(	".app_audio_const")
#pragma code_seg(	".app_audio_code")
#endif/*SUPPORT_MS_EXTENSIONS*/

#define LOG_TAG             "[APP_AUDIO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define WARNING_TONE_VOL_FIXED 1

struct __audio_bt_vol {
    u8 addr[6];
    s16 music_volume;
    u8 update_flag;
    u8 old;
};
static char *dvol_type[] = {"Music", "Call", "Tone", "Ring", "KTone"};
//数字音量类型的个数
#define DVOL_TYPE_NUM 5
typedef short unaligned_u16 __attribute__((aligned(1)));
struct app_audio_config {
    u8 state;								/*当前声音状态*/
    u8 prev_state;							/*上一个声音状态*/
    u8 mute_when_vol_zero;
    volatile u8 fade_gain_l;
    volatile u8 fade_gain_r;
    volatile s16 fade_dgain_l;
    volatile s16 fade_dgain_r;
    volatile s16 fade_dgain_step_l;
    volatile s16 fade_dgain_step_r;
    volatile int fade_timer;
    s16 digital_volume;
    u8 analog_volume_l;
    u8 analog_volume_r;
    s16 max_volume[APP_AUDIO_STATE_WTONE + 1];
    u8 sys_cvol_max;
    u8 call_cvol_max;
    unaligned_u16 *sys_cvol;
    unaligned_u16 *call_cvol;
    u8  music_mute_state;                       /*记录当前音乐是否处于mute*/
    u8  call_mute_state;                       /*记录当前通话是否处于mute*/
    u8  wtone_mute_state;                       /*记录当前提示音是否处于mute*/
};

/*声音状态字符串定义*/
static const char *audio_state[] = {
    "idle",
    "music",
    "call",
    "tone",
    "err",
};

/*音量类型字符串定义*/
static const char *vol_type[] = {
    "VOL_D",
    "VOL_A",
    "VOL_AD",
    "VOL_D_HW",
    "VOL_ERR",
};

static struct app_audio_config app_audio_cfg = {0};
#define __this      (&app_audio_cfg)
extern struct audio_dac_hdl dac_hdl;
extern struct dac_platform_data dac_data;


/*关闭audio相关模块使能*/
void audio_disable_all(void)
{
    //DAC:DACEN
    JL_AUDIO->DAC_CON &= ~BIT(4);
    //ADC:ADCEN
    JL_AUDIO->ADC_CON &= ~BIT(4);
    //EQ:
    JL_EQ->CON0 &= ~BIT(1);
    //FFT:
    JL_FFT->CON = BIT(1);//置1强制关闭模块，不管是否已经运算完成
    //ANC:anc_en anc_start
    JL_ANC->CON0 &= ~(BIT(1) | BIT(29));

}

REGISTER_UPDATE_TARGET(audio_update_target) = {
    .name = "audio",
    .driver_close = audio_disable_all,
};

/*
 *************************************************************
 *
 *	audio volume fade
 *
 *************************************************************
 */

static void audio_fade_timer(void *priv)
{
    u8 gain_l = dac_hdl.vol_l;
    u8 gain_r = dac_hdl.vol_r;
    //printf("<fade:%d-%d-%d-%d>", gain_l, gain_r, __this->fade_gain_l, __this->fade_gain_r);
    local_irq_disable();
    if ((gain_l == __this->fade_gain_l) && (gain_r == __this->fade_gain_r)) {
        usr_timer_del(__this->fade_timer);
        __this->fade_timer = 0;
        /*音量为0的时候mute住*/
        audio_dac_set_L_digital_vol(&dac_hdl, gain_l ? __this->digital_volume : 0);
        audio_dac_set_R_digital_vol(&dac_hdl, gain_r ? __this->digital_volume : 0);
        if ((gain_l == 0) && (gain_r == 0)) {
            if (__this->mute_when_vol_zero) {
                __this->mute_when_vol_zero = 0;
                /* audio_dac_mute(&dac_hdl, 1); */
            }
        }
        local_irq_enable();
        /*
         *淡入淡出结束，也把当前的模拟音量设置一下，防止因为淡入淡出的音量和保存音量的变量一致，
         *而寄存器已经被清0的情况
         */
        audio_dac_set_analog_vol(&dac_hdl, gain_l);
        /* log_info("dac_fade_end, VOL : 0x%x 0x%x\n", JL_ADDA->DAA_CON1, JL_AUDIO->DAC_VL0); */
        return;
    }
    if (gain_l > __this->fade_gain_l) {
        gain_l--;
    } else if (gain_l < __this->fade_gain_l) {
        gain_l++;
    }

    if (gain_r > __this->fade_gain_r) {
        gain_r--;
    } else if (gain_r < __this->fade_gain_r) {
        gain_r++;
    }
    audio_dac_set_analog_vol(&dac_hdl, gain_l);
    local_irq_enable();
}

static int audio_fade_timer_add(u8 gain_l, u8 gain_r)
{
    /* r_printf("dac_fade_begin:0x%x,gain_l:%d,gain_r:%d\n", __this->fade_timer, gain_l, gain_r); */
    local_irq_disable();
    __this->fade_gain_l = gain_l;
    __this->fade_gain_r = gain_r;
    if (__this->fade_timer == 0) {
        __this->fade_timer = usr_timer_add((void *)0, audio_fade_timer, 2, 1);
        /* y_printf("fade_timer:0x%x", __this->fade_timer); */
    }
    local_irq_enable();

    return 0;
}


#if (SYS_VOL_TYPE == VOL_TYPE_AD)

#define DGAIN_SET_MAX_STEP (300)
#define DGAIN_SET_MIN_STEP (30)

static unsigned short combined_vol_list[17][2] = {
    { 0,     0}, //0: None
    { 0,  2657}, // 1:-45.00 db
    { 0,  3476}, // 2:-42.67 db
    { 0,  4547}, // 3:-40.33 db
    { 0,  5948}, // 4:-38.00 db
    { 0,  7781}, // 5:-35.67 db
    { 0, 10179}, // 6:-33.33 db
    { 0, 13316}, // 7:-31.00 db
    { 1, 14198}, // 8:-28.67 db
    { 2, 14851}, // 9:-26.33 db
    { 3, 15596}, // 10:-24.00 db
    { 5, 13043}, // 11:-21.67 db
    { 6, 13567}, // 12:-19.33 db
    { 7, 14258}, // 13:-17.00 db
    { 8, 14749}, // 14:-14.67 db
    { 9, 15389}, // 15:-12.33 db
    {10, 16144}, // 16:-10.00 db
};

static unsigned short call_combined_vol_list[16][2] = {
    { 0,     0}, //0: None
    { 0,  4725}, // 1:-40.00 db
    { 0,  5948}, // 2:-38.00 db
    { 0,  7488}, // 3:-36.00 db
    { 0,  9427}, // 4:-34.00 db
    { 0, 11868}, // 5:-32.00 db
    { 0, 14941}, // 6:-30.00 db
    { 1, 15331}, // 7:-28.00 db
    { 2, 15432}, // 8:-26.00 db
    { 3, 15596}, // 9:-24.00 db
    { 4, 15788}, // 10:-22.00 db
    { 5, 15802}, // 11:-20.00 db
    { 6, 15817}, // 12:-18.00 db
    { 7, 15998}, // 13:-16.00 db
    { 8, 15926}, // 14:-14.00 db
    { 9, 15991}, // 15:-12.00 db
};

void audio_combined_vol_init(u8 cfg_en)
{
    u16 sys_cvol_len = 0;
    u16 call_cvol_len = 0;
    u8 *sys_cvol  = NULL;
    u8 *call_cvol  = NULL;
    s16 *cvol;

    __this->sys_cvol_max = ARRAY_SIZE(combined_vol_list) - 1;
    __this->sys_cvol = (unaligned_u16 *)combined_vol_list;
    __this->call_cvol_max = ARRAY_SIZE(call_combined_vol_list) - 1;
    __this->call_cvol = (unaligned_u16 *)call_combined_vol_list;

    if (cfg_en) {
        sys_cvol  = syscfg_ptr_read(CFG_COMBINE_SYS_VOL_ID, &sys_cvol_len);
        //ASSERT(((u32)sys_cvol & BIT(0)) == 0, "sys_cvol addr unalignd(2):%x\n", sys_cvol);
        if (sys_cvol && sys_cvol_len) {
            __this->sys_cvol = (unaligned_u16 *)sys_cvol;
            __this->sys_cvol_max = sys_cvol_len / 4 - 1;
            //y_printf("read sys_combine_vol succ:%x,len:%d",__this->sys_cvol,sys_cvol_len);
            /* cvol = __this->sys_cvol;
            for(int i = 0,j = 0;i < (sys_cvol_len / 2);j++) {
            	printf("sys_vol %d: %d, %d\n",j,*cvol++,*cvol++);
            	i += 2;
            } */
        } else {
            r_printf("read sys_cvol false:%x,%x\n", sys_cvol, sys_cvol_len);
        }

        call_cvol  = syscfg_ptr_read(CFG_COMBINE_CALL_VOL_ID, &call_cvol_len);
        //ASSERT(((u32)call_cvol & BIT(0)) == 0, "call_cvol addr unalignd(2):%d\n", call_cvol);
        if (call_cvol && call_cvol_len) {
            __this->call_cvol = (unaligned_u16 *)call_cvol;
            __this->call_cvol_max = call_cvol_len / 4 - 1;
            //y_printf("read call_combine_vol succ:%x,len:%d",__this->call_cvol,call_cvol_len);
            /* cvol = __this->call_cvol;
            for(int i = 0,j = 0;i < call_cvol_len / 2;j++) {
            	printf("call_vol %d: %d, %d\n",j,*cvol++,*cvol++);
            	i += 2;
            } */
        } else {
            r_printf("read call_combine_vol false:%x,%x\n", call_cvol, call_cvol_len);
        }
    }

    log_info("sys_cvol_max:%d,call_cvol_max:%d\n", __this->sys_cvol_max, __this->call_cvol_max);
}


static void audio_combined_fade_timer(void *priv)
{
    u8 gain_l = dac_hdl.vol_l;
    u8 gain_r = dac_hdl.vol_r;
    s16 dgain_l = dac_hdl.d_volume[DA_LEFT];
    s16 dgain_r = dac_hdl.d_volume[DA_RIGHT];

    __this->fade_dgain_step_l = __builtin_abs(dgain_l - __this->fade_dgain_l) / \
                                (__builtin_abs(gain_l - __this->fade_gain_l) + 1);
    if (__this->fade_dgain_step_l > DGAIN_SET_MAX_STEP) {
        __this->fade_dgain_step_l = DGAIN_SET_MAX_STEP;
    } else if (__this->fade_dgain_step_l < DGAIN_SET_MIN_STEP) {
        __this->fade_dgain_step_l = DGAIN_SET_MIN_STEP;
    }

    __this->fade_dgain_step_r = __builtin_abs(dgain_r - __this->fade_dgain_r) / \
                                (__builtin_abs(gain_r - __this->fade_gain_r) + 1);
    if (__this->fade_dgain_step_r > DGAIN_SET_MAX_STEP) {
        __this->fade_dgain_step_r = DGAIN_SET_MAX_STEP;
    } else if (__this->fade_dgain_step_r < DGAIN_SET_MIN_STEP) {
        __this->fade_dgain_step_r = DGAIN_SET_MIN_STEP;
    }

    /* log_info("<a:%d-%d-%d-%d d:%d-%d-%d-%d-%d-%d>\n", \ */
    /* gain_l, gain_r, __this->fade_gain_l, __this->fade_gain_r, \ */
    /* dgain_l, dgain_r, __this->fade_dgain_l, __this->fade_dgain_r, \ */
    /* __this->fade_dgain_step_l, __this->fade_dgain_step_r); */

    local_irq_disable();

    if ((gain_l == __this->fade_gain_l) \
        && (gain_r == __this->fade_gain_r) \
        && (dgain_l == __this->fade_dgain_l)\
        && (dgain_r == __this->fade_dgain_r)) {
        usr_timer_del(__this->fade_timer);
        __this->fade_timer = 0;
        /*音量为0的时候mute住*/
        if ((gain_l == 0) && (gain_r == 0)) {
            if (__this->mute_when_vol_zero) {
                __this->mute_when_vol_zero = 0;
                /* audio_dac_mute(&dac_hdl, 1); */
            }
        }

        local_irq_enable();
        /* log_info("dac_fade_end,VOL:0x%x-0x%x-%d-%d-%d-%d\n", \ */
        /* JL_ADDA->DAA_CON1, JL_AUDIO->DAC_VL0,  \ */
        /* __this->fade_gain_l, __this->fade_gain_r, \ */
        /* __this->fade_dgain_l, __this->fade_dgain_r); */
        return;
    }
    if ((gain_l != __this->fade_gain_l) \
        || (gain_r != __this->fade_gain_r)) {
        if (gain_l > __this->fade_gain_l) {
            gain_l--;
        } else if (gain_l < __this->fade_gain_l) {
            gain_l++;
        }

        if (gain_r > __this->fade_gain_r) {
            gain_r--;
        } else if (gain_r < __this->fade_gain_r) {
            gain_r++;
        }

        audio_dac_set_analog_vol(&dac_hdl, gain_l);
        local_irq_enable();//fix : 不同时调模拟和数字可以避免杂音
        return;
    }

    if ((dgain_l != __this->fade_dgain_l) \
        || (dgain_r != __this->fade_dgain_r)) {

        if (gain_l != __this->fade_gain_l) {
            if (dgain_l > __this->fade_dgain_l) {
                if ((dgain_l - __this->fade_dgain_l) >= __this->fade_dgain_step_l) {
                    dgain_l -= __this->fade_dgain_step_l;
                } else {
                    dgain_l = __this->fade_dgain_l;
                }
            } else if (dgain_l < __this->fade_dgain_l) {
                if ((__this->fade_dgain_l - dgain_l) >= __this->fade_dgain_step_l) {
                    dgain_l += __this->fade_dgain_step_l;
                } else {
                    dgain_l = __this->fade_dgain_l;
                }
            }
        } else {
            dgain_l = __this->fade_dgain_l;
        }

        if (gain_r != __this->fade_gain_r) {
            if (dgain_r > __this->fade_dgain_r) {
                if ((dgain_r - __this->fade_dgain_r) >= __this->fade_dgain_step_r) {
                    dgain_r -= __this->fade_dgain_step_r;
                } else {
                    dgain_r = __this->fade_dgain_r;
                }
            } else if (dgain_r < __this->fade_dgain_r) {
                if ((__this->fade_dgain_r - dgain_r) >= __this->fade_dgain_step_r) {
                    dgain_r += __this->fade_dgain_step_r;
                } else {
                    dgain_r = __this->fade_dgain_r;
                }
            }
        } else {
            dgain_r = __this->fade_dgain_r;
        }
        audio_dac_set_digital_vol(&dac_hdl, dgain_l);
    }

    local_irq_enable();
}


static int audio_combined_fade_timer_add(u8 gain_l, u8 gain_r)
{
    u8  gain_max;
    u8  target_again_l = 0;
    u8  target_again_r = 0;
    u16 target_dgain_l = 0;
    u16 target_dgain_r = 0;

    if (__this->state == APP_AUDIO_STATE_CALL) {
        gain_max = __this->call_cvol_max;
        gain_l = (gain_l > gain_max) ? gain_max : gain_l;
        gain_r = (gain_r > gain_max) ? gain_max : gain_r;
        target_again_l = *(&__this->call_cvol[gain_l * 2]);
        target_again_r = *(&__this->call_cvol[gain_r * 2]);
        target_dgain_l = *(&__this->call_cvol[gain_l * 2 + 1]);
        target_dgain_r = *(&__this->call_cvol[gain_r * 2 + 1]);
    } else {
        gain_max = __this->sys_cvol_max;
        gain_l = (gain_l > gain_max) ? gain_max : gain_l;
        gain_r = (gain_r > gain_max) ? gain_max : gain_r;
        target_again_l = *(&__this->sys_cvol[gain_l * 2]);
        target_again_r = *(&__this->sys_cvol[gain_r * 2]);
        target_dgain_l = *(&__this->sys_cvol[gain_l * 2 + 1]);
        target_dgain_r = *(&__this->sys_cvol[gain_r * 2 + 1]);
    }
#if 0//TCFG_AUDIO_ANC_ENABLE
    target_again_l = anc_dac_gain_get(ANC_DAC_CH_L);
    target_again_r = anc_dac_gain_get(ANC_DAC_CH_R);
#endif

    printf("[l]v:%d,Av:%d,Dv:%d", gain_l, target_again_l, target_dgain_l);
    //y_printf("[r]v:%d,Av:%d,Dv:%d", gain_r, target_again_r, target_dgain_r);
    /* log_info("dac_com_fade_begin:0x%x\n", __this->fade_timer); */

    local_irq_disable();

    __this->fade_gain_l  = target_again_l;
    __this->fade_gain_r  = target_again_r;
    __this->fade_dgain_l = target_dgain_l;
    __this->fade_dgain_r = target_dgain_r;

    if (__this->fade_timer == 0) {
        __this->fade_timer = usr_timer_add((void *)0, audio_combined_fade_timer, 2, 1);
        /* log_info("combined_fade_timer:0x%x", __this->fade_timer); */
    }

    local_irq_enable();

    return 0;
}

#endif/*SYS_VOL_TYPE == VOL_TYPE_AD*/

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
#define DVOL_HW_LEVEL_MAX	31	/*注意:总共是(DVOL_HW_LEVEL_MAX + 1)级*/
const u16 hw_dig_vol_table[DVOL_HW_LEVEL_MAX + 1] = {
    0	, //0
    93	, //1
    111	, //2
    132	, //3
    158	, //4
    189	, //5
    226	, //6
    270	, //7
    323	, //8
    386	, //9
    462	, //10
    552	, //11
    660	, //12
    789	, //13
    943	, //14
    1127, //15
    1347, //16
    1610, //17
    1925, //18
    2301, //19
    2751, //20
    3288, //21
    3930, //22
    4698, //23
    5616, //24
    6713, //25
    8025, //26
    9592, //27
    11466,//28
    15200,//29
    16000,//30
    16384 //31
};
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW*/

void audio_volume_list_init(u8 cfg_en)
{
#if (SYS_VOL_TYPE == VOL_TYPE_AD)
    audio_combined_vol_init(cfg_en);
#elif (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
    /* audio_hw_digital_vol_init(cfg_en); */
#endif/*SYS_VOL_TYPE*/
}

static void set_audio_device_volume(u8 type, s16 vol)
{
    audio_dac_set_analog_vol(&dac_hdl, vol);
}

static int get_audio_device_volume(u8 vol_type)
{
    return 0;
}

void volume_up_down_direct(s16 value)
{

}

/*
*********************************************************************
*          			Audio Volume Fade
* Description: 音量淡入淡出
* Arguments  : left_vol
*			   right_vol
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_fade_in_fade_out(u8 left_vol, u8 right_vol)
{
    /*根据audio state切换的时候设置的最大音量,限制淡入淡出的最大音量*/
    u8 max_vol_l = __this->max_volume[__this->state];
    u8 max_vol_r = max_vol_l;
    printf("[fade]state:%s,max_volume:%d,cur:%d,%d", audio_state[__this->state], max_vol_l, left_vol, left_vol);
    /*淡入淡出音量限制*/
    u8 left_gain = left_vol > max_vol_l ? max_vol_l : left_vol;
    u8 right_gain = right_vol > max_vol_r ? max_vol_r : right_vol;

    //printf("vol_type:%s,target:%d,%d\n",vol_type[SYS_VOL_TYPE],left_gain,right_gain);

#if TCFG_CALL_USE_DIGITAL_VOLUME
    if (__this->state == APP_AUDIO_STATE_CALL) {
        /*通话使用数字音量的时候，模拟音量默认最大*/
        left_gain = max_vol_l;
        right_gain = max_vol_r;
        //printf("phone_call_use_digital_volume,a_vol max:%d,%d",left_gain,right_gain);
    }
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/

    /*数字音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#if 0//TCFG_AUDIO_ANC_ENABLE
    if (left_gain || (anc_mode_get() != ANC_OFF)) {
        left_gain = anc_dac_gain_get(ANC_DAC_CH_L);
        right_gain = anc_dac_gain_get(ANC_DAC_CH_R);
    }
    /* audio_fade_timer_add(left_gain, right_gain); */
#else
    u8 dvol_idx = 0; //记录音量通道供数字音量控制使用
    s16 volume = right_gain;
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        dvol_idx = MUSIC_DVOL;
        break;
    case APP_AUDIO_STATE_CALL:
        dvol_idx = CALL_DVOL;
        break;
    case APP_AUDIO_STATE_WTONE:
        dvol_idx = TONE_DVOL;
        break;
    default:
        break;
    }
    if (volume > __this->max_volume[__this->state]) {
        volume = __this->max_volume[__this->state];
    }
    printf("set_vol[%s]:=%d\n", audio_state[__this->state], volume);
    extern void audio_digital_vol_set_no_fade(u8 dvol_idx, u8 vol);
    if (left_gain || right_gain) {
        audio_dac_set_digital_vol(&dac_hdl, DEFAULT_DIGITAL_VOLUME);
        /* audio_digital_vol_set_no_fade(dvol_idx, volume); */
    } else {
        audio_dac_set_digital_vol(&dac_hdl, 0);
        /* audio_digital_vol_set_no_fade(dvol_idx, 0); */
    }
    //audio_fade_timer_add(max_vol_l, max_vol_r);
#endif/*TCFG_AUDIO_ANC_ENABLE*/
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

    /*模拟音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_ANALOG)
    audio_fade_timer_add(left_gain, right_gain);
#endif/*SYS_VOL_TYPE == VOL_TYPE_ANALOG*/

    /*模拟数字联合音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_AD)
#if TCFG_CALL_USE_DIGITAL_VOLUME
    if (__this->state == APP_AUDIO_STATE_CALL) {
        audio_fade_timer_add(left_gain, right_gain);
    } else {
        audio_combined_fade_timer_add(left_gain, right_gain);
    }
#else
    audio_combined_fade_timer_add(left_gain, right_gain);
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
#endif/*SYS_VOL_TYPE == VOL_TYPE_AD*/

    /*硬件数字音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
    u8 dvol_hw_level = 0;
    extern float eq_db2mag(float x);
    float dvol_db;
    float dvol_gain;
    int dvol_max;
    if (__this->state == APP_AUDIO_STATE_CALL) {
        dvol_hw_level = app_var.aec_dac_gain * DVOL_HW_LEVEL_MAX / BT_CALL_VOL_LEAVE_MAX;
        dvol_max = hw_dig_vol_table[dvol_hw_level];
        dvol_db = (BT_CALL_VOL_LEAVE_MAX - left_vol) * BT_CALL_VOL_STEP;
        dvol_gain = eq_db2mag(dvol_db);//dB转换倍数
        __this->digital_volume = (s16)(dvol_max * dvol_gain);
    } else {
        dvol_db =  BT_MUSIC_VOL_MAX + (BT_MUSIC_VOL_LEAVE_MAX - left_gain) * BT_MUSIC_VOL_STEP;
        __this->digital_volume = (s16)(16384.0f * eq_db2mag(dvol_db));
        /* dvol_hw_level = left_gain * DVOL_HW_LEVEL_MAX / get_max_sys_vol(); */
        /* __this->digital_volume = hw_dig_vol_table[dvol_hw_level]; */
        /* __this->digital_volume = left_gain * 16384 / get_max_sys_vol(); */
    }
    //printf("dvol_hw:%d->%d->%d",left_gain, (int)(dvol_db*10),__this->digital_volume);
    audio_dac_set_digital_vol(&dac_hdl, __this->digital_volume);
    //audio_fade_timer_add(__this->analog_volume_l, __this->analog_volume_r);
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW*/
}

int audio_digital_vol_node_name_get(u8 dvol_idx, char *node_name)
{
    struct app_mode *mode;
    mode = app_get_current_mode();
    int i = 0;
    for (i = 0; i < DVOL_TYPE_NUM; i++) {
        if (dvol_idx & BIT(i)) {
            switch (mode->name) {
            case APP_MODE_BT:
#if TCFG_AUDIO_DUT_ENABLE
                if (audio_dec_dut_en_get(1)) {
                    sprintf(node_name, "%s%s", "Vol_Btd", dvol_type[i]);
                    printf("vol_name:%d,%s\n", __LINE__, node_name);
                    break;
                }
#endif/*TCFG_AUDIO_DUT_ENABLE*/
                if (esco_player_is_playing(NULL)) { //用于区分通话和播歌的提示音
                    sprintf(node_name, "%s%s", "Vol_Btc", dvol_type[i]);
                } else {
                    sprintf(node_name, "%s%s", "Vol_Btm", dvol_type[i]);
                }
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                break;
            case APP_MODE_IDLE:
                sprintf(node_name, "%s%s", "Vol_Sys", dvol_type[i]);
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                break;
            default:
                printf("vol_name:%d,NULL\n", __LINE__);
                return -1;
            }
        } //end of if
    } //end of for
    return 0;
}

int audio_digital_vol_update_parm(u8 dvol_idx, s32 param)
{
    int err = 0;
    char vol_name[32];
    err = audio_digital_vol_node_name_get(dvol_idx, vol_name);
    if (!err) {
        err |= jlstream_set_node_param(NODE_UUID_VOLUME_CTRLER, vol_name, &param, sizeof(param));
    }
    return err;
}

extern struct volume_cfg default_vol_cfg;
//获取当前模式music数据流节点的默认音量
int audio_digital_vol_default_init(void)
{
    int ret = 0;
    if (app_var.volume_def_state) {
        char vol_name[32];
        struct volume_cfg cfg;
        ret = audio_digital_vol_node_name_get(MUSIC_DVOL, vol_name);
        if (!ret) {
            ret = volume_ioc_get_cfg(vol_name, &cfg);
        } else {
            cfg = default_vol_cfg;
        }
        app_var.music_volume = cfg.cur_vol;
    }
    return ret;
}

//设置是否使用工具上配置的当前音量
void app_audio_set_volume_def_state(u8 volume_def_state)
{
    app_var.volume_def_state = volume_def_state;
}


static void app_audio_set_vol_timer_func(void *arg)
{
    u8 dvol_idx = ((u32)(arg) >> 16) & 0xff;
    s16 volume = ((u32)arg) & 0xffff;
    s32 param = VOLUME_NODE_CMD_SET_VOL  | volume;
    audio_digital_vol_update_parm(dvol_idx, param);
}

static void app_audio_set_mute_timer_func(void *arg)
{
    u8 dvol_idx = ((u32)(arg) >> 16) & 0xff;
    s16 mute_en = ((u32)arg) & 0xffff;
    s32 param = VOLUME_NODE_CMD_SET_MUTE  | mute_en;
    audio_digital_vol_update_parm(dvol_idx, param);
}

/*
*********************************************************************
*          			Audio Volume Set
* Description: 音量设置
* Arguments  : state	目标声音通道
*			   volume	目标音量值
*			   fade		是否淡入淡出
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_app_volume_set(u8 state, s16 volume, u8 fade)
{
    // y_printf("audio_app_volume_set, state = %d, volume = %d, fade = %d", state, volume, fade);
    u8 dvol_idx = 0; //记录音量通道供数字音量控制使用
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume = volume;
        dvol_idx = MUSIC_DVOL;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume = volume;
        dvol_idx = CALL_DVOL;
#if TCFG_CALL_USE_DIGITAL_VOLUME
        dac_digital_vol_set(volume, volume, 1);
        return;
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
        break;
    case APP_AUDIO_STATE_WTONE:
#if WARNING_TONE_VOL_FIXED
        return;
#endif
        app_var.wtone_volume = volume;
        dvol_idx = TONE_DVOL | RING_DVOL | KEY_TONE_DVOL;
        break;
    default:
        return;
    }
    app_audio_bt_volume_save(state);
    /*printf("set_vol[%s]:%s=%d\n", audio_state[__this->state], audio_state[state], volume);*/

    if (state == __this->state) {

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        if (volume > __this->max_volume[state]) {
            volume = __this->max_volume[state];
        }
        u32 param = dvol_idx << 16 | volume;
        sys_timeout_add((void *)param, app_audio_set_vol_timer_func, 5); //5ms后更新音量
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

        audio_dac_set_volume(&dac_hdl, volume);
        if (audio_dac_get_status(&dac_hdl) && (!app_audio_get_dac_digital_mute())) {
            if (fade) {
                audio_fade_in_fade_out(volume, volume);
            } else {
                audio_dac_set_analog_vol(&dac_hdl, volume);
            }
        }
    }
}

/*
*********************************************************************
*          			Audio Volume MUTE
* Description: 将数据静音或者解开静音
* Arguments  : mute_en	是否使能静音, 0:不使能,1:使能
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_app_mute_en(u8 mute_en)
{
    u8 dvol_idx = 0; //记录音量通道供数字音量控制使用
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        dvol_idx = MUSIC_DVOL;
        __this->music_mute_state = mute_en;
        break;
    case APP_AUDIO_STATE_CALL:
        dvol_idx = CALL_DVOL;
        __this->call_mute_state = mute_en;
        break;
    case APP_AUDIO_STATE_WTONE:
#if WARNING_TONE_VOL_FIXED
        return;
#endif
        dvol_idx = TONE_DVOL | RING_DVOL | KEY_TONE_DVOL;
        __this->wtone_mute_state = mute_en;
        break;
    default:
        break;
    }
    u32 param = dvol_idx << 16 | mute_en;
    sys_timeout_add((void *)param, app_audio_set_mute_timer_func, 5); //5ms后将数据mute 或者解mute
}
/*
*********************************************************************
*                  Audio Volume Get
* Description: 音量获取
* Arguments  : state	要获取音量值的音量状态
* Return	 : 返回指定状态对应得音量值
* Note(s)    : None.
*********************************************************************
*/
s16 app_audio_get_volume(u8 state)
{
    s16 volume = 0;
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
        volume = app_var.call_volume;
        break;
    case APP_AUDIO_STATE_WTONE:
        volume = app_var.wtone_volume;
        if (!volume) {
            volume = app_var.music_volume;
        }
        break;
    case APP_AUDIO_CURRENT_STATE:
        volume = app_audio_get_volume(__this->state);
        break;
    default:
        break;
    }

    return volume;
}

/*
*********************************************************************
*                  Audio Mute Get
* Description: mute状态获取
* Arguments  : state	要获取是否mute的音量状态
* Return	 : 返回指定状态对应的mute状态
* Note(s)    : None.
*********************************************************************
*/
u8 app_audio_get_mute_state(u8 state)
{
    u8 mute_state = 0;
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        mute_state = __this->music_mute_state;
        break;
    case APP_AUDIO_STATE_CALL:
        mute_state = __this->call_mute_state;
        break;
    case APP_AUDIO_STATE_WTONE:
        mute_state = __this->wtone_mute_state;
        break;
    case APP_AUDIO_CURRENT_STATE:
        mute_state = app_audio_get_mute_state(__this->state);
        break;
    default:
        break;
    }
    return mute_state;
}


/*
*********************************************************************
*                  Audio Mute
* Description: 静音控制
* Arguments  : value Mute操作
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
static const char *audio_mute_string[] = {
    "mute_default",
    "unmute_default",
    "mute_L",
    "unmute_L",
    "mute_R",
    "unmute_R",
};

void app_audio_mute(u8 value)
{
    printf("audio_mute:%s", audio_mute_string[value]);
    switch (value) {
    case AUDIO_MUTE_DEFAULT:
        audio_dac_digital_mute(&dac_hdl, 1);
        break;
    case AUDIO_UNMUTE_DEFAULT:
        audio_dac_digital_mute(&dac_hdl, 0);
        break;
    }
}

u8 app_audio_get_dac_digital_mute() //获取DAC 是否mute
{
    return audio_dac_digital_mute_state(&dac_hdl);
}

/*
*********************************************************************
*                  Audio Volume Up
* Description: 增加当前音量通道的音量
* Arguments  : value	要增加的音量值
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_app_volume_up(u8 value)
{
    s16 volume = 0;
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume += value;
        if (app_var.music_volume > app_audio_get_max_volume()) {
            app_var.music_volume = app_audio_get_max_volume();
        }
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume += value;
#if TCFG_CALL_USE_DIGITAL_VOLUME
        volume = app_var.call_volume;
        dac_digital_vol_set(volume, volume, 1);
        return;
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/

        /*模拟音量类型，通话的时候直接限制最大音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_ANALOG)
        if (app_var.call_volume > app_var.aec_dac_gain) {
            app_var.call_volume = app_var.aec_dac_gain;
        }
#endif/*SYS_VOL_TYPE == VOL_TYPE_ANALOG*/
        volume = app_var.call_volume;
        break;
    case APP_AUDIO_STATE_WTONE:
#if WARNING_TONE_VOL_FIXED
        return;
#endif
        app_var.wtone_volume += value;
        if (app_var.wtone_volume > app_audio_get_max_volume()) {
            app_var.wtone_volume = app_audio_get_max_volume();
        }
        volume = app_var.wtone_volume;
        break;
    default:
        return;
    }
    app_audio_set_volume(__this->state, volume, 1);
}

/*
*********************************************************************
*                  Audio Volume Down
* Description: 减少当前音量通道的音量
* Arguments  : value	要减少的音量值
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_app_volume_down(u8 value)
{
    s16 volume = 0;
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume -= value;
        if (app_var.music_volume < 0) {
            app_var.music_volume = 0;
        }
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume -= value;
        if (app_var.call_volume < 0) {
            app_var.call_volume = 0;
        }
        volume = app_var.call_volume;
#if TCFG_CALL_USE_DIGITAL_VOLUME
        dac_digital_vol_set(volume, volume, 1);
        return;
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
        break;
    case APP_AUDIO_STATE_WTONE:
#if WARNING_TONE_VOL_FIXED
        return;
#endif
        app_var.wtone_volume -= value;
        if (app_var.wtone_volume < 0) {
            app_var.wtone_volume = 0;
        }
        volume = app_var.wtone_volume;
        break;
    default:
        return;
    }

    app_audio_set_volume(__this->state, volume, 1);
}

/*level:0~15*/
static const u16 phone_call_dig_vol_tab[] = {
    0,	//0
    111,//1
    161,//2
    234,//3
    338,//4
    490,//5
    708,//6
    1024,//7
    1481,//8
    2142,//9
    3098,//10
    4479,//11
    6477,//12
    9366,//13
    14955,//14
    16384 //15
};

/*
*********************************************************************
*          			Audio Volume Set
* Description: 音量设置
* Arguments  : state	目标声音通道
*			   volume	目标音量值
*			   fade		是否淡入淡出
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_init_dig_vol(u8 state, s16 volume, u8 fade, dvol_handle *dvol_hdl)
{
    u8 dvol_idx = 0; //记录音量通道供数字音量控制使用
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume = volume;
        dvol_idx = MUSIC_DVOL;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume = volume;
        dvol_idx = CALL_DVOL;
#if TCFG_CALL_USE_DIGITAL_VOLUME
        dac_digital_vol_set(volume, volume, 1);
        return;
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
        break;
    case APP_AUDIO_STATE_WTONE:
        app_var.wtone_volume = volume;
        dvol_idx = TONE_DVOL;
        break;
    default:
        return;
    }
    app_audio_bt_volume_save(state);
    /*printf("set_vol[%s]:%s=%d\n", audio_state[__this->state], audio_state[state], volume);*/

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    if (volume > __this->max_volume[state]) {
        volume = __this->max_volume[state];
    }
    audio_digital_vol_set(dvol_hdl, volume);
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

    if (state == __this->state) {
        audio_dac_set_volume(&dac_hdl, volume);
        if (audio_dac_get_status(&dac_hdl) && (!app_audio_get_dac_digital_mute())) {
            if (fade) {
                audio_fade_in_fade_out(volume, volume);
            } else {
                audio_dac_set_analog_vol(&dac_hdl, volume);
            }
        }
    }
}

/*
*********************************************************************
*                  Audio State Switch
* Description: 切换声音状态
* Arguments  : state
*			   max_volume
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/

void app_audio_state_switch(u8 state, s16 max_volume, dvol_handle *dvol_hdl)
{
    /*printf("audio_state:%s->%s,max_vol:%d\n", audio_state[__this->state], audio_state[state], max_volume);*/

    __this->prev_state = __this->state;
    __this->state = state;
    /*跟进音量类型，限制最大音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
    __this->analog_volume_l = MAX_ANA_VOL;
    __this->analog_volume_r = MAX_ANA_VOL;
#else
    __this->digital_volume = DEFAULT_DIGITAL_VOLUME;
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW*/
    /*记录当前状态对应的最大音量*/
    __this->max_volume[state] = max_volume;

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    __this->analog_volume_l = MAX_ANA_VOL;
    __this->analog_volume_r = MAX_ANA_VOL;
#if 0//TCFG_AUDIO_ANC_ENABLE
    __this->analog_volume_l = anc_dac_gain_get(ANC_DAC_CH_L);
    __this->analog_volume_r = anc_dac_gain_get(ANC_DAC_CH_R);
    //g_printf("anc mode,vol_l:%d,vol_r:%d", __this->analog_volume_l, __this->analog_volume_r);
#endif/*TCFG_AUDIO_ANC_ENABLE*/
    __this->digital_volume = DEFAULT_DIGITAL_VOLUME;
#else
    if (__this->state == APP_AUDIO_STATE_CALL) {
#if TCFG_CALL_USE_DIGITAL_VOLUME
        u8 call_volume_level_max = ARRAY_SIZE(phone_call_dig_vol_tab) - 1;
        app_var.call_volume = (app_var.call_volume > call_volume_level_max) ? call_volume_level_max : app_var.call_volume;
        __this->digital_volume = phone_call_dig_vol_tab[app_var.call_volume];
        printf("call_volume:%d,digital_volume:%d", app_var.call_volume, __this->digital_volume);
        dac_digital_vol_open();
        dac_digital_vol_tab_register(phone_call_dig_vol_tab, ARRAY_SIZE(phone_call_dig_vol_tab));
        /*调数字音量的时候，模拟音量定最大*/
        audio_dac_set_analog_vol(&dac_hdl, max_volume);
#elif (SYS_VOL_TYPE == VOL_TYPE_AD)
        /*通话联合音量调节的时候，最大音量为15，和手机的等级一致*/
        __this->max_volume[state] = BT_CALL_VOL_LEAVE_MAX;
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
    }
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

    app_audio_init_dig_vol(state, app_audio_get_volume(state), 1, dvol_hdl);
}

/*
*********************************************************************
*                  Audio State Exit
* Description: 退出当前的声音状态
* Arguments  : state	要退出的声音状态
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_state_exit(u8 state)
{
#if TCFG_CALL_USE_DIGITAL_VOLUME
    if (__this->state == APP_AUDIO_STATE_CALL) {
        dac_digital_vol_close();
    }
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
    if (state == __this->state) {
        __this->state = __this->prev_state;
        __this->prev_state = APP_AUDIO_STATE_IDLE;
    } else if (state == __this->prev_state) {
        __this->prev_state = APP_AUDIO_STATE_IDLE;
    }
}

/*
*********************************************************************
*                  Audio Set Max Volume
* Description: 设置最大音量
* Arguments  : state		要设置最大音量的声音状态
*			   max_volume	最大音量
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_set_max_volume(u8 state, s16 max_volume)
{
    __this->max_volume[state] = max_volume;
}

/*
*********************************************************************
*                  Audio State Get
* Description: 获取当前声音状态
* Arguments  : None.
* Return	 : 返回当前的声音状态
* Note(s)    : None.
*********************************************************************
*/
u8 app_audio_get_state(void)
{
    return __this->state;
}

/*
*********************************************************************
*                  Audio Volume_Max Get
* Description: 获取当前声音通道的最大音量
* Arguments  : None.
* Return	 : 返回当前的声音通道最大音量
* Note(s)    : None.
*********************************************************************
*/
s16 app_audio_get_max_volume(void)
{
    if (__this->state == APP_AUDIO_STATE_IDLE) {
        return  app_audio_volume_max_query(AppVol_BT_MUSIC);
    }
#if TCFG_CALL_USE_DIGITAL_VOLUME
    if (__this->state == APP_AUDIO_STATE_CALL) {
        return (ARRAY_SIZE(phone_call_dig_vol_tab) - 1);
    }
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
    return __this->max_volume[__this->state];
}

void app_audio_set_mix_volume(u8 front_volume, u8 back_volume)
{
    /*set_audio_device_volume(AUDIO_MIX_FRONT_VOL, front_volume);
    set_audio_device_volume(AUDIO_MIX_BACK_VOL, back_volume);*/
}
#if 0

void audio_vol_test()
{
    app_set_sys_vol(10, 10);
    log_info("sys vol %d %d\n", get_audio_device_volume(AUDIO_SYS_VOL) >> 16, get_audio_device_volume(AUDIO_SYS_VOL) & 0xffff);
    log_info("ana vol %d %d\n", get_audio_device_volume(AUDIO_ANA_VOL) >> 16, get_audio_device_volume(AUDIO_ANA_VOL) & 0xffff);
    log_info("dig vol %d %d\n", get_audio_device_volume(AUDIO_DIG_VOL) >> 16, get_audio_device_volume(AUDIO_DIG_VOL) & 0xffff);
    log_info("max vol %d %d\n", get_audio_device_volume(AUDIO_MAX_VOL) >> 16, get_audio_device_volume(AUDIO_MAX_VOL) & 0xffff);

    app_set_max_vol(30);
    app_set_ana_vol(25, 24);
    app_set_dig_vol(90, 90);

    log_info("sys vol %d %d\n", get_audio_device_volume(AUDIO_SYS_VOL) >> 16, get_audio_device_volume(AUDIO_SYS_VOL) & 0xffff);
    log_info("ana vol %d %d\n", get_audio_device_volume(AUDIO_ANA_VOL) >> 16, get_audio_device_volume(AUDIO_ANA_VOL) & 0xffff);
    log_info("dig vol %d %d\n", get_audio_device_volume(AUDIO_DIG_VOL) >> 16, get_audio_device_volume(AUDIO_DIG_VOL) & 0xffff);
    log_info("max vol %d %d\n", get_audio_device_volume(AUDIO_MAX_VOL) >> 16, get_audio_device_volume(AUDIO_MAX_VOL) & 0xffff);
}
#endif


int esco_dec_dac_gain_set(u8 gain)
{
    y_printf("esco_dec_dac_gain_set");
    app_var.aec_dac_gain = gain;
    if (esco_player_runing()) {
#if (SYS_VOL_TYPE == VOL_TYPE_ANALOG)
        audio_dac_set_analog_vol(&dac_hdl, gain);
#else
        app_audio_set_max_volume(APP_AUDIO_STATE_CALL, gain);
        y_printf("-----3333333333");
        app_audio_set_volume(APP_AUDIO_STATE_CALL, app_audio_get_volume(APP_AUDIO_STATE_CALL), 1);
#endif/*SYS_VOL_TYPE*/
    }
    return 0;
}

#if TCFG_1T2_VOL_RESUME_WHEN_NO_SUPPORT_VOL_SYNC

extern u8 get_music_vol_for_no_vol_sync_dev(u8 *addr);
extern void set_music_vol_for_no_vol_sync_dev(u8 *addr, u8 vol);
//保存没有音量同步设备音量
void app_audio_bt_volume_save(u8 state)
{
    u8 avrcp_vol = 0;
    u8 cur_btaddr[6];
    struct app_mode *mode;
    mode = app_get_current_mode();
    if ((state != APP_AUDIO_STATE_MUSIC) || (mode->name != APP_MODE_BT)) {
        //只有蓝牙模式才会保存音量
        //暂时不对其他音量记录，通话音量蓝牙库会返回
        return;
    }
    a2dp_player_get_btaddr(cur_btaddr);
    struct volume_cfg vol_cfg = {0};
    s16 max_vol = app_audio_volume_max_query(AppVol_BT_MUSIC);
    avrcp_vol = (app_var.music_volume * 127) / max_vol;
    set_music_vol_for_no_vol_sync_dev(cur_btaddr, avrcp_vol);
}

//更新没有音量同步设备音量
u8 app_audio_bt_volume_update(u8 *btaddr, u8 state)
{
    u8 vol = 0;
    vol = get_music_vol_for_no_vol_sync_dev(btaddr);
    if (vol == 0xff) {
        set_music_vol_for_no_vol_sync_dev(btaddr, 127);
        vol = 127;
    }
    printf("btaddr vol update %s vol %d\n", audio_state[state], vol);
    return vol;
}
#else
void app_audio_bt_volume_save(u8 state) {}
u8 app_audio_bt_volume_update(u8 *btaddr, u8 state) {}
#endif/*TCFG_1T2_VOL_RESUME_WHEN_NO_SUPPORT_VOL_SYNC*/

/*
*********************************************************************
*           app_audio_dac_vol_mode_set
* Description: DAC 音量增强模式切换
* Arguments  : mode		1：音量增强模式  0：普通模式
* Return	 : 0：成功  -1：失败
* Note(s)    : None.
*********************************************************************
*/
void app_audio_dac_vol_mode_set(u8 mode)
{
    int ret = 0;
    audio_dac_volume_enhancement_mode_set(&dac_hdl, mode);
    syscfg_write(CFG_VOLUME_ENHANCEMENT_MODE, &mode, 1);
    printf("DAC VOL MODE SET: %s\n", mode ? "VOLUME_ENHANCEMENT_MODE" : "NORMAL_MODE");
}

/*
*********************************************************************
*           app_audio_dac_vol_mode_get
* Description: DAC 音量增强模式状态获取
* Arguments  : None.
* Return	 : 1：音量增强模式  0：普通模式
* Note(s)    : None.
*********************************************************************
*/
u8 app_audio_dac_vol_mode_get(void)
{
    return audio_dac_volume_enhancement_mode_get(&dac_hdl);
}

void app_audio_set_volume(u8 state, s16 volume, u8 fade)
{
    audio_app_volume_set(state, volume, fade);
}


void app_audio_change_volume(u8 state, s16 volume)
{
    u8 dvol_idx = 0; //记录音量通道供数字音量控制使用
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume = volume;
        dvol_idx = MUSIC_DVOL;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume = volume;
        dvol_idx = CALL_DVOL;
#if TCFG_CALL_USE_DIGITAL_VOLUME
        dac_digital_vol_set(volume, volume, 1);
        return;
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
        break;
    case APP_AUDIO_STATE_WTONE:
#if WARNING_TONE_VOL_FIXED
        return;
#endif
        app_var.wtone_volume = volume;
        dvol_idx = TONE_DVOL | RING_DVOL | KEY_TONE_DVOL;
        break;
    default:
        return;
    }
    app_audio_bt_volume_save(state);
    /*printf("set_vol[%s]:%s=%d\n", audio_state[__this->state], audio_state[state], volume);*/
}

void app_audio_volume_up(u8 value)
{
    //调音量就取消使用默认值
    app_var.volume_def_state = 0;
    audio_app_volume_up(value);
}

void app_audio_volume_down(u8 value)
{
    //调音量就取消使用默认值
    app_var.volume_def_state = 0;
    audio_app_volume_down(value);
}

s16 app_audio_volume_max_query(audio_vol_index_t index)
{
    if (index < Vol_NULL) {
        return volume_ioc_get_max_level(audio_vol_str[index]);
    } else {
        return volume_ioc_get_max_level(audio_vol_str[Vol_NULL]);
    }
}


/*************************************************************/
//   自定义声音设置的，在这里进行
/*************************************************************/
extern USER_VAR user_var;
//这里改变提示音的音量
void user_set_tone_volume(u8 fade, s16 volume)
{
    y_printf("user_set_tone_volume, volume = %d", volume);
    u8 dvol_idx = 0; //记录音量通道供数字音量控制使用
    app_var.wtone_volume = volume;
    dvol_idx = TONE_DVOL | RING_DVOL | KEY_TONE_DVOL;
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    if (volume > __this->max_volume[APP_AUDIO_STATE_WTONE]) {
        volume = __this->max_volume[APP_AUDIO_STATE_WTONE];
    }
    g_printf("app_var.wtone_volume = %d, volume = %d", app_var.wtone_volume, volume);
    u32 param = dvol_idx << 16 | volume;
    sys_timeout_add((void *)param, app_audio_set_vol_timer_func, 5); //5ms后更新音量
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/
    audio_dac_set_volume(&dac_hdl, volume);
    if (audio_dac_get_status(&dac_hdl)) {
        if (fade) {
            audio_fade_in_fade_out(volume, volume);
        } else {
            audio_dac_set_analog_vol(&dac_hdl, volume);
        }
    }
}

//这里使能mute掉所有声音
void user_voice_mute(u8 en)
{
    y_printf("user_voice_mute, en = %d", en);
    user_var.voice_mute_en = en;
}
/*************************************************************/
