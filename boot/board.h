#ifndef BOARD_H
#define BOARD_H

// After boot/startup.c runs, clk_sys is driven directly by the 12 MHz crystal.
// (PLL to 150 MHz is a later step once this is verified on hardware.)
#define SYS_CLK_HZ 12000000u

#endif
