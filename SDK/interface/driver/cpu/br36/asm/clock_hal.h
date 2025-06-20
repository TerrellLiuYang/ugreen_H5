#ifndef  __CLOCK_HAL_H__
#define  __CLOCK_HAL_H__




#include "typedef.h"

enum CLK_OUT_SOURCE0 {
    CLK_OUT_SRC0_NONE = 0,
    CLK_OUT_SRC0_SFC_CLK,
    CLK_OUT_SRC0_HSB_CLK,
    CLK_OUT_SRC0_LSB_CLK,
    CLK_OUT_SRC0_STD_48M,
    CLK_OUT_SRC0_STD_24M,
    CLK_OUT_SRC0_RC_16M,
    CLK_OUT_SRC0_LRC_CLK,
    CLK_OUT_SRC0_RTC_OSC,
    CLK_OUT_SRC0_BTOSC_24M,
    CLK_OUT_SRC0_BTOSC_48M,
    CLK_OUT_SRC0_XOSC_FSCL,
    CLK_OUT_SRC0_P33_RCLK,
    CLK_OUT_SRC0_PLL_ALINK0_CLK,
    CLK_OUT_SRC0_PLL_96M,
    CLK_OUT_SRC0_PLL_75M,
};

void clk_out0(u8 gpio, enum CLK_OUT_SOURCE0 clk);
void clk_out1(u8 gpio, enum CLK_OUT_SOURCE0 clk);
void clk_out2(u8 gpio, enum CLK_OUT_SOURCE0 clk);
void clk_out3(u8 gpio, enum CLK_OUT_SOURCE0 clk);

void clk_out0_close(u8 gpio);
void clk_out1_close(u8 gpio);
void clk_out2_close(u8 gpio);
void clk_out3_close(u8 gpio);


enum CLK_OUT_SOURCE {
    //ch0,ch1,ch2,ch3.  no div
    CLK_OUT_NONE = 0x0f00,
    CLK_OUT_SFC_CLK,
    CLK_OUT_HSB_CLK,
    CLK_OUT_LSB_CLK,
    CLK_OUT_STD_48M,
    CLK_OUT_STD_24M,
    CLK_OUT_RC_16M,
    CLK_OUT_LRC_CLK,
    CLK_OUT_RTC_OSC,
    CLK_OUT_BTOSC_24M,
    CLK_OUT_BTOSC_48M,
    CLK_OUT_XOSC_FSCL,
    CLK_OUT_P33_RCLK,
    CLK_OUT_PLL_ALINK0_CLK,
    CLK_OUT_PLL_96M,
    CLK_OUT_PLL_75M,
};

#define CLK_OUT_CH_MASK             0x0000000f
#define CLK_OUT_CH0_SEL(clk)        SFR(JL_CLOCK->CLK_CON0,16,4,clk)
#define CLK_OUT_CH0_GET_CLK()       ((JL_CLOCK->CLK_CON0>>16)&0x0f)
#define CLK_OUT_CH0_DIV(div)        //no div
#define CLK_OUT_CH0_EN(en)         //no en
#define CLK_OUT0_FIXED_IO_EN(en)   //no en
#define IS_CLK_OUT0_FIXED_IO()     (0)//no fix

#define CLK_OUT_CH1_SEL(clk)        SFR(JL_CLOCK->CLK_CON0,20,4,clk)
#define CLK_OUT_CH1_GET_CLK()       ((JL_CLOCK->CLK_CON0>>20)&0x0f)
#define CLK_OUT_CH1_DIV(div)        //no div
#define CLK_OUT_CH1_EN(en)         //no en
#define CLK_OUT1_FIXED_IO_EN(en)   //no en
#define IS_CLK_OUT1_FIXED_IO()     (0)//no fix

#define CLK_OUT_CH2_SEL(clk)        SFR(JL_CLOCK->CLK_CON0,24,4,clk)
#define CLK_OUT_CH2_GET_CLK()       ((JL_CLOCK->CLK_CON0>>24)&0x0f)
#define CLK_OUT_CH2_DIV(div)        //no div
#define CLK_OUT_CH2_EN(en)         //no en
#define CLK_OUT2_FIXED_IO_EN(en)   //no en
#define IS_CLK_OUT2_FIXED_IO()     (0)//no fix

#define CLK_OUT_CH3_SEL(clk)        SFR(JL_CLOCK->CLK_CON0,28,4,clk)
#define CLK_OUT_CH3_GET_CLK()       ((JL_CLOCK->CLK_CON0>>28)&0x0f)
#define CLK_OUT_CH3_DIV(div)        //no div
#define CLK_OUT_CH3_EN(en)         //no en
#define CLK_OUT3_FIXED_IO_EN(en)   //no en
#define IS_CLK_OUT3_FIXED_IO()     (0)//no fix

#define CLK_OUT_CH4_SEL(clk)        //no ch4
#define CLK_OUT_CH4_GET_CLK()       (0)//no ch4
#define CLK_OUT_CH4_DIV(div)        //no div
#define CLK_OUT_CH4_EN(en)         //no en
#define CLK_OUT4_FIXED_IO_EN(en)   //no en
#define IS_CLK_OUT4_FIXED_IO()     (0)//no fix
u32 clk_out_fixed_io_check(u32 gpio);








//for bt
void clk_set_osc_cap(u8 sel_l, u8 sel_r);
u32 clk_get_osc_cap();

#define BT_CLOCK_IN(x)          SFR(JL_CLOCK->CLK_CON1,  14,  2,  x)
//for MACRO - BT_CLOCK_IN
enum {
    BT_CLOCK_IN_STD_48M = 0,
    BT_CLOCK_IN_OSC,
    BT_CLOCK_IN_HSB,
    BT_CLOCK_IN_DISABLE,
};

#endif  /*CLOCK_HAL_H*/
