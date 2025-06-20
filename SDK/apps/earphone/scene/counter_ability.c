#include "ability_uuid.h"
#include "app_main.h"


struct counter_info {
    u16 uuid;
    int value;
} __attribute__((packed));

struct counter_hdl {
    struct list_head entry;
    u16 uuid;
    int value;
};


static struct list_head counter_head = LIST_HEAD_INIT(counter_head);


static struct counter_hdl *counter_for_uuid(u16 uuid)
{
    struct counter_hdl *counter;

    list_for_each_entry(counter, &counter_head, entry) {
        if (counter->uuid == uuid) {
            return counter;
        }
    }
    return NULL;
}

static bool is_counter_less_than(struct scene_param *param)
{
    struct counter_info *info = (struct counter_info *)param->arg;

    struct counter_hdl *counter = counter_for_uuid(info->uuid);
    if (counter) {
        return counter->value < info->value;
    }
    return 0;
}

static bool is_counter_greater_than_or_equal(struct scene_param *param)
{
    return !is_counter_less_than(param);
}

static const struct scene_state counter_state_table[] = {
    [0] = {
        .match = is_counter_less_than,
    },
    [1] = {
        .match = is_counter_greater_than_or_equal,
    },
};


static void counter_action_add(struct scene_param *param)
{
    struct counter_info *info = (struct counter_info *)param->arg;
    struct counter_hdl *counter = malloc(sizeof(*counter));
    if (!counter) {
        return;
    }
    counter->uuid   = info->uuid;
    counter->value  = info->value;
    list_add_tail(&counter->entry, &counter_head);

    /*printf("action_add_counter: %d\n", info->counter);*/
}

static void counter_action_del(struct scene_param *param)
{
    int uuid = *(u16 *)param->arg;

    struct counter_hdl *counter = counter_for_uuid(uuid);
    if (counter) {
        __list_del_entry(&counter->entry);
        free(counter);
    }
}

static void counter_action_inc(struct scene_param *param)
{
    struct counter_info *info = (struct counter_info *)param->arg;
    struct counter_hdl *counter = counter_for_uuid(info->uuid);
    if (counter) {
        counter->value += info->value;
    }
}

static void counter_action_dec(struct scene_param *param)
{
    struct counter_info *info = (struct counter_info *)param->arg;
    struct counter_hdl *counter = counter_for_uuid(info->uuid);
    if (counter) {
        counter->value -= info->value;
    }
}

static void counter_action_set(struct scene_param *param)
{
    struct counter_info *info = (struct counter_info *)param->arg;
    struct counter_hdl *counter = counter_for_uuid(info->uuid);
    if (counter) {
        counter->value = info->value;
    }
}

static const struct scene_action counter_action_table[] = {
    [0] = {
        .action = counter_action_add,
    },
    [1] = {
        .action = counter_action_del,
    },
    [2] = {
        .action = counter_action_inc,
    },
    [4] = {
        .action = counter_action_dec,
    },
    [5] = {
        .action = counter_action_set,
    },
};

REGISTER_SCENE_ABILITY(counter_ability) = {
    .uuid   = UUID_COUNTER,
    .state  = counter_state_table,
    .action = counter_action_table,
};


