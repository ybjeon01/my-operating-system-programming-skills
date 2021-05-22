; macro:
;  FLOPPY144 : read sectors up to 18 instead of 36

[ORG 0x00]
[BITS 16]

SECTION .text

; initialize code segment before running code
jmp 0x07C0:START

; env for MINT64OS
TOTALSECTORCOUNT: dw 1 ; size of MINT64OS image is 1 sectors
                       ; Ch06 EntryPoint.S size is 1 sector

; code section
START:

    ; use ds for accessing variables such as MESSAGE1
    mov ax, 0x07C0
    mov ds, ax

    ; use es for accessing video memory
    mov ax, 0xB800
    mov es, ax

    ; set regs for a stack from 0x0000:0000 to 0x0000:FFFF (64KB)
    mov ax, 0x0000
    mov ss, ax
    mov sp, 0xFFFE ; 0xFFFE is not mistake! this is for alignment
    mov bp, 0xFFFE

    ;; start of .CLEARSCREENLOOP
    mov si, 0 ; video memory addr index

; clear all screen and change text color to green
; and print start message to the top of screen
.CLEARSCREENLOOP:

    mov byte [es:si], 0x00 ; delete all letters in screen
    mov byte [es:si+1], 0x0A ; black background/ green color

    add si, 2

    cmp si, 80 * 25 * 2 ; 80 columns, 25 rows, 2 bytes 
    jl .CLEARSCREENLOOP

    mov si, 0
    mov di, 0

    ; print welcome message
    push MESSAGE1     ; addr of message
    push 0            ; y coordinate
    push 0            ; x coordinate
    call PRINTMESSAGE
    add sp, 6         ; according to cdecl coordinate, remove params from stack

    ; print message for loading OS on the second line
    push IMAGELOADINGMESSAGE
    push 1
    push 0
    call PRINTMESSAGE
    add sp, 6

; Before loading OS, reset disk by using BIOS I/O service
RESETDISK:
    ; call BIOS reset function
    mov ax, 0 ; service num: 0 (reset)
    mov dl, 0 ; drive num: 0 (floppy)
    int 0x13

    jc HANDLEDISKERROR

    ;; start of READDATA

    ; 0x10000 is start addr of OS in memory
    ; regs for reading data: dest_addr
    mov si, 0x1000
    mov es, si
    mov bx, 0x0000

    ; reg for reading data: count
    mov di, word [TOTALSECTORCOUNT]

READDATA:
    ; check if there is no more sectors to read
    cmp di, 0
    je READEND

    sub di, 0x01

    ; call BIOS I/O service
    mov ah, 0x02                ; service number (read sector)
    mov al, 0x01                ; number of sectors to read
    mov ch, byte [TRACKNUMBER]
    mov cl, byte [SECTORNUMBER]
    mov dh, byte [HEADNUMBER]
    mov dl, 0x00                ; drive to read (0=Floppy)
    int 0x13                    ; interrupt to execute service

    jc HANDLEDISKERROR    

    ; set next dest_addr: after 512 byte from prev dest
    add si, 0x0020
    mov es, si

    ; set sector, head, track numbers to next src_addr
    ; floppy sector range: 1~18 in 1.44MB and 1~36 in 2.88MB
    ; floppy head range: 0~1
    ; floppy track(cylinder) range: 0~79
    mov al, byte [SECTORNUMBER]
    add al, 0x01
    mov byte [SECTORNUMBER], al

    %if FLOPPY == 144
        cmp al, 19 ; for 1.44MB
    %elif FLOPPY == 288
        cmp al, 37 ; for 2.88MB floppy which is default of QEMU
    %else
        %error "FLOPPY should be 144 or 288"
    %endif

    jl READDATA

    xor byte [HEADNUMBER], 0x01
    mov byte [SECTORNUMBER], 0x01
    cmp byte [HEADNUMBER], 0x00
    jne READDATA

    add byte [TRACKNUMBER], 0x01
    jmp READDATA

READEND:

    ; print loading complete message at (1,20)
    push LOADINGCOMPLETEMESSAGE
    push 1
    push 20
    call PRINTMESSAGE
    add sp, 6

    ;; start of executing loaded OS
    jmp 0x1000:0x0000

;; From here, functions are defined except HANDLEDISKERROR

; print error message and run infinte loop
HANDLEDISKERROR:
    ; print error message at (1, 20)
    push DISKERRORMESSAGE
    push 1
    push 20
    call PRINTMESSAGE

    jmp $

; function signature in C: PrintMessage(iX, iY, pcString)
; follow cdecl calling convention
PRINTMESSAGE:
    ; set env for current function: new stack, empty regs
    ; all values will be restored at the end
    push bp
    mov bp, sp

    push es
    push si
    push di
    push ax
    push cx
    push dx
    ; end of setting env

    ; seg for video memory address
    mov ax, 0xB800
    mov es, ax

    ; calcuate video address for x and y coordinates
    ; y coordinate
    mov ax, word [bp+6] ; short iY
    mov si, 160         ; 80 * 2 (row * char size of text mode)
    mul si              ; iY * 160
    mov di, ax

    ; x coordinate
    mov ax, word [bp+4] ; short iX
    mov si, 2           ; 2 (char size of text mode)
    mul si              ; iX * 2
    add di, ax          ; di == iY * 160 + iX * 2

    mov si, word [bp+8] ; char *pcString

.MESSAGELOOP:
    mov cl, byte [si]

    ; if string ends with 0, then get out of the loop
    ; I defined every string to ends with 0
    cmp cl, 0
    je .MESSAGEEND

    mov byte [es:di], cl

    add si, 1
    add di, 2

    jmp .MESSAGELOOP

.MESSAGEEND:
    ; pop in the reverse order of push because stack is Last-In, First-out
    pop dx
    pop cx
    pop ax
    pop di
    pop si
    pop es
    pop bp
    ret ; go back to calling address

;; Start of data section

MESSAGE1: db 'Mint64 OS Boot Loader Start~!!', 0 ; welcome message

; error message when failed to reset disk or to read data from disk
DISKERRORMESSAGE: db 'DISK Error~!!', 0

; message to print while loading
IMAGELOADINGMESSAGE: db 'OS Image Loading...', 0

; message to print when finished loading OS
LOADINGCOMPLETEMESSAGE: db 'Complete~!!', 0

; disk related variables
; vars for reading data: src_addr
SECTORNUMBER: db 0x02 ; os image starts from 0x02 sector
HEADNUMBER: db 0x00
TRACKNUMBER: db 0x00

; add padding between above code and MBR signature
times 510 - ($ - $$) db 0x00

; MBR signature
db 0x55
db 0xAA