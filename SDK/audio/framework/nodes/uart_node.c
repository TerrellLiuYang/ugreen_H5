#include "jlstream.h"
#include "uart.h"
#include "gpio_config.h"
#include "effects/effects_adj.h"
#include "app_config.h"

#ifdef TCFG_UART_NODE_ENABLE

struct uart_node_data {
    u16 port_uuid;
    u32 baudrate;
} __attribute__((packed));

struct uart_node_hdl {
    int uart;
};


static void uart_node_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct uart_node_hdl *hdl = (struct uart_node_hdl *)iport->private_data;
    struct stream_frame *frame;
    struct stream_node *node = iport->node;

    while (1) {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            break;
        }
        if (hdl->uart != -1) {
            uart_send_bytes(hdl->uart, frame->data, frame->len);
        }
        if (node->oport) {
            jlstream_push_frame(node->oport, frame);
        } else {
            jlstream_free_frame(frame);
        }
    }
}

static int uart_node_bind(struct stream_node *node, u16 uuid)
{
    struct uart_node_hdl *hdl = zalloc(sizeof(*hdl));
    node->type = NODE_TYPE_BYPASS;
    node->private_data = hdl;
    return 0;
}

static void uart_node_open_iport(struct stream_iport *iport)
{
    iport->private_data = iport->node->private_data;
    iport->handle_frame = uart_node_handle_frame;
}

static void uart_ioc_start(struct uart_node_hdl *hdl, struct stream_node *node)
{
    struct uart_node_data config;

    /*
     *获取配置文件内的参数,及名字
     * */
    if (jlstream_read_node_data_new(node->uuid, node->subid, (void *)&config, NULL)) {
        printf("%s, read node data err\n", __FUNCTION__);
        return;
    }

    u32 tx_pin = uuid2gpio(config.port_uuid);
    struct uart_config ut = {
        .baud_rate = config.baudrate,
        .tx_pin = tx_pin,
        .rx_pin = -1,
    };

    hdl->uart = uart_init(-1, &ut);
    if (hdl->uart < 0) {
        printf("open uart dev err\n");
        hdl->uart  = -1;
    }
    struct uart_dma_config dma_config = {
        .event_mask = UART_EVENT_TX_DONE,
    };
    uart_dma_init(hdl->uart, &dma_config);
}

static int uart_node_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    struct uart_node_hdl *hdl = (struct uart_node_hdl *)iport->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        uart_node_open_iport(iport);
        break;
    case NODE_IOC_START:
        uart_ioc_start(hdl, iport->node);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        if (hdl->uart != -1) {
            uart_deinit(hdl->uart);
            hdl->uart = -1 ;
        }
        break;
    }

    return 0;
}

static void uart_node_release(struct stream_node *node)
{
    free(node->private_data);
}


REGISTER_STREAM_NODE_ADAPTER(uart_node_adapter) = {
    .name       = "uart_dump",
    .uuid       = NODE_UUID_UART_DUMP,
    .bind       = uart_node_bind,
    .ioctl      = uart_node_ioctl,
    .release    = uart_node_release,
};

#endif

