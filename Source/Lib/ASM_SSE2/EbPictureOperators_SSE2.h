/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureOperators_SSE2_h
#define EbPictureOperators_SSE2_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

void FullDistortionKernel4x4_32bit_BT_SSE2(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void FullDistortionKernelCbfZero4x4_32bit_BT_SSE2(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void FullDistortionKernelIntra4x4_32bit_BT_SSE2(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void FullDistortionKernel8x8_32bit_BT_SSE2(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void FullDistortionKernelCbfZero8x8_32bit_BT_SSE2(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void FullDistortionKernelIntra8x8_32bit_BT_SSE2(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void FullDistortionKernelIntra16MxN_32bit_BT_SSE2(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

void FullDistortionKernel16MxN_32bit_BT_SSE2(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);


void FullDistortionKernelCbfZero16MxN_32bit_BT_SSE2(
    EB_S16  *coeff,
    EB_U32   coeffStride,
    EB_S16  *reconCoeff,
    EB_U32   reconCoeffStride,
    EB_U64   distortionResult[2],
    EB_U32   areaWidth,
    EB_U32   areaHeight);

//-----
extern void ZeroOutCoeff4x4_SSE(
    EB_S16*                  coeffbuffer,
    EB_U32                   coeffStride,
    EB_U32                   coeffOriginIndex,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);
extern void ZeroOutCoeff8x8_SSE2(
    EB_S16*                  coeffbuffer,
    EB_U32                   coeffStride,
    EB_U32                   coeffOriginIndex,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);
extern void ZeroOutCoeff16x16_SSE2(
    EB_S16*                  coeffbuffer,
    EB_U32                   coeffStride,
    EB_U32                   coeffOriginIndex,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);
extern void ZeroOutCoeff32x32_SSE2(
    EB_S16*                  coeffbuffer,
    EB_U32                   coeffStride,
    EB_U32                   coeffOriginIndex,
    EB_U32                   areaWidth,
    EB_U32                   areaHeight);

extern void ResidualKernel16bit_SSE2_INTRIN(
	EB_U16   *input,
	EB_U32   inputStride,
	EB_U16   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight);

void PictureCopyKernel4x4_SSE_INTRIN(
	EB_BYTE                  src,
	EB_U32                   srcStride,
	EB_BYTE                  dst,
	EB_U32                   dstStride,
	EB_U32                   areaWidth,
	EB_U32                   areaHeight);

void PictureCopyKernel8x8_SSE2_INTRIN(
	EB_BYTE                  src,
	EB_U32                   srcStride,
	EB_BYTE                  dst,
	EB_U32                   dstStride,
	EB_U32                   areaWidth,
	EB_U32                   areaHeight);

void PictureCopyKernel16x16_SSE2_INTRIN(
	EB_BYTE                  src,
	EB_U32                   srcStride,
	EB_BYTE                  dst,
	EB_U32                   dstStride,
	EB_U32                   areaWidth,
	EB_U32                   areaHeight);


void PictureCopyKernel32x32_SSE2_INTRIN(
	EB_BYTE                  src,
	EB_U32                   srcStride,
	EB_BYTE                  dst,
	EB_U32                   dstStride,
	EB_U32                   areaWidth,
	EB_U32                   areaHeight);

void PictureCopyKernel64x64_SSE2_INTRIN(
	EB_BYTE                  src,
	EB_U32                   srcStride,
	EB_BYTE                  dst,
	EB_U32                   dstStride,
	EB_U32                   areaWidth,
	EB_U32                   areaHeight);

void PictureAdditionKernel4x4_SSE_INTRIN(
	EB_U8  *predPtr,
	EB_U32  predStride,
	EB_S16 *residualPtr,
	EB_U32  residualStride,
	EB_U8  *reconPtr,
	EB_U32  reconStride,
	EB_U32  width,
	EB_U32  height);

void PictureAdditionKernel8x8_SSE2_INTRIN(
	EB_U8  *predPtr,
	EB_U32  predStride,
	EB_S16 *residualPtr,
	EB_U32  residualStride,
	EB_U8  *reconPtr,
	EB_U32  reconStride,
	EB_U32  width,
	EB_U32  height);

void PictureAdditionKernel16x16_SSE2_INTRIN(
	EB_U8  *predPtr,
	EB_U32  predStride,
	EB_S16 *residualPtr,
	EB_U32  residualStride,
	EB_U8  *reconPtr,
	EB_U32  reconStride,
	EB_U32  width,
	EB_U32  height);

void PictureAdditionKernel32x32_SSE2_INTRIN(
	EB_U8  *predPtr,
	EB_U32  predStride,
	EB_S16 *residualPtr,
	EB_U32  residualStride,
	EB_U8  *reconPtr,
	EB_U32  reconStride,
	EB_U32  width,
	EB_U32  height);

void PictureAdditionKernel64x64_SSE2_INTRIN(
	EB_U8  *predPtr,
	EB_U32  predStride,
	EB_S16 *residualPtr,
	EB_U32  residualStride,
	EB_U8  *reconPtr,
	EB_U32  reconStride,
	EB_U32  width,
	EB_U32  height);

void ResidualKernel4x4_SSE_INTRIN(
	EB_U8   *input,
	EB_U32   inputStride,
	EB_U8   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight);

void ResidualKernel8x8_SSE2_INTRIN(
	EB_U8   *input,
	EB_U32   inputStride,
	EB_U8   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight);

void ResidualKernel16x16_SSE2_INTRIN(
	EB_U8   *input,
	EB_U32   inputStride,
	EB_U8   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight);

void ResidualKernelSubSampled4x4_SSE_INTRIN(
	EB_U8   *input,
	EB_U32   inputStride,
	EB_U8   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight,
    EB_U8    lastLine );

void ResidualKernelSubSampled8x8_SSE2_INTRIN(
	EB_U8   *input,
	EB_U32   inputStride,
	EB_U8   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight,
    EB_U8    lastLine);

void ResidualKernelSubSampled16x16_SSE2_INTRIN(
	EB_U8   *input,
	EB_U32   inputStride,
	EB_U8   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight,
    EB_U8    lastLine);

void ResidualKernelSubSampled32x32_SSE2_INTRIN(
	EB_U8   *input,
	EB_U32   inputStride,
	EB_U8   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight,
    EB_U8    lastLine);

void ResidualKernelSubSampled64x64_SSE2_INTRIN(
	EB_U8   *input,
	EB_U32   inputStride,
	EB_U8   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight,
    EB_U8    lastLine);

void ResidualKernel32x32_SSE2_INTRIN(
	EB_U8   *input,
	EB_U32   inputStride,
	EB_U8   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight);

void ResidualKernel64x64_SSE2_INTRIN(
	EB_U8   *input,
	EB_U32   inputStride,
	EB_U8   *pred,
	EB_U32   predStride,
	EB_S16  *residual,
	EB_U32   residualStride,
	EB_U32   areaWidth,
	EB_U32   areaHeight);

void PictureAdditionKernel16bit_SSE2_INTRIN(
	EB_U16  *predPtr,
	EB_U32  predStride,
	EB_S16 *residualPtr,
	EB_U32  residualStride,
	EB_U16  *reconPtr,
	EB_U32  reconStride,
	EB_U32  width,
	EB_U32  height);



#ifdef __cplusplus
}
#endif
#endif // EbPictureOperators_SSE2_h
