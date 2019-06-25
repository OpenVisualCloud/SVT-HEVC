/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPackUnPack_asm_h
#define EbPackUnPack_asm_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

void EB_ENC_UnPack8BitData_SSE2_INTRIN(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,
    EB_U32       width,
    EB_U32       height);

void EB_ENC_UnPack8BitDataSafeSub_SSE2_INTRIN(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,
    EB_U32       width,
    EB_U32       height
    );

void UnpackAvg_SSE2_INTRIN(
        EB_U16 *ref16L0,
        EB_U32  refL0Stride,
        EB_U16 *ref16L1,
        EB_U32  refL1Stride,
        EB_U8  *dstPtr,
        EB_U32  dstStride,
        EB_U32  width,
        EB_U32  height);


#ifdef __cplusplus
}
#endif
#endif // EbPackUnPack_asm_h

