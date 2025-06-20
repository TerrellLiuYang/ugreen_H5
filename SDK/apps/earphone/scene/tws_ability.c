#include "ability_uuid.h"
#include "app_main.h"
#include "bt_tws.h"

#include "classic/tws_api.h"
#include "app_config.h"



static bool tws_detach_by_timeout(void *arg)
{
    return true;
}
#if TCFG_USER_TWS_ENABLE

static bool is_tws_event_enter_sniff(struct scene_param *param)
{
    struct tws_event *evt = (struct tws_event *)param->msg;

    return evt->args[0] == 1;
}

static bool is_tws_event_exit_sniff(struct scene_param *param)
{
    struct tws_event *evt = (struct tws_event *)param->msg;

    return evt->args[0] == 0;
}

static const struct scene_event tws_event_table[] = {
    [0] = {
        .event = TWS_EVENT_CONNECTED,
    },
    [1] = {
        .event = TWS_EVENT_CONNECTION_DETACH,
    },
    [2] = {
        .event = TWS_EVENT_ROLE_SWITCH,
    },
    [3] = {
        .event = 0x0100 | APP_MSG_TWS_POWERON_PAIR_TIMEOUT
    },
    [4] = {
        .event = 0x0100 | APP_MSG_TWS_POWERON_CONN_TIMEOUT
    },
    [5] = {
        .event = 0x0100 | APP_MSG_TWS_START_PAIR_TIMEOUT,
    },
    [6] = {
        .event = 0x0100 | APP_MSG_TWS_START_CONN_TIMEOUT,
    },
    [7] = {
        .event = TWS_EVENT_MODE_CHANGE,
        .match = is_tws_event_enter_sniff,
    },
    [8] = {
        .event = TWS_EVENT_MODE_CHANGE,
        .match = is_tws_event_exit_sniff,
    },

    { 0xffffffff, NULL },
};


static bool tws_is_paired(struct scene_param *param)
{
    return !!(tws_api_get_tws_state() & TWS_STA_TWS_PAIRED);
}

static bool tws_is_unpaired(struct scene_param *param)
{
    return !!(tws_api_get_tws_state() & TWS_STA_TWS_UNPAIRED);
}

static bool tws_is_connected(struct scene_param *param)
{
    return !!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED);
}

static bool tws_is_disconnected(struct scene_param *param)
{
    return !(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED);
}

static bool tws_is_master(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    priv->local_action = 1;
    return tws_api_get_role() == TWS_ROLE_MASTER;
}

static bool tws_is_slave(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    priv->local_action = 1;
    return tws_api_get_role() == TWS_ROLE_SLAVE;
}

static bool tws_is_left_channel(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    priv->local_action = 1;
    return tws_api_get_local_channel() == 'L' ? true : false;
}

static bool tws_is_right_channel(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    priv->local_action = 1;
    return tws_api_get_local_channel() == 'R' ? true : false;
}

static const struct scene_state tws_state_table[] = {
    [0] = {
        .match = tws_is_paired
    },
    [1] = {
        .match = tws_is_unpaired
    },
    [2] = {
        .match = tws_is_connected
    },
    [3] = {
        .match = tws_is_disconnected
    },
    [4] = {
        .match = tws_is_left_channel
    },
    [5] = {
        .match = tws_is_right_channel
    },
    [6] = {
        .match = tws_is_master
    },
    [7] = {
        .match = tws_is_slave
    },
};


static void tws_action_start_pair(struct scene_param *param)
{
    app_send_message(APP_MSG_TWS_START_PAIR, 0);
}

static void tws_action_wait_pair(struct scene_param *param)
{
    app_send_message(APP_MSG_TWS_WAIT_PAIR, 0);
}

static void tws_action_start_conn(struct scene_param *param)
{
    app_send_message(APP_MSG_TWS_START_CONN, 0);
}

static void tws_action_wait_conn(struct scene_param *param)
{
    app_send_message(APP_MSG_TWS_WAIT_CONN, 0);
}

static void tws_action_start_pair_and_conn_process(struct scene_param *param)
{
    app_send_message(APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS, 0);
}

static void tws_action_role_switch(struct scene_param *param)
{
    tws_api_role_switch();
}

static void tws_action_open_role_switch(struct scene_param *param)
{
    tws_api_auto_role_switch_enable();
}

static void tws_action_close_role_switch(struct scene_param *param)
{
    tws_api_auto_role_switch_disable();
}

static void tws_action_detach(struct scene_param *param)
{
    tws_api_detach(TWS_DETACH_BY_LOCAL, 5000);
}

static void tws_action_remove_pair(struct scene_param *param)
{
    int err = tws_api_remove_pairs();
    if (err) {
        bt_tws_remove_pairs();
    }
}




static const struct scene_action tws_action_table[] = {
    [0] = {
        .action = tws_action_start_pair
    },
    [1] = {
        .action = tws_action_wait_pair,
    },
    [2] = {
        .action = tws_action_start_conn
    },
    [3] = {
        .action = tws_action_wait_conn
    },
    [4] = {
        .action = tws_action_start_pair_and_conn_process
    },
    [5] = {
        .action = tws_action_role_switch
    },
    [6] = {
        .action = tws_action_open_role_switch
    },
    [7] = {
        .action = tws_action_close_role_switch
    },
    [8] = {
        .action = tws_action_detach
    },
    [9] = {
        .action = tws_action_remove_pair
    },

};

REGISTER_SCENE_ABILITY(tws_ability) = {
    .uuid   = UUID_TWS,
    .event  = tws_event_table,
    .state  = tws_state_table,
    .action = tws_action_table,
};

static int tws_scene_msg_handler(int *_msg)
{
    u8 *event = (u8 *)_msg;

    scene_mgr_event_match(UUID_TWS, event[0], event);
    return 0;
}


APP_MSG_HANDLER(tws_scene_msg_entry) = {
    .owner = 0xff,
    .from = MSG_FROM_TWS,
    .handler = tws_scene_msg_handler,
};

static int tws_scene_app_msg_handler(int *_msg)
{
    scene_mgr_event_match(UUID_TWS, 0x0100 | _msg[0], _msg);
    return 0;
}
APP_MSG_HANDLER(tws_scene_app_msg_entry) = {
    .owner = 0xff,
    .from = MSG_FROM_APP,
    .handler = tws_scene_app_msg_handler,
};
#endif






