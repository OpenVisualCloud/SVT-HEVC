/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTransforms_C_h
#define EbTransforms_C_h

#include "EbDefinitions.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SHIFT_INV_1ST          7
#define SHIFT_INV_2ND          12
    
extern void QuantizeInvQuantize(
    EB_S16           *coeff,
    const EB_U32     coeffStride,
    EB_S16           *quantCoeff,
    EB_S16           *reconCoeff,
    const EB_U32     qFunc,
    const EB_U32     q_offset,
    const EB_S32     shiftedQBits,
    const EB_S32     shiftedFFunc,
    const EB_S32     iq_offset,
    const EB_S32     shiftNum,
    const EB_U32     areaSize,
    EB_U32			 *nonzerocoeff);

void UpdateQiQCoef_R(
		EB_S16           *quantCoeff,
		EB_S16           *reconCoeff,
		const EB_U32      coeffStride,
		const EB_S32      shiftedFFunc,
		const EB_S32      iq_offset,
		const EB_S32      shiftNum,
		const EB_U32      areaSize,
		EB_U32           *nonzerocoeff,
		EB_U32            componentType,
		EB_SLICE          sliceType,
		EB_U32            temporalLayer,
		EB_U32            enableCbflag,
		EB_U8             enableContouringQCUpdateFlag);


extern void UpdateQiQCoef(
	EB_S16           *quantCoeff,
	EB_S16           *reconCoeff,
	const EB_U32      coeffStride,
	const EB_S32      shiftedFFunc,
	const EB_S32      iq_offset,
	const EB_S32      shiftNum,
	const EB_U32      areaSize,
	EB_U32           *nonzerocoeff,
	EB_U32            componentType,
	EB_SLICE          sliceType,
	EB_U32            temporalLayer,
	EB_U32            enableCbflag,
	EB_U8             enableContouringQCUpdateFlag);

void Transform32x32(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void Transform32x32Estimate(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void Transform16x16(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void Transform16x16Estimate(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void Transform8x8(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void Transform4x4(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void DstTransform4x4(
    EB_S16                  *residual,
    const EB_U32             srcStride,
    EB_S16                  *transformCoefficients,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void InvTransform32x32(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void InvTransform16x16(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void InvTransform8x8(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void InvTransform4x4(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

void InvDstTransform4x4(
    EB_S16                  *transformCoefficients,
    const EB_U32             srcStride,
    EB_S16                  *residual,
    const EB_U32             dstStride,
    EB_S16                  *transformInnerArrayPtr,
    EB_U32                   bitIncrement);

#ifdef __cplusplus
}
#endif
#endif // EbTransforms_C_h

