#ifndef JLSTREAM_H
#define JLSTREAM_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "node_uuid.h"
#include "media/audio_base.h"
#include "system/task.h"
#include "system/spinlock.h"
#include "fs/resfile.h"


#define __STREAM_CACHE_CODE     __attribute__((section(".stream.text.cache.L2")))
#define __STREAM_CACHE_CODE     __attribute__((section(".stream.text.cache.L2")))

extern const int CONFIG_JLSTREAM_MULTI_THREAD_ENABLE;

struct stream_oport;
struct stream_node;
struct stream_snode;
struct stream_note;
struct jlstream;

#ifndef SEEK_SET
#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */
#endif


#define NODE_IOC_OPEN_IPORT         0x00010000
#define NODE_IOC_CLOSE_IPORT        0x00010001
#define NODE_IOC_OPEN_OPORT         0x00010002
#define NODE_IOC_CLOSE_OPORT        0x00010003

#define NODE_IOC_SET_FILE           0x00020000
#define NODE_IOC_OPEN_FILE          0x00020001
#define NODE_IOC_GET_FMT            0x00020002
#define NODE_IOC_SET_FMT            0x00020003
#define NODE_IOC_CLR_FMT            0x00020004
#define NODE_IOC_GET_DELAY          0x00020005      // 获取缓存数据的延时
#define NODE_IOC_SET_SCENE          0x00020006      // 设置数据流的场景
#define NODE_IOC_SET_CHANNEL        0x00020007
#define NODE_IOC_NEGOTIATE          0x00020008      // 各个节点参数协商
#define NODE_IOC_FLUSH_OUT          0x00020009
#define NODE_IOC_SET_TIME_STAMP     0x0002000a
#define NODE_IOC_SYNCTS             0x0002000b
#define NODE_IOC_NODE_CONFIG        0x0002000c
#define NODE_IOC_SET_PRIV_FMT 		0x0002000d		//设置节点私有参数
#define NODE_IOC_NAME_MATCH         0x0002000e
#define NODE_IOC_SET_PARAM          0x0002000f
#define NODE_IOC_GET_PARAM          0x00020010
#define NODE_IOC_DECODER_PP		    0x00020011		//解码暂停再重开
#define NODE_IOC_DECODER_FF	    	0x00020012		//快进
#define NODE_IOC_DECODER_FR    		0x00020013		//快退
#define NODE_IOC_DECODER_REPEAT     0x00020014      //无缝循环播放
#define NODE_IOC_DECODER_DEST_PLAY  0x00020015      //跳到指定位置播放
#define NODE_IOC_GET_CUR_TIME  		0x00020016      //获取音乐播放当前时间
#define NODE_IOC_GET_TOTAL_TIME  	0x00020017      //获取音乐播放总时间
#define NODE_IOC_GET_BP  			0x00020018      //获取解码断点信息
#define NODE_IOC_SET_BP         	0x00020019      //设置解码断点信息
#define NODE_IOC_SET_FILE_LEN       0x0002001a      //设置解码文件长度信息
#define NODE_IOC_SET_BP_A           0x0002001b      //设置复读A点
#define NODE_IOC_SET_BP_B           0x0002001c      //设置复读B点
#define NODE_IOC_SET_AB_REPEAT      0x0002001d      //设置AB点复读模式
#define NODE_IOC_GET_ODEV_CACHE     0x0002001e      //获取输出设备的缓存采样数
#define NODE_IOC_SET_BTADDR         0x0002001f
#define NODE_IOC_SET_ENC_FMT        0x00020020
#define NODE_IOC_GET_ENC_FMT        0x00020021
#define NODE_IOC_GET_HEAD_INFO      0x00020022      //获取编码的头文件信息
#define NODE_IOC_SET_TASK           0x00020023
#define NODE_IOC_FSEEK              0x00020024
#define NODE_IOC_GET_BTADDR         0x00020025


#define NODE_IOC_START              (0x00040000 | NODE_STA_RUN)
#define NODE_IOC_PAUSE              (0x00040000 | NODE_STA_PAUSE)
#define NODE_IOC_STOP               (0x00040000 | NODE_STA_STOP)
#define NODE_IOC_SUSPEND            (0x00040000 | NODE_STA_SUSPEND)

#define TIMESTAMP_US_DENOMINATOR    32

enum stream_event {
    STREAM_EVENT_NONE,
    STREAM_EVENT_INIT,
    STREAM_EVENT_LOAD_DECODER,
    STREAM_EVENT_LOAD_ENCODER,
    STREAM_EVENT_UNLOAD_DECODER,
    STREAM_EVENT_UNLOAD_ENCODER,

    STREAM_EVENT_SUSPEND,
    STREAM_EVENT_READY,
    STREAM_EVENT_START,
    STREAM_EVENT_PAUSE,
    STREAM_EVENT_STOP,

    STREAM_EVENT_CLOSE_PLAYER,
    STREAM_EVENT_CLOSE_RECODER,
    STREAM_EVENT_PREEMPTED,
    STREAM_EVENT_NEGOTIATE_FAILD,
    STREAM_EVENT_GET_PIPELINE_UUID,

    STREAM_EVENT_GET_COEXIST_POLICY,
    STREAM_EVENT_INC_SYS_CLOCK,

    STREAM_EVENT_GET_NODE_PARM,
    STREAM_EVENT_GET_EFF_ONLINE_PARM,

    STREAM_EVENT_A2DP_ENERGY,
};

enum stream_scene {
    STREAM_SCENE_A2DP,
    STREAM_SCENE_ESCO,
    STREAM_SCENE_TWS_MUSIC,     // TWS转发本地音频文件
    STREAM_SCENE_AI_VOICE,
    STREAM_SCENE_LINEIN,        //linein 模式
    STREAM_SCENE_FM,            //FM 模式
    STREAM_SCENE_MUSIC,         //本地音乐
    STREAM_SCENE_RECODER,       //录音
    STREAM_SCENE_SPDIF,
    STREAM_SCENE_PC_SPK,
    STREAM_SCENE_PC_MIC,
    STREAM_SCENE_MIC_EFFECT,
    STREAM_SCENE_DEV_FLOW,
    STREAM_SCENE_TONE = 0x10,
    STREAM_SCENE_RING = 0x30,
    STREAM_SCENE_KEY_TONE = 0x50,
};

enum stream_type {
    STREAM_TYPE_DEC,
    STREAM_TYPE_ENC,
};

enum stream_coexist {
    STREAM_COEXIST_AUTO,
    STREAM_COEXIST_DISABLE,
};


enum stream_state {
    STREAM_STA_INIT,
    STREAM_STA_NEGOTIATE,
    STREAM_STA_READY,
    STREAM_STA_START,
    STREAM_STA_STOP,
    STREAM_STA_PLAY,

    STREAM_STA_PAUSE        = 0x10,
    STREAM_STA_PREEMPTED    = 0x20,
    STREAM_STA_SUSPEND      = 0x30,
};

enum stream_node_state  {
    NODE_STA_RUN                    = 0x0001,
    NODE_STA_PAUSE                  = 0x0002,
    NODE_STA_SUSPEND                = 0x0004,
    NODE_STA_STOP                   = 0x0008,
    NODE_STA_PEND                   = 0x0010,
    NODE_STA_DEC_NO_DATA            = 0x0020,
    NODE_STA_DEC_END                = 0x0040,
    NODE_STA_DEC_PAUSE              = 0x0080,
    NODE_STA_DEC_ERR                = 0x00c0,
    NODE_STA_SOURCE_NO_DATA         = 0x0100,
    NODE_STA_DEV_ERR                = 0x0200,
    NODE_STA_ENC_END                = 0x0400,
    NODE_STA_OUTPUT_TO_FAST         = 0x0800,   //解码输出太多主动挂起
    NODE_STA_OUTPUT_BLOCKED         = 0x1000,   //终端节点缓存满,数据写不进去
    NODE_STA_OUTPUT_SPLIT           = 0x2000,
};

enum stream_node_type {
    NODE_TYPE_SYNC      = 0x01,
    NODE_TYPE_BYPASS    = 0x03,
    NODE_TYPE_ASYNC     = 0x10,
    NODE_TYPE_IRQ       = 0x20,
};

enum negotiation_state {
    NEGO_STA_CONTINUE       = 0x01,
    NEGO_STA_ACCPTED        = 0x02,
    NEGO_STA_SUSPEND        = 0x04,
    NEGO_STA_DELAY          = 0x08,
    NEGO_STA_ABORT          = 0x10,
};


struct stream_fmt {
    u8 channel_mode;
    u32 sample_rate;
    u32 coding_type;
};

struct stream_enc_fmt {
    u8 channel;
    u32 sample_rate;
    u32 bit_rate;
    u32 coding_type;
};

struct stream_file_ops {
    int (*read)(void *file, u8 *buf, int len);
    int (*seek)(void *file, int offset, int fromwhere);
    int (*write)(void *file, u8 *data, int len);
    int (*close)(void *file);
    int (*get_fmt)(void *file, struct stream_fmt *fmt);
    int (*get_name)(void *file, u8 *name, int len);
};

struct stream_file_info {
    void *file;
    const char *fname;
    const struct stream_file_ops *ops;
    struct audio_dec_breakpoint *dbp;
    enum stream_scene scene;
    u32 coding_type;
};

struct stream_dec_file {
    void *file;
    int (*fread)(void *file, u8 *buf, u32 fpos, int len, OS_SEM *sem);
};

struct stream_get_fmt {
    struct stream_file_info *file_info;
    struct stream_fmt *fmt;
};

struct stream_decoder_info {
    enum stream_scene scene;
    u32 coding_type;
    const char *task_name;
};

struct stream_encoder_info {
    enum stream_scene scene;
    u32 coding_type;
    const char *task_name;
};

/*
 * scene_a和scene_b指定的格式有冲突，scene_a抢占scene_b
 */
struct stream_coexist_policy {
    enum stream_scene scene_a;
    enum stream_scene scene_b;
    u32 coding_a;
    u32 coding_b;
};

enum frame_flags {
    FRAME_FLAG_FLUSH_OUT                = 0x01,
    FRAME_FLAG_FILL_ZERO                = 0x03,     //补0包
    FRAME_FLAG_FILL_PACKET              = 0x05,     //补数据包
    FRAME_FLAG_TIMESTAMP_ENABLE         = 0x08,     //时间戳有效
    FRAME_FLAG_RESET_TIMESTAMP_BIT      = 0x10,
    FRAME_FLAG_RESET_TIMESTAMP          = 0x11,     //重设各节点的时间戳
    FRAME_FLAG_SET_DRIFT_BIT            = 0x20,
    FRAME_FLAG_SET_D_SAMPLE_RATE        = 0x21,     //设置sampte_rate的delta
    FRAME_FLAG_UPDATE_TIMESTAMP         = 0x40,     //更新时间戳，驱动节点重设
    FRAME_FLAG_UPDATE_DRIFT_SAMPLE_RATE = 0x80,
    FRAME_FLAG_SYS_TIMESTAMP_ENABLE     = 0x100,    //数据帧使用系统时间戳
    FRAME_FLAG_UPDATE_INFO              = FRAME_FLAG_UPDATE_TIMESTAMP | FRAME_FLAG_UPDATE_DRIFT_SAMPLE_RATE,
};

//数据位宽
enum data_bit_width {
    DATA_BIT_WIDE_16BIT = 0,
    DATA_BIT_WIDE_32BIT = 1,
};

struct stream_frame {
    struct list_head entry;
    enum frame_flags flags;
    enum data_bit_width bit_wide;
    u8  ref;
    u16  delay;
    u16 offset;
    u16 len;
    u16 size;
    s16 d_sample_rate;
    u32 timestamp;
    u32 fpos;
    u8 *data;
};


struct stream_thread {
    struct list_head entry;
    u8 id;
    u8 node_num;
    u8 debug;
    u8 run_cnt;
    u16 timer;
    u16 start_mask;
    u16 id_mask;
    const char *name;
    OS_SEM sem;
    OS_MUTEX mutex;
};

struct stream_iport {
    struct list_head frames;

    u8 id;

    enum stream_node_state state;

    u32 run_time;       //usec

    void *private_data;

    struct stream_node *node;

    struct stream_oport *prev;

    struct stream_iport *sibling;

    void (*handle_frame)(struct stream_iport *, struct stream_note *note);
};


struct stream_oport {

    s16 d_sample_rate;

    u8 id;

    struct stream_fmt fmt;

    enum frame_flags flags;

    u32 offset;

    u32 timestamp;

    struct stream_node *node;

    struct stream_iport *next;

    struct stream_oport *sibling;

};


struct stream_node_adapter {
    const char *name;

    u16 uuid;

    u8 ability_channel_in;
    u8 ability_channel_out;
    u8 ability_channel_convert;
    u8 ability_resample;

    int (*bind)(struct stream_node *, u16 uuid);

    int (*ioctl)(struct stream_iport *iport, int cmd, int arg);

    void (*release)(struct stream_node *node);
};

struct stream_node {

    u8 subid;

    u16 uuid;

    enum stream_node_type type;

    enum stream_node_state state;

    void *private_data;

    struct stream_iport *iport;

    struct stream_oport *oport;

    struct stream_thread *thread;

    const struct stream_node_adapter *adapter;
};

struct stream_snode {
    struct stream_node node;

    struct jlstream *stream;

    void (*push_data)(struct stream_snode *node, struct stream_note *note);
    void (*seek_data)(struct stream_snode *node, u32 fpos);
};

enum {
    THREAD_TYPE_SOURCE  = 0x01,
    THREAD_TYPE_DEC     = 0x02,
    THREAD_TYPE_ENC     = 0x04,
    THREAD_TYPE_OUTPUT  = 0x08,
};


struct stream_note {

    u8 thread_type;
    u8 thread_match;

    int delay;

    int sleep;

    u32 jiffies;

    enum stream_node_state state;

    struct stream_thread *thread;

    struct jlstream *stream;
};

struct stream_ctrl {
    enum stream_state state;
    u16 fade_msec;
    u32 fade_timeout;
    OS_SEM sem;
};

struct jlstream {
    struct list_head entry;

    u32 end_jiffies;

    u32 coding_type;

    u8 id;
    u8 ref;
    u8 sleep_reason;
    u8 run_cnt;
    u8 delay;
    u8 incr_sys_clk;
    u16 timer;

    enum stream_state state;
    enum stream_state pp_state;

    enum stream_scene scene;

    enum stream_coexist coexist;

    struct stream_snode *snode;

    struct stream_ctrl ctrl;

    u16 callback_timer;
    struct list_head callback_head;
    void *callback_arg;
    const char *callback_task;
    void (*callback_func)(void *, int);
};


#define REGISTER_STREAM_NODE_ADAPTER(adapter) \
    const struct stream_node_adapter adapter sec(.stream_node_adapter)

#define PCM_SAMPLE_ONE_SECOND   (1000000 * TIMESTAMP_US_DENOMINATOR)
#define PCM_SAMPLE_TO_TIMESTAMP(frames, sample_rate) \
    ((u64)(frames) * PCM_SAMPLE_ONE_SECOND / (sample_rate))

#define TIME_TO_PCM_SAMPLES(time, sample_rate) \
    (((u64)time * sample_rate / PCM_SAMPLE_ONE_SECOND) + (((u64)time * sample_rate) % PCM_SAMPLE_ONE_SECOND == 0 ? 0 : 1))

extern spinlock_t g_stream_lock;

static inline void jlstream_lock()
{
    //spin_lock(&g_stream_lock);
    local_irq_disable();
}

static inline void jlstream_unlock()
{
    //spin_unlock(&g_stream_lock);
    local_irq_enable();
}

int jlstream_event_notify(enum stream_event, int arg);



/*
 * 申请一个指定长度的frame
 */
struct stream_frame *jlstream_get_frame(struct stream_oport *, u16 len);

/*
 * 将frame推送给后级节点
 */
void jlstream_push_frame(struct stream_oport *, struct stream_frame *);

/*
 * 获取一个前级推送的frame
 */
struct stream_frame *jlstream_pull_frame(struct stream_iport *, struct stream_note *);

/*
 * 从前级推送的frame中读取指定长度数据
 */
int jlstream_pull_data(struct stream_iport *iport, u32 fpos, u8 *buf, int len);


/*
 * 释放已经处理完数据的frame
 */
void jlstream_free_frame(struct stream_frame *frame);

/*
 * 释放所有缓存的frame
 */
void jlstream_free_iport_frames(struct stream_iport *iport);



void jlstream_frame_bypass(struct stream_iport *iport, struct stream_note *note);


int jlstream_get_iport_data_len(struct stream_iport *iport);

int jlstream_get_iport_delay(struct stream_iport *iport);

int jlstream_get_iport_frame_num(struct stream_iport *iport);

void jlstream_try_run_by_snode(struct stream_node *node);

int stream_node_ioctl(struct stream_node *node, u16 uuid, int cmd, int arg);

int jlstream_node_ioctl(struct jlstream *stream, u16 uuid, int cmd, int arg);

int jlstream_ioctl(struct jlstream *jlstream, int cmd, int arg);

int jlstream_iport_ioctl(struct stream_iport *iport, int cmd, int arg);


struct jlstream *jlstream_for_node(struct stream_node *node);

int jlstream_read_node_data(u16 uuid, u8 subid, u8 *buf);
int jlstream_read_node_port_data_wide(u16 uuid, u8 subid, u8 *buf);

int jlstream_read_stream_crc();

int jlstream_get_delay(struct jlstream *stream);


/*
 * 数据流创建和控制接口
 */
struct jlstream *jlstream_pipeline_parse(u16 pipeline, u16 source_uuid);
struct jlstream *jlstream_pipeline_parse_by_node_name(u16 pipeline, const char *node_name);

void jlstream_set_scene(struct jlstream *stream, enum stream_scene scene);

void jlstream_set_coexist(struct jlstream *stream, enum stream_coexist coexist);

void jlstream_set_callback(struct jlstream *stream, void *arg,
                           void (*callback)(void *, int));


/*
 * 设置文件句柄和文件操作接口
 */
int jlstream_set_dec_file(struct jlstream *stream, void *file_hdl,
                          const struct stream_file_ops *ops);

int jlstream_set_enc_file(struct jlstream *stream, void *file_hdl,
                          const struct stream_file_ops *ops);
/*
 * 启动数据流
 */
int jlstream_start(struct jlstream *stream);


/*
 * 播放/暂停切换接口,fade_msec 为淡入/淡出时间
 */
int jlstream_pp_toggle(struct jlstream *stream, u16 fade_msec);


/*
 *停止数据流,fade_msec 为淡出时间
 */
void jlstream_stop(struct jlstream *stream, u16 fade_msec);


/*
 *释放数据流
 */
void jlstream_release(struct jlstream *stream);



/*
 * 获取节点和设置节点参数接口
 */
void *jlstream_get_node(u16 node_uuid, const char *name);

int jlstream_set_node_param_s(void *node, void *param, u16 param_len);

int jlstream_get_node_param_s(void *node, void *param, u16 param_len);

void jlstream_put_node(void *);

int jlstream_set_node_param(u16 node_uuid, const char *name, void *param, u16 param_len);

int jlstream_get_node_param(u16 node_uuid, const char *name, void *param, u16 param_len);




/*
 * 数据淡入淡出接口
 *
 */
struct jlstream_fade {
    u8 in;
    u8 out;
    u8 channel;
    s16 step;
    s16 value;
};

/*
 * dir 0:淡出,  1:淡入
 */
void jlstream_fade_init(struct jlstream_fade *fade, int dir, u32 sample_rate,
                        u8 channel, u16 msec);

enum stream_fade_result {
    STREAM_FADE_IN,
    STREAM_FADE_OUT,
    STREAM_FADE_IN_END,
    STREAM_FADE_OUT_END,
    STREAM_FADE_END,
};

enum stream_fade_result jlstream_fade_data(struct jlstream_fade *fade, u8 *data, int len);

int jlstream_fade_out(int value, s16 step, s16 *data, int len, u8 channel);



void jlstream_del_node_from_thread(struct stream_node *node);

int jlstream_add_node_2_thread(struct stream_node *node, const char *task_name);

RESFILE *jlstream_file_open();
void jlstream_global_lock();
void jlstream_global_unlock();
bool jlstream_global_locked();

int jlstream_global_pause();
int jlstream_global_resume();

#endif

