#include "gpio_config.h"
#include "syscfg_id.h"
#include "system/init.h"
#include "cpu/includes.h"
#include "app_config.h"
#include "key/iokey.h"
#include "asm/power_interface.h"

struct gpio_cfg_item {
    u8 len;
    u16 uuid;
    u8  mode;
    u8  hd;
} __attribute__((packed));


extern const struct iokey_platform_data *get_iokey_platform_data();
extern void lp_touch_key_disable(void);

static void gpio_config_at_power_off(u16 *port_group);


//maskrom 使用到的io
static void mask_io_cfg()
{
    struct boot_soft_flag_t boot_soft_flag = {0};
    boot_soft_flag.flag0.boot_ctrl.wdt_dis = 0;
    boot_soft_flag.flag0.boot_ctrl.poweroff = 0;
    boot_soft_flag.flag0.boot_ctrl.is_port_b = JL_IOMAP->CON0 & BIT(16) ? 1 : 0;

    boot_soft_flag.flag1.misc.usbdm = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag1.misc.usbdp = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag1.misc.uart_key_port = 0;
    boot_soft_flag.flag1.misc.ldoin = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag2.pa7_pb4.pa7 = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag2.pa7_pb4.pb4 = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag3.pc3_pc5.pc3 = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag3.pc3_pc5.pc5 = SOFTFLAG_HIGH_RESISTANCE;
    mask_softflag_config(&boot_soft_flag);

}


enum {
    PORTA_GROUP = 0,
    PORTB_GROUP,
    PORTC_GROUP,
    PORTD_GROUP,
    PORTP_GROUP,
};

static void port_protect(u16 *port_group, u32 port_num)
{
    if (port_num == NO_CONFIG_PORT) {
        return;
    }
    port_group[port_num / IO_GROUP_NUM] &= ~BIT(port_num % IO_GROUP_NUM);
}

extern u8 chargestore_get_power_status(void);
extern u8 is_power_level_low(void);
/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
void gpio_config_soft_poweroff(void)
{
    y_printf("gpio_config_soft_poweroff");
    y_printf("check storage mode: %d, %d, %d!!!", chargestore_get_power_status(), testbox_get_ex_enter_storage_mode_flag(), is_power_level_low());
    //关机都关触摸，只能充电仓激活开机
    lp_touch_key_disable();
    u16 port_group[] = {
        [PORTA_GROUP] = 0x1ff,
        [PORTB_GROUP] = 0x3ff,//
        [PORTC_GROUP] = 0x7f,//
        [PORTD_GROUP] = 0x7f,
        [PORTP_GROUP] = 0x1,//
    };

    mask_io_cfg();

    extern u32 spi_get_port(void);
    if (spi_get_port() == 0) {
        port_group[PORTD_GROUP] &= ~0x1f;
    } else if (spi_get_port() == 1) {
        port_group[PORTD_GROUP] &= ~0x73;
    }

    if (get_sfc_bit_mode() == 4) {
        port_group[PORTC_GROUP] &= ~BIT(6);
        port_group[PORTA_GROUP] &= ~BIT(7);
    }

#if TCFG_IOKEY_ENABLE
    port_protect(port_group, get_iokey_platform_data()->port[0].key_type.one_io.port);
#endif

    gpio_config_at_power_off(port_group);

    gpio_set_mode(PORTA, port_group[PORTA_GROUP], PORT_HIGHZ);
    gpio_keep_mode_at_sleep(PORTA, ~port_group[PORTA_GROUP]);
    gpio_set_mode(PORTB, port_group[PORTB_GROUP], PORT_HIGHZ);
    gpio_keep_mode_at_sleep(PORTB, ~port_group[PORTB_GROUP]);
    gpio_set_mode(PORTC, port_group[PORTC_GROUP], PORT_HIGHZ);
    gpio_keep_mode_at_sleep(PORTC, ~port_group[PORTC_GROUP]);
    gpio_set_mode(PORTD, port_group[PORTD_GROUP], PORT_HIGHZ);
    gpio_keep_mode_at_sleep(PORTD, ~port_group[PORTD_GROUP]);
    gpio_set_mode(PORTP, port_group[PORTP_GROUP], PORT_HIGHZ);
    gpio_keep_mode_at_sleep(PORTP, ~port_group[PORTP_GROUP]);

    gpio_set_mode(PORTUSB, IO_PORT_USB_MASK, PORT_HIGHZ); //dp dm
    gpio_keep_mode_at_sleep(PORTUSB, ~IO_PORT_USB_MASK);

#if (TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_ANC_BOX_ENABLE)
    power_wakeup_index_enable(2, 0);
    gpio_set_mode(IO_PORT_SPILT(TCFG_CHARGESTORE_PORT), PORT_HIGHZ);
#endif

//船运模式IO设置
#if 1//TCFG_CHARGESTORE_ENABLE
    // y_printf("check storage mode: %d, %d, %d!!!", chargestore_get_power_status(), testbox_get_ex_enter_storage_mode_flag(), is_power_level_low());
    // if ((chargestore_get_power_status()==0) || testbox_get_ex_enter_storage_mode_flag() || is_power_level_low()) { 
    if (testbox_get_ex_enter_storage_mode_flag() || is_power_level_low()) { 
        //船运输出高电平
        y_printf("enter storage mode!!!");
#if TCFG_LP_TOUCH_KEY_ENABLE
        power_set_vddio_keep(0);                //关VDDIO KEEP
        // extern void lp_touch_key_disable(void);
        // lp_touch_key_disable();                 //仓储模式关内置触摸
#endif
        gpio_set_mode(IO_PORT_SPILT(IO_PORTB_09), PORT_OUTPUT_HIGH);
    } else {
        gpio_set_mode(IO_PORT_SPILT(IO_PORTB_09), PORT_OUTPUT_LOW);
    }
#endif
    P11_PORT_CON0 = 0;
}

static u32 usb_io_con = 0;

void gpio_enter_sleep_config()
{
    usb_io_con = JL_USB_IO->CON0;

    gpio_set_mode(PORTUSB, IO_PORT_USB_MASK, PORT_HIGHZ); //dp dm
}

void gpio_exit_sleep_config()
{
    JL_USB_IO->CON0 = usb_io_con;
}

int gpio_config_at_power_on()
{
#if TCFG_IO_CFG_AT_POWER_ON
    struct gpio_cfg_item item;
    puts("gpio_cfg_at_power_on\n");

    for (int i = 0; ; i++) {
        int len = syscfg_read_item(CFG_ID_IO_CFG_AT_POWERON, i, &item, sizeof(item));
        if (len != sizeof(item)) {
            break;
        }
        u8 gpio = uuid2gpio(item.uuid);
        struct gpio_config config = {
            .pin   = BIT(gpio % 16),
            .mode   = item.mode,
            .hd     = item.hd,
        };
        gpio_init(gpio / 16, &config);
    }
#endif
    return 0;
}

static void gpio_config_at_power_off(u16 *port_group)
{
#if TCFG_IO_CFG_AT_POWER_OFF
    struct gpio_cfg_item item;

    for (int i = 0; ; i++) {
        int len = syscfg_read_item(CFG_ID_IO_CFG_AT_POWEROFF, i, &item, sizeof(item));
        if (len != sizeof(item)) {
            break;
        }
        u8 gpio = uuid2gpio(item.uuid);
        struct gpio_config config = {
            .pin   = BIT(gpio % 16),
            .mode   = item.mode,
            .hd     = item.hd,
        };
        gpio_init(gpio / 16, &config);
        port_protect(port_group, gpio);
    }
#endif
}

static int _gpio_init()
{
#if 0
    //disable PA6 PC3 USB_IO by ic
    {
        gpio_set_die(IO_PORTA_05, 0);
        gpio_set_dieh(IO_PORTA_05, 0);

        gpio_set_die(IO_PORTA_06, 0);
        gpio_set_dieh(IO_PORTA_06, 0);

        gpio_set_die(IO_PORTC_03, 0);
        gpio_set_dieh(IO_PORTC_03, 0);

        gpio_set_die(IO_PORT_DP, 0);
        gpio_set_dieh(IO_PORT_DP, 0);

        /************mic**************/
        gpio_set_pull_up(IO_PORTC_04, 0);
        gpio_set_pull_down(IO_PORTC_04, 0);
        gpio_set_die(IO_PORTC_04, 0);
        gpio_set_dieh(IO_PORTC_04, 0);

        gpio_set_direction(IO_PORT_DM, 0);
        gpio_set_output_value(IO_PORT_DM, 1);
    }
#endif
    gpio_config_at_power_on();
    return 0;
}
early_initcall(_gpio_init);

static void gpio_uninit()
{
    gpio_config_soft_poweroff();
}
platform_uninitcall(gpio_uninit);


