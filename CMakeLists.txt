cmake_minimum_required(VERSION 3.13)

# Bare-metal build with no vendor SDK: boot code, linker script, kernel and
# drivers are all in this repo.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(pico_rtos C ASM)
set(CMAKE_C_STANDARD 11)

set(CPU_FLAGS -mcpu=cortex-m33+nofp -mthumb -mfloat-abi=soft)
add_compile_options(${CPU_FLAGS} -Os -g -Wall -Wextra -ffreestanding
    -ffunction-sections -fdata-sections -fno-tree-loop-distribute-patterns)
add_link_options(${CPU_FLAGS} -nostdlib -Wl,--gc-sections
    -T ${CMAKE_SOURCE_DIR}/boot/link.ld -Wl,-Map=rtos_app.map)

add_executable(rtos_app
    boot/startup.c
    app/main.c
    kernel/kernel.c
    kernel/context_switch.S
    kernel/sync.c
    drivers/gpio.c
)
target_include_directories(rtos_app PRIVATE boot kernel drivers)
set_target_properties(rtos_app PROPERTIES SUFFIX .elf)

# Pico 2 W expects a UF2 with the RP2350 ARM-Secure family ID.
add_custom_command(TARGET rtos_app POST_BUILD
    COMMAND picotool uf2 convert $<TARGET_FILE:rtos_app> rtos_app.uf2 --family rp2350-arm-s
    COMMENT "Creating rtos_app.uf2")
