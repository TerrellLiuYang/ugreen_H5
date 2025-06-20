#ifndef AUDIO_ANC_H
#define AUDIO_ANC_H

#include "generic/typedef.h"
#include "asm/anc.h"
#include "anc_btspp.h"
#include "anc_uart.h"
#include "app_config.h"
#include "in_ear_detect/in_ear_manage.h"
#include "asm/dac.h"

/*******************ANC User Config***********************/
#define ANC_COEFF_SAVE_ENABLE		1	/*ANC滤波器表保存使能*/
#define ANC_INFO_SAVE_ENABLE		0	/*ANC信息记忆:保存上一次关机时所处的降噪模式等等*/
#define ANC_TONE_PREEMPTION			0	/*ANC提示音打断播放(1)还是叠加播放(0)*/
#define ANC_BOX_READ_COEFF			1	/*支持通过工具读取ANC训练系数*/
#define ANC_FADE_EN					1	/*ANC淡入淡出使能*/
#define ANC_MODE_SYSVDD_EN 			1	/*ANC模式提高SYSVDD，避免某些IC电压太低导致ADC模块工作不正常*/
#define ANC_TONE_END_MODE_SW		1	/*ANC提示音结束进行模式切换*/
#define ANC_MODE_EN_MODE_NEXT_SW	0	/*ANC提示音结束后才允许下一次模式切换*/
#define ANC_MODE_FADE_LVL			1	/*降噪模式淡入步进*/
#define ANC_DEVELOPER_MODE_EN		0	/*ANC开发者模式使能*/

/*
   音乐响度ANC动态增益调节, 随着音乐响度增大而减小ANC增益, 开此功能可关闭AUDIO_DRC
 */
#define ANC_MUSIC_DYNAMIC_GAIN_EN					1		/*音乐动态ANC增益使能*/
#define ANC_MUSIC_DYNAMIC_GAIN_THR 					-12		/*音乐动态ANC增益触发阈值(单位dB), range: (-30 ~ 0]*/
#define ANC_MUSIC_DYNAMIC_GAIN_SINGLE_FB			0		/*动态增益单独调FB增益使能*/
/*ANC工具配对码使能*/
#define ANCTOOL_NEED_PAIR_KEY   1
#define ANCTOOL_PAIR_KEY  		"123456" /*ANC工具默认配对码*/

/*******************ANC User Config End*******************/

#if TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_L
#define TCFG_AUDIO_ANC_CH 	ANC_L_CH
#elif TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_R
#define TCFG_AUDIO_ANC_CH 	ANC_R_CH
#else
#define TCFG_AUDIO_ANC_CH 	(ANC_L_CH | ANC_R_CH)
#endif/*TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_L*/

#define ANC_CONFIG_LFF_EN ((TCFG_AUDIO_ANC_TRAIN_MODE & (ANC_HYBRID_EN | ANC_FF_EN)) && (TCFG_AUDIO_ANC_CH & ANC_L_CH))
#define ANC_CONFIG_LFB_EN ((TCFG_AUDIO_ANC_TRAIN_MODE & (ANC_HYBRID_EN | ANC_FB_EN)) && (TCFG_AUDIO_ANC_CH & ANC_L_CH))
#define ANC_CONFIG_RFF_EN ((TCFG_AUDIO_ANC_TRAIN_MODE & (ANC_HYBRID_EN | ANC_FF_EN)) && (TCFG_AUDIO_ANC_CH & ANC_R_CH))
#define ANC_CONFIG_RFB_EN ((TCFG_AUDIO_ANC_TRAIN_MODE & (ANC_HYBRID_EN | ANC_FB_EN)) && (TCFG_AUDIO_ANC_CH & ANC_R_CH))


/*ANC模式调试信息*/
static const char *anc_mode_str[] = {
    "NULL",			/*无效/非法*/
    "ANC_OFF",		/*关闭模式*/
    "ANC_ON",		/*降噪模式*/
    "Transparency",	/*通透模式*/
    "ANC_BYPASS",	/*BYPASS模式*/
    "ANC_TRAIN",	/*训练模式*/
    "ANC_TRANS_TRAIN",	/*通透训练模式*/
};

/*ANC状态调试信息*/
static const char *anc_state_str[] = {
    "anc_close",	/*关闭状态*/
    "anc_init",		/*初始化状态*/
    "anc_open",		/*打开状态*/
};

/*ANC MSG List*/
enum {
    ANC_MSG_TRAIN_OPEN = 0xA1,
    ANC_MSG_TRAIN_CLOSE,
    ANC_MSG_RUN,
    ANC_MSG_FADE_END,
    ANC_MSG_MODE_SYNC,
    ANC_MSG_TONE_SYNC,
    ANC_MSG_DRC_TIMER,
    ANC_MSG_DEBUG_OUTPUT,
    ANC_MSG_MUSIC_DYN_GAIN,
};

/*ANC记忆信息*/
typedef struct {
    u8 mode;		/*当前模式*/
    u8 mode_enable; /*使能的模式*/
#if INEAR_ANC_UI
    u8 inear_tws_mode;
#endif/*INEAR_ANC_UI*/
    //s32 coeff[488];
} anc_info_t;

/*ANC初始化*/
void anc_init(void);

/*ANC训练模式*/
void anc_train_open(u8 mode, u8 debug_sel);

/*ANC关闭训练*/
void anc_train_close(void);

/*
 *ANC状态获取
 *return 0: idle(初始化)
 *return 1: busy(ANC/通透/训练)
 */
u8 anc_status_get(void);

u8 anc_mode_get(void);

#define ANC_DAC_CH_L	0
#define ANC_DAC_CH_R	1
/*获取anc模式，dac左右声道的增益*/
u8 anc_dac_gain_get(u8 ch);

/*获取anc模式，ref_mic的增益*/
u8 anc_mic_gain_get(void);

/*获取anc模式，指定mic的增益, mic_sel:目标MIC通道*/
u8 audio_anc_mic_gain_get(u8 mic_sel);

/*ANC模式切换(切换到指定模式)，并配置是否播放提示音*/
void anc_mode_switch(u8 mode, u8 tone_play);

/*ANC模式同步(tws模式)*/
void anc_mode_sync(u8 *data);

void anc_poweron(void);

/*ANC poweroff*/
void anc_poweroff(void);

/*ANC模式切换(下一个模式)*/
void anc_mode_next(void);

/*ANC通过ui菜单选择anc模式,处理快速切换的情景*/
void anc_ui_mode_sel(u8 mode, u8 tone_play);

/*设置ANC支持切换的模式*/
void anc_mode_enable_set(u8 mode_enable);

/*anc coeff 长度大小获取*/
int anc_coeff_size_get(u8 mode);

void anc_coeff_size_set(u8 mode, int len);

int *anc_debug_ctr(u8 free_en);
/*
 *查询当前ANC是否处于训练状态
 *@return 1:处于训练状态
 *@return 0:其他状态
 */
int anc_train_open_query(void);

/*ANC在线调试繁忙标志设置*/
void anc_online_busy_set(u8 en);

/*ANC在线调试繁忙标志获取*/
u8 anc_online_busy_get(void);

/*tws同步播放模式提示音，并且同步进入anc模式*/
void anc_tone_sync_play(int tone_name);

/*anc coeff读接口*/
int *anc_coeff_read(void);

/*anc coeff写接口*/
int anc_coeff_write(int *coeff, u16 len);

/*ANC挂起*/
void anc_suspend(void);

/*ANC恢复*/
void anc_resume(void);

/*通话动态MIC增益开始函数*/
void anc_dynamic_micgain_start(u8 audio_mic_gain);

/*通话动态MIC增益结束函数*/
void anc_dynamic_micgain_stop(void);

/*anc_gains参数读写接口*/
void anc_param_fill(u8 cmd, anc_gain_t *cfg);

/*ANC_DUT audio模块使能函数，用于分离功耗*/
void audio_anc_dut_enable_set(u8 enablebit);

/*ANC增益调节 音乐响度检测*/
void audio_anc_music_dynamic_gain_det(s16 *data, int len);

/*ANC增益调节 音乐响度初始化*/
void audio_anc_music_dynamic_gain_init(int sr);

/*ANC增益调节 音乐响度复位*/
void audio_anc_music_dynamic_gain_reset(u8 effect_reset_en);

/*ANC淡入淡出增益获取接口*/
int audio_anc_fade_gain_get(void);

/*ANC淡入淡出增益设置接口*/
void audio_anc_fade_gain_set(int gain);
extern int anc_uart_write(u8 *buf, u8 len);
extern void ci_send_packet(u32 id, u8 *packet, int size);

u8 audio_anc_develop_get(void);
void audio_anc_develop_set(u8 mode);

#endif/*AUDIO_ANC_H*/
