#ifndef reverb_api_h__
#define reverb_api_h__

#include "AudioEffect_DataType.h"

typedef  struct  _Plate_reverb_parm_ {
    int wet;                      //0-300%
    int dry;                      //0-200%
    int pre_delay;                 //0-40ms
    int highcutoff;                //0-20k 高频截止
    int diffusion;                  //0-100%
    int decayfactor;                //0-100%
    int highfrequencydamping;       //0-100%
    int modulate;                  // 0或1
    int roomsize;                   //20%-100%
    af_DataType dataTypeobj;
} Plate_reverb_parm;

typedef struct  _EF_REVERB0_FIX_PARM {
    unsigned int sr;
    unsigned int nch;//输入通道数
} EF_REVERB0_FIX_PARM;


typedef struct __PLATE_REVERB_FUNC_API_ {
    unsigned int (*need_buf)(Plate_reverb_parm *preverb_parm);
    int (*open)(unsigned int *ptr, Plate_reverb_parm *preverb_parm, EF_REVERB0_FIX_PARM *echo_fix_parm);
    int (*init)(unsigned int *ptr, Plate_reverb_parm *preverb_parm);
    int (*run)(unsigned int *ptr, short *inbuf, short *outdata, int len);
} PLATE_REVERB0_FUNC_API;

extern PLATE_REVERB0_FUNC_API *get_plate_reverb_func_api();
extern PLATE_REVERB0_FUNC_API *get_plate_reverb_adv_func_api();



#endif // reverb_api_h__
