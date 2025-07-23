/****************************************************************************
 *
 *   Copyright (c) 2018-2019 PX4 Development Team. All rights reserved.
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
 * @file hrt.c
 * Author: Zhihong Chen <zhihong.chen@hpmicro.com>
 *
 * High-resolution timer callouts and timekeeping.
 *
 * This can use any GPT timer.
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

#include <board_config.h>
#include <drivers/drv_hrt.h>

#include "board.h"
#include "hpm_gptmr_drv.h"
#include "hpm_clock_drv.h"

#undef PPM_DEBUG

#ifdef CONFIG_DEBUG_HRT
#  define hrtinfo _info
#else
#  define hrtinfo(x...)
#endif

#ifdef HRT_TIMER

#define HRT_TIMER_FREQ_MHZ 100

/* HRT configuration */
#if HRT_TIMER == 0
#  define HRT_TIMER_CLOCK_NAME clock_gptmr0
#  define HRT_TIMER_BASE       HPM_GPTMR0
#  define HRT_TIMER_VECTOR     HPM_IRQn_GPTMR0
#elif HRT_TIMER == 1
#  define HRT_TIMER_CLOCK_NAME clock_gptmr1
#  define HRT_TIMER_BASE       HPM_GPTMR1
#  define HRT_TIMER_VECTOR     HPM_IRQn_GPTMR1
#elif HRT_TIMER == 2
#  define HRT_TIMER_CLOCK_NAME clock_gptmr2
#  define HRT_TIMER_BASE       HPM_GPTMR2
#  define HRT_TIMER_VECTOR     HPM_IRQn_GPTMR2
#elif HRT_TIMER == 3
#  define HRT_TIMER_CLOCK_NAME clock_gptmr3
#  define HRT_TIMER_BASE       HPM_GPTMR3
#  define HRT_TIMER_VECTOR     HPM_IRQn_GPTMR3
#elif HRT_TIMER == 4
#  define HRT_TIMER_CLOCK_NAME clock_gptmr4
#  define HRT_TIMER_BASE       HPM_GPTMR4
#  define HRT_TIMER_VECTOR     HPM_IRQn_GPTMR4
#elif HRT_TIMER == 5
#  define HRT_TIMER_CLOCK_NAME clock_gptmr5
#  define HRT_TIMER_BASE       HPM_GPTMR5
#  define HRT_TIMER_VECTOR     HPM_IRQn_GPTMR5
#elif HRT_TIMER == 6
#  define HRT_TIMER_CLOCK_NAME clock_gptmr6
#  define HRT_TIMER_BASE       HPM_GPTMR6
#  define HRT_TIMER_VECTOR     HPM_IRQn_GPTMR6
#elif HRT_TIMER == 7
#  define HRT_TIMER_CLOCK_NAME clock_gptmr7
#  define HRT_TIMER_BASE       HPM_GPTMR7
#  define HRT_TIMER_VECTOR     HPM_IRQn_GPTMR7
#endif
#  define HRT_TIMER_CLOCK_FREQ  clock_get_frequency(HRT_TIMER_CLOCK_NAME)

#if HRT_TIMER == 0 && defined(CONFIG_HPM_TIMER0)
#  error must not set CONFIG_HPM_TIMER0=y and HRT_TIMER=0
#elif   HRT_TIMER == 1 && defined(CONFIG_HPM_TIMER1)
#  error must not set CONFIG_HPM_TIMER1=y and HRT_TIMER=1
#elif   HRT_TIMER == 2 && defined(CONFIG_HPM_TIMER2)
#  error must not set CONFIG_HPM_TIMER2=y and HRT_TIMER=2
#elif   HRT_TIMER == 3 && defined(CONFIG_HPM_TIMER3)
#  error must not set CONFIG_HPM_TIMER3=y and HRT_TIMER=3
#elif   HRT_TIMER == 4 && defined(CONFIG_HPM_TIMER4)
#  error must not set CONFIG_HPM_TIMER4=y and HRT_TIMER=4
#elif   HRT_TIMER == 5 && defined(CONFIG_HPM_TIMER5)
#  error must not set CONFIG_HPM_TIMER5=y and HRT_TIMER=5
#elif   HRT_TIMER == 6 && defined(CONFIG_HPM_TIMER6)
#  error must not set CONFIG_HPM_TIMER6=y and HRT_TIMER=6
#elif   HRT_TIMER == 7 && defined(CONFIG_HPM_TIMER7)
#  error must not set CONFIG_HPM_TIMER7=y and HRT_TIMER=7
#endif

/**
* Minimum/maximum deadlines.
*
* These are suitable for use with a 32-bit timer/counter clocked
* at 100MHz.  The high-resolution timer need only guarantee that it
* not wrap more than once in the 5s period for absolute
* time to be consistently maintained.
*
* The minimum deadline must be such that the time taken between
* reading a time and writing a deadline to the timer cannot
* result in missing the deadline.
*/
#define HRT_INTERVAL_MIN	50
#define HRT_INTERVAL_MAX	5000000    /* unit: us */

/*
* Period of the free-running counter, in microseconds.
*/
#define HRT_COUNTER_PERIOD	4294967296LL

/*
* Scaling factor(s) for the free-running counter; convert an input
* in counts to a time in microseconds.
*/
#define HRT_COUNTER_SCALE(_c)	(_c)

#define rCNT gptmr_channel_get_counter(HRT_TIMER_BASE, HRT_TIMER_CHANNEL, gptmr_counter_type_normal)

#if (HRT_TIMER_CHANNEL < 0) || (HRT_TIMER_CHANNEL > 3)
#  error HRT_TIMER_CHANNEL must be a value between 0 and 3
#endif

/*
 * Queue of callout entries.
 */
static struct sq_queue_s  callout_queue;

/* latency baseline (last compare value applied) */
static uint32_t           latency_baseline;

/* timer count at interrupt (for latency purposes) */
static uint32_t           latency_actual;

/* latency histogram */
const uint16_t latency_bucket_count = LATENCY_BUCKET_COUNT;
const uint16_t latency_buckets[LATENCY_BUCKET_COUNT] = { 1, 2, 5, 10, 20, 50, 100, 1000 };
__EXPORT uint32_t latency_counters[LATENCY_BUCKET_COUNT + 1];

/* timer-specific functions */
static void hrt_tim_init(void);
static int  hrt_tim_isr(int irq, void *context, void *args);
static void hrt_latency_update(void);

/* callout list manipulation */
static void hrt_call_internal(struct hrt_call *entry, hrt_abstime deadline, hrt_abstime interval, hrt_callout callout,
			      void *arg);
static void hrt_call_enter(struct hrt_call *entry);
static void hrt_call_reschedule(void);
static void hrt_call_invoke(void);

#if !defined(HRT_PPM_CHANNEL)

/* When HRT_PPM_CHANNEL is not used provide null operations */

#  define GPT_CR_IM_BOTH    0
#  define STATUS_PPM        0
#  define IFIE_PPM          0

#else

/* Sanity checking */

#   if (HRT_PPM_CHANNEL != HRT_TIMER_CHANNEL)
#     error HRT_PPM_CHANNEL must be the same as HRT_TIMER_CHANNEL
#   endif

/*
 * PPM decoder tuning parameters
 */
#  define PPM_MIN_PULSE_WIDTH    200    /**< minimum width of a valid first pulse */
#  define PPM_MAX_PULSE_WIDTH    600    /**< maximum width of a valid first pulse */
#  define PPM_MIN_CHANNEL_VALUE  800    /**< shortest valid channel signal */
#  define PPM_MAX_CHANNEL_VALUE  2200   /**< longest valid channel signal */
#  define PPM_MIN_START          2300   /**< shortest valid start gap (only 2nd part of pulse) */

/* decoded PPM buffer */

#  define PPM_MIN_CHANNELS       5
#  define PPM_MAX_CHANNELS       20

/** Number of same-sized frames required to 'lock' */

#  define PPM_CHANNEL_LOCK       4 /**< should be less than the input timeout */

__EXPORT uint16_t ppm_buffer[PPM_MAX_CHANNELS];
__EXPORT uint16_t ppm_frame_length = 0;
__EXPORT unsigned ppm_decoded_channels = 0;
__EXPORT uint64_t ppm_last_valid_decode = 0;


#  if defined(PPM_DEBUG)

#    define EDGE_BUFFER_COUNT       32

/* PPM edge history */

__EXPORT uint32_t ppm_edge_history[EDGE_BUFFER_COUNT];
unsigned ppm_edge_next;

/* PPM pulse history */

__EXPORT uint32_t ppm_pulse_history[EDGE_BUFFER_COUNT];
unsigned ppm_pulse_next;
# endif

static uint32_t ppm_temp_buffer[PPM_MAX_CHANNELS];

/** PPM decoder state machine */
struct {
	uint32_t	last_edge;	/**< last capture time */
	uint32_t	last_mark;	/**< last significant edge */
	uint32_t	frame_start;	/**< the frame width */
	unsigned	next_channel;	/**< next channel index */
	enum {
		UNSYNCH = 0,
		ARM,
		ACTIVE,
		INACTIVE
	} phase;
} ppm;

static void	hrt_ppm_decode(uint32_t status);
#endif /* HRT_PPM_CHANNEL */

/**
 * Initialize the timer we are going to use.
 */
static void hrt_tim_init(void)
{
	gptmr_channel_config_t config;
	clock_add_to_group(HRT_TIMER_CLOCK_NAME, 0);

	assert(HRT_TIMER_CLOCK_FREQ == (HRT_TIMER_FREQ_MHZ * 1000000));

	/* claim our interrupt vector */

	irq_attach(HRT_TIMER_VECTOR, hrt_tim_isr, NULL);

	gptmr_channel_get_default_config(HRT_TIMER_BASE, &config);
	config.mode = gptmr_work_mode_capture_at_both_edge;
	config.cmp[0] = 1000 * HRT_TIMER_FREQ_MHZ;    /* 1ms */
	config.cmp[1] = 0xFFFFFFFFu;
	config.reload = 0xFFFFFFFFu;
	gptmr_channel_config(HRT_TIMER_BASE, HRT_TIMER_CHANNEL, &config, false);
	gptmr_channel_reset_count(HRT_TIMER_BASE, HRT_TIMER_CHANNEL);
	gptmr_start_counter(HRT_TIMER_BASE, HRT_TIMER_CHANNEL);

#ifdef HRT_PPM_CHANNEL
	/* enable interrupts */
	gptmr_enable_irq(HRT_TIMER_BASE, GPTMR_CH_CAP_IRQ_MASK(HRT_PPM_CHANNEL) | GPTMR_CH_CMP_IRQ_MASK(HRT_TIMER_CHANNEL, 0));
#else
	gptmr_enable_irq(HRT_TIMER_BASE, GPTMR_CH_CMP_IRQ_MASK(HRT_TIMER_CHANNEL, 0));
#endif
	up_enable_irq(HRT_TIMER_VECTOR);
}

#ifdef HRT_PPM_CHANNEL
/**
 * Handle the PPM decoder state machine.
 */
static void hrt_ppm_decode(uint32_t status)
{
	static bool s_first_cap;
	static uint8_t s_cap_idx;
	uint32_t count_rise = gptmr_channel_get_counter(HRT_TIMER_BASE, HRT_PPM_CHANNEL, gptmr_counter_type_rising_edge);
	uint32_t count_fall = gptmr_channel_get_counter(HRT_TIMER_BASE, HRT_PPM_CHANNEL, gptmr_counter_type_falling_edge);
	uint32_t count;
	uint32_t width;
	uint32_t interval;
	unsigned i;

	if (!s_first_cap) {
		if (count_rise < count_fall) {
			count = count_fall;
			s_cap_idx = 0;

		} else {
			count = count_rise;
			s_cap_idx = 1;
		}

		s_first_cap = true;

	} else {
		if (s_cap_idx == 0) {
			count = count_rise;
			s_cap_idx = 1;

		} else {
			count = count_fall;
			s_cap_idx = 0;
		}
	}

	count /= HRT_TIMER_FREQ_MHZ;    /* Unit: 1us */

	/* how long since the last edge? - this handles counter wrapping implicitly. */
	width = count - ppm.last_edge;

#if PPM_DEBUG
	ppm_edge_history[ppm_edge_next++] = width;

	if (ppm_edge_next >= EDGE_BUFFER_COUNT) {
		ppm_edge_next = 0;
	}

#endif

	/*
	 * if this looks like a start pulse, then push the last set of values
	 * and reset the state machine
	 */
	if (width >= PPM_MIN_START) {

		/*
		 * If the number of channels changes unexpectedly, we don't want
		 * to just immediately jump on the new count as it may be a result
		 * of noise or dropped edges.  Instead, take a few frames to settle.
		 */
		if (ppm.next_channel != ppm_decoded_channels) {
			static unsigned new_channel_count;
			static unsigned new_channel_holdoff;

			if (new_channel_count != ppm.next_channel) {
				/* start the lock counter for the new channel count */
				new_channel_count = ppm.next_channel;
				new_channel_holdoff = PPM_CHANNEL_LOCK;

			} else if (new_channel_holdoff > 0) {
				/* this frame matched the last one, decrement the lock counter */
				new_channel_holdoff--;

			} else {
				/* we have seen PPM_CHANNEL_LOCK frames with the new count, accept it */
				ppm_decoded_channels = new_channel_count;
				new_channel_count = 0;
			}

		} else {
			/* frame channel count matches expected, let's use it */
			if (ppm.next_channel >= PPM_MIN_CHANNELS) {
				for (i = 0; i < ppm.next_channel; i++) {
					ppm_buffer[i] = ppm_temp_buffer[i];
				}

				ppm_last_valid_decode = hrt_absolute_time();

			}
		}

		/* reset for the next frame */
		ppm.next_channel = 0;

		/* next edge is the reference for the first channel */
		ppm.phase = ARM;

		ppm.last_edge = count;
		return;
	}

	switch (ppm.phase) {
	case UNSYNCH:
		/* we are waiting for a start pulse - nothing useful to do here */
		break;

	case ARM:

		/* we expect a pulse giving us the first mark */
		if (width < PPM_MIN_PULSE_WIDTH || width > PPM_MAX_PULSE_WIDTH) {
			goto error;        /* pulse was too short or too long */
		}

		/* record the mark timing, expect an inactive edge */
		ppm.last_mark = ppm.last_edge;

		/* frame length is everything including the start gap */
		ppm_frame_length = (uint16_t)(ppm.last_edge - ppm.frame_start);
		ppm.frame_start = ppm.last_edge;
		ppm.phase = ACTIVE;
		break;

	case INACTIVE:

		/* we expect a short pulse */
		if (width < PPM_MIN_PULSE_WIDTH || width > PPM_MAX_PULSE_WIDTH) {
			goto error;        /* pulse was too short or too long */
		}

		/* this edge is not interesting, but now we are ready for the next mark */
		ppm.phase = ACTIVE;
		break;

	case ACTIVE:
		/* determine the interval from the last mark */
		interval = count - ppm.last_mark;
		ppm.last_mark = count;

#if PPM_DEBUG
		ppm_pulse_history[ppm_pulse_next++] = interval;

		if (ppm_pulse_next >= EDGE_BUFFER_COUNT) {
			ppm_pulse_next = 0;
		}

#endif

		/* if the mark-mark timing is out of bounds, abandon the frame */
		if ((interval < PPM_MIN_CHANNEL_VALUE) || (interval > PPM_MAX_CHANNEL_VALUE)) {
			goto error;
		}

		/* if we have room to store the value, do so */
		if (ppm.next_channel < PPM_MAX_CHANNELS) {
			ppm_temp_buffer[ppm.next_channel++] = interval;
		}

		ppm.phase = INACTIVE;
		break;

	}

	ppm.last_edge = count;
	return;

	/* the state machine is corrupted; reset it */

error:
	/* we don't like the state of the decoder, reset it and try again */
	ppm.phase = UNSYNCH;
	ppm_decoded_channels = 0;

}
#endif /* HRT_PPM_CHANNEL */

/**
 * Handle the compare interrupt by calling the callout dispatcher
 * and then re-scheduling the next deadline.
 */
static int
hrt_tim_isr(int irq, void *context, void *arg)
{
	/* grab the timer for latency tracking purposes */

	latency_actual = rCNT;    /* Unit: 0.01us */

	/* copy interrupt status */
	uint32_t status = gptmr_get_status(HRT_TIMER_BASE);

	/* ack the interrupts we just read */

	gptmr_clear_status(HRT_TIMER_BASE, status);

#ifdef HRT_PPM_CHANNEL

	/* was this a PPM edge? */
	if (status & (GPTMR_CH_CAP_STAT_MASK(HRT_PPM_CHANNEL))) {
		hrt_ppm_decode(status);
	}

#endif

	/* was this a timer tick? */
	if (status & GPTMR_CH_CMP_STAT_MASK(HRT_TIMER_CHANNEL, 0)) {

		/* do latency calculations */
		hrt_latency_update();

		/* run any callouts that have met their deadline */
		hrt_call_invoke();

		/* and schedule the next interrupt */
		hrt_call_reschedule();
	}

	return OK;
}

/**
 * Fetch a never-wrapping absolute time value in microseconds from
 * some arbitrary epoch shortly after system start.
 */
hrt_abstime
hrt_absolute_time(void)
{
	hrt_abstime	abstime;
	uint32_t	count;
	irqstate_t	flags;

	/*
	 * Counter state.  Marked volatile as they may change
	 * inside this routine but outside the irqsave/restore
	 * pair.  Discourage the compiler from moving loads/stores
	 * to these outside of the protected range.
	 */
	static volatile hrt_abstime base_time;
	static volatile uint32_t last_count;

	/* prevent re-entry */
	flags = px4_enter_critical_section();

	/* get the current counter value */
	count = rCNT / HRT_TIMER_FREQ_MHZ;    /* Unit: 1us */

	/*
	 * Determine whether the counter has wrapped since the
	 * last time we're called.
	 *
	 * This simple test is sufficient due to the guarantee that
	 * we are always called at least once per counter period.
	 */
	if (count < last_count) {
		base_time += HRT_COUNTER_PERIOD;
	}

	/* save the count for next time */
	last_count = count;

	/* compute the current time */
	abstime = HRT_COUNTER_SCALE(base_time + count);

	px4_leave_critical_section(flags);

	return abstime;
}

/**
 * Store the absolute time in an interrupt-safe fashion
 */
void
hrt_store_absolute_time(volatile hrt_abstime *t)
{
	irqstate_t flags = px4_enter_critical_section();
	*t = hrt_absolute_time();
	px4_leave_critical_section(flags);
}

/**
 * Initialize the high-resolution timing module.
 */
void
hrt_init(void)
{
	sq_init(&callout_queue);
	hrt_tim_init();

#ifdef HRT_PPM_CHANNEL
	/* configure the PPM input pin */
	px4_arch_configgpio(GPIO_PPM_IN);
#endif
}

/**
 * Call callout(arg) after interval has elapsed.
 */
void
hrt_call_after(struct hrt_call *entry, hrt_abstime delay, hrt_callout callout, void *arg)
{
	hrt_call_internal(entry,
			  hrt_absolute_time() + delay,
			  0,
			  callout,
			  arg);
}

/**
 * Call callout(arg) at calltime.
 */
void
hrt_call_at(struct hrt_call *entry, hrt_abstime calltime, hrt_callout callout, void *arg)
{
	hrt_call_internal(entry, calltime, 0, callout, arg);
}

/**
 * Call callout(arg) every period.
 */
void
hrt_call_every(struct hrt_call *entry, hrt_abstime delay, hrt_abstime interval, hrt_callout callout, void *arg)
{
	hrt_call_internal(entry,
			  hrt_absolute_time() + delay,
			  interval,
			  callout,
			  arg);
}

static void
hrt_call_internal(struct hrt_call *entry, hrt_abstime deadline, hrt_abstime interval, hrt_callout callout, void *arg)
{
	irqstate_t flags = px4_enter_critical_section();

	/* if the entry is currently queued, remove it */
	/* note that we are using a potentially uninitialized
	   entry->link here, but it is safe as sq_rem() doesn't
	   dereference the passed node unless it is found in the
	   list. So we potentially waste a bit of time searching the
	   queue for the uninitialized entry->link but we don't do
	   anything actually unsafe.
	*/
	if (entry->deadline != 0) {
		sq_rem(&entry->link, &callout_queue);
	}

	entry->deadline = deadline;
	entry->period = interval;
	entry->callout = callout;
	entry->arg = arg;

	hrt_call_enter(entry);

	px4_leave_critical_section(flags);
}

/**
 * If this returns true, the call has been invoked and removed from the callout list.
 *
 * Always returns false for repeating callouts.
 */
bool
hrt_called(struct hrt_call *entry)
{
	return (entry->deadline == 0);
}

/**
 * Remove the entry from the callout list.
 */
void
hrt_cancel(struct hrt_call *entry)
{
	irqstate_t flags = px4_enter_critical_section();

	sq_rem(&entry->link, &callout_queue);
	entry->deadline = 0;

	/* if this is a periodic call being removed by the callout, prevent it from
	 * being re-entered when the callout returns.
	 */
	entry->period = 0;

	px4_leave_critical_section(flags);
}

static void
hrt_call_enter(struct hrt_call *entry)
{
	struct hrt_call	*call, *next;

	call = (struct hrt_call *)(void *)sq_peek(&callout_queue);

	if ((call == NULL) || (entry->deadline < call->deadline)) {
		sq_addfirst(&entry->link, &callout_queue);
		hrtinfo("call enter at head, reschedule\n");
		/* we changed the next deadline, reschedule the timer event */
		hrt_call_reschedule();

	} else {
		do {
			next = (struct hrt_call *)(void *)sq_next(&call->link);

			if ((next == NULL) || (entry->deadline < next->deadline)) {
				hrtinfo("call enter after head\n");
				sq_addafter(&call->link, &entry->link, &callout_queue);
				break;
			}
		} while ((call = next) != NULL);
	}

	hrtinfo("scheduled\n");
}

static void
hrt_call_invoke(void)
{
	struct hrt_call	*call;
	hrt_abstime deadline;

	while (true) {
		/* get the current time */
		hrt_abstime now = hrt_absolute_time();

		call = (struct hrt_call *)(void *)sq_peek(&callout_queue);

		if (call == NULL) {
			break;
		}

		if (call->deadline > now) {
			break;
		}

		sq_rem(&call->link, &callout_queue);
		hrtinfo("call pop\n");

		/* save the intended deadline for periodic calls */
		deadline = call->deadline;

		/* zero the deadline, as the call has occurred */
		call->deadline = 0;

		/* invoke the callout (if there is one) */
		if (call->callout) {
			hrtinfo("call %p: %p(%p)\n", call, call->callout, call->arg);
			call->callout(call->arg);
		}

		/* if the callout has a non-zero period, it has to be re-entered */
		if (call->period != 0) {
			// re-check call->deadline to allow for
			// callouts to re-schedule themselves
			// using hrt_call_delay()
			if (call->deadline <= now) {
				call->deadline = deadline + call->period;
			}

			hrt_call_enter(call);
		}
	}
}

/**
 * Reschedule the next timer interrupt.
 *
 * This routine must be called with interrupts disabled.
 */
static void
hrt_call_reschedule()
{
	hrt_abstime	now = hrt_absolute_time();
	struct hrt_call	*next = (struct hrt_call *)(void *)sq_peek(&callout_queue);
	hrt_abstime	deadline = now + HRT_INTERVAL_MAX;

	/*
	 * Determine what the next deadline will be.
	 *
	 * Note that we ensure that this will be within the counter
	 * period, so that when we truncate all but the low 32 bits
	 * the next time the compare matches it will be the deadline
	 * we want.
	 *
	 * It is important for accurate timekeeping that the compare
	 * interrupt fires sufficiently often that the base_time update in
	 * hrt_absolute_time runs at least once per timer period.
	 */
	if (next != NULL) {
		hrtinfo("entry in queue\n");

		if (next->deadline <= (now + HRT_INTERVAL_MIN)) {
			hrtinfo("pre-expired\n");
			/* set a minimal deadline so that we call ASAP */
			deadline = now + HRT_INTERVAL_MIN;

		} else if (next->deadline < deadline) {
			hrtinfo("due soon\n");
			deadline = next->deadline;
		}
	}

	hrtinfo("schedule for %ul at %ul\n", (unsigned long)(deadline & 0xffffffff), (unsigned long)(now & 0xffffffff));

	/* set the new compare value and remember it for latency tracking */
	latency_baseline = deadline * HRT_TIMER_FREQ_MHZ;    /* Unit: 0.01us */

	gptmr_update_cmp(HRT_TIMER_BASE, HRT_TIMER_CHANNEL, 0, latency_baseline);
}

static void
hrt_latency_update(void)
{
	uint16_t latency = latency_actual - latency_baseline;
	unsigned	index;

	latency /= HRT_TIMER_FREQ_MHZ;    /* Unit: 1us */

	/* bounded buckets */
	for (index = 0; index < LATENCY_BUCKET_COUNT; index++) {
		if (latency <= latency_buckets[index]) {
			latency_counters[index]++;
			return;
		}
	}

	/* catch-all at the end */
	latency_counters[index]++;
}

void
hrt_call_init(struct hrt_call *entry)
{
	memset(entry, 0, sizeof(*entry));
}

void
hrt_call_delay(struct hrt_call *entry, hrt_abstime delay)
{
	entry->deadline = hrt_absolute_time() + delay;
}

#endif /* HRT_TIMER */
