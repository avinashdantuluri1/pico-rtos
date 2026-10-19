/* Linker script for RP2350 (Pico 2 W): 4 MB flash via XIP, 512 KB main SRAM. */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x10000000, LENGTH = 4M
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 512K
}

ENTRY(reset_handler)

SECTIONS
{
    /* Vector table first, image-definition block right after (must be in first 4 KB) */
    .text :
    {
        KEEP(*(.vectors))
        KEEP(*(.embedded_block))
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Initialized globals live in RAM at run time but are stored in flash */
    __data_load = LOADADDR(.data);
    .data :
    {
        __data_start = .;
        *(.data*)
        . = ALIGN(4);
        __data_end = .;
    } > RAM AT > FLASH

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) }

    /* Main stack grows down from the end of RAM */
    __stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
