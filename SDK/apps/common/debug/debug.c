#include "app_config.h"
#include "typedef.h"



#if CONFIG_DEBUG_ENABLE == 0

int putchar(int a)
{
    return a;
}

int puts(const char *out)
{
    return 0;
}

int printf(const char *format, ...)
{
    return 0;
}


void put_buf(const u8 *buf, int len)
{

}

void put_u8hex(u8 dat)
{

}

void put_u16hex(u16 dat)
{

}

void put_u32hex(u32 dat)
{

}

void put_float(double fv)
{

}

void log_print(int level, const char *tag, const char *format, ...)
{

}

#if CONFIG_DEBUG_LITE_ENABLE==0
void log_putbyte(char c)
{

}
#endif /* #ifndef CONFIG_DEBUG_LITE_ENABLE */

int assert_printf(const char *format, ...)
{
    /* extern void mem_unfree_dump(); */
    /* mem_unfree_dump(); */
    cpu_assert_debug();

    return 0;
}

#endif
