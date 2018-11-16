/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureOperators_C_h
#define EbPictureOperators_C_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
    
void PictureAverageKernel(
    EB_BYTE                  src0,
    EB_U32                   src0Stride,
    EB_BYTE                  src1,
    EB_U32                   src1Stride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);

void PictureAverageKernel1Line_C(
    EB_BYTE                  src0,
    EB_BYTE                  src1,
    EB_BYTE                  dst,
    EB_U32                   areaWidth);

void PictureCopyKernel(
    EB_BYTE                  src,
    EB_U32                   srcStride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight,
    EB_U32                   bytesPerSample);

void PictureAdditionKernel(
    EB_U8  *predPtr,
    EB_U32  predStride,
    EB_S16 *residualPtr,
    EB_U32  residualStride,
    EB_U8  *reconPtr,
    EB_U32  reconStride,
    EB_U32  width,
    EB_U32  height);

void PictureAdditionKernel16bit(
    EB_U16  *predPtr,
    EB_U32  predStride,
    EB_S16 *residualPtr,
    EB_U32  residualStride,
    EB_U16  *reconPtr,
    EB_U32  reconStride,
    EB_U32  width,
    EB_U32  height);

void CopyKernel8Bit(
    EB_BYTE                  src,
    EB_U32                   srcStride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);

void CopyKernel16Bit(
    EB_BYTE                  src,
    EB_U32                   srcStride,
    EB_BYTE                  dst,
    EB_U32                   dstStride,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);

void ResidualKernelSubSampled(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight,
    EB_U8    lastLine);

void ResidualKernel(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void ResidualKernel16bit(
    EB_U16   *input,
    EB_U32   inputStride,
    EB_U16   *pred,
    EB_U32   predStride,
    EB_S16  *residual,
    EB_U32   residualStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void ZeroOutCoeffKernel(
    EB_S16*                  coeffbuffer,
    EB_U32                   coeffStride,
    EB_U32                   coeffOriginIndex,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);

void FullDistortionKernel_32bit(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void FullDistortionKernelCbfZero_32bit(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void FullDistortionKernelIntra_32bit(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

EB_U64 SpatialFullDistortionKernel(
    EB_U8   *input,
    EB_U32   inputStride,
    EB_U8   *recon,
    EB_U32   reconStride,
    EB_U32   areaWidth,
    EB_U32   areaHeight);

EB_U64 Compute8x8Satd(
    EB_S16 *diff);

EB_U64 Compute8x8Satd_U8(
    EB_U8  *diff,
    EB_U64 *dcValue,
    EB_U32  srcStride);

#ifdef __cplusplus
}
#endif        
#endif // EbPictureOperators_C_h