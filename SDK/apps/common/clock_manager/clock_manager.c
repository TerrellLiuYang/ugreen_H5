
#include "clock_manager.h"
#include "system/init.h"
#include "system/timer.h"
#include "system/task.h"
#include "os/os_api.h"
#include "clock.h"
#include "list.h"


typedef struct clock_manager_st {
    struct list_head entry;
    char        name[32];
    u32         freq;
} clock_manager_item;

static struct list_head clk_mgr_head = LIST_HEAD_INIT(clk_mgr_head);

static clock_manager_item clk_locker;
static u16 clk_ref_timer;
static u8 ref_cnt;

#if 0
#define CLOCK_MANAGER_INIT_CRITICAL()
#define CLOCK_MANAGER_ENTER_CRITICAL()      local_irq_disable()
#define CLOCK_MANAGER_EXIT_CRITICAL()       local_irq_enable()
#else
static OS_MUTEX clk_mutex;
#define CLOCK_MANAGER_INIT_CRITICAL()       os_mutex_create(&clk_mutex)
#define CLOCK_MANAGER_ENTER_CRITICAL()      os_mutex_pend(&clk_mutex, 0)
#define CLOCK_MANAGER_EXIT_CRITICAL()       os_mutex_post(&clk_mutex)
#endif

extern void core_voltage_dump();        //debug info api

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_alloc，此函数会触发时钟频率设置
 *
 * @param name
 * @param clk
 *
 * @return 0:succ
 */
/* ----------------------------------------------------------------------------*/
int clock_alloc(const char *name, u32 clk)
{
    clock_manager_item *p;
    u32 total;

    CLOCK_MANAGER_ENTER_CRITICAL();

    total = 0;
    list_for_each_entry(p, &clk_mgr_head, entry) {
        total += p->freq;
        if (0 == strcmp(p->name, name)) {
            CLOCK_MANAGER_EXIT_CRITICAL();
            return -1;
        }
    }

    clock_manager_item *it = malloc(sizeof(clock_manager_item));
    if (it == NULL) {
        CLOCK_MANAGER_EXIT_CRITICAL();
        return -1;
    }

    /* it->name = name; */
    strcpy(it->name, name);
    it->freq = clk;

    list_add_tail(&it->entry, &clk_mgr_head);

    clock_refurbish();

    CLOCK_MANAGER_EXIT_CRITICAL();

    return 0;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_free，此函数会触发时钟频率设置
 *
 * @param name
 *
 * @return 0:succ
 */
/* ----------------------------------------------------------------------------*/
int clock_free(char *name)
{
    clock_manager_item *p, *n;
    u32 total;

    CLOCK_MANAGER_ENTER_CRITICAL();

    total = 0;
    list_for_each_entry_safe(p, n, &clk_mgr_head, entry) {
        if (0 == strcmp(p->name, name)) {
            __list_del_entry(&p->entry);
            free(p);
            continue;
        }
        total += p->freq;
    }

    clock_refurbish();

    /* clk_set("sys", total); */
    CLOCK_MANAGER_EXIT_CRITICAL();

    return 0;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_manager_dump
 */
/* ----------------------------------------------------------------------------*/
static u32 clock_list_sum(void)
{
    clock_manager_item *p;
    u32 total;

    total = 0;
    list_for_each_entry(p, &clk_mgr_head, entry) {
        /*printf("%s : %dHz", p->name, p->freq);*/
        total += p->freq;
    }
    /*printf("%s : %dHz", "total", total);*/

    return total;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_manager_test
 */
/* ----------------------------------------------------------------------------*/
void clock_manager_test(void)
{
    clock_alloc("clk_mgr_1", 2 * 1000000U);
    clock_manager_dump();

    clock_alloc("clk_mgr_2", 2 * 1000000U);
    clock_manager_dump();

    clock_free("clk_mgr_1");
    clock_manager_dump();

    clock_free("clk_mgr_2");
    clock_manager_dump();

    printf("test over");
    while (1);
}


void clock_lock_dump(void)
{
    printf("clock_lock owner : %s", clk_locker.name);
    printf("clock_lock freq  : %d", clk_locker.freq);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_lock
 *
 * @param name: owner name
 * @param clk: lock_freq
 *
 * @return 0:succ, other:err code
 */
/* ----------------------------------------------------------------------------*/
int clock_lock(const char *name, u32 clk)
{
    CLOCK_MANAGER_ENTER_CRITICAL();

    if (clk_locker.freq) {
        //has been locked, lock fail
        CLOCK_MANAGER_EXIT_CRITICAL();
        return -1;
    }

    clk_locker.freq = clk;
    strcpy(clk_locker.name, name);

    clk_set_api("sys", clk);

    CLOCK_MANAGER_EXIT_CRITICAL();

    return 0;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_unlock
 *
 * @param name: owner name
 *
 * @return 0:succ, other:err code
 */
/* ----------------------------------------------------------------------------*/
int clock_unlock(char *name)
{
    printf("%s", __func__);

    CLOCK_MANAGER_ENTER_CRITICAL();

    clock_lock_dump();

    if (clk_locker.freq == 0) {
        //has been locked
        /* ASSERT(0, "please lock it befor unlock"); */
        CLOCK_MANAGER_EXIT_CRITICAL();
        return -1;
    }

    if (strcmp(clk_locker.name, name)) {
        /* ASSERT(0, "locker owner is %s", clk_locker.name); */
        CLOCK_MANAGER_EXIT_CRITICAL();
        return -2;
    }

    clk_locker.freq = 0;

    //refurbish clock after unlock succ
    clock_refurbish();

    CLOCK_MANAGER_EXIT_CRITICAL();

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_manager_reflash
 */
/* ----------------------------------------------------------------------------*/

#define CLOCK_DETECT_PERIOD      (4 * 1000)         //4s
#define CLOCK_DETECT_COUNTER     (7)                //7 times

#define CLOCK_MINIMUM_FREQ       (24  * 1000000)    //24MHz
#define CLOCK_MAXIMUM_FREQ       clk_get_max_frequency()


static const int clock_table[] = {
    24 * MHz, 48 * MHz, 60 * MHz, 64 * MHz, 76 * MHz,
    80 * MHz, 96 * MHz, 120 * MHz, 128 * MHz, 140 * MHz, 160 * MHz,
    200 * MHz, 240 * MHz, 280 * MHz, 320 * MHz,
};

static u32 __get_clock(u32 dest_clk)
{
    for (int i = 0; i < ARRAY_SIZE(clock_table); i++) {
        if (dest_clk <= clock_table[i]) {
            return clock_table[i];
        }
    }
    return clock_table[ARRAY_SIZE(clock_table) - 1];
}
static u8 clk_adjust_step = 0;

#ifdef CONFIG_EARPHONE_CASE_ENABLE
static u8 cpu_usage_limit[] = {75, 80, 85};
#else
static u8 cpu_usage_limit[] = {40, 50, 60};
#endif
/* --------------------------------------------------------------------------*/
/**
 * @brief clk_ref_cal
 */
/* ----------------------------------------------------------------------------*/
static void clk_ref_cal(void)
{
    u32 clk_freq;
    u32 clk_decr;
    u32 pct;

#if 0   //效率统计

    extern void CacheReport(void);
    CacheReport();

    extern void task_info_output(void);
    task_info_output();

#endif


    int usage[2] = { 0, 0 };
    int a = os_cpu_usage(NULL, usage);
    task_info_reset();

    if (a < 0) {
        return;
    }
    int usage_max = MAX(usage[0], usage[1]);

    int curr_clk = clk_get("sys");
    int dest_clk = curr_clk;
__again:
    switch (clk_adjust_step) {
    case 0:
        dest_clk = __get_clock(curr_clk / 50 * usage_max);
        clk_adjust_step = 1;
        break;
    case 1:
        if (usage_max < cpu_usage_limit[0]) {
            dest_clk = __get_clock(curr_clk / cpu_usage_limit[1] * usage_max);
        } else if (usage_max > cpu_usage_limit[2]) {
            dest_clk = __get_clock(curr_clk / (cpu_usage_limit[1] - 10) * usage_max);
        } else {
            clk_adjust_step = 2;
        }
        break;
    case 2:
        if (usage_max >= cpu_usage_limit[0] && usage_max <= cpu_usage_limit[2]) {
            break;
        }
        clk_adjust_step = 1;
        goto __again;
    }

    int min_clk = clock_list_sum();
    if (dest_clk < min_clk) {
        dest_clk = min_clk;
    }

    r_printf("cpu: %d %d  clk:%d %d %d, %d\n", usage[0], usage[1],
             curr_clk, min_clk, dest_clk, clk_adjust_step);

    //clock lock
    if (clk_locker.freq) {
        dest_clk = clk_locker.freq;
    } else if (dest_clk < min_clk) {
        dest_clk = min_clk;
    }

    clk_set_api("sys", dest_clk);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief clk_ref_fun
 *
 * @param p
 */
/* ----------------------------------------------------------------------------*/
static void clk_ref_fun(void *p)
{
    CLOCK_MANAGER_ENTER_CRITICAL();
    if (ref_cnt++ < CLOCK_DETECT_COUNTER) {
        clk_ref_cal();
    } else {
        sys_timer_del(clk_ref_timer);
        clk_ref_timer = 0;
        /* printf("clock_reflash_stop"); */
    }
    CLOCK_MANAGER_EXIT_CRITICAL();
}

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_refurbish，MIPS变化时调用此函数刷新。
 */
/* ----------------------------------------------------------------------------*/
void clock_refurbish(void)
{
    if (cpu_in_irq() || cpu_irq_disabled()) {
        //以上情况，需要改为在APP任务上设置
        int msg[3];
        msg[0] = (int)clock_refurbish;
        msg[1] = 1;
        msg[2] = 0;
        os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        /* sys_timeout_add(NULL, (void *)clock_refurbish, 1); */
        return;
    }

    CLOCK_MANAGER_ENTER_CRITICAL();

    //clk driver 需要提供每个芯片可以设置的最高挡位
    clk_set_api("sys", CLOCK_MAXIMUM_FREQ);

    ref_cnt = 0;
    clk_adjust_step = 0;

    if (clk_ref_timer) {
        sys_timer_modify(clk_ref_timer, CLOCK_DETECT_PERIOD);
    } else {
        clk_ref_timer = sys_timer_add(NULL, clk_ref_fun, CLOCK_DETECT_PERIOD);
    }

    ASSERT(clk_ref_timer);

    task_info_reset();

    CLOCK_MANAGER_EXIT_CRITICAL();
}

_NOINLINE_
int clk_set_unused(const char *name, int clk)
{
    return 0;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief init
 */
/* ----------------------------------------------------------------------------*/
static int clock_manager_init(void)
{
    CLOCK_MANAGER_INIT_CRITICAL();

    clock_alloc("base", CLOCK_MINIMUM_FREQ);

    return 0;
}
module_initcall(clock_manager_init);

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_manager_test
 */
/* ----------------------------------------------------------------------------*/
static void clk_test2_tmr_fun(void *p)
{
    clk_set_api("sys", CLOCK_MAXIMUM_FREQ);

    clock_refurbish();
}

void clock_manager_test2(void)
{
    sys_timer_add(NULL, clk_test2_tmr_fun, 60 * 1000);
}



static void clk_test3_tmr_fun(void *p)
{
	printf("clk_test3_tmr_fun222222222222222222222222222222222\n");
    int ret;
    ret = clock_unlock("sys");
    /* ASSERT(ret == 0); */
}
void clock_manager_test3(void)
{
    int ret;
	printf("clock_manager_test3333333333333333333333333333333333333333333\n");
    ret = clock_lock("sys", 128 * 1000000U);
    ASSERT(ret == 0);
    sys_timeout_add(NULL, clk_test3_tmr_fun, 15 * 1000);// sys_timeout_add(NULL, clk_test3_tmr_fun, 30 * 1000);

    /* void mpu_set(int idx, u32 begin, u32 end, u32 inv, const char *format, ...); */
    /* u32 addr = (u32)(&clk_locker.freq); */
    /* mpu_set(2, addr, addr+3, 0, "Cr"); */
}

