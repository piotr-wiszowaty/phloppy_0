/*
 * phloppy_0 - Commodore Amiga floppy drive emulator
 * Copyright (C) 2016-2018 Piotr Wiszowaty
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stm32f446xx.h>
#include "fmc.h"

#define FMC_SDRAM_CMD_CLK_ENABLE	1
#define FMC_SDRAM_CMD_PALL		2
#define FMC_SDRAM_CMD_AUTOREFRESH_MODE	3
#define FMC_SDRAM_CMD_LOAD_MODE		4
#define FMC_SDRAM_CMD_SELF_REFRESH	5
#define FMC_SDRAM_CMD_POWER_DOWN	6

extern void wait_us(int us);

void fmc_sdram_init()
{
	FMC_Bank5_6->SDCR[0] = FMC_SDCR1_SDCLK_1 | FMC_SDCR1_RBURST | FMC_SDCR1_RPIPE_1;
	FMC_Bank5_6->SDCR[1] = FMC_SDCR1_NR_0 | FMC_SDCR1_MWID_0 | FMC_SDCR1_NB | FMC_SDCR1_CAS;

	FMC_Bank5_6->SDTR[0] = 7 << FMC_SDTR1_TRC_Pos | 2 << FMC_SDTR1_TRP_Pos;
	FMC_Bank5_6->SDTR[1] = 2 << FMC_SDTR1_TMRD_Pos | 7 << FMC_SDTR1_TXSR_Pos |
				4 << FMC_SDTR1_TRAS_Pos | 2 << FMC_SDTR1_TWR_Pos |
				2 << FMC_SDTR1_TRCD_Pos;

	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = FMC_SDRAM_CMD_CLK_ENABLE | FMC_SDCMR_CTB2 | 1 << FMC_SDCMR_NRFS_Pos;
	wait_us(200);

	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = FMC_SDRAM_CMD_PALL | FMC_SDCMR_CTB2 | 1 << FMC_SDCMR_NRFS_Pos;

	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = FMC_SDRAM_CMD_AUTOREFRESH_MODE | FMC_SDCMR_CTB2 | 4 << FMC_SDCMR_NRFS_Pos;

	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = FMC_SDRAM_CMD_LOAD_MODE | FMC_SDCMR_CTB2 | 1 << FMC_SDCMR_NRFS_Pos | 0x231 << FMC_SDCMR_MRD_Pos;

	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDRTR |= 683 << FMC_SDRTR_COUNT_Pos;
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
}

void fmc_sdram_self_refresh()
{
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = FMC_SDRAM_CMD_SELF_REFRESH | FMC_SDCMR_CTB2;
}

void fmc_sdram_power_down()
{
	while(FMC_Bank5_6->SDSR & FMC_SDSR_BUSY);
	FMC_Bank5_6->SDCMR = FMC_SDRAM_CMD_POWER_DOWN | FMC_SDCMR_CTB2;
}
