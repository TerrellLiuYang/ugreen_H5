#ifndef  __GPADC_HW_H__
#define  __GPADC_HW_H__
//br28
#include "generic/typedef.h"
#define ADC_CH_MASK_TYPE_SEL	0xffff0000
#define ADC_CH_MASK_CH_SEL	    0x0000ffff

#define ADC_CH_TYPE_LPCTM  	(0xc<<16)
#define ADC_CH_TYPE_AUDIO  	(0xd<<16)
#define ADC_CH_TYPE_PMU    	(0xe<<16)
#define ADC_CH_TYPE_BT     	(0xf<<16)
#define ADC_CH_TYPE_IO		(0x10<<16)

#define ADC_CH_LPCTM_		(ADC_CH_TYPE_LPCTM | 0x0)
#define ADC_CH_AUDIO_		(ADC_CH_TYPE_AUDIO)
#define ADC_CH_PMU_VBG  	(ADC_CH_TYPE_PMU | 0x0)//MVBG/WVBG
#define ADC_CH_PMU_VSW  	(ADC_CH_TYPE_PMU | 0x1)
#define ADC_CH_PMU_PROGI	(ADC_CH_TYPE_PMU | 0x2)
#define ADC_CH_PMU_PROGF	(ADC_CH_TYPE_PMU | 0x3)
#define ADC_CH_PMU_VTEMP	(ADC_CH_TYPE_PMU | 0x4)
#define ADC_CH_PMU_LDO5V 	(ADC_CH_TYPE_PMU | 0x5) //1/4vpwr
#define ADC_CH_PMU_VBAT 	(ADC_CH_TYPE_PMU | 0x6)  //1/4vbat
#define ADC_CH_PMU_VBAT_2	(ADC_CH_TYPE_PMU | 0x7)
#define ADC_CH_PMU_VB17  	(ADC_CH_TYPE_PMU | 0x8)
#define ADC_CH_PMU_PVDD_PROB (ADC_CH_TYPE_PMU | 0x9)
#define ADC_CH_PMU_VDC15	(ADC_CH_TYPE_PMU | 0xa)
#define ADC_CH_PMU_DVDD		(ADC_CH_TYPE_PMU | 0xb)
#define ADC_CH_PMU_RVDD  	(ADC_CH_TYPE_PMU | 0xc)
#define ADC_CH_PMU_TWVD  	(ADC_CH_TYPE_PMU | 0xd)
#define ADC_CH_PMU_PVDD  	(ADC_CH_TYPE_PMU | 0xe)
#define ADC_CH_PMU_EVDD  	(ADC_CH_TYPE_PMU | 0xf)
#define ADC_CH_BT_			(ADC_CH_TYPE_BT | 0x0)
#define ADC_CH_IO_PA3  	(ADC_CH_TYPE_IO | 0x0)
#define ADC_CH_IO_PA5   (ADC_CH_TYPE_IO | 0x1)
#define ADC_CH_IO_PA6   (ADC_CH_TYPE_IO | 0x2)
#define ADC_CH_IO_PA8   (ADC_CH_TYPE_IO | 0x3)
#define ADC_CH_IO_PC4   (ADC_CH_TYPE_IO | 0x4)
#define ADC_CH_IO_PC5   (ADC_CH_TYPE_IO | 0x5)
#define ADC_CH_IO_PB1   (ADC_CH_TYPE_IO | 0x6)
#define ADC_CH_IO_PB2   (ADC_CH_TYPE_IO | 0x7)
#define ADC_CH_IO_PB4   (ADC_CH_TYPE_IO | 0x8)
#define ADC_CH_IO_PB6   (ADC_CH_TYPE_IO | 0x9)
#define ADC_CH_IO_DP   	(ADC_CH_TYPE_IO | 0xA)
#define ADC_CH_IO_DM   	(ADC_CH_TYPE_IO | 0xB)

#include "asm/voltage.h"
#define     ADC_VBG_CENTER  (801 + voltage_api_get_mvbg_diff())
#define     ADC_VBG_TRIM_STEP     3   //
#define     ADC_VBG_DATA_WIDTH    4

enum AD_CH {
    AD_CH_LPCTM = ADC_CH_LPCTM_,
    AD_CH_AUDIO = ADC_CH_AUDIO_,
    AD_CH_PMU_VBG = ADC_CH_PMU_VBG,
    AD_CH_PMU_VSW,
    AD_CH_PMU_PROGI,
    AD_CH_PMU_PROGF,
    AD_CH_PMU_VTEMP,
    AD_CH_PMU_LDO5V,
    AD_CH_PMU_VBAT, // 1/4 VBAT
    AD_CH_PMU_VBAT_2, // 1/2 VBAT  SDK中禁止使用
    AD_CH_PMU_VB17,
    AD_CH_PMU_PVDD_PROB,
    AD_CH_PMU_VDC15,
    AD_CH_PMU_DVDD,
    AD_CH_PMU_RVDD,
    AD_CH_PMU_TWVD,
    AD_CH_PMU_PVDD,
    AD_CH_PMU_EVDD,
    AD_CH_BT = ADC_CH_BT_,
    AD_CH_IO_PA3 = ADC_CH_IO_PA3,
    AD_CH_IO_PA5,
    AD_CH_IO_PA6,
    AD_CH_IO_PA8,
    AD_CH_IO_PC4,
    AD_CH_IO_PC5,
    AD_CH_IO_PB1,
    AD_CH_IO_PB2,
    AD_CH_IO_PB4,
    AD_CH_IO_PB6,
    AD_CH_IO_DP,
    AD_CH_IO_DM,

    AD_CH_IOVDD = 0xffffffff,
};

#define AD_CH_LDOREF    AD_CH_PMU_VBG


extern void adc_pmu_ch_select(u32 ch);
extern u32 efuse_get_gpadc_vbg_trim();


#endif  /*GPADC_HW_H*/


