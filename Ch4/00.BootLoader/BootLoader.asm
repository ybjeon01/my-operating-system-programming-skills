[ORG 0x00]
[BITS 16]

SECTION .text

; initialize all segment before running code
jmp 0x07C0:START

START:
    ; use ds for accessing variables such as MESSAGE1
    mov ax, 0x07C0
    mov ds, ax

    ; use es for accessing video memory
    mov ax, 0xB800
    mov es, ax

    mov si, 0;

; clear all screen
.CLEARSCREENLOOP:

    mov byte [es:si], 0x00
    mov byte [es:si+1], 0x0A ; black background/ green color

    add si, 2

    cmp si, 80 * 25 * 2 ; 80 columns, 25 rows, 2 bytes 
    jl .CLEARSCREENLOOP

    mov si, 0
    mov di, 0

; print message 
.MESSAGELOOP:

    mov cl, byte [si + MESSAGE1]

    cmp cl, 0

    je .ENDMESSAGELOOP

    mov byte [es:di], cl

    add si, 1
    add di, 2

    jmp .MESSAGELOOP

.ENDMESSAGELOOP:

    jmp $

MESSAGE1: db 'Hello, Mint64OS', 0

times 510 - ($ - $$) db 0x00

db 0x55
db 0xAA