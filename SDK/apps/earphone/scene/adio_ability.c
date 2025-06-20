#include "ability_uuid.h"
#include "app_main.h"
#include "gpio_config.h"
#include "gpadc.h"

struct adio_param {
    u16 uuid;
    u16 value;
} __attribute__((packed));

static int adio_get_value(struct scene_param *param, enum gpio_mode mode)
{
    int adc_value = 0;
    struct adio_param *p = (struct adio_param *)param->arg;

    u8 gpio = uuid2gpio(p->uuid);
    struct gpio_config config = {
        .pin   = BIT(gpio % 16),
        .mode   = mode,
        .hd     = 0,
    };
    gpio_init(gpio / 16, &config);

    u8 adc_ch = adc_io2ch(gpio);
    if (adc_ch != 0xff) {
        for (int i = 0; i < 4; i++) {
            adc_value += adc_get_value_blocking(adc_ch);
        }
        adc_value /= 4;
        printf("adc_value: %d\n", adc_value);
    }

    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_HIGHZ);

    return adc_value;
}

static bool adio_up_value_big_than(struct scene_param *param)
{
    struct adio_param *p = (struct adio_param *)param->arg;

    return adio_get_value(param, PORT_INPUT_PULLUP_10K) > p->value ? 1 : 0;
}

static bool adio_up_value_less_than(struct scene_param *param)
{
    struct adio_param *p = (struct adio_param *)param->arg;

    return adio_get_value(param, PORT_INPUT_PULLUP_10K) < p->value ? 1 : 0;
}

static bool adio_down_value_big_than(struct scene_param *param)
{
    struct adio_param *p = (struct adio_param *)param->arg;

    return adio_get_value(param, PORT_INPUT_PULLDOWN_10K) > p->value ? 1 : 0;
}

static bool adio_down_value_less_than(struct scene_param *param)
{
    struct adio_param *p = (struct adio_param *)param->arg;

    return adio_get_value(param, PORT_INPUT_PULLDOWN_10K) < p->value ? 1 : 0;
}

static bool adio_float_value_big_than(struct scene_param *param)
{
    struct adio_param *p = (struct adio_param *)param->arg;

    return adio_get_value(param, PORT_INPUT_FLOATING) > p->value ? 1 : 0;
}

static bool adio_float_value_less_than(struct scene_param *param)
{
    struct adio_param *p = (struct adio_param *)param->arg;

    return adio_get_value(param, PORT_INPUT_FLOATING) < p->value ? 1 : 0;
}

static const struct scene_state adio_state_table[] = {
    [0] = {
        .match = adio_up_value_big_than,
    },
    [1] = {
        .match = adio_up_value_less_than,
    },
    [2] = {
        .match = adio_down_value_big_than,
    },
    [3] = {
        .match = adio_down_value_less_than,
    },
    [4] = {
        .match = adio_float_value_big_than,
    },
    [5] = {
        .match = adio_float_value_less_than,
    },

};

REGISTER_SCENE_ABILITY(adio_ability) = {
    .uuid   = UUID_ADIO,
    .state  = adio_state_table,
};


