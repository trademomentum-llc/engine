ENTRY(_start)

SECTIONS
{
    . = 0xFFFF800000000000;

    .text : ALIGN(4096) {
        *(.text.startup)
        *(.text)
    }

    .rodata : ALIGN(4096) {
        *(.rodata)
    }

    .data : ALIGN(4096) {
        *(.data)
    }

    .bss : ALIGN(4096) {
        *(.bss)
    }
}
