/*
* The MIT License (MIT)
* 
* Copyright (c) 2015 David Ogilvy (MetalPhreak)
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Changes from the original:
* - remove parts unused in phloppy_0
*/

#ifndef _SPI_H
#define _SPI_H

#include <c_types.h>
#include "spi_register.h"

// SPI hardware modules
#define SPI 0
#define HSPI 1

#define SPI_CLK_USE_DIV 0
#define SPI_CLK_80MHZ_NODIV 1

#define SPI_BYTE_ORDER_HIGH_TO_LOW 1
#define SPI_BYTE_ORDER_LOW_TO_HIGH 0

#ifndef CPU_CLK_FREQ				// Should already be defined in eagle_soc.h
#define CPU_CLK_FREQ (80*1000000)
#endif

// Default SPI clock settings
#define SPI_CLK_PREDIV 10
#define SPI_CLK_CNTDIV 2
#define SPI_CLK_FREQ (CPU_CLK_FREQ/(SPI_CLK_PREDIV*SPI_CLK_CNTDIV)) // 80 / 20 = 4 MHz

void spi_init();
void spi_mode(uint8 spi_cpha, uint8 spi_cpol);
void spi_clock(uint16 prediv, uint8 cntdiv);
void spi_tx_byte_order(uint8 byte_order);
void spi_rx_byte_order(uint8 byte_order);
void spi_transaction(uint8 *data, int32 length);

#define spi_busy(spi_no) READ_PERI_REG(SPI_CMD(spi_no)) & SPI_USR

#endif

