#include "jlstream.h"
#include "classic/tws_api.h"
#include "media/audio_base.h"
#include "app_main.h"
#include "audio_config.h"
#include "audio_dvol.h"
#include "effects/effects_adj.h"
#include "volume_node.h"
#include "scene_switch.h"
#include "app_config.h"

struct volume_hdl {
    enum stream_scene scene;
    dvol_handle *dvol_hdl; //音量操作句柄
    u8 state;
    struct stream_node *node;	//节点句柄
    char name[16]; //节点名称
    u8 bypass;//是否跳过节点处理
    struct volume_cfg *vol_cfg;
    u16 cfg_len;//配置数据实际大小
    u8 online_update_disable;//是否支持在线音量更新
};

//默认音量配置
struct volume_cfg default_vol_cfg = {
    .bypass = 0,
    .cfg_level_max = 16,
    .cfg_vol_min = -45,
    .cfg_vol_max = 0,
    .vol_table_custom = 0,
    .cur_vol = 11,
};

__STREAM_CACHE_CODE
static void volume_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *in_frame, *out_frame;
    int wlen;

    struct stream_frame *frame;
    struct stream_node *node = iport->node;
    struct volume_hdl *hdl = (struct volume_hdl *)iport->private_data;

    while (1) {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            break;
        }
        if (!hdl->bypass) {
            wlen = audio_digital_vol_run(hdl->dvol_hdl, (s16 *)frame->data, frame->len);
        }
        if (node->oport) {
            jlstream_push_frame(node->oport, frame);
        } else {
            jlstream_free_frame(frame);
        }
    }
}

static int volume_bind(struct stream_node *node, u16 uuid)
{
    struct volume_hdl *hdl = zalloc(sizeof(*hdl));

    node->private_data = hdl;

    hdl->node = node;
    return 0;
}

static void volume_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = volume_handle_frame;
    iport->private_data = iport->node->private_data;
}

/*节点参数协商*/
static int volume_ioc_negotiate(struct stream_iport *iport)
{
    return 0;
}

void audio_digital_vol_cfg_init(dvol_handle *dvol, struct volume_cfg *vol_cfg) //初始化配置
{
    if (dvol) {
        dvol-> cfg_vol_max = vol_cfg->cfg_vol_max;
        dvol-> cfg_vol_min = vol_cfg->cfg_vol_min;
        dvol-> cfg_level_max = vol_cfg->cfg_level_max;
        dvol-> vol_table_custom = vol_cfg->vol_table_custom;
#if VOL_TAB_CUSTOM_EN
        if (dvol->vol_table_custom == VOLUME_TABLE_CUSTOM_EN) {
            dvol->vol_table = vol_cfg->vol_table ;
        }
#endif
        dvol->vol_limit = (dvol->vol_limit > dvol->cfg_level_max) ? dvol->cfg_level_max : dvol->vol_limit;
        u16 vol_level = dvol->vol * dvol->vol_limit / dvol->vol_max;
        if (vol_level == 0) {
            dvol->vol_target = 0;
        } else if (dvol->vol_table) {
            dvol->vol_target = dvol->vol_table[vol_level - 1];
        } else {
            extern float eq_db2mag(float x);
            u16 step = (dvol->cfg_level_max - 1 > 0) ? (dvol->cfg_level_max - 1) : 1;
            float step_db = (dvol->cfg_vol_max - dvol->cfg_vol_min) / (float)step;
            float dvol_db = dvol->cfg_vol_min  + (vol_level - 1) * step_db;
            float dvol_gain = eq_db2mag(dvol_db);//dB转换倍数
            dvol->vol_target = (s16)(16384.0  * dvol_gain + 0.5);
            dvol->vol_fade = dvol->vol_target;
            printf("vol param:%d,(%d/100)dB,cur:%d,max:%d,(%d/100)dB", dvol->vol_target, (int)step_db * 100, vol_level, vol_cfg->cfg_level_max, (int)dvol_db * 100);
#if 0
//打印音量表每一级的目标音量值和dB值,调试用
            printf("=========================================vol table=================================================================");
            int i = 1;
            for (i = 1; i < dvol->cfg_level_max + 1; i++) {
                float debug_db = dvol->cfg_vol_min  + (i - 1) * step_db;
                float debug_gain = eq_db2mag(debug_db);//dB转换倍数
                int debug_target = (s16)(16384.0  * debug_gain + 0.5);
                printf("dvol[%d] = %d,(%d / 100)dB", i, debug_target, (int)(debug_db * 100));
            }
            printf("====================================================================================================================");
#endif
        }

        //if ((dvol->cfg_vol_min == -45) && (dvol->cfg_level_max == 31) && (dvol->cfg_vol_max == 0)) {
            dvol-> vol_table_default = 1; //使用默认的音量表
        //}
    }
}

static void volume_ioc_start(struct volume_hdl *hdl)
{
    /*
     *获取配置文件内的参数,及名字
     * */
    int len = 0;
    struct volume_cfg *vol_cfg = NULL;
    if (!hdl->vol_cfg) {
        struct cfg_info info  = {0};             //节点配置相关信息（参数存储的目标地址、配置项大小）
        int ret = jlstream_read_node_info_data(hdl->node->uuid, hdl->node->subid, hdl->name, &info);
        /* printf("enter volume_node.c %d,%d,%d\n", __LINE__, ret, info.size); */
        if (ret) {
            hdl->cfg_len = info.size;
            vol_cfg = hdl->vol_cfg =  zalloc(info.size);
            len = jlstream_read_form_cfg_data(&info, (void *)vol_cfg);
            if (info.size > sizeof(struct volume_cfg)) { //有自定义音量表,dB转成对应音量
                for (int i = 0; i < vol_cfg->cfg_level_max ; i++) {
                    /* printf("custom dvol [%d] = %d / 100 dB", i, (int)(hdl->vol_cfg->vol_table[i] * 100)); */
                    float dvol_gain = eq_db2mag(hdl->vol_cfg->vol_table[i]);//dB转换倍数
                    hdl->vol_cfg->vol_table [i]  = (s16)(16384.0  * dvol_gain + 0.5);
                    /* printf("custom dvol[%d] = %d", i, (int)hdl->vol_cfg->vol_table[i]); */
                }
            }
        }
    } else {
        len = hdl->cfg_len;
        vol_cfg = hdl->vol_cfg;
    }
    if (!len) {
        printf("%s, read node data err\n", __FUNCTION__);
    }

    printf("%s len %d, sizeof(cfg) %lu\n", __func__,  len, sizeof(struct volume_cfg));
    if (len != hdl->cfg_len) {
        if (hdl->vol_cfg) {
            free(hdl->vol_cfg);
            hdl->vol_cfg = NULL;
        }
        vol_cfg = &default_vol_cfg;
    } else {
        printf("vol read config ok :%d,%d,%d,%d,%d\n", vol_cfg->cfg_level_max, (int)vol_cfg->cfg_vol_min, (int)vol_cfg->cfg_vol_max, vol_cfg->vol_table_custom, vol_cfg->bypass);
    }

    /*
       *获取在线调试的临时参数
       * */
    u32 online_cfg_len = sizeof(struct volume_cfg) + DIGITAL_VOLUME_LEVEL_MAX * sizeof(float);
    struct volume_cfg *online_vol_cfg = zalloc(online_cfg_len);
    if (jlstream_read_effects_online_param(hdl->node->uuid, hdl->name, online_vol_cfg, online_cfg_len)) {
        /* printf("cfg_level_max = %d\n", online_vol_cfg->cfg_level_max); */
        /* printf("cfg_vol_min = %d\n", online_vol_cfg->cfg_vol_min); */
        /* printf("cfg_vol_max = %d\n", online_vol_cfg->cfg_vol_max); */
        /* printf("vol_table_custom = %d\n", online_vol_cfg->vol_table_custom); */
        /* printf("cur_vol = %d\n", online_vol_cfg->cur_vol); */
        /* printf("tab_len = %d\n", online_vol_cfg->tab_len); */
        hdl->vol_cfg-> cfg_vol_max =  online_vol_cfg->cfg_vol_max;
        hdl->vol_cfg-> cfg_vol_min = online_vol_cfg->cfg_vol_min;
        hdl->vol_cfg-> bypass = online_vol_cfg->bypass;
#if VOL_TAB_CUSTOM_EN
        if (hdl->vol_cfg->tab_len == online_vol_cfg->tab_len && hdl->vol_cfg->tab_len) {
            for (int i = 0; i < hdl->vol_cfg->cfg_level_max ; i++) {//重新计算音量表的值
                printf("custom dvol [%d] = %d / 100 dB", i, (int)(online_vol_cfg->vol_table[i] * 100));
                float dvol_gain = eq_db2mag(online_vol_cfg->vol_table[i]);//dB转换倍数
                hdl->vol_cfg->vol_table [i]  = (s16)(16384.0  * dvol_gain + 0.5);
                printf("custom dvol[%d] = %d", i, (int)online_vol_cfg->vol_table[i]);
            }
        }
#endif
        printf("get volume online param\n");
    }
    free(online_vol_cfg);
    hdl->bypass = vol_cfg->bypass;
    switch (hdl->scene) {
    case STREAM_SCENE_TONE:
    case STREAM_SCENE_RING:
    case STREAM_SCENE_KEY_TONE:
        /*puts("set_tone_volume\n");*/
        hdl->state = APP_AUDIO_STATE_WTONE;
#if TONE_BGM_FADEOUT
        /*printf("tone_player.c %d tone play,BGM fade_out automatically\n", __LINE__);*/
        audio_digital_vol_bg_fade(1);
#endif/*TONE_BGM_FADEOUT*/
        if (!hdl->dvol_hdl) {
            hdl->dvol_hdl =  audio_digital_vol_open(app_audio_get_volume(APP_AUDIO_STATE_WTONE), vol_cfg->cfg_level_max, TONE_DVOL_FS, -1);
        }
        audio_digital_vol_cfg_init(hdl->dvol_hdl, vol_cfg);
        break;
    case STREAM_SCENE_A2DP:
    case STREAM_SCENE_LINEIN:
    case STREAM_SCENE_SPDIF:
    case STREAM_SCENE_PC_SPK:
    case STREAM_SCENE_PC_MIC:
    case STREAM_SCENE_MUSIC:
    case STREAM_SCENE_FM:
    case STREAM_SCENE_MIC_EFFECT:
        puts("set_a2dp_volume\n");
        hdl->state = APP_AUDIO_STATE_MUSIC;
        if (!hdl->dvol_hdl) {
            hdl->dvol_hdl = audio_digital_vol_open(app_audio_get_volume(APP_AUDIO_STATE_MUSIC), vol_cfg->cfg_level_max, MUSIC_DVOL_FS, -1);
        }
        audio_digital_vol_cfg_init(hdl->dvol_hdl, vol_cfg);
        break;
    case STREAM_SCENE_ESCO:
        puts("set_esco_volume\n");
        hdl->state = APP_AUDIO_STATE_CALL;
        if (!hdl->dvol_hdl) {
            hdl->dvol_hdl = audio_digital_vol_open(app_audio_get_volume(APP_AUDIO_STATE_CALL), vol_cfg->cfg_level_max, CALL_DVOL_FS, -1);
        }
        audio_digital_vol_cfg_init(hdl->dvol_hdl, vol_cfg);
        break;
    default:
        break;
    }
    if (hdl->dvol_hdl) {
        hdl->dvol_hdl->mute_en = app_audio_get_mute_state(hdl->state);
        if (hdl->dvol_hdl->mute_en) {
            hdl->dvol_hdl->vol_fade = 0;
        }
    }
    if (hdl->scene == STREAM_SCENE_MIC_EFFECT) {
        return;
    }
#ifdef DVOL_2P1_CH_DVOL_ADJUST_NODE
#if (DVOL_2P1_CH_DVOL_ADJUST_NODE == DVOL_2P1_CH_DVOL_ADJUST_LR)
    char *substr = strstr(hdl->name, "Music");
    if (!strcmp(substr, "Music") || !strcmp(substr, "Music2")) { //找到默认初始化为最大音量的节点
        //printf("enter volume_node.c %d,%p,%s\n", __LINE__, hdl->dvol_hdl, substr);
        audio_digital_vol_set(hdl->dvol_hdl, hdl->dvol_hdl->vol_max);
        hdl->dvol_hdl->mute_en = 0;
        hdl->online_update_disable = 1;
    } else {
        //printf("enter volume_node.c %d,%p,%s\n", __LINE__, hdl->dvol_hdl, substr);
        app_audio_state_switch(hdl->state, vol_cfg->cfg_level_max, hdl->dvol_hdl);
    }
#elif (DVOL_2P1_CH_DVOL_ADJUST_NODE == DVOL_2P1_CH_DVOL_ADJUST_SW)
    char *substr = strstr(hdl->name, "Music");
    if (!strcmp(substr, "Music") || !strcmp(substr, "Music1")) { //找到默认初始化为最大音量的节点
        // printf("enter volume_node.c %d,%p,%s\n", __LINE__, hdl->dvol_hdl, substr);
        audio_digital_vol_set(hdl->dvol_hdl, hdl->dvol_hdl->vol_max);
        hdl->dvol_hdl->mute_en = 0;
        hdl->online_update_disable = 1;
    } else {
        // printf("enter volume_node.c %d,%p,%s\n", __LINE__, hdl->dvol_hdl, substr);
        app_audio_state_switch(hdl->state, vol_cfg->cfg_level_max, hdl->dvol_hdl);
    }
#else
    if (memchr(hdl->name, '1', 16) || memchr(hdl->name, '2', 16)) { //找到默认初始化为最大音量的节点
        //printf("enter volume_node.c %d,%p\n", __LINE__, hdl->dvol_hdl);
        audio_digital_vol_set(hdl->dvol_hdl, hdl->dvol_hdl->vol_max);
        hdl->online_update_disable = 1;
        hdl->dvol_hdl->mute_en = 0;
    } else {
        //printf("enter volume_node.c %d,%p\n", __LINE__, hdl->dvol_hdl);
        app_audio_state_switch(hdl->state, vol_cfg->cfg_level_max, hdl->dvol_hdl);
    }
#endif
#else
    app_audio_state_switch(hdl->state, vol_cfg->cfg_level_max, hdl->dvol_hdl);
#endif
}

static void volume_ioc_stop(struct volume_hdl *hdl)
{
    if (hdl->scene != STREAM_SCENE_MIC_EFFECT) {
        app_audio_state_exit(hdl->state);
    }
#if TONE_BGM_FADEOUT
    printf("tone_player.c %d tone play,BGM fade_out close\n", __LINE__);
    audio_digital_vol_bg_fade(0);
#endif/*TONE_BGM_FADEOUT*/
    audio_digital_vol_close(hdl->dvol_hdl);
    hdl->dvol_hdl = NULL;
}

int volume_ioc_get_cfg(const char *name, struct volume_cfg *vol_cfg)
{
    char mode_index = get_current_scene();
    struct cfg_info info  = {0};             //节点配置相关信息（参数存储的目标地址、配置项大小）
    int ret = jlstream_read_info_data(mode_index, name, 0, &info);
    if (!ret) {
        *vol_cfg = default_vol_cfg;
    } else {
        struct volume_cfg *temp_vol_cfg =  zalloc(info.size);
        int len = jlstream_read_form_cfg_data(&info, (void *)temp_vol_cfg);
        if (len == info.size) {
            *vol_cfg = *temp_vol_cfg;
        } else {
            *vol_cfg = default_vol_cfg;
        }
        free(temp_vol_cfg); //赋值完结构体释放内存
        /* printf("volume node read cfg name %s", name); */
        /* printf("cfg_level_max = %d\n", vol_cfg->cfg_level_max); */
        /* printf("cfg_vol_min = %d\n", vol_cfg->cfg_vol_min); */
        /* printf("cfg_vol_max = %d\n", vol_cfg->cfg_vol_max); */
        /* printf("vol_table_custom = %d\n", vol_cfg->vol_table_custom); */
        /* printf("cur_vol = %d\n", vol_cfg->cur_vol); */
    }
    return ret;
}

u16 volume_ioc_get_max_level(const char *name)
{
    struct volume_cfg vol_cfg;
    volume_ioc_get_cfg(name, &vol_cfg);
    return vol_cfg.cfg_level_max;
}


static int volume_ioc_update_parm(struct volume_hdl *hdl, int parm)
{
    int ret = false;
    s32 value = *((s32 *)parm);
    u32 cmd = value  & 0xff000000;
    switch (cmd) {
    case VOLUME_NODE_CMD_SET_VOL:
        s16 volume = value & 0xffff;
        if (volume < 0) {
            volume = 0;
        }
        if (hdl && hdl->dvol_hdl) {
            audio_digital_vol_set(hdl->dvol_hdl, volume);
            printf("volume update success : %d\n", volume);
            ret = true;
        }
        break;
    case VOLUME_NODE_CMD_SET_MUTE:
        s16 mute_en = value & 0xffff;
        if (hdl && hdl->dvol_hdl) {
            audio_digital_vol_mute_set(hdl->dvol_hdl, mute_en);
            printf("mute update success : %d\n", mute_en);
            ret = true;
        }
        break;
    default:
        //音量结构体只有最小音量为-256dB,-255dB时才会走到上面的case,超过了工具上可配置的最小值-100dB
        u16 cfg_value = value & 0xffff;
        if (cfg_value > VOL_CFG_THRESHOLD) {
            struct volume_cfg *vol_cfg = (struct volume_cfg *)parm;
            if (hdl && hdl->dvol_hdl) {
                if (!hdl->online_update_disable) {
                    hdl->dvol_hdl-> cfg_vol_max =  vol_cfg->cfg_vol_max;
                    hdl->dvol_hdl-> cfg_vol_min = vol_cfg->cfg_vol_min;
#if VOL_TAB_CUSTOM_EN
                    if (hdl->vol_cfg->tab_len == vol_cfg->tab_len && hdl->vol_cfg->tab_len) {
                        for (int i = 0; i < hdl->vol_cfg->cfg_level_max ; i++) {//重新计算音量表的值
                            printf("custom dvol [%d] = %d / 100 dB", i, (int)(vol_cfg->vol_table[i] * 100));
                            float dvol_gain = eq_db2mag(vol_cfg->vol_table[i]);//dB转换倍数
                            hdl->vol_cfg->vol_table [i]  = (s16)(16384.0  * dvol_gain + 0.5);
                            printf("custom dvol[%d] = %d", i, (int)vol_cfg->vol_table[i]);
                        }
                        if (hdl->dvol_hdl->vol_table_custom == VOLUME_TABLE_CUSTOM_EN) {
                            hdl->dvol_hdl->vol_table = hdl->vol_cfg->vol_table ;
                        }
                    }
#endif
                    hdl->bypass = vol_cfg->bypass;
                    audio_digital_vol_set(hdl->dvol_hdl, vol_cfg->cur_vol);
                    app_audio_change_volume(app_audio_get_state(), vol_cfg->cur_vol);
                }
                /* printf("cfg_level_max = %d\n", vol_cfg->cfg_level_max); */
                /* printf("cfg_vol_min = %d\n", vol_cfg->cfg_vol_min); */
                /* printf("cfg_vol_max = %d\n", vol_cfg->cfg_vol_max); */
                /* printf("vol_table_custom = %d\n", vol_cfg->vol_table_custom); */
                /* printf("cur_vol = %d\n", vol_cfg->cur_vol); */
                printf("volume update success : %d\n", vol_cfg->cur_vol);
                ret = true;
                return ret;
            }
        }
        printf("parm update failed : %x\n", value);
        break;
    }
    return ret;
}

static int volume_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    struct volume_hdl *hdl = (struct volume_hdl *)iport->private_data;

    int ret = 0;
    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        volume_open_iport(iport);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = (enum stream_scene)arg;
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= volume_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        volume_ioc_start(hdl);
        break;
    case NODE_IOC_STOP:
    case NODE_IOC_SUSPEND:
        volume_ioc_stop(hdl);
        break;
    case NODE_IOC_NAME_MATCH:
        /* printf("volume_node name match :%s,%s\n", hdl->name, (const char *)arg); */
        if (!strcmp((const char *)arg, hdl->name)) {
            ret = 1;
        }
        break;
    case NODE_IOC_SET_PARAM:
        ret = volume_ioc_update_parm(hdl, arg);
        break;
    }

    return ret;
}

static void volume_release(struct stream_node *node)
{
    struct volume_hdl *hdl = (struct volume_hdl *)node->private_data;
    if (hdl->vol_cfg) {
        free(hdl->vol_cfg);
        hdl->vol_cfg = NULL;
    }
    free(hdl);
}


REGISTER_STREAM_NODE_ADAPTER(volume_node_adapter) = {
    .name       = "volume",
    .uuid       = NODE_UUID_VOLUME_CTRLER,
    .bind       = volume_bind,
    .ioctl      = volume_ioctl,
    .release    = volume_release,
};

REGISTER_ONLINE_ADJUST_TARGET(volume) = {
    .uuid = NODE_UUID_VOLUME_CTRLER,
};

