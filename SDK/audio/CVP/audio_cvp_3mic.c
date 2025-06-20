/*
 ****************************************************************
 *							AUDIO DMS(DualMic System)
 * File  : audio_aec_dms.c
 * By    :
 * Notes : AEC回音消除 + 双mic降噪(ENC)
 *
 ****************************************************************
 */
#include "audio_cvp.h"
#include "system/includes.h"
#include "media/includes.h"
#include "effects/eq_config.h"
#include "effects/audio_pitch.h"
#include "circular_buf.h"
#include "audio_cvp_online.h"
#include "cvp_node.h"
#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif/*TCFG_USER_TWS_ENABLE*/
#include "overlay_code.h"
#if TCFG_AUDIO_CVP_SYNC
#include "audio_cvp_sync.h"
#endif/*TCFG_AUDIO_CVP_SYNC*/
#include "audio_dc_offset_remove.h"
#include "audio_cvp_def.h"
#include "effects/audio_gain_process.h"
#include "amplitude_statistic.h"
#include "app_main.h"
#include "lib_h/jlsp_ns.h"
#include "fs/sdfile.h"
#if TCFG_AUDIO_DUT_ENABLE
//#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#if !defined(TCFG_CVP_DEVELOP_ENABLE) || (TCFG_CVP_DEVELOP_ENABLE == 0)

#if TCFG_AUDIO_TRIPLE_MIC_ENABLE
#define LOG_TAG_CONST       AEC_USER
#define LOG_TAG             "[AEC_USER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
#include "audio_cvp_debug.c"

#define AEC_USER_MALLOC_ENABLE	1
/*CVP_TOGGLE:AEC模块使能开关，Disable则数据完全不经过处理，AEC模块不占用资源*/
#define CVP_TOGGLE				1	/*回音消除模块开关*/

#ifdef CONFIG_FPGA_ENABLE
const u8 CONST_AEC_ENABLE = 1;
#else
const u8 CONST_AEC_ENABLE = 1;
#endif/*CONFIG_FPGA_ENABLE*/

/*
 * 串口写卡数据导出
 * 0 : 关闭数据导出
 * 1 : 导出 mic0 mic1 mic2 far out
 */
#ifdef AUDIO_PCM_DEBUG
const u8 CONST_AEC_EXPORT = 1;
#else
const u8 CONST_AEC_EXPORT = 0;
#endif

//*********************************************************************************//
//                                预处理配置(Pre-process Config)               	   //
//*********************************************************************************//
/*预增益配置*/
#define CVP_PRE_GAIN_ENABLE				0	//算法处理前预加数字增益放大

/*响度指示器*/
#define CVP_LOUDNESS_TRACE_ENABLE		0	//跟踪获取当前mic输入幅值

//*********************************************************************************//
//                                调试配置(Debug Config)                           //
//*********************************************************************************//
/*使能即可跟踪通话过程的内存情况*/
#define CVP_MEM_TRACE_ENABLE	0

/*
 *延时估计使能
 *点烟器需要做延时估计
 *其他的暂时不需要做
 */
const u8 CONST_AEC_DLY_EST = 0;

#if TCFG_AEC_SIMPLEX
const u8 CONST_AEC_SIMPLEX = 1;
#else
const u8 CONST_AEC_SIMPLEX = 0;
#endif/*TCFG_AEC_SIMPLEX*/

/*
 * 非线性压制模式选择
 * JLSP_NLP_MODE1: 模式1为单独的NLP回声抑制，回声压制会偏过，该模式下NLP模块可以单独开启
 * JLSP_NLP_MODE2: 模式2下回声信号会先经过AEC线性压制，然后进行NLP非线性压制
 *                 此模式NLP不能单独打开需要同时打开AEC,使用AEC模块压制不够时，建议开启该模式
 */
const u8 CONST_JLSP_NLP_MODE = JLSP_NLP_MODE1;

/*
 * 风噪降噪模式选择
 * JLSP_WD_MODE1: 模式1为常规的降风噪模式，风噪残余会偏大些
 * JLSP_WD_MODE2: 模式2为神经网络降风噪，风噪抑制会比较干净，但是会需要多消耗31kb的flash
 */
const u8 CONST_JLSP_WD_MODE = JLSP_WD_MODE1;

/*参考数据变采样处理*/
#if TCFG_USB_MIC_CVP_ENABLE
const u8 CONST_REF_SRC = 1;
#else
const u8 CONST_REF_SRC = 0;
#endif /*TCFG_USB_MIC_CVP_ENABLE*/

/*数据输出开头丢掉的数据包数*/
#define AEC_OUT_DUMP_PACKET		15
/*数据输出开头丢掉的数据包数*/
#define AEC_IN_DUMP_PACKET		1

__attribute__((weak))u32 usb_mic_is_running()
{
    return 0;
}

/*复用lmp rx buf(一般通话的时候复用)
 *rx_buf概率产生碎片，导致alloc失败，因此默认配0
 */
#define MALLOC_MULTIPLEX_EN		0
extern void *lmp_malloc(int);
extern void lmp_free(void *);
void *zalloc_mux(int size)
{
#if MALLOC_MULTIPLEX_EN
    void *p = NULL;
    do {
        p = lmp_malloc(size);
        if (p) {
            break;
        }
        printf("aec_malloc wait...\n");
        os_time_dly(2);
    } while (1);
    if (p) {
        memset(p, 0, size);
    }
    printf("[malloc_mux]p = 0x%x,size = %d\n", p, size);
    return p;
#else
    return zalloc(size);
#endif
}

void free_mux(void *p)
{
#if MALLOC_MULTIPLEX_EN
    printf("[free_mux]p = 0x%x\n", p);
    lmp_free(p);
#else
    free(p);
#endif
}

struct audio_aec_hdl {
    volatile u8 start;				//aec模块状态
    u8 inbuf_clear_cnt;		//aec输入数据丢掉
    u8 output_fade_in;		//aec输出淡入使能
    u8 output_fade_in_gain;	//aec输出淡入增益
    u8 EnableBit;			//aec使能模块
    u8 input_clear;			//清0输入数据标志
    u16 dump_packet;		//前面如果有杂音，丢掉几包
    void *dcc_hdl;
    struct tms_attr attr;	//aec模块参数属性
    struct audio_cvp_pre_param_t pre;	//预处理配置
    float *TransferFunc;
};
#if AEC_USER_MALLOC_ENABLE
struct audio_aec_hdl *aec_hdl = NULL;
#else
struct audio_aec_hdl aec_handle;
struct audio_aec_hdl *aec_hdl = &aec_handle;
#endif/*AEC_USER_MALLOC_ENABLE*/

int audio_cvp_probe_param_update(struct audio_cvp_pre_param_t *cfg)
{
    if (aec_hdl) {
        aec_hdl->pre = *cfg;
    }
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Process_Probe
* Description: AEC模块数据前处理回调(预处理)
* Arguments  : talk_mic     主麦数据地址
*			   talk_ref_mic	副麦数据地址
*			   fb_mic	    FB麦数据地址
*			   ref	参考数据地址
*			   len	数据长度(Unit:byte)
* Return	 : 0 成功 其他 失败
* Note(s)    : 在源数据经过CVP模块前，可以增加自定义处理
*********************************************************************
*/
static LOUDNESS_M_STRUCT mic_loudness;
static int audio_aec_probe(short *talk_mic, short *talk_ref_mic, short *fb_mic, short *ref, u16 len)
{
#if TCFG_AUDIO_MIC_ARRAY_TRIM_ENABLE
    if (app_var.audio_mic_array_trim_en) {
        //表示使用主副麦差值计算，且仅减小增益
        if (app_var.audio_mic_array_diff_cmp != 1.0f) {
            if (app_var.audio_mic_array_diff_cmp > 1.0f) {
                float talk_mic_gain = 1.0 / app_var.audio_mic_array_diff_cmp;
                GainProcess_16Bit(talk_mic, talk_mic, talk_mic_gain, 1, 1, 1, len >> 1);
            } else {
                float talk_ref_mic_gain = app_var.audio_mic_array_diff_cmp;
                GainProcess_16Bit(talk_ref_mic, talk_ref_mic, talk_ref_mic_gain, 1, 1, 1, len >> 1);
            }
        } else {	//表示使用每个MIC与金机曲线的差值
            GainProcess_16Bit(talk_mic, talk_mic, app_var.audio_mic_cmp.talk, 1, 1, 1, len >> 1);
            GainProcess_16Bit(talk_ref_mic, talk_ref_mic, app_var.audio_mic_cmp.ff, 1, 1, 1, len >> 1);
            GainProcess_16Bit(fb_mic, fb_mic, app_var.audio_mic_cmp.fb, 1, 1, 1, len >> 1);
        }
    }
#endif

    if (aec_hdl->pre.pre_gain_en) {
        GainProcess_16Bit(talk_mic, talk_mic, aec_hdl->pre.talk_mic_gain, 1, 1, 1, len >> 1);
        GainProcess_16Bit(talk_ref_mic, talk_ref_mic, aec_hdl->pre.talk_ref_mic_gain, 1, 1, 1, len >> 1);
    }
    if (aec_hdl->dcc_hdl) {
        audio_dc_offset_remove_run(aec_hdl->dcc_hdl, (void *)talk_mic, len);
    }

#if CVP_LOUDNESS_TRACE_ENABLE
    loudness_meter_short(&mic_loudness, talk_mic, len >> 1);
#endif/*CVP_LOUDNESS_TRACE_ENABLE*/
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Process_Post
* Description: AEC模块数据后处理回调
* Arguments  : data 数据地址
*			   len	数据长度
* Return	 : 0 成功 其他 失败
* Note(s)    : 在数据处理完毕，可以增加自定义后处理
*********************************************************************
*/
static int audio_aec_post(s16 *data, u16 len)
{
    return 0;
}


/*跟踪系统内存使用情况:physics memory size xxxx bytes*/
static void sys_memory_trace(void)
{
    static int cnt = 0;
    if (cnt++ > 200) {
        cnt = 0;
        mem_stats();
    }
}

int audio_aec_sync_buffer_set(s16 *data, int len)
{
    return cvp_node_output_handle(data, len);
}
/*
*********************************************************************
*                  Audio AEC Output Handle
* Description: AEC模块数据输出回调
* Arguments  : data 输出数据地址
*			   len	输出数据长度
* Return	 : 数据输出消耗长度
* Note(s)    : None.
*********************************************************************
*/
extern void esco_enc_resume(void);
static int audio_aec_output(s16 *data, u16 len)
{
#if (((defined TCFG_KWS_VOICE_RECOGNITION_ENABLE) && TCFG_KWS_VOICE_RECOGNITION_ENABLE) || \
     ((defined TCFG_CALL_KWS_SWITCH_ENABLE) && TCFG_CALL_KWS_SWITCH_ENABLE))
    //Voice Recognition get mic data here
    extern void kws_aec_data_output(void *priv, s16 * data, int len);
    kws_aec_data_output(NULL, data, len);

#endif/*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

#if CVP_MEM_TRACE_ENABLE
    sys_memory_trace();
#endif/*CVP_MEM_TRACE_ENABLE*/

    if (aec_hdl->dump_packet) {
        aec_hdl->dump_packet--;
        memset(data, 0, len);
    } else  {
        if (aec_hdl->output_fade_in) {
            s32 tmp_data;
            //printf("fade:%d\n",aec_hdl->output_fade_in_gain);
            for (int i = 0; i < len / 2; i++) {
                tmp_data = data[i];
                data[i] = tmp_data * aec_hdl->output_fade_in_gain >> 7;
            }
            aec_hdl->output_fade_in_gain += 12;
            if (aec_hdl->output_fade_in_gain >= 128) {
                aec_hdl->output_fade_in = 0;
            }
        }
    }
#if TCFG_AUDIO_CVP_SYNC
    audio_cvp_sync_run(data, len);
    return len;
#endif/*TCFG_AUDIO_CVP_SYNC*/
    return cvp_node_output_handle(data, len);
}

#define AUDIO_3MIC_PARAM_FILE 	SDFILE_RES_ROOT_PATH"3mic_coeff.bin"
void *read_triple_mic_param()
{
    if (aec_hdl == NULL) {
        return NULL;
    }
    FILE *fp = NULL;
    u8 *param_ptr = NULL;
    u32 param_len = 0;
    struct vfs_attr param_attr = {0};
    //===============================//
    //          打开参数文件         //
    //===============================//
    fp = fopen(AUDIO_3MIC_PARAM_FILE, "r");
    if (!fp) {
        printf("[err] read 3mic_coeff.bin fail !!!");
        return NULL;
    }
    fget_attrs(fp, &param_attr);

    param_len = param_attr.fsize;
    printf("param_len %d", param_len);
    param_ptr = (void *)param_attr.sclust;

    fclose(fp);

    aec_hdl->TransferFunc = zalloc(param_len);
    if (aec_hdl->TransferFunc == NULL) {
        return NULL;
    }
    memcpy(aec_hdl->TransferFunc, param_ptr, param_len);
    return aec_hdl->TransferFunc;
}

/*
*********************************************************************
*                  Audio AEC Parameters
* Description: AEC模块配置参数
* Arguments  : p	参数指针
* Return	 : None.
* Note(s)    : 读取配置文件成功，则使用配置文件的参数配置，否则使用默
*			   认参数配置
*********************************************************************
*/
static void audio_aec_param_init(struct tms_attr *p)
{
    int ret = 0;
    AEC_TMS_CONFIG cfg;
    //读取工具配置参数+预处理参数
    ret = cvp_node_param_cfg_read(&cfg, 0);
#if TCFG_AEC_TOOL_ONLINE_ENABLE
    ret = aec_cfg_online_update_fill(&cfg, sizeof(AEC_TMS_CONFIG));
#endif/*TCFG_AEC_TOOL_ONLINE_ENABLE*/

    if (ret == sizeof(AEC_TMS_CONFIG)) {
        log_info("read tms_param ok\n");
        p->EnableBit = cfg.enable_module;
        p->ul_eq_en = cfg.ul_eq_en;
        p->agc_type = cfg.agc_type;
        if (p->agc_type == AGC_EXTERNAL) {
            p->AGC_NDT_fade_in_step = cfg.agc.agc_ext.ndt_fade_in;
            p->AGC_NDT_fade_out_step = cfg.agc.agc_ext.ndt_fade_out;
            p->AGC_DT_fade_in_step = cfg.agc.agc_ext.dt_fade_in;
            p->AGC_DT_fade_out_step = cfg.agc.agc_ext.dt_fade_out;
            p->AGC_NDT_max_gain = cfg.agc.agc_ext.ndt_max_gain;
            p->AGC_NDT_min_gain = cfg.agc.agc_ext.ndt_min_gain;
            p->AGC_NDT_speech_thr = cfg.agc.agc_ext.ndt_speech_thr;
            p->AGC_DT_max_gain = cfg.agc.agc_ext.dt_max_gain;
            p->AGC_DT_min_gain = cfg.agc.agc_ext.dt_min_gain;
            p->AGC_DT_speech_thr = cfg.agc.agc_ext.dt_speech_thr;
            p->AGC_echo_present_thr = cfg.agc.agc_ext.echo_present_thr;
        } else {
            p->min_mag_db_level = cfg.agc.agc_int.min_mag_db_level;
            p->max_mag_db_level = cfg.agc.agc_int.max_mag_db_level;
            p->addition_mag_db_level = cfg.agc.agc_int.addition_mag_db_level;
            p->clip_mag_db_level = cfg.agc.agc_int.clip_mag_db_level;
            p->floor_mag_db_level = cfg.agc.agc_int.floor_mag_db_level;
        }

        p->aec_process_maxfrequency = cfg.aec_process_maxfrequency;
        p->aec_process_minfrequency = cfg.aec_process_minfrequency;
        p->af_length = cfg.af_length;

        p->nlp_process_maxfrequency = cfg.nlp_process_maxfrequency;
        p->nlp_process_minfrequency = cfg.nlp_process_minfrequency;
        p->overdrive = cfg.overdrive;

        p->aggressfactor = cfg.aggressfactor;
        p->minsuppress = cfg.minsuppress;
        p->init_noise_lvl = cfg.init_noise_lvl;
        p->DNS_highGain = 1.0f; /*EQ强度, 范围：1.0f~3.5f,越大越强*/
        p->DNS_rbRate = 0.1f;   /*混响强度，范围：0~0.9f,越大越强*/
        p->enhance_flag = 1;

        p->FB_EnableBit = AEC_EN | NLP_EN;
        p->Tri_CutTh = 2000;        //fb麦统计截止频率
        p->Tri_SnrThreshold0 = 0.5f;//sir设定阈值
        p->Tri_SnrThreshold1 = 0.5f;//sir设定阈值
        p->Tri_FbAlignedDb = 0.0f;	//fb和主副麦之间需要补偿增益
        p->Tri_FbCompenDb = 0.0f;//fb补偿增益
        p->Tri_TfEqSel = 0;//eq,传递函数的选择：0选择eq，1选择传递函数，2选择传递函数和eq
        p->enc_process_maxfreq = cfg.enc_process_maxfreq;
        p->enc_process_minfreq = cfg.enc_process_minfreq;
        p->sir_maxfreq = cfg.sir_maxfreq;
        p->mic_distance = cfg.mic_distance;
        p->target_signal_degradation = cfg.target_signal_degradation;
        p->enc_aggressfactor = cfg.enc_aggressfactor;
        p->enc_minsuppress = cfg.enc_minsuppress;
        p->Tri_CompenDb = 17;       //mic增益补偿, dB
        p->Tri_Bf_Enhance = 0;      //bf是否高频增强
        p->Tri_LowFreqHoldEn = 0;
        p->Tri_SupressFactor = 0.6f;

        p->wn_msc_th = cfg.wn_msc_th;
        p->ms_th = cfg.ms_th;
        p->wn_gain_offset = cfg.wn_gain_offset;

        p->adc_ref_en = cfg.adc_ref_en;

        p->output_sel = cfg.output_sel;

    } else {
        p->EnableBit = WNC_EN | AEC_EN | ANS_EN | ENC_EN | AGC_EN;
        p->ul_eq_en = 1;
        p->agc_type = AGC_INTERNAL;
        p->AGC_NDT_fade_in_step = 1.3f;
        p->AGC_NDT_fade_out_step = 0.9f;
        p->AGC_DT_fade_in_step = 1.3f;
        p->AGC_DT_fade_out_step = 0.9f;
        p->AGC_NDT_max_gain = 12.f;
        p->AGC_NDT_min_gain = 0.f;
        p->AGC_NDT_speech_thr = -50.f;
        p->AGC_DT_max_gain = 12.f;
        p->AGC_DT_min_gain = 0.f;
        p->AGC_DT_speech_thr = -40.f;
        p->AGC_echo_present_thr = -70.f;

        p->min_mag_db_level = -50;       //语音能量放大下限阈值（单位db,默认-50，范围（-90db~-35db））
        p->max_mag_db_level = -3;       //语音能量放大上限阈值（单位db,默认-3，范围（-90db~0db））
        p->addition_mag_db_level = 0;   //语音补偿能量值（单位db,默认0，范围（0db~20db））
        p->clip_mag_db_level = -3;      //语音最大截断能量值（单位db,默认-3，范围（-10db~db））
        p->floor_mag_db_level = -70;    //语音最小截断能量值（单位db,默认-70，范围（-90db~-35db）

        p->aec_process_maxfrequency = 8000;
        p->aec_process_minfrequency = 0;
        p->af_length = 128;

        p->nlp_process_maxfrequency = 8000;
        p->nlp_process_minfrequency = 0;
        p->overdrive = 1;

        p->aggressfactor = 1.0f;
        p->minsuppress = 0.05f;
        p->init_noise_lvl = -75.f;
        p->DNS_highGain = 1.0f; /*EQ强度, 范围：1.0f~3.5f,越大越强*/
        p->DNS_rbRate = 0.01f;   /*混响强度，范围：0~0.9f,越大越强*/
        p->enhance_flag = 1;

        p->FB_EnableBit = AEC_EN | NLP_EN;
        p->Tri_CutTh = 2000;        //fb麦统计截止频率
        p->Tri_SnrThreshold0 = 0.5f;//sir设定阈值
        p->Tri_SnrThreshold1 = 0.5f;//sir设定阈值
        p->Tri_FbAlignedDb = 0.0f;	//fb和主副麦之间需要补偿增益
        p->Tri_FbCompenDb = 0.0f;//fb补偿增益
        p->Tri_TfEqSel = 0;//eq,传递函数的选择：0选择eq，1选择传递函数，2选择传递函数和eq
        p->enc_process_maxfreq = 8000;
        p->enc_process_minfreq = 0;
        p->sir_maxfreq = 3000;
        p->mic_distance = 0.025f;
        p->target_signal_degradation = 0.78;
        p->enc_aggressfactor = 1.f;
        p->enc_minsuppress = 0.05f;
        p->Tri_CompenDb = 17;       //mic增益补偿, dB
        p->Tri_Bf_Enhance = 0;      //bf是否高频增强
        p->Tri_LowFreqHoldEn = 0;
        p->Tri_SupressFactor = 0.6f;

        p->wn_msc_th = 0.6f;
        p->ms_th = 80.0f;
        p->wn_gain_offset = 5;

        p->adc_ref_en = 0;

        p->output_sel = DMS_OUTPUT_SEL_DEFAULT;
        log_error("read tms_param default\n");
    }
    log_info("TMS:WNC[%d] AEC[%d] NLP[%d] NS[%d] ENC[%d] AGC[%d]", !!(p->EnableBit & WNC_EN), !!(p->EnableBit & AEC_EN), !!(p->EnableBit & NLP_EN), !!(p->EnableBit & ANS_EN), !!(p->EnableBit & ENC_EN), !!(p->EnableBit & AGC_EN));

    p->Tri_TransferFunc = read_triple_mic_param();
    if (p->Tri_TransferFunc) {
        printf("3mic_coeff read ok %x", (int)p->Tri_TransferFunc);
    }

#if TCFG_AUDIO_MIC_ARRAY_TRIM_ENABLE
    app_var.enc_degradation = p->target_signal_degradation;
#endif

    p->AGC_echo_hold = 0;
    p->AGC_echo_look_ahead = 0;

    /* aec_param_dump(p); */
}

/*
*********************************************************************
*                  Audio AEC Open
* Description: 初始化AEC模块
* Arguments  : sr 			采样率(8000/16000)
*			   enablebit	使能模块(AEC/NLP/AGC/ANS...)
*			   out_hdl		自定义回调函数，NULL则用默认的回调
* Return	 : 0 成功 其他 失败
* Note(s)    : 该接口是对audio_aec_init的扩展，支持自定义使能模块以及
*			   数据输出回调函数
*********************************************************************
*/
int audio_aec_open(struct audio_aec_init_param_t *init_param, s16 enablebit, int (*out_hdl)(s16 *data, u16 len))
{
    s16 sample_rate = init_param->sample_rate;
    u16 ref_sr = init_param->ref_sr;
    struct tms_attr *aec_param;
    printf("audio_tms_open\n");
    mem_stats();
#if AEC_USER_MALLOC_ENABLE
    aec_hdl = zalloc(sizeof(struct audio_aec_hdl));
    if (aec_hdl == NULL) {
        log_error("aec_hdl malloc failed");
        return -ENOMEM;
    }
#endif/*AEC_USER_MALLOC_ENABLE*/

#if TCFG_AUDIO_CVP_SYNC
    audio_cvp_sync_open(sample_rate);
#endif/*TCFG_AUDIO_CVP_SYNC*/

    overlay_load_code(OVERLAY_AEC);

#if CVP_LOUDNESS_TRACE_ENABLE
    loudness_meter_init(&mic_loudness, sample_rate, 50, 0);
#endif/*CVP_LOUDNESS_TRACE_ENABLE*/

    aec_hdl->dump_packet = AEC_OUT_DUMP_PACKET;
    aec_hdl->inbuf_clear_cnt = AEC_IN_DUMP_PACKET;
    aec_hdl->output_fade_in = 1;
    aec_hdl->output_fade_in_gain = 0;
    aec_param = &aec_hdl->attr;
    aec_param->aec_probe = audio_aec_probe;
    aec_param->aec_post = audio_aec_post;
    aec_param->output_handle = audio_aec_output;

    if (ref_sr) {
        aec_param->ref_sr  = ref_sr;
    } else {
        aec_param->ref_sr  = usb_mic_is_running();
    }
    if (aec_param->ref_sr == 0) {
        if (TCFG_ESCO_DL_CVSD_SR_USE_16K && (sample_rate == 8000)) {
            aec_param->ref_sr = 16000;	//CVSD 下行为16K
        } else {
            aec_param->ref_sr = sample_rate;
        }
    }

    audio_aec_param_init(aec_param);

    if (enablebit >= 0) {
        aec_param->EnableBit = enablebit;
    }
    if (out_hdl) {
        aec_param->output_handle = out_hdl;
    }

#if TCFG_AEC_SIMPLEX
    aec_param->wn_en = 1;
    aec_param->EnableBit = AEC_MODE_SIMPLEX;
    if (sr == 8000) {
        aec_param->SimplexTail = aec_param->SimplexTail / 2;
    }
#else
    aec_param->wn_en = 0;
#endif/*TCFG_AEC_SIMPLEX*/

    if (sample_rate == 16000) { //WideBand宽带
        aec_param->wideband = 1;
        aec_param->hw_delay_offset = 60;
    } else {//NarrowBand窄带
        aec_param->wideband = 0;
        aec_param->hw_delay_offset = 55;
    }

    if (audio_adc_file_get_mic_mode(0) == AUDIO_MIC_CAPLESS_MODE) {
        aec_hdl->dcc_hdl = audio_dc_offset_remove_open(sample_rate, 1);
    }

    //aec_param_dump(aec_param);
    aec_hdl->EnableBit = aec_param->EnableBit;
    aec_param->aptfilt_only = 0;
#if (((defined TCFG_KWS_VOICE_RECOGNITION_ENABLE) && TCFG_KWS_VOICE_RECOGNITION_ENABLE) || \
     ((defined TCFG_CALL_KWS_SWITCH_ENABLE) && TCFG_CALL_KWS_SWITCH_ENABLE))
    extern u8 kws_get_state(void);
    if (kws_get_state()) {
        aec_param->EnableBit = AEC_EN;
        aec_param->aptfilt_only = 1;
        printf("kws open,aec_enablebit=%x", aec_param->EnableBit);
        //临时关闭aec, 对比测试
        //aec_param->toggle = 0;
    }
#endif/*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

    y_printf("[aec_user]aec_open\n");
#if CVP_TOGGLE
    int ret = aec_open(aec_param);
    if (ret == -EPERM) {
        //该芯片不支持双mic ENC功能，请选择大于等于8Mbit容量的芯片
        printf("Chip not support ENC(tms+aec):Please select >= 8Mbit flash!");
    }
    //ASSERT(ret != -EPERM, "Chip not support dms+aec mode!!");
#endif
    aec_hdl->start = 1;
    mem_stats();
    printf("audio_tms_open succ\n");
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Init
* Description: 初始化AEC模块
* Arguments  : sample_rate 采样率(8000/16000)
* Return	 : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_aec_init(struct audio_aec_init_param_t *init_param)
{
    return audio_aec_open(init_param, -1, NULL);
}

/*
*********************************************************************
*                  Audio AEC Reboot
* Description: AEC模块复位接口
* Arguments  : reduce 复位/恢复标志
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_aec_reboot(u8 reduce)
{
    if (aec_hdl) {
        printf("audio_aec_tms_reboot:%x,%x,start:%d", aec_hdl->EnableBit, aec_hdl->attr.EnableBit, aec_hdl->start);
        if (aec_hdl->start) {
            if (reduce) {
                aec_hdl->attr.EnableBit = AEC_EN;
                aec_hdl->attr.aptfilt_only = 1;
                aec_tms_reboot(aec_hdl->attr.EnableBit);
            } else {
                if (aec_hdl->EnableBit != aec_hdl->attr.EnableBit) {
                    aec_hdl->attr.aptfilt_only = 0;
                    aec_tms_reboot(aec_hdl->EnableBit);
                }
            }
        }
    } else {
        printf("audio_aec close now\n");
    }
}

/*
*********************************************************************
*                  Audio AEC Output Select
* Description: 输出选择
* Arguments  : sel = DMS_OUTPUT_SEL_DEFAULT 默认输出算法处理结果
*				   = DMS_OUTPUT_SEL_MASTER	输出主mic(通话mic)的原始数据
*				   = DMS_OUTPUT_SEL_SLAVE	输出副mic(降噪mic)的原始数据
*				   = DMS_OUTPUT_SEL_FBMIC	输出FBmic(降噪mic)的原始数据
*			   agc 输出数据要不要经过agc自动增益控制模块
* Return	 : None.
* Note(s)    : 可以通过选择不同的输出，来测试mic的频响和ENC指标
*********************************************************************
*/
void audio_aec_output_sel(CVP_OUTPUT_ENUM sel, u8 agc)
{
    if (aec_hdl)	{
        printf("tms_output_sel:%d\n", sel);
        if (agc) {
            aec_hdl->attr.EnableBit |= AGC_EN;
        } else {
            aec_hdl->attr.EnableBit &= ~AGC_EN;
        }
        aec_hdl->attr.output_sel = sel;
    }
}

/*
*********************************************************************
*                  Audio AEC Close
* Description: 关闭AEC模块
* Arguments  : None.
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_aec_close(void)
{
    printf("audio_aec_close:%x", (u32)aec_hdl);
    if (aec_hdl) {
        aec_hdl->start = 0;
#if CVP_TOGGLE
        aec_close();
#endif

#if TCFG_AUDIO_CVP_SYNC
        //在AEC关闭之后再关，否则还会跑cvp_sync_run,导致越界
        audio_cvp_sync_close();
#endif/*TCFG_AUDIO_CVP_SYNC*/

        if (aec_hdl->dcc_hdl) {
            audio_dc_offset_remove_close(aec_hdl->dcc_hdl);
            aec_hdl->dcc_hdl = NULL;
        }

        if (aec_hdl->TransferFunc) {
            free(aec_hdl->TransferFunc);
            aec_hdl->TransferFunc = NULL;
        }
        local_irq_disable();
#if AEC_USER_MALLOC_ENABLE
        free(aec_hdl);
#endif/*AEC_USER_MALLOC_ENABLE*/
        aec_hdl = NULL;
        local_irq_enable();
    }
}

/*
*********************************************************************
*                  Audio AEC Status
* Description: AEC模块当前状态
* Arguments  : None.
* Return	 : 0 关闭 其他 打开
* Note(s)    : None.
*********************************************************************
*/
u8 audio_aec_status(void)
{
    if (aec_hdl) {
        return aec_hdl->start;
    }
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Input
* Description: AEC源数据输入
* Arguments  : buf	输入源数据地址
*			   len	输入源数据长度
* Return	 : None.
* Note(s)    : 输入一帧数据，唤醒一次运行任务处理数据，默认帧长256点
*********************************************************************
*/
void audio_aec_inbuf(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
        if (aec_hdl->input_clear) {
            memset(buf, 0, len);
        }
#if CVP_TOGGLE
        if (aec_hdl->inbuf_clear_cnt) {
            aec_hdl->inbuf_clear_cnt--;
            memset(buf, 0, len);
        }
        int ret = aec_in_data(buf, len);
        if (ret == -1) {
        } else if (ret == -2) {
            log_error("aec inbuf full\n");
        }
#else
        aec_hdl->arrt.output_handle(buf, len);
#endif/*CVP_TOGGLE*/
    }
}

/*
*********************************************************************
*                  Audio AEC Input Reference
* Description: AEC源参考数据输入
* Arguments  : buf	输入源数据地址
*			   len	输入源数据长度
* Return	 : None.
* Note(s)    : 双mic ENC的参考mic数据输入,单mic的无须调用该接口
*********************************************************************
*/
void audio_aec_inbuf_ref(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
        aec_in_data_ref(buf, len);
    }
}

void audio_aec_inbuf_ref_1(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
        aec_in_data_ref_1(buf, len);
    }
}

/*
*********************************************************************
*                  Audio AEC Reference
* Description: AEC模块参考数据输入
* Arguments  : buf	输入参考数据地址
*			   len	输入参考数据长度
* Return	 : None.
* Note(s)    : 声卡设备是DAC，默认不用外部提供参考数据
*********************************************************************
*/
void audio_aec_refbuf(s16 *data0, s16 *data1, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
#if CVP_TOGGLE
        aec_ref_data(data0, data1, len);
#endif/*CVP_TOGGLE*/
    }
}


/*
*********************************************************************
*                  			Audio CVP IOCTL
* Description: CVP功能配置
* Arguments  : cmd 		操作命令
*		       value 	操作数
*		       priv 	操作内存地址
* Return	 : 0 成功 其他 失败
* Note(s)    : (1)比如动态开关降噪NS模块:
*				audio_cvp_ioctl(CVP_NS_SWITCH,1,NULL);	//降噪关
*				audio_cvp_ioctl(CVP_NS_SWITCH,0,NULL);  //降噪开
*********************************************************************
*/
int audio_cvp_ioctl(int cmd, int value, void *priv)
{
    if (aec_hdl && aec_hdl->start) {
        return aec_dms_ioctl(cmd, value, priv);
    } else {
        return -1;
    }
}

/*
*********************************************************************
*                  Audio CVP Toggle Set
* Description: CVP模块算法开关使能
* Arguments  : toggle 0 关闭算法 1 打开算法
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int audio_cvp_toggle_set(u8 toggle)
{
    if (aec_hdl) {
        aec_tms_toggle(toggle);
        return 0;
    }
    return 1;
}

/*获取风噪的检测结果，1：有风噪，0：无风噪*/
int audio_cvp_dms_wnc_state(void)
{
    int state = 0;
    if (aec_hdl) {
        state = cvp_tms_get_wind_detect_state();
        printf("wnc state : %d", state);
    } else {
        state = -1;
    }
    return state;
}

/*
 * 获取风噪检测信息
 * wd_flag: 0 没有风噪，1 有风噪
 * 风噪强度r: 0~BIT(16)
 * wd_lev: 风噪等级，0：弱风，1：中风，2：强风
 * */
int audio_cvp_get_wind_detect_info(int *wd_flag, int *wd_val, int *wd_lev)
{
    if (aec_hdl && aec_hdl->start) {
        return jlsp_tms_get_wind_detect_info(wd_flag, wd_val, wd_lev);
    }
    return -1;
}

/*tri_en: 1 正常3MIC降噪
 *        0 变成2MIC降噪，不使用 FB MIC数据*/
int audio_tms_mode_choose(u8 tri_en)
{
    if (aec_hdl && aec_hdl->start) {
        return jlsp_tms_mode_choose(tri_en);
    }
    return -1;
}

/**
 * 以下为用户层扩展接口
 */
//pbg profile use it,don't delete
void aec_input_clear_enable(u8 enable)
{
    if (aec_hdl) {
        aec_hdl->input_clear = enable;
        log_info("aec_input_clear_enable= %d\n", enable);
    }
}

#endif/*TCFG_AUDIO_TRIPLE_MIC_ENABLE == 1*/
#endif /*TCFG_CVP_DEVELOP_ENABLE */
