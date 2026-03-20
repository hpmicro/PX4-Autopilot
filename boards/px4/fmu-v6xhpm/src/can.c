/****************************************************************************
 *
 *   Copyright (C) 2016, 2018 PX4 Development Team. All rights reserved.
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
 * @file can.c
 *
 * Board-specific can functions.
 */

/************************************************************************************
 * Included Files
 ************************************************************************************/
#include <px4_platform_common/px4_config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <debug.h>
#include "hpm_gpio_drv.h"
#include "hpm_iomux.h"

int hpm_init_can_pins(int port)
{
	if (port == 0) {
		HPM_IOC->PAD[IOC_PAD_PD14].FUNC_CTL = IOC_PD14_FUNC_CTL_CAN0_TXD;
		HPM_IOC->PAD[IOC_PAD_PD11].FUNC_CTL = IOC_PD11_FUNC_CTL_CAN0_RXD;
	} else if (port == 1) {
		HPM_IOC->PAD[IOC_PAD_PE31].FUNC_CTL = IOC_PE31_FUNC_CTL_CAN1_TXD;
		HPM_IOC->PAD[IOC_PAD_PE30].FUNC_CTL = IOC_PE30_FUNC_CTL_CAN1_RXD;
	} else if (port == 2) {
		HPM_IOC->PAD[IOC_PAD_PD20].FUNC_CTL = IOC_PD20_FUNC_CTL_CAN2_TXD;
		HPM_IOC->PAD[IOC_PAD_PD25].FUNC_CTL = IOC_PD25_FUNC_CTL_CAN2_RXD;
	} else if(port == 3) {
		HPM_IOC->PAD[IOC_PAD_PE26].FUNC_CTL = IOC_PE26_FUNC_CTL_CAN3_TXD;
		HPM_IOC->PAD[IOC_PAD_PE29].FUNC_CTL = IOC_PE29_FUNC_CTL_CAN3_RXD;
	}
	return 0;
}
