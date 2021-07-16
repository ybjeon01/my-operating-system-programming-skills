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

; context switch related function
global kSwitchContext

; Processor Halt related functions
global kHlt

; MUTEX related functions
global kTestAndSet

;FPU related functions
global kInitializeFPU, kSaveFPUContext, kLoadFPUContext, kSetTS, kClearTS


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


;; context switch related macros and functions


; macro that stores registers to task CONTEXT array
; it is required to set CONTEXT array as stack
; and push SS, RSP, RFLAGS, CS, RIP first before
; using this macro
; you can see similar macro in ISR.asm
%macro KSAVECONTEXT 0
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    mov ax, ds
    push rax
    mov ax, es
    push rax
    push fs
    push gs 
%endmacro


; macro that restore registers to task CONTEXT array
; it is required to set CONTEXT array as stack
; and the array shoud have registers pushed by
; KSAVECONTEXT macro
; you can see similar macro in ISR.asm
%macro KLOADCONTEXT 0
    pop gs
    pop fs
    pop rax
    mov es, ax
    pop rax
    mov ds, ax
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp        
%endmacro


; save current task context to task data structure
; and load next task context to run
; params:
;   pstCurrentContext: pointer where current context will be stored
;   pstNextContext: pointer to context to load
; info:
;   this looks like a function. However, it works slight different way.
;   this code must be called, but it never returns. and it preserves
;   registers instead of pushing to stack, so it can store the registers
;   of the task
kSwitchContext:
    push rbp
    mov rbp, rsp    ; use rbp as a base to point return address

    pushfq      ; cmp can modify RFLAGS, so push it before cmp
    cmp rdi, 0
    je .LoadContext ; if pstCurrentContet is empty
    popfq


    ;; save current task context

    push rax    ; rax reigster is used as temp variable
    
    mov ax, ss
    mov qword [rdi + (23 * 8)], rax

    ; save rsp of caller before calling this func
    ; 16 is rbp + return addr in stack
    mov rax, rbp
    add rax, 16
    mov qword [rdi + (22 * 8)], rax

    ; save rflags
    pushfq
    pop rax
    mov qword [rdi + (21 * 8)], rax

    ; save cs
    mov ax, cs
    mov qword [rdi + (20 * 8)], rax

    ; save instruct pointer that points after this func
    mov rax, qword [rbp + 8]
    mov qword [rdi + (19 * 8)], rax


    ;; prepare before calling KSAVECONTEXT
    ;; In other words, leave all registers like before calling func

    pop rax
    pop rbp

    ; change rsp that points to current context
    add rdi, (19 * 8)
    mov rsp, rdi
    sub rdi, (19 * 8)

    KSAVECONTEXT

.LoadContext:
    ; set rsp to points next context
    mov rsp, rsi

    KLOADCONTEXT
    ; restore 5 items in the top of stack
    ; which are SS, RSP, RFLAGS, CS, RIP
    ; and instead of returning, execute code in CS:IP 
    iretq


;; Processor Halt related functions

; stop processor executing instructions until signals happen
kHlt:
    hlt
    hlt
    ret


;; MUTEX related functions

; test a value in destination memory with compare value. and if they are
; the same value, move the value in bSource to destination memory
; params:
;   pbDestination: pointer to a value which you may want to change
;   bCompare: a value to compare with a value in pbDestination
;   bSource: a value to put if the compared values are the same
kTestAndSet:
    mov rax, rsi ; move second parameter (bCompare) to rax

    ; if value in rax is the same with the value in [rdi]
    ; move the value in dl (third param) to [rdi] and 
    ; set ZF to 1
    lock cmpxchg byte [rdi], dl

    je .SUCCESS

    .NOTSAME:
        mov rax, 0x00 ; BOOL FALSE
        ret

    .SUCCESS:
        mov rax, 0x01 ; BOOL TRUE
        ret


;; FPU related functions

; initialize registers in FPU device
; info:
;   this initialization is for tasks that use FPU device, so it is necessary
;   that each task calls this function.
kInitializeFPU:
    finit
    ret


; save FPU context to TCB
; params:
;   pvFPUContext: FPU context in a TCB
; info:
;    address of pvFPUContext must be aligned by 16 bytes
kSaveFPUContext:
    fxsave [rdi]
    ret

; load FPU context of a TCB to FPU device
; params:
;   pvFPUContext: FPU context in a TCB
; info:
;    address of pvFPUContext must be aligned by 16 bytes
kLoadFPUContext:
    fxrstor [rdi]
    ret


; set TS bit in CR0
; info:
;   if TS bit is set, using FPU device causes exception.
;   MINT64OS is responsible to switch FPU context and clear
;   the bit
kSetTS:
    push rax
    mov rax, cr0
    or rax, 0x08    ; set TS (bit 7) to 1
    mov cr0, rax

    pop rax
    ret

; clear TS bit in CR0
; info:
;   if TS bit is set, using FPU device causes exception.
;   MINT64OS is responsible to switch FPU context and clear
;   the bit
kClearTS:
    clts    ; instruction that clears ts bit
    ret