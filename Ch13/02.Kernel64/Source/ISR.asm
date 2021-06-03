;; Every ISR pass a exception/interrupt number parameter to C code and
;; some ISRs pass error number too

[BITS 64]

SECTION .text

extern kCommonExceptionHandler, kCommonInterruptHandler, kKeyboardHandler


;; 0 to 19, reserved exception/interrupt ISRs

global kISRDivideError, kISRDebug, kISRNMI, kISRBreakPoint, kISROverflow
global kISRBoundRangeExceeded, kISRInvalidOpcode, kISRDeviceNotAvailable
global kISRDoubleFault, kISRCoprocessorSegmentOverrun, kISRInvalidTSS
global kISRSegmentNotPresent, kISRStackSegmentFault, kISRGeneralProtection
global kISRPageFault, kISR15, kISRFPUError, kISRAlignmentCheck
global kISRMachineCheck, kISRSIMDError


;; 20 to 31, reserved exception/interrupt ISR
;; a dummy handler

global kISRETCException


;; 32 to 47, PIC interrupt ISRs

global kISRTimer, kISRKeyboard, kISRSlavePIC, kISRSerial2, kISRSerial1, kISRParallel2
global kISRFloppy, kISRParallel1, kISRRTC, kISRReserved, kISRNotUsed1, kISRNotUsed2
global kISRMouse, kISRCoprocessor, kISRHDD1, kISRHDD2


;; 48 to end, a dummpy handler

global kISRETCInterrupt


;; macro that switch context
;; every ISR uses this macro

; 0 means no parameter
%macro KSAVECONTEXT 0
    ;; push general registers

    push rbp
    mov rbp, rsp
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


    ;; siwtch segment selector

    mov ax, 0x10    ; 0x10: kernel data segment descriptor offset
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
%endmacro


;; macro that restores context
;; every ISR uses this macro

%macro KLOADCONTEXT 0
    ;; restore all segment selectors
    pop gs
    pop fs
    pop rax
    mov es, ax
    pop rax
    mov ds, ax

    
    ;; restore all general registers

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


;; 0 to 19, reserved exception/interrupt ISRs

; #0, Divide Error ISR
kISRDivideError:
    KSAVECONTEXT  ; switch context
    ; exception number as first parameter
    mov rdi, 0
    call kCommonExceptionHandler
    KLOADCONTEXT    ; restore general context
    iretq           ; restore context from IST stack and return

; #1, Debug ISR
kISRDebug:
    KSAVECONTEXT
    mov rdi, 1
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #2, NMI ISR
kISRNMI:
    KSAVECONTEXT
    mov rdi, 2
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #3, BreakPoint ISR
kISRBreakPoint:
    KSAVECONTEXT
    mov rdi, 4
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #4, Overflow ISR
kISROverflow:
    KSAVECONTEXT
    mov rdi, 4
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #5, Bound Range Exceeded ISR
kISRBoundRangeExceeded:
    KSAVECONTEXT
    mov rdi, 5
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #6, Invalid Opcode ISR
kISRInvalidOpcode:
    KSAVECONTEXT
    mov rdi, 6
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #7, Device not Available ISR
kISRDeviceNotAvailable:
    KSAVECONTEXT
    mov rdi, 7
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #8, Double Fault ISR
; error code as second parameter to C function
kISRDoubleFault:
    KSAVECONTEXT
    mov rdi, 8
    mov rsi, qword [rbp + 8] 
    call kCommonExceptionHandler
    KLOADCONTEXT
    add rsp, 8  ; remove error code from stack
    iretq

; #9, Coprocessor Segment Overrun ISR
kISRCoprocessorSegmentOverrun:
    KSAVECONTEXT
    mov rdi, 9
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #10, Invalid TSS ISR
; error code as second parameter to C function
kISRInvalidTSS:
    KSAVECONTEXT
    mov rdi, 10
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler
    KLOADCONTEXT
    add rsp, 8
    iretq

; #11, Segment Not Present ISR
; error code as second parameter to C function
kISRSegmentNotPresent:
    KSAVECONTEXT
    mov rdi, 11
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler
    KLOADCONTEXT
    add rsp, 8
    iretq

; #12, Stack Segment Fault ISR
; error code as second parameter to C function
kISRStackSegmentFault:
    KSAVECONTEXT
    mov rdi, 12
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler
    KLOADCONTEXT
    add rsp, 8
    iretq

; #13 General Protection ISR
; error code as second parameter to C function
kISRGeneralProtection:
    KSAVECONTEXT
    mov rdi, 13
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler
    KLOADCONTEXT
    add rsp, 8
    iretq

; #14, Page Fault ISR
; error code as second parameter to C function
kISRPageFault:
    KSAVECONTEXT
    mov rdi, 14
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler
    KLOADCONTEXT
    add rsp, 8
    iretq

; #15, Reserved ISR
kISR15:
    KSAVECONTEXT
    mov rdi, 15
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #16, FPU Error ISR
kISRFPUError:
    KSAVECONTEXT
    mov rdi, 16
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #17, Alignment Check ISR
; error code as second parameter to C function
kISRAlignmentCheck:
    KSAVECONTEXT
    mov rdi, 17
    mov rsi, qword [rbp + 8]
    call kCommonExceptionHandler
    KLOADCONTEXT
    add rsp, 8
    iretq

; #18, Machine Check ISR
kISRMachineCheck:
    KSAVECONTEXT
    mov rdi, 18
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq

; #19, SIMD Floating Point Exception ISR
kISRSIMDError:
    KSAVECONTEXT
    mov rdi, 19
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq


;; 20 to 31, handles rest of reserved exceptions from 20 to 31

; ETC Reserved ISR
kISRETCException:
    KSAVECONTEXT
    mov rdi, 20
    call kCommonExceptionHandler
    KLOADCONTEXT
    iretq


;; 32 to 47, PIC interrupt ISRs

; #32, Timer ISR
kISRTimer:
    KSAVECONTEXT
    mov rdi, 32
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #33, Keyboard ISR
kISRKeyboard:
    KSAVECONTEXT
    mov rdi, 33
    call kKeyboardHandler
    KLOADCONTEXT
    iretq

; #34, slave PIC ISR
kISRSlavePIC:
    KSAVECONTEXT
    mov rdi, 34
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #35, serial port 2 ISR
kISRSerial2:
    KSAVECONTEXT
    mov rdi, 35
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #36, serial port 1 ISR
kISRSerial1:
    KSAVECONTEXT
    mov rdi, 36
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #37, parallel port 2 ISR
kISRParallel2:
    KSAVECONTEXT
    mov rdi, 37
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #38, floppy disk controller ISR
kISRFloppy:
    KSAVECONTEXT
    mov rdi, 38
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #39, parallel port 1 ISR
kISRParallel1:
    KSAVECONTEXT
    mov rdi, 39
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #40, RTC ISR
kISRRTC:
    KSAVECONTEXT
    mov rdi, 40
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #41,PIC reserved interrupt ISR
kISRReserved:
    KSAVECONTEXT
    mov rdi, 41
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #42, PIC not used
kISRNotUsed1:
    KSAVECONTEXT
    mov rdi, 42
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #43, PIC not used
kISRNotUsed2:
    KSAVECONTEXT
    mov rdi, 43
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #44, mouse ISR
kISRMouse:
    KSAVECONTEXT
    mov rdi, 44
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #45 coprocessor ISR
kISRCoprocessor:
    KSAVECONTEXT
    mov rdi, 45
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #46, hard disk 1 ISR
kISRHDD1:
    KSAVECONTEXT
    mov rdi, 46
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq

; #47, hard disk 2 ISR
kISRHDD2:
    KSAVECONTEXT
    mov rdi, 47
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq


;; 48 to end, a dummpy handler

; ETC interrupt ISR
kISRETCInterrupt:
    KSAVECONTEXT
    mov rdi, 48
    call kCommonInterruptHandler
    KLOADCONTEXT
    iretq