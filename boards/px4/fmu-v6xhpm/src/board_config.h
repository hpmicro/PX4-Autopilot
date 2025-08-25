/****************************************************************************
 *
 *   Copyright (c) 2018-2019 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file board_config.h
 *
 * PX4 fmu-v6xrt internal definitions
 */

#pragma once

/****************************************************************************************************
 * Included Files
 ****************************************************************************************************/

#include <nuttx/config.h>

#include <px4_platform_common/px4_config.h>
#include <nuttx/compiler.h>
#include <stdint.h>

#include <hpm_gpio.h>

#define HRT_TIMER 1
#define HRT_TIMER_CHANNEL 1 // HPM GPTMR channel 1

/* This board provides a DMA pool and APIs */
#define BOARD_DMA_ALLOC_POOL_SIZE 5120

#define BOARD_ENABLE_CONSOLE_BUFFER

#define PX4_I2C_BUS_MTD	1

#define ADC_BATTERY_VOLTAGE_CHANNEL  0
#define ADC_BATTERY_CURRENT_CHANNEL  1

/* RC Serial port */

#define RC_SERIAL_PORT                     "/dev/ttyS6"
#define RC_SERIAL_SINGLEWIRE

#define RC_INVERT_INPUT(invert) rc_input_invert(invert)

#define GPIO_nSAFETY_SWITCH_LED_OUT_INIT   /* PF07 */ (GPIO_INPUT|GPIO_FLOAT|GPIO_PORTF|GPIO_PIN7)
#define GPIO_nSAFETY_SWITCH_LED_OUT        /* PF07 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_OUTPUT_SET|GPIO_PORTF|GPIO_PIN7)

/* Enable the FMU to control it if there is no px4io fixme:This should be BOARD_SAFETY_LED(__ontrue) */
#define GPIO_LED_SAFETY GPIO_nSAFETY_SWITCH_LED_OUT

#define GPIO_SAFETY_SWITCH_IN              /* PF05 */ (GPIO_INPUT|GPIO_SMT|GPIO_PORTF|GPIO_PIN5)
/* Enable the FMU to use the switch it if there is no px4io fixme:This should be BOARD_SAFTY_BUTTON() */
#define GPIO_BTN_SAFETY GPIO_SAFETY_SWITCH_IN /* Enable the FMU to control it if there is no px4io */

__BEGIN_DECLS

/****************************************************************************************************
 * Public Types
 ****************************************************************************************************/

/****************************************************************************************************
 * Public data
 ****************************************************************************************************/

#ifndef __ASSEMBLY__

/****************************************************************************************************
 * Public Functions
 ****************************************************************************************************/

extern int hpm_sdioinitialize(void);

extern void hpm_spiinitialize(void);

extern int hpm_usbinitialize(void);

extern void board_peripheral_reset(int ms);

extern int board_read_VBUS_state(void);

extern void rc_input_invert(bool invert);

#include <px4_platform_common/board_common.h>

#endif /* __ASSEMBLY__ */

__END_DECLS
