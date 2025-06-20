#include "scene_manager.h"
#include "generic/list.h"
#include "fs/resfile.h"
#include "app_config.h"


struct event_handle {
    struct list_head entry;
    scene_event_func_t match;
    struct list_head state;
};

struct state_handle {
    u8 arg_len;
    u8 uuid1;
    u8 uuid2;
    struct list_head entry;
    scene_state_func_t match;
    u8 arg[0];
};

struct action_handle {
    u8 arg_len;
    u8 uuid1;
    u8 uuid2;
    struct list_head entry;
    scene_action_func_t action;
    u8 arg[0];
};

struct scene_handle {
    struct list_head entry;
    struct list_head event;
    struct list_head action;
    struct scene_param param;
};

struct scene_file_iter {
    u16 pos;
    u16 len;
    u16 ptr;
    u8 buf[256];
};

struct event_info {
    u8 uuid;
    u8 index;
    int *msg;
    scene_event_func_t match;
};

struct scene_item {
    u8 type;
    u8 uuid;
    u8 index;
    u8 data_len;
    u8 data[0];
};

static RESFILE *scene_file = NULL;

static int read_scene(int ptr, u8 *buf, u16 max_len)
{
    if (scene_file == NULL) {
        scene_file = resfile_open(FLASH_RES_PATH"scene.bin");
        if (!scene_file) {
            printf("open file scene.bin faild\n");
            return 0;
        }
    }
    resfile_seek(scene_file, ptr, RESFILE_SEEK_SET);
    int rlen = resfile_read(scene_file, buf, max_len);
    for (int i = 0; i < rlen;) {
        int s_len = buf[i + 1] + 2;
        if (i + s_len > rlen) {
            return i;
        }
        i += s_len;
    }
    return rlen;
}

static const struct scene_ability *get_scene_ability(u8 uuid)
{
    const struct scene_ability *ability;

    list_for_each_scene_ability(ability) {
        if (ability->uuid == uuid) {
            return ability;
        }
    }
    return NULL;
}

static int get_event_info(struct event_info *info, int event)
{
    const struct scene_ability *ability = get_scene_ability(info->uuid);
    ASSERT(ability != NULL);

    struct scene_param param = {
        .msg = info->msg,
        .arg = NULL,
    };

    for (int index = 0; ability->event[index].event != 0xffffffff; index++) {
        if (ability->event[index].event == event) {
            info->match = ability->event[index].match;
            if (!info->match || info->match(&param)) {
                info->index = index;
                return 1;
            }
            break;
        }
    }
    return 0;
}

static scene_state_func_t get_state_match_func(struct scene_item *item)
{
    const struct scene_ability *ability;
    ability = get_scene_ability(item->uuid);
    if (!ability) {
        return NULL;
    }
    return ability->state[item->index].match;
}

static scene_action_func_t get_action_func(struct scene_item *item)
{
    const struct scene_ability *ability;
    ability = get_scene_ability(item->uuid);
    if (!ability) {
        return NULL;
    }
    return ability->action[item->index].action;
}

static bool unknow_state_func(struct scene_param *param)
{
    return 0;
}


static int scene_pick(struct scene_handle **_scene, struct event_info *info,
                      struct scene_file_iter *iter)
{
    struct scene_handle *scene;
    struct event_handle *event;
    struct state_handle *state;
    struct action_handle *action;

    if (*_scene == NULL) {
        scene = malloc(sizeof(struct scene_handle));
        *_scene = scene;
    } else {
        scene = *_scene;
    }
    INIT_LIST_HEAD(&scene->event);
    INIT_LIST_HEAD(&scene->action);

    while (1) {
        if (iter->len == 0) {
            iter->len = read_scene(iter->ptr, iter->buf, 256);
            if (iter->len <= 0) {
                break;
            }
            iter->pos = 0;
            iter->ptr += iter->len;
        }

        while (iter->len) {
            u8 *buf = iter->buf + iter->pos;
            u8 crc = *buf++;
            u8 len = *buf++;
            iter->len -= len + 2;
            iter->pos += len + 2;

            int scene_match = 0;
            int event_match = 0;

            for (int i = 0; i < len;) {
                struct scene_item *item = (struct scene_item *)(buf + i);

                switch (item->type) {
                case 0x01:
                    if (item->uuid != info->uuid || item->index != info->index) {
                        event_match = 0;
                        break;
                    }
                    if (item->data_len) {
                        ASSERT(info->match, "%d, %d\n", info->uuid, info->index);
                        struct scene_param param = {
                            .msg = info->msg,
                            .arg = item->data,
                            .arg_len = item->data_len,
                        };
                        if (!info->match(&param)) {
                            event_match = 0;
                            break;
                        }
                    }
                    event_match = 1;
                    scene_match = 1;
                    event = malloc(sizeof(struct event_handle));
                    list_add_tail(&event->entry, &scene->event);
                    INIT_LIST_HEAD(&event->state);
                    break;
                case 0x02:
                    if (event_match == 1) {
                        scene_state_func_t func;
                        func = get_state_match_func(item);
                        if (!func) {
                            func = unknow_state_func;
                        }
                        state = malloc(sizeof(*state) + item->data_len);
                        state->match = func;
                        state->uuid1 = item->uuid;
                        state->uuid2 = item->index;
                        state->arg_len = item->data_len;
                        if (item->data_len) {
                            memcpy(state->arg, item->data, item->data_len);
                        }
                        list_add_tail(&state->entry, &event->state);
                    }
                    break;
                case 0x03:
                    if (scene_match) {
                        scene_action_func_t func;
                        func = get_action_func(item);
                        if (func) {
                            action = malloc(sizeof(*action) + item->data_len);
                            action->action = func;
                            action->uuid1 = item->uuid;
                            action->uuid2 = item->index;
                            action->arg_len = item->data_len;
                            if (item->data_len) {
                                memcpy(action->arg, item->data, item->data_len);
                            }
                            list_add_tail(&action->entry, &scene->action);
                        }
                    }
                    break;
                }
                i += sizeof(*item) + item->data_len;
            }
            if (scene_match) {
                return 1;
            }
        }
    }

    return 0;
}

static void scene_mgr_log_print(struct scene_handle *scene,
                                u8 event_uuid, u8 event_index,
                                struct list_head *state_head,
                                struct state_handle *no_match)
{
#if CONFIG_DEBUG_ENABLE
    char scene_log[256];
    int log_size = 256 - 4;

    int offset = snprintf(scene_log, log_size,
                          "[scene]: event: %d/%d state:", event_uuid, event_index);

    struct state_handle *state;
    list_for_each_entry(state, state_head, entry) {
        offset += snprintf(scene_log + offset, log_size - offset, " %d/%d",
                           state->uuid1, state->uuid2);
        if (state->arg_len == 0) {
            scene_log[offset++] = ' ';
        } else {
            scene_log[offset++] = '/';
            scene_log[offset++] = '[';
            for (int i = 0; i < state->arg_len;) {
                offset += snprintf(scene_log + offset, log_size - offset, "%02x",
                                   state->arg[i]);
                if (++i == state->arg_len) {
                    break;
                }
                scene_log[offset++] = ' ';
            }
            scene_log[offset++] = ']';
        }
        if (state == no_match) {
            break;
        }
    }

    if (!no_match) {
        offset += snprintf(scene_log + offset, log_size - offset,  " action:");
        struct action_handle *action;
        list_for_each_entry(action, &scene->action, entry) {
            offset += snprintf(scene_log + offset, log_size - offset, " %d/%d",
                               action->uuid1, action->uuid2);
            if (action->arg_len != 0) {
                scene_log[offset++] = '/';
                scene_log[offset++] = '[';
                for (int i = 0; i < action->arg_len;) {
                    offset += snprintf(scene_log + offset, log_size - offset, "%02x",
                                       action->arg[i]);
                    if (++i == action->arg_len) {
                        break;
                    }
                    scene_log[offset++] = ' ';
                }
                scene_log[offset++] = ']';
            }
        }
    }

    scene_log[offset] = '\n';
    scene_log[offset + 1] = '\0';
    puts(scene_log);
#endif
}


void scene_mgr_event_match(u8 uuid, int _event, void *msg)
{
    struct scene_handle *scene = NULL, *n;
    struct event_handle *event, *ne, *match_event;
    struct state_handle *state, *ns;
    struct action_handle *action, *na;

    struct event_info info = {
        .uuid = uuid,
        .msg  = msg,
    };
    if (!get_event_info(&info, _event)) {
        return;
    }
    /*printf("[scene]: event: %d, %d\n", info.uuid, info.index);*/

    struct scene_file_iter iter = {
        .len = 0,
        .pos = 0,
        .ptr = 0,
    };

    struct list_head head = { &head, &head };

    while (scene_pick(&scene, &info, &iter)) {
        match_event = NULL;

        scene->param.uuid   = uuid;
        scene->param.msg    = msg;

        list_for_each_entry(event, &scene->event, entry) {
            memset(scene->param.user_type, 0, sizeof(scene->param.user_type));
            int match = 1;
            list_for_each_entry_safe(state, ns, &event->state, entry) {
                scene->param.arg = state->arg;
                scene->param.arg_len = state->arg_len;
                if (!state->match(&scene->param)) {
                    match = 0;
                    scene_mgr_log_print(scene, info.uuid, info.index, &event->state, state);
                    break;
                }
            }
            if (match) {
                match_event = event;
                break;
            }
        }

        /* 只保留match的event和state */
        list_for_each_entry_safe(event, ne, &scene->event, entry) {
            if (event != match_event) {
                list_for_each_entry_safe(state, ns, &event->state, entry) {
                    __list_del_entry(&state->entry);
                    free(state);
                }
                __list_del_entry(&event->entry);
                free(event);
            }
        }

        if (match_event) {
            list_add_tail(&scene->entry, &head);
            scene = NULL;
        } else {
            list_for_each_entry_safe(action, na, &scene->action, entry) {
                __list_del_entry(&action->entry);
                free(action);
            }
        }
    }
    if (scene) {
        free(scene);
    }

    list_for_each_entry_safe(scene, n, &head, entry) {

        list_for_each_entry_safe(event, ne, &scene->event, entry) {
            scene_mgr_log_print(scene, info.uuid, info.index, &event->state, NULL);
            list_for_each_entry_safe(state, ns, &event->state, entry) {
                __list_del_entry(&state->entry);
                free(state);
            }
            __list_del_entry(&event->entry);
            free(event);
        }

        list_for_each_entry_safe(action, na, &scene->action, entry) {
            scene->param.arg = action->arg;
            scene->param.arg_len = action->arg_len;
            action->action(&scene->param);
            __list_del_entry(&action->entry);
            free(action);
        }
        __list_del_entry(&scene->entry);
        free(scene);
    }
}

