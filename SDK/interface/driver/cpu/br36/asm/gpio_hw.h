#ifndef _GPIO_HW_H
#define _GPIO_HW_H

#include "typedef.h"
#include "asm/power/p33_sfr.h"

#define IO_GROUP_NUM 		16

#define IO_PORTA_00 				(IO_GROUP_NUM * 0 + 0)//A-A=0
#define IO_PORTA_01 				(IO_GROUP_NUM * 0 + 1)
#define IO_PORTA_02 				(IO_GROUP_NUM * 0 + 2)
#define IO_PORTA_03 				(IO_GROUP_NUM * 0 + 3)
#define IO_PORTA_04 				(IO_GROUP_NUM * 0 + 4)
#define IO_PORTA_05 				(IO_GROUP_NUM * 0 + 5)
#define IO_PORTA_06 				(IO_GROUP_NUM * 0 + 6)
#define IO_PORTA_07 				(IO_GROUP_NUM * 0 + 7)
#define IO_PORTA_08 				(IO_GROUP_NUM * 0 + 8)
#define IO_PORT_PA_MASK              0x01ff

#define IO_PORTB_00 				(IO_GROUP_NUM * 1 + 0)//B-A=1
#define IO_PORTB_01 				(IO_GROUP_NUM * 1 + 1)
#define IO_PORTB_02 				(IO_GROUP_NUM * 1 + 2)
#define IO_PORTB_03 				(IO_GROUP_NUM * 1 + 3)
#define IO_PORTB_04 				(IO_GROUP_NUM * 1 + 4)
#define IO_PORTB_05 				(IO_GROUP_NUM * 1 + 5)
#define IO_PORTB_06 				(IO_GROUP_NUM * 1 + 6)
#define IO_PORTB_07 				(IO_GROUP_NUM * 1 + 7)
#define IO_PORTB_08 				(IO_GROUP_NUM * 1 + 8)
#define IO_PORTB_09 				(IO_GROUP_NUM * 1 + 9)
#define IO_PORT_PB_MASK              0x03ff

#define IO_PORTC_00 				(IO_GROUP_NUM * 2 + 0)//C-A=2
#define IO_PORTC_01 				(IO_GROUP_NUM * 2 + 1)
#define IO_PORTC_02 				(IO_GROUP_NUM * 2 + 2)
#define IO_PORTC_03 				(IO_GROUP_NUM * 2 + 3)
#define IO_PORTC_04 				(IO_GROUP_NUM * 2 + 4)
#define IO_PORTC_05 				(IO_GROUP_NUM * 2 + 5)
#define IO_PORTC_06 				(IO_GROUP_NUM * 2 + 6)
#define IO_PORT_PC_MASK              0x007f

#define IO_PORTD_00 				(IO_GROUP_NUM * 3 + 0)//D-A=3
#define IO_PORTD_01 				(IO_GROUP_NUM * 3 + 1)
#define IO_PORTD_02 				(IO_GROUP_NUM * 3 + 2)
#define IO_PORTD_03 				(IO_GROUP_NUM * 3 + 3)
#define IO_PORTD_04 				(IO_GROUP_NUM * 3 + 4)
#define IO_PORTD_05 				(IO_GROUP_NUM * 3 + 5)
#define IO_PORTD_06 				(IO_GROUP_NUM * 3 + 6)
#define IO_PORT_PD_MASK              0x007f


#define IO_PORTP_00 				(IO_GROUP_NUM * 13 + 0)//pp固定13
#define IO_PORT_PP_MASK              0x0001

#define IO_PORT_LDOIN   			IO_PORTP_00

#define IO_MAX_NUM 					(IO_PORTP_00 + 1)


#define IO_PORT_DP                  (IO_GROUP_NUM * 14 + 0)//usb固定14
#define IO_PORT_DM                  (IO_GROUP_NUM * 14 + 1)
#define IO_PORT_USB_MASK            0x03
#define IS_PORT_USB(x)              (x <= IO_PORT_DM)//无usb赋0

//无PR
// #define IO_PORT_PR_00               (IO_GROUP_NUM * 15 + 0)//pr固定15
// #define IO_PORT_PR_01               (IO_GROUP_NUM * 15 + 1)
// #define IO_PORT_PR_02               (IO_GROUP_NUM * 15 + 2)
// #define IO_PORT_PR_03               (IO_GROUP_NUM * 15 + 3)
// #define IO_PORT_PR_04               (IO_GROUP_NUM * 15 + 4)
// #define IO_PORT_PR_MASK              0x1f

#define IO_PORT_MAX					(IO_PORT_DM + 1)

#define P11_IO_OFFSET               0
#define IO_PORT_P00                 (IO_PORT_MAX + P11_IO_OFFSET)

#define P33_IO_OFFSET               1
#define IO_CHGFL_DET                (IO_PORT_MAX + P33_IO_OFFSET + 0)
#define IO_VBGOK_DET                (IO_PORT_MAX + P33_IO_OFFSET + 1)
#define IO_VBTCH_DET                (IO_PORT_MAX + P33_IO_OFFSET + 2)
#define IO_LDOIN_DET                (IO_PORT_MAX + P33_IO_OFFSET + 3)
// #define IO_SPDIF_RX_DET             (IO_PORT_MAX + P33_IO_OFFSET + 4)


#define GPIOA                       (IO_GROUP_NUM * 0)
#define GPIOB                       (IO_GROUP_NUM * 1)
#define GPIOC                       (IO_GROUP_NUM * 2)
#define GPIOD                       (IO_GROUP_NUM * 3)
#define GPIOP                       (IO_GROUP_NUM * 13)
#define GPIOUSB                     (IO_GROUP_NUM * 14)
// #define GPIOR                       (IO_GROUP_NUM * 15) //br36 no pr
#define GPIOP33                     (IO_PORT_MAX + P33_IO_OFFSET)
#define GPIOP11                     (IO_PORT_MAX + P11_IO_OFFSET)

enum gpio_port {
    PORTA = 0,
    PORTB = 1,
    PORTC = 2,
    PORTD = 3,
    PORTP = 13,//13
    PORTUSB = 14,//14
    // PORTR,  //15 无pr
};

struct port_reg {
    volatile unsigned int out;
    volatile unsigned int in;
    volatile unsigned int dir;
    volatile unsigned int die;
    volatile unsigned int pu0;
    // volatile unsigned int pu1;
    volatile unsigned int pd0;
    // volatile unsigned int pd1;
    volatile unsigned int hd0;
    volatile unsigned int hd1;
    volatile unsigned int dieh;
};
#define GPIO_PX_PU_REG_NUM 1
#define GPIO_PX_PD_REG_NUM 1
#define GPIO_PX_HD_REG_NUM 2
#define GPIO_PX_DIEH_REG_NUM 1
#define GPIO_PX_SPL_REG_NUM  0
struct usb_reg {
    volatile unsigned int out: 2; //0
        volatile unsigned int dir: 2; //2
        volatile unsigned int pu0: 2; //4
        // volatile unsigned int pu1:2;//6
        volatile unsigned int pd0: 2; //6
        volatile unsigned int die: 2; //8
        volatile unsigned int dieh: 2; //10
        volatile unsigned int : 4; //12
        volatile unsigned int in: 2; //16
        volatile unsigned int res: 14; //18
    };
#define GPIO_USB_PU_REG_NUM 1
#define GPIO_USB_PD_REG_NUM 1
#define GPIO_USB_HD_REG_NUM 0
#define GPIO_USB_DIEH_REG_NUM 1
#define GPIO_USB_SPL_REG_NUM  0

//无PR
// struct port_pr_reg {
//     volatile unsigned int in;
//     volatile unsigned int out;
//     volatile unsigned int dir;
//     volatile unsigned int die;
//     volatile unsigned int pu0;
//     // volatile unsigned int pu1;
//     volatile unsigned int pd0;
//     // volatile unsigned int pd1;
//     volatile unsigned int hd0;
//     // volatile unsigned int hd1;
// };
// #define GPIO_PR_PU_REG_NUM 0
// #define GPIO_PR_PD_REG_NUM 0
// #define GPIO_PR_HD_REG_NUM 0
// #define GPIO_PR_DIEH_REG_NUM 0
// #define GPIO_PR_SPL_REG_NUM  0

#define GPIO_PU_REG_NUM 1 //max_num
#define GPIO_PD_REG_NUM 1 //max_num
#define GPIO_HD_REG_NUM 2 //max_num


// ----------------------------------------
// enum gpio_hd_mode {//no use
//     GPIO_HD_2p4mA,		#<{(| 最大驱动电流  2.4mA |)}>#
//     GPIO_HD_8p0mA,		#<{(| 最大驱动电流  8.0mA |)}>#
//     GPIO_HD_26p4mA,		#<{(| 最大驱动电流  26.4mA  |)}>#
//     GPIO_HD_50p0mA,		#<{(| 最大驱动电流 50.0mA |)}>#
// };

//===================================================//
// Crossbar ENUM
//===================================================//
    enum PFI_TABLE {
    PFI_GP_ICH0 = ((u32)(&(JL_IMAP->FI_GP_ICH0))),
    PFI_GP_ICH1 = ((u32)(&(JL_IMAP->FI_GP_ICH1))),
    PFI_GP_ICH2 = ((u32)(&(JL_IMAP->FI_GP_ICH2))),
    PFI_GP_ICH3 = ((u32)(&(JL_IMAP->FI_GP_ICH3))),
    PFI_GP_ICH4 = ((u32)(&(JL_IMAP->FI_GP_ICH4))),
    PFI_GP_ICH5 = ((u32)(&(JL_IMAP->FI_GP_ICH5))),
    PFI_GP_ICH6 = ((u32)(&(JL_IMAP->FI_GP_ICH6))),
    PFI_GP_ICH7 = ((u32)(&(JL_IMAP->FI_GP_ICH7))),

    PFI_TMR0_CIN = ((u32)(&(JL_IMAP->FI_TMR0_CIN))),
    PFI_TMR0_CAP = ((u32)(&(JL_IMAP->FI_TMR0_CAP))),
    PFI_TMR1_CIN = ((u32)(&(JL_IMAP->FI_TMR1_CIN))),
    PFI_TMR1_CAP = ((u32)(&(JL_IMAP->FI_TMR1_CAP))),
    PFI_TMR2_CIN = ((u32)(&(JL_IMAP->FI_TMR2_CIN))),
    PFI_TMR2_CAP = ((u32)(&(JL_IMAP->FI_TMR2_CAP))),
    PFI_TMR3_CIN = ((u32)(&(JL_IMAP->FI_TMR3_CIN))),
    PFI_TMR3_CAP = ((u32)(&(JL_IMAP->FI_TMR3_CAP))),
    PFI_TMR4_CIN = ((u32)(&(JL_IMAP->FI_TMR4_CIN))),
    PFI_TMR4_CAP = ((u32)(&(JL_IMAP->FI_TMR4_CAP))),
    PFI_TMR5_CIN = ((u32)(&(JL_IMAP->FI_TMR5_CIN))),
    PFI_TMR5_CAP = ((u32)(&(JL_IMAP->FI_TMR5_CAP))),
    PFI_SPI0_CLK = ((u32)(&(JL_IMAP->FI_SPI0_CLK))),
    PFI_SPI0_DA0 = ((u32)(&(JL_IMAP->FI_SPI0_DA0))),
    PFI_SPI0_DA1 = ((u32)(&(JL_IMAP->FI_SPI0_DA1))),
    PFI_SPI0_DA2 = ((u32)(&(JL_IMAP->FI_SPI0_DA2))),
    PFI_SPI0_DA3 = ((u32)(&(JL_IMAP->FI_SPI0_DA3))),
    PFI_SPI1_CLK = ((u32)(&(JL_IMAP->FI_SPI1_CLK))),
    PFI_SPI1_DA0 = ((u32)(&(JL_IMAP->FI_SPI1_DA0))),
    PFI_SPI1_DA1 = ((u32)(&(JL_IMAP->FI_SPI1_DA1))),
    PFI_SPI1_DA2 = ((u32)(&(JL_IMAP->FI_SPI1_DA2))),
    PFI_SPI1_DA3 = ((u32)(&(JL_IMAP->FI_SPI1_DA3))),
    PFI_SPI2_CLK = ((u32)(&(JL_IMAP->FI_SPI2_CLK))),
    PFI_SPI2_DA0 = ((u32)(&(JL_IMAP->FI_SPI2_DA0))),
    PFI_SPI2_DA1 = ((u32)(&(JL_IMAP->FI_SPI2_DA1))),
    PFI_SPI2_DA2 = ((u32)(&(JL_IMAP->FI_SPI2_DA2))),
    PFI_SPI2_DA3 = ((u32)(&(JL_IMAP->FI_SPI2_DA3))),
    PFI_SD0_CMD = ((u32)(&(JL_IMAP->FI_SD0_CMD))),
    PFI_SD0_DA0 = ((u32)(&(JL_IMAP->FI_SD0_DA0))),
    PFI_SD0_DA1 = ((u32)(&(JL_IMAP->FI_SD0_DA1))),
    PFI_SD0_DA2 = ((u32)(&(JL_IMAP->FI_SD0_DA2))),
    PFI_SD0_DA3 = ((u32)(&(JL_IMAP->FI_SD0_DA3))),
    PFI_IIC_SCL = ((u32)(&(JL_IMAP->FI_IIC_SCL))),
    PFI_IIC_SDA = ((u32)(&(JL_IMAP->FI_IIC_SDA))),
    PFI_UART0_RX = ((u32)(&(JL_IMAP->FI_UART0_RX))),
    PFI_UART1_RX = ((u32)(&(JL_IMAP->FI_UART1_RX))),
    PFI_UART1_CTS = ((u32)(&(JL_IMAP->FI_UART1_CTS))),
    PFI_UART2_RX = ((u32)(&(JL_IMAP->FI_UART2_RX))),
    PFI_RDEC0_DAT0 = ((u32)(&(JL_IMAP->FI_RDEC0_DAT0))),
    PFI_RDEC0_DAT1 = ((u32)(&(JL_IMAP->FI_RDEC0_DAT1))),
    PFI_ALNK0_MCLK = ((u32)(&(JL_IMAP->FI_ALNK0_MCLK))),
    PFI_ALNK0_LRCK = ((u32)(&(JL_IMAP->FI_ALNK0_LRCK))),
    PFI_ALNK0_SCLK = ((u32)(&(JL_IMAP->FI_ALNK0_SCLK))),
    PFI_ALNK0_DAT0 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT0))),
    PFI_ALNK0_DAT1 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT1))),
    PFI_ALNK0_DAT2 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT2))),
    PFI_ALNK0_DAT3 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT3))),
    PFI_PLNK_DAT0 = ((u32)(&(JL_IMAP->FI_PLNK_DAT0))),
    PFI_PLNK_DAT1 = ((u32)(&(JL_IMAP->FI_PLNK_DAT1))),
    PFI_CHAIN_IN0 = ((u32)(&(JL_IMAP->FI_CHAIN_IN0))),
    PFI_CHAIN_IN1 = ((u32)(&(JL_IMAP->FI_CHAIN_IN1))),
    PFI_CHAIN_IN2 = ((u32)(&(JL_IMAP->FI_CHAIN_IN2))),
    PFI_CHAIN_IN3 = ((u32)(&(JL_IMAP->FI_CHAIN_IN3))),
    PFI_CHAIN_RST = ((u32)(&(JL_IMAP->FI_CHAIN_RST))),
    PFI_TOTAl = ((u32)(&(JL_IMAP->FI_TOTAL))),
};


#define  INPUT_GP_ICH_MAX  8
#define  OUTPUT_GP_OCH_MAX 12

enum OUTPUT_CH_SIGNAL {
    OUTPUT_CH_SIGNAL_P11_DBG_OUT,
    OUTPUT_CH_SIGNAL_CLOCK_OUT0,
    OUTPUT_CH_SIGNAL_CLOCK_OUT1,
    OUTPUT_CH_SIGNAL_CLOCK_OUT2,
    OUTPUT_CH_SIGNAL_CLOCK_OUT3,
    OUTPUT_CH_SIGNAL_WLC_INT_ACTIVE,
    OUTPUT_CH_SIGNAL_WLC_INT_STATUS,
    OUTPUT_CH_SIGNAL_WLC_INT_FREQ,
    OUTPUT_CH_SIGNAL_WL_DBG0,
    OUTPUT_CH_SIGNAL_WL_DBG1,
    OUTPUT_CH_SIGNAL_WL_DBG2,
    OUTPUT_CH_SIGNAL_WL_DBG3,
    OUTPUT_CH_SIGNAL_WL_DBG4,
    OUTPUT_CH_SIGNAL_WL_DBG5,
    OUTPUT_CH_SIGNAL_WL_DBG6,
    OUTPUT_CH_SIGNAL_WL_DBG7,
    OUTPUT_CH_SIGNAL_WL_PA_EN,
    OUTPUT_CH_SIGNAL_WL_LNA_EN,
    OUTPUT_CH_SIGNAL_PMU_ANA_DBG,
};

enum INPUT_CH_TYPE {
    INPUT_CH_TYPE_GP_ICH = 0,
    INPUT_CH_TYPE_TIME0_PWM = 8,
    INPUT_CH_TYPE_TIME1_PWM,
};
enum INPUT_CH_SIGNAL {
    //ICH_CON0
    INPUT_CH_SIGNAL_WAKEUP,
    INPUT_CH_SIGNAL_IRFLT,
    INPUT_CH_SIGNAL_CAP,
    INPUT_CH_SIGNAL_CLK_PIN,
    INPUT_CH_SIGNAL_WL_ICH_SEL,

    INPUT_CH_SIGNAL_RESERVE0,
    INPUT_CH_SIGNAL_RESERVE1,
    INPUT_CH_SIGNAL_RESERVE2,
};

enum gpio_function {
    PORT_FUNC_NULL,    //null
    PORT_FUNC_UART0_TX, //out
    PORT_FUNC_UART0_RX,//in
    PORT_FUNC_UART1_TX, //out
    PORT_FUNC_UART1_RX,//in
    PORT_FUNC_UART1_RTS,//out
    PORT_FUNC_UART1_CTS,//in
    PORT_FUNC_UART2_TX, //out
    PORT_FUNC_UART2_RX,//in

    PORT_FUNC_SPI0_CLK,
    PORT_FUNC_SPI0_DA0,
    PORT_FUNC_SPI0_DA1,
    PORT_FUNC_SPI0_DA2,
    PORT_FUNC_SPI0_DA3,
    PORT_FUNC_SPI1_CLK,
    PORT_FUNC_SPI1_DA0,
    PORT_FUNC_SPI1_DA1,
    PORT_FUNC_SPI1_DA2,
    PORT_FUNC_SPI1_DA3,
    PORT_FUNC_SPI2_CLK,
    PORT_FUNC_SPI2_DA0,
    PORT_FUNC_SPI2_DA1,
    PORT_FUNC_SPI2_DA2,
    PORT_FUNC_SPI2_DA3,

    PORT_FUNC_IIC_SCL,
    PORT_FUNC_IIC_SDA,

    PORT_FUNC_SD0_CLK,//out
    PORT_FUNC_SD0_CMD,
    PORT_FUNC_SD0_DA0,
    PORT_FUNC_SD0_DA1,
    PORT_FUNC_SD0_DA2,
    PORT_FUNC_SD0_DA3,

    PORT_FUNC_GPADC,    //in
    PORT_FUNC_PWM_LED,

    PORT_FUNC_PLNK_SCLK,//out
    PORT_FUNC_PLNK_DAT0,//in
    PORT_FUNC_PLNK_DAT1,//in
    PORT_FUNC_ANC_MICCK,//out


    PORT_FUNC_OCH_CLOCK_OUT0,
    PORT_FUNC_OCH_CLOCK_OUT1,
    PORT_FUNC_OCH_CLOCK_OUT2,
    PORT_FUNC_OCH_CLOCK_OUT3,

    PORT_FUNC_TIMER0_PWM,
    PORT_FUNC_TIMER1_PWM,
    PORT_FUNC_TIMER2_PWM,
    PORT_FUNC_TIMER3_PWM,
    PORT_FUNC_TIMER4_PWM,
    PORT_FUNC_TIMER5_PWM,
    PORT_FUNC_TIMER0_CAPTURE,
    PORT_FUNC_TIMER1_CAPTURE,
    PORT_FUNC_TIMER2_CAPTURE,
    PORT_FUNC_TIMER3_CAPTURE,
    PORT_FUNC_TIMER4_CAPTURE,
    PORT_FUNC_TIMER5_CAPTURE,
    PORT_FUNC_IRFLT,

    PORT_FUNC_RDEC0_PORT0,
    PORT_FUNC_RDEC0_PORT1,
};

/**************************************************/
#define __struct(x) (struct x##_reg *)
#define _struct(x) __struct(x)
#ifdef GPIOA
#define __PORTPA ((struct port_reg *)JL_PORTA)
#endif
#ifdef GPIOB
#define __PORTPB ((struct port_reg *)JL_PORTB)
#endif
#ifdef GPIOC
#define __PORTPC ((struct port_reg *)JL_PORTC)
#endif
#ifdef GPIOD
#define __PORTPD ((struct port_reg *)JL_PORTD)
#endif
#ifdef GPIOE
#define __PORTPE ((struct port_reg *)JL_PORTE)
#endif
#ifdef GPIOF
#define __PORTPF ((struct port_reg *)JL_PORTF)
#endif
#ifdef GPIOG
#define __PORTPG ((struct port_reg *)JL_PORTG)
#endif
#ifdef GPIOH
#define __PORTPH ((struct port_reg *)JL_PORTH)
#endif
#ifdef GPIOP
#define __PORTPP ((struct port_reg *)JL_PORTP)
#endif
#ifdef GPIOR
#define __PORTPR ((struct port_pr_reg *)R3_PR_IO_P)
#endif
#ifdef GPIOUSB
#define __PORTPU ((struct usb_reg *)JL_USB_IO)
#endif
#define __portx(x,y) __PORT##x->y
#define _portx(x,y)  __portx(x,y)
#define __toggle_port(x,y) __PORT##x->out ^= y;
#define _toggle_port(port,pin) __toggle_port(port,pin)

/*************************function*************************/
struct port_reg *gpio2reg(u32 gpio);

/**
 * @brief usb_iomode
 *
 * @param enable 1，使能；0，关闭
 */
void usb_iomode(u32 enable);
/**
 * @brief gpio_write
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，输出1；0，输出0
 *
 * @return
 */
int gpio_hw_write(u32 gpio, u32 value);//return <0:error
/**
 * @brief gpio_read
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 * @return
 */
int gpio_hw_read(u32 gpio);//return <0:error

int get_gpio(const char *p);//return <0:error

/**************************************************************/
/*********************multi pin interface***************************/
/**
 * @brief port_set_direction
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 * @param value 1，输入；0，输出
 *
 * @return <0 :error
 */
int gpio_hw_set_direction(enum gpio_port port, u32 pin, u32 value);
/**
 * @brief port_direction_input
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 *
 * @return <0 :error
 */
int gpio_hw_direction_input(enum gpio_port port, u32 pin);
/**
 * @brief port_direction_output/port_write_port/port_set_output_value
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 * @param value 1，输出1；0，输出0
 * @return <0 :error
 */
int gpio_hw_direction_output(enum gpio_port port, u32 pin, int value);/////////
int gpio_hw_write_port(enum gpio_port port, u32 pin, u32 value);
int gpio_hw_set_output_value(enum gpio_port port, u32 pin, u32 value);
/**
 * @brief port_set_pull_up/port_set_pull_down/port_set_hd/
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 * @param value 参考枚举gpio_pullup_mode/gpio_pulldown_mode/gpio_drive_strength
 * @return <0 :error
 */
int gpio_hw_set_pull_up(enum gpio_port port, u32 pin, enum gpio_pullup_mode value);
int gpio_hw_set_pull_down(enum gpio_port port, u32 pin, enum gpio_pulldown_mode value);//portabcdpr:pd0,pd1,usb:pd0
int gpio_hw_set_drive_strength(enum gpio_port port, u32 pin, enum gpio_drive_strength value);
/**
 * @brief port_set_die/port_set_dieh
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 * @param value 1，设置1；0，设置0
 * @return <0 :error
 */
int gpio_hw_set_die(enum gpio_port port, u32 pin, int value);
int gpio_hw_set_dieh(enum gpio_port port, u32 pin, u32 value);

//iic专用
int gpio_hw_set_spl(enum gpio_port port, u32 pin, u32 value);
/**
 * @brief gpio_hw_read_port/gpio_hw_read_out_level/gpio_hw_read_drive_strength
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 *
 * @return <0 :error ；other：ok
 */
int gpio_hw_read_port(enum gpio_port port, u32 pin);
int gpio_hw_read_out_level(enum gpio_port port, u32 pin);
u32 gpio_hw_read_drive_strength(enum gpio_port port, u32 pin);//return hd1:高16位,  hd0:低16位


//=================================================================================//
//@brief: CrossBar 获取某IO的输出映射寄存器
//@input:
// 		gpio: 需要输出外设信号的IO口; 如IO_PORTA_00
//@return:
// 		输出映射寄存器地址; 如&(JL_OMAP->PA0_OUT)
//=================================================================================//
u32 *gpio2crossbar_outreg(u32 gpio);

//=================================================================================//
//@brief: CrossBar 获取某IO的输入映射序号
//@input:
// 		gpio: 需要输出外设信号的IO口; 如IO_PORTA_00
//@return:
// 		输出映射序号; 如PA0_IN
//=================================================================================//
u32 gpio2crossbar_inport(u32 gpio);

//=================================================================================//
//@brief: CrossBar 输出设置 API, 将指定IO口设置为某个外设的输出
//@input:
// 		gpio: 需要输出外设信号的IO口;
// 		fun_index: 需要输出到指定IO口的外设信号, 可以输出外设信号列表请查看io_omap.h文件;
// 		dir_ctl: IO口方向由外设控制使能, 常设为1;
// 		data_ctl: IO口电平状态由外设控制使能, 常设为1;
//@return:
// 		1)0: 执行正确;
//		2)-EINVAL: 传参出错;
//@note: 所映射的IO需要在设置IO状态为输出配置;
//@example: 将UART0的Tx信号输出到IO_PORTA_05口:
// 			gpio_direction_output(IO_PORTA_05, 1); //设置IO为输出状态
//			gpio_set_fun_output_port(IO_PORTA_05, FO_UART0_TX, 1, 1); //将UART0的Tx信号输出到IO_PORTA_05口
//=================================================================================//
int gpio_set_fun_output_port(u32 gpio, u32 fun_index, u8 dir_ctl, u8 data_ctl);

//=================================================================================//
//@brief: CrossBar 输出设置 API, 将指定IO释放外设控制, 变为普通IO;
//@input:
// 		gpio: 需要释放外设控制IO口, 释放后变为普通IO模式;
//@return:
// 		1)0: 执行正确;
//		2)-EINVAL: 传参出错;
//@note:
//@example: 将IO_PORTA_05口被某一外设控制状态释放:
// 			gpio_disable_fun_output_port(IO_PORTA_05);
//=================================================================================//
int gpio_disable_fun_output_port(u32 gpio);

//=================================================================================//
//@brief: CrossBar 输入设置 API, 将某个外设的输入设置为从某个IO输入
//@input:
// 		gpio: 需要输入外设信号的IO口;
// 		pfun: 需要从指定IO输入的外设信号, 可以输入的外设信号列表请查看gpio.h文件enum PFI_TABLE枚举项;
//@return:
// 		1)0: 执行正确;
//		2)-EINVAL: 传参出错;
//@note: 所映射的IO需要在设置IO状态为输入配置;
//@example: 将UART0的Rx信号设置为IO_PORTA_05口输入:
//			gpio_set_die(IO_PORTA_05, 1); 		//数字输入使能
//			gpio_set_pull_up(IO_PORTA_05, 1);  //上拉输入使能
//			gpio_direction_input(IO_PORTA_05);  //设置IO为输入状态
//			gpio_set_fun_input_port(IO_PORTA_05, PFI_UART0_RX); //将UART0的Rx信号设置为IO_PORTA_05口输入
//=================================================================================//
int gpio_set_fun_input_port(u32 gpio, enum PFI_TABLE pfun);

//=================================================================================//
//@brief: CrossBar 输入设置 API, 将某个外设信号释放IO口控制, 变为普通IO;
//@input:
// 		pfun: 需要释放由某个IO口输入的外设信号, 外设信号列表请查看gpio.h文件enum PFI_TABLE枚举项;
//@return:  默认为0, 无出错处理;
//@note:
//@example: 将外设信号PFI_UART0_RX释放由某个IO输入:
// 			gpio_disable_fun_input_port(PFI_UART0_RX);
//=================================================================================//
int gpio_disable_fun_input_port(enum PFI_TABLE pfun);

//=================================================================================//
//@brief: Output Channel输出设置 API, 将指定IO口设置为某个外设的输出
//@input:
// 		gpio: 需要输出外设信号的IO口;
// 		signal: 将enum OUTPUT_CH_SIGNAL列表中需要输出到指定IO口的外设信号, 可以输出的外设信号列表请查看gpio.h文件的enum OUTPUT_CH_SIGNAL枚举项;
//@return:  默认为0, 出错内部触发ASSERT;
//@note: 所映射的IO需要在设置IO状态为输出配置;
//@example: 将OUTPUT_CH_SIGNAL_MC_PWM0_H的Tx信号输出到IO_PORTA_05口:
// 			gpio_direction_output(IO_PORTA_05, 1); //设置IO为输出状态
//			gpio_och_sel_output_signal(IO_PORTA_05, OUTPUT_CH_SIGNAL_MC_PWM0_H); //将OUTPUT_CH_SIGNAL_MC_PWM0_H信号输出到IO_PORTA_05口
//=================================================================================//
int gpio_och_sel_output_signal(u32 gpio, enum OUTPUT_CH_SIGNAL signal);

//=================================================================================//
//@brief: Output Channel 输出设置 API, 将指定IO释放外设控制, 变为普通IO;
//@input:
// 		gpio: 需要释放外设控制IO口, 释放后变为普通IO模式;
// 		signal: 将enum OUTPUT_CH_SIGNAL列表中需要取消输出的外设信号, 外设信号列表请查看gpio.h文件的enum OUTPUT_CH_SIGNAL枚举项;;
//@return:  默认为0, 无出错处理;
//@note:
//@example: 将OUTPUT_CH_SIGNAL_MC_PWM0_H取消输出IO_PORTA_05:
// 			gpio_och_disable_output_signal(IO_PORTA_05, OUTPUT_CH_SIGNAL_MC_PWM0_H);
//=================================================================================//
int gpio_och_disable_output_signal(u32 gpio, enum OUTPUT_CH_SIGNAL signal);

//=================================================================================//
//@brief: Input Channel 输入设置 API, 将某个外设的输入设置为从某个IO输入
//@input:
// 		gpio: 需要输入外设信号的IO口;
// 		signal: 需要从指定IO输入的外设信号, 可以输入的外设信号列表请查看gpio.h文件enum INPUT_CH_SIGNAL枚举项;
//      type: INPUT_CH 类型, 常设为INPUT_CH_TYPE_GP_ICH;
//@return:  默认为0, 出错内部触发ASSERT;
//@note: 所映射的IO需要在设置IO状态为输入配置;
//@example: 将INPUT_CH_SIGNAL_TIMER0_CIN信号设置为IO_PORTA_05口输入:
//			gpio_set_die(IO_PORTA_05, 1); 		//数字输入使能
//			gpio_set_pull_up(IO_PORTA_05, 1);  //上拉输入使能
//			gpio_direction_input(IO_PORTA_05);  //设置IO为输入状态
//			gpio_ich_sel_input_signal(IO_PORTA_05, INPUT_CH_SIGNAL_TIMER0_CIN, INPUT_CH_TYPE_GP_ICH); //将INPUT_CH_SIGNAL_TIMER0_CIN信号设置为IO_PORTA_05口输入
//=================================================================================//
int gpio_ich_sel_input_signal(u32 gpio, enum INPUT_CH_SIGNAL signal, enum INPUT_CH_TYPE type);

//=================================================================================//
//@brief: Input Channel 输入设置 API, 将某个外设信号释放IO口控制, 变为普通IO;
//@input:
// 		gpio: 需要取消输入外设信号的IO口;
// 		signal: 需要取消输入的外设信号, 外设信号列表请查看gpio.h文件enum INPUT_CH_SIGNAL枚举项;
//      type: INPUT_CH 类型, 常设为INPUT_CH_TYPE_GP_ICH;
//@return:  默认为0, 无出错处理;
//@note:
//@example: 将外设信号INPUT_CH_SIGNAL_TIMER0_CIN释放由某个IO输入:
// 			gpio_ich_disable_input_signal(IO_PORTA_05, INPUT_CH_SIGNAL_TIMER0_CIN, INPUT_CH_TYPE_GP_ICH);
//=================================================================================//
int gpio_ich_disable_input_signal(u32 gpio, enum INPUT_CH_SIGNAL signal, enum INPUT_CH_TYPE type);

u32 gpio_get_ich_use_flag();
//获取空闲的gp_ich
//return: 0xff:error
u8 gpio_get_unoccupied_gp_ich();
//value:gp_ich序号
void gpio_release_gp_ich(u8 value);

#endif //_GPIO_HW_H
