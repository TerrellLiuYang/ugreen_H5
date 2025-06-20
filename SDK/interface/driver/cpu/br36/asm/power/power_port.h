#ifndef __POWER_PORT_H__
#define __POWER_PORT_H__

#define     _PORT(p)            JL_PORT##p
#define     _PORT_IN(p,b)       P##p##b##_IN
#define     _PORT_OUT(p,b)      JL_OMAP->P##p##b##_OUT
#define     SPI_PORT(p)         _PORT(p)
#define     SPI0_FUNC_OUT(p,b)  _PORT_OUT(p,b)
#define     SPI0_FUNC_IN(p,b)   _PORT_IN(p,b)
#define     PORT_SPI0_PWRA      D
#define     SPI0_PWRA           4
#define     PORT_SPI0_CSA       D
#define     SPI0_CSA            3
#define     PORT_SPI0_CLKA      D
#define     SPI0_CLKA           0
#define     PORT_SPI0_DOA       D
#define     SPI0_DOA            1
#define     PORT_SPI0_DIA       D
#define     SPI0_DIA            2
#define     PORT_SPI0_D2A       C
#define     SPI0_D2A            6
#define     PORT_SPI0_D3A       A
#define     SPI0_D3A            7
#define     PORT_SPI0_PWRB      D
#define     SPI0_PWRB           4
#define     PORT_SPI0_CSB       D
#define     SPI0_CSB            5
#define     PORT_SPI0_CLKB      D
#define     SPI0_CLKB           0
#define     PORT_SPI0_DOB       D
#define     SPI0_DOB            1
#define     PORT_SPI0_DIB       D
#define     SPI0_DIB            6
#define     PORT_SPI0_D2B       C
#define     SPI0_D2B            6
#define     PORT_SPI0_D3B       A
#define     SPI0_D3B            7
void port_init(void);

u8 get_sfc_bit_mode();
#endif
