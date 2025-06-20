#ifndef _CHANNEL_EXPANDER__H
#define _CHANNEL_EXPANDER__H

#include "audio_splicing.h"

typedef struct _CH_EXPANDER_PARM {
    u8 slience; //rl rr声道是否复制数据输出，1 不输出，0 输出
} CH_EXPANDER_PARM;

typedef struct __ChannelExpanderParam_TOOL_SET__ {
    CH_EXPANDER_PARM parm;
} channel_expander_param_tool_set;

typedef struct _channel_expander_hdl_ {
    CH_EXPANDER_PARM parm;
    u8 update;
} channel_expander_hdl;

/*声道扩展打开*/
channel_expander_hdl *audio_channel_expander_open(CH_EXPANDER_PARM *parm);

/*声道扩展关闭*/
void audio_channel_expander_close(channel_expander_hdl *hdl);

/*声道扩展参数更新*/
void audio_channel_expander_update_parm(channel_expander_hdl *hdl, CH_EXPANDER_PARM *parm);

/*声道扩展数据处理*/
int audio_channel_adapter_run(channel_expander_hdl *hdl, short *out, short *in, int len, u8 out_ch_num, u8 in_ch_num);

#endif
