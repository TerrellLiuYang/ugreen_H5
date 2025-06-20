#include "app_key_dut.h"
#include "app_msg.h"
#include "system/spinlock.h"
#include "app_config.h"

#if TCFG_APP_KEY_DUT_ENABLE

#if 1
#define key_dut_log     printf
#else
#define key_dut_log
#endif

/* 最大30字节，因为每次给上位机最大只能发送30字节数据)
 * 要求：建议上位机查询间隔小于1s，
 *       以保证查询间隔内出现的按键事件的字符串不大于KEY_LIST_ZISE*/
#define KEY_LIST_ZISE 30

typedef struct {
    u8 index; //记录当前存储的位置
    char key_list[KEY_LIST_ZISE];  //存储按键事件
    OS_MUTEX mutex;
} key_dut_hdl_t;
key_dut_hdl_t *key_dut_hdl = NULL;

/*开始按键产测*/
int app_key_dut_start()
{
    key_dut_log("app_key_dut_start");
    if (key_dut_hdl) {
        key_dut_log("app key dut alreadly open !!!");
        return 0;
    }
    key_dut_hdl_t *hdl = zalloc(sizeof(key_dut_hdl_t));
    key_dut_log("hdl zise : %d", (int)sizeof(key_dut_hdl_t));
    if (hdl == NULL) {
        key_dut_log("key dut malloc fail !!!");
        return -1;
    }
    os_mutex_create(&hdl->mutex);
    hdl->index = 0;
    key_dut_hdl = hdl;
    return 0;
}

/*结束按键产测*/
int app_key_dut_stop()
{
    key_dut_hdl_t *hdl = (key_dut_hdl_t *)key_dut_hdl;
    key_dut_log("app_key_dut_stop");
    if (hdl) {
        free(hdl);
        key_dut_hdl = NULL;
    }
    return 0;
}

/* 获取已经按下的按键列表
 * 要求：上位机查询间隔小于1s，
 *       以保证查询间隔内出现的按键事件的字符串不大于KEY_LIST_ZISE*/
int get_key_press_list(void *key_list)
{
    key_dut_hdl_t *hdl = (key_dut_hdl_t *)key_dut_hdl;
    if (hdl) {
        /*记录按键列表字符长度*/
        int key_list_len = hdl->index;
        os_mutex_pend(&hdl->mutex, 0);
        /*拷贝按键事件列表*/
        memcpy(key_list, hdl->key_list, key_list_len);
        /*将按键事件列表清零*/
        memset(hdl->key_list, '\0', (int)sizeof(hdl->key_list));
        hdl->index = 0;
        os_mutex_post(&hdl->mutex);

        return key_list_len;
    }
    return 0;
}

/*发送按键事件到key dut*/
int app_key_dut_send(int *msg, char *key_name)
{
    key_dut_hdl_t *hdl = (key_dut_hdl_t *)key_dut_hdl;
    /*判断是否已经开按键产测*/
    if (!hdl) {
        return 0;
    }
    /*判断是否按键产生的事件*/
    if (!APP_MSG_FROM_KEY(msg[0])) {
        return 0;
    }
    key_dut_log("[key_dut] key_event : %s", key_name);

    /*字符串长度(包括结束符'\0')*/
    int key_name_len = strlen(key_name) + 1;
    /* ASSERT((hdl->index + key_name_len) <= KEY_LIST_ZISE); */

    os_mutex_pend(&hdl->mutex, 0);
    if ((hdl->index + key_name_len) > KEY_LIST_ZISE) {
        printf("[error][key_dut]  key_name_list size over %d !!!", KEY_LIST_ZISE);
        memset(hdl->key_list, '\0', (int)sizeof(hdl->key_list));
        hdl->index = 0;
    }

    /*将按键事件名字存起来*/
    memcpy(&hdl->key_list[hdl->index], key_name, key_name_len);
    /* key_dut_log("[key_dut] key_name : %s", &hdl->key_list[hdl->index]); */
    /*记录下一次存放起始位置*/
    hdl->index += key_name_len;
    os_mutex_post(&hdl->mutex);

    return 0;
}

#endif /*TCFG_APP_KEY_DUT_ENABLE*/
