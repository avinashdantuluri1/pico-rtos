cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)

include(pico_sdk_import.cmake)
project(pico_rtos C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

# Pico SDK is used only for the build system, boot code and linker script.
# The kernel and drivers talk to hardware registers directly.
add_executable(rtos_app
    app/main.c
)
target_include_directories(rtos_app PRIVATE kernel drivers)
target_link_libraries(rtos_app pico_runtime pico_standard_link)
pico_add_extra_outputs(rtos_app)
