#include "ability_uuid.h"
#include "app_main.h"
#include "resfile.h"
#include "app_tone.h"
#include "bt_tws.h"


static const char *tone_file_patch_en = "tone_en/";
static const char *tone_file_patch_zh = "tone_zh/";

static void tone_aciton_get_file_path(struct scene_param *param, char *file_name)
{
    int len1;
    if (tone_language_get() == TONE_ENGLISH) {
        len1 = strlen(tone_file_patch_en);
        memcpy(file_name, tone_file_patch_en, len1);
    } else {
        len1 = strlen(tone_file_patch_zh);
        memcpy(file_name, tone_file_patch_zh, len1);
    }
    int len2 = param->arg_len;
    memcpy(file_name + len1, (char *)param->arg, param->arg_len);
    file_name[len1 + len2] = '.';
    file_name[len1 + len2 + 1] = '*';
    file_name[len1 + len2 + 2] = '\0';

    /*r_printf("tone_path: %s\n", file_name);*/
}

static bool is_play_file_end(struct scene_param  *param)
{
    char file_name[48];
    int *msg = (int *)param->msg;

    if (!param->arg) {
        return true;
    }

    tone_aciton_get_file_path(param, file_name);
    if (msg[1] == tone_player_get_fname_uuid(file_name)) {
        return true;
    }
    return false;
}

static const struct scene_event tone_event_table[] = {
    [1] = {
        .event = STREAM_EVENT_STOP,
        .match = is_play_file_end,
    },

    { 0xffffffff, NULL },
};

static bool tone_is_play_end()
{
    return true;
}

static const struct scene_state tone_state_table[] = {
    [0] = {
        .match = tone_is_play_end
    },
};


static void tone_action_play_file_in_mix_mode(struct scene_param *param)
{
    char file_name[48];
    tone_aciton_get_file_path(param, file_name);
    play_tone_file(file_name);
}

static void tone_action_play_file_alone(struct scene_param *param)
{
    char file_name[48];
    tone_aciton_get_file_path(param, file_name);
    play_tone_file_alone(file_name);
}

#if TCFG_USER_TWS_ENABLE
static void tone_action_tws_play_file_in_mix_mode(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    if (!priv->local_action) {
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            return;
        }
    }
    char file_name[48];
    tone_aciton_get_file_path(param, file_name);
    tws_play_tone_file(file_name, 400);
}

static void tone_action_tws_play_file_alone(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    if (!priv->local_action) {
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            return;
        }
    }
    char file_name[48];
    tone_aciton_get_file_path(param, file_name);
    tws_play_tone_file_alone(file_name, 400);
}
#endif

static const struct scene_action tone_action_table[] = {
    [0] = {
        .action = tone_action_play_file_in_mix_mode
    },
    [1] = {
        .action = tone_action_play_file_alone
    },
#if TCFG_USER_TWS_ENABLE
    [2] = {
        .action = tone_action_tws_play_file_in_mix_mode
    },
    [3] = {
        .action = tone_action_tws_play_file_alone
    },
#endif

};

REGISTER_SCENE_ABILITY(tone_ability) = {
    .uuid   = UUID_TONE,
    .event  = tone_event_table,
    .state  = tone_state_table,
    .action = tone_action_table,
};

static int tone_scene_msg_handler(int *msg)
{
    scene_mgr_event_match(UUID_TONE, msg[0], msg);
    return 0;
}


APP_MSG_HANDLER(tone_scene_msg_entry) = {
    .from = MSG_FROM_TONE,
    .owner = 0xff,
    .handler = tone_scene_msg_handler,
};

