#ifndef __P11_APP_H__
#define __P11_APP_H__

#define P33_TEST_ENABLE()           P11_P11_SYS_CON0 |= BIT(5)
#define P33_TEST_DISABLE()          P11_P11_SYS_CON0 &= ~BIT(5)

#define GET_P11_SYS_RST_SRC()	    P11_RST_SRC

#define LP_PWR_IDLE(x)              SFR(P11_PWR_CON, 0, 1, x)
#define LP_PWR_STANDBY(x)           SFR(P11_PWR_CON, 1, 1, x)
#define LP_PWR_SLEEP(x)             SFR(P11_PWR_CON, 2, 1, x)
#define LP_PWR_SSMODE(x)            SFR(P11_PWR_CON, 3, 1, x)
#define LP_PWR_SOFT_RESET(x)        SFR(P11_PWR_CON, 4, 1, x)
#define LP_PWR_INIT_FLAG()          (P11_PWR_CON & BIT(5))
#define LP_PWR_RST_FLAG_CLR(x)      SFR(P11_PWR_CON, 6, 1, x)
#define LP_PWR_RST_FLAG()           (P11_PWR_CON & BIT(7))

#define P11_ISOLATE(x)              SFR(P11_SYS_CON2, 0, 1, x)
#define P11_RX_DISABLE(x)           SFR(P11_SYS_CON2, 1, 1, x)
#define P11_TX_DISABLE(x)           SFR(P11_SYS_CON2, 2, 1, x)
#define P11_P2M_RESET(x)            SFR(P11_SYS_CON2, 3, 1, x)
#define P11_M2P_RESET_MASK(x)       SFR(P11_SYS_CON2, 4, 1, x)
#define P11_RESET_SYS(x)            SFR(P11_SYS_CON2, 6, 1, x)
#define SYS_RESET_P11(x)            SFR(P11_SYS_CON2, 7, 1, x)

#define LP_TMR0_EN(x)               SFR(P11_TMR0_CON0, 0, 1, x)
#define LP_TMR0_CTU(x)              SFR(P11_TMR0_CON0, 1, 1, x)
#define LP_TMR0_P11_WKUP_IE(x)      SFR(P11_TMR0_CON0, 2, 1, x)
#define LP_TMR0_P11_TO_IE(x)        SFR(P11_TMR0_CON0, 3, 1, x)
#define LP_TMR0_CLR_P11_WKUP(x)     SFR(P11_TMR0_CON0, 4, 1, x)
#define LP_TMR0_P11_WKUP(x)         (P11_TMR0_CON0 & BIT(5))
#define LP_TMR0_CLR_P11_TO(x)       SFR(P11_TMR0_CON0, 6, 1, x)
#define LP_TMR0_P11_TO(x)           (P11_TMR0_CON0 & BIT(7))

#define LP_TMR0_SW_KICK_START(x)    SFR(P11_TMR0_CON1, 0, 1, x)
#define LP_TMR0_HW_KICK_START(x)    SFR(P11_TMR0_CON1, 1, 1, x)
#define LP_TMR0_WKUP_IE(x)          SFR(P11_TMR0_CON1, 2, 1, x)
#define LP_TMR0_TO_IE(x)            SFR(P11_TMR0_CON1, 3, 1, x)
#define LP_TMR0_CLR_MSYS_WKUP(x)    SFR(P11_TMR0_CON1, 4, 1, x)
#define LP_TMR0_MSYS_WKUP(x)        (P11_TMR0_CON1 & BIT(5))
#define LP_TMR0_CLR_MSYS_TO(x)      SFR(P11_TMR0_CON1, 6, 1, x)
#define LP_TMR0_MSYS_TO(x)          (P11_TMR0_CON1 & BIT(7))

#define LP_TMR0_CLK_SEL(x)          SFR(P11_TMR0_CON2, 0, 2, x)
#define LP_TMR0_CLK_DIV(x)          SFR(P11_TMR0_CON2, 2, 2, x)
#define LP_TMR0_KST(x)              SFR(P11_TMR0_CON2, 4, 1, x)
#define LP_TMR0_RUN()               (P11_TMR0_CON2 & BIT(5))
#define LP_TMR0_SAMPLE_KST(x)       SFR(P11_TMR0_CON2, 6, 1, x)
#define LP_TMR0_SAMPLE_DONE()       (P11_TMR0_CON2 & BIT(7))

#define LP_TMR1_EN(x)               SFR(P11_TMR1_CON0, 0, 1, x)
#define LP_TMR1_CTU(x)              SFR(P11_TMR1_CON0, 1, 1, x)
#define LP_TMR1_P11_WKUP_IE(x)      SFR(P11_TMR1_CON0, 2, 1, x)
#define LP_TMR1_P11_TO_IE(x)        SFR(P11_TMR1_CON0, 3, 1, x)
#define LP_TMR1_CLR_P11_WKUP(x)     SFR(P11_TMR1_CON0, 4, 1, x)
#define LP_TMR1_WKUP(x)             SFR(P11_TMR1_CON0, 5, 1, x)
#define LP_TMR1_CLR_P11_TO(x)       SFR(P11_TMR1_CON0, 6, 1, x)
#define LP_TMR1_P11_TO(x)           SFR(P11_TMR1_CON0, 7, 1, x)

#define LP_TMR1_SW_KICK_START(x)    SFR(P11_TMR1_CON1, 0, 1, x)
#define LP_TMR1_HW_KICK_START(x)    SFR(P11_TMR1_CON1, 1, 1, x)
#define LP_TMR1_WKUP_IE(x)          SFR(P11_TMR1_CON1, 2, 1, x)
#define LP_TMR1_TO_IE(x)            SFR(P11_TMR1_CON1, 3, 1, x)
#define LP_TMR1_CLR_MSYS_WKUP(x)    SFR(P11_TMR1_CON1, 4, 1, x)
#define LP_TMR1_MSYS_WKUP(x)        SFR(P11_TMR1_CON1, 5, 1, x)
#define LP_TMR1_CLR_MSYS_TO(x)      SFR(P11_TMR1_CON1, 6, 1, x)
#define LP_TMR1_MSYS_TO(x)          SFR(P11_TMR1_CON1, 7, 1, x)

#define LP_TMR1_CLK_SEL(x)          SFR(P11_TMR1_CON2, 0, 2, x)
#define LP_TMR1_CLK_DIV(x)          SFR(P11_TMR1_CON2, 2, 2, x)
#define LP_TMR1_KST(x)              SFR(P11_TMR1_CON2, 4, 1, x)
#define LP_TMR1_RUN()               (P11_TMR1_CON2 & BIT(5))
#define LP_TMR1_SAMPLE_KST(x)       SFR(P11_TMR1_CON2, 6, 1, x)
#define LP_TMR1_SAMPLE_DONE()       (P11_TMR1_CON2 & BIT(7))

//===========================================================================//
//
//                             	SFR
//
//===========================================================================//
#define CLOCK_KEEP(en)						\
	if(en){									\
		P11_BTOSC_CON = BIT(4) | BIT(0); 	\
	}else{  								\
		P11_BTOSC_CON = 0;					\
	}

#endif
