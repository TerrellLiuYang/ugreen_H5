#include "app_config.h"
#include "utils/syscfg_id.h"
#include "gpio_config.h"
#include "key/iokey.h"
#include "key_driver.h"

#ifndef CONFIG_IOKEY_MAX_NUM
#define CONFIG_IOKEY_MAX_NUM 5
#endif

struct iokey_info {
    u8  len;
    u16 key_uuid;
    u16 io_uuid;
    u8 detect;
    u8 long_press_enable;
    u8 long_press_time;
} __attribute__((packed));


static struct iokey_port iokey_ports[CONFIG_IOKEY_MAX_NUM];
static struct iokey_platform_data platform_data;

static const u16 key_uuid_table[][2] = {
    { 0xaefa, KEY_POWER },
    { 0x368c, KEY_NEXT },
    { 0x842a, KEY_PREV },
    { 0x0bb0, KEY_SLIDER },
    { 0xd212, KEY_MODE },
    { 0x6a23, KEY_PLAY },
};

u8 uuid2keyValue(u16 uuid)
{
    for (int i = 0; i < ARRAY_SIZE(key_uuid_table); i++) {
        if (key_uuid_table[i][0] == uuid) {
            return key_uuid_table[i][1];
        }
    }

    return 0xff;
}

const struct iokey_platform_data *get_iokey_platform_data()
{
    struct iokey_info info[CONFIG_IOKEY_MAX_NUM];

    if (platform_data.enable) {
        return &platform_data;
    }

    int len = syscfg_read(CFG_IOKEY_ID, info, sizeof(info));
    if (len <= 0) {
        return NULL;
    }
    printf("iokey_len: %d, %x, %x\n", len, info[0].key_uuid, info[0].io_uuid);

    platform_data.num       = len / sizeof(struct iokey_info);
    platform_data.port      = iokey_ports;
    platform_data.enable    = 1;

    for (int i = 0; i < platform_data.num; i++) {
        if (info[i].detect == 1) {
            iokey_ports[i].connect_way = ONE_PORT_TO_LOW;
        } else {
            iokey_ports[i].connect_way = ONE_PORT_TO_HIGH;
        }
        iokey_ports[i].key_type.one_io.port = uuid2gpio(info[i].io_uuid);
        iokey_ports[i].key_value = uuid2keyValue(info[i].key_uuid);
        if (info[i].long_press_enable) {
            platform_data.long_press_enable = 1;
            platform_data.long_press_time = info[i].long_press_time;
            platform_data.long_press_port = uuid2gpio(info[i].io_uuid);
            platform_data.long_press_level = (info[i].detect == 1) ? 0 : 1;
        }
        printf("iokey:%d,prot:%d,value:%d,c_way:%d long_press_en:%d time:%ds\n", i, iokey_ports[i].key_type.one_io.port, iokey_ports[i].key_value, iokey_ports[i].connect_way, info[i].long_press_enable, info[i].long_press_time);
    }

    return &platform_data;
}

bool is_iokey_press_down()
{
#if TCFG_IOKEY_ENABLE
    if (platform_data.enable == 0) {
        return false;
    }
    for (int i = 0; i < platform_data.num; i++) {
        if (iokey_ports[i].key_value == KEY_POWER) {
            int value = gpio_read(iokey_ports[i].key_type.one_io.port);
            if (iokey_ports[i].connect_way == ONE_PORT_TO_LOW) {
                return value == 0 ? true : false;
            }
            return value == 1 ? true : false;
        }
    }
#endif
    return false;
}


