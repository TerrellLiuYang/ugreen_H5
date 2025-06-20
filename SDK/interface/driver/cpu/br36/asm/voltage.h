#ifndef __VOLTAGE_H__
#define __VOLTAGE_H__

#include "typedef.h"

#define VDDIO_LEV_DIFF  200 //VDDIO一个档位200mV
#define VDDIO_LEV_MAX   8   //一共8个档位

#define RVDD_LEV_DIFF   30  //RVDD一个档位30mV
#define RVDD_LEV_MAX    16  //一共16个档位

#define DVDD_LEV_DIFF   30  //RVDD一个档位30mV
#define DVDD_LEV_MAX    16  //一共16个档位

#define VDC13_LEV_DIFF  50  //RVDD一个档位50mV
#define VDC13_LEV_MAX   13  //一共13个档位

#define MVBG_LEV_DIFF   15  //RVDD一个档位15mV
#define MVBG_LEV_MAX    16  //一共16个档位

#define MVBG_DEF_VOLT   800 //校准后的MVBG电压

/*----------------------------------------------------------------------------*/
/*
 * @brief 设置主VBG的档位
 * @param  lev:目标档位值
 * @return 返回默认的VBG档位
 * @note  该api主要是提供给电流校准使用,改变了MVBG值内部会根据VBG改变其他系统
 *        电压的档位
 */
/*-----------------------------------------------------------------------------*/
s8 voltage_api_set_mvbg(u8 lev);

/*----------------------------------------------------------------------------*/
/*
 * @brief  设置vdc13 dvdd rvdd vddio电压档位,
 * @param  vdc13_lev:vdc13目标档位值,传入0xff表示不改变
 * @param  dvdd_lev:dvdd目标档位值,传入0xff表示不改变
 * @param  rvdd_lev:rvdd目标档位值,传入0xff表示不改变
 * @param  vddio_lev:vddio目标档位值,传入0xff表示不改变
 * @return NULL
 * @note  该api主要是提供给时钟系统调用,内部会根据VBG档位对应修改
 */
/*-----------------------------------------------------------------------------*/
void voltage_api_set_lev(u8 vdc13_lev, u8 dvdd_lev, u8 rvdd_lev, u8 vddio_lev);

/*----------------------------------------------------------------------------*/
/*
 * @brief  获取当前MVBG调整的压差
 * @param  NULL
 * @return MVBG调整的压差,负值表示VBG相对800mV减小了,正值表示VBG相对800mV增大了
 * @note  该api主要是提供给adc计算实际电压使用
 */
/*-----------------------------------------------------------------------------*/
s32 voltage_api_get_mvbg_diff(void);

#endif

