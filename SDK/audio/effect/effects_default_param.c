#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "jlstream.h"
#include "cfg_tool.h"
#include "effects/effects_adj.h"
#include "effects/audio_eq.h"
#include "effects/eq_config.h"
#include "effects/audio_wdrc.h"
#include "effects/audio_pitchspeed.h"
#include "effects/audio_spk_eq.h"
#include "media/audio_energy_detect.h"
#include "bass_treble.h"
#include "scene_switch.h"
#include "effects_default_param.h"
#include "ascii.h"

struct effect_default_hdl {
    u16 pitchV;
};

struct effect_default_hdl effect_default = {
    .pitchV = 32768,
};

static u32 effect_strcmp(const char *str1, const char *str2);

u8 __attribute__((weak)) get_current_scene()
{
    return 0;
}
u8 __attribute__((weak)) get_mic_current_scene()
{
    return 0;
}
u8 __attribute__((weak)) get_music_eq_preset_index(void)
{
    return 0;
}

float powf(float x, float y);
static int get_pitchV(float pitch)
{
    int pitchV = 32768 * powf(2.0f, pitch / 12);
    pitchV = (pitchV >= 65536) ? 65534 : pitchV; //传入算法是u16类型，作特殊处理防止溢出
    return pitchV;
}

u16 audio_pitch_default_parm_set(u8 pitch_mode)
{
    float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    effect_default.pitchV = get_pitchV(pitch_param_table[pitch_mode]);
    return effect_default.pitchV;
}

static void audio_energy_det_handler(void *prive, u8 event, u8 ch, u8 ch_total)
{
    char name[16];
    memcpy(name, prive, strlen(prive));
    //name 模块名称，唯一标识
    // ch < ch_total 时：表示通道(ch)触发的事件
    // ch = ch_total 时：表示全部通道都触发的事件

    if (ch < ch_total) {
        /*
         *针对独立通道，比如立体声场景，这里就分别显示ch0和ch1的能量
         *适用于独立控制声道mute的场景
         */
        printf(">>>>name:%s ch:%d %s\n", name, ch, event ? ("MUTE") : ("UNMUTE"));
    }

    if (ch == ch_total) {
        /*
         *针对所有通道，比如立体声场景，只有ch0和ch1都是mute，all_ch才是mute
         *适用于控制整体
         */
        printf(">>>>name:%s ch_total %s\n", name,  event ? ("MUTE") : ("UNMUTE"));
    }
}

/*
 *获取需要指定得默认配置
 * */
int get_eff_default_param(int arg)
{
    int ret = 0;

    struct node_id *nodeid = (struct node_id *)arg;
    struct _name {
        char name[16];
    };
    struct _name *name = (struct _name *)arg;
#ifdef TCFG_SPEAKER_EQ_NODE_ENABLE
    if (!strcmp(name->name, "spk_eq")) {
        ret = spk_eq_read_from_vm((void *)arg);
    }
#endif

    if (!strncmp(name->name, "PitchSpeed", strlen("PitchSpeed"))) { //音乐变速变调 默认参数获取
        struct pitch_speed_update_parm *get_parm = (struct pitch_speed_update_parm *)arg;
        get_parm->speedV = 80;
        get_parm->pitchV = effect_default.pitchV;
        ret = 1;
    }

#ifdef TCFG_EQ_ENABLE
    if (!strncmp(name->name, "MusicEq", strlen("MusicEq"))) { //音乐eq命名默认： MusicEq + 类型，例如蓝牙音乐eq：MusicEqBt
        struct eq_default_parm *get_eq_parm = (struct eq_default_parm *)arg;
        /*
         *默认系数使用eq文件内的哪个配置表
         * */
        char tar_cfg_index = 0;
        get_cur_eq_num(&tar_cfg_index);//获取当前配置项序号
        get_eq_parm->cfg_index = tar_cfg_index;

        /*
         *是否使用sdk内的默认系数表
         * */
        get_eq_parm->default_tab.seg_num = get_cur_eq_tab(&get_eq_parm->default_tab.global_gain, &get_eq_parm->default_tab.seg);
        ret = 1;
    }
#endif

    if (!strncmp(name->name, "EnergyDet", strlen("EnergyDet"))) {//能量检查 回调接口配置
        struct energy_detect_get_parm *get_parm = (struct energy_detect_get_parm *)arg;
        if (get_parm->type == SET_ENERGY_DET_EVENT_HANDLER) {
            get_parm->event_handler = audio_energy_det_handler;
            ret = 1;
        }
    }

#ifdef TCFG_WDRC_NODE_ENABLE
    if (!strncmp(name->name, "MusicDrc", strlen("MusicDrc"))) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;//目标配置项
        ret = 1;
    }
#endif

#ifdef TCFG_BASS_TREBLE_NODE_ENABLE
    if (!strncmp(name->name, "MusicBassTre", strlen("MusicBassTre")) || (!effect_strcmp(name->name, "BassMedia"))) {
        ret = get_music_bass_treble_parm(arg);
    }
#endif

#ifdef TCFG_BASS_TREBLE_NODE_ENABLE
    if (!strncmp(name->name, "MicBassTre", strlen("MicBassTre")) || (!effect_strcmp(name->name, "BassTreEff"))) {
        ret = get_mic_bass_treble_parm(arg);
    }
#endif

#ifdef TCFG_SURROUND_NODE_ENABLE
    if (!effect_strcmp(name->name, "SurMedia")) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;
        get_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#ifdef TCFG_CROSSOVER_NODE_ENABLE
    if (!effect_strcmp(name->name, "CrossMedia")) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;
        get_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#if (defined(TCFG_3BAND_MERGE_ENABLE) || defined(TCFG_2BAND_MERGE_ENABLE))
    if (!effect_strcmp(name->name, "BandMedia")) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;
        get_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#ifdef TCFG_BASS_TREBLE_NODE_ENABLE
    if (!effect_strcmp(name->name, "BassMedia")) {
        struct bass_treble_default_parm *get_bass_parm = (struct bass_treble_default_parm *)arg;
        get_bass_parm->cfg_index = 0;
        get_bass_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#ifdef TCFG_STEROMIX_NODE_ENABLE
    if (!effect_strcmp(name->name, "Smix*Media")) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;
        get_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#ifdef TCFG_WDRC_NODE_ENABLE
    if (!effect_strcmp(name->name, "Drc*Media")) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;
        get_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#ifdef TCFG_EQ_ENABLE
    if (!effect_strcmp(name->name, "Eq*Media")) {
        struct eq_default_parm *get_eq_parm = (struct eq_default_parm *)arg;
        if (!effect_strcmp(name->name, "Eq0Media")) {
            get_eq_parm->cfg_index = get_music_eq_preset_index();
        } else {
            get_eq_parm->cfg_index = 0;
        }
        get_eq_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#ifdef TCFG_VOCAL_REMOVER_NODE_ENABLE
    if (!effect_strcmp(name->name, "VocalRemovMedia")) {
    }
#endif

#ifdef TCFG_EQ_ENABLE
    if (!effect_strcmp(name->name, "EscoDlEq") || !effect_strcmp(name->name, "EscoUlEq")) {
        struct eq_default_parm *get_eq_parm = (struct eq_default_parm *)arg;
        int type = lmp_private_get_esco_packet_type();
        int media_type = type & 0xff;
        if (media_type == 0) {//narrow band
            get_eq_parm->cfg_index = 1;
        } else {//wide band
            get_eq_parm->cfg_index = 0;
        }
        ret = 1;
    }
#endif


    return ret;
}

static u32 effect_strcmp(const char *str1, const char *str2)
{
    return ASCII_StrCmp(str1, str2, strlen(str1) + 1);
}
