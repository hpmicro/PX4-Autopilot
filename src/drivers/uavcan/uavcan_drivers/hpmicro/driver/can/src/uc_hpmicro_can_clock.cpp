/*
 * Copyright (C) 2014 Pavel Kirienko <pavel.kirienko@gmail.com>
 */

#include <uavcan_hpmicro_can/clock.hpp>
#include <uavcan_hpmicro_can/thread.hpp>
#include "internal.hpp"

#if UAVCAN_HPMICRO_TIMER_NUMBER

#include "hpm_soc_irq.h"

# include <nuttx/arch.h>
# include <nuttx/irq.h>
# include <arch/board/board.h>
# include <board_config.h>

#include <cassert>
#include <math.h>
#include <riscv_internal.h>

#include "hpm_gptmr_drv.h"
#include "hpm_clock_drv.h"

/*
 * Timer instance
 */
# if UAVCAN_HPMICRO_NUTTX
# if UAVCAN_HPMICRO_TIMER_NUMBER >= 0 && UAVCAN_HPMICRO_TIMER_NUMBER <= 7
#  define TIMX                    UAVCAN_HPMICRO_GLUE2(HPM_GPTMR, UAVCAN_HPMICRO_TIMER_NUMBER)

#  define TIMX_IRQn               UAVCAN_HPMICRO_GLUE2(HPM_IRQn_GPTMR, UAVCAN_HPMICRO_TIMER_NUMBER)
#  define TIMX_CLOCK              UAVCAN_HPMICRO_GLUE2(clock_gptmr, UAVCAN_HPMICRO_TIMER_NUMBER)
# else
#  error "This UAVCAN_HPMICRO_TIMER_NUMBER is not supported yet"
# endif
# endif


extern "C" UAVCAN_HPMICRO_IRQ_HANDLER(TIMX_IRQHandler);

namespace uavcan_hpmicro_can
{
namespace clock
{
namespace
{

const uavcan::uint32_t USecPerOverflow = 65536;

Mutex mutex;

bool initialized = false;

bool utc_set = false;
bool utc_locked = false;
uavcan::uint32_t utc_jump_cnt = 0;
UtcSyncParams utc_sync_params;
float utc_prev_adj = 0;
float utc_rel_rate_ppm = 0;
float utc_rel_rate_error_integral = 0;
uavcan::int32_t utc_accumulated_correction_nsec = 0;
uavcan::int32_t utc_correction_nsec_per_overflow = 0;
uavcan::MonotonicTime prev_utc_adj_at;

uavcan::uint64_t time_mono = 0;
uavcan::uint64_t time_utc = 0;
uavcan::uint32_t gptmr_ticks_per_usec = 1;

}


void init()
{
	CriticalSectionLocker lock;

	if (initialized) {
		return;
	}

	initialized = true;


# if UAVCAN_HPMICRO_NUTTX

	uint32_t gptmr_freq;
	gptmr_channel_config_t config;
	// Attach IRQ
	irq_attach(TIMX_IRQn, &TIMX_IRQHandler, NULL);

	clock_add_to_group(TIMX_CLOCK, 0);

	gptmr_freq = clock_get_frequency(TIMX_CLOCK);
	gptmr_ticks_per_usec = gptmr_freq / 1000000;
	gptmr_channel_get_default_config(TIMX, &config);

	// Start the timer
	// reload = ticks per USecPerOverflow(65536us), matches USecPerOverflow constant
	config.reload = gptmr_ticks_per_usec * USecPerOverflow;
	gptmr_channel_config(TIMX, 0, &config, false);
	gptmr_start_counter(TIMX, 0);


	gptmr_enable_irq(TIMX, GPTMR_CH_RLD_IRQ_MASK(0));
	up_enable_irq(TIMX_IRQn);
	UAVCAN_ASSERT(TIMX->IRQEN & GPTMR_CH_RLD_IRQ_MASK(0));

# endif
}

void setUtc(uavcan::UtcTime time)
{
	MutexLocker mlocker(mutex);
	UAVCAN_ASSERT(initialized);

	{
		CriticalSectionLocker locker;
		time_utc = time.toUSec();
	}

	utc_set = true;
	utc_locked = false;
	utc_jump_cnt++;
	utc_prev_adj = 0;
	utc_rel_rate_ppm = 0;
}

static uavcan::uint64_t sampleUtcFromCriticalSection()
{
# if UAVCAN_HPMICRO_NUTTX

	UAVCAN_ASSERT(initialized);
	UAVCAN_ASSERT(TIMX->IRQEN & GPTMR_CH_RLD_IRQ_MASK(0));  // check IRQ enabled

	volatile uavcan::uint64_t time = time_utc;
	volatile uavcan::uint32_t raw_cnt = gptmr_channel_get_counter(TIMX, 0, gptmr_counter_type_normal);
	volatile uavcan::uint32_t cnt = raw_cnt / gptmr_ticks_per_usec;  // convert ticks to us

	if (gptmr_check_status(TIMX, GPTMR_CH_RLD_STAT_MASK(0))) {
		raw_cnt = gptmr_channel_get_counter(TIMX, 0, gptmr_counter_type_normal);
		cnt = raw_cnt / gptmr_ticks_per_usec;
		const uavcan::int32_t add = uavcan::int32_t(USecPerOverflow) +
					    (utc_accumulated_correction_nsec + utc_correction_nsec_per_overflow) / 1000;
		time = uavcan::uint64_t(uavcan::int64_t(time) + add);
	}

	return time + cnt;
# endif
}

uavcan::uint64_t getUtcUSecFromCanInterrupt()
{
	return utc_set ? sampleUtcFromCriticalSection() : 0;
}

uavcan::MonotonicTime getMonotonic()
{
	uavcan::uint64_t usec = 0;
	// Scope Critical section
	{
		CriticalSectionLocker locker;

		volatile uavcan::uint64_t time = time_mono;

# if UAVCAN_HPMICRO_NUTTX

		volatile uavcan::uint32_t cnt = gptmr_channel_get_counter(TIMX, 0, gptmr_counter_type_normal) / gptmr_ticks_per_usec;  // convert ticks to us

		if (gptmr_check_status(TIMX, GPTMR_CH_RLD_STAT_MASK(0))) {
			cnt = gptmr_channel_get_counter(TIMX, 0, gptmr_counter_type_normal) / gptmr_ticks_per_usec;
# endif
			time += USecPerOverflow;
		}

		usec = time + cnt;

# ifndef NDEBUG
		static uavcan::uint64_t prev_usec = 0;      // Self-test
		UAVCAN_ASSERT(prev_usec <= usec);
		(void)prev_usec;
		prev_usec = usec;
# endif
	} // End Scope Critical section

	return uavcan::MonotonicTime::fromUSec(usec);
}

uavcan::UtcTime getUtc()
{
	if (utc_set) {
		uavcan::uint64_t usec = 0;
		{
			CriticalSectionLocker locker;
			usec = sampleUtcFromCriticalSection();
		}
		return uavcan::UtcTime::fromUSec(usec);
	}

	return uavcan::UtcTime();
}

static float lowpass(float xold, float xnew, float corner, float dt)
{
	const float tau = 1.F / corner;
	return (dt * xnew + tau * xold) / (dt + tau);
}

static void updateRatePID(uavcan::UtcDuration adjustment)
{
	const uavcan::MonotonicTime ts = getMonotonic();
	const float dt = float((ts - prev_utc_adj_at).toUSec()) / 1e6F;
	prev_utc_adj_at = ts;
	const float adj_usec = float(adjustment.toUSec());

	/*
	 * Target relative rate in PPM
	 * Positive to go faster
	 */
	const float target_rel_rate_ppm = adj_usec * utc_sync_params.offset_p;

	/*
	 * Current relative rate in PPM
	 * Positive if the local clock is faster
	 */
	const float new_rel_rate_ppm = (utc_prev_adj - adj_usec) / dt; // rate error in [usec/sec], which is PPM
	utc_prev_adj = adj_usec;
	utc_rel_rate_ppm = lowpass(utc_rel_rate_ppm, new_rel_rate_ppm, utc_sync_params.rate_error_corner_freq, dt);

	const float rel_rate_error = target_rel_rate_ppm - utc_rel_rate_ppm;

	if (dt > 10) {
		utc_rel_rate_error_integral = 0;

	} else {
		utc_rel_rate_error_integral += rel_rate_error * dt * utc_sync_params.rate_i;
		utc_rel_rate_error_integral =
			uavcan::max(utc_rel_rate_error_integral, -utc_sync_params.max_rate_correction_ppm);
		utc_rel_rate_error_integral =
			uavcan::min(utc_rel_rate_error_integral, utc_sync_params.max_rate_correction_ppm);
	}

	/*
	 * Rate controller
	 */
	float total_rate_correction_ppm = rel_rate_error + utc_rel_rate_error_integral;
	total_rate_correction_ppm = uavcan::max(total_rate_correction_ppm, -utc_sync_params.max_rate_correction_ppm);
	total_rate_correction_ppm = uavcan::min(total_rate_correction_ppm, utc_sync_params.max_rate_correction_ppm);

	utc_correction_nsec_per_overflow = uavcan::int32_t((USecPerOverflow * 1000) * (total_rate_correction_ppm / 1e6F));

//    syslog("$ adj=%f   rel_rate=%f   rel_rate_eint=%f   tgt_rel_rate=%f   ppm=%f\n",
//              adjUAVCAN_STM32H7_NUTTX_usec, utc_rel_rate_ppm, utc_rel_rate_error_integral, target_rel_rate_ppm,
// total_rate_correction_ppm);
}

void adjustUtc(uavcan::UtcDuration adjustment)
{
	MutexLocker mlocker(mutex);
	UAVCAN_ASSERT(initialized);

	if (adjustment.getAbs() > utc_sync_params.min_jump || !utc_set) {
		const uavcan::int64_t adj_usec = adjustment.toUSec();

		{
			CriticalSectionLocker locker;

			if ((adj_usec < 0) && uavcan::uint64_t(-adj_usec) > time_utc) {
				time_utc = 1;

			} else {
				time_utc = uavcan::uint64_t(uavcan::int64_t(time_utc) + adj_usec);
			}
		}

		utc_set = true;
		utc_locked = false;
		utc_jump_cnt++;
		utc_prev_adj = 0;
		utc_rel_rate_ppm = 0;

	} else {
		updateRatePID(adjustment);

		if (!utc_locked) {
			utc_locked =
				(std::abs(utc_rel_rate_ppm) < utc_sync_params.lock_thres_rate_ppm) &&
				(std::abs(utc_prev_adj) < float(utc_sync_params.lock_thres_offset.toUSec()));
		}
	}
}

float getUtcRateCorrectionPPM()
{
	MutexLocker mlocker(mutex);
	const float rate_correction_mult = float(utc_correction_nsec_per_overflow) / float(USecPerOverflow * 1000);
	return 1e6F * rate_correction_mult;
}

uavcan::uint32_t getUtcJumpCount()
{
	MutexLocker mlocker(mutex);
	return utc_jump_cnt;
}

bool isUtcLocked()
{
	MutexLocker mlocker(mutex);
	return utc_locked;
}

UtcSyncParams getUtcSyncParams()
{
	MutexLocker mlocker(mutex);
	return utc_sync_params;
}

void setUtcSyncParams(const UtcSyncParams &params)
{
	MutexLocker mlocker(mutex);
	// Add some sanity check
	utc_sync_params = params;
}

} // namespace clock

SystemClock &SystemClock::instance()
{
	static union SystemClockStorage {
		uavcan::uint8_t buffer[sizeof(SystemClock)];
		long long _aligner_1;
		long double _aligner_2;
	} storage;

	SystemClock *const ptr = reinterpret_cast<SystemClock *>(storage.buffer);

	if (!clock::initialized) {
		MutexLocker mlocker(clock::mutex);
		clock::init();
		new (ptr)SystemClock();
	}

	return *ptr;
}

} // namespace uavcan_hpmicro_can


/**
 * Timer interrupt handler
 */

extern "C"
UAVCAN_HPMICRO_IRQ_HANDLER(TIMX_IRQHandler)
{
	UAVCAN_HPMICRO_IRQ_PROLOGUE();
	using namespace uavcan_hpmicro_can::clock;
	UAVCAN_ASSERT(initialized);
# if UAVCAN_HPMICRO_NUTTX
	gptmr_clear_status(TIMX, GPTMR_CH_RLD_STAT_MASK(0));
# endif
	time_mono += USecPerOverflow;

	if (utc_set) {
		time_utc += USecPerOverflow;
		utc_accumulated_correction_nsec += utc_correction_nsec_per_overflow;

		if (std::abs(utc_accumulated_correction_nsec) >= 1000) {
			time_utc = uavcan::uint64_t(uavcan::int64_t(time_utc) + utc_accumulated_correction_nsec / 1000);
			utc_accumulated_correction_nsec %= 1000;
		}

		// Correction decay - 1 nsec per 65536 usec
		if (utc_correction_nsec_per_overflow > 0) {
			utc_correction_nsec_per_overflow--;

		} else if (utc_correction_nsec_per_overflow < 0) {
			utc_correction_nsec_per_overflow++;

		} else {
			; // Zero
		}
	}

	UAVCAN_HPMICRO_IRQ_EPILOGUE();
}

#endif
