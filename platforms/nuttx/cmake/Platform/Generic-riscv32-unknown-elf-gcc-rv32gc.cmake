
if(CONFIG_ARCH_DPFPU)
	message(STATUS "Enabling double FP precision hardware instructions")
	set(cpu_flags "-march=rv32gc -mabi=lp32d -mcmodel=medany")
else()
	set(cpu_flags "-march=rv32imac -mabi=lp32 -mcmodel=medany")
endif()

set(CMAKE_C_FLAGS "${cpu_flags}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${cpu_flags}" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS "${cpu_flags} -D__ASSEMBLY__" CACHE STRING "" FORCE)
