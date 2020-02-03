/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTransforms_SSE2_h
#define EbTransforms_SSE2_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

extern EB_ALIGN(16) const EB_S16 DstTransformAsmConst_SSE2[88];
extern EB_ALIGN(16) const EB_S16 InvTransformAsmConst_SSE2[1512];
extern EB_ALIGN(16) const EB_S16 InvDstTransformAsmConst_SSE2[72];


extern void Transform4x4_SSE2_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

extern void DstTransform4x4_SSE2_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

extern void Transform8x8_SSE2_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

extern void Transform16x16_SSE2(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);


extern void Transform32x32_SSE2(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

extern void InvTransform8x8_SSE2_INTRIN(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

extern void InvTransform4x4_SSE2_INTRIN(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

extern void InvDstTransform4x4_SSE2_INTRIN(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

extern void PfreqN4Transform8x8_SSE2_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

extern void PfreqTransform8x8_SSE2_INTRIN(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void PfreqEstimateInvTransform32x32_SSE2(
    EB_S16 *src,
    const EB_U32  src_stride,
    EB_S16 *dst,
    const EB_U32  dst_stride,
    EB_S16 *intermediate,
    EB_U32  addshift);

void PfreqN4Transform32x32_SSE2(
    EB_S16 *src,
    const EB_U32 src_stride,
    EB_S16 *dst,
    const EB_U32 dst_stride,
    EB_S16 *intermediate,
    EB_U32 addshift);

void PfreqTransform32x32_SSE2(
    EB_S16 *src,
    const EB_U32 src_stride,
    EB_S16 *dst,
    const EB_U32 dst_stride,
    EB_S16 *intermediate,
    EB_U32 addshift);

void PfreqTransform16x16_SSE2(
    EB_S16 *src,
    const EB_U32 src_stride,
    EB_S16 *dst,
    const EB_U32 dst_stride,
    EB_S16 *intermediate,
    EB_U32 addshift);

void PfreqN4Transform16x16_SSE2(
    EB_S16 *src,
    const EB_U32 src_stride,
    EB_S16 *dst,
    const EB_U32 dst_stride,
    EB_S16 *intermediate,
    EB_U32 addshift);

void EstimateInvTransform32x32_SSE2(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void EstimateInvTransform16x16_SSE2(
    EB_S16                  *src,
    EB_U32                   src_stride,
    EB_S16                  *dst,
    EB_U32                   dst_stride,
    EB_S16                  *intermediate,
    EB_U32                   addshift);

#ifdef __cplusplus
}
#endif
#endif // EbTransforms_SSE2_h

