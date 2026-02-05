/****************************************************************************
 * nuttx-configs/px4/fmu-v6xhpm-siyi/include/board.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __NUTTX_CONFIG_PX4_FMU_V6XHPM_SIYI_INCLUDE_BOARD_H
#define __NUTTX_CONFIG_PX4_FMU_V6XHPM_SIYI_INCLUDE_BOARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef BOARD_APP_CORE
#define BOARD_APP_CORE  HPM_CORE0
#endif

/* UART Pins */
#define GPIO_UART0_TX (GPIO_ALT | GPIO_AF2 | GPIO_PORTY | GPIO_PIN6)
#define GPIO_UART0_RX (GPIO_ALT | GPIO_AF2 | GPIO_PORTY | GPIO_PIN7)

#define GPIO_UART1_TX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN19)
#define GPIO_UART1_RX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN18)

#define GPIO_UART2_TX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN24)
#define GPIO_UART2_RX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN23)

#define GPIO_UART3_TX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN31)
#define GPIO_UART3_RX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN30)

#define GPIO_UART5_TX (GPIO_ALT | GPIO_AF2 | GPIO_PORTD | GPIO_PIN7)
#define GPIO_UART5_RX (GPIO_ALT | GPIO_AF2 | GPIO_PORTD | GPIO_PIN6)

#define GPIO_UART6_TX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN28)
#define GPIO_UART6_RX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN27)
#define GPIO_UART6_CTS (GPIO_ALT | GPIO_AF3 | GPIO_PORTD | GPIO_PIN9)
#define GPIO_UART6_RTS (GPIO_ALT | GPIO_AF3 | GPIO_PORTD | GPIO_PIN10)

#define GPIO_UART8_TX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN7)
#define GPIO_UART8_RX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN6)
#define GPIO_UART8_CTS (GPIO_ALT | GPIO_AF3 | GPIO_PORTD | GPIO_PIN11)
#define GPIO_UART8_RTS (GPIO_ALT | GPIO_AF3 | GPIO_PORTD | GPIO_PIN14)

#define GPIO_UART9_TX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN12)
#define GPIO_UART9_RX (GPIO_ALT | GPIO_AF2 | GPIO_PORTC | GPIO_PIN11)
#define GPIO_UART9_CTS (GPIO_ALT | GPIO_AF3 | GPIO_PORTA | GPIO_PIN12)
#define GPIO_UART9_RTS (GPIO_ALT | GPIO_AF3 | GPIO_PORTA | GPIO_PIN13)

/* I2C Pins */
#define GPIO_I2C0_SCL (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN11 | GPIO_LOOPBACK)
#define GPIO_I2C0_SDA (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN10 | GPIO_LOOPBACK)

#define GPIO_I2C1_SCL (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN | GPIO_DS_12mA | GPIO_PORTD | GPIO_PIN5 | GPIO_LOOPBACK)
#define GPIO_I2C1_SDA (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN | GPIO_DS_12mA | GPIO_PORTD | GPIO_PIN8 | GPIO_LOOPBACK)

#define GPIO_I2C2_SCL (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN4 | GPIO_LOOPBACK)
#define GPIO_I2C2_SDA (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN3 | GPIO_LOOPBACK)

#define GPIO_I2C3_SCL (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN14 | GPIO_LOOPBACK)
#define GPIO_I2C3_SDA (GPIO_ALT | GPIO_AF4 | GPIO_OPENDRAIN | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN13 | GPIO_LOOPBACK)

/* SPI Pins */
#define GPIO_SPI0_SCK  (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTZ | GPIO_PIN3 | GPIO_LOOPBACK)
#define GPIO_SPI0_MISO (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTZ | GPIO_PIN5)
#define GPIO_SPI0_MOSI (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTZ | GPIO_PIN4)

#define GPIO_SPI1_SCK  (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTA | GPIO_PIN21 | GPIO_LOOPBACK)
#define GPIO_SPI1_MISO (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTA | GPIO_PIN23)
#define GPIO_SPI1_MOSI (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTA | GPIO_PIN16)

#define GPIO_SPI2_SCK  (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN0 | GPIO_LOOPBACK)
#define GPIO_SPI2_MISO (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTA | GPIO_PIN31)
#define GPIO_SPI2_MOSI (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTA | GPIO_PIN27)

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifndef __ASSEMBLY__

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
}
#endif
#endif /* __ASSEMBLY__ */
#endif /* __NUTTX_CONFIG_PX4_FMU_V6XHPM_SIYI_INCLUDE_BOARD_H */
