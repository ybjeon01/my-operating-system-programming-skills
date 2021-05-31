[BITS 64]

SECTION .text

; read a byte from port
; or write a byte to port
global kInPortByte, kOutPortByte

; function that reads one byte from port
; this function follows IA-32e mode function convention
; param:
;   port address (word): memory address where data is stored
; return:
;   a byte value from the port I/O address
kInPortByte:
    push rdx
    mov rax, 0	 ; initialize register to zero
    mov rdx, rdi ; move first parameter (port addr) to rax
    in al, dx    ; read byte

    pop rdx
    ret


; function that writes one byte to port
; this function follows IA-32e mode function convention
; param:
;   port address (word): I/O port address to write data
;   data (byte): data to write
kOutPortByte:
    push rdx
    push rax

    mov rdx, rdi ; move first parameter (port addr) to rdi
    mov rax, rsi ; move second parameter (data) to rax
    out dx, al   ; write byte

    pop rax
    pop rdx
    ret