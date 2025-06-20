/*
 ****************************************************************
 *							FFT Accelerator
 * Briefs:
 * Notes :
 *
 ****************************************************************
 */
#include "system/includes.h"
#include "hw_fft.h"

const int JL_HW_FFT_V = FFT_V3 ;    //FFT硬件版本
static u8 fft_init = 0;
OS_MUTEX fft_mutex;

void hw_fft_wrap(pi32v2_hw_fft_ctx *ctx)
{
    if (fft_init == 0) {
        fft_init = 1;
        os_mutex_create(&fft_mutex);
    }
    os_mutex_pend(&fft_mutex, 0);

    JL_FFT->CON = 0;
    JL_FFT->CON |= 1 << 8;

    JL_FFT->CADR = (unsigned int)ctx;
    JL_FFT->CON |= 1;
    while ((JL_FFT->CON & (1 << 7)) == 0);
    JL_FFT->CON |= (1 << 6);

    /* printf("fft succ\n"); */
    os_mutex_post(&fft_mutex);
}


/*********************************************************************
*                  hw_fft_config
* Description: 根据配置生成 FFT_config
* Arguments  :N 运算数据量；
			  log2N 运算数据量的对数值
			  is_same_addr 输入输出是否同一个地址，0:否，1:是
			  is_ifft 运算类型 0:FFT运算, 1:IFFT运算
			  is_real 运算数据的类型  1:实数, 0:复数
* Return	 :ConfgPars 写入FFT寄存器
* Note(s)    : None.
*********************************************************************/
unsigned int hw_fft_config(int N, int log2N, int is_same_addr, int is_ifft, int is_real)
{
    unsigned int ConfgPars;
    ConfgPars = 0;
    ConfgPars |= is_same_addr;
    ConfgPars |= is_real << 1;
    if (is_ifft == 0) {
        if (is_real == 1) {
            ConfgPars |= (log2N - 3) << 4;
            ConfgPars |= (log2N - 2) << 8;
        } else {
            ConfgPars |= (log2N - 2) << 4;
            ConfgPars |= (log2N - 1) << 8;
        }
    } else {
        if (is_real == 1) {
            ConfgPars |= 1 << 2;
            ConfgPars |= (2) << 4;
            ConfgPars |= (log2N - 2) << 8;
        } else {
            ConfgPars |= 1 << 2;
            ConfgPars |= (3) << 4;
            ConfgPars |= (log2N - 1) << 8;
        }
    }
    ConfgPars |= (N) << 16;
    /* printf("ConfgPars%x\n",ConfgPars); */
    return ConfgPars;
}

/*********************************************************************
*                  hw_fft_run
* Description: fft/ifft运算函数
* Arguments  :fft_config FFT运算配置寄存器值
			  in  输入数据地址
			  out 输出数据地址
* Return	 : void
* Note(s)    : None.
*********************************************************************/
void hw_fft_run(unsigned int fft_config, const int *in, int *out)
{
    pi32v2_hw_fft_ctx ctx;
    ctx.fft_config = fft_config;
    ctx.in = in;
    ctx.out = out;
    hw_fft_wrap(&ctx);
}



/*
*********************************************************************
*                  Make_FFT_Config
* Description: 根据配置生成 FFTconfig
* Arguments  :N 运算数据量；log2N 运算数据量的对数值
			  same_addr 输入输出是否同一个地址，是则写1，否则写0
			  is_ifft IFFT运算写1，FFT运算写0
			  is_real 实数运算写1, 复数运算写0
* Return	 :FFTN_CON 写入FFT寄存器
* Note(s)    : None.
*********************************************************************
*/
unsigned int make_fft_config(int N, int log2N, int same_addr, int is_ifft, int is_real)
{
    /* printf("N%d,log2n%d,same_addr%d,is_forward%d,is_real%d\n",N,log2N,same_addr,is_ifft,is_real); */
    unsigned int ConfgPars;
    ConfgPars = 0;
    ConfgPars |= same_addr;
    ConfgPars |= is_real << 1;
    if (is_ifft == 0) {
        if (is_real == 1) {
            ConfgPars |= (log2N - 3) << 4;
            ConfgPars |= (log2N - 2) << 8;
        } else {
            ConfgPars |= (log2N - 2) << 4;
            ConfgPars |= (log2N - 1) << 8;
        }
    } else {
        if (is_real == 1) {
            ConfgPars |= 1 << 2;
            ConfgPars |= (2) << 4;
            ConfgPars |= (log2N - 2) << 8;
        } else {
            ConfgPars |= 1 << 2;
            ConfgPars |= (3) << 4;
            ConfgPars |= (log2N - 1) << 8;
        }
    }
    ConfgPars |= (N) << 16;
    /* printf("ConfgPars%x\n",ConfgPars); */
    return ConfgPars;
}

void _fixfft_wrap(pi32v2_hw_fft_ctx *ctx, const int *in, int *out)
{
    ctx->in = in;
    ctx->out = out;
    hw_fft_wrap(ctx);
}

