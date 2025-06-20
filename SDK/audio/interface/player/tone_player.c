#include "tone_player.h"
#include "fs/resfile.h"
#include "os/os_api.h"
#include "system/init.h"
#include "decoder_node.h"
#include "jldemuxer.h"


extern const struct decoder_plug_ops decoder_plug_begin[];
extern const struct decoder_plug_ops decoder_plug_end[];
const struct stream_file_ops tone_file_ops;

static u8 g_player_id = 0;
static OS_MUTEX g_tone_mutex;
static struct list_head g_head = LIST_HEAD_INIT(g_head);

static int tone_player_start(struct tone_player *);
static int tone_file_close(void *file);

__attribute__((weak))
void tone_event_to_user(int event, u16 fname_uuid)
{

}

static u32 JBHash(u8 *data, int len)
{
    u32 hash = 5381;

    while (len--) {
        hash += (hash << 5) + (*data++);
    }

    return hash;
}

static void *open_next_file(struct tone_player *player)
{
    if (player->next_file) {
        void *file = player->next_file;
        player->next_file = NULL;
        return file;
    }

    int index = ++player->index;

    do {
        if (player->file_name_list[index]) {
            char file_path[48];
            strcpy(file_path, FLASH_RES_PATH);
            strcpy(file_path + strlen(FLASH_RES_PATH), player->file_name_list[index]);
            void *file = resfile_open(file_path);
            if (file) {
                player->index = index;
                return file;
            }
            index++;
        } else {
            player->index = 0;
            break;
        }
    } while (index != player->index);

    return NULL;
}

void tone_player_free(struct tone_player *player)
{
    int fname_uuid = player->fname_uuid;
    void *cb_priv = player->priv;
    tone_player_cb_t callback = player->callback;

    if (--player->ref == 0) {
        tone_file_close(player);
        free(player);
    }
    if (callback) {
        callback(cb_priv,  STREAM_EVENT_STOP);
    } else {
        tone_event_to_user(STREAM_EVENT_STOP, fname_uuid);
    }
}

static void tone_player_callback(void *_player_id, int event)
{
    void *file = NULL;
    struct tone_player *player;

    printf("tone_callback: %x, %d\n", event, (u8)_player_id);

    switch (event) {
    case STREAM_EVENT_START:
        if (list_empty(&g_head)) {          //先判断是否为空防止触发异常
            break;
        }
        os_mutex_pend(&g_tone_mutex, 0);
        player = list_first_entry(&g_head, struct tone_player, entry);
        if (player->player_id == (u8)_player_id) {
            if (player->callback) {
                player->callback(player->priv,  STREAM_EVENT_START);
            }
        }
        os_mutex_post(&g_tone_mutex);
        break;
    case STREAM_EVENT_PREEMPTED:
    case STREAM_EVENT_NEGOTIATE_FAILD:
    /*
     * 提示音被抢占或者参数协商失败,直接结束播放
     */
    case STREAM_EVENT_STOP:
        if (list_empty(&g_head)) {          //先判断是否为空防止触发异常
            break;
        }
        os_mutex_pend(&g_tone_mutex, 0);
        player = list_first_entry(&g_head, struct tone_player, entry);
        if (player->player_id != (u8)_player_id) {
            os_mutex_post(&g_tone_mutex);
            printf("player_id_not_match: %d\n", player->player_id);
            break;
        }

        if (event == STREAM_EVENT_STOP) {
            /*
             * 打开文件列表的下一个文件,重新启动解码
             */
            while (1) {
                file = open_next_file(player);
                if (!file) {
                    break;
                }
                resfile_close(player->file);
                player->file = file;
                int err = jlstream_start(player->stream);
                if (err == 0) {
                    break;
                }
            }
        }

        if (!file) {
            jlstream_release(player->stream);

            list_del(&player->entry);
            if (!list_empty(&g_head)) {
                struct tone_player *p;
                p = list_first_entry(&g_head, struct tone_player, entry);
                tone_player_start(p);
            }

            tone_player_free(player);
        }

        os_mutex_post(&g_tone_mutex);
        break;
    }
}

static int tone_file_read(void *file, u8 *buf, int len)
{
    int offset = 0;
    struct tone_player *player = (struct tone_player *)file;

    while (len) {
        if (!player->file) {
            break;
        }
        int rlen = resfile_read(player->file, buf + offset, len);
        if (rlen < 0) {
            break;
        }
        offset += rlen;
        if ((len -= rlen) == 0) {
            break;
        }
        if (player->scene == STREAM_SCENE_RING) {
            if (player->coding_type == AUDIO_CODING_WTGV2) {
                //WTS拼接提示音时，去掉头1byte
                resfile_seek(player->file, 1, RESFILE_SEEK_SET);
            } else {
                resfile_seek(player->file, 0, RESFILE_SEEK_SET);
                if (player->coding_type == AUDIO_CODING_MP3) {
                    break;
                }
            }
            continue;
        }
        if (player->next_file) {
            break;
        }

        /*
         * 打开文件列表中的下一个文件,检查format是否和当前文件相同
         * 如果相同就继读下一个文件, 可以保证TWS同步播放文件列表情况下的音频同步
         */
        int index = player->index;
        void *file = open_next_file(player);
        if (!file) {
            player->index = index;
            break;
        }
        char name[16];
        resfile_get_name(file, name, 16);
        struct stream_file_ops file_ops = {
            .read       = (int (*)(void *, u8 *, int))resfile_read,
            .seek       = (int (*)(void *, int, int))resfile_seek,
        };
        struct stream_file_info file_info = {
            .file   = file,
            .fname  = name,
            .ops    = &file_ops,
        };
        struct stream_fmt fmt;
        int err = jldemuxer_get_tone_file_fmt(&file_info, &fmt);
        if (err == 0) {
            if (player->coding_type == fmt.coding_type &&
                player->sample_rate == fmt.sample_rate &&
                player->channel_mode == fmt.channel_mode) {
                resfile_close(player->file);
                player->file = file;
                if (player->coding_type == AUDIO_CODING_WTGV2) {
                    //WTS拼接提示音时，去掉头1byte
                    resfile_seek(player->file, 1, RESFILE_SEEK_SET);
                }
                continue;
            }
        }
        player->next_file = file;
        break;
    }

    return offset;
}

static int tone_file_seek(void *file, int offset, int fromwhere)
{
    struct tone_player *player = (struct tone_player *)file;

    return resfile_seek(player->file, offset, fromwhere);
}

static int tone_file_close(void *file)
{
    struct tone_player *player = (struct tone_player *)file;

    if (player->file) {
        resfile_close(player->file);
        player->file = NULL;
    }
    if (player->next_file) {
        resfile_close(player->next_file);
        player->next_file = NULL;
    }
    return 0;
}

static int tone_file_get_fmt(void *file, struct stream_fmt *fmt)
{
    struct tone_player *player = (struct tone_player *)file;

    char name[16];
    resfile_get_name(player->file, name, 16);

    struct stream_file_info file_info = {
        .file   = player,
        .fname  = name,
        .ops    = &tone_file_ops,
    };
    int err = jldemuxer_get_tone_file_fmt(&file_info, fmt);
    if (err) {
        player->coding_type = AUDIO_CODING_UNKNOW;
        return err;
    }

    player->coding_type = fmt->coding_type;
    player->sample_rate = fmt->sample_rate;
    player->channel_mode = fmt->channel_mode;

    return 0;
}

const struct stream_file_ops tone_file_ops = {
    .read       = tone_file_read,
    .seek       = tone_file_seek,
    .close      = tone_file_close,
    .get_fmt    = tone_file_get_fmt,
};


static int tone_player_start(struct tone_player *player)
{
    int err = -EINVAL;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"tone");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_TONE);
    if (!player->stream) {
        goto __exit0;
    }
    int player_id = player->player_id;
    jlstream_set_callback(player->stream, (void *)player_id, tone_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);

    if (player->callback) {
        err = player->callback(player->priv, STREAM_EVENT_INIT);
        if (err) {
            goto __exit1;
        }
    }
    jlstream_set_dec_file(player->stream, player, &tone_file_ops);

    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    list_del(&player->entry);
    tone_player_free(player);
    return err;
}


int tone_player_init(struct tone_player *player, const char *file_name)
{
    void *file = NULL;
    int fname_uuid = 0;

    player->ref = 1;
    if (file_name) {
        char file_path[64];
        strcpy(file_path, FLASH_RES_PATH);
        strcpy(file_path + strlen(FLASH_RES_PATH), file_name);
        file = resfile_open(file_path);
        if (!file) {
            printf("tone_player_faild: %s\n", file_name);
            return -EINVAL;
        }
        fname_uuid = JBHash((u8 *)file_name, strlen(file_name));
        printf("tone_player: %s\n", file_name);
    }
    player->file        = file;
    player->scene       = STREAM_SCENE_TONE;
    player->player_id   = g_player_id++;
    player->fname_uuid  = fname_uuid;
    player->coexist     = STREAM_COEXIST_AUTO;
    INIT_LIST_HEAD(&player->entry);

    return 0;
}

static struct tone_player *tone_player_create(const char *file_name)
{
    struct tone_player *player;

    player = zalloc(sizeof(*player));
    if (!player) {
        return NULL;
    }

    int err = tone_player_init(player, file_name);
    if (err) {
        tone_player_free(player);
        return NULL;
    }

    return player;
}

int tone_player_add(struct tone_player *player)
{
    os_mutex_pend(&g_tone_mutex, 0);

    if (list_empty(&g_head)) {
        int err = tone_player_start(player);
        if (err) {
            os_mutex_post(&g_tone_mutex);
            return err;
        }
    }
    list_add_tail(&player->entry, &g_head);

    os_mutex_post(&g_tone_mutex);
    return 0;
}

int play_tone_file(const char *file_name)
{
    struct tone_player *player;

    player = tone_player_create(file_name);
    if (!player) {
        return -ENOMEM;
    }
    return tone_player_add(player);
}

int play_tone_file_callback(const char *file_name, void *priv, tone_player_cb_t callback)
{
    struct tone_player *player;

    player = tone_player_create(file_name);
    if (!player) {
        callback(priv, STREAM_EVENT_STOP);
        return -ENOMEM;
    }
    player->priv        = priv;
    player->callback    = callback;

    return tone_player_add(player);
}

static struct tone_player *tone_files_create(const char *const file_name[], u8 file_num)
{
    struct tone_player *player;

    if (file_num > MAX_FILE_NUM) {
        return NULL;
    }

    player = tone_player_create(file_name[0]);
    if (!player) {
        return NULL;
    }
    for (int i = 0; i < file_num; i++) {
        player->file_name_list[i] = file_name[i];
    }
    return player;
}

int play_tone_files(const char *const file_name[], u8 file_num)
{
    struct tone_player *player;

    player = tone_files_create(file_name, file_num);
    if (!player) {
        return -EFAULT;
    }
    return tone_player_add(player);
}

int play_tone_files_alone(const char *const file_name[], u8 file_num)
{
    struct tone_player *player;

    player = tone_files_create(file_name, file_num);
    if (!player) {
        return -EFAULT;
    }
    player->coexist = STREAM_COEXIST_DISABLE;
    return tone_player_add(player);
}

int play_tone_files_callback(const char *const file_name[], u8 file_num,
                             void *priv, tone_player_cb_t callback)
{

    struct tone_player *player;

    player = tone_files_create(file_name, file_num);
    if (!player) {
        callback(priv, STREAM_EVENT_STOP);
        return -EFAULT;
    }
    player->priv = priv;
    player->callback = callback;
    return tone_player_add(player);
}

int play_tone_files_alone_callback(const char *const file_name[], u8 file_num,
                                   void *priv, tone_player_cb_t callback)
{

    struct tone_player *player;

    player = tone_files_create(file_name, file_num);
    if (!player) {
        callback(priv, STREAM_EVENT_STOP);
        return -EFAULT;
    }
    player->priv = priv;
    player->callback = callback;
    player->coexist = STREAM_COEXIST_DISABLE;
    return tone_player_add(player);
}

int play_tone_file_alone(const char *file_name)
{
    struct tone_player *player;

    player = tone_player_create(file_name);
    if (!player) {
        return -ENOMEM;
    }
    player->coexist = STREAM_COEXIST_DISABLE;

    return tone_player_add(player);
}


int play_tone_file_alone_callback(const char *file_name, void *priv,
                                  tone_player_cb_t callback)
{
    struct tone_player *player;

    player = tone_player_create(file_name);
    if (!player) {
        callback(priv, STREAM_EVENT_STOP);
        return -ENOMEM;
    }
    player->coexist     = STREAM_COEXIST_DISABLE;
    player->priv        = priv;
    player->callback    = callback;

    return tone_player_add(player);
}

int tone_player_runing()
{
    local_irq_disable();
    int ret = list_empty(&g_head) ? 0 : 1;
    local_irq_enable();
    return ret;
}

void tone_player_stop()
{
    struct tone_player *player, *n;

    os_mutex_pend(&g_tone_mutex, 0);

    list_for_each_entry_safe(player, n, &g_head, entry) {
        __list_del_entry(&player->entry);
        if (player->stream) {
            jlstream_stop(player->stream, 50);
            jlstream_release(player->stream);
        }
        tone_player_free(player);
    }

    os_mutex_post(&g_tone_mutex);
}

u16 tone_player_get_fname_uuid(const char *fname)
{
    return JBHash((u8 *)fname, strlen(fname));
}


static int __tone_player_init()
{
    os_mutex_create(&g_tone_mutex);
    return 0;
}
__initcall(__tone_player_init);


