#include "ability_uuid.h"
#include "app_main.h"
#include "bt_tws.h"
#include "app_tone.h"


struct timer_info {
    u16 uuid;
    u32 msec;
    u16 counter;
} __attribute__((packed));

struct timer_hdl {
    struct list_head entry;
    u16 uuid;
    u16 counter;
    u16 id;
};

struct rand_timer_info {
    u16 uuid;
    u16 a;
    u16 b;
    u16 c;
    u16 counter;
};


struct rand_timer_hdl {
    struct list_head entry;
    struct rand_timer_info info;
    u16 id;
};

static struct list_head timer_head = LIST_HEAD_INIT(timer_head);
static struct list_head rand_timer_head = LIST_HEAD_INIT(rand_timer_head);


static struct timer_hdl *timer_for_uuid(u16 uuid)
{
    struct timer_hdl *timer;

    list_for_each_entry(timer, &timer_head, entry) {
        if (timer->uuid == uuid) {
            return timer;
        }
    }
    return NULL;
}

static struct rand_timer_hdl *rand_timer_for_uuid(u16 uuid)
{
    struct rand_timer_hdl *timer;

    list_for_each_entry(timer, &rand_timer_head, entry) {
        if (timer->info.uuid == uuid) {
            return timer;
        }
    }
    return NULL;
}

static bool timer_event_match(struct scene_param *param)
{
    if (param->msg && param->arg) {
        u16 uuid = (param->arg[1] << 8) | param->arg[0];
        return (u16)param->msg == uuid;
    }
    return 1;
}


static const struct scene_event timer_event_table[] = {
    [0] = {
        .event = APP_MSG_SYS_TIMER,
        .match = timer_event_match,
    },

    { 0xffffffff, NULL },
};


static bool timer_counter_equal_to(struct scene_param *param)
{
    u16 uuid = *(u16 *)param->arg;
    u16 value = *(u16 *)(param->arg + 2);

    struct timer_hdl *timer = timer_for_uuid(uuid);
    if (timer) {
        return timer->counter == value;
    }

    struct rand_timer_hdl *rand_timer = rand_timer_for_uuid(uuid);
    if (rand_timer) {
        return rand_timer->info.counter == value;
    }
    return 0;
}

static bool timer_counter_greater_than(struct scene_param *param)
{
    u16 uuid = *(u16 *)param->arg;
    u16 value = *(u16 *)(param->arg + 2);

    struct timer_hdl *timer = timer_for_uuid(uuid);
    if (timer) {
        return timer->counter > value;
    }

    struct rand_timer_hdl *rand_timer = rand_timer_for_uuid(uuid);
    if (rand_timer) {
        return rand_timer->info.counter > value;
    }
    return 0;
}

static const struct scene_state timer_state_table[] = {
    [0] = {
        .match = timer_counter_equal_to,
    },
    [1] = {
        .match = timer_counter_greater_than,
    },
};

static void timer_event_handler(void *_timer)
{
    struct timer_hdl *timer = (struct timer_hdl *)_timer;
    int uuid = timer->uuid;

    scene_mgr_event_match(UUID_SYS_TIMER, APP_MSG_SYS_TIMER, (void *)uuid);

    if (timer->counter) {
        if (--timer->counter == 0) {
            sys_timer_del(timer->id);
            list_del(&timer->entry);
            free(timer);
        }
    }
}

static void timer_action_add(struct scene_param *param)
{
    struct timer_info *info = (struct timer_info *)param->arg;
    struct timer_hdl *timer;

    timer = timer_for_uuid(info->uuid);
    if (timer) {
        sys_timer_modify(timer->id, info->msec);
        return;
    }

    timer = malloc(sizeof(*timer));
    if (!timer) {
        return;
    }
    timer->uuid     = info->uuid;
    timer->counter  = info->counter;
    timer->id       = sys_timer_add(timer, timer_event_handler, info->msec);
    if (timer->id == 0) {
        free(timer);
        return;
    }
    list_add_tail(&timer->entry, &timer_head);

    /*printf("action_add_timer: %d\n", info->counter);*/
}

static void timer_action_del(struct scene_param *param)
{
    int uuid = *(u16 *)param->arg;

    struct timer_hdl *timer = timer_for_uuid(uuid);
    if (timer) {
        sys_timer_del(timer->id);
        __list_del_entry(&timer->entry);
        free(timer);
        return;
    }

    struct rand_timer_hdl *rand_timer = rand_timer_for_uuid(uuid);
    if (rand_timer) {
        sys_timer_del(rand_timer->id);
        __list_del_entry(&rand_timer->entry);
        free(rand_timer);
        return;
    }
}

static void rand_timer_event_handler(void *_timer)
{
    struct rand_timer_hdl *timer = (struct rand_timer_hdl *)_timer;

    int uuid = timer->info.uuid;
    scene_mgr_event_match(UUID_SYS_TIMER, APP_MSG_SYS_TIMER, (void *)uuid);

    if (timer->info.counter) {
        if (--timer->info.counter == 0) {
            sys_timer_del(timer->id);
            list_del(&timer->entry);
            free(timer);
            return;
        }
    }
    int msec = (rand32() % timer->info.a + 1) * timer->info.b + timer->info.c;
    sys_timer_modify(timer->id, msec);
}

static void timer_action_add_rand(struct scene_param *param)
{
    struct rand_timer_hdl *timer;

    timer = malloc(sizeof(*timer));
    if (!timer) {
        return;
    }

    memcpy(&timer->info, param->arg, sizeof(struct rand_timer_info));

    if (timer->info.a == 0) {
        timer->info.a = 10;
    }
    int msec = (rand32() % timer->info.a + 1) * timer->info.b + timer->info.c;
    timer->id = sys_timer_add(timer, rand_timer_event_handler, msec);
    if (timer->id == 0) {
        free(timer);
        return;
    }
    list_add_tail(&timer->entry, &rand_timer_head);
}

static const struct scene_action timer_action_table[] = {
    [0] = {
        .action = timer_action_add,
    },
    [1] = {
        .action = timer_action_del,
    },
    [2] = {
        .action = timer_action_add_rand,
    },
};

REGISTER_SCENE_ABILITY(sys_timer_ability) = {
    .uuid   = UUID_SYS_TIMER,
    .event  = timer_event_table,
    .state  = timer_state_table,
    .action = timer_action_table,
};


