/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTransforms_SSE4_1_h
#define EbTransforms_SSE4_1_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
extern EB_ALIGN(16) const EB_S16 TransformAsmConst_SSE4_1[1632];

extern void Transform8x8_SSE4_1_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);
extern void PfreqTransform8x8_SSE4_1_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);
extern void PfreqN4Transform8x8_SSE4_1_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

#ifdef __cplusplus
}
#endif
#endif // EbTransforms_SSE4_1_h