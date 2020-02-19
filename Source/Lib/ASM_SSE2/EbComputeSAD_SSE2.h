/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeSAD_SSE2_h
#define EbComputeSAD_SSE2_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

extern EB_U32 Compute4xMSad_SSE2_INTRIN(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width);                         // input parameter, block width (N)

extern EB_U32 Compute8xMSad_SSE2_INTRIN(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width);                          // input parameter, block width (N)

extern EB_U32 Compute16xMSad_SSE2_INTRIN(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width);                         // input parameter, block width (N)

extern EB_U32 Compute32xMSad_SSE2_INTRIN(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width);                         // input parameter, block width (N)

extern EB_U32 Compute64xMSad_SSE2_INTRIN(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width);                         // input parameter, block width (N)

extern EB_U32 CombinedAveraging4xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width);

extern EB_U32 CombinedAveraging8xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width);

extern EB_U32 CombinedAveraging16xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width);

extern EB_U32 CombinedAveraging24xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width);

extern EB_U32 CombinedAveraging32xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width);

extern EB_U32 CombinedAveraging48xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width);

extern EB_U32 CombinedAveraging64xMSAD_SSE2_INTRIN(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width);

#ifdef __cplusplus
}
#endif
#endif // EbComputeSAD_SSE2_h