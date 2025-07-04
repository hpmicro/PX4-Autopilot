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
 * @file io_gpio.c
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

#include "io_gpio.h"
#include "hpm_gpio_drv.h"

struct gpio_callback_s {
	xcpt_t callback;
	void *arg;
	GPIO_Type *ptr;
	uint16_t port;
};

#ifdef IRQn_GPIO0_A
static struct gpio_callback_s g_gpioa_callbacks[32];
#endif
#ifdef IRQn_GPIO0_B
static struct gpio_callback_s g_gpiob_callbacks[32];
#endif
#ifdef IRQn_GPIO0_C
static struct gpio_callback_s g_gpioc_callbacks[32];
#endif
#ifdef IRQn_GPIO0_D
static struct gpio_callback_s g_gpiod_callbacks[32];
#endif
#ifdef IRQn_GPIO0_E
static struct gpio_callback_s g_gpioe_callbacks[32];
#endif
#ifdef IRQn_GPIO0_F
static struct gpio_callback_s g_gpiof_callbacks[32];
#endif
#ifdef IRQn_GPIO0_X
static struct gpio_callback_s g_gpiox_callbacks[32];
#endif
#ifdef IRQn_GPIO0_Y
static struct gpio_callback_s g_gpioy_callbacks[32];
#endif
#ifdef IRQn_GPIO0_Z
static struct gpio_callback_s g_gpioz_callbacks[32];
#endif

/****************************************************************************
 * Name: hpm_config_gpio
 *
 * Description:
 *   Configure a GPIO pin based on bit-encoded description of the pin.
 *   Once it is configured as Alternative (GPIO_ALT|GPIO_CNF_AFPP|...)
 *   function, it must be unconfigured with hpm_unconfiggpio() with
 *   the same pinset first before it can be set to non-alternative function.
 *
 * Returned Value:
 *   OK on success
 *   A negated errno value on invalid port, or when pin is locked as ALT
 *   function.
 *
 * To-Do: Auto Power Enable
 ****************************************************************************/

int hpm_config_gpio(uint32_t pinset)
{
	irqstate_t flags;
	IOC_Type *ioc_ptr;
	GPIO_Type *gpio_ptr;
	uint32_t pad_ctl = 0;
	uint32_t fun_ctl = 0;
	uint32_t port = (pinset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
	uint32_t pin = (pinset & GPIO_PIN_MASK) >> GPIO_PIN_SHIFT;
	uint32_t pad_index = port * 32 + pin;

#ifndef HPM_PIOC
	assert(port != GPIO_PORTY);
#endif

#ifndef HPM_BIOC
	assert(port != GPIO_PORTZ);
#endif

	flags = enter_critical_section();

	switch (pinset & GPIO_CONTROLLER_MASK) {
	case GPIO0:
		gpio_ptr = HPM_GPIO0;
		gpiom_set_pin_controller(HPM_GPIOM, port, pin, gpiom_soc_gpio0);
#ifdef HPM_PIOC

		if (port == GPIO_PORTY) {
			HPM_PIOC->PAD[pad_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF3);
		}

#endif
#ifdef HPM_BIOC

		if (port == GPIO_PORTZ) {
			HPM_BIOC->PAD[pad_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF3);
		}

#endif
		ioc_ptr = HPM_IOC;
		break;

#if defined(HPM_FGPIO) || defined(HPM_FGPIO0)

	case FGPIO0:
#ifdef HPM_FGPIO
		gpio_ptr = HPM_FGPIO;
#endif
#ifdef HPM_FGPIO0
		gpio_ptr = HPM_FGPIO0;
#endif
		gpiom_set_pin_controller(HPM_GPIOM, port, pin, gpiom_core0_fast);
#ifdef HPM_PIOC

		if (port == GPIO_PORTY) {
			HPM_PIOC->PAD[pad_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF3);
		}

#endif
#ifdef HPM_BIOC

		if (port == GPIO_PORTZ) {
			HPM_BIOC->PAD[pad_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF3);
		}

#endif
		ioc_ptr = HPM_IOC;
		break;
#endif

#ifdef HPM_GPIO1

	case GPIO1:
		gpio_ptr = HPM_GPIO1;
		gpiom_set_pin_controller(HPM_GPIOM, port, pin, gpiom_soc_gpio1);
#ifdef HPM_PIOC

		if (port == GPIO_PORTY) {
			HPM_PIOC->PAD[pad_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF3);
		}

#endif
#ifdef HPM_BIOC

		if (port == GPIO_PORTZ) {
			HPM_BIOC->PAD[pad_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF3);
		}

#endif
		ioc_ptr = HPM_IOC;
		break;
#endif

#ifdef HPM_FGPIO1

	case FGPIO1:
		gpio_ptr = HPM_FGPIO1;
		gpiom_set_pin_controller(HPM_GPIOM, port, pin, gpiom_core1_fast);
#ifdef HPM_PIOC

		if (port == GPIO_PORTY) {
			HPM_PIOC->PAD[pad_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF3);
		}

#endif
#ifdef HPM_BIOC

		if (port == GPIO_PORTZ) {
			HPM_BIOC->PAD[pad_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF3);
		}

#endif
		ioc_ptr = HPM_IOC;
		break;
#endif

#ifdef HPM_PGPIO

	case PGPIO:
		gpio_ptr = HPM_PGPIO;
		ioc_ptr = HPM_PIOC;
		break;
#endif

#ifdef HPM_BGPIO

	case BGPIO:
		gpio_ptr = HPM_BGPIO;
		ioc_ptr = HPM_BIOC;
		break;
#endif

	default:
		break;
	}

	switch (pinset & GPIO_MODE_MASK) {
	default:
	case GPIO_INPUT:
		fun_ctl |= IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF0);
		gpio_set_pin_input(ptr, port, pin);
		break;

	case GPIO_OUTPUT:
		fun_ctl |= IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF0);
		gpio_set_pin_output_with_initial(ptr, port, pin, (pinset & GPIO_OUTPUT_SET) >> GPIO_OUTPUT_INIT_SHIFT);
		break;

	case GPIO_ALT:
		fun_ctl |= IOC_PAD_FUNC_CTL_ALT_SELECT_SET((pinset & GPIO_AF_MASK) >> GPIO_AF_SHIFT);
		break;
		gpiom_set_pin_controller(HPM_GPIOM, port, pin, gpiom_core0_fast);
#ifdef HPM_PIOC

		if (port == GPIO_PORTY) {
			HPM_PIOC->PAD[pad_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF3);
		}

#endif
#ifdef HPM_BIOC

		if (port == GPIO_PORTZ) {
			HPM_BIOC->PAD[pad_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(GPIO_AF3);
		}

#endif
		ioc_ptr = HPM_IOC;

	case GPIO_ANALOG:
		fun_ctl |= IOC_PAD_FUNC_CTL_ANALOG_MASK;
		break;

	default:
		break;
	}

	if (pinset & GPIO_LOOPBACK) {
		fun_ctl |= IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
	}

	if (pinset & GPIO_SMT) {
		pad_ctl |= IOC_PAD_PAD_CTL_SMT_MASK;
	}

	if (pinset & GPIO_OPENDRAIN) {
		pad_ctl |= IOC_PAD_PAD_CTL_OD_MASK;
	}

	switch (pinset & GPIO_PUPD_MASK) {
	case GPIO_PULLUP:
		pad_ctl |= IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1);
		break;

	case GPIO_PULLDOWN:
		pad_ctl |= IOC_PAD_PAD_CTL_PE_SET(1);
		break;

	case GPIO_FLOAT:
	default:
		break;
	}

	pad_ctl |= IOC_PAD_PAD_CTL_DS_SET((pinset & GPIO_DS_MASK) >> GPIO_DS_SHIFT);

#if defined(CONFIG_ARCH_CHIP_HPM6750_SDK) && CONFIG_ARCH_CHIP_HPM6750_SDK

	if (pinset & GPIO_1V8) {
		pad_ctl |= IOC_PAD_PAD_CTL_MS_MASK;
	}

#else

	if (pinset & GPIO_SPEED_FAST) {
		pad_ctl |= IOC_PAD_PAD_CTL_SR_MASK;
	}
#endif

	ioc_ptr->PAD[pad_index].FUNC_CTL = fun_ctl;
	ioc_ptr->PAD[pad_index].PAD_CTL = pad_ctl;

	leave_critical_section(flags);
	return OK;
}

/****************************************************************************
 * Name: hpm_unconfig_gpio
 *
 * Description:
 *   Unconfigure a GPIO pin based on bit-encoded description of the pin, set
 *   it into default HiZ state (and possibly mark it's unused) and unlock it
 *   whether it was previously selected as alternative function
 *   (GPIO_ALT|GPIO_CNF_AFPP|...).
 *
 *   This is a safety function and prevents hardware from shocks, as
 *   unexpected write to the Timer Channel Output GPIO to fixed '1' or '0'
 *   while it should operate in PWM mode could produce excessive on-board
 *   currents and trigger over-current/alarm function.
 *
 * Returned Value:
 *  OK on success
 *  A negated errno value on invalid port
 *
 * To-Do: Auto Power Disable
 ****************************************************************************/

int hpm_unconfig_gpio(uint32_t pinset)
{
	/* Reuse port and pin number and set it to default HiZ INPUT */

	pinset &= ~(GPIO_MODE_MASK | GPIO_PUPD_MASK);
	pinset |= GPIO_INPUT | GPIO_FLOAT;

	/* To-Do: Mark its unuse for automatic power saving options */

	return hpm_config_gpio(pinset);
}

/****************************************************************************
 * Name: hpm_gpiowrite
 *
 * Description:
 *   Write one or zero to the selected GPIO pin
 *
 ****************************************************************************/

void hpm_gpio_write(uint32_t pinset, bool value)
{
	GPIO_Type *gpio_ptr;
	uint32_t port = (pinset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
	uint32_t pin = (pinset & GPIO_PIN_MASK) >> GPIO_PIN_SHIFT;

	switch (pinset & GPIO_CONTROLLER_MASK) {
	case GPIO0:
		gpio_ptr = HPM_GPIO0;
		break;

#if defined(HPM_FGPIO) || defined(HPM_FGPIO0)

	case FGPIO0:
#ifdef HPM_FGPIO
		gpio_ptr = HPM_FGPIO;
#endif
#ifdef HPM_FGPIO0
		gpio_ptr = HPM_FGPIO0;
#endif
		break;
#endif

#ifdef HPM_GPIO1

	case GPIO1:
		gpio_ptr = HPM_GPIO1;
		break;
#endif

#ifdef HPM_FGPIO1

	case FGPIO1:
		gpio_ptr = HPM_FGPIO1;
		break;
#endif

#ifdef HPM_PGPIO

	case PGPIO:
		gpio_ptr = HPM_PGPIO;
		break;
#endif

#ifdef HPM_BGPIO

	case BGPIO:
		gpio_ptr = HPM_BGPIO;
		break;
#endif

	default:
		break;
	}

	gpio_write_pin(gpio_ptr, port, pin, value);
}

/****************************************************************************
 * Name: hpm_gpioread
 *
 * Description:
 *   Read one or zero from the selected GPIO pin
 *
 ****************************************************************************/

bool hpm_gpio_read(uint32_t pinset)
{
	GPIO_Type *gpio_ptr;
	uint32_t port = (pinset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
	uint32_t pin = (pinset & GPIO_PIN_MASK) >> GPIO_PIN_SHIFT;
	bool ret = 0;

	switch (pinset & GPIO_CONTROLLER_MASK) {
	case GPIO0:
		gpio_ptr = HPM_GPIO0;
		break;

#if defined(HPM_FGPIO) || defined(HPM_FGPIO0)

	case FGPIO0:
#ifdef HPM_FGPIO
		gpio_ptr = HPM_FGPIO;
#endif
#ifdef HPM_FGPIO0
		gpio_ptr = HPM_FGPIO0;
#endif
		break;
#endif

#ifdef HPM_GPIO1

	case GPIO1:
		gpio_ptr = HPM_GPIO1;
		break;
#endif

#ifdef HPM_FGPIO1

	case FGPIO1:
		gpio_ptr = HPM_FGPIO1;
		break;
#endif

#ifdef HPM_PGPIO

	case PGPIO:
		gpio_ptr = HPM_PGPIO;
		break;
#endif

#ifdef HPM_BGPIO

	case BGPIO:
		gpio_ptr = HPM_BGPIO;
		break;
#endif

	default:
		break;
	}

	switch (pinset & GPIO_MODE_MASK) {
	case GPIO_OUTPUT:
		ret = gpio_get_pin_output_status(gpio_ptr, port, pin);
		break;

	case GPIO_INPUT:
	default:
		ret = gpio_read_pin(gpio_ptr, port, pin);
		break;
	}

	return ret;
}

/****************************************************************************
 * Name: hpm_gpio_interrupt
 *
 * Description:
 *   gpio interrupt.
 *
 ****************************************************************************/

static int hpm_gpio_interrupt(int irq, void *context, void *arg)
{
	int ret = 0;
	struct gpio_callback_s *cb = (struct gpio_callback_s *)arg;

	for (int i = 0; i < 32; i++) {
		if (cb[i].ptr != NULL) {
			if (gpio_check_pin_interrupt_enabled(cb[i].ptr, cb[i].port, i)
			    && gpio_check_pin_interrupt_flag(cb[i].ptr, cb[i].port, i)) {
				gpio_clear_pin_interrupt_flag(cb[i].ptr, cb[i].port, i);

				if (cb[i].callback != NULL) {
					xcpt_t callback = cb[i].callback;
					void *cbarg = cb[i].arg;
					int tmp;

					tmp = callback(irq, context, cbarg);

					if (tmp < 0) {
						ret = tmp;
					}
				}
			}
		}
	}

	return ret;
}

/****************************************************************************
 * Name: hpm_gpio_setevent
 *
 * Description:
 *   Sets/clears GPIO based event and interrupt triggers.
 *
 * Input Parameters:
 *  - pinset:      GPIO pin configuration
 *  - risingedge:  Enables interrupt on rising edges
 *  - fallingedge: Enables interrupt on falling edges
 *  - event:       Generate event when set
 *  - func:        When non-NULL, generate interrupt
 *  - arg:         Argument passed to the interrupt callback
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure indicating the
 *   nature of the failure.
 *
 ****************************************************************************/

int hpm_gpio_setevent(uint32_t pinset, bool risingedge, bool fallingedge, bool event, xcpt_t func, void *arg)
{
	irqstate_t flags;
	int irq = -1;
	struct gpio_callback_s *cb;
	GPIO_Type *ptr;
	uint32_t port = (pinset & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
	uint32_t pin = (pinset & GPIO_PIN_MASK) >> GPIO_PIN_SHIFT;

	switch (pinset & GPIO_CONTROLLER_MASK) {
	case GPIO0:
		ptr = HPM_GPIO0;
		break;

#ifdef HPM_GPIO1

	case GPIO1:
		ptr = HPM_GPIO1;
		break;
#endif

#if defined(HPM_FGPIO) || defined(HPM_FGPIO0)

	case FGPIO0:
		return -1; /* Don't support IRQ */
#endif

#ifdef HPM_FGPIO1

	case FGPIO1:
		return -1; /* Don't support IRQ */
#endif

#ifdef HPM_PGPIO

	case PGPIO:
		ptr = HPM_PGPIO;
		break;
#endif

#ifdef HPM_BGPIO

	case BGPIO:
		ptr = HPM_BGPIO;
		break;
#endif

	default:
		return -1;
	}

	if (ptr == HPM_GPIO0) {
		switch (port) {
#ifdef HPM_IRQn_GPIO0_A

		case 0:
			irq = HPM_IRQn_GPIO0_A;
			cb = &g_gpioa_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO0_B

		case 1:
			irq = HPM_IRQn_GPIO0_B;
			cb = &g_gpiob_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO0_C

		case 2:
			irq = HPM_IRQn_GPIO0_C;
			cb = &g_gpioc_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO0_D

		case 3:
			irq = HPM_IRQn_GPIO0_D;
			cb = &g_gpiod_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO0_E

		case 4:
			irq = HPM_IRQn_GPIO0_E;
			cb = &g_gpioe_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO0_F

		case 5:
			irq = HPM_IRQn_GPIO0_F;
			cb = &g_gpiof_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO0_X

		case 13:
			irq = HPM_IRQn_GPIO0_X;
			cb = &g_gpiox_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO0_Y

		case 14:
			irq = HPM_IRQn_GPIO0_Y;
			cb = &g_gpioy_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO0_Z

		case 15:
			irq = HPM_IRQn_GPIO0_Z;
			cb = &g_gpioz_callbacks[0];
			break;
#endif

		default:
			return -1;
		}
	}

#ifdef HPM_GPIO1

	else if (ptr == HPM_GPIO1) {
		switch (port) {
#ifdef HPM_IRQn_GPIO1_A

		case 0:
			irq = HPM_IRQn_GPIO1_A;
			cb = &g_gpioa_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO1_B

		case 1:
			irq = HPM_IRQn_GPIO1_B;
			cb = &g_gpiob_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO1_C

		case 2:
			irq = HPM_IRQn_GPIO1_C;
			cb = &g_gpioc_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO1_D

		case 3:
			irq = HPM_IRQn_GPIO1_D;
			cb = &g_gpiod_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO1_E

		case 4:
			irq = HPM_IRQn_GPIO1_E;
			cb = &g_gpioe_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO1_F

		case 5:
			irq = HPM_IRQn_GPIO1_F;
			cb = &g_gpiof_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO1_X

		case 13:
			irq = HPM_IRQn_GPIO1_X;
			cb = &g_gpiox_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO1_Y

		case 14:
			irq = HPM_IRQn_GPIO1_Y;
			cb = &g_gpioy_callbacks[0];
			break;
#endif
#ifdef HPM_IRQn_GPIO1_Z

		case 15:
			irq = HPM_IRQn_GPIO1_Z;
			cb = &g_gpioz_callbacks[0];
			break;
#endif

		default:
			return -1;
		}
	}

#endif
#ifdef HPM_IRQn_PGPIO

	else if (ptr == HPM_PGPIO) {
		switch (port) {
		case 14:
			irq = HPM_IRQn_PGPIO;
			cb = &g_gpioy_callbacks[0];
			break;

		default:
			return -1;
		}
	}

#endif
#ifdef HPM_IRQn_BGPIO

	else if (ptr == HPM_BGPIO) {
		switch (port) {
		case 15:
			irq = HPM_IRQn_BGPIO;
			cb = &g_gpioz_callbacks[0];
			break;

		default:
			return -1;
		}
	}

#endif

	if (irq < 0) {
		return -1;
	}

	/* Get the previous GPIO IRQ handler; Save the new IRQ handler. */
	cb[pin].callback = func;
	cb[pin].arg = arg;
	cb[pin].ptr = ptr;
	cb[pin].port = port;

	flags = enter_critical_section();

	if (risingedge && !fallingedge) {
		gpio_config_pin_interrupt(ptr, port, pin, gpio_interrupt_trigger_edge_rising);

	} else if (!risingedge && fallingedge) {
		gpio_config_pin_interrupt(ptr, port, pin, gpio_interrupt_trigger_edge_falling);
	}

#if defined(GPIO_SOC_HAS_EDGE_BOTH_INTERRUPT) && (GPIO_SOC_HAS_EDGE_BOTH_INTERRUPT == 1)

	else if (risingedge && fallingedge) {
		gpio_config_pin_interrupt(ptr, port, pin, gpio_interrupt_trigger_edge_both);
	}

#endif

	hpm_config_gpio(pinset);

	gpio_clear_pin_interrupt_flag(ptr, port, pin);

	if (func) {
		irq_attach(irq, hpm_gpio_interrupt, (void *)cb);
		gpio_enable_pin_interrupt(ptr, port, pin);
		up_enable_irq(irq);

	} else {
		cb[pin].ptr = NULL;
		gpio_disable_pin_interrupt(ptr, port, pin);

		bool disirq = true;

		for (int i = 0; i < 32; i++) {
			if (cb[i].callback != NULL) {
				disirq = false;
				break;
			}
		}

		if (disirq) {
			up_disable_irq(irq);
		}
	}

	leave_critical_section(flags);

	return OK;
}
