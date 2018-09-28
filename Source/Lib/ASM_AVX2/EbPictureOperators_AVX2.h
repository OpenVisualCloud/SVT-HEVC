/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureOperators_AVX2
#define EbPictureOperators_AVX2

#include "EbTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void EB_ENC_msbPack2D_AVX2_INTRIN_AL(
	EB_U8     *in8BitBuffer,
	EB_U32     in8Stride,
	EB_U8     *innBitBuffer,
	EB_U16    *out16BitBuffer,
	EB_U32     innStride,
	EB_U32     outStride,
	EB_U32     width,
	EB_U32     height);


extern void CompressedPackmsb_AVX2_INTRIN(
	EB_U8     *in8BitBuffer,
	EB_U32     in8Stride,
	EB_U8     *innBitBuffer,
	EB_U16    *out16BitBuffer,
	EB_U32     innStride,
	EB_U32     outStride,
	EB_U32     width,
	EB_U32     height);


void CPack_AVX2_INTRIN(
	const EB_U8     *innBitBuffer,
	EB_U32     innStride,
	EB_U8     *inCompnBitBuffer,
	EB_U32     outStride,
	EB_U8    *localCache,
	EB_U32     width,
	EB_U32     height);


void UnpackAvg_AVX2_INTRIN(
	    EB_U16 *ref16L0,
        EB_U32  refL0Stride,
        EB_U16 *ref16L1,
        EB_U32  refL1Stride,
        EB_U8  *dstPtr,
        EB_U32  dstStride,      
        EB_U32  width,
        EB_U32  height);

EB_S32  sumResidual8bit_AVX2_INTRIN(
                     EB_S16 * inPtr,
                     EB_U32   size,
                     EB_U32   strideIn );
void memset16bitBlock_AVX2_INTRIN (
                    EB_S16 * inPtr,
                    EB_U32   strideIn,
                    EB_U32   size,
                    EB_S16   value
    );


void UnpackAvgSafeSub_AVX2_INTRIN(
	    EB_U16 *ref16L0,
        EB_U32  refL0Stride,
        EB_U16 *ref16L1,
        EB_U32  refL1Stride,
        EB_U8  *dstPtr,
        EB_U32  dstStride,
        EB_U32  width,
        EB_U32  height);

#ifdef __cplusplus
}
#endif
#endif // EbPictureOperators_AVX2
