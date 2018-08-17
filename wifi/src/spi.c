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
* - move initialization code directly into spi_init
* - simplify spi_transaction
* - reformat (to closer match the rest of the project)
*/

#include <eagle_soc.h>
#include "spi.h"

void spi_init()
{
	WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2); //GPIO12 is HSPI MISO pin (Master Data In)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2); //GPIO14 is HSPI CLK pin (Clock)
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2); //GPIO15 is HSPI CS pin (Chip Select / Slave Select)

#if 0
	// 1 MHz
	WRITE_PERI_REG(SPI_CLOCK(HSPI), 7 << SPI_CLKDIV_PRE_S | 9 << SPI_CLKCNT_N_S | 5 << SPI_CLKCNT_H_S);
#else
	// 10 MHz
	WRITE_PERI_REG(SPI_CLOCK(HSPI), 7 << SPI_CLKCNT_N_S | 3 << SPI_CLKCNT_H_S);
#endif

	spi_tx_byte_order(SPI_BYTE_ORDER_LOW_TO_HIGH);
	spi_rx_byte_order(SPI_BYTE_ORDER_LOW_TO_HIGH); 

	SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_CS_SETUP|SPI_CS_HOLD);
	CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_FLASH_MODE);
}

// Description: Configures SPI mode parameters for clock edge and clock polarity.
//  Parameters: spi_cpha - (0) Data is valid on clock leading edge
//			             (1) Data is valid on clock trailing edge
//			  spi_cpol - (0) Clock is low when inactive
//			             (1) Clock is high when inactive

void spi_mode(uint8 spi_cpha, uint8 spi_cpol)
{
	if (!spi_cpha == !spi_cpol) {
		CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_CK_OUT_EDGE);
	} else {
		SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_CK_OUT_EDGE);
	}
	if (spi_cpol) {
		SET_PERI_REG_MASK(SPI_PIN(HSPI), SPI_IDLE_EDGE);
	} else {
		CLEAR_PERI_REG_MASK(SPI_PIN(HSPI), SPI_IDLE_EDGE);
	}
}

// Description: Setup the byte order for shifting data out of buffer
//  Parameters: byte_order - SPI_BYTE_ORDER_HIGH_TO_LOW (1) 
//						   Data is sent out starting with Bit31 and down to Bit0
//
//						   SPI_BYTE_ORDER_LOW_TO_HIGH (0)
//						   Data is sent out starting with the lowest BYTE, from 
//						   MSB to LSB, followed by the second lowest BYTE, from
//						   MSB to LSB, followed by the second highest BYTE, from
//						   MSB to LSB, followed by the highest BYTE, from MSB to LSB
//						   0xABCDEFGH would be sent as 0xGHEFCDAB

void spi_tx_byte_order(uint8 byte_order)
{
	if (byte_order) {
		SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_WR_BYTE_ORDER);
	} else {
		CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_WR_BYTE_ORDER);
	}
}

// Description: Setup the byte order for shifting data into buffer
//  Parameters: byte_order - SPI_BYTE_ORDER_HIGH_TO_LOW (1) 
//						   Data is read in starting with Bit31 and down to Bit0
//
//						   SPI_BYTE_ORDER_LOW_TO_HIGH (0)
//						   Data is read in starting with the lowest BYTE, from 
//						   MSB to LSB, followed by the second lowest BYTE, from
//						   MSB to LSB, followed by the second highest BYTE, from
//						   MSB to LSB, followed by the highest BYTE, from MSB to LSB
//						   0xABCDEFGH would be read as 0xGHEFCDAB

void spi_rx_byte_order(uint8 byte_order)
{
	if (byte_order) {
		SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_RD_BYTE_ORDER);
	} else {
		CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_RD_BYTE_ORDER);
	}
}

// Description: SPI transaction function
//  Parameters: data - buffer
//			    length - buffer length

void spi_transaction(uint8 *data, int32 length)
{
	uint32 *p;
	int i;
	uint16 l = (length << 3) - 1;

	while (spi_busy(HSPI));

	CLEAR_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_MOSI | SPI_USR_MISO | SPI_USR_COMMAND | SPI_USR_ADDR | SPI_USR_DUMMY | SPI_DOUTDIN);
	SET_PERI_REG_MASK(SPI_USER(HSPI), SPI_USR_MOSI | SPI_DOUTDIN);
	WRITE_PERI_REG(SPI_USER1(HSPI), (l << SPI_USR_MOSI_BITLEN_S) | ((-l & 0xffff) << SPI_USR_MISO_BITLEN_S));
	for (i = 0, p = (uint32 *) data; i < length; i += 4) {
		WRITE_PERI_REG(SPI_W0(HSPI) + i, *p++);
	}

	SET_PERI_REG_MASK(SPI_CMD(HSPI), SPI_USR);
	while (spi_busy(HSPI));

	for (i = 0, p = (uint32 *) data; i < length; i += 4) {
		*p++ = READ_PERI_REG(SPI_W0(HSPI) + i);
	}
}
