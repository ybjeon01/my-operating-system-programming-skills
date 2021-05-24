OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)

SECTIONS {
    .text 0x10200 : {
        Main.o(.text)
    }
    .rodata : {
        *(.rodata)
        . = ALIGN(512);
    }

    /DISCARD/ : {
        *(*)
    }
}
