/*
 * Copyright (C) 2025 HPMicro
 */

#pragma once

/**
 * OS detection
 */
#ifndef UAVCAN_HPMICRO_NUTTX
# error "Only NuttX is supported"
#endif

/**
 * Number of interfaces must be enabled explicitly
 */
#if !defined(UAVCAN_HPMICRO_NUM_IFACES) || (UAVCAN_HPMICRO_NUM_IFACES > 4)
# error "UAVCAN_HPMICRO_NUM_IFACES must be set to 1, 2, 3, or 4"
#endif

/**
 * Any General-Purpose timer (TIM2, TIM3, TIM4, TIM5)
 * e.g. -DUAVCAN_HPMICRO_TIMER_NUMBER=2
 */
#ifndef UAVCAN_HPMICRO_TIMER_NUMBER
// In this case the clock driver should be implemented by the application
# define UAVCAN_HPMICRO_TIMER_NUMBER 0
#endif
