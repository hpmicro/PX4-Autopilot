/****************************************************************************
 *
 *   Copyright (C) 2024 PX4 Development Team. All rights reserved.
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
 * @file io_timer.c
 *
 */

#include <px4_platform_common/px4_config.h>
#include <systemlib/px4_macros.h>
#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include <sys/types.h>
#include <stdbool.h>

#include <assert.h>
#include <debug.h>
#include <time.h>
#include <queue.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <arch/board/board.h>
#include <drivers/drv_pwm_output.h>

#include <px4_arch/io_timer.h>
// #include <px4_arch/dshot.h>

#include <hpm_gpio.h>
#include "hpm_trgm_drv.h"
#include "hpm_pwm_drv.h"
#include "hpm_clock_drv.h"

static int io_timer_handler0(int irq, void *context, void *arg);
static int io_timer_handler1(int irq, void *context, void *arg);
static int io_timer_handler2(int irq, void *context, void *arg);
static int io_timer_handler3(int irq, void *context, void *arg);

#if !defined(BOARD_PWM_FREQ)
#define BOARD_PWM_FREQ 1000000
#endif

#if !defined(BOARD_ONESHOT_FREQ)
#define BOARD_ONESHOT_FREQ 8000000
#endif

//                                                                NotUsed   PWMOut  PWMIn Capture OneShot Trigger Dshot LED PPS Other
io_timer_channel_allocation_t channel_allocations[IOTimerChanModeSize] = { UINT16_MAX,   0,  0,  0, 0, 0, 0, 0, 0, 0 };

typedef uint8_t io_timer_allocation_t; /* big enough to hold MAX_IO_TIMERS */

io_timer_channel_allocation_t timer_allocations[MAX_IO_TIMERS] = { };

#if !defined(BOARD_HAS_NO_CAPTURE)

/* Stats and handlers are only useful for Capture */

typedef struct channel_stat_t {
	uint32_t            isr_cout;
	uint32_t            overflows;
} channel_stat_t;

static channel_stat_t io_timer_channel_stats[MAX_TIMER_IO_CHANNELS];

static struct channel_handler_entry {
	channel_handler_t callback;
	void              *context;
} channel_handlers[MAX_TIMER_IO_CHANNELS];
#endif // !defined(BOARD_HAS_NO_CAPTURE)


static int io_timer_handler(uint16_t timer_index)
{
#if !defined(BOARD_HAS_NO_CAPTURE)
	/* Read the count at the time of the interrupt */

	uint16_t count = rCNT(timer_index);

	/* Read the HRT at the time of the interrupt */

	hrt_abstime now = hrt_absolute_time();

	const io_timers_t *tmr = &io_timers[timer_index];

	/* What is pending and enabled? */

	uint16_t statusr = rSR(timer_index);
	uint16_t enabled = rDIER(timer_index) & GTIM_SR_CCIF;

	/* Iterate over the timer_io_channels table */

	uint32_t first_channel_index = io_timers_channel_mapping.element[timer_index].first_channel_index;
	uint32_t last_channel_index = first_channel_index + io_timers_channel_mapping.element[timer_index].channel_count;

	for (unsigned chan_index = first_channel_index; chan_index < last_channel_index; chan_index++) {

		uint16_t masks = timer_io_channels[chan_index].masks;

		/* Do we have an enabled channel */

		if (enabled & masks) {


			if (statusr & masks & GTIM_SR_CCIF) {

				io_timer_channel_stats[chan_index].isr_cout++;

				/* Call the client to read the CCxR etc and clear the CCxIF */

				if (channel_handlers[chan_index].callback) {
					channel_handlers[chan_index].callback(channel_handlers[chan_index].context, tmr,
									      chan_index, &timer_io_channels[chan_index],
									      now, count);
				}
			}

			if (statusr & masks & GTIM_SR_CCOF) {

				/* Error we has a second edge before we cleared CCxR */

				io_timer_channel_stats[chan_index].overflows++;
			}
		}
	}

	/* Clear all the SR bits for interrupt enabled channels only */

	rSR(timer_index) = ~(statusr & (enabled | enabled << 8));
#endif // !defined(BOARD_HAS_NO_CAPTURE)
	return 0;
}

int io_timer_handler0(int irq, void *context, void *arg)
{
	return io_timer_handler(0);
}

int io_timer_handler1(int irq, void *context, void *arg)
{
	return io_timer_handler(1);
}

int io_timer_handler2(int irq, void *context, void *arg)
{
	return io_timer_handler(2);
}

int io_timer_handler3(int irq, void *context, void *arg)
{
	return io_timer_handler(3);
}

static inline int validate_timer_index(unsigned timer)
{
	return (timer < MAX_IO_TIMERS && io_timers[timer].base != 0) ? 0 : -EINVAL;
}

int io_timer_allocate_timer(unsigned timer, io_timer_channel_mode_t mode)
{
	int ret = -EINVAL;

	if (validate_timer_index(timer) == 0) {
		// check if timer is unused or already set to the mode we want
		if (timer_allocations[timer] == IOTimerChanMode_NotUsed || timer_allocations[timer] == mode) {
			timer_allocations[timer] = mode;
			ret = 0;

		} else {
			ret = -EBUSY;
		}
	}

	return ret;
}

int io_timer_unallocate_timer(unsigned timer)
{
	int ret = -EINVAL;

	if (validate_timer_index(timer) == 0) {
		timer_allocations[timer] = IOTimerChanMode_NotUsed;
		ret = 0;
	}

	return ret;
}

static inline int channels_timer(unsigned channel)
{
	return timer_io_channels[channel].timer_index;
}

static uint32_t get_timer_channels(unsigned timer)
{
	uint32_t channels = 0;
	static uint32_t channels_cache[MAX_IO_TIMERS] = {0};

	if (validate_timer_index(timer) < 0) {
		return channels;

	} else {
		if (channels_cache[timer] == 0) {
			/* Gather the channel bits that belong to the timer */

			uint32_t first_channel_index = io_timers_channel_mapping.element[timer].first_channel_index;
			uint32_t last_channel_index = first_channel_index + io_timers_channel_mapping.element[timer].channel_count;

			for (unsigned chan_index = first_channel_index; chan_index < last_channel_index; chan_index++) {
				channels |= 1 << chan_index;
			}

			/* cache them */

			channels_cache[timer] = channels;
		}
	}

	return channels_cache[timer];
}

int io_timer_validate_channel_index(unsigned channel)
{
	int rv = -EINVAL;

	if (channel < MAX_TIMER_IO_CHANNELS) {

		/* test timer for validity */

		if (io_timers[channels_timer(channel)].base != 0) {
			rv = 0;
		}
	}

	return rv;
}

uint32_t io_timer_channel_get_gpio_output(unsigned channel)
{
	if (io_timer_validate_channel_index(channel) != 0) {
		return 0;
	}

	return (timer_io_channels[channel].gpio_out & (GPIO_PORT_MASK | GPIO_PIN_MASK)) |
	       (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_OUTPUT_CLEAR);
}

uint32_t io_timer_channel_get_as_pwm_input(unsigned channel)
{
	if (io_timer_validate_channel_index(channel) != 0) {
		return 0;
	}

	return timer_io_channels[channel].gpio_in;
}

int io_timer_get_mode_channels(io_timer_channel_mode_t mode)
{
	if (mode < IOTimerChanModeSize) {
		return channel_allocations[mode];
	}

	return 0;
}

int io_timer_get_channel_mode(unsigned channel)
{
	io_timer_channel_allocation_t bit = 1 << channel;

	for (int mode = IOTimerChanMode_NotUsed; mode < IOTimerChanModeSize; mode++) {
		if (bit & channel_allocations[mode]) {
			return mode;
		}
	}

	return -1;
}

static int reallocate_channel_resources(uint32_t channels, io_timer_channel_mode_t mode,
					io_timer_channel_mode_t new_mode)
{
	/* If caller mode is not based on current setting adjust it */

	if ((channels & channel_allocations[IOTimerChanMode_NotUsed]) == channels) {
		mode = IOTimerChanMode_NotUsed;
	}

	/* Remove old set of channels from original */

	channel_allocations[mode] &= ~channels;

	/* Will this change ?*/

	uint32_t before = channel_allocations[new_mode] & channels;

	/* add in the new set */

	channel_allocations[new_mode] |= channels;

	/* Indicate a mode change */

	return before ^ channels;
}

__EXPORT int io_timer_allocate_channel(unsigned channel, io_timer_channel_mode_t mode)
{
	irqstate_t flags = px4_enter_critical_section();
	int existing_mode = io_timer_get_channel_mode(channel);
	int ret = -EBUSY;

	if (existing_mode <= IOTimerChanMode_NotUsed || existing_mode == (int)mode) {
		io_timer_channel_allocation_t bit = 1 << channel;
		channel_allocations[IOTimerChanMode_NotUsed] &= ~bit;
		channel_allocations[mode] |= bit;
		ret = 0;
	}

	px4_leave_critical_section(flags);

	return ret;
}


int io_timer_unallocate_channel(unsigned channel)
{
	int mode = io_timer_get_channel_mode(channel);

	if (mode > IOTimerChanMode_NotUsed) {
		io_timer_channel_allocation_t bit = 1 << channel;
		channel_allocations[mode] &= ~bit;
		channel_allocations[IOTimerChanMode_NotUsed] |= bit;
	}

	return mode;
}

static int allocate_channel(unsigned channel, io_timer_channel_mode_t mode)
{
	int rv = -EINVAL;

	if (mode != IOTimerChanMode_NotUsed) {
		rv = io_timer_validate_channel_index(channel);

		if (rv == 0) {
			rv = io_timer_allocate_channel(channel, mode);
		}
	}

	return rv;
}

static int timer_set_rate(unsigned timer, unsigned rate)
{
	/* configure the timer to update at the desired rate */

	uint32_t reload = (clock_get_frequency(io_timers[timer].clock_name) / rate) - 1;
	pwm_cmp_config_t cmp_config = {0};

	pwm_set_reload((PWM_Type *)io_timers[timer].base, 0, reload);

	cmp_config.mode = pwm_cmp_mode_output_compare;
	cmp_config.cmp = reload;
	cmp_config.update_trigger = pwm_shadow_register_update_on_modify;
	pwm_load_cmp_shadow_on_match((PWM_Type *)io_timers[timer].base, 16,  &cmp_config);

	return 0;
}

static inline void io_timer_set_oneshot_mode(unsigned timer, int changed_channels)
{
	pwm_set_reload((PWM_Type *)io_timers[timer].base, 0, 0xFFFFFF);

	for (unsigned channel = 0; changed_channels != 0 &&  channel < MAX_TIMER_IO_CHANNELS; channel++) {
		if (changed_channels & (1 << channel)) {

			/* configure the channel */

			pwm_cmp_config_t cmp_config = { 0 };

			cmp_config.mode = pwm_cmp_mode_output_compare;
			cmp_config.cmp = 0xFFFFFF;
			cmp_config.update_trigger = pwm_shadow_register_update_on_shlk;
			pwm_config_cmp((PWM_Type *)io_timers[timer].base, timer_io_channels[channel].timer_channel * 2,  &cmp_config);
			pwm_config_cmp((PWM_Type *)io_timers[timer].base, timer_io_channels[channel].timer_channel * 2 + 1,  &cmp_config);

			changed_channels &= ~(1 << channel);

		}
	}

	pwm_issue_shadow_register_lock_event((PWM_Type *)io_timers[timer].base);
}

static inline void io_timer_set_PWM_mode(unsigned timer, int changed_channels)
{
	for (unsigned channel = 0; changed_channels != 0 &&  channel < MAX_TIMER_IO_CHANNELS; channel++) {
		if (changed_channels & (1 << channel)) {

			/* configure the channel */

			pwm_cmp_config_t cmp_config = { 0 };

			cmp_config.mode = pwm_cmp_mode_output_compare;
			cmp_config.cmp = 0xFFFFFF;
			cmp_config.update_trigger = pwm_shadow_register_update_on_hw_event;
			pwm_config_cmp((PWM_Type *)io_timers[timer].base, timer_io_channels[channel].timer_channel * 2,  &cmp_config);
			pwm_config_cmp((PWM_Type *)io_timers[timer].base, timer_io_channels[channel].timer_channel * 2 + 1,  &cmp_config);

			changed_channels &= ~(1 << channel);

		}
	}
}

void io_timer_trigger(unsigned channels_mask)	/* Trigger all timer's channels in Oneshot mode */
{
	int oneshots = io_timer_get_mode_channels(IOTimerChanMode_OneShot) & channels_mask;

	if (oneshots != 0) {
		struct {
			uint32_t base;
			uint32_t trgm_base;
		} action_cache[MAX_IO_TIMERS] = {0};

		int actions = 0;

		// Get the Timer modules addresses
		for (int timer = 0; timer < MAX_IO_TIMERS && io_timers[timer].base != 0; timer++) {
			if (action_cache[actions].base == 0) {
				action_cache[actions].base = io_timers[timer].base;
				action_cache[actions].trgm_base = io_timers[timer].trgm_base;

			} else if (action_cache[actions].base != io_timers[timer].base) {
				actions++;
				action_cache[actions].base = io_timers[timer].base;
				action_cache[actions].trgm_base = io_timers[timer].trgm_base;
			}
		}

		/* Now do them all with the shortest delay in between */

		irqstate_t flags = px4_enter_critical_section();

		for (actions = 0; actions < MAX_IO_TIMERS && action_cache[actions].base != 0; actions++) {
			pwm_issue_shadow_register_lock_event((PWM_Type *)action_cache[actions].base);

			trgm_output_update_source((TRGM_Type *)action_cache[actions].trgm_base, TRGM_TRGOCFG_PWM_SYNCI, 1);
			trgm_output_update_source((TRGM_Type *)action_cache[actions].trgm_base, TRGM_TRGOCFG_PWM_SYNCI, 0);
		}

		px4_leave_critical_section(flags);
	}
}

int io_timer_init_timer(unsigned timer, io_timer_channel_mode_t mode)
{
	if (validate_timer_index(timer) != 0) {
		return -EINVAL;
	}

	io_timer_channel_mode_t previous_mode = timer_allocations[timer];
	int rv = io_timer_allocate_timer(timer, mode);

	/* Do this only once per timer */
	if (rv == 0 && previous_mode == IOTimerChanMode_NotUsed) {

		irqstate_t flags = px4_enter_critical_section();

		/* enable the timer clock before we try to talk to it */
		clock_add_to_group((clock_name_t)io_timers[timer].clock_name, 0);

		/* disable and configure the timer */
		pwm_stop_counter((PWM_Type *)io_timers[timer].base);

		/* enable reload when synci assert */
		pwm_enable_reload_at_synci((PWM_Type *)io_timers[timer].base);

		/*
		 * Note we do the Standard PWM Out init here
		 * default to updating at 50Hz
		 */

		timer_set_rate(timer, 50);

		/*
		 * Note that the timer is left disabled with IRQ subs installed
		 * and active but DEIR bits are not set.
		 */
		xcpt_t handler;

		switch (io_timers[timer].base) {
		case HPM_PWM0_BASE: handler = io_timer_handler0; break;

		case HPM_PWM1_BASE: handler = io_timer_handler1; break;

		case HPM_PWM2_BASE: handler = io_timer_handler2; break;

		case HPM_PWM3_BASE: handler = io_timer_handler3; break;

		default:
			handler = NULL;
			rv = -EINVAL;
			break;
		}

		if (handler) {
			irq_attach(io_timers[timer].vectorno, handler, NULL);
			up_enable_irq(io_timers[timer].vectorno);
		}

		px4_leave_critical_section(flags);
	}

	return rv;
}

int io_timer_set_pwm_rate(unsigned timer, unsigned rate)
{
	/* Change only a timer that is owned by pwm or one shot */
	if (timer_allocations[timer] != IOTimerChanMode_PWMOut && timer_allocations[timer] != IOTimerChanMode_OneShot) {
		return -EINVAL;
	}

	/* Get the channel bits that belong to the timer and are in PWM or OneShot mode */

	uint32_t channels = get_timer_channels(timer) & (io_timer_get_mode_channels(IOTimerChanMode_OneShot) |
			    io_timer_get_mode_channels(IOTimerChanMode_PWMOut));

	/* Request to use OneShot ?*/

	if (PWM_RATE_ONESHOT == rate) {

		/* Request to use OneShot
		 */
		int changed_channels = reallocate_channel_resources(channels, IOTimerChanMode_PWMOut, IOTimerChanMode_OneShot);

		/* Did the allocation change */
		if (changed_channels) {
			io_timer_set_oneshot_mode(timer, changed_channels);
		}

	} else {

		/* Request to use PWM
		 */
		int changed_channels = reallocate_channel_resources(channels, IOTimerChanMode_OneShot, IOTimerChanMode_PWMOut);

		if (changed_channels) {
			io_timer_set_PWM_mode(timer, changed_channels);
		}

		timer_set_rate(timer, rate);
	}

	return OK;
}

int io_timer_channel_init(unsigned channel, io_timer_channel_mode_t mode,
			  channel_handler_t channel_handler, void *context)
{
	if (io_timer_validate_channel_index(channel) != 0) {
		return -EINVAL;
	}

	uint32_t gpio = 0;

	/* figure out the GPIO config first */

	switch (mode) {

	case IOTimerChanMode_OneShot:
	case IOTimerChanMode_PWMOut:
	case IOTimerChanMode_Trigger:
		gpio = timer_io_channels[channel].gpio_out;
		break;

	case IOTimerChanMode_PWMIn:
	case IOTimerChanMode_Capture:
		return -EINVAL;
		break;

	case IOTimerChanMode_NotUsed:
		break;

	default:
		return -EINVAL;
	}

	irqstate_t flags = px4_enter_critical_section(); // atomic channel allocation and hw config

	int previous_mode = io_timer_get_channel_mode(channel);
	int rv = allocate_channel(channel, mode);
	unsigned timer = channels_timer(channel);

	if (rv == 0) {
		/* Try to reserve & initialize the timer - it will only do it once */

		rv = io_timer_init_timer(timer, mode);

		if (rv != 0 && previous_mode == IOTimerChanMode_NotUsed) {
			/* free the channel if it was not used before */
			io_timer_unallocate_channel(channel);
		}
	}

	/* Valid channel should now be reserved in new mode */

	if (rv == 0) {

		/* Set up IO */
		if (gpio) {
			px4_arch_configgpio(gpio);
		}

		/* configure the channel */

		pwm_cmp_config_t cmp_config[2] = {0};
		pwm_config_t pwm_config;

		pwm_get_default_pwm_config((PWM_Type *)io_timers[timer].base, &pwm_config);
		pwm_config.enable_output = true;

		cmp_config[0].mode = pwm_cmp_mode_output_compare;
		cmp_config[0].cmp = 0xFFFFFF;
		if (mode == IOTimerChanMode_PWMOut) {
			cmp_config[0].update_trigger = pwm_shadow_register_update_on_hw_event;
		} else {
			cmp_config[0].update_trigger = pwm_shadow_register_update_on_shlk;
		}

		cmp_config[1].mode = pwm_cmp_mode_output_compare;
		cmp_config[1].cmp = 0xFFFFFF;
		if (mode == IOTimerChanMode_PWMOut) {
			cmp_config[1].update_trigger = pwm_shadow_register_update_on_hw_event;
		} else {
			cmp_config[1].update_trigger = pwm_shadow_register_update_on_shlk;
		}

		/*
		 * config pwm
		 */
		if (status_success != pwm_setup_waveform((PWM_Type *)io_timers[timer].base, timer_io_channels[channel].timer_channel, &pwm_config, timer_io_channels[channel].timer_channel * 2, &cmp_config[0], 2)) {
			printf("failed to setup waveform\n");
		}


#if !defined(BOARD_HAS_NO_CAPTURE)
		channel_handlers[channel].callback = channel_handler;
		channel_handlers[channel].context = context;
		pwm_enable_irq((PWM_Type *)io_timers[timer].base, PWM_IRQ_CMP(timer_io_channels[channel].timer_channel));
#endif
	}

	px4_leave_critical_section(flags);

	return rv;
}

int io_timer_set_enable(bool state, io_timer_channel_mode_t mode, io_timer_channel_allocation_t masks)
{
	switch (mode) {
	case IOTimerChanMode_NotUsed:
	case IOTimerChanMode_PWMOut:
	case IOTimerChanMode_OneShot:
	case IOTimerChanMode_Trigger:
		break;

	case IOTimerChanMode_PWMIn:
	case IOTimerChanMode_Capture:
	default:
		return -EINVAL;
	}

	/* Was the request for all channels in this mode ?*/

	if (masks == IO_TIMER_ALL_MODES_CHANNELS) {

		/* Yes - we provide them */

		masks = channel_allocations[mode];

	} else {

		/* No - caller provided mask */

		/* Only allow the channels in that mode to be affected */

		masks &= channel_allocations[mode];

	}

	if (masks) {
		struct {
			uint32_t base;
			uint32_t trgm_base;
			uint32_t io_index;
			uint32_t gpios[MAX_TIMER_IO_CHANNELS];
		} action_cache[MAX_IO_TIMERS];

		unsigned int actions = 0;
		memset(action_cache, 0, sizeof(action_cache));

		for (int timer = 0; timer < MAX_IO_TIMERS && io_timers[timer].base != 0; timer++) {
			if (action_cache[actions].base == 0) {
				action_cache[actions].base = io_timers[timer].base;
				action_cache[actions].trgm_base = io_timers[timer].trgm_base;
			} else if (action_cache[actions].base != io_timers[timer].base) {
				actions++;
				action_cache[actions].base = io_timers[timer].base;
				action_cache[actions].trgm_base = io_timers[timer].trgm_base;
			}
		}

		int chan_index = 0;

		for (actions = 0; actions < MAX_IO_TIMERS; actions++) {
			if (action_cache[actions].base == 0) {
				// All have been processed
				break;
			}

			for (; masks != 0 && chan_index < MAX_TIMER_IO_CHANNELS; chan_index++) {

				if (masks & (1 << chan_index)) {

					if (io_timers[timer_io_channels[chan_index].timer_index].base != action_cache[actions].base) {
						break;
					}

					masks &= ~(1 << chan_index);

					action_cache[actions].gpios[action_cache[actions].io_index++] = timer_io_channels[chan_index].gpio_out;
				}
			}
		}

		irqstate_t flags = px4_enter_critical_section();

		for (actions = 0; actions < arraySize(action_cache); actions++) {
			if (action_cache[actions].base == 0) {
				break;
			}

			for (unsigned int index = 0; index < action_cache[actions].io_index; index++) {
				if (action_cache[actions].gpios[index]) {
					px4_arch_configgpio(action_cache[actions].gpios[index]);
				}
			}
			trgm_output_update_source((TRGM_Type *)action_cache[actions].trgm_base, TRGM_TRGOCFG_PWM_SYNCI, 1);
			trgm_output_update_source((TRGM_Type *)action_cache[actions].trgm_base, TRGM_TRGOCFG_PWM_SYNCI, 0);
			pwm_start_counter((PWM_Type *)action_cache[actions].base);
		}

		px4_leave_critical_section(flags);
	}

	return 0;
}

int io_timer_set_ccr(unsigned channel, uint16_t value)     // value in 1us, oneshot125 should be div8
{
	int rv = io_timer_validate_channel_index(channel);
	int mode = io_timer_get_channel_mode(channel);
	int timer = channels_timer(channel);
	unsigned int ccr;

	if (rv == 0) {
		if ((mode != IOTimerChanMode_PWMOut) &&
		    (mode != IOTimerChanMode_OneShot) &&
		    (mode != IOTimerChanMode_Dshot) &&
		    (mode != IOTimerChanMode_Trigger)) {

			rv = -EIO;

		} else {
			/* configure the channel */
			if (mode == IOTimerChanMode_OneShot) {
				ccr = (uint32_t)value * (clock_get_frequency(io_timers[timer].clock_name) / BOARD_ONESHOT_FREQ);
			} else {
				ccr = (uint32_t)value * (clock_get_frequency(io_timers[timer].clock_name) / BOARD_PWM_FREQ);
			}
			pwm_cmp_update_cmp_value((PWM_Type *)io_timers[timer].base, (timer_io_channels[channel].timer_channel * 2), 0, 0);
			pwm_cmp_update_cmp_value((PWM_Type *)io_timers[timer].base, (timer_io_channels[channel].timer_channel * 2) + 1, ccr , 0);
		}
	}

	return rv;
}

uint16_t io_channel_get_ccr(unsigned channel)     // return value in 1us
{
	uint32_t value = 0;
	int timer = channels_timer(channel);

	if (io_timer_validate_channel_index(channel) == 0) {
		int mode = io_timer_get_channel_mode(channel);

		if ((mode == IOTimerChanMode_PWMOut) ||
		    (mode == IOTimerChanMode_OneShot) ||
		    (mode == IOTimerChanMode_Trigger)) {
			value = pwm_cmp_get_cmp_value((PWM_Type *)io_timers[timer].base, (timer_io_channels[channel].timer_channel * 2) + 1);
			if (mode == IOTimerChanMode_OneShot) {
				value = value / (clock_get_frequency(io_timers[timer].clock_name) / BOARD_ONESHOT_FREQ);
			} else {
				value = value / (clock_get_frequency(io_timers[timer].clock_name) / BOARD_PWM_FREQ);
			}
		}
	}

	return value;
}

uint32_t io_timer_get_group(unsigned timer)
{
	return get_timer_channels(timer);

}
