/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbMeSadCalculation_C.h"
#include "EbComputeSAD_C.h"
#include "EbDefinitions.h"
#include "EbUtility.h"

/*******************************************
Calcualte SAD for 16x16 and its 8x8 sublcoks
and check if there is improvment, if yes keep
the best SAD+MV
*******************************************/
void SadCalculation_8x8_16x16(
    EB_U8   *src,
    EB_U32   srcStride,
    EB_U8   *ref,
    EB_U32   refStride,
    EB_U32  *pBestSad8x8,
    EB_U32  *pBestSad16x16,
    EB_U32  *pBestMV8x8,
    EB_U32  *pBestMV16x16,
    EB_U32   mv,
    EB_U32  *pSad16x16)
{
    EB_U64 sad8x8_0, sad8x8_1, sad8x8_2, sad8x8_3;
    EB_U64 sad16x16;

    EB_U32   srcStrideSub = (srcStride << 1); //TODO get these from outside
    EB_U32   refStrideSub = (refStride << 1);


    sad8x8_0 = (Compute8x4SAD_Kernel(src, srcStrideSub, ref, refStrideSub)) << 1;
    if (sad8x8_0 < pBestSad8x8[0]) {
        pBestSad8x8[0] = (EB_U32)sad8x8_0;
        pBestMV8x8[0] = mv;
    }

    sad8x8_1 = (Compute8x4SAD_Kernel(src + 8, srcStrideSub, ref + 8, refStrideSub)) << 1;
    if (sad8x8_1 < pBestSad8x8[1]){
        pBestSad8x8[1] = (EB_U32)sad8x8_1;
        pBestMV8x8[1] = mv;
    }

    sad8x8_2 = (Compute8x4SAD_Kernel(src + (srcStride << 3), srcStrideSub, ref + (refStride << 3), refStrideSub)) << 1;
    if (sad8x8_2 < pBestSad8x8[2]){
        pBestSad8x8[2] = (EB_U32)sad8x8_2;
        pBestMV8x8[2] = mv;
    }

    sad8x8_3 = (Compute8x4SAD_Kernel(src + (srcStride << 3) + 8, srcStrideSub, ref + (refStride << 3) + 8, refStrideSub)) << 1;
    if (sad8x8_3 < pBestSad8x8[3]){
        pBestSad8x8[3] = (EB_U32)sad8x8_3;
        pBestMV8x8[3] = mv;
    }

    sad16x16 = sad8x8_0 + sad8x8_1 + sad8x8_2 + sad8x8_3;
    if (sad16x16 < pBestSad16x16[0]){
        pBestSad16x16[0] = (EB_U32)sad16x16;
        pBestMV16x16[0] = mv;
    }

    *pSad16x16 = (EB_U32)sad16x16;
}

/*******************************************
Calcualte SAD for 32x32,64x64 from 16x16
and check if there is improvment, if yes keep
the best SAD+MV
*******************************************/
void SadCalculation_32x32_64x64(
    EB_U32  *pSad16x16,
    EB_U32  *pBestSad32x32,
    EB_U32  *pBestSad64x64,
    EB_U32  *pBestMV32x32,
    EB_U32  *pBestMV64x64,
    EB_U32   mv)
{

    EB_U32 sad32x32_0, sad32x32_1, sad32x32_2, sad32x32_3, sad64x64;

    sad32x32_0 = pSad16x16[0] + pSad16x16[1] + pSad16x16[2] + pSad16x16[3];
    if (sad32x32_0<pBestSad32x32[0]){
        pBestSad32x32[0] = sad32x32_0;
        pBestMV32x32[0] = mv;
    }

    sad32x32_1 = pSad16x16[4] + pSad16x16[5] + pSad16x16[6] + pSad16x16[7];
    if (sad32x32_1<pBestSad32x32[1]){
        pBestSad32x32[1] = sad32x32_1;
        pBestMV32x32[1] = mv;
    }

    sad32x32_2 = pSad16x16[8] + pSad16x16[9] + pSad16x16[10] + pSad16x16[11];
    if (sad32x32_2<pBestSad32x32[2]){
        pBestSad32x32[2] = sad32x32_2;
        pBestMV32x32[2] = mv;
    }

    sad32x32_3 = pSad16x16[12] + pSad16x16[13] + pSad16x16[14] + pSad16x16[15];
    if (sad32x32_3<pBestSad32x32[3]){
        pBestSad32x32[3] = sad32x32_3;
        pBestMV32x32[3] = mv;
    }

    sad64x64 = sad32x32_0 + sad32x32_1 + sad32x32_2 + sad32x32_3;
    if (sad64x64<pBestSad64x64[0]){
        pBestSad64x64[0] = sad64x64;
        pBestMV64x64[0] = mv;
    }
}

void InitializeBuffer_32bits(
    EB_U32*		Pointer,
    EB_U32		Count128,
    EB_U32		Count32,
    EB_U32		Value) {

    EB_U32  cuIndex;

    for (cuIndex = 0; cuIndex < ((Count128 << 2) + Count32); cuIndex++) {

        Pointer[cuIndex] = Value;
    }
}


/*******************************************
* WeightSearchRegion
*   Apply weight and offsets for reference
*   samples and sotre them in local buffer
*******************************************/
void WeightSearchRegion(
    EB_U8                   *inputBuffer,
    EB_U32                   inputStride,
    EB_U8                   *dstBuffer,
    EB_U32                   dstStride,
    EB_U32                   searchAreaWidth,
    EB_U32                   searchAreaHeight,
    EB_S16                   wpWeight,
    EB_S16                   wpOffset,
    EB_U32                   wpLumaWeightDenominatorShift)
{
    EB_U32 ySearchIndex = 0;
    EB_U32 xSearchIndex = 0;
    EB_U32 inputRegionIndex;
    EB_U32 dstRegionIndex;
    EB_S32 wpLumaWeightDenominatorOffset = (1 << (wpLumaWeightDenominatorShift)) >> 1;

    while (ySearchIndex < searchAreaHeight) {
        while (xSearchIndex < searchAreaWidth) {

            inputRegionIndex = xSearchIndex + ySearchIndex * inputStride;
            dstRegionIndex = xSearchIndex + ySearchIndex * dstStride;

            dstBuffer[dstRegionIndex] =
                (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, (((wpWeight * inputBuffer[inputRegionIndex]) + wpLumaWeightDenominatorOffset) >> wpLumaWeightDenominatorShift) + wpOffset);

            // Next x position
            xSearchIndex++;
        }
        // Intilize x position to 0
        xSearchIndex = 0;
        // Next y position
        ySearchIndex++;
    }
}
