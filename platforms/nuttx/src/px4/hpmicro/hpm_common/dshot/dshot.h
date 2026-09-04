/****************************************************************************
 *
 *   Copyright (c) 2024 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary form, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#pragma once

#include "hpm_soc.h"

/*
 * DShot DMA glue: per-PWM configuration (TRGM DMA source base + DMAMUX
 * MOT slot base).
 *
 * `trgm_src_base` is the TRGM DMA source id for this PWM's CMP9;
 * CMP10/11/12 are base+1/+2/+3 (same numeric value across all HPM PWMs).
 * `mot_src_base` is the DMAMUX MOTx_0 source id.
 *
 * The data table below is the default.  Boards that need to add or
 * remove PWMs (e.g. a 4-PWM board, or a SoC that lacks PWM1) can
 * #define DSHOT_PWM_DMA_OVERRIDE before including this header and
 * provide their own dshot_pwm_dma_map[] / dshot_pwm_dma_map_count.
 */

struct dshot_pwm_dma_t {
	PWM_Type *pwm;
	uint8_t  trgm_src_base;
	uint8_t  mot_src_base;
};

#ifndef DSHOT_PWM_DMA_OVERRIDE

static const struct dshot_pwm_dma_t dshot_pwm_dma_map[] = {
#if defined(HPM_PWM0) && defined(HPM_TRGM0_DMA_SRC_PWM0_CMP9) && defined(HPM_DMA_SRC_MOT0_0)
	{HPM_PWM0, HPM_TRGM0_DMA_SRC_PWM0_CMP9, HPM_DMA_SRC_MOT0_0},
#endif
#if defined(HPM_PWM1) && defined(HPM_TRGM1_DMA_SRC_PWM1_CMP9) && defined(HPM_DMA_SRC_MOT1_0)
	{HPM_PWM1, HPM_TRGM1_DMA_SRC_PWM1_CMP9, HPM_DMA_SRC_MOT1_0},
#endif
};

static const unsigned dshot_pwm_dma_map_count =
	sizeof(dshot_pwm_dma_map) / sizeof(dshot_pwm_dma_map[0]);

#endif /* DSHOT_PWM_DMA_OVERRIDE */
