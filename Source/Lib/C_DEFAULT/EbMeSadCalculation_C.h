/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMeSadCalculation_C_h
#define EbMeSadCalculation_C_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

extern void SadCalculation_8x8_16x16(
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

extern void SadCalculation_32x32_64x64(
    EB_U32  *pSad16x16,
    EB_U32  *pBestSad32x32,
    EB_U32  *pBestSad64x64,
    EB_U32  *pBestMV32x32,
    EB_U32  *pBestMV64x64,
    EB_U32   mv);

extern void InitializeBuffer_32bits(
    EB_U32*        Pointer,
    EB_U32        Count128,
    EB_U32        Count32,
    EB_U32        Value);

void WeightSearchRegion(
    EB_U8                   *inputBuffer,
    EB_U32                   inputStride,
    EB_U8                   *dstBuffer,
    EB_U32                   dstStride,
    EB_U32                   searchAreaWidth,
    EB_U32                   searchAreaHeight,
    EB_S16                   wpWeight,
    EB_S16                   wpOffset,
    EB_U32                   wpLumaWeightDenominatorShift);

#ifdef __cplusplus
}
#endif
#endif // EbMeSadCalculation_C_h