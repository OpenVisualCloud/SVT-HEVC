; 
; Copyright(c) 2018 Intel Corporation
; SPDX - License - Identifier: BSD - 2 - Clause - Patent
; 

%include "x64inc.asm"
%include "x64Macro.asm"

section .text

%define boShift         5

%define inputSamplePtr  r0
%define inputStride     r1
%define reconSamplePtr  r2
%define reconStride     r3
%define lcuWidth        r4
%define lcuWidthW       r4w
%define lcuHeight       r5
%define lcuHeightW      r5w
%define eoCount         r1

%define rTemp           r8
%define reconStrideTemp r9
%define rCountX         r10

%define xmm_n1          xmm1
%define xmm_1           xmm2
%define xmm_n3          xmm3
%define xmm_n4          xmm4
%define xmm_FF          xmm14

%define OFFSET_EO_DIFF_0  0
%define OFFSET_EO_COUNT_0 (OFFSET_EO_DIFF_0 +40h)
%define OFFSET_EO_DIFF_1  (OFFSET_EO_COUNT_0+40h)
%define OFFSET_EO_COUNT_1 (OFFSET_EO_DIFF_1 +40h)
%define OFFSET_EO_DIFF_2  (OFFSET_EO_COUNT_1+40h)
%define OFFSET_EO_COUNT_2 (OFFSET_EO_DIFF_2 +40h)
%define OFFSET_EO_DIFF_3  (OFFSET_EO_COUNT_2+40h)
%define OFFSET_EO_COUNT_3 (OFFSET_EO_DIFF_3 +40h)

%macro MACRO_INIT_EO 2 ; OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0
    movdqa          [rTemp+%1],     xmm0
    movdqa          [rTemp+%1+10h], xmm0
    movdqa          [rTemp+%1+20h], xmm0
    movdqa          [rTemp+%1+30h], xmm0
    movdqa          [rTemp+%2],     xmm0
    movdqa          [rTemp+%2+10h], xmm0
    movdqa          [rTemp+%2+20h], xmm0
    movdqa          [rTemp+%2+30h], xmm0
%endmacro

%macro MACRO_GATHER_BO 5
    pextrw          rIndex,         %3, %5
    pextrw          rDiff,          %4, %5
    shl             rDiff,          48
    sar             rDiff,          48
    add             dword [%1+4*rIndex], rDiffD ; boDiff[boIndex] += diff;
    add             word [%2+2*rIndex], 1       ; boCount[boIndex] ++;
%endmacro

%macro MACRO_CALC_EO_INDEX 2
    movdqu          xmm11,          [%1]
    movdqu          xmm12,          [%1+16]
    psubw           xmm11,          xmm5
    psubw           xmm12,          xmm6
    packsswb        xmm11,          xmm12
    movdqa          xmm12,          xmm11
    pcmpgtb         xmm11,          xmm0
    pcmpgtb         xmm12,          xmm_n1
    paddb           xmm11,          xmm12       ; signLeft - 1

    movdqu          xmm13,          [%2]
    movdqu          xmm9,           [%2+16]
    psubw           xmm13,          xmm5
    psubw           xmm9,           xmm6
    packsswb        xmm13,          xmm9
    movdqa          xmm9,           xmm13
    pcmpgtb         xmm13,          xmm0
    pcmpgtb         xmm9,           xmm_n1
    paddb           xmm13,          xmm9        ; signRight - 1
    paddb           xmm11,          xmm13       ; signLeft + signRight - 2
%endmacro

%macro MACRO_CALC_EO_INDEX_HALF 2
    movdqu          xmm11,          [%1]
    psubw           xmm11,          xmm5
    packsswb        xmm11,          xmm12
    movdqa          xmm12,          xmm11
    pcmpgtb         xmm11,          xmm0
    pcmpgtb         xmm12,          xmm_n1
    paddb           xmm11,          xmm12       ; signLeft - 1

    movdqu          xmm13,          [%2]
    psubw           xmm13,          xmm5
    packsswb        xmm13,          xmm9
    movdqa          xmm9,           xmm13
    pcmpgtb         xmm13,          xmm0
    pcmpgtb         xmm9,           xmm_n1
    paddb           xmm13,          xmm9        ; signRight - 1
    paddb           xmm11,          xmm13       ; signLeft + signRight - 2
%endmacro

%macro MACRO_GATHER_EO_X 3
    movdqa          xmm12,          xmm11
    pcmpeqb         xmm12,          %3
    movdqa          xmm13,          xmm7
    movdqa          xmm10,          xmm12
    punpcklbw       xmm10,          xmm10
    pand            xmm13,          xmm10
    pmaddwd         xmm13,          xmm15
    paddd           xmm13,          [rTemp+%1]
    movdqa          xmm9,           xmm8
    movdqa          xmm10,          xmm12
    punpckhbw       xmm10,          xmm10
    pand            xmm9,           xmm10
    pmaddwd         xmm9,           xmm15
    paddd           xmm13,          xmm9
    movdqa          [rTemp+%1],     xmm13
    pand            xmm12,          xmm_1
    psadbw          xmm12,          xmm0
    paddd           xmm12,          [rTemp+%2]
    movdqa          [rTemp+%2],     xmm12
%endmacro

%macro MACRO_GATHER_EO_X_HALF 3
    movdqa          xmm12,          xmm11
    pcmpeqb         xmm12,          %3
    movdqa          xmm13,          xmm7
    movdqa          xmm10,          xmm12
    punpcklbw       xmm10,          xmm10
    pand            xmm13,          xmm10
    pmaddwd         xmm13,          xmm15
    paddd           xmm13,          [rTemp+%1]
    movdqa          [rTemp+%1],     xmm13
    pand            xmm12,          xmm_1
    psadbw          xmm12,          xmm0
    paddd           xmm12,          [rTemp+%2]
    movdqa          [rTemp+%2],     xmm12
%endmacro

%macro MACRO_GATHER_EO 2 ; OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0
    MACRO_GATHER_EO_X %1+0,   %2+0,   xmm_n4
    MACRO_GATHER_EO_X %1+10h, %2+10h, xmm_n3
    MACRO_GATHER_EO_X %1+20h, %2+20h, xmm_n1
    MACRO_GATHER_EO_X %1+30h, %2+30h, xmm0
%endmacro

%macro MACRO_GATHER_EO_HALF 2 ; OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0
    MACRO_GATHER_EO_X_HALF %1+0,   %2+0,   xmm_n4
    MACRO_GATHER_EO_X_HALF %1+10h, %2+10h, xmm_n3
    MACRO_GATHER_EO_X_HALF %1+20h, %2+20h, xmm_n1
    MACRO_GATHER_EO_X_HALF %1+30h, %2+30h, xmm0
%endmacro

%macro MACRO_SAVE_EO_X 4
    movdqa          xmm9,           [rTemp+%1]
    movdqa          xmm10,          xmm9
    psrldq          xmm9,           8
    paddd           xmm9,           xmm10
    movdqa          xmm10,          xmm9
    psrldq          xmm9,           4
    paddd           xmm9,           xmm10
    movd            [eoDiff+%2],    xmm9
    movdqa          xmm11,          [rTemp+%3]
    movdqa          xmm12,          xmm11
    psrldq          xmm11,          8
    paddd           xmm11,          xmm12
    movd            rDiffD,         xmm11
    mov             word [eoCount+%4], rDiffW
%endmacro

%macro MACRO_SAVE_EO_Y 4
    movdqa          xmm9,           [rTemp+%1]
    movdqa          xmm10,          xmm9
    psrldq          xmm9,           8
    paddd           xmm9,           xmm10
    movdqa          xmm10,          xmm9
    psrldq          xmm9,           4
    paddd           xmm9,           xmm10
    movd            [eoDiff+%2],    xmm9
    movdqa          xmm11,          [rTemp+%3]
    movdqa          xmm12,          xmm11
    psrldq          xmm11,          8
    paddd           xmm11,          xmm12
    movd            rDiffD,         xmm11
    sub             rDiff,          lcuWidth
    mov             word [eoCount+%4], rDiffW
%endmacro

%macro MACRO_SAVE_EO 4
    MACRO_SAVE_EO_X %1+  0, %2+ 0, %3+  0, %4+0
    MACRO_SAVE_EO_X %1+10h, %2+ 4, %3+10h, %4+2
    MACRO_SAVE_EO_X %1+20h, %2+ 8, %3+20h, %4+4
    MACRO_SAVE_EO_Y %1+30h, %2+12, %3+30h, %4+6
%endmacro

; ----------------------------------------------------------------------------------------

cglobal GatherSaoStatisticsLcu16bit_SSE2

%define boDiff          r6
%define boCount         r7
%define eoDiff          r0
%define rDiff           r11
%define rDiffD          r11d
%define rDiffW          r11w
%define rIndex          r12
%define rCountY         r13

; Requirement: lcuWidth = 28, 56 or lcuWidth % 16 = 0
; Requirement: lcuHeight > 2

    PUSH_REG 7
    PUSH_REG 8
    PUSH_REG 9
    PUSH_REG 10
    PUSH_REG 11
    PUSH_REG 12
    PUSH_REG 13

    pxor            xmm0,           xmm0
    MACRO_INIT_XMM  xmm15,          r6, 0x0001000100010001 ;  0x0001000100010001 0001000100010001
    MACRO_INIT_XMM  xmm_n1,         r6, 0xFFFFFFFFFFFFFFFF ;  0xFFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF
    movdqa          xmm_n3,         xmm_n1
    paddb           xmm_n3,         xmm_n1
    paddb           xmm_n3,         xmm_n1      ; 0xFDFDFDFDFDFDFDFD FDFDFDFDFDFDFDFD
    movdqa          xmm_n4,         xmm_n1
    paddb           xmm_n4,         xmm_n3      ; 0xFCFCFCFCFCFCFCFC FCFCFCFCFCFCFCFC
    movdqa          xmm_1,          xmm0
    psubb           xmm_1,          xmm_n1

    GET_PARAM_5UXD
    GET_PARAM_6UXD
    GET_PARAM_7Q
    GET_PARAM_8Q boCount
    SUB_RSP         512+16
    mov             rTemp,          rsp
    add             rTemp,          15
    and             rTemp,          ~15         ; make rTemp 16-byte aligned

    ; Initialize SAO Arrays
    ; BO
    movdqu          [boDiff],       xmm0
    movdqu          [boDiff+10h],   xmm0
    movdqu          [boDiff+20h],   xmm0
    movdqu          [boDiff+30h],   xmm0
    movdqu          [boDiff+40h],   xmm0
    movdqu          [boDiff+50h],   xmm0
    movdqu          [boDiff+60h],   xmm0
    movdqu          [boDiff+70h],   xmm0
    movdqu          [boCount],      xmm0
    movdqu          [boCount+10h],  xmm0
    movdqu          [boCount+20h],  xmm0
    movdqu          [boCount+30h],  xmm0

    ; EO
    MACRO_INIT_EO OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0
    MACRO_INIT_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1
    MACRO_INIT_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2
    MACRO_INIT_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    sub             lcuHeight,      2
    mov             rCountY,        lcuHeight
    lea             inputSamplePtr, [inputSamplePtr+2*inputStride+2]
    lea             reconSamplePtr, [reconSamplePtr+2]

    cmp             lcuWidth,       16
    je              Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE16

    cmp             lcuWidth,       28
    je              Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE28

    cmp             lcuWidth,       56
    je              Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE56

    sub             lcuWidth,       16
    sub             inputStride,    lcuWidth
    mov             reconStrideTemp, reconStride
    sub             reconStrideTemp, lcuWidth

;Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE16TIME:
    movdqa          xmm_FF,         xmm_n1
    psrldq          xmm_FF,         2

Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE16TIME_01:
    mov             rCountX,        lcuWidth

Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE16TIME_02:
    ;----------- 0-15 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+16]
    movdqu          xmm7,           [inputSamplePtr]
    movdqu          xmm8,           [inputSamplePtr+16]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    ; BO
    movdqa          xmm9,           xmm5
    movdqa          xmm10,          xmm6
    psrlw           xmm9,           boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;
    psrlw           xmm10,          boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;

    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 0
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 1
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 2
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 3
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 4
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 5
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 6
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 7
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 0
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 1
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 2
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 3
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 4
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 5
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 6
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 7

    ; EO-0
    MACRO_CALC_EO_INDEX reconSamplePtr+2*reconStride-2, reconSamplePtr+2*reconStride+2
    MACRO_GATHER_EO OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr, reconSamplePtr+4*reconStride
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+32]
    lea             reconSamplePtr, [reconSamplePtr+32]
    sub             rCountX,        16
    jne             Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE16TIME_02

    ;----------- 48-61 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+16]
    movdqu          xmm7,           [inputSamplePtr]
    movdqu          xmm8,           [inputSamplePtr+16]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    ; BO
    movdqa          xmm9,           xmm5
    movdqa          xmm10,          xmm6
    psrlw           xmm9,           boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;
    psrlw           xmm10,          boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;

    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 0
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 1
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 2
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 3
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 4
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 5
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 6
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 7
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 0
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 1
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 2
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 3
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 4
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 5
;   MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 6 ; skip last 2 samples
;   MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 7 ; skip last 2 samples

    pslldq          xmm8,           4           ; skip last 2 samples
    psrldq          xmm8,           4           ; skip last 2 samples

    ; EO-0
    MACRO_CALC_EO_INDEX reconSamplePtr+2*reconStride-2, reconSamplePtr+2*reconStride+2
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr, reconSamplePtr+4*reconStride
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+2*inputStride]
    lea             reconSamplePtr, [reconSamplePtr+2*reconStrideTemp]
    sub             rCountY,        1
    jne             Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE16TIME_01

    mov             lcuWidth,       2
    jmp             Label_GatherSaoStatisticsLcu_END

Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE56:
    movdqa          xmm_FF,         xmm_n1
    psrldq          xmm_FF,         10

    sub             lcuWidth,       8
    sub             inputStride,    lcuWidth
    mov             reconStrideTemp, reconStride
    sub             reconStrideTemp, lcuWidth

Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE56_01:
    mov             rCountX,        lcuWidth

Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE56_02:
    ;----------- 0-15 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+16]
    movdqu          xmm7,           [inputSamplePtr]
    movdqu          xmm8,           [inputSamplePtr+16]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    ; BO
    movdqa          xmm9,           xmm5
    movdqa          xmm10,          xmm6
    psrlw           xmm9,           boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;
    psrlw           xmm10,          boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;

    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 0
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 1
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 2
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 3
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 4
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 5
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 6
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 7
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 0
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 1
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 2
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 3
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 4
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 5
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 6
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 7

    ; EO-0
    MACRO_CALC_EO_INDEX reconSamplePtr+2*reconStride-2, reconSamplePtr+2*reconStride+2
    MACRO_GATHER_EO OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr, reconSamplePtr+4*reconStride
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+32]
    lea             reconSamplePtr, [reconSamplePtr+32]
    sub             rCountX,        16
    jne             Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE56_02

    ;----------- 48-53 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm7,           [inputSamplePtr]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    ; BO
    movdqa          xmm9,           xmm5
    psrlw           xmm9,           boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;

    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 0
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 1
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 2
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 3
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 4
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 5

    pslldq          xmm7,           4          ; skip last 10 samples
    psrldq          xmm7,           4          ; skip last 10 samples

    ; EO-0
    MACRO_CALC_EO_INDEX_HALF reconSamplePtr+2*reconStride-2, reconSamplePtr+2*reconStride+2
    pand            xmm11,          xmm_FF      ; skip last 10 samples
    MACRO_GATHER_EO_HALF OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0

    ; EO-90
    MACRO_CALC_EO_INDEX_HALF reconSamplePtr, reconSamplePtr+4*reconStride
    pand            xmm11,          xmm_FF      ; skip last 10 samples
    MACRO_GATHER_EO_HALF OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX_HALF reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    pand            xmm11,          xmm_FF      ; skip last 10 samples
    MACRO_GATHER_EO_HALF OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX_HALF reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    pand            xmm11,          xmm_FF      ; skip last 10 samples
    MACRO_GATHER_EO_HALF OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+2*inputStride]
    lea             reconSamplePtr, [reconSamplePtr+2*reconStrideTemp]
    sub             rCountY,        1
    jne             Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE56_01

    mov             lcuWidth,       10
    jmp             Label_GatherSaoStatisticsLcu_END

Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE28:
    movdqa          xmm_FF,         xmm_n1
    psrldq          xmm_FF,         6

Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE28_01:
    ;----------- 0-15 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+16]
    movdqu          xmm7,           [inputSamplePtr]
    movdqu          xmm8,           [inputSamplePtr+16]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    ; BO
    movdqa          xmm9,           xmm5
    movdqa          xmm10,          xmm6
    psrlw           xmm9,           boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;
    psrlw           xmm10,          boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;

    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 0
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 1
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 2
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 3
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 4
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 5
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 6
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 7
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 0
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 1
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 2
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 3
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 4
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 5
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 6
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 7

    ; EO-0
    MACRO_CALC_EO_INDEX reconSamplePtr+2*reconStride-2, reconSamplePtr+2*reconStride+2
    MACRO_GATHER_EO OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr, reconSamplePtr+4*reconStride
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    ;----------- 16-25 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride+32]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+48]
    movdqu          xmm7,           [inputSamplePtr+32]
    movdqu          xmm8,           [inputSamplePtr+48]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    ; BO
    movdqa          xmm9,           xmm5
    movdqa          xmm10,          xmm6
    psrlw           xmm9,           boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;
    psrlw           xmm10,          boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;

    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 0
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 1
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 2
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 3
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 4
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 5
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 6
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 7
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 0
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 1
;   MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 2 ; skip last 6 samples
;   MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 3 ; skip last 6 samples
;   MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 4 ; skip last 6 samples
;   MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 5 ; skip last 6 samples
;   MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 6 ; skip last 6 samples
;   MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 7 ; skip last 6 samples

    pslldq          xmm8,           12          ; skip last 6 samples
    psrldq          xmm8,           12          ; skip last 6 samples

    ; EO-0
    MACRO_CALC_EO_INDEX reconSamplePtr+2*reconStride+30, reconSamplePtr+2*reconStride+34
    pand            xmm11,          xmm_FF      ; skip last 6 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr+32, reconSamplePtr+4*reconStride+32
    pand            xmm11,          xmm_FF      ; skip last 6 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr+30, reconSamplePtr+4*reconStride+34
    pand            xmm11,          xmm_FF      ; skip last 6 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+34, reconSamplePtr+4*reconStride+30
    pand            xmm11,          xmm_FF      ; skip last 6 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+2*inputStride]
    lea             reconSamplePtr, [reconSamplePtr+2*reconStride]
    sub             rCountY,        1
    jne             Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE28_01

    mov             lcuWidth,       6
    jmp             Label_GatherSaoStatisticsLcu_END

Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE16:
    movdqa          xmm_FF,         xmm_n1
    psrldq          xmm_FF,         2

Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE16_01:
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+16]
    movdqu          xmm7,           [inputSamplePtr]
    movdqu          xmm8,           [inputSamplePtr+16]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    ; BO
    movdqa          xmm9,           xmm5
    movdqa          xmm10,          xmm6
    psrlw           xmm9,           boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;
    psrlw           xmm10,          boShift     ; boIndex = temporalReconSamplePtr[i] >> boShift;

    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 0
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 1
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 2
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 3
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 4
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 5
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 6
    MACRO_GATHER_BO boDiff, boCount, xmm9,  xmm7, 7
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 0
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 1
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 2
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 3
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 4
    MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 5
;   MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 6 ; skip last 2 samples
;   MACRO_GATHER_BO boDiff, boCount, xmm10, xmm8, 7 ; skip last 2 samples

    pslldq          xmm8,           4           ; skip last 2 samples
    psrldq          xmm8,           4           ; skip last 2 samples

    ; EO-0
    MACRO_CALC_EO_INDEX reconSamplePtr+2*reconStride-2, reconSamplePtr+2*reconStride+2
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_0, OFFSET_EO_COUNT_0

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr, reconSamplePtr+4*reconStride
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+2*inputStride]
    lea             reconSamplePtr, [reconSamplePtr+2*reconStride]
    sub             rCountY,        1
    jne             Label_GatherSaoStatisticsLcu16bit_SSE2_SIZE16_01

    mov             lcuWidth,       2

Label_GatherSaoStatisticsLcu_END:
    GET_PARAM_9Q    eoDiff
    GET_PARAM_10Q   eoCount
    imul            lcuWidthW,      lcuHeightW

    MACRO_SAVE_EO OFFSET_EO_DIFF_0,  0, OFFSET_EO_COUNT_0,  0
    MACRO_SAVE_EO OFFSET_EO_DIFF_1, 20, OFFSET_EO_COUNT_1, 10
    MACRO_SAVE_EO OFFSET_EO_DIFF_2, 40, OFFSET_EO_COUNT_2, 20
    MACRO_SAVE_EO OFFSET_EO_DIFF_3, 60, OFFSET_EO_COUNT_3, 30

    ADD_RSP         512+16
    POP_REG 13
    POP_REG 12
    POP_REG 11
    POP_REG 10
    POP_REG 9
    POP_REG 8
    POP_REG 7
    ret

%undef boDiff
%undef boCount
%undef eoDiff
%undef rCountX
%undef rCountY
%undef rIndex

; ----------------------------------------------------------------------------------------

cglobal GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2

%define eoDiff          r6
%define rDiff           r0
%define rDiffD          r0d
%define rDiffW          r0w
%define rCountX         r6
%define rCountY         r7

; Requirement: lcuWidth = 28, 56 or lcuWidth % 16 = 0
; Requirement: lcuHeight > 2

    PUSH_REG 7
    PUSH_REG 8
    PUSH_REG 9

    pxor            xmm0,           xmm0
    MACRO_INIT_XMM  xmm15,          r6, 0x0001000100010001 ;  0x0001000100010001 0001000100010001
    MACRO_INIT_XMM  xmm_n1,         r6, 0xFFFFFFFFFFFFFFFF ;  0xFFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF
    movdqa          xmm_n3,         xmm_n1
    paddb           xmm_n3,         xmm_n1
    paddb           xmm_n3,         xmm_n1      ; 0xFDFDFDFDFDFDFDFD FDFDFDFDFDFDFDFD
    movdqa          xmm_n4,         xmm_n1
    paddb           xmm_n4,         xmm_n3      ; 0xFCFCFCFCFCFCFCFC FCFCFCFCFCFCFCFC
    movdqa          xmm_1,          xmm0
    psubb           xmm_1,          xmm_n1

    GET_PARAM_5UXD
    GET_PARAM_6UXD
    SUB_RSP         512+16
    mov             rTemp,          rsp
    add             rTemp,          15
    and             rTemp,          ~15         ; make rTemp 16-byte aligned

    ; Initialize SAO Arrays
    ; EO
    MACRO_INIT_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1
    MACRO_INIT_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2
    MACRO_INIT_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    sub             lcuHeight,      2
    mov             rCountY,        lcuHeight
    lea             inputSamplePtr, [inputSamplePtr+2*inputStride+2]
    lea             reconSamplePtr, [reconSamplePtr+2]

    cmp             lcuWidth,       16
    je              Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE16

    cmp             lcuWidth,       28
    je              Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE28

    cmp             lcuWidth,       56
    je              Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE56

    sub             lcuWidth,       16
    sub             inputStride,    lcuWidth
    mov             reconStrideTemp, reconStride
    sub             reconStrideTemp, lcuWidth

;Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE16TIME:
    movdqa          xmm_FF,         xmm_n1
    psrldq          xmm_FF,         2

Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE16TIME_01:
    mov             rCountX,        lcuWidth

Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE16TIME_02:
    ;----------- 0-15 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+16]
    movdqu          xmm7,           [inputSamplePtr]
    movdqu          xmm8,           [inputSamplePtr+16]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr, reconSamplePtr+4*reconStride
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+32]
    lea             reconSamplePtr, [reconSamplePtr+32]
    sub             rCountX,        16
    jne             Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE16TIME_02

    ;----------- 48-61 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+16]
    movdqu          xmm7,           [inputSamplePtr]
    movdqu          xmm8,           [inputSamplePtr+16]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    pslldq          xmm8,           4           ; skip last 2 samples
    psrldq          xmm8,           4           ; skip last 2 samples

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr, reconSamplePtr+4*reconStride
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+2*inputStride]
    lea             reconSamplePtr, [reconSamplePtr+2*reconStrideTemp]
    sub             rCountY,        1
    jne             Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE16TIME_01

    mov             lcuWidth,       2
    jmp             Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_END

Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE56:
    movdqa          xmm_FF,         xmm_n1
    psrldq          xmm_FF,         10

    sub             lcuWidth,       8
    sub             inputStride,    lcuWidth
    mov             reconStrideTemp, reconStride
    sub             reconStrideTemp, lcuWidth

Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE56_01:
    mov             rCountX,        lcuWidth

Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE56_02:
    ;----------- 0-15 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+16]
    movdqu          xmm7,           [inputSamplePtr]
    movdqu          xmm8,           [inputSamplePtr+16]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr, reconSamplePtr+4*reconStride
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+32]
    lea             reconSamplePtr, [reconSamplePtr+32]
    sub             rCountX,        16
    jne             Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE56_02

    ;----------- 48-53 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm7,           [inputSamplePtr]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    pslldq          xmm7,           4          ; skip last 10 samples
    psrldq          xmm7,           4          ; skip last 10 samples

    ; EO-90
    MACRO_CALC_EO_INDEX_HALF reconSamplePtr, reconSamplePtr+4*reconStride
    pand            xmm11,          xmm_FF      ; skip last 10 samples
    MACRO_GATHER_EO_HALF OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX_HALF reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    pand            xmm11,          xmm_FF      ; skip last 10 samples
    MACRO_GATHER_EO_HALF OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX_HALF reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    pand            xmm11,          xmm_FF      ; skip last 10 samples
    MACRO_GATHER_EO_HALF OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+2*inputStride]
    lea             reconSamplePtr, [reconSamplePtr+2*reconStrideTemp]
    sub             rCountY,        1
    jne             Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE56_01

    mov             lcuWidth,       10
    jmp             Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_END

Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE28:
    movdqa          xmm_FF,         xmm_n1
    psrldq          xmm_FF,         6

Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE28_01:
    ;----------- 0-15 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+16]
    movdqu          xmm7,           [inputSamplePtr]
    movdqu          xmm8,           [inputSamplePtr+16]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr, reconSamplePtr+4*reconStride
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    ;----------- 16-25 -----------
    movdqu          xmm5,           [reconSamplePtr+2*reconStride+32]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+48]
    movdqu          xmm7,           [inputSamplePtr+32]
    movdqu          xmm8,           [inputSamplePtr+48]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    pslldq          xmm8,           12          ; skip last 6 samples
    psrldq          xmm8,           12          ; skip last 6 samples

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr+32, reconSamplePtr+4*reconStride+32
    pand            xmm11,          xmm_FF      ; skip last 6 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr+30, reconSamplePtr+4*reconStride+34
    pand            xmm11,          xmm_FF      ; skip last 6 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+34, reconSamplePtr+4*reconStride+30
    pand            xmm11,          xmm_FF      ; skip last 6 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+2*inputStride]
    lea             reconSamplePtr, [reconSamplePtr+2*reconStride]
    sub             rCountY,        1
    jne             Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE28_01

    mov             lcuWidth,       6
    jmp             Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_END

Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE16:
    movdqa          xmm_FF,         xmm_n1
    psrldq          xmm_FF,         2

Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE16_01:
    movdqu          xmm5,           [reconSamplePtr+2*reconStride]
    movdqu          xmm6,           [reconSamplePtr+2*reconStride+16]
    movdqu          xmm7,           [inputSamplePtr]
    movdqu          xmm8,           [inputSamplePtr+16]
    psubw           xmm7,           xmm5        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];
    psubw           xmm8,           xmm6        ; diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

    pslldq          xmm8,           4           ; skip last 2 samples
    psrldq          xmm8,           4           ; skip last 2 samples

    ; EO-90
    MACRO_CALC_EO_INDEX reconSamplePtr, reconSamplePtr+4*reconStride
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_1, OFFSET_EO_COUNT_1

    ; EO-135
    MACRO_CALC_EO_INDEX reconSamplePtr-2, reconSamplePtr+4*reconStride+2
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_2, OFFSET_EO_COUNT_2

    ; EO-45
    MACRO_CALC_EO_INDEX reconSamplePtr+2, reconSamplePtr+4*reconStride-2
    pand            xmm11,          xmm_FF      ; skip last 2 samples
    MACRO_GATHER_EO OFFSET_EO_DIFF_3, OFFSET_EO_COUNT_3

    lea             inputSamplePtr, [inputSamplePtr+2*inputStride]
    lea             reconSamplePtr, [reconSamplePtr+2*reconStride]
    sub             rCountY,        1
    jne             Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_16bit_SSE2_SIZE16_01

    mov             lcuWidth,       2

Label_GatherSaoStatisticsLcu_OnlyEo_90_45_135_END:
    GET_PARAM_7Q
    GET_PARAM_8Q    eoCount
    imul            lcuWidthW,      lcuHeightW

    MACRO_SAVE_EO OFFSET_EO_DIFF_1, 20, OFFSET_EO_COUNT_1, 10
    MACRO_SAVE_EO OFFSET_EO_DIFF_2, 40, OFFSET_EO_COUNT_2, 20
    MACRO_SAVE_EO OFFSET_EO_DIFF_3, 60, OFFSET_EO_COUNT_3, 30

    ADD_RSP         512+16
    POP_REG 9
    POP_REG 8
    POP_REG 7
    ret

%undef eoDiff
%undef rDiff
%undef rDiffD
%undef rDiffW
%undef rCountX
%undef rCountY

; ----------------------------------------------------------------------------------------
