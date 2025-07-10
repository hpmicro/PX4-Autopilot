/************************************************************************************
 * nuttx-configs/px4/fmu-v6xrt/include/board.h
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Authors: Gregory Nutt <gnutt@nuttx.org>
 *            David Sidrane <david_s5@nscdg.com>
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
 * 3. Neither the name NuttX nor the names of its contributors may be
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
 ************************************************************************************/

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/


// #define USE_XPI_MTD  // 使用XPI驱动MTD, 需要CONFIG_HPM_XPI_DRV和CONFIG_HPM_XPI1


/* Clocking *************************************************************************/

/* SDIO *********************************************************************/

/* Pin drive characteristics - drive strength in particular may need tuning
 * for specific boards.
 */



/* LED definitions ******************************************************************/
/* The px4 fmu-v6x board has numerous LEDs but only three, LED_GREEN a Green LED,
 * LED_BLUE a Blue LED and LED_RED a Red LED, that can be controlled by software.
 *
 * If CONFIG_ARCH_LEDS is not defined, then the user can control the LEDs in any way.
 * The following definitions are used to access individual LEDs.
 */

/* LED index values for use with board_userled() */

#define BOARD_LED1        0
#define BOARD_LED2        1
#define BOARD_LED3        2
#define BOARD_NLEDS       3

#define BOARD_LED_RED     BOARD_LED1
#define BOARD_LED_GREEN   BOARD_LED2
#define BOARD_LED_BLUE    BOARD_LED3

/* LED bits for use with board_userled_all() */

#define BOARD_LED1_BIT    (1 << BOARD_LED1)
#define BOARD_LED2_BIT    (1 << BOARD_LED2)
#define BOARD_LED3_BIT    (1 << BOARD_LED3)

/* If CONFIG_ARCH_LEDS is defined, the usage by the board port is defined in
 * include/board.h and src/stm32_leds.c. The LEDs are used to encode OS-related
 * events as follows:
 *
 *
 *   SYMBOL                     Meaning                      LED state
 *                                                        Red   Green Blue
 *   ----------------------  --------------------------  ------ ------ ----*/

#define LED_STARTED        0 /* NuttX has been started   OFF    OFF   OFF  */
#define LED_HEAPALLOCATE   1 /* Heap has been allocated  OFF    OFF   ON   */
#define LED_IRQSENABLED    2 /* Interrupts enabled       OFF    ON    OFF  */
#define LED_STACKCREATED   3 /* Idle stack created       OFF    ON    ON   */
#define LED_INIRQ          4 /* In an interrupt          N/C    N/C   GLOW */
#define LED_SIGNAL         5 /* In a signal handler      N/C    GLOW  N/C  */
#define LED_ASSERTION      6 /* An assertion failed      GLOW   N/C   GLOW */
#define LED_PANIC          7 /* The system has crashed   Blink  OFF   N/C  */
#define LED_IDLE           8 /* MCU is is sleep mode     ON     OFF   OFF  */

#ifdef USE_XPI_MTD
/* XPI 引脚配置 */

// XPI1.A.CA
#define GPIO_XPI1_SCK  	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTC | GPIO_PIN17 | GPIO_LOOPBACK)
#define GPIO_XPI1_D0 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTC | GPIO_PIN22)
#define GPIO_XPI1_D1 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTC | GPIO_PIN26)
#define GPIO_XPI1_CS 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTC | GPIO_PIN16)

// // XPI1.A.CB
// #define GPIO_XPI1_SCK  	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTC | GPIO_PIN15 | GPIO_LOOPBACK)
// #define GPIO_XPI1_D0 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTC | GPIO_PIN12)
// #define GPIO_XPI1_D1 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTC | GPIO_PIN11)
// #define GPIO_XPI1_CS 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTC | GPIO_PIN10)

// // XPI1.B.CA
// #define GPIO_XPI1_SCK  	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTD | GPIO_PIN10 | GPIO_LOOPBACK)
// #define GPIO_XPI1_D0 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTD | GPIO_PIN15)
// #define GPIO_XPI1_D1 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTD | GPIO_PIN12)
// #define GPIO_XPI1_CS 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTD | GPIO_PIN13)

// // XPI1.B.CB
// #define GPIO_XPI1_SCK  	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTC | GPIO_PIN31 | GPIO_LOOPBACK)
// #define GPIO_XPI1_D0 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTD | GPIO_PIN4)
// #define GPIO_XPI1_D1 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTD | GPIO_PIN3)
// #define GPIO_XPI1_CS 	(GPIO_ALT | GPIO_AF14 | GPIO_DS_3V3_30P67 | GPIO_PORTC | GPIO_PIN30)

#endif

/* Thus if the Green LED is statically on, NuttX has successfully booted and
 * is, apparently, running normally.  If the Red LED is flashing at
 * approximately 2Hz, then a fatal error has been detected and the system
 * has halted.
 *

/* SPI引脚配置 */

// #define GPIO_SPI1_SCK  (GPIO_ALT | GPIO_AF5 | GPIO_DS_3V3_30P67 | GPIO_PORTA | GPIO_PIN27 | GPIO_LOOPBACK) // 特别注意!!!!!! SPI时钟引脚需要设置GPIO_LOOPBACK
// #define GPIO_SPI1_MISO (GPIO_ALT | GPIO_AF5 | GPIO_DS_3V3_30P67 | GPIO_PORTA | GPIO_PIN28)
// #define GPIO_SPI1_MOSI (GPIO_ALT | GPIO_AF5 | GPIO_DS_3V3_30P67 | GPIO_PORTA | GPIO_PIN29)

#define GPIO_SPI2_SCK  (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN21 | GPIO_LOOPBACK) // 特别注意!!!!!! SPI时钟引脚需要设置GPIO_LOOPBACK
#define GPIO_SPI2_MISO (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN25)
#define GPIO_SPI2_MOSI (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN22)


/* I2C引脚配置 */

#define GPIO_I2C2_SCL  (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN | GPIO_DS_3V3_30P67  | GPIO_PORTB | GPIO_PIN8 | GPIO_LOOPBACK)
#define GPIO_I2C2_SDA  (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN | GPIO_DS_3V3_30P67  | GPIO_PORTB | GPIO_PIN9 | GPIO_LOOPBACK)


/* PIO Disambiguation ***************************************************************/
/* LPUARTs
 *
 * We pull down CTS so that if nothing is connected, conde will not block.
 */

/* Thus if the Green LED is statically on, NuttX has successfully booted and
 * is, apparently, running normally.  If the Red LED is flashing at
 * approximately 2Hz, then a fatal error has been detected and the system
 * has halted.
 */

/* Alternate function pin selections ************************************************/

// #define GPIO_UART0_RX  (GPIO_ALT | GPIO_AF2 | GPIO_PORTA | GPIO_PIN1)
// #define GPIO_UART0_TX  (GPIO_ALT | GPIO_AF2 | GPIO_PORTA | GPIO_PIN0)
#define GPIO_UART0_RX  (GPIO_ALT | GPIO_AF2 | GPIO_PORTY | GPIO_PIN7)
#define GPIO_UART0_TX  (GPIO_ALT | GPIO_AF2 | GPIO_PORTY | GPIO_PIN6)
// #define CONFIG_UART0_TXFIFO_THRES 0

#define GPIO_UART3_RX  (GPIO_ALT | GPIO_AF2 | GPIO_PORTB | GPIO_PIN14)
#define GPIO_UART3_TX  (GPIO_ALT | GPIO_AF2 | GPIO_PORTB | GPIO_PIN15)
// #define CONFIG_UART3_RXFIFO_THRES 14
// #define CONFIG_UART3_TXFIFO_THRES 0



// #define UART_RIRQ_PROBE_GPIO (GPIO_OUTPUT | GPIO_OUTPUT_CLEAR | GPIO_PORTA | GPIO_PIN31)
// #define UART_TIRQ_PROBE_GPIO (GPIO_OUTPUT | GPIO_OUTPUT_CLEAR | GPIO_PORTB | GPIO_PIN11)

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/************************************************************************************
 * Public Functions
 ************************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */

