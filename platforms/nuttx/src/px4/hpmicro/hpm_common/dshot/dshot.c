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

/**
 * @file dshot.c
 *
 * HPMicro DShot output driver using PWM + DMA.
 *
 * PWM edge-aligned mode with two CMPs per channel:
 *   CMP_START = 0        -> output HIGH at counter wrap
 *   CMP_END   = T0H/T1H  -> output LOW at compare match
 *
 * A trigger CMP fires at the end of each bit period and triggers a DMA
 * transfer that updates CMP_END for the next bit. Each frame in the DMA
 * buffer is guarded by leading/trailing zeros so the output stays LOW
 * between frames. Before each frame the counter is re-aligned so that all
 * channels start on the same bit. All channels on a PWM module transmit
 * simultaneously.
 *
 * DMA completion uses raw HAL ISR + semaphore instead of busy-wait.
 */

#include <px4_platform_common/px4_config.h>
#include <px4_platform_common/micro_hal.h>
#include <px4_platform_common/log.h>
#include <px4_platform_common/time.h>
#include <px4_arch/io_timer.h>
#include <drivers/drv_dshot.h>
#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/semaphore.h>

#include <arch/board/board.h>
#include "hpm_pwm_drv.h"
#include "hpm_trgm_drv.h"
#include "hpm_clock_drv.h"
#include "hpm_dma_drv.h"
#include "hpm_dmamux_drv.h"
#include "hpm_dma_mgr.h"
#include "hpm_soc.h"
#include "hpm_misc.h"

#define DSHOT_FRAME_BITS            16u
#define DSHOT_THROTTLE_POSITION     5u
#define DSHOT_TELEMETRY_POSITION    4u
#define NIBBLES_SIZE                4u
#define DSHOT_NUMBER_OF_NIBBLES     3u
#define DSHOT_BUFFER_SIZE           18u /* 16 bit times + leading/trailing zero guard */

#define DSHOT_MAX_CHANNELS          MAX_TIMER_IO_CHANNELS
#define DSHOT_MAX_CH_PER_TIMER      4u
#define DSHOT_RELOAD_HIGH_THRESHOLD 400u

static const uint8_t s_sub_ch_cmp_start[DSHOT_MAX_CH_PER_TIMER] = {0, 2, 4, 6};
static const uint8_t s_sub_ch_cmp_trig[DSHOT_MAX_CH_PER_TIMER]  = {9, 10, 11, 12};
#define DSHOT_CMP_SHADOW_MATCH  8u

typedef struct {
    uint32_t pwm_freq_KHz;
    uint32_t t0h_ns;
    uint32_t t1h_ns;
} dshot_freq_duty_list;

static dshot_freq_duty_list dshot_freq_duty_ctx[4] = {
    {150, 2500, 5000},
    {300, 1250, 2500},
    {600, 625, 1250},
    {1200, 313, 625}
};

static const uint8_t s_trgm_dma_src[DSHOT_MAX_CH_PER_TIMER] = {
	HPM_TRGM0_DMA_SRC_PWM0_CMP9,
	HPM_TRGM0_DMA_SRC_PWM0_CMP10,
	HPM_TRGM0_DMA_SRC_PWM0_CMP11,
	HPM_TRGM0_DMA_SRC_PWM0_CMP12,
};

typedef struct {
	bool     initialized;
	uint16_t frame_data;
	bool     tx_pending;
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
	bool            isr_attached;

	uint8_t         num_sub_ch;
	uint32_t        px4_ch_mask;
	uint8_t         px4_ch_ofs[DSHOT_MAX_CH_PER_TIMER];

	dma_resource_t       dma_res[DSHOT_MAX_CH_PER_TIMER];
	dma_channel_config_t dma_cfg[DSHOT_MAX_CH_PER_TIMER];

	uint32_t dma_buf[DSHOT_MAX_CH_PER_TIMER][DSHOT_BUFFER_SIZE];

	sem_t            dma_sem;
	volatile uint8_t active_sub_mask;
	volatile uint8_t dma_done_mask;
} dshot_timer_t;

static dshot_channel_t dshot_channels[DSHOT_MAX_CHANNELS];
ATTR_PLACE_AT_NONCACHEABLE_WITH_ALIGNMENT(8) dshot_timer_t   dshot_timers[MAX_IO_TIMERS];

static void dshot_timer_deinit(uint8_t timer_index);
static int  dshot_timer_init(uint8_t timer_index, unsigned dshot_freq, uint32_t channel_mask);

static int dshot_hdma_isr(int irq, void *context, void *arg)
{
	(void)context;
	(void)arg;

	if (irq != HPM_IRQn_HDMA) {
		return 0;
	}

	for (uint8_t t = 0; t < MAX_IO_TIMERS; t++) {
		dshot_timer_t *tmr = &dshot_timers[t];

		if (!tmr->configured || !tmr->active_sub_mask) {
			continue;
		}

		for (uint8_t sc = 0; sc < tmr->num_sub_ch; sc++) {
			if (!(tmr->active_sub_mask & (1u << sc))) {
				continue;
			}

			dma_resource_t *res = &tmr->dma_res[sc];

			uint32_t status = dma_check_transfer_status(res->base, res->channel);

			if (status & DMA_CHANNEL_STATUS_TC) {
				dma_clear_transfer_status(res->base, res->channel);
				tmr->dma_done_mask |= (1u << sc);
			}
		}

		if (tmr->dma_done_mask == tmr->active_sub_mask) {
			tmr->dma_done_mask = 0;
			nxsem_post(&tmr->dma_sem);
		}
	}

	return 0;
}

static void reset_pwm_counter(dshot_timer_t *t)
{
	pwm_timer_reset(t->pwm);
	pwm_set_start_count(t->pwm, 0, 0);

	pwm_enable_reload_at_synci(t->pwm);
	trgm_output_update_source(t->trgm, t->trgm_synci_out, 1);
	trgm_output_update_source(t->trgm, t->trgm_synci_out, 0);
	t->pwm->GCR &= ~(PWM_GCR_RLDSYNCEN_MASK | PWM_GCR_XRLDSYNCEN_MASK);
}

static uint16_t dshot_compute_frame(uint16_t throttle, bool telemetry)
{
	uint16_t packet = throttle << DSHOT_THROTTLE_POSITION;
	packet |= ((uint16_t)telemetry & 0x01u) << DSHOT_TELEMETRY_POSITION;

	uint16_t cs = packet >> NIBBLES_SIZE;
	uint16_t ck = 0;

	for (unsigned i = 0; i < DSHOT_NUMBER_OF_NIBBLES; i++) {
		ck ^= (cs & 0x0Fu);
		cs >>= NIBBLES_SIZE;
	}

	packet |= (ck & 0x0Fu);

	return packet;
}

static void dshot_fill_dma_buffer(uint32_t *buf, uint16_t frame_data,
				  uint32_t t0h, uint32_t t1h)
{
	uint16_t packet = frame_data;

	buf[0] = 0x0;

	for (uint8_t i = 1; i <= DSHOT_FRAME_BITS; i++) {
		uint32_t raw_cmp = (packet & 0x8000) ? t1h : t0h;
		buf[i] = PWM_CMP_CMP_SET(raw_cmp);
		packet <<= 1;
	}

	buf[DSHOT_BUFFER_SIZE - 1] = 0x0;
}

int up_dshot_init(uint32_t channel_mask, unsigned dshot_freq,
		  bool enable_bidirectional_dshot)
{
	(void)enable_bidirectional_dshot;

	PX4_INFO("DShot: init freq=%u Hz, channels=%u bidirectional=%u",
		(unsigned)dshot_freq, (unsigned)channel_mask, (unsigned)enable_bidirectional_dshot);

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

		dshot_timer_t *tmr = &dshot_timers[t];

		if (tmr->num_sub_ch >= DSHOT_MAX_CH_PER_TIMER) {
			PX4_ERR("DShot: timer %u: too many channels", t);
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

		if (dshot_timer_init(t, dshot_freq, channel_mask) < 0) {
			PX4_ERR("DShot: timer %u init failed", t);
			dshot_timer_deinit(t);
			continue;
		}

		initialized_mask |= dshot_timers[t].px4_ch_mask;
	}

	return (int)(channel_mask & initialized_mask);
}

static int dshot_timer_init(uint8_t timer_index, unsigned dshot_freq,
			    uint32_t channel_mask)
{
	dshot_timer_t  *t   = &dshot_timers[timer_index];
	PWM_Type       *pwm = (PWM_Type *)io_timers[timer_index].base;
	TRGM_Type      *trgm = (TRGM_Type *)io_timers[timer_index].trgm_base;

	t->pwm  = pwm;
	t->trgm = trgm;
	t->trgm_synci_out = TRGM_TRGOCFG_PWM_SYNCI;
	t->active_sub_mask = 0;

	clock_add_to_group((clock_name_t)io_timers[timer_index].clock_name, 0);

	t->clk_freq = clock_get_frequency((clock_name_t)io_timers[timer_index].clock_name);
	t->reload   = t->clk_freq / dshot_freq - 1;

	t->t0h = 0;
	t->t1h = 0;

	for (uint8_t i = 0; i < (uint8_t)(sizeof(dshot_freq_duty_ctx) / sizeof(dshot_freq_duty_ctx[0])); i++) {
		if (dshot_freq == (unsigned)dshot_freq_duty_ctx[i].pwm_freq_KHz * 1000u) {
			t->t0h = (uint32_t)((uint64_t)dshot_freq_duty_ctx[i].t0h_ns * t->clk_freq / 1000000000ULL);
			t->t1h = (uint32_t)((uint64_t)dshot_freq_duty_ctx[i].t1h_ns * t->clk_freq / 1000000000ULL);
			break;
		}
	}

	if (!t->t0h || !t->t1h) {
		PX4_ERR("DShot: timer %u: unsupported freq %u Hz", timer_index, dshot_freq);
		return -1;
	}

	nxsem_init(&t->dma_sem, 0, 0);

	pwm_stop_counter(pwm);
	reset_pwm_counter(t);

	pwm_config_t pwm_cfg = {0};
	pwm_get_default_pwm_config(pwm, &pwm_cfg);
	pwm_cfg.enable_output          = true;
	pwm_cfg.dead_zone_in_half_cycle = 0;
	pwm_cfg.invert_output          = false;

	pwm_set_reload(pwm, 0, t->reload);
	pwm_set_start_count(pwm, 0, 0);

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		uint8_t          px4_ch     = t->px4_ch_ofs[sc];
		uint8_t          out_pin    = timer_io_channels[px4_ch].timer_channel;
		uint32_t         ch_idx     = s_sub_ch_cmp_start[sc];
		pwm_cmp_config_t cmp_cfg[2];

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
		trgm_dma_request_config(trgm, sc, s_trgm_dma_src[sc]);
	}

	uint32_t dma_req_mask = 0;

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		dma_req_mask |= (1UL << s_sub_ch_cmp_trig[sc]);
	}

	pwm_enable_dma_request(pwm, dma_req_mask);

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		if (status_success != dma_mgr_request_specified_resource(&t->dma_res[sc], HPM_HDMA)) {
			PX4_ERR("DShot: timer %u: dma resource request failed", timer_index);
			return -1;
		}

		dmamux_config(HPM_DMAMUX, t->dma_res[sc].channel,
			      (uint8_t)(HPM_DMA_SRC_MOT0_0 + sc), true);

		uint32_t cmp_data_idx = s_sub_ch_cmp_start[sc] + 1;
		uint32_t dst_addr = (uint32_t)core_local_mem_to_sys_address(
			0, (uint32_t)&pwm->CMP[cmp_data_idx]);

		dma_channel_config_t *cfg = &t->dma_cfg[sc];

		dma_default_channel_config(t->dma_res[sc].base, cfg);

		cfg->src_addr       = (uint32_t)core_local_mem_to_sys_address(
			0, (uint32_t)&t->dma_buf[sc][0]);
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

	irq_attach(HPM_IRQn_HDMA, dshot_hdma_isr, NULL);
	intc_set_irq_priority(IRQn_HDMA, 1);
	up_enable_irq(HPM_IRQn_HDMA);
	t->isr_attached = true;

	t->configured = true;

	return 0;
}

static void dshot_timer_deinit(uint8_t timer_index)
{
	dshot_timer_t *t = &dshot_timers[timer_index];

	if (!t->configured) {
		return;
	}

	pwm_stop_counter(t->pwm);

	if (t->isr_attached) {
		up_disable_irq(HPM_IRQn_HDMA);
		irq_detach(HPM_IRQn_HDMA);
		t->isr_attached = false;
	}

	nxsem_destroy(&t->dma_sem);

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		dmamux_config(HPM_DMAMUX, t->dma_res[sc].channel,
			      (uint8_t)(HPM_DMA_SRC_MOT0_0 + sc), false);
	}

	t->configured = false;
	t->num_sub_ch = 0;
	t->px4_ch_mask = 0;
}

static uint16_t s_dbg_last_throttle[DSHOT_MAX_CHANNELS];

void dshot_motor_data_set(unsigned channel, uint16_t throttle, bool telemetry)
{
	if (channel >= DSHOT_MAX_CHANNELS || !dshot_channels[channel].initialized) {
		return;
	}

	if (throttle != 0 && throttle != s_dbg_last_throttle[channel]) {
		s_dbg_last_throttle[channel] = throttle;
		PX4_INFO("dshot_motor_data_set: ch %u throttle=%u", channel, throttle);
	}

	uint16_t packet = dshot_compute_frame(throttle, telemetry);

	dshot_channels[channel].frame_data = packet;
	dshot_channels[channel].tx_pending = true;

	uint8_t timer_index = timer_io_channels[channel].timer_index;
	dshot_timer_t *t = &dshot_timers[timer_index];

	uint8_t sub_ch = 0xFF;

	for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
		if (t->px4_ch_ofs[sc] == channel) {
			sub_ch = sc;
			break;
		}
	}

	if (sub_ch >= DSHOT_MAX_CH_PER_TIMER) {
		return;
	}

	dshot_fill_dma_buffer(t->dma_buf[sub_ch], packet, t->t0h, t->t1h);
}

void up_dshot_trigger(void)
{
	for (uint8_t timer_index = 0; timer_index < MAX_IO_TIMERS; timer_index++) {
		dshot_timer_t *t = &dshot_timers[timer_index];

		if (!t->configured) {
			continue;
		}

		uint8_t tx_mask = 0;

		for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
			uint8_t px4_ch = t->px4_ch_ofs[sc];

			if (dshot_channels[px4_ch].tx_pending) {
				tx_mask |= (1u << sc);
				dshot_channels[px4_ch].tx_pending = false;
			}
		}

		if (!tx_mask) {
			continue;
		}

		t->active_sub_mask = tx_mask;
		t->dma_done_mask   = 0;

		pwm_stop_counter(t->pwm);
		reset_pwm_counter(t);

		for (uint8_t sc = 0; sc < t->num_sub_ch; sc++) {
			if (!(tx_mask & (1u << sc))) {
				continue;
			}

			dma_setup_channel(t->dma_res[sc].base, t->dma_res[sc].channel,
					  &t->dma_cfg[sc], true);
			dma_enable_channel_interrupt(t->dma_res[sc].base, t->dma_res[sc].channel,
				DMA_INTERRUPT_MASK_TERMINAL_COUNT);
		}

		pwm_start_counter(t->pwm);

		nxsem_wait(&t->dma_sem);
		t->active_sub_mask = 0;
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
	return 0;
}

int up_bdshot_get_erpm(uint8_t channel, int *erpm)
{
	(void)channel;
	(void)erpm;
	return -1;
}

int up_bdshot_channel_status(uint8_t channel)
{
	(void)channel;
	return -1;
}

void up_bdshot_status(void)
{
}
