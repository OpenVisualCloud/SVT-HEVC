/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbComputeSAD_AVX2_h
#define EbComputeSAD_AVX2_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

EB_U32 Compute24xMSad_AVX2_INTRIN(
	EB_U8  *src,                            // input parameter, source samples Ptr
	EB_U32  srcStride,                      // input parameter, source stride
	EB_U8  *ref,                            // input parameter, reference samples Ptr
	EB_U32  refStride,                      // input parameter, reference stride  
	EB_U32  height,                         // input parameter, block height (M)
	EB_U32  width);                         // input parameter, block width (N)    


EB_U32 Compute32xMSad_AVX2_INTRIN(
	EB_U8  *src,                            // input parameter, source samples Ptr
	EB_U32  srcStride,                      // input parameter, source stride
	EB_U8  *ref,                            // input parameter, reference samples Ptr
	EB_U32  refStride,                      // input parameter, reference stride  
	EB_U32  height,                         // input parameter, block height (M)
	EB_U32  width);                         // input parameter, block width (N)    

EB_U32 Compute40xMSad_AVX2_INTRIN(
	EB_U8  *src,                            // input parameter, source samples Ptr
	EB_U32  srcStride,                      // input parameter, source stride
	EB_U8  *ref,                            // input parameter, reference samples Ptr
	EB_U32  refStride,                      // input parameter, reference stride  
	EB_U32  height,                         // input parameter, block height (M)
	EB_U32  width);                         // input parameter, block width (N)    

EB_U32 Compute48xMSad_AVX2_INTRIN(
	EB_U8  *src,                            // input parameter, source samples Ptr
	EB_U32  srcStride,                      // input parameter, source stride
	EB_U8  *ref,                            // input parameter, reference samples Ptr
	EB_U32  refStride,                      // input parameter, reference stride  
	EB_U32  height,                         // input parameter, block height (M)
	EB_U32  width);                         // input parameter, block width (N)    

EB_U32 Compute56xMSad_AVX2_INTRIN(
	EB_U8  *src,                            // input parameter, source samples Ptr
	EB_U32  srcStride,                      // input parameter, source stride
	EB_U8  *ref,                            // input parameter, reference samples Ptr
	EB_U32  refStride,                      // input parameter, reference stride  
	EB_U32  height,                         // input parameter, block height (M)
	EB_U32  width);                         // input parameter, block width (N)    

EB_U32 Compute64xMSad_AVX2_INTRIN(
	EB_U8  *src,                            // input parameter, source samples Ptr
	EB_U32  srcStride,                      // input parameter, source stride
	EB_U8  *ref,                            // input parameter, reference samples Ptr
	EB_U32  refStride,                      // input parameter, reference stride  
	EB_U32  height,                         // input parameter, block height (M)
	EB_U32  width);                         // input parameter, block width (N) 


void SadLoopKernel_AVX2_INTRIN(
	EB_U8  *src,                            // input parameter, source samples Ptr
	EB_U32  srcStride,                      // input parameter, source stride
	EB_U8  *ref,                            // input parameter, reference samples Ptr
	EB_U32  refStride,                      // input parameter, reference stride
	EB_U32  height,                         // input parameter, block height (M)
	EB_U32  width,                          // input parameter, block width (N)
	EB_U64 *bestSad,
	EB_S16 *xSearchCenter,
	EB_S16 *ySearchCenter,
	EB_U32  srcStrideRaw,                   // input parameter, source stride (no line skipping)
	EB_S16 searchAreaWidth,
	EB_S16 searchAreaHeight);

#ifdef NON_AVX512_SUPPORT
void GetEightHorizontalSearchPointResults_8x8_16x16_PU_AVX2_INTRIN(
    EB_U8   *src,
    EB_U32   srcStride,
    EB_U8   *ref,
    EB_U32   refStride,
    EB_U32  *pBestSad8x8,
    EB_U32  *pBestMV8x8,
    EB_U32  *pBestSad16x16,
    EB_U32  *pBestMV16x16,
    EB_U32   mv,
    EB_U16  *pSad16x16);

void GetEightHorizontalSearchPointResults_32x32_64x64_PU_AVX2_INTRIN(
    EB_U16  *pSad16x16,
    EB_U32  *pBestSad32x32,
    EB_U32  *pBestSad64x64,
    EB_U32  *pBestMV32x32,
    EB_U32  *pBestMV64x64,
    EB_U32   mv);

#endif

#ifdef __cplusplus
}
#endif        
#endif // EbComputeSAD_AVX2_h

