/*
 ****************************************************************
 *							AUDIO ANC
 * File  : audio_anc.c
 * By    :
 * Notes : ref_mic = 参考mic
 *		   err_mic = 误差mic
 *
 ****************************************************************
 */
#include "system/includes.h"
#include "audio_anc.h"
#include "system/task.h"
#include "timer.h"
#include "asm/power/p33.h"
#include "online_db_deal.h"
#include "app_tone.h"
#include "asm/audio_adc.h"
#include "audio_anc_coeff.h"
#include "btstack/avctp_user.h"
#include "app_main.h"
#include "audio_manager.h"
#include "audio_config.h"
#include "esco_player.h"
#include "adc_file.h"
#if ANC_MUSIC_DYNAMIC_GAIN_EN
#include "amplitude_statistic.h"
#endif/*ANC_MUSIC_DYNAMIC_GAIN_EN*/
#if RCSP_ADV_EN
#include "adv_anc_voice.h"
#endif
#if BT_AI_SEL_PROTOCOL==LL_SYNC_EN
#include "ble_iot_anc_manager.h"
#endif

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif/*TCFG_USER_TWS_ENABLE*/
#ifdef SUPPORT_MS_EXTENSIONS
#pragma   bss_seg(".anc_user.data.bss")
#pragma  data_seg(".anc_user.data")
#pragma const_seg(".anc_user.text.const")
#pragma  code_seg(".anc_user.text")
#endif/*SUPPORT_MS_EXTENSIONS*/

#if INEAR_ANC_UI
extern u8 inear_tws_ancmode;
#endif/*INEAR_ANC_UI*/

#if TCFG_AUDIO_ANC_ENABLE

#if 1
#define user_anc_log	printf
#else
#define user_anc_log(...)
#endif/*log_en*/


/*************************ANC增益配置******************************/
#define ANC_DAC_GAIN			13			//ANC Speaker增益
#define ANC_REF_MIC_GAIN	 	8			//ANC参考Mic增益
#define ANC_ERR_MIC_GAIN	    8			//ANC误差Mic增益
/*****************************************************************/

static void anc_mix_out_audio_drc_thr(float thr);
static void anc_fade_in_timeout(void *arg);
static void anc_mode_switch_deal(u8 mode);
static void anc_timer_deal(void *priv);
static int anc_app_online_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size);
static anc_coeff_t *anc_cfg_test(u8 coeff_en, u8 gains_en);
static void anc_fade_in_timer_add(audio_anc_t *param);
void anc_dmic_io_init(audio_anc_t *param, u8 en);
u8 anc_btspp_train_again(u8 mode, u32 dat);
static void anc_tone_play_and_mode_switch(u8 mode, u8 preemption, u8 cb_sel);
void anc_mode_enable_set(u8 mode_enable);
void audio_anc_post_msg_drc(void);
void audio_anc_post_msg_debug(void);
void audio_anc_music_dynamic_gain_process(void);
void audio_anc_mic_management(audio_anc_t *param);
#define ESCO_MIC_DUMP_CNT	10
extern void esco_mic_dump_set(u16 dump_cnt);

typedef struct {
    u8 state;		/*ANC状态*/
    u8 mode_num;	/*模式总数*/
    u8 mode_enable;	/*使能的模式*/
    u8 anc_parse_seq;
    u8 suspend;		/*挂起*/
    u8 ui_mode_sel; /*ui菜单降噪模式选择*/
    u8 new_mode;	/*记录模式切换最新的目标模式*/
    u8 last_mode;	/*记录模式切换的上一个模式*/
    u8 tone_mode;	/*记录当前提示音播放的模式*/
    u8 dy_mic_ch;			/*动态MIC增益类型*/
    char mic_diff_gain;		/*动态MIC增益差值*/
    u16 ui_mode_sel_timer;
    u16 fade_in_timer;
    u16 fade_gain;
    u16 mic_resume_timer;	/*动态MIC增益恢复定时器id*/
    float drc_ratio;		/*动态MIC增益，对应DRC增益比例*/
    volatile u8 mode_switch_lock;/*且模式锁存，1表示正在切ANC模式*/
    volatile u8 sync_busy;
    audio_anc_t param;

#if ANC_MUSIC_DYNAMIC_GAIN_EN
    char loud_dB;					/*音乐动态增益-当前音乐幅值*/
    s16 loud_nowgain;				/*音乐动态增益-当前增益*/
    s16 loud_targetgain;			/*音乐动态增益-目标增益*/
    u16 loud_timeid;				/*音乐动态增益-定时器ID*/
    LOUDNESS_M_STRUCT loud_hdl;		/*音乐动态增益-操作句柄*/
#endif/*ANC_MUSIC_DYNAMIC_GAIN_EN*/

} anc_t;
static anc_t *anc_hdl = NULL;

__attribute__((weak))
void audio_anc_event_to_user(int mode)
{

}

static void anc_task(void *p)
{
    int res;
    int anc_ret = 0;
    int msg[16];
    u8 cur_anc_mode;
    u32 pend_timeout = portMAX_DELAY;
    user_anc_log(">>>ANC TASK<<<\n");
    while (1) {
        res = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (res == OS_TASKQ) {
            switch (msg[1]) {
            case ANC_MSG_TRAIN_OPEN:/*启动训练模式*/
                audio_mic_pwr_ctl(MIC_PWR_ON);
                anc_dmic_io_init(&anc_hdl->param, 1);
                user_anc_log("ANC_MSG_TRAIN_OPEN");
                audio_anc_train(&anc_hdl->param, 1);
                os_time_dly(1);
                anc_train_close();
                audio_mic_pwr_ctl(MIC_PWR_OFF);
                break;
            case ANC_MSG_TRAIN_CLOSE:/*训练关闭模式*/
                user_anc_log("ANC_MSG_TRAIN_CLOSE");
                anc_train_close();
                break;
            case ANC_MSG_RUN:
                cur_anc_mode = anc_hdl->param.mode;/*保存当前anc模式，防止切换过程anc_hdl->param.mode发生切换改变*/
                audio_anc_event_to_user(cur_anc_mode);
                user_anc_log("ANC_MSG_RUN:%s \n", anc_mode_str[cur_anc_mode]);
#if (RCSP_ADV_EN && RCSP_ADV_ANC_VOICE)
                /*APP增益设置*/
                anc_hdl->param.anc_fade_gain = rcsp_adv_anc_voice_value_get(cur_anc_mode);
#endif/*RCSP_ADV_EN && RCSP_ADV_ANC_VOICE*/
#if BT_AI_SEL_PROTOCOL==LL_SYNC_EN
                /*APP增益设置*/
                anc_hdl->param.anc_fade_gain = iot_anc_effect_value_get(cur_anc_mode);
#endif
#if ANC_MUSIC_DYNAMIC_GAIN_EN
                audio_anc_music_dynamic_gain_reset(0);	/*音乐动态增益，切模式复位状态*/
#endif/*ANC_MUSIC_DYNAMIC_GAIN_EN*/
#if (TCFG_AUDIO_ADC_MIC_CHA == LADC_CH_PLNK)
                /* esco_mic_dump_set(ESCO_MIC_DUMP_CNT); */
#endif/*LADC_CH_PLNK*/
                if (anc_hdl->state == ANC_STA_INIT) {
                    audio_mic_pwr_ctl(MIC_PWR_ON);
                    anc_dmic_io_init(&anc_hdl->param, 1);
#if ANC_MODE_SYSVDD_EN
                    clk_set_lowest_voltage(SYSVDD_VOL_SEL_105V);	//进入ANC时提高SYSVDD电压
#endif/*ANC_MODE_SYSVDD_EN*/
                    anc_ret = audio_anc_run(&anc_hdl->param);
                    if (anc_ret == 0) {
                        anc_hdl->state = ANC_STA_OPEN;
                    } else {
                        /*
                         *-EPERM(-1):不支持ANC(非差分MIC，或混合馈使用4M的flash)
                         *-EINVAL(-22):参数错误(anc_coeff.bin参数为空, 或不匹配)
                         */
                        user_anc_log("audio_anc open Failed:%d\n", anc_ret);
                        anc_hdl->mode_switch_lock = 0;
                        anc_hdl->state = ANC_STA_INIT;
                        audio_mic_pwr_ctl(MIC_PWR_OFF);
                        break;
                    }
                } else {
                    audio_anc_run(&anc_hdl->param);
                    if (cur_anc_mode == ANC_OFF) {
                        anc_hdl->state = ANC_STA_INIT;
                        anc_dmic_io_init(&anc_hdl->param, 0);
                        audio_mic_pwr_ctl(MIC_PWR_OFF);
                    }
                }
                if (cur_anc_mode == ANC_OFF) {
#if ANC_MODE_SYSVDD_EN
                    clk_set_lowest_voltage(SYSVDD_VOL_SEL_084V);	//退出ANC恢复普通模式
#endif/*ANC_MODE_SYSVDD_EN*/
                    /*anc关闭，如果没有连接蓝牙，倒计时进入自动关机*/
                    extern u8 bt_get_total_connect_dev(void);
                    if (bt_get_total_connect_dev() == 0) {    //已经没有设备连接
                        sys_auto_shut_down_enable();
                    }
                }
#if TCFG_AUDIO_DYNAMIC_ADC_GAIN
                if (esco_player_runing() && (anc_hdl->state == ANC_STA_OPEN)) {
                    if (anc_hdl->dy_mic_ch != MIC_NULL) {
                        audio_anc_drc_ctrl(&anc_hdl->param, anc_hdl->drc_ratio);
                    } else if (anc_hdl->drc_ratio != 1.0f) {	//开机之后先通话,再切ANC时
#if (!TCFG_AUDIO_DUAL_MIC_ENABLE && (TCFG_AUDIO_ADC_MIC_CHA & AUDIO_ADC_MIC_1)) || \
	(TCFG_AUDIO_DUAL_MIC_ENABLE && (TCFG_AUDIO_DMS_MIC_MANAGE == DMS_MASTER_MIC0))
                        u8 aec_mic_gain = audio_adc_file_get_gain(1);
#else
                        u8 aec_mic_gain = audio_adc_file_get_gain(0);
#endif/*TCFG_AUDIO_DUAL_MIC_ENABLE*/
                        anc_dynamic_micgain_start(aec_mic_gain);
                    }
                }
#endif/*TCFG_AUDIO_DYNAMIC_ADC_GAIN*/
                if (cur_anc_mode == ANC_ON && (anc_hdl->param.lfb_en && anc_hdl->param.rfb_en)) {
                    os_time_dly(40);	//针对头戴式FB，延时过滤哒哒声
                } else if (cur_anc_mode != ANC_OFF) {
                    os_time_dly(6);		//延时过滤哒哒声
                }
                anc_hdl->mode_switch_lock = 0;
                anc_fade_in_timer_add(&anc_hdl->param);
                anc_hdl->last_mode = cur_anc_mode;
                break;
            case ANC_MSG_MODE_SYNC:
                user_anc_log("anc_mode_sync:%d", msg[2]);
                anc_mode_switch(msg[2], 0);
                break;
#if TCFG_USER_TWS_ENABLE
            case ANC_MSG_TONE_SYNC:
                user_anc_log("anc_tone_sync_play:%d", msg[2]);
                /* if (anc_hdl->mode_switch_lock) { */
                /* user_anc_log("anc mode switch lock\n"); */
                /* break; */
                /* } */
                /* anc_hdl->mode_switch_lock = 1; */
                if (msg[2] == SYNC_TONE_ANC_OFF) {
                    anc_hdl->param.mode = ANC_OFF;
                } else if (msg[2] == SYNC_TONE_ANC_ON) {
                    anc_hdl->param.mode = ANC_ON;
                } else {
                    anc_hdl->param.mode = ANC_TRANSPARENCY;
                }
                if (anc_hdl->suspend) {
                    anc_hdl->param.tool_enablebit = 0;
                }
                anc_hdl->new_mode = anc_hdl->param.mode;	//确保准备切的新模式，与最后一个提示音播放的模式一致
                anc_tone_play_and_mode_switch(anc_hdl->param.mode, ANC_TONE_PREEMPTION, ANC_TONE_END_MODE_SW);
                //anc_mode_switch_deal(anc_hdl->param.mode);
                anc_hdl->sync_busy = 0;
                break;
#endif/*TCFG_USER_TWS_ENABLE*/
            case ANC_MSG_FADE_END:
                break;
            case ANC_MSG_DRC_TIMER:
                audio_anc_drc_process();
                break;
            case ANC_MSG_DEBUG_OUTPUT:
                audio_anc_debug_data_output();
                break;
#if ANC_MUSIC_DYNAMIC_GAIN_EN
            case ANC_MSG_MUSIC_DYN_GAIN:
                audio_anc_music_dynamic_gain_process();
                break;
#endif/*ANC_MUSIC_DYNAMIC_GAIN_EN*/
            }
        } else {
            user_anc_log("res:%d,%d", res, msg[1]);
        }
    }
}

void anc_param_gain_t_printf(u8 cmd)
{
    char *str[2] = {
        "ANC_CFG_READ",
        "ANC_CFG_WRITE"
    };
    anc_gain_param_t *gains = &anc_hdl->param.gains;
    user_anc_log("-------------anc_param anc_gain_t-%s--------------\n", str[cmd - 1]);
    user_anc_log("dac_gain %d\n", gains->dac_gain);
    user_anc_log("ref_mic_gain %d\n", gains->ref_mic_gain);
    user_anc_log("err_mic_gain %d\n", gains->err_mic_gain);
    user_anc_log("cmp_en %d\n",		  gains->cmp_en);
    user_anc_log("drc_en %d\n",  	  gains->drc_en);
    user_anc_log("ahs_en %d\n",		  gains->ahs_en);
    user_anc_log("dcc_sel %d\n",	  gains->dcc_sel);
    user_anc_log("gain_sign 0X%x\n",  gains->gain_sign);
    user_anc_log("noise_lvl 0X%x\n",  gains->noise_lvl);
    user_anc_log("fade_step 0X%x\n",  gains->fade_step);
    user_anc_log("alogm %d\n",		  gains->alogm);
    user_anc_log("trans_alogm %d\n",  gains->trans_alogm);

    user_anc_log("ancl_ffgain %d.%d\n", ((int)(gains->l_ffgain * 100.0)) / 100, ((int)(gains->l_ffgain * 100.0)) % 100);
    user_anc_log("ancl_fbgain %d.%d\n", ((int)(gains->l_fbgain * 100.0)) / 100, ((int)(gains->l_fbgain * 100.0)) % 100);
    user_anc_log("ancl_trans_gain %d.%d\n", ((int)(gains->l_transgain * 100.0)) / 100, ((int)(gains->l_transgain * 100.0)) % 100);
    user_anc_log("ancl_cmpgain %d.%d\n", ((int)(gains->l_cmpgain * 100.0)) / 100, ((int)(gains->l_cmpgain * 100.0)) % 100);
    user_anc_log("ancr_ffgain %d.%d\n", ((int)(gains->r_ffgain * 100.0)) / 100, ((int)(gains->r_ffgain * 100.0)) % 100);
    user_anc_log("ancr_fbgain %d.%d\n", ((int)(gains->r_fbgain * 100.0)) / 100, ((int)(gains->r_fbgain * 100.0)) % 100);
    user_anc_log("ancr_trans_gain %d.%d\n", ((int)(gains->r_transgain * 100.0)) / 100, ((int)(gains->r_transgain * 100.0)) % 100);
    user_anc_log("ancr_cmpgain %d.%d\n", ((int)(gains->r_cmpgain * 100.0)) / 100, ((int)(gains->r_cmpgain * 100.0)) % 100);

    user_anc_log("drcff_zero_det %d\n",    gains->drcff_zero_det);
    user_anc_log("drcff_dat_mode %d\n",    gains->drcff_dat_mode);
    user_anc_log("drcff_lpf_sel %d\n",     gains->drcff_lpf_sel);
    user_anc_log("drcfb_zero_det %d\n",    gains->drcfb_zero_det);
    user_anc_log("drcfb_dat_mode %d\n",    gains->drcfb_dat_mode);
    user_anc_log("drcfb_lpf_sel %d\n",     gains->drcfb_lpf_sel);

    user_anc_log("drcff_low_thr %d\n",     gains->drcff_lthr);
    user_anc_log("drcff_high_thr %d\n",    gains->drcff_hthr);
    user_anc_log("drcff_low_gain %d\n",    gains->drcff_lgain);
    user_anc_log("drcff_high_gain %d\n",   gains->drcff_hgain);
    user_anc_log("drcff_nor_gain %d\n",    gains->drcff_norgain);

    user_anc_log("drcfb_low_thr %d\n",     gains->drcfb_lthr);
    user_anc_log("drcfb_high_thr %d\n",    gains->drcfb_hthr);
    user_anc_log("drcfb_low_gain %d\n",    gains->drcfb_lgain);
    user_anc_log("drcfb_high_gain %d\n",   gains->drcfb_hgain);
    user_anc_log("drcfb_nor_gain %d\n",    gains->drcfb_norgain);

    user_anc_log("drctrans_low_thr %d\n",     gains->drctrans_lthr);
    user_anc_log("drctrans_high_thr %d\n",    gains->drctrans_hthr);
    user_anc_log("drctrans_low_gain %d\n",    gains->drctrans_lgain);
    user_anc_log("drctrans_high_gain %d\n",   gains->drctrans_hgain);
    user_anc_log("drctrans_nor_gain %d\n",    gains->drctrans_norgain);

    user_anc_log("ahs_dly %d\n",     gains->ahs_dly);
    user_anc_log("ahs_tap %d\n",     gains->ahs_tap);
    user_anc_log("ahs_wn_shift %d\n", gains->ahs_wn_shift);
    user_anc_log("ahs_wn_sub %d\n",  gains->ahs_wn_sub);
    user_anc_log("ahs_shift %d\n",   gains->ahs_shift);
    user_anc_log("ahs_u %d\n",       gains->ahs_u);
    user_anc_log("ahs_gain %d\n",    gains->ahs_gain);

    user_anc_log("audio_drc_thr %d/10\n", (int)(gains->audio_drc_thr * 10.0));
}

/*ANC配置参数填充*/
void anc_param_fill(u8 cmd, anc_gain_t *cfg)
{
    if (cmd == ANC_CFG_READ) {
        cfg->gains = anc_hdl->param.gains;
    } else if (cmd == ANC_CFG_WRITE) {
        anc_hdl->param.gains = cfg->gains;
        //默认DRC设置为初始值
        anc_hdl->param.gains.drcff_zero_det = 0;
        anc_hdl->param.gains.drcff_dat_mode = 0;//hbit control l_gain, lbit control h_gain
        anc_hdl->param.gains.drcff_lpf_sel  = 0;
        anc_hdl->param.gains.drcfb_zero_det = 0;
        anc_hdl->param.gains.drcfb_dat_mode = 0;//hbit control l_gain, lbit control h_gain
        anc_hdl->param.gains.drcfb_lpf_sel  = 0;
        anc_hdl->param.gains.developer_mode = ANC_DEVELOPER_MODE_EN;
        audio_anc_param_normalize(&anc_hdl->param);
    }
    /* anc_param_gain_t_printf(cmd); */
}


/*ANC初始化*/
void anc_init(void)
{
    anc_hdl = zalloc(sizeof(anc_t));
    user_anc_log("anc_hdl size:%lu\n", sizeof(anc_t));
    ASSERT(anc_hdl);
    anc_debug_init(&anc_hdl->param);
    audio_anc_param_init(&anc_hdl->param);
#if TCFG_ANC_TOOL_DEBUG_ONLINE
    app_online_db_register_handle(DB_PKT_TYPE_ANC, anc_app_online_parse);
#endif/*TCFG_ANC_TOOL_DEBUG_ONLINE*/
    anc_hdl->param.post_msg_drc = audio_anc_post_msg_drc;
    anc_hdl->param.post_msg_debug = audio_anc_post_msg_debug;
#if TCFG_ANC_MODE_ANC_EN
    anc_hdl->mode_enable |= ANC_ON_BIT;
#endif/*TCFG_ANC_MODE_ANC_EN*/
#if TCFG_ANC_MODE_TRANS_EN
    anc_hdl->mode_enable |= ANC_TRANS_BIT;
#endif/*TCFG_ANC_MODE_TRANS_EN*/
#if TCFG_ANC_MODE_OFF_EN
    anc_hdl->mode_enable |= ANC_OFF_BIT;
#endif/*TCFG_ANC_MODE_OFF_EN*/
    anc_mode_enable_set(anc_hdl->mode_enable);
    anc_hdl->param.cfg_online_deal_cb = anc_cfg_online_deal;
    anc_hdl->param.mode = ANC_OFF;
    anc_hdl->param.production_mode = 0;
    anc_hdl->param.anc_fade_en = ANC_FADE_EN;/*ANC淡入淡出，默认开*/
    anc_hdl->param.anc_fade_gain = 16384;/*ANC淡入淡出增益,16384 (0dB) max:32767*/
    anc_hdl->param.tool_enablebit = TCFG_AUDIO_ANC_TRAIN_MODE;
    anc_hdl->param.online_busy = 0;		//在线调试繁忙标志位
    //36
    anc_hdl->param.dut_audio_enablebit = ANC_DUT_CLOSE;
    anc_hdl->param.fade_time_lvl = ANC_MODE_FADE_LVL;
    anc_hdl->param.enablebit = TCFG_AUDIO_ANC_TRAIN_MODE;
    anc_hdl->param.ch = TCFG_AUDIO_ANC_CH;
    anc_hdl->param.ff_yorder = TCFG_AUDIO_ANC_MAX_ORDER;
    anc_hdl->param.trans_yorder = TCFG_AUDIO_ANC_MAX_ORDER;
    anc_hdl->param.fb_yorder = TCFG_AUDIO_ANC_MAX_ORDER;
    anc_hdl->param.mic_type[0] = (ANC_CONFIG_LFF_EN) ? TCFG_AUDIO_ANCL_FF_MIC : MIC_NULL;
    anc_hdl->param.mic_type[1] = (ANC_CONFIG_LFB_EN) ? TCFG_AUDIO_ANCL_FB_MIC : MIC_NULL;
    anc_hdl->param.mic_type[2] = (ANC_CONFIG_RFF_EN) ? TCFG_AUDIO_ANCR_FF_MIC : MIC_NULL;
    anc_hdl->param.mic_type[3] = (ANC_CONFIG_RFB_EN) ? TCFG_AUDIO_ANCR_FB_MIC : MIC_NULL;
    anc_hdl->param.lff_en = ANC_CONFIG_LFF_EN;
    anc_hdl->param.lfb_en = ANC_CONFIG_LFB_EN;
    anc_hdl->param.rff_en = ANC_CONFIG_RFF_EN;
    anc_hdl->param.rfb_en = ANC_CONFIG_RFB_EN;
    anc_hdl->param.debug_sel = 0;
    anc_hdl->param.gains.version = ANC_GAINS_VERSION;
    anc_hdl->param.gains.dac_gain = ANC_DAC_GAIN;
    anc_hdl->param.gains.ref_mic_gain = ANC_REF_MIC_GAIN;
    anc_hdl->param.gains.err_mic_gain = ANC_ERR_MIC_GAIN;
    anc_hdl->param.gains.cmp_en = 1;
    anc_hdl->param.gains.drc_en = 0;
    anc_hdl->param.gains.ahs_en = 1;
    anc_hdl->param.gains.dcc_sel = 4;
    anc_hdl->param.gains.gain_sign = 0;
    anc_hdl->param.gains.alogm = ANC_ALOGM6;
    anc_hdl->param.gains.trans_alogm = ANC_ALOGM4;
    anc_hdl->param.gains.l_ffgain = 1.0f;
    anc_hdl->param.gains.l_fbgain = 1.0f;
    anc_hdl->param.gains.l_cmpgain = 1.0f;
    anc_hdl->param.gains.l_transgain = 1.0f;
    anc_hdl->param.gains.r_ffgain = 1.0f;
    anc_hdl->param.gains.r_fbgain = 1.0f;
    anc_hdl->param.gains.r_cmpgain = 1.0f;
    anc_hdl->param.gains.r_transgain = 1.0f;

    anc_hdl->param.gains.drcff_zero_det = 0;
    anc_hdl->param.gains.drcff_dat_mode = 0;//hbit control l_gain, lbit control h_gain
    anc_hdl->param.gains.drcff_lpf_sel  = 0;
    anc_hdl->param.gains.drcfb_zero_det = 0;
    anc_hdl->param.gains.drcfb_dat_mode = 0;//hbit control l_gain, lbit control h_gain
    anc_hdl->param.gains.drcfb_lpf_sel  = 0;

    anc_hdl->param.gains.drcff_lthr = 0;
    anc_hdl->param.gains.drcff_hthr = 6000 ;
    anc_hdl->param.gains.drcff_lgain = 0;
    anc_hdl->param.gains.drcff_hgain = 512;
    anc_hdl->param.gains.drcff_norgain = 1024 ;

    anc_hdl->param.gains.drctrans_lthr = 0;
    anc_hdl->param.gains.drctrans_hthr = 32767 ;
    anc_hdl->param.gains.drctrans_lgain = 0;
    anc_hdl->param.gains.drctrans_hgain = 1024;
    anc_hdl->param.gains.drctrans_norgain = 1024 ;

    anc_hdl->param.gains.drcfb_lthr = 0;
    anc_hdl->param.gains.drcfb_hthr = 6000;
    anc_hdl->param.gains.drcfb_lgain = 0;
    anc_hdl->param.gains.drcfb_hgain = 512;
    anc_hdl->param.gains.drcfb_norgain = 1024;

    anc_hdl->param.gains.ahs_dly = 1;
    anc_hdl->param.gains.ahs_tap = 100;
    anc_hdl->param.gains.ahs_wn_shift = 9;
    anc_hdl->param.gains.ahs_wn_sub = 1;
    anc_hdl->param.gains.ahs_shift = 210;
    anc_hdl->param.gains.ahs_u = 4000;
    anc_hdl->param.gains.ahs_gain = -1024;
    anc_hdl->param.gains.audio_drc_thr = -6.0f;

#if ANC_COEFF_SAVE_ENABLE
    anc_db_init();
    /*读取ANC增益配置*/
    anc_gain_t *db_gain = (anc_gain_t *)anc_db_get(ANC_DB_GAIN, &anc_hdl->param.gains_size);
    if (db_gain) {
        user_anc_log("anc_gain_db get succ,len:%d\n", anc_hdl->param.gains_size);
        if (audio_anc_gains_version_verify(&anc_hdl->param, db_gain)) {
            db_gain = (anc_gain_t *)anc_db_get(ANC_DB_GAIN, &anc_hdl->param.gains_size);
        }
        anc_param_fill(ANC_CFG_WRITE, db_gain);
    } else {
        user_anc_log("anc_gain_db get failed");
        anc_param_gain_t_printf(ANC_CFG_READ);
    }
    /*读取ANC滤波器系数*/
    anc_coeff_t *db_coeff = (anc_coeff_t *)anc_db_get(ANC_DB_COEFF, &anc_hdl->param.coeff_size);
    if (!anc_coeff_fill(db_coeff)) {
        user_anc_log("anc_coeff_db get succ,len:%d", anc_hdl->param.coeff_size);
    } else {
        user_anc_log("anc_coeff_db get failed");
#if 0	//测试数据写入是否正常，或写入自己想要的滤波器数据
        extern anc_coeff_t *anc_cfg_test(audio_anc_t *param, u8 coeff_en, u8 gains_en);
        anc_coeff_t *test_coeff = anc_cfg_test(&anc_hdl->param, 1, 1);
        if (test_coeff) {
            int ret = anc_db_put(&anc_hdl->param, NULL, test_coeff);
            if (ret == 0) {
                user_anc_log("anc_db put test succ\n");
            } else {
                user_anc_log("anc_db put test err\n");
            }
        }
        anc_coeff_t *test_coeff1 = (anc_coeff_t *)anc_db_get(ANC_DB_COEFF, &anc_hdl->param.coeff_size);
        anc_coeff_fill(test_coeff1);
#endif
    }
#endif/*ANC_COEFF_SAVE_ENABLE*/

    /* sys_timer_add(NULL, anc_timer_deal, 5000); */
    audio_anc_mic_management(&anc_hdl->param);

    audio_anc_max_yorder_verify(&anc_hdl->param, TCFG_AUDIO_ANC_MAX_ORDER);
    anc_mix_out_audio_drc_thr(anc_hdl->param.gains.audio_drc_thr);
    task_create(anc_task, NULL, "anc");
    anc_hdl->state = ANC_STA_INIT;
    anc_hdl->dy_mic_ch = MIC_NULL;

    user_anc_log("anc_init ok");
}

void anc_train_open(u8 mode, u8 debug_sel)
{
    user_anc_log("ANC_Train_Open\n");
    local_irq_disable();
    if (anc_hdl && (anc_hdl->state == ANC_STA_INIT)) {
        /*防止重复打开训练模式*/
        if (anc_hdl->param.mode == ANC_TRAIN) {
            local_irq_enable();
            return;
        }
        /*anc工作，退出sniff*/
        bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
        /*anc工作，关闭自动关机*/
        sys_auto_shut_down_disable();
        audio_mic_pwr_ctl(MIC_PWR_ON);

        anc_hdl->param.debug_sel = debug_sel;
        anc_hdl->param.mode = mode;
        /*训练的时候，申请buf用来保存训练参数*/
        local_irq_enable();
        os_taskq_post_msg("anc", 1, ANC_MSG_TRAIN_OPEN);
        user_anc_log("%s ok\n", __FUNCTION__);
        return;
    }
    local_irq_enable();
}

void anc_train_close(void)
{
    int ret = 0;
    if (anc_hdl == NULL) {
        return;
    }
    if (anc_hdl && (anc_hdl->param.mode == ANC_TRAIN)) {
        anc_hdl->param.train_para.train_busy = 0;
        anc_hdl->param.mode = ANC_OFF;
        anc_hdl->state = ANC_STA_INIT;
        audio_anc_train(&anc_hdl->param, 0);
#if TCFG_USER_TWS_ENABLE
        /*训练完毕，tws正常配置连接*/
        bt_tws_poweron();
#endif/*TCFG_USER_TWS_ENABLE*/
        user_anc_log("anc_train_close ok\n");
    }
}

/*查询当前ANC是否处于训练状态*/
int anc_train_open_query(void)
{
    if (anc_hdl) {
        if (anc_hdl->param.mode == ANC_TRAIN) {
            return 1;
        }
    }
    return 0;
}

extern void audio_dump();
static void anc_timer_deal(void *priv)
{
    u8 dac_again_l = JL_ADDA->DAA_CON1 & 0xF;
    u8 dac_again_r = (JL_ADDA->DAA_CON1 >> 8) & 0xF;
    u32 dac_dgain_l = JL_AUDIO->DAC_VL0 & 0xFFFF;
    u32 dac_dgain_r = (JL_AUDIO->DAC_VL0 >> 16) & 0xFFFF;
    u8 mic0_gain = (JL_ADDA->ADA_CON2 >> 24) & 0x1F;
    u8 mic1_gain = (JL_ADDA->ADA_CON1 >> 24) & 0x1F;
    u32 anc_drcffgain = (JL_ANC->CON16 >> 16) & 0xFFFF;
    u32 anc_drcfbgain = (JL_ANC->CON19 >> 16) & 0xFFFF;
    user_anc_log("MIC_G:%d,%d,DAC_AG:%d,%d,DAC_DG:%d,%d,ANC_NFFG:%d ,ANC_NFBG :%d\n", mic0_gain, mic1_gain, dac_again_l, dac_again_r, dac_dgain_l, dac_dgain_r, (short)anc_drcffgain, (short)anc_drcfbgain);
    /* mem_stats(); */
    /* audio_dump(); */
}

static int anc_tone_play_cb(void *priv, enum stream_event event)
{
    u8 next_mode = anc_hdl->tone_mode;

    if (event != STREAM_EVENT_STOP) {
        return 0;
    }
    printf("anc_tone_play_cb,anc_mode:%d,%d,new_mode:%d\n", next_mode, anc_hdl->param.mode, anc_hdl->new_mode);
    /*
     *当最新的目标ANC模式和即将切过去的模式一致，才进行模式切换实现。
     *否则表示，模式切换操作（比如快速按键切模式）new_mode领先于ANC模式切换实现next_mode
     *则直接在切模式的时候，实现最新的目标模式
     */
    if (anc_hdl->param.mode == next_mode) {
        anc_mode_switch_deal(next_mode);
    } else {
        anc_hdl->mode_switch_lock = 0;
    }
    return 0;
}

/*
*********************************************************************
*                  Audio ANC Tone Play And Mode Switch
* Description: ANC提示音播放和模式切换
* Arguments  : mode 	    当前模式
*			   preemption	提示音抢断播放标识
*			   cb_sel	 	切模式方式选择
* Return	 : None.
* Note(s)    : 通过cb_sel选择是在播放提示音切模式，还是播提示音同时切
*			   模式
*********************************************************************
*/
static void anc_tone_play_and_mode_switch(u8 mode, u8 preemption, u8 cb_sel)
{
    if (cb_sel) {
        /*ANC打开情况下，播提示音的同时，anc效果淡出*/
        if (anc_hdl->state == ANC_STA_OPEN) {
            audio_anc_fade(0, anc_hdl->param.anc_fade_en, 1);
        }
        if (mode == ANC_ON) {
            play_tone_file_callback(get_tone_files()->anc_on, NULL, anc_tone_play_cb);
        } else if (mode == ANC_TRANSPARENCY) {
            play_tone_file_callback(get_tone_files()->anc_trans, NULL, anc_tone_play_cb);
        } else if (mode == ANC_OFF) {
            play_tone_file_callback(get_tone_files()->anc_off, NULL, anc_tone_play_cb);
        }
        anc_hdl->tone_mode = anc_hdl->param.mode;
    } else {
        /* tone_play_index(index, preemption); */
        if (mode == ANC_ON) {
            play_tone_file(get_tone_files()->anc_on);
        } else if (mode == ANC_TRANSPARENCY) {
            play_tone_file(get_tone_files()->anc_trans);
        } else if (mode == ANC_OFF) {
            play_tone_file(get_tone_files()->anc_off);
        }
        anc_mode_switch_deal(anc_hdl->param.mode);
    }
}

static void anc_tone_stop(void)
{
    if (anc_hdl && anc_hdl->mode_switch_lock) {
        tone_player_stop();
    }
}

/*
*********************************************************************
*                  anc_fade_in_timer
* Description: ANC增益淡入函数
* Arguments  : None.
* Return	 : None.
* Note(s)    :通过定时器的控制，使ANC的淡入增益一点一点叠加
*********************************************************************
*/
static void anc_fade_in_timer(void *arg)
{
    anc_hdl->fade_gain += (anc_hdl->param.anc_fade_gain / anc_hdl->param.fade_time_lvl);
    if (anc_hdl->fade_gain > anc_hdl->param.anc_fade_gain) {
        anc_hdl->fade_gain = anc_hdl->param.anc_fade_gain;
        usr_timer_del(anc_hdl->fade_in_timer);
    }
    audio_anc_fade(anc_hdl->fade_gain, anc_hdl->param.anc_fade_en, 1);
}

static void anc_fade_in_timer_add(audio_anc_t *param)
{
    u32 alogm, dly;
    if (param->anc_fade_en) {
        anc_hdl->param.fade_lock = 0;	//解锁淡入淡出增益
        alogm = (param->mode == ANC_TRANSPARENCY) ? param->gains.trans_alogm : param->gains.alogm;
        dly = audio_anc_fade_dly_get(param->anc_fade_gain, alogm);
        anc_hdl->fade_gain = 0;
        if (param->mode == ANC_ON) {		//可自定义模式，当前仅在ANC模式下使用
            user_anc_log("anc_fade_time_lvl  %d\n", param->fade_time_lvl);
            anc_hdl->fade_gain = (param->anc_fade_gain / param->fade_time_lvl);
            user_anc_log("gain:%d->%d\n", (JL_ANC->CON7 >> 9) & 0xFFFF, anc_hdl->fade_gain);
            audio_anc_fade(anc_hdl->fade_gain, param->anc_fade_en, 1);
            if (param->fade_time_lvl > 1) {
                anc_hdl->fade_in_timer = usr_timer_add((void *)0, anc_fade_in_timer, dly, 1);
            }
        } else if (param->mode != ANC_OFF) {
            audio_anc_fade(param->anc_fade_gain, param->anc_fade_en, 1);
        }
    }
}
static void anc_fade_out_timeout(void *arg)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_RUN);
}

static void anc_fade(u32 gain)
{
    u32	alogm, dly;
    if (anc_hdl) {
        alogm = (anc_hdl->last_mode == ANC_TRANSPARENCY) ? anc_hdl->param.gains.trans_alogm : anc_hdl->param.gains.alogm;
        dly = audio_anc_fade_dly_get(anc_hdl->param.anc_fade_gain, alogm);
        user_anc_log("anc_fade:%d,dly:%d", gain, dly);
        audio_anc_fade(gain, anc_hdl->param.anc_fade_en, 1);
        if (anc_hdl->param.anc_fade_en) {
            usr_timeout_del(anc_hdl->fade_in_timer);
            usr_timeout_add((void *)0, anc_fade_out_timeout, dly, 1);
        } else {/*不淡入淡出，则直接切模式*/
            os_taskq_post_msg("anc", 1, ANC_MSG_RUN);
        }
    }
}

/*ANC淡入淡出增益设置接口*/
void audio_anc_fade_gain_set(int gain)
{
    anc_hdl->param.anc_fade_gain = gain;
    if (anc_hdl->param.mode != ANC_OFF) {
        audio_anc_fade(gain, anc_hdl->param.anc_fade_en, 1);
    }
}
/*ANC淡入淡出增益接口*/
int audio_anc_fade_gain_get(void)
{
    return anc_hdl->param.anc_fade_gain;
}

/*
 *mode:降噪/通透/关闭
 *tone_play:切换模式的时候，是否播放提示音
 */
static void anc_mode_switch_deal(u8 mode)
{
    user_anc_log("anc switch,state:%s", anc_state_str[anc_hdl->state]);
    /* anc_hdl->mode_switch_lock = 1; */
    if (anc_hdl->state == ANC_STA_OPEN) {
        user_anc_log("anc open now,switch mode:%d", mode);
        anc_fade(0);//切模式，先fade_out
    } else if (anc_hdl->state == ANC_STA_INIT) {
        if (anc_hdl->param.mode != ANC_OFF) {
            /*anc工作，关闭自动关机*/
            sys_auto_shut_down_disable();
            os_taskq_post_msg("anc", 1, ANC_MSG_RUN);
        } else {
            user_anc_log("anc close now,new mode is ANC_OFF\n");
            anc_hdl->mode_switch_lock = 0;
        }
    } else {
        user_anc_log("anc state err:%d\n", anc_hdl->state);
        anc_hdl->mode_switch_lock = 0;
    }
}

void anc_gain_app_value_set(int app_value)
{
    static int anc_gain_app_value = -1;
    if (-1 == app_value && -1 != anc_gain_app_value) {
        /* anc_hdl->param.anc_ff_gain = anc_gain_app_value; */
    }
    anc_gain_app_value = app_value;
}


#if TCFG_USER_TWS_ENABLE
/*tws同步播放模式提示音，并且同步进入anc模式*/
void anc_tone_sync_play(int tone_name)
{
    if (anc_hdl) {
        user_anc_log("anc_tone_sync_play:%d", tone_name);
        os_taskq_post_msg("anc", 2, ANC_MSG_TONE_SYNC, tone_name);
    }
}

static void anc_play_tone_at_same_time_handler(int tone_name, int err)
{
    switch (tone_name) {
    case SYNC_TONE_ANC_ON:
    case SYNC_TONE_ANC_OFF:
    case SYNC_TONE_ANC_TRANS:
        anc_tone_sync_play(tone_name);
        break;
    }
}

TWS_SYNC_CALL_REGISTER(anc_tws_tone_play) = {
    .uuid = 0x123A9E53,
    .task_name = "app_core",
    .func = anc_play_tone_at_same_time_handler,
};

void anc_play_tone_at_same_time(int tone_name, int msec)
{
    tws_api_sync_call_by_uuid(0x123A9E53, tone_name, msec);
}
#endif/*TCFG_USER_TWS_ENABLE*/

#define TWS_ANC_SYNC_TIMEOUT	400 //ms
void anc_mode_switch(u8 mode, u8 tone_play)
{
    if (anc_hdl == NULL) {
        return;
    }
    /*模式切换超出范围*/
    if ((mode > ANC_BYPASS) || (mode < ANC_OFF)) {
        user_anc_log("anc mode switch err:%d", mode);
        return;
    }
    /*模式切换同一个*/
    if ((anc_hdl->param.mode == mode) || (anc_hdl->new_mode == mode)) {
        user_anc_log("anc mode switch err:same mode");
        return;
    }
#if ANC_MODE_EN_MODE_NEXT_SW
    if (anc_hdl->mode_switch_lock) {
        user_anc_log("anc mode switch lock\n");
        return;
    }
    anc_hdl->mode_switch_lock = 1;
#endif/*ANC_MODE_EN_MODE_NEXT_SW*/


    anc_hdl->new_mode = mode;/*记录最新的目标ANC模式*/

    if (anc_hdl->suspend) {
        anc_hdl->param.tool_enablebit = 0;
    }

    /*
     *ANC模式提示音播放规则
     *(1)根据应用选择是否播放提示音：tone_play
     *(2)tws连接的时候，主机发起模式提示音同步播放
     *(3)单机的时候，直接播放模式提示音
     */
    if (tone_play) {
#if TCFG_USER_TWS_ENABLE
        if (get_tws_sibling_connect_state()) {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                user_anc_log("[tws_master]anc_tone_sync_play");
                anc_hdl->sync_busy = 1;
                if (anc_hdl->new_mode == ANC_ON) {
                    anc_play_tone_at_same_time(SYNC_TONE_ANC_ON, TWS_ANC_SYNC_TIMEOUT);
                } else if (anc_hdl->new_mode == ANC_OFF) {
                    anc_play_tone_at_same_time(SYNC_TONE_ANC_OFF, TWS_ANC_SYNC_TIMEOUT);
                } else {
                    anc_play_tone_at_same_time(SYNC_TONE_ANC_TRANS, TWS_ANC_SYNC_TIMEOUT);
                }
            }
            return;
        } else {
            user_anc_log("anc_tone_play");
            anc_hdl->param.mode = mode;
            anc_tone_play_and_mode_switch(mode, ANC_TONE_PREEMPTION, ANC_TONE_END_MODE_SW);
        }
#else
        anc_hdl->param.mode = mode;
        anc_tone_play_and_mode_switch(mode, ANC_TONE_PREEMPTION, ANC_TONE_END_MODE_SW);
#endif/*TCFG_USER_TWS_ENABLE*/
    } else {
        anc_hdl->param.mode = mode;
        anc_mode_switch_deal(mode);
    }
}

static void anc_ui_mode_sel_timer(void *priv)
{
    if (anc_hdl->ui_mode_sel && (anc_hdl->ui_mode_sel != anc_hdl->param.mode)) {
        /*
         *提示音不打断
         *tws提示音同步播放完成
         */
        /* if ((tone_get_status() == 0) && (anc_hdl->sync_busy == 0)) { */
        if (anc_hdl->sync_busy == 0) {
            user_anc_log("anc_ui_mode_sel_timer:%d,sync_busy:%d", anc_hdl->ui_mode_sel, anc_hdl->sync_busy);
            anc_mode_switch(anc_hdl->ui_mode_sel, 1);
            sys_timer_del(anc_hdl->ui_mode_sel_timer);
            anc_hdl->ui_mode_sel_timer = 0;
        }
    }
}

/*ANC通过ui菜单选择anc模式,处理快速切换的情景*/
void anc_ui_mode_sel(u8 mode, u8 tone_play)
{
    /*
     *timer存在表示上个模式还没有完成切换
     *提示音不打断
     *tws提示音同步播放完成
     */
    /* if ((anc_hdl->ui_mode_sel_timer == 0) && (tone_get_status() == 0) && (anc_hdl->sync_busy == 0)) { */
    if ((anc_hdl->ui_mode_sel_timer == 0) && (anc_hdl->sync_busy == 0)) {
        user_anc_log("anc_ui_mode_sel[ok]:%d", mode);
        anc_mode_switch(mode, tone_play);
    } else {
        user_anc_log("anc_ui_mode_sel[dly]:%d,timer:%d,sync_busy:%d", mode, anc_hdl->ui_mode_sel_timer, anc_hdl->sync_busy);
        anc_hdl->ui_mode_sel = mode;
        if (anc_hdl->ui_mode_sel_timer == 0) {
            anc_hdl->ui_mode_sel_timer = sys_timer_add(NULL, anc_ui_mode_sel_timer, 50);
        }
    }
}

/*ANC模式同步(tws模式)*/
void anc_mode_sync(u8 *data)
{
    if (anc_hdl) {
        os_taskq_post_msg("anc", 2, ANC_MSG_MODE_SYNC, data[0]);
    }
}

/*ANC挂起*/
void anc_suspend(void)
{
    if (anc_hdl) {
        user_anc_log("anc_suspend\n");
        anc_hdl->suspend = 1;
        anc_hdl->param.tool_enablebit = 0;	//使能设置为0
        if (anc_hdl->param.anc_fade_en) {	  //挂起ANC增益淡出
            audio_anc_fade(0, anc_hdl->param.anc_fade_en, 1);
        }
        audio_anc_tool_en_ctrl(&anc_hdl->param, 0);
    }
}

/*ANC恢复*/
void anc_resume(void)
{
    if (anc_hdl) {
        user_anc_log("anc_resume\n");
        anc_hdl->suspend = 0;
        anc_hdl->param.tool_enablebit = anc_hdl->param.enablebit;	//使能恢复
        audio_anc_tool_en_ctrl(&anc_hdl->param, anc_hdl->param.enablebit);
        if (anc_hdl->param.anc_fade_en) {	  //恢复ANC增益淡入
            audio_anc_fade(anc_hdl->param.anc_fade_gain, anc_hdl->param.anc_fade_en, 1);
        }
    }
}

/*ANC信息保存*/
void anc_info_save()
{
    if (anc_hdl) {
        anc_info_t anc_info;
        int ret = syscfg_read(CFG_ANC_INFO, &anc_info, sizeof(anc_info));
        if (ret == sizeof(anc_info)) {
#if INEAR_ANC_UI
            if (anc_info.mode == anc_hdl->param.mode && anc_info.inear_tws_mode == inear_tws_ancmode) {
#else
            if (anc_info.mode == anc_hdl->param.mode) {
#endif/*INEAR_ANC_UI*/
                user_anc_log("anc info.mode == cur_anc_mode");
                return;
            }
        } else {
            user_anc_log("read anc_info err");
        }

        user_anc_log("save anc_info");
        anc_info.mode = anc_hdl->param.mode;
#if INEAR_ANC_UI
        anc_info.inear_tws_mode = inear_tws_ancmode;
#endif/*INEAR_ANC_UI*/
        ret = syscfg_write(CFG_ANC_INFO, &anc_info, sizeof(anc_info));
        if (ret != sizeof(anc_info)) {
            user_anc_log("anc info save err!\n");
        }

    }
}

/*系统上电的时候，根据配置决定是否进入上次的模式*/
void anc_poweron(void)
{
    if (anc_hdl) {
#if ANC_INFO_SAVE_ENABLE
        anc_info_t anc_info;
        int ret = syscfg_read(CFG_ANC_INFO, &anc_info, sizeof(anc_info));
        if (ret == sizeof(anc_info)) {
            user_anc_log("read anc_info succ,state:%s,mode:%s", anc_state_str[anc_hdl->state], anc_mode_str[anc_info.mode]);
#if INEAR_ANC_UI
            inear_tws_ancmode = anc_info.inear_tws_mode;
#endif/*INEAR_ANC_UI*/
            if ((anc_hdl->state == ANC_STA_INIT) && (anc_info.mode != ANC_OFF)) {
                anc_mode_switch(anc_info.mode, 0);
            }
        } else {
            user_anc_log("read anc_info err");
        }
#endif/*ANC_INFO_SAVE_ENABLE*/
    }
}

/*ANC poweroff*/
void anc_poweroff(void)
{
    if (anc_hdl) {
        user_anc_log("anc_cur_state:%s\n", anc_state_str[anc_hdl->state]);
#if ANC_INFO_SAVE_ENABLE
        anc_info_save();
#endif/*ANC_INFO_SAVE_ENABLE*/
        if (anc_hdl->state == ANC_STA_OPEN) {
            user_anc_log("anc_poweroff\n");
            /*close anc module when fade_out timeout*/
            anc_hdl->param.mode = ANC_OFF;
            anc_fade(0);
        }
    }
}

/*模式切换测试demo*/
#define ANC_MODE_NUM	3 /*ANC模式循环切换*/
static const u8 anc_mode_switch_tab[ANC_MODE_NUM] = {
    ANC_OFF,
    ANC_TRANSPARENCY,
    ANC_ON,
};
void anc_mode_next(void)
{
    if (anc_hdl) {
        if (anc_train_open_query()) {
            return;
        }
        u8 next_mode = 0;
        local_irq_disable();
        anc_hdl->param.anc_fade_en = ANC_FADE_EN;	//防止被其他地方清0
        for (u8 i = 0; i < ANC_MODE_NUM; i++) {
            if (anc_mode_switch_tab[i] == anc_hdl->param.mode) {
                next_mode = i + 1;
                if (next_mode >= ANC_MODE_NUM) {
                    next_mode = 0;
                }
                if ((anc_hdl->mode_enable & BIT(anc_mode_switch_tab[next_mode])) == 0) {
                    user_anc_log("anc_mode_filt,next:%d,en:%d", next_mode, anc_hdl->mode_enable);
                    next_mode++;
                    if (next_mode >= ANC_MODE_NUM) {
                        next_mode = 0;
                    }
                }
                //g_printf("fine out anc mode:%d,next:%d,i:%d",anc_hdl->param.mode,next_mode,i);
                break;
            }
        }
        local_irq_enable();
        //user_anc_log("anc_next_mode:%d old:%d,new:%d", next_mode, anc_hdl->param.mode, anc_mode_switch_tab[next_mode]);
        u8 new_mode = anc_mode_switch_tab[next_mode];
        user_anc_log("new_mode:%s", anc_mode_str[new_mode]);
        anc_mode_switch(anc_mode_switch_tab[next_mode], 1);
    }
}

/*设置ANC支持切换的模式*/
void anc_mode_enable_set(u8 mode_enable)
{
    if (anc_hdl) {
        anc_hdl->mode_enable = mode_enable;
        u8 mode_cnt = 0;
        for (u8 i = 1; i < 4; i++) {
            if (mode_enable & BIT(i)) {
                mode_cnt++;
                user_anc_log("%s Select", anc_mode_str[i]);
            }
        }
        user_anc_log("anc_mode_enable_set:%d", mode_cnt);
        anc_hdl->mode_num = mode_cnt;
    }
}

/*获取anc状态，0:空闲，l:忙*/
u8 anc_status_get(void)
{
    u8 status = 0;
    if (anc_hdl) {
        if (anc_hdl->state == ANC_STA_OPEN) {
            status = 1;
        }
    }
    return status;
}

/*获取anc当前模式*/
u8 anc_mode_get(void)
{
    if (anc_hdl) {
        //user_anc_log("anc_mode_get:%s", anc_mode_str[anc_hdl->param.mode]);
        return anc_hdl->param.mode;
    }
    return 0;
}

/*获取anc模式，dac左右声道的增益*/
u8 anc_dac_gain_get(u8 ch)
{
    u8 gain = 0;
    if (anc_hdl) {
        gain = anc_hdl->param.gains.dac_gain;
    }
    return gain;
}

/*获取anc模式，ref_mic的增益*/
u8 anc_mic_gain_get(void)
{
    u8 gain = 0;
    if (anc_hdl) {
        gain = anc_hdl->param.gains.ref_mic_gain;
    }
    return gain;
}


/*获取anc模式，指定mic的增益, mic_sel:目标MIC通道*/
u8 audio_anc_mic_gain_get(u8 mic_sel)
{
    if (anc_hdl) {
        if (mic_sel < AUDIO_ADC_MAX_NUM)  {
            return anc_hdl->param.mic_param[mic_sel].gain;
        }
    }
    return 0;
}

/*anc coeff读接口*/
int *anc_coeff_read(void)
{
#if ANC_BOX_READ_COEFF
    int *coeff = anc_db_get(ANC_DB_COEFF, &anc_hdl->param.coeff_size);
    if (coeff) {
        coeff = (int *)((u8 *)coeff);
    }
    user_anc_log("anc_coeff_read:0x%x", (u32)coeff);
    return coeff;
#else
    return NULL;
#endif/*ANC_BOX_READ_COEFF*/
}

/*anc coeff写接口*/
int anc_coeff_write(int *coeff, u16 len)
{
    int ret = 0;
    u8 single_type = 0;
    u16 config_len = anc_coeff_size_count(&anc_hdl->param);
    anc_coeff_t *db_coeff = (anc_coeff_t *)coeff;
    anc_coeff_t *tmp_coeff = NULL;

    user_anc_log("anc_coeff_write:0x%x, len:%d, config_len %d", (u32)coeff, len, config_len);
    ret = anc_coeff_check(db_coeff, len);
    if (ret) {
        return ret;
    }
    if (db_coeff->cnt != 1) {
        ret = anc_db_put(&anc_hdl->param, NULL, (anc_coeff_t *)coeff);
    } else {	//保存对应的单项滤波器
        db_coeff = (anc_coeff_t *)anc_db_get(ANC_DB_COEFF, &anc_hdl->param.write_coeff_size);
        tmp_coeff = malloc(anc_hdl->param.write_coeff_size);
        if (tmp_coeff) {
            memcpy(tmp_coeff, db_coeff, anc_hdl->param.coeff_size);
        }
        anc_coeff_single_fill((anc_coeff_t *)coeff, tmp_coeff);
        ret = anc_db_put(&anc_hdl->param, NULL, tmp_coeff);
        if (tmp_coeff) {
            free(tmp_coeff);
        }
    }
    if (ret) {
        user_anc_log("anc_coeff_write err:%d", ret);
        return -1;
    }
    db_coeff = (anc_coeff_t *)anc_db_get(ANC_DB_COEFF, &anc_hdl->param.coeff_size);
    anc_coeff_fill(db_coeff);
    if (anc_hdl->param.mode != ANC_OFF) {		//实时更新填入使用
        anc_coeff_online_update(&anc_hdl->param, 1);
    }
    return 0;
}

static u8 ANC_idle_query(void)
{
    if (anc_hdl) {
        /*ANC训练模式，不进入低功耗*/
        if (anc_train_open_query()) {
            return 0;
        }
    }
    return 1;
}

static enum LOW_POWER_LEVEL ANC_level_query(void)
{
    if (anc_hdl == NULL) {
        return LOW_POWER_MODE_SLEEP;
    }
    /*根据anc的状态选择sleep等级*/
    if (anc_status_get() || anc_hdl->mode_switch_lock) {
        /*anc打开，进入轻量级低功耗*/
        return LOW_POWER_MODE_LIGHT_SLEEP;
    }
    /*anc关闭，进入最优低功耗*/
    return LOW_POWER_MODE_SLEEP;
}

REGISTER_LP_TARGET(ANC_lp_target) = {
    .name       = "ANC",
    .level      = ANC_level_query,
    .is_idle    = ANC_idle_query,
};

void chargestore_uart_data_deal(u8 *data, u8 len)
{
    /* anc_uart_process(data, len); */
}

#if TCFG_ANC_TOOL_DEBUG_ONLINE
//回复包
void anc_ci_send_packet(u32 id, u8 *packet, int size)
{
    if (DB_PKT_TYPE_ANC == id) {
        app_online_db_ack(anc_hdl->anc_parse_seq, packet, size);
        return;
    }
    /* y_printf("anc_app_spp_tx\n"); */
    ci_send_packet(id, packet, size);
}

//接收函数
static int anc_app_online_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size)
{
    /* y_printf("anc_app_spp_rx\n"); */
    anc_hdl->anc_parse_seq  = ext_data[1];
    anc_spp_rx_packet(packet, size);
    return 0;
}
//发送函数
void anc_btspp_packet_tx(u8 *packet, int size)
{
    app_online_db_send(DB_PKT_TYPE_ANC, packet, size);
}
#endif/*TCFG_ANC_TOOL_DEBUG_ONLINE*/

u8 anc_btspp_train_again(u8 mode, u32 dat)
{
    /* tws_api_detach(TWS_DETACH_BY_POWEROFF); */
    int ret = 0;
    user_anc_log("anc_btspp_train_again\n");
    audio_anc_close();
    anc_hdl->state = ANC_STA_INIT;
    anc_train_open(mode, (u8)dat);
    return 1;
}

u8 audio_anc_develop_get(void)
{
    return 0;
    //return anc_hdl->param.developer_mode;
}

void audio_anc_develop_set(u8 mode)
{

}

/*ANC配置在线读取接口*/
int anc_cfg_online_deal(u8 cmd, anc_gain_t *cfg)
{
    /*同步在线更新配置*/
    int ret = 0;
    anc_param_fill(cmd, cfg);
    if (cmd == ANC_CFG_WRITE) {
        /*实时更新ANC配置*/
        audio_anc_reset(0);
        anc_mix_out_audio_drc_thr(anc_hdl->param.gains.audio_drc_thr);
        audio_anc_gain_sign(&anc_hdl->param);
        audio_anc_dac_gain(cfg->gains.dac_gain, cfg->gains.dac_gain);
        audio_anc_mic_management(&anc_hdl->param);
        audio_anc_mic_gain(anc_hdl->param.mic_param[0].gain, anc_hdl->param.mic_param[1].gain);
        audio_anc_drc_ctrl(&anc_hdl->param, 1.0);
        if (anc_hdl->param.mode == ANC_TRANSPARENCY) {
            audio_anc_dcc_set(1);
        } else {
            audio_anc_dcc_set(anc_hdl->param.gains.dcc_sel);
        }
        audio_anc_max_yorder_verify(&anc_hdl->param, TCFG_AUDIO_ANC_MAX_ORDER);
        anc_coeff_online_update(&anc_hdl->param, 0);
        if (anc_hdl->param.mode == ANC_ON) {
            audio_anc_cmp_en(&anc_hdl->param, anc_hdl->param.gains.cmp_en);
        }
        audio_anc_reset(1);
        /*仅修改增益配置*/
        ret = anc_db_put(&anc_hdl->param, cfg, NULL);
        if (ret) {
            user_anc_log("ret %d\n", ret);
            anc_hdl->param.lff_coeff = NULL;
            anc_hdl->param.rff_coeff = NULL;
            anc_hdl->param.lfb_coeff = NULL;
            anc_hdl->param.rfb_coeff = NULL;
        };
    }
    return 0;
}

/*ANC数字MIC IO配置*/
static atomic_t dmic_mux_ref;
#define DMIC_SCLK_FROM_PLNK		0
#define DMIC_SCLK_FROM_ANC		1
void dmic_io_mux_ctl(u8 en, u8 sclk_sel)
{
#if TCFG_AUDIO_PLNK_SCLK_PIN != NO_CONFIG_PORT
    user_anc_log("dmic_io_mux,en:%d,sclk:%d,ref:%d\n", en, sclk_sel, atomic_read(&dmic_mux_ref));
    if (en) {
        if (atomic_read(&dmic_mux_ref)) {
            user_anc_log("DMIC_IO_MUX open now\n");
            if (sclk_sel == DMIC_SCLK_FROM_ANC) {
                user_anc_log("plink_sclk -> anc_sclk\n");
                gpio_set_fun_output_port(TCFG_AUDIO_PLNK_SCLK_PIN, FO_ANC_MICCK, 0, 1);
            }
            atomic_inc_return(&dmic_mux_ref);
            return;
        }
        if (sclk_sel == DMIC_SCLK_FROM_ANC) {
            gpio_set_function(IO_PORT_SPILT(TCFG_AUDIO_PLNK_SCLK_PIN), PORT_FUNC_ANC_MICCK);
        } else {
            gpio_set_function(IO_PORT_SPILT(TCFG_AUDIO_PLNK_SCLK_PIN), PORT_FUNC_PLNK_SCLK);
        }
#if TCFG_AUDIO_PLNK_DAT0_PIN != NO_CONFIG_PORT
        //anc data0 port init
        gpio_set_function(IO_PORT_SPILT(TCFG_AUDIO_PLNK_DAT0_PIN), PORT_FUNC_PLNK_DAT0);
#endif/*TCFG_AUDIO_PLNK_DAT0_PIN != NO_CONFIG_PORT*/

#if TCFG_AUDIO_PLNK_DAT1_PIN != NO_CONFIG_PORT
        //anc data1 port init
        gpio_set_function(IO_PORT_SPILT(TCFG_AUDIO_PLNK_DAT1_PIN), PORT_FUNC_PLNK_DAT1);
#endif/*TCFG_AUDIO_PLNK_DAT1_PIN != NO_CONFIG_PORT*/
        atomic_inc_return(&dmic_mux_ref);
    } else {
        if (atomic_read(&dmic_mux_ref)) {
            atomic_dec_return(&dmic_mux_ref);
            if (atomic_read(&dmic_mux_ref)) {
                if (sclk_sel == DMIC_SCLK_FROM_ANC) {
                    user_anc_log("anc close now,anc_sclk->plnk_sclk\n");
                    gpio_set_fun_output_port(TCFG_AUDIO_PLNK_SCLK_PIN, FO_PLNK_SCLK, 0, 1);
                } else {
                    user_anc_log("plnk close,anc_plnk open\n");
                }
            } else {
                user_anc_log("dmic all close,disable plnk io_mapping output\n");
                gpio_disable_function(IO_PORT_SPILT(TCFG_AUDIO_PLNK_SCLK_PIN), PORT_FUNC_PLNK_SCLK);
#if TCFG_AUDIO_PLNK_DAT0_PIN != NO_CONFIG_PORT
                gpio_disable_function(IO_PORT_SPILT(TCFG_AUDIO_PLNK_DAT0_PIN), PORT_FUNC_PLNK_DAT0);
#endif/*TCFG_AUDIO_PLNK_DAT0_PIN != NO_CONFIG_PORT*/
#if TCFG_AUDIO_PLNK_DAT1_PIN != NO_CONFIG_PORT
                gpio_disable_function(IO_PORT_SPILT(TCFG_AUDIO_PLNK_DAT1_PIN), PORT_FUNC_PLNK_DAT1);
#endif/*TCFG_AUDIO_PLNK_DAT1_PIN != NO_CONFIG_PORT*/
            }
        } else {
            user_anc_log("dmic_mux_ref NULL\n");
        }
    }
#endif/*TCFG_AUDIO_PLNK_SCLK_PIN*/
}
void anc_dmic_io_init(audio_anc_t *param, u8 en)
{
    if (en) {
        int i;
        for (i = 0; i < 4; i++) {
            if ((param->mic_type[i] > A_MIC1) && (param->mic_type[i] != MIC_NULL)) {
                user_anc_log("anc_dmic_io_init %d:%d\n", i, param->mic_type[i]);
                dmic_io_mux_ctl(1, DMIC_SCLK_FROM_ANC);
                break;
            }
        }
    } else {
        dmic_io_mux_ctl(0, DMIC_SCLK_FROM_ANC);
    }
}

extern void mix_out_drc_threadhold_update(float threadhold);
static void anc_mix_out_audio_drc_thr(float thr)
{
    thr = (thr > 0.0) ? 0.0 : thr;
    thr = (thr < -6.0) ? -6.0 : thr;
    /* mix_out_drc_threadhold_update(thr); */
}

#if TCFG_AUDIO_DYNAMIC_ADC_GAIN
void anc_dynamic_micgain_start(u8 audio_mic_gain)
{
    int anc_mic_gain, i;
    float ffgain;
    float multiple = 1.259;

    if (!anc_status_get()) {
        return;
    }
    anc_hdl->drc_ratio = 1.0;
    ffgain = (float)(anc_hdl->param.mode == ANC_TRANSPARENCY ? anc_hdl->param.gains.drctrans_norgain : anc_hdl->param.gains.drcff_norgain);
    //查询遍历当前的通话共用MIC
    if (TCFG_AUDIO_ADC_MIC_CHA & AUDIO_ADC_MIC_0) {
        for (i = 0; i < 4; i += 2) {
            if (anc_hdl->param.mic_type[i] == A_MIC0) {
                anc_hdl->dy_mic_ch = A_MIC0;
                anc_mic_gain = anc_hdl->param.gains.ref_mic_gain;
            }
        }
    }
    if (TCFG_AUDIO_ADC_MIC_CHA & AUDIO_ADC_MIC_1) {
        for (i = 0; i < 4; i += 2) {
            if (anc_hdl->param.mic_type[i] == A_MIC1) {
                anc_hdl->dy_mic_ch = A_MIC1;
                anc_mic_gain = anc_hdl->param.gains.err_mic_gain;
            }
        }
    }
    if (anc_hdl->dy_mic_ch == MIC_NULL) {
        user_anc_log("There is no common MIC\n");
        return;
    }
    if (anc_hdl->mic_resume_timer) {
        sys_timer_del(anc_hdl->mic_resume_timer);
    }
    anc_hdl->mic_diff_gain = audio_mic_gain - anc_mic_gain;
    if (anc_hdl->mic_diff_gain > 0) {
        for (i = 0; i < anc_hdl->mic_diff_gain; i++) {
            ffgain /= multiple;
            anc_hdl->drc_ratio *= multiple;
        }
    } else {
        for (i = 0; i > anc_hdl->mic_diff_gain; i--) {
            ffgain *= multiple;
            anc_hdl->drc_ratio /= multiple;
        }
    }
    audio_anc_drc_ctrl(&anc_hdl->param, anc_hdl->drc_ratio);
    if (TCFG_AUDIO_ANC_CH == (ANC_L_CH | ANC_R_CH)) {	//需保持两个MIC增益一致
        audio_anc_mic_gain(audio_mic_gain, audio_mic_gain);
    }
    user_anc_log("dynamic mic:audio_g %d,anc_g %d, diff_g %d, ff_gain %d, \n", audio_mic_gain, anc_mic_gain, anc_hdl->mic_diff_gain, (s16)ffgain);
}

void anc_dynamic_micgain_resume_timer(void *priv)
{
    float multiple = 1.259;
    if (anc_hdl->mic_diff_gain > 0) {
        anc_hdl->mic_diff_gain--;
        anc_hdl->drc_ratio /= multiple;
        if (!anc_hdl->mic_diff_gain) {
            anc_hdl->drc_ratio = 1.0;
        }
        audio_anc_drc_ctrl(&anc_hdl->param, anc_hdl->drc_ratio);
        if (TCFG_AUDIO_ANC_CH == (ANC_L_CH | ANC_R_CH)) {	//需保持两个MIC增益一致
            audio_anc_mic_gain(anc_hdl->param.gains.ref_mic_gain + anc_hdl->mic_diff_gain, anc_hdl->param.gains.err_mic_gain + anc_hdl->mic_diff_gain);
            /* user_anc_log("mic0 gain / mic1 gain %d\n", anc_hdl->param.gains.ref_mic_gain + anc_hdl->mic_diff_gain); */
        } else if (anc_hdl->dy_mic_ch == A_MIC0) {
            audio_anc_mic_gain(anc_hdl->param.gains.ref_mic_gain + anc_hdl->mic_diff_gain, anc_hdl->param.gains.err_mic_gain);
            /* user_anc_log("mic0 gain %d\n", anc_hdl->param.gains.ref_mic_gain + anc_hdl->mic_diff_gain); */
        } else {
            audio_anc_mic_gain(anc_hdl->param.gains.ref_mic_gain, anc_hdl->param.gains.err_mic_gain + anc_hdl->mic_diff_gain);
            /* user_anc_log("mic1 gain %d\n", anc_hdl->param.gains.err_mic_gain + anc_hdl->mic_diff_gain); */
        }
    } else if (anc_hdl->mic_diff_gain < 0) {
        anc_hdl->mic_diff_gain++;
        anc_hdl->drc_ratio *= multiple;
        if (!anc_hdl->mic_diff_gain) {
            anc_hdl->drc_ratio = 1.0;
        }
        audio_anc_drc_ctrl(&anc_hdl->param, anc_hdl->drc_ratio);
        if (TCFG_AUDIO_ANC_CH == (ANC_L_CH | ANC_R_CH)) {	//需保持两个MIC增益一致
            audio_anc_mic_gain(anc_hdl->param.gains.ref_mic_gain + anc_hdl->mic_diff_gain, anc_hdl->param.gains.err_mic_gain + anc_hdl->mic_diff_gain);
            /* user_anc_log("mic0 gain / mic1 gain %d\n", anc_hdl->param.gains.ref_mic_gain + anc_hdl->mic_diff_gain); */
        } else if (anc_hdl->dy_mic_ch == A_MIC0) {
            audio_anc_mic_gain(anc_hdl->param.gains.ref_mic_gain + anc_hdl->mic_diff_gain, anc_hdl->param.gains.err_mic_gain);
            /* user_anc_log("mic0 gain %d\n", anc_hdl->param.gains.ref_mic_gain + anc_hdl->mic_diff_gain); */
        } else {
            audio_anc_mic_gain(anc_hdl->param.gains.ref_mic_gain, anc_hdl->param.gains.err_mic_gain + anc_hdl->mic_diff_gain);
            /* user_anc_log("mic1 gain %d\n", anc_hdl->param.gains.ref_mic_gain + anc_hdl->mic_diff_gain); */
        }
    } else {
        sys_timer_del(anc_hdl->mic_resume_timer);
        anc_hdl->mic_resume_timer = 0;
    }
}

void anc_dynamic_micgain_stop(void)
{
    if ((anc_hdl->dy_mic_ch != MIC_NULL) && anc_status_get()) {
        anc_hdl->mic_resume_timer = sys_timer_add(NULL, anc_dynamic_micgain_resume_timer, 10);
    }
}
#endif/*TCFG_AUDIO_DYNAMIC_ADC_GAIN*/

void audio_anc_mic_management(audio_anc_t *param)
{
    int i, j;
    u8 double_mult_flag = 0;
    struct adc_file_cfg *cfg = audio_adc_file_get_cfg();
    for (i = 0; i < 4; i++) {
        if (param->mic_type[i] == A_MIC0) {
            param->mic_param[0].en = 1;
            param->mic_param[0].type = i;
            if (cfg->mic_en_map & AUDIO_ADC_MIC_0) {
                param->mic_param[0].mult_flag = 1;
            }
        }
        if (param->mic_type[i] == A_MIC1) {
            param->mic_param[1].en = 1;
            param->mic_param[1].type = i;
            if (cfg->mic_en_map & AUDIO_ADC_MIC_1) {
                param->mic_param[1].mult_flag = 1;
            }
        }
    }
    for (i = 0; i < 2; i++) {
        anc_hdl->param.mic_param[i].mic_p.mic_mode      = cfg->param[i].mic_mode;
        anc_hdl->param.mic_param[i].mic_p.mic_ain_sel   = cfg->param[i].mic_ain_sel;
        anc_hdl->param.mic_param[i].mic_p.mic_bias_sel  = cfg->param[i].mic_bias_sel;
        anc_hdl->param.mic_param[i].mic_p.mic_bias_rsel = cfg->param[i].mic_bias_rsel;
        anc_hdl->param.mic_param[i].mic_p.mic_dcc       = cfg->param[i].mic_dcc;
        if ((param->ch == (ANC_L_CH | ANC_R_CH)) && param->mic_param[i].mult_flag) {
            for (j = 0; j < 2; j++) {
                if ((param->mic_param[j].type + 1) % 2) {
                    param->mic_param[j].mult_flag = 1;
                }
            }
        }
    }
    param->mic_param[0].gain = param->gains.ref_mic_gain;
    param->mic_param[1].gain = param->gains.err_mic_gain;
    for (i = 0; i < 2; i++) {
        printf("mic%d en %d, gain %d, type %d, mult_flag %d\n", i, param->mic_param[i].en, \
               param->mic_param[i].gain, param->mic_param[i].type, param->mic_param[i].mult_flag);
    }
}

void audio_anc_post_msg_drc(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_DRC_TIMER);
}

void audio_anc_post_msg_debug(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_DEBUG_OUTPUT);
}

void audio_anc_dut_enable_set(u8 enablebit)
{
    anc_hdl->param.dut_audio_enablebit = enablebit;
}

#if ANC_MUSIC_DYNAMIC_GAIN_EN

#define ANC_MUSIC_DYNAMIC_GAIN_NORGAIN				1024	/*正常默认增益*/
#define ANC_MUSIC_DYNAMIC_GAIN_TRIGGER_INTERVAL		6		/*音乐响度取样间隔*/

#if ANC_MUSIC_DYNAMIC_GAIN_SINGLE_FB
void audio_anc_music_dyn_gain_resume_timer(void *priv)
{
    if (anc_hdl->loud_nowgain < anc_hdl->loud_targetgain) {
        anc_hdl->loud_nowgain += 5;      ///+5耗时194 * 10ms
        if (anc_hdl->loud_nowgain >= anc_hdl->loud_targetgain) {
            anc_hdl->loud_nowgain = anc_hdl->loud_targetgain;
            sys_timer_del(anc_hdl->loud_timeid);
            anc_hdl->loud_timeid = 0;
        }
    } else if (anc_hdl->loud_nowgain > anc_hdl->loud_targetgain) {
        anc_hdl->loud_nowgain -= 5;      ///-5耗时194 * 10ms
        if (anc_hdl->loud_nowgain <= anc_hdl->loud_targetgain) {
            anc_hdl->loud_nowgain = anc_hdl->loud_targetgain;
            sys_timer_del(anc_hdl->loud_timeid);
            anc_hdl->loud_timeid = 0;
        }
    }
    audio_anc_norgain(anc_hdl->param.gains.drcff_norgain, anc_hdl->loud_nowgain);
    /* user_anc_log("fb resume anc_hdl->loud_nowgain %d\n", anc_hdl->loud_nowgain); */
}

#endif/*ANC_MUSIC_DYNAMIC_GAIN_SINGLE_FB*/

void audio_anc_music_dynamic_gain_process(void)
{
    if (anc_hdl->loud_nowgain != anc_hdl->loud_targetgain) {
//        user_anc_log("low_nowgain %d, targergain %d\n", anc_hdl->loud_nowgain, anc_hdl->loud_targetgain);
#if ANC_MUSIC_DYNAMIC_GAIN_SINGLE_FB
        if (!anc_hdl->loud_timeid) {
            anc_hdl->loud_timeid = sys_timer_add(NULL, audio_anc_music_dyn_gain_resume_timer, 10);
        }
#else
        anc_hdl->loud_nowgain = anc_hdl->loud_targetgain;
        audio_anc_fade(anc_hdl->loud_targetgain << 4, anc_hdl->param.anc_fade_en, 1);
#endif/*ANC_MUSIC_DYNAMIC_GAIN_SINGLE_FB*/
    }
}

void audio_anc_music_dynamic_gain_reset(u8 effect_reset_en)
{
    if (anc_hdl->loud_targetgain != ANC_MUSIC_DYNAMIC_GAIN_NORGAIN) {
        /* user_anc_log("audio_anc_music_dynamic_gain_reset\n"); */
        anc_hdl->loud_targetgain = ANC_MUSIC_DYNAMIC_GAIN_NORGAIN;
        if (anc_hdl->loud_timeid) {
            sys_timer_del(anc_hdl->loud_timeid);
            anc_hdl->loud_timeid = 0;
        }
        if (effect_reset_en && anc_hdl->param.mode == ANC_ON) {
            os_taskq_post_msg("anc", 1, ANC_MSG_MUSIC_DYN_GAIN);
        } else {
            anc_hdl->loud_nowgain = ANC_MUSIC_DYNAMIC_GAIN_NORGAIN;
            anc_hdl->loud_dB = 1;	//清除上次保留的dB值, 避免切模式后不触发的情况
            anc_hdl->loud_hdl.peak_val = ANC_MUSIC_DYNAMIC_GAIN_THR;
        }
    }
}

void audio_anc_music_dynamic_gain_dB_get(int dB)
{
    float mult = 1.122f;
    float target_mult = 1.0f;
    int cnt, i, gain;
#if ANC_MUSIC_DYNAMIC_GAIN_TRIGGER_INTERVAL > 1 //区间变化，避免频繁操作
    if (dB > ANC_MUSIC_DYNAMIC_GAIN_THR) {
        int temp_dB = dB / ANC_MUSIC_DYNAMIC_GAIN_TRIGGER_INTERVAL;
        dB = temp_dB * ANC_MUSIC_DYNAMIC_GAIN_TRIGGER_INTERVAL;
    }
#endif/*ANC_MUSIC_DYNAMIC_GAIN_TRIGGER_INTERVAL*/
    if (anc_hdl->loud_dB != dB) {
        anc_hdl->loud_dB = dB;
        if (dB > ANC_MUSIC_DYNAMIC_GAIN_THR) {
            cnt = dB - ANC_MUSIC_DYNAMIC_GAIN_THR;
            for (i = 0; i < cnt; i++) {
                target_mult *= mult;
            }
            gain = (int)((float)ANC_MUSIC_DYNAMIC_GAIN_NORGAIN / target_mult);
//            user_anc_log("cnt %d, dB %d, gain %d\n", cnt, dB, gain);
            anc_hdl->loud_targetgain = gain;
            os_taskq_post_msg("anc", 1, ANC_MSG_MUSIC_DYN_GAIN);
        } else {
            audio_anc_music_dynamic_gain_reset(1);
        }
    }
}

void audio_anc_music_dynamic_gain_det(s16 *data, int len)
{
    /* putchar('l'); */
    if (anc_hdl->param.mode == ANC_ON) {
        loudness_meter_short(&anc_hdl->loud_hdl, data, len >> 1);
        audio_anc_music_dynamic_gain_dB_get(anc_hdl->loud_hdl.peak_val);
    }
}


void audio_anc_music_dynamic_gain_init(int sr)
{
    /* user_anc_log("audio_anc_music_dynamic_gain_init %d\n", sr); */
    anc_hdl->loud_nowgain = ANC_MUSIC_DYNAMIC_GAIN_NORGAIN;
    anc_hdl->loud_targetgain = ANC_MUSIC_DYNAMIC_GAIN_NORGAIN;
    anc_hdl->loud_dB = 1;     //清除上次保留的dB值
    loudness_meter_init(&anc_hdl->loud_hdl, sr, 50, 0);
    anc_hdl->loud_hdl.peak_val = ANC_MUSIC_DYNAMIC_GAIN_THR;
}
#endif/*ANC_MUSIC_DYNAMIC_GAIN_EN*/

#else

/*获取anc状态，0:空闲，l:忙*/
u8 anc_status_get(void)
{
    return 0;
}

#endif/*TCFG_AUDIO_ANC_ENABLE*/
