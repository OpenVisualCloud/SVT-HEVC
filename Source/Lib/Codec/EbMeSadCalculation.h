/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMeSadCalculation_h
#define EbMeSadCalculation_h

#include "EbMeSadCalculation_C.h"
#include "EbMeSadCalculation_SSE2.h"

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

/***************************************
* Function Types
***************************************/
typedef void(*EB_SADCALCULATION8X8AND16X16_TYPE)(
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

typedef void(*EB_SADCALCULATION32X32AND64X64_TYPE)(
    EB_U32  *pSad16x16,
    EB_U32  *pBestSad32x32,
    EB_U32  *pBestSad64x64,
    EB_U32  *pBestMV32x32,
    EB_U32  *pBestMV64x64,
    EB_U32   mv);

typedef void(*EB_INIALIZEBUFFER_32BITS)(
    EB_U32*		Pointer,
    EB_U32		Count128,
    EB_U32		Count32,
    EB_U32		Value);

typedef void(*EB_RECTAMPSADCALCULATION8X8AND16X16_TYPE)(
    EB_U8   *src,
    EB_U32   srcStride,
    EB_U8   *ref,
    EB_U32   refStride,
    EB_U32  *pBestSad8x8,
    EB_U32  *pBestSad16x16,
    EB_U32  *pBestMV8x8,
    EB_U32  *pBestMV16x16,
    EB_U32   mv,
    EB_U32  *pSad16x16,
    EB_U32  *pSad8x8);

typedef void(*EB_RECTAMPSADCALCULATION32X32AND64X64_TYPE)(
    EB_U32  *pSad16x16,
    EB_U32  *pSad32x32,
    EB_U32  *pBestSad32x32,
    EB_U32  *pBestSad64x64,
    EB_U32  *pBestMV32x32,
    EB_U32  *pBestMV64x64,
    EB_U32   mv);

typedef void(*EB_RECTAMPSADCALCULATION_TYPE)(
    EB_U32  *pSad8x8,
    EB_U32  *pSad16x16,
    EB_U32  *pSad32x32,
    EB_U32  *pBestSad64x32,
    EB_U32  *pBestMV64x32,
    EB_U32  *pBestSad32x16,
    EB_U32  *pBestMV32x16,
    EB_U32  *pBestSad16x8,
    EB_U32  *pBestMV16x8,
    EB_U32  *pBestSad32x64,
    EB_U32  *pBestMV32x64,
    EB_U32  *pBestSad16x32,
    EB_U32  *pBestMV16x32,
    EB_U32  *pBestSad8x16,
    EB_U32  *pBestMV8x16,
    EB_U32  *pBestSad64x16,
    EB_U32  *pBestMV64x16,
    EB_U32  *pBestSad32x8,
    EB_U32  *pBestMV32x8,
    EB_U32  *pBestSad64x48,
    EB_U32  *pBestMV64x48,
    EB_U32  *pBestSad32x24,
    EB_U32  *pBestMV32x24,
    EB_U32  *pBestSad16x64,
    EB_U32  *pBestMV16x64,
    EB_U32  *pBestSad8x32,
    EB_U32  *pBestMV8x32,
    EB_U32  *pBestSad48x64,
    EB_U32  *pBestMV48x64,
    EB_U32  *pBestSad24x32,
    EB_U32  *pBestMV24x32,
    EB_U32   mv);


/***************************************
* Function Tables
***************************************/
static EB_SADCALCULATION8X8AND16X16_TYPE SadCalculation_8x8_16x16_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    SadCalculation_8x8_16x16,
    // AVX2
	SadCalculation_8x8_16x16_SSE2_INTRIN,
};

static EB_SADCALCULATION32X32AND64X64_TYPE SadCalculation_32x32_64x64_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    SadCalculation_32x32_64x64,
    // AVX2      
	SadCalculation_32x32_64x64_SSE2_INTRIN,
};


static EB_INIALIZEBUFFER_32BITS InitializeBuffer_32bits_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    InitializeBuffer_32bits,
    // AVX2
    InitializeBuffer_32bits_SSE2_INTRIN
};


#ifdef __cplusplus
}
#endif        
#endif // EbMeSadCalculation_h