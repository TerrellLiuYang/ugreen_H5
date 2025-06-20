#ifndef __P11_SFR_H__
#define __P11_SFR_H__

#define P11_ACCESS(x)   (*(volatile u8 *)(0x1A0000 + x))

/*xdata区域*/
#define P11_P2M_ACCESS(x)      P11_ACCESS(0x274c + x)
#define P11_M2P_ACCESS(x)      P11_ACCESS(0x278c + x)
/*sfr区域*/
#define P11_SFR(x)      	   P11_ACCESS(0x3F00 + x)

//===========================================================================//
//
//                             standard SFR
//
//===========================================================================//

//  0x80 - 0x87
//******************************
#define    P11_DPCON0          P11_SFR(0x80)
#define    P11_SP              P11_SFR(0x81)
#define    P11_DP0L            P11_SFR(0x82)
#define    P11_DP0H            P11_SFR(0x83)
#define    P11_DP1L            P11_SFR(0x84)
#define    P11_DP1H            P11_SFR(0x85)
#define    P11_SPH             P11_SFR(0x86)
#define    P11_PDATA           P11_SFR(0x87)

//  0x88 - 0x8F
//******************************
#define    P11_WDT_CON0        P11_SFR(0x88)
#define    P11_WDT_CON1        P11_SFR(0x89)
#define    P11_PWR_CON         P11_SFR(0x8a)
#define    P11_CLK_CON0        P11_SFR(0x8b)
#define    P11_CLK_CON1        P11_SFR(0x8c)
#define    P11_BTOSC_CON       P11_SFR(0x8d)
#define    P11_RST_SRC         P11_SFR(0x8e)
#define    P11_SYS_DIV         P11_SFR(0x8f)

//  0xA0 - 0xA7
//******************************
#define    P11_SDIV_DA0        P11_SFR(0xa0)
#define    P11_SDIV_DA1        P11_SFR(0xa1)
#define    P11_SDIV_DA2        P11_SFR(0xa2)
#define    P11_SDIV_DA3        P11_SFR(0xa3)
#define    P11_SDIV_DB0        P11_SFR(0xa4)
#define    P11_SDIV_DB1        P11_SFR(0xa5)
#define    P11_SDIV_DB2        P11_SFR(0xa6)
#define    P11_SDIV_DB3        P11_SFR(0xa7)

//  0xA8 - 0xAF
//******************************
#define    P11_IE0             P11_SFR(0xa8)
#define    P11_IE1             P11_SFR(0xa9)
#define    P11_IPCON0L         P11_SFR(0xaa)
#define    P11_IPCON0H         P11_SFR(0xab)
#define    P11_IPCON1L         P11_SFR(0xac)
#define    P11_IPCON1H         P11_SFR(0xad)
#define    P11_INT_BASE        P11_SFR(0xae)
#define    P11_SINT_CON        P11_SFR(0xaf)

//  0xB0 - 0xB7
//******************************
#define    P11_WKUP_SRC0       P11_SFR(0xb0)
#define    P11_WKUP_SRC1       P11_SFR(0xb1)
#define    P11_TMR4_CON0       P11_SFR(0xb2)
#define    P11_TMR4_CON1       P11_SFR(0xb3)
#define    P11_TMR4_CON2       P11_SFR(0xb4)
#define    P11_TMR4_PRD0       P11_SFR(0xb5)
#define    P11_TMR4_PRD1       P11_SFR(0xb6)
#define    P11_TMR4_CNT0       P11_SFR(0xb7)

//  0xB8 - 0xBF
//******************************
#define    P11_TMR4_CNT1       P11_SFR(0xb8)
#define    P11_MEM_CFG0        P11_SFR(0xb9)
#define    P11_MEM_CFG1        P11_SFR(0xba)

#define    P11_WKUP_CON0       P11_SFR(0xbc)
#define    P11_WKUP_CON1       P11_SFR(0xbd)
#define    P11_WKUP_CON2       P11_SFR(0xbe)
#define    P11_WKUP_CON3       P11_SFR(0xbf)
//  0xC0 - 0xC7
//******************************
#define    P11_LC_MPC_CON      P11_SFR(0xc0)
#define    P11_PORT_CON0       P11_SFR(0xc1)
#define    P11_PORT_CON1       P11_SFR(0xc2)
#define    P11_MEM_PWR_CON     P11_SFR(0xc3)
#define    P11_WKUP_EN0        P11_SFR(0xc4)
#define    P11_WKUP_EN1        P11_SFR(0xc5)
#define    P11_LCTM_CON        P11_SFR(0xc6)
#define    P11_SDIV_CON        P11_SFR(0xc7)

//  0xC8 - 0xCF
//******************************
#define    P11_UART_CON0       P11_SFR(0xc8)
#define    P11_UART_CON1       P11_SFR(0xc9)
#define    P11_UART_STA        P11_SFR(0xca)
#define    P11_UART_BUF        P11_SFR(0xcb)
#define    P11_UART_BAUD       P11_SFR(0xcc)

#define    P11_MSYS_DATA       P11_SFR(0xcf)
//  0xD0 - 0xD7
//******************************
#define    P11_PSW             P11_SFR(0xd0)
#define    P11_TMR0_CON0       P11_SFR(0xd1)
#define    P11_TMR0_CON1       P11_SFR(0xd2)
#define    P11_TMR0_CON2       P11_SFR(0xd3)
#define    P11_TMR0_PRD0       P11_SFR(0xd4)
#define    P11_TMR0_PRD1       P11_SFR(0xd5)
#define    P11_TMR0_PRD2       P11_SFR(0xd6)
#define    P11_TMR0_PRD3       P11_SFR(0xd7)

//  0xD8 - 0xDF
//******************************
#define    P11_TMR0_RSC0       P11_SFR(0xd8)
#define    P11_TMR0_RSC1       P11_SFR(0xd9)
#define    P11_TMR0_RSC2       P11_SFR(0xda)
#define    P11_TMR0_RSC3       P11_SFR(0xdb)
#define    P11_TMR0_CNT0       P11_SFR(0xdc)
#define    P11_TMR0_CNT1       P11_SFR(0xdd)
#define    P11_TMR0_CNT2       P11_SFR(0xde)
#define    P11_TMR0_CNT3       P11_SFR(0xdf)

//  0xE0 - 0xE7
//******************************
#define    P11_ACC             P11_SFR(0xe0)
#define    P11_TMR1_CON0       P11_SFR(0xe1)
#define    P11_TMR1_CON1       P11_SFR(0xe2)
#define    P11_TMR1_CON2       P11_SFR(0xe3)
#define    P11_TMR1_PRD0       P11_SFR(0xe4)
#define    P11_TMR1_PRD1       P11_SFR(0xe5)
#define    P11_TMR1_CNT0       P11_SFR(0xe6)
#define    P11_TMR1_CNT1       P11_SFR(0xe7)

//  0xE8 - 0xEF
//******************************
#define    P11_TMR2_CON0       P11_SFR(0xe8)
#define    P11_TMR2_CON1       P11_SFR(0xe9)
#define    P11_TMR2_CON2       P11_SFR(0xea)
#define    P11_TMR2_PRD0       P11_SFR(0xeb)
#define    P11_TMR2_PRD1       P11_SFR(0xec)
#define    P11_TMR2_CNT0       P11_SFR(0xed)
#define    P11_TMR2_CNT1       P11_SFR(0xee)

//  0xF0 - 0xF7
//******************************
#define    P11_BREG            P11_SFR(0xf0)
#define    P11_P2M_INT_IE      P11_SFR(0xf1)
#define    P11_P2M_INT_SET     P11_SFR(0xf2)
#define    P11_P2M_INT_CLR     P11_SFR(0xf3)
#define    P11_P2M_INT_PND     P11_SFR(0xf4)
#define    P11_P2M_CLK_CON0    P11_SFR(0xf5)
#define    P11_P11_SYS_CON0    P11_SFR(0xf6)
#define    P11_P11_SYS_CON1    P11_SFR(0xf7)

//  0xF8 - 0xFF
//******************************
#define    P11_SYS_CON2    	   P11_SFR(0xf8)
#define    P11_M2P_INT_IE      P11_SFR(0xf9)
#define    P11_M2P_INT_SET     P11_SFR(0xfa)
#define    P11_M2P_INT_CLR     P11_SFR(0xfb)
#define    P11_M2P_INT_PND     P11_SFR(0xfc)
#define    P11_MBIST_CON       P11_SFR(0xfd)
//#define    P11_              P11_SFR(0xfe)
#define    P11_SIM_END         P11_SFR(0xff)

//===========================================================================//
//
//                             LPCTM SFR
//
//===========================================================================//
//  0x00 - 0x07
//******************************
#define    LPCTM_CON0          	   P11_SFR(0x00)
#define    LPCTM_CON1          	   P11_SFR(0x01)
#define    LPCTM_CON2          	   P11_SFR(0x02)
#define    LPCTM_CON3          	   P11_SFR(0x03)
#define    LPCTM_ANA0          	   P11_SFR(0x04)
#define    LPCTM_ANA1          	   P11_SFR(0x05)
#define    LPCTM_RESH          	   P11_SFR(0x06)
#define    LPCTM_RESL          	   P11_SFR(0x07)

#define    P11_TMR3_CON0       	   P11_SFR(0x08)
#define    P11_TMR3_CON1       	   P11_SFR(0x09)
#define    P11_TMR3_CON2       	   P11_SFR(0x0a)
#define    P11_TMR3_PRD0       	   P11_SFR(0x0b)
#define    P11_TMR3_PRD1       	   P11_SFR(0x0c)
#define    P11_TMR3_PRD2       	   P11_SFR(0x0d)
#define    P11_TMR3_PRD3       	   P11_SFR(0x0e)
#define    P11_TMR3_CNT0       	   P11_SFR(0x0f)
#define    P11_TMR3_CNT1       	   P11_SFR(0x10)
#define    P11_TMR3_CNT2       	   P11_SFR(0x11)
#define    P11_TMR3_CNT3       	   P11_SFR(0x12)

#endif

