/****************************************************************************
 * platforms/nuttx/src/px4/nxp/imrt/romapi/imxrt_romapi.c
 *
 * Copyright 2017-2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
****************************************************************************/

/****************************************************************************
 *
 * Included Files
 ****************************************************************************/

#include "board_config.h"

#include <stdbool.h>
#include <debug.h>
#include <stdint.h>
#include <stdbool.h>

#include "riscv_internal.h"

#include "hpm_sdk/drivers/inc/hpm_romapi_xpi_def.h"


#define OTP_CHIP_UUID_IDX_START (88U)
#define OTP_CHIP_UUID_IDX_END   (91U)

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/**
 * @brief OTP driver interface
 */
typedef struct {
    /**< OTP driver interface version */
    uint32_t version;
    /**< OTP driver interface: init */
    void (*init)(void);
    /**< OTP driver interface: deinit */
    void (*deinit)(void);
    /**< OTP driver interface: read from shadow */
    uint32_t (*read_from_shadow)(uint32_t addr);
    /**< OTP driver interface: read from ip */
    uint32_t (*read_from_ip)(uint32_t addr);
    /**< OTP driver interface: program */
    hpm_stat_t (*program)(uint32_t addr, const uint32_t *src, uint32_t num_of_words);
    /**< OTP driver interface: reload */
	uint32_t rsv1;
	uint32_t rsv2;
	uint32_t rsv3;
    /**< OTP driver interface: set_configurable_region */
    hpm_stat_t (*set_configurable_region)(uint32_t start, uint32_t num_of_words);
    /**< OTP driver interface: write_shadow_register */
    hpm_stat_t (*write_shadow_register)(uint32_t addr, uint32_t data);
} otp_driver_interface_t;


#define g_xpi_otp_driver_interface ((const otp_driver_interface_t *)(*(uint32_t*)0x2001FF0CU))



