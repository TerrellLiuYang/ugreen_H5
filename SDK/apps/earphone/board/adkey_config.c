#include "app_config.h"
#include "utils/syscfg_id.h"
#include "gpio_config.h"
#include "key/adkey.h"
#include "key_driver.h"

struct _cfg_adkey {
    u8  len;
    u16 uuid_key_value;
    u16 res;
} __attribute__((packed));


struct adkey_info {
    u8  len1;
    u16 io_uuid;
    u16 pull_up_type;
    u16 pull_up_value;
    u16 max_ad_value;
    u8 len2;
    u8  long_press_enable;
    u8  long_press_time;
    struct _cfg_adkey cfg_key_list[CONFIG_ADKEY_MAX_NUM];
} __attribute__((packed));


static struct adkey_platform_data platform_data;

static const u16 key_uuid_table[][2] = {
    { 0xD191, KEY_AD_NUM0 },
    { 0xD192, KEY_AD_NUM1 },
    { 0xD193, KEY_AD_NUM2 },
    { 0xD194, KEY_AD_NUM3 },
    { 0xD195, KEY_AD_NUM4 },
    { 0xD196, KEY_AD_NUM5 },
    { 0xD197, KEY_AD_NUM6 },
    { 0xD198, KEY_AD_NUM7 },
    { 0xD199, KEY_AD_NUM8 },
    { 0xD19A, KEY_AD_NUM9 },
    { 0x0402, KEY_AD_NUM10 },
    { 0x0403, KEY_AD_NUM11 },
    { 0x0404, KEY_AD_NUM12 },
    { 0x0405, KEY_AD_NUM13 },
    { 0x0406, KEY_AD_NUM14 },
    { 0x0407, KEY_AD_NUM15 },
    { 0x0408, KEY_AD_NUM16 },
    { 0x0409, KEY_AD_NUM17 },
    { 0x040a, KEY_AD_NUM18 },
    { 0x040b, KEY_AD_NUM19 },
};

u8 uuid2adkeyValue(u16 uuid)
{
    for (int i = 0; i < ARRAY_SIZE(key_uuid_table); i++) {
        if (key_uuid_table[i][0] == uuid) {
            return key_uuid_table[i][1];
        }
    }

    return 0xff;
}


const struct adkey_platform_data *get_adkey_platform_data()
{
    struct adkey_info info;
    u16 key_numbers;

    if (platform_data.enable) {
        return &platform_data;
    }

    int len = syscfg_read(CFG_ADKEY_ID, &info, sizeof(struct adkey_info));

    if (len <= 0) {
        puts("ERR:Can not read the adkey config,total adkeys should <= CONFIG_ADKEY_MAX_NUM\n");
        return NULL;
    }
    printf("adkey info len: %d\n", len);
    key_numbers =  CONFIG_ADKEY_MAX_NUM - ((sizeof(info) - len) / sizeof(struct _cfg_adkey));

    platform_data.adkey_pin = uuid2gpio(info.io_uuid);
    platform_data.extern_up_en = info.pull_up_type;
    platform_data.ad_channel = adc_io2ch(platform_data.adkey_pin);
    platform_data.long_press_enable = info.long_press_enable;
    platform_data.long_press_time = info.long_press_time;

    if (platform_data.ad_channel == 0xff) {
        puts("ERR:Please check that the ad key is correct\n");
        return NULL;
    }

    printf("key_numbers: %d\n", key_numbers);
    printf("adkey_pin: %d\n", platform_data.adkey_pin);
    printf("extern_up_en: %d\n", platform_data.extern_up_en);
    printf("pull_up_value: %d\n", info.pull_up_value);
    printf("ad_channel: %d\n", platform_data.ad_channel);
    printf("long_press_enable:%d time:%ds\n", platform_data.long_press_enable, platform_data.long_press_time);

    u16 i = 0;
    u16 j = 0;
    u16 tmp = 0;
    u16 ordering_res[CONFIG_ADKEY_MAX_NUM][2];

    //记录电阻与序号
    for (i = 0; i < key_numbers; i++) {
        ordering_res[i][0] =  info.cfg_key_list[i].res;
        ordering_res[i][1] =  i;
        /* printf("ordering_res:%d,s_number:%d\n", ordering_res[i][0], ordering_res[i][1]); */
    }
    //按电阻从小到大排序,序号要跟随电阻值
    for (i = 0; i < key_numbers - 1; i++) {
        for (j = 0; j < key_numbers - 1 - i; j++) {
            if (ordering_res[j][0] > ordering_res[j + 1][0]) {
                tmp = ordering_res[j][0];
                ordering_res[j][0] = ordering_res[j + 1][0];
                ordering_res[j + 1][0] = tmp;

                tmp = ordering_res[j][1];
                ordering_res[j][1] = ordering_res[j + 1][1];
                ordering_res[j + 1][1] = tmp;
            }
        }
    }

    /*for test查看排序后的数据*/
    /* for (i = 0; i < key_numbers; i++) { */
    /*     printf("after ordering res:%d,s_number:%d\n", ordering_res[i][0], ordering_res[i][1]); */
    /* } */

    //配置 ad判断值与key值
    u16 tmp_value1 = 0;
    u16 tmp_value2 = 0;
    for (i = 0; i < key_numbers; i++) {
        tmp_value1 = info.max_ad_value * ordering_res[i][0] / (ordering_res[i][0] + info.pull_up_value);
        if (i == key_numbers - 1) {
            tmp_value2 = info.max_ad_value;
        } else {
            tmp_value2 = info.max_ad_value * ordering_res[i + 1][0] / (ordering_res[i + 1][0] + info.pull_up_value);
        }
        platform_data.ad_value[i] = (tmp_value1 + tmp_value2) / 2;
        platform_data.key_value[i] = uuid2adkeyValue(info.cfg_key_list[ordering_res[i][1]].uuid_key_value);

        printf("ADkey:%d,judge_advalue:%d,key_value:%d\n", i, platform_data.ad_value[i], platform_data.key_value[i]);
    }

    platform_data.enable    = 1;

    return &platform_data;
}


bool is_adkey_press_down()
{
#if TCFG_ADKEY_ENABLE
    if (platform_data.enable == 0) {
        return false;
    }

    if (adc_get_value(platform_data.ad_channel) < 10) {
        return true;
    }
#endif
    return false;
}

int get_adkey_io()
{
#if TCFG_ADKEY_ENABLE
    if (platform_data.enable) {
        return platform_data.adkey_pin;
    }
#endif
    return -1;
}

