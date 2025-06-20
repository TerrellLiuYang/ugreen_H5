#include "jlstream.h"
#include "classic/tws_api.h"
#include "media/audio_base.h"
#include "a2dp_player.h"
#include "btstack/a2dp_media_codec.h"
#include "media/bt_audio_timestamp.h"
#include "effects/audio_pitchspeed.h"
#include "sdk_config.h"

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#if (defined TCFG_AUDIO_SPEAK_TO_CHAT_ENABLE) && TCFG_AUDIO_SPEAK_TO_CHAT_ENABLE
#include "icsd_adt_app.h"
#endif

struct a2dp_player {
    u8 bt_addr[6];
    u16 retry_timer;
    s8 a2dp_pitch_mode;
    struct jlstream *stream;
};

static struct a2dp_player *g_a2dp_player = NULL;
extern const int CONFIG_BTCTLER_TWS_ENABLE;


static void a2dp_player_callback(void *private_data, int event)
{
    struct a2dp_player *player = g_a2dp_player;

    printf("a2dp_callback: %d\n", event);
}

static void a2dp_player_set_audio_channel(struct a2dp_player *player)
{
    int channel = AUDIO_CH_MIX;
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        channel = tws_api_get_local_channel() == 'L' ? AUDIO_CH_L : AUDIO_CH_R;
    }

    jlstream_ioctl(player->stream, NODE_IOC_SET_CHANNEL, channel);
}

void a2dp_player_tws_event_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    u8 state = evt->args[1];

    switch (evt->event) {
    case TWS_EVENT_MONITOR_START:
        if (!(state & TWS_STA_SBC_OPEN)) {
            break;
        }
    case TWS_EVENT_CONNECTION_DETACH:
    case TWS_EVENT_REMOVE_PAIRS:
        if (g_a2dp_player) {
            a2dp_player_set_audio_channel(g_a2dp_player);
        }
        break;
    default:
        break;
    }
    a2dp_tws_timestamp_event_handler(evt->event, evt->args);
}

static int a2dp_player_create(u8 *btaddr)
{
    int err;
    int uuid;
    struct a2dp_player *player = g_a2dp_player;

    uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"a2dp");

    if (player) {
        if (player->stream) {
            if (!memcmp(player->bt_addr, btaddr, 6))  {
                return -EEXIST;
            }
            puts("a2dp_player_busy\n");
            return -EBUSY;
        }
        if (player->retry_timer) {
            sys_timer_del(player->retry_timer);
            player->retry_timer = 0;
        }
    } else {
        player = zalloc(sizeof(*player));
        if (!player) {
            return -ENOMEM;
        }
        g_a2dp_player = player;
    }

    memcpy(player->bt_addr, btaddr, 6);

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_A2DP_RX);
    if (!player->stream) {
        printf("create a2dp stream faild\n");
        return -EFAULT;
    }

    return 0;
}

void a2dp_player_low_latency_enable(u8 enable)
{
    a2dp_file_low_latency_enable(enable);
}

static void retry_open_a2dp_player(void *p)
{
    printf("retry_open_a2dp_player");
    if (g_a2dp_player && !g_a2dp_player->stream) {
        a2dp_player_open(g_a2dp_player->bt_addr);
    }
}

static void retry_start_a2dp_player(void *p)
{
	printf("retry_start_a2dp_player");
    if (g_a2dp_player && g_a2dp_player->stream) {
        int err = jlstream_start(g_a2dp_player->stream);
        if (err == 0) {
            sys_timer_del(g_a2dp_player->retry_timer);
            g_a2dp_player->retry_timer = 0;
        }
    }
}

int a2dp_player_open(u8 *btaddr)
{
    int err;

#if (defined TCFG_AUDIO_SPEAK_TO_CHAT_ENABLE) && TCFG_AUDIO_SPEAK_TO_CHAT_ENABLE
    if (get_speak_to_chat_state() == AUDIO_ADT_CHAT) {
        audio_speak_to_char_sync_suspend();
    }
#endif
    err = a2dp_player_create(btaddr);
    if (err) {
        if (err == -EFAULT) {
            g_a2dp_player->retry_timer = sys_timer_add(NULL, retry_open_a2dp_player, 200);
        }
        return err;
    }
    struct a2dp_player *player =  g_a2dp_player;

    player->a2dp_pitch_mode = PITCH_0; //默认打开是原声调

    jlstream_set_callback(player->stream, NULL, a2dp_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_A2DP);

    if (CONFIG_BTCTLER_TWS_ENABLE) {
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            int channel = tws_api_get_local_channel() == 'L' ? AUDIO_CH_L : AUDIO_CH_R;
            jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                                NODE_IOC_SET_CHANNEL, channel);
        }
    }
    err = jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE,
                              NODE_IOC_SET_BTADDR, (int)player->bt_addr);
    if (err == 0) {
        err = jlstream_start(player->stream);
        if (err) {
            g_a2dp_player->retry_timer = sys_timer_add(NULL, retry_start_a2dp_player, 200);
            return 0;
        }
    }
    if (err) {
        jlstream_release(player->stream);
        free(player);
        g_a2dp_player = NULL;
        return err;
    }

    puts("a2dp_open_dec_file_suss\n");

    return 0;
}

int a2dp_player_runing()
{
    return g_a2dp_player ? 1 : 0;
}

int a2dp_player_get_btaddr(u8 *btaddr)
{
    if (g_a2dp_player) {
        memcpy(btaddr, g_a2dp_player->bt_addr, 6);
        return 1;
    }
    return 0;
}

bool a2dp_player_is_playing(u8 *bt_addr)
{
    if (g_a2dp_player && memcmp(bt_addr, g_a2dp_player->bt_addr, 6) == 0) {
        return 1;
    }
    return 0;
}

int a2dp_player_start_slience_detect(u8 *btaddr, void (*handler)(u8 *, bool), int msec)
{
    if (!g_a2dp_player || memcmp(g_a2dp_player->bt_addr, btaddr, 6))  {
        return -EINVAL;
    }
    return 0;
}

void a2dp_player_close(u8 *btaddr)
{
    struct a2dp_player *player = g_a2dp_player;

    if (!player) {
        return;
    }
    if (memcmp(player->bt_addr, btaddr, 6)) {
        return;
    }
    if (player->retry_timer) {
        sys_timer_del(player->retry_timer);
        player->retry_timer = 0;
    }
    if (player->stream) {
        jlstream_stop(player->stream, 0);
        jlstream_release(player->stream);
    }
    free(player);
    g_a2dp_player = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"a2dp");
}

//复位当前的数据流
void a2dp_player_reset(void)
{
    u8 bt_addr[6];
    if (g_a2dp_player) {
        memcpy(bt_addr, g_a2dp_player->bt_addr, 6);
        a2dp_player_close(bt_addr);
        a2dp_player_open(bt_addr);
    }
}

//变调接口
int a2dp_file_pitch_up()
{
    struct a2dp_player *player = g_a2dp_player;
    if (!player) {
        return -1;
    }
    float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    player->a2dp_pitch_mode++;
    if (player->a2dp_pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        player->a2dp_pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    printf("play pitch up+++%d\n", player->a2dp_pitch_mode);
    int ret = a2dp_file_set_pitch(player->a2dp_pitch_mode);
    ret = (ret == true) ? player->a2dp_pitch_mode : -1;
    return ret;
}

int a2dp_file_pitch_down()
{
    struct a2dp_player *player = g_a2dp_player;
    if (!player) {
        return -1;
    }
    player->a2dp_pitch_mode--;
    if (player->a2dp_pitch_mode < 0) {
        player->a2dp_pitch_mode = 0;
    }
    printf("play pitch down---%d\n", player->a2dp_pitch_mode);
    int ret = a2dp_file_set_pitch(player->a2dp_pitch_mode);
    ret = (ret == true) ? player->a2dp_pitch_mode : -1;
    return ret;
}

int a2dp_file_set_pitch(enum _pitch_level pitch_mode)
{
    struct a2dp_player *player = g_a2dp_player;
    float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    if (pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    pitch_speed_param_tool_set pitch_param = {
        .pitch = pitch_param_table[pitch_mode],
        .speed = 1,
    };
    if (player) {
        player->a2dp_pitch_mode = pitch_mode;
        return jlstream_node_ioctl(player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&pitch_param);
    }
    return -1;
}

void a2dp_file_pitch_mode_init(enum _pitch_level pitch_mode)
{
    struct a2dp_player *player = g_a2dp_player;
    if (player) {
        player->a2dp_pitch_mode = pitch_mode;
    }
}
