/****************************************************************************
 *
 *   Copyright (c) 2024 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary form, with or without
 * modification, are permitted provided that the following conditions
 * are met:
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

#include <px4_platform_common/px4_config.h>

#include <stdbool.h>
#include <stdint.h>

#include "board_config.h"

#include <arch/board/board.h>
#include <hpm_gpio.h>

/*
 * Two on-board LEDs, both active-high.  See board_config.h.
 *   LED1 = PB31, LED2 = PB01.
 *
 * The on macro selects the GPIO_OUTPUT_SET variant, the off macro the
 * GPIO_OUTPUT_CLEAR variant, so we keep a pair of pin configs per LED
 * and pick the right one at runtime.
 */

static const uint32_t g_ledmap_on[] = {
	GPIO_LED1_ON,  /* BOARD_LED1 (red, status)  */
	GPIO_LED2_ON,  /* BOARD_LED2 (blue, activity) */
};

static const uint32_t g_ledmap_off[] = {
	GPIO_LED1_OFF,
	GPIO_LED2_OFF,
};

__EXPORT void led_init(void)
{
	/* Configured to off (output low = LED off for active-high). */
	for (size_t l = 0; l < sizeof(g_ledmap_off) / sizeof(g_ledmap_off[0]); l++) {
		if (g_ledmap_off[l] != 0) {
			hpm_config_gpio(g_ledmap_off[l]);
		}
	}
}

__EXPORT void led_on(int led)
{
	if (led >= 0 && led < (int)(sizeof(g_ledmap_on) / sizeof(g_ledmap_on[0]))
	    && g_ledmap_on[led] != 0) {
		hpm_config_gpio(g_ledmap_on[led]);
	}
}

__EXPORT void led_off(int led)
{
	if (led >= 0 && led < (int)(sizeof(g_ledmap_off) / sizeof(g_ledmap_off[0]))
	    && g_ledmap_off[led] != 0) {
		hpm_config_gpio(g_ledmap_off[led]);
	}
}

__EXPORT void led_toggle(int led)
{
	if (led >= 0 && led < (int)(sizeof(g_ledmap_on) / sizeof(g_ledmap_on[0]))
	    && g_ledmap_on[led] != 0) {
		bool is_on = hpm_gpio_read(g_ledmap_on[led]);
		hpm_config_gpio(is_on ? g_ledmap_off[led] : g_ledmap_on[led]);
	}
}
