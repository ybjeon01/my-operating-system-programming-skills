[ORG 0x00]
[BITS 16]

SECTION .text

START:
    ;; get memory map
    ;; memory entry count (DWORD) at 0x20000
    ;; memory netries (QWORD+QWORD+dWORD) at 0x20004
    mov ax, 0x2000
    mov ds, ax
    mov es, ax

    mov eax, mmap_ent
    mov [ds:mmap_ent], eax ;; initialize size to 0
    call do_e820

    ;; end of calling E820 BIOS service


    mov ax, 0x1000
    mov ds, ax
    mov es, ax

    ;; A20 Gate activation
    ; activate A20 by using BIOS service, if it fails
    ; try activating by using system control port
    mov ax, 0x2401 ; 0x2401: activate
    int 0x15       ; 0x15: miscellaneous system services

    jc .A20GATEERROR ; if cflag is set, it fails
    jmp .A20GATESUCCESS

    .A20GATEERROR:
        in al, 0x92  ; 0x92: system control port
        or al, 0x02  ; set A20 Gate bit (set bit 1)
        and al, 0xFE ; prevent reset (clear bit 0)
        out 0x92, al

    .A20GATESUCCESS:
        cli ; ignore interrupt until interrupt handler is set

        lgdt [GDTR]
        
            ; PG=0, CD=1, NW=0, AM=0, WP=0, NE=0, ET=1, EM=0, MP=1, PE=1
            ; CD means cache disable
            ; PE is protection enable    
            ; EM, ET, MP, PE are FPU-related fields and they are set
            ; although their feature is not used in IA-32 mode. they will
            ; be explained in detail later. Don't worry about the fields now
            mov eax, 0x4000003B
            mov cr0, eax

            ; 0x18 is offset to the protected mode's code segment descriptor
            ; from GDT PROTECTEDMODE - $$ + 0x10000 is offset to the code from
            ; segment
            jmp dword 0x18: (PROTECTEDMODE - $$ + 0x10000)


; use the INT 0x15, eax= 0xE820 BIOS function to get a memory map
; note: initially di is 0, be sure to set it to a value so that the 
; BIOS code will not be overwritten. The consequence of overwriting
; the BIOS code will lead to problems like getting stuck in `int 0x15`
;
; inputs: es:di -> destination buffer for 24 byte entries
; outputs: bp = entry count, trashes all registers except esi
mmap_ent equ 0x0000     ; the number of entries will be stored at 0x20000
do_e820:
    mov di, 0x0004      ; Set di to 0x20004. 
    xor ebx, ebx        ; ebx must be 0 to start
    mov edx, 0x0534D4150    ; Place "SMAP" into edx
    mov eax, 0xe820
    mov [es:di + 20], dword 1   ; force a valid ACPI 3.X entry
    mov ecx, 24	    ; ask for 24 bytes
    int 0x15
    jc short .failed    ; carry set on first call means "unsupported function"
    mov edx, 0x0534D4150    ; Some BIOSes apparently trash this register?
    cmp eax, edx        ; on success, eax must have been reset to "SMAP"
    jne short .failed
    test ebx, ebx       ; ebx = 0 implies list is only 1 entry long (worthless)
    je short .failed
    jmp short .jmpin
.e820lp:
    mov eax, 0xe820     ; eax, ecx get trashed on every int 0x15 call
    mov [es:di + 20], dword 1   ; force a valid ACPI 3.X entry
    mov ecx, 24     ; ask for 24 bytes again
    int 0x15
    jc short .e820f     ; carry set means "end of list already reached"
    mov edx, 0x0534D4150    ; repair potentially trashed register
.jmpin:
    jcxz .skipent       ; skip any 0 length entries
    cmp cl, 20          ; got a 24 byte ACPI 3.X response?
    jbe short .notext
    test byte [es:di + 20], 1
    je short .skipent
.notext:
    mov ecx, [es:di + 8]     ; get lower uint32_t of memory region length
    or ecx, [es:di + 12]    ; "or" it with upper uint32_t to test for zero
    jz .skipent     ; if length uint64_t is 0, skip entry

    mov eax, dword [mmap_ent]
    add eax, 1
    mov dword [mmap_ent], eax

    add di, 24
.skipent:
    test ebx, ebx       ; if ebx resets to 0, list is complete
    jne short .e820lp
.e820f:
    clc     ; there is "jc" on end of list to this point, so the carry must be cleared
    ret
.failed:
    stc     ; "function unsupported" error exit
    ret


[BITS 32]
PROTECTEDMODE:
    ; set code and data segment
    mov ax, 0x20 ; 0x20 = offset to IA-32 mode's data segment descriptor
    mov ds, ax
    mov es, ax ; it is not required to set es, fs, and gs
    mov fs, ax
    mov gs, ax

    ; set stack segment
    ; because bootloader is not used anymore stack is from 0x0C700 ~ 0xFFFE0
    mov ss, ax ; data segment is also used as stack segment
    mov esp, 0xFFFE
    mov ebp, 0xFFFE

    ;; print message
    
    ; printMessage(success_message, 2, 0)
    push (SWITCHSUCCESSMESSAGE - $$ + 0x10000)
    push 2
    push 0
    call PRINTMESSAGE
    add esp, 12 ; 4 bytes (Protected Mode) * 3

    ; 0x18 is offset to IA-32's code descriptor
    jmp dword 0x18:0x10200

; function signature in C: PrintMessage(iX, iY, pcString)
; follow cdecl calling convention
; function for Protected Mode
PRINTMESSAGE:
    ; set env for current function: new stack, empty regs
    ; all values will be restored at the end
    push ebp
    mov ebp, esp
    push esi
    push edi
    push eax
    push ecx
    push edx
    ; end of setting env

    ; calculate video address for x and y coordinates
    ; y coordinate
    mov eax, dword [ebp + 12] ; short iY
    mov esi, 160              ; 80 * 2 (row * char size of text mode)
    mul esi                   ; iY * 160
    mov edi, eax

    ; x coordinate
    mov eax, dword [ebp + 8] ; short iX
    mov esi, 2               ; 2 (char size of text mode)
    mul esi                  ; iX * 2
    add edi, eax             ; edi == iY * 160 + iX * 2

    mov esi, dword [ebp + 16] ; char* pcString

.MESSAGELOOP:
    mov cl, byte [esi]

    ; if string ends with 0, then get out of the loop
    ; I defined every string to ends with 0
    cmp cl, 0
    je .MESSAGEEND

    ; Unlike print function in real code
    ; 0xB0000 is directly added to effective
    ; address  
    mov byte [edi + 0xB8000], cl
    add edi, 2
    add esi, 1

    jmp .MESSAGELOOP

.MESSAGEEND:
    ; pop in the reverse order of push because stack is Last-In, First-out
    pop edx
    pop ecx
    pop eax
    pop edi
    pop esi
    pop ebp
    ret ; go back to calling address

; enforce data after this instruction on memory address that is multiply of 8
align 8, db 0

; Because GDTR 6 bytes data structure, add 2 bytes to align memory layout
dw 0x0000

; GDTR data structure
GDTR:
    dw GDTEND - GDT - 1 ; GDTR size starts from 0 which count as 1 descriptor
    dd (GDT - $$  + 0x10000) ; this entry.S starts at 0x10000

GDT:
    ; first descriptor must be null descriptor
    NULLDescriptor:
        dw 0x0000
        dw 0x0000
        db 0x0000
        db 0x00
        db 0x00
        db 0x00

    ; IA-32e mode code descriptor for kernel
    IA_32eCODEDESCRIPTOR:
        ; BASE: 0x00000000
        ; LIMIT: 0xFFFFF
        ; P=1, DPL=0, Code Segment, Execute/Read
        ; G=1, D=0, L=1
        dw 0xFFFF ; Limit (16:0]
        dw 0x0000 ; Base (32:16)
        db 0x00   ; Base (40:32]
        db 0x9A   ; P/DPL/S/Type (48:40]
        db 0xAF   ; G/D/L/LIMIT (56:48]
        db 0x00   ; BASE (64:56]

    ; IA-32e mode data and stack descriptor for kernel
    IA_32eDATADESCRIPTOR:
        ; BASE: 0x00000000
        ; LIMIT: 0xFFFFF
        ; P=1, DPL=0, Data Segment, Read/Write
        ; G=1, B=0, L=1
        dw 0xFFFF ; Limit (16:0]
        dw 0x0000 ; Base (32:16)
        db 0x00   ; Base (40:32]
        db 0x92   ; P/DPL/S/Type (48:40]
        db 0xAF   ; G/D/L/LIMIT (56:48]
        db 0x00   ; BASE (64:56]

    ; protected mode's code descriptor (CS)
    CODEDESCRIPTOR:
        ; BASE: 0x00000000
        ; LIMIT: 0xFFFFF
        ; P=1, DPL=0, Code Segment, Execute/Read
        ; G=1, D=1, L=0
        dw 0xFFFF ; Limit (16:0]
        dw 0x0000 ; Base (32:16)
        db 0x00   ; Base (40:32]
        db 0x9A   ; P/DPL/S/Type (48:40]
        db 0xCF   ; G/D/L/LIMIT (56:48]
        db 0x00   ; BASE (64:56]

    ; protected mode's data and stack descriptor (DS, SS)
    DATADESCRIPTOR:
        ; BASE: 0x00000000
        ; LIMIT: 0xFFFFF
        ; P=1, DPL=0, Data Segment, Read/Write
        ; G=1, B=1, L=0
        dw 0xFFFF ; Limit (16:0]
        dw 0x0000 ; Base (32:16)
        db 0x00   ; Base (40:32]
        db 0x92   ; P/DPL/S/Type (48:40]
        db 0xCF   ; G/D/L/LIMIT (56:48]
        db 0x00   ; BASE (64:56]
GDTEND:

;; Collection of data

; success message for Switching to Protected Mode
SWITCHSUCCESSMESSAGE: db "Switch To Protected Mode Success~!!", 0

times 512 - ($ - $$) db 0x00