;
; Copyright(c) 2018 Intel Corporation
; SPDX - License - Identifier: BSD - 2 - Clause - Patent
;

%include "x64inc.asm"
%include "x64Macro.asm"
section .text
; ----------------------------------------------------------------------------------------

cglobal PictureCopyKernel_SSE2

; Requirement: areaWidthInBytes = 4, 8, 12, 16, 24, 32, 48, 64 or 128
; Requirement: areaHeight % 2 = 0

%define src              r0
%define srcStrideInBytes r1
%define dst              r2
%define dstStrideInBytes r3
%define areaWidthInBytes r4
%define areaHeight       r5

    GET_PARAM_5UXD
    GET_PARAM_6UXD
    XMM_SAVE

    cmp             areaWidthInBytes,        16
    jg              Label_PictureCopyKernel_SSE2_WIDTH_Big
    je              Label_PictureCopyKernel_SSE2_WIDTH16

    cmp             areaWidthInBytes,        4
    je              Label_PictureCopyKernel_SSE2_WIDTH4

    cmp             areaWidthInBytes,        8
    je              Label_PictureCopyKernel_SSE2_WIDTH8

Label_PictureCopyKernel_SSE2_WIDTH12:
    movq            xmm0,           [src]
    movd            xmm1,           [src+8]
    movq            xmm2,           [src+srcStrideInBytes]
    movd            xmm3,           [src+srcStrideInBytes+8]
    lea             src,            [src+2*srcStrideInBytes]
    movq            [dst],          xmm0
    movd            [dst+8],        xmm1
    movq            [dst+dstStrideInBytes], xmm2
    movd            [dst+dstStrideInBytes+8], xmm3
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             areaHeight,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH12

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH8:
    movq            xmm0,           [src]
    movq            xmm1,           [src+srcStrideInBytes]
    lea             src,            [src+2*srcStrideInBytes]
    movq            [dst],          xmm0
    movq            [dst+dstStrideInBytes], xmm1
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             areaHeight,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH8

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH4:
    movd            xmm0,           [src]
    movd            xmm1,           [src+srcStrideInBytes]
    lea             src,            [src+2*srcStrideInBytes]
    movd            [dst],          xmm0
    movd            [dst+dstStrideInBytes], xmm1
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             areaHeight,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH4

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH_Big:
    cmp             areaWidthInBytes,        24
    je              Label_PictureCopyKernel_SSE2_WIDTH24

    cmp             areaWidthInBytes,        32
    je              Label_PictureCopyKernel_SSE2_WIDTH32

    cmp             areaWidthInBytes,        48
    je              Label_PictureCopyKernel_SSE2_WIDTH48

    cmp             areaWidthInBytes,        64
    je              Label_PictureCopyKernel_SSE2_WIDTH64

Label_PictureCopyKernel_SSE2_WIDTH128:
    movdqu          xmm0,           [src]
    movdqu          xmm1,           [src+16]
    movdqu          xmm2,           [src+32]
    movdqu          xmm3,           [src+48]
    movdqu          xmm4,           [src+64]
    movdqu          xmm5,           [src+80]
    movdqu          xmm6,           [src+96]
    movdqu          xmm7,           [src+112]
    lea             src,            [src+srcStrideInBytes]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+32],       xmm2
    movdqu          [dst+48],       xmm3
    movdqu          [dst+64],       xmm4
    movdqu          [dst+80],       xmm5
    movdqu          [dst+96],       xmm6
    movdqu          [dst+112],      xmm7
    lea             dst,            [dst+dstStrideInBytes]
    sub             areaHeight,     1
    jne             Label_PictureCopyKernel_SSE2_WIDTH128

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH64:
    movdqu          xmm0,           [src]
    movdqu          xmm1,           [src+16]
    movdqu          xmm2,           [src+32]
    movdqu          xmm3,           [src+48]
    movdqu          xmm4,           [src+srcStrideInBytes]
    movdqu          xmm5,           [src+srcStrideInBytes+16]
    movdqu          xmm6,           [src+srcStrideInBytes+32]
    movdqu          xmm7,           [src+srcStrideInBytes+48]
    lea             src,            [src+2*srcStrideInBytes]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+32],       xmm2
    movdqu          [dst+48],       xmm3
    movdqu          [dst+dstStrideInBytes],    xmm4
    movdqu          [dst+dstStrideInBytes+16], xmm5
    movdqu          [dst+dstStrideInBytes+32], xmm6
    movdqu          [dst+dstStrideInBytes+48], xmm7
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             areaHeight,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH64

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH48:
    movdqu          xmm0,           [src]
    movdqu          xmm1,           [src+16]
    movdqu          xmm2,           [src+32]
    movdqu          xmm3,           [src+srcStrideInBytes]
    movdqu          xmm4,           [src+srcStrideInBytes+16]
    movdqu          xmm5,           [src+srcStrideInBytes+32]
    lea             src,            [src+2*srcStrideInBytes]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+32],       xmm2
    movdqu          [dst+dstStrideInBytes], xmm3
    movdqu          [dst+dstStrideInBytes+16], xmm4
    movdqu          [dst+dstStrideInBytes+32], xmm5
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             areaHeight,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH48

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH32:
    movdqu          xmm0,           [src]
    movdqu          xmm1,           [src+16]
    movdqu          xmm2,           [src+srcStrideInBytes]
    movdqu          xmm3,           [src+srcStrideInBytes+16]
    lea             src,            [src+2*srcStrideInBytes]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+dstStrideInBytes], xmm2
    movdqu          [dst+dstStrideInBytes+16], xmm3
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             areaHeight,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH32

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH24:
    movdqu          xmm0,           [src]
    movq            xmm1,           [src+16]
    movdqu          xmm2,           [src+srcStrideInBytes]
    movq            xmm3,           [src+srcStrideInBytes+16]
    lea             src,            [src+2*srcStrideInBytes]
    movdqu          [dst],          xmm0
    movq            [dst+16],       xmm1
    movdqu          [dst+dstStrideInBytes], xmm2
    movq            [dst+dstStrideInBytes+16], xmm3
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             areaHeight,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH24

    XMM_RESTORE
    ret

Label_PictureCopyKernel_SSE2_WIDTH16:
    movdqu          xmm0,           [src]
    movdqu          xmm1,           [src+srcStrideInBytes]
    lea             src,            [src+2*srcStrideInBytes]
    movdqu          [dst],          xmm0
    movdqu          [dst+dstStrideInBytes], xmm1
    lea             dst,            [dst+2*dstStrideInBytes]
    sub             areaHeight,     2
    jne             Label_PictureCopyKernel_SSE2_WIDTH16

    XMM_RESTORE
    ret
; ----------------------------------------------------------------------------------------

cglobal ZeroOutCoeff4x4_SSE

    lea             r0,             [r0+2*r2]
    lea             r3,             [r1+2*r1]
    pxor            mm0,            mm0

    movq            [r0],           mm0
    movq            [r0+2*r1],      mm0
    movq            [r0+4*r1],      mm0
    movq            [r0+2*r3],      mm0

%if NEED_EMMS
    emms
%endif

    ret

; ----------------------------------------------------------------------------------------

cglobal ZeroOutCoeff8x8_SSE2

; TODO: use "movdqa" if coeffbuffer is guaranteed to be 16-byte aligned.

    lea             r0,             [r0+2*r2]
    lea             r3,             [r1+2*r1]
    pxor            xmm0,           xmm0

    movdqu          [r0],           xmm0
    movdqu          [r0+2*r1],      xmm0
    movdqu          [r0+4*r1],      xmm0
    movdqu          [r0+2*r3],      xmm0
    lea             r0,             [r0+8*r1]
    movdqu          [r0],           xmm0
    movdqu          [r0+2*r1],      xmm0
    movdqu          [r0+4*r1],      xmm0
    movdqu          [r0+2*r3],      xmm0

    ret

; ----------------------------------------------------------------------------------------

cglobal ZeroOutCoeff16x16_SSE2

; TODO: use "movdqa" if coeffbuffer is guaranteed to be 16-byte aligned.

    lea             r0,             [r0+2*r2]
    lea             r3,             [r1+2*r1]
    pxor            xmm0,           xmm0

    movdqu          [r0],           xmm0
    movdqu          [r0+16],        xmm0
    movdqu          [r0+2*r1],      xmm0
    movdqu          [r0+2*r1+16],   xmm0
    movdqu          [r0+4*r1],      xmm0
    movdqu          [r0+4*r1+16],   xmm0
    movdqu          [r0+2*r3],      xmm0
    movdqu          [r0+2*r3+16],   xmm0
    lea             r0,             [r0+8*r1]
    movdqu          [r0],           xmm0
    movdqu          [r0+16],        xmm0
    movdqu          [r0+2*r1],      xmm0
    movdqu          [r0+2*r1+16],   xmm0
    movdqu          [r0+4*r1],      xmm0
    movdqu          [r0+4*r1+16],   xmm0
    movdqu          [r0+2*r3],      xmm0
    movdqu          [r0+2*r3+16],   xmm0
    lea             r0,             [r0+8*r1]

    movdqu          [r0],           xmm0
    movdqu          [r0+16],        xmm0
    movdqu          [r0+2*r1],      xmm0
    movdqu          [r0+2*r1+16],   xmm0
    movdqu          [r0+4*r1],      xmm0
    movdqu          [r0+4*r1+16],   xmm0
    movdqu          [r0+2*r3],      xmm0
    movdqu          [r0+2*r3+16],   xmm0
    lea             r0,             [r0+8*r1]
    movdqu          [r0],           xmm0
    movdqu          [r0+16],        xmm0
    movdqu          [r0+2*r1],      xmm0
    movdqu          [r0+2*r1+16],   xmm0
    movdqu          [r0+4*r1],      xmm0
    movdqu          [r0+4*r1+16],   xmm0
    movdqu          [r0+2*r3],      xmm0
    movdqu          [r0+2*r3+16],   xmm0

    ret

; ----------------------------------------------------------------------------------------

cglobal ZeroOutCoeff32x32_SSE2

; TODO: use "movdqa" if coeffbuffer is guaranteed to be 16-byte aligned.

    lea             r0,             [r0+2*r2]
    lea             r3,             [r1+2*r1]
    mov             r4,             4
    pxor            xmm0,           xmm0

Label_ZeroOutCoeff32x32_SSE2_01:
    movdqu          [r0],           xmm0
    movdqu          [r0+16],        xmm0
    movdqu          [r0+32],        xmm0
    movdqu          [r0+48],        xmm0
    movdqu          [r0+2*r1],      xmm0
    movdqu          [r0+2*r1+16],   xmm0
    movdqu          [r0+2*r1+32],   xmm0
    movdqu          [r0+2*r1+48],   xmm0
    movdqu          [r0+4*r1],      xmm0
    movdqu          [r0+4*r1+16],   xmm0
    movdqu          [r0+4*r1+32],   xmm0
    movdqu          [r0+4*r1+48],   xmm0
    movdqu          [r0+2*r3],      xmm0
    movdqu          [r0+2*r3+16],   xmm0
    movdqu          [r0+2*r3+32],   xmm0
    movdqu          [r0+2*r3+48],   xmm0
    lea             r0,             [r0+8*r1]
    movdqu          [r0],           xmm0
    movdqu          [r0+16],        xmm0
    movdqu          [r0+32],        xmm0
    movdqu          [r0+48],        xmm0
    movdqu          [r0+2*r1],      xmm0
    movdqu          [r0+2*r1+16],   xmm0
    movdqu          [r0+2*r1+32],   xmm0
    movdqu          [r0+2*r1+48],   xmm0
    movdqu          [r0+4*r1],      xmm0
    movdqu          [r0+4*r1+16],   xmm0
    movdqu          [r0+4*r1+32],   xmm0
    movdqu          [r0+4*r1+48],   xmm0
    movdqu          [r0+2*r3],      xmm0
    movdqu          [r0+2*r3+16],   xmm0
    movdqu          [r0+2*r3+32],   xmm0
    movdqu          [r0+2*r3+48],   xmm0
    lea             r0,             [r0+8*r1]
    sub             r4,             1
    jne             Label_ZeroOutCoeff32x32_SSE2_01

    ret

; ----------------------------------------------------------------------------------------

cglobal PictureAverageKernel_SSE2

; Requirement: puWidth         = 4, 8, 12, 16, 24, 32, 48 or 64
; Requirement: puHeight   %  2 = 0
; Requirement: src0       % 16 = 0 when puWidth >= 16
; Requirement: src1       % 16 = 0 when puWidth >= 16
; Requirement: dst        % 16 = 0 when puWidth >= 16
; Requirement: src0Stride % 16 = 0 when puWidth >= 16
; Requirement: src1Stride % 16 = 0 when puWidth >= 16
; Requirement: dstStride  % 16 = 0 when puWidth >= 16

%define src0       r0
%define src0Stride r1
%define src1       r2
%define src1Stride r3
%define dst        r4
%define dstStride  r5
%define areaWidth  r6
%define areaHeight r7

    GET_PARAM_5Q
    GET_PARAM_6UXD
    GET_PARAM_7UXD
    PUSH_REG 7
    XMM_SAVE
    GET_PARAM_8UXD  r7d                         ; areaHeight

    cmp             areaWidth,      16
    jg              Label_PictureAverageKernel_SSE2_WIDTH_Big
    je              Label_PictureAverageKernel_SSE2_WIDTH16

    cmp             areaWidth,      4
    je              Label_PictureAverageKernel_SSE2_WIDTH4

    cmp             areaWidth,      8
    je              Label_PictureAverageKernel_SSE2_WIDTH8

Label_PictureAverageKernel_SSE2_WIDTH12:
    movq            mm0,            [src0]
    movd            mm1,            [src0+8]
    movq            mm2,            [src0+src0Stride]
    movd            mm3,            [src0+src0Stride+8]
    pavgb           mm0,            [src1]
    pavgb           mm1,            [src1+8]
    pavgb           mm2,            [src1+src1Stride]
    pavgb           mm3,            [src1+src1Stride+8]
    lea             src0,           [src0+2*src0Stride]
    lea             src1,           [src1+2*src1Stride]
    movq            [dst],          mm0
    movd            [dst+8],        mm1
    movq            [dst+dstStride], mm2
    movd            [dst+dstStride+8], mm3
    lea             dst,            [dst+2*dstStride]
    sub             areaHeight,     2
    jne             Label_PictureAverageKernel_SSE2_WIDTH12

    XMM_RESTORE
    POP_REG 7

%if NEED_EMMS
    emms
%endif
    ret

Label_PictureAverageKernel_SSE2_WIDTH8:
    movq            mm0,            [src0]
    movq            mm1,            [src0+src0Stride]
    pavgb           mm0,            [src1]
    pavgb           mm1,            [src1+src1Stride]
    lea             src0,           [src0+2*src0Stride]
    lea             src1,           [src1+2*src1Stride]
    movq            [dst],          mm0
    movq            [dst+dstStride], mm1
    lea             dst,            [dst+2*dstStride]
    sub             areaHeight,     2
    jne             Label_PictureAverageKernel_SSE2_WIDTH8

    XMM_RESTORE
    POP_REG 7

%if NEED_EMMS
    emms
%endif
    ret

Label_PictureAverageKernel_SSE2_WIDTH4:
    movd            mm0,            [src0]
    movd            mm1,            [src0+src0Stride]
    pavgb           mm0,            [src1]
    pavgb           mm1,            [src1+src1Stride]
    lea             src0,           [src0+2*src0Stride]
    lea             src1,           [src1+2*src1Stride]
    movd            [dst],          mm0
    movd            [dst+dstStride], mm1
    lea             dst,            [dst+2*dstStride]
    sub             areaHeight,     2
    jne             Label_PictureAverageKernel_SSE2_WIDTH4

    XMM_RESTORE
    POP_REG 7

%if NEED_EMMS
    emms
%endif
    ret

Label_PictureAverageKernel_SSE2_WIDTH_Big:
    cmp             areaWidth,      24
    je              Label_PictureAverageKernel_SSE2_WIDTH24

    cmp             areaWidth,      32
    je              Label_PictureAverageKernel_SSE2_WIDTH32

    cmp             areaWidth,      48
    je              Label_PictureAverageKernel_SSE2_WIDTH48

Label_PictureAverageKernel_SSE2_WIDTH64:
    movdqu          xmm0,           [src0]
    movdqu          xmm1,           [src0+16]
    movdqu          xmm2,           [src0+32]
    movdqu          xmm3,           [src0+48]
    movdqu          xmm4,           [src0+src0Stride]
    movdqu          xmm5,           [src0+src0Stride+16]
    movdqu          xmm6,           [src0+src0Stride+32]
    movdqu          xmm7,           [src0+src0Stride+48]
	movdqu			xmm8,			[src1]
	pavgb           xmm0,			xmm8
	movdqu			xmm8,			[src1+16]
	pavgb           xmm1,			xmm8
	movdqu			xmm8,			[src1+32]
	pavgb           xmm2,			xmm8
	movdqu			xmm8,			[src1+48]
	pavgb           xmm3,			xmm8
	movdqu			xmm8,			[src1+src1Stride]
	pavgb           xmm4,			xmm8
	movdqu			xmm8,			[src1+src1Stride+16]
	pavgb           xmm5,			xmm8
	movdqu			xmm8,			[src1+src1Stride+32]
	pavgb           xmm6,			xmm8
	movdqu			xmm8,			[src1+src1Stride+48]
	pavgb           xmm7,			xmm8
    lea             src0,           [src0+2*src0Stride]
    lea             src1,           [src1+2*src1Stride]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+32],       xmm2
    movdqu          [dst+48],       xmm3
    movdqu          [dst+dstStride], xmm4
    movdqu          [dst+dstStride+16], xmm5
    movdqu          [dst+dstStride+32], xmm6
    movdqu          [dst+dstStride+48], xmm7
    lea             dst,            [dst+2*dstStride]
    sub             areaHeight,     2
    jne             Label_PictureAverageKernel_SSE2_WIDTH64

    XMM_RESTORE
    POP_REG 7
    ret

Label_PictureAverageKernel_SSE2_WIDTH48:
    movdqu          xmm0,           [src0]
    movdqu          xmm1,           [src0+16]
    movdqu          xmm2,           [src0+32]
    movdqu          xmm3,           [src0+src0Stride]
    movdqu          xmm4,           [src0+src0Stride+16]
    movdqu          xmm5,           [src0+src0Stride+32]
	movdqu          xmm6,			[src1]
	pavgb           xmm0,			xmm6
	movdqu          xmm6,			[src1+16]
	pavgb           xmm1,			xmm6
	movdqu          xmm6,			[src1+32]
	pavgb           xmm2,			xmm6
	movdqu          xmm6,			[src1+src1Stride]
	pavgb           xmm3,			xmm6
	movdqu          xmm6,			[src1+src1Stride+16]
	pavgb           xmm4,			xmm6
	movdqu          xmm6,			[src1+src1Stride+32]
	pavgb           xmm5,			xmm6
    lea             src0,           [src0+2*src0Stride]
    lea             src1,           [src1+2*src1Stride]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+32],       xmm2
    movdqu          [dst+dstStride], xmm3
    movdqu          [dst+dstStride+16], xmm4
    movdqu          [dst+dstStride+32], xmm5
    lea             dst,            [dst+2*dstStride]
    sub             areaHeight,     2
    jne             Label_PictureAverageKernel_SSE2_WIDTH48

    XMM_RESTORE
    POP_REG 7
    ret

Label_PictureAverageKernel_SSE2_WIDTH32:
    movdqu          xmm0,           [src0]
    movdqu          xmm1,           [src0+16]
    movdqu          xmm2,           [src0+src0Stride]
    movdqu          xmm3,           [src0+src0Stride+16]
	movdqu			xmm4,			[src1]
	pavgb           xmm0,           xmm4
	movdqu			xmm4,			[src1+16]
	pavgb           xmm1,           xmm4
	movdqu			xmm4,			[src1+src1Stride]
	pavgb           xmm2,           xmm4
	movdqu			xmm4,			[src1+src1Stride+16]
	pavgb           xmm3,           xmm4
    lea             src0,           [src0+2*src0Stride]
    lea             src1,           [src1+2*src1Stride]
    movdqu          [dst],          xmm0
    movdqu          [dst+16],       xmm1
    movdqu          [dst+dstStride], xmm2
    movdqu          [dst+dstStride+16], xmm3
    lea             dst,            [dst+2*dstStride]
    sub             areaHeight,     2
    jne             Label_PictureAverageKernel_SSE2_WIDTH32

    XMM_RESTORE
    POP_REG 7
    ret

Label_PictureAverageKernel_SSE2_WIDTH24:
    movdqu          xmm0,           [src0]
    movq            mm0,            [src0+16]
    movdqu          xmm1,           [src0+src0Stride]
    movq            mm1,            [src0+src0Stride+16]
    movdqu          xmm2,           [src1]
    pavgb           xmm0,           xmm2
    pavgb           mm0,            [src1+16]
    movdqu          xmm3,           [src1+src1Stride]
    pavgb           xmm1,           xmm3
    pavgb           mm1,            [src1+src1Stride+16]
    lea             src0,           [src0+2*src0Stride]
    lea             src1,           [src1+2*src1Stride]
    movdqu          [dst],          xmm0
    movq            [dst+16],       mm0
    movdqu          [dst+dstStride], xmm1
    movq            [dst+dstStride+16], mm1
    lea             dst,            [dst+2*dstStride]
    sub             areaHeight,     2
    jne             Label_PictureAverageKernel_SSE2_WIDTH24

    XMM_RESTORE
    POP_REG 7

%if NEED_EMMS
    emms
%endif
    ret

Label_PictureAverageKernel_SSE2_WIDTH16:
    movdqu          xmm0,           [src0]
    movdqu          xmm1,           [src0+src0Stride]
	movdqu          xmm4,           [src1]
	pavgb           xmm0,           xmm4
	movdqu          xmm4,           [src1+src1Stride]
	pavgb           xmm1,           xmm4
    lea             src0,           [src0+2*src0Stride]
    lea             src1,           [src1+2*src1Stride]
    movdqu          [dst],          xmm0
    movdqu          [dst+dstStride], xmm1
    lea             dst,            [dst+2*dstStride]
    sub             areaHeight,     2
    jne             Label_PictureAverageKernel_SSE2_WIDTH16

    XMM_RESTORE
    POP_REG 7
    ret

; ----------------------------------------------------------------------------------------
	cglobal EbHevcLog2f_SSE2
	bsr rax, r0
	ret
