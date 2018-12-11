/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMeSadCalculation_asm_h
#define EbMeSadCalculation_asm_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif 


extern void InitializeBuffer_32bits_SSE2_INTRIN(
	EB_U32*		Pointer,
	EB_U32		Count128,
	EB_U32		Count32,
	EB_U32		Value);

void SadCalculation_8x8_16x16_SSE2_INTRIN(
	EB_U8   *src,
	EB_U32   srcStride,  
	EB_U8   *ref,        
	EB_U32   refStride,
	EB_U32  *pBestSad8x8,
	EB_U32  *pBestSad16x16,
	EB_U32  *pBestMV8x8,
	EB_U32  *pBestMV16x16,
	EB_U32   mv,
	EB_U32  *pSad16x16);


 void SadCalculation_32x32_64x64_SSE2_INTRIN(
	EB_U32  *pSad16x16,
	EB_U32  *pBestSad32x32,
	EB_U32  *pBestSad64x64,
	EB_U32  *pBestMV32x32,
	EB_U32  *pBestMV64x64,
	EB_U32   mv);

#ifdef __cplusplus
}
#endif
#endif // EbMeSadCalculation_asm_h

