/****************************************************************************
 *
 *   Copyright (C) 2012 PX4 Development Team. All rights reserved.
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

#include <px4_arch/io_timer_hw_description.h>

#include <hpm_soc.h>
#include <hpm_clock_drv.h>

constexpr io_timers_t io_timers[MAX_IO_TIMERS] = {
	initIOTimer(Timer::PWM0),
	initIOTimer(Timer::PWM1),
	// initIOTimer(Timer::PWM2),
	// initIOTimer(Timer::PWM3),
};

constexpr timer_io_channels_t timer_io_channels[MAX_TIMER_IO_CHANNELS] = {
	initIOTimerChannel(io_timers, {Timer::PWM1, Timer::Channel0}, {GPIO::PortA, GPIO::Pin19}, Timer::TRGM_NotUsed),
	initIOTimerChannel(io_timers, {Timer::PWM1, Timer::Channel1}, {GPIO::PortA, GPIO::Pin18}, Timer::TRGM_NotUsed),
	initIOTimerChannel(io_timers, {Timer::PWM1, Timer::Channel2}, {GPIO::PortA, GPIO::Pin17}, Timer::TRGM_NotUsed),
	initIOTimerChannel(io_timers, {Timer::PWM1, Timer::Channel3}, {GPIO::PortA, GPIO::Pin16}, Timer::TRGM_NotUsed),
};

/*
 * Bidirectional DShot sample timers, indexed by io_timers[] (not PWM number).
 * GPTMR0 is the NuttX timer and GPTMR1 is the PX4 HRT. PWM0 has no routed
 * outputs; all four PWM1 outputs are sampled together on PortA.
 */
constexpr bdshot_sample_timer_t bdshot_sample_timers[MAX_IO_TIMERS] = {
	{}, /* PWM0: no outputs */
	{ HPM_GPTMR3_BASE, 0, clock_gptmr3, HPM_DMA_SRC_GPTMR3_0 }, /* PWM1: outputs 1-4 */
};

constexpr io_timers_channel_mapping_t io_timers_channel_mapping =
	initIOTimerChannelMapping(io_timers, timer_io_channels);
