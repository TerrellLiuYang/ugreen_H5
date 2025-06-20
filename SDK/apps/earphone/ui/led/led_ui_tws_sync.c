#include "generic/typedef.h"
#include "classic/tws_api.h"
#include "led/led_ui_manager.h"
#include "app_config.h"


#if TCFG_USER_TWS_ENABLE

bool led_tws_sync_check(void)
{
    return tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED;
}

void led_tws_state_sync(int led_state, int msec, u8 force_sync)
{
    if ((tws_api_get_role() == TWS_ROLE_MASTER) || force_sync) {
        printf("role: %d, send sync: 0x%x, force_sync: %d", tws_api_get_role(), led_state, force_sync);
        tws_api_sync_call_by_uuid(0x2A1E3095, led_state, msec);
    }
}

void led_tws_state_sync_by_name(char *name, int msec, u8 force_sync)
{
    u32 uuid = led_ui_JBHash((u8 *)name, strlen(name));

    led_tws_state_sync(uuid, msec, force_sync);
}

/*
 * LED状态同步
 */
static void led_state_sync_handler(int state, int err)
{
    r_printf("%s: state: 0x%x, err: 0x%x", __func__, state, err); //master: err = 1(Tx), slave: 2(Rx)
    led_ui_do_display((u16)state, 1);
}

TWS_SYNC_CALL_REGISTER(tws_led_sync) = {
    .uuid = 0x2A1E3095,
    .task_name = "app_core",
    .func = led_state_sync_handler,
};

#endif

