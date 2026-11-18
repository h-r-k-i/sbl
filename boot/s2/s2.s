[bits 16]

%if 0
 FILENAME:      ./boot/s2/s2.s
 NAME:          SBL, Stage 2
 DESCRIPTION:   Sets up the device for booting into the kernel,
                exits Real Mode, and boots Stage 3.
 AUTHOR:        Haruki Tokumei, Haruki Media Group
 DATE:          28 January 2026
 COPYRIGHT:     Haruki Media Group
 LICENSE:       MIT License (./LICENSE)
%endif

%if 0
0100 0000 | Top of memory hole
000F 0000 | Video memory, MM I/O, BIOS data
000A 0000 | Bottom of memory hole
0009 0000 | Kernel preload end at 0009 F000
0000 1000 | ...
     F000 | Kernel preload start
     E000 | Top of protected mode stack at F000
     D000 | ...
     C000 | Disk buffer ends at C7FF
     B000 | ...
     A000 | ...
     9000 | ...
     8000 | Disk buffer start
     7000 | boot1 stored between 7C00-7DFF
     6000 | Toop of real mode stack at 7000
     5000 | ...
     4000 | ...
     3000 | boot2 ends at 31FF
     2000 | ...
     1000 | boot2 start
     0000 | Reserved (really 000-500 is reserved)


%endif

%macro cpush 1
    push word 0
    push word %1
%endmacro

%macro ccall 1
    push word 0
    call %1
%endmacro

global start
extern c_add, sbl_build, sbl_read


start:
    cli
    xor ax, ax
    mov ss, ax
    sti

    mov [boot_drive], dl
    mov dl, [boot_drive]
    mov bp, boot_drive

    pop dx
    mov [lba_state], dx
    xor dx, dx


    ; clear all the registers except the ones I can't.
    cli
    xor ax, ax
    xor bx, bx
    xor cx, cx
    xor dh, dh
    mov si, 0x7000
    xor di, di

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ss, ax
    sti

    ; confirm for the user that we're in S2

    mov ah, 0x13
    xor dx, dx
    mov al, 0x01
    mov bh, 0
    mov bl, 0x07
    mov cx, m1l
    push ds
    pop es
    mov bp, m1
    int 0x10
    inc dh
    mov [row], dh

    ; test call of c function
    cpush 4
    cpush 3
    ccall c_add
    add sp, 8
    cmp ax, 7
    jne hang3

    jmp stage4

stage0_1:
    ; S1: Build the SBL Data structure
    sti
    cpush s3_sectors
    cpush ImpAreaSize
    cpush ImpArea
    cpush SBL_Data
    ccall sbl_build
    add sp, 16
    cmp ax, 177
    je hang4 ; if 177 is returned then the SBL build succeeded BUT we're in debug mode so the bootloader will halt to allow map validation
    cli
    cli


; S3: Read kernel from disk to lower memory.
stage3:

    ; Reset the disk controller (the following code is a literal copy-
    ; paste from S1 lmfaoooo)
    xor bl, bl
    mov ah, 0x00
    xor dx, dx
    mov dl, [boot_drive]
    int 0x13

    ; Get the value we stored at the beginning of S2
    mov dx, [lba_state]
    cmp dx, 0x464C
    je sector_read_hell
    cmp dx, 0x5343
    jne hang    ; if we can get neither FL nor SC something got fucked up
    ; that or I fucked up

    ; Now that we've confirmed that we can use the modern disk reader,
    ; do that to read into s3 (this has just the number changed lel)
    push cs
    pop ds
    mov si, DAP_S3
    mov ah, 0x42
    mov dl, [boot_drive] ; todo: add more comments because assembly scares me
    int 0x13

    ; if fail, try a different way
    jc sector_read_hell
    jmp final

    sector_read_hell:
        ; I don't wanna fucking bother adding support for PCs that require
        ; me to do bullshit geometry checks and whatnot
        ; so you know what? I'm just not going to do it.
        ; Fuck you.

        mov ah, 0x13
        mov al, 0x01
        mov bh, 0
        mov bl, 0x07
        mov dh, [row]
        xor dl, dl
        mov cx, m6l
        push ds
        pop es
        mov bp, m6
        int 0x10
        inc dh
        mov [row], dh
        xor dx, dx

        hang2:
            hlt
            jmp hang2

    final:
        nop ; lmao
        jmp stage6

; S4: Enable A20 line.
stage4:
    jmp setA20
    ; I had an elaborate method to triple-check that A20 is
    ; enabled with a function and then if it's not working
    ; then give up but it breaks so fuck it violently
    ; enable A20 

    setA20:
        ; violently tell QEMU to stfu with A20
        ; in al, 0x92
        ; and al, 0xFD
        ; out 0x92, al
        ; make sure to comment this shit out before testing on hardware
        ; that doesn't jerk you off and enable A20 at startup
        ; like the OptiPlex
        ; although on Haswell+ CPUs A20 may be forcibly set to HIGH

        mov ah, 0x13
        mov al, 0x01
        mov bh, 0
        mov bl, 0x07
        mov dh, [row]
        xor dl, dl
        mov cx, m8l
        push ds
        pop es
        mov bp, m8
        int 0x10
        inc dh
        mov [row], dh

        ; Try each method in turn, verifying with an actual wraparound test after
        ; each one, and only fall back to the keyboard controller (which may not
        ; exist on legacy-free machines) if everything else failed.

        ; 0. maybe it's already on (firmware / Haswell+)
        call check_a20
        jnz aftsA20

        ; 1. BIOS
        mov ax, 0x2401
        int 0x15
        call check_a20
        jnz aftsA20

        ; 2. Fast A20 (port 0x92)
        in al, 0x92
        test al, 2
        jnz .no_fast
        or al, 2
        and al, 0xFE
        out 0x92, al
        .no_fast:
        call check_a20_retry
        jnz aftsA20

        ; 3. Keyboard controller (every wait has a timeout)
        ; GOD KILL ME I FUCKING HATE BOOTLOADER DEVELOPMENT
        ; fuck it stealing from OSDev
        call enable_A20
        call check_a20_retry
        jnz aftsA20

        jmp hang_a20 ; nothing worked; loading anything above 1MB would silently wrap

        aftsA20:
        jmp stage5

    ; S5: Disable interrupts.
stage5:
    nop ; lol part 2: electric boogaloo
    cli
    jmp stage0_1

; S6: Load and jump to Stage 3 (addl unreal mode bullshit)
stage6:
    ; Set up the IDT and GDT
    setup_idt:
        ; Vectors 0-31 (CPU exceptions) except NMI go to exc_halt: several of them
        ; push an error code (a bare iret would return to garbage) and faults would
        ; just re-execute forever. NMI and 32-255 keep the plain iret stub.
        o32 xor ecx, ecx            ; ecx = vector number
        lea edi, [IDT]

    .loop69:
        mov eax, shit_isr
        cmp ecx, 32
        jae .gotisr
        cmp ecx, 2                  ; NMI: harmless to return from
        je .gotisr
        mov eax, exc_halt
    .gotisr:
        mov word [edi], ax
        mov word [edi+2], 0x08
        mov byte [edi+4], 0
        mov byte [edi+5], 0x8E
        shr eax, 16
        mov word [edi+6], ax

        add edi, 8
        inc ecx
        cmp ecx, 256
        jb .loop69
    
    o32 mov eax, GDT_DESC
    o32 mov ecx, 0x900
    o32 mov [ecx], eax
    o32 add ecx, 4
    o32 mov eax, IDTR
    o32 mov [ecx], eax
    
    o32 mov eax, 0x4F4B454E ; NEKO (Magic Number)
    o32 mov ebx, SBL_Data
    o32 mov edx, [boot_drive]
    o32 and edx, 0x000000FF
    jmp 0x0000:s3_loc

[bits 16]
hang:
    cli
    mov ah, 0x13
    mov al, 0x01
    mov bh, 0
    mov bl, 0x07
    mov dh, [row]
    xor dl, dl
    mov cx, m2l
    push ds
    pop es
    mov bp, m2
    int 0x10
    inc dh
    mov [row], dh
    hlt
    jmp hang
    
; Keyboard-controller A20 enable. Every wait times out (CF=1), so a missing
; 8042 (port 0x64 floats to 0xFF on legacy-free hardware) can't hang us.
enable_A20:
    pushf
    cli

    call    a20wait
    jc      .done               ; no controller at all
    mov     al,0xAD             ; disable keyboard
    out     0x64,al

    call    a20wait
    jc      .reenable
    mov     al,0xD0             ; read output port
    out     0x64,al

    call    a20wait2
    jc      .reenable
    in      al,0x60
    mov     ah, al              ; a20wait* only clobber AL

    call    a20wait
    jc      .reenable
    mov     al,0xD1             ; write output port
    out     0x64,al

    call    a20wait
    jc      .reenable
    mov     al, ah
    or      al,2
    out     0x60,al

    .reenable:
    call    a20wait
    mov     al,0xAE             ; re-enable keyboard
    out     0x64,al

    .done:
    popf
    ret

; wait for the 8042 input buffer to drain; CF=1 on timeout
a20wait:
    o32 push ecx
    o32 mov ecx, 0x100000
    .spin:
    in      al,0x64
    test    al,2
    jz      .ok
    o32 dec ecx
    jnz     .spin
    o32 pop ecx
    stc
    ret
    .ok:
    o32 pop ecx
    clc
    ret

; wait for the 8042 output buffer to fill; CF=1 on timeout
a20wait2:
    o32 push ecx
    o32 mov ecx, 0x100000
    .spin:
    in      al,0x64
    test    al,1
    jnz     .ok
    o32 dec ecx
    jnz     .spin
    o32 pop ecx
    stc
    ret
    .ok:
    o32 pop ecx
    clc
    ret

; Wraparound test: writes 0x0000:0x0500 and 0xFFFF:0x0510 (= 0x100500).
; If they alias, A20 is off. Returns ZF=0 (jnz) when A20 is ON, ZF=1 when OFF.
; Preserves everything except flags.
check_a20:
    push ax
    push bx
    push ds
    push es
    push si
    push di
    pushf
    cli

    xor ax, ax
    mov es, ax
    not ax
    mov ds, ax                  ; real-mode load: base 0xFFFF0, cached limit kept
    mov di, 0x0500
    mov si, 0x0510

    mov bl, [es:di]             ; save both original bytes
    mov bh, [ds:si]

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
    mov al, [es:di]             ; 0xFF here means the write to 1MB+ wrapped

    mov [ds:si], bh             ; restore (if wrapped these are the same byte,
    mov [es:di], bl             ;  and BL == BH, so the order doesn't matter)

    popf                        ; restore IF before producing the result flag
    cmp al, 0xFF                ; ZF=1 -> wrapped -> A20 off; ZF=0 -> on

    pop di                      ; pops don't touch flags
    pop si
    pop es
    pop ds
    pop bx
    pop ax
    ret

; check_a20 a few times; some chipsets take a moment after port 0x92 / 8042 writes
check_a20_retry:
    push cx
    mov cx, 0x1000
    .again:
    call check_a20
    jnz .done
    loop .again
    cmp cx, cx                  ; ZF=1: still off
    .done:
    pop cx
    ret

hang_a20:
    cli
    mov ah, 0x13
    mov al, 0x01
    mov bh, 0
    mov bl, 0x07
    mov dh, [row]
    xor dl, dl
    mov cx, m13l
    push ds
    pop es
    mov bp, m13
    int 0x10
    .halt:
    hlt
    jmp .halt

hang_dbg:
    hlt
    jmp hang_dbg

hang3:
    cli
    mov ah, 0x13
    mov al, 0x01
    mov bh, 0
    mov bl, 0x07
    mov dh, [row]
    xor dl, dl
    mov cx, m11l
    push ds
    pop es
    mov bp, m11
    int 0x10
    inc dh
    mov [row], dh
    hlt
    jmp hang2

hang4:
    cli
    mov ah, 0x13
    mov al, 0x01
    mov bh, 0
    mov bl, 0x07
    mov dh, [row]
    xor dl, dl
    mov cx, m12l
    push ds
    pop es
    mov bp, m12
    int 0x10
    inc dh
    mov [row], dh
    hlt
    jmp hang2


boot_drive: db 0
row: db 0
lba_state: dw 0

; messages
m1 db 'Loading from hard drive...', 0x0a, 0x0d, 0x00
m1l equ $ - m1

m2 db 'E: Something happened. Please check drive state.', 0x0a, 0x0d, 0x00
m2l equ $ - m2

m6 db 'E: Could not use LBA reading, quitting! (get a PC from this millenium, asshole)', 0x0a, 0x0d, 0x00
m6l equ $ - m6

m8 db 'Enabling A20!', 0x0a, 0x0d, 0x00
m8l equ $ - m8

m13 db 'E: Could not enable A20 (BIOS, port 0x92 and 8042 all failed).', 0x0a, 0x0d, 0x00
m13l equ $ - m13

m11 db "E: get this fucking function to work you idiot", 0x0a, 0x0d, 0x00
m11l equ $ - m11

m12 db "E: run pmemsave to get a memory map and then run without debug mode", 0x0a, 0x0d, "note: if you see this msg on a real machine esp outside of debug mode something has gone very wrong", 0x0a, 0x0d, 0x00
m12l equ $ - m12

DAP_S3:
    db 0x10         ; Size of DAP
    db 0            ; Reserved
    dw s3_sectors   ; Sectors to read; hardcoded for now
    dw s3_off       ; Offset to load to
    dw s3_seg       ; Segment to load to
    dq 25           ; Starting LBA sector (zero-based)

SBL_Data equ 0x5000

s3_off equ 0x0000
s3_seg equ 0x0800
s3_loc equ 0x8000
s3_sectors equ 78

ImpArea equ $

shit_isr:
    iret        ; CF: decodes as IRETD when run from the 32-bit code segment

; Placeholder for CPU exceptions until the kernel installs its own IDT.
; Never returns, so the error code some exceptions push doesn't matter.
; cli/hlt/jmp short encode identically in 16- and 32-bit code.
exc_halt:
    cli
    .halt:
    hlt
    jmp short .halt

GDT_START:
    dq 0                    ; HAS TO BE NULL
    dq 0x00CF9A000000FFFF   ; flat code segment
    dq 0x00CF92000000FFFF   ; flat data segment
GDT_END:

GDT_DESC:
    dw GDT_END - GDT_START - 1  ; start, offset by 1 because idfk
    dd GDT_START                ; base addr

IDT:
    times 256 * 8 db 0 ; Fuck You! Eat My Fucking Asshole!

IDTR:
    dw (256 * 8) - 1
    dd IDT

ImpAreaSize equ $ - ImpArea

; padding + 0xABAB signature are appended by the linker script (s2.ld),
; since this file no longer knows the final size once s2.c is linked in