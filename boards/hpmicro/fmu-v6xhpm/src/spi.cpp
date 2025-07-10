/************************************************************************************
 *
 *   Copyright (C) 2016, 2018 Gregory Nutt. All rights reserved.
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
 ************************************************************************************/

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <px4_arch/spi_hw_description.h>
#include <drivers/drv_sensor.h>
#include <nuttx/spi/spi.h>

#if defined(CONFIG_SPI)

/* hpm_spibus_pins_init
 * hpm_spi_master中被调用
 */
void hpm_spibus_pins_init(int bus){

	switch(bus){
		case 0:
#if defined(CONFIG_HPM_SPI0)
			px4_arch_configgpio(GPIO_SPI0_SCK);
			px4_arch_configgpio(GPIO_SPI0_MISO);
			px4_arch_configgpio(GPIO_SPI0_MOSI);
#endif
			break;
		case 1:
#if defined(CONFIG_HPM_SPI1)
			px4_arch_configgpio(GPIO_SPI1_SCK);
			px4_arch_configgpio(GPIO_SPI1_MISO);
			px4_arch_configgpio(GPIO_SPI1_MOSI);
#endif
			break;
		case 2:
#if defined(CONFIG_HPM_SPI2)
			px4_arch_configgpio(GPIO_SPI2_SCK);
			px4_arch_configgpio(GPIO_SPI2_MISO);
			px4_arch_configgpio(GPIO_SPI2_MOSI);
#endif
			break;
		case 3:
#if defined(CONFIG_HPM_SPI3)
			px4_arch_configgpio(GPIO_SPI3_SCK);
			px4_arch_configgpio(GPIO_SPI3_MISO);
			px4_arch_configgpio(GPIO_SPI3_MOSI);

#endif
			break;
		case 4:
#if defined(CONFIG_HPM_SPI4)
			px4_arch_configgpio(GPIO_SPI4_SCK);
			px4_arch_configgpio(GPIO_SPI4_MISO);
			px4_arch_configgpio(GPIO_SPI4_MOSI);
#endif
			break;
		case 5:
#if defined(CONFIG_HPM_SPI5)
			px4_arch_configgpio(GPIO_SPI5_SCK);
			px4_arch_configgpio(GPIO_SPI5_MISO);
			px4_arch_configgpio(GPIO_SPI5_MOSI);
#endif
			break;
	}

}

constexpr px4_spi_bus_t px4_spi_buses[SPI_BUS_MAX_BUS_ITEMS] = {
#ifdef CONFIG_HPM_SPI0
	initSPIBus(SPI::Bus::SPI0, {
		// initSPIDevice(DRV_IMU_DEVTYPE_ICM20602, SPI::CS{GPIO::PortA, GPIO::Pin9}, SPI::DRDY{GPIO::PortF, GPIO::Pin2}),
	}),
#endif
#ifdef CONFIG_HPM_SPI1
	initSPIBus(SPI::Bus::SPI1, {
		initSPIDevice(SPIDEV_FLASH(0), SPI::CS{GPIO::PortA, GPIO::Pin26}),
		initSPIDevice(DRV_IMU_DEVTYPE_ADIS16470, SPI::CS{GPIO::PortB, GPIO::Pin10}, SPI::DRDY{GPIO::PortB, GPIO::Pin12}),
		initSPIDevice(DRV_GYR_DEVTYPE_BMI088, SPI::CS{GPIO::PortB, GPIO::Pin13}, SPI::DRDY{GPIO::PortA, GPIO::Pin31}),
		// initSPIDevice(DRV_GYR_DEVTYPE_BMI088, SPI::CS{GPIO::PortB, GPIO::Pin13}),
		initSPIDevice(DRV_ACC_DEVTYPE_BMI088, SPI::CS{GPIO::PortA, GPIO::Pin9}, SPI::DRDY{GPIO::PortB, GPIO::Pin11}),
		initSPIDevice(DRV_IMU_DEVTYPE_ICM20689, SPI::CS{GPIO::PortY, GPIO::Pin1}, SPI::DRDY{GPIO::PortY, GPIO::Pin0}),
	}),
#endif
#ifdef CONFIG_HPM_SPI2
	initSPIBus(SPI::Bus::SPI2, {
		// initSPIDevice(DRV_GYR_DEVTYPE_BMI088, SPI::CS{GPIO::PortC, GPIO::Pin8}, SPI::DRDY{GPIO::PortY, GPIO::Pin7}),
		// initSPIDevice(DRV_ACC_DEVTYPE_BMI088, SPI::CS{GPIO::PortD, GPIO::Pin4}, SPI::DRDY{GPIO::PortY, GPIO::Pin6}),
	}),
#endif
#ifdef CONFIG_HPM_SPI3
	initSPIBusExternal(SPI::Bus::SPI3, {
		// initSPIConfigExternal(SPI::CS{GPIO::PortX, GPIO::Pin10}, SPI::DRDY{GPIO::PortD, GPIO::Pin11}),
		// initSPIConfigExternal(SPI::CS{GPIO::PortA, GPIO::Pin15}, SPI::DRDY{GPIO::PortD, GPIO::Pin12}),
	}),
#endif

};

// static constexpr bool unused = validateSPIConfig(px4_spi_buses);

#endif
