/****************************************************************************
 *
 * Copyright (C) 2024 PX4 Development Team. All rights reserved.
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

#include <px4_platform_common/px4_config.h>
#include <px4_platform_common/micro_hal.h>
#include <px4_platform_common/log.h>
#include <px4_platform_common/time.h>
#include <px4_arch/io_timer.h>
#include <drivers/drv_dshot.h>
#include <drivers/drv_hrt.h>
#include <stdio.h>
#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/clock.h>
#include <nuttx/semaphore.h>

#include <arch/board/board.h>
#include "hpm_pwm_drv.h"
#include "hpm_trgm_drv.h"
#include "hpm_clock_drv.h"
#include "hpm_gptmr_drv.h"
#include "hpm_dma_drv.h"
#include "hpm_dmamux_drv.h"
#include "hpm_dma_mgr.h"
#include "hpm_soc.h"
#include "hpm_misc.h"
#include "dshot.h"
#include "hpm_gpiom_drv.h"

/*
 * DShot DMA glue: looks up per-PWM TRGM DMA source base and DMAMUX MOT
 * slot base from dshot_pwm_dma_map (defined board-side in board_config.h).
 * No hardcoded PWM data here.
 */

static const struct dshot_pwm_dma_t *dshot_find_pwm(PWM_Type *pwm)
{
	for (unsigned i = 0; i < dshot_pwm_dma_map_count; i++) {
		if (dshot_pwm_dma_map[i].pwm == pwm) {
			return &dshot_pwm_dma_map[i];
		}
	}
	return NULL;
}

uint8_t dshot_get_trgm_dma_src(PWM_Type *pwm, uint8_t sub_ch)
{
	if (sub_ch >= 4u) {
		return 0xFF;
	}

	const struct dshot_pwm_dma_t *cfg = dshot_find_pwm(pwm);
	if (cfg == NULL) {
		return 0xFF;
	}

	return (uint8_t)(cfg->trgm_src_base + sub_ch);
}

uint8_t dshot_get_mot_dma_src_base(PWM_Type *pwm)
{
	const struct dshot_pwm_dma_t *cfg = dshot_find_pwm(pwm);
	if (cfg == NULL) {
		return 0xFF;
	}

	return cfg->mot_src_base;
}

#define DSHOT_FRAME_BITS            16u
#define DSHOT_THROTTLE_POSITION     5u
#define DSHOT_TELEMETRY_POSITION    4u
#define NIBBLES_SIZE                4u
#define DSHOT_NUMBER_OF_NIBBLES     3u
/* One leading guard and two trailing guards. DMA writes CMP shadow registers:
 * at the final transfer, the first trailing guard must already be active. */
#define DSHOT_BUFFER_SIZE           (DSHOT_FRAME_BITS + 3u)

#define DSHOT_MAX_CHANNELS          MAX_TIMER_IO_CHANNELS
#define DSHOT_MAX_CH_PER_TIMER      4u

/* bidirectional DShot telemetry capture (oversampling, PWMV1 + DMAV1) */
#define DSHOT_TELEMETRY_BITS        21u  /* eRPM frame: 1 start bit + 20 GCR bits */
#define DSHOT_TELEMETRY_GCR_NIBBLES 4u
#define DSHOT_BD_OVERSAMPLE_RATIO   6u   /* 6 samples per reply bit */
#define DSHOT_MAX_START_CANDIDATES  8u   /* frame-start candidates per reply */
#define DSHOT_BD_SAMPLE_WINDOW_US   80u  /* deadtime + reply + margin (default only; dshot_freq_duty_ctx overrides per speed) */
#define DSHOT_BD_RX_BUF_SIZE        1024u /* one 32-bit GPIO read per sample, max for DShot150 window */
#define DSHOT_BD_DEADTIME_US        35u  /* 30us direction switch + 5us margin */

typedef enum {
	DSHOT_DECODE_OK = 0,
	DSHOT_DECODE_GCR,
	DSHOT_DECODE_CRC,
	DSHOT_DECODE_SHORT,
	DSHOT_DECODE_GAP,
} dshot_decode_result_t;

static const uint8_t s_sub_ch_cmp_start[DSHOT_MAX_CH_PER_TIMER] = {0, 2, 4, 6};
static const uint8_t s_sub_ch_cmp_trig[DSHOT_MAX_CH_PER_TIMER]  = {9, 10, 11, 12};
#define DSHOT_CMP_SHADOW_MATCH  8u

typedef struct {
    uint32_t pwm_freq_KHz;
    uint32_t t0h_ns;
    uint32_t t1h_ns;
    /* sample window for bdshot capture. The sampler starts together with the
     * PWM counter in up_dshot_trigger() (deterministic, independent of DMA-ISR
     * latency), so the window has to hold the full outgoing 16-bit frame plus
     * the ESC dead-zone plus the 21-bit eRPM reply:
     *   t_tx   = 16 * (t0h + t1h) / 2
     *   t_rep  = 21 / (dshot * 5/4)        (GCR @ 5/4 of base bitrate)
     *   t_gap  = ESC dead-zone: ~34us measured at throttle 0, but the ESC
     *            aligns telemetry to commutation gaps when the motor spins
     *            and the observed reply start drifts out to ~75us -- budget
     *            100us for it.
     * e.g. DShot150:  t_tx=107us, t_rep=112us -> ~330us window
     *      DShot300:  t_tx= 53us, t_rep= 56us -> ~220us
     *      DShot600:  t_tx= 27us, t_rep= 28us -> ~170us (772 samples)
     *      DShot1200: t_tx= 13us, t_rep= 14us -> ~110us (1000 samples, the
     *                 1024-entry buffer caps the dead-zone budget here)
     * During TX the GPIO DI reads the stuck PWM-owned level (constant), which
     * just extends the leading idle run the frame-start detector looks for.
     */
    uint32_t sample_window_us;
} dshot_freq_duty_list;

static dshot_freq_duty_list dshot_freq_duty_ctx[4] = {
    {150,  2500, 5000,  330u},   /* DShot150  */
    {300,  1250, 2500,  220u},   /* DShot300  */
    {600,   625, 1250,  170u},   /* DShot600  */
    {1200,  313,  625,   110u},  /* DShot1200 */
};

typedef struct {
	volatile uint32_t *oe_clear;
	uint32_t           oe_mask;
	volatile uint32_t *func_ctl;
	volatile uint32_t *gpiom_assign;
} bdshot_pin_release_t;

static inline void dshot_gpio_split(uint32_t pinset, GPIO_Type **out_gpio, uint8_t *out_port)
{
	GPIO_Type *gpio_ptr = HPM_GPIO0;
	*out_port = (uint8_t)((pinset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT);

	switch (pinset & GPIO_CONTROLLER_MASK) {
#ifdef HPM_GPIO1
	case GPIO1:
		gpio_ptr = HPM_GPIO1;
		break;
#endif
#ifdef HPM_FGPIO
	case FGPIO:
		gpio_ptr = HPM_FGPIO;
		break;
#endif
	default:
		break;
	}

	*out_gpio = gpio_ptr;
}

typedef struct {
	bool     initialized;
	uint16_t frame_data;
	bool     tx_pending;

	int32_t  erpm;
	bool     erpm_ready;      /* a decode attempt completed this cycle */
	bool     erpm_valid;      /* the attempt produced a valid eRPM reply */
	hrt_abstime last_ok_us;   /* last successful reply decode (0 = never), for online detection */
} dshot_channel_t;

typedef struct {
	PWM_Type       *pwm;
	TRGM_Type      *trgm;
	uint32_t        trgm_synci_out;
	uint32_t        clk_freq;
	uint32_t        reload;
	uint32_t        t0h;
	uint32_t        t1h;
	bool            configured;

	uint8_t         num_sub_ch;
	uint32_t        px4_ch_mask;
	uint8_t         px4_ch_ofs[DSHOT_MAX_CH_PER_TIMER];

	dma_resource_t       dma_res[DSHOT_MAX_CH_PER_TIMER];
	dma_channel_config_t dma_cfg[DSHOT_MAX_CH_PER_TIMER];
	uint8_t              dma_alloc_mask;
	uint32_t             dma_req_mask;
	bool                 sem_initialized;

	sem_t            dma_sem;
	volatile uint8_t active_sub_mask;
	volatile uint8_t dma_done_mask;

	bool            timer_claimed;

	/* bidirectional DShot telemetry capture (oversampling, PWMV1 + DMAV1) */
	bool            bdshot_enabled;
	volatile bool   rx_active;
	uint32_t        dshot_freq;
	volatile hrt_abstime dir_switch_time;
	volatile hrt_abstime tx_start_time;      /* set at PWM counter start (Phase 2 crit section) */

	/* deferred decode state: Phase 0 stops the capture and records where it
	 * landed; the decode runs right after, while the sampler is idle */
	volatile uint8_t  rx_buf_idx;        /* buffer the in-flight frame captures into */
	uint8_t         rx_decode_buf;       /* buffer holding the capture awaiting decode */
	uint32_t        rx_decode_count;     /* valid samples in rx_decode_buf */
	bool            rx_decode_pending;

	/* sample clock + sample DMA + sample buffer geometry */
	GPTMR_Type     *sample_gptmr;
	uint8_t         sample_gptmr_ch;
	uint8_t         sample_dmamux_src;
	uint32_t        bd_sample_ticks;         /* sample period in GPTMR ticks (SDK writes reload-1) */
	uint32_t        bd_samples_per_bit;      /* integer sppb */
	uint32_t        bd_samples_per_bit_q8;   /* real sppb x256 (e.g. 6.06 -> 1551) */
	uint32_t        bd_rx_count;             /* samples per frame */

	dma_resource_t       sample_dma;          /* ONE sample DMA shared by all motors on this timer */
	dma_channel_config_t sample_cfg;

	/* which GPIO controller + port the timer module's motors live on */
	GPIO_Type      *rx_gpio;
	uint8_t         rx_port;
	uint8_t         rx_pin_bit[DSHOT_MAX_CH_PER_TIMER];

	/* precomputed direction-switch register images (ISR timing path) */
	bdshot_pin_release_t release[DSHOT_MAX_CH_PER_TIMER];

	/* pre-computed DMA register images (one per sample arm) */
	uint32_t sample_src_addr;
	uint32_t sample_dst_addr;
	uint32_t sample_transize;
	uint32_t sample_ctrl;
	uint32_t sample_intstatus_clr;
} dshot_timer_t;

static dshot_channel_t dshot_channels[DSHOT_MAX_CHANNELS];

/* Must live in .noncacheable.bss (not the legacy .noncacheable section):
 * up_dshot_init() calls dshot_timer_deinit() before its own memset, so on a
 * warm reset (reflash/reboot without power cycle) a retained
 * sem_initialized/dma_sem would make nxsem_destroy() walk stale holder
 * pointers and fault.  .noncacheable.bss is zeroed by the startup code. */
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(8) dshot_timer_t   dshot_timers[MAX_IO_TIMERS];

static uint32_t s_dma_buf[MAX_IO_TIMERS][DSHOT_MAX_CH_PER_TIMER][DSHOT_BUFFER_SIZE]
	__attribute__((section(".ahb_sram"), aligned(8)));

static uint32_t s_rx_sample_buf[MAX_IO_TIMERS][2][DSHOT_BD_RX_BUF_SIZE]
	__attribute__((section(".ahb_sram"), aligned(8)));


static bool s_bdshot_enabled;

static void dshot_timer_deinit(uint8_t timer_index);
static int  dshot_timer_init(uint8_t timer_index, unsigned dshot_freq);

static uint8_t nibbles_from_mapped(uint8_t mapped)
{
	switch (mapped) {
	case 0x19: return 0x00;
	case 0x1B: return 0x01;
	case 0x12: return 0x02;
	case 0x13: return 0x03;
	case 0x1D: return 0x04;
	case 0x15: return 0x05;
	case 0x16: return 0x06;
	case 0x17: return 0x07;
	case 0x1A: return 0x08;
	case 0x09: return 0x09;
	case 0x0A: return 0x0A;
	case 0x0B: return 0x0B;
	case 0x1E: return 0x0C;
	case 0x0D: return 0x0D;
	case 0x0E: return 0x0E;
	case 0x0F: return 0x0F;
	default:   return 0xFF;
	}
}

static uint32_t bdshot_gcr_to_period(uint32_t v, bool *gcr_valid)
{
	if (gcr_valid) {
		*gcr_valid = false;
	}

	uint32_t gcr = v ^ (v >> 1);
	uint32_t data = 0;

	for (uint8_t i = 0; i < DSHOT_TELEMETRY_GCR_NIBBLES; i++) {
		uint8_t nibble = nibbles_from_mapped((uint8_t)(gcr & 0x1Fu));

		if (nibble == 0xFFu) {
			return 0;
		}

		data |= (uint32_t)nibble << (4 * i);
		gcr >>= 5;
	}

	/* all GCR nibbles decoded cleanly: failure from here on is value-level */
	if (gcr_valid) {
		*gcr_valid = true;
	}

	uint32_t crc      = data & 0xFu;
	uint32_t payload  = (data & 0xFFF0u) >> 4;
	uint32_t calc_crc = (~(payload ^ (payload >> 4) ^ (payload >> 8))) & 0x0Fu;

	if (crc != calc_crc) {
		return 0;
	}

	uint32_t shift = (data & 0xE000u) >> 13;
	return ((data & 0x1FF0u) >> 4) << shift;
}

static void compute_bd_rx_params(uint32_t clk_freq, unsigned dshot_freq,
				 uint32_t sample_window_us,
				 uint32_t *out_sample_ticks,
				 uint32_t *out_sample_count)
{
	uint32_t reply_freq  = ((uint32_t)dshot_freq * 5u) / 4u;
	uint32_t sample_freq = (uint32_t)DSHOT_BD_OVERSAMPLE_RATIO * reply_freq;
	uint32_t ticks = clk_freq / sample_freq;

	if (ticks < 4u) {
		ticks = 4u;
	}

	uint32_t sample_period_ns = (uint64_t)(ticks) * 1000000000ULL / clk_freq;
	uint64_t count_64 = (uint64_t)sample_window_us * 1000ULL / sample_period_ns;

	if (count_64 < 32u) {
		count_64 = 32u;
	}

	if (count_64 > DSHOT_BD_RX_BUF_SIZE) {
		count_64 = DSHOT_BD_RX_BUF_SIZE;
	}

	/* gptmr_channel_config() writes reload - 1 to the RLD register, so pass
	 * the full period here to get exactly `ticks` clocks per sample. */
	*out_sample_ticks = ticks;
	*out_sample_count = (uint32_t)count_64;
}

static uint32_t decode_telemetry_packet(const uint32_t *edge_time,
					uint16_t edge_count, uint32_t samples_per_bit_q8,
					uint32_t sample_count, uint8_t *fail_reason)
{
	if (fail_reason) {
		*fail_reason = DSHOT_DECODE_OK;
	}

	if (edge_count < 3 || samples_per_bit_q8 == 0) {
		if (fail_reason) {
			*fail_reason = DSHOT_DECODE_SHORT;
		}

		return 0;
	}

	/* A complete frame occupies 21 bit periods. If the capture ends before
	 * that, padding the missing bits would fabricate data: reject it. */
	uint32_t frame_samples = (uint32_t)(((uint64_t)DSHOT_TELEMETRY_BITS * samples_per_bit_q8) >> 8);

	/* Allow one sample of rounding slack at the end of the capture. */
	if (frame_samples == 0u || (uint64_t)sample_count + 1u < (uint64_t)edge_time[0] + frame_samples) {
		if (fail_reason) {
			*fail_reason = DSHOT_DECODE_SHORT;
		}

		return 0;
	}

	uint64_t value = 0;
	uint32_t shifted = 0;
	uint32_t prev = edge_time[0];
	uint8_t  cur = 1;

	/* Decode exactly one frame starting at edge_time[0] (the frame start
	 * located by bdshot_extract_all()). Do not scan shifted windows: a 4-bit
	 * CRC is far too weak to select a frame start, and a shifted window can
	 * decode garbage (e.g. 64128us) from a frame whose real CRC failed. */
	for (uint16_t i = 1; i < edge_count; i++) {
		if (shifted >= DSHOT_TELEMETRY_BITS) {
			break;
		}

		uint32_t delta = edge_time[i] - prev;
		/* delta samples / sppb in Q8: bits = (delta<<8) / sppb_q8 */
		uint32_t bits = ((delta << 8) + samples_per_bit_q8 / 2u) / samples_per_bit_q8;

		/* The reply is followed by idle. If a run crosses the 21-bit frame
		 * end, it is the trailing idle (possibly ended by a post-frame
		 * edge). Keep only the bits up to the frame end and stop; do not
		 * reject it as a GAP. */
		if (shifted + bits > DSHOT_TELEMETRY_BITS) {
			uint32_t need = DSHOT_TELEMETRY_BITS - shifted;

			for (uint32_t b = 0; b < need; b++) {
				value = (value << 1) | cur;
			}

			shifted = DSHOT_TELEMETRY_BITS;
			break;
		}

		if (bits > 8) {
			/* GCR data runs are at most 8 bit periods. A longer run cannot
			 * be part of the 21-bit frame. The start has already been
			 * validated, so do not re-anchor inside the capture: reject. */
			if (fail_reason) {
				*fail_reason = DSHOT_DECODE_GAP;
			}

			return 0;
		}

		/* sub-0.5-bit interval: sampling jitter/glitches are 1-2 samples
		 * (<0.35 bit at 6x oversampling), while a genuine 1-bit run can
		 * jitter down to ~4 samples (~0.66 bit). The old 0.8-bit threshold
		 * dropped legitimate jittered 1-bit runs and corrupted the frame. */
		if (((uint64_t)(delta << 8)) * 2u < (uint64_t)samples_per_bit_q8) {
			/* sampling jitter near an edge or direction-switch bounce.
				* Ignore (keep prev). */
			continue;
		}

		for (uint32_t b = 0; b < bits; b++) {
			value = (value << 1) | cur;
		}

		shifted += bits;
		cur ^= 1;
		prev = edge_time[i];
	}

	/* Do not reject a valid frame because the idle line after it glitched:
	 * once GCR+CRC validate, only the 21-bit reply window matters. */

	/* The final run has no closing edge: it carries the last bit(s) and the
	 * idle level, so extend it with the current level until we have 21 bits. */
	while (shifted < DSHOT_TELEMETRY_BITS) {
		value = (value << 1) | cur;
		shifted++;
	}

	/* Only the window aligned with the frame start is valid. If a run crossed
	 * the frame boundary, keep its first 21 bits and ignore the idle tail. */
	uint32_t v = (uint32_t)((value >> (shifted - DSHOT_TELEMETRY_BITS)) & 0x1FFFFFu);
	bool gcr_ok = false;
	uint32_t period = bdshot_gcr_to_period(v, &gcr_ok);

	if (!gcr_ok) {
		if (fail_reason) {
			*fail_reason = DSHOT_DECODE_GCR;
		}

	} else if (period == 0u) {
		if (fail_reason) {
			*fail_reason = DSHOT_DECODE_CRC;
		}
	}

	return period;
}


typedef struct {
	uint8_t  start_count;
	uint16_t edge_cnt;
	uint32_t starts[DSHOT_MAX_START_CANDIDATES];
	uint32_t edges[64];
} bdshot_extracted_t;

static void bdshot_extract_all(dshot_timer_t *t, const uint32_t *samples, uint32_t count,
			       bdshot_extracted_t out[])
{
	typedef struct {
		uint32_t mask;
		uint32_t cand_idx;
		uint32_t run;
		uint32_t low_run, low_start;
		bool     record_edges;
		uint8_t  lvl;
		uint8_t  cand_lvl;
		uint8_t  cand_age;
		uint8_t  cand_pending;
	} ch_state_t;

	ch_state_t ch[DSHOT_MAX_CH_PER_TIMER];

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		ch[sc].mask         = (uint32_t)1u << t->rx_pin_bit[sc];
		ch[sc].lvl          = (samples[0] & ch[sc].mask) ? 1u : 0u;
		ch[sc].cand_idx     = 0;
		ch[sc].cand_lvl     = 0;
		ch[sc].cand_age     = 0;
		ch[sc].cand_pending = 0;
		ch[sc].run          = 0;
		ch[sc].record_edges = false;
		ch[sc].low_run      = 0;
		ch[sc].low_start    = 0;
		out[sc].start_count = 0;
		out[sc].edge_cnt    = 0;
	}

	/* A GCR data run is at most 8 bits (including code boundaries). Even if
	 * the TX phase itself is not seen as a continuous high level, the ESC
	 * dead-zone after the direction switch is ~30us, i.e. >=9 bit periods at
	 * DShot300. Requiring 9 bits rejects data runs as frame-start candidates
	 * without depending on the TX phase being high. */
	uint32_t min_high = 9u * t->bd_samples_per_bit;

	for (uint32_t i = 1; i < count; i++) {
		uint32_t w = samples[i];

		for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
			ch_state_t *c = &ch[sc];
			uint8_t b = (w & c->mask) ? 1u : 0u;

			if (b) {
				c->run++;
				c->low_run = 0;

				if (c->run >= min_high) {
					c->record_edges = true;
				}

			} else {
				/* Match the 4-sample edge filter below: a low pulse
				 * shorter than that is a glitch and must not end the
				 * high run (otherwise a candidate points at a glitch
				 * that has no corresponding edge). */
				if (c->low_run < 4u) {
					if (c->low_run == 0u) {
						c->low_start = i;
					}

					c->low_run++;

					if (c->low_run == 4u) {
						/* Ends of long high runs are start candidates. */
						if (c->run >= min_high
						    && out[sc].start_count < DSHOT_MAX_START_CANDIDATES) {
							out[sc].starts[out[sc].start_count++] = c->low_start;
						}

						c->run = 0;
					}
				}
			}

			if (c->cand_pending) {
				if (b == c->cand_lvl) {
					if (++c->cand_age >= 3u) {
						if (c->record_edges
						    && out[sc].edge_cnt < (uint16_t)(sizeof(out[sc].edges) / sizeof(out[sc].edges[0]))) {
							out[sc].edges[out[sc].edge_cnt++] = c->cand_idx;
						}

						c->lvl = c->cand_lvl;
						c->cand_pending = 0;
					}

				} else {
					c->cand_pending = 0;
				}
			}

			if (!c->cand_pending && b != c->lvl) {
				c->cand_idx     = i;
				c->cand_lvl     = b;
				c->cand_age     = 0;
				c->cand_pending = 1;
			}
		}
	}

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		ch_state_t *c = &ch[sc];

		/* A run that reaches the end of the capture was never terminated and
		 * is not a frame-start candidate (it is the trailing idle). */
		if (c->record_edges && c->cand_pending
		    && out[sc].edge_cnt < (uint16_t)(sizeof(out[sc].edges) / sizeof(out[sc].edges[0]))) {
			out[sc].edges[out[sc].edge_cnt++] = c->cand_idx;
		}
	}
}


static uint32_t decode_candidate_start(dshot_timer_t *t,
				       const uint32_t *edges, uint16_t edge_cnt,
				       uint32_t start, uint32_t sample_count,
				       hrt_abstime retry_deadline,
				       uint8_t *fail_reason,
				       bool *start_reject, bool *short_reject,
				       bool *width_reject)
{
	*start_reject = false;
	*short_reject = false;
	*width_reject = false;

	uint16_t kept = 0;

	while (kept < edge_cnt && edges[kept] < start) {
		kept++;
	}

	if (kept >= edge_cnt || edges[kept] != start) {
		*start_reject = true;
		return 0;
	}

	if (edge_cnt - kept < 3 || sample_count - (start - 1u) < 4u) {
		*short_reject = true;
		return 0;
	}

	/* The start bit is a full reply bit. In the 500 MS/s LA capture, valid
	 * start low runs are >=1.35us, while real pre-frame glitches are
	 * <0.5us. Reject a first low run shorter than 0.75 bit; this can only
	 * drop a candidate, it cannot fabricate an eRPM. */
	uint32_t start_low = edges[kept + 1] - edges[kept];

	if ((uint64_t)start_low * 12u * 256u
	    < (uint64_t)t->bd_samples_per_bit_q8 * 9u) {
		*width_reject = true;
		return 0;
	}

	const uint32_t *e = edges + kept;
	uint16_t ec = (uint16_t)(edge_cnt - kept);

	uint32_t period = decode_telemetry_packet(e, ec, t->bd_samples_per_bit_q8,
						  sample_count, fail_reason);

	/* Only retry with a slightly different bit timebase. Moving the edge
	 * start would allow a CRC failure to become a false positive. */
	if (period == 0u && hrt_absolute_time() < retry_deadline) {
		uint32_t sppb_up   = t->bd_samples_per_bit_q8 + (t->bd_samples_per_bit_q8 >> 8);
		uint32_t sppb_down = t->bd_samples_per_bit_q8 - (t->bd_samples_per_bit_q8 >> 8);
		period = decode_telemetry_packet(e, ec, sppb_up,
						 sample_count, fail_reason);

		if (period == 0u && hrt_absolute_time() < retry_deadline) {
			period = decode_telemetry_packet(e, ec, sppb_down,
							 sample_count, fail_reason);
		}
	}

	return period;
}

static void bdshot_decode_oversample(dshot_timer_t *t, const uint32_t *samples,
				     uint32_t sample_count)
{
	bdshot_extracted_t ext[DSHOT_MAX_CH_PER_TIMER];

	if (sample_count >= 4) {
		bdshot_extract_all(t, samples, sample_count, ext);
	}

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		uint8_t px4_ch = t->px4_ch_ofs[sc];
		dshot_channels[px4_ch].erpm_valid = false;

		if (sample_count < 4) {
			dshot_channels[px4_ch].erpm = 0;
			dshot_channels[px4_ch].erpm_ready = true;
			continue;
		}

		const uint32_t *edges = ext[sc].edges;
		uint16_t edge_cnt = ext[sc].edge_cnt;
		uint8_t start_count = ext[sc].start_count;

		if (start_count == 0u) {
			dshot_channels[px4_ch].erpm = 0;
			dshot_channels[px4_ch].erpm_ready = true;
			continue;
		}

		hrt_abstime retry_deadline = hrt_absolute_time() + 120u;

		uint32_t period = 0;
		bool fallback_used = false;

		for (uint8_t ci = 0; ci < start_count; ci++) {
			uint32_t cand = ext[sc].starts[ci];

			bool start_reject = false;
			bool short_reject = false;
			bool width_reject = false;
			uint8_t reason = DSHOT_DECODE_OK;
			uint32_t got = decode_candidate_start(t, edges, edge_cnt, cand,
							      sample_count, retry_deadline,
							      &reason,
							      &start_reject, &short_reject,
							      &width_reject);

			if (got != 0u) {
				period = got;
				break;
			}

			if (start_reject || short_reject || width_reject) {
				/* No committed edge at this start, too little capture
				 * left or a sub-bit start pulse: try the next candidate. */
				continue;
			}

			/* GAP means this candidate ended a high run that cannot be
			 * part of a GCR frame (a pre-frame glitch). The real start is
			 * the next candidate; try it. For any other decode failure, do
			 * not scan further: that would risk accepting a post-frame
			 * glitch as a new frame. */
			if (reason != DSHOT_DECODE_GAP || fallback_used) {
				break;
			}

			fallback_used = true;
		}
		if (period == 0) {
			dshot_channels[px4_ch].erpm = 0;

		} else {
			dshot_channels[px4_ch].erpm_valid = true;

			if (period == 65408u) {
				dshot_channels[px4_ch].erpm = 0;

			} else {
				dshot_channels[px4_ch].erpm = (int32_t)((1000000 * 60 / 100 + period / 2) / period);
			}

			dshot_channels[px4_ch].last_ok_us = hrt_absolute_time();
		}

		dshot_channels[px4_ch].erpm_ready = true;
	}

}

static int setup_dshot_oversample_rx(uint8_t timer_index, unsigned dshot_freq, uint32_t sample_window_us)
{
	dshot_timer_t *t = &dshot_timers[timer_index];

	if (bdshot_sample_timers[timer_index].sample_gptmr_base == 0) {
		PX4_ERR("DShot: timer %u has no sample timer in bdshot_sample_timers[]", timer_index);
		return -1;
	}

	t->sample_gptmr     = (GPTMR_Type *)bdshot_sample_timers[timer_index].sample_gptmr_base;
	t->sample_gptmr_ch  = bdshot_sample_timers[timer_index].sample_gptmr_ch;
	t->sample_dmamux_src = bdshot_sample_timers[timer_index].dmamux_src;

	GPIO_Type *ref_gpio = NULL;
	uint8_t    ref_port = 0xFFu;
	bool       ref_set  = false;

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		uint8_t px4_ch = t->px4_ch_ofs[sc];
		GPIO_Type *gpio_ptr;
		uint8_t port;

		dshot_gpio_split(timer_io_channels[px4_ch].gpio_out, &gpio_ptr, &port);

		if (!ref_set) {
			ref_gpio = gpio_ptr;
			ref_port = port;
			ref_set  = true;

		} else if (gpio_ptr != ref_gpio || port != ref_port) {
			PX4_ERR("DShot BD: timer %u motor %u on different GPIO port, oversampling needs a single port",
				timer_index, px4_ch);
			return -1;
		}

		t->rx_pin_bit[sc] = (uint8_t)((timer_io_channels[px4_ch].gpio_out & GPIO_PIN_MASK) >> GPIO_PIN_SHIFT);

#ifdef IOC_PAD_PAD_CTL_HYS_SET
		/* enable input hysteresis on the pad: the bdshot reply edges can sit
		 * near the input threshold and bubble; HYS keeps the sampled bit
		 * stream clean for the 3-sample glitch filter (not all IOC IP
		 * versions have a HYS field, e.g. HPM6700 does not) */
		HPM_IOC->PAD[port * 32u + t->rx_pin_bit[sc]].PAD_CTL |= IOC_PAD_PAD_CTL_HYS_SET(1u);
#endif

		bdshot_pin_release_t *r = &t->release[sc];
		uint8_t pin = t->rx_pin_bit[sc];

		r->oe_clear     = &gpio_ptr->OE[port].CLEAR;
		r->oe_mask      = 1u << pin;
		r->func_ctl     = &HPM_IOC->PAD[port * 32u + pin].FUNC_CTL;
		r->gpiom_assign = &HPM_GPIOM->ASSIGN[port].PIN[pin];
	}

	t->rx_gpio = ref_gpio;
	t->rx_port = ref_port;

	clock_name_t sample_clock = (clock_name_t)bdshot_sample_timers[timer_index].sample_clock;
	clock_add_to_group(sample_clock, 0);
	uint32_t gptmr_clk = clock_get_frequency(sample_clock);

	if (gptmr_clk == 0u) {
		PX4_ERR("DShot: timer %u: sample clock has zero frequency", timer_index);
		return -1;
	}

	compute_bd_rx_params(gptmr_clk, dshot_freq, sample_window_us,
			     &t->bd_sample_ticks, &t->bd_rx_count);

	uint32_t reply_freq = ((uint32_t)dshot_freq * 5u) / 4u;

	/* use the actual (integer-divided) sample rate, not the nominal 6x rate */
	uint32_t actual_sample_freq = gptmr_clk / t->bd_sample_ticks;
	t->bd_samples_per_bit = (actual_sample_freq + reply_freq / 2u) / reply_freq;

	if (t->bd_samples_per_bit == 0u) {
		t->bd_samples_per_bit = 1u;
	}

	/* actual sample period is bd_sample_ticks clocks (SDK stores period - 1) */
	uint64_t sppb_q8_64 = ((uint64_t)gptmr_clk << 8)
			      / ((uint64_t)t->bd_sample_ticks * reply_freq);
	t->bd_samples_per_bit_q8 = (uint32_t)sppb_q8_64;

	if (t->bd_samples_per_bit_q8 == 0u) {
		t->bd_samples_per_bit_q8 = t->bd_samples_per_bit << 8;
	}

	/* sample clock GPTMR: free-running, reload = sample period, DMA on reload.
	 * bd_sample_ticks is the full period; the SDK applies the -1 for RLD. */
	gptmr_channel_config_t gptmr_cfg;
	gptmr_channel_get_default_config(t->sample_gptmr, &gptmr_cfg);
	gptmr_cfg.mode              = gptmr_work_mode_no_capture;
	gptmr_cfg.dma_request_event = gptmr_dma_request_on_reload;
	gptmr_cfg.reload            = t->bd_sample_ticks;
	gptmr_channel_config(t->sample_gptmr, t->sample_gptmr_ch, &gptmr_cfg, false);

	if (status_success != dma_mgr_request_specified_resource(&t->sample_dma, HPM_HDMA)) {
		PX4_ERR("DShot: timer %u: sample dma request failed", timer_index);
		return -1;
	}

	dmamux_config(HPM_DMAMUX, t->sample_dma.channel,
		      (uint8_t)t->sample_dmamux_src, true);

	DMA_Type *base = t->sample_dma.base;
	uint8_t   ch   = t->sample_dma.channel;

	dma_default_channel_config(base, &t->sample_cfg);
	t->sample_cfg.priority       = DMA_CHANNEL_PRIORITY_HIGH;
	t->sample_cfg.src_addr       = core_local_mem_to_sys_address(
					      0, (uint32_t)&t->rx_gpio->DI[t->rx_port].VALUE);
	t->sample_cfg.src_width      = DMA_TRANSFER_WIDTH_WORD;
	t->sample_cfg.src_mode       = DMA_HANDSHAKE_MODE_HANDSHAKE;
	t->sample_cfg.src_addr_ctrl  = DMA_ADDRESS_CONTROL_FIXED;
	t->sample_cfg.src_burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
	t->sample_cfg.dst_addr       = core_local_mem_to_sys_address(
					      0, (uint32_t)&s_rx_sample_buf[timer_index][0][0]);
	t->sample_cfg.dst_width      = DMA_TRANSFER_WIDTH_WORD;
	t->sample_cfg.dst_addr_ctrl  = DMA_ADDRESS_CONTROL_INCREMENT;
	t->sample_cfg.dst_mode       = DMA_HANDSHAKE_MODE_NORMAL;
	t->sample_cfg.linked_ptr     = 0;
	t->sample_cfg.size_in_byte   = t->bd_rx_count * sizeof(uint32_t);
	t->sample_cfg.interrupt_mask = DMA_INTERRUPT_MASK_TERMINAL_COUNT;

	dma_channel_config_t *cfg = &t->sample_cfg;
	t->sample_src_addr      = cfg->src_addr;
	t->sample_dst_addr      = cfg->dst_addr;
	t->sample_transize      = cfg->size_in_byte >> cfg->src_width;
	t->sample_intstatus_clr = (DMA_INTSTATUS_TC_SET(1) | DMA_INTSTATUS_ABORT_SET(1)
				   | DMA_INTSTATUS_ERROR_SET(1)) << ch;
	t->sample_ctrl          = DMA_CHCTRL_CTRL_PRIORITY_SET(cfg->priority)
				   | DMA_CHCTRL_CTRL_SRCBURSTSIZE_SET(cfg->src_burst_size)
				   | DMA_CHCTRL_CTRL_SRCWIDTH_SET(cfg->src_width)
				   | DMA_CHCTRL_CTRL_DSTWIDTH_SET(cfg->dst_width)
				   | DMA_CHCTRL_CTRL_SRCMODE_SET(cfg->src_mode)
				   | DMA_CHCTRL_CTRL_DSTMODE_SET(cfg->dst_mode)
				   | DMA_CHCTRL_CTRL_SRCADDRCTRL_SET(cfg->src_addr_ctrl)
				   | DMA_CHCTRL_CTRL_DSTADDRCTRL_SET(cfg->dst_addr_ctrl)
				   | DMA_CHCTRL_CTRL_SRCREQSEL_SET(ch)
				   | DMA_CHCTRL_CTRL_DSTREQSEL_SET(ch)
				   | cfg->interrupt_mask
				   | DMA_CHCTRL_CTRL_ENABLE_MASK;

	t->bdshot_enabled = true;
	t->rx_active      = false;

	return 0;
}

static void bdshot_arm_sample_dma(dshot_timer_t *t)
{
	irqstate_t flags = px4_enter_critical_section();

	DMA_Type *base = t->sample_dma.base;
	uint8_t   ch   = t->sample_dma.channel;

	base->CHCTRL[ch].SRCADDR  = t->sample_src_addr;
	base->CHCTRL[ch].DSTADDR  = t->sample_dst_addr
				    + (uint32_t)t->rx_buf_idx * (DSHOT_BD_RX_BUF_SIZE * sizeof(uint32_t));
	base->CHCTRL[ch].TRANSIZE = t->sample_transize;
	base->INTSTATUS           = t->sample_intstatus_clr;
	base->CHCTRL[ch].CTRL     = t->sample_ctrl;

	px4_leave_critical_section(flags);
}

static void bdshot_switch_to_input(dshot_timer_t *t)
{
	uint32_t bit_period_cycles = t->clk_freq / t->dshot_freq;
	volatile uint32_t delay = bit_period_cycles / 5u;

	while (delay-- > 0) {
	}

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		if (!(t->active_sub_mask & (1u << sc))) {
			continue;
		}

		const bdshot_pin_release_t *r = &t->release[sc];

		*r->oe_clear = r->oe_mask;
		*r->func_ctl = 0u;
		*r->gpiom_assign = (*r->gpiom_assign & ~(GPIOM_ASSIGN_PIN_SELECT_MASK))
				   | GPIOM_ASSIGN_PIN_SELECT_SET(gpiom_soc_gpio0);
	}

	t->dir_switch_time = hrt_absolute_time();
	t->rx_active       = true;
}

static void bdshot_restore_outputs(dshot_timer_t *t)
{
	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		uint8_t px4_ch = t->px4_ch_ofs[sc];
		uint32_t gpio_out = timer_io_channels[px4_ch].gpio_out;

#ifdef IOC_PAD_PAD_CTL_HYS_SET
		/* hpm_config_gpio() rewrites PAD_CTL. Preserve the RX hysteresis
		 * enabled at setup when restoring the PWM function for the next TX. */
		gpio_out |= GPIO_SMT;
#endif
		px4_arch_configgpio(gpio_out);
	}
}

static void bdshot_finish_capture_and_restore(dshot_timer_t *t)
{
	if (!t->rx_active) {
		return;
	}

	uint32_t frame_us = (DSHOT_TELEMETRY_BITS * 1000000) / (t->dshot_freq * 5u / 4u);
	hrt_abstime deadtime = (hrt_abstime)(DSHOT_BD_DEADTIME_US + frame_us);

	/* dir_switch_time is written from the TX DMA ISR; read it atomically
	 * with respect to the ISR on 32-bit targets. */
	irqstate_t read_flags = px4_enter_critical_section();
	hrt_abstime dir_switch_time = t->dir_switch_time;
	px4_leave_critical_section(read_flags);

	if ((hrt_absolute_time() - dir_switch_time) < deadtime) {
		gptmr_stop_counter(t->sample_gptmr, t->sample_gptmr_ch);
		dma_abort_channel(t->sample_dma.base, 1u << t->sample_dma.channel);

		bdshot_restore_outputs(t);

		t->rx_decode_pending = false;
		t->rx_active = false;
		return;
	}

	gptmr_stop_counter(t->sample_gptmr, t->sample_gptmr_ch);
	dma_abort_channel(t->sample_dma.base, 1u << t->sample_dma.channel);
	dma_clear_transfer_status(t->sample_dma.base, t->sample_dma.channel);

	uint8_t tidx = (uint8_t)(t - dshot_timers);
	uint32_t buf_base = core_local_mem_to_sys_address(
				0, (uint32_t)&s_rx_sample_buf[tidx][t->rx_buf_idx][0]);
	uint32_t dst_addr = t->sample_dma.base->CHCTRL[t->sample_dma.channel].DSTADDR;
	uint32_t written  = (dst_addr >= buf_base) ? (dst_addr - buf_base) / 4u : 0u;

	if (written > t->bd_rx_count) {
		written = t->bd_rx_count;
	}

	t->rx_decode_buf     = t->rx_buf_idx;
	t->rx_decode_count   = written;
	t->rx_decode_pending = true;
	t->rx_active         = false;

	bdshot_restore_outputs(t);
}

static void bdshot_decode_pending(dshot_timer_t *t)
{
	if (!t->rx_decode_pending) {
		return;
	}

	t->rx_decode_pending = false;
	bdshot_decode_oversample(t, s_rx_sample_buf[(uint8_t)(t - dshot_timers)][t->rx_decode_buf],
				 t->rx_decode_count);
}

static void dshot_dma_tc_callback(DMA_Type *ptr, uint32_t channel, void *user_data)
{
	dshot_timer_t *tmr = (dshot_timer_t *)user_data;

	if (tmr == NULL || !tmr->configured || tmr->active_sub_mask == 0) {
		return;
	}

	for (uint8_t sc = 0; sc < tmr->num_sub_ch; sc++) {
		if (!(tmr->active_sub_mask & (1u << sc))) {
			continue;
		}

		if (tmr->dma_res[sc].base != ptr || tmr->dma_res[sc].channel != channel) {
			continue;
		}

		tmr->dma_done_mask |= (1u << sc);
		break;
	}

	if (tmr->dma_done_mask != tmr->active_sub_mask) {
		return;
	}

	if (tmr->bdshot_enabled) {
		if (!tmr->rx_active) {
			bdshot_switch_to_input(tmr);
		}

	} else {
		/* The first trailing guard has already latched at reload. Stop here
		 * rather than letting scheduler latency extend the PWM burst. */
		pwm_stop_counter(tmr->pwm);
		tmr->dma_done_mask = 0;
		nxsem_post(&tmr->dma_sem);
	}
}

static void reset_pwm_counter(dshot_timer_t *t)
{
#if PWM_SOC_TIMER_RESET_SUPPORT
	pwm_timer_reset(t->pwm);
#endif
	pwm_set_start_count(t->pwm, 0, 0);

	pwm_enable_reload_at_synci(t->pwm);
	trgm_output_update_source(t->trgm, t->trgm_synci_out, 0);
	trgm_output_update_source(t->trgm, t->trgm_synci_out, 1);
	trgm_output_update_source(t->trgm, t->trgm_synci_out, 0);
	t->pwm->GCR &= ~(PWM_GCR_RLDSYNCEN_MASK | PWM_GCR_XRLDSYNCEN_MASK);
}

static uint16_t dshot_compute_frame(uint16_t throttle, bool telemetry, bool invert_crc)
{
	uint16_t packet = throttle << DSHOT_THROTTLE_POSITION;
	packet |= ((uint16_t)telemetry & 0x01u) << DSHOT_TELEMETRY_POSITION;

	uint16_t cs = packet >> NIBBLES_SIZE;
	uint16_t ck = 0;

	for (unsigned i = 0; i < DSHOT_NUMBER_OF_NIBBLES; i++) {
		ck ^= (cs & 0x0Fu);
		cs >>= NIBBLES_SIZE;
	}

	if (invert_crc) {
		ck = (~ck) & 0x0Fu;
	}

	packet |= (ck & 0x0Fu);

	return packet;
}

static void dshot_fill_dma_buffer(uint32_t *buf, uint16_t frame_data,
				  uint32_t t0h, uint32_t t1h)
{
	uint16_t packet = frame_data;

	for (uint8_t i = 0; i < DSHOT_BUFFER_SIZE; i++) {
		buf[i] = 0x0;
	}

	for (uint8_t i = 1; i <= DSHOT_FRAME_BITS; i++) {
		uint32_t raw_cmp = (packet & 0x8000) ? t1h : t0h;
		buf[i] = PWM_CMP_CMP_SET(raw_cmp);
		packet <<= 1;
	}
}

int up_dshot_init(uint32_t channel_mask, unsigned dshot_freq,
		  bool enable_bidirectional_dshot)
{
	/* Reject an unsupported bidirectional configuration before touching any
	 * hardware or driver state: falling back silently to one-way DShot would
	 * leave the caller waiting for eRPM replies that can never arrive.
	 * Returning an empty channel mask (0) makes DShot.cpp take its existing
	 * "no outputs initialized" path and request_stop() the module. */
	if (enable_bidirectional_dshot && channel_mask != 0) {
		if (dshot_freq != 300000u && dshot_freq != 600000u) {
			PX4_ERR("DShot: bidirectional mode supports only DShot300/DShot600, requested %u Hz; refusing configuration",
				(unsigned)dshot_freq);

			for (uint8_t ti = 0; ti < MAX_IO_TIMERS; ti++) {
				dshot_timer_deinit(ti);
			}

			memset(dshot_channels, 0, sizeof(dshot_channels));
			memset(dshot_timers, 0, sizeof(dshot_timers));
			s_bdshot_enabled = false;
			return 0;
		}

		for (unsigned ch = 0; ch < DSHOT_MAX_CHANNELS; ch++) {
			if (!(channel_mask & (1u << ch))) {
				continue;
			}

			uint8_t t = timer_io_channels[ch].timer_index;

			if (t >= MAX_IO_TIMERS || !io_timers[t].base) {
				continue;
			}

			if (bdshot_sample_timers[t].sample_gptmr_base == 0) {
				PX4_ERR("DShot: timer %u has no sample timer for bidirectional mode; refusing configuration",
					t);

				for (uint8_t ti = 0; ti < MAX_IO_TIMERS; ti++) {
					dshot_timer_deinit(ti);
				}

				memset(dshot_channels, 0, sizeof(dshot_channels));
				memset(dshot_timers, 0, sizeof(dshot_timers));
				s_bdshot_enabled = false;
				return 0;
			}
		}
	}

	s_bdshot_enabled = enable_bidirectional_dshot;

	PX4_INFO("DShot: init freq=%u Hz, channels=0x%lx bidirectional=%u",
		(unsigned)dshot_freq, (unsigned long)channel_mask, (unsigned)s_bdshot_enabled);

	for (uint8_t t = 0; t < MAX_IO_TIMERS; t++) {
		dshot_timer_deinit(t);
	}

	memset(dshot_channels, 0, sizeof(dshot_channels));
	memset(dshot_timers, 0, sizeof(dshot_timers));

	for (uint8_t t = 0; t < MAX_IO_TIMERS; t++) {
		for (uint8_t sc = 0; sc < DSHOT_MAX_CH_PER_TIMER; sc++) {
			dshot_timers[t].px4_ch_ofs[sc] = 0xFF;
		}
	}

	for (unsigned ch = 0; ch < DSHOT_MAX_CHANNELS; ch++) {
		if (!(channel_mask & (1u << ch))) {
			continue;
		}

		uint8_t t = timer_io_channels[ch].timer_index;

		if (t >= MAX_IO_TIMERS || !io_timers[t].base) {
			continue;
		}

		/* claim the output channel so no other module can reuse it */
		if (io_timer_allocate_channel(ch, IOTimerChanMode_Dshot) != 0) {
			PX4_WARN("DShot: channel %u already in use", ch);
			continue;
		}

		dshot_timer_t *tmr = &dshot_timers[t];

		if (tmr->num_sub_ch >= DSHOT_MAX_CH_PER_TIMER) {
			PX4_ERR("DShot: timer %u: too many channels", t);
			io_timer_unallocate_channel(ch);   /* 回滚上面刚申请的通道 */
			continue;
		}

		uint8_t sub_ch = tmr->num_sub_ch;

		tmr->px4_ch_ofs[sub_ch] = (uint8_t)ch;
		tmr->px4_ch_mask       |= (1u << ch);
		tmr->num_sub_ch++;

		dshot_channels[ch].initialized = true;
		px4_arch_configgpio(timer_io_channels[ch].gpio_out);
	}

	uint32_t initialized_mask = 0;

	for (uint8_t t = 0; t < MAX_IO_TIMERS; t++) {
		if (dshot_timers[t].num_sub_ch == 0) {
			continue;
		}

		if (dshot_timer_init(t, dshot_freq) < 0) {
			PX4_ERR("DShot: timer %u init failed", t);

			for (uint8_t sc = 0; sc < dshot_timers[t].num_sub_ch; sc++) {
				uint8_t ch = dshot_timers[t].px4_ch_ofs[sc];

				if (ch < DSHOT_MAX_CHANNELS) {
					dshot_channels[ch].initialized = false;
				}
			}

			dshot_timer_deinit(t);
			continue;
		}

		initialized_mask |= dshot_timers[t].px4_ch_mask;
	}

	return (int)(channel_mask & initialized_mask);
}

static int dshot_timer_init(uint8_t timer_index, unsigned dshot_freq)
{
	dshot_timer_t  *t   = &dshot_timers[timer_index];
	PWM_Type       *pwm = (PWM_Type *)io_timers[timer_index].base;
	TRGM_Type      *trgm = (TRGM_Type *)io_timers[timer_index].trgm_base;

	if (io_timer_allocate_timer(timer_index, IOTimerChanMode_Dshot) != 0) {
		PX4_ERR("DShot: timer %u already in use", timer_index);
		return -1;
	}

	t->timer_claimed = true;

	/* up_dshot_init() validates the bidirectional configuration (rate and
	 * sample timer) before this point, so it is guaranteed usable here. */
	bool bdshot_capable = s_bdshot_enabled;

	t->pwm  = pwm;
	t->trgm = trgm;
	t->trgm_synci_out = TRGM_TRGOCFG_PWM_SYNCI;
	t->active_sub_mask = 0;
	t->dshot_freq = dshot_freq;

	clock_add_to_group((clock_name_t)io_timers[timer_index].clock_name, 0);

	t->clk_freq = clock_get_frequency((clock_name_t)io_timers[timer_index].clock_name);

	if (dshot_freq == 0 || t->clk_freq < dshot_freq) {
		PX4_ERR("DShot: timer %u: invalid freq %u Hz (clk %lu Hz)",
			timer_index, dshot_freq, (unsigned long)t->clk_freq);
		return -1;
	}

	t->reload   = t->clk_freq / dshot_freq - 1;

	t->t0h = 0;
	t->t1h = 0;

	uint32_t sample_window_us = DSHOT_BD_SAMPLE_WINDOW_US;
	for (uint8_t i = 0; i < (uint8_t)(sizeof(dshot_freq_duty_ctx) / sizeof(dshot_freq_duty_ctx[0])); i++) {
		if (dshot_freq == (unsigned)dshot_freq_duty_ctx[i].pwm_freq_KHz * 1000u) {
			t->t0h = (uint32_t)((uint64_t)dshot_freq_duty_ctx[i].t0h_ns * t->clk_freq / 1000000000ULL);
			t->t1h = (uint32_t)((uint64_t)dshot_freq_duty_ctx[i].t1h_ns * t->clk_freq / 1000000000ULL);
			sample_window_us = dshot_freq_duty_ctx[i].sample_window_us;
			break;
		}
	}

	if (!t->t0h || !t->t1h) {
		PX4_ERR("DShot: timer %u: unsupported freq %u Hz", timer_index, dshot_freq);
		return -1;
	}

	nxsem_init(&t->dma_sem, 0, 0);
	t->sem_initialized = true;

	pwm_stop_counter(pwm);
	reset_pwm_counter(t);

	pwm_config_t pwm_cfg = {0};
	pwm_get_default_pwm_config(pwm, &pwm_cfg);
	pwm_cfg.enable_output          = true;
	pwm_cfg.dead_zone_in_half_cycle = 0;
	/* Ordinary DShot idles low. Invert only for bidirectional DShot,
	 * which idles high, as selected by enable_bidirectional_dshot. */
	pwm_cfg.invert_output          = bdshot_capable;

	pwm_set_reload(pwm, 0, t->reload);
	pwm_set_start_count(pwm, 0, 0);

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		uint8_t          px4_ch     = t->px4_ch_ofs[sc];
		uint8_t          out_pin    = timer_io_channels[px4_ch].timer_channel;
		uint32_t         ch_idx     = s_sub_ch_cmp_start[sc];
		pwm_cmp_config_t cmp_cfg[2] = {0};

		cmp_cfg[0].mode           = pwm_cmp_mode_output_compare;
		cmp_cfg[0].cmp            = 0;
		cmp_cfg[0].update_trigger = pwm_shadow_register_update_on_modify;

		cmp_cfg[1].mode           = pwm_cmp_mode_output_compare;
		cmp_cfg[1].cmp            = 0;
		cmp_cfg[1].update_trigger = pwm_shadow_register_update_on_hw_event;

		if (status_success != pwm_setup_waveform(pwm, out_pin, &pwm_cfg,
							 (uint8_t)ch_idx,
							 cmp_cfg, 2)) {
			PX4_ERR("DShot: waveform failed timer %u ch %u",
				timer_index, px4_ch);
			return -1;
		}
	}

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		uint8_t px4_ch = t->px4_ch_ofs[sc];

		if (timer_io_channels[px4_ch].trgm_px == 0xFFu) {
			continue;
		}

		trgm_output_t trgm_config;

		trgm_config.invert = false;
		trgm_config.type   = trgm_output_same_as_input;
		trgm_config.input  = (uint8_t)(timer_io_channels[px4_ch].timer_channel - 8u)
				     + HPM_TRGM0_INPUT_SRC_PWM0_CH8REF;
		trgm_output_config(trgm, timer_io_channels[px4_ch].trgm_px, &trgm_config);
		trgm_enable_io_output(trgm, 1u << timer_io_channels[px4_ch].trgm_px);

		trgm_filter_t trgm_filter;

		trgm_filter.invert        = false;
		trgm_filter.sync          = false;
		trgm_filter.mode          = trgm_filter_mode_delay;
		trgm_filter.filter_length = 1;
		trgm_filter_config(trgm, timer_io_channels[px4_ch].trgm_px, &trgm_filter);
	}

	pwm_cmp_config_t sha_cmp = {0};
	sha_cmp.mode           = pwm_cmp_mode_output_compare;
	sha_cmp.cmp            = t->reload;
	sha_cmp.update_trigger = pwm_shadow_register_update_on_modify;
	pwm_load_cmp_shadow_on_match(pwm, DSHOT_CMP_SHADOW_MATCH, &sha_cmp);

	pwm_issue_shadow_register_lock_event(pwm);

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		pwm_cmp_config_t trig_cmp = {0};
		trig_cmp.mode = pwm_cmp_mode_output_compare;
		trig_cmp.update_trigger = pwm_shadow_register_update_on_modify;

		trig_cmp.cmp = 2 + 2 * sc;

		pwm_config_cmp(pwm, s_sub_ch_cmp_trig[sc], &trig_cmp);
	}

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		uint8_t dma_src = dshot_get_trgm_dma_src(pwm, sc);

		if (dma_src == 0xFF) {
			PX4_ERR("DShot: PWM %p sub_ch %u has no TRGM DMA src", pwm, sc);
			return -1;
		}

		trgm_dma_request_config(trgm, sc, dma_src);
	}

	uint32_t dma_req_mask = 0;

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		dma_req_mask |= (1UL << s_sub_ch_cmp_trig[sc]);
	}

	pwm_enable_dma_request(pwm, dma_req_mask);
	t->dma_req_mask = dma_req_mask;

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		/* all DMA (output and telemetry capture) runs on HDMA */
		if (status_success != dma_mgr_request_specified_resource(&t->dma_res[sc], HPM_HDMA)) {
			PX4_ERR("DShot: timer %u: dma resource request failed", timer_index);
			return -1;
		}

		t->dma_alloc_mask |= (1u << sc);

		uint8_t mot_src_base = dshot_get_mot_dma_src_base(pwm);

		if (mot_src_base == 0xFF) {
			PX4_ERR("DShot: PWM %p has no MOT DMA src", pwm);
			return -1;
		}

		dmamux_config(HPM_DMAMUX, t->dma_res[sc].channel,
			      (uint8_t)(mot_src_base + sc), true);

		uint32_t cmp_data_idx = s_sub_ch_cmp_start[sc] + 1;
		uint32_t dst_addr = (uint32_t)core_local_mem_to_sys_address(
			0, (uint32_t)&pwm->CMP[cmp_data_idx]);

		dma_channel_config_t *cfg = &t->dma_cfg[sc];

		dma_default_channel_config(t->dma_res[sc].base, cfg);

		cfg->src_addr       = (uint32_t)core_local_mem_to_sys_address(
			0, (uint32_t)&s_dma_buf[timer_index][sc][0]);
		cfg->src_width      = DMA_TRANSFER_WIDTH_WORD;
		cfg->src_mode       = DMA_HANDSHAKE_MODE_NORMAL;
		cfg->src_addr_ctrl  = DMA_ADDRESS_CONTROL_INCREMENT;
		cfg->src_burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
		cfg->dst_addr       = dst_addr;
		cfg->dst_width      = DMA_TRANSFER_WIDTH_WORD;
		cfg->dst_addr_ctrl  = DMA_ADDRESS_CONTROL_FIXED;
		cfg->dst_mode       = DMA_HANDSHAKE_MODE_HANDSHAKE;
		cfg->linked_ptr     = 0;
		cfg->size_in_byte   = DSHOT_BUFFER_SIZE * 4;
		cfg->interrupt_mask = DMA_INTERRUPT_MASK_TERMINAL_COUNT;
	}

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		dma_mgr_install_chn_tc_callback(&t->dma_res[sc], dshot_dma_tc_callback, t);
	}

	up_enable_irq(HPM_IRQn_HDMA);

	if (bdshot_capable) {
		if (setup_dshot_oversample_rx(timer_index, dshot_freq, sample_window_us) < 0) {
			PX4_ERR("DShot: timer %u: oversample RX setup failed", timer_index);
			return -1;
		}
	}

	t->configured = true;

	return 0;
}

static void dshot_timer_deinit(uint8_t timer_index)
{
	dshot_timer_t *t = &dshot_timers[timer_index];

	if (t->timer_claimed) {
		io_timer_unallocate_timer(timer_index);
		t->timer_claimed = false;
	}

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		io_timer_unallocate_channel(t->px4_ch_ofs[sc]);
	}

	if (t->pwm != NULL) {
		pwm_stop_counter(t->pwm);
	}

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		uint8_t px4_ch = t->px4_ch_ofs[sc];
		uint8_t ch_idx = s_sub_ch_cmp_start[sc];

		if (t->pwm != NULL) {
			pwm_disable_output(t->pwm, timer_io_channels[px4_ch].timer_channel);
			pwm_update_raw_cmp_edge_aligned(t->pwm, (uint8_t)(ch_idx + 1), 0);
		}

		/* Leave the pad in the default high-impedance state instead of
		 * leaving it muxed to a stopped PWM output. */
		px4_arch_unconfiggpio(timer_io_channels[px4_ch].gpio_out);
	}

	if (t->pwm != NULL) {
		pwm_issue_shadow_register_lock_event(t->pwm);
	}

	t->configured = false;

	if (t->sem_initialized) {
		nxsem_destroy(&t->dma_sem);
		t->sem_initialized = false;
	}

	if (t->pwm != NULL && t->dma_req_mask != 0) {
		pwm_disable_dma_request(t->pwm, t->dma_req_mask);
		t->dma_req_mask = 0;
	}

	for (uint8_t sc = 0; sc < DSHOT_MAX_CH_PER_TIMER; sc++) {
		if (!(t->dma_alloc_mask & (1u << sc))) {
			continue;
		}

		if (t->dma_res[sc].base) {
			uint8_t mot_src_base = (t->pwm != NULL) ? dshot_get_mot_dma_src_base(t->pwm) : 0xFFu;

			if (mot_src_base != 0xFF) {
				dmamux_config(HPM_DMAMUX, t->dma_res[sc].channel,
					      (uint8_t)(mot_src_base + sc), false);
			}

			dma_mgr_release_resource(&t->dma_res[sc]);
		}

		t->dma_alloc_mask &= ~(1u << sc);
	}

	/* sample_dma 的释放不能再用 bdshot_enabled 做条件：它在
	 * setup_dshot_oversample_rx() 中先于 bdshot_enabled=true 申请 */
	if (t->sample_dma.base != NULL) {
		dma_abort_channel(t->sample_dma.base, 1u << t->sample_dma.channel);
		dma_mgr_release_resource(&t->sample_dma);
		dmamux_config(HPM_DMAMUX, t->sample_dma.channel,
			      t->sample_dmamux_src, false);
		t->sample_dma.base = NULL;

		if (t->sample_gptmr != NULL) {
			gptmr_stop_counter(t->sample_gptmr, t->sample_gptmr_ch);
		}

		t->bdshot_enabled = false;
	}

	t->num_sub_ch = 0;
	t->px4_ch_mask = 0;
}

void dshot_motor_data_set(unsigned channel, uint16_t throttle, bool telemetry)
{
	if (channel >= DSHOT_MAX_CHANNELS || !dshot_channels[channel].initialized) {
		return;
	}

	uint8_t timer_index = timer_io_channels[channel].timer_index;
	bool invert_crc = (timer_index < MAX_IO_TIMERS) && dshot_timers[timer_index].bdshot_enabled;

	uint16_t packet = dshot_compute_frame(throttle, invert_crc ? true : telemetry, invert_crc);

	dshot_channels[channel].frame_data = packet;
	dshot_channels[channel].tx_pending = true;
}

void up_dshot_trigger(void)
{
	for (uint8_t timer_index = 0; timer_index < MAX_IO_TIMERS; timer_index++) {
		dshot_timer_t *t = &dshot_timers[timer_index];

		if (!t->configured) {
			continue;
		}

		if (t->bdshot_enabled) {
			bdshot_finish_capture_and_restore(t);
		}

		uint8_t tx_mask = 0;

		if (t->bdshot_enabled) {
			tx_mask = (uint8_t)((1u << t->num_sub_ch) - 1);

			for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
				dshot_channels[t->px4_ch_ofs[sc]].tx_pending = false;
			}

		} else
		{
			for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
				uint8_t px4_ch = t->px4_ch_ofs[sc];

				if (dshot_channels[px4_ch].tx_pending) {
					tx_mask |= (1u << sc);
					dshot_channels[px4_ch].tx_pending = false;
				}
			}
		}

		if (!tx_mask) {
			continue;
		}

		while (nxsem_trywait(&t->dma_sem) == 0) {
		}

		if (t->bdshot_enabled) {
			t->rx_buf_idx ^= 1u;
			bdshot_arm_sample_dma(t);
		}

		t->active_sub_mask = tx_mask;
		t->dma_done_mask   = 0;

		pwm_stop_counter(t->pwm);
		reset_pwm_counter(t);

		for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
			if (!(tx_mask & (1u << sc))) {
				continue;
			}

			uint8_t px4_ch = t->px4_ch_ofs[sc];

			dshot_fill_dma_buffer(s_dma_buf[timer_index][sc],
					      dshot_channels[px4_ch].frame_data, t->t0h, t->t1h);

			dma_setup_channel(t->dma_res[sc].base, t->dma_res[sc].channel,
					  &t->dma_cfg[sc], true);

			dma_enable_channel_interrupt(t->dma_res[sc].base, t->dma_res[sc].channel,
						     DMA_INTERRUPT_MASK_TERMINAL_COUNT);
		}
	}

	dshot_timer_t *start_tmrs[MAX_IO_TIMERS];
	uint8_t start_count = 0;

	for (uint8_t timer_index = 0; timer_index < MAX_IO_TIMERS; timer_index++) {
		dshot_timer_t *t = &dshot_timers[timer_index];

		if (t->configured && t->active_sub_mask) {
			start_tmrs[start_count++] = t;
		}
	}

	irqstate_t flags = px4_enter_critical_section();
	hrt_abstime tx_now = hrt_absolute_time();

	for (uint8_t i = 0; i < start_count; i++) {
		start_tmrs[i]->pwm->GCR |= PWM_GCR_CEN_MASK;

		if (start_tmrs[i]->bdshot_enabled) {
			gptmr_channel_reset_count(start_tmrs[i]->sample_gptmr,
						  start_tmrs[i]->sample_gptmr_ch);
			gptmr_start_counter(start_tmrs[i]->sample_gptmr,
					    start_tmrs[i]->sample_gptmr_ch);

			start_tmrs[i]->dir_switch_time = tx_now;
			start_tmrs[i]->tx_start_time   = tx_now;
		}
	}

	px4_leave_critical_section(flags);

	/* TX is now running on all timers (driven by PWM+DMA hardware).
	 * Decode the previous capture while the frame goes out, so that a slow
	 * decode can never delay the TX start. The switch back to input happens
	 * in the TX DMA TC interrupt, independent of this thread. */
	for (uint8_t timer_index = 0; timer_index < MAX_IO_TIMERS; timer_index++) {
		dshot_timer_t *t = &dshot_timers[timer_index];

		if (t->configured && t->bdshot_enabled) {
			bdshot_decode_pending(t);
		}
	}

	for (uint8_t timer_index = 0; timer_index < MAX_IO_TIMERS; timer_index++) {
		dshot_timer_t *t = &dshot_timers[timer_index];

		if (!t->configured || !t->active_sub_mask) {
			continue;
		}

		if (t->bdshot_enabled) {
			uint32_t frame_tx_us = (DSHOT_BUFFER_SIZE * 1000000u) / t->dshot_freq;
			hrt_abstime deadline = t->tx_start_time + frame_tx_us + 15u;

			while (!t->rx_active && hrt_absolute_time() < deadline) {
			}

		if (!t->rx_active) {
				irqstate_t swflags = px4_enter_critical_section();

				if (!t->rx_active) {
					for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
						if (t->active_sub_mask & (1u << sc)) {
							dma_resource_t *res = &t->dma_res[sc];
							dma_clear_transfer_status(res->base, res->channel);
						}
					}

					bdshot_switch_to_input(t);
				}

				px4_leave_critical_section(swflags);
			}

			pwm_stop_counter(t->pwm);
			t->active_sub_mask = 0;
			t->dma_done_mask   = 0;

		} else {
			if (nxsem_tickwait(&t->dma_sem, MSEC2TICK(10)) < 0) {
				PX4_ERR("DShot: timer %u DMA timeout", timer_index);

				for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
					if (!(t->active_sub_mask & (1u << sc))) {
						continue;
					}

					dma_mgr_disable_channel(&t->dma_res[sc]);
				}
			}

			pwm_stop_counter(t->pwm);
			t->active_sub_mask = 0;
		}
	}
}

int up_dshot_arm(bool armed)
{
	for (uint8_t timer_index = 0; timer_index < MAX_IO_TIMERS; timer_index++) {
		dshot_timer_t *t = &dshot_timers[timer_index];

		if (!t->configured) {
			continue;
		}

		if (armed) {
			pwm_start_counter(t->pwm);

			for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
				uint8_t px4_ch = t->px4_ch_ofs[sc];
				pwm_enable_output(t->pwm,
					timer_io_channels[px4_ch].timer_channel);
			}

		} else {
			pwm_stop_counter(t->pwm);

			/* 丢弃可能仍在途的 TX DMA，并清掉帧状态掩码：否则
			 * disarm 瞬间的 TC 回调/残留掩码会留到下一次 rearm。 */
			for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
				if (t->dma_res[sc].base != NULL) {
					dma_abort_channel(t->dma_res[sc].base, 1u << t->dma_res[sc].channel);
					dma_clear_transfer_status(t->dma_res[sc].base, t->dma_res[sc].channel);
				}
			}

			t->active_sub_mask = 0;
			t->dma_done_mask   = 0;

			if (t->bdshot_enabled) {
				gptmr_stop_counter(t->sample_gptmr, t->sample_gptmr_ch);
				dma_abort_channel(t->sample_dma.base,
						  1u << t->sample_dma.channel);
				t->rx_active = false;
				t->rx_decode_pending = false;
			}

			for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
				uint8_t px4_ch = t->px4_ch_ofs[sc];
				uint8_t ch_idx = s_sub_ch_cmp_start[sc];

				pwm_disable_output(t->pwm,
					timer_io_channels[px4_ch].timer_channel);

				pwm_update_raw_cmp_edge_aligned(t->pwm,
					(uint8_t)(ch_idx + 1), 0);
			}

			pwm_issue_shadow_register_lock_event(t->pwm);
		}
	}

	return 0;
}


int up_bdshot_num_erpm_ready(void)
{
	int num_ready = 0;

	for (unsigned ch = 0; ch < DSHOT_MAX_CHANNELS; ch++) {
		if (dshot_channels[ch].initialized && dshot_channels[ch].erpm_ready) {
			num_ready++;
		}
	}

	return num_ready;
}

int up_bdshot_get_erpm(uint8_t channel, int *erpm)
{
	if (channel >= DSHOT_MAX_CHANNELS || !dshot_channels[channel].initialized) {
		return -1;
	}

	uint8_t timer_index = timer_io_channels[channel].timer_index;

	if (!dshot_timers[timer_index].bdshot_enabled) {
		return -1;
	}

	/* Do not hand out a value that was not produced by a frame since the
	 * last read: the upper layer compares a global ready count against the
	 * number of active motors, so a not-ready channel must fail here. */
	if (!dshot_channels[channel].erpm_ready) {
		return -1;
	}

	bool valid = dshot_channels[channel].erpm_valid;
	dshot_channels[channel].erpm_ready = false;

	/* A rejected frame must not be published as a valid 0 eRPM. */
	if (!valid) {
		return -1;
	}

	*erpm = dshot_channels[channel].erpm;

	return 0;
}

int up_bdshot_channel_status(uint8_t channel)
{
	if (channel >= DSHOT_MAX_CHANNELS || !dshot_channels[channel].initialized) {
		return 0;
	}

	if (!dshot_timers[timer_io_channels[channel].timer_index].bdshot_enabled) {
		return 0;
	}

	if (dshot_channels[channel].last_ok_us == 0) {
		return 0;
	}

	return (hrt_absolute_time() - dshot_channels[channel].last_ok_us) < (hrt_abstime)100000u ? 1 : 0;
}

void up_bdshot_status(void)
{
	PX4_INFO("dshot driver stats:");

	if (s_bdshot_enabled) {
		PX4_INFO("Bidirectional DShot enabled");
	}

	for (uint8_t timer_index = 0; timer_index < MAX_IO_TIMERS; timer_index++) {
		const dshot_timer_t *t = &dshot_timers[timer_index];

		if (t->num_sub_ch == 0) {
			continue;
		}

		PX4_INFO("Timer %u: %u channel(s)%s", timer_index, t->num_sub_ch,
			 t->bdshot_enabled ? ", bidirectional" : "");

		for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
			uint8_t px4_ch = t->px4_ch_ofs[sc];

			if (px4_ch >= DSHOT_MAX_CHANNELS || !dshot_channels[px4_ch].initialized) {
				continue;
			}

			PX4_INFO("  Motor %u: erpm %ld, %s", px4_ch,
				 (long)dshot_channels[px4_ch].erpm,
				 up_bdshot_channel_status(px4_ch) ? "online" : "no reply");
		}
	}
}
