[ORG 0x00]
[BITS 16]

SECTION .text

; initialize code segment before running code
jmp 0x07C0:START

; env for MINT64OS
; TOTALSECTORCOUNT and KERNEL32SECTOR will be modified by ImageMaker
; program in Utility directory.
; When you read the code, just assume that TOTALSECTORCOUNT is total number of
; sectors of overall image except bootloader part
; and KERNEL32SECTOR is the number of sectors of Kernel32 
TOTALSECTORCOUNT: dw 0 
KERNEL32SECTORCOUNT: dw 0

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

    ; BIOS set dl to a drive that has this bootlader program
    mov byte [BOOTDRIVE], dl

    ; print message by using BIOS service. Some computers requires using BIOS
    ; services before using video memory address. Otherwise, nothing is printed
    ; on screen
    mov si, MESSAGE1
    call print


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
    mov dl, byte [BOOTDRIVE] ; drive num: (floppy1=0x0, hdd1=0x80)
    int 0x13

    jc HANDLEDISKERROR

; read drive maximum CHS
READDISKPARAMTER:
    mov ah, 0x08
    mov dl, byte [BOOTDRIVE]
    int 0x13
    jc HANDLEDISKERROR

    mov byte [LASTHEAD], dh
    mov al, cl
    and al, 0x3F

    mov byte [LASTSECTOR], al
    mov byte [LASTTRACK], ch


    ;; start of READDATA

    ; 0x10000 is start addr of OS in memory
    ; regs for reading data: dest_addr
    ; es:bx address to load the data
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
    mov dl, byte [BOOTDRIVE]    ; drive to read (0x0=Floppy1, 0x80=HDD1)
    int 0x13                    ; interrupt to execute service

    jc HANDLEDISKERROR    

    ; set next dest_addr: after 512 byte from prev dest
    add si, 0x0020
    mov es, si

    mov al, byte [SECTORNUMBER]
    add al, 0x01
    mov byte [SECTORNUMBER], al

    cmp al, byte [LASTSECTOR]
    jbe READDATA

    add byte [HEADNUMBER], 0x01
    mov byte [SECTORNUMBER], 0x01


    mov al, byte [LASTHEAD]
    cmp byte [HEADNUMBER], al
    ja .ADDTRACK
    jmp READDATA

.ADDTRACK:
    mov byte [HEADNUMBER], 0x00
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

; print message by using BIOS service instead of video address
print:
    mov ah, 0Eh                       
again1:
    lodsb                             
    or al, al                         
    jz done1                         
    int 10h                           
    jmp again1                       
done1:
   ret  


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

BOOTDRIVE: db 0x00
LASTSECTOR: db 0x00
LASTHEAD: db 0x00
LASTTRACK: db 0x00

;; Some BIOS requires USB to have valid partition entry information
;; in MBR. I tested that a 2015 samsumg laptop does not require this info
;; but a 2018 acer laptop requires this


; Partition1 0x01BE  (i.e. first partition-entry begins from 0x01BE)
; Partition2 0x01CE
; Partition3 0x01DE
; Partition4 0x01EE
; We only fill/use Partition1
TIMES 0x1BE - ($ - $$) db 0

;; it's a dummy partition-entry (sectornumbers can't be zeros,
;; starting CHS and LBA values should be the same if converted to each other).

db 0x80     ; Boot indicator flag (0x80 means bootable)
db 0        ; Starting head
db 3        ; Starting sector (6 bits, bits 6-7 are upper 2 bits of cylinder)
db 0        ; Starting cylinder (10 bits)
db 0x8B     ; System ID   (0x8B means FAT32)
db 0        ; Ending head
db 100      ; Ending sector (6 bits, bits 6-7 are upper 2 bits of cylinder)
db 0        ; Ending cylinder (10 bits)
dd 2        ; Relative sector (32 bits, start of partition)
dd 97       ; Total sectors in partition (32 bits)


; add padding between above code and MBR signature
times 510 - ($ - $$) db 0x00

; MBR signature
db 0x55
db 0xAA