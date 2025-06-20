/*
 ******************************************************************************************
 *							Audio Setup
 *
 * Discription: 音频模块初始化，配置，调试等
 *
 * Notes:
 ******************************************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "sdk_config.h"
#include "asm/audio_adc.h"
#include "media/audio_energy_detect.h"
#include "adc_file.h"
#include "gpio_config.h"
#include "audio_demo/audio_demo.h"
#include "update.h"

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#include "audio_dvol.h"
#endif

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_SMART_VOICE_ENABLE
#include "smart_voice.h"
#endif

struct audio_dac_hdl dac_hdl;
struct audio_adc_hdl adc_hdl;

typedef struct {
    u8 audio_inited;
    atomic_t ref;
} audio_setup_t;
audio_setup_t audio_setup = {0};
#define __this      (&audio_setup)

#if TCFG_MC_BIAS_AUTO_ADJUST
u8 mic_bias_rsel_use_save[AUDIO_ADC_MIC_MAX_NUM] = {0};
u8 save_mic_bias_rsel[AUDIO_ADC_MIC_MAX_NUM]     = {0};
u8 mic_ldo_vsel_use_save = 0;
u8 save_mic_ldo_vsel     = 0;
#endif // #if TCFG_MC_BIAS_AUTO_ADJUST

___interrupt
static void audio_irq_handler()
{
    if (JL_AUDIO->DAC_CON & BIT(7)) {
        JL_AUDIO->DAC_CON |= BIT(6);
        if (JL_AUDIO->DAC_CON & BIT(5)) {
            audio_dac_irq_handler(&dac_hdl);
        }
    }

    if (JL_AUDIO->ADC_CON & BIT(7)) {
        JL_AUDIO->ADC_CON |= BIT(6);
        if ((JL_AUDIO->ADC_CON & BIT(5)) && (JL_AUDIO->ADC_CON & BIT(4))) {
            audio_adc_irq_handler(&adc_hdl);
        }
    }
}


void audio_fade_in_fade_out(u8 left_vol, u8 right_vol);
extern u32 read_capless_DTB(void);
extern struct dac_platform_data dac_data;

int get_dac_channel_num(void)
{
    return dac_hdl.channel;
}

void audio_dac_initcall(void)
{
    int err;
    printf("audio_dac_initcall\n");

#if TCFG_AUDIO_ANC_ENABLE
    dac_data.analog_gain = anc_dac_gain_get(ANC_DAC_CH_L);		//获取ANC设置的模拟增益
#endif

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_init(NULL, 0);
#endif
    audio_dac_init(&dac_hdl, &dac_data);

    u8 mode = TCFG_AUDIO_DAC_DEFAULT_VOL_MODE;
    if (1 != syscfg_read(CFG_VOLUME_ENHANCEMENT_MODE, &mode, 1)) {
        printf("vm no CFG_VOLUME_ENHANCEMENT_MODE !\n");
    }
    /* printf("enter audio_init.c %d,%d\n",mode,__LINE__); */
    app_audio_dac_vol_mode_set(mode);

#if defined(TCFG_AUDIO_DAC_24BIT_MODE) && TCFG_AUDIO_DAC_24BIT_MODE
    audio_dac_set_bit_mode(&dac_hdl, 1);
#endif


    u32 dacr32 = read_capless_DTB();
    audio_dac_set_capless_DTB(&dac_hdl, dacr32);
    audio_dac_set_analog_vol(&dac_hdl, 0);

    request_irq(IRQ_AUDIO_IDX, 2, audio_irq_handler, 0);
    struct audio_dac_trim dac_trim = {0};
    int len = syscfg_read(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    if (len != sizeof(dac_trim)) {
        audio_dac_do_trim(&dac_hdl, &dac_trim, 0);
        syscfg_write(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    }
#if TCFG_SUPPORT_MIC_CAPLESS
    mic_capless_trim_run();
#endif
    audio_dac_set_trim_value(&dac_hdl, &dac_trim);
    audio_dac_set_fade_handler(&dac_hdl, NULL, audio_fade_in_fade_out);

    /*硬件SRC模块滤波器buffer设置，可根据最大使用数量设置整体buffer*/
    /* audio_src_base_filt_init(audio_src_hw_filt, sizeof(audio_src_hw_filt)); */

#if AUDIO_OUTPUT_AUTOMUTE
    mix_out_automute_open();
#endif  //#if AUDIO_OUTPUT_AUTOMUTE
}

static u8 audio_init_complete()
{
    if (!__this->audio_inited) {
        return 0;
    }
    return 1;
}

REGISTER_LP_TARGET(audio_init_lp_target) = {
    .name = "audio_init",
    .is_idle = audio_init_complete,
};

extern void dac_analog_power_control(u8 en);
void audio_fast_mode_test()
{
    audio_dac_set_volume(&dac_hdl, app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    /* dac_analog_power_control(1);////将关闭基带，不开可发现，不可连接 */
    audio_dac_start(&dac_hdl);
    audio_adc_mic_demo_open(AUDIO_ADC_MIC_CH, 10, 16000, 1);

}

struct audio_adc_private_param adc_private_param = {
    .mic_ldo_vsel   = TCFG_AUDIO_MIC_LDO_VSEL,
    .mic_ldo_isel   = TCFG_AUDIO_MIC_LDO_ISEL,
    .adca_reserved0 = 0,
    .adcb_reserved0 = 0,
};

struct adc_file_cfg adc_file_cfg_default = {
    .mic_en_map       = 0b0001,

    .param[0].mic_gain      = 10,
    .param[0].mic_pre_gain  = 1,   // 0:0dB   1:6dB
    .param[0].mic_mode      = AUDIO_MIC_CAP_MODE,
    .param[0].mic_ain_sel   = AUDIO_MIC0_CH0,
    .param[0].mic_bias_sel  = AUDIO_MIC_BIAS_CH0,
    .param[0].mic_bias_rsel = 4,
    .param[0].mic_dcc       = 8,

    .param[1].mic_gain      = 10,
    .param[1].mic_pre_gain  = 1,   // 0:0dB   1:6dB
    .param[1].mic_mode      = AUDIO_MIC_CAP_MODE,
    .param[1].mic_ain_sel   = AUDIO_MIC1_CH0,
    .param[1].mic_bias_sel  = AUDIO_MIC_BIAS_CH1,
    .param[1].mic_bias_rsel = 4,
    .param[1].mic_dcc       = 8,
};

#if TCFG_AUDIO_LINEIN_ENABLE
#include "linein_file.h"
struct linein_file_cfg linein_file_cfg_default = {
    .linein_en_map       = 0b0011,

    .param[0].linein_gain      = 3,
    .param[0].linein_pre_gain  = 0,    // 0:0dB   1:6dB
    .param[0].linein_ain_sel   = AUDIO_LINEIN0_CH0,
    .param[0].linein_dcc       = 14,

    .param[1].linein_gain      = 3,
    .param[1].linein_pre_gain  = 0,    // 0:0dB   1:6dB
    .param[1].linein_ain_sel   = AUDIO_LINEIN1_CH0,
    .param[1].linein_dcc       = 14,
};
#endif

void audio_input_initcall(void)
{
    printf("audio_input_initcall\n");
#if TCFG_MC_BIAS_AUTO_ADJUST
    extern u8 mic_ldo_vsel_use_save;
    extern u8 save_mic_ldo_vsel;
    if (mic_ldo_vsel_use_save) {
        adc_private_param.mic_ldo_vsel = save_mic_ldo_vsel;
    }
#endif

    audio_adc_init(&adc_hdl, &adc_private_param);
    audio_adc_file_init();

#if TCFG_AUDIO_LINEIN_ENABLE
    audio_linein_file_init();
#endif/*TCFG_AUDIO_LINEIN_ENABLE*/
}

/*
 *MIC电源管理
 *设置mic vdd对应port的状态
 */
void audio_mic_pwr_ctl(audio_mic_pwr_t state)
{
    int i;
    struct adc_file_cfg *cfg = audio_adc_file_get_cfg();
    switch (state) {
    case MIC_PWR_OFF:
        if (audio_adc_is_active()) {
            printf("MIC_IO_PWR AUDIO_ADC is useing !\n");
            return;
        }
        printf("MIC_IO_PWR close\n");
    case MIC_PWR_INIT:
        /*mic供电IO配置：输出0*/
        for (i = 0; i < AUDIO_ADC_MIC_MAX_NUM; i++) {
            if (cfg->mic_en_map & BIT(i)) {
                if ((cfg->param[i].mic_bias_sel == 0) && (cfg->param[i].power_io != 0)) {
                    u32 gpio = uuid2gpio(cfg->param[i].power_io);
                    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_OUTPUT_LOW);
                }
            }
        }
        /*
        //测试发现 少数MIC在开机的时候，需要将MIC_IN拉低，否则工作异常
        gpio_set_mode(IO_PORT_SPILT(IO_PORTA_01), PORT_OUTPUT_LOW);	//MIC0
        gpio_set_mode(IO_PORT_SPILT(IO_PORTA_08), PORT_OUTPUT_LOW);	//MIC1
         */
#if (defined TCFG_AUDIO_PLNK_PWR_PORT) && (TCFG_AUDIO_PLNK_PWR_PORT != NO_CONFIG_PORT)
        gpio_set_mode(IO_PORT_SPILT(TCFG_AUDIO_PLNK_PWR_PORT), PORT_OUTPUT_LOW);
#endif
        break;

    case MIC_PWR_ON:
        /*mic供电IO配置：输出1*/
        for (i = 0; i < AUDIO_ADC_MIC_MAX_NUM; i++) {
            if (cfg->mic_en_map & BIT(i)) {
                if ((cfg->param[i].mic_bias_sel == 0) && (cfg->param[i].power_io != 0)) {
                    u32 gpio = uuid2gpio(cfg->param[i].power_io);
                    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_OUTPUT_HIGH);
                }
            }
        }
#if (defined TCFG_AUDIO_PLNK_PWR_PORT) && (TCFG_AUDIO_PLNK_PWR_PORT != NO_CONFIG_PORT)
        gpio_set_mode(IO_PORT_SPILT(TCFG_AUDIO_PLNK_PWR_PORT), PORT_OUTPUT_HIGH);
#endif
        break;

    case MIC_PWR_DOWN:
        break;
    }
}

#ifndef TCFG_AUDIO_DAC_LDO_VOLT_HIGH
#define TCFG_AUDIO_DAC_LDO_VOLT_HIGH 0
#endif

struct dac_platform_data dac_data = {
    .ldo_volt       = TCFG_AUDIO_DAC_LDO_VOLT,                   //DACVDD等级
    .ldo_volt_high  = TCFG_AUDIO_DAC_LDO_VOLT_HIGH,              //音量增强模式 DACVDD等级
    .vcmo_en        = 0,                                         //是否打开VCOMO
    .output         = TCFG_AUDIO_DAC_CONNECT_MODE,               //DAC输出配置，和具体硬件连接有关，需根据硬件来设置
    .ldo_isel       = 0,
    .pa_isel        = 4,
    .ldo_fb_isel    = 0,
    .lpf_isel       = 0x2,
    .dsm_clk        = DAC_DSM_6MHz,
    .analog_gain    = MAX_ANA_VOL,
    .light_close    = 0,
    .power_on_mode  = 0,
    .vcm_cap_en     = 1,                 // VCM引脚是否有电容  0:没有  1:有
    .dma_buf_time_ms    = TCFG_AUDIO_DAC_BUFFER_TIME_MS,
};


static void wl_audio_clk_on(void)
{
    JL_WL_AUD->CON0 = 1;
}

void dac_config_at_sleep()
{
}

static void audio_config_trace(void *priv)
{
    printf(">>Audio_Config_Trace:\n");
    audio_config_dump();
    //audio_adda_dump();
}

/*audio模块初始化*/
static int audio_init()
{
    puts("audio_init\n");
#if ((defined TCFG_AUDIO_DATA_EXPORT_DEFINE) && (TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_UART))
    extern void uartSendInit();
    uartSendInit();
#endif/*TCFG_AUDIO_DATA_EXPORT_DEFINE*/

    wl_audio_clk_on();
    audio_input_initcall();
#if TCFG_AUDIO_ANC_ENABLE
    anc_init();
#endif
    //AC700N DAC初始化在ANC之后，因为需要读取ANC DAC模拟增益
    audio_dac_initcall();

#if TCFG_AUDIO_DUT_ENABLE
    audio_dut_init();
#endif /*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_AUDIO_CONFIG_TRACE
    sys_timer_add(NULL, audio_config_trace, TCFG_AUDIO_CONFIG_TRACE);
#endif/*TCFG_AUDIO_CONFIG_TRACE*/

#if TCFG_SMART_VOICE_ENABLE
    audio_smart_voice_detect_init(NULL);
#endif /* #if TCFG_SMART_VOICE_ENABLE */

    __this->audio_inited = 1;
    return 0;
}
platform_initcall(audio_init);

static void audio_uninit()
{
    dac_power_off();
}
platform_uninitcall(audio_uninit);

/*
 *dac快速校准
 */
//#define DAC_TRIM_FAST_EN
#ifdef DAC_TRIM_FAST_EN
u8 dac_trim_fast_en()
{
    return 1;
}
#endif/*DAC_TRIM_FAST_EN*/

/*
 *自定义dac上电延时时间，具体延时多久应通过示波器测量
 */
#if 1
void dac_power_on_delay()
{
    os_time_dly(50);
}
#endif

void dac_power_on(void)
{
    /* log_info(">>>dac_power_on:%d", __this->ref.counter); */
    if (atomic_inc_return(&__this->ref) == 1) {
        audio_dac_open(&dac_hdl);
    }
}

void dac_power_off(void)
{
    /*log_info(">>>dac_power_off:%d", __this->ref.counter);*/
    if (atomic_read(&__this->ref) != 0 && atomic_dec_return(&__this->ref)) {
        return;
    }
#if 0
    app_audio_mute(AUDIO_MUTE_DEFAULT);
    if (dac_hdl.vol_l || dac_hdl.vol_r) {
        u8 fade_time = dac_hdl.vol_l * 2 / 10 + 1;
        os_time_dly(fade_time); //br30进入低功耗无法使用os_time_dly
        printf("fade_time:%d ms", fade_time);
    }
#endif
    audio_dac_close(&dac_hdl);
}

#if 0
extern struct adc_platform_data adc_data;
int read_mic_type_config(void)
{
    MIC_TYPE_CONFIG mic_type;
    int ret = syscfg_read(CFG_MIC_TYPE_ID, &mic_type, sizeof(MIC_TYPE_CONFIG));
    if (ret > 0) {
        log_info_hexdump((u8 *)&mic_type, sizeof(MIC_TYPE_CONFIG));
        adc_data.mic_mode = mic_type.type;
        adc_data.mic_bias_res = mic_type.pull_up;
        adc_data.mic_ldo_vsel = mic_type.ldo_lev;
        return 0;
    }
    return -1;
}
#endif

/*音频模块寄存器跟踪*/
void audio_adda_dump(void) //打印所有的dac,adc寄存器
{
    printf("JL_WL_AUD CON0:%x", JL_WL_AUD->CON0);
    printf("AUD_CON:%x", JL_AUDIO->AUD_CON);
    printf("DAC_CON:%x", JL_AUDIO->DAC_CON);
    printf("ADC_CON:%x", JL_AUDIO->ADC_CON);
    printf("DAC_VL0:%x", JL_AUDIO->DAC_VL0);
    printf("DAC_TM0:%x", JL_AUDIO->DAC_TM0);
    printf("DAC_DTB:%x", JL_AUDIO->DAC_DTB);
    printf("DAA_CON 0:%x 	1:%x,	2:%x,	3:%x,   4:%x,   7:%x\n", JL_ADDA->DAA_CON0, JL_ADDA->DAA_CON1, JL_ADDA->DAA_CON2, JL_ADDA->DAA_CON3, JL_ADDA->DAA_CON4, JL_ADDA->DAA_CON7);
    printf("ADA_CON 0:%x	1:%x	2:%x,	3:%x,	4:%x,	5:%x\n", JL_ADDA->ADA_CON0, JL_ADDA->ADA_CON1, JL_ADDA->ADA_CON2, JL_ADDA->ADA_CON3, JL_ADDA->ADA_CON4, JL_ADDA->ADA_CON5);
}

/*音频模块配置跟踪*/
void audio_config_dump()
{
    u8 dac_bit_width = ((JL_AUDIO->DAC_CON & BIT(20)) ? 24 : 16);
    u8 adc_bit_width = ((JL_AUDIO->ADC_CON & BIT(20)) ? 24 : 16);
    int dac_dgain_max = 16384;
    int dac_again_max = 15;
    int mic_gain_max = 19;
    u8 dac_dcc = (JL_AUDIO->DAC_CON >> 12) & 0xF;
    u8 mic0_dcc = (JL_AUDIO->ADC_CON >> 12) & 0xF;
    u8 mic1_dcc = JL_AUDIO->ADC_CON1 & 0xF;

    u8 dac_again_l = JL_ADDA->DAA_CON1 & 0xF;
    u8 dac_again_r = (JL_ADDA->DAA_CON1 >> 8) & 0xF;
    u32 dac_dgain_l = JL_AUDIO->DAC_VL0 & 0xFFFF;
    u32 dac_dgain_r = (JL_AUDIO->DAC_VL0 >> 16) & 0xFFFF;
    u8 mic0_0_6 = (JL_ADDA->ADA_CON1 >> 17) & 0x1;
    u8 mic1_0_6 = (JL_ADDA->ADA_CON2 >> 17) & 0x1;
    u8 mic0_gain = (JL_ADDA->ADA_CON2 >> 24) & 0x1F;
    u8 mic1_gain = (JL_ADDA->ADA_CON1 >> 24) & 0x1F;
    short anc_gain = JL_ANC->CON5 & 0xFFFF;

    printf("[ADC]BitWidth:%d,DCC:%d,%d\n", adc_bit_width, mic0_dcc, mic1_dcc);
    printf("[ADC]Gain(Max:%d):%d,%d,6dB_Boost:%d,%d,\n", mic_gain_max, mic0_gain, mic1_gain, mic0_0_6, mic1_0_6);

    printf("[DAC]BitWidth:%d,DCC:%d\n", dac_bit_width, dac_dcc);
    printf("[DAC]AGain(Max:%d):%d,%d,DGain(Max:%d):%d,%d\n", dac_again_max, dac_again_l, dac_again_r, dac_dgain_max, dac_dgain_l, dac_dgain_r);

    printf("[ANC]Gain:%d\n", anc_gain);
}
