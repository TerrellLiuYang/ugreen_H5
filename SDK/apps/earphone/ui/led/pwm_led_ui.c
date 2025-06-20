#include "driver/gpio_config.h"
#include "asm/pwm_led_hw.h"
#include "led_ui_cfg.h"
#include "led_ui_manager.h"

struct pwm_led_action_hw_info {
    u16 led_uuid;
    u16 io_uuid;
    u16 bright;
    u8 logic;
    u8 mode;
    u8 breath_rate;
};

enum PWM_LED_FLASH_MODE {
    PERIOD_SINGLE_FLASH, 	//周期单闪
    PERIOD_DUAL_FLASH, 		//周期互闪
    PERIOD_DOUBLE_FLASH, 	//周期双闪
    PERIOD_SINGLE_BREATHE,  //周期单灯呼吸
    PERIOD_DUAL_BREATHE, 	//周期双灯呼吸
};

struct pwm_led_mode_templet {
    u8 flash_mode; //enum PWM_LED_FLASH_MODE
    u8 timer_cnt;
    u8 flash_cnt;
};

static struct pwm_led_mode_templet templet_table[] = {
    {PERIOD_SINGLE_FLASH, 	2, 	1},
    {PERIOD_DUAL_FLASH, 	2, 	2},
    {PERIOD_DOUBLE_FLASH, 	4, 	2},
    {PERIOD_SINGLE_BREATHE, 3, 	1},
    {PERIOD_DUAL_BREATHE, 	6, 	2}
};


int pwm_led_ui_period_action_display_try(u16 offset)
{
    u8 actions = 0;
    struct led_action action;
    u8 error = 0;
    u8 pwm_led_two_io_mode = 0;

    /* y_printf("%s: offset: %d", __func__, offset); */

    struct led_state_config state;

    /* return -9; */

    led_ui_state_config_get(&state, offset);

    /* y_printf("uuid: 0x%x, len: %d", state.uuid, state.len); */

    //解析配置的周期规律:
    if (state.len - 2 == 0) {
        return -1; //没有动作配置, 参数配置错误
    }

    actions = (state.len - 2) / sizeof(struct led_action);
    offset += sizeof(struct led_state_config); //Seek To The First Action

    //读取最后一个action判断是否为循环模式
    led_ui_action_config_get(&action, offset + (actions - 1) * sizeof(struct led_action));

    if (action.next != ACTION_LOOP) {
        return -2;
    }

    struct led_hw_config hw_config;
    struct pwm_led_action_hw_info hw_info[2];
    u8 led_uuid_cnt = 0;
    u8 hw_index;
    u8 timer_cnt = 0;
    u8 flash_cnt = 0;
    u16 flash_index = 0;
    u16 timer_msec[8];

    memset((void *)hw_info, 0, sizeof(hw_info));
    memset((void *)timer_msec, 0, sizeof(timer_msec));

    //循环模式, 解析循环规律:
    for (u8 i = 0; i < actions; i++) {
        led_ui_action_config_get(&action, offset + i * sizeof(struct led_action));
        error = led_ui_hw_info_get_by_uuid(action.led_uuid, &hw_config);
        if (error == 0) {
            continue;
        }
        if ((hw_config.led_logic != PWM_LOW_LOGIC) &&
            (hw_config.led_logic != PWM_HIGH_LOGIC)) {
            return -3;
        }

        if (led_uuid_cnt == 0) {
            hw_info[0].led_uuid = hw_config.led_uuid;
            hw_info[0].io_uuid = hw_config.io_uuid;
            hw_info[0].logic = hw_config.led_logic;
            hw_index = 0;
            led_uuid_cnt++;
        } else if (led_uuid_cnt == 1) {
            if (hw_config.led_uuid != hw_info[0].led_uuid) {
                hw_info[1].led_uuid = hw_config.led_uuid;
                hw_info[1].io_uuid = hw_config.io_uuid;
                hw_info[1].logic = hw_config.led_logic;

                if (hw_info[0].io_uuid != hw_info[1].io_uuid) {
                    pwm_led_two_io_mode = 1;
                    /* PWMLED_LOG_INFO("pwm_led_two_io_mode = 1"); */

                    /* 这个返回值曾经用于指示不支持多IO，但是现在支持了*/
                    /* return -4; */
                }
                if (hw_info[0].logic == hw_info[1].logic) {
                    //这个返回值曾经用于指示不支持相同逻辑，但是现在支持了
                    /* return -5; */
                }
                hw_index = 1;
                led_uuid_cnt++;
            } else {
                hw_index = 0;
            }
        } else if (led_uuid_cnt == 2) {
            if (hw_config.led_uuid == hw_info[0].led_uuid) {
                hw_index = 0;
            } else if (hw_config.led_uuid == hw_info[1].led_uuid) {
                hw_index = 1;
            } else {
                return -6;
                //不支持3个LED操作
            }
        }

        if (hw_info[hw_index].mode == 0) {
            if (action.breath_rate) {
                hw_info[hw_index].mode = 3; //呼吸模式
                hw_info[hw_index].breath_rate = action.breath_rate; //呼吸模式
            } else if (action.on_off == 1) {
                hw_info[hw_index].mode = 1; //普通闪烁模式
            }
            hw_info[hw_index].bright = (action.bright * 500) / 100; //呼吸模式
        }

        if (action.time) {
            if (timer_cnt >= ARRAY_SIZE(timer_msec)) {
                return -7;
            }
            timer_msec[timer_cnt++] = action.time;
            if (action.on_off == LED_TURN_ON) {
                flash_cnt++;
            }
        }
    }

    u8 index;
    u16 breath_time;
    u16 breath_delay_time;
    for (index = 0; index < ARRAY_SIZE(templet_table); index++) {
        if ((templet_table[index].timer_cnt == timer_cnt) &&
            (templet_table[index].flash_cnt == flash_cnt)) {
            break;
        }
    }
    if (index >= ARRAY_SIZE(templet_table)) {
        return -8; //Not Hit
    }

    if ((hw_info[0].mode) && (hw_info[1].mode)) {
        flash_index = 2;
    } else if ((hw_info[0].mode)) {
        hw_index = 0;
        flash_index = hw_info[hw_index].logic;
    } else {
        hw_index = 1;
        flash_index = hw_info[hw_index].logic;
    }

    switch (templet_table[index].flash_mode) {
    case PERIOD_DUAL_FLASH:
    case PERIOD_DUAL_BREATHE:

        if (pwm_led_two_io_mode == 1) {
            /* PWMLED_LOG_ERR("pwm_led_two_io_mode = 1, need led_ui_single_action_display to change IO, ret = -4"); */
            //ret = -4 双io的闪灯也需要走led_ui_single_action_display来切灯
            //必须这里返回，因为下面的代码就操作到io了。

            /* 双io需要用到定时器切io来选模块的方式来切灯 */
            //因此功耗会比单灯高一点，因为会按照灯效周期唤醒

            /* 不能够像单io那样直接模块使能省掉定时器操作，可以在低功耗情况下使用 */

            return -4;
        }
        break;
    default:
        break;

    }

    //硬件配置:
    u8 gpio = uuid2gpio(hw_info[hw_index].io_uuid);
    pwm_led_hw_io_enable(gpio);

    /* y_printf("flash_mode: %d, led logic: %d, gpio: 0x%x, bright: %d", templet_table[index].flash_mode, flash_index, gpio, hw_info[hw_index].bright); */
    for (u8 i = 0; i < timer_cnt; i++) {
        /* y_printf("timer %d: %d", i, timer_msec[i]); */
    }

    pwm_led_hw_display_ctrl_clear();

    switch (templet_table[index].flash_mode) {
    case PERIOD_SINGLE_FLASH:
        pwm_led_one_flash_display(flash_index,
                                  hw_info[hw_index].bright,
                                  hw_info[hw_index].bright,
                                  timer_msec[0] + timer_msec[1], 10, timer_msec[0]);
        break;
    case PERIOD_DUAL_FLASH:
        pwm_led_one_flash_display(2,
                                  hw_info[0].bright,
                                  hw_info[1].bright,
                                  timer_msec[0], -1, -1);
        break;
    case PERIOD_DOUBLE_FLASH:
        pwm_led_double_flash_display(flash_index,
                                     hw_info[hw_index].bright,
                                     hw_info[hw_index].bright,
                                     timer_msec[0] + timer_msec[1] + timer_msec[2] + timer_msec[3],
                                     timer_msec[0], timer_msec[1], timer_msec[2]);
        break;
    case PERIOD_SINGLE_BREATHE:
        breath_time = timer_msec[0] + timer_msec[1];
        breath_delay_time = breath_time / 2;
        breath_delay_time = (20 - hw_info[hw_index].breath_rate) * breath_delay_time / 20;
        pwm_led_breathe_display(flash_index, breath_time,
                                hw_info[hw_index].bright,
                                hw_info[hw_index].bright,
                                breath_delay_time, breath_delay_time, timer_msec[2]);
        break;
    case PERIOD_DUAL_BREATHE:
        breath_time = timer_msec[0] + timer_msec[1];
        breath_delay_time = breath_time / 2;
        breath_delay_time = (20 - hw_info[hw_index].breath_rate) * breath_delay_time / 20;
        pwm_led_breathe_display(flash_index, breath_time,
                                hw_info[0].bright,
                                hw_info[1].bright,
                                breath_delay_time, breath_delay_time, timer_msec[2]);
        break;
    default:
        break;
    }
    led_hw_keep_data.keep_data_flag = 1;

    return 0;
}

void pwm_led_ui_action_pre()
{
    //切换新的灯效后需要重置该变量
    led_hw_keep_data.prev_breath_time =  0;
    //默认不需要保存数据
    led_hw_keep_data.keep_data_flag =  0;
}

void pwm_led_ui_action_display(struct led_action *action, struct led_hw_config *hw_config)
{
    struct pwm_led_hw_ctrl ctrl;

    ctrl.state = (action->on_off == 1) ? 1 : 0;
    ctrl.gpio = uuid2gpio(hw_config->io_uuid);
    ctrl.logic = (hw_config->led_logic == PWM_LOW_LOGIC) ? 0 : 1;
    ctrl.bright = action->bright;

    if (action->breath_rate) {
        /* 3.使用发现单io推双灯验证推2个呼吸灯效则使用了特殊模式操作，推三个则使用了使用led_ui_state_do_action函数，推灯出现重复操作现象，灯效被打断。 */
        /* 3.因此通过以下策略： */
        /* 在do_action的时候，如果第一次检测到是呼吸波形，第二次也是呼吸波形并且时间和上一次的一致就跳过不操作呼吸 */
        /* 4.目前单io和双io都支持两个以上的呼吸灯效 */
        if (led_hw_keep_data.prev_breath_time == action->time) {
            led_hw_keep_data.prev_breath_time =  0;
            PWMLED_HW_LOG_INFO("led_hw_keep_data.prev_breath_time == action->time,skip next breathe action");
            return ;
        }
        ctrl.breath_time = action->time;
        led_hw_keep_data.prev_breath_time = action->time;
        ctrl.breath_rate = action->breath_rate;
    }

    pwm_led_hw_display_ctrl(&ctrl);
}



