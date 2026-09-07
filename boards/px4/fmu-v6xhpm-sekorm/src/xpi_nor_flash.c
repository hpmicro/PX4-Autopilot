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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/fs/fs.h>

#include <errno.h>
#include <string.h>

#include <px4_platform_common/px4_mtd.h>
#include "px4_log.h"

/* hpm_romapi.h inline functions and the ROM API buffer casts (uint8_t* -> uint32_t*)
 * trigger -Wcast-align under -Werror */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"

#include "hpm_romapi.h"
#include "hpm_sdk/drivers/inc/hpm_romapi_xpi_def.h"
#include "hpm_clock_drv.h"

#include "board_config.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/**
 * @brief FLASH configuration option definitions:
 * option[0]:
 *    [31:16] 0xfcf9 - FLASH configuration option tag
 *    [15:4]  0 - Reserved
 *    [3:0]   option words (exclude option[0])
 * option[1]:
 *    [31:28] Flash probe type
 *      0 - SFDP SDR / 1 - SFDP DDR
 *      2 - 1-4-4 Read (0xEB, 24-bit address) / 3 - 1-2-2 Read(0xBB, 24-bit address)
 *      4 - HyperFLASH 1.8V / 5 - HyperFLASH 3V
 *      6 - OctaBus DDR (SPI -> OPI DDR)
 *      8 - Xccela DDR (SPI -> OPI DDR)
 *      10 - EcoXiP DDR (SPI -> OPI DDR)
 *    [27:24] Command Pads after Power-on Reset
 *      0 - SPI / 1 - DPI / 2 - QPI / 3 - OPI
 *    [23:20] Command Pads after Configuring FLASH
 *      0 - SPI / 1 - DPI / 2 - QPI / 3 - OPI
 *    [19:16] Quad Enable Sequence (for the device support SFDP 1.0 only)
 *      0 - Not needed
 *      1 - QE bit is at bit 6 in Status Register 1
 *      2 - QE bit is at bit1 in Status Register 2
 *      3 - QE bit is at bit7 in Status Register 2
 *      4 - QE bit is at bit1 in Status Register 2 and should be programmed by 0x31
 *    [15:8] Dummy cycles
 *      0 - Auto-probed / detected / default value
 *      Others - User specified value, for DDR read, the dummy cycles should be 2 * cycles on FLASH datasheet
 *    [7:4] Misc.
 *      0 - Not used
 *      1 - SPI mode
 *      2 - Internal loopback
 *      3 - External DQS
 *    [3:0] Frequency option
 *      1 - 30MHz / 2 - 50MHz / 3 - 66MHz / 4 - 80MHz / 5 - 100MHz / 6 - 120MHz / 7 - 133MHz / 8 - 166MHz
 *
 * option[2] (Effective only if the bit[3:0] in option[0] > 1)
 *    [31:20]  Reserved
 *    [19:16] IO voltage
 *      0 - 3V / 1 - 1.8V
 *    [15:12] Pin group
 *      0 - 1st group / 1 - 2nd group
 *    [11:8] Connection selection
 *      0 - CA_CS0 / 1 - CB_CS0 / 2 - CA_CS0 + CB_CS0 (Two FLASH connected to CA and CB respectively)
 *    [7:0] Drive Strength
 *      0 - Default value
 * option[3] (Effective only if the bit[3:0] in option[0] > 2, required only for the QSPI NOR FLASH that not supports
 *              JESD216)
 *    [31:16] reserved
 *    [15:12] Sector Erase Command Option, not required here
 *    [11:8]  Sector Size Option, not required here
 *    [7:0] Flash Size Option
 *      0 - 4MB / 1 - 8MB / 2 - 16MB
 */

/* XPI1: data flash (XT25F128FSSIGU, 16MB QSPI) on HPM6360.
 * Probed in single-SPI mode by rom_xpi_nor_auto_config(), then switched to
 * 1-1-4 Quad for PP (0x32) and Read (0x6B, 8 dummy) in flexspi_attach().
 *   header  = 0xfcf90001 (XPI_NOR_CFG_OPTION_TAG, 1 option word)
 *   option0 = 0x00000015 (probe=SFDP SDR, freq_opt=5 -> 100MHz)
 *   option1 = 0x00001000 (1st IO group, CA_CS0, default drive strength, 3.0V) */
#define XPI1_NOR_OPT_HEADER   0xfcf90001UL
#define XPI1_NOR_OPT0         0x00000014UL
#define XPI1_NOR_OPT1         0x00001000UL

/* XPI1 memory-mapped flash base (HPM6360: XPI0=0x80000000, XPI1=0x90000000) */
#define XPI1_FLASH_BASE       0x90000000UL

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct hpm_xpi_nor_dev_s {
	struct mtd_dev_s   mtd;
	XPI_Type          *xpibase;
	xpi_nor_config_t   nor_config;
	uint32_t           sector_size;
	uint32_t           page_size;
	uint32_t           flash_size;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int hpm_xpi_nor_erase(struct mtd_dev_s *dev, off_t startblock, size_t nblocks);
static ssize_t hpm_xpi_nor_read(struct mtd_dev_s *dev, off_t offset, size_t nbytes, uint8_t *buffer);
static ssize_t hpm_xpi_nor_bread(struct mtd_dev_s *dev, off_t startblock, size_t nblocks, uint8_t *buffer);
static ssize_t hpm_xpi_nor_bwrite(struct mtd_dev_s *dev, off_t startblock, size_t nblocks, const uint8_t *buffer);
static int hpm_xpi_nor_ioctl(struct mtd_dev_s *dev, int cmd, unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct hpm_xpi_nor_dev_s g_xpi_nor = {
	.mtd =
	{
		.erase   = hpm_xpi_nor_erase,
		.bread   = hpm_xpi_nor_bread,
		.bwrite  = hpm_xpi_nor_bwrite,
		.read    = hpm_xpi_nor_read,
		.ioctl   = hpm_xpi_nor_ioctl,
#ifdef CONFIG_MTD_BYTE_WRITE
		.write   = NULL,
#endif
		.name    = "hpm_xpi1_nor",
	},
	.xpibase = HPM_XPI1,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static ssize_t hpm_xpi_nor_read(struct mtd_dev_s *dev, off_t offset, size_t nbytes, uint8_t *buffer)
{
	struct hpm_xpi_nor_dev_s *priv = (struct hpm_xpi_nor_dev_s *)dev;
	hpm_stat_t status;

	if (buffer == NULL || nbytes == 0) {
		return -EINVAL;
	}

	status = rom_xpi_nor_read(priv->xpibase, xpi_xfer_channel_auto, &priv->nor_config,
				  (void *)buffer, (uint32_t)offset, (uint32_t)nbytes);

	if (status != status_success) {
		PX4_ERR("XPI1 NOR read failed: %d @0x%lx len=%lu", (int)status,
			(unsigned long)offset, (unsigned long)nbytes);
		return -EIO;
	}

	return (ssize_t)nbytes;
}

static ssize_t hpm_xpi_nor_bread(struct mtd_dev_s *dev, off_t startblock, size_t nblocks, uint8_t *buffer)
{
	ssize_t nbytes;

	nbytes = hpm_xpi_nor_read(dev, startblock * g_xpi_nor.sector_size,
				  nblocks * g_xpi_nor.sector_size, buffer);

	return (nbytes >= 0) ? (ssize_t)nblocks : nbytes;
}

static ssize_t hpm_xpi_nor_bwrite(struct mtd_dev_s *dev, off_t startblock, size_t nblocks, const uint8_t *buffer)
{
	struct hpm_xpi_nor_dev_s *priv = (struct hpm_xpi_nor_dev_s *)dev;
	hpm_stat_t status;
	off_t offset = startblock * priv->sector_size;
	size_t nbytes = nblocks * priv->sector_size;

	if (buffer == NULL || nbytes == 0) {
		return -EINVAL;
	}

	for (size_t i = 0; i < nblocks; i++) {
		status = rom_xpi_nor_erase_sector(priv->xpibase, xpi_xfer_channel_auto, &priv->nor_config,
						 (uint32_t)(offset + (off_t)i * priv->sector_size));

		if (status != status_success) {
			PX4_ERR("XPI1 NOR bwrite erase_sector failed: %d @0x%lx", (int)status,
				(unsigned long)(offset + (off_t)i * priv->sector_size));
			return -EIO;
		}
	}

	/* rom_xpi_nor_program is page-bound: a single call must not cross a
	 * page boundary, and may only program up to one page worth of data.
	 * Loop in page-sized chunks so the whole sector is actually programmed. */
	const uint8_t *src = buffer;
	off_t prog_offset = offset;
	size_t remaining = nbytes;

	while (remaining > 0) {
		size_t chunk = remaining;

		if (chunk > priv->page_size) {
			chunk = priv->page_size;
		}

		status = rom_xpi_nor_program(priv->xpibase, xpi_xfer_channel_auto, &priv->nor_config,
					     (const uint32_t *)src, (uint32_t)prog_offset, (uint32_t)chunk);

		if (status != status_success) {
			PX4_ERR("XPI1 NOR program failed: %d @0x%lx len=%lu", (int)status,
				(unsigned long)prog_offset, (unsigned long)chunk);
			return -EIO;
		}

		src += chunk;
		prog_offset += (off_t)chunk;
		remaining -= chunk;
	}

	return (ssize_t)nblocks;
}

static int hpm_xpi_nor_erase(struct mtd_dev_s *dev, off_t startblock, size_t nblocks)
{
	struct hpm_xpi_nor_dev_s *priv = (struct hpm_xpi_nor_dev_s *)dev;
	hpm_stat_t status = status_success;

	for (size_t i = 0; i < nblocks; i++) {
		status = rom_xpi_nor_erase_sector(priv->xpibase, xpi_xfer_channel_auto, &priv->nor_config,
						 (uint32_t)((startblock + i) * priv->sector_size));

		if (status != status_success) {
			PX4_ERR("XPI1 NOR ERASE FAILED\n");
			return -EIO;
		}
	}

	return OK;
}

static int hpm_xpi_nor_erase_chip(struct hpm_xpi_nor_dev_s *priv)
{
	return (rom_xpi_nor_erase_chip(priv->xpibase, xpi_xfer_channel_auto, &priv->nor_config) == status_success) ?
	       OK : -EIO;
}

static int hpm_xpi_nor_ioctl(struct mtd_dev_s *dev, int cmd, unsigned long arg)
{
	struct hpm_xpi_nor_dev_s *priv = (struct hpm_xpi_nor_dev_s *)dev;
	int ret = -EINVAL;

	switch (cmd) {
	case MTDIOC_GEOMETRY: {
			struct mtd_geometry_s *geo = (struct mtd_geometry_s *)((uintptr_t)arg);

			if (geo) {
				geo->blocksize    = priv->sector_size;
				geo->erasesize    = priv->sector_size;
				geo->neraseblocks = priv->flash_size / priv->sector_size;
				ret = OK;
			}
		}
		break;

	case BIOC_PARTINFO: {
			struct partition_info_s *info = (struct partition_info_s *)arg;

			if (info) {
				info->numsectors  = priv->flash_size / priv->sector_size;
				info->sectorsize  = priv->sector_size;
				info->startsector = 0;
				info->parent[0]   = '\0';
				ret = OK;
			}
		}
		break;

	case MTDIOC_BULKERASE:
		ret = hpm_xpi_nor_erase_chip(priv);
		break;

	case MTDIOC_ERASESTATE:
		ret = 0xff;
		break;

	case MTDIOC_PROTECT:
	case MTDIOC_UNPROTECT:
		ret = OK;
		break;

	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int flexspi_attach(mtd_instance_s *instance)
{
	hpm_stat_t status;
	xpi_nor_config_option_t cfg_option;
	struct hpm_xpi_nor_dev_s *priv = &g_xpi_nor;

	/* Configure XPI1 pins (XT25F128FSSIGU on XPI1 CA: D0=PB12 D1=PB15 D2=PB13 D3=PB16 SCLK=PB14 CS0=PB17) */

	hpm_config_gpio(GPIO_XPI1_SCK);
	hpm_config_gpio(GPIO_XPI1_D0);
	hpm_config_gpio(GPIO_XPI1_D1);
	hpm_config_gpio(GPIO_XPI1_D2);
	hpm_config_gpio(GPIO_XPI1_D3);
	hpm_config_gpio(GPIO_XPI1_CS);

	/* Configure XPI1 serial root clock (24MHz OSC, conservative timing) */

	clock_add_to_group(clock_xpi1, 0);
	clock_set_source_divider(clock_xpi1, clk_src_osc24m, 1);

	/* Probe and configure the NOR flash via ROM auto_config in one step (same
	 * as the SDK xpi_nor_test_last_sector sample). */

	cfg_option.header.U  = XPI1_NOR_OPT_HEADER;
	cfg_option.option0.U = XPI1_NOR_OPT0;
	cfg_option.option1.U = XPI1_NOR_OPT1;
	cfg_option.option2.U = 0;

	status = rom_xpi_nor_auto_config(priv->xpibase, &priv->nor_config, &cfg_option);

	if (status != status_success) {
		PX4_ERR("XPI1 NOR auto_config failed: %d", (int)status);
		return -ENXIO;
	}

	xpi_instr_seq_t pp_quad = { .entry = {
		XPI_INSTR_SEQ(XPI_PHASE_CMD_SDR,   XPI_1PAD, 0x32, XPI_PHASE_RADDR_SDR, XPI_1PAD, 24),
		XPI_INSTR_SEQ(XPI_PHASE_WRITE_SDR, XPI_4PADS, 0,   XPI_PHASE_STOP,      XPI_1PAD, 0),
		0, 0,
	} };
	xpi_instr_seq_t read_quad = { .entry = {
		XPI_INSTR_SEQ(XPI_PHASE_CMD_SDR,   XPI_1PAD, 0x6B, XPI_PHASE_RADDR_SDR, XPI_1PAD, 24),
		XPI_INSTR_SEQ(XPI_PHASE_DUMMY_SDR, XPI_1PAD, 8,   XPI_PHASE_READ_SDR,  XPI_4PADS, 0),
		XPI_INSTR_SEQ(XPI_PHASE_STOP,      XPI_1PAD, 0,   XPI_PHASE_STOP,      XPI_1PAD, 0),
		0,
	} };
	memcpy(priv->nor_config.instr_set[xpi_std_nor_seq_idx_page_program].entry,
		pp_quad.entry, sizeof(pp_quad.entry));
	memcpy(priv->nor_config.instr_set[xpi_std_nor_seq_idx_read].entry,
		read_quad.entry, sizeof(read_quad.entry));
	priv->nor_config.device_info.data_pads = 4;

	/* Get flash geometry */

	status = rom_xpi_nor_get_property(priv->xpibase, &priv->nor_config, xpi_nor_property_sector_size,
					  &priv->sector_size);

	if (status != status_success || priv->sector_size == 0) {
		PX4_ERR("XPI1 NOR get sector size failed: %d", (int)status);
		return -ENXIO;
	}

	status = rom_xpi_nor_get_property(priv->xpibase, &priv->nor_config, xpi_nor_property_page_size,
					  &priv->page_size);

	if (status != status_success || priv->page_size == 0) {
		PX4_ERR("XPI1 NOR get page size failed: %d", (int)status);
		return -ENXIO;
	}

	status = rom_xpi_nor_get_property(priv->xpibase, &priv->nor_config, xpi_nor_property_total_size,
					  &priv->flash_size);

	if (status != status_success || priv->flash_size == 0) {
		PX4_ERR("XPI1 NOR get flash size failed: %d", (int)status);
		return -ENXIO;
	}

	PX4_INFO("XPI1 NOR: size=%lu KB sector=%lu B page=%lu B",
		 priv->flash_size / 1024, priv->sector_size, priv->page_size);

	instance->mtd_dev = &priv->mtd;
	return OK;
}

#pragma GCC diagnostic pop
