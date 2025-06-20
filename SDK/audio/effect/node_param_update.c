#include "jlstream.h"
#include "node_uuid.h"
#include "effects/effects_adj.h"
#include "node_param_update.h"


/* 各模块参数更新接口 */
void stero_mix_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    stereo_mix_gain_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_STEROMIX, node_name, &cfg, sizeof(cfg));
}
void surround_effect_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    surround_effect_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_SURROUND, node_name, &cfg, sizeof(cfg));
}
void crossover_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    crossover_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    if (cfg.parm.way_num == 3) {
        jlstream_set_node_param(NODE_UUID_CROSSOVER, node_name, &cfg, sizeof(cfg));
    } else {
        jlstream_set_node_param(NODE_UUID_CROSSOVER_2BAND, node_name, &cfg, sizeof(cfg));
    }
}
void band_merge_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    multi_mix_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_3BAND_MERGE, node_name, &cfg, sizeof(cfg));
}
void drc_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    wdrc_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_WDRC, node_name, &cfg, sizeof(cfg));
}
void bass_treble_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    bass_treble_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_BASS_TREBLE, node_name, &cfg, sizeof(cfg));
}
void autotune_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    autotune_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_AUTOTUNE, node_name, &cfg, sizeof(cfg));
}
void chorus_udpate_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    chorus_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_CHORUS, node_name, &cfg, sizeof(cfg));
}
void dynamic_eq_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    dynamic_eq_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_DYNAMIC_EQ, node_name, &cfg, sizeof(cfg));
}
void echo_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{

    echo_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_ECHO, node_name, &cfg, sizeof(cfg));
}
void howling_frequency_shift_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    howling_pitchshift_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_FREQUENCY_SHIFT, node_name, &cfg, sizeof(cfg));
}
void howling_suppress_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    notch_howling_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_HOWLING_SUPPRESS, node_name, &cfg, sizeof(cfg));
}
void gain_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    gain_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_GAIN, node_name, &cfg, sizeof(cfg));
}
void noisegate_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    noisegate_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_NOISEGATE, node_name, &cfg, sizeof(cfg));
}
void reverb_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    plate_reverb_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_PLATE_REVERB, node_name, &cfg, sizeof(cfg));
}
void reverb_advance_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    plate_reverb_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_PLATE_REVERB_ADVANCE, node_name, &cfg, sizeof(cfg));
}
void spectrum_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct spectrum_parm  cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_SPECTRUM, node_name, &cfg, sizeof(cfg));
}
void stereo_widener_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    stereo_widener_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_STEREO_WIDENER, node_name, &cfg, sizeof(cfg));
}
void virtual_bass_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    virtual_bass_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_VBASS, node_name, &cfg, sizeof(cfg));
}
void voice_changer_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    voice_changer_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        return;
    }
    jlstream_set_node_param(NODE_UUID_VOICE_CHANGER, node_name, &cfg, sizeof(cfg));
}
void channel_expander_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    channel_expander_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return;
    }
    jlstream_set_node_param(NODE_UUID_CHANNEL_EXPANDER, node_name, &cfg, sizeof(cfg));
}


/* eq参数更新接口 */
void eq_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct cfg_info info  = {0};             //节点配置相关信息（参数存储的目标地址、配置项大小）
    int ret = jlstream_read_form_node_info_base(mode_index, node_name, cfg_index, &info);
    if (!ret) {
        struct eq_tool *tab = zalloc(info.size);
        if (!jlstream_read_form_cfg_data(&info, tab)) {
            printf("user eq cfg parm read err\n");
            free(tab);
            return;
        }

        //运行时，直接设置更新
        struct eq_adj eff = {0};
        eff.type = EQ_GLOBAL_GAIN_CMD;
        eff.param.global_gain =  tab->global_gain;
        eff.fade_parm.fade_time = 10;        //ms，淡入timer执行的周期
        eff.fade_parm.fade_step = 0.1f;      //淡入步进
        jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));//更新总增益

        eff.type = EQ_SEG_NUM_CMD;
        eff.param.seg_num = tab->seg_num;
        jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));//更新滤波器段数

        for (int i = 0; i < tab->seg_num; i++) {
            eff.type = EQ_SEG_CMD;
            memcpy(&eff.param.seg, &tab->seg[i], sizeof(struct eq_seg_info));
            jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));//更新滤波器系数
        }
        free(tab);
    }
}

