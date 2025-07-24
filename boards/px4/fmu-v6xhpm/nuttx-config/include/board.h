/****************************************************************************
 * nuttx-configs/px4/fmu-v6xhpm/include/board.h
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

#ifndef __NUTTX_CONFIG_PX4_FMU_V6XHPM_INCLUDE_BOARD_H
#define __NUTTX_CONFIG_PX4_FMU_V6XHPM_INCLUDE_BOARD_H

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

/* SPI Pins */
#define GPIO_SPI2_SCK  (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN21 | GPIO_LOOPBACK)
#define GPIO_SPI2_MISO (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN25)
#define GPIO_SPI2_MOSI (GPIO_ALT | GPIO_AF5 | GPIO_DS_12mA | GPIO_PORTB | GPIO_PIN22)

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
#endif /* __NUTTX_CONFIG_PX4_FMU_V6XHPM_INCLUDE_BOARD_H */
