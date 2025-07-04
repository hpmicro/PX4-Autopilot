/****************************************************************************
 *
 *   Copyright (C) 2024 PX4 Development Team. All rights reserved.
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
 * @file drv_io_gpio.h
 *
 */

#pragma once
__BEGIN_DECLS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/*
 * GPIOM Selected
 */

#define GPIO_CONTROLLER_SHIFT (29) /* Bits 31-29: GPIOM */
#define GPIO_CONTROLLER_MASK  (7 << GPIO_CONTROLLER_SHIFT)
#define GPIO0                 (0 << GPIO_CONTROLLER_SHIFT)
#define GPIO1                 (1 << GPIO_CONTROLLER_SHIFT)
#define FGPIO0                (2 << GPIO_CONTROLLER_SHIFT)
#define FGPIO1                (3 << GPIO_CONTROLLER_SHIFT)
#define PGPIO                 (4 << GPIO_CONTROLLER_SHIFT)
#define BGPIO                 (5 << GPIO_CONTROLLER_SHIFT)

/*
 * GPIO Mode
 */

#define GPIO_MODE_SHIFT (27) /* Bits 28-27: Pin mode */
#define GPIO_MODE_MASK  (3 << GPIO_MODE_SHIFT)
#define GPIO_INPUT      (0 << GPIO_MODE_SHIFT) /* GPIO input */
#define GPIO_OUTPUT     (1 << GPIO_MODE_SHIFT) /* GPIO output */
#define GPIO_ALT        (2 << GPIO_MODE_SHIFT) /* Peripheral */
#define GPIO_ANALOG     (3 << GPIO_MODE_SHIFT) /* Analog */

/*
 * ALT Function
 */

#define GPIO_AF_SHIFT (22) /* Bits 26-22: Peripheral alternate function */
#define GPIO_AF_MASK  (0x1F << GPIO_AF_SHIFT)
#define GPIO_AF0      (0 << GPIO_AF_SHIFT)  /* Alternate function 1 */
#define GPIO_AF1      (1 << GPIO_AF_SHIFT)  /* Alternate function 1 */
#define GPIO_AF2      (2 << GPIO_AF_SHIFT)  /* Alternate function 2 */
#define GPIO_AF3      (3 << GPIO_AF_SHIFT)  /* Alternate function 3 */
#define GPIO_AF4      (4 << GPIO_AF_SHIFT)  /* Alternate function 4 */
#define GPIO_AF5      (5 << GPIO_AF_SHIFT)  /* Alternate function 5 */
#define GPIO_AF6      (6 << GPIO_AF_SHIFT)  /* Alternate function 6 */
#define GPIO_AF7      (7 << GPIO_AF_SHIFT)  /* Alternate function 7 */
#define GPIO_AF8      (8 << GPIO_AF_SHIFT)  /* Alternate function 8 */
#define GPIO_AF9      (9 << GPIO_AF_SHIFT)  /* Alternate function 9 */
#define GPIO_AF10     (10 << GPIO_AF_SHIFT) /* Alternate function 10 */
#define GPIO_AF11     (11 << GPIO_AF_SHIFT) /* Alternate function 11 */
#define GPIO_AF12     (12 << GPIO_AF_SHIFT) /* Alternate function 12 */
#define GPIO_AF13     (13 << GPIO_AF_SHIFT) /* Alternate function 13 */
#define GPIO_AF14     (14 << GPIO_AF_SHIFT) /* Alternate function 14 */
#define GPIO_AF15     (15 << GPIO_AF_SHIFT) /* Alternate function 15 */
#define GPIO_AF16     (16 << GPIO_AF_SHIFT) /* Alternate function 16 */
#define GPIO_AF17     (17 << GPIO_AF_SHIFT) /* Alternate function 17 */
#define GPIO_AF18     (18 << GPIO_AF_SHIFT) /* Alternate function 18 */
#define GPIO_AF19     (19 << GPIO_AF_SHIFT) /* Alternate function 19 */
#define GPIO_AF20     (20 << GPIO_AF_SHIFT) /* Alternate function 20 */
#define GPIO_AF21     (21 << GPIO_AF_SHIFT) /* Alternate function 21 */
#define GPIO_AF22     (22 << GPIO_AF_SHIFT) /* Alternate function 22 */
#define GPIO_AF23     (23 << GPIO_AF_SHIFT) /* Alternate function 23 */
#define GPIO_AF24     (24 << GPIO_AF_SHIFT) /* Alternate function 24 */
#define GPIO_AF25     (25 << GPIO_AF_SHIFT) /* Alternate function 25 */
#define GPIO_AF26     (26 << GPIO_AF_SHIFT) /* Alternate function 26 */
#define GPIO_AF27     (27 << GPIO_AF_SHIFT) /* Alternate function 27 */
#define GPIO_AF28     (28 << GPIO_AF_SHIFT) /* Alternate function 28 */
#define GPIO_AF29     (29 << GPIO_AF_SHIFT) /* Alternate function 29 */
#define GPIO_AF30     (30 << GPIO_AF_SHIFT) /* Alternate function 30 */
#define GPIO_AF31     (31 << GPIO_AF_SHIFT) /* Alternate function 31 */

/*
 * Open Drain/Push pull
 */
#define GPIO_OPENDRAIN (1 << 21)
#define GPIO_PUSHPULL  (0)

/*
 * Pull up/down
 */
#define GPIO_PUPD_SHIFT (19) /* Bits 19-20: Pull up/down */
#define GPIO_PUPD_MASK  (3 << GPIO_PUPD_SHIFT)
#define GPIO_FLOAT      (0 << GPIO_PUPD_SHIFT) /* No pull-up, No pull-down */
#define GPIO_PULLUP     (1 << GPIO_PUPD_SHIFT) /* Pull-up */
#define GPIO_PULLDOWN   (2 << GPIO_PUPD_SHIFT) /* Pull-down */

/*
 * Schmitt Input
 */
#define GPIO_SMT (1 << 18) /* Bit18: Schmitt trigger enable */

/*
 * Loop back output to input
 */
#define GPIO_LOOPBACK (1 << 17) /* Bit17: loop-back enable */

#if defined(CONFIG_ARCH_CHIP_HPM6750_SDK) && CONFIG_ARCH_CHIP_HPM6750_SDK
/*
 * GPIO Voltage
 */
#define GPIO_1V8 (1 << 16)
#define GPIO_3V3 (0)

#else
/*
 * Driver Speed
 */
#define GPIO_SPEED_FAST   (1 << 16)
#define GPIO_SPEED_SLOW   (0)

#endif

/* Outpus Init values
 *
 */
#define GPIO_OUTPUT_INIT_SHIFT (12)
#define GPIO_OUTPUT_SET        (1 << GPIO_OUTPUT_INIT_SHIFT)
#define GPIO_OUTPUT_CLEAR      (0)

/*
 * Driver Stength
 */
#define GPIO_DS_SHIFT (9) /* Bits 9-11: GPIO Driver Stength */
#define GPIO_DS_MASK  (7 << GPIO_DS_SHIFT)
#define GPIO_DS0      (0 << GPIO_DS_SHIFT)
#define GPIO_DS1      (1 << GPIO_DS_SHIFT)
#define GPIO_DS2      (2 << GPIO_DS_SHIFT)
#define GPIO_DS3      (3 << GPIO_DS_SHIFT)
#define GPIO_DS4      (4 << GPIO_DS_SHIFT)
#define GPIO_DS5      (5 << GPIO_DS_SHIFT)
#define GPIO_DS6      (6 << GPIO_DS_SHIFT)
#define GPIO_DS7      (7 << GPIO_DS_SHIFT)

/*
 * GPIO Ports
 */

#define GPIO_PORT_SHIFT (5) /* Bits 8-5 */
#define GPIO_PORT_MASK  (0x0F << GPIO_PORT_SHIFT)
#define GPIO_PORTA      (0 << GPIO_PORT_SHIFT)
#define GPIO_PORTB      (1 << GPIO_PORT_SHIFT)
#define GPIO_PORTC      (2 << GPIO_PORT_SHIFT)
#define GPIO_PORTD      (3 << GPIO_PORT_SHIFT)
#define GPIO_PORTE      (4 << GPIO_PORT_SHIFT)
#define GPIO_PORTF      (5 << GPIO_PORT_SHIFT)
#define GPIO_PORTX      (13 << GPIO_PORT_SHIFT)
#define GPIO_PORTY      (14 << GPIO_PORT_SHIFT)
#define GPIO_PORTZ      (15 << GPIO_PORT_SHIFT)

/*
 * GPIO Pins
 */
#define GPIO_PIN_SHIFT (0) /* Bits 4-0 */
#define GPIO_PIN_MASK  (0x1F << GPIO_PIN_SHIFT)
#define GPIO_PIN0      (0 << GPIO_PIN_SHIFT)  /* Pin  0 */
#define GPIO_PIN1      (1 << GPIO_PIN_SHIFT)  /* Pin  1 */
#define GPIO_PIN2      (2 << GPIO_PIN_SHIFT)  /* Pin  2 */
#define GPIO_PIN3      (3 << GPIO_PIN_SHIFT)  /* Pin  3 */
#define GPIO_PIN4      (4 << GPIO_PIN_SHIFT)  /* Pin  4 */
#define GPIO_PIN5      (5 << GPIO_PIN_SHIFT)  /* Pin  5 */
#define GPIO_PIN6      (6 << GPIO_PIN_SHIFT)  /* Pin  6 */
#define GPIO_PIN7      (7 << GPIO_PIN_SHIFT)  /* Pin  7 */
#define GPIO_PIN8      (8 << GPIO_PIN_SHIFT)  /* Pin  8 */
#define GPIO_PIN9      (9 << GPIO_PIN_SHIFT)  /* Pin  9 */
#define GPIO_PIN10     (10 << GPIO_PIN_SHIFT) /* Pin 10 */
#define GPIO_PIN11     (11 << GPIO_PIN_SHIFT) /* Pin 11 */
#define GPIO_PIN12     (12 << GPIO_PIN_SHIFT) /* Pin 12 */
#define GPIO_PIN13     (13 << GPIO_PIN_SHIFT) /* Pin 13 */
#define GPIO_PIN14     (14 << GPIO_PIN_SHIFT) /* Pin 14 */
#define GPIO_PIN15     (15 << GPIO_PIN_SHIFT) /* Pin 15 */
#define GPIO_PIN16     (16 << GPIO_PIN_SHIFT) /* Pin 16 */
#define GPIO_PIN17     (17 << GPIO_PIN_SHIFT) /* Pin 17 */
#define GPIO_PIN18     (18 << GPIO_PIN_SHIFT) /* Pin 18 */
#define GPIO_PIN19     (19 << GPIO_PIN_SHIFT) /* Pin 19 */
#define GPIO_PIN20     (20 << GPIO_PIN_SHIFT) /* Pin 20 */
#define GPIO_PIN21     (21 << GPIO_PIN_SHIFT) /* Pin 21 */
#define GPIO_PIN22     (22 << GPIO_PIN_SHIFT) /* Pin 22 */
#define GPIO_PIN23     (23 << GPIO_PIN_SHIFT) /* Pin 23 */
#define GPIO_PIN24     (24 << GPIO_PIN_SHIFT) /* Pin 24 */
#define GPIO_PIN25     (25 << GPIO_PIN_SHIFT) /* Pin 25 */
#define GPIO_PIN26     (26 << GPIO_PIN_SHIFT) /* Pin 26 */
#define GPIO_PIN27     (27 << GPIO_PIN_SHIFT) /* Pin 27 */
#define GPIO_PIN28     (28 << GPIO_PIN_SHIFT) /* Pin 28 */
#define GPIO_PIN29     (29 << GPIO_PIN_SHIFT) /* Pin 29 */
#define GPIO_PIN30     (30 << GPIO_PIN_SHIFT) /* Pin 30 */
#define GPIO_PIN31     (31 << GPIO_PIN_SHIFT) /* Pin 31 */


int hpm_config_gpio(uint32_t pinset);
int hpm_unconfig_gpio(uint32_t pinset);
void hpm_gpio_write(uint32_t pinset, bool value);
bool hpm_gpio_read(uint32_t pinset);
int hpm_gpio_setevent(uint32_t pinset, bool risingedge, bool fallingedge, bool event, xcpt_t func, void *arg);


__END_DECLS
