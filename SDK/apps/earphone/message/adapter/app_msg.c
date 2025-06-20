#include "app_msg.h"
#include "key_driver.h"



void app_send_message(int _msg, int arg)
{
    int msg[2];

    msg[0] = _msg;
    msg[1] = arg;
    os_taskq_post_type("app_core", MSG_FROM_APP, 2, msg);
}


void app_send_message_from(int from, int argc, int *msg)
{
    os_taskq_post_type("app_core", from, (argc + 3) / 4, msg);
}


int app_key_event_remap(int *_event)
{
    struct key_event *key = (struct key_event *)_event;

    return APP_KEY_MSG_REMAP(key->value, key->event);

    return 0;
}
