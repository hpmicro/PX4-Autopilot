/****************************************************************************
 *
 *   Copyright (c) 2021 PX4 Development Team. All rights reserved.
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
 * 修改：xiaoyonghui 2024年05月20日13:29:54
 ****************************************************************************/
#pragma once



#include <nuttx/kmalloc.h>
#include <px4_platform/micro_hal.h>

__BEGIN_DECLS
#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/i2c/i2c_master.h>

#include <hpm_gpio.h>
#include <hpm_i2c.h>
#include <hpm_spi_master.h>

/*  defines the 128 bit UUID as
 *
 *  OCOTP 0x410 bits 31:0
 *  OCOTP 0x420 bits 63:32
 *
 *  PX4 uses the words in bigendian order MSB to LSB
 *   word  [0]    [1]
 *   bits 63-32, 31-00,
 */
#define PX4_CPU_UUID_BYTE_LENGTH                16
#define PX4_CPU_UUID_WORD32_LENGTH              (PX4_CPU_UUID_BYTE_LENGTH/sizeof(uint32_t))

/* The mfguid will be an array of bytes with
 * MSD @ index 0 - LSD @ index PX4_CPU_MFGUID_BYTE_LENGTH-1
 *
 * It will be converted to a string with the MSD on left and LSD on the right most position.
 */
#define PX4_CPU_MFGUID_BYTE_LENGTH              PX4_CPU_UUID_BYTE_LENGTH

// /* define common formating across all commands*/

#define PX4_CPU_UUID_WORD32_FORMAT              "%08x"
#define PX4_CPU_UUID_WORD32_SEPARATOR           ":"

#define PX4_CPU_UUID_WORD32_UNIQUE_H            0 /* Least significant digits change the most (die wafer,X,Y */
#define PX4_CPU_UUID_WORD32_UNIQUE_M            1 /* Most significant digits change the least (lot#) */

/*                                                  Separator    nnn:nnn:nnnn     2 char per byte           term */
#define PX4_CPU_UUID_WORD32_FORMAT_SIZE         (PX4_CPU_UUID_WORD32_LENGTH-1+(2*PX4_CPU_UUID_BYTE_LENGTH)+1)
#define PX4_CPU_MFGUID_FORMAT_SIZE              ((2*PX4_CPU_MFGUID_BYTE_LENGTH)+1)

#if defined(CONFIG_SPI)
#define px4_spibus_initialize(bus_num_1based)   hpm_spibus_initialize(bus_num_1based)
#endif

#if defined(CONFIG_I2C)
#define px4_i2cbus_initialize(bus_num_1based)   hpm_i2cbus_initialize(bus_num_1based)
#define px4_i2cbus_uninitialize(pdev)           hpm_i2cbus_uninitialize(pdev)
#endif

#define px4_arch_configgpio(pinset)             hpm_config_gpio(pinset)
#define px4_arch_unconfiggpio(pinset)           hpm_unconfig_gpio(pinset)
#define px4_arch_gpioread(pinset)               hpm_gpio_read(pinset)
#define px4_arch_gpiowrite(pinset, value)       hpm_gpio_write(pinset, value)
#define px4_arch_gpiosetevent(pinset,r,f,e,fp,a)  hpm_gpio_setevent(pinset,r,f,e,fp,a)

#if defined(CONFIG_ARCH_DCACHE)
#  define px4_cache_aligned_alloc(s) kmm_memalign(HPM_L1C_CACHELINE_SIZE,(s))
#  define px4_cache_aligned_data     aligned_data(HPM_L1C_CACHELINE_SIZE)
#else
#  define px4_cache_aligned_alloc kmm_malloc
#  define px4_cache_aligned_data()
#endif


__END_DECLS
