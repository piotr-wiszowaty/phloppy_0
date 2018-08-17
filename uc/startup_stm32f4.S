// phloppy_0 - Commodore Amiga floppy drive emulator
// Copyright (C) 2016-2018 Piotr Wiszowaty
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

	.equ	STACK_FILL, 0xdeadbeef

	.text
	.balign 2
	.syntax	unified
	.cpu	cortex-m4
	.fpu	softvfp
	.thumb
	.thumb_func
	.global Reset_Handler

Reset_Handler:
	movs	r4, #4

	ldr	r1, =__data_init_start
    	ldr	r2, =__data_start
    	ldr	r3, =__data_end
data_init_loop:
	cmp	r2, r3
	beq	data_copy_done
	ldr	r0, [r1]
	str	r0, [r2]
	add	r1, r4
	add	r2, r4
	b	data_init_loop
data_copy_done:

	movs	r0, #0
	ldr	r1, =__bss_start
	ldr	r2, =__bss_end
bss_clear_loop:
	cmp	r1, r2
	beq	bss_clear_done
	str	r0, [r1]
	add	r1, r4
	b	bss_clear_loop
bss_clear_done:

	ldr	r0, =STACK_FILL
	ldr	r1, =__stack_start
	ldr	r2, =__stack_end
stack_fill_loop:
	cmp	r1, r2
	beq	stack_fill_done
	str	r0, [r1]
	add	r1, r4
	b	stack_fill_loop
stack_fill_done:

	ldr	r0, =main
	blx	r0
	b	.

__Default_Handler:
	ldr	r3, NVIC_INT_CTRL_CONST
	ldr	r2, [r3, #0]
	uxtb	r2, r2
	b	.

NVIC_INT_CTRL_CONST: .word 0xe000ed04

	.section .vectors, "a", %progbits
vectors:
	.word	__stack_end
	.word	Reset_Handler			// Reset
	.word	NMI_Handler			// Non-maskable interrupt (RCC clock security system)
	.word	HardFault_Handler		// All class of fault
	.word	MemManage_Handler		// Memory management
	.word	BusFault_Handler		// Pre-fetch fault, memory access fault
	.word	UsageFault_Handler		// Undefined instruction or illegal state
	.word	__Default_Handler		// Reserved 0x1C
	.word	__Default_Handler		// Reserved 0x20
	.word	__Default_Handler		// Reserved 0x24
	.word	__Default_Handler		// Reserved 0x28
	.word	SVC_Handler			// System service call via SWI instruction
	.word	DebugMon_Handler		// Debug monitor
	.word	__Default_Handler		// Reserved 0x34
	.word	PendSV_Handler			// Pendable request for system service
	.word	SysTick_Handler			// System tick timer
	.word	WWDG_IRQHandler			// Window WatchDog
	.word	PVD_IRQHandler			// PVD through EXTI Line detection
	.word	TAMP_STAMP_IRQHandler		// Tamper and TimeStamps through the EXTI line
	.word	RTC_WKUP_IRQHandler		// RTC Wakeup through the EXTI line
	.word	FLASH_IRQHandler		// FLASH
	.word	RCC_IRQHandler			// RCC
	.word	EXTI0_IRQHandler		// EXTI Line0
	.word	EXTI1_IRQHandler		// EXTI Line1
	.word	EXTI2_IRQHandler		// EXTI Line2
	.word	EXTI3_IRQHandler		// EXTI Line3
	.word	EXTI4_IRQHandler		// EXTI Line4
	.word	DMA1_Stream0_IRQHandler		// DMA1 Stream 0
	.word	DMA1_Stream1_IRQHandler		// DMA1 Stream 1
	.word	DMA1_Stream2_IRQHandler		// DMA1 Stream 2
	.word	DMA1_Stream3_IRQHandler		// DMA1 Stream 3
	.word	DMA1_Stream4_IRQHandler		// DMA1 Stream 4
	.word	DMA1_Stream5_IRQHandler		// DMA1 Stream 5
	.word	DMA1_Stream6_IRQHandler		// DMA1 Stream 6
	.word	ADC_IRQHandler			// ADC1, ADC2 and ADC3s
	.word	CAN1_TX_IRQHandler		// CAN1 TX
	.word	CAN1_RX0_IRQHandler		// CAN1 RX0
	.word	CAN1_RX1_IRQHandler		// CAN1 RX1
	.word	CAN1_SCE_IRQHandler		// CAN1 SCE
	.word	EXTI9_5_IRQHandler		// External Line[9:5]s
	.word	TIM1_BRK_TIM9_IRQHandler	// TIM1 Break and TIM9
	.word	TIM1_UP_TIM10_IRQHandler	// TIM1 Update and TIM10
	.word	TIM1_TRG_COM_TIM11_IRQHandler	// TIM1 Trigger and Commutation and TIM11
	.word	TIM1_CC_IRQHandler		// TIM1 Capture Compare
	.word	TIM2_IRQHandler			// TIM2
	.word	TIM3_IRQHandler			// TIM3
	.word	TIM4_IRQHandler			// TIM4
	.word	I2C1_EV_IRQHandler		// I2C1 Event
	.word	I2C1_ER_IRQHandler		// I2C1 Error
	.word	I2C2_EV_IRQHandler		// I2C2 Event
	.word	I2C2_ER_IRQHandler		// I2C2 Error
	.word	SPI1_IRQHandler			// SPI1
	.word	SPI2_IRQHandler			// SPI2
	.word	USART1_IRQHandler		// USART1
	.word	USART2_IRQHandler		// USART2
	.word	USART3_IRQHandler		// USART3
	.word	EXTI15_10_IRQHandler		// External Line[15:10]s
	.word	RTC_Alarm_IRQHandler		// RTC Alarm (A and B) through EXTI Line
	.word	OTG_FS_WKUP_IRQHandler		// USB OTG FS Wakeup through EXTI line
	.word	TIM8_BRK_TIM12_IRQHandler	// TIM8 Break and TIM12
	.word	TIM8_UP_TIM13_IRQHandler	// TIM8 Update and TIM13
	.word	TIM8_TRG_COM_TIM14_IRQHandler	// TIM8 Trigger and Commutation and TIM14
	.word	TIM8_CC_IRQHandler		// TIM8 Capture Compare
	.word	DMA1_Stream7_IRQHandler		// DMA1 Stream7
	.word	FMC_IRQHandler			// FMC
	.word	SDIO_IRQHandler			// SDIO
	.word	TIM5_IRQHandler			// TIM5
	.word	SPI3_IRQHandler			// SPI3
	.word	UART4_IRQHandler		// UART4
	.word	UART5_IRQHandler		// UART5
	.word	TIM6_DAC_IRQHandler		// TIM6 and DAC1&2 underrun errors
	.word	TIM7_IRQHandler			// TIM7
	.word	DMA2_Stream0_IRQHandler		// DMA2 Stream 0
	.word	DMA2_Stream1_IRQHandler		// DMA2 Stream 1
	.word	DMA2_Stream2_IRQHandler		// DMA2 Stream 2
	.word	DMA2_Stream3_IRQHandler		// DMA2 Stream 3
	.word	DMA2_Stream4_IRQHandler		// DMA2 Stream 4
	.word	ETH_IRQHandler			// Ethernet
	.word	ETH_WKUP_IRQHandler		// Ethernet Wakeup through EXTI line
	.word	CAN2_TX_IRQHandler		// CAN2 TX
	.word	CAN2_RX0_IRQHandler		// CAN2 RX0
	.word	CAN2_RX1_IRQHandler		// CAN2 RX1
	.word	CAN2_SCE_IRQHandler		// CAN2 SCE
	.word	OTG_FS_IRQHandler		// USB OTG FS
	.word	DMA2_Stream5_IRQHandler		// DMA2 Stream 5
	.word	DMA2_Stream6_IRQHandler		// DMA2 Stream 6
	.word	DMA2_Stream7_IRQHandler		// DMA2 Stream 7
	.word	USART6_IRQHandler		// USART6
	.word	I2C3_EV_IRQHandler		// I2C3 event
	.word	I2C3_ER_IRQHandler		// I2C3 error
	.word	OTG_HS_EP1_OUT_IRQHandler	// USB OTG HS End Point 1 Out
	.word	OTG_HS_EP1_IN_IRQHandler	// USB OTG HS End Point 1 In
	.word	OTG_HS_WKUP_IRQHandler		// USB OTG HS Wakeup through EXTI
	.word	OTG_HS_IRQHandler		// USB OTG HS
	.word	DCMI_IRQHandler			// DCMI
	.word	__Default_Handler		// Reserved
	.word	HASH_RNG_IRQHandler		// Hash and Rng
	.word	FPU_IRQHandler			// FPU
	.word	UART7_IRQHandler		// UART7
	.word	UART8_IRQHandler		// UART8
	.word	SPI4_IRQHandler			// SPI4
	.word	SPI5_IRQHandler			// SPI5 						
	.word	SPI6_IRQHandler			// SPI6						
	.word	SAI1_IRQHandler			// SAI1						
	.word	LTDC_IRQHandler			// LTDC_IRQHandler			
	.word	LTDC_ER_IRQHandler		// LTDC_ER_IRQHandler			
	.word	DMA2D_IRQHandler		// DMA2D

	.weak   NMI_Handler
	.thumb_set NMI_Handler, __Default_Handler

	.weak	HardFault_Handler
	.thumb_set HardFault_Handler, __Default_Handler

	.weak	MemManage_Handler
	.thumb_set MemManage_Handler, __Default_Handler

	.weak	BusFault_Handler
	.thumb_set BusFault_Handler, __Default_Handler

	.weak	BusFault_Handler
	.thumb_set BusFault_Handler, __Default_Handler

	.weak	UsageFault_Handler
	.thumb_set UsageFault_Handler, __Default_Handler

	.weak	SVC_Handler
	.thumb_set SVC_Handler, __Default_Handler

	.weak	DebugMon_Handler
	.thumb_set DebugMon_Handler, __Default_Handler

	.weak	PendSV_Handler
	.thumb_set PendSV_Handler, __Default_Handler

	.weak	SysTick_Handler
	.thumb_set SysTick_Handler, __Default_Handler

	.weak	WWDG_IRQHandler
	.thumb_set WWDG_IRQHandler, __Default_Handler

	.weak	PVD_IRQHandler
	.thumb_set PVD_IRQHandler, __Default_Handler

	.weak	TAMP_STAMP_IRQHandler
	.thumb_set TAMP_STAMP_IRQHandler, __Default_Handler

	.weak	RTC_WKUP_IRQHandler
	.thumb_set RTC_WKUP_IRQHandler, __Default_Handler

	.weak	FLASH_IRQHandler
	.thumb_set FLASH_IRQHandler, __Default_Handler

	.weak	RCC_IRQHandler
	.thumb_set RCC_IRQHandler, __Default_Handler

	.weak	EXTI0_IRQHandler
	.thumb_set EXTI0_IRQHandler, __Default_Handler

	.weak	EXTI1_IRQHandler
	.thumb_set EXTI1_IRQHandler, __Default_Handler

	.weak	EXTI2_IRQHandler
	.thumb_set EXTI2_IRQHandler, __Default_Handler

	.weak	EXTI3_IRQHandler
	.thumb_set EXTI3_IRQHandler, __Default_Handler

	.weak	EXTI4_IRQHandler
	.thumb_set EXTI4_IRQHandler, __Default_Handler

	.weak	DMA1_Stream0_IRQHandler
	.thumb_set DMA1_Stream0_IRQHandler, __Default_Handler

	.weak	DMA1_Stream1_IRQHandler
	.thumb_set DMA1_Stream1_IRQHandler, __Default_Handler

	.weak	DMA1_Stream2_IRQHandler
	.thumb_set DMA1_Stream2_IRQHandler, __Default_Handler

	.weak	DMA1_Stream3_IRQHandler
	.thumb_set DMA1_Stream3_IRQHandler, __Default_Handler

	.weak	DMA1_Stream4_IRQHandler
	.thumb_set DMA1_Stream4_IRQHandler, __Default_Handler

	.weak	DMA1_Stream5_IRQHandler
	.thumb_set DMA1_Stream5_IRQHandler, __Default_Handler

	.weak	DMA1_Stream6_IRQHandler
	.thumb_set DMA1_Stream6_IRQHandler, __Default_Handler

	.weak	ADC_IRQHandler
	.thumb_set ADC_IRQHandler, __Default_Handler

	.weak	CAN1_TX_IRQHandler
	.thumb_set CAN1_TX_IRQHandler, __Default_Handler

	.weak	CAN1_RX0_IRQHandler
	.thumb_set CAN1_RX0_IRQHandler, __Default_Handler

	.weak	CAN1_RX1_IRQHandler
	.thumb_set CAN1_RX1_IRQHandler, __Default_Handler

	.weak	CAN1_SCE_IRQHandler
	.thumb_set CAN1_SCE_IRQHandler, __Default_Handler

	.weak	EXTI9_5_IRQHandler
	.thumb_set EXTI9_5_IRQHandler, __Default_Handler

	.weak	TIM1_BRK_TIM9_IRQHandler
	.thumb_set TIM1_BRK_TIM9_IRQHandler, __Default_Handler

	.weak	TIM1_UP_TIM10_IRQHandler
	.thumb_set TIM1_UP_TIM10_IRQHandler, __Default_Handler

	.weak	TIM1_TRG_COM_TIM11_IRQHandler
	.thumb_set TIM1_TRG_COM_TIM11_IRQHandler, __Default_Handler

	.weak	TIM1_CC_IRQHandler
	.thumb_set TIM1_CC_IRQHandler, __Default_Handler

	.weak	TIM2_IRQHandler
	.thumb_set TIM2_IRQHandler, __Default_Handler

	.weak	TIM3_IRQHandler
	.thumb_set TIM3_IRQHandler, __Default_Handler

	.weak	TIM4_IRQHandler
	.thumb_set TIM4_IRQHandler, __Default_Handler

	.weak	I2C1_EV_IRQHandler
	.thumb_set I2C1_EV_IRQHandler, __Default_Handler

	.weak	I2C1_ER_IRQHandler
	.thumb_set I2C1_ER_IRQHandler, __Default_Handler

	.weak	I2C2_EV_IRQHandler
	.thumb_set I2C2_EV_IRQHandler, __Default_Handler

	.weak	I2C2_ER_IRQHandler
	.thumb_set I2C2_ER_IRQHandler, __Default_Handler

	.weak	SPI1_IRQHandler
	.thumb_set SPI1_IRQHandler, __Default_Handler

	.weak	SPI2_IRQHandler
	.thumb_set SPI2_IRQHandler, __Default_Handler

	.weak	USART1_IRQHandler
	.thumb_set USART1_IRQHandler, __Default_Handler

	.weak	USART2_IRQHandler
	.thumb_set USART2_IRQHandler, __Default_Handler

	.weak	USART3_IRQHandler
	.thumb_set USART3_IRQHandler, __Default_Handler

	.weak	EXTI15_10_IRQHandler
	.thumb_set EXTI15_10_IRQHandler, __Default_Handler

	.weak	RTC_Alarm_IRQHandler
	.thumb_set RTC_Alarm_IRQHandler, __Default_Handler

	.weak	OTG_FS_WKUP_IRQHandler
	.thumb_set OTG_FS_WKUP_IRQHandler, __Default_Handler

	.weak	TIM8_BRK_TIM12_IRQHandler
	.thumb_set TIM8_BRK_TIM12_IRQHandler, __Default_Handler

	.weak	TIM8_UP_TIM13_IRQHandler
	.thumb_set TIM8_UP_TIM13_IRQHandler, __Default_Handler

	.weak	TIM8_TRG_COM_TIM14_IRQHandler
	.thumb_set TIM8_TRG_COM_TIM14_IRQHandler, __Default_Handler

	.weak	TIM8_CC_IRQHandler
	.thumb_set TIM8_CC_IRQHandler, __Default_Handler

	.weak	DMA1_Stream7_IRQHandler
	.thumb_set DMA1_Stream7_IRQHandler, __Default_Handler

	.weak	FMC_IRQHandler
	.thumb_set FMC_IRQHandler, __Default_Handler

	.weak	SDIO_IRQHandler
	.thumb_set SDIO_IRQHandler, __Default_Handler

	.weak	TIM5_IRQHandler
	.thumb_set TIM5_IRQHandler, __Default_Handler

	.weak	SPI3_IRQHandler
	.thumb_set SPI3_IRQHandler, __Default_Handler

	.weak	UART4_IRQHandler
	.thumb_set UART4_IRQHandler, __Default_Handler

	.weak	UART5_IRQHandler
	.thumb_set UART5_IRQHandler, __Default_Handler

	.weak	TIM6_DAC_IRQHandler
	.thumb_set TIM6_DAC_IRQHandler, __Default_Handler

	.weak	TIM7_IRQHandler
	.thumb_set TIM7_IRQHandler, __Default_Handler

	.weak	DMA2_Stream0_IRQHandler
	.thumb_set DMA2_Stream0_IRQHandler, __Default_Handler

	.weak	DMA2_Stream1_IRQHandler
	.thumb_set DMA2_Stream1_IRQHandler, __Default_Handler

	.weak	DMA2_Stream2_IRQHandler
	.thumb_set DMA2_Stream2_IRQHandler, __Default_Handler

	.weak	DMA2_Stream3_IRQHandler
	.thumb_set DMA2_Stream3_IRQHandler, __Default_Handler

	.weak	DMA2_Stream4_IRQHandler
	.thumb_set DMA2_Stream4_IRQHandler, __Default_Handler

	.weak	ETH_IRQHandler
	.thumb_set ETH_IRQHandler, __Default_Handler

	.weak	ETH_WKUP_IRQHandler
	.thumb_set ETH_WKUP_IRQHandler, __Default_Handler

	.weak	CAN2_TX_IRQHandler
	.thumb_set CAN2_TX_IRQHandler, __Default_Handler

	.weak	CAN2_RX0_IRQHandler
	.thumb_set CAN2_RX0_IRQHandler, __Default_Handler

	.weak	CAN2_RX1_IRQHandler
	.thumb_set CAN2_RX1_IRQHandler, __Default_Handler

	.weak	CAN2_SCE_IRQHandler
	.thumb_set CAN2_SCE_IRQHandler, __Default_Handler

	.weak	OTG_FS_IRQHandler
	.thumb_set OTG_FS_IRQHandler, __Default_Handler

	.weak	DMA2_Stream5_IRQHandler
	.thumb_set DMA2_Stream5_IRQHandler, __Default_Handler

	.weak	DMA2_Stream6_IRQHandler
	.thumb_set DMA2_Stream6_IRQHandler, __Default_Handler

	.weak	DMA2_Stream7_IRQHandler
	.thumb_set DMA2_Stream7_IRQHandler, __Default_Handler

	.weak	USART6_IRQHandler
	.thumb_set USART6_IRQHandler, __Default_Handler

	.weak	I2C3_EV_IRQHandler
	.thumb_set I2C3_EV_IRQHandler, __Default_Handler

	.weak	I2C3_ER_IRQHandler
	.thumb_set I2C3_ER_IRQHandler, __Default_Handler

	.weak	OTG_HS_EP1_OUT_IRQHandler
	.thumb_set OTG_HS_EP1_OUT_IRQHandler, __Default_Handler

	.weak	OTG_HS_EP1_IN_IRQHandler
	.thumb_set OTG_HS_EP1_IN_IRQHandler, __Default_Handler

	.weak	OTG_HS_WKUP_IRQHandler
	.thumb_set OTG_HS_WKUP_IRQHandler, __Default_Handler

	.weak	OTG_HS_IRQHandler
	.thumb_set OTG_HS_IRQHandler, __Default_Handler

	.weak	DCMI_IRQHandler
	.thumb_set DCMI_IRQHandler, __Default_Handler

	.weak	HASH_RNG_IRQHandler
	.thumb_set HASH_RNG_IRQHandler, __Default_Handler

	.weak	FPU_IRQHandler
	.thumb_set FPU_IRQHandler, __Default_Handler

	.weak	UART7_IRQHandler
	.thumb_set UART7_IRQHandler, __Default_Handler

	.weak	UART8_IRQHandler
	.thumb_set UART8_IRQHandler, __Default_Handler

	.weak	SPI4_IRQHandler
	.thumb_set SPI4_IRQHandler, __Default_Handler

	.weak	SPI5_IRQHandler
	.thumb_set SPI5_IRQHandler, __Default_Handler

	.weak	SPI6_IRQHandler
	.thumb_set SPI6_IRQHandler, __Default_Handler

	.weak	SAI1_IRQHandler
	.thumb_set SAI1_IRQHandler, __Default_Handler

	.weak	LTDC_IRQHandler
	.thumb_set LTDC_IRQHandler, __Default_Handler

	.weak	LTDC_ER_IRQHandler
	.thumb_set LTDC_ER_IRQHandler, __Default_Handler

	.weak	DMA2D_IRQHandler
	.thumb_set DMA2D_IRQHandler, __Default_Handler
