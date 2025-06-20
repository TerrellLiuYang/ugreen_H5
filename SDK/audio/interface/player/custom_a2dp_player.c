#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "app_main.h"
#include "btstack/a2dp_media_codec.h"
#include "bt_tws.h"

#if CUSTOM_A2DP_EN
//注意：
//1、需要将bt_start_a2dp_slience_detect return掉
//2、需要将tws_a2dp_play.c里面的BT_STATUS_A2DP_MEDIA_STOP处理注释掉
//3、需要将tws_a2dp_play.c里面的TWS_EVENT_MONITOR_START处理注释掉
//4、需要工具上面将一拖二场景的A2DP抢占去掉

extern u8 tws_monitor_start;

#define AVC_PLAY			0x44
#define AVC_STOP			0x45
#define AVC_PAUSE			0x46

#define AVCTP_DEVICE_NUM            2

//延迟两秒，防止手机反复暂停播放时会切换到另一个手机上。
#define AVCTP_CTRL_STOP_TIME        2000

struct avctp_ctrl_tws_t {
    unsigned long remain_time;  /*! \brief      剩余时间 */
    u8 mac[6];                  /*! \brief      mac地址 */
};

struct avctp_ctrl_t {
    unsigned long target_time;  /*! \brief      目标时间 */
    u16 timer;                  /*! \brief      定时器 */
    u8 mac[6];                  /*! \brief      mac地址 */
};
struct avctp_ctrl_t avctp_ctrl[AVCTP_DEVICE_NUM] = {0};

//对耳同步数据结构为 TYPE(1 byte) + txdo(1 byte) + data(n byte)
struct custom_a2dp_tws_data_t {
    u8 type;
    u8 tx_do;
    u8 data[0];
};

enum CUSTOM_A2DP_SYNC_TYPE {
    CUSTOM_A2DP_SYNC_AVCTP_STOP     =   1, //AVCTP同步暂停
    CUSTOM_A2DP_SYNC_AVDTP_STOP     =   2, //AVDTP同步暂停
    CUSTOM_A2DP_SYNC_INFO           =   3, //同步对耳信息，用于主从切换时，旧主机发给新主机
};

static void custom_a2dp_sync_in_task(u8 *data, u16 len);
extern void tws_a2dp_sync_play(u8 *bt_addr, bool tx_do_action);

//同步数据给对耳
void custom_a2dp_tws_send(u8 type, u8 *_data, u8 len, u8 tx_do)
{
    u8 data[len + sizeof(struct custom_a2dp_tws_data_t)];
    struct custom_a2dp_tws_data_t *p = (struct custom_a2dp_tws_data_t *)data;

    p->type = type;
    p->tx_do = tx_do;
    if(len) {
        memcpy(p->data, _data, len);
    }

    int err = tws_api_send_data_to_sibling(data, sizeof(data), 0x23072609);
    if (err) {
        custom_a2dp_sync_in_task(data, sizeof(data));
    }
}

//收到AVCTP暂停指令超时处理函数
static void avctp_delay_stop_timeout(void *priv)
{
    struct avctp_ctrl_t *p = (struct avctp_ctrl_t *)priv;
    u8 *other_dev_addr = btstack_get_other_dev_addr(p->mac);

    printf("%s, timer:%d", __func__, p->timer);
    put_buf(p->mac, 6);

    if(!other_dev_addr) {
        printf("only_one_device, wait_avdtp_stop");
    } else if(btstack_get_device_a2dp_state(btstack_get_conn_device(other_dev_addr)) != BT_MUSIC_STATUS_STARTING || a2dp_player_is_playing(other_dev_addr)) {
        printf("a2dp_state:%d, a2dp_running:%d", btstack_get_device_a2dp_state(btstack_get_conn_device(other_dev_addr)), a2dp_player_is_playing(other_dev_addr));
    } else {
        custom_a2dp_tws_send(CUSTOM_A2DP_SYNC_AVCTP_STOP, p->mac, 6, 1);
    }

    memset(p, 0, sizeof(*p));
}

//收到手机的AVCTP_STOP后启动定时器
int avctp_delay_deal_start(u8 *mac)
{
    int i = 0;
    u8 empty_mac[6] = {0};

    for (i = 0; i < AVCTP_DEVICE_NUM; i++) {
        if (!memcmp(mac, avctp_ctrl[i].mac, 6)) { //如果定时器存在，修改时间
            avctp_ctrl[i].target_time = jiffies_msec() + AVCTP_CTRL_STOP_TIME;
            sys_timer_modify(avctp_ctrl[i].timer, AVCTP_CTRL_STOP_TIME);
            return 1;
        }
    }

    for (i = 0; i < AVCTP_DEVICE_NUM; i++) {
        if (!memcmp(empty_mac, avctp_ctrl[i].mac, 6)) { //新建AVCTP超时处理定时器
            memcpy(avctp_ctrl[i].mac, mac, 6);
            printf("%s, start", __func__);
            avctp_ctrl[i].target_time = jiffies_msec() + AVCTP_CTRL_STOP_TIME;
            avctp_ctrl[i].timer = sys_timeout_add((void *)&avctp_ctrl[i], avctp_delay_stop_timeout, AVCTP_CTRL_STOP_TIME);
            return 1;
        }
    }
    return 0;
}

//定时器期间收到手机的AVCTP_START，则去掉定时器
int avctp_delay_deal_stop(u8 *mac)
{
    for (int i = 0; i < AVCTP_DEVICE_NUM; i++) {
        if (!memcmp(mac, avctp_ctrl[i].mac, 6)) {
            printf("%s, cancle", __func__);
            sys_timeout_del(avctp_ctrl[i].timer);
            memset(&avctp_ctrl[i], 0, sizeof(avctp_ctrl[i]));
            return 1;
        }
    }
    return 0;
}

//同步定时器数据给对耳。主从切换后新的主机启动定时器
static void avctp_ctrl_send_2_remote()
{
    u8 num = 0;
    struct avctp_ctrl_tws_t tws_info[AVCTP_DEVICE_NUM] = {0};

    printf("%s, send_to_master", __func__);

    for (u8 i = 0; i < AVCTP_DEVICE_NUM; i++) {
        if (avctp_ctrl[i].timer) {
            sys_timeout_del(avctp_ctrl[i].timer);

            memcpy(tws_info[num].mac, avctp_ctrl[i].mac, 6);
            tws_info[num].remain_time = avctp_ctrl[i].target_time - jiffies_msec();
            printf("remain_time:%lu", tws_info[num].remain_time);

            put_buf(tws_info[num].mac, 6);
            num++;
        }
    }

    if(num == 0) {
        return;
    }

    memset(avctp_ctrl, 0, sizeof(avctp_ctrl));
    custom_a2dp_tws_send(CUSTOM_A2DP_SYNC_INFO, (u8 *)tws_info, sizeof(struct avctp_ctrl_tws_t) * num, 0);
}

//收到对耳同步过来的定时器数据
static void avctp_ctrl_recv_from_remote(u8 *data, u16 len)
{
    struct avctp_ctrl_tws_t tws_info[AVCTP_DEVICE_NUM] = {0};
    printf("%s, len:%d", __func__, len);
    put_buf(data, len);

    memcpy(tws_info, data, len);

    for (u8 i = 0; i < AVCTP_DEVICE_NUM; i++) {
        if (tws_info[i].remain_time) {
            memcpy(avctp_ctrl[i].mac, tws_info[i].mac, 6);
            avctp_ctrl[i].target_time = jiffies_msec() + tws_info[i].remain_time;
            avctp_ctrl[i].timer = sys_timeout_add((void *)&avctp_ctrl[i], avctp_delay_stop_timeout, AVCTP_CTRL_STOP_TIME);
        }
        printf("timer:%d, remain_time:%lu, target_time:%lu", avctp_ctrl[i].timer, tws_info[i].remain_time, avctp_ctrl[i].target_time);
        put_buf(tws_info[i].mac, 6);
    }
}

static int custom_a2dp_bt_status_event_handler(int *event)
{
    struct bt_event *bt = (struct bt_event *)event;

    if(tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }

    switch (bt->event) {
    case BT_STATUS_A2DP_MEDIA_START:
        y_printf("BT_STATUS_A2DP_MEDIA_START custom");    
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        y_printf("BT_STATUS_A2DP_MEDIA_STOP custom");
        //需要将tws_a2dp_play.c里面的处理注释掉
        avctp_delay_deal_stop(bt->args); //去掉定时器
        custom_a2dp_tws_send(CUSTOM_A2DP_SYNC_AVDTP_STOP, bt->args, 6, 1); //对耳同步停止解码
        break;
    case BT_STATUS_AVRCP_INCOME_OPID:
        printf("BT_STATUS_AVRCP_INCOME_OPID: 0x%x", bt->value);
        
        if (bt->value == AVC_PLAY) {
            avctp_delay_deal_stop(bt->args);
        } else if (bt->value == AVC_STOP || bt->value == AVC_PAUSE) {
            avctp_delay_deal_start(bt->args);
        }
        break;
    }
    return 0;
}
/*APP_MSG_PROB_HANDLER(custom_a2dp_stack_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = custom_a2dp_bt_status_event_handler,
};*/

APP_MSG_HANDLER(custom_a2dp_stack_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = custom_a2dp_bt_status_event_handler,
};

static int custom_a2dp_tws_msg_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    u8 role = evt->args[0];
    u8 *bt_addr = evt->args + 3;
    int ret = 0;

    switch (evt->event) {
    case TWS_EVENT_MONITOR_START:
        y_printf("TWS_EVENT_MONITOR_START");
        tws_monitor_start = 1;
        //需要将tws_a2dp_play.c里面的TWS_EVENT_MONITOR_START处理注释掉
        if (role == TWS_ROLE_SLAVE) {
            break;
        }
        if (a2dp_player_is_playing(bt_addr)) { //监听上之后，主机发指令让从机开始解码
            tws_a2dp_sync_play(bt_addr, 0);
        }
        //ret = 1; //返回1不跑标准SDK流程
        break;
    case TWS_EVENT_ROLE_SWITCH:
        y_printf("TWS_EVENT_ROLE_SWITCH");
        if(role == TWS_ROLE_SLAVE) {
            avctp_ctrl_send_2_remote(); //旧主机将定时器信息同步给新主机
        }
        break;
    }
    return ret;
}
/*APP_MSG_PROB_HANDLER(custom_a2dp_tws_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = custom_a2dp_tws_msg_handler,
};*/

APP_MSG_HANDLER(custom_a2dp_tws_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = custom_a2dp_tws_msg_handler,
};

static void custom_a2dp_sync_in_task(u8 *data, u16 len)
{
    struct custom_a2dp_tws_data_t *p = (struct custom_a2dp_tws_data_t *)data;
    u8 *other_dev_addr = btstack_get_other_dev_addr(p->data);

    printf("type:%d, len:%d", p->type, len);
    put_buf(p->data, len - sizeof(struct custom_a2dp_tws_data_t));
    if(other_dev_addr) {
        printf("other_device_addr:");
        put_buf(other_dev_addr, 6);
    }

    switch (p->type) {
    case CUSTOM_A2DP_SYNC_AVDTP_STOP:
        y_printf("CUSTOM_A2DP_SYNC_AVDTP_STOP");
        a2dp_player_close(p->data);
        a2dp_media_close(p->data);
        y_printf("btstack_get_device_a2dp_state(btstack_get_conn_device(other_dev_addr)): %d", btstack_get_device_a2dp_state(btstack_get_conn_device(other_dev_addr)));
        y_printf("a2dp_player_is_playing(other_dev_addr): %d", a2dp_player_is_playing(other_dev_addr));
        // if(other_dev_addr && btstack_get_device_a2dp_state(btstack_get_conn_device(other_dev_addr)) == BT_MUSIC_STATUS_STARTING && !a2dp_player_is_playing(other_dev_addr)) { //另一个设备在播歌且未开解码。关闭音频流，等协议栈重新发送MEDIA_START
        if(other_dev_addr && (btstack_get_device_a2dp_state(btstack_get_conn_device(other_dev_addr)) == BT_MUSIC_STATUS_STARTING || btstack_get_device_a2dp_state(btstack_get_conn_device(other_dev_addr)) == BT_MUSIC_STATUS_SUSPENDING) && !a2dp_player_is_playing(other_dev_addr)) { //另一个设备在播歌且未开解码。关闭音频流，等协议栈重新发送MEDIA_START
            y_printf("a2dp_media_close(other_dev_addr)");
            a2dp_media_close(other_dev_addr);
        }
        break;
    case CUSTOM_A2DP_SYNC_AVCTP_STOP:
        y_printf("CUSTOM_A2DP_SYNC_AVCTP_STOP");
        a2dp_player_close(p->data);
        if(other_dev_addr) {
            y_printf("a2dp_media_close(other_dev_addr)");
            a2dp_media_close(other_dev_addr);
        }
        break;
    case CUSTOM_A2DP_SYNC_INFO:
        avctp_ctrl_recv_from_remote(p->data, len - sizeof(struct custom_a2dp_tws_data_t));
        break;
    }
}

static void __custom_a2dp_sync_in_task(u8 *data, u16 len)
{
    custom_a2dp_sync_in_task(data, len);
    free(data);
}

static void custom_a2dp_sync_in_irq(void *_data, u16 len, bool rx)
{
    struct custom_a2dp_tws_data_t *p = (struct custom_a2dp_tws_data_t *)_data;
    if(!p->tx_do && !rx) {
        return;
    }

    u8 *buf = malloc(len);
    if (!buf) {
        return;
    }
    memcpy(buf, _data, len);

    int msg[] = {(int)__custom_a2dp_sync_in_task, 2, (int)buf, len};

    os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
}

REGISTER_TWS_FUNC_STUB(csutom_a2dp_sync_stub) = {
    .func_id = 0x23072609,
    .func = custom_a2dp_sync_in_irq,
};

#endif

