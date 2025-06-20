#ifndef _AUDIO_VBASS_API_H_
#define _AUDIO_VBASS_API_H_
#include "system/includes.h"
#include "media/audio_stream.h"
#include "effects/VirtualBass_api.h"

typedef struct _VirtualBassUdateParam {
    int ratio;
    int boost;
    int fc;
} VirtualBassUdateParam;

//虚拟低音
typedef struct _VirtualBass_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    VirtualBassUdateParam parm;
} virtual_bass_param_tool_set;


typedef struct _vbass_hdl {
    void *workbuf;           //vbass 运行句柄及buf
    VirtualBassParam parm;
    u8 status;                           //内部运行状态机
    u8 update;                           //设置参数更新标志
    u8 out32bit;
} vbass_hdl;


int audio_vbass_run(vbass_hdl *hdl, void *datain, void *dataout, u32 len);
vbass_hdl *audio_vbass_open(VirtualBassParam *parm);
int audio_vbass_close(vbass_hdl *hdl);
void audio_vbass_set_bit_wide(vbass_hdl *hdl, u32 out32bit);

int audio_vbass_update_parm(vbass_hdl *hdl, VirtualBassUdateParam *parm);
void audio_vbass_bypass(vbass_hdl *hdl, u8 bypass);

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

#endif

