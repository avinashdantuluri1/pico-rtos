# pico-rtos

A small real-time operating system kernel for the ARM Cortex-M33 (Raspberry Pi RP2350, Pico 2 W),
written from scratch in C and ARM assembly. No vendor OS, HAL or SDK: the boot code (vector table, image
definition block, clock setup), linker script, kernel and drivers are all in this repo and access hardware
registers directly. Only `arm-none-eabi-gcc`, cmake/ninja and `picotool` (to make the UF2) are needed.

## Planned features
- SysTick-driven priority scheduler
- PendSV context switching (ARM assembly)
- Per-task stacks with overflow detection (Cortex-M33 PSPLIM)
- Semaphores, message queues, mutexes with priority inheritance
- Register-level UART driver and a shell for live task inspection
- I2C and SPI drivers (BMP280 sensor)
- Measured context-switch time and interrupt latency (logic analyzer)

## Build
Requires cmake, ninja, picotool and the Arm GNU Toolchain (`arm-none-eabi-gcc`).

```bash
cmake -G Ninja -B build .
ninja -C build
```

Hold BOOTSEL while plugging in the board, then copy `build/rtos_app.uf2` to the `RP2350` drive.

## Roadmap
| # | Milestone | Status |
|---|---|---|
| 0 | Build setup, SysTick blink via raw registers | Builds; not yet run on hardware |
| 1 | Flash and blink on hardware | |
| 2 | Two tasks + PendSV context switch | Builds; not yet run on hardware |
| B | Own boot code, no SDK (12 MHz crystal clock; PLL to 150 MHz still to do) | Builds; not yet run on hardware |
| 3 | Priorities, sleep, idle task | Builds; not yet run on hardware |
| 4 | Semaphores, queues, mutex + priority inheritance | Builds; not yet run on hardware |
| 5 | UART driver + shell | |
| 6 | I2C / SPI BMP280 drivers | |
| 7 | Stack overflow detection | |
| 8 | Logic-analyzer measurements | |

## Results
Measurements will be added here once Milestone 8 is complete.

## License
MIT
