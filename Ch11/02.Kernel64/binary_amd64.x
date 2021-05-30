OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386:x86-64)

SECTIONS {
    /* long mode code resides at 0x200000
     * Instead of concatenating EntryPoint.bin with Kernel.bin, EntryPoint is
     * also involved in the linking process
     */ 

    .text 0x200000 : {
        /* text section of Main file comes first in file*/
        EntryPoint.o(.text)
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

    /* discard all other sections in input files. If this is not specified,
     * ld adds the sections somewhere in the program although instruction to
     * add the sections is not in the script
     */
    /DISCARD/ : {
        *(*)
    }
}