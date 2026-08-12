/****************************************************************************
 *
 *   Copyright (c) 2012-2022 PX4 Development Team. All rights reserved.
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

#include <nuttx/config.h>
#include <board_config.h>

#include <nuttx/spi/spi.h>
#include <px4_platform_common/px4_manifest.h>

#if defined(CONFIG_MTD)

/* XT25F128FSSIGU (16MB QSPI NOR) on XPI1, exposed as MTD storage.
 * Block size = sector size (4KB, see xpi_nor_flash.c) -> nblocks below are 4KB units. */
static const px4_mft_device_t xpi1_flash = {
	.bus_type = px4_mft_device_t::FLEXSPI,
	.devid    = SPIDEV_FLASH(0)
};

static const px4_mtd_entry_t board_flash = {
	.device = &xpi1_flash,
	.npart = 6,
	.partd = {
		{
			.type = MTD_PARAMETERS,
			.path = "/fs/mtd_params",
			.nblocks = 8 // 32KB
		},
		{
			.type = MTD_CALDATA,
			.path = "/fs/mtd_caldata",
			.nblocks = 8 // 32KB
		},
		{
			.type = MTD_MFT_VER,
			.path = "/fs/mtd_mft_ver",
			.nblocks = 1 // 4KB
		},
		{
			.type = MTD_MFT_REV,
			.path = "/fs/mtd_mft_rev",
			.nblocks = 1 // 4KB
		},
		{
			.type = MTD_NET,
			.path = "/fs/mtd_net",
			.nblocks = 1 // 4KB
		},
		{
			.type = MTD_ID,
			.path = "/fs/mtd_id",
			.nblocks = 1 // 4KB
		},
	},
};

static const px4_mtd_manifest_t board_mtd_config = {
	.nconfigs = 1,
	.entries = {
		&board_flash
	}
};

static const px4_mft_entry_s mtd_mft = {
	.type = MTD,
	.pmft = (void *) &board_mtd_config,
};

static const px4_mft_entry_s mft_mft = {
	.type = MFT,
#ifdef BOARD_HAS_HW_SPLIT_VERSIONING
	.pmft = (void *)system_query_manifest ,//system_query_manifest,待完善
#else
	.pmft = (void *)NULL,
#endif
};

static const px4_mft_s mft = {
	.nmft = 2,
	.mfts = {
		&mtd_mft,
		&mft_mft,
	}
};

const px4_mft_s *board_get_manifest(void)
{
	return &mft;
}
#endif
