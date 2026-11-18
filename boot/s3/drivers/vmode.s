[bits 16]

%if 0
 FILENAME:      ./boot/s3/drivers/vmode.s
 NAME:          SBL, Stage 3 video mode functions
 DESCRIPTION:   Video mode switching functions for the SBL Stage 3 bootloader.
 AUTHOR:        Haruki Tokumei, Haruki Media Group
 DATE:          18 November 2026
 COPYRIGHT:     Haruki Media Group (notably a lot of this is still MIT license)
 LICENSE:       MIT License (./LICENSE)
 NOTE:          none
%endif

; renamed arg_segment because segment is an actual nasm keyword
%define arg_segment [bp + 0x08]
; renamed arg_offset to match arg_segment
%define arg_offset [bp + 0x06]

global SetVideoMode

SetVideoMode:
    ; variables:
    ; [bp + 0x00]: original base pointer (NOT A VARIABLE DO NOT FUCKING USE)
    ; [bp + 0x02]: return address (see warning above)
    ; [bp + 0x06]: video mode (uint16_t)

    push bp
    mov bp, sp
    push es
    push di
    o32 push ebx
    push ds
    push si

    mov ax, 0x4F02
    mov bx, [bp + 0x06] ; video mode
    or bx, 0x4000
    sti
    int 0x10
    cli

    pop si
    pop ds
    o32 pop ebx
    pop di
    pop es
    pop bp
    o32 ret