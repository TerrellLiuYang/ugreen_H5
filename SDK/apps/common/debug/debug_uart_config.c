#include "app_config.h"
#include "cpu/includes.h"
#include "gpio_config.h"

#if CONFIG_DEBUG_ENABLE || CONFIG_DEBUG_LITE_ENABLE

#define     DEBUG_UART_NUM  0
#define     MAX_DEBUG_FIFO  256

static u8 debug_uart_buf[2][MAX_DEBUG_FIFO];
static u32 tx_jiffies = 0;
static u16 pos = 0;
static u8 uart_buffer_index = 0;
static u8 uart_mode = 0;        //0:typical putbyte, 1:exception putbyte


#define     DEBUG_UART_DMA_EN   0
static void uart_irq(uart_dev uart_num, enum uart_event event)
{
    tx_jiffies = jiffies;
    uart_send_bytes(DEBUG_UART_NUM, debug_uart_buf[uart_buffer_index], pos);
    uart_buffer_index = !uart_buffer_index;
    pos = 0;
}
void debug_uart_init()
{
    struct uart_config debug_uart_config = {
        .baud_rate = TCFG_DEBUG_UART_BAUDRATE,
        .tx_pin = TCFG_DEBUG_UART_TX_PIN,
        .rx_pin = -1,
    };

    uart_init(DEBUG_UART_NUM, &debug_uart_config);

#if DEBUG_UART_DMA_EN
    struct uart_dma_config dma_config = {
        .event_mask = UART_EVENT_TX_DONE,
        .irq_callback = uart_irq,
    };
    uart_dma_init(DEBUG_UART_NUM, &dma_config);
#endif
}


static void __putbyte(char a)
{
#if DEBUG_UART_DMA_EN

    debug_uart_buf[uart_buffer_index][pos] = a;
    pos++;
    if ((jiffies - tx_jiffies > 10) || (pos == MAX_DEBUG_FIFO)) {
        tx_jiffies = jiffies;
        uart_wait_tx_idle(DEBUG_UART_NUM, 10);
        uart_send_bytes(DEBUG_UART_NUM, debug_uart_buf[uart_buffer_index], pos);
        uart_buffer_index = !uart_buffer_index;
        pos = 0;
    }

#else

    /* if(a == '\n'){                          */
    /*     uart_putbyte(DEBUG_UART_NUM, '\r'); */
    /* }                                       */

    uart_putbyte(DEBUG_UART_NUM, a);

#endif
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 通用打印putbyte函数，用于SDK调试
 *
 * @param a char
 */
/* ----------------------------------------------------------------------------*/
void putbyte(char a)
{
    if (uart_mode == 0) {
        __putbyte(a);
    }
}


/* --------------------------------------------------------------------------*/
/**
 * @brief 特俗场景putbyte函数，暂用于异常服务函数
 *          调用此函数后，通用putbyte函数会停用，防止打印冲突。
 * @param a char
 */
/* ----------------------------------------------------------------------------*/
//excpt默认使用内部打印函数，使用uart0，放RAM。
//若出现异常无打印情况，考虑打开此函数，放Flash。
#if 0
void excpt_putbyte(char a)
{
    uart_mode = 1;
    __putbyte(a);
}
#endif

#endif
