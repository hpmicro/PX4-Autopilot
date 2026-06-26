#pragma once

#include "../../../hpm_common/include/px4_arch/micro_hal.h"


#define HPM_L1C_CACHELINE_SIZE (64)
#define PX4_SOC_ARCH_ID            0x200A
#define PX4_NUMBER_I2C_BUSES        4


#if defined(USE_XPI_MTD)
#  define HAS_FLEXSPI
#endif
