OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)

SECTIONS {
    /* code resides at 0x10200 */
    .text 0x10200 : {
        /* text section of Main file comes first in file*/
        Main.o(.text)
        /* text section of other files comes after main in the order of input */
        *(.text)
    }
    . = ALIGN(512);

    /* as .rodata data comes here, operand of instruction that references to a
     * data in .rodata is adjusted
     */
    .rodata : {
        *(.rodata)

        /* add padding to the end so .rodata is aligned to multiply of 512 */
    }
    . = ALIGN(512);
    .data : {
        *(.data)
    }
    . = ALIGN(512);
    .bss : {
        *(.bss)
    }

    /* discard all other sections in input files. If this is not specified,
     * ld adds the sections somewhere in the program although instruction to
     * add the sections is not in the script
     */
    /DISCARD/ : {
        *(*)
    }
}