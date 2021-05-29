[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x1000:START ;; right after bootloader sector

SECTORCOUNT: dw 0x0000 ; currently executing sector
TOTALSECTORCOUNT equ 1024 ; virtual OS's num of sectors

;; code section
START:
    ; segment setting for each sector
    mov ax, cs ; ex: 0x0000, 0x0020, ...
    mov ds, ax
    mov ax, 0xB800;
    mov es, ax;

    ; print number for current memory addr and jump to next memory addr 
    ; which is multiply of 512 bytes
    %assign i 0
    %rep TOTALSECTORCOUNT ; repeat as many as TOTALSECTORCOUNT
        %assign i i+1

        mov ax, 2
        mul word [SECTORCOUNT]
        mov si, ax ; si(video memory offset) = char_size * sector_count

        ; 160 * 2 + es:si == third line + offset
        mov byte [es:si + (160 * 2)], '0' + (i % 10)
        add word [SECTORCOUNT], 1

        %if i == TOTALSECTORCOUNT
            jmp $ ; end of virtual OS
        %else
            jmp (0x1000 + i * 0x20):0x0000 ; jump to next memory addr
        %endif

        ; $$ == SECTION .code == 0x00
        ; $ == cur addr that can be over 0x10000 + 512 bytes
        times (512 - ($ - $$) % 512) db 0x00
    %endrep