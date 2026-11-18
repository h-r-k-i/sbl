[bits 16]

%if 0
 FILENAME:      ./boot/s3/drivers/protected.s
 NAME:          SBL, Stage 3 protected mode functions
 DESCRIPTION:   Does protected mode setup and then jumps to the kernel.
 AUTHOR:        Haruki Tokumei, Haruki Media Group
 DATE:          15 November 2026
 COPYRIGHT:     Haruki Media Group (notably a lot of this is still MIT license)
 LICENSE:       MIT License (./LICENSE)
 NOTE:          none
%endif

; renamed arg_segment because segment is an actual nasm keyword
%define arg_segment [bp + 0x08]
; renamed arg_offset to match arg_segment
%define arg_offset [bp + 0x06]

global RunKernel

RunKernel:
    ; variables:
    ; [bp + 0x00]: original base pointer (NOT A VARIABLE DO NOT FUCKING USE)
    ; [bp + 0x02]: return address (see warning above)
    ; [bp + 0x06]: kernel entry point (uint32_t)
    ; [bp + 0x0A]: magic number (uint32_t)
    ; [bp + 0x0E]: bootloader map address (uint32_t)

    push bp
    mov bp, sp
    push es
    push di
    push bx
    push ds
    push si

    ; GDT and IDT pointers are set up in S2 at 0x900 so we can just load them from there
    xor ax, ax
    mov ds, ax

    cli

    o32 mov eax, [0x900]
    o32 lgdt [eax]
    o32 mov eax, [0x904]
    o32 lidt [eax]

    ; enable protected mode
    in al, 0x70
    or al, 0x80
    out 0x70, al
    mov eax, cr0
    or al, 1
    mov cr0, eax

    jmp dword 0x08:RunKernel_protected

    pop si
    pop ds
    pop bx
    pop di
    pop es
    pop bp
    o32 ret

[bits 32]
RunKernel_protected:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    in al, 0x70
    and al, 0x7F
    out 0x70, al
    mov al, 0xFF
    out 0xA1, al
    out 0x21, al

    cld

    o32 mov esp, 0x200000
    o32 mov eax, [bp + 0x0A]
    o32 mov ebx, [bp + 0x0E]
    o32 mov ecx, [bp + 0x06]
    jmp ecx