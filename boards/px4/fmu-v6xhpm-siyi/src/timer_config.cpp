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


constexpr io_timers_t io_timers[MAX_IO_TIMERS] = {
	// initIOTimer(Timer::PWM0),
	initIOTimer(Timer::PWM1),
	// initIOTimer(Timer::PWM2),
	initIOTimer(Timer::PWM3),
};

constexpr timer_io_channels_t timer_io_channels[MAX_TIMER_IO_CHANNELS] = {
	initIOTimerChannel(io_timers, {Timer::PWM3, Timer::Channel2}, {GPIO::PortD, GPIO::Pin0}, Timer::TRGM_NotUsed),
	initIOTimerChannel(io_timers, {Timer::PWM3, Timer::Channel3}, {GPIO::PortD, GPIO::Pin1}, Timer::TRGM_NotUsed),
	initIOTimerChannel(io_timers, {Timer::PWM3, Timer::Channel4}, {GPIO::PortD, GPIO::Pin2}, Timer::TRGM_NotUsed),
	initIOTimerChannel(io_timers, {Timer::PWM3, Timer::Channel7}, {GPIO::PortC, GPIO::Pin29}, Timer::TRGM_NotUsed),
	initIOTimerChannel(io_timers, {Timer::PWM1, Timer::Channel11}, {GPIO::PortC, GPIO::Pin4}, Timer::TRGM_P11),
	initIOTimerChannel(io_timers, {Timer::PWM1, Timer::Channel9}, {GPIO::PortC, GPIO::Pin5}, Timer::TRGM_P9),
	initIOTimerChannel(io_timers, {Timer::PWM1, Timer::Channel8}, {GPIO::PortC, GPIO::Pin9}, Timer::TRGM_P8),
	initIOTimerChannel(io_timers, {Timer::PWM1, Timer::Channel10}, {GPIO::PortC, GPIO::Pin8}, Timer::TRGM_P10),
};

constexpr io_timers_channel_mapping_t io_timers_channel_mapping =
	initIOTimerChannelMapping(io_timers, timer_io_channels);
