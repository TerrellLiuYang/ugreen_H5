/*
 ******************************************************************************************
 *							Audio Config
 *
 * Discription: 音频模块与芯片系列相关配置
 *
 * Notes:
 ******************************************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_config.h"

/*
 *******************************************************************
 *						Audio Codec Config
 *******************************************************************
 */
//***********************
//*		AAC Codec       *
//***********************
const int AAC_DEC_MP4A_LATM_ANALYSIS = 1;


//***********************
//*		Audio Params    *
//***********************
void audio_adc_param_fill(struct mic_open_param *mic_param, struct adc_file_param *param)
{
    mic_param->mic_mode      = param->mic_mode;
    mic_param->mic_ain_sel   = param->mic_ain_sel;
    mic_param->mic_bias_sel  = param->mic_bias_sel;
    mic_param->mic_bias_rsel = param->mic_bias_rsel;
    mic_param->mic_dcc       = param->mic_dcc;
}
