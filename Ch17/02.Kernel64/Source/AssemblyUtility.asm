[BITS 64]

SECTION .text

; I/O port related functions
global kInPortByte, kOutPortByte

; GDT, IDT, and TSS related functions
global kLoadGDTR, kLoadTR, kLoadIDTR

; interrupt related functions
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS

;Time Stamp Counter related functions
global kReadTSC


;; I/O port related functions

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


;; GDT, IDT, and TSS related functions

; function that loads GDTR address to register
; param: 
;   qwGDTRAddress: address of GDTR
kLoadGDTR:
    lgdt [rdi]
    ret

; function that loads TSS descriptor offset to TR register
; param:
;   wTSSSegmentOffset: TSS descriptor offset in GDT
kLoadTR:
    ltr di;
    ret

; function that loads IDTR address to register
; param:
;   qwIDTRAddress: address of IDTR
kLoadIDTR:
    lidt [rdi]
    ret


;; interrupt related functions

; function that activates interrupt
kEnableInterrupt:
    sti
    ret

; function that deactivates interrupt
kDisableInterrupt:
    cli
    ret

; function that returns RFLAGS
kReadRFLAGS:
    pushfq   ; push RFLAGS to stack
    pop rax
    ret


;; Time Stamp Counter related functions

; returns time stamp counter
; return:
;   rax: time stamp counter of QWORD size 
kReadTSC:
    push rdx;
    rdtsc       ; save tsc at EDX:EAX (high:low)

    shl rdx, 32 ; save tsc at RAX 
    or rax, rdx

    pop rdx
    ret 