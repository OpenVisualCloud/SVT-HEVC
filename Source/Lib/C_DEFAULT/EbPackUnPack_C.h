/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPackUnPack_C_h
#define EbPackUnPack_C_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif



void EB_ENC_msbPack2D(
    EB_U8     *in8BitBuffer,
    EB_U32     in8Stride,
    EB_U8     *innBitBuffer,
    EB_U16    *out16BitBuffer,
    EB_U32     innStride,
    EB_U32     outStride,
    EB_U32     width,
    EB_U32     height);

void CompressedPackmsb(
    EB_U8     *in8BitBuffer,
    EB_U32     in8Stride,
    EB_U8     *innBitBuffer,
    EB_U16    *out16BitBuffer,
    EB_U32     innStride,
    EB_U32     outStride,
    EB_U32     width,
    EB_U32     height);
void CPack_C(
    const EB_U8     *innBitBuffer,
    EB_U32     innStride,
    EB_U8     *inCompnBitBuffer,
    EB_U32     outStride,
    EB_U8    *localCache,
    EB_U32     width,
    EB_U32     height);



void EB_ENC_msbUnPack2D(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U8       *outnBitBuffer,
    EB_U32       out8Stride,
    EB_U32       outnStride,
    EB_U32       width,
    EB_U32       height);
void UnPack8BitData(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,
    EB_U32       width,
    EB_U32       height);
void UnpackAvg(
    EB_U16      *ref16L0,
    EB_U32       refL0Stride,
    EB_U16      *ref16L1,
    EB_U32       refL1Stride,
    EB_U8       *dstPtr,
    EB_U32       dstStride,
    EB_U32       width,
    EB_U32       height);

void UnPack8BitDataSafeSub(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,
    EB_U32       width,
    EB_U32       height
);

void UnpackAvgSafeSub(
    EB_U16      *ref16L0,
    EB_U32       refL0Stride,
    EB_U16      *ref16L1,
    EB_U32       refL1Stride,
    EB_U8       *dstPtr,
    EB_U32       dstStride,
    EB_U32       width,
    EB_U32       height);


#ifdef __cplusplus
}
#endif
#endif // EbPackUnPack_C_h
