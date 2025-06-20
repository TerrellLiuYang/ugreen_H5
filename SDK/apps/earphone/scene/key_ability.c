#include "ability_uuid.h"
#include "app_main.h"
#include "key_driver.h"
#include "bt_tws.h"

#if CONFIG_KEY_SCENE_ENABLE


static const struct scene_event iokey_event_table[] = {
    [0] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_CLICK)    //click
    },
    [1] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_LONG)    //long
    },
    [2] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_HOLD)    //hold
    },
    [3] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_UP)    //up
    },
    [4] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_DOUBLE_CLICK)    //double click
    },
    [5] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TRIPLE_CLICK)    //triple click
    },
    [6] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_FOURTH_CLICK)    //fourth click
    },
    [7] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_FIRTH_CLICK)    //firth click
    },
    [8] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_SEXTUPLE_CLICK)
    },
    [9] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_SEPTUPLE_CLICK)
    },
    [10] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_HOLD_1SEC)
    },
    [11] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_HOLD_3SEC)
    },
    [12] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_HOLD_5SEC)
    },
    [13] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_HOLD_8SEC)
    },
    [14] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_HOLD_10SEC)
    },
    [15] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_CLICK)
    },
    [16] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_DOUBLE_CLICK)
    },
    [17] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_TRIPLE_CLICK)
    },
    [18] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_FOURTH_CLICK)
    },
    [19] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_FIRTH_CLICK)
    },
    [20] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_SEXTUPLE_CLICK)
    },
    [21] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_SEPTUPLE_CLICK)
    },
    [22] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_HOLD_1SEC)
    },
    [23] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_HOLD_3SEC)
    },
    [24] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_HOLD_5SEC)
    },
    [25] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_HOLD_8SEC)
    },
    [26] = {
        .event = APP_KEY_MSG_REMAP(UUID_KEY, KEY_ACTION_TWS_HOLD_10SEC)
    },
};

REGISTER_SCENE_ABILITY(key_pwr_ability) = {
    .uuid   = UUID_KEY_PWR,
    .event  = iokey_event_table,
};

REGISTER_SCENE_ABILITY(key_next_ability) = {
    .uuid   = UUID_KEY_NEXT,
    .event  = iokey_event_table,
};

REGISTER_SCENE_ABILITY(key_prev_ability) = {
    .uuid   = UUID_KEY_PREV,
    .event  = iokey_event_table,
};

REGISTER_SCENE_ABILITY(key_mode_ability) = {
    .uuid   = UUID_KEY_MODE,
    .event  = iokey_event_table,
};

REGISTER_SCENE_ABILITY(key_play_ability) = {
    .uuid   = UUID_KEY_PLAY,
    .event  = iokey_event_table,
};

static const struct scene_event key_slider_event_table[] = {
    [0] = {
        .event = APP_KEY_MSG_REMAP(KEY_SLIDER, KEY_SLIDER_UP)    //up
    },
    [1] = {
        .event = APP_KEY_MSG_REMAP(KEY_SLIDER, KEY_SLIDER_DOWN)    //down
    },
};
REGISTER_SCENE_ABILITY(key_slider_ability) = {
    .uuid   = UUID_KEY_SLIDER,
    .event  = key_slider_event_table,
};





#if TCFG_ADKEY_ENABLE

extern u8 uuid2adkeyValue(u16 uuid);

static bool adkey_event_match(struct scene_param *param)
{
    if (!param->arg) {
        return true;
    }
    int *msg = (int *)param->msg;
    u16 uuid = (param->arg[1] << 8) | param->arg[0];

    /*r_printf("--adkey_event_match: %x, %x\n", uuid, msg[0]);*/

    u8 key_value = uuid2adkeyValue(uuid);
    if (key_value == APP_MSG_KEY_VALUE(msg[0])) {
        return true;
    }

    return false;
}

static const struct scene_event adkey_event_table[] = {
    [0] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_CLICK),    //click
        .match = adkey_event_match,
    },
    [1] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_LONG),    //long
        .match = adkey_event_match,
    },
    [2] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_HOLD),    //hold
        .match = adkey_event_match,
    },
    [3] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_UP),    //up
        .match = adkey_event_match,
    },
    [4] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_DOUBLE_CLICK),    //double click
        .match = adkey_event_match,
    },
    [5] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_TRIPLE_CLICK),    //triple click
        .match = adkey_event_match,
    },
    [6] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_FOURTH_CLICK),    //fourth click
        .match = adkey_event_match,
    },
    [7] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_FIRTH_CLICK),    //firth click
        .match = adkey_event_match,
    },
    [8] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_SEXTUPLE_CLICK),
        .match = adkey_event_match,
    },
    [9] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_SEPTUPLE_CLICK),
        .match = adkey_event_match,
    },
    [10] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_HOLD_1SEC),
        .match = adkey_event_match,
    },
    [11] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_HOLD_3SEC),    //hold 3s
        .match = adkey_event_match,
    },
    [12] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_HOLD_5SEC),    //hold 5s
        .match = adkey_event_match,
    },
    [13] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_HOLD_8SEC),
        .match = adkey_event_match,
    },
    [14] = {
        .event = APP_KEY_MSG_REMAP(UUID_ADKEY, KEY_ACTION_HOLD_10SEC),
        .match = adkey_event_match,
    },
};

REGISTER_SCENE_ABILITY(adkey_ability) = {
    .uuid   = UUID_ADKEY,
    .event  = adkey_event_table,
};
#endif



static bool is_key_msg_from_local(struct scene_param *param)
{
    int *msg = (int *)param->msg;

    if (!APP_MSG_FROM_KEY(msg[0])) {
        return 0;
    }
    return msg[1] != APP_KEY_MSG_FROM_TWS;
}

static bool is_key_msg_from_tws(struct scene_param *param)
{
    return !is_key_msg_from_local(param);
}

static const struct scene_state key_state_table[] = {
    [0] = {
        .match = is_key_msg_from_local,
    },
    [1] = {
        .match = is_key_msg_from_tws,
    },
};


REGISTER_SCENE_ABILITY(key_driver_ability) = {
    .uuid  = UUID_KEY,
    .state  = key_state_table,
};

static int scene_key_msg_handler(int *msg)
{
    u8 uuid;
    u8 key_value;
    int event = msg[0];

    if (!APP_MSG_FROM_KEY(msg[0])) {
        return 0;
    }

    key_value = APP_MSG_KEY_VALUE(msg[0]);

    switch (key_value) {
    case KEY_POWER:
        uuid = UUID_KEY_PWR;
        event = APP_KEY_MSG_REMAP(UUID_KEY, APP_MSG_KEY_ACTION(msg[0]));
        break;
    case KEY_NEXT:
        uuid = UUID_KEY_NEXT;
        event = APP_KEY_MSG_REMAP(UUID_KEY, APP_MSG_KEY_ACTION(msg[0]));
        break;
    case KEY_PREV:
        uuid = UUID_KEY_PREV;
        event = APP_KEY_MSG_REMAP(UUID_KEY, APP_MSG_KEY_ACTION(msg[0]));
        break;
    case KEY_MODE:
        uuid = UUID_KEY_MODE;
        event = APP_KEY_MSG_REMAP(UUID_KEY, APP_MSG_KEY_ACTION(msg[0]));
        break;
    case KEY_PLAY:
        uuid = UUID_KEY_PLAY;
        event = APP_KEY_MSG_REMAP(UUID_KEY, APP_MSG_KEY_ACTION(msg[0]));
        break;
    case KEY_SLIDER:
        uuid = UUID_KEY_SLIDER;
        break;
    default:
#if TCFG_ADKEY_ENABLE
        if (key_value >= KEY_AD_NUM0 && key_value <= KEY_AD_NUM19) {
            uuid = UUID_ADKEY;
            event = APP_KEY_MSG_REMAP(UUID_ADKEY, APP_MSG_KEY_ACTION(msg[0]));
            break;
        }
#endif
        return 0;
    }

    scene_mgr_event_match(uuid, event, msg);

    return 0;
}

APP_MSG_HANDLER(scene_key_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = scene_key_msg_handler,
};


#endif
