#include "cpu/includes.h"
#include "asm/power_interface.h"
#include "system/includes.h"
#include "system/bank_switch.h"

#include "app_config.h"


#define LOG_TAG             "[SETUP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

extern void sys_timer_init(void);
extern void tick_timer_init(void);
extern void exception_irq_handler(void);
extern int __crc16_mutex_init();



#if 0
___interrupt
void exception_irq_handler(void)
{
    ___trig;

    exception_analyze();

    log_flush();
    while (1);
}
#endif



/*
 * 此函数在cpu0上电后首先被调用,负责初始化cpu内部模块
 *
 * 此函数返回后，操作系统才开始初始化并运行
 *
 */

#if 0
static void early_putchar(char a)
{
    if (a == '\n') {
        UT2_BUF = '\r';
        __asm_csync();
        while ((UT2_CON & BIT(15)) == 0);
    }
    UT2_BUF = a;
    __asm_csync();
    while ((UT2_CON & BIT(15)) == 0);
}

void early_puts(char *s)
{
    do {
        early_putchar(*s);
    } while (*(++s));
}
#endif

void cpu_assert_debug()
{
#if CONFIG_DEBUG_ENABLE
    log_flush();
    local_irq_disable();
    while (1);
#else
    P3_PCNT_SET0 = 0xac;
    cpu_reset();
#endif
}


void app_bank_init()
{
    /*request_irq(IRQ_SYSCALL_IDX, 3, bank_syscall_entry, 0);*/
    load_common_code();
}

u32 stack_magic[4] sec(.stack_magic);
u32 stack_magic0[4] sec(.stack_magic0);
void memory_init(void);

__attribute__((weak))
void app_main()
{
    while (1) {
        asm("idle");
    }
}

void setup_arch()
{
    //关闭所有timer的ie使能
    bit_clr_ie(IRQ_TIME0_IDX);
    bit_clr_ie(IRQ_TIME1_IDX);
    bit_clr_ie(IRQ_TIME2_IDX);
    bit_clr_ie(IRQ_TIME3_IDX);
    bit_clr_ie(IRQ_TIME4_IDX);
    bit_clr_ie(IRQ_TIME5_IDX);

    q32DSP(core_num())->PMU_CON1 &= ~BIT(8);

    //asm("trigger ");

    memory_init();
    memset(stack_magic, 0x5a, sizeof(stack_magic));
    memset(stack_magic0, 0x5a, sizeof(stack_magic0));

    //P11 系统必须提前打开
    p11_init();

    wdt_init(WDT_8S);
    /* wdt_close(); */

    clk_voltage_init(TCFG_CLOCK_MODE, SYSVDD_VOL_SEL_126V);
#if (TCFG_MAX_LIMIT_SYS_CLOCK==MAX_LIMIT_SYS_CLOCK_160M)
    clk_early_init(PLL_REF_XOSC_DIFF, TCFG_CLOCK_OSC_HZ, 240 * MHz);//288:max clock 144  240:max clock 160
#else
    clk_early_init(PLL_REF_XOSC_DIFF, TCFG_CLOCK_OSC_HZ, 192 * MHz);
#endif

    os_init();
    tick_timer_init();
    sys_timer_init();

    port_init();

#if CONFIG_DEBUG_ENABLE || CONFIG_DEBUG_LITE_ENABLE
    void debug_uart_init();
    debug_uart_init();

#if CONFIG_DEBUG_ENABLE
    log_early_init(1024 * 2);
#endif

#endif
    extern u8 get_charge_state(void);
    get_charge_state();//开机检测是否充电在线，需要唤醒锂保

    log_i("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    log_i("         setup_arch %s %s", __DATE__, __TIME__);
    log_i("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    clock_dump();


    power_reset_source_dump();

    power_wakeup_reason_dump();

    //Register debugger interrupt
    request_irq(0, 2, exception_irq_handler, 0);
    request_irq(1, 2, exception_irq_handler, 0);
    code_movable_init();

    debug_init();

    __crc16_mutex_init();
    extern void bt_osc_offset_ext_updata(s32 offset);
    u8 temp_buf[6]; 
    int ret = syscfg_read(CFG_BT_FRE_OFFSET, temp_buf, 6);
    log_i("CFG_BT_FRE_OFFSET, ret:%d\n");
    if (ret != 6) {
        bt_osc_offset_ext_updata(23);
    }
    
    
    app_main();
}

/*-----------------------------------------------------------*/
