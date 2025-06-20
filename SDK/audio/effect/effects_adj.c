
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "jlstream.h"
#include "cfg_tool.h"
#include "effects/effects_adj.h"
#include "effects/eq_config.h"
#include "effects/audio_eq.h"
#include "effects/audio_bass_treble_eq.h"

#define LOG_TAG_CONST EFFECTS
#define LOG_TAG     "[EFFECTS_ADJ]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"



#if TCFG_CFG_TOOL_ENABLE

struct eff_mode {
    u16 uuid;
    u16 mode_num;    //模式个数
    u16 mode_index;  //当前模式序号
} __attribute__((packed));

struct stream_crc {
    u16 uuid;
    u16 crc;
};

struct eff_struct { //参数结构，节点结构组成uuid name parm_struct
    u16 uuid;
    u16 reserve;
    char data[0];
} __attribute__((packed));

struct eff_online_packet {
    int cmd;
    struct eff_struct par;
};

struct eq_list_entry {
    struct list_head entry;
    struct eq_online *tab;
};

struct eff_list_entry {
    struct list_head entry;
    u32 len;
    struct eff_online_packet *packet;
};

struct eff_list {
    struct list_head head;            //链表头
};
static struct eff_list eff_hdl_eq = {0};
static struct eff_list eff_hdl = {0};

static spinlock_t eff_lock[2];
static int eff_list_init(void)
{
    INIT_LIST_HEAD(&eff_hdl_eq.head);
    INIT_LIST_HEAD(&eff_hdl.head);
    spin_lock_init(&eff_lock[0]);
    spin_lock_init(&eff_lock[1]);
    return 0;
}
__initcall(eff_list_init);

/*
 *eq调音临时系数更新
 * */
void eq_list_entry_update(struct eq_list_entry *hdl, struct eff_online_packet *ep, u32 size, int *ret, int *start, int *end)
{
    int *data = (int *)&ep->par.data[16];
    if (data[0] == EQ_SPECIAL_CMD) { //type bypass global_gain  sen_num
        if (data[3] != hdl->tab->seg_num) {
            if (hdl->tab) {
                free(hdl->tab);
            }
            hdl->tab = malloc(sizeof(struct eq_online) + data[3] * sizeof(struct eq_seg_info));
        }

        hdl->tab->is_bypass = data[1];
        memcpy(&hdl->tab->global_gain, &data[2], 4);//总增益是浮点
        hdl->tab->seg_num = data[3];
        hdl->tab->uuid = ep->par.uuid;
        memcpy(hdl->tab->name, &ep->par.data, 16);
        log_debug("multi hdl->tab->is_bypass %d\n", hdl->tab->is_bypass);
        log_debug("multi hdl->tab->global_gain %x\n", *(int *)&hdl->tab->global_gain);
        log_debug("multi hdl->tab->seg_num %d\n", hdl->tab->seg_num);
        *ret = EQ_SPECIAL_CMD;
        return;
    } else if (data[0] == EQ_SPECIAL_SEG_CMD) { //type seg[n]
        hdl->tab->uuid = ep->par.uuid;
        memcpy(hdl->tab->name, &ep->par.data, 16);
        struct eq_seg_info *src_seg = (struct eq_seg_info *)&data[1];
        int tar_size = size - 8 - 16; ////减去cmd uuid name
        *start = src_seg[0].index;
        for (int i = 0; i < hdl->tab->seg_num; i++) {
            if (tar_size >= sizeof(struct eq_seg_info)) {
                memcpy(&hdl->tab->seg[src_seg[i].index], &src_seg[i], sizeof(struct eq_seg_info));
                struct eq_seg_info *seg = (struct eq_seg_info *)&hdl->tab->seg[src_seg[i].index];
                /* log_debug("multi soure idx:%d, iir:%d, freq:%d, gain:0x%08x, q:0x%08x ", seg->index, seg->iir_type, seg->freq, *(int *)&seg->gain, *(int *)&seg->q); */
                tar_size -= sizeof(struct eq_seg_info);
                *end = src_seg[i].index;
            }
        }
        *ret = EQ_SPECIAL_SEG_CMD;
        return;
    }

    struct eq_adj eq = {0};
    memcpy(&eq, &ep->par.data[16], size - 8 - 16);//减去cmd uuid name
    if (eq.type == EQ_IS_BYPASS_CMD) {
        hdl->tab->is_bypass = eq.param.is_bypass;
        log_debug("tab->is_bypass %d\n", hdl->tab->is_bypass);
    } else if (eq.type == EQ_GLOBAL_GAIN_CMD) {
        hdl->tab->global_gain = eq.param.global_gain;
        log_debug("hdl->tab->global_gain %x\n", *(int *)&hdl->tab->global_gain);
    } else if (eq.type == EQ_SEG_NUM_CMD) {
        if (eq.param.seg_num != hdl->tab->seg_num) {
            float global_gain = hdl->tab->global_gain;
            int is_bypass = hdl->tab->is_bypass;
            if (hdl->tab) {
                free(hdl->tab);
            }
            hdl->tab = malloc(sizeof(struct eq_online) + eq.param.seg_num * sizeof(struct eq_seg_info));
            hdl->tab->global_gain = global_gain;
            hdl->tab->is_bypass = is_bypass;
            log_debug("hdl->tab->is_bypass %d\n", hdl->tab->is_bypass);
            log_debug("hdl->tab->global_gain %x\n", *(int *)&hdl->tab->global_gain);
        }
        hdl->tab->seg_num = eq.param.seg_num;
        log_debug("hdl->tab->seg_num %d\n", hdl->tab->seg_num);
    } else if (eq.type == EQ_SEG_CMD) {
        memcpy(&hdl->tab->seg[eq.param.seg.index], &eq.param.seg, sizeof(struct eq_seg_info));
        struct eq_seg_info *seg = (struct eq_seg_info *)&hdl->tab->seg[eq.param.seg.index];
        log_debug("soure idx:%d, iir:%d, freq:%d, gain:0x%08x, q:0x%08x ", seg->index, seg->iir_type, seg->freq, *(int *)&seg->gain, *(int *)&seg->q);
    }
    hdl->tab->uuid = ep->par.uuid;
    memcpy(hdl->tab->name, &ep->par.data, 16);
}
/*
 *通用音效临时参数更新
 * */
void eff_list_entry_update(struct eff_list_entry *hdl, struct eff_online_packet *ep, u32 size)
{
    if (size != hdl->len) {
        if (hdl->packet) {
            free(hdl->packet);
        }
        hdl->len = size;
        hdl->packet = malloc(size);
    }
    hdl->packet->cmd = ep->cmd;
    hdl->packet->par.uuid = ep->par.uuid;
    memcpy(&hdl->packet->par, &ep->par, size - 4);
}

/*
 *把在线调试的参数建立到链表内，方便节点切歌时，获取当前已经调试的参数
 * */
void eff_entry_add(void *packet, u32 size, int *ret, int *start, int *end)
{
    struct eff_online_packet *ep = (struct eff_online_packet *)packet;
    char name[16];
    char src_name[16];
    memcpy(name, ep->par.data, sizeof(name));
    if (!strlen(name)) { //节点名字未填写，不做记录
        return;
    }
    if (ep->par.uuid != NODE_UUID_EQ) {
        spin_lock(&eff_lock[0]);
        if (!list_empty(&eff_hdl.head)) {
            struct eff_list_entry *hdl;
            list_for_each_entry(hdl, &eff_hdl.head, entry) {
                memcpy(src_name, hdl->packet->par.data, 16);
                if ((hdl->packet->par.uuid == ep->par.uuid) && !strncmp(src_name, name, 16)) { //查询已存在,更新参数
                    eff_list_entry_update(hdl, ep, size);
                    log_debug("node update\n");
                    spin_unlock(&eff_lock[0]);
                    return;
                }
            }
        }
        //查询未存在，记录参数，并add到链表
        struct eff_list_entry *hdl = zalloc(sizeof(struct eff_list_entry));
        eff_list_entry_update(hdl, ep, size);
        list_add(&hdl->entry, &eff_hdl.head);
        log_debug("node add\n");
        spin_unlock(&eff_lock[0]);
    } else {
        spin_lock(&eff_lock[1]);
        if (!list_empty(&eff_hdl_eq.head)) { //非空
            struct eq_list_entry *hdl;
            list_for_each_entry(hdl, &eff_hdl_eq.head, entry) {
                if ((hdl->tab->uuid == ep->par.uuid) && !strncmp(hdl->tab->name, name, 16)) { //查询已存在,更新参数
                    eq_list_entry_update(hdl, ep, size, ret, start, end);
                    log_debug("eq_list upadate\n");
                    spin_unlock(&eff_lock[1]);
                    return;
                }
            }
        }
        //查询未存在，记录参数，并add到链表
        struct eq_list_entry *hdl = zalloc(sizeof(struct eq_list_entry));
        int default_section = 10;
        hdl->tab = malloc(sizeof(struct eq_online) + default_section * sizeof(struct eq_seg_info));
        hdl->tab->seg_num = default_section;
        eq_list_entry_update(hdl, ep, size, ret, start, end);
        list_add(&hdl->entry, &eff_hdl_eq.head);
        log_debug("eq_list add\n");
        spin_unlock(&eff_lock[1]);
    }
}
/*
 *节点通过uuid 与name获取在线调试音效的参数
 * */
int get_eff_online_param(u32 uuid, char *name, void *packet)
{
    int ret = 0;
    char src_name[16];
    if (uuid != NODE_UUID_EQ) {
        spin_lock(&eff_lock[0]);
        if (!list_empty(&eff_hdl.head)) { //非空
            struct eff_list_entry *hdl;
            list_for_each_entry(hdl, &eff_hdl.head, entry) {
                memcpy(src_name, hdl->packet->par.data, 16);
                if ((hdl->packet->par.uuid == uuid) && !strncmp(src_name, name, 16)) {
                    char *ptr = (char *)packet;//结构：uuid name param
                    char *src = (char *)&hdl->packet->par;
                    memcpy(&ptr[20], &src[20], hdl->len - 4 - 20);// 4byte 是减去cmd的长度, 20是 uuid reserve name 长度
                    ret = 1;
                    break;
                }
            }
        }
        spin_unlock(&eff_lock[0]);
    } else {
        spin_lock(&eff_lock[1]);
        if (!list_empty(&eff_hdl_eq.head)) { //非空
            struct eq_list_entry *hdl;
            list_for_each_entry(hdl, &eff_hdl_eq.head, entry) {
                if ((hdl->tab->uuid == uuid) && !strncmp(hdl->tab->name, name, 16)) {
                    struct eq_online *tar = (struct eq_online *)packet;
                    tar->is_bypass = hdl->tab->is_bypass;
                    tar->global_gain = hdl->tab->global_gain;
                    tar->seg_num = hdl->tab->seg_num;
                    memcpy(tar->seg, hdl->tab->seg, sizeof(struct eq_seg_info)*hdl->tab->seg_num);
                    /* log_debug("tar seg %x\n", (int)tar->seg); */
                    /* for (int i = 0; i < tar->seg_num; i++) { */
                    /* struct eq_seg_info *seg = (struct eq_seg_info *)&hdl->tab->seg[i]; */
                    /* log_debug("source idx:%d, iir:%d, freq:%d, gain:0x%08x, q:0x%08x ", seg->index, seg->iir_type, seg->freq, *(int *)&seg->gain, *(int *)&seg->q); */
                    /* } */
                    ret = 1;
                    break;
                }
            }
        }
        spin_unlock(&eff_lock[1]);
    }
    return ret;
}
/*
 *接收上位机发来的模式信息，小机可用于播放相应模式提示音
 * */
void eff_mode_switch(void *priv)
{
    struct eff_mode *param = (struct eff_mode *)priv;
    log_debug("uuid     0x%x\n", param->uuid);
    log_debug("mode_num   %d\n", param->mode_num);
    log_debug("mode_index %d\n", param->mode_index);
}

/*
 *在线调音开始时会检查，界面框图流程的crc与stream.bin的crc是否一致
 *不一致时，需要重新更新stream.bin并编译下载代码
 * */
int eff_stream_crc_check(void *priv)
{
    struct stream_crc *check = (struct stream_crc *)priv;
    u16 crc = jlstream_read_stream_crc();
    if (crc == check->crc) {
        return true;
    } else {
        log_error("check stream crc fail, please update stream.bin and re-download \n");
    }
    return false;
}

int indicator_get_param(struct eff_online_packet *ep, u8 sq)
{
    char name[16];
    int res = ERR_ACCEPTABLE;
    struct indicator_private_cmd {
        u16 uuid;
        u8 subid;
        u8 reserve;
    };
    struct indicator_private_cmd *indator = (struct indicator_private_cmd *)&ep->par;
    name[0] = indator->subid;
    memcpy(&name[1], "indicator", strlen("indicator"));//指示器控件无自定义节点明，此处特殊处理
    struct indicator_upload_data indicator_data = {0};
    res = jlstream_get_node_param(ep->par.uuid, name, &indicator_data, sizeof(indicator_data));
    if (res == sizeof(indicator_data)) {
        /* printf("peak %d\n", indicator_data.peak); */
        eff_node_send_packet(REPLY_TO_TOOL, sq, (u8 *)&indicator_data, sizeof(indicator_data));
        res = ERR_ACCEPTABLE;
    } else {
        res = ERR_COMM;
    }
    return res;
}
/*
 *高低音获取当前gain并上传
 * */
static int bass_treble_get_node_param(struct eff_online_packet *ep, u8 sq)
{
    int res = ERR_COMM;
    struct bass_treble_private_cmd { //协议相关
        u16 uuid;
        char name[16];
        char mark[16];
    } __attribute__((packed));
    struct seg_gain gain = {0};
    struct bass_treble_private_cmd *get_param = (struct bass_treble_private_cmd *)&ep->par;
    if (!strcmp("low", get_param->mark)) {
        gain.index = 0;
    } else if (!strcmp("mid", get_param->mark)) {
        gain.index = 1;
    } else if (!strcmp("hig", get_param->mark)) {
        gain.index = 2;
    }
    res = jlstream_get_node_param(get_param->uuid, get_param->name, &gain, sizeof(gain));//获取当前增益
    if (res == sizeof(gain)) {
        struct bass_treb_upload { //协议相关
            u8 len;
            u8 type;
            float cur_gain;
        } __attribute__((packed));

        struct bass_treb_upload upload_param;
        upload_param.len = sizeof(struct bass_treb_upload) - 1;
        upload_param.type = 0x1;//数据类型 0：u32 1:float
        upload_param.cur_gain = gain.gain;
        log_debug("bass treble idx : %d, gain: 0x%x\n", gain.index, *(int *)&gain.gain);
        eff_node_send_packet(REPLY_TO_TOOL, sq, (u8 *)&upload_param, sizeof(upload_param));//上传当前增益
        res = ERR_ACCEPTABLE;
    }
    return res;
}

/*
 *获取当前音量并上传
 * */
static int volume_get_node_param(struct eff_online_packet *ep, u8 sq)
{
    int res = ERR_COMM;
    struct volume_private_cmd { //协议相关
        u16 uuid;
        char name[16];
        char mark[16];
    } __attribute__((packed));

    struct volume_cfg_tmp {//临时标识，待音量节点完善后，再具体调整
        u16 cfg_level_max;    //最大音量等级
        s32  cfg_vol_min;      //最小音量,dB
        u8 vol_table_custom; //是否自定义音量表
        s32  cfg_vol_max;     //最大音量,dB
        s16 cur_vol;        //当前音量
    } __attribute__((packed));


    struct volume_private_cmd *get_param = (struct volume_private_cmd *)&ep->par;
    if (!strcmp("volume", get_param->mark)) {
        struct volume_cfg_tmp cfg = {0};
        res = jlstream_get_node_param(get_param->uuid, get_param->name, &cfg, sizeof(cfg));//获取当前增益
        if (res == sizeof(cfg)) {
            struct volume_upload { //协议相关
                u8 len;
                u8 type;
                s16 cur_vol;
            } __attribute__((packed));

            struct volume_upload upload_param;
            upload_param.len = sizeof(struct volume_upload) - 1;
            upload_param.type = 0x0;//数据类型 0：u32 1:float
            upload_param.cur_vol = cfg.cur_vol;
            log_debug("volume get cur_vol %d\n", cfg.cur_vol);
            eff_node_send_packet(REPLY_TO_TOOL, sq, (u8 *)&upload_param, sizeof(upload_param));//上传当前增益
            res = ERR_ACCEPTABLE;
        }
    }
    return res;
}


/*
 *表单节点获取当前值
 * */
static int form_node_get_parm(struct eff_online_packet *ep, u8 sq)
{
    int res = ERR_COMM;
    switch (ep->par.uuid) {
    case NODE_UUID_BASS_TREBLE:
        res = bass_treble_get_node_param(ep, sq);
        break;
    case NODE_UUID_VOLUME_CTRLER:
        /* res = volume_get_node_param(ep, sq); */
        break;
    default:
        //do smt
        break;
    }
    return res;
}
/*
 *工具获取支持在线调试的节点列表
 * */
static int check_eff_node_online_support(struct eff_online_packet *ep, u8 sq)
{
    int res = ERR_ACCEPTABLE;
    u16 cnt = 0;
    struct effects_online_adjust *p;
    list_for_online_adjust_target(p) {
        /* printf("uuid %x\n", p->uuid);	 */
        cnt++;
    }
    eff_node_send_packet(REPLY_TO_TOOL, sq, (u8 *)effects_online_adjust_begin, (int)(cnt * sizeof(u16))); //上传当前增益
    return res;
}
/*
*工具同步界面全部参数下来时，快速更新数据给节点
 * */
static void eq_fast_update(char *name, u8 type, int start, int end)
{

    struct eq_tool *tab = NULL;
    struct eq_online *online_parm = zalloc(sizeof(struct eq_online) + sizeof(struct eq_seg_info) * AUDIO_EQ_MAX_SECTION);
    if (online_parm) {
        online_parm->uuid = NODE_UUID_EQ;
        memcpy(online_parm->name, name, strlen(name));
        int online_ret = jlstream_event_notify(STREAM_EVENT_GET_EFF_ONLINE_PARM, (int)online_parm);
        if (online_ret) {
            int seg_size = sizeof(struct eq_seg_info) * online_parm->seg_num;
            tab = zalloc(sizeof(struct eq_tool) + seg_size);
            tab->is_bypass = online_parm->is_bypass;
            tab->global_gain = online_parm->global_gain;
            tab->seg_num = online_parm->seg_num;
            memcpy(tab->seg, online_parm->seg, sizeof(struct eq_seg_info)*tab->seg_num);
        }
        free(online_parm);
    }
    if (!tab) {
        return;
    }


    struct eq_adj eff = {0};
    if (type == EQ_SPECIAL_CMD) {
        //运行时，直接设置更新
        eff.type = EQ_SEG_NUM_CMD;
        eff.param.seg_num = tab->seg_num;
        jlstream_set_node_param(NODE_UUID_EQ, name, &eff, sizeof(eff));//更新滤波器段数

        eff.type = EQ_IS_BYPASS_CMD;
        eff.param.is_bypass = tab->is_bypass;
        jlstream_set_node_param(NODE_UUID_EQ, name, &eff, sizeof(eff));//更新bypass状态

        eff.type = EQ_GLOBAL_GAIN_CMD;
        eff.param.global_gain =  tab->global_gain;
        jlstream_set_node_param(NODE_UUID_EQ, name, &eff, sizeof(eff));//更新总增益
    } else if (type == EQ_SPECIAL_SEG_CMD) {
        for (int i = start; i <= end; i++) {
            eff.type = EQ_SEG_CMD;
            memcpy(&eff.param.seg, &tab->seg[i], sizeof(struct eq_seg_info));
            jlstream_set_node_param(NODE_UUID_EQ, name, &eff, sizeof(eff));//更新滤波器系数
        }
    }
    free(tab);
}
static s32 eff_online_update(void *packet, u32 size, u8 sq)
{
    int eq_ret = 0;
    int start = 0;
    int end = 0;
    int res = ERR_ACCEPTABLE;
    char name[16];
    struct eff_online_packet *ep = (struct eff_online_packet *)packet;
    log_debug("cmd %x\n", ep->cmd);
    switch (ep->cmd) {
    case EFF_ADJ_CMD:
        memcpy(name, ep->par.data, sizeof(name));
        log_debug("uuid:0x%x name %s\n", ep->par.uuid, name);
        eff_entry_add(packet, size, (int *)&eq_ret, &start, &end);
        if (eq_ret == EQ_SPECIAL_CMD) {
            res = ERR_NONE;
            eq_fast_update(name, eq_ret, start, end);
            break;
        }

        if (eq_ret == EQ_SPECIAL_SEG_CMD) {
            res = ERR_NONE;
            eq_fast_update(name, eq_ret, start, end);
            break;
        }
        res = jlstream_set_node_param(ep->par.uuid, name, &ep->par.data[16], size - sizeof(ep->cmd) - sizeof(ep->par.uuid) - sizeof(ep->par.reserve) - 16); //减去cmd 减uuid 减revere 减name
        break;
    case EFF_MODE_CMD:
        eff_mode_switch((void *)&ep->par);
        res = ERR_NONE;
        break;
    case EFF_CRC_CMD:
        if (eff_stream_crc_check((void *)&ep->par)) {
            res = ERR_NONE;
        } else {
            res = EFF_ERR_CRC;
        }
        break;
    case EFF_INDICATOR_CMD:
        res = indicator_get_param(ep, sq);
        break;
    case EFF_FORM_CMD:
        res = form_node_get_parm(ep, sq);
        break;
    case EFF_ONLINE_CMD:
        res = check_eff_node_online_support(ep, sq);
        break;
    default:
        break;
    }

    return res;
}

static void eff_send_packet(u32 id, u8 sq, u8 *packet, int size)
{
    all_assemble_package_send_to_pc(id, sq, packet, size);
}

void eff_node_send_packet(u32 id, u8 sq, u8 *packet, int size)
{
    all_assemble_package_send_to_pc(id, sq, packet, size);
}

static void effect_tool_callback(u8 *packet, u32 size)
{
    int res = 0;
    u8 id = packet[0];
    u8 sq = packet[1];
    u8 *eff_packet = (u8 *)&packet[2];
    ASSERT(((int)eff_packet & 0x3) == 0, "packet %x size %d\n", (unsigned int)eff_packet, size - 2);

    res = eff_online_update((void *)&packet[2], size - 2, sq);
    switch (res) {
    case EFF_ERR_TYPE_PTR_NULL:
        log_debug("buf err");
        eff_send_packet(REPLY_TO_TOOL, sq, (u8 *)"ER_FLOW_OPEN_BUF", strlen("ER_FLOW_OPEN_BUF"));
        break;
    case EFF_ERR_TYPE_ALGORITHM_NULL:
        log_debug("algorithm err");
        eff_send_packet(REPLY_TO_TOOL, sq, (u8 *)"ER_FLOW_OPEN_ALGORITHM", strlen("ER_FLOW_OPEN_ALGORITHM"));
        break;
    case ERR_COMM:
        log_debug("Nack");
        eff_send_packet(REPLY_TO_TOOL, sq, (u8 *)"ER", strlen("ER"));
        break;
    case EFF_ERR_CRC:
        log_debug("crc err");
        eff_send_packet(REPLY_TO_TOOL, sq, (u8 *)"ER_FLOW_CRC", strlen("ER_FLOW_CRC"));
        break;
    case ERR_ACCEPTABLE:
        //可接受的错误,由case内部处理,返回内部数据
        log_debug("accept");
        break;
    default:
        log_debug("Ack");
        eff_send_packet(REPLY_TO_TOOL, sq, (u8 *)"OK", strlen("OK"));
        break;
    }
}


REGISTER_DETECT_TARGET(eff_adj_target) = {
    .id         = EFF_CONFIG_ID,
    .tool_message_deal   = effect_tool_callback,
};




#endif


