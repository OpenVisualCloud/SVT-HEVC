/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#ifndef EBMCP_AVX2_H
#define EBMCP_AVX2_H

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

#define USE_PRE_COMPUTE             0

// extern EB_ALIGN(16) const EB_S16 IntraPredictionConst_AVX2[344];

void ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_U16 *dst, EB_U32 dstStride, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);
void ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN(EB_U16 *refPic, EB_U32 srcStride, EB_S16 *dst, EB_U32 puWidth, EB_U32 puHeight, EB_S16 *firstPassIFDst, EB_U32 fracPosx, EB_U32 fracPosy);

#ifdef __cplusplus
}
#endif
#endif // EBMCP_AVX2_H
