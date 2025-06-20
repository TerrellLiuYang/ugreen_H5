#include "generic/typedef.h"
#include "gpio.h"
#include "asm/power/p33.h"
#include "asm/hwi.h"
#include "gpio.h"
#include "asm/charge.h"
#include "asm/chargestore.h"
#include "update.h"
#include "app_config.h"
#include "clock.h"
#include "app_msg.h"
#if (TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE)

struct chargestore_handle {
    const struct chargestore_platform_data *data;
    JL_UART_TypeDef *UART;
    u32 baudrate;
};
#define DMA_ISR_LEN 64
#define DMA_BUF_LEN 64
#define __this  (&hdl)
static struct chargestore_handle hdl;
u8 uart_dma_buf[DMA_BUF_LEN] __attribute__((aligned(4)));
volatile u8 send_busy;

//串口时钟和串口超时时钟是分开的
#define UART_SRC_CLK    clk_get("uart")
#define UART_OT_CLK     clk_get("lsb")

enum {
    UPGRADE_NULL = 0,
    UPGRADE_USB_HARD_KEY,
    UPGRADE_USB_SOFTKEY,
    UPGRADE_UART_SOFT_KEY,
    UPGRADE_UART_ONE_WIRE_HARD_KEY,
};
extern void charge_reset_pb5_pd_status(void);
extern void nvram_set_boot_state(u32 state);
void chargestore_set_update_ram(void)
{
    //需要补充设置ram
    int tmp;
    __asm__ volatile("%0 =icfg" : "=r"(tmp));
    tmp &= ~(3 << 8);
    __asm__ volatile("icfg = %0" :: "r"(tmp));//GIE1
    local_irq_disable();
    nvram_set_boot_state(UPGRADE_UART_SOFT_KEY);
}

void __attribute__((weak)) chargestore_data_deal(u8 cmd, u8 *data, u8 len)
{

}

void __attribute__((weak)) chargestore_uart_data_deal(u8 *data, u8 len)
{

}

static u8 chargestore_get_f95_det_res(u32 equ_res)
{
    u8 det_res = (equ_res + 50) / 100;
    if (det_res > 0) {
        det_res -= 1;
    }
    if (det_res > 0x0f) {
        det_res = 0x0f;
    }
    return det_res;
}

//br30(PMU数据未知,IC还未给出测试数据,说是和BR23基本一致)
u8 chargestore_get_det_level(u8 chip_type)
{
    u32 res = 1600;
    switch (chip_type) {
    case TYPE_F95:
        if (IS_L5V_LOAD_EN()) {
            res = (GET_L5V_RES_DET_S_SEL() + 1) * 50;
        }
        return chargestore_get_f95_det_res(res);
    case TYPE_NORMAL:
    default:
        return 0x0f;
    }
}

___interrupt
static void uart_isr(void)
{
    u16 i;
    u32 rx_len = 0;
    extern USER_VAR user_var;
       
    
    if ((__this->UART->CON0 & BIT(2)) && (__this->UART->CON0 & BIT(15))) {
        __this->UART->CON0 |= BIT(13);
        send_busy = 0;
        chargestore_data_deal(CMD_COMPLETE, NULL, 0);
    }
    if ((__this->UART->CON0 & BIT(3)) && (__this->UART->CON0 & BIT(14))) {
        __this->UART->CON0 |= BIT(12);//清RX PND
        // chargestore_data_deal(CMD_RECVDATA, uart_dma_buf, DMA_ISR_LEN);
        // putchar('~');
        // put_buf(uart_dma_buf, DMA_ISR_LEN);
        if (ldoin_uart_data_callback(uart_dma_buf, DMA_ISR_LEN) == 0) {
            chargestore_data_deal(CMD_RECVDATA, uart_dma_buf, DMA_ISR_LEN);
        }
        memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf));
        __this->UART->RXSADR = (u32)uart_dma_buf;
        __this->UART->RXEADR = (u32)(uart_dma_buf + DMA_BUF_LEN);
        __this->UART->RXCNT = DMA_ISR_LEN;
    }
    if ((__this->UART->CON0 & BIT(5)) && (__this->UART->CON0 & BIT(11))) {
        //OTCNT PND
        __this->UART->CON0 |= BIT(7);//DMA模式
        __this->UART->CON0 |= BIT(10);//清OTCNT PND
        asm volatile("nop");
        rx_len = __this->UART->HRXCNT;//读当前串口接收数据的个数
        __this->UART->CON0 |= BIT(12);//清RX PND(这里的顺序不能改变，这里要清一次)
        // chargestore_data_deal(CMD_RECVDATA, uart_dma_buf, rx_len);
        // chargestore_uart_data_deal(uart_dma_buf, rx_len);
        // putchar('#');
        // put_buf(uart_dma_buf, rx_len);
        if (ldoin_uart_data_callback(uart_dma_buf, rx_len) == 0) {
            chargestore_data_deal(CMD_RECVDATA, uart_dma_buf, rx_len);
            chargestore_uart_data_deal(uart_dma_buf, rx_len);
        }
        memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf));
        __this->UART->RXSADR = (u32)uart_dma_buf;
        __this->UART->RXEADR = (u32)(uart_dma_buf + DMA_BUF_LEN);
        __this->UART->RXCNT = DMA_ISR_LEN;
    }
}

void chargestore_write(u8 *data, u8 len)
{
    u32 data_addr = (u32)data;
    if (data_addr % 4) {//4byte对齐
        ASSERT(0, "%s: unaligned accesses!", __func__);
    }
    send_busy = 1;
    __this->UART->TXADR = data_addr;
    __this->UART->TXCNT = len;
}

void chargestore_open(u8 mode)
{
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);
    if (mode == MODE_RECVDATA) {
#if (TCFG_CHARGESTORE_PORT == IO_PORT_LDOIN)
putchar('1');
        charge_set_ldo5v_detect_stop(0);
#endif
        gpio_set_mode(IO_PORT_SPILT(__this->data->io_port), PORT_INPUT_FLOATING);
        __this->UART->CON1 &= ~BIT(4);
        if (__this->UART == JL_UART0) {
            gpio_set_function(IO_PORT_SPILT(__this->data->io_port), PORT_FUNC_UART0_RX);
        } else if (__this->UART == JL_UART1) {
            gpio_set_function(IO_PORT_SPILT(__this->data->io_port), PORT_FUNC_UART1_RX);
        } else {
            gpio_set_function(IO_PORT_SPILT(__this->data->io_port), PORT_FUNC_UART2_RX);
        }
        memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf));
        __this->UART->RXSADR = (u32)uart_dma_buf;
        __this->UART->RXEADR = (u32)(uart_dma_buf + DMA_BUF_LEN);
        __this->UART->RXCNT = DMA_ISR_LEN;
        __this->UART->CON0 |= BIT(6) | BIT(5) | BIT(3);
    } else {
#if (TCFG_CHARGESTORE_PORT == IO_PORT_LDOIN)
    putchar('2');
        charge_set_ldo5v_detect_stop(1);
#endif
        gpio_set_mode(IO_PORT_SPILT(__this->data->io_port), PORT_OUTPUT_HIGH);
        gpio_set_drive_strength(IO_PORT_SPILT(__this->data->io_port), PORT_DRIVE_STRENGT_8p0mA);
        __this->UART->CON1 |= BIT(4);
        if (__this->UART == JL_UART0) {
            gpio_set_function(IO_PORT_SPILT(__this->data->io_port), PORT_FUNC_UART0_TX);
        } else if (__this->UART == JL_UART1) {
            gpio_set_function(IO_PORT_SPILT(__this->data->io_port), PORT_FUNC_UART1_TX);
        } else {
            gpio_set_function(IO_PORT_SPILT(__this->data->io_port), PORT_FUNC_UART2_TX);
        }
        __this->UART->CON0 |= BIT(2);
    }
    __this->UART->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(1) | BIT(0);
}

void chargestore_close(void)
{
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10) | BIT(0);
    gpio_set_mode(IO_PORT_SPILT(__this->data->io_port), PORT_INPUT_FLOATING);
    gpio_set_drive_strength(IO_PORT_SPILT(__this->data->io_port), PORT_DRIVE_STRENGT_2p4mA);
    memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf));
#if (TCFG_CHARGESTORE_PORT == IO_PORT_LDOIN)
    putchar('3');
    charge_set_ldo5v_detect_stop(0);
#endif
}

void chargestore_set_baudrate(u32 baudrate)
{
    u32 uart_timeout;
    __this->baudrate = baudrate;
    uart_timeout = 20 * 1000000 / __this->baudrate;
    __this->UART->OTCNT = uart_timeout * (UART_OT_CLK / 1000000);
    __this->UART->BAUD = (UART_SRC_CLK / __this->baudrate) / 4 - 1;
}

extern void ldoin_uart_test_init(void *uart);
void chargestore_init(const struct chargestore_platform_data *data)
{
    u32 uart_timeout;
    __this->data = (struct chargestore_platform_data *)data;
    ASSERT(data);
    if (!(JL_UART0->CON0 & BIT(0))) {
        JL_UART0->CON0 = BIT(13) | BIT(12) | BIT(10);
        request_irq(IRQ_UART0_IDX, 2, uart_isr, 0);
        __this->UART = JL_UART0;
    } else if (!(JL_UART1->CON0 & BIT(0))) {
        JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
        request_irq(IRQ_UART1_IDX, 2, uart_isr, 0);
        __this->UART = JL_UART1;
    } else if (!(JL_UART2->CON0 & BIT(0))) {
        JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
        request_irq(IRQ_UART2_IDX, 2, uart_isr, 0);
        __this->UART = JL_UART2;
    } else {
        ASSERT(0, "uart all used!\n");
    }
    send_busy = 0;
    uart_timeout = 20 * 1000000 / __this->data->baudrate;
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10) | BIT(1) | BIT(0);//占用该串口,不被其他模块使用
    __this->UART->OTCNT = uart_timeout * (UART_OT_CLK / 1000000);
    __this->UART->BAUD = (UART_SRC_CLK / __this->data->baudrate) / 4 - 1;
    __this->baudrate = __this->data->baudrate;
    gpio_set_mode(IO_PORT_SPILT(__this->data->io_port), PORT_INPUT_FLOATING);
    ldoin_uart_test_init(__this->data);
}

static void clock_critical_enter(void)
{
}

static void clock_critical_exit(void)
{
    u32 uart_timeout;
    if (__this->UART == NULL) {
        return;
    }
    uart_timeout = 20 * 1000000 / __this->baudrate;
    __this->UART->OTCNT = uart_timeout * (UART_OT_CLK / 1000000);
    //__this->UART->BAUD = (UART_SRC_CLK / __this->baudrate) / 4 - 1;
}
CLOCK_CRITICAL_HANDLE_REG(chargestore, clock_critical_enter, clock_critical_exit)

#endif

