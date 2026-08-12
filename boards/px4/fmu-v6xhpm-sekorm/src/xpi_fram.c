/****************************************************************************
 * boards/px4/fmu-v6xrt/src/hpm_xpi_fram.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/kmalloc.h>
#include <nuttx/signal.h>
#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>

#include <px4_platform_common/px4_mtd.h>
#include "px4_log.h"

#include "hpm_sdk/drivers/inc/hpm_romapi_xpi_def.h"

#include "board_config.h"


#ifdef CONFIG_HPM_XPI_DRV

#define FRAM_SIZE        0x8000U
#define FRAM_PAGE_SIZE   0x0080U
#define FRAM_SECTOR_SIZE 0x0080U



enum {
	/* SPI instructions */

	READ_ID,
	READ_STATUS_REG,
	WRITE_STATUS_REG,
	WRITE_ENABLE,
	READ_FAST,
	PAGE_PROGRAM,
};

static const uint32_t g_xpi_fram_lut[][4] = {
	[READ_ID] =
	{
		XPI_INSTR_SEQ(XPI_PHASE_CMD_SDR,       XPI_1PAD, 0x9f,
				XPI_PHASE_READ_SDR,  XPI_1PAD, 0x04),
	},

	[READ_STATUS_REG] =
	{
		XPI_INSTR_SEQ(XPI_PHASE_CMD_SDR,       XPI_1PAD, 0x05,
				XPI_PHASE_READ_SDR,  XPI_1PAD, 0x04),
	},

	[WRITE_STATUS_REG] =
	{
		XPI_INSTR_SEQ(XPI_PHASE_CMD_SDR,       XPI_1PAD, 0x01,
				XPI_PHASE_WRITE_SDR, XPI_1PAD, 0x04),
	},

	[WRITE_ENABLE] =
	{
		XPI_INSTR_SEQ(XPI_PHASE_CMD_SDR,       XPI_1PAD, 0x06,
				XPI_PHASE_STOP,      XPI_1PAD, 0),
	},
	[READ_FAST] =
	{
		XPI_INSTR_SEQ(XPI_PHASE_CMD_SDR,       XPI_1PAD, 0x0b,
				XPI_PHASE_RADDR_SDR, XPI_1PAD, 0x10),
		XPI_INSTR_SEQ(XPI_PHASE_DUMMY_SDR, XPI_1PAD, 0x08,
				XPI_PHASE_READ_SDR,  XPI_1PAD, 0x04),
	},
	[PAGE_PROGRAM] =
	{
		XPI_INSTR_SEQ(XPI_PHASE_CMD_SDR,       XPI_1PAD, 0x02,
				XPI_PHASE_RADDR_SDR, XPI_1PAD, 0x10),
		XPI_INSTR_SEQ(XPI_PHASE_WRITE_SDR, XPI_1PAD, 0x04,
				XPI_PHASE_STOP,      XPI_1PAD, 0),
	},
};

typedef struct {
    struct {
        uint8_t priority;                   /* Offset: 0x00 */
        uint8_t master_idx;                 /* Offset: 0x01 */
        uint8_t buf_size_in_dword;          /* Offset: 0x02 */
        bool enable_prefetch;               /* Offset: 0x03 */
    } entry[8];
} xpi_ahb_buffer_cfg_t;

/**
 * @brief XPI driver interface
 */
typedef struct {
    /**< XPI driver interface: version */
    uint32_t version;
    /**< XPI driver interface: get default configuration */
    hpm_stat_t (*get_default_config)(xpi_config_t *xpi_config);
    /**< XPI driver interface: get default device configuration */
    hpm_stat_t (*get_default_device_config)(xpi_device_config_t *dev_config);
    /**< XPI driver interface: initialize the XPI using xpi_config */
    hpm_stat_t (*init)(XPI_Type *base, xpi_config_t *xpi_config);
    /**< XPI driver interface: configure the AHB buffer */
    hpm_stat_t (*config_ahb_buffer)(XPI_Type *base, xpi_ahb_buffer_cfg_t *ahb_buf_cfg);
    /**< XPI driver interface: configure the device */
    hpm_stat_t (*config_device)(XPI_Type *base, xpi_device_config_t *dev_cfg, xpi_channel_t channel);
    /**< XPI driver interface: update instruction talbe */
    hpm_stat_t (*update_instr_table)(XPI_Type *base, const uint32_t *inst_base, uint32_t seq_idx, uint32_t num);
    /**< XPI driver interface: transfer command/data using block interface */
    hpm_stat_t (*transfer_blocking)(XPI_Type *base, xpi_xfer_ctx_t *xfer);
    /**< Software reset the XPI controller */
    void (*software_reset)(XPI_Type *base);
    /**< XPI driver interface: Check whether IP is idle */
    bool (*is_idle)(XPI_Type *base);
    /**< XPI driver interface: update delay line setting */
    void (*update_dllcr)(XPI_Type *base, uint32_t serial_root_clk_freq, uint32_t data_valid_time, xpi_channel_t channel,
                         uint32_t dly_target);
    /**< XPI driver interface: Get absolute address for APB transfer */
    hpm_stat_t (*get_abs_apb_xfer_addr)(XPI_Type *base, xpi_xfer_channel_t channel, uint32_t in_addr,
                                        uint32_t *out_addr);
} xpi_driver_interface_t;


// struct ramtron_parts_s
// {
//   FAR const char *name;
//   uint8_t id1;
//   uint8_t id2;
//   uint32_t size;
//   uint8_t addr_len;
//   uint32_t speed;
// #ifdef CONFIG_RAMTRON_CHUNKING
//   bool chunked;                            /* True: write buffer size limitations */
//   uint16_t chunksize;                      /* Write chunk Size */
// #endif
// };


// static const struct ramtron_parts_s g_ramtron_parts[] =
// {
//   {
//     "FM25V01",                    /* name */
//     0x21,                         /* id1 */
//     0x00,                         /* id2 */
//     16L * 1024L,                  /* size */
//     2,                            /* addr_len */
//     RAMTRON_INIT_CLK_MAX          /* speed */
// #ifdef CONFIG_RAMTRON_CHUNKING
//     , false,                      /* chunked */
//     RAMTRON_EMULATE_PAGE_SIZE     /* chunksize */
// #endif
//   },
//   {
//     "FM25V01A",                   /* name */
//     0x21,                         /* id1 */
//     0x08,                         /* id2 */
//     16L * 1024L,                  /* size */
//     2,                            /* addr_len */
//     RAMTRON_INIT_CLK_MAX          /* speed */
// #ifdef CONFIG_RAMTRON_CHUNKING
//     , false,                      /* chunked */
//     RAMTRON_EMULATE_PAGE_SIZE     /* chunksize */
// #endif
//   },
//   {
//     "FM25V02",                    /* name */
//     0x22,                         /* id1 */
//     0x00,                         /* id2 */
//     32L * 1024L,                  /* size */
//     2,                            /* addr_len */
//     RAMTRON_INIT_CLK_MAX          /* speed */
// #ifdef CONFIG_RAMTRON_CHUNKING
//     , false,                      /* chunked */
//     RAMTRON_EMULATE_PAGE_SIZE     /* chunksize */
// #endif
//   },
//   {
//     "FM25V02A",                   /* name */
//     0x22,                         /* id1 */
//     0x08,                         /* id2 */
//     32L * 1024L,                  /* size */
//     2,                            /* addr_len */
//     RAMTRON_INIT_CLK_MAX          /* speed */
// #ifdef CONFIG_RAMTRON_CHUNKING
//     , false,                      /* chunked */
//     RAMTRON_EMULATE_PAGE_SIZE     /* chunksize */
// #endif
//   },
// #endif
//   {
//     NULL,                         /* name */
//     0,                            /* id1 */
//     0,                            /* id2 */
//     0,                            /* size */
//     0,                            /* addr_len */
//     0                             /* speed */
// #ifdef CONFIG_RAMTRON_CHUNKING
//     , false,                      /* chunked */
//     0,                            /* chunksize */
// #endif
//   }
// };


/**< Bootloader API table Root */
#define g_xpi_driver_interface ((xpi_driver_interface_t *)(0x2001FFA0U))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* XPI NOR device private data */

struct hpm_xpi_fram_dev_s {
	struct mtd_dev_s mtd;
	xpi_driver_interface_t *inf;   /* Saved XPI interface instance */
	XPI_Type        *xpibase;      /* SPIn base address */
	xpi_xfer_channel_t channel;
	xpi_device_config_t *config;
};

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

/* MTD driver methods */

static int hpm_xpi_fram_erase(struct mtd_dev_s *dev,
				    off_t startblock,
				    size_t nblocks);
static ssize_t hpm_xpi_fram_read(struct mtd_dev_s *dev,
				       off_t offset,
				       size_t nbytes,
				       uint8_t *buffer);
static ssize_t hpm_xpi_fram_bread(struct mtd_dev_s *dev,
					off_t startblock,
					size_t nblocks,
					uint8_t *buffer);
static ssize_t hpm_xpi_fram_bwrite(struct mtd_dev_s *dev,
		off_t startblock,
		size_t nblocks,
		const uint8_t *buffer);
static int hpm_xpi_fram_ioctl(struct mtd_dev_s *dev,
				    int cmd,
				    unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static xpi_device_config_t g_xpi_device_config = {
	.size_in_kbytes = 32,
	.serial_root_clk_freq = 4000000,
	.enable_write_mask = 0,
	.data_valid_time = 0,
	.cs_interval = 0,
	.cs_hold_time = 12,
	.cs_setup_time = 12,

	.column_addr_size = 0,
	.enable_word_address = 0,
	.ahb_write_seq_idx = 0,
	.ahb_write_seq_num = 0,
	.ahb_read_seq_idx = READ_FAST,
	.ahb_read_seq_num = 1,
	.ahb_write_wait_interval = 0,
};

static struct hpm_xpi_fram_dev_s g_xpi_fram = {
	.mtd =
	{
		.erase  = hpm_xpi_fram_erase,
		.bread  = hpm_xpi_fram_bread,
		.bwrite = hpm_xpi_fram_bwrite,
		.read   = hpm_xpi_fram_read,
		.ioctl  = hpm_xpi_fram_ioctl,
#ifdef CONFIG_MTD_BYTE_WRITE
		.write  = NULL,
#endif
		.name   = "hpm_xpi_fram"
	},
	.inf = g_xpi_driver_interface,
	.xpibase = (XPI_Type *) 0xF3044000UL,//XPI1 需要根据实际修改 !!!!!
	.channel = xpi_xfer_channel_a1,  // 需要根据实际修改
	.config = &g_xpi_device_config
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int hpm_xpi_fram_get_vendor_id(
	const struct hpm_xpi_fram_dev_s *dev,
	uint8_t *vendor_id)
{
	uint8_t buffer[10] = {0x9f, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5};
	int stat;

	xpi_xfer_ctx_t transfer = {
		.addr = 0,
		.channel = dev->channel,
		.cmd_type = xpi_apb_xfer_type_read,
		.seq_num = 1,
		.seq_idx = READ_ID,
		.buf = (void *) buffer,
		.xfer_size = 10,
	};

	stat = dev->inf->transfer_blocking(dev->xpibase, &transfer);

	if (stat != 0) {
		return -EIO;
	}

	*vendor_id = buffer[0];

	return 0;
}

static int hpm_xpi_fram_read_status(
	const struct hpm_xpi_fram_dev_s *dev,
	uint32_t *status)
{
	int stat;

	xpi_xfer_ctx_t transfer = {
		.addr = 0,
		.channel = dev->channel,
		.cmd_type = xpi_apb_xfer_type_read,
		.seq_num = 1,
		.seq_idx = READ_STATUS_REG,
		.xfer_size = 1,
	};

	stat =dev->inf->transfer_blocking(dev->xpibase, &transfer);


	if (stat != 0) {
		return -EIO;
	}

	return 0;
}

#if 0
static int hpm_xpi_fram_write_status(
	const struct hpm_xpi_fram_dev_s *dev,
	uint32_t *status)
{
	int stat;

	struct flexspi_transfer_s transfer = {
		.device_address = 0,
		.port = dev->port,
		.cmd_type = FLEXSPI_WRITE,
		.seq_number = 1,
		.seq_index = WRITE_STATUS_REG,
		.data = status,
		.data_size = 1,
	};

	stat = FLEXSPI_TRANSFER(dev->flexspi, &transfer);

	if (stat != 0) {
		return -EIO;
	}

	return 0;
}
#endif

static int hpm_xpi_fram_write_enable(
	const struct hpm_xpi_fram_dev_s *dev)
{
	int stat;

	xpi_xfer_ctx_t transfer = {
		.addr = 0,
		.channel = dev->channel,
		.cmd_type = xpi_apb_xfer_type_cmd,
		.seq_num = 1,
		.seq_idx = WRITE_ENABLE,
		.buf = NULL,
		.xfer_size = 1,
	};

	stat =dev->inf->transfer_blocking(dev->xpibase, &transfer);

	if (stat != 0) {
		return -EIO;
	}

	return 0;
}

static int hpm_xpi_fram_erase_sector(
	const struct hpm_xpi_fram_dev_s *dev,
	off_t offset)
{
	int stat;
	size_t remaining = FRAM_SECTOR_SIZE;
	uint8_t buffer[FRAM_SECTOR_SIZE] = {0xff};

	xpi_xfer_ctx_t transfer = {
		.channel = dev->channel,
		.cmd_type = xpi_apb_xfer_type_write,
		.seq_num = 1,
		.seq_idx = PAGE_PROGRAM,
		.buf = (void *) buffer,
	};

	while (remaining > 0) {
		transfer.addr = offset;
		transfer.xfer_size = MIN(128, remaining);

		stat =dev->inf->transfer_blocking(dev->xpibase, &transfer);

		if (stat != 0) {
			return -EIO;
		}

		remaining -= transfer.xfer_size;
		offset += transfer.xfer_size;
	}

	return 0;
}

static int hpm_xpi_fram_erase_chip(
	const struct hpm_xpi_fram_dev_s *dev)
{
	int stat;
	size_t remaining = FRAM_SIZE;
	size_t offset = 0;
	uint8_t buffer[FRAM_SECTOR_SIZE] = {0xff};

	xpi_xfer_ctx_t transfer = {
		.channel = dev->channel,
		.cmd_type = xpi_apb_xfer_type_write,
		.seq_num = 1,
		.seq_idx = PAGE_PROGRAM,
		.buf = (void *) buffer,
	};

	while (remaining > 0) {
		transfer.addr = offset;
		transfer.xfer_size = MIN(128, remaining);

		stat =dev->inf->transfer_blocking(dev->xpibase, &transfer);

		if (stat != 0) {
			return -EIO;
		}

		remaining -= transfer.xfer_size;
		offset += transfer.xfer_size;
	}


	return 0;
}

static int hpm_xpi_fram_page_program(
	const struct hpm_xpi_fram_dev_s *dev,
	off_t offset,
	const void *buffer,
	size_t len)
{
	int stat;

	xpi_xfer_ctx_t transfer = {
		.addr = offset,
		.channel = dev->channel,
		.cmd_type = xpi_apb_xfer_type_write,
		.seq_num = 1,
		.seq_idx = PAGE_PROGRAM,
		.buf =  (uint32_t *)buffer,
		.xfer_size = len,
	};

	stat =dev->inf->transfer_blocking(dev->xpibase, &transfer);

	if (stat != 0) {
		return -EIO;
	}

	return 0;
}

static int hpm_xpi_fram_wait_bus_busy(
	const struct hpm_xpi_fram_dev_s *dev)
{
	uint32_t status = 0;
	int ret;

	do {
		ret = hpm_xpi_fram_read_status(dev, &status);

		if (ret) {
			return ret;
		}
	} while (status & 1);

	return 0;
}

static ssize_t hpm_xpi_fram_read(struct mtd_dev_s *dev,
				       off_t offset,
				       size_t nbytes,
				       uint8_t *buffer)
{

#if 1
	struct hpm_xpi_fram_dev_s *priv =
		(struct hpm_xpi_fram_dev_s *)dev;
	int stat;
	size_t remaining = nbytes;
	uint8_t src[FRAM_SECTOR_SIZE];

	xpi_xfer_ctx_t transfer = {
		.channel = priv->channel,
		.cmd_type = xpi_apb_xfer_type_read,
		.seq_num = 1,
		.seq_idx = READ_FAST,
		.buf = (void *)src,
	};

	while (remaining > 0) {
		transfer.addr = offset;
		transfer.xfer_size = MIN(128, remaining);

		stat =priv->inf->transfer_blocking(priv->xpibase, &transfer);

		if (stat != 0) {
			return -EIO;
		}
		memcpy(buffer, src, transfer.xfer_size);

		remaining -= transfer.xfer_size;
		offset += transfer.xfer_size;
		buffer += transfer.xfer_size;
	}

	return 0;

#else
	struct hpm_xpi_fram_dev_s *priv =
		(struct hpm_xpi_fram_dev_s *)dev;
	uint8_t *src;

	finfo("offset: %08lx nbytes: %d\n", (long)offset, (int)nbytes);

	if (priv->channel >= 2) {
		return -EIO;
	}

	src = priv->xpibase + offset;

	memcpy(buffer, src, nbytes);

	finfo("return nbytes: %d\n", (int)nbytes);
	return (ssize_t)nbytes;
#endif
}

static ssize_t hpm_xpi_fram_bread(struct mtd_dev_s *dev,
					off_t startblock,
					size_t nblocks,
					uint8_t *buffer)
{
	ssize_t nbytes;

	finfo("startblock: %08lx nblocks: %d\n", (long)startblock, (int)nblocks);

	/* On this device, we can handle the block read just like the byte-oriented
	 * read
	 */

	nbytes = hpm_xpi_fram_read(dev, startblock * FRAM_PAGE_SIZE,
					 nblocks * FRAM_PAGE_SIZE, buffer);

	if (nbytes > 0) {
		nbytes /= FRAM_PAGE_SIZE;
	}

	return nbytes;
}

static ssize_t hpm_xpi_fram_bwrite(struct mtd_dev_s *dev,
		off_t startblock,
		size_t nblocks,
		const uint8_t *buffer)
{
	struct hpm_xpi_fram_dev_s *priv =
		(struct hpm_xpi_fram_dev_s *)dev;
	size_t len = nblocks * FRAM_PAGE_SIZE;
	off_t offset = startblock * FRAM_PAGE_SIZE;
	uint8_t *src = (uint8_t *) buffer;
// #ifdef CONFIG_ARCH_DCACHE
// 	uint8_t *dst = priv->xpibase + startblock * FRAM_PAGE_SIZE;
// #endif
	int i;

	finfo("startblock: %08lx nblocks: %d\n", (long)startblock, (int)nblocks);

	while (len) {
		i = MIN(FRAM_PAGE_SIZE, len);
		hpm_xpi_fram_write_enable(priv);
		hpm_xpi_fram_page_program(priv, offset, src, i);
		hpm_xpi_fram_wait_bus_busy(priv);
		priv->inf->software_reset(priv->xpibase);
		offset += i;
		len -= i;
	}

// #ifdef CONFIG_ARCH_DCACHE
// 	up_invalidate_dcache((uintptr_t)dst,
// 			     (uintptr_t)dst + nblocks * FRAM_PAGE_SIZE);
// #endif

	return nblocks;
}

static int hpm_xpi_fram_erase(struct mtd_dev_s *dev,
				    off_t startblock,
				    size_t nblocks)
{
	struct hpm_xpi_fram_dev_s *priv =
		(struct hpm_xpi_fram_dev_s *)dev;
	size_t blocksleft = nblocks;
// #ifdef CONFIG_ARCH_DCACHE
// 	uint8_t *dst = priv->xpibase + startblock * FRAM_SECTOR_SIZE;
// #endif

	finfo("startblock: %08lx nblocks: %d\n", (long)startblock, (int)nblocks);

	while (blocksleft-- > 0) {
		/* Erase each sector */

		hpm_xpi_fram_write_enable(priv);
		hpm_xpi_fram_erase_sector(priv, startblock * FRAM_SECTOR_SIZE);
		hpm_xpi_fram_wait_bus_busy(priv);
		priv->inf->software_reset(priv->xpibase);
		startblock++;
	}

// #ifdef CONFIG_ARCH_DCACHE
// 	up_invalidate_dcache((uintptr_t)dst,
// 			     (uintptr_t)dst + nblocks * FRAM_SECTOR_SIZE);
// #endif

	return (int)nblocks;
}

static int hpm_xpi_fram_ioctl(struct mtd_dev_s *dev,
				    int cmd,
				    unsigned long arg)
{
	struct hpm_xpi_fram_dev_s *priv =
		(struct hpm_xpi_fram_dev_s *)dev;
	int ret = -EINVAL; /* Assume good command with bad parameters */

	finfo("cmd: %d\n", cmd);

	switch (cmd) {
	case MTDIOC_GEOMETRY: {
			struct mtd_geometry_s *geo =
				(struct mtd_geometry_s *)((uintptr_t)arg);

			if (geo) {
				/* Populate the geometry structure with information need to
				 * know the capacity and how to access the device.
				 *
				 * NOTE:
				 * that the device is treated as though it where just an array
				 * of fixed size blocks.  That is most likely not true, but the
				 * client will expect the device logic to do whatever is
				 * necessary to make it appear so.
				 */

				geo->blocksize    = (FRAM_PAGE_SIZE);
				geo->erasesize    = (FRAM_SECTOR_SIZE);
				geo->neraseblocks = (FRAM_SIZE / FRAM_SECTOR_SIZE);

				ret               = OK;

				finfo("blocksize: %lu erasesize: %lu neraseblocks: %lu\n",
				      geo->blocksize, geo->erasesize, geo->neraseblocks);
			}
		}
		break;

	case BIOC_PARTINFO: {
			struct partition_info_s *info =
				(struct partition_info_s *)arg;

			if (info != NULL) {
				info->numsectors  = (FRAM_SIZE / FRAM_SECTOR_SIZE);
				info->sectorsize  = FRAM_PAGE_SIZE;
				info->startsector = 0;
				info->parent[0]   = '\0';
				ret               = OK;
			}
		}
		break;

	case MTDIOC_BULKERASE: {
			/* Erase the entire device */

			hpm_xpi_fram_write_enable(priv);
			hpm_xpi_fram_erase_chip(priv);
			hpm_xpi_fram_wait_bus_busy(priv);
			priv->inf->software_reset(priv->xpibase);
			ret               = OK;
		}
		break;

	case MTDIOC_PROTECT:

		/* TODO */

		break;

	case MTDIOC_UNPROTECT:

		/* TODO */

		break;

	default:
		ret = -ENOTTY; /* Bad/unsupported command */
		break;
	}

	finfo("return %d\n", ret);
	return ret;
}



/****************************************************************************
 * Name: hpm_xpi_fram_initialize
 *
 * Description:
 *  This function is called by board-bringup logic to configure the
 *  flash device.
 *
 * Returned Value:
 *   Zero is returned on success.  Otherwise, a negated errno value is
 *   returned to indicate the nature of the failure.
 *
 ****************************************************************************/

int hpm_xpi_fram_initialize(void)
{

	uint8_t vendor_id;
	struct mtd_dev_s *dev = &g_xpi_fram.mtd;
	struct hpm_xpi_fram_dev_s *priv =
		(struct hpm_xpi_fram_dev_s *)dev;
	int ret = -ENODEV;
	/* Configure multiplexed pins as connected on the board */

	hpm_config_gpio(GPIO_XPI1_CS);
	hpm_config_gpio(GPIO_XPI1_D0);
	hpm_config_gpio(GPIO_XPI1_D1);
	hpm_config_gpio(GPIO_XPI1_SCK);

	/* Select XPI2 */


	xpi_config_t default_config;
	priv->inf->get_default_config(&default_config);
	priv->inf->config_device(priv->xpibase,priv->config, (xpi_channel_t)priv->channel);
	priv->inf->init(priv->xpibase, &default_config);

	xpi_ahb_buffer_cfg_t ahb_cfg;
	for(uint32_t i = 0; i < 8; i++){
		ahb_cfg.entry[i].enable_prefetch=false;
	}
	ahb_cfg.entry[0].priority=0;
	ahb_cfg.entry[0].master_idx=0;
	ahb_cfg.entry[0].buf_size_in_dword=64;
	ahb_cfg.entry[0].enable_prefetch=true;
	priv->inf->config_ahb_buffer(priv->xpibase, &ahb_cfg);

	priv->inf->update_instr_table(priv->xpibase, (const uint32_t *)g_xpi_fram_lut, 0, sizeof(g_xpi_fram_lut) / 4);
	priv->inf->software_reset(priv->xpibase);

	// g_xpi_fram.flexspi = hpm_xpi_initialize(1);

	// if (g_xpi_fram.flexspi) {
	// 	FLEXSPI_SET_DEVICE_CONFIG(g_xpi_fram.flexspi,
	// 				  g_xpi_fram.config,
	// 				  g_xpi_fram.port);
	// 	FLEXSPI_UPDATE_LUT(g_xpi_fram.flexspi,
	// 			   0,
	// 			   (const uint32_t *)g_flexspi_fram_lut,
	// 			   sizeof(g_flexspi_fram_lut) / 4);
	// 	FLEXSPI_SOFTWARE_RESET(g_xpi_fram.flexspi);
	// 	ret = OK;

		if (hpm_xpi_fram_get_vendor_id(&g_xpi_fram, &vendor_id)) {
			ret = -EIO;
		}
	// }



	return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
int flexspi_attach(mtd_instance_s *instance)
{
	int rv = hpm_xpi_fram_initialize();

	if (rv != OK) {
		_info("failed to initalize flexspi bus");
		return -ENXIO;
	}

	instance->mtd_dev = &g_xpi_fram.mtd;
	return OK;
}

#endif /* CONFIG_HPM_XPI_DRV */
