[BITS 64]

Section .text

extern Main ; Main function in 02.Kernel64/Main.c

START:
    mov ax, 0x10  ; IA-32e mode's data segment; def is in 32 bit EntryPoint.s
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; address from 0x600000~0x6FFFFF (1MiB) is for stack
    mov ss, ax
    mov rsp, 0x6FFFF8
    mov rbp, 0x6FFFF8

    ; Unlike 32 bit EntryPoint.s, here I show there is another way to execute
    ; code in C language
    call Main

    jmp $  ; this instruction will not be executed 