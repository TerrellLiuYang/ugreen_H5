#include "system/init.h"
#include "classic/tws_api.h"
#include "app_msg.h"




static void send_tws_event(int argc, u8 *argv)
{
    ASSERT(((u32)argv & 0x3) == 0);
    app_send_message_from(MSG_FROM_TWS, argc, (int *)argv);
}



static int tws_event_callback_init()
{
    tws_api_set_event_handler(send_tws_event);
    return 0;
}
__initcall(tws_event_callback_init);
