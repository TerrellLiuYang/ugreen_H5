#include "ability_uuid.h"
#include "app_main.h"
#include "bt_tws.h"
#include "app_tone.h"


static bool app_enter_bt_mode(struct scene_param *param)
{
    int *msg = (int *)param->msg;
    if (msg) {
        int mode = msg[1] & 0xff;
        return mode == APP_MODE_BT;
    }
    return 1;
}

static bool app_exit_bt_mode(struct scene_param *param)
{
    int *msg = (int *)param->msg;
    if (msg) {
        int mode = msg[1] & 0xff;
        return mode == APP_MODE_BT;
    }
    return 1;
}


static const struct scene_event app_event_table[] = {
    [0] = {
        .event = APP_MSG_POWER_ON
    },
    [1] = {
        .event = APP_MSG_POWER_OFF
    },
    [2] = {
        .event = APP_MSG_ENTER_MODE,
        .match = app_enter_bt_mode,
    },
    [3] = {
        .event = APP_MSG_EXIT_MODE,
        .match = app_exit_bt_mode,
    },

    { 0xffffffff, NULL },
};

static bool app_is_bt_mode(struct scene_param *param)
{
    return app_in_mode(APP_MODE_BT);
}

static bool app_is_idle_mode(struct scene_param *param)
{
    return app_in_mode(APP_MODE_IDLE);
}

static bool app_is_poweroff_step(struct scene_param *param)
{
    return app_var.goto_poweroff_flag;
}

static bool app_is_not_poweroff_step(struct scene_param *param)
{
    return !app_var.goto_poweroff_flag;
}

static bool app_is_poweron_by_key(struct scene_param *param)
{
    return app_var.poweron_reason == SYS_POWERON_BY_KEY ? 1 : 0;
}

static bool app_is_poweron_by_out_box(struct scene_param *param)
{
    return app_var.poweron_reason == SYS_POWERON_BY_OUT_BOX ? 1 : 0;
}

static bool app_is_poweroff_by_key(struct scene_param *param)
{
    return app_var.poweroff_reason == SYS_POWEROFF_BY_KEY ? 1 : 0;
}

static bool app_is_poweroff_by_in_box(struct scene_param *param)
{
    return app_var.poweroff_reason == SYS_POWEROFF_BY_IN_BOX ? 1 : 0;
}

static bool app_is_poweroff_by_timeout(struct scene_param *param)
{
    return app_var.poweroff_reason == SYS_POWEROFF_BY_TIMEOUT ? 1 : 0;
}

static bool app_is_poweron_time_less_than(struct scene_param *param)
{
    return jiffies_msec() / 1000 < param->arg[0] ? 1 : 0;
}

static bool app_is_poweron_time_big_than(struct scene_param *param)
{
    return jiffies_msec() / 1000 > param->arg[0] ? 1 : 0;
}


static const struct scene_state app_state_table[] = {
    [0] = {
        .match = app_is_bt_mode
    },
    [1] = {
        .match = app_is_idle_mode
    },
    [2] = {
        .match = app_is_poweroff_step
    },
    [3] = {
        .match = app_is_not_poweroff_step
    },
    [4] = {
        .match = app_is_poweron_by_key
    },
    [5] = {
        .match = app_is_poweron_by_out_box
    },
    [6] = {
        .match = app_is_poweroff_by_key
    },
    [7] = {
        .match = app_is_poweroff_by_in_box
    },
    [8] = {
        .match = app_is_poweroff_by_timeout
    },
    [9] = {
        .match = app_is_poweron_time_less_than
    },
    [10] = {
        .match = app_is_poweron_time_big_than
    },
};


#if TCFG_USER_TWS_ENABLE
enum {
    API_POWER_OFF,
    API_TWS_POWER_OFF,
};

static void call_app_api(int api, int err)
{
    switch (api) {
    case API_POWER_OFF:
        sys_enter_soft_poweroff(POWEROFF_NORMAL);
        break;
    case API_TWS_POWER_OFF:
        sys_enter_soft_poweroff(POWEROFF_NORMAL_TWS);
        break;
    default:
        break;
    }
}

TWS_SYNC_CALL_REGISTER(app_api_sync_call_entry) = {
    .uuid       = 0x891E7CD3,
    .func       = call_app_api,
    .task_name  = "app_core",
};

static void tws_sync_call_app_api(int api)
{
    tws_api_sync_call_by_uuid(0x891E7CD3, api, 100);
}

static void app_action_power_off(struct scene_param *param)
{
    call_app_api(API_POWER_OFF, 0);
}

static void app_action_tws_power_off(struct scene_param *param)
{
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        tws_sync_call_app_api(API_TWS_POWER_OFF);
    }
}
#else
static void app_action_power_off(struct scene_param *param)
{
    sys_enter_soft_poweroff(POWEROFF_NORMAL);
}
static void app_action_tws_power_off(struct scene_param *param)
{

}
#endif


static void app_action_close_8s_reset(struct scene_param *param)
{
    P3_PINR_CON &= ~BIT(1);
}

static void app_action_open_8s_reset(struct scene_param *param)
{
    P3_PINR_CON |= BIT(1);
}

static void app_action_use_english_tone(struct scene_param *param)
{
    tone_language_set(TONE_ENGLISH);
}

static void app_action_use_chinese_tone(struct scene_param *param)
{
    tone_language_set(TONE_CHINESE);
}

static void app_action_use_tone_toggle(struct scene_param *param)
{
    enum tone_language lang = tone_language_get();
    if (lang == TONE_ENGLISH) {
        tone_language_set(TONE_CHINESE);
    } else {
        tone_language_set(TONE_ENGLISH);
    }
}

static const struct scene_action app_action_table[] = {
    [0] = {
        .action = app_action_power_off,
    },
    [1] = {
        .action = app_action_tws_power_off,
    },
    [4] = {
        .action = app_action_close_8s_reset,
    },
    [5] = {
        .action = app_action_open_8s_reset,
    },
    [6] = {
        .action = app_action_use_english_tone,
    },
    [7] = {
        .action = app_action_use_chinese_tone,
    },
    [8] = {
        .action = app_action_use_tone_toggle,
    },

};

REGISTER_SCENE_ABILITY(app_ability) = {
    .uuid   = UUID_APP,
    .event  = app_event_table,
    .state  = app_state_table,
    .action = app_action_table,
};

static int app_scene_msg_handler(int *_msg)
{
    u8 *event = (u8 *)_msg;

    scene_mgr_event_match(UUID_APP, event[0], event);
    return 0;
}


APP_MSG_HANDLER(app_scene_msg_entry) = {
    .from = MSG_FROM_APP,
    .owner = 0,
    .handler = app_scene_msg_handler,
};

