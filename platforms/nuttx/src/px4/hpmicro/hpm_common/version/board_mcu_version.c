/****************************************************************************
 *
 *   Copyright (C) 2017 PX4 Development Team. All rights reserved.
 *   Author: @author David Sidrane <david_s5@nscdg.com>
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
 * @file board_mcu_version.c
 * Implementation of STM32 based SoC version API
 * xiaoyonghui 待完善
 */

#include <px4_platform_common/px4_config.h>
#include <px4_platform_common/defines.h>
#include <px4_arch/romapi.h>

int board_mcu_version(char *rev, const char **revstr, const char **errata)
{

	uint32_t chip_id = g_xpi_otp_driver_interface->read_from_shadow(64);// CHIPID
	if(chip_id == 0x20201241){
		*revstr = "HPM6750IVM2";
		*rev = '2';
		*errata = NULL;
		return 2;
	}
	// uint32_t abc = getreg32(STM32_DEBUGMCU_BASE);

	// int32_t chip_version = abc & DEVID_MASK;
	// enum MCU_REV revid = (abc & REVID_MASK) >> 16;
	// const char *chip_errata = NULL;

	// switch (chip_version) {


	// case STM32H74xx_75xx:
	// 	*revstr = "STM32H7[4|5]xxx";
	// 	break;

	// case STM32F74xxx_75xxx:
	// 	*revstr = "STM32F74xxx";
	// 	break;

	// case STM32F76xxx_77xxx:
	// 	*revstr = "STM32F76xxx";
	// 	break;

	// case STM32F40x_41x:
	// 	*revstr = "STM32F40x";
	// 	break;

	// case STM32F42x_43x:
	// 	*revstr = "STM32F42x";
	// 	/* Set possible errata */
	// 	chip_errata = STM32_F4_ERRATA;
	// 	break;

	// case STM32F103_LD:
	// 	*revstr = "STM32F1xx Low";
	// 	break;

	// case STM32F103_MD:
	// 	*revstr = "STM32F1xx Med";
	// 	break;

	// case STM32F103_HD:
	// 	*revstr = "STM32F1xx Hi";
	// 	break;

	// case STM32F103_XLD:
	// 	*revstr = "STM32F1xx XL";
	// 	break;

	// case STM32F103_CON:
	// 	*revstr = "STM32F1xx Con";
	// 	break;

	// default:
	// 	*revstr = "STM32F???";
	// 	break;
	// }

	// switch (revid) {

	// case MCU_REV_STM32F4_REV_A:
	// 	*rev = 'A';
	// 	break;

	// case MCU_REV_STM32F4_REV_Z:
	// 	*rev = 'Z';
	// 	break;

	// case MCU_REV_STM32F4_REV_Y:
	// 	*rev = 'Y';
	// 	break;

	// case MCU_REV_STM32F4_REV_1:
	// 	*rev = '1';
	// 	break;

	// case MCU_REV_STM32F4_REV_3:
	// 	*rev = chip_version == STM32H74xx_75xx ? 'X' : '3' ;
	// 	chip_errata = NULL;
	// 	break;

	// case MCU_REV_STM32F4_REV_5:
	// 	// MCU_REV_STM32F4_REV_B shares the same REV_ID
	// 	// MCU_REV_STM32H7_REV_V shares the same REV_ID
	// 	*rev = chip_version == STM32H74xx_75xx ? 'V' : '5';
	// 	chip_errata = NULL;
	// 	break;

	// default:
	// 	// todo add rev for 103 - if needed
	// 	*rev = '?';
	// 	revid = -1;
	// 	break;
	// }

	// if (errata) {
	// 	*errata = chip_errata;
	// }

	// return revid;
	return -1;
}
