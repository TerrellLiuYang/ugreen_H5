#include "app_main.h"
#include "ability_uuid.h"
#include "battery_manager.h"
#include "app_power_manage.h"



static const struct scene_event bat_event_table[] = {
    [0] = {
        .event = POWER_EVENT_POWER_WARNING,
    },
    [1] = {
        .event = POWER_EVENT_POWER_LOW,
    },
    [2] = {
        .event = BAT_MSG_CHARGE_START,
    },
    [3] = {
        .event = BAT_MSG_CHARGE_FULL,
    },
    [4] = {
        .event = BAT_MSG_CHARGE_CLOSE,
    },

    { 0xffffffff, NULL },
};

static bool is_level_big_than(struct scene_param *param)
{
    return get_vbat_percent() >= param->arg[0];
}

static bool is_level_less_than(struct scene_param *param)
{
    return get_vbat_percent() <= param->arg[0];
}

static bool is_voltage_big_than(struct scene_param *param)
{
    return get_vbat_percent() >= *(u16 *)param->arg;
}

static bool is_voltage_less_than(struct scene_param *param)
{
    return get_vbat_percent() <= *(u16 *)param->arg;
}


static const struct scene_state bat_state_table[] = {
    [0] = {
        .match = is_level_big_than,
    },
    [1] = {
        .match = is_level_less_than,
    },
    [2] = {
        .match = is_voltage_big_than,
    },
    [3] = {
        .match = is_voltage_less_than,
    },

    { NULL }
};



REGISTER_SCENE_ABILITY(battery_ability_entry) = {
    .uuid   = UUID_BATTERY,
    .event  = bat_event_table,
    .state  = bat_state_table,
};

static int battery_msg_handler(int *msg)
{
    scene_mgr_event_match(UUID_BATTERY, msg[0], msg);
    return 0;
}
APP_MSG_HANDLER(bat_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BATTERY,
    .handler    = battery_msg_handler,
};

