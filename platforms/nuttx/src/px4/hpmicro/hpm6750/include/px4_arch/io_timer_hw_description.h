/****************************************************************************
 *
 *   Copyright (c) 2019 PX4 Development Team. All rights reserved.
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
#pragma once


#include "../../../hpm_common/include/px4_arch/io_timer_hw_description.h"

static inline constexpr timer_io_channels_t initIOTimerGPIOInOut(Timer::TimerChannel timer, GPIO::GPIOPin pin)
{
	timer_io_channels_t ret{};
	uint32_t pin_port = getGPIOPort(pin.port) | getGPIOPin(pin.pin);

	ret.gpio_in = GPIO_AF16 | (GPIO_ALT | GPIO_FLOAT) | pin_port;
	ret.gpio_out = GPIO_AF16 | (GPIO_ALT | GPIO_PUSHPULL) | pin_port;

	return ret;
}

static inline constexpr io_timers_t initIOTimer(Timer::Timer timer)
{
	io_timers_t ret{};

	switch (timer) {
	case Timer::PWM0:
		ret.base = HPM_PWM0_BASE;
		ret.trgm_base = HPM_TRGM0_BASE;
		ret.clock_name = (uint32_t)clock_mot0;
		ret.vectorno =  HPM_IRQn_PWM0;
		break;

	case Timer::PWM1:
		ret.base = HPM_PWM1_BASE;
		ret.trgm_base = HPM_TRGM1_BASE;
		ret.clock_name = (uint32_t)clock_mot1;
		ret.vectorno =  HPM_IRQn_PWM1;
		break;

	case Timer::PWM2:
		ret.base = HPM_PWM2_BASE;
		ret.trgm_base = HPM_TRGM2_BASE;
		ret.clock_name = (uint32_t)clock_mot2;
		ret.vectorno =  HPM_IRQn_PWM2;
		break;

	case Timer::PWM3:
		ret.base = HPM_PWM3_BASE;
		ret.trgm_base = HPM_TRGM3_BASE;
		ret.clock_name = (uint32_t)clock_mot3;
		ret.vectorno =  HPM_IRQn_PWM3;
		break;

	default: break;
	}

	return ret;
}

