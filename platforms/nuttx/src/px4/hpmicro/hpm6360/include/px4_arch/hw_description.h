/****************************************************************************
 *
 *   Copyright (C) 2019 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in
 *	the documentation and/or other materials provided with the
 *	distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *	used to endorse or promote products derived from this software
 *	without specific prior written permission.
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

#include "../../../hpm_common/include/px4_arch/hw_description.h"

/*
 * PWM
 */

namespace Timer
{
enum Timer {
	PWM0 = 0,
	PWM1,
};

enum Channel {
	Channel0 = 0,
	Channel1,
	Channel2,
	Channel3,
	Channel4,
	Channel5,
	Channel6,
	Channel7,
	Channel8,
	Channel9,
	Channel10,
	Channel11,
	Channel12,
	Channel13,
	Channel14,
	Channel15,
	Channel16,
	Channel17,
	Channel18,
	Channel19,
	Channel20,
	Channel21,
	Channel22,
	Channel23,
	ChannelInvalid = 0xFF,
};

struct TimerChannel {
	Timer timer;
	Channel channel;
};

enum TRGM_Px {
	TRGM_P0 = 0,
	TRGM_P1,
	TRGM_P2,
	TRGM_P3,
	TRGM_P4,
	TRGM_P5,
	TRGM_P6,
	TRGM_P7,
	TRGM_P8,
	TRGM_P9,
	TRGM_P10,
	TRGM_P11,
	TRGM_NotUsed = 0xFF,
};
}

static inline constexpr uint32_t timerBaseRegister(Timer::Timer timer)
{
	switch (timer) {
	case Timer::PWM0: return HPM_PWM0_BASE;

	case Timer::PWM1: return HPM_PWM1_BASE;
	}

	return 0;
}
