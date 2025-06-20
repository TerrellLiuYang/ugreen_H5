#include "app_msg.h"
#include "system/event.h"
#include "usb/usb_task.h"

struct _device_event {
    u8 event;
    int value;
};

void sdx_dev_event_to_user(u32 arg, u8 sdx_status, u8 sdx_index)
{

    int msg[3];

    msg[0] = arg;
    msg[1] = sdx_status;

    if (arg == (u32)DRIVER_EVENT_FROM_SD0) {
        msg[2] = (int)"sd0";
    } else {
        msg[2] = (int)"sd1";
    }

    printf("sd dev msg %x, %x, %x\n", msg[0], msg[1], msg[2]);


    os_taskq_post_type("app_core", MSG_FROM_DEVICE, 3, msg);

}

void usb_driver_event_to_user(u32 from, u32 event, void *arg)
{
    int msg[3] = {0};

    msg[0] = from;
    msg[1] = event;
    msg[2] = (int)arg;
    os_taskq_post_type("app_core", MSG_FROM_DEVICE, 3, msg);
}

void usb_driver_event_from_otg(u32 from, u32 event, void *arg)
{
    os_taskq_post_msg("usb_stack", 4, USBSTACK_OTG_MSG, from, event, arg);
}


