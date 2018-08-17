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
#include "wifi_parameters.h"

#ifndef VERSION
#error "VERSION undefined"
#endif
#if (VERSION != 0) && (VERSION != 2)
#error "VERSION invalid (valid: 0, 2)"
#endif

// [long word]
#define RAW_TRACK_SIZE	(12668/4)

// [long word]
#define MFM_TRACK_SIZE	(11*1088/4)
// [long word]
#define MFM_GAP_SIZE	(RAW_TRACK_SIZE - MFM_TRACK_SIZE)
#define READ_DELAY_US	10000

// [long word]
#define ADF_TRACK_SIZE	(512*11 / 4)

#define MAX_DIRTY_TTS	256

#define HCLK		160000000
#define PCLK1		40000000
#define TIM_PCLK1	(2*PCLK1)
#define PCLK2		80000000
#define TIM_PCLK2	(2*PCLK2)

#define TIM3_FREQ	10000
#define TIM4_FREQ	2000000
#define TIM5_FREQ	2000
#define TIM6_FREQ	1000000
#define TIM8_FREQ	10000000
#define TIM12_FREQ	10000000

// [100 us]
#define SDRAM_IDLE_TIME	50000

#define setup_pin(port, bit, mode, speed, altfun)									\
	GPIO##port->MODER = (GPIO##port->MODER & ~(3<<2*(bit))) | ((mode) << 2*(bit));					\
	GPIO##port->OSPEEDR = (GPIO##port->OSPEEDR & ~(3<<2*(bit))) | ((speed) << 2*(bit));				\
	GPIO##port->AFR[(bit)>>3] = (GPIO##port->AFR[(bit)>>3] & ~(15<<4*((bit)&7))) | (altfun << 4*((bit) & 7))

#define FLOPPY0_SEL	(GPIOG->IDR & (1 << SEL0))
#define FLOPPY1_SEL	(GPIOG->IDR & (1 << SEL1))
#define FLOPPY2_SEL	(GPIOD->IDR & (1 << SEL2))
#define FLOPPY3_SEL	(GPIOD->IDR & (1 << SEL3))

#define EMPTY_TRACK_MASK_FLOPPY0_HEAD0	0x01
#define EMPTY_TRACK_MASK_FLOPPY0_HEAD1	0x02
#define EMPTY_TRACK_MASK_FLOPPY1_HEAD0	0x04
#define EMPTY_TRACK_MASK_FLOPPY1_HEAD1	0x08
#define EMPTY_TRACK_MASK_FLOPPY2_HEAD0	0x10
#define EMPTY_TRACK_MASK_FLOPPY2_HEAD1	0x20
#define EMPTY_TRACK_MASK_FLOPPY3_HEAD0	0x40
#define EMPTY_TRACK_MASK_FLOPPY3_HEAD1	0x80

#define END		0xc0
#define ESC		0xdb
#define ESC_END		0xdc
#define ESC_ESC		0xdd

typedef enum {
	NOP,
	OP,
	TRANSMIT
} State;

typedef enum {
	SYNC_WORD,
	INFO_ODD,
	INFO_EVEN,
	LABEL_ODD,
	LABEL_EVEN,
	HDR_CHECKSUM_ODD,
	HDR_CHECKSUM_EVEN,
	DATA_CHECKSUM_ODD,
	DATA_CHECKSUM_EVEN,
	DATA_ODD,
	DATA_EVEN
} Decode_state;

#define OP_NOP		0x00
#define OP_INSERT0	0x01
#define OP_INSERT1	0x02
#define OP_EJECT0	0x03
#define OP_EJECT1	0x04
#define OP_FILL0	0x05
#define OP_FILL1	0x06
#define OP_WPROT0	0x07
#define OP_WPROT1	0x08
#define OP_WUNPROT0	0x09
#define OP_WUNPROT1	0x0a
#define OP_INSERT2	0x11
#define OP_INSERT3	0x12
#define OP_EJECT2	0x13
#define OP_EJECT3	0x14
#define OP_FILL2	0x15
#define OP_FILL3	0x16
#define OP_WPROT2	0x17
#define OP_WPROT3	0x18
#define OP_WUNPROT2	0x19
#define OP_WUNPROT3	0x1a
#define OP_TYPE0_ADF	0x1b
#define OP_TYPE1_ADF	0x1c
#define OP_TYPE2_ADF	0x1d
#define OP_TYPE3_ADF	0x1e
#define OP_TYPE0_RAW	0x1f
#define OP_TYPE1_RAW	0x20
#define OP_TYPE2_RAW	0x21
#define OP_TYPE3_RAW	0x22
#define OP_SETUP_WIFI	0x80

// PA
#define FLOP1_TRK0	8
#define ENA3		9
#define ENA2		10
#define FLOP2_TRK0	15

// PB
#define SDCKE1		5
#define SDNE1		6
#define FLOP3_TRK0	11
#define WPROT		13
#define DIR		14
#define XCLK		15

// PC
#define SDNWE		0
#define DKWDB		6
#define ENA0		7
#define ENA1		8
#define FLOP0_TRK0	9
#define LED_RED		10
#define LED_BLUE	11
#define LED_GREEN	12

// PD
#define SD_D2		0
#define SD_D3		1
#define LED_YELLOW	2
#define SEL2		4
#define SEL3		5
#define SD_D13		8
#define SD_D14		9
#define SD_D15		10
#define SIDE		11
#define DKRD		12
#define DKWEB		13
#define SD_D0		14
#define SD_D1		15

// PE
#define NBL0		0
#define NBL1		1
#define ESP_CLK		2
#define ESP_CS		4
#define ESP_MISO	5
#define ESP_MOSI	6
#define SD_D4		7
#define SD_D5		8
#define SD_D6		9
#define SD_D7		10
#define SD_D8		11
#define SD_D9		12
#define SD_D10		13
#define SD_D11		14
#define SD_D12		15

// PF
#define SD_A0		0
#define SD_A1		1
#define SD_A2		2
#define SD_A3		3
#define SD_A4		4
#define SD_A5		5
#define SDNRAS		11
#define SD_A6		12
#define SD_A7		13
#define SD_A8		14
#define SD_A9		15

// PG
#define SD_A10		0
#define SD_A11		1
#define INDEX		2
#define SEL0		3
#define BA0		4
#define BA1		5
#define SEL1		6
#define STEP		7
#define SDNCLK		8
#define SREQ		13
#define MREQ		14
#define SDNCAS		15

#if VERSION != 0
#define EXTI_SEL23_MSK	((1 << SEL2) | (1 << SEL3))
#else
#define EXTI_SEL23_MSK	0
#endif

const struct wifi_parameters wifi_parameters = {
	{192, 168, 4, 1},
	{255, 255, 255, 0},
	{192, 168, 4, 2},
	{192, 168, 4, 3},
	"Phloppy_0",
	"password_0"
};

const unsigned short dkrd_tim_arr_lut[] = {2*TIM4_FREQ/1000000-1, 4*TIM4_FREQ/1000000-1, 6*TIM4_FREQ/1000000-1, 8*TIM4_FREQ/1000000-1};

volatile int timer0 = 0;
volatile int timer1 = SDRAM_IDLE_TIME;

unsigned int mfm_track_floppy0_head0[RAW_TRACK_SIZE];
unsigned int mfm_track_floppy0_head1[RAW_TRACK_SIZE];
unsigned int mfm_track_floppy1_head0[RAW_TRACK_SIZE];
unsigned int mfm_track_floppy1_head1[RAW_TRACK_SIZE];
unsigned int mfm_track_floppy2_head0[RAW_TRACK_SIZE];
unsigned int mfm_track_floppy2_head1[RAW_TRACK_SIZE];
unsigned int mfm_track_floppy3_head0[RAW_TRACK_SIZE];
unsigned int mfm_track_floppy3_head1[RAW_TRACK_SIZE];
unsigned int *mfm_track = mfm_track_floppy0_head0;
volatile unsigned int current_mfm_long;
volatile unsigned int mfm_offset;
volatile unsigned int mfm_bitmask;
volatile unsigned int mfm_bitcount;
volatile unsigned int mfm_track_type;
volatile unsigned int received_sectors;
volatile unsigned int current_track_empty;
volatile unsigned int empty_tracks;
volatile Decode_state mfm_decode_state;
volatile int mfm_break;
volatile unsigned int floppy_type;					// 0: ADF, 1: raw
unsigned int *floppy0_data = (unsigned int *) 0xd0000000;
unsigned int *floppy1_data = (unsigned int *) 0xd0200000;
unsigned int *floppy2_data = (unsigned int *) 0xd0400000;
unsigned int *floppy3_data = (unsigned int *) 0xd0600000;
volatile int floppy0_current_cylinder;
volatile int floppy1_current_cylinder;
volatile int floppy2_current_cylinder;
volatile int floppy3_current_cylinder;
int floppy0_write_protected = 1;
int floppy1_write_protected = 1;
int floppy2_write_protected = 1;
int floppy3_write_protected = 1;
volatile unsigned int *written_track_start;
unsigned char floppy0_dirty_tts[MAX_DIRTY_TTS];				// (cylinder << 1) | head
unsigned char floppy1_dirty_tts[MAX_DIRTY_TTS];				// (cylinder << 1) | head
unsigned char floppy2_dirty_tts[MAX_DIRTY_TTS];				// (cylinder << 1) | head
unsigned char floppy3_dirty_tts[MAX_DIRTY_TTS];				// (cylinder << 1) | head
volatile int floppy0_dirty_tt_wi = 0;
volatile int floppy1_dirty_tt_wi = 0;
volatile int floppy2_dirty_tt_wi = 0;
volatile int floppy3_dirty_tt_wi = 0;
volatile int floppy0_dirty_tt_ri = 0;
volatile int floppy1_dirty_tt_ri = 0;
volatile int floppy2_dirty_tt_ri = 0;
volatile int floppy3_dirty_tt_ri = 0;

inline void sdram_enter_low_power_mode()
{
	fmc_sdram_self_refresh();
	GPIOC->BSRR = 1 << LED_GREEN;
}

inline void sdram_exit_low_power_mode()
{
	timer1 = SDRAM_IDLE_TIME;
	GPIOC->BSRR = 0x10000 << LED_GREEN;
}

void select_mfm_track()
{
	int head = (GPIOD->IDR & (1 << SIDE)) == 0;

	if (!FLOPPY0_SEL) {
		mfm_track = head ? mfm_track_floppy0_head1 : mfm_track_floppy0_head0;
		current_track_empty = (empty_tracks & (head ? EMPTY_TRACK_MASK_FLOPPY0_HEAD1 : EMPTY_TRACK_MASK_FLOPPY0_HEAD0)) != 0;
	} else if (!FLOPPY1_SEL) {
		mfm_track = head ? mfm_track_floppy1_head1 : mfm_track_floppy1_head0;
		current_track_empty = (empty_tracks & (head ? EMPTY_TRACK_MASK_FLOPPY1_HEAD1 : EMPTY_TRACK_MASK_FLOPPY1_HEAD0)) != 0;
#if VERSION != 0
	} else if (!FLOPPY2_SEL) {
		mfm_track = head ? mfm_track_floppy2_head1 : mfm_track_floppy2_head0;
		current_track_empty = (empty_tracks & (head ? EMPTY_TRACK_MASK_FLOPPY2_HEAD1 : EMPTY_TRACK_MASK_FLOPPY2_HEAD0)) != 0;
	} else {
		mfm_track = head ? mfm_track_floppy3_head1 : mfm_track_floppy3_head0;
		current_track_empty = (empty_tracks & (head ? EMPTY_TRACK_MASK_FLOPPY3_HEAD1 : EMPTY_TRACK_MASK_FLOPPY3_HEAD0)) != 0;
#endif
	}
}

inline void start_index_pulse()
{
	GPIOG->BSRR = 0x10000 << INDEX;
	TIM3->CNT = 0;
	TIM3->CR1 = TIM_CR1_CEN;
}

inline void stop_index_pulse()
{
	GPIOG->BSRR = 1 << INDEX;
	TIM3->CR1 = 0;
}

void TIM3_IRQHandler()
{
	TIM3->SR = 0;
	stop_index_pulse();
}

inline void wrap_mfm_offset()
{
	if (++mfm_offset == RAW_TRACK_SIZE) {
		mfm_offset = 0;
		start_index_pulse();
	}
}

inline void next_mfm_bit()
{
	if (mfm_bitmask == 1) {
		mfm_bitmask = 0x80000000;
		current_mfm_long = mfm_track[mfm_offset];
		wrap_mfm_offset();
	} else {
		mfm_bitmask >>= 1;
	}
}

void TIM4_IRQHandler()
{
	int k;

	TIM4->SR = 0;

	if (mfm_break) {
		// Stop sending track data
		TIM4->CCMR1 = 4 << TIM_CCMR1_OC1M_Pos;			// Force inactive level
		TIM4->CR1 = 0;
		if (mfm_break == 2) {
			TIM5->EGR = TIM_EGR_UG;				// Trigger immediate track sending
		}
		mfm_break = 0;
		GPIOD->BSRR = 1 << LED_YELLOW;
	} else {
		// Skip '1' bit
		next_mfm_bit();

		// Count '0' bits
		for (k = 0; !(current_mfm_long & mfm_bitmask); k++) {
			next_mfm_bit();
		}
		TIM4->ARR = dkrd_tim_arr_lut[k];
	}
}

void decode_bit(unsigned int bit)
{
	static unsigned int *odd_lword;
	static unsigned int *even_lword;
	static unsigned int info;

	current_mfm_long = (current_mfm_long << 1) | bit;
	mfm_bitcount++;

	if (mfm_bitcount == 32) {
		mfm_track[mfm_offset] = current_mfm_long;
		wrap_mfm_offset();
	}

	switch (mfm_decode_state) {
		case SYNC_WORD:
			if (current_mfm_long == 0x44894489) {
				mfm_decode_state = INFO_ODD;
				mfm_bitcount = 0;
				received_sectors++;
			}
			break;
		case INFO_ODD:
			if (mfm_bitcount == 32) {
				mfm_decode_state = INFO_EVEN;
				mfm_bitcount = 0;
				info = (current_mfm_long & 0x55555555) << 1;
			}
			break;
		case INFO_EVEN:
			if (mfm_bitcount == 32) {
				mfm_decode_state = LABEL_ODD;
				mfm_bitcount = 0;
				info |= current_mfm_long & 0x55555555;
				odd_lword = even_lword = (unsigned int *) &written_track_start[((info >> 8) & 0xff) << (9-2)];
			}
			break;
		case LABEL_ODD:
			if (mfm_bitcount == 4*32) {
				mfm_decode_state = LABEL_EVEN;
				mfm_bitcount = 0;
			}
			break;
		case LABEL_EVEN:
			if (mfm_bitcount == 4*32) {
				mfm_decode_state = HDR_CHECKSUM_ODD;
				mfm_bitcount = 0;
			}
			break;
		case HDR_CHECKSUM_ODD:
			if (mfm_bitcount == 32) {
				mfm_decode_state = HDR_CHECKSUM_EVEN;
				mfm_bitcount = 0;
			}
			break;
		case HDR_CHECKSUM_EVEN:
			if (mfm_bitcount == 32) {
				mfm_decode_state = DATA_CHECKSUM_ODD;
				mfm_bitcount = 0;
			}
			break;
		case DATA_CHECKSUM_ODD:
			if (mfm_bitcount == 32) {
				mfm_decode_state = DATA_CHECKSUM_EVEN;
				mfm_bitcount = 0;
			}
			break;
		case DATA_CHECKSUM_EVEN:
			if (mfm_bitcount == 32) {
				mfm_decode_state = DATA_ODD;
				mfm_bitcount = 0;
			}
			break;
		case DATA_ODD:
			if (mfm_bitcount && ((mfm_bitcount & 31) == 0)) {
				*odd_lword++ = __REV((current_mfm_long & 0x55555555) << 1);
			}
			if (mfm_bitcount == 512/4*32) {
				mfm_decode_state = DATA_EVEN;
				mfm_bitcount = 0;
			}
			break;
		case DATA_EVEN:
			if (mfm_bitcount && ((mfm_bitcount & 31) == 0)) {
				*even_lword++ |= __REV(current_mfm_long & 0x55555555);
			}
			if (mfm_bitcount == 512/4*32) {
				mfm_decode_state = SYNC_WORD;
				mfm_bitcount = 0;
			}
			break;
	}
}

void decode_raw_bit(unsigned int bit)
{
	current_mfm_long = (current_mfm_long << 1) | bit;
	if (++mfm_bitcount == 32) {
		mfm_bitcount = 0;
		mfm_track[mfm_offset] = current_mfm_long;
		written_track_start[mfm_offset] = __REV(current_mfm_long);
		wrap_mfm_offset();
	}
}

void TIM8_CC_IRQHandler()
{
	int k;
	unsigned short ccr1 = TIM8->CCR1;

	if (ccr1 < (TIM8_FREQ * 5 / 1000000)) {
		// 4 us - '10'
		k = 2;
	} else if (ccr1 < (TIM8_FREQ * 7 / 1000000)) {
		// 6 us - '100'
		k = 3;
	} else {
		// 8 us - '1000'
		k = 4;
	}
	if (mfm_track_type) {			// Raw track
		decode_raw_bit(1);
		while (--k) {
			decode_raw_bit(0);
		}
	} else {				// ADF track
		decode_bit(1);
		while (--k) {
			decode_bit(0);
		}
	}
}

void wait_us(int us)
{
	// assume 10 us period
	timer0 = us / 10;
	while (timer0);
}

void wait_ms(int ms)
{
	// assume 100 us period
	timer0 = ms * 10;
	while (timer0);
}

void TIM6_DAC_IRQHandler()
{
	TIM6->SR = 0;
	if (timer0) {
		timer0--;
	}
	if (timer1) {
		timer1--;
		if (!timer1) {
			sdram_enter_low_power_mode();
		}
	}
}

void TIM5_IRQHandler()
{
	TIM5->SR = 0;

	// Stop this timer
	TIM5->CR1 = 0;

	// Start sending track data
	select_mfm_track();
	mfm_break = 0;
	mfm_offset = 0;
	mfm_bitmask = 0x80000000;
	mfm_bitcount = 0;
	current_mfm_long = mfm_track[mfm_offset++];
	if (!current_track_empty) {
		TIM4->CCMR1 = 6 << TIM_CCMR1_OC1M_Pos;			// PWM mode 1 (active when CNT<CCR1)
		TIM4->CR1 = TIM_CR1_CEN;
	}
	start_index_pulse();
	GPIOD->BSRR = 0x10000 << LED_YELLOW;
}

inline void stop_sending_track_data()
{
	mfm_break = 1;
}

inline void stop_sending_track_data_now()
{
	mfm_break = 2;
}

inline void restart_send_delay_timer()
{
	TIM5->CNT = 0;
	TIM5->CR1 = TIM_CR1_CEN;
	sdram_exit_low_power_mode();
}

inline void stop_send_delay_timer()
{
	TIM5->CR1 = 0;
}

inline void exti_sel0()
{
	if (FLOPPY0_SEL) {					// rising edge
		stop_send_delay_timer();
		stop_sending_track_data();
		GPIOB->BSRR = 1 << WPROT;
		stop_index_pulse();
	} else {						// falling edge
		restart_send_delay_timer();
		mfm_track_type = (floppy_type & 0x01) != 0;
		if (floppy0_write_protected) {
			GPIOB->BSRR = 0x10000 << WPROT;
		}
	}
}

inline void exti_sel1()
{
	if (FLOPPY1_SEL) {					// rising edge
		stop_send_delay_timer();
		stop_sending_track_data();
		GPIOB->BSRR = 1 << WPROT;
		stop_index_pulse();
	} else {						// falling edge
		restart_send_delay_timer();
		mfm_track_type = (floppy_type & 0x02) != 0;
		if (floppy1_write_protected) {
			GPIOB->BSRR = 0x10000 << WPROT;
		}
	}
}

inline void exti_sel2()
{
	if (GPIOA->ODR & (1 << ENA2)) {
		if (FLOPPY2_SEL) {					// rising edge
			stop_send_delay_timer();
			stop_sending_track_data();
			GPIOB->BSRR = 1 << WPROT;
			stop_index_pulse();
		} else {						// falling edge
			restart_send_delay_timer();
			mfm_track_type = (floppy_type & 0x04) != 0;
			if (floppy2_write_protected) {
				GPIOB->BSRR = 0x10000 << WPROT;
			}
		}
	}
}

inline void exti_sel3()
{
	if (GPIOA->ODR & (1 << ENA3)) {
		if (FLOPPY3_SEL) {					// rising edge
			stop_send_delay_timer();
			stop_sending_track_data();
			GPIOB->BSRR = 1 << WPROT;
			stop_index_pulse();
		} else {						// falling edge
			restart_send_delay_timer();
			mfm_track_type = (floppy_type & 0x08) != 0;
			if (floppy3_write_protected) {
				GPIOB->BSRR = 0x10000 << WPROT;
			}
		}
	}
}

inline void exti_step()
{
	stop_sending_track_data();

	if (FLOPPY0_SEL == 0) {
		if (GPIOB->IDR & (1 << DIR)) {
			if (floppy0_current_cylinder) {
				floppy0_current_cylinder--;
			}
		} else {
			if (floppy0_current_cylinder < 79) {
				floppy0_current_cylinder++;
			}
		}
		GPIOC->BSRR = (floppy0_current_cylinder == 0 ? 0x10000 : 1) << FLOP0_TRK0;
	} else if (FLOPPY1_SEL == 0) {
		if (GPIOB->IDR & (1 << DIR)) {
			if (floppy1_current_cylinder) {
				floppy1_current_cylinder--;
			}
		} else {
			if (floppy1_current_cylinder < 79) {
				floppy1_current_cylinder++;
			}
		}
		GPIOA->BSRR = (floppy1_current_cylinder == 0 ? 0x10000 : 1) << FLOP1_TRK0;
	} else if (FLOPPY2_SEL == 0) {
		if (GPIOB->IDR & (1 << DIR)) {
			if (floppy2_current_cylinder) {
				floppy2_current_cylinder--;
			}
		} else {
			if (floppy2_current_cylinder < 79) {
				floppy2_current_cylinder++;
			}
		}
		GPIOA->BSRR = (floppy2_current_cylinder == 0 ? 0x10000 : 1) << FLOP2_TRK0;
	} else {
		if (GPIOB->IDR & (1 << DIR)) {
			if (floppy3_current_cylinder) {
				floppy3_current_cylinder--;
			}
		} else {
			if (floppy3_current_cylinder < 79) {
				floppy3_current_cylinder++;
			}
		}
		GPIOB->BSRR = (floppy3_current_cylinder == 0 ? 0x10000 : 1) << FLOP3_TRK0;
	}

	restart_send_delay_timer();
}

inline void exti_side()
{
	restart_send_delay_timer();
	stop_sending_track_data_now();
}

inline void flush_last_raw_lword()
{
	int i;

	for (i = 0; i < 31; i++) {
		decode_raw_bit(~(current_mfm_long & 1));
	}
}

inline void exti_dkweb()
{
	int head = (GPIOD->IDR & (1 << SIDE)) == 0;
	int track_size;

	if (GPIOD->IDR & (1 << DKWEB)) {
		// rising edge
		if (FLOPPY0_SEL == 0) {
			if (floppy_type & 0x01) {
				flush_last_raw_lword();
			}
			if (received_sectors > 0 || floppy_type & 0x01) {
				floppy0_dirty_tts[floppy0_dirty_tt_wi++ & (MAX_DIRTY_TTS-1)] = (floppy0_current_cylinder << 1) | head;
			}
		} else if (FLOPPY1_SEL == 0) {
			if (floppy_type & 0x02) {
				flush_last_raw_lword();
			}
			if (received_sectors > 0 || floppy_type & 0x02) {
				floppy1_dirty_tts[floppy1_dirty_tt_wi++ & (MAX_DIRTY_TTS-1)] = (floppy1_current_cylinder << 1) | head;
			}
		} else if (FLOPPY2_SEL == 0) {
			if (floppy_type & 0x04) {
				flush_last_raw_lword();
			}
			if (received_sectors > 0 || floppy_type & 0x04) {
				floppy2_dirty_tts[floppy2_dirty_tt_wi++ & (MAX_DIRTY_TTS-1)] = (floppy2_current_cylinder << 1) | head;
			}
		} else {
			if (floppy_type & 0x08) {
				flush_last_raw_lword();
			}
			if (received_sectors > 0 || floppy_type & 0x08) {
				floppy3_dirty_tts[floppy3_dirty_tt_wi++ & (MAX_DIRTY_TTS-1)] = (floppy3_current_cylinder << 1) | head;
			}
		}
		TIM8->CR1 = 0;
		restart_send_delay_timer();
		start_index_pulse();
	} else {
		// falling edge
		stop_send_delay_timer();
		stop_sending_track_data();
		if ((FLOPPY0_SEL == 0 && !floppy0_write_protected) ||
			       	(FLOPPY1_SEL == 0 && !floppy1_write_protected) ||
				(FLOPPY2_SEL == 0 && !floppy2_write_protected) ||
				(FLOPPY3_SEL == 0 && !floppy3_write_protected)) {
			current_mfm_long = 0x00000000;
			mfm_bitcount = 0;
			mfm_offset = 0;
			mfm_decode_state = SYNC_WORD;
			received_sectors = 0;
			if (FLOPPY0_SEL == 0) {
				if (floppy_type & 0x01) {
					track_size = RAW_TRACK_SIZE;
				} else {
					track_size = ADF_TRACK_SIZE;
				}
				mfm_track = head ? mfm_track_floppy0_head1 : mfm_track_floppy0_head0;
				written_track_start = floppy0_data + ((floppy0_current_cylinder << 1) | head) * track_size;
			} else if (FLOPPY1_SEL == 0) {
				if (floppy_type & 0x02) {
					track_size = RAW_TRACK_SIZE;
				} else {
					track_size = ADF_TRACK_SIZE;
				}
				mfm_track = head ? mfm_track_floppy1_head1 : mfm_track_floppy1_head0;
				written_track_start = floppy1_data + ((floppy1_current_cylinder << 1) | head) * track_size;
			} else if (FLOPPY2_SEL == 0) {
				if (floppy_type & 0x04) {
					track_size = RAW_TRACK_SIZE;
				} else {
					track_size = ADF_TRACK_SIZE;
				}
				mfm_track = head ? mfm_track_floppy2_head1 : mfm_track_floppy2_head0;
				written_track_start = floppy2_data + ((floppy2_current_cylinder << 1) | head) * track_size;
			} else {
				if (floppy_type & 0x08) {
					track_size = RAW_TRACK_SIZE;
				} else {
					track_size = ADF_TRACK_SIZE;
				}
				mfm_track = head ? mfm_track_floppy3_head1 : mfm_track_floppy3_head0;
				written_track_start = floppy3_data + ((floppy2_current_cylinder << 1) | head) * track_size;
			}
			TIM8->CR1 = TIM_CR1_CEN;
		}
	}
}

void EXTI3_IRQHandler()
{
	if (EXTI->PR & (1 << SEL0)) {
		EXTI->PR = 1 << SEL0;
		exti_sel0();
	}
}

void EXTI4_IRQHandler()
{
	if (EXTI->PR & (1 << SEL2)) {
		EXTI->PR = 1 << SEL2;
		exti_sel2();
	}
}

void EXTI9_5_IRQHandler()
{
	if (EXTI->PR & (1 << SEL1)) {
		EXTI->PR = 1 << SEL1;
		exti_sel1();
	}

	if (EXTI->PR & (1 << SEL3)) {
		EXTI->PR = 1 << SEL3;
		exti_sel3();
	}

	if (EXTI->PR & (1 << STEP)) {
		EXTI->PR = 1 << STEP;
		exti_step();
	}
}

void EXTI15_10_IRQHandler()
{
	if (EXTI->PR & (1 << SIDE)) {
		EXTI->PR = 1 << SIDE;
		exti_side();
	}

	if (EXTI->PR & (1 << DKWEB)) {
		EXTI->PR = 1 << DKWEB;
		exti_dkweb();
	}
}

void esp_transaction(char *tx_buffer, char *rx_buffer)
{
	// Prepare SPI transmission
	DMA2->LIFCR = DMA2->LISR;
	DMA2->HIFCR = DMA2->HISR;
	DMA2_Stream0->PAR = (uint32_t) &SPI4->DR;
	DMA2_Stream4->PAR = (uint32_t) &SPI4->DR;
	DMA2_Stream0->M0AR = (uint32_t) rx_buffer;
	DMA2_Stream4->M0AR = (uint32_t) tx_buffer;
	DMA2_Stream0->NDTR = 64;
	DMA2_Stream4->NDTR = 64;
	DMA2_Stream0->CR = (4 << DMA_SxCR_CHSEL_Pos) | DMA_SxCR_MINC | (0 << DMA_SxCR_DIR_Pos) | DMA_SxCR_EN;
	DMA2_Stream4->CR = (5 << DMA_SxCR_CHSEL_Pos) | DMA_SxCR_MINC | (1 << DMA_SxCR_DIR_Pos) | DMA_SxCR_EN;

	// Signal to master readiness for transmission
	GPIOG->BSRR = 1 << SREQ;

	// Wait for transmission end
	while (GPIOG->IDR & (1 << MREQ));
	GPIOG->BSRR = 0x10000 << SREQ;
}

unsigned int mfm_checksum(unsigned int *data, int length)
{
	unsigned int result = 0;
	while (length--) {
		result ^= *data ^ (*data >> 1);
		data++;
	}
	return result & 0x55555555;
}

unsigned int mfm_encode_long(unsigned int *buffer, unsigned int value, unsigned int prev_bit)
{
	unsigned int x = value & 0x55555555;
	*buffer = x | (0xaaaaaaaa & ~((x << 1) | ((x >> 1) | prev_bit)));
	return value << 31;
}

void encode_mfm_track(unsigned int *user_data, unsigned int *mfm_track, int cylinder, int head, unsigned int empty_track_mask)
{
	int i;
	int s;
	unsigned int chksum;
	unsigned int tt = ((cylinder << 1) | head) << 16;
	unsigned int info;
	unsigned int prev_bit = 0x00000000;

	for (s = 0; s < 11; s++) {
		*mfm_track++ = 0xaaaaaaaa ^ prev_bit;
		*mfm_track++ = 0x44894489;
		prev_bit = 0x80000000;

		/* header */
		info = 0xff000000 | tt | (s << 8) | (11 - s);		// info
		chksum = mfm_checksum(&info, 1);

		prev_bit = mfm_encode_long(mfm_track++, info >> 1, prev_bit);
		prev_bit = mfm_encode_long(mfm_track++, info, prev_bit);
		for (i = 0; i < 2*4; i++) {
			prev_bit = mfm_encode_long(mfm_track++, 0, prev_bit);
		}

		/* header checksum */
		prev_bit = mfm_encode_long(mfm_track++, chksum >> 1, prev_bit);
		prev_bit = mfm_encode_long(mfm_track++, chksum, prev_bit);

		/* data checksum */
		chksum = __REV(mfm_checksum(user_data, 512/4));
		prev_bit = mfm_encode_long(mfm_track++, chksum >> 1, prev_bit);
		prev_bit = mfm_encode_long(mfm_track++, chksum, prev_bit);

		/* data */
		for (i = 0; i < 512/4; i++) {
			prev_bit = mfm_encode_long(mfm_track++, __REV(user_data[i] >> 1), prev_bit);
		}
		for (i = 0; i < 512/4; i++) {
			prev_bit = mfm_encode_long(mfm_track++, __REV(*user_data++), prev_bit);
		}
	}
	*mfm_track = 0xaaaaaaaa ^ prev_bit;

	empty_tracks &= ~empty_track_mask;
}

void encode_raw_track(unsigned int *user_data, unsigned int *mfm_track, unsigned int empty_track_mask)
{
	int empty = 1;
	int i;

	for (i = 0; i < RAW_TRACK_SIZE; i++) {
		if (*user_data) {
			empty = 0;
		}
		*mfm_track++ = __REV(*user_data++);
	}
	if (empty) {
		empty_tracks |= empty_track_mask;
	} else {
		empty_tracks &= ~empty_track_mask;
	}
}

static inline int slip_decode(unsigned char c)
{
	static int escape = 0;

	if (escape) {
		escape = 0;
		if (c == ESC_END) {
			return END;
		} else if (c == ESC_ESC) {
			return ESC;
		} else {
			return -1;		// error
		}
	} else {
		if (c == END) {
			return -END;
		} else if (c == ESC) {
			escape = 1;
			return -ESC;
		} else {
			return c;
		}
	}
}

int slip_encode(unsigned char c)
{
	static int escape = 0;

	if (escape) {
		escape = 0;
		if (c == END) {
			return ESC_END;
		} else if (c == ESC) {
			return ESC_ESC;
		} else {
			return -1;
		}
	} else {
		if (c == END || c == ESC) {
			escape = 1;
			return -ESC;
		} else {
			return c;
		}
	}
}

int main()
{
	State rx_state = NOP;
	int floppy0_encoded_cylinder = -1;
	int floppy1_encoded_cylinder = -1;
	int floppy2_encoded_cylinder = -1;
	int floppy3_encoded_cylinder = -1;
	char rx_buffer[64];
	char tx_buffer[64];
	unsigned char *floppy_fill_ptr = (unsigned char *) -1;
	unsigned int tt = 0xffffffff;
	int track_size;
	char *tx_ptr = 0;
	char *tx_end = 0;
	int tx_pending = 0;
	int wifi_setup = 0;
	int c = 0;
	int i;

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
			RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
			RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_CRCEN;

	// Setup LEDs
	setup_pin(C, LED_RED, 1, 0, 0);
	setup_pin(C, LED_BLUE, 1, 0, 0);
	setup_pin(C, LED_GREEN, 1, 0, 0);
	setup_pin(D, LED_YELLOW, 1, 0, 0);

	GPIOC->BSRR = 1<<LED_RED | 1<<LED_BLUE | 1<<LED_GREEN;
	GPIOD->BSRR = 1<<LED_YELLOW;

	// Setup clocks
	RCC->CR |= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY));
	RCC->PLLCFGR = 15 << RCC_PLLCFGR_PLLQ_Pos | 0 << RCC_PLLCFGR_PLLP_Pos | RCC_PLLCFGR_PLLSRC |
		       160 << RCC_PLLCFGR_PLLN_Pos | 4 << RCC_PLLCFGR_PLLM_Pos;
	RCC->CR |= RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY));
	FLASH->ACR = FLASH_ACR_DCEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS;
	while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != FLASH_ACR_LATENCY_5WS);
	RCC->CFGR = RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_HPRE_DIV1 | RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL);

	// Setup ESP8266 communication pins
	setup_pin(G, SREQ, 1, 1, 0);
	setup_pin(E, ESP_CS, 2, 2, 5);
	setup_pin(E, ESP_CLK, 2, 2, 5);
	setup_pin(E, ESP_MISO, 2, 2, 5);
	setup_pin(E, ESP_MOSI, 2, 2, 5);

	// Setup floppy pins
	setup_pin(A, ENA3, 1, 1, 0);
	setup_pin(A, ENA2, 1, 1, 0);
	setup_pin(A, FLOP1_TRK0, 1, 1, 0);
	setup_pin(A, FLOP2_TRK0, 1, 1, 0);
	setup_pin(B, FLOP3_TRK0, 1, 1, 0);
	setup_pin(B, WPROT, 1, 1, 0);
	setup_pin(B, DIR, 0, 0, 0);
	setup_pin(C, ENA0, 1, 1, 0);
	setup_pin(C, ENA1, 1, 1, 0);
	setup_pin(C, FLOP0_TRK0, 1, 1, 0);
	setup_pin(D, SEL2, 0, 0, 0);
	setup_pin(D, SEL3, 0, 0, 0);
	setup_pin(D, SIDE, 0, 0, 0);
	setup_pin(D, DKWEB, 0, 0, 0);
	setup_pin(G, INDEX, 1, 2, 0);
	setup_pin(G, SEL0, 0, 0, 0);
	setup_pin(G, SEL1, 0, 0, 0);
	setup_pin(G, STEP, 0, 0, 0);

	GPIOB->BSRR = 1 << WPROT | 1 << FLOP3_TRK0;
	GPIOC->BSRR = 0x10000 << ENA0 | 0x10000 << ENA1 | 1 << FLOP0_TRK0;
	GPIOA->BSRR = 1 << FLOP1_TRK0 | 1 << FLOP2_TRK0 | 0x10000 << ENA2 | 0x10000 << ENA3;

	// Setup TIM12 - 10 MHz xclk
	RCC->APB1ENR |= RCC_APB1ENR_TIM12EN;
	setup_pin(B, XCLK, 2, 3, 9);					// TIM12_CH2
	TIM12->ARR = TIM_PCLK1 / TIM12_FREQ - 1;
	TIM12->CCR2 = (TIM_PCLK1 / TIM12_FREQ) / 2;
	TIM12->CCER = TIM_CCER_CC2E;
	TIM12->CCMR1 = 6 << TIM_CCMR1_OC2M_Pos;
	TIM12->CR1 = TIM_CR1_CEN;

	// Setup TIM5 - send delay
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
	TIM5->PSC = TIM_PCLK1 / TIM5_FREQ - 1;
	TIM5->ARR = TIM5_FREQ * READ_DELAY_US / 1000000 - 1;
	TIM5->DIER = TIM_DIER_UIE;
	NVIC_EnableIRQ(TIM5_IRQn);

	// Setup TIM6 - 10 us timer
	RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
	TIM6->PSC = TIM_PCLK1 / TIM6_FREQ - 1;
	TIM6->ARR = TIM6_FREQ / 100000 - 1;
	TIM6->DIER = TIM_DIER_UIE;
	TIM6->SR = 0;
	TIM6->CR1 = TIM_CR1_CEN;
	NVIC_EnableIRQ(TIM6_DAC_IRQn);

	// Setup TIM3 - INDEX pulse timer (2.5 ms pulse width)
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	TIM3->PSC = TIM_PCLK1 / TIM3_FREQ - 1;
	TIM3->ARR = TIM3_FREQ / 400 - 1;
	TIM3->DIER = TIM_DIER_UIE;
	TIM3->SR = 0;
	TIM3->CR1 = TIM_CR1_CEN;
	NVIC_EnableIRQ(TIM3_IRQn);

	// Setup FMC
	RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
	setup_pin(B, SDCKE1, 2, 3, 12);
	setup_pin(B, SDNE1, 2, 3, 12);
	setup_pin(C, SDNWE, 2, 3, 12);
	setup_pin(D, SD_D2, 2, 3, 12);
	setup_pin(D, SD_D3, 2, 3, 12);
	setup_pin(D, SD_D13, 2, 3, 12);
	setup_pin(D, SD_D14, 2, 3, 12);
	setup_pin(D, SD_D15, 2, 3, 12);
	setup_pin(D, SD_D0, 2, 3, 12);
	setup_pin(D, SD_D1, 2, 3, 12);
	setup_pin(E, NBL0, 2, 3, 12);
	setup_pin(E, NBL1, 2, 3, 12);
	setup_pin(E, SD_D4, 2, 3, 12);
	setup_pin(E, SD_D5, 2, 3, 12);
	setup_pin(E, SD_D6, 2, 3, 12);
	setup_pin(E, SD_D7, 2, 3, 12);
	setup_pin(E, SD_D8, 2, 3, 12);
	setup_pin(E, SD_D9, 2, 3, 12);
	setup_pin(E, SD_D10, 2, 3, 12);
	setup_pin(E, SD_D11, 2, 3, 12);
	setup_pin(E, SD_D12, 2, 3, 12);
	setup_pin(F, SD_A0, 2, 3, 12);
	setup_pin(F, SD_A1, 2, 3, 12);
	setup_pin(F, SD_A2, 2, 3, 12);
	setup_pin(F, SD_A3, 2, 3, 12);
	setup_pin(F, SD_A4, 2, 3, 12);
	setup_pin(F, SD_A5, 2, 3, 12);
	setup_pin(F, SDNRAS, 2, 3, 12);
	setup_pin(F, SD_A6, 2, 3, 12);
	setup_pin(F, SD_A7, 2, 3, 12);
	setup_pin(F, SD_A8, 2, 3, 12);
	setup_pin(F, SD_A9, 2, 3, 12);
	setup_pin(G, SD_A10, 2, 3, 12);
	setup_pin(G, SD_A11, 2, 3, 12);
	setup_pin(G, BA0, 2, 3, 12);
	setup_pin(G, BA1, 2, 3, 12);
	setup_pin(G, SDNCLK, 2, 3, 12);
	setup_pin(G, SDNCAS, 2, 3, 12);
	fmc_sdram_init();

	// Re-setup TIM6 - 100 us timer
	TIM6->ARR = TIM6_FREQ / 10000 - 1;

	// Setup TIM4 - MFM signal generator
	RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
	setup_pin(D, DKRD, 2, 2, 2);					// TIM4_CH1
	TIM4->PSC = TIM_PCLK1 / TIM4_FREQ - 1;
	TIM4->ARR = dkrd_tim_arr_lut[0];
	TIM4->CCR1 = 1;
	TIM4->CCER = TIM_CCER_CC1P | TIM_CCER_CC1E;
	TIM4->DIER = TIM_DIER_UIE;
	TIM4->CCMR1 = 4 << TIM_CCMR1_OC1M_Pos;				// Force inactive level
	TIM4->CCMR1 = 6 << TIM_CCMR1_OC1M_Pos;				// PWM mode 1 (active when CNT<CCR1)
	NVIC_EnableIRQ(TIM4_IRQn);

	// Setup TIM8 - MFM signal scanner
	RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;
	setup_pin(C, DKWDB, 2, 0, 3);					// TIM8_CH1
	TIM8->ARR = 0xffff;
	TIM8->PSC = TIM_PCLK2 / TIM8_FREQ - 1;
	TIM8->SMCR = (5 << TIM_SMCR_TS_Pos) | (4 << TIM_SMCR_SMS_Pos);
	TIM8->CCMR1 = 1 << TIM_CCMR1_CC1S_Pos;				// IC1 -> TI1
	TIM8->CCER = TIM_CCER_CC1P | TIM_CCER_CC1E;			// TI1FP1 falling edge
	TIM8->DIER = TIM_DIER_CC1IE;
	NVIC_EnableIRQ(TIM8_CC_IRQn);

	// Setup EXTI interrupts (SEL0:G3, SEL1:G6, SEL2:D4, SEL3:D5, STEP:G7, SIDE:D11, DKWEB:D13)
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	SYSCFG->EXTICR[0] = 0x6000;		// PG3
	SYSCFG->EXTICR[1] = 0x6633;		// PG7, PG6, PD5, PD4
	SYSCFG->EXTICR[2] = 0x3000;		// PD11
	SYSCFG->EXTICR[3] = 0x0030;		// PD13
	EXTI->IMR = (1 << SEL0) | (1 << SEL1) | (1 << STEP) | (1 << DKWEB) | (1 << SIDE) | EXTI_SEL23_MSK;
	EXTI->RTSR = (1 << SEL0) | (1 << SEL1) | (1 << STEP) | (1 << DKWEB) | (1 << SIDE) | EXTI_SEL23_MSK;
	EXTI->FTSR = (1 << SEL0) | (1 << SEL1) | (1 << DKWEB) | (1 << SIDE) | EXTI_SEL23_MSK;
	NVIC_EnableIRQ(EXTI3_IRQn);
	NVIC_EnableIRQ(EXTI4_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	NVIC_EnableIRQ(EXTI15_10_IRQn);

	// Test SDRAM
	for (i = 0; i < 8*1048576/4; i++) {
		floppy0_data[i] = i;
	}
	for (i = 0; i < 8*1048576/4; i++) {
		if (floppy0_data[i] != i) {
			break;
		}
	}
	if (i < 8*1048576/4) {
		// Blink green LED on SDRAM error
		while (1) {
			GPIOC->ODR ^= 1 << LED_GREEN;
			for (i = 0; i < 20000000; i++) {
				asm ("nop\n\t");
			}
		}
	}

	// Setup DMA2 & SPI4 - ESP8266 communication
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
	RCC->APB2ENR |= RCC_APB2ENR_SPI4EN;
	SPI4->CR2 = SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;
	SPI4->CR1 = SPI_CR1_SPE;

	// Prepare track buffers
	for (i = MFM_TRACK_SIZE; i < MFM_TRACK_SIZE+MFM_GAP_SIZE; i++) {
		mfm_track_floppy0_head0[i] = 0xaaaaaaaa;
		mfm_track_floppy0_head1[i] = 0xaaaaaaaa;
		mfm_track_floppy1_head0[i] = 0xaaaaaaaa;
		mfm_track_floppy1_head1[i] = 0xaaaaaaaa;
		mfm_track_floppy2_head0[i] = 0xaaaaaaaa;
		mfm_track_floppy2_head1[i] = 0xaaaaaaaa;
		mfm_track_floppy3_head0[i] = 0xaaaaaaaa;
		mfm_track_floppy3_head1[i] = 0xaaaaaaaa;
	}

	GPIOC->BSRR = 0x10000 << LED_GREEN;

	for (;;) {
		if ((GPIOC->ODR & (1 << ENA0)) && (floppy0_encoded_cylinder != floppy0_current_cylinder)) {
			floppy0_encoded_cylinder = floppy0_current_cylinder;
			if (floppy_type & 0x01) {
				i = floppy0_current_cylinder * 2 * RAW_TRACK_SIZE;
				encode_raw_track(floppy0_data + i, mfm_track_floppy0_head0, EMPTY_TRACK_MASK_FLOPPY0_HEAD0);
				encode_raw_track(floppy0_data + i + RAW_TRACK_SIZE, mfm_track_floppy0_head1, EMPTY_TRACK_MASK_FLOPPY0_HEAD1);
			} else {
				i = floppy0_current_cylinder * 2 * ADF_TRACK_SIZE;
				encode_mfm_track(floppy0_data + i, mfm_track_floppy0_head0, floppy0_current_cylinder, 0, EMPTY_TRACK_MASK_FLOPPY0_HEAD0);
				encode_mfm_track(floppy0_data + i + ADF_TRACK_SIZE, mfm_track_floppy0_head1, floppy0_current_cylinder, 1, EMPTY_TRACK_MASK_FLOPPY0_HEAD1);
			}
		}
		if ((GPIOC->ODR & (1 << ENA1)) && (floppy1_encoded_cylinder != floppy1_current_cylinder)) {
			floppy1_encoded_cylinder = floppy1_current_cylinder;
			if (floppy_type & 0x02) {
				i = floppy1_current_cylinder * 2 * RAW_TRACK_SIZE;
				encode_raw_track(floppy1_data + i, mfm_track_floppy1_head0, EMPTY_TRACK_MASK_FLOPPY1_HEAD0);
				encode_raw_track(floppy1_data + i + RAW_TRACK_SIZE, mfm_track_floppy1_head1, EMPTY_TRACK_MASK_FLOPPY1_HEAD1);
			} else {
				i = floppy1_current_cylinder * 2 * ADF_TRACK_SIZE;
				encode_mfm_track(floppy1_data + i, mfm_track_floppy1_head0, floppy1_current_cylinder, 0, EMPTY_TRACK_MASK_FLOPPY1_HEAD0);
				encode_mfm_track(floppy1_data + i + ADF_TRACK_SIZE, mfm_track_floppy1_head1, floppy1_current_cylinder, 1, EMPTY_TRACK_MASK_FLOPPY1_HEAD1);
			}
		}
		if ((GPIOA->ODR & (1 << ENA2)) && (floppy2_encoded_cylinder != floppy2_current_cylinder)) {
			floppy2_encoded_cylinder = floppy2_current_cylinder;
			if (floppy_type & 0x04) {
				i = floppy2_current_cylinder * 2 * RAW_TRACK_SIZE;
				encode_raw_track(floppy2_data + i, mfm_track_floppy2_head0, EMPTY_TRACK_MASK_FLOPPY2_HEAD0);
				encode_raw_track(floppy2_data + i + RAW_TRACK_SIZE, mfm_track_floppy2_head1, EMPTY_TRACK_MASK_FLOPPY2_HEAD1);
			} else {
				i = floppy2_current_cylinder * 2 * ADF_TRACK_SIZE;
				encode_mfm_track(floppy2_data + i, mfm_track_floppy2_head0, floppy2_current_cylinder, 0, EMPTY_TRACK_MASK_FLOPPY2_HEAD0);
				encode_mfm_track(floppy2_data + i + ADF_TRACK_SIZE, mfm_track_floppy2_head1, floppy2_current_cylinder, 1, EMPTY_TRACK_MASK_FLOPPY2_HEAD1);
			}
		}
		if ((GPIOA->ODR & (1 << ENA3)) && (floppy3_encoded_cylinder != floppy3_current_cylinder)) {
			floppy3_encoded_cylinder = floppy3_current_cylinder;
			if (floppy_type & 0x08) {
				i = floppy3_current_cylinder * 2 * RAW_TRACK_SIZE;
				encode_raw_track(floppy3_data + i, mfm_track_floppy3_head0, EMPTY_TRACK_MASK_FLOPPY3_HEAD0);
				encode_raw_track(floppy3_data + i + RAW_TRACK_SIZE, mfm_track_floppy3_head1, EMPTY_TRACK_MASK_FLOPPY3_HEAD1);
			} else {
				i = floppy3_current_cylinder * 2 * ADF_TRACK_SIZE;
				encode_mfm_track(floppy3_data + i, mfm_track_floppy3_head0, floppy3_current_cylinder, 0, EMPTY_TRACK_MASK_FLOPPY3_HEAD0);
				encode_mfm_track(floppy3_data + i + ADF_TRACK_SIZE, mfm_track_floppy3_head1, floppy3_current_cylinder, 1, EMPTY_TRACK_MASK_FLOPPY3_HEAD1);
			}
		}

		if (!tx_pending) {
			for (i = 1; i < sizeof(tx_buffer);) {
				if (tx_ptr < tx_end) {
					tx_pending = 1;
					switch (c = slip_encode(*tx_ptr)) {
						case -ESC:
							tx_buffer[i++] = ESC;
							break;
						case -1:
							tx_buffer[i++] = 0;
							tx_ptr = tx_end;
							break;
						default:
							tx_buffer[i++] = c;
							tx_ptr++;
							break;
					}
				} else {
					if (wifi_setup) {
						wifi_setup = 0;
						for (tx_ptr = (char *) &wifi_parameters; tx_ptr < (char *) &wifi_parameters + sizeof(struct wifi_parameters); tx_ptr++) {
							tx_buffer[i++] = *tx_ptr;
						}
						tx_ptr = tx_end;
					} else if (i < sizeof(tx_buffer)-4 && floppy0_dirty_tt_ri != floppy0_dirty_tt_wi) {
						// Start encoding track from floppy 0
						tx_pending = 1;
						tt = floppy0_dirty_tts[floppy0_dirty_tt_ri++ & (MAX_DIRTY_TTS-1)];
						track_size = (floppy_type & 0x01) ? RAW_TRACK_SIZE : ADF_TRACK_SIZE;
						tx_ptr = (char *) &floppy0_data[tt * track_size];
						tx_end = tx_ptr + track_size*4;
						tx_buffer[i++] = END;
						tx_buffer[i++] = 0;				// drive number
						switch (c = slip_encode(tt)) {
							case -ESC:
								tx_buffer[i++] = ESC;
								tx_buffer[i++] = slip_encode(tt);
								break;
							case -1:
								tx_buffer[i++] = 0;
								tx_ptr = tx_end;
								break;
							default:
								tx_buffer[i++] = c;
								break;
						}
					} else if (i < sizeof(tx_buffer)-4 && floppy1_dirty_tt_ri != floppy1_dirty_tt_wi) {
						// Start encoding track from floppy 1
						tx_pending = 1;
						tt = floppy1_dirty_tts[floppy1_dirty_tt_ri++ & (MAX_DIRTY_TTS-1)];
						track_size = (floppy_type & 0x02) ? RAW_TRACK_SIZE : ADF_TRACK_SIZE;
						tx_ptr = (char *) &floppy1_data[tt * track_size];
						tx_end = tx_ptr + track_size*4;
						tx_buffer[i++] = END;
						tx_buffer[i++] = 1;				// drive number
						switch (c = slip_encode(tt)) {
							case -ESC:
								tx_buffer[i++] = ESC;
								tx_buffer[i++] = slip_encode(tt);
								break;
							case -1:
								tx_buffer[i++] = 0;
								tx_ptr = tx_end;
								break;
							default:
								tx_buffer[i++] = c;
								break;
						}
					} else if (i < sizeof(tx_buffer)-4 && floppy2_dirty_tt_ri != floppy2_dirty_tt_wi) {
						// Start encoding track from floppy 2
						tx_pending = 1;
						tt = floppy2_dirty_tts[floppy2_dirty_tt_ri++ & (MAX_DIRTY_TTS-1)];
						track_size = (floppy_type & 0x04) ? RAW_TRACK_SIZE : ADF_TRACK_SIZE;
						tx_ptr = (char *) &floppy2_data[tt * track_size];
						tx_end = tx_ptr + track_size*4;
						tx_buffer[i++] = END;
						tx_buffer[i++] = 2;				// drive number
						switch (c = slip_encode(tt)) {
							case -ESC:
								tx_buffer[i++] = ESC;
								tx_buffer[i++] = slip_encode(tt);
								break;
							case -1:
								tx_buffer[i++] = 0;
								tx_ptr = tx_end;
								break;
							default:
								tx_buffer[i++] = c;
								break;
						}
					} else if (i < sizeof(tx_buffer)-4 && floppy3_dirty_tt_ri != floppy3_dirty_tt_wi) {
						// Start encoding track from floppy 3
						tx_pending = 1;
						tt = floppy3_dirty_tts[floppy3_dirty_tt_ri++ & (MAX_DIRTY_TTS-1)];
						track_size = (floppy_type & 0x08) ? RAW_TRACK_SIZE : ADF_TRACK_SIZE;
						tx_ptr = (char *) &floppy3_data[tt * track_size];
						tx_end = tx_ptr + track_size*4;
						tx_buffer[i++] = END;
						tx_buffer[i++] = 3;				// drive number
						switch (c = slip_encode(tt)) {
							case -ESC:
								tx_buffer[i++] = ESC;
								tx_buffer[i++] = slip_encode(tt);
								break;
							case -1:
								tx_buffer[i++] = 0;
								tx_ptr = tx_end;
								break;
							default:
								tx_buffer[i++] = c;
								break;
						}
					} else {
						tx_buffer[i++] = 0;
					}
				}
			}
			tx_buffer[0] = tx_pending;
		}
		GPIOC->BSRR = ((tx_ptr != tx_end) ? 0x10000 : 1) << LED_BLUE;

		if ((GPIOG->IDR & (1 << MREQ))) {
			GPIOC->BSRR = 0x10000 << LED_RED;
			tx_pending = 0;
			esp_transaction(tx_buffer, rx_buffer);
			for (i = 0; i < sizeof(rx_buffer); i++) {
				// Process next byte from input buffer
				c = slip_decode(rx_buffer[i]);
				if (c == -END) {
					rx_state = OP;
				} else if (c > -1) {
					if (rx_state == TRANSMIT) {
						*floppy_fill_ptr++ = c;
						sdram_exit_low_power_mode();
					} else if (rx_state == NOP) {
						// pass
					} else if (rx_state == OP) {
						switch (c) {
							case OP_NOP:
								rx_state = NOP;
								break;
							case OP_INSERT0:
								rx_state = NOP;
								sdram_exit_low_power_mode();
								if (floppy_type & 0x01) {
									encode_raw_track(floppy0_data, mfm_track_floppy0_head0, EMPTY_TRACK_MASK_FLOPPY0_HEAD0);
									encode_raw_track(floppy0_data + RAW_TRACK_SIZE, mfm_track_floppy0_head1, EMPTY_TRACK_MASK_FLOPPY0_HEAD1);
								} else {
									encode_mfm_track(floppy0_data, mfm_track_floppy0_head0, 0, 0, EMPTY_TRACK_MASK_FLOPPY0_HEAD0);
									encode_mfm_track(floppy0_data + ADF_TRACK_SIZE, mfm_track_floppy0_head1, 0, 1, EMPTY_TRACK_MASK_FLOPPY0_HEAD1);
								}
								floppy0_current_cylinder = 0;
								floppy0_encoded_cylinder = 0;
								floppy0_dirty_tt_ri = floppy0_dirty_tt_wi;
								GPIOC->BSRR = 0x10000 << FLOP0_TRK0;
								GPIOC->BSRR = 1 << ENA0;
								break;
							case OP_INSERT1:
								rx_state = NOP;
								sdram_exit_low_power_mode();
								if (floppy_type & 0x02) {
									encode_raw_track(floppy1_data, mfm_track_floppy1_head0, EMPTY_TRACK_MASK_FLOPPY1_HEAD0);
									encode_raw_track(floppy1_data + RAW_TRACK_SIZE, mfm_track_floppy1_head1, EMPTY_TRACK_MASK_FLOPPY1_HEAD1);
								} else {
									encode_mfm_track(floppy1_data, mfm_track_floppy1_head0, 0, 0, EMPTY_TRACK_MASK_FLOPPY1_HEAD0);
									encode_mfm_track(floppy1_data + ADF_TRACK_SIZE, mfm_track_floppy1_head1, 0, 1, EMPTY_TRACK_MASK_FLOPPY1_HEAD1);
								}
								floppy1_current_cylinder = 0;
								floppy1_encoded_cylinder = 0;
								floppy1_dirty_tt_ri = floppy1_dirty_tt_wi;
								GPIOA->BSRR = 0x10000 << FLOP1_TRK0;
								GPIOC->BSRR = 1 << ENA1;
								break;
							case OP_INSERT2:
								rx_state = NOP;
								sdram_exit_low_power_mode();
								if (floppy_type & 0x04) {
									encode_raw_track(floppy2_data, mfm_track_floppy2_head0, EMPTY_TRACK_MASK_FLOPPY2_HEAD0);
									encode_raw_track(floppy2_data + RAW_TRACK_SIZE, mfm_track_floppy2_head1, EMPTY_TRACK_MASK_FLOPPY2_HEAD1);
								} else {
									encode_mfm_track(floppy2_data, mfm_track_floppy2_head0, 0, 0, EMPTY_TRACK_MASK_FLOPPY2_HEAD0);
									encode_mfm_track(floppy2_data + ADF_TRACK_SIZE, mfm_track_floppy2_head1, 0, 1, EMPTY_TRACK_MASK_FLOPPY2_HEAD1);
								}
								floppy2_current_cylinder = 0;
								floppy2_encoded_cylinder = 0;
								floppy2_dirty_tt_ri = floppy2_dirty_tt_wi;
								GPIOA->BSRR = 0x10000 << FLOP2_TRK0;
								GPIOA->BSRR = 1 << ENA2;
								break;
							case OP_INSERT3:
								rx_state = NOP;
								sdram_exit_low_power_mode();
								if (floppy_type & 0x08) {
									encode_raw_track(floppy3_data, mfm_track_floppy3_head0, EMPTY_TRACK_MASK_FLOPPY3_HEAD0);
									encode_raw_track(floppy3_data + RAW_TRACK_SIZE, mfm_track_floppy3_head1, EMPTY_TRACK_MASK_FLOPPY3_HEAD1);
								} else {
									encode_mfm_track(floppy3_data, mfm_track_floppy3_head0, 0, 0, EMPTY_TRACK_MASK_FLOPPY3_HEAD0);
									encode_mfm_track(floppy3_data + ADF_TRACK_SIZE, mfm_track_floppy3_head1, 0, 1, EMPTY_TRACK_MASK_FLOPPY3_HEAD1);
								}
								floppy3_current_cylinder = 0;
								floppy3_encoded_cylinder = 0;
								floppy3_dirty_tt_ri = floppy3_dirty_tt_wi;
								GPIOB->BSRR = 0x10000 << FLOP3_TRK0;
								GPIOA->BSRR = 1 << ENA3;
								break;
							case OP_EJECT0:
								rx_state = NOP;
								GPIOC->BSRR = 1 << FLOP0_TRK0;
								GPIOC->BSRR = 0x10000 << ENA0;
								break;
							case OP_EJECT1:
								rx_state = NOP;
								GPIOA->BSRR = 1 << FLOP1_TRK0;
								GPIOC->BSRR = 0x10000 << ENA1;
								break;
							case OP_EJECT2:
								rx_state = NOP;
								GPIOA->BSRR = 1 << FLOP2_TRK0;
								GPIOA->BSRR = 0x10000 << ENA2;
								break;
							case OP_EJECT3:
								rx_state = NOP;
								GPIOB->BSRR = 1 << FLOP3_TRK0;
								GPIOA->BSRR = 0x10000 << ENA3;
								break;
							case OP_FILL0:
								rx_state = TRANSMIT;
								floppy_fill_ptr = (unsigned char *) floppy0_data;
								break;
							case OP_FILL1:
								rx_state = TRANSMIT;
								floppy_fill_ptr = (unsigned char *) floppy1_data;
								break;
							case OP_FILL2:
								rx_state = TRANSMIT;
								floppy_fill_ptr = (unsigned char *) floppy2_data;
								break;
							case OP_FILL3:
								rx_state = TRANSMIT;
								floppy_fill_ptr = (unsigned char *) floppy3_data;
								break;
							case OP_WPROT0:
								rx_state = NOP;
								floppy0_write_protected = 1;
								break;
							case OP_WPROT1:
								rx_state = NOP;
								floppy1_write_protected = 1;
								break;
							case OP_WPROT2:
								rx_state = NOP;
								floppy2_write_protected = 1;
								break;
							case OP_WPROT3:
								rx_state = NOP;
								floppy3_write_protected = 1;
								break;
							case OP_WUNPROT0:
								rx_state = NOP;
								floppy0_write_protected = 0;
								break;
							case OP_WUNPROT1:
								rx_state = NOP;
								floppy1_write_protected = 0;
								break;
							case OP_WUNPROT2:
								rx_state = NOP;
								floppy2_write_protected = 0;
								break;
							case OP_WUNPROT3:
								rx_state = NOP;
								floppy3_write_protected = 0;
								break;
							case OP_TYPE0_ADF:
								rx_state = NOP;
								floppy_type &= ~0x01;
								break;
							case OP_TYPE1_ADF:
								rx_state = NOP;
								floppy_type &= ~0x02;
								break;
							case OP_TYPE2_ADF:
								rx_state = NOP;
								floppy_type &= ~0x04;
								break;
							case OP_TYPE3_ADF:
								rx_state = NOP;
								floppy_type &= ~0x08;
								break;
							case OP_TYPE0_RAW:
								rx_state = NOP;
								floppy_type |= 0x01;
								break;
							case OP_TYPE1_RAW:
								rx_state = NOP;
								floppy_type |= 0x02;
								break;
							case OP_TYPE2_RAW:
								rx_state = NOP;
								floppy_type |= 0x04;
								break;
							case OP_TYPE3_RAW:
								rx_state = NOP;
								floppy_type |= 0x08;
								break;
							case OP_SETUP_WIFI:
								wifi_setup = 1;
								break;
							default:
								// error: unknown operation
								break;
						}
					}
				}
			}
		} else {
			GPIOC->BSRR = 1 << LED_RED;
		}
	}
}
