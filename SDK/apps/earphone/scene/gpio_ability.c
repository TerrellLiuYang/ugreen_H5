#include "ability_uuid.h"
#include "app_main.h"
#include "gpio_config.h"

struct gpio_param {
    u16 uuid;
    u8  hd;
} __attribute__((packed));


static void gpio_set_output_0(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    u8 gpio = uuid2gpio(*(u16 *)param->arg);
    struct gpio_config config = {
        .pin   = BIT(gpio % 16),
        .mode   = PORT_OUTPUT_LOW,
        .hd     = param->arg[2],
    };

    gpio_init(gpio / 16, &config);
}

static void gpio_set_output_1(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    u8 gpio = uuid2gpio(*(u16 *)param->arg);
    struct gpio_config config = {
        .pin   = BIT(gpio % 16),
        .mode   = PORT_OUTPUT_HIGH,
        .hd     = param->arg[2],
    };

    gpio_init(gpio / 16, &config);
}

static void gpio_set_output_toggle(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    u8 gpio = uuid2gpio(*(u16 *)param->arg);

#ifndef CONFIG_CPU_BR27
    local_irq_disable();
    gpio_toggle_port(gpio / 16, BIT(gpio % 16));
    gpio_set_drive_strength(gpio / 16, BIT(gpio % 16), param->arg[2]);
    local_irq_enable();
#endif
}

static void gpio_set_pu_input(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    u8 gpio = uuid2gpio(*(u16 *)param->arg);
    struct gpio_config config = {
        .pin   = BIT(gpio % 16),
        .mode   = PORT_INPUT_PULLUP_10K,
    };

    gpio_init(gpio / 16, &config);
}

static void gpio_set_pd_input(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    u8 gpio = uuid2gpio(*(u16 *)param->arg);
    struct gpio_config config = {
        .pin   = BIT(gpio % 16),
        .mode   = PORT_INPUT_PULLDOWN_10K,
    };

    gpio_init(gpio / 16, &config);
}

static void gpio_set_hi_z(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    u8 gpio = uuid2gpio(*(u16 *)param->arg);
    struct gpio_config config = {
        .pin   = BIT(gpio % 16),
        .mode   = PORT_HIGHZ,
    };

    gpio_init(gpio / 16, &config);
}

static const struct scene_action gpio_action_table[] = {
    [0] = {
        .action = gpio_set_output_0,
    },
    [1] = {
        .action = gpio_set_output_1,
    },
    [2] = {
        .action = gpio_set_output_toggle,
    },
    [3] = {
        .action = gpio_set_pu_input,
    },
    [4] = {
        .action = gpio_set_pd_input,
    },
    [5] = {
        .action = gpio_set_hi_z,
    },

};

REGISTER_SCENE_ABILITY(gpio_ability) = {
    .uuid   = UUID_IO,
    .action = gpio_action_table,
};


