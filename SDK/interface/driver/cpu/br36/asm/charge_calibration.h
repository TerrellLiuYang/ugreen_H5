#ifndef _CHARGE_CALIBRATION_H_
#define _CHARGE_CALIBRATION_H_

#include "typedef.h"

//电流校准的返回值
#define CALIBRATION_SUCC        0   //正常
#define CALIBRATION_VBG_ERR     1   //VBG档位不够导致错误
#define CALIBRATION_VBAT_ERR    2   //VBAT满电档位不够导致错误
#define CALIBRATION_MA_ERR      3   //电流设置太小,无法调整档位了
#define CALIBRATION_FAIL        4   //调整不到设置范围
#define CALIBRATION_CONTINUE    0xFF

typedef struct _calibration_result_ {
    u8 vbg_lev;
    u8 curr_lev;
    u8 vbat_lev;
    u8 hv_mode;
} calibration_result;

calibration_result charge_calibration_get_result(void);
void charge_calibration_set_current_limit(u32 max_current, u32 min_current);
u8 charge_calibration_report_current(u32 current);

#endif    //_CHARGE_CALIBRATION_H_

