#include "jlstream.h"
#include "esco_player.h"
#include "sdk_config.h"
#include "app_config.h"

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#include "icsd_adt_app.h"
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

struct esco_player {
    u8 bt_addr[6];
    struct jlstream *stream;
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    u8 icsd_adt_state;
#endif
};

static struct esco_player *g_esco_player = NULL;


static void esco_player_callback(void *private_data, int event)
{
    struct esco_player *player = g_esco_player;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

int esco_player_open(u8 *bt_addr)
{
    int err;
    struct esco_player *player;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"esco");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ESCO_RX);
    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

#if TCFG_ESCO_DL_CVSD_SR_USE_16K
    err = jlstream_node_ioctl(player->stream, NODE_UUID_BT_AUDIO_SYNC, NODE_IOC_SET_PRIV_FMT, 1);
#endif /*TCFG_ESCO_DL_CVSD_SR_USE_16K*/

    jlstream_set_callback(player->stream, player->stream, esco_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_ESCO);
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_BTADDR, (int)bt_addr);
    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    memcpy(player->bt_addr, bt_addr, 6);
    g_esco_player = player;

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    /*通话前关闭adt*/
    player->icsd_adt_state = audio_icsd_adt_is_running();
    if (player->icsd_adt_state) {
        audio_icsd_adt_close();
    }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

bool esco_player_runing()
{
    return g_esco_player != NULL;
}

int esco_player_get_btaddr(u8 *btaddr)
{
    if (g_esco_player) {
        memcpy(btaddr, g_esco_player->bt_addr, 6);
        return 1;
    }
    return 0;
}
int esco_player_suspend(u8 *bt_addr)
{
    if (g_esco_player && (memcmp(g_esco_player->bt_addr, bt_addr, 6) == 0)) {
        jlstream_ioctl(g_esco_player->stream, NODE_IOC_SUSPEND, 0);
    }
    return 0;
}
int esco_player_start(u8 *bt_addr)
{
    if (g_esco_player && (memcmp(g_esco_player->bt_addr, bt_addr, 6) == 0)) {
        jlstream_ioctl(g_esco_player->stream, NODE_IOC_START, 0);
    }
    return 0;
}

/*
	ESCO状态检测，
	param:btaddr 目标蓝牙地址, 传NULL则只检查耳机状态
 */
int esco_player_is_playing(u8 *btaddr)
{
    int cur_addr = 1;
    if (btaddr && g_esco_player) {	//当前蓝牙地址检测
        cur_addr = (!memcmp(btaddr, g_esco_player->bt_addr, 6));
    }
    if (g_esco_player && cur_addr) {
        return true;
    }
    return false;
}

void esco_player_close()
{
    struct esco_player *player = g_esco_player;

    if (!player) {
        return;
    }
    jlstream_stop(player->stream, 0);
    jlstream_release(player->stream);

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    if (player->icsd_adt_state) {
        audio_icsd_adt_open();
    }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
    free(player);
    g_esco_player = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"esco");
}


