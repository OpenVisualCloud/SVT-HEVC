/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbComputeSAD_C.h"
#include "EbUtility.h"
#include "EbDefinitions.h"

/*******************************************
* CombinedAveragingSAD
*
*******************************************/
EB_U32 CombinedAveragingSAD(
    EB_U8  *src,
    EB_U32  srcStride,
    EB_U8  *ref1,
    EB_U32  ref1Stride,
    EB_U8  *ref2,
    EB_U32  ref2Stride,
    EB_U32  height,
    EB_U32  width)
{
    EB_U32 x, y;
    EB_U32 sad = 0;
    EB_U8 avgpel;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            avgpel = (ref1[x] + ref2[x] + 1) >> 1;
            sad += EB_ABS_DIFF(src[x], avgpel);
        }
        src += srcStride;
        ref1 += ref1Stride;
        ref2 += ref2Stride;
    }

    return sad;
}

/*******************************************
* Compute8x4SAD_Default
*   Unoptimized 8x4 SAD
*******************************************/
EB_U32 Compute8x4SAD_Kernel(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride)                      // input parameter, reference stride
{
    EB_U32 rowNumberInBlock8x4;
    EB_U32 sadBlock8x4 = 0;

    for (rowNumberInBlock8x4 = 0; rowNumberInBlock8x4 < 4; ++rowNumberInBlock8x4) {
        sadBlock8x4 += EB_ABS_DIFF(src[0x00], ref[0x00]);
        sadBlock8x4 += EB_ABS_DIFF(src[0x01], ref[0x01]);
        sadBlock8x4 += EB_ABS_DIFF(src[0x02], ref[0x02]);
        sadBlock8x4 += EB_ABS_DIFF(src[0x03], ref[0x03]);
        sadBlock8x4 += EB_ABS_DIFF(src[0x04], ref[0x04]);
        sadBlock8x4 += EB_ABS_DIFF(src[0x05], ref[0x05]);
        sadBlock8x4 += EB_ABS_DIFF(src[0x06], ref[0x06]);
        sadBlock8x4 += EB_ABS_DIFF(src[0x07], ref[0x07]);
        src += srcStride;
        ref += refStride;
    }

    return sadBlock8x4;
}

void SadLoopKernelSparse(
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
    EB_S16 searchAreaHeight)
{
    EB_S16 xSearchIndex;
    EB_S16 ySearchIndex;

    *bestSad = 0xffffff;




    for (ySearchIndex = 0; ySearchIndex < searchAreaHeight; ySearchIndex++)
    {

        for (xSearchIndex = 0; xSearchIndex < searchAreaWidth; xSearchIndex++)
        {

            EB_U8 doThisPoint = 0;
            EB_U32 group = (xSearchIndex / 8);
            if ((group & 1) == (ySearchIndex & 1))
                doThisPoint = 1;

            if (doThisPoint) {
                EB_U32 x, y;
                EB_U32 sad = 0;

                for (y = 0; y < height; y++)
                {
                    for (x = 0; x < width; x++)
                    {
                        sad += EB_ABS_DIFF(src[y*srcStride + x], ref[xSearchIndex + y*refStride + x]);
                    }

                }

                // Update results
                if (sad < *bestSad)
                {
                    *bestSad = sad;
                    *xSearchCenter = xSearchIndex;
                    *ySearchCenter = ySearchIndex;
                }
            }

        }

        ref += srcStrideRaw;
    }

    return;
}

/*******************************************
*   returns NxM Sum of Absolute Differences
Note: moved from picture operators.
keep this function here for profiling
issues.
*******************************************/
EB_U32 FastLoop_NxMSadKernel(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride,                      // input parameter, reference stride
    EB_U32  height,                         // input parameter, block height (M)
    EB_U32  width)                          // input parameter, block width (N)
{
    EB_U32 x, y;
    EB_U32 sad = 0;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            sad += EB_ABS_DIFF(src[x], ref[x]);
        }
        src += srcStride;
        ref += refStride;
    }

    return sad;
}

void SadLoopKernel(
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
	EB_S16 searchAreaHeight)
{
	EB_S16 xSearchIndex;
	EB_S16 ySearchIndex;

	*bestSad = 0xffffff;

	for (ySearchIndex = 0; ySearchIndex < searchAreaHeight; ySearchIndex++)
	{
		for (xSearchIndex = 0; xSearchIndex < searchAreaWidth; xSearchIndex++)
		{
			EB_U32 x, y;
			EB_U32 sad = 0;

			for (y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++)
				{
					sad += EB_ABS_DIFF(src[y*srcStride + x], ref[xSearchIndex + y*refStride + x]);
				}

			}

			// Update results
			if (sad < *bestSad)
			{
				*bestSad = sad;
				*xSearchCenter = xSearchIndex;
				*ySearchCenter = ySearchIndex;
			}
		}

		ref += srcStrideRaw;
	}

	return;
}

//compute a 8x4 SAD  
static EB_U32 Subsad8x8(
    EB_U8  *src,                            // input parameter, source samples Ptr
    EB_U32  srcStride,                      // input parameter, source stride
    EB_U8  *ref,                            // input parameter, reference samples Ptr
    EB_U32  refStride)                     // input parameter, reference stride
{
    EB_U32 x, y;
    EB_U32 sadBlock8x4 = 0;


    srcStride = srcStride * 2;
    refStride = refStride * 2;

    for (y = 0; y < 4; y++)
    {
        for (x = 0; x < 8; x++)
        {
            sadBlock8x4 += EB_ABS_DIFF(src[y*srcStride + x], ref[y*refStride + x]);
        }

    }

    return sadBlock8x4;
}

/*******************************************
* GetEightHorizontalSearchPointResults_8x8_16x16_PU
*******************************************/
void GetEightHorizontalSearchPointResults_8x8_16x16_PU(
    EB_U8   *src,
    EB_U32   srcStride,
    EB_U8   *ref,
    EB_U32   refStride,
    EB_U32  *pBestSad8x8,
    EB_U32  *pBestMV8x8,
    EB_U32  *pBestSad16x16,
    EB_U32  *pBestMV16x16,
    EB_U32   mv,
    EB_U16  *pSad16x16)
{
    EB_U32 xSearchIndex;
    EB_S16 xMv, yMv;
    EB_U32 sad8x8_0, sad8x8_1, sad8x8_2, sad8x8_3;
    EB_U16 sad16x16;


    /*
    -------------------------------------   -----------------------------------
    | 8x8_00 | 8x8_01 | 8x8_04 | 8x8_05 |   8x8_16 | 8x8_17 | 8x8_20 | 8x8_21 |
    -------------------------------------   -----------------------------------
    | 8x8_02 | 8x8_03 | 8x8_06 | 8x8_07 |   8x8_18 | 8x8_19 | 8x8_22 | 8x8_23 |
    -----------------------   -----------   ----------------------   ----------
    | 8x8_08 | 8x8_09 | 8x8_12 | 8x8_13 |   8x8_24 | 8x8_25 | 8x8_29 | 8x8_29 |
    ----------------------    -----------   ---------------------    ----------
    | 8x8_10 | 8x8_11 | 8x8_14 | 8x8_15 |   8x8_26 | 8x8_27 | 8x8_30 | 8x8_31 |
    -------------------------------------   -----------------------------------

    -------------------------------------   -----------------------------------
    | 8x8_32 | 8x8_33 | 8x8_36 | 8x8_37 |   8x8_48 | 8x8_49 | 8x8_52 | 8x8_53 |
    -------------------------------------   -----------------------------------
    | 8x8_34 | 8x8_35 | 8x8_38 | 8x8_39 |   8x8_50 | 8x8_51 | 8x8_54 | 8x8_55 |
    -----------------------   -----------   ----------------------   ----------
    | 8x8_40 | 8x8_41 | 8x8_44 | 8x8_45 |   8x8_56 | 8x8_57 | 8x8_60 | 8x8_61 |
    ----------------------    -----------   ---------------------    ----------
    | 8x8_42 | 8x8_43 | 8x8_46 | 8x8_48 |   8x8_58 | 8x8_59 | 8x8_62 | 8x8_63 |
    -------------------------------------   -----------------------------------
    */

    /*
    ----------------------    ----------------------
    |  16x16_0  |  16x16_1  |  16x16_4  |  16x16_5  |
    ----------------------    ----------------------
    |  16x16_2  |  16x16_3  |  16x16_6  |  16x16_7  |
    -----------------------   -----------------------
    |  16x16_8  |  16x16_9  |  16x16_12 |  16x16_13 |
    ----------------------    ----------------------
    |  16x16_10 |  16x16_11 |  16x16_14 |  16x16_15 |
    -----------------------   -----------------------
    */

    for (xSearchIndex = 0; xSearchIndex < 8; xSearchIndex++)
    {

        //8x8_0        
        sad8x8_0 = Subsad8x8(src, srcStride, ref + xSearchIndex, refStride);
        if (2 * sad8x8_0 < pBestSad8x8[0]) {
            pBestSad8x8[0] = 2 * sad8x8_0;
            xMv = _MVXT(mv) + (EB_S16)xSearchIndex * 4;
            yMv = _MVYT(mv);
            pBestMV8x8[0] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
        }

        //8x8_1        
        sad8x8_1 = Subsad8x8(src + 8, srcStride, ref + xSearchIndex + 8, refStride);
        if (2 * sad8x8_1 < pBestSad8x8[1]) {
            pBestSad8x8[1] = 2 * sad8x8_1;
            xMv = _MVXT(mv) + (EB_S16)xSearchIndex * 4;
            yMv = _MVYT(mv);
            pBestMV8x8[1] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
        }

        //8x8_2        
        sad8x8_2 = Subsad8x8(src + 8 * srcStride, srcStride, ref + xSearchIndex + 8 * refStride, refStride);
        if (2 * sad8x8_2 < pBestSad8x8[2]) {
            pBestSad8x8[2] = 2 * sad8x8_2;
            xMv = _MVXT(mv) + (EB_S16)xSearchIndex * 4;
            yMv = _MVYT(mv);
            pBestMV8x8[2] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
        }

        //8x8_3        
        sad8x8_3 = Subsad8x8(src + 8 + 8 * srcStride, srcStride, ref + 8 + 8 * refStride + xSearchIndex, refStride);
        if (2 * sad8x8_3 < pBestSad8x8[3]) {
            pBestSad8x8[3] = 2 * sad8x8_3;
            xMv = _MVXT(mv) + (EB_S16)xSearchIndex * 4;
            yMv = _MVYT(mv);
            pBestMV8x8[3] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
        }


        //16x16
        sad16x16 = (EB_U16)(sad8x8_0 + sad8x8_1 + sad8x8_2 + sad8x8_3);
        pSad16x16[xSearchIndex] = sad16x16;  //store the intermediate 16x16 SAD for 32x32 in subsampled form.
        if ((EB_U32)(2 * sad16x16) < pBestSad16x16[0]) {
            pBestSad16x16[0] = 2 * sad16x16;
            xMv = _MVXT(mv) + (EB_S16)xSearchIndex * 4;
            yMv = _MVYT(mv);
            pBestMV16x16[0] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
        }

    }
}

    
/*******************************************
Calcualte SAD for 32x32,64x64 from 16x16
and check if there is improvement, if yes keep
the best SAD+MV
*******************************************/
void GetEightHorizontalSearchPointResults_32x32_64x64(
    EB_U16  *pSad16x16,
    EB_U32  *pBestSad32x32,
    EB_U32  *pBestSad64x64,
    EB_U32  *pBestMV32x32,
    EB_U32  *pBestMV64x64,
    EB_U32   mv)
{
    EB_S16 xMv, yMv;
    EB_U32 sad32x32_0, sad32x32_1, sad32x32_2, sad32x32_3, sad64x64;
    EB_U32 xSearchIndex;

    /*--------------------
    |  32x32_0  |  32x32_1
    ----------------------
    |  32x32_2  |  32x32_3
    ----------------------*/


    /*  data ordering in pSad16x16 buffer

    Search    Search            Search
    Point 0   Point 1           Point 7
    ---------------------------------------
    16x16_0    |    x    |    x    | ...... |    x    |
    ---------------------------------------
    16x16_1    |    x    |    x    | ...... |    x    |

    16x16_n    |    x    |    x    | ...... |    x    |

    ---------------------------------------
    16x16_15   |    x    |    x    | ...... |    x    |
    ---------------------------------------
    */



    for (xSearchIndex = 0; xSearchIndex < 8; xSearchIndex++)
    {

        //32x32_0
        sad32x32_0 = pSad16x16[0 * 8 + xSearchIndex] + pSad16x16[1 * 8 + xSearchIndex] + pSad16x16[2 * 8 + xSearchIndex] + pSad16x16[3 * 8 + xSearchIndex];

        if (2 * sad32x32_0 < pBestSad32x32[0]){
            pBestSad32x32[0] = 2 * sad32x32_0;
            xMv = _MVXT(mv) + (EB_S16)xSearchIndex * 4;
            yMv = _MVYT(mv);
            pBestMV32x32[0] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
        }

        //32x32_1
        sad32x32_1 = pSad16x16[4 * 8 + xSearchIndex] + pSad16x16[5 * 8 + xSearchIndex] + pSad16x16[6 * 8 + xSearchIndex] + pSad16x16[7 * 8 + xSearchIndex];

        if (2 * sad32x32_1 < pBestSad32x32[1]){
            pBestSad32x32[1] = 2 * sad32x32_1;
            xMv = _MVXT(mv) + (EB_S16)xSearchIndex * 4;
            yMv = _MVYT(mv);
            pBestMV32x32[1] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
        }

        //32x32_2
        sad32x32_2 = pSad16x16[8 * 8 + xSearchIndex] + pSad16x16[9 * 8 + xSearchIndex] + pSad16x16[10 * 8 + xSearchIndex] + pSad16x16[11 * 8 + xSearchIndex];

        if (2 * sad32x32_2 < pBestSad32x32[2]){
            pBestSad32x32[2] = 2 * sad32x32_2;
            xMv = _MVXT(mv) + (EB_S16)xSearchIndex * 4;
            yMv = _MVYT(mv);
            pBestMV32x32[2] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
        }


        //32x32_3
        sad32x32_3 = pSad16x16[12 * 8 + xSearchIndex] + pSad16x16[13 * 8 + xSearchIndex] + pSad16x16[14 * 8 + xSearchIndex] + pSad16x16[15 * 8 + xSearchIndex];

        if (2 * sad32x32_3 < pBestSad32x32[3]){
            pBestSad32x32[3] = 2 * sad32x32_3;
            xMv = _MVXT(mv) + (EB_S16)xSearchIndex * 4;
            yMv = _MVYT(mv);
            pBestMV32x32[3] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);
        }


        //64x64
        sad64x64 = sad32x32_0 + sad32x32_1 + sad32x32_2 + sad32x32_3;
        if (2 * sad64x64 <= pBestSad64x64[0]){
            pBestSad64x64[0] = 2 * sad64x64;
            xMv = _MVXT(mv) + (EB_S16)xSearchIndex * 4;
            yMv = _MVYT(mv);
            pBestMV64x64[0] = ((EB_U16)yMv << 16) | ((EB_U16)xMv);

        }

    }

}
