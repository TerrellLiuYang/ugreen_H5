#ifndef __LP_IPC__
#define __LP_IPC__

//===========================================================================//
//
//                            P2M_MEM(跟p11同步)
//
//===========================================================================//
#define P2M_WKUP_SRC                P11_P2M_ACCESS(0)
#define P2M_WKUP_PND                P11_P2M_ACCESS(1)
#define P2M_WKUP_LEVEL              P11_P2M_ACCESS(2)

#define P2M_REPLY_COMMON_CMD        P11_P2M_ACCESS(4)
#define P2M_RTC_CMD					P11_P2M_ACCESS(5)
#define P2M_SOFTOFF					P11_P2M_ACCESS(6)

#define P2M_CTMU_KEY_EVENT          P11_P2M_ACCESS(8)
#define P2M_CTMU_KEY_CNT      	    P11_P2M_ACCESS(9)
#define P2M_CTMU_WKUP_MSG		    P11_P2M_ACCESS(10)
#define P2M_CTMU_EARTCH_EVENT	    P11_P2M_ACCESS(11)

#define P2M_MASSAGE_CTMU_CH0_L_RES                 12
#define P2M_MASSAGE_CTMU_CH0_H_RES                 13
#define P2M_CTMU_CH0_L_RES      	P11_P2M_ACCESS(12)
#define P2M_CTMU_CH0_H_RES      	P11_P2M_ACCESS(13)
#define P2M_CTMU_CH1_L_RES      	P11_P2M_ACCESS(14)
#define P2M_CTMU_CH1_H_RES      	P11_P2M_ACCESS(15)
#define P2M_CTMU_CH2_L_RES      	P11_P2M_ACCESS(16)
#define P2M_CTMU_CH2_H_RES      	P11_P2M_ACCESS(17)
#define P2M_CTMU_CH3_L_RES      	P11_P2M_ACCESS(18)
#define P2M_CTMU_CH3_H_RES      	P11_P2M_ACCESS(19)
#define P2M_CTMU_CH4_L_RES      	P11_P2M_ACCESS(20)
#define P2M_CTMU_CH4_H_RES      	P11_P2M_ACCESS(21)

#define P2M_CTMU_EARTCH_L_IIR_VALUE     P11_P2M_ACCESS(22)
#define P2M_CTMU_EARTCH_H_IIR_VALUE     P11_P2M_ACCESS(23)
#define P2M_CTMU_EARTCH_L_TRIM_VALUE    P11_P2M_ACCESS(24)
#define P2M_CTMU_EARTCH_H_TRIM_VALUE    P11_P2M_ACCESS(25)
#define P2M_CTMU_EARTCH_L_DIFF_VALUE    P11_P2M_ACCESS(26)
#define P2M_CTMU_EARTCH_H_DIFF_VALUE    P11_P2M_ACCESS(27)

//===========================================================================//
//
//                            M2P_MEM(跟P11同步)
//
//===========================================================================//
#define M2P_COMMON_CMD            						P11_M2P_ACCESS(0)
#define M2P_VDDIO_KEEP 	            					P11_M2P_ACCESS(1)
#define M2P_WDVDD                   					P11_M2P_ACCESS(2)

/*LRC相关配置*/
#define M2P_LRC_KEEP 	            					P11_M2P_ACCESS(8)
#define M2P_LRC_PRD                 		            P11_M2P_ACCESS(9)
#define M2P_LRC_TMR_50us  	        					P11_M2P_ACCESS(10)
#define M2P_LRC_TMR_200us 	        					P11_M2P_ACCESS(11)
#define M2P_LRC_TMR_600us 	        					P11_M2P_ACCESS(12)
#define M2P_LRC_FREQUENCY_RCL							P11_M2P_ACCESS(13)
#define M2P_LRC_FREQUENCY_RCH							P11_M2P_ACCESS(14)

/*RTC相关*/
#define M2P_RTC_KEEP									P11_M2P_ACCESS(16)
#define M2P_RTC_LRC_FEQL								P11_M2P_ACCESS(17)
#define M2P_RTC_LRC_FEQH								P11_M2P_ACCESS(18)
#define M2P_RTC_DAT0									P11_M2P_ACCESS(19)
#define M2P_RTC_DAT1									P11_M2P_ACCESS(20)
#define M2P_RTC_DAT2									P11_M2P_ACCESS(21)
#define M2P_RTC_DAT3									P11_M2P_ACCESS(22)
#define M2P_RTC_DAT4									P11_M2P_ACCESS(23)

#define M2P_RTC_ALARM0                                  P11_M2P_ACCESS(24)
#define M2P_RTC_ALARM1                                  P11_M2P_ACCESS(25)
#define M2P_RTC_ALARM2                                  P11_M2P_ACCESS(26)
#define M2P_RTC_ALARM3                                  P11_M2P_ACCESS(27)
#define M2P_RTC_ALARM4                                  P11_M2P_ACCESS(28)
#define M2P_RTC_ALARM_EN                                P11_M2P_ACCESS(29)
#define M2P_RTC_TRIM_TIME                               P11_M2P_ACCESS(30)

/*触摸所有通道配置*/
#define M2P_CTMU_CMD  									P11_M2P_ACCESS(31)
#define M2P_CTMU_MSG                                    P11_M2P_ACCESS(32)
#define M2P_CTMU_KEEP									P11_M2P_ACCESS(33)
#define M2P_CTMU_PRD0               		            P11_M2P_ACCESS(34)
#define M2P_CTMU_PRD1                                   P11_M2P_ACCESS(35)
#define M2P_CTMU_CH_ENABLE								P11_M2P_ACCESS(36)
#define M2P_CTMU_CH_DEBUG								P11_M2P_ACCESS(37)
#define M2P_CTMU_CH_CFG						            P11_M2P_ACCESS(38)
#define M2P_CTMU_CH_WAKEUP_EN					        P11_M2P_ACCESS(39)
#define M2P_CTMU_EARTCH_CH						        P11_M2P_ACCESS(40)
#define M2P_CTMU_TIME_BASE          					P11_M2P_ACCESS(41)

#define M2P_CTMU_LONG_TIMEL                             P11_M2P_ACCESS(42)
#define M2P_CTMU_LONG_TIMEH                             P11_M2P_ACCESS(43)
#define M2P_CTMU_HOLD_TIMEL                             P11_M2P_ACCESS(44)
#define M2P_CTMU_HOLD_TIMEH                             P11_M2P_ACCESS(45)
#define M2P_CTMU_SOFTOFF_LONG_TIMEL                     P11_M2P_ACCESS(46)
#define M2P_CTMU_SOFTOFF_LONG_TIMEH                     P11_M2P_ACCESS(47)
#define M2P_CTMU_LONG_PRESS_RESET_TIME_VALUE_L  		P11_M2P_ACCESS(48)//长按复位
#define M2P_CTMU_LONG_PRESS_RESET_TIME_VALUE_H  		P11_M2P_ACCESS(49)//长按复位

#define M2P_CTMU_INEAR_VALUE_L                          P11_M2P_ACCESS(50)
#define M2P_CTMU_INEAR_VALUE_H							P11_M2P_ACCESS(51)
#define M2P_CTMU_OUTEAR_VALUE_L                         P11_M2P_ACCESS(52)
#define M2P_CTMU_OUTEAR_VALUE_H                         P11_M2P_ACCESS(53)
#define M2P_CTMU_EARTCH_TRIM_VALUE_L                    P11_M2P_ACCESS(54)
#define M2P_CTMU_EARTCH_TRIM_VALUE_H                    P11_M2P_ACCESS(55)

#define M2P_MASSAGE_CTMU_CH0_CFG0L                                     56
#define M2P_MASSAGE_CTMU_CH0_CFG0H                                     57
#define M2P_MASSAGE_CTMU_CH0_CFG1L                                     58
#define M2P_MASSAGE_CTMU_CH0_CFG1H                                     59
#define M2P_MASSAGE_CTMU_CH0_CFG2L                                     60
#define M2P_MASSAGE_CTMU_CH0_CFG2H                                     61

#define M2P_CTMU_CH0_CFG0L                              P11_M2P_ACCESS(56)
#define M2P_CTMU_CH0_CFG0H                              P11_M2P_ACCESS(57)
#define M2P_CTMU_CH0_CFG1L                              P11_M2P_ACCESS(58)
#define M2P_CTMU_CH0_CFG1H                              P11_M2P_ACCESS(59)
#define M2P_CTMU_CH0_CFG2L                              P11_M2P_ACCESS(60)
#define M2P_CTMU_CH0_CFG2H                              P11_M2P_ACCESS(61)

#define M2P_CTMU_CH1_CFG0L                              P11_M2P_ACCESS(64)
#define M2P_CTMU_CH1_CFG0H                              P11_M2P_ACCESS(65)
#define M2P_CTMU_CH1_CFG1L                              P11_M2P_ACCESS(66)
#define M2P_CTMU_CH1_CFG1H                              P11_M2P_ACCESS(67)
#define M2P_CTMU_CH1_CFG2L                              P11_M2P_ACCESS(68)
#define M2P_CTMU_CH1_CFG2H                              P11_M2P_ACCESS(69)

#define M2P_CTMU_CH2_CFG0L                              P11_M2P_ACCESS(72)
#define M2P_CTMU_CH2_CFG0H                              P11_M2P_ACCESS(73)
#define M2P_CTMU_CH2_CFG1L                              P11_M2P_ACCESS(74)
#define M2P_CTMU_CH2_CFG1H                              P11_M2P_ACCESS(75)
#define M2P_CTMU_CH2_CFG2L                              P11_M2P_ACCESS(76)
#define M2P_CTMU_CH2_CFG2H                              P11_M2P_ACCESS(77)

#define M2P_CTMU_CH3_CFG0L                              P11_M2P_ACCESS(80)
#define M2P_CTMU_CH3_CFG0H                              P11_M2P_ACCESS(81)
#define M2P_CTMU_CH3_CFG1L                              P11_M2P_ACCESS(82)
#define M2P_CTMU_CH3_CFG1H                              P11_M2P_ACCESS(83)
#define M2P_CTMU_CH3_CFG2L                              P11_M2P_ACCESS(84)
#define M2P_CTMU_CH3_CFG2H                              P11_M2P_ACCESS(85)

#define M2P_CTMU_CH4_CFG0L                              P11_M2P_ACCESS(88)
#define M2P_CTMU_CH4_CFG0H                              P11_M2P_ACCESS(89)
#define M2P_CTMU_CH4_CFG1L                              P11_M2P_ACCESS(90)
#define M2P_CTMU_CH4_CFG1H                              P11_M2P_ACCESS(91)
#define M2P_CTMU_CH4_CFG2L                              P11_M2P_ACCESS(92)
#define M2P_CTMU_CH4_CFG2H                              P11_M2P_ACCESS(93)

//===========================================================================//
//
//              	M2P和P2M中断索引(跟P11同步)
//
//===========================================================================//
enum {
    M2P_LP_INDEX    = 0,
    M2P_PF_INDEX,
    M2P_P33_INDEX,
    M2P_SF_INDEX,
    M2P_CTMU_INDEX,
    M2P_CCMD_INDEX,       //common cmd
};

enum {
    P2M_LP_INDEX    = 0,
    P2M_PF_INDEX,
    P2M_WK_INDEX    = 1,
    P2M_WDT_INDEX,
    P2M_LP_INDEX2,
    P2M_CTMU_INDEX,
    P2M_CTMU_POWUP,
    P2M_REPLY_CCMD_INDEX,  //reply common cmd
    P2M_RTC_INDEX,
};
//===========================================================================//
//
//              	M2P_CMD和P2M_CMD(跟P11同步)
//
//===========================================================================//
enum {
    CLOSE_P33_INTERRUPT = 1,
    OPEN_P33_INTERRUPT,
    OPEN_RTC_INIT,
    OPEN_RTC_INTERRUPT,
    CLOSE_RTC_INTERRUPT,

};

void lp_ipc_init();

#endif
