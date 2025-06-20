#include "app_config.h"
#include "cpu/includes.h"
#include "asm/dac.h"
#include "asm/charge.h"


const struct low_power_param power_param = {
    //sniff时芯片是否进入低功耗
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,

    //外接晶振频率
    .btosc_hz       = TCFG_CLOCK_OSC_HZ,

    //提供给低功耗模块的延时(不需要需修改)
    .delay_us       = TCFG_CLOCK_SYS_HZ / 1000000L,

    //进入低功耗时BTOSC是否保持
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,

    //强VDDIO等级,可选：2.0V  2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.6V
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,

    //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,

    .osc_type       = TCFG_LOWPOWER_OSC_TYPE,

#if TCFG_LP_TOUCH_KEY_ENABLE
    .lpctmu_en 		= TCFG_LP_TOUCH_KEY_ENABLE,
#endif

    .vddio_keep     = TCFG_LOWPOWER_VDDIO_KEEP,

    .light_sleep_attribute = TCFG_LOWPOWER_LIGHT_SLEEP_ATTRIBUTE,
};

static struct port_wakeup port0 = {
    .pullup_down_enable = 1,                //配置I/O 内部上下拉是否使能
    .edge               = FALLING_EDGE,     //唤醒方式选择,可选：上升沿\下降沿
    .filter             = PORT_FLT_8ms,
    .iomap              = IO_PORTB_01,      //唤醒口选择
};

#if (TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_ANC_BOX_ENABLE)
struct port_wakeup port1 = {
    .pullup_down_enable = DISABLE,          //配置I/O 内部上下拉是否使能
    .edge               = FALLING_EDGE,     //唤醒方式选择,可选：上升沿\下降沿
    .filter             = PORT_FLT_1ms,
    .iomap              = TCFG_CHARGESTORE_PORT,    //唤醒口选择
};
#endif

struct port_wakeup charge_port = {
    .edge               = RISING_EDGE,      //唤醒方式选择,可选：上升沿\下降沿\双边沿
    .filter             = PORT_FLT_16ms,
    .iomap              = IO_CHGFL_DET,     //唤醒口选择
};

struct port_wakeup vbat_port = {
    .edge               = BOTH_EDGE,        //唤醒方式选择,可选：上升沿\下降沿\双边沿
    .filter             = PORT_FLT_16ms,
    .iomap              = IO_VBTCH_DET,     //唤醒口选择
};

struct port_wakeup ldoin_port = {
    .edge               = BOTH_EDGE,        //唤醒方式选择,可选：上升沿\下降沿\双边沿
    .filter             = PORT_FLT_16ms,
    .iomap              = IO_LDOIN_DET,     //唤醒口选择
};

static const struct wakeup_param wk_param = {
#if (!TCFG_LP_TOUCH_KEY_ENABLE)
    .port[1] = &port0,
#endif
#if (TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_ANC_BOX_ENABLE)
    .port[2] = &port1,
#endif

#if TCFG_CHARGE_ENABLE
    .aport[0] = &charge_port,
    .aport[1] = &vbat_port,
    .aport[2] = &ldoin_port,
#endif
};


extern void dac_config_at_sleep();
extern void gpio_enter_sleep_config();
extern void gpio_exit_sleep_config();

static void sleep_enter_callback(u8 step)
{
    /* 此函数禁止添加打印 */
    putchar('<');
    dac_config_at_sleep();
    gpio_enter_sleep_config();
}

static void sleep_exit_callback(u32 usec)
{
    gpio_exit_sleep_config();
    putchar('>');
}

static void board_set_soft_poweroff(void)
{
    do_platform_uninitcall();
}

static void port_wakeup_callback(u8 index, u8 gpio)
{
    switch (index) {
#if (TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_ANC_BOX_ENABLE)
    case 2:
        extern void chargestore_ldo5v_fall_deal(void);
        chargestore_ldo5v_fall_deal();
        break;
#endif
    }
}

static void aport_wakeup_callback(u8 index, u8 gpio, POWER_WKUP_EDGE edge)
{
#if TCFG_CHARGE_ENABLE
    switch (gpio) {
    case IO_CHGFL_DET://charge port
        charge_wakeup_isr();
        break;
    case IO_VBTCH_DET://vbat port
    case IO_LDOIN_DET://ldoin port
        ldoin_wakeup_isr();
        break;
    }
#endif
}



void board_power_init(void)
{
    puts("power config\n");

    power_init(&power_param);

    power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL,
                       sleep_enter_callback, sleep_exit_callback,
                       board_set_soft_poweroff);

    power_wakeup_init(&wk_param);

    power_awakeup_set_callback(aport_wakeup_callback);
    power_wakeup_set_callback(port_wakeup_callback);

#if (!TCFG_CHARGE_ENABLE)
    power_set_mode(TCFG_LOWPOWER_POWER_SEL);
#endif
}

