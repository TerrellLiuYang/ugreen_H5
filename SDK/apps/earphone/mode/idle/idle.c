#include "idle.h"
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "app_tone.h"
#include "asm/charge.h"
#include "app_charge.h"
#include "app_main.h"
#include "led/led_ui_manager.h"
#include "vm.h"
#include "app_chargestore.h"
#include "app_testbox.h"
#include "user_cfg.h"
#include "key/key_driver.h"


#if TCFG_ANC_BOX_ENABLE
#include "app_ancbox.h"
#endif

#define LOG_TAG             "[APP_IDLE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"



static void app_idle_enter_softoff(void)
{
    //ui_update_status(STATUS_POWEROFF);
    while (led_ui_manager_status_busy_query()) {

        /* 如果新增的灯效等待时间过长，而等待的时候没有喂狗，会导致看门狗异常起来，打断了正常的sdk流程 */
        extern void wdt_clear(void);
        wdt_clear();

        log_info("ui_status:%d\n", led_ui_manager_status_busy_query());
    }
#if TCFG_CHARGE_ENABLE
    if (get_lvcmp_det() && (0 == get_charge_full_flag())) {
        log_info("charge inset, system reset!\n");
        cpu_reset();
    }
#endif

#if TCFG_SMART_VOICE_ENABLE
    extern void audio_smart_voice_detect_close(void);
    audio_smart_voice_detect_close();
#endif

    extern void dac_power_off(void);
    dac_power_off();    // 关机前先关dac

    power_set_soft_poweroff();
}





static void idle_mode_key_msg_handler(int *msg)
{
    int key_value = APP_MSG_KEY_VALUE(msg[0]);
    int key_action = APP_MSG_KEY_ACTION(msg[0]);

    switch (key_value) {
    case KEY_POWER:
        switch (key_action) {
        case KEY_ACTION_CLICK:
            break;
        case KEY_ACTION_HOLD:
            break;
        }
        break;
    case KEY_NEXT:
        break;
    case KEY_PREV:
        break;
    default:
        break;
    }
}


static int idle_mode_msg_handler(int *msg)
{
    const struct app_msg_handler *handler;

    switch (msg[0]) {
    case MSG_FROM_APP:
        idle_mode_key_msg_handler(msg + 1);
        break;
    case MSG_FROM_TONE:
        break;
    }

    return 0;
}

void app_power_off(void *priv)
{
    app_idle_enter_softoff();
}

static int app_power_off_tone_cb(void *priv, enum stream_event event)
{
    if (event == STREAM_EVENT_STOP) {
        app_idle_enter_softoff();
    }
    return 0;
}

static int idle_mode_enter(int param)
{
    log_info("idle_mode_enter: %d\n", param);

    switch (param) {
    case IDLE_MODE_PLAY_POWEROFF:
        if (app_var.goto_poweroff_flag) {
            syscfg_write(CFG_MUSIC_VOL, &app_var.music_volume, 2);
            //如果开启了VM配置项暂存RAM功能则在关机前保存数据到vm_flash
            if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) {
                vm_flush2flash(1);
            }
            os_taskq_flush();
            int ret = play_tone_file_callback(get_tone_files()->power_off, NULL,
                                              app_power_off_tone_cb);
            printf("power_off tone play ret:%d", ret);
            if (ret) {
                if (app_var.goto_poweroff_flag) {
                    log_info("power_off tone play err,enter soft poweroff");
                    app_idle_enter_softoff();
                }
            }
        }
        break;
    case IDLE_MODE_WAIT_POWEROFF:
        os_taskq_flush();
        syscfg_write(CFG_MUSIC_VOL, &app_var.music_volume, 2);
        break;
    case IDLE_MODE_CHARGE:
        break;
    }

    return 0;
}

static const struct app_mode_ops idle_mode_ops = {
    .enter          = idle_mode_enter,
    .msg_handler    = idle_mode_msg_handler,
};
REGISTER_APP_MODE(idle_mode) = {
    .name   = APP_MODE_IDLE,
    .index  = 0,
    .ops    = &idle_mode_ops,
};
