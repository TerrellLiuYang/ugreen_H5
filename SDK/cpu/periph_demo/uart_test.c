#include "system/includes.h"
#include "uart.h"

static void uart_irq_func(int uart_num, enum uart_event event)
{
    if (event & UART_EVENT_TX_DONE) {
        printf("uart[%d] tx done", uart_num);
    }

    if (event & UART_EVENT_RX_DATA) {
        printf("uart[%d] rx data", uart_num);
    }

    if (event & UART_EVENT_RX_FIFO_OVF) {
        printf("uart[%d] rx fifo ovf", uart_num);
    }
}

struct uart_frame {
    u16 crc;
    u16 length;
    u8 data[0];
};
void uart_sync_demo(void *p)
{
    struct uart_config config = {
        .baud_rate = 2000000,
        .tx_pin = IO_PORTA_01,
        .rx_pin = IO_PORTA_02,
        .parity = UART_PARITY_DISABLE,
    };

    void *uart_rx_ptr = dma_malloc(768);

    struct uart_dma_config dma = {
        .rx_timeout_thresh = 100,
        .frame_size = 32,
        .event_mask = UART_EVENT_RX_DATA | UART_EVENT_RX_FIFO_OVF,
        .irq_callback = uart_irq_func,
        .rx_cbuffer = uart_rx_ptr,
        .rx_cbuffer_size = 768,
    };

    int r = uart_init(1, &config);
    if (r < 0) {
        printf("init error %d", r);
    }
    uart_dma_init(1, &dma);

    uart_dump();
    printf("uart init ok\n");
    struct uart_frame *frame = (struct uart_frame *)dma_malloc(512);

    while (1) {
        r = uart_recv_blocking(1, frame, 512, 10);
        r = uart_send_blocking(1, frame, r, 20);
    }
}
void uart_demo()
{
    int err = task_create(uart_sync_demo, NULL, "periph_demo");
    if (err != OS_NO_ERR) {
        r_printf("creat fail %x\n", err);
    }
}
