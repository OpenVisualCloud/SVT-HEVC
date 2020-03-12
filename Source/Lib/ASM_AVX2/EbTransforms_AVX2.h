/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTransforms_AVX2_h
#define EbTransforms_AVX2_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

#ifdef VNNI_SUPPORT
#define EbHevcTransform32_INTRIN EbHevcTransform32_VNNI_INTRIN
#else 
#define EbHevcTransform32_INTRIN EbHevcTransform32_AVX2_INTRIN
#endif



void QuantizeInvQuantize8x8_AVX2_INTRIN(
	EB_S16          *coeff,
	const EB_U32     coeffStride,
	EB_S16          *quantCoeff,
	EB_S16          *reconCoeff,
	const EB_U32     qFunc,
	const EB_U32     q_offset,
	const EB_S32     shiftedQBits,
	const EB_S32     shiftedFFunc,
	const EB_S32     iq_offset,
	const EB_S32     shiftNum,
	const EB_U32     areaSize,
	EB_U32          *nonzerocoeff);

void QuantizeInvQuantizeNxN_AVX2_INTRIN(
	EB_S16          *coeff,
	const EB_U32     coeffStride,
	EB_S16          *quantCoeff,
	EB_S16          *reconCoeff,
	const EB_U32     qFunc,
	const EB_U32     q_offset,
	const EB_S32     shiftedQBits,
	const EB_S32     shiftedFFunc,
	const EB_S32     iq_offset,
	const EB_S32     shiftNum,
	const EB_U32     areaSize,
	EB_U32          *nonzerocoeff);

void lowPrecisionTransform16x16_AVX2_INTRIN(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_S16 *intermediate, EB_U32 addshift);
void lowPrecisionTransform32x32_AVX2_INTRIN(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_S16 *intermediate, EB_U32 addshift);

void PfreqTransform32x32_AVX2_INTRIN(
	EB_S16 *src,
	const EB_U32 src_stride,
	EB_S16 *dst,
	const EB_U32 dst_stride,
	EB_S16 *intermediate,
	EB_U32 addshift);

void PfreqN4Transform32x32_AVX2_INTRIN(
	EB_S16 *src,
	const EB_U32 src_stride,
	EB_S16 *dst,
	const EB_U32 dst_stride,
	EB_S16 *intermediate,
	EB_U32 addshift);

void MatMult4x4_AVX2_INTRIN(
	EB_S16*              coeff,
	const EB_U32         coeffStride,
    const EB_U16        *maskingMatrix,
    const EB_U32         maskingMatrixStride,  //Matrix size
    const EB_U32         computeSize,  //Computation area size
	const EB_S32         offset,     //(PMP_MAX >> 1)
	const EB_S32         shiftNum, //PMP_PRECISION
	EB_U32*              nonzerocoeff);

void MatMult4x4_OutBuff_AVX2_INTRIN(
	EB_S16*              coeff,
	const EB_U32         coeffStride,
	EB_S16*              coeffOut,
	const EB_U32         coeffOutStride,

	const EB_U16        *maskingMatrix,
	const EB_U32         maskingMatrixStride, 
	const EB_U32         computeSize,         
	const EB_S32         offset,              
	const EB_S32         shiftNum,            
	EB_U32*              nonzerocoeff);


void MatMult8x8_AVX2_INTRIN(
	EB_S16*              coeff,
	const EB_U32         coeffStride,
    const EB_U16        *maskingMatrix,
    const EB_U32         maskingMatrixStride,  //Matrix size
    const EB_U32         computeSize,  //Computation area size
	const EB_S32         offset,     //(PMP_MAX >> 1)
	const EB_S32         shiftNum, //PMP_PRECISION
	EB_U32*              nonzerocoeff);

void MatMultNxN_AVX2_INTRIN(
	EB_S16*              coeff,
	const EB_U32         coeffStride,
    const EB_U16        *maskingMatrix,
    const EB_U32         maskingMatrixStride,  //Matrix size
    const EB_U32         computeSize,  //Computation area size
	const EB_S32         offset,     //(PMP_MAX >> 1)
	const EB_S32         shiftNum, //PMP_PRECISION
	EB_U32*              nonzerocoeff);



#ifdef __cplusplus
}
#endif
#endif // EbTransforms_AVX2_h


