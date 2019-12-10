;
; Copyright(c) 2018 Intel Corporation
; SPDX - License - Identifier: BSD - 2 - Clause - Patent
;

%include "x64inc.asm"

section .text

; (Copied from Intel 64 and IA-32 Architectures Software Developer's Manual)
; ("9.6.3 Using the EMMS Instruction")
; The EMMS instruction should be used in each of the following cases:
;  1. When an application using the x87 FPU instructions calls an MMX technology
;     library/DLL (use the EMMS instruction at the end of the MMX code).
;  2. When an application using MMX instructions calls a x87 FPU floating-point
;     library/DLL (use the EMMS instruction before calling the x87 FPU code).
;  3. When a switch is made between MMX code in a task or thread and other tasks or
;     threads in cooperative operating systems, unless it is certain that more MMX
;     instructions will be executed before any x87 FPU code.

; So if EbHevcRunEmms() is called according to the above cases,
; then the "emms" instruction in all other assembly functions can be removed.

cglobal EbHevcRunEmms
    emms
    ret

; ----------------------------------------------------------------------------------------

cglobal EbHevcSaveRegister
%ifdef WIN64
    movdqa          [r0],           xmm6
    movdqa          [r0+0x10],      xmm7
    movdqa          [r0+0x20],      xmm8
    movdqa          [r0+0x30],      xmm9
    movdqa          [r0+0x40],      xmm10
    movdqa          [r0+0x50],      xmm11
    movdqa          [r0+0x60],      xmm12
    movdqa          [r0+0x70],      xmm13
    movdqa          [r0+0x80],      xmm14
    movdqa          [r0+0x90],      xmm15
%endif
    ret

; ----------------------------------------------------------------------------------------

cglobal EbHevcRestoreRegister
%ifdef WIN64
    movdqa          xmm6,           [r0]
    movdqa          xmm7,           [r0+0x10]
    movdqa          xmm8,           [r0+0x20]
    movdqa          xmm9,           [r0+0x30]
    movdqa          xmm10,          [r0+0x40]
    movdqa          xmm11,          [r0+0x50]
    movdqa          xmm12,          [r0+0x60]
    movdqa          xmm13,          [r0+0x70]
    movdqa          xmm14,          [r0+0x80]
    movdqa          xmm15,          [r0+0x90]
%endif
    ret
