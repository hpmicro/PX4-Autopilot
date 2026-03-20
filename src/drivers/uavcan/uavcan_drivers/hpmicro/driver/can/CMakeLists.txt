include_directories(
    ./include
    )

add_library(uavcan_hpmicro_can_driver STATIC
    ./src/uc_hpmicro_can.cpp
    ./src/uc_hpmicro_can_clock.cpp
    ./src/uc_hpmicro_can_thread.cpp
    )

add_dependencies(uavcan_hpmicro_can_driver uavcan)

install(DIRECTORY include/uavcan_hpmicro_can DESTINATION include)
install(TARGETS uavcan_hpmicro_can_driver DESTINATION lib)

# vim: set et ft=cmake fenc=utf-8 ff=unix sts=4 sw=4 ts=4 :)
