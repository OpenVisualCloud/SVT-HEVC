/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPackUnPack_AVX2_h
#define EbPackUnPack_AVX2_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NON_AVX512_SUPPORT
    void EB_ENC_msbUnPack2D_AVX512_INTRIN(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U8       *outnBitBuffer,
    EB_U32       out8Stride,
    EB_U32       outnStride,
    EB_U32       width,
    EB_U32       height);
#else
    void EB_ENC_msbUnPack2D_AVX2_INTRIN(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U8       *outnBitBuffer,
    EB_U32       out8Stride,
    EB_U32       outnStride,
    EB_U32       width,
    EB_U32       height);
#endif


#ifdef __cplusplus
}
#endif

#endif // EbPackUnPack_AVX2_h
