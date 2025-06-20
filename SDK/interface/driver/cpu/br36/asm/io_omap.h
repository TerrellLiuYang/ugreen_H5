
//===============================================================================//
//
//      output function define
//
//===============================================================================//
#define FO_GP_OCH0        ((0 << 2)|BIT(1))
#define FO_GP_OCH1        ((1 << 2)|BIT(1))
#define FO_GP_OCH2        ((2 << 2)|BIT(1))
#define FO_GP_OCH3        ((3 << 2)|BIT(1))
#define FO_GP_OCH4        ((4 << 2)|BIT(1))
#define FO_GP_OCH5        ((5 << 2)|BIT(1))
#define FO_GP_OCH6        ((6 << 2)|BIT(1))
#define FO_GP_OCH7        ((7 << 2)|BIT(1))
#define FO_GP_OCH8        ((8 << 2)|BIT(1))
#define FO_GP_OCH9        ((9 << 2)|BIT(1))
#define FO_GP_OCH10        ((10 << 2)|BIT(1))
#define FO_GP_OCH11        ((11 << 2)|BIT(1))
#define FO_TMR0_PWM        ((12 << 2)|BIT(1)|BIT(0))
#define FO_TMR1_PWM        ((13 << 2)|BIT(1)|BIT(0))
#define FO_TMR2_PWM        ((14 << 2)|BIT(1)|BIT(0))
#define FO_TMR3_PWM        ((15 << 2)|BIT(1)|BIT(0))
#define FO_TMR4_PWM        ((16 << 2)|BIT(1)|BIT(0))
#define FO_TMR5_PWM        ((17 << 2)|BIT(1)|BIT(0))
#define FO_SPI0_CLK        ((18 << 2)|BIT(1)|BIT(0))
#define FO_SPI0_DA0        ((19 << 2)|BIT(1)|BIT(0))
#define FO_SPI0_DA1        ((20 << 2)|BIT(1)|BIT(0))
#define FO_SPI0_DA2        ((21 << 2)|BIT(1)|BIT(0))
#define FO_SPI0_DA3        ((22 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_CLK        ((23 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA0        ((24 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA1        ((25 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA2        ((26 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA3        ((27 << 2)|BIT(1)|BIT(0))
#define FO_SPI2_CLK        ((28 << 2)|BIT(1)|BIT(0))
#define FO_SPI2_DA0        ((29 << 2)|BIT(1)|BIT(0))
#define FO_SPI2_DA1        ((30 << 2)|BIT(1)|BIT(0))
#define FO_SPI2_DA2       ((31 << 2)|BIT(1)|BIT(0))
#define FO_SPI2_DA3       ((32 << 2)|BIT(1)|BIT(0))
#define FO_SD0_CLK        ((33 << 2)|BIT(1)|BIT(0))
#define FO_SD0_CMD        ((34 << 2)|BIT(1)|BIT(0))
#define FO_SD0_DA0        ((35 << 2)|BIT(1)|BIT(0))
#define FO_SD0_DA1        ((36 << 2)|BIT(1)|BIT(0))
#define FO_SD0_DA2        ((37 << 2)|BIT(1)|BIT(0))
#define FO_SD0_DA3        ((38 << 2)|BIT(1)|BIT(0))
#define FO_IIC_SCL         ((39 << 2)|BIT(1)|BIT(0))
#define FO_IIC_SDA         ((40 << 2)|BIT(1)|BIT(0))
#define FO_LEDC0_DO         ((41 << 2)|BIT(1)|BIT(0))
#define FO_LEDC1_DO        ((42 << 2)|BIT(1)|BIT(0))
#define FO_UART0_TX         ((43 << 2)|BIT(1)|BIT(0))
#define FO_UART1_TX         ((44 << 2)|BIT(1)|BIT(0))
#define FO_UART1_RTS        ((45 << 2)|BIT(1)|BIT(0))
#define FO_UART2_TX        ((46 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_MCLK        ((47 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_LRCK        ((48 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_SCLK        ((49 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT0        ((50 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT1        ((51 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT2        ((52 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT3        ((53 << 2)|BIT(1)|BIT(0))
#define FO_ANC_MICCK	    ((54 << 2)|BIT(1)|BIT(0))
#define FO_PLNK_SCLK         ((55 << 2)|BIT(1)|BIT(0))
#define FO_CHAIN_OUT0        ((56 << 2)|BIT(1)|BIT(0))
#define FO_CHAIN_OUT1        ((57 << 2)|BIT(1)|BIT(0))
#define FO_CHAIN_OUT2        ((58 << 2)|BIT(1)|BIT(0))
#define FO_CHAIN_OUT3        ((59 << 2)|BIT(1)|BIT(0))

//===============================================================================//
//
//      IO output select sfr
//
//===============================================================================//
typedef struct {
    __RW __u8 PA0_OUT;
    __RW __u8 PA1_OUT;
    __RW __u8 PA2_OUT;
    __RW __u8 PA3_OUT;
    __RW __u8 PA4_OUT;
    __RW __u8 PA5_OUT;
    __RW __u8 PA6_OUT;
    __RW __u8 PA7_OUT;
    __RW __u8 PA8_OUT;
    __RW __u8 PB0_OUT;
    __RW __u8 PB1_OUT;
    __RW __u8 PB2_OUT;
    __RW __u8 PB3_OUT;
    __RW __u8 PB4_OUT;
    __RW __u8 PB5_OUT;
    __RW __u8 PB6_OUT;
    __RW __u8 PB7_OUT;
    __RW __u8 PB8_OUT;
    __RW __u8 PB9_OUT;
    __RW __u8 PC0_OUT;
    __RW __u8 PC1_OUT;
    __RW __u8 PC2_OUT;
    __RW __u8 PC3_OUT;
    __RW __u8 PC4_OUT;
    __RW __u8 PC5_OUT;
    __RW __u8 PC6_OUT;
    __RW __u8 USBDP_OUT;
    __RW __u8 USBDM_OUT;
    __RW __u8 PP0_OUT;
} JL_OMAP_TypeDef;

#define JL_OMAP_BASE      (ls_base + map_adr(0x56, 0x00))
#define JL_OMAP           ((JL_OMAP_TypeDef   *)JL_OMAP_BASE)

