#include "ability_uuid.h"
#include "app_main.h"
#include "utils/syscfg_id.h"
#include "gpio_config.h"
#include "led/led_ui_manager.h"

static void led_do_state(struct scene_param *param)
{
    struct scene_private_data  *priv = (struct scene_private_data *)param->user_type;

    u16 uuid = param->arg[0] | (param->arg[1] << 8);
    printf("led display: %d, %x\n", priv->local_action, uuid);

    // led_ui_manager_display(priv->local_action, uuid, 0);

    return;
}

static void led_insert_state(struct scene_param *param)
{
    struct scene_private_data  *priv = (struct scene_private_data *)param->user_type;

    u16 uuid = param->arg[0] | (param->arg[1] << 8);
    printf("led insert display: %d, %x\n", priv->local_action, uuid);

    // led_ui_manager_display(priv->local_action, uuid, 1);
}


static const struct scene_action led_action_table[] = {
    [0] = {
        .action = led_do_state
    },
    [1] = {
        .action = led_insert_state
    },
};

REGISTER_SCENE_ABILITY(led_ability) = {
    .uuid   = UUID_LED,
    .action = led_action_table,
};


