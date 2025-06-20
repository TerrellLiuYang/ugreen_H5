#include "app_config.h"
#include "utils/syscfg_id.h"
#include "driver/gpio_config.h"
#include "system/init.h"
#include "system/spinlock.h"
#include "led_ui_tws_sync.h"
#include "led_ui_manager.h"
#include "led_ui_timer.h"
#include "pwm_led_ui.h"
#include "asm/pwm_led_hw.h"
#include "app_msg.h"
#include "app_config.h"


struct led_ui_manager {
    u8 init;
    u8 local_action_flag;
    u16 hw_len;
    u16 state_len;
    u8 *state_ptr;
    u8 *hw_ptr;
    volatile u16 current_state_uuid;
    volatile u8 current_state_loop;
    volatile u8 current_state_have_time; //没有时间的表示常亮状态
    u16 current_state_offset;
    u16 current_action_index;
    u8 current_action_max_num;

    u8 wait;
    u8 en;
    u8 insert_state_local_flag;
    u16 insert_state_uuid;
    u16 bak_state_uuid;
    spinlock_t lock;
};

static struct led_ui_manager led_ui;

#define __this 		(&led_ui)

u32 led_ui_JBHash(u8 *data, int len)
{
    u32 hash = 5381;

    while (len--) {
        hash += (hash << 5) + (*data++);
    }

    return hash;
}

static void led_config_file_init(void)
{
    u8 *hw_buf = NULL;
    u8 *state_buf = NULL;
    u16 hw_len = 0;
    u16 state_len = 0;
    int ret = 0;

    if (__this->init == 0) {
        hw_buf = syscfg_ptr_read(CFG_LED_ID, &hw_len);
        if (hw_buf == NULL) {
            printf("led hw config, Not Found");
            return;
        }

        printf("led hw config, len: 0x%x", hw_len);
        put_buf(hw_buf, hw_len);

        state_buf = syscfg_ptr_read(CFG_ID_LED_CTRL, &state_len);
        if (state_buf == NULL) {
            printf("led ctrl config, Not Found");
            return;
        }

        printf("led state config, len: 0x%x", state_len);
        /* put_buf(state_buf, state_len); */
        spin_lock_init(&(__this->lock));

        __this->hw_ptr = hw_buf;
        __this->hw_len = hw_len;
        __this->state_ptr = state_buf;
        __this->state_len = state_len;

        __this->init = 1;
        __this->en = 1;
    }
}

static u16 led_ui_display_match_by_uuid(u16 index)
{
    struct led_state_config state;

    for (u16 i = 0; i < __this->state_len;) {
        memcpy(&state, __this->state_ptr + i, sizeof(struct led_state_config));
        if (state.uuid == index) {
            return i;
        }

        i += state.len + 1;
    }

    return (u16)(-1);
}


u16 led_ui_state_config_get(struct led_state_config *config, u16 offset)
{
    u16 ret = 0;
    if (config && __this->state_ptr) {
        memcpy(config, __this->state_ptr + offset, sizeof(struct led_state_config));
        ret = sizeof(struct led_state_config);
    }

    return ret;
}

u16 led_ui_action_config_get(struct led_action *config, u16 offset)
{
    u16 ret = 0;

    if (config && __this->state_ptr) {
        memcpy(config, __this->state_ptr + offset, sizeof(struct led_action));
        ret = sizeof(struct led_action);
    }

    return ret;
}

static u16 led_hw_match_by_uuid(u16 index, u8 op)
{
    struct led_hw_config hw_config;

    u16 ret = (u16)(-1);

    for (u16 i = 0; i < __this->hw_len;) {
        memcpy(&hw_config, __this->hw_ptr, sizeof(struct led_hw_config));
        if (hw_config.led_uuid == index) {
            if (op == 0) {
                ret = i;
            } else if (op == 1) {
                ret = hw_config.io_uuid;
            } else if (op == 1) {
                ret = hw_config.led_logic;
            }
            break;
        }

        i += hw_config.len + 1;
    }

    return ret;
}

static u8 led_action_info_get(struct led_action *action)
{
    if ((__this->current_action_index >= __this->current_action_max_num) || (action == NULL)) {
        return 0;
    }

    u8 *file_offset = __this->state_ptr + __this->current_state_offset +
                      sizeof(struct led_state_config) + __this->current_action_index * sizeof(struct led_action);
    memcpy(action, file_offset, sizeof(struct led_action));


    return sizeof(struct led_action);
}

u8 led_ui_hw_info_get_by_uuid(u16 index, struct led_hw_config *hw_config)
{
    u8 ret = 0;

    for (u16 i = 0; i < __this->hw_len;) {
        memcpy(hw_config, __this->hw_ptr + i, sizeof(struct led_hw_config));
        if (hw_config->led_uuid == index) {
            ret = sizeof(struct led_hw_config);
            break;
        }

        i += sizeof(struct led_hw_config);
    }

    return ret;
}

u8 led_ui_hw_info_get_led_io(u8 index)
{
    struct led_hw_config hw_config;
    u8 ret = 0;

    u32 address_offset = index * sizeof(struct led_hw_config);

    memcpy(&hw_config, __this->hw_ptr + address_offset, sizeof(struct led_hw_config));

    /* printf("uuid = 0x%x",hw_config.led_uuid); */
    ret = uuid2gpio(hw_config.io_uuid);
    return ret;
}

static void led_ui_state_do_action(void)
{
    struct led_action action;
    struct led_hw_config hw_config;
    u8 error = 0;
    u8 local_flag;
    u16 recover_uuid;
    u8 is_wait;

    error = led_action_info_get(&action);
    if (error == 0) {
        goto __do_action_end;
    }

    error = led_ui_hw_info_get_by_uuid(action.led_uuid, &hw_config);
    if (error == 0) {
        printf("led_uuid: 0x%x, not found", action.led_uuid);
        goto __do_action_end;
    }

    switch (hw_config.led_logic) {
    case PWM_LOW_LOGIC:
    case PWM_HIGH_LOGIC:
        pwm_led_ui_action_display(&action, &hw_config);
        break;
    case IO_LOW_LOGIC:
        break;
    case IO_HIGH_LOGIC:
        break;
    default:
        break;
    }
    __this->current_action_index++;

    extern void log_pwm_led_info();
    log_pwm_led_info();
    //Next Action:
    switch (action.next) {
    case ACTION_WAIT:
        if (action.time) {
            led_ui_timer_add(led_ui_state_do_action, action.time);
        } else {
            led_ui_state_do_action();
        }
        break;
    case ACTION_CONTINUE:
        led_ui_state_do_action();
        break;
    case ACTION_END:
        spin_lock(&(__this->lock));
        recover_uuid = __this->bak_state_uuid;
        local_flag = __this->local_action_flag;
        is_wait = __this->wait;
        if (__this->current_state_have_time) { //常亮灯效不清当前uuid
            __this->current_state_uuid = 0;
        }
        __this->bak_state_uuid = 0;
        __this->wait = 0;
        spin_unlock(&(__this->lock));
        if (recover_uuid) {
            led_ui_manager_display(local_flag, recover_uuid, is_wait ? 0 : 2);
        }
        break;
    case ACTION_LOOP:
        __this->current_action_index = 0;
        if (action.time) {
            led_ui_timer_add(led_ui_state_do_action, action.time);
        } else {
            led_ui_state_do_action();
        }
        break;
    default:
        break;
    }

__do_action_end:
    return;
}

static u8 led_ui_action_with_wait_time(u16 index)
{
    struct led_state_config state;
    struct led_action action;
    u8 have_time = 0;
    u8 actions = 0;

    spin_lock(&(__this->lock));

    led_ui_state_config_get(&state, index);
    actions = (state.len - 2) / sizeof(struct led_action);

    for (u8 i = 0; i < actions; i++) {
        led_ui_action_config_get(&action, index + sizeof(struct led_state_config) + i * sizeof(struct led_action));
        if (action.time && (action.next == ACTION_WAIT)) {
            have_time = 1;
            break;
        }
    }
    spin_unlock(&(__this->lock));

    return have_time;
}

static u8 led_ui_loop_check(u16 index)
{
    struct led_state_config state;
    struct led_action action;
    u16 offset = index;
    u8 actions = 0;

    led_ui_state_config_get(&state, index);

    //解析配置的周期规律:
    actions = (state.len - 2) / sizeof(struct led_action);
    offset += sizeof(struct led_state_config); //Seek To The First Action

    //读取最后一个action判断是否为循环模式
    led_ui_action_config_get(&action, offset + (actions - 1) * sizeof(struct led_action));

    if (action.next == ACTION_LOOP) {
        return 1;
    }

    return 0;
}

static int led_ui_single_action_display(u16 offset)
{
    struct led_state_config state;

    memcpy(&state, __this->state_ptr + offset, sizeof(struct led_state_config));

    if (state.len - 2 == 0) {
        return -2; //没有动作配置, 参数配置错误
    }

    __this->current_state_uuid = state.uuid;
    __this->current_state_offset = offset;
    __this->current_state_loop = led_ui_loop_check(offset);
    __this->current_state_have_time = led_ui_action_with_wait_time(offset);
    printf("current_state_loop = %d, have_time = %d", __this->current_state_loop, __this->current_state_have_time);
    __this->current_action_max_num = state.len - 2 / sizeof(struct led_action);
    __this->current_action_index = 0;

    //Do Action:
    led_ui_state_do_action();

    return 0;
}

static int led_ui_period_action_display_try(u16 offset)
{
    int ret = pwm_led_ui_period_action_display_try(offset);

    printf("%s: ret = %d", __func__, ret);

    return ret;
}

__attribute__((weak))
u16 led_uuid_change_by_user(u16 uuid, u8 *sync, u8 *local_action_flag)
{
    return uuid;
}

void led_ui_do_display(u16 uuid, u8 sync)
{
    printf("%s: uuid: 0x%x, en:%d, wait:%d", __func__, uuid, __this->en, __this->wait);
    if (!__this->en) {
        return;
    }

    if (__this->wait) {
        printf("tws_sync_ui, cancel wait, uuid:%x", uuid);
        __this->wait = 0;
        __this->bak_state_uuid = 0;
    }

    int ret = 0;
    uuid = led_uuid_change_by_user(uuid, &sync, &__this->local_action_flag);
    u16 index = led_ui_display_match_by_uuid(uuid);
    if (index == (u16)(-1)) {
        printf("%d: led ui uuid 0x%x not found", __LINE__, uuid);
        return;
    }

    pwm_led_ui_action_pre();
    led_ui_timer_free();

    if (sync) {
        led_ui_single_action_display(index);
    } else {
        ret = led_ui_period_action_display_try(index);
        if (ret < 0) {
            led_ui_single_action_display(index);
        } else {
            __this->current_state_uuid = uuid;
            __this->current_state_loop = 1;
            __this->current_state_have_time = 0;
        }
    }
}

void led_ui_manager_enable(u8 en)
{
    printf("%s, %d", __func__, en);
    __this->en = en;
}

void led_ui_manager_set_wait(u8 wait)
{
    printf("func:%s, cur_wait:%d, wait:%d", __FUNCTION__, __this->wait, wait);
    if (__this->wait && wait == 0) {
        __this->bak_state_uuid = 0;
    }
    __this->wait = wait;
}

u16 led_ui_get_current_uuid()
{
    return __this->current_state_uuid;
}

void led_ui_manager_display(u8 local_action, u16 uuid, u8 mode)
{
    if (__this->init == 0 || !__this->en) {
        return;
    }
    /* if (__this->current_state_uuid == uuid) { */
    /* return; */
    /* } */

    u8 force = 0;
    if (__this->wait) {
        printf("wait_current_ui_end, uuid:%x, flag:%d", uuid, local_action);
        __this->bak_state_uuid = uuid;
        __this->local_action_flag = local_action;
        return;
    }

    spin_lock(&(__this->lock));

    if (mode == 1) {
        //insert:
        u16 offset = led_ui_display_match_by_uuid(uuid);
        if (offset == (u16)(-1)) {
            spin_unlock(&(__this->lock));
            printf("%d: led ui uuid 0x%x not found", __LINE__, uuid);
            return;
        }
        if (led_ui_loop_check(offset) != 0) {
            spin_unlock(&(__this->lock));
            printf("%d: led ui uuid 0x%x loop, not supoort intsert", __LINE__, uuid);
            return;
        }
        __this->insert_state_uuid = uuid;
        __this->insert_state_local_flag = local_action;
        if (__this->bak_state_uuid == 0) {
            __this->bak_state_uuid = __this->current_state_uuid;
        }
    } else if (mode == 2) {
        //recover:
        __this->local_action_flag = local_action;
        if (__this->insert_state_local_flag && !local_action) {
            //Whatever master or slave
            __this->insert_state_local_flag = 0;
            force = 1;
        }
    } else {
        //state change:
        __this->local_action_flag = local_action;
        __this->bak_state_uuid = 0;
    }

    spin_unlock(&(__this->lock));

    if (led_tws_sync_check() && (!local_action)) {
        led_tws_state_sync(uuid, 300, force);
    } else {
        led_ui_do_display(uuid, 0);
    }
}

int led_ui_manager_init(void)
{
    led_config_file_init();

    if (__this->init == 0) {
        return 0;
    }

    return 0;
}

u8 led_ui_manager_status_busy_query(void)
{
    if (__this->init == 0) {
        return 0;
    }

    if (__this->current_state_uuid) {
        if (__this->current_state_loop) {
            return 0;
        } else {
            if (__this->current_state_have_time) {
                return 1;
            } else {
                return 0;
            }
        }
	}
    return 0;
}


void led_ui_manager_display_by_name(u8 local_action, char *name, u8 mode)
{
    u32 uuid = led_ui_JBHash((u8 *)name, strlen(name));

    printf("led display name: %s, uuid: 0x%x", name, uuid);

    led_ui_manager_display(local_action, (u16)uuid, mode);
}


//=================================================================//
//                      LED UI Test Code                           //
//=================================================================//

#ifdef PWM_LED_DEBUG_ENABLE
/* #if 1 //LED UI Test Code */
#if (PWM_LED_TEST_MODE != 0)
#include "system/timer.h"
struct led_mode {
    u16 uuid;
    char *name;
};

struct led_mode led_mode_table[] = {
    {0xae15, "PWM_LED_BLUE_RED_ALL_ON"},
    {0Xb37e, "PWM_LED_BLUE_RED_ALL_OFF"},
    {0X1160, "PWM_LED_BLUE_OFF_RED_ON"},
    {0X58b6, "PWM_LED_BLUE_OFF_RED_SLOW_FLASH"},
    {0Xacb8, "PWM_LED_BLUE_OFF_RED_FAST_FLASH"},
    {0Xcbf6, "PWM_LED_BLUE_OFF_RED_5S_FLASH"},
    {0X5857, "PWM_LED_BLUE_OFF_RED_5S_DOUBLE_FLASH"},
    {0X861e, "PWM_LED_BLUE_OFF_RED_BREATHE"},
    {0X4203, "PWM_LED_BLUE_OFF_RED_FLASH_THREE_TIME"},

    {0Xfc35, "PWM_LED_RED_OFF_BLUE_ON"},
    {0X438b, "PWM_LED_RED_OFF_BLUE_SLOW_FLASH"},
    {0X978d, "PWM_LED_RED_OFF_BLUE_FAST_FLASH"},
    {0Xc3eb, "PWM_LED_RED_OFF_BLUE_5S_FLASH"},
    {0X504c, "PWM_LED_RED_OFF_BLUE_5S_DOUBLE_FLASH"},
    {0X70f3, "PWM_LED_RED_OFF_BLUE_BREATHE"},
    {0X8778, "PWM_LED_RED_OFF_BLUE_FLASH_THREE_TIME"},

    {0X8a52, "PWM_LED_RED_BLUE_FAST_FLASH_ALTERNATELY"},
    {0X3650, "PWM_LED_RED_BLUE_SLOW_FLASH_ALTERNATELY"},
    {0X63b8, "PWM_LED_RED_BLUE_FAST_BREATHE_ALTERNATELY"}
};

u32 usr_led_timer_lock_id = 0;
void usr_led_timer_lock(void *priv)
{
    PWMLED_LOG_INFO("led timer done allow to enter powerdown");
    sys_hi_timer_del(usr_led_timer_lock_id);
    return ;
}

char led_ui_test_index = -1;
char led_ui_test_index_next2 = -1;
static void led_ui_manager_mode_test(void *priv)
{
    led_ui_test_index++;
    led_ui_test_index_next2 = led_ui_test_index + 2;

    if (led_ui_test_index >= ARRAY_SIZE(led_mode_table)) {
        led_ui_test_index = 0;
    }

    if (led_ui_test_index_next2 >= ARRAY_SIZE(led_mode_table)) {
        led_ui_test_index_next2 = 0;
    }

    PWMLED_LOG_INFO("LED UI Manager: display %d: %s", led_ui_test_index, led_mode_table[led_ui_test_index].name);

#ifdef PWM_LED_IO_TEST
    LED_TIMER_TEST_PORT->OUT ^= BIT(LED_TIMER_TEST_PIN);
#endif

#if (PWM_LED_TEST_MODE == 1)
    led_ui_manager_display(0, led_mode_table[led_ui_test_index].uuid, 0);

#else


    //*********************************************************************************
    PWMLED_LOG_DEBUG("bt_tws_remove_pairs>>>>>");

    extern int tws_api_remove_pairs();
    extern void bt_tws_remove_pairs();

    int err = tws_api_remove_pairs();
    if (err) {
        bt_tws_remove_pairs();
    }

    os_time_dly(200);

    /* APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS,    // 执行配对和连接的默认流程 */
    PWMLED_LOG_DEBUG("APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS>>>>");
    app_send_message(APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS, 0);

    //*********************************************************************************

#endif //#if 0

//
#if 1
    PWMLED_LOG_DEBUG("20s not in powerdown>>>");

    //30s内不进入低功耗
    usr_led_timer_lock_id = sys_hi_timeout_add(NULL, usr_led_timer_lock, 30000);
#endif //#if 0
    //
    //

}



static char *led_mode_name_table[] = {
    "PWM_LED_BLUE_RED_ALL_OFF",
    "PWM_LED_BLUE_OFF_RED_ON",
    "PWM_LED_BLUE_OFF_RED_SLOW_FLASH",
    "PWM_LED_BLUE_OFF_RED_FAST_FLASH",
    "PWM_LED_BLUE_OFF_RED_5S_FLASH",
    "PWM_LED_BLUE_OFF_RED_5S_DOUBLE_FLASH",
    "PWM_LED_BLUE_OFF_RED_BREATHE",
    "PWM_LED_BLUE_OFF_RED_FLASH_THREE_TIME",
    "PWM_LED_RED_OFF_BLUE_ON",
    "PWM_LED_RED_OFF_BLUE_SLOW_FLASH",
    "PWM_LED_RED_OFF_BLUE_FAST_FLASH",
    "PWM_LED_RED_OFF_BLUE_5S_FLASH",
    "PWM_LED_RED_OFF_BLUE_5S_DOUBLE_FLASH",
    "PWM_LED_RED_OFF_BLUE_BREATHE",
    "PWM_LED_RED_OFF_BLUE_FLASH_THREE_TIME",
    "PWM_LED_RED_BLUE_FAST_FLASH_ALTERNATELY",
    "PWM_LED_RED_BLUE_SLOW_FLASH_ALTERNATELY",
    "PWM_LED_RED_BLUE_FAST_BREATHE_ALTERNATELY"
};

extern void pwm_led_hw_info_dump(void);

static void led_ui_manager_mode_by_name_test(void *priv)
{
    static u8 test_index_2 = 0;
    if (test_index_2 >= ARRAY_SIZE(led_mode_name_table)) {
        test_index_2 = 0;
    }

    /* pwm_led_hw_info_dump(); */

    led_ui_manager_display_by_name(0, led_mode_name_table[test_index_2++], 0);
}

#endif /* #if 0 //LED UI Test Code */

int led_ui_manager_test_code(void)
{
#if (PWM_LED_TEST_MODE != 0)
    PWMLED_LOG_INFO("led_ui_manager_test_code>>>");

#ifdef PWM_LED_IO_TEST
    //设置为输出
    LED_TIMER_TEST_PORT->DIR &= ~BIT(LED_TIMER_TEST_PIN);
    //低
    LED_TIMER_TEST_PORT->OUT &= ~BIT(LED_TIMER_TEST_PIN);
#endif

    //30s的不进入低功耗时间+20s的进入低功耗时间
    sys_timer_add(NULL, led_ui_manager_mode_test, 50000);
    /* sys_hi_timer_add(NULL, led_ui_manager_mode_test, 40000); */

    /* sys_timer_add(NULL, led_ui_manager_mode_by_name_test, 20000); */

#endif //#if 0

    return 0;
}
late_initcall(led_ui_manager_test_code);

#endif
