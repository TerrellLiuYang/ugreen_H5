#include "jlstream.h"
#include "overlay_code.h"
#include "media/audio_base.h"
#include "clock_manager/clock_manager.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "app_tone.h"
#include "app_config.h"
#include "earphone.h"
#include "effects/effects_adj.h"
#include "audio_manager.h"

#if TCFG_AUDIO_DUT_ENABLE
#include "test_tools/audio_dut_control.h"
#endif

#define PIPELINE_UUID_TONE_NORMAL   0x7674
#define PIPELINE_UUID_A2DP          0xD96F
#define PIPELINE_UUID_ESCO          0xBA11
#define PIPELINE_UUID_AI_VOICE      0x5475
#define PIPELINE_UUID_A2DP_DUT      0xC9DB
#define PIPELINE_UUID_DEV_FLOW      0x959E


#if TCFG_A2DP_PREEMPTED_ENABLE
const int CONFIG_A2DP_ENERGY_CALC_ENABLE = 0;
#else
const int CONFIG_A2DP_ENERGY_CALC_ENABLE = 1;
#endif

static u8 g_a2dp_slience;
static u32 g_a2dp_slience_begin;

static void a2dp_energy_detect_handler(int *arg)
{
    if (CONFIG_A2DP_ENERGY_CALC_ENABLE == 0)  {
        return;
    }
    int energy = arg[0];
    if (energy == 0) {
        if (g_a2dp_slience_begin == 0) {
            g_a2dp_slience_begin = jiffies_msec();
        } else {
            int msec = jiffies_msec2offset(g_a2dp_slience_begin, jiffies_msec());
            if (msec >= 2000 && g_a2dp_slience == 0) {
                g_a2dp_slience = 1;
                audio_event_to_user(AUDIO_EVENT_A2DP_NO_ENERGY);
            }
        }
    } else {
        g_a2dp_slience = 0;
        g_a2dp_slience_begin = 0;
    }
}

static int get_pipeline_uuid(const char *name)
{
    if (!strcmp(name, "tone")) {
        if (a2dp_player_runing()) {
            return PIPELINE_UUID_A2DP;
        }
        if (esco_player_runing()) {
            return PIPELINE_UUID_ESCO;
        }
        if (ring_player_runing()) {
            return PIPELINE_UUID_ESCO;
        }
        return PIPELINE_UUID_TONE_NORMAL;
    }

    if (!strcmp(name, "ring")) {
        clock_alloc("esco", 48 * 1000000UL);
        return PIPELINE_UUID_ESCO;
    }
    if (!strcmp(name, "esco")) {
        // clock_alloc("esco", 48 * 1000000UL);
		int old_sys_clk = clk_get("sys");
		clock_alloc("sys", 96 * 1000000L - old_sys_clk);//96 0831 jwk
        audio_event_to_user(AUDIO_EVENT_ESCO_START);
        return PIPELINE_UUID_ESCO;
    }

    if (!strcmp(name, "a2dp")) {
        clock_alloc("a2dp", 12 * 1000000UL);
        audio_event_to_user(AUDIO_EVENT_A2DP_START);
#if TCFG_AUDIO_DUT_ENABLE
        if (audio_dec_dut_en_get(0)) {
            return PIPELINE_UUID_A2DP_DUT;
        }
#endif
        return PIPELINE_UUID_A2DP;
    }


    if (!strcmp(name, "ai_voice")) {
        /* clock_alloc("a2dp", 24 * 1000000UL); */
        return PIPELINE_UUID_AI_VOICE;
    }

    if (!strcmp(name, "dev_flow")) {
        return PIPELINE_UUID_DEV_FLOW;
    }

    return 0;
}

static void player_close_handler(const char *name)
{
    if (!strcmp(name, "a2dp")) {
        clock_free("a2dp");
        audio_event_to_user(AUDIO_EVENT_A2DP_STOP);
    } else if (!strcmp(name, "esco")) {
        clock_free("esco");
        audio_event_to_user(AUDIO_EVENT_ESCO_STOP);
    }
}

static int load_decoder_handler(struct stream_decoder_info *info)
{
    if (info->coding_type == AUDIO_CODING_AAC) {
        printf("overlay_lode_code: aac\n");
        audio_overlay_load_code(AUDIO_CODING_AAC);
    }
    if (info->scene == STREAM_SCENE_A2DP) {
        info->task_name = "a2dp_dec";
        g_a2dp_slience = 0;
        g_a2dp_slience_begin = 0;
    } else if (info->scene == STREAM_SCENE_TONE) {
        info->task_name = "tone_dec";
    } else if (info->scene == STREAM_SCENE_RING) {
        info->task_name = "ring_dec";
    } else if (info->scene == STREAM_SCENE_KEY_TONE) {
        info->task_name = "key_tone_dec";
    } else if (info->scene == STREAM_SCENE_DEV_FLOW) {
        info->task_name = "dev_flow";
    }
    return 0;
}

static int load_encoder_handler(struct stream_encoder_info *info)
{
    if (info->scene == STREAM_SCENE_ESCO) {	//AEC PLC共用overlay
        printf("overlay_lode_code: aec\n");
        overlay_load_code(OVERLAY_AEC);
    }
    return 0;
}


/*
 *获取需要指定得默认配置
 * */
static int get_node_parm(int arg)
{
    int ret = 0;
    ret = get_eff_default_param(arg);
    return ret ;
}
/*
*获取ram内在线音效参数
*/
static int get_eff_online_parm(int arg)
{
    int ret = 0;
#if TCFG_CFG_TOOL_ENABLE
    struct eff_parm {
        int uuid;
        char name[16];
        u8 data[0];
    };
    struct eff_parm *parm = (struct eff_parm *)arg;
    /* printf("eff_online_uuid %x, %s\n", parm->uuid, parm->name); */
    ret = get_eff_online_param(parm->uuid, parm->name, (void *)arg);
#endif
    return ret;
}
int jlstream_event_notify(enum stream_event event, int arg)
{
    int ret = 0;

    switch (event) {
    case STREAM_EVENT_LOAD_DECODER:
        ret = load_decoder_handler((struct stream_decoder_info *)arg);
        break;
    case STREAM_EVENT_LOAD_ENCODER:
        ret = load_encoder_handler((struct stream_encoder_info *)arg);
        break;
    case STREAM_EVENT_GET_PIPELINE_UUID:
        ret = get_pipeline_uuid((const char *)arg);
        r_printf("pipeline_uuid: %x\n", ret);
        clock_refurbish();
        break;
    case STREAM_EVENT_CLOSE_PLAYER:
        player_close_handler((const char *)arg);
        break;
    case STREAM_EVENT_INC_SYS_CLOCK:
        clock_refurbish();
        break;
    case STREAM_EVENT_GET_NODE_PARM:
        ret = get_node_parm(arg);
        break;
    case STREAM_EVENT_GET_EFF_ONLINE_PARM:
        ret = get_eff_online_parm(arg);
        break;
    case STREAM_EVENT_A2DP_ENERGY:
        a2dp_energy_detect_handler((int *)arg);
        break;
    default:
        break;
    }

    return ret;
}
