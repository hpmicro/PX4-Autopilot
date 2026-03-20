/*
 * Copyright (C) 2014 Pavel Kirienko <pavel.kirienko@gmail.com>
 */

#pragma once

#include <uavcan_hpmicro_can/build_config.hpp>

#if UAVCAN_HPMICRO_NUTTX
# include <nuttx/arch.h>
# include <arch/board/board.h>
# include <syslog.h>
#else
# error "Unknown OS"
#endif

/**
 * Debug output
 */
#ifndef UAVCAN_HPMICRO_LOG
// syslog() crashes the system in this context
// # if UAVCAN_HPMICRO_NUTTX && CONFIG_ARCH_LOWPUTC
# if 1
#  define UAVCAN_HPMICRO_LOG(fmt, ...)  printf(fmt "\r\n", ##__VA_ARGS__)
# else
#  define UAVCAN_HPMICRO_LOG(...)       ((void)0)
# endif
#endif

/**
 * IRQ handler macros
 */
#if UAVCAN_HPMICRO_NUTTX
# define UAVCAN_HPMICRO_IRQ_HANDLER(id)  int id(int irq, FAR void* context, FAR void *arg)
# define UAVCAN_HPMICRO_IRQ_PROLOGUE()
# define UAVCAN_HPMICRO_IRQ_EPILOGUE()    return 0;
#endif

/**
 * Glue macros
 */
#define UAVCAN_HPMICRO_GLUE2_(A, B)       A##B
#define UAVCAN_HPMICRO_GLUE2(A, B)        UAVCAN_HPMICRO_GLUE2_(A, B)

#define UAVCAN_HPMICRO_GLUE3_(A, B, C)    A##B##C
#define UAVCAN_HPMICRO_GLUE3(A, B, C)     UAVCAN_HPMICRO_GLUE3_(A, B, C)

namespace uavcan_hpmicro_can
{
#if UAVCAN_HPMICRO_NUTTX

struct CriticalSectionLocker {
	const irqstate_t flags_;

	CriticalSectionLocker()
		: flags_(enter_critical_section())
	{ }

	~CriticalSectionLocker()
	{
		leave_critical_section(flags_);
	}
};

#endif

namespace clock
{
uavcan::uint64_t getUtcUSecFromCanInterrupt();
}
}
