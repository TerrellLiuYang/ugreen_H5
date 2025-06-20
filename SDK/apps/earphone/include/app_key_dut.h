#ifndef __APP_KEY_DUT_H__
#define __APP_KEY_DUT_H__

#include "generic/typedef.h"

/*开始按键产测*/
int app_key_dut_start();

/*结束按键产测*/
int app_key_dut_stop();

/*获取已经按下的按键*/
int get_key_press_list(void *key_list);

/*发送按键事件到key dut*/
int app_key_dut_send(int *msg, char *key_name);

#endif /*__APP_KEY_DUT_H__*/
