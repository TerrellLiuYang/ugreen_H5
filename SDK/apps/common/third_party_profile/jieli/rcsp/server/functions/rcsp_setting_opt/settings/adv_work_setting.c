#include "app_config.h"
#include "adv_work_setting.h"
#include "syscfg_id.h"
#include "ble_rcsp_server.h"
#include "rcsp_adv_bluetooth.h"

#include "rcsp_setting_sync.h"
#include "rcsp_setting_opt.h"
#include "os/os_api.h"
#include "btstack/avctp_user.h"
#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if (RCSP_MODE && RCSP_ADV_EN)
#if RCSP_ADV_WORK_SET_ENABLE

static u8 g_work_mode = RCSPWorkModeNormal;

extern void bt_set_low_latency_mode(int enable, u8 tone_play_enable, int delay_ms);
static void set_work_setting(u8 *work_setting_info)
{
    g_work_mode = *work_setting_info;
    /* printf("rcsp_work %s, %s, %d, mode:%d\n", __FILE__, __FUNCTION__, __LINE__, g_work_mode); */
}

static int get_work_setting(u8 *work_setting_info)
{
    /* printf("rcsp_work %s, %s, %d, mode:%d\n", __FILE__, __FUNCTION__, __LINE__, g_work_mode); */
    *work_setting_info = g_work_mode;
    return 0;
}

static void adv_work_setting_vm_value(u8 *work_setting_info)
{
    syscfg_write(CFG_RCSP_ADV_WORK_SETTING, work_setting_info, sizeof(u8));
}

static void adv_work_setting_sync(u8 *work_setting_info)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_rcsp_setting(ATTR_TYPE_WORK_MODE);
    }
#endif
}

static void __rcsp_work_set_call(void)
{
    /* printf("__rcsp_work_set_call\n"); */
    if (RCSPWorkModeNormal == g_work_mode) {
        bt_set_low_latency_mode(0, 1, 300);
    } else if (RCSPWorkModeGame == g_work_mode) {
        bt_set_low_latency_mode(1, 1, 300);
    }
}

static void rcsp_work_set_deal(void)
{
    int err;
    int msg[3];
    msg[0] = (int) __rcsp_work_set_call;
    msg[1] = 1;
    msg[2] = 0;

    err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
    /* printf("%s, err %x\n",  __FUNCTION__, err); */
}

static void update_work_setting_state(void)
{
    /* printf("%s, set deal\n", __FUNCTION__); */
    rcsp_work_set_deal();
}

static void deal_work_setting(u8 *work_setting_info, u8 write_vm, u8 tws_sync)
{
    /* printf("rcsp_work %s, %s, %d, work_setting_info:%d\n", __FILE__, __FUNCTION__, __LINE__, *work_setting_info); */
    if (work_setting_info) {
        set_work_setting(work_setting_info);
    }
    if (write_vm) {
        adv_work_setting_vm_value(work_setting_info);
    }
    if (tws_sync) {
        adv_work_setting_sync(work_setting_info);
    }
    update_work_setting_state();
}

void rcsp_set_work_mode(RCSPWorkMode work_mode)
{
    u8 _work_mode = work_mode;
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        if (TWS_ROLE_MASTER == tws_api_get_role()) {
            deal_work_setting(&_work_mode, 1, 1);
        }
    } else {
        deal_work_setting(&_work_mode, 1, 1);
    }
#else
    deal_work_setting(&_work_mode, 1, 1);
#endif
    u8 adv_cmd = 0x0;
    adv_info_device_request(&adv_cmd, sizeof(adv_cmd));
}

int ble_vendor_is_slow_state(void)
{
    if ((BT_STATUS_PLAYING_MUSIC == bt_get_connect_status()) && (RCSPWorkModeGame == g_work_mode)) {
        return 1;
    } else {
        return 0;
    }
}

static int adv_work_opt_init(void)
{
    u8 work_setting_info = RCSPWorkModeNormal;
    if (rcsp_read_data_from_vm(CFG_RCSP_ADV_WORK_SETTING, &work_setting_info, sizeof(work_setting_info))) {
        if (work_setting_info != RCSPWorkModeNormal && work_setting_info != RCSPWorkModeGame) {
            work_setting_info = RCSPWorkModeNormal;
        }
        set_work_setting(&work_setting_info);
        __rcsp_work_set_call();
    }
    return 0;
}

static int work_set_setting_extra_handle(void *setting_data, void *setting_data_len)
{
    u8 dlen = *((u8 *)setting_data_len);
    u8 *work_mode_setting_data = (u8 *)setting_data;
    memcpy(&g_work_mode, work_mode_setting_data, dlen);
    return 0;
}

static int work_get_setting_extra_handle(void *setting_data, void *setting_data_len)
{
    u8 **setting_data_ptr = (u8 **)setting_data;
    *setting_data_ptr = &g_work_mode;
    return sizeof(g_work_mode);
}

static RCSP_SETTING_OPT adv_work_opt = {
    .data_len = 1,
    .setting_type = ATTR_TYPE_WORK_MODE,
    .syscfg_id = CFG_RCSP_ADV_WORK_SETTING,
    .deal_opt_setting = deal_work_setting,
    .set_setting = set_work_setting,
    .get_setting = get_work_setting,
    .custom_setting_init = adv_work_opt_init,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = NULL,
    .custom_setting_release = NULL,
    .set_setting_extra_handle = work_set_setting_extra_handle,
    .get_setting_extra_handle = work_get_setting_extra_handle,
};

REGISTER_APP_SETTING_OPT(adv_work_opt);

#endif
#endif
