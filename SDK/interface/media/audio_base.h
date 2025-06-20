#ifndef AUDIO_BASE_H
#define AUDIO_BASE_H

#include "generic/typedef.h"


#define AUDIO_CODING_UNKNOW       0x00000000
#define AUDIO_CODING_MP3          0x00000001
#define AUDIO_CODING_WMA          0x00000002
#define AUDIO_CODING_WAV          0x00000004
#define AUDIO_CODING_SBC          0x00000010
#define AUDIO_CODING_MSBC         0x00000020
#define AUDIO_CODING_G729         0x00000040
#define AUDIO_CODING_CVSD         0x00000080
#define AUDIO_CODING_PCM          0x00000100
#define AUDIO_CODING_AAC          0x00000200
#define AUDIO_CODING_MTY          0x00000400
#define AUDIO_CODING_FLAC         0x00000800
#define AUDIO_CODING_APE          0x00001000
#define AUDIO_CODING_M4A          0x00002000
#define AUDIO_CODING_AMR          0x00004000
#define AUDIO_CODING_DTS          0x00008000
#define AUDIO_CODING_APTX         0x00010000
#define AUDIO_CODING_LDAC         0x00020000
#define AUDIO_CODING_G726         0x00040000
#define AUDIO_CODING_MIDI         0x00080000
#define AUDIO_CODING_OPUS         0x00100000
#define AUDIO_CODING_SPEEX        0x00200000
#define AUDIO_CODING_LC3          0x00400000
#define AUDIO_CODING_WTGV2        0x01000000
#define AUDIO_CODING_ALAC         0x02000000
#define AUDIO_CODING_SINE         0x04000000
#define AUDIO_CODING_F2A          0x08000000
#define AUDIO_CODING_AIFF         0x10000000

#define AUDIO_CODING_STU_PICK     0x10000000
#define AUDIO_CODING_STU_APP      0x20000000


#define AUDIO_INPUT_FILE          0x01
#define AUDIO_INPUT_FRAME         0x02


enum audio_channel {
    AUDIO_CH_L          = (1 << 4) | 1,           	//左声道（单声道）
    AUDIO_CH_R          = (1 << 4) | 2,           	//右声道（单声道）
    AUDIO_CH_DIFF       = (1 << 4) | 3,        	//差分（单声道）
    AUDIO_CH_MIX        = (1 << 4) | 4,           //左右声道混合(单声道)
    AUDIO_CH_LR         = (2 << 4) | 5,       	//立体声
    AUDIO_CH_DUAL_L     = (2 << 4) | 6,  		//双声道都为左
    AUDIO_CH_DUAL_R     = (2 << 4) | 7,  		//双声道都为右
    AUDIO_CH_DUAL_LR    = (2 << 4) | 8,  		//双声道为左右混合
    AUDIO_CH_QUAD       = (4 << 4) | 9,  		//四声道（LRLR）

    AUDIO_CH_MAX = 0xff,
};

#define AUDIO_CH_NUM(ch) ((ch) >> 4)


/*! \brief      音频处理结构 */
struct audio_fmt {
    u8  channel;        /*!<  */
    u16  frame_len;      /*!<  幁长度 (bytes)*/
    u16 sample_rate;    /*!<  采样率 e.g. 48kHz/44.1kHz/32kHz*/
    u32 coding_type;
    u32 bit_rate;       /*!<  比特率 (bps)*/
    u32 total_time;
    u32 quality;
    u32 complexity;
    void *priv;
};
















#endif

