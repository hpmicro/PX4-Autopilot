/****************************************************************************
 *
 *   Copyright (C) 2020 PX4 Development Team. All rights reserved.
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

#include <px4_arch/i2c_hw_description.h>
#include <drivers/drv_hrt.h>

#if defined(CONFIG_I2C)
/****************************************************************************
 * Name: hpm6750_i2cbus_pins_initialize
 *
 * Description:
 *   Initialize the selected I2C port pins
 *
 * Input Parameters:
 *   Port number (for hardware that has multiple CAN interfaces)
 *
 * Returned Value:
 *   Zero on success; a negated errno on failure
 *
 ****************************************************************************/
int hpm_i2cbus_pins_init(int port)
{
	switch(port){
		case 0:
#if defined(CONFIG_HPM_I2C0)
			px4_arch_configgpio(GPIO_I2C0_SCL);
			px4_arch_configgpio(GPIO_I2C0_SDA);
#endif
			break;
		case 1:
#if defined(CONFIG_HPM_I2C1)
			px4_arch_configgpio(GPIO_I2C1_SCL);
			px4_arch_configgpio(GPIO_I2C1_SDA);
#endif
			break;
		case 2:
#if defined(CONFIG_HPM_I2C2)
			px4_arch_configgpio(GPIO_I2C2_SCL);
			px4_arch_configgpio(GPIO_I2C2_SDA);
#endif
			break;
		case 3:
#if defined(CONFIG_HPM_I2C3)
			px4_arch_configgpio(GPIO_I2C3_SCL);
			px4_arch_configgpio(GPIO_I2C3_SDA);
#endif
			break;
	}

 return 0;
}

constexpr px4_i2c_bus_t px4_i2c_buses[I2C_BUS_MAX_BUS_ITEMS] = {
	// initI2CBusExternal(0),
	// initI2CBusExternal(1),
	initI2CBusExternal(2),
	initI2CBusInternal(-1),
};


#endif
