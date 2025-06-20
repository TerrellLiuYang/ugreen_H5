/*********************************************************************************************
    *   Filename        : lib_driver_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-18 14:58

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "app_config.h"
#include "system/includes.h"
#include "media/includes.h"
#include "asm/audio_adc.h"
#include "audio_config.h"
#include "media/audio_def.h"


/*
 *******************************************************************
 *						Audio Common Config
 *******************************************************************
 */
const int global_bit_wide  = BIT_WIDE_16BIT;//输出设备位宽配置，音效算法位宽饱和处理相关 0:16bit 1:24bit

#if OS_CPU_CORE > 1
const int CONFIG_JLSTREAM_MULTI_THREAD_ENABLE = 1; //音频流多线程使能
#else
const int CONFIG_JLSTREAM_MULTI_THREAD_ENABLE = 0;
#endif


/*
 *******************************************************************
 *						Audio Hardware Config
 *******************************************************************
 */

//***********************
//*		Audio ADC       *
//***********************
/*省电容mic使能配置*/
#ifndef TCFG_SUPPORT_MIC_CAPLESS
#define TCFG_SUPPORT_MIC_CAPLESS	0
#endif/*TCFG_SUPPORT_MIC_CAPLESS*/
const u8 const_mic_capless_en = TCFG_SUPPORT_MIC_CAPLESS;
/*是否支持多个ADC 异步打开功能*/
const u8 const_adc_async_en = 0;

//***********************
//*		Audio DAC       *
//***********************
//<DAC FIFO Mixer>
/* #if TCFG_MIC_EFFECT_ENABLE */
/* const int config_audio_dac_mix_enable = 1; */
/* #else */
/* const int config_audio_dac_mix_enable = 0; */
/* #endif */

//<DAC NoiseGate>
#if TCFG_MIC_EFFECT_ENABLE
const int config_audio_dac_noisefloor_optimize_enable = 0;
#else
const int config_audio_dac_noisefloor_optimize_enable = BIT(1);
#endif


//
const unsigned char config_audio_dac_underrun_protect = 1;


//<DAC trim>
#if ((SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW) || (SYS_VOL_TYPE == VOL_TYPE_DIGITAL))
const char config_audio_dac_trim_enable = 1;
#else
const char config_audio_dac_trim_enable = 1;
#endif


#if TCFG_LOWPOWER_LOWPOWER_SEL
const int config_audio_dac_delay_off_ms = 300;
#else
const int config_audio_dac_delay_off_ms = 0;
#endif

//
const int config_audio_dac_vcm_chsel_delay = 1500;

//AUTO MUTE
#if AUDIO_OUTPUT_AUTOMUTE
const int config_automute_enable = 1;
#else
const int config_automute_enable = 0;
#endif

//***********************
//*		FFT && CORDIC   *
//***********************
enum {
    PLATFORM_FREQSHIFT_CORDIC = 0,
    PLATFORM_FREQSHIFT_CORDICV2 = 1
};


/* 备注:  */
/* br23/br25/br30/br34/br40是PLATFORM_FREQSHIFT_CORDIC  */
/* ， br36/br28/br27是PLATFORM_FREQSHIFT_CORDICV2 */

#if (CONFIG_CPU_BR28 || CONFIG_CPU_BR27 || CONFIG_CPU_BR36)
#define  PLATFORM_PARM_SEL  PLATFORM_FREQSHIFT_CORDICV2
#else
#define  PLATFORM_PARM_SEL PLATFORM_FREQSHIFT_CORDIC
#endif

#if (CONFIG_CPU_BR28 || CONFIG_CPU_BR27)
#define  PLATFORM_PARM_FFT_SEL  PLATFORM_FREQSHIFT_CORDICV2
#else
#define  PLATFORM_PARM_FFT_SEL PLATFORM_FREQSHIFT_CORDIC
#endif


/*
 *******************************************************************
 *						Audio Stream Config
 *******************************************************************
 */
#if (TCFG_AUDIO_DECODER_OCCUPY_TRACE)
const u8 audio_decoder_occupy_trace_enable = 1;
const u8 audio_decoder_occupy_trace_dump = 0;
#else
const u8 audio_decoder_occupy_trace_enable = 0;
const u8 audio_decoder_occupy_trace_dump = 0;
#endif/*TCFG_AUDIO_DECODER_OCCUPY_TRACE*/

// 混音器声音突出使能
const char config_audio_mixer_ch_highlight_enable = 0;


// tws音频解码自动设置输出声道。
// 单声道：AUDIO_CH_L/AUDIO_CH_R。双声道：AUDIO_CH_DUAL_L/AUDIO_CH_DUAL_R
// 关闭后，按照output_ch_num和output_ch_type/ch_type设置输出声道
const int audio_tws_auto_channel = 1;

// mixer在单独任务中输出
#if TCFG_MIXER_CYCLIC_TASK_EN
const int config_mixer_task = 1;
#else
const int config_mixer_task = 0;
#endif

#ifdef CONFIG_256K_FLASH
// mixer模块使能。不使能将关闭大部分功能，mix为直通
const int config_mixer_en = 0;
// mixer变采样使能
const int config_mixer_src_en = 0;

// audio解码资源叠加功能使能。不使能，如果配置了叠加方式，将改成抢占方式
const int config_audio_dec_wait_protect_en = 0;

// audio数据流分支功能使能。
const int config_audio_stream_frame_copy_en = 0;

// audio dec app调用mixer相关函数控制。关闭后需上层设置数据流的输出节点
const int audio_dec_app_mix_en = 0;

#else

// mixer模块使能。不使能将关闭大部分功能，mix为直通
const int config_mixer_en = 1;
// mixer变采样使能
const int config_mixer_src_en = 1;

// audio解码资源叠加功能使能。不使能，如果配置了叠加方式，将改成抢占方式
const int config_audio_dec_wait_protect_en = 1;

// audio数据流分支功能使能。
const int config_audio_stream_frame_copy_en = 1;

// audio dec app调用mixer相关函数控制。关闭后需上层设置数据流的输出节点
const int audio_dec_app_mix_en = 1;

#endif

// audio数据流分支cbuf大小控制
const int config_audio_stream_frame_copy_cbuf_min = 128;
const int config_audio_stream_frame_copy_cbuf_max = 1024;

// 超时等待其他解码unactive步骤完成
const int config_audio_dec_unactive_to = 0;
// audio数据流ioctrl使能
const int config_audio_stream_frame_ioctrl_en = 0;

// audio dec app tws同步使能
const int audio_dec_app_sync_en = 0;

// 解码使用单独任务做输出
#if TCFG_AUDIO_DEC_OUT_TASK
const int config_audio_dec_out_task_en = 1;
#else
const int config_audio_dec_out_task_en = 0;
#endif

#ifdef CONFIG_256K_FLASH
const char config_audio_mini_enable = 1;
#else
const char config_audio_mini_enable = 0;
#endif

/*
 *******************************************************************
 *						Audio Codec Config
 *******************************************************************
 */

//重复播放
#if FILE_DEC_REPEAT_EN
const u8 file_dec_repeat_en = 1;
#else
const u8 file_dec_repeat_en = 0;
#endif
//指定位置播放
#if FILE_DEC_DEST_PLAY
const u8 file_dec_dest_play = 1;
#else
const u8 file_dec_dest_play = 0;
#endif

//***********************
//*	   common Codec     *
//***********************
const int CONFIG_DEC_SUPPORT_BITWIDTH = 16;  	 //  24bit 或者 16bit
const int CONFIG_DEC_SUPPORT_CHANNELS = 2;   	 //  支持的最大声道数
#ifndef AUDIO_DEC_MAX_SAMPERATE
#define  AUDIO_DEC_MAX_SAMPERATE       48000
#endif
const int CONFIG_DEC_SUPPORT_SAMPLERATE = AUDIO_DEC_MAX_SAMPERATE; //  支持的最大采样率

//***********************
//*		AAC Codec       *
//***********************

const int CONFIG_DEC_SUPPORT_DAB_AAC = 0; //AAC 文件的dab+ 类 解码支持，每帧读960进行解码,使用m4a的解码库;
//***********************
//*		MP3 Codec       *
//***********************
const int MP3_SEARCH_MAX = 200; //本地解码设成200， 网络解码可以设成3
const int MP3_TGF_TWS_EN = 1;   //tws解码使能
const int MP3_TGF_POSPLAY_EN = 1; //定点播放 获取ms级别时间 接口使能
const int MP3_TGF_AB_EN = 1;   //AB点复读使能
const int MP3_TGF_FASTMO = 0;  //快速解码使能【默认关闭，之前给一个sdk单独加的，配置是否解高频，解双声道等】
// 解码一次输出点数，1代表32对点，n就是n*32对点
// 超过1时，解码需要使用malloc，如config_mp3_dec_use_malloc=1
const int MP3_OUTPUT_LEN = 1;

#define FAST_FREQ_restrict				0x01 //限制超过16k的频率不解【一般超出人耳听力范围，但是仪器会测出来】
#define FAST_FILTER_restrict			0x02 //限制滤波器长度【子带滤波器旁瓣加大，边缘不够陡】
#define FAST_CHANNEL_restrict			0x04 //混合左右声道，再解码【如果是左右声道独立性较强的歌曲，会牺牲空间感，特别耳机听会听出来的话】
const int config_mp3_dec_speed_mode 	=  0;//FAST_FREQ_restrict | FAST_FILTER_restrict | FAST_CHANNEL_restrict; //3个开关都置上，是最快的解码模式

const int CONFIG_MUSIC_FILE_BUF_SIZE = 4 * 1024;

const int const_audio_mp3_dec16_fifo_precision = 16;  //  24 或者 16
//***********************
//*		WAV Codec       *
//***********************
const int WAV_MAX_BITRATEV = (48 * 2 * 32); // wav最大支持比特率，单位kbps
// 解码一次输出点数,建议范围32到900,例如128代表128对点
// 超过128时，解码需要使用malloc，如config_wav_dec_use_malloc=1
const int  WAV_DECODER_PCM_POINTS = 128;

// output超过128时，如果不使用malloc，需要增大对应buf
// 可以看打印中解码器需要的大小，一般输出每增加1长度增加4个字节
int wav_mem_ext[(1336  + 3) / 4] SEC(.wav_mem); //超过128要增加这个数组的大小

//<ADPCM Type>
const int const_sel_adpcm_type = 0;//1：使用imaen_adpcm,  0:msen_adpcm

//***********************
//*		M4A Codec       *
//***********************

const int const_audio_m4a_dec16_fifo_precision = 16;  //  24 或者 16

//***********************
//*		WMA Codec       *
//***********************
const int WMA_TWSDEC_EN = 0;   // wma tws 解码控制
// 解码一次输出点数，1代表32对点，n就是n*32对点
// 超过1时，解码需要使用malloc，如config_mp3_dec_use_malloc=1
const int WMA_OUTPUT_LEN = 1;

const int const_audio_wma_dec16_fifo_precision = 16;  //  24 或者 16
//***********************
//*		OPUS Codec      *
//***********************
const int OPUS_SRINDEX = 0; //选择opus解码文件的帧大小，0代表一帧40字节，1代表一帧80字节，2代表一帧160字节

//***********************
//*		APE Codec      *
//***********************
const u32 APE_DEC_SUPPORT_LEVEL = 1;    //最高支持的层数  0:Fast   1:Normal    2:High

//***********************
//* 	tone Codec      *
//***********************
/* f2a解码常量设置 */
const int F2A_JUST_VOL = 0;  //帧长是否不超过512，置1的话，需要的buf会变小。默认置0，buf为11872.

const int WTGV2_STACK2BUF = 0;  //等于1时解码buf会加大760，栈会减小

//wts解码支持采样率可选择，可以同时打开也可以单独打开
const  int  silk_fsN_enable = 1;   //支持8-12k采样率
const  int  silk_fsW_enable = 1;  //支持16-24k采样率

//***********************
//* 	LC3 Codec      *
//***********************
const int LC3_SUPPORT_CH = 2;     //lc3解码输入通道数 1：单声道输入， 2:双声道输入(br30可支持2)
const int LC3_DMS_VAL = 25;      //单位ms, 【只支持 25,50,100】
const int LC3_DMS_FSINDEX = 4;    //配置采样率【只支持0到4】，影响用哪组表以及一次的处理长度(<=8k的时候，配0. <=16k的时候，配1.<=24k的时候，配2.<=32k的时候，配3.<=48k的时候，配4)
const int LC3_QUALTIY_CONFIG = 4;//【范围1到4， 1需要的速度最少，这个默认先配4】

//***********************
//* 	MIDI Codec      *
//***********************
#ifdef CONFIG_MIDI_DEC_ADDR
const int MIDI_TONE_MODE = 0;//0是地址访问(仅支持在内置flash,读数快，消耗mips低)，1 是文件访问(内置、外挂flash,sd,u盘均可,读数慢，消耗mips较大)
#else
const int MIDI_TONE_MODE = 1;
#endif
const int MAINTRACK_USE_CHN = 0;	//MAINTRACK_USE_CHN控制主通道区分方式；   0用track号区分主通道 1用chn号区分主通道
const int MAX_PLAYER_CNT = 18;     //控制可配置的最大同时发声的key数的BUF[1,32]

//***********************
//* 	 SBC Codec      *
//***********************
#ifdef SBC_CUSTOM_DECODER_BUF_SIZE
const short config_sbc_decoder_buf_size = 512;
#endif

//***********************
//* 	 MSBC Codec      *
//***********************
const int config_msbc_decode_input_frame_replace = CONFIG_MSBC_INPUT_FRAME_REPLACE_DISABLE;

#if TCFG_BT_DONGLE_ENABLE //使用JL_DONGLE 时开启msbc软件编码
const int config_msbc_encode_used_software_enable = 1;
#else
const int config_msbc_encode_used_software_enable = 0;
#endif
/*
 *******************************************************************
 *						Audio Music Playback Config
 *******************************************************************
 */
// 快进快退到文件end返回结束消息
const int config_decoder_ff_fr_end_return_event_end = 0;

/*
 *******************************************************************
 *						Audio Effects Config
 *******************************************************************
 */


//***********************
//* 	 EQ             *
//***********************
#ifdef TCFG_SPEAKER_EQ_NODE_ENABLE
#if EQ_SECTION_MAX < 10
#undef EQ_SECTION_MAX
#define EQ_SECTION_MAX 10
#endif
#endif

const int AUDIO_EQ_MAX_SECTION = EQ_SECTION_MAX;

#ifdef TCFG_EQ_ENABLE
const int config_audio_eq_en = EQ_EN
#ifdef TCFG_CROSSOVER_NODE_ENABLE
                               | EQ_HW_CROSSOVER_TYPE0_EN
#endif
                               ;
#else
const int config_audio_eq_en = 0;
#endif

const int const_eq_debug = 0;
const int AUDIO_EQ_CLEAR_MEM_BY_MUTE_TIME_MS = 0;//300 //连续多长时间静音就清除EQ MEM
const int AUDIO_EQ_CLEAR_MEM_BY_MUTE_LIMIT = 0; //静音判断阀值

//***********************
//* 	 PLC            *
//***********************
const int A2DP_AUDIO_PLC_ENABLE = 0;
/*
 * 播歌PLC仅淡入淡出配置,
 * 0 - 补包运算(Ram 3660bytes, Code 1268bytes)
 * 1 - 仅淡入淡出(Ram 688bytes, Code 500bytes)
 */
const int LPC_JUST_FADE = 0;

//影响plc申请的buf大小跟速度，这个值越大，申请的buf越多，速度也越快。
//增加的buf大小是  APLC_MOV_STAKLEN *类型(16bit是 sizeof(short), 32bit 是sizeof(int))
const int APLC_MOV_STAKLEN = 1024;

//***********************
//*   Howling Suppress  *
//***********************
const int howling_freshift_PLATFORM = PLATFORM_PARM_SEL;
const int howling_freshift_highmode_flag = 0;              //移频快速模式
const int howling_pitchshift_fastmode_flag   = 1;//移频啸叫抑制快速模式使能

//***********************
//*   Voice Changer     *
//***********************
const int vc_pitchshift_fastmode_flag        = 1; //变声快速模式使能
const  int  vc_pitchshift_only = 0;
//变声库数学函数版本配置，br23/br25/br30/br34/br40是PLATFORM_VOICECHANGE_CORDIC，br27/br28/br36是PLATFORM_VOICECHANGE_CORDICV2
const  int  voicechange_mathfun_PLATFORM = PLATFORM_PARM_SEL;

//***********************
//*   	ECHO            *
//***********************
const int ECHO_INT_VAL_OUT = 0;       //  置1: echo的输出是int 后级需接DRC限幅 功能未实现
#ifndef AEF_ECHO_DELAY_MAX
#define AEF_ECHO_DELAY_MAX	200
#endif
const int auido_echo_delay_ms  = AEF_ECHO_DELAY_MAX;//回声最大延时(单位ms),值越大RAM需求就越大

#if(CONFIG_CPU_BR25)
const  int DOWN_S_FLAG 				= 1; //混响降采样处理使能
#else
const  int DOWN_S_FLAG 				= 0; //混响降采样处理使能
#endif

//***********************
//*   	Reverb          *
//***********************
const int PLATE_REVERB_ROOM_SIZE_Mutiplier = 2; // 影响了plateReverb的nee_buf的大小( 约等于 33k * PLATE_REVERB_ROOM_SIZE_Mutiplier)，对应的是roomsize=100对应的是多大

//***********************
//*   	Others          *
//***********************
const  int RS_FAST_MODE_QUALITY = 2;	//软件变采样 滤波阶数配置，范围2到8， 8代表16阶的变采样模式 ,速度跟它的大小呈正相关


/*
 *******************************************************************
 *						Audio Smart Voice Config
 *******************************************************************
 */
/*杰理KWS关键词识别*/
#if TCFG_SMART_VOICE_ENABLE
const int CONFIG_KWS_WAKE_WORD_MODEL_ENABLE = 0;
const int CONFIG_KWS_MULTI_CMD_MODEL_ENABLE = 1;
#if TCFG_CALL_KWS_SWITCH_ENABLE
const int CONFIG_KWS_CALL_CMD_MODEL_ENABLE  = 1;
#else
const int CONFIG_KWS_CALL_CMD_MODEL_ENABLE  = 0;
#endif

/*参数位置选择
 * 0 : 全部放在flash
 * 1 : 全部放在ram
 * 2 : 一部分放在flash，一部分放在ram
 * */
#ifdef CONFIG_CPU_BR36
const int CONFIG_KWS_RAM_USE_ENABLE = 2;
#else
const int CONFIG_KWS_RAM_USE_ENABLE = 1;
#endif
const int CONFIG_KWS_ONLINE_MODEL = 1;
#if (TCFG_AUDIO_KWS_LANGUAGE_SEL == KWS_CH)
const float CONFIG_KWS_MULTI_CONFIDENCE[] = {
    0.4, 0.4, 0.4, 0.4, //小杰小杰，小杰同学，播放音乐，停止播放
    0.4, 0.4, 0.4, 0.4, //暂停播放，增大音量，减小音量，上一首
    0.4, 0.4, 0.4, 0.4, //下一首，打开降噪，关闭降噪，打开通透
};

const float CONFIG_KWS_CALL_CONFIDENCE[] = {
    0.4, 0.4, //接听电话，挂断电话
};
#elif (TCFG_AUDIO_KWS_LANGUAGE_SEL == KWS_FAR_CH)
const float CONFIG_KWS_MULTI_CONFIDENCE[] = {
    0.58, 0.5, 0.5, 0.5, //小杰小杰，小杰同学，播放音乐，停止播放
    0.47, 0.45, 0.47, 0.51, //暂停播放，增大音量，减小音量，上一首
    0.55, 0.4, 0.4, 0.4, //下一首，打开降噪，关闭降噪，打开通透
};

const float CONFIG_KWS_CALL_CONFIDENCE[] = {
    0.4, 0.4, //接听电话，挂断电话
};
#else /*TCFG_AUDIO_KWS_LANGUAGE_SEL == KWS_INDIA_EN*/
const float CONFIG_KWS_MULTI_CONFIDENCE[] = {
    0.4, 0.4, 0.3, 0.4, //小杰小杰，小杰同学，播放音乐，停止播放
    0.4, 0.4, 0.3, 0.4, //暂停播放，增大音量，减小音量，上一首
    0.3, 0.4, 0.4, 0.4, //下一首，打开降噪，关闭降噪，打开通透
};

const float CONFIG_KWS_CALL_CONFIDENCE[] = {
    0.4, 0.3, //接听电话，挂断电话
};
#endif /*TCFG_AUDIO_KWS_LANGUAGE_SEL*/
#else /*TCFG_SMART_VOICE_ENABLE == 0*/
const int CONFIG_KWS_WAKE_WORD_MODEL_ENABLE = 0;
const int CONFIG_KWS_MULTI_CMD_MODEL_ENABLE = 0;
const int CONFIG_KWS_CALL_CMD_MODEL_ENABLE  = 0;
const int CONFIG_KWS_ONLINE_MODEL = 0;
#endif /*TCFG_SMART_VOICE_ENABLE*/

#if (defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
const int config_resample_pcm_frame_buffer = 512;
#else
const int config_resample_pcm_frame_buffer = 256;
#endif

/*
 *******************************************************************
 *						Audio Memory Alloc Config
 *******************************************************************
 */
#if CONFIG_MEDIA_LIB_USE_MALLOC
const int config_mp3_dec_use_malloc     = 1;
const int config_mp3pick_dec_use_malloc = 1;
const int config_wma_dec_use_malloc     = 1;
const int config_wmapick_dec_use_malloc = 1;
const int config_m4a_dec_use_malloc     = 1;
const int config_m4apick_dec_use_malloc = 1;
const int config_wav_dec_use_malloc     = 1;
const int config_alac_dec_use_malloc    = 1;
const int config_dts_dec_use_malloc     = 1;
const int config_amr_dec_use_malloc     = 1;
const int config_flac_dec_use_malloc    = 1;
const int config_ape_dec_use_malloc     = 1;
const int config_aac_dec_use_malloc     = 1;
const int config_aptx_dec_use_malloc    = 1;
const int config_midi_dec_use_malloc    = 1;
const int config_lc3_dec_use_malloc     = 1;
const int config_speex_dec_use_malloc   = 1;
const int config_opus_dec_use_malloc   = 1;
#else
const int config_mp3_dec_use_malloc     = 0;
const int config_mp3pick_dec_use_malloc = 0;
const int config_wma_dec_use_malloc     = 0;
const int config_wmapick_dec_use_malloc = 0;
const int config_m4a_dec_use_malloc     = 0;
const int config_m4apick_dec_use_malloc = 0;
const int config_wav_dec_use_malloc     = 0;
const int config_alac_dec_use_malloc    = 0;
const int config_dts_dec_use_malloc     = 0;
const int config_amr_dec_use_malloc     = 0;
const int config_flac_dec_use_malloc    = 0;
const int config_ape_dec_use_malloc     = 0;
const int config_aac_dec_use_malloc     = 0;
const int config_aptx_dec_use_malloc    = 0;
const int config_midi_dec_use_malloc    = 0;
const int config_lc3_dec_use_malloc     = 0;
const int config_speex_dec_use_malloc   = 0;
const int config_opus_dec_use_malloc   = 0;

#endif

/*
 *******************************************************************
 *						Audio Log Debug Config
 *
 * @brief Log (Verbose/Info/Debug/Warn/Error)
 *******************************************************************
 */
const char log_tag_const_v_DRC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_DRC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_DRC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_DRC  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_DRC  = CONFIG_DEBUG_LIB(TRUE);


const char log_tag_const_v_EQ  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_EQ  = CONFIG_DEBUG_LIB(const_eq_debug);
const char log_tag_const_d_EQ  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_EQ  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_EQ  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUD_ADC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUD_ADC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUD_ADC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_AUD_ADC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_AUD_ADC  = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_AUD_DAC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUD_DAC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUD_DAC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_AUD_DAC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_AUD_DAC  = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_APP_DAC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_APP_DAC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_APP_DAC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_APP_DAC  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_APP_DAC  = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_AUD_AUX  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUD_AUX  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUD_AUX  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_AUD_AUX  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_AUD_AUX  = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_MIXER  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_MIXER  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_MIXER  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_MIXER  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_MIXER  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_STREAM  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_AUDIO_STREAM  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUDIO_STREAM  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUDIO_STREAM  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_STREAM  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_DECODER  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_AUDIO_DECODER  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUDIO_DECODER  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUDIO_DECODER  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_DECODER  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_ENCODER  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_AUDIO_ENCODER  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUDIO_ENCODER  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUDIO_ENCODER  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_ENCODER  = CONFIG_DEBUG_LIB(TRUE);


const char log_tag_const_v_SYNCTS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_SYNCTS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_SYNCTS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_SYNCTS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_SYNCTS  = CONFIG_DEBUG_LIB(TRUE);


const char log_tag_const_v_EFFECTS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_EFFECTS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_EFFECTS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_EFFECTS  = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_EFFECTS  = CONFIG_DEBUG_LIB(TRUE);


/*
 *******************************************************************
 *						Audio Others Config
 *******************************************************************
 */
#ifdef CONFIG_ANC_30C_ENABLE
const char config_audio_30c_en = 1;
#else
const char config_audio_30c_en = 0;
#endif

//数字音量节点 是否使用汇编优化 不支持的芯片需置0
#if(CONFIG_CPU_BR18)
const int const_config_digvol_use_round = 0;
#else
const int const_config_digvol_use_round = 1;
#endif

// 解码任务测试
const int audio_decoder_test_en = 0;
// 当audio_decoder_test_en使能时需要实现以下接口
#if 0
void audio_decoder_test_out_before(struct audio_decoder *dec, void *buff, int len) {} ;
void audio_decoder_test_out_after(struct audio_decoder *dec, int wlen) {} ;
void audio_decoder_test_read_before(struct audio_decoder *dec, int len, u32 offset) {} ;
void audio_decoder_test_read_after(struct audio_decoder *dec, u8 *data, int rlen) {} ;
void audio_decoder_test_get_frame_before(struct audio_decoder *dec) {} ;
void audio_decoder_test_get_frame_after(struct audio_decoder *dec, u8 *frame, int rlen) {} ;
void audio_decoder_test_fetch_before(struct audio_decoder *dec) {} ;
void audio_decoder_test_fetch_after(struct audio_decoder *dec, u8 *frame, int rlen) {} ;
void audio_decoder_test_run_before(struct audio_decoder *dec) {} ;
void audio_decoder_test_run_after(struct audio_decoder *dec, int err) {} ;
#else
// 接口实现示例
//#include "common/demo/audio_decoder_test.c"
#endif


// 编码任务测试
const int audio_encoder_test_en = 0;
// 当audio_encoder_test_en使能时需要实现以下接口
#if 0
void audio_encoder_test_out_before(struct audio_encoder *enc, void *buff, int len) {} ;
void audio_encoder_test_out_after(struct audio_encoder *enc, int wlen) {} ;
void audio_encoder_test_get_frame_before(struct audio_encoder *enc, u16 frame_len) {} ;
void audio_encoder_test_get_frame_after(struct audio_encoder *enc, s16 *frame, int rlen) {} ;
void audio_encoder_test_run_before(struct audio_encoder *enc) {} ;
void audio_encoder_test_run_after(struct audio_encoder *enc, int err) {} ;
#else
// 接口实现示例
//#include "common/demo/audio_encoder_test.c"
#endif



/*********************Audio Config End************************/

