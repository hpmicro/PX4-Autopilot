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


/* I2C busses */

/* Devices on the onboard buses.
 *
 * Note that these are unshifted addresses.
 */
#define BOARD_MTD_NUM_EEPROM        2 /* MTD: base_eeprom, imu_eeprom*/


#define ADC_BATTERY_VOLTAGE_CHANNEL  0
#define ADC_BATTERY_CURRENT_CHANNEL  1

/* RC Serial port: UART1 on PB24/PB25 (Receiver connector CN4) */
#define RC_SERIAL_PORT                "/dev/ttyS1"
#define RC_SERIAL_SINGLEWIRE
#define RC_INVERT_INPUT(invert)       rc_input_invert(invert)

/* I2C Pins */
#define GPIO_I2C0_SCL_GPIO (GPIO_OUTPUT | GPIO_OPENDRAIN  | GPIO_PORTA | GPIO_PIN23 | GPIO_LOOPBACK)
#define GPIO_I2C0_SDA_GPIO (GPIO_OUTPUT | GPIO_OPENDRAIN  | GPIO_PORTA | GPIO_PIN24 | GPIO_LOOPBACK)
#define GPIO_I2C0_SCL      (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN  | GPIO_PORTA | GPIO_PIN23 | GPIO_LOOPBACK)
#define GPIO_I2C0_SDA      (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN  | GPIO_PORTA | GPIO_PIN24 | GPIO_LOOPBACK)

#define GPIO_I2C1_SCL_GPIO (GPIO_OUTPUT | GPIO_OPENDRAIN  | GPIO_PORTA | GPIO_PIN25 | GPIO_LOOPBACK)
#define GPIO_I2C1_SDA_GPIO (GPIO_OUTPUT | GPIO_OPENDRAIN  | GPIO_PORTA | GPIO_PIN26 | GPIO_LOOPBACK)
#define GPIO_I2C1_SCL      (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN  | GPIO_PORTA | GPIO_PIN25 | GPIO_LOOPBACK)
#define GPIO_I2C1_SDA      (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN  | GPIO_PORTA | GPIO_PIN26 | GPIO_LOOPBACK)

/* SPI Pins */
#define GPIO_SPI0_SCK  (GPIO_ALT | GPIO_AF5  | GPIO_PORTA | GPIO_PIN30 | GPIO_LOOPBACK)
#define GPIO_SPI0_MISO (GPIO_ALT | GPIO_AF5  | GPIO_PORTA | GPIO_PIN29)
#define GPIO_SPI0_MOSI (GPIO_ALT | GPIO_AF5  | GPIO_PORTA | GPIO_PIN31)

#define GPIO_SPI3_SCK  (GPIO_ALT | GPIO_AF5  | GPIO_PORTC | GPIO_PIN20 | GPIO_LOOPBACK)
#define GPIO_SPI3_MISO (GPIO_ALT | GPIO_AF5  | GPIO_PORTC | GPIO_PIN19)
#define GPIO_SPI3_MOSI (GPIO_ALT | GPIO_AF5  | GPIO_PORTC | GPIO_PIN21)

/* XPI1 NOR flash (XT25F128FSSIGU, 16MB QSPI) */
#define GPIO_XPI1_SCK  (GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_23 | GPIO_PORTB | GPIO_PIN14)
#define GPIO_XPI1_D0   (GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_23 | GPIO_PORTB | GPIO_PIN12)
#define GPIO_XPI1_D1   (GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_23 | GPIO_PORTB | GPIO_PIN15)
#define GPIO_XPI1_D2   (GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_23 | GPIO_PORTB | GPIO_PIN13)
#define GPIO_XPI1_D3   (GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_23 | GPIO_PORTB | GPIO_PIN16)
#define GPIO_XPI1_CS   (GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_23 | GPIO_PORTB | GPIO_PIN17)

/* Board LEDs: PB31 (LED1) and PB01 (LED2), active-high. */
#define GPIO_LED1_INIT                    /* PB31 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN31)
#define GPIO_LED1_ON                      /* PB31 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_OUTPUT_SET|GPIO_PORTB|GPIO_PIN31)
#define GPIO_LED1_OFF                     /* PB31 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN31)
#define GPIO_LED2_INIT                    /* PB01 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN1)
#define GPIO_LED2_ON                      /* PB01 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_OUTPUT_SET|GPIO_PORTB|GPIO_PIN1)
#define GPIO_LED2_OFF                     /* PB01 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN1)

/* This board has no safety button: no GPIO_BTN_SAFETY / GPIO_LED_SAFETY
 * definitions, and the safety_button driver is disabled in the board
 * configuration.  Arming is not gated by a safety switch.
 */

/* PWM */
#define BOARD_NUM_IO_TIMERS         2
#define DIRECT_PWM_OUTPUT_CHANNELS  4

/* Input Capture not supported */

#define BOARD_HAS_NO_CAPTURE

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

extern int hpm_init_can_pins(int port);

#include <px4_platform_common/board_common.h>

#endif /* __ASSEMBLY__ */

__END_DECLS
