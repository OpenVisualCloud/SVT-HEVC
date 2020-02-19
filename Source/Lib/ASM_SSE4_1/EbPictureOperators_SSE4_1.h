/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureOperators_SSE4_1_h
#define EbPictureOperators_SSE4_1_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

EB_U64 Compute8x8Satd_SSE4(
    EB_S16 *diff);       // input parameter, diff samples Ptr

EB_U64 Compute8x8Satd_U8_SSE4(
    EB_U8  *src,       // input parameter, diff samples Ptr
    EB_U64 *dcValue,
    EB_U32  srcStride);

EB_U64 SpatialFullDistortionKernel4x4_SSSE3_INTRIN(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *recon,
    EB_U32   reconStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);

EB_U64 SpatialFullDistortionKernel8x8_SSSE3_INTRIN(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *recon,
    EB_U32   reconStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);

EB_U64 SpatialFullDistortionKernel16MxN_SSSE3_INTRIN(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *recon,
    EB_U32   reconStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);

#ifdef __cplusplus
}
#endif
#endif // EbPictureOperators_SSE4_1_h

