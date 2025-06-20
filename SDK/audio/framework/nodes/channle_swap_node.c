#include "jlstream.h"
#include "overlay_code.h"
#include "app_config.h"

#ifdef TCFG_CHANNEL_SWAP_NODE_ENABLE

static void channle_swap_node_run(s16 *ptr, int npoint)
{
    int tmp32_1;
    asm volatile(
        " 1: \n\t"
        " rep %[npoint] { \n\t"
        "   %[tmp32_1] = [%[ptr]] \n\t"
        "   h[%[ptr]++=2] = %[tmp32_1].h \n\t"
        "   h[%[ptr]++=2] = %[tmp32_1].l \n\t"
        " } \n\t"
        " if( %[npoint] != 0 ) goto 1b \n\t"
        :
        [ptr]"=&r"(ptr),
        [tmp32_1]"=&r"(tmp32_1),
        [npoint]"=&r"(npoint)
        :
        "0"(ptr),
        "1"(tmp32_1),
        "2"(npoint)
    );
}

static void channle_swap_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *frame;
    struct stream_node *node = iport->node;

    while (1) {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            break;
        }
        channle_swap_node_run((s16 *)frame->data, frame->len >> 2);	//执行时间 46us/4096 byte
        if (node->oport) {
            jlstream_push_frame(node->oport, frame);
        } else {
            jlstream_free_frame(frame);
        }
    }
}

/*节点预处理-在ioctl之前*/
static int channle_swap_adapter_bind(struct stream_node *node, u16 uuid)
{
    return 0;
}

static void channle_swap_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = channle_swap_handle_frame;
    iport->private_data = iport->node->private_data;
}

/*节点参数协商*/
static int channle_swap_ioc_negotiate(struct stream_iport *iport)
{
    return 0;
}

/*节点start函数*/
static void channle_swap_ioc_start(void)
{

}

/*节点stop函数*/
static void channle_swap_ioc_stop(void)
{

}

/*节点ioctl函数*/
static int channle_swap_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        channle_swap_ioc_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= channle_swap_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        channle_swap_ioc_start();
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        channle_swap_ioc_stop();
        break;
    }
    return ret;
}

/*节点用完释放函数*/
static void channle_swap_adapter_release(struct stream_node *node)
{

}

REGISTER_STREAM_NODE_ADAPTER(channle_swap_node_adapter) = {
    .name       = "channle_swap",
    .uuid       = NODE_UUID_CHANNLE_SWAP,
    .bind       = channle_swap_adapter_bind,
    .ioctl      = channle_swap_adapter_ioctl,
    .release    = channle_swap_adapter_release,
    //固定要求输出为双声道
    .ability_channel_in = 2,
    .ability_channel_out = 2,
};

#endif
