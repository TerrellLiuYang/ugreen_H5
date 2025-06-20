/**@file  		power_reset.c
* @brief    	复位相关接口
* @details
* @author
* @date     	2021-8-26
* @version  	V1.0
* @copyright    Copyright:(c)JIELI  2011-2020  @ , All Rights Reserved.
 */

#ifndef __POWER_RESET_H__
#define __POWER_RESET_H__

/*复位原因*/
enum RST_REASON {
    /*主系统*/
    MSYS_P11_RST,
    MSYS_DVDD_POR_RST,
    MSYS_SOFT_RST,
    MSYS_P2M_RST,
    MSYS_POWER_RETURN,
    /*P11*/
    P11_PVDD_POR_RST,
    P11_IVS_RST,
    P11_P33_RST,
    P11_WDT_RST,
    P11_SOFT_RST,
    P11_MSYS_RST = 10,
    P11_POWER_RETURN,
    /*P33*/
    P33_VDDIO_POR_RST,
    P33_VDDIO_LVD_RST,
    P33_VCM_RST,
    P33_PPINR_RST,
    P33_P11_RST,
    P33_SOFT_RST,
    P33_PPINR1_RST,
    P33_POWER_RETURN,
    /*SUB*/
    P33_EXCEPTION_SOFT_RST = 20,
    P33_ASSERT_SOFT_RST,
};

void power_reset_source_dump(void);

void p33_soft_reset(void);

u8 is_reset_source(enum RST_REASON index);

#endif
