/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdio.h>

#include "EbDefinitions.h"

#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbMotionEstimation.h"
#include "EbUtility.h"

#include "EbComputeSAD.h"
#include "EbReferenceObject.h"
#include "EbAvcStyleMcp.h"
#include "EbMeSadCalculation.h"

#include "EbIntraPrediction.h"
#include "EbLambdaRateTables.h"
#include "EbPictureOperators.h"

#define OIS_TH_COUNT	4

EB_S32 OisPointTh[3][MAX_TEMPORAL_LAYERS][OIS_TH_COUNT] = {
    {
        // Light OIS
        { -20, 50, 150, 200 },
        { -20, 50, 150, 200 },
        { -20, 50, 100, 150 },
        { -20, 50, 200, 300 },
        { -20, 50, 200, 300 },
        { -20, 50, 200, 300 }
    },
    { 
        // Default OIS
        { -150, 0, 150, 200 },
        { -150, 0, 150, 200 },
        { -125, 0, 100, 150 },
        { -50, 50, 200, 300 },
        { -50, 50, 200, 300 },
        { -50, 50, 200, 300 }
    }
    ,
    { 
        // Heavy OIS
        { -400, -300, -200, 0 },
        { -400, -300, -200, 0 },
        { -400, -300, -200, 0 },
        { -400, -300, -200, 0 },
        { -400, -300, -200, 0 },
        { -400, -300, -200, 0 }
    }
};

	


/********************************************
 * Macros
 ********************************************/
#define SIXTEENTH_DECIMATED_SEARCH_WIDTH	16
#define SIXTEENTH_DECIMATED_SEARCH_HEIGHT	8

/********************************************
 * Constants
 ********************************************/

#define TOP_LEFT_POSITION       0 
#define TOP_POSITION            1 
#define TOP_RIGHT_POSITION      2 
#define RIGHT_POSITION          3 
#define BOTTOM_RIGHT_POSITION   4 
#define BOTTOM_POSITION         5 
#define BOTTOM_LEFT_POSITION    6 
#define LEFT_POSITION           7

// The interpolation is performed using a set of three 4 tap filters
#define IFShiftAvcStyle         1
#define F0 0
#define F1 1
#define F2 2

static const EB_U8 numberOfOisModePoints[5/*IOS poitnt*/] = {
	1, 3, 5, 7, 9
};

static EB_U8 totalIntraLumaMode[5][4] = {
	/*depth0*/    /*depth1*/    /*depth2*/    /*depth3*/
	{ 0, 1, 1, 1 },  // Very fast
	{ 0, 3, 3, 3 },  // fast
	{ 0, 5, 5, 5 },  // Meduim
	{ 0, 7, 7, 7 },  // Complex
	{ 0, 9, 9, 9 }   // Very Complex (MAX_OIS_1)
};


#define  MAX_SAD_VALUE 64*64*255

EB_U32 tab32x32[16] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15 };
EB_U32 tab8x8[64] = { 0, 1, 4, 5, 16, 17, 20, 21, 2, 3, 6, 7, 18, 19, 22, 23, 8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31,
32, 33, 36, 37, 48, 49, 52, 53, 34, 35, 38, 39, 50, 51, 54, 55, 40, 41, 44, 45, 56, 57, 60, 61, 42, 43, 46, 47, 58, 59, 62, 63
};


static const EB_U32 partitionWidth[MAX_ME_PU_COUNT] = {
	64,                                                                     // 0
	32, 32, 32, 32,                                                         // 1-4
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,   // 5-20
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,   // 21-36
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,   // 37-52
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,   // 53-68
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};  // 69-84

static const EB_U32 partitionHeight[MAX_ME_PU_COUNT] = {
	64,
	32, 32, 32, 32,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

static const EB_U32 puSearchIndexMap[MAX_ME_PU_COUNT][2] = {
	{ 0, 0 },
	{ 0, 0 }, { 32, 0 }, { 0, 32 }, { 32, 32 },

	{ 0, 0 }, { 16, 0 }, { 32, 0 }, { 48, 0 },
	{ 0, 16 }, { 16, 16 }, { 32, 16 }, { 48, 16 },
	{ 0, 32 }, { 16, 32 }, { 32, 32 }, { 48, 32 },
	{ 0, 48 }, { 16, 48 }, { 32, 48 }, { 48, 48 },

	{ 0, 0 }, { 8, 0 }, { 16, 0 }, { 24, 0 }, { 32, 0 }, { 40, 0 }, { 48, 0 }, { 56, 0 },
	{ 0, 8 }, { 8, 8 }, { 16, 8 }, { 24, 8 }, { 32, 8 }, { 40, 8 }, { 48, 8 }, { 56, 8 },
	{ 0, 16 }, { 8, 16 }, { 16, 16 }, { 24, 16 }, { 32, 16 }, { 40, 16 }, { 48, 16 }, { 56, 16 },
	{ 0, 24 }, { 8, 24 }, { 16, 24 }, { 24, 24 }, { 32, 24 }, { 40, 24 }, { 48, 24 }, { 56, 24 },
	{ 0, 32 }, { 8, 32 }, { 16, 32 }, { 24, 32 }, { 32, 32 }, { 40, 32 }, { 48, 32 }, { 56, 32 },
	{ 0, 40 }, { 8, 40 }, { 16, 40 }, { 24, 40 }, { 32, 40 }, { 40, 40 }, { 48, 40 }, { 56, 40 },
	{ 0, 48 }, { 8, 48 }, { 16, 48 }, { 24, 48 }, { 32, 48 }, { 40, 48 }, { 48, 48 }, { 56, 48 },
	{ 0, 56 }, { 8, 56 }, { 16, 56 }, { 24, 56 }, { 32, 56 }, { 40, 56 }, { 48, 56 }, { 56, 56 }};


static const EB_U8 subPositionType[16] = { 0, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 2, 2, 2, 2, 2 };


// Intra Open Loop
EB_U32 iSliceModesArray[11] = { EB_INTRA_PLANAR, EB_INTRA_DC, EB_INTRA_HORIZONTAL, EB_INTRA_VERTICAL, EB_INTRA_MODE_2, EB_INTRA_MODE_18, EB_INTRA_MODE_34, EB_INTRA_MODE_6, EB_INTRA_MODE_14, EB_INTRA_MODE_22, EB_INTRA_MODE_30 };
EB_U32 stage1ModesArray[9] = { EB_INTRA_HORIZONTAL, EB_INTRA_VERTICAL, EB_INTRA_MODE_2, EB_INTRA_MODE_18, EB_INTRA_MODE_34, EB_INTRA_MODE_6, EB_INTRA_MODE_14, EB_INTRA_MODE_22, EB_INTRA_MODE_30 };

#define REFERENCE_PIC_LIST_0  0
#define REFERENCE_PIC_LIST_1  1


/*******************************************
* GetEightHorizontalSearchPointResultsAll85CUs
*******************************************/
static void GetEightHorizontalSearchPointResultsAll85PUs_C(
	MeContext_t             *contextPtr,
	EB_U32                   listIndex,
	EB_U32                   searchRegionIndex,
	EB_U32                   xSearchIndex,
	EB_U32                   ySearchIndex	
)
{
	EB_U8  *srcPtr = contextPtr->lcuSrcPtr;

	EB_U8  *refPtr = contextPtr->integerBufferPtr[listIndex][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * contextPtr->interpolatedFullStride[listIndex][0]);

	EB_U32 reflumaStride = contextPtr->interpolatedFullStride[listIndex][0];

	EB_U32 searchPositionTLIndex = searchRegionIndex;
	EB_U32 searchPositionIndex;
	EB_U32 blockIndex;

	EB_U32 srcNext16x16Offset = (MAX_LCU_SIZE << 4);
	EB_U32 refNext16x16Offset = (reflumaStride << 4);

	EB_U32 currMVy = (((EB_U16)ySearchIndex) << 18);
	EB_U16 currMVx = (((EB_U16)xSearchIndex << 2));
	EB_U32 currMV = currMVy | currMVx;

	EB_U32  *pBestSad8x8 = contextPtr->pBestSad8x8;
	EB_U32  *pBestSad16x16 = contextPtr->pBestSad16x16;
	EB_U32  *pBestSad32x32 = contextPtr->pBestSad32x32;
	EB_U32  *pBestSad64x64 = contextPtr->pBestSad64x64;

	EB_U32  *pBestMV8x8 = contextPtr->pBestMV8x8;
	EB_U32  *pBestMV16x16 = contextPtr->pBestMV16x16;
	EB_U32  *pBestMV32x32 = contextPtr->pBestMV32x32;
	EB_U32  *pBestMV64x64 = contextPtr->pBestMV64x64;

	EB_U16  *pSad16x16 = contextPtr->pEightPosSad16x16;

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

	const EB_U32  srcStride = contextPtr->lcuSrcStride;
	srcNext16x16Offset = srcStride << 4;

	//---- 16x16_0
	blockIndex = 0;
	searchPositionIndex = searchPositionTLIndex;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[0], &pBestMV8x8[0], &pBestSad16x16[0], &pBestMV16x16[0], currMV, &pSad16x16[0 * 8]);
	//---- 16x16_1
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionTLIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[4], &pBestMV8x8[4], &pBestSad16x16[1], &pBestMV16x16[1], currMV, &pSad16x16[1 * 8]);
	//---- 16x16_4
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[16], &pBestMV8x8[16], &pBestSad16x16[4], &pBestMV16x16[4], currMV, &pSad16x16[4 * 8]);
	//---- 16x16_5
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[20], &pBestMV8x8[20], &pBestSad16x16[5], &pBestMV16x16[5], currMV, &pSad16x16[5 * 8]);



	//---- 16x16_2
	blockIndex = srcNext16x16Offset;
	searchPositionIndex = searchPositionTLIndex + refNext16x16Offset;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[8], &pBestMV8x8[8], &pBestSad16x16[2], &pBestMV16x16[2], currMV, &pSad16x16[2 * 8]);
	//---- 16x16_3
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[12], &pBestMV8x8[12], &pBestSad16x16[3], &pBestMV16x16[3], currMV, &pSad16x16[3 * 8]);
	//---- 16x16_6
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[24], &pBestMV8x8[24], &pBestSad16x16[6], &pBestMV16x16[6], currMV, &pSad16x16[6 * 8]);
	//---- 16x16_7
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[28], &pBestMV8x8[28], &pBestSad16x16[7], &pBestMV16x16[7], currMV, &pSad16x16[7 * 8]);


	//---- 16x16_8
	blockIndex = (srcNext16x16Offset << 1);
	searchPositionIndex = searchPositionTLIndex + (refNext16x16Offset << 1);
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[32], &pBestMV8x8[32], &pBestSad16x16[8], &pBestMV16x16[8], currMV, &pSad16x16[8 * 8]);
	//---- 16x16_9
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[36], &pBestMV8x8[36], &pBestSad16x16[9], &pBestMV16x16[9], currMV, &pSad16x16[9 * 8]);
	//---- 16x16_12
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[48], &pBestMV8x8[48], &pBestSad16x16[12], &pBestMV16x16[12], currMV, &pSad16x16[12 * 8]);
	//---- 16x1_13
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[52], &pBestMV8x8[52], &pBestSad16x16[13], &pBestMV16x16[13], currMV, &pSad16x16[13 * 8]);



	//---- 16x16_10
	blockIndex = (srcNext16x16Offset * 3);
	searchPositionIndex = searchPositionTLIndex + (refNext16x16Offset * 3);
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[40], &pBestMV8x8[40], &pBestSad16x16[10], &pBestMV16x16[10], currMV, &pSad16x16[10 * 8]);
	//---- 16x16_11
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[44], &pBestMV8x8[44], &pBestSad16x16[11], &pBestMV16x16[11], currMV, &pSad16x16[11 * 8]);
	//---- 16x16_14
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[56], &pBestMV8x8[56], &pBestSad16x16[14], &pBestMV16x16[14], currMV, &pSad16x16[14 * 8]);
	//---- 16x16_15
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	GetEightHorizontalSearchPointResults_8x8_16x16_PU(srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[60], &pBestMV8x8[60], &pBestSad16x16[15], &pBestMV16x16[15], currMV, &pSad16x16[15 * 8]);




	//32x32 and 64x64
	GetEightHorizontalSearchPointResults_32x32_64x64(pSad16x16, pBestSad32x32, pBestSad64x64, pBestMV32x32, pBestMV64x64, currMV);

}

#ifdef NON_AVX512_SUPPORT
/*******************************************
* GetEightHorizontalSearchPointResultsAll85CUs
*******************************************/
static void GetEightHorizontalSearchPointResultsAll85PUs_AVX2_INTRIN(
    MeContext_t             *contextPtr,
    EB_U32                   listIndex,
    EB_U32                   searchRegionIndex,
    EB_U32                   xSearchIndex,
    EB_U32                   ySearchIndex
)
{
    EB_U8  *srcPtr = contextPtr->lcuSrcPtr;

    EB_U8  *refPtr = contextPtr->integerBufferPtr[listIndex][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * contextPtr->interpolatedFullStride[listIndex][0]);

    EB_U32 reflumaStride = contextPtr->interpolatedFullStride[listIndex][0];

    EB_U32 searchPositionTLIndex = searchRegionIndex;
    EB_U32 searchPositionIndex;
    EB_U32 blockIndex;

    EB_U32 srcNext16x16Offset = (MAX_LCU_SIZE << 4);
    EB_U32 refNext16x16Offset = (reflumaStride << 4);

    EB_U32 currMVy = (((EB_U16)ySearchIndex) << 18);
    EB_U16 currMVx = (((EB_U16)xSearchIndex << 2));
    EB_U32 currMV = currMVy | currMVx;

    EB_U32  *pBestSad8x8 = contextPtr->pBestSad8x8;
    EB_U32  *pBestSad16x16 = contextPtr->pBestSad16x16;
    EB_U32  *pBestSad32x32 = contextPtr->pBestSad32x32;
    EB_U32  *pBestSad64x64 = contextPtr->pBestSad64x64;

    EB_U32  *pBestMV8x8 = contextPtr->pBestMV8x8;
    EB_U32  *pBestMV16x16 = contextPtr->pBestMV16x16;
    EB_U32  *pBestMV32x32 = contextPtr->pBestMV32x32;
    EB_U32  *pBestMV64x64 = contextPtr->pBestMV64x64;

    EB_U16  *pSad16x16 = contextPtr->pEightPosSad16x16;

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

    const EB_U32  srcStride = contextPtr->lcuSrcStride;
    srcNext16x16Offset = srcStride << 4;

    //---- 16x16_0
    blockIndex = 0;
    searchPositionIndex = searchPositionTLIndex;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[0], &pBestMV8x8[0], &pBestSad16x16[0], &pBestMV16x16[0], currMV, &pSad16x16[0 * 8]);
    //---- 16x16_1
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionTLIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[4], &pBestMV8x8[4], &pBestSad16x16[1], &pBestMV16x16[1], currMV, &pSad16x16[1 * 8]);
    //---- 16x16_4
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[16], &pBestMV8x8[16], &pBestSad16x16[4], &pBestMV16x16[4], currMV, &pSad16x16[4 * 8]);
    //---- 16x16_5
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[20], &pBestMV8x8[20], &pBestSad16x16[5], &pBestMV16x16[5], currMV, &pSad16x16[5 * 8]);



    //---- 16x16_2
    blockIndex = srcNext16x16Offset;
    searchPositionIndex = searchPositionTLIndex + refNext16x16Offset;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[8], &pBestMV8x8[8], &pBestSad16x16[2], &pBestMV16x16[2], currMV, &pSad16x16[2 * 8]);
    //---- 16x16_3
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[12], &pBestMV8x8[12], &pBestSad16x16[3], &pBestMV16x16[3], currMV, &pSad16x16[3 * 8]);
    //---- 16x16_6
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[24], &pBestMV8x8[24], &pBestSad16x16[6], &pBestMV16x16[6], currMV, &pSad16x16[6 * 8]);
    //---- 16x16_7
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[28], &pBestMV8x8[28], &pBestSad16x16[7], &pBestMV16x16[7], currMV, &pSad16x16[7 * 8]);


    //---- 16x16_8
    blockIndex = (srcNext16x16Offset << 1);
    searchPositionIndex = searchPositionTLIndex + (refNext16x16Offset << 1);
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[32], &pBestMV8x8[32], &pBestSad16x16[8], &pBestMV16x16[8], currMV, &pSad16x16[8 * 8]);
    //---- 16x16_9
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[36], &pBestMV8x8[36], &pBestSad16x16[9], &pBestMV16x16[9], currMV, &pSad16x16[9 * 8]);
    //---- 16x16_12
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[48], &pBestMV8x8[48], &pBestSad16x16[12], &pBestMV16x16[12], currMV, &pSad16x16[12 * 8]);
    //---- 16x1_13
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[52], &pBestMV8x8[52], &pBestSad16x16[13], &pBestMV16x16[13], currMV, &pSad16x16[13 * 8]);



    //---- 16x16_10
    blockIndex = (srcNext16x16Offset * 3);
    searchPositionIndex = searchPositionTLIndex + (refNext16x16Offset * 3);
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[40], &pBestMV8x8[40], &pBestSad16x16[10], &pBestMV16x16[10], currMV, &pSad16x16[10 * 8]);
    //---- 16x16_11
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[44], &pBestMV8x8[44], &pBestSad16x16[11], &pBestMV16x16[11], currMV, &pSad16x16[11 * 8]);
    //---- 16x16_14
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[56], &pBestMV8x8[56], &pBestSad16x16[14], &pBestMV16x16[14], currMV, &pSad16x16[14 * 8]);
    //---- 16x16_15
    blockIndex = blockIndex + 16;
    searchPositionIndex = searchPositionIndex + 16;
    GetEightHorizontalSearchPointResults_8x8_16x16_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](srcPtr + blockIndex, contextPtr->lcuSrcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[60], &pBestMV8x8[60], &pBestSad16x16[15], &pBestMV16x16[15], currMV, &pSad16x16[15 * 8]);




    //32x32 and 64x64
    GetEightHorizontalSearchPointResults_32x32_64x64_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](pSad16x16, pBestSad32x32, pBestSad64x64, pBestMV32x32, pBestMV64x64, currMV);

}
#endif

typedef void(*EB_FPSEARCH_FUNC)(
	MeContext_t             *contextPtr,
	EB_U32                   listIndex,
	EB_U32                   searchRegionIndex,
	EB_U32                   xSearchIndex,
	EB_U32                   ySearchIndex);

/*****************************
* Function Tables
*****************************/
static EB_FPSEARCH_FUNC FUNC_TABLE GetEightHorizontalSearchPointResultsAll85PUs_funcPtrArray[EB_ASM_TYPE_TOTAL] =
{
	GetEightHorizontalSearchPointResultsAll85PUs_C,
#ifndef NON_AVX512_SUPPORT
	GetEightHorizontalSearchPointResultsAll85PUs_AVX512_INTRIN
#else
    GetEightHorizontalSearchPointResultsAll85PUs_AVX2_INTRIN
#endif
};


/*******************************************
 * GetSearchPointResults
 *******************************************/
static void GetSearchPointResults(
	MeContext_t             *contextPtr,                    // input parameter, ME context Ptr, used to get LCU Ptr   
	EB_U32                   listIndex,                     // input parameter, reference list index
	EB_U32                   searchRegionIndex,             // input parameter, search area origin, used to point to reference samples
	EB_U32                   xSearchIndex,                  // input parameter, search region position in the horizontal direction, used to derive xMV
	EB_U32                   ySearchIndex)                  // input parameter, search region position in the vertical direction, used to derive yMV  
{
	EB_U8  *srcPtr = contextPtr->lcuSrcPtr;

	// EB_U8  *refPtr = refPicPtr->bufferY; // NADER
	EB_U8  *refPtr = contextPtr->integerBufferPtr[listIndex][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * contextPtr->interpolatedFullStride[listIndex][0]);

	// EB_U32 reflumaStride = refPicPtr->strideY; // NADER
	EB_U32 reflumaStride = contextPtr->interpolatedFullStride[listIndex][0];

	EB_U32 searchPositionTLIndex = searchRegionIndex;
	EB_U32 searchPositionIndex;
	EB_U32 blockIndex;

	EB_U32 srcNext16x16Offset = (MAX_LCU_SIZE << 4);
	//EB_U32 refNext16x16Offset = (refPicPtr->strideY << 4); // NADER
	EB_U32 refNext16x16Offset = (reflumaStride << 4);

	EB_U32 currMV1 = (((EB_U16)ySearchIndex) << 18);
	EB_U16 currMV2 = (((EB_U16)xSearchIndex << 2));
	EB_U32 currMV = currMV1 | currMV2;


	EB_U32  *pBestSad8x8 = contextPtr->pBestSad8x8;
	EB_U32  *pBestSad16x16 = contextPtr->pBestSad16x16;
	EB_U32  *pBestSad32x32 = contextPtr->pBestSad32x32;
	EB_U32  *pBestSad64x64 = contextPtr->pBestSad64x64;

	EB_U32  *pBestMV8x8 = contextPtr->pBestMV8x8;
	EB_U32  *pBestMV16x16 = contextPtr->pBestMV16x16;
	EB_U32  *pBestMV32x32 = contextPtr->pBestMV32x32;
	EB_U32  *pBestMV64x64 = contextPtr->pBestMV64x64;
	EB_U32  *pSad16x16 = contextPtr->pSad16x16;


	const EB_U32  srcStride = contextPtr->lcuSrcStride;
	srcNext16x16Offset = srcStride << 4;

	//---- 16x16 : 0
	blockIndex = 0;
	searchPositionIndex = searchPositionTLIndex;
	SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[0], &pBestSad16x16[0], &pBestMV8x8[0], &pBestMV16x16[0], currMV, &pSad16x16[0]);

	//---- 16x16 : 1
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionTLIndex + 16;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[4], &pBestSad16x16[1], &pBestMV8x8[4], &pBestMV16x16[1], currMV, &pSad16x16[1]);

    //---- 16x16 : 4
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[16], &pBestSad16x16[4], &pBestMV8x8[16], &pBestMV16x16[4], currMV, &pSad16x16[4]);


	//---- 16x16 : 5
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[20], &pBestSad16x16[5], &pBestMV8x8[20], &pBestMV16x16[5], currMV, &pSad16x16[5]);


	//---- 16x16 : 2
	blockIndex = srcNext16x16Offset;
	searchPositionIndex = searchPositionTLIndex + refNext16x16Offset;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[8], &pBestSad16x16[2], &pBestMV8x8[8], &pBestMV16x16[2], currMV, &pSad16x16[2]);

    //---- 16x16 : 3
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[12], &pBestSad16x16[3], &pBestMV8x8[12], &pBestMV16x16[3], currMV, &pSad16x16[3]);

    //---- 16x16 : 6
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
	SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[24], &pBestSad16x16[6], &pBestMV8x8[24], &pBestMV16x16[6], currMV, &pSad16x16[6]);

    //---- 16x16 : 7
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[28], &pBestSad16x16[7], &pBestMV8x8[28], &pBestMV16x16[7], currMV, &pSad16x16[7]);


	//---- 16x16 : 8
	blockIndex = (srcNext16x16Offset << 1);
	searchPositionIndex = searchPositionTLIndex + (refNext16x16Offset << 1);
	SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[32], &pBestSad16x16[8], &pBestMV8x8[32], &pBestMV16x16[8], currMV, &pSad16x16[8]);

    //---- 16x16 : 9
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[36], &pBestSad16x16[9], &pBestMV8x8[36], &pBestMV16x16[9], currMV, &pSad16x16[9]);

    //---- 16x16 : 12
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[48], &pBestSad16x16[12], &pBestMV8x8[48], &pBestMV16x16[12], currMV, &pSad16x16[12]);

    //---- 16x16 : 13
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[52], &pBestSad16x16[13], &pBestMV8x8[52], &pBestMV16x16[13], currMV, &pSad16x16[13]);

	//---- 16x16 : 10
	blockIndex = (srcNext16x16Offset * 3);
	searchPositionIndex = searchPositionTLIndex + (refNext16x16Offset * 3);
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[40], &pBestSad16x16[10], &pBestMV8x8[40], &pBestMV16x16[10], currMV, &pSad16x16[10]);

    //---- 16x16 : 11
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[44], &pBestSad16x16[11], &pBestMV8x8[44], &pBestMV16x16[11], currMV, &pSad16x16[11]);

    //---- 16x16 : 14
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[56], &pBestSad16x16[14], &pBestMV8x8[56], &pBestMV16x16[14], currMV, &pSad16x16[14]);

    //---- 16x16 : 15
	blockIndex = blockIndex + 16;
	searchPositionIndex = searchPositionIndex + 16;
    SadCalculation_8x8_16x16_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](srcPtr + blockIndex, srcStride, refPtr + searchPositionIndex, reflumaStride, &pBestSad8x8[60], &pBestSad16x16[15], &pBestMV8x8[60], &pBestMV16x16[15], currMV, &pSad16x16[15]);

	SadCalculation_32x32_64x64_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](pSad16x16, pBestSad32x32, pBestSad64x64, pBestMV32x32, pBestMV64x64, currMV);

}

/*******************************************
 * FullPelSearch_LCU
 *******************************************/
static void FullPelSearch_LCU(
	MeContext_t             *contextPtr,
	EB_U32                   listIndex,
	EB_S16                   xSearchAreaOrigin,
	EB_S16		             ySearchAreaOrigin,
	EB_U32                   searchAreaWidth,
	EB_U32                   searchAreaHeight
)
{

	EB_U32  xSearchIndex, ySearchIndex;

	EB_U32  searchAreaWidthRest8 = searchAreaWidth & 7;
	EB_U32  searchAreaWidthMult8 = searchAreaWidth - searchAreaWidthRest8;

	for (ySearchIndex = 0; ySearchIndex < searchAreaHeight; ySearchIndex++){

		for (xSearchIndex = 0; xSearchIndex < searchAreaWidthMult8; xSearchIndex += 8){

			//this function will do:  xSearchIndex, +1, +2, ..., +7
#ifndef NON_AVX512_SUPPORT
			GetEightHorizontalSearchPointResultsAll85PUs_funcPtrArray[ !!(ASM_TYPES & AVX512_MASK) ](
#else
            GetEightHorizontalSearchPointResultsAll85PUs_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
#endif
				contextPtr,
				listIndex,
				xSearchIndex + ySearchIndex * contextPtr->interpolatedFullStride[listIndex][0],
				xSearchIndex + xSearchAreaOrigin,
				ySearchIndex + ySearchAreaOrigin
			);
		}

		for (xSearchIndex = searchAreaWidthMult8; xSearchIndex < searchAreaWidth; xSearchIndex++){

			GetSearchPointResults(
				contextPtr,
				listIndex,
				xSearchIndex + ySearchIndex * contextPtr->interpolatedFullStride[listIndex][0],
				xSearchIndex + xSearchAreaOrigin,
				ySearchIndex + ySearchAreaOrigin);



		}

	}
}

/*******************************************
 * InterpolateSearchRegion AVC
 *   interpolates the search area
 *   the whole search area is interpolated 15 times
 *   for each sub position an interpolation is done
 *   15 buffers are required for the storage of the interpolated samples.
 *   F0: {-4, 54, 16, -2}
 *   F1: {-4, 36, 36, -4}
 *   F2: {-2, 16, 54, -4}
 ********************************************/
void InterpolateSearchRegionAVC(
	MeContext_t             *contextPtr,           // input/output parameter, ME context ptr, used to get/set interpolated search area Ptr
	EB_U32                   listIndex,            // Refrence picture list index
	EB_U8                   *searchRegionBuffer,   // input parameter, search region index, used to point to reference samples
	EB_U32                   lumaStride,           // input parameter, reference Picture stride
	EB_U32                   searchAreaWidth,      // input parameter, search area width
	EB_U32                   searchAreaHeight)     // input parameter, search area height
{

	//      0    1    2    3
	// 0    A    a    b    c
	// 1    d    e    f    g
	// 2    h    i    j    k
	// 3    n    p    q    r

	// Position  Frac-pos Y  Frac-pos X  Horizontal filter  Vertical filter
	// A         0           0           -                  -
	// a         0           1           F0                 -
	// b         0           2           F1                 -
	// c         0           3           F2                 -
	// d         1           0           -                  F0
	// e         1           1           F0                 F0
	// f         1           2           F1                 F0
	// g         1           3           F2                 F0
	// h         2           0           -                  F1
	// i         2           1           F0                 F1
	// j         2           2           F1                 F1
	// k         2           3           F2                 F1
	// n         3           0           -                  F2
	// p         3           1           F0                 F2
	// q         3           2           F1                 F2
	// r         3           3           F2                 F2

	// Start a b c

	// The Search area needs to be a multiple of 8 to align with the ASM kernel
	// Also the search area must be oversized by 2 to account for edge conditions
	EB_U32 searchAreaWidthForAsm = ROUND_UP_MUL_8(searchAreaWidth + 2);

	// Half pel interpolation of the search region using f1 -> posbBuffer
	if (searchAreaWidthForAsm){

		AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][1](
			searchRegionBuffer - (ME_FILTER_TAP >> 1) * lumaStride - (ME_FILTER_TAP >> 1) + 1,
			lumaStride,
			contextPtr->posbBuffer[listIndex][0],
			contextPtr->interpolatedStride,
			searchAreaWidthForAsm,
			searchAreaHeight + ME_FILTER_TAP,
			contextPtr->avctempBuffer,
			2);
	}

	// Half pel interpolation of the search region using f1 -> poshBuffer
	if (searchAreaWidthForAsm){
		AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][2](
			searchRegionBuffer - (ME_FILTER_TAP >> 1) * lumaStride - 1 + lumaStride,
			lumaStride,
			contextPtr->poshBuffer[listIndex][0],
			contextPtr->interpolatedStride,
			searchAreaWidthForAsm,
			searchAreaHeight + 1,
			contextPtr->avctempBuffer,
			2);
	}

	if (searchAreaWidthForAsm){
		// Half pel interpolation of the search region using f1 -> posjBuffer
		AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][2](
			contextPtr->posbBuffer[listIndex][0] + contextPtr->interpolatedStride,
			contextPtr->interpolatedStride,
			contextPtr->posjBuffer[listIndex][0],
			contextPtr->interpolatedStride,
			searchAreaWidthForAsm,
			searchAreaHeight + 1,
			contextPtr->avctempBuffer,
			2);
	}



	return;
}

/*******************************************
 * PU_HalfPelRefinement
 *   performs Half Pel refinement for one PU
 *******************************************/
static void PU_HalfPelRefinement(
    SequenceControlSet_t    *sequenceControlSetPtr,             // input parameter, Sequence control set Ptr 
    MeContext_t             *contextPtr,                        // input parameter, ME context Ptr, used to get LCU Ptr
    EB_U8                   *refBuffer,
    EB_U32                   refStride,
    EB_U32                  *pBestSsd,
    EB_U32                   puLcuBufferIndex,                  // input parameter, PU origin, used to point to source samples
    EB_U8                   *posbBuffer,                        // input parameter, position "b" interpolated search area Ptr
    EB_U8                   *poshBuffer,                        // input parameter, position "h" interpolated search area Ptr
    EB_U8                   *posjBuffer,                        // input parameter, position "j" interpolated search area Ptr
    EB_U32                   puWidth,                           // input parameter, PU width
    EB_U32                   puHeight,                          // input parameter, PU height
    EB_S16                   xSearchAreaOrigin,                 // input parameter, search area origin in the horizontal direction, used to point to reference samples
    EB_S16                   ySearchAreaOrigin,                 // input parameter, search area origin in the vertical direction, used to point to reference samples
    EB_U32                  *pBestSad,
    EB_U32                  *pBestMV,
    EB_U8                   *psubPelDirection
)
{

    EncodeContext_t         *encodeContextPtr = sequenceControlSetPtr->encodeContextPtr;

    EB_S32 searchRegionIndex;
    EB_U64 bestHalfSad = 0;
    EB_U64 distortionLeftPosition = 0;
    EB_U64 distortionRightPosition = 0;
    EB_U64 distortionTopPosition = 0;
    EB_U64 distortionBottomPosition = 0;
    EB_U64 distortionTopLeftPosition = 0;
    EB_U64 distortionTopRightPosition = 0;
    EB_U64 distortionBottomLeftPosition = 0;
    EB_U64 distortionBottomRightPosition = 0;

    EB_S16 xMvHalf[8];
    EB_S16 yMvHalf[8];

    EB_S16 xMv = _MVXT(*pBestMV);
    EB_S16 yMv = _MVYT(*pBestMV);
    EB_S16 xSearchIndex = (xMv >> 2) - xSearchAreaOrigin;
    EB_S16 ySearchIndex = (yMv >> 2) - ySearchAreaOrigin;

    (void)sequenceControlSetPtr;
    (void)encodeContextPtr;

    //TODO : remove these, and update the MV by just shifts

    xMvHalf[0] = xMv - 2; // L  position
    xMvHalf[1] = xMv + 2; // R  position
    xMvHalf[2] = xMv;     // T  position
    xMvHalf[3] = xMv;     // B  position
    xMvHalf[4] = xMv - 2; // TL position
    xMvHalf[5] = xMv + 2; // TR position
    xMvHalf[6] = xMv + 2; // BR position
    xMvHalf[7] = xMv - 2; // BL position

    yMvHalf[0] = yMv;     // L  position
    yMvHalf[1] = yMv;     // R  position
    yMvHalf[2] = yMv - 2; // T  position
    yMvHalf[3] = yMv + 2; // B  position
    yMvHalf[4] = yMv - 2; // TL position
    yMvHalf[5] = yMv - 2; // TR position
    yMvHalf[6] = yMv + 2; // BR position
    yMvHalf[7] = yMv + 2; // BL position

    // Compute SSD for the best full search candidate
    if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
        *pBestSsd = (EB_U32) SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(puWidth) - 2](
            &(contextPtr->lcuSrcPtr[puLcuBufferIndex]),
            contextPtr->lcuSrcStride,
            &(refBuffer[ySearchIndex * refStride + xSearchIndex]),
            refStride,
            puHeight,
            puWidth);
    }

	// Use SATD only when QP mod, and RC are OFF
	// QP mod, and RC assume that ME distotion is always SAD.
	// This problem might be solved by computing SAD for the best position after fractional search is done, or by considring the full pel resolution SAD.
	{
		// L position
		searchRegionIndex = xSearchIndex + (EB_S16)contextPtr->interpolatedStride * ySearchIndex;

        distortionLeftPosition = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?            
            SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(puWidth) - 2](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posbBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth) :
            (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                (NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride << 1, &(posbBuffer[searchRegionIndex]), contextPtr->interpolatedStride << 1, puHeight >> 1, puWidth)) << 1 :
                NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posbBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);

        if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
            if (distortionLeftPosition < *pBestSsd) {
                *pBestSad = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posbBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);
                *pBestMV = ((EB_U16)yMvHalf[0] << 16) | ((EB_U16)xMvHalf[0]);
                *pBestSsd = (EB_U32)distortionLeftPosition;
            }
        }
        else {
            if (distortionLeftPosition < *pBestSad) {
                *pBestSad = (EB_U32)distortionLeftPosition;
                *pBestMV = ((EB_U16)yMvHalf[0] << 16) | ((EB_U16)xMvHalf[0]);
            }
        }

		// R position
		searchRegionIndex++;

        distortionRightPosition = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ? 
            SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(puWidth) - 2](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posbBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth) :
            (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                (NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride << 1, &(posbBuffer[searchRegionIndex]), contextPtr->interpolatedStride << 1, puHeight >> 1, puWidth)) << 1 :
                 NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posbBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);


        if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
            if (distortionRightPosition < *pBestSsd) {
                *pBestSad = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posbBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);
                *pBestMV = ((EB_U16)yMvHalf[1] << 16) | ((EB_U16)xMvHalf[1]);
                *pBestSsd = (EB_U32)distortionRightPosition;
            }
        }
        else {
            if (distortionRightPosition < *pBestSad) {
                *pBestSad = (EB_U32)distortionRightPosition;
                *pBestMV = ((EB_U16)yMvHalf[1] << 16) | ((EB_U16)xMvHalf[1]);
            }      
        }

		// T position
		searchRegionIndex = xSearchIndex + (EB_S16)contextPtr->interpolatedStride * ySearchIndex;

        distortionTopPosition = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ? 
            SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(puWidth) - 2](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(poshBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth) :
            (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                (NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride << 1, &(poshBuffer[searchRegionIndex]), contextPtr->interpolatedStride << 1, puHeight >> 1, puWidth)) << 1 :
                 NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(poshBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);

        if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
            if (distortionTopPosition < *pBestSsd) {
                *pBestSad = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(poshBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);
                *pBestMV = ((EB_U16)yMvHalf[2] << 16) | ((EB_U16)xMvHalf[2]);
                *pBestSsd = (EB_U32)distortionTopPosition;
            }
        }
        else {       
            if (distortionTopPosition < *pBestSad) {
                *pBestSad = (EB_U32)distortionTopPosition;
                *pBestMV = ((EB_U16)yMvHalf[2] << 16) | ((EB_U16)xMvHalf[2]);
            }
        }

		// B position
		searchRegionIndex += (EB_S16)contextPtr->interpolatedStride;

        distortionBottomPosition = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ? 
            SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(puWidth) - 2](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(poshBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth) :
            (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                (NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride << 1, &(poshBuffer[searchRegionIndex]), contextPtr->interpolatedStride << 1, puHeight >> 1, puWidth)) << 1 :
                NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(poshBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);

        if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
            if (distortionBottomPosition < *pBestSsd) {
                *pBestSad = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(poshBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);
                *pBestMV = ((EB_U16)yMvHalf[3] << 16) | ((EB_U16)xMvHalf[3]);
                *pBestSsd = (EB_U32)distortionBottomPosition;
            }
        }
        else {
            if (distortionBottomPosition < *pBestSad) {
                *pBestSad = (EB_U32)distortionBottomPosition;
                *pBestMV = ((EB_U16)yMvHalf[3] << 16) | ((EB_U16)xMvHalf[3]);
            }
        }

		//TL position
		searchRegionIndex = xSearchIndex + (EB_S16)contextPtr->interpolatedStride * ySearchIndex;

        distortionTopLeftPosition = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ? 
            SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(puWidth) - 2](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth) :
            (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                (NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride << 1, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride << 1, puHeight >> 1, puWidth)) << 1 :
                NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);

        if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
            if (distortionTopLeftPosition < *pBestSsd) {
                *pBestSad = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);
                *pBestMV = ((EB_U16)yMvHalf[4] << 16) | ((EB_U16)xMvHalf[4]);
                *pBestSsd = (EB_U32)distortionTopLeftPosition;
            }
        }
        else {
            if (distortionTopLeftPosition < *pBestSad) {
                *pBestSad = (EB_U32)distortionTopLeftPosition;
                *pBestMV = ((EB_U16)yMvHalf[4] << 16) | ((EB_U16)xMvHalf[4]);
            }
       
        }

		//TR position
		searchRegionIndex++;

        distortionTopRightPosition = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
            SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(puWidth) - 2](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth) :
            (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                (NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride << 1, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride << 1, puHeight >> 1, puWidth)) << 1 :
                NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);

        if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
            if (distortionTopRightPosition < *pBestSsd) {
                *pBestSad = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);
                *pBestMV = ((EB_U16)yMvHalf[5] << 16) | ((EB_U16)xMvHalf[5]);
                *pBestSsd = (EB_U32)distortionTopRightPosition;
            }
        }
        else {
            if (distortionTopRightPosition < *pBestSad) {
                *pBestSad = (EB_U32)distortionTopRightPosition;
                *pBestMV = ((EB_U16)yMvHalf[5] << 16) | ((EB_U16)xMvHalf[5]);
            }
        }

		//BR position
		searchRegionIndex += (EB_S16)contextPtr->interpolatedStride;

        distortionBottomRightPosition = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
            SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(puWidth) - 2](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth) :
            (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                (NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride << 1, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride << 1, puHeight >> 1, puWidth)) << 1 :
                NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);

        if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
            if (distortionBottomRightPosition < *pBestSsd) {
                *pBestSad = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth);
                *pBestMV = ((EB_U16)yMvHalf[6] << 16) | ((EB_U16)xMvHalf[6]);
                *pBestSsd = (EB_U32)distortionBottomRightPosition;
            }
        }
        else {       
            if (distortionBottomRightPosition < *pBestSad) {
                *pBestSad = (EB_U32)distortionBottomRightPosition;
                *pBestMV = ((EB_U16)yMvHalf[6] << 16) | ((EB_U16)xMvHalf[6]);
            }   
        }

		//BL position
		searchRegionIndex--;

        distortionBottomLeftPosition = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
            SpatialFullDistortionKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][Log2f(puWidth) - 2](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth) :
            (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                (NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride << 1, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride << 1, puHeight >> 1, puWidth)) << 1 :
                (NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth));

        if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
            if (distortionBottomLeftPosition < *pBestSsd) {
                *pBestSad = (EB_U32)(NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][puWidth >> 3](&(contextPtr->lcuSrcPtr[puLcuBufferIndex]), contextPtr->lcuSrcStride, &(posjBuffer[searchRegionIndex]), contextPtr->interpolatedStride, puHeight, puWidth));
                *pBestMV = ((EB_U16)yMvHalf[7] << 16) | ((EB_U16)xMvHalf[7]);
                *pBestSsd = (EB_U32)distortionBottomLeftPosition;
            }
        }
        else {
            if (distortionBottomLeftPosition < *pBestSad) {
                *pBestSad = (EB_U32)distortionBottomLeftPosition;
                *pBestMV = ((EB_U16)yMvHalf[7] << 16) | ((EB_U16)xMvHalf[7]);
            }
        }
	}

	bestHalfSad = MIN(distortionLeftPosition, MIN(distortionRightPosition, MIN(distortionTopPosition, MIN(distortionBottomPosition, MIN(distortionTopLeftPosition, MIN(distortionTopRightPosition, MIN(distortionBottomLeftPosition, distortionBottomRightPosition)))))));


	if (bestHalfSad == distortionLeftPosition) {
		*psubPelDirection = LEFT_POSITION;
	}
	else if (bestHalfSad == distortionRightPosition) {
		*psubPelDirection = RIGHT_POSITION;
	}
	else if (bestHalfSad == distortionTopPosition) {
		*psubPelDirection = TOP_POSITION;
	}
	else if (bestHalfSad == distortionBottomPosition) {
		*psubPelDirection = BOTTOM_POSITION;
	}
	else if (bestHalfSad == distortionTopLeftPosition) {
		*psubPelDirection = TOP_LEFT_POSITION;
	}
	else if (bestHalfSad == distortionTopRightPosition) {
		*psubPelDirection = TOP_RIGHT_POSITION;
	}
	else if (bestHalfSad == distortionBottomLeftPosition) {
		*psubPelDirection = BOTTOM_LEFT_POSITION;
	}
	else if (bestHalfSad == distortionBottomRightPosition) {
		*psubPelDirection = BOTTOM_RIGHT_POSITION;
	}

	return;
}



/*******************************************
 * HalfPelSearch_LCU
 *   performs Half Pel refinement for the 85 PUs
 *******************************************/
void HalfPelSearch_LCU(
	SequenceControlSet_t    *sequenceControlSetPtr,             // input parameter, Sequence control set Ptr   
	MeContext_t             *contextPtr,                        // input/output parameter, ME context Ptr, used to get/update ME results
    EB_U8                   *refBuffer,   
    EB_U32                   refStride,           
	EB_U8                   *posbBuffer,                        // input parameter, position "b" interpolated search area Ptr
	EB_U8                   *poshBuffer,                        // input parameter, position "h" interpolated search area Ptr
	EB_U8                   *posjBuffer,                        // input parameter, position "j" interpolated search area Ptr
	EB_S16                   xSearchAreaOrigin,                 // input parameter, search area origin in the horizontal direction, used to point to reference samples
	EB_S16                   ySearchAreaOrigin,                 // input parameter, search area origin in the vertical direction, used to point to reference samples
	EB_BOOL					 disable8x8CuInMeFlag,
	EB_BOOL					enableHalfPel32x32,
	EB_BOOL					enableHalfPel16x16,
	EB_BOOL					enableHalfPel8x8){

	EB_U32 idx;
	EB_U32 puIndex;
	EB_U32 puShiftXIndex;
	EB_U32 puShiftYIndex;
	EB_U32 puLcuBufferIndex;
	EB_U32 posbBufferIndex;
	EB_U32 poshBufferIndex;
	EB_U32 posjBufferIndex;

    if (contextPtr->fractionalSearch64x64){
        PU_HalfPelRefinement(
            sequenceControlSetPtr,
            contextPtr,
            &(refBuffer[0]),
            refStride,
            contextPtr->pBestSsd64x64,
            0,
            &(posbBuffer[0]),
            &(poshBuffer[0]),
            &(posjBuffer[0]),
            64,
            64,
            xSearchAreaOrigin,
            ySearchAreaOrigin,
            contextPtr->pBestSad64x64,
            contextPtr->pBestMV64x64,
            &contextPtr->psubPelDirection64x64);
    }

	if ( enableHalfPel32x32 ) 
	{
		// 32x32 [4 partitions]
		for (puIndex = 0; puIndex < 4; ++puIndex) {

			puShiftXIndex = (puIndex & 0x01) << 5;
			puShiftYIndex = (puIndex >> 1) << 5;

			puLcuBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->lcuSrcStride;
			posbBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->interpolatedStride;
			poshBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->interpolatedStride;
			posjBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->interpolatedStride;


			PU_HalfPelRefinement(
				sequenceControlSetPtr,
				contextPtr,
                &(refBuffer[puShiftYIndex * refStride + puShiftXIndex]),
                refStride,
                &contextPtr->pBestSsd32x32[puIndex],
				puLcuBufferIndex,
				&(posbBuffer[posbBufferIndex]),
				&(poshBuffer[poshBufferIndex]),
				&(posjBuffer[posjBufferIndex]),
				32,
				32,
				xSearchAreaOrigin,
				ySearchAreaOrigin,
				&contextPtr->pBestSad32x32[puIndex],
				&contextPtr->pBestMV32x32[puIndex],
				&contextPtr->psubPelDirection32x32[puIndex]);
		}
	}
	if ( enableHalfPel16x16 )
	{
		// 16x16 [16 partitions]
		for (puIndex = 0; puIndex < 16; ++puIndex) {

			idx = tab32x32[puIndex];   //TODO bitwise this

			puShiftXIndex = (puIndex & 0x03) << 4;
			puShiftYIndex = (puIndex >> 2) << 4;

			puLcuBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->lcuSrcStride;
			posbBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->interpolatedStride;
			poshBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->interpolatedStride;
			posjBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->interpolatedStride;

			PU_HalfPelRefinement(
				sequenceControlSetPtr,
				contextPtr,
                &(refBuffer[puShiftYIndex * refStride + puShiftXIndex]),
                refStride,
                &contextPtr->pBestSsd16x16[idx],
				puLcuBufferIndex,
				&(posbBuffer[posbBufferIndex]),
				&(poshBuffer[poshBufferIndex]),
				&(posjBuffer[posjBufferIndex]),
				16,
				16,
				xSearchAreaOrigin,
				ySearchAreaOrigin,
				&contextPtr->pBestSad16x16[idx],
				&contextPtr->pBestMV16x16[idx],
				&contextPtr->psubPelDirection16x16[idx]);
		}
	}
	if ( enableHalfPel8x8   )
	{
		// 8x8   [64 partitions]
		if (!disable8x8CuInMeFlag){
			for (puIndex = 0; puIndex < 64; ++puIndex) {

				idx = tab8x8[puIndex];  //TODO bitwise this

				puShiftXIndex = (puIndex & 0x07) << 3;
				puShiftYIndex = (puIndex >> 3) << 3;

				puLcuBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->lcuSrcStride;

				posbBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->interpolatedStride;
				poshBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->interpolatedStride;
				posjBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->interpolatedStride;

				PU_HalfPelRefinement(
					sequenceControlSetPtr,
					contextPtr,
                    &(refBuffer[puShiftYIndex * refStride + puShiftXIndex]),
                    refStride,
                    &contextPtr->pBestSsd8x8[idx],
					puLcuBufferIndex,
					&(posbBuffer[posbBufferIndex]),
					&(poshBuffer[poshBufferIndex]),
					&(posjBuffer[posjBufferIndex]),
					8,
					8,
					xSearchAreaOrigin,
					ySearchAreaOrigin,
					&contextPtr->pBestSad8x8[idx],
					&contextPtr->pBestMV8x8[idx],
					&contextPtr->psubPelDirection8x8[idx]);

			}
		}
	}

	return;
}

/*******************************************
* CombinedAveragingSSD
*
*******************************************/
EB_U32 CombinedAveragingSSD(
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
    EB_U32 ssd = 0;
    EB_U8 avgpel;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            avgpel = (ref1[x] + ref2[x] + 1) >> 1;
            ssd += SQR((EB_S64)(src[x]) - (avgpel));
        }
        src += srcStride;
        ref1 += ref1Stride;
        ref2 += ref2Stride;
    }

    return ssd;
}

/*******************************************
* PU_QuarterPelRefinementOnTheFly
*   performs Quarter Pel refinement for each PU
*******************************************/
static void PU_QuarterPelRefinementOnTheFly(
    MeContext_t           *contextPtr,                      // [IN] ME context Ptr, used to get LCU Ptr
    EB_U32                *pBestSsd,
    EB_U32                 puLcuBufferIndex,                // [IN] PU origin, used to point to source samples
    EB_U8                **buf1,                            // [IN] 
    EB_U32                *buf1Stride,
    EB_U8                **buf2,                            // [IN]  
    EB_U32                *buf2Stride,
    EB_U32                 puWidth,                         // [IN]  PU width
    EB_U32                 puHeight,                        // [IN]  PU height
    EB_S16                 xSearchAreaOrigin,               // [IN] search area origin in the horizontal direction, used to point to reference samples
    EB_S16                 ySearchAreaOrigin,               // [IN] search area origin in the vertical direction, used to point to reference samples
    EB_U32                *pBestSad,
    EB_U32                *pBestMV,
    EB_U8                  subPelDirection)
{

    EB_S16 xMv = _MVXT(*pBestMV);
    EB_S16 yMv = _MVYT(*pBestMV);

    EB_S16 xSearchIndex = ((xMv + 2) >> 2) - xSearchAreaOrigin;
    EB_S16 ySearchIndex = ((yMv + 2) >> 2) - ySearchAreaOrigin;

    EB_U64 dist;

    EB_BOOL validTL, validT, validTR, validR, validBR, validB, validBL, validL;

    EB_S16 xMvQuarter[8];
    EB_S16 yMvQuarter[8];
    EB_S32 searchRegionIndex1 = 0;
    EB_S32 searchRegionIndex2 = 0;

    if ((yMv & 2) + ((xMv & 2) >> 1)) {

        validTL = (EB_BOOL)(subPelDirection == RIGHT_POSITION || subPelDirection == BOTTOM_RIGHT_POSITION || subPelDirection == BOTTOM_POSITION);
        validT = (EB_BOOL)(subPelDirection == BOTTOM_RIGHT_POSITION || subPelDirection == BOTTOM_POSITION || subPelDirection == BOTTOM_LEFT_POSITION);
        validTR = (EB_BOOL)(subPelDirection == BOTTOM_POSITION || subPelDirection == BOTTOM_LEFT_POSITION || subPelDirection == LEFT_POSITION);
        validR = (EB_BOOL)(subPelDirection == BOTTOM_LEFT_POSITION || subPelDirection == LEFT_POSITION || subPelDirection == TOP_LEFT_POSITION);
        validBR = (EB_BOOL)(subPelDirection == LEFT_POSITION || subPelDirection == TOP_LEFT_POSITION || subPelDirection == TOP_POSITION);
        validB = (EB_BOOL)(subPelDirection == TOP_LEFT_POSITION || subPelDirection == TOP_POSITION || subPelDirection == TOP_RIGHT_POSITION);
        validBL = (EB_BOOL)(subPelDirection == TOP_POSITION || subPelDirection == TOP_RIGHT_POSITION || subPelDirection == RIGHT_POSITION);
        validL = (EB_BOOL)(subPelDirection == TOP_RIGHT_POSITION || subPelDirection == RIGHT_POSITION || subPelDirection == BOTTOM_RIGHT_POSITION);

    }
    else {

        validTL = (EB_BOOL)(subPelDirection == LEFT_POSITION || subPelDirection == TOP_LEFT_POSITION || subPelDirection == TOP_POSITION);
        validT = (EB_BOOL)(subPelDirection == TOP_LEFT_POSITION || subPelDirection == TOP_POSITION || subPelDirection == TOP_RIGHT_POSITION);
        validTR = (EB_BOOL)(subPelDirection == TOP_POSITION || subPelDirection == TOP_RIGHT_POSITION || subPelDirection == RIGHT_POSITION);
        validR = (EB_BOOL)(subPelDirection == TOP_RIGHT_POSITION || subPelDirection == RIGHT_POSITION || subPelDirection == BOTTOM_RIGHT_POSITION);
        validBR = (EB_BOOL)(subPelDirection == RIGHT_POSITION || subPelDirection == BOTTOM_RIGHT_POSITION || subPelDirection == BOTTOM_POSITION);
        validB = (EB_BOOL)(subPelDirection == BOTTOM_RIGHT_POSITION || subPelDirection == BOTTOM_POSITION || subPelDirection == BOTTOM_LEFT_POSITION);
        validBL = (EB_BOOL)(subPelDirection == BOTTOM_POSITION || subPelDirection == BOTTOM_LEFT_POSITION || subPelDirection == LEFT_POSITION);
        validL = (EB_BOOL)(subPelDirection == BOTTOM_LEFT_POSITION || subPelDirection == LEFT_POSITION || subPelDirection == TOP_LEFT_POSITION);
    }

    xMvQuarter[0] = xMv - 1; // L  position
    xMvQuarter[1] = xMv + 1; // R  position
    xMvQuarter[2] = xMv;     // T  position
    xMvQuarter[3] = xMv;     // B  position
    xMvQuarter[4] = xMv - 1; // TL position
    xMvQuarter[5] = xMv + 1; // TR position
    xMvQuarter[6] = xMv + 1; // BR position
    xMvQuarter[7] = xMv - 1; // BL position

    yMvQuarter[0] = yMv;     // L  position
    yMvQuarter[1] = yMv;     // R  position
    yMvQuarter[2] = yMv - 1; // T  position
    yMvQuarter[3] = yMv + 1; // B  position
    yMvQuarter[4] = yMv - 1; // TL position
    yMvQuarter[5] = yMv - 1; // TR position
    yMvQuarter[6] = yMv + 1; // BR position
    yMvQuarter[7] = yMv + 1; // BL position

                             // Use SATD only when QP mod, and RC are OFF
                             // QP mod, and RC assume that ME distotion is always SAD.
                             // This problem might be solved by computing SAD for the best position after fractional search is done, or by considring the full pel resolution SAD.

    {
        // L position
        if (validL) {

            searchRegionIndex1 = (EB_S32)xSearchIndex + (EB_S32)buf1Stride[0] * (EB_S32)ySearchIndex;
            searchRegionIndex2 = (EB_S32)xSearchIndex + (EB_S32)buf2Stride[0] * (EB_S32)ySearchIndex;

            dist = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
                CombinedAveragingSSD(&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[0] + searchRegionIndex1, buf1Stride[0], buf2[0] + searchRegionIndex2, buf2Stride[0], puHeight, puWidth) :
                (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                    (NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE << 1, buf1[0] + searchRegionIndex1, buf1Stride[0] << 1, buf2[0] + searchRegionIndex2, buf2Stride[0] << 1, puHeight >> 1, puWidth)) << 1 :
                    NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[0] + searchRegionIndex1, buf1Stride[0], buf2[0] + searchRegionIndex2, buf2Stride[0], puHeight, puWidth);

            if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
                if (dist < *pBestSsd) {
                    *pBestSad = (EB_U32)NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[0] + searchRegionIndex1, buf1Stride[0], buf2[0] + searchRegionIndex2, buf2Stride[0], puHeight, puWidth);
                    *pBestMV = ((EB_U16)yMvQuarter[0] << 16) | ((EB_U16)xMvQuarter[0]);
                    *pBestSsd = (EB_U32)dist;
                }
            }
            else {
                if (dist < *pBestSad) {
                    *pBestSad = (EB_U32)dist;
                    *pBestMV = ((EB_U16)yMvQuarter[0] << 16) | ((EB_U16)xMvQuarter[0]);
                }
            }
        }

        // R positions
        if (validR) {

            searchRegionIndex1 = (EB_S32)xSearchIndex + (EB_S32)buf1Stride[1] * (EB_S32)ySearchIndex;
            searchRegionIndex2 = (EB_S32)xSearchIndex + (EB_S32)buf2Stride[1] * (EB_S32)ySearchIndex;

            dist = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
                CombinedAveragingSSD(&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[1] + searchRegionIndex1, buf1Stride[1], buf2[1] + searchRegionIndex2, buf2Stride[1], puHeight, puWidth) :
                (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                    (NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE << 1, buf1[1] + searchRegionIndex1, buf1Stride[1] << 1, buf2[1] + searchRegionIndex2, buf2Stride[1] << 1, puHeight >> 1, puWidth)) << 1 :
                    NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[1] + searchRegionIndex1, buf1Stride[1], buf2[1] + searchRegionIndex2, buf2Stride[1], puHeight, puWidth);

            if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
                if (dist < *pBestSsd) {
                    *pBestSad = (EB_U32)NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[1] + searchRegionIndex1, buf1Stride[1], buf2[1] + searchRegionIndex2, buf2Stride[1], puHeight, puWidth);
                    *pBestMV = ((EB_U16)yMvQuarter[1] << 16) | ((EB_U16)xMvQuarter[1]);
                    *pBestSsd = (EB_U32)dist;
                }
            }
            else {
                if (dist < *pBestSad) {
                    *pBestSad = (EB_U32)dist;
                    *pBestMV = ((EB_U16)yMvQuarter[1] << 16) | ((EB_U16)xMvQuarter[1]);
                }      
            }
        }

        // T position
        if (validT) {

            searchRegionIndex1 = (EB_S32)xSearchIndex + (EB_S32)buf1Stride[2] * (EB_S32)ySearchIndex;
            searchRegionIndex2 = (EB_S32)xSearchIndex + (EB_S32)buf2Stride[2] * (EB_S32)ySearchIndex;

            dist = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
                CombinedAveragingSSD(&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[2] + searchRegionIndex1, buf1Stride[2], buf2[2] + searchRegionIndex2, buf2Stride[2], puHeight, puWidth) :
                (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                    (NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE << 1, buf1[2] + searchRegionIndex1, buf1Stride[2] << 1, buf2[2] + searchRegionIndex2, buf2Stride[2] << 1, puHeight >> 1, puWidth)) << 1 :
                    NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[2] + searchRegionIndex1, buf1Stride[2], buf2[2] + searchRegionIndex2, buf2Stride[2], puHeight, puWidth);

            if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
                if (dist < *pBestSsd) {
                    *pBestSad = (EB_U32)NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[2] + searchRegionIndex1, buf1Stride[2], buf2[2] + searchRegionIndex2, buf2Stride[2], puHeight, puWidth);
                    *pBestMV = ((EB_U16)yMvQuarter[2] << 16) | ((EB_U16)xMvQuarter[2]);
                    *pBestSsd = (EB_U32)dist;
                }
            }
            else {
                if (dist < *pBestSad) {
                    *pBestSad = (EB_U32)dist;
                    *pBestMV = ((EB_U16)yMvQuarter[2] << 16) | ((EB_U16)xMvQuarter[2]);
                }
            }
        }

        // B position
        if (validB) {

            searchRegionIndex1 = (EB_S32)xSearchIndex + (EB_S32)buf1Stride[3] * (EB_S32)ySearchIndex;
            searchRegionIndex2 = (EB_S32)xSearchIndex + (EB_S32)buf2Stride[3] * (EB_S32)ySearchIndex;

            dist = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
                CombinedAveragingSSD(&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[3] + searchRegionIndex1, buf1Stride[3], buf2[3] + searchRegionIndex2, buf2Stride[3], puHeight, puWidth) :
                (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                    (NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE << 1, buf1[3] + searchRegionIndex1, buf1Stride[3] << 1, buf2[3] + searchRegionIndex2, buf2Stride[3] << 1, puHeight >> 1, puWidth)) << 1 :
                    NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[3] + searchRegionIndex1, buf1Stride[3], buf2[3] + searchRegionIndex2, buf2Stride[3], puHeight, puWidth);

            if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
                if (dist < *pBestSsd) {
                    *pBestSad = (EB_U32)NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[3] + searchRegionIndex1, buf1Stride[3], buf2[3] + searchRegionIndex2, buf2Stride[3], puHeight, puWidth);
                    *pBestMV  = ((EB_U16)yMvQuarter[3] << 16) | ((EB_U16)xMvQuarter[3]);
                    *pBestSsd = (EB_U32)dist;
                }
            }
            else {
                if (dist < *pBestSad) {
                    *pBestSad = (EB_U32)dist;
                    *pBestMV = ((EB_U16)yMvQuarter[3] << 16) | ((EB_U16)xMvQuarter[3]);
                }
            }
        }

        //TL position
        if (validTL) {

            searchRegionIndex1 = (EB_S32)xSearchIndex + (EB_S32)buf1Stride[4] * (EB_S32)ySearchIndex;
            searchRegionIndex2 = (EB_S32)xSearchIndex + (EB_S32)buf2Stride[4] * (EB_S32)ySearchIndex;

            dist = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
                CombinedAveragingSSD(&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[4] + searchRegionIndex1, buf1Stride[4], buf2[4] + searchRegionIndex2, buf2Stride[4], puHeight, puWidth) :
                (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                    (NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE << 1, buf1[4] + searchRegionIndex1, buf1Stride[4] << 1, buf2[4] + searchRegionIndex2, buf2Stride[4] << 1, puHeight >> 1, puWidth)) << 1 :
                    NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[4] + searchRegionIndex1, buf1Stride[4], buf2[4] + searchRegionIndex2, buf2Stride[4], puHeight, puWidth);

            if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
                if (dist < *pBestSsd) {
                    *pBestSad = (EB_U32)NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[4] + searchRegionIndex1, buf1Stride[4], buf2[4] + searchRegionIndex2, buf2Stride[4], puHeight, puWidth);
                    *pBestMV = ((EB_U16)yMvQuarter[4] << 16) | ((EB_U16)xMvQuarter[4]);
                    *pBestSsd = (EB_U32)dist;
                }
            }
            else {
                if (dist < *pBestSad) {
                    *pBestSad = (EB_U32)dist;
                    *pBestMV = ((EB_U16)yMvQuarter[4] << 16) | ((EB_U16)xMvQuarter[4]);
                }
         
            }
        }

        //TR position
        if (validTR) {

            searchRegionIndex1 = (EB_S32)xSearchIndex + (EB_S32)buf1Stride[5] * (EB_S32)ySearchIndex;
            searchRegionIndex2 = (EB_S32)xSearchIndex + (EB_S32)buf2Stride[5] * (EB_S32)ySearchIndex;

            dist = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
                CombinedAveragingSSD(&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[5] + searchRegionIndex1, buf1Stride[5], buf2[5] + searchRegionIndex2, buf2Stride[5], puHeight, puWidth) :\
                (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                    (NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE << 1, buf1[5] + searchRegionIndex1, buf1Stride[5] << 1, buf2[5] + searchRegionIndex2, buf2Stride[5] << 1, puHeight >> 1, puWidth)) << 1 :
                    NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[5] + searchRegionIndex1, buf1Stride[5], buf2[5] + searchRegionIndex2, buf2Stride[5], puHeight, puWidth);

            if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
                if (dist < *pBestSsd) {
                    *pBestSad = (EB_U32)NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[5] + searchRegionIndex1, buf1Stride[5], buf2[5] + searchRegionIndex2, buf2Stride[5], puHeight, puWidth);
                    *pBestMV = ((EB_U16)yMvQuarter[5] << 16) | ((EB_U16)xMvQuarter[5]);
                    *pBestSsd = (EB_U32)dist;
                }
            }
            else {
                if (dist < *pBestSad) {
                    *pBestSad = (EB_U32)dist;
                    *pBestMV = ((EB_U16)yMvQuarter[5] << 16) | ((EB_U16)xMvQuarter[5]);
                }
            }
        }

        //BR position
        if (validBR) {

            searchRegionIndex1 = (EB_S32)xSearchIndex + (EB_S32)buf1Stride[6] * (EB_S32)ySearchIndex;
            searchRegionIndex2 = (EB_S32)xSearchIndex + (EB_S32)buf2Stride[6] * (EB_S32)ySearchIndex;

            dist = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
                CombinedAveragingSSD(&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[6] + searchRegionIndex1, buf1Stride[6], buf2[6] + searchRegionIndex2, buf2Stride[6], puHeight, puWidth) :
                (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                    (NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE << 1, buf1[6] + searchRegionIndex1, buf1Stride[6] << 1, buf2[6] + searchRegionIndex2, buf2Stride[6] << 1, puHeight >> 1, puWidth)) << 1 :
                    NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[6] + searchRegionIndex1, buf1Stride[6], buf2[6] + searchRegionIndex2, buf2Stride[6], puHeight, puWidth);

            if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
                if (dist < *pBestSsd) {
                    *pBestSad = (EB_U32)NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[6] + searchRegionIndex1, buf1Stride[6], buf2[6] + searchRegionIndex2, buf2Stride[6], puHeight, puWidth);
                    *pBestMV = ((EB_U16)yMvQuarter[6] << 16) | ((EB_U16)xMvQuarter[6]);
                    *pBestSsd = (EB_U32)dist;
                }
            }
            else {
                if (dist < *pBestSad) {
                    *pBestSad = (EB_U32)dist;
                    *pBestMV = ((EB_U16)yMvQuarter[6] << 16) | ((EB_U16)xMvQuarter[6]);
                }       
            }
        }

        //BL position
        if (validBL) {

            searchRegionIndex1 = (EB_S32)xSearchIndex + (EB_S32)buf1Stride[7] * (EB_S32)ySearchIndex;
            searchRegionIndex2 = (EB_S32)xSearchIndex + (EB_S32)buf2Stride[7] * (EB_S32)ySearchIndex;

            dist = (contextPtr->fractionalSearchMethod == SSD_SEARCH) ?
                CombinedAveragingSSD(&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[7] + searchRegionIndex1, buf1Stride[7], buf2[7] + searchRegionIndex2, buf2Stride[7], puHeight, puWidth) :
                (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
                    (NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE << 1, buf1[7] + searchRegionIndex1, buf1Stride[7] << 1, buf2[7] + searchRegionIndex2, buf2Stride[7] << 1, puHeight >> 1, puWidth)) << 1 :
                    NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[7] + searchRegionIndex1, buf1Stride[7], buf2[7] + searchRegionIndex2, buf2Stride[7], puHeight, puWidth);

            if (contextPtr->fractionalSearchMethod == SSD_SEARCH) {
                if (dist < *pBestSsd) {
                    *pBestSad = (EB_U32)NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](&(contextPtr->lcuBuffer[puLcuBufferIndex]), MAX_LCU_SIZE, buf1[7] + searchRegionIndex1, buf1Stride[7], buf2[7] + searchRegionIndex2, buf2Stride[7], puHeight, puWidth);
                    *pBestMV = ((EB_U16)yMvQuarter[7] << 16) | ((EB_U16)xMvQuarter[7]);
                    *pBestSsd = (EB_U32)dist;
                }
            }
            else {
                if (dist < *pBestSad) {
                    *pBestSad = (EB_U32)dist;
                    *pBestMV = ((EB_U16)yMvQuarter[7] << 16) | ((EB_U16)xMvQuarter[7]);
                }
            }
        }
    }


    return;
}

/*******************************************
* SetQuarterPelRefinementInputsOnTheFly
*   determine the 2 half pel buffers to do
averaging for Quarter Pel Refinement
*******************************************/
static void SetQuarterPelRefinementInputsOnTheFly(
    EB_U8  * pos_Full,   //[IN] points to A 
    EB_U32   FullStride, //[IN] 
    EB_U8  * pos_b,     //[IN] points to b   
    EB_U8  * pos_h,     //[IN] points to h 
    EB_U8  * pos_j,     //[IN] points to j 
    EB_U32   Stride,    //[IN]
    EB_S16   xMv,        //[IN]         
    EB_S16   yMv,        //[IN]  
    EB_U8  **buf1,       //[OUT] 
    EB_U32 * buf1Stride, //[OUT] 
    EB_U8  **buf2,       //[OUT]  
    EB_U32*  buf2Stride  //[OUT] 
)
{

    EB_U32  quarterPelRefinementMethod = (yMv & 2) + ((xMv & 2) >> 1);

    //for each one of the 8 postions, we need to determine the 2 half pel buffers to  do averaging  

    //     A    a    b    c
    //     d    e    f    g
    //     h    i    j    k
    //     n    p    q    r

    switch (quarterPelRefinementMethod) {

    case EB_QUARTER_IN_FULL:

        /*c=b+A*/ buf1[0] = pos_b;                     buf1Stride[0] = Stride;        buf2[0] = pos_Full;             buf2Stride[0] = FullStride;
        /*a=A+b*/ buf1[1] = pos_Full;                  buf1Stride[1] = FullStride;    buf2[1] = pos_b + 1;             buf2Stride[1] = Stride;
        /*n=h+A*/ buf1[2] = pos_h;                      buf1Stride[2] = Stride;        buf2[2] = pos_Full;              buf2Stride[2] = FullStride;
        /*d=A+h*/ buf1[3] = pos_Full;                   buf1Stride[3] = FullStride;    buf2[3] = pos_h + Stride;        buf2Stride[3] = Stride;
        /*r=b+h*/ buf1[4] = pos_b;                      buf1Stride[4] = Stride;        buf2[4] = pos_h;                 buf2Stride[4] = Stride;
        /*p=h+b*/ buf1[5] = pos_h;                      buf1Stride[5] = Stride;        buf2[5] = pos_b + 1;             buf2Stride[5] = Stride;
        /*e=h+b*/ buf1[6] = pos_h + Stride;             buf1Stride[6] = Stride;        buf2[6] = pos_b + 1;             buf2Stride[6] = Stride;
        /*g=b+h*/ buf1[7] = pos_b;                      buf1Stride[7] = Stride;        buf2[7] = pos_h + Stride;        buf2Stride[7] = Stride;

        break;

    case EB_QUARTER_IN_HALF_HORIZONTAL:

        /*a=A+b*/ buf1[0] = pos_Full - 1;               buf1Stride[0] = FullStride;    buf2[0] = pos_b;                buf2Stride[0] = Stride;
        /*c=b+A*/ buf1[1] = pos_b;                     buf1Stride[1] = Stride;        buf2[1] = pos_Full;             buf2Stride[1] = FullStride;
        /*q=j+b*/ buf1[2] = pos_j;                     buf1Stride[2] = Stride;        buf2[2] = pos_b;                buf2Stride[2] = Stride;
        /*f=b+j*/ buf1[3] = pos_b;                     buf1Stride[3] = Stride;        buf2[3] = pos_j + Stride;        buf2Stride[3] = Stride;
        /*p=h+b*/ buf1[4] = pos_h - 1;                  buf1Stride[4] = Stride;        buf2[4] = pos_b;                buf2Stride[4] = Stride;
        /*r=b+h*/ buf1[5] = pos_b;                     buf1Stride[5] = Stride;        buf2[5] = pos_h;                buf2Stride[5] = Stride;
        /*g=b+h*/ buf1[6] = pos_b;                     buf1Stride[6] = Stride;        buf2[6] = pos_h + Stride;        buf2Stride[6] = Stride;
        /*e=h+b*/ buf1[7] = pos_h - 1 + Stride;         buf1Stride[7] = Stride;        buf2[7] = pos_b;                buf2Stride[7] = Stride;

        break;

    case EB_QUARTER_IN_HALF_VERTICAL:

        /*k=j+h*/buf1[0] = pos_j;                      buf1Stride[0] = Stride;        buf2[0] = pos_h;                 buf2Stride[0] = Stride;
        /*i=h+j*/buf1[1] = pos_h;                      buf1Stride[1] = Stride;        buf2[1] = pos_j + 1;              buf2Stride[1] = Stride;
        /*d=A+h*/buf1[2] = pos_Full - FullStride;      buf1Stride[2] = FullStride;    buf2[2] = pos_h;                  buf2Stride[2] = Stride;
        /*n=h+A*/buf1[3] = pos_h;                       buf1Stride[3] = Stride;        buf2[3] = pos_Full;               buf2Stride[3] = FullStride;
        /*g=b+h*/buf1[4] = pos_b - Stride;              buf1Stride[4] = Stride;        buf2[4] = pos_h;                  buf2Stride[4] = Stride;
        /*e=h+b*/buf1[5] = pos_h;                      buf1Stride[5] = Stride;        buf2[5] = pos_b + 1 - Stride;     buf2Stride[5] = Stride;
        /*p=h+b*/buf1[6] = pos_h;                      buf1Stride[6] = Stride;        buf2[6] = pos_b + 1;              buf2Stride[6] = Stride;
        /*r=b+h*/buf1[7] = pos_b;                      buf1Stride[7] = Stride;        buf2[7] = pos_h;                 buf2Stride[7] = Stride;

        break;

    case EB_QUARTER_IN_HALF_DIAGONAL:

        /*i=h+j*/buf1[0] = pos_h - 1;                   buf1Stride[0] = Stride;        buf2[0] = pos_j;                  buf2Stride[0] = Stride;
        /*k=j+h*/buf1[1] = pos_j;                       buf1Stride[1] = Stride;        buf2[1] = pos_h;                  buf2Stride[1] = Stride;
        /*f=b+j*/buf1[2] = pos_b - Stride;              buf1Stride[2] = Stride;        buf2[2] = pos_j;                  buf2Stride[2] = Stride;
        /*q=j+b*/buf1[3] = pos_j;                       buf1Stride[3] = Stride;        buf2[3] = pos_b;                  buf2Stride[3] = Stride;
        /*e=h+b*/buf1[4] = pos_h - 1;                   buf1Stride[4] = Stride;        buf2[4] = pos_b - Stride;         buf2Stride[4] = Stride;
        /*g=b+h*/buf1[5] = pos_b - Stride;              buf1Stride[5] = Stride;        buf2[5] = pos_h;                  buf2Stride[5] = Stride;
        /*r=b+h*/buf1[6] = pos_b;                      buf1Stride[6] = Stride;        buf2[6] = pos_h;                  buf2Stride[6] = Stride;
        /*p=h+b*/buf1[7] = pos_h - 1;                   buf1Stride[7] = Stride;        buf2[7] = pos_b;                  buf2Stride[7] = Stride;

        break;

    default:
        break;

    }

    return;
}

/*******************************************
* QuarterPelSearch_LCU
*   performs Quarter Pel refinement for the 85 PUs
*******************************************/
static void QuarterPelSearch_LCU(
    MeContext_t					*contextPtr,                     //[IN/OUT]  ME context Ptr, used to get/update ME results
    EB_U8						*pos_Full,                       //[IN] 
    EB_U32						FullStride,                      //[IN] 
    EB_U8						*pos_b,                          //[IN]
    EB_U8						*pos_h,                          //[IN]
    EB_U8						*pos_j,                          //[IN]
    EB_S16						xSearchAreaOrigin,               //[IN] search area origin in the horizontal direction, used to point to reference samples
    EB_S16						ySearchAreaOrigin,               //[IN] search area origin in the vertical direction, used to point to reference samples
    EB_BOOL						disable8x8CuInMeFlag,
    EB_BOOL					    enableHalfPel32x32,
    EB_BOOL					    enableHalfPel16x16,
    EB_BOOL					    enableHalfPel8x8,
    EB_BOOL						enableQuarterPel)
{
    EB_U32  puIndex;

    EB_U32  puShiftXIndex;
    EB_U32  puShiftYIndex;

    EB_U32  puLcuBufferIndex;

    //for each one of the 8 positions, we need to determine the 2 buffers to  do averaging
    EB_U8  *buf1[8];
    EB_U8  *buf2[8];

    EB_U32  buf1Stride[8];
    EB_U32  buf2Stride[8];

    EB_S16  xMv, yMv;
    EB_U32  nidx;

    if (contextPtr->fractionalSearch64x64) {
        xMv = _MVXT(*contextPtr->pBestMV64x64);
        yMv = _MVYT(*contextPtr->pBestMV64x64);

        SetQuarterPelRefinementInputsOnTheFly(
            pos_Full,
            FullStride,
            pos_b,
            pos_h,
            pos_j,
            contextPtr->interpolatedStride,
            xMv,
            yMv,
            buf1, buf1Stride,
            buf2, buf2Stride);

        PU_QuarterPelRefinementOnTheFly(
            contextPtr,
            contextPtr->pBestSsd64x64,
            0,
            buf1, buf1Stride,
            buf2, buf2Stride,
            32, 32,
            xSearchAreaOrigin,
            ySearchAreaOrigin,
            contextPtr->pBestSad64x64,
            contextPtr->pBestMV64x64,
            contextPtr->psubPelDirection64x64);
    }

    if (enableQuarterPel && enableHalfPel32x32)
    {
        // 32x32 [4 partitions]
        for (puIndex = 0; puIndex < 4; ++puIndex) {

            xMv = _MVXT(contextPtr->pBestMV32x32[puIndex]);
            yMv = _MVYT(contextPtr->pBestMV32x32[puIndex]);

            SetQuarterPelRefinementInputsOnTheFly(
                pos_Full,
                FullStride,
                pos_b,
                pos_h,
                pos_j,
                contextPtr->interpolatedStride,
                xMv,
                yMv,
                buf1, buf1Stride,
                buf2, buf2Stride);


            puShiftXIndex = (puIndex & 0x01) << 5;
            puShiftYIndex = (puIndex >> 1) << 5;

            puLcuBufferIndex = puShiftXIndex + puShiftYIndex * MAX_LCU_SIZE;

            buf1[0] = buf1[0] + puShiftXIndex + puShiftYIndex * buf1Stride[0];              buf2[0] = buf2[0] + puShiftXIndex + puShiftYIndex * buf2Stride[0];
            buf1[1] = buf1[1] + puShiftXIndex + puShiftYIndex * buf1Stride[1];              buf2[1] = buf2[1] + puShiftXIndex + puShiftYIndex * buf2Stride[1];
            buf1[2] = buf1[2] + puShiftXIndex + puShiftYIndex * buf1Stride[2];              buf2[2] = buf2[2] + puShiftXIndex + puShiftYIndex * buf2Stride[2];
            buf1[3] = buf1[3] + puShiftXIndex + puShiftYIndex * buf1Stride[3];              buf2[3] = buf2[3] + puShiftXIndex + puShiftYIndex * buf2Stride[3];
            buf1[4] = buf1[4] + puShiftXIndex + puShiftYIndex * buf1Stride[4];              buf2[4] = buf2[4] + puShiftXIndex + puShiftYIndex * buf2Stride[4];
            buf1[5] = buf1[5] + puShiftXIndex + puShiftYIndex * buf1Stride[5];              buf2[5] = buf2[5] + puShiftXIndex + puShiftYIndex * buf2Stride[5];
            buf1[6] = buf1[6] + puShiftXIndex + puShiftYIndex * buf1Stride[6];              buf2[6] = buf2[6] + puShiftXIndex + puShiftYIndex * buf2Stride[6];
            buf1[7] = buf1[7] + puShiftXIndex + puShiftYIndex * buf1Stride[7];              buf2[7] = buf2[7] + puShiftXIndex + puShiftYIndex * buf2Stride[7];


            PU_QuarterPelRefinementOnTheFly(
                contextPtr,
                &contextPtr->pBestSsd32x32[puIndex],
                puLcuBufferIndex,
                buf1, buf1Stride,
                buf2, buf2Stride,
                32, 32,
                xSearchAreaOrigin,
                ySearchAreaOrigin,
                &contextPtr->pBestSad32x32[puIndex],
                &contextPtr->pBestMV32x32[puIndex],
                contextPtr->psubPelDirection32x32[puIndex]);

        }
    }

    if (enableQuarterPel && enableHalfPel16x16)
    {
        // 16x16 [16 partitions]
        for (puIndex = 0; puIndex < 16; ++puIndex) {

            nidx = tab32x32[puIndex];

            xMv = _MVXT(contextPtr->pBestMV16x16[nidx]);
            yMv = _MVYT(contextPtr->pBestMV16x16[nidx]);

            SetQuarterPelRefinementInputsOnTheFly(
                pos_Full,
                FullStride,
                pos_b,
                pos_h,
                pos_j,
                contextPtr->interpolatedStride,
                xMv,
                yMv,
                buf1, buf1Stride,
                buf2, buf2Stride);


            puShiftXIndex = (puIndex & 0x03) << 4;
            puShiftYIndex = (puIndex >> 2) << 4;

            puLcuBufferIndex = puShiftXIndex + puShiftYIndex * MAX_LCU_SIZE;

            buf1[0] = buf1[0] + puShiftXIndex + puShiftYIndex * buf1Stride[0];              buf2[0] = buf2[0] + puShiftXIndex + puShiftYIndex * buf2Stride[0];
            buf1[1] = buf1[1] + puShiftXIndex + puShiftYIndex * buf1Stride[1];              buf2[1] = buf2[1] + puShiftXIndex + puShiftYIndex * buf2Stride[1];
            buf1[2] = buf1[2] + puShiftXIndex + puShiftYIndex * buf1Stride[2];              buf2[2] = buf2[2] + puShiftXIndex + puShiftYIndex * buf2Stride[2];
            buf1[3] = buf1[3] + puShiftXIndex + puShiftYIndex * buf1Stride[3];              buf2[3] = buf2[3] + puShiftXIndex + puShiftYIndex * buf2Stride[3];
            buf1[4] = buf1[4] + puShiftXIndex + puShiftYIndex * buf1Stride[4];              buf2[4] = buf2[4] + puShiftXIndex + puShiftYIndex * buf2Stride[4];
            buf1[5] = buf1[5] + puShiftXIndex + puShiftYIndex * buf1Stride[5];              buf2[5] = buf2[5] + puShiftXIndex + puShiftYIndex * buf2Stride[5];
            buf1[6] = buf1[6] + puShiftXIndex + puShiftYIndex * buf1Stride[6];              buf2[6] = buf2[6] + puShiftXIndex + puShiftYIndex * buf2Stride[6];
            buf1[7] = buf1[7] + puShiftXIndex + puShiftYIndex * buf1Stride[7];              buf2[7] = buf2[7] + puShiftXIndex + puShiftYIndex * buf2Stride[7];

            PU_QuarterPelRefinementOnTheFly(
                contextPtr,
                &contextPtr->pBestSsd16x16[nidx],
                puLcuBufferIndex,
                buf1, buf1Stride,
                buf2, buf2Stride,
                16, 16,
                xSearchAreaOrigin,
                ySearchAreaOrigin,
                &contextPtr->pBestSad16x16[nidx],
                &contextPtr->pBestMV16x16[nidx],
                contextPtr->psubPelDirection16x16[nidx]);
        }
    }

    if (enableQuarterPel && enableHalfPel8x8)
    {
        // 8x8   [64 partitions]
        if (!disable8x8CuInMeFlag) {
            for (puIndex = 0; puIndex < 64; ++puIndex) {

                nidx = tab8x8[puIndex];

                xMv = _MVXT(contextPtr->pBestMV8x8[nidx]);
                yMv = _MVYT(contextPtr->pBestMV8x8[nidx]);

                SetQuarterPelRefinementInputsOnTheFly(
                    pos_Full,
                    FullStride,
                    pos_b,
                    pos_h,
                    pos_j,
                    contextPtr->interpolatedStride,
                    xMv,
                    yMv,
                    buf1, buf1Stride,
                    buf2, buf2Stride);


                puShiftXIndex = (puIndex & 0x07) << 3;
                puShiftYIndex = (puIndex >> 3) << 3;

                puLcuBufferIndex = puShiftXIndex + puShiftYIndex * MAX_LCU_SIZE;

                buf1[0] = buf1[0] + puShiftXIndex + puShiftYIndex * buf1Stride[0];              buf2[0] = buf2[0] + puShiftXIndex + puShiftYIndex * buf2Stride[0];
                buf1[1] = buf1[1] + puShiftXIndex + puShiftYIndex * buf1Stride[1];              buf2[1] = buf2[1] + puShiftXIndex + puShiftYIndex * buf2Stride[1];
                buf1[2] = buf1[2] + puShiftXIndex + puShiftYIndex * buf1Stride[2];              buf2[2] = buf2[2] + puShiftXIndex + puShiftYIndex * buf2Stride[2];
                buf1[3] = buf1[3] + puShiftXIndex + puShiftYIndex * buf1Stride[3];              buf2[3] = buf2[3] + puShiftXIndex + puShiftYIndex * buf2Stride[3];
                buf1[4] = buf1[4] + puShiftXIndex + puShiftYIndex * buf1Stride[4];              buf2[4] = buf2[4] + puShiftXIndex + puShiftYIndex * buf2Stride[4];
                buf1[5] = buf1[5] + puShiftXIndex + puShiftYIndex * buf1Stride[5];              buf2[5] = buf2[5] + puShiftXIndex + puShiftYIndex * buf2Stride[5];
                buf1[6] = buf1[6] + puShiftXIndex + puShiftYIndex * buf1Stride[6];              buf2[6] = buf2[6] + puShiftXIndex + puShiftYIndex * buf2Stride[6];
                buf1[7] = buf1[7] + puShiftXIndex + puShiftYIndex * buf1Stride[7];              buf2[7] = buf2[7] + puShiftXIndex + puShiftYIndex * buf2Stride[7];

                PU_QuarterPelRefinementOnTheFly(
                    contextPtr,
                    &contextPtr->pBestSsd8x8[nidx],
                    puLcuBufferIndex,
                    buf1, buf1Stride,
                    buf2, buf2Stride,
                    8, 8,
                    xSearchAreaOrigin,
                    ySearchAreaOrigin,
                    &contextPtr->pBestSad8x8[nidx],
                    &contextPtr->pBestMV8x8[nidx],
                    contextPtr->psubPelDirection8x8[nidx]);
            }
        }
    }

    return;
}


void HmeOneQuadrantLevel0(
	MeContext_t             *contextPtr,                        // input/output parameter, ME context Ptr, used to get/update ME results
	EB_S16                   originX,                           // input parameter, LCU position in the horizontal direction- sixteenth resolution
	EB_S16                   originY,                           // input parameter, LCU position in the vertical direction- sixteenth resolution
	EB_U32                   lcuWidth,                          // input parameter, LCU pwidth - sixteenth resolution                
	EB_U32                   lcuHeight,                         // input parameter, LCU height - sixteenth resolution
	EB_S16                   xHmeSearchCenter,                  // input parameter, HME search center in the horizontal direction
	EB_S16                   yHmeSearchCenter,                  // input parameter, HME search center in the vertical direction
	EbPictureBufferDesc_t   *sixteenthRefPicPtr,                // input parameter, sixteenth reference Picture Ptr	
	EB_U64                  *level0BestSad,                     // output parameter, Level0 SAD at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_S16                  *xLevel0SearchCenter,               // output parameter, Level0 xMV at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_S16                  *yLevel0SearchCenter,               // output parameter, Level0 yMV at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_U32                   searchAreaMultiplierX,
	EB_U32                   searchAreaMultiplierY)
{

	EB_S16 xTopLeftSearchRegion;
	EB_S16 yTopLeftSearchRegion;
	EB_U32 searchRegionIndex;
	EB_S16 xSearchAreaOrigin;
	EB_S16 ySearchAreaOrigin;
	EB_S16 xSearchRegionDistance;
	EB_S16 ySearchRegionDistance;

	EB_S16 padWidth;
	EB_S16 padHeight;


    EB_S16 searchAreaWidth  = (EB_S16)(((contextPtr->hmeLevel0TotalSearchAreaWidth  * searchAreaMultiplierX) / 100));
	EB_S16 searchAreaHeight = (EB_S16)(((contextPtr->hmeLevel0TotalSearchAreaHeight * searchAreaMultiplierY) / 100));

    if(contextPtr->hmeSearchType == HME_SPARSE)
          searchAreaWidth  = ((searchAreaWidth +4)>>3)<<3;  //round down/up the width to the nearest multiple of 8.

	xSearchRegionDistance = xHmeSearchCenter;
	ySearchRegionDistance = yHmeSearchCenter;
	padWidth = (EB_S16)(sixteenthRefPicPtr->originX) - 1;
	padHeight = (EB_S16)(sixteenthRefPicPtr->originY) - 1;


    xSearchAreaOrigin = -(EB_S16)(searchAreaWidth  >> 1) + xSearchRegionDistance;
	ySearchAreaOrigin = -(EB_S16)(searchAreaHeight >> 1) + ySearchRegionDistance;

	// Correct the left edge of the Search Area if it is not on the reference Picture
	xSearchAreaOrigin = ((originX + xSearchAreaOrigin) < -padWidth) ?
		-padWidth - originX :
		xSearchAreaOrigin;

	searchAreaWidth = ((originX + xSearchAreaOrigin) < -padWidth) ?
		searchAreaWidth - (-padWidth - (originX + xSearchAreaOrigin)) :
		searchAreaWidth;

	// Correct the right edge of the Search Area if its not on the reference Picture
	xSearchAreaOrigin = ((originX + xSearchAreaOrigin) > (EB_S16)sixteenthRefPicPtr->width - 1) ?
		xSearchAreaOrigin - ((originX + xSearchAreaOrigin) - ((EB_S16)sixteenthRefPicPtr->width - 1)) :
		xSearchAreaOrigin;

	searchAreaWidth = ((originX + xSearchAreaOrigin + searchAreaWidth) > (EB_S16)sixteenthRefPicPtr->width) ?
		MAX(1, searchAreaWidth - ((originX + xSearchAreaOrigin + searchAreaWidth) - (EB_S16)sixteenthRefPicPtr->width)) :
		searchAreaWidth;

	// Correct the top edge of the Search Area if it is not on the reference Picture
	ySearchAreaOrigin = ((originY + ySearchAreaOrigin) < -padHeight) ?
		-padHeight - originY :
		ySearchAreaOrigin;

	searchAreaHeight = ((originY + ySearchAreaOrigin) < -padHeight) ?
		searchAreaHeight - (-padHeight - (originY + ySearchAreaOrigin)) :
		searchAreaHeight;

	// Correct the bottom edge of the Search Area if its not on the reference Picture
	ySearchAreaOrigin = ((originY + ySearchAreaOrigin) > (EB_S16)sixteenthRefPicPtr->height - 1) ?
		ySearchAreaOrigin - ((originY + ySearchAreaOrigin) - ((EB_S16)sixteenthRefPicPtr->height - 1)) :
		ySearchAreaOrigin;

	searchAreaHeight = (originY + ySearchAreaOrigin + searchAreaHeight > (EB_S16)sixteenthRefPicPtr->height) ?
		MAX(1, searchAreaHeight - ((originY + ySearchAreaOrigin + searchAreaHeight) - (EB_S16)sixteenthRefPicPtr->height)) :
		searchAreaHeight;

	xTopLeftSearchRegion = ((EB_S16)sixteenthRefPicPtr->originX + originX) + xSearchAreaOrigin;
	yTopLeftSearchRegion = ((EB_S16)sixteenthRefPicPtr->originY + originY) + ySearchAreaOrigin;
	searchRegionIndex = xTopLeftSearchRegion + yTopLeftSearchRegion * sixteenthRefPicPtr->strideY;


	{

		if ((searchAreaWidth & 15) != 0)
		{
			searchAreaWidth = (EB_S16)((double)((searchAreaWidth >> 4) << 4));
		}
#ifndef NON_AVX512_SUPPORT
	    if (((searchAreaWidth & 15) == 0) && (!!(ASM_TYPES & AVX512_MASK)))
#else
        if (((searchAreaWidth & 15) == 0) && (!!(ASM_TYPES & AVX2_MASK)))
#endif
	    {
#ifndef NON_AVX512_SUPPORT
		    SadLoopKernel_AVX512_HmeL0_INTRIN(
			    &contextPtr->sixteenthLcuBuffer[0],
			    contextPtr->sixteenthLcuBufferStride,
			    &sixteenthRefPicPtr->bufferY[searchRegionIndex],
			    sixteenthRefPicPtr->strideY * 2,
			    lcuHeight >> 1, lcuWidth,
			    // results 
			    level0BestSad,
			    xLevel0SearchCenter,
			    yLevel0SearchCenter,
			    // range 
			    sixteenthRefPicPtr->strideY,
			    searchAreaWidth,
			    searchAreaHeight
			    );
#else
            SadLoopKernel_AVX2_HmeL0_INTRIN(
                &contextPtr->sixteenthLcuBuffer[0],
                contextPtr->sixteenthLcuBufferStride,
                &sixteenthRefPicPtr->bufferY[searchRegionIndex],
                sixteenthRefPicPtr->strideY * 2,
                lcuHeight >> 1, lcuWidth,
                // results
                level0BestSad,
                xLevel0SearchCenter,
                yLevel0SearchCenter,
                // range
                sixteenthRefPicPtr->strideY,
                searchAreaWidth,
                searchAreaHeight
            );
#endif

        }
        else {
            {
                // Only width equals 16 (LCU equals 64) is updated 
                // other width sizes work with the old code as the one in"SadLoopKernel_SSE4_1_INTRIN"

                SadLoopKernel(
                    &contextPtr->sixteenthLcuBuffer[0],
                    contextPtr->sixteenthLcuBufferStride,
                    &sixteenthRefPicPtr->bufferY[searchRegionIndex],
                    sixteenthRefPicPtr->strideY * 2,
                    lcuHeight >> 1, lcuWidth,
                    /* results */
                    level0BestSad,
                    xLevel0SearchCenter,
                    yLevel0SearchCenter,
                    /* range */
                    sixteenthRefPicPtr->strideY,
                    searchAreaWidth,
                    searchAreaHeight
                    );
            }
        }
    }

	*level0BestSad *= 2; // Multiply by 2 because considered only ever other line
	*xLevel0SearchCenter += xSearchAreaOrigin;
	*xLevel0SearchCenter *= 4; // Multiply by 4 because operating on 1/4 resolution
	*yLevel0SearchCenter += ySearchAreaOrigin;
	*yLevel0SearchCenter *= 4; // Multiply by 4 because operating on 1/4 resolution

	return;
}


void HmeLevel0(
	MeContext_t             *contextPtr,                        // input/output parameter, ME context Ptr, used to get/update ME results
	EB_S16                   originX,                           // input parameter, LCU position in the horizontal direction- sixteenth resolution
	EB_S16                   originY,                           // input parameter, LCU position in the vertical direction- sixteenth resolution
	EB_U32                   lcuWidth,                          // input parameter, LCU pwidth - sixteenth resolution                
	EB_U32                   lcuHeight,                         // input parameter, LCU height - sixteenth resolution
	EB_S16                   xHmeSearchCenter,                  // input parameter, HME search center in the horizontal direction
	EB_S16                   yHmeSearchCenter,                  // input parameter, HME search center in the vertical direction
	EbPictureBufferDesc_t   *sixteenthRefPicPtr,                // input parameter, sixteenth reference Picture Ptr
	EB_U32                   searchRegionNumberInWidth,         // input parameter, search region number in the horizontal direction
	EB_U32                   searchRegionNumberInHeight,        // input parameter, search region number in the vertical direction
	EB_U64                  *level0BestSad,                     // output parameter, Level0 SAD at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_S16                  *xLevel0SearchCenter,               // output parameter, Level0 xMV at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_S16                  *yLevel0SearchCenter,               // output parameter, Level0 yMV at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_U32                   searchAreaMultiplierX,
	EB_U32                   searchAreaMultiplierY)
{

	EB_S16 xTopLeftSearchRegion;
	EB_S16 yTopLeftSearchRegion;
	EB_U32 searchRegionIndex;
	EB_S16 xSearchAreaOrigin;
	EB_S16 ySearchAreaOrigin;
	EB_S16 xSearchRegionDistance;
	EB_S16 ySearchRegionDistance;

	EB_S16 padWidth;
	EB_S16 padHeight;

	// Adjust SR size based on the searchAreaShift
	EB_S16 searchAreaWidth = (EB_S16)(((contextPtr->hmeLevel0SearchAreaInWidthArray[searchRegionNumberInWidth] * searchAreaMultiplierX) / 100));
	EB_S16 searchAreaHeight = (EB_S16)(((contextPtr->hmeLevel0SearchAreaInHeightArray[searchRegionNumberInHeight] * searchAreaMultiplierY) / 100));


	xSearchRegionDistance = xHmeSearchCenter;
	ySearchRegionDistance = yHmeSearchCenter;
	padWidth = (EB_S16)(sixteenthRefPicPtr->originX) - 1;
	padHeight = (EB_S16)(sixteenthRefPicPtr->originY) - 1;

	while (searchRegionNumberInWidth) {
		searchRegionNumberInWidth--;
		xSearchRegionDistance += (EB_S16)(((contextPtr->hmeLevel0SearchAreaInWidthArray[searchRegionNumberInWidth] * searchAreaMultiplierX) / 100));
	}

	while (searchRegionNumberInHeight) {
		searchRegionNumberInHeight--;
		ySearchRegionDistance += (EB_S16)(((contextPtr->hmeLevel0SearchAreaInHeightArray[searchRegionNumberInHeight] * searchAreaMultiplierY) / 100));
	}

	xSearchAreaOrigin = -(EB_S16)((((contextPtr->hmeLevel0TotalSearchAreaWidth * searchAreaMultiplierX) / 100) ) >> 1) + xSearchRegionDistance;
	ySearchAreaOrigin = -(EB_S16)((((contextPtr->hmeLevel0TotalSearchAreaHeight * searchAreaMultiplierY) / 100)) >> 1) + ySearchRegionDistance;

	// Correct the left edge of the Search Area if it is not on the reference Picture
	xSearchAreaOrigin = ((originX + xSearchAreaOrigin) < -padWidth) ?
		-padWidth - originX :
		xSearchAreaOrigin;

	searchAreaWidth = ((originX + xSearchAreaOrigin) < -padWidth) ?
		searchAreaWidth - (-padWidth - (originX + xSearchAreaOrigin)) :
		searchAreaWidth;

	// Correct the right edge of the Search Area if its not on the reference Picture
	xSearchAreaOrigin = ((originX + xSearchAreaOrigin) > (EB_S16)sixteenthRefPicPtr->width - 1) ?
		xSearchAreaOrigin - ((originX + xSearchAreaOrigin) - ((EB_S16)sixteenthRefPicPtr->width - 1)) :
		xSearchAreaOrigin;

	searchAreaWidth = ((originX + xSearchAreaOrigin + searchAreaWidth) > (EB_S16)sixteenthRefPicPtr->width) ?
		MAX(1, searchAreaWidth - ((originX + xSearchAreaOrigin + searchAreaWidth) - (EB_S16)sixteenthRefPicPtr->width)) :
		searchAreaWidth;

	// Correct the top edge of the Search Area if it is not on the reference Picture
	ySearchAreaOrigin = ((originY + ySearchAreaOrigin) < -padHeight) ?
		-padHeight - originY :
		ySearchAreaOrigin;

	searchAreaHeight = ((originY + ySearchAreaOrigin) < -padHeight) ?
		searchAreaHeight - (-padHeight - (originY + ySearchAreaOrigin)) :
		searchAreaHeight;

	// Correct the bottom edge of the Search Area if its not on the reference Picture
	ySearchAreaOrigin = ((originY + ySearchAreaOrigin) > (EB_S16)sixteenthRefPicPtr->height - 1) ?
		ySearchAreaOrigin - ((originY + ySearchAreaOrigin) - ((EB_S16)sixteenthRefPicPtr->height - 1)) :
		ySearchAreaOrigin;

	searchAreaHeight = (originY + ySearchAreaOrigin + searchAreaHeight > (EB_S16)sixteenthRefPicPtr->height) ?
		MAX(1, searchAreaHeight - ((originY + ySearchAreaOrigin + searchAreaHeight) - (EB_S16)sixteenthRefPicPtr->height)) :
		searchAreaHeight;

	xTopLeftSearchRegion = ((EB_S16)sixteenthRefPicPtr->originX + originX) + xSearchAreaOrigin;
	yTopLeftSearchRegion = ((EB_S16)sixteenthRefPicPtr->originY + originY) + ySearchAreaOrigin;
	searchRegionIndex = xTopLeftSearchRegion + yTopLeftSearchRegion * sixteenthRefPicPtr->strideY;
	
	if (((lcuWidth  & 7) == 0) || (lcuWidth == 4))
	{
#ifndef NON_AVX512_SUPPORT
        if (((searchAreaWidth & 15) == 0) && (!!(ASM_TYPES & AVX512_MASK)))
#else
        if (((searchAreaWidth & 15) == 0) && (!!(ASM_TYPES & AVX2_MASK)))
#endif
		{
#ifndef NON_AVX512_SUPPORT
		    SadLoopKernel_AVX512_HmeL0_INTRIN(
				&contextPtr->sixteenthLcuBuffer[0],
				contextPtr->sixteenthLcuBufferStride,
				&sixteenthRefPicPtr->bufferY[searchRegionIndex],
				sixteenthRefPicPtr->strideY * 2,
				lcuHeight >> 1, lcuWidth,
				// results 
				level0BestSad,
				xLevel0SearchCenter,
				yLevel0SearchCenter,
				// range 
				sixteenthRefPicPtr->strideY,
				searchAreaWidth,
				searchAreaHeight
				);
#else
            SadLoopKernel_AVX2_HmeL0_INTRIN(
                &contextPtr->sixteenthLcuBuffer[0],
                contextPtr->sixteenthLcuBufferStride,
                &sixteenthRefPicPtr->bufferY[searchRegionIndex],
                sixteenthRefPicPtr->strideY * 2,
                lcuHeight >> 1, lcuWidth,
                // results
                level0BestSad,
                xLevel0SearchCenter,
                yLevel0SearchCenter,
                // range
                sixteenthRefPicPtr->strideY,
                searchAreaWidth,
                searchAreaHeight
            );
#endif
        }
		else
		    {
			    // Put the first search location into level0 results
			    NxMSadLoopKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
				    &contextPtr->sixteenthLcuBuffer[0],
				    contextPtr->sixteenthLcuBufferStride,
				    &sixteenthRefPicPtr->bufferY[searchRegionIndex],
				    sixteenthRefPicPtr->strideY * 2,
				    lcuHeight >> 1, lcuWidth,
				    /* results */
				    level0BestSad,
				    xLevel0SearchCenter,
				    yLevel0SearchCenter,
				    /* range */
				    sixteenthRefPicPtr->strideY,
				    searchAreaWidth,
				    searchAreaHeight
				    );
		}
	}
	else
	{
		SadLoopKernel(
			&contextPtr->sixteenthLcuBuffer[0],
			contextPtr->sixteenthLcuBufferStride,
			&sixteenthRefPicPtr->bufferY[searchRegionIndex],
			sixteenthRefPicPtr->strideY * 2,
			lcuHeight >> 1, lcuWidth,
			/* results */
			level0BestSad,
			xLevel0SearchCenter,
			yLevel0SearchCenter,
			/* range */
			sixteenthRefPicPtr->strideY,
			searchAreaWidth,
			searchAreaHeight
			);
	}

	*level0BestSad *= 2; // Multiply by 2 because considered only ever other line
	*xLevel0SearchCenter += xSearchAreaOrigin;
	*xLevel0SearchCenter *= 4; // Multiply by 4 because operating on 1/4 resolution
	*yLevel0SearchCenter += ySearchAreaOrigin;
	*yLevel0SearchCenter *= 4; // Multiply by 4 because operating on 1/4 resolution

	return;
}

void HmeLevel1(
	MeContext_t             *contextPtr,                        // input/output parameter, ME context Ptr, used to get/update ME results
	EB_S16                   originX,                           // input parameter, LCU position in the horizontal direction - quarter resolution
	EB_S16                   originY,                           // input parameter, LCU position in the vertical direction - quarter resolution
	EB_U32                   lcuWidth,                          // input parameter, LCU pwidth - quarter resolution                
	EB_U32                   lcuHeight,                         // input parameter, LCU height - quarter resolution
	EbPictureBufferDesc_t   *quarterRefPicPtr,                  // input parameter, quarter reference Picture Ptr
	EB_S16                   hmeLevel1SearchAreaInWidth,         // input parameter, hme level 1 search area in width
	EB_S16                   hmeLevel1SearchAreaInHeight,        // input parameter, hme level 1 search area in height
	EB_S16                   xLevel0SearchCenter,               // input parameter, best Level0 xMV at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_S16                   yLevel0SearchCenter,               // input parameter, best Level0 yMV at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_U64                  *level1BestSad,                     // output parameter, Level1 SAD at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_S16                  *xLevel1SearchCenter,               // output parameter, Level1 xMV at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_S16                  *yLevel1SearchCenter)               // output parameter, Level1 yMV at (searchRegionNumberInWidth, searchRegionNumberInHeight)
{


	EB_S16 xTopLeftSearchRegion;
	EB_S16 yTopLeftSearchRegion;
	EB_U32 searchRegionIndex;
	// round the search region width to nearest multiple of 8 if it is less than 8 or non multiple of 8
	// SAD calculation performance is the same for searchregion width from 1 to 8
	EB_S16 searchAreaWidth = (hmeLevel1SearchAreaInWidth < 8) ? 8 : (hmeLevel1SearchAreaInWidth & 7) ? hmeLevel1SearchAreaInWidth + (hmeLevel1SearchAreaInWidth - ((hmeLevel1SearchAreaInWidth >> 3) << 3)) : hmeLevel1SearchAreaInWidth;
	EB_S16 searchAreaHeight = hmeLevel1SearchAreaInHeight;

	EB_S16 xSearchAreaOrigin;
	EB_S16 ySearchAreaOrigin;

	EB_S16 padWidth = (EB_S16)(quarterRefPicPtr->originX) - 1;
	EB_S16 padHeight = (EB_S16)(quarterRefPicPtr->originY) - 1;

	xSearchAreaOrigin = -(searchAreaWidth >> 1) + xLevel0SearchCenter;
	ySearchAreaOrigin = -(searchAreaHeight >> 1) + yLevel0SearchCenter;

	// Correct the left edge of the Search Area if it is not on the reference Picture
    xSearchAreaOrigin = ((originX + xSearchAreaOrigin) < -padWidth) ?
        -padWidth - originX :
        xSearchAreaOrigin;

    searchAreaWidth = ((originX + xSearchAreaOrigin) < -padWidth) ?
        searchAreaWidth - (-padWidth - (originX + xSearchAreaOrigin)) :
        searchAreaWidth;
	// Correct the right edge of the Search Area if its not on the reference Picture
	xSearchAreaOrigin = ((originX + xSearchAreaOrigin) > (EB_S16)quarterRefPicPtr->width - 1) ?
		xSearchAreaOrigin - ((originX + xSearchAreaOrigin) - ((EB_S16)quarterRefPicPtr->width - 1)) :
		xSearchAreaOrigin;

	searchAreaWidth = ((originX + xSearchAreaOrigin + searchAreaWidth) > (EB_S16)quarterRefPicPtr->width) ?
		MAX(1, searchAreaWidth - ((originX + xSearchAreaOrigin + searchAreaWidth) - (EB_S16)quarterRefPicPtr->width)) :
		searchAreaWidth;

	// Correct the top edge of the Search Area if it is not on the reference Picture
	ySearchAreaOrigin = ((originY + ySearchAreaOrigin) < -padHeight) ?
		-padHeight - originY :
		ySearchAreaOrigin;

	searchAreaHeight = ((originY + ySearchAreaOrigin) < -padHeight) ?
		searchAreaHeight - (-padHeight - (originY + ySearchAreaOrigin)) :
		searchAreaHeight;

	// Correct the bottom edge of the Search Area if its not on the reference Picture
	ySearchAreaOrigin = ((originY + ySearchAreaOrigin) > (EB_S16)quarterRefPicPtr->height - 1) ?
		ySearchAreaOrigin - ((originY + ySearchAreaOrigin) - ((EB_S16)quarterRefPicPtr->height - 1)) :
		ySearchAreaOrigin;

	searchAreaHeight = (originY + ySearchAreaOrigin + searchAreaHeight > (EB_S16)quarterRefPicPtr->height) ?
		MAX(1, searchAreaHeight - ((originY + ySearchAreaOrigin + searchAreaHeight) - (EB_S16)quarterRefPicPtr->height)) :
		searchAreaHeight;

	// Move to the top left of the search region
	xTopLeftSearchRegion = ((EB_S16)quarterRefPicPtr->originX + originX) + xSearchAreaOrigin;
	yTopLeftSearchRegion = ((EB_S16)quarterRefPicPtr->originY + originY) + ySearchAreaOrigin;
	searchRegionIndex = xTopLeftSearchRegion + yTopLeftSearchRegion * quarterRefPicPtr->strideY;
	
	if (((lcuWidth & 7) == 0) || (lcuWidth == 4))
	{
		// Put the first search location into level0 results
		NxMSadLoopKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
			&contextPtr->quarterLcuBuffer[0],
			contextPtr->quarterLcuBufferStride * 2,
			&quarterRefPicPtr->bufferY[searchRegionIndex],
			quarterRefPicPtr->strideY * 2,
			lcuHeight >> 1, lcuWidth,
			/* results */
			level1BestSad,
			xLevel1SearchCenter,
			yLevel1SearchCenter,
			/* range */
			quarterRefPicPtr->strideY,
			searchAreaWidth,
			searchAreaHeight
			);
	}
	else
	{
		SadLoopKernel(
			&contextPtr->quarterLcuBuffer[0],
			contextPtr->quarterLcuBufferStride * 2,
			&quarterRefPicPtr->bufferY[searchRegionIndex],
			quarterRefPicPtr->strideY * 2,
			lcuHeight >> 1, lcuWidth,
			/* results */
			level1BestSad,
			xLevel1SearchCenter,
			yLevel1SearchCenter,
			/* range */
			quarterRefPicPtr->strideY,
			searchAreaWidth,
			searchAreaHeight
			);
	}

	*level1BestSad *= 2; // Multiply by 2 because considered only ever other line
	*xLevel1SearchCenter += xSearchAreaOrigin;
	*xLevel1SearchCenter *= 2; // Multiply by 2 because operating on 1/2 resolution
	*yLevel1SearchCenter += ySearchAreaOrigin;
	*yLevel1SearchCenter *= 2; // Multiply by 2 because operating on 1/2 resolution

	return;
}

void HmeLevel2(
	MeContext_t             *contextPtr,                        // input/output parameter, ME context Ptr, used to get/update ME results
	EB_S16                   originX,                           // input parameter, LCU position in the horizontal direction
	EB_S16                   originY,                           // input parameter, LCU position in the vertical direction
	EB_U32                   lcuWidth,                          // input parameter, LCU pwidth - full resolution                
	EB_U32                   lcuHeight,                         // input parameter, LCU height - full resolution
	EbPictureBufferDesc_t   *refPicPtr,                         // input parameter, reference Picture Ptr
	EB_U32                   searchRegionNumberInWidth,         // input parameter, search region number in the horizontal direction
	EB_U32                   searchRegionNumberInHeight,        // input parameter, search region number in the vertical direction
	EB_S16                   xLevel1SearchCenter,               // input parameter, best Level1 xMV at(searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_S16                   yLevel1SearchCenter,               // input parameter, best Level1 yMV at(searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_U64                  *level2BestSad,                     // output parameter, Level2 SAD at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_S16                  *xLevel2SearchCenter,               // output parameter, Level2 xMV at (searchRegionNumberInWidth, searchRegionNumberInHeight)
	EB_S16                  *yLevel2SearchCenter)               // output parameter, Level2 yMV at (searchRegionNumberInWidth, searchRegionNumberInHeight)
{


	EB_S16 xTopLeftSearchRegion;
	EB_S16 yTopLeftSearchRegion;
	EB_U32 searchRegionIndex;

	// round the search region width to nearest multiple of 8 if it is less than 8 or non multiple of 8
	// SAD calculation performance is the same for searchregion width from 1 to 8
	EB_S16 hmeLevel2SearchAreaInWidth = (EB_S16)contextPtr->hmeLevel2SearchAreaInWidthArray[searchRegionNumberInWidth];
	EB_S16 searchAreaWidth = (hmeLevel2SearchAreaInWidth < 8) ? 8 : (hmeLevel2SearchAreaInWidth & 7) ? hmeLevel2SearchAreaInWidth + (hmeLevel2SearchAreaInWidth - ((hmeLevel2SearchAreaInWidth >> 3) << 3)) : hmeLevel2SearchAreaInWidth;
	EB_S16 searchAreaHeight = (EB_S16)contextPtr->hmeLevel2SearchAreaInHeightArray[searchRegionNumberInHeight];

	EB_S16 xSearchAreaOrigin;
	EB_S16 ySearchAreaOrigin;

	EB_S16 padWidth = (EB_S16)MAX_LCU_SIZE - 1;
	EB_S16 padHeight = (EB_S16)MAX_LCU_SIZE - 1;

	xSearchAreaOrigin = -(searchAreaWidth >> 1) + xLevel1SearchCenter;
	ySearchAreaOrigin = -(searchAreaHeight >> 1) + yLevel1SearchCenter;

	// Correct the left edge of the Search Area if it is not on the reference Picture
	xSearchAreaOrigin = ((originX + xSearchAreaOrigin) < -padWidth) ?
		-padWidth - originX :
		xSearchAreaOrigin;

	searchAreaWidth = ((originX + xSearchAreaOrigin) < -padWidth) ?
		searchAreaWidth - (-padWidth - (originX + xSearchAreaOrigin)) :
		searchAreaWidth;

	// Correct the right edge of the Search Area if its not on the reference Picture
	xSearchAreaOrigin = ((originX + xSearchAreaOrigin) > (EB_S16)refPicPtr->width - 1) ?
		xSearchAreaOrigin - ((originX + xSearchAreaOrigin) - ((EB_S16)refPicPtr->width - 1)) :
		xSearchAreaOrigin;

	searchAreaWidth = ((originX + xSearchAreaOrigin + searchAreaWidth) > (EB_S16)refPicPtr->width) ?
		MAX(1, searchAreaWidth - ((originX + xSearchAreaOrigin + searchAreaWidth) - (EB_S16)refPicPtr->width)) :
		searchAreaWidth;

	// Correct the top edge of the Search Area if it is not on the reference Picture
	ySearchAreaOrigin = ((originY + ySearchAreaOrigin) < -padHeight) ?
		-padHeight - originY :
		ySearchAreaOrigin;

	searchAreaHeight = ((originY + ySearchAreaOrigin) < -padHeight) ?
		searchAreaHeight - (-padHeight - (originY + ySearchAreaOrigin)) :
		searchAreaHeight;

	// Correct the bottom edge of the Search Area if its not on the reference Picture
	ySearchAreaOrigin = ((originY + ySearchAreaOrigin) > (EB_S16)refPicPtr->height - 1) ?
		ySearchAreaOrigin - ((originY + ySearchAreaOrigin) - ((EB_S16)refPicPtr->height - 1)) :
		ySearchAreaOrigin;

	searchAreaHeight = (originY + ySearchAreaOrigin + searchAreaHeight > (EB_S16)refPicPtr->height) ?
		MAX(1, searchAreaHeight - ((originY + ySearchAreaOrigin + searchAreaHeight) - (EB_S16)refPicPtr->height)) :
		searchAreaHeight;

	// Move to the top left of the search region
	xTopLeftSearchRegion = ((EB_S16)refPicPtr->originX + originX) + xSearchAreaOrigin;
	yTopLeftSearchRegion = ((EB_S16)refPicPtr->originY + originY) + ySearchAreaOrigin;
	searchRegionIndex = xTopLeftSearchRegion + yTopLeftSearchRegion * refPicPtr->strideY;
	if ((((lcuWidth & 7) == 0) && (lcuWidth != 40) && (lcuWidth != 56)))
	{
		// Put the first search location into level0 results
		NxMSadLoopKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
			contextPtr->lcuSrcPtr,
			contextPtr->lcuSrcStride * 2,
			&refPicPtr->bufferY[searchRegionIndex],
			refPicPtr->strideY * 2,
			lcuHeight >> 1, lcuWidth,
			/* results */
			level2BestSad,
			xLevel2SearchCenter,
			yLevel2SearchCenter,
			/* range */
			refPicPtr->strideY,
			searchAreaWidth,
			searchAreaHeight
			);
	}
	else
	{		
		// Put the first search location into level0 results
		SadLoopKernel(
			contextPtr->lcuSrcPtr,
			contextPtr->lcuSrcStride * 2,
			&refPicPtr->bufferY[searchRegionIndex],
			refPicPtr->strideY * 2,
			lcuHeight >> 1, lcuWidth,
			/* results */
			level2BestSad,
			xLevel2SearchCenter,
			yLevel2SearchCenter,
			/* range */
			refPicPtr->strideY,
			searchAreaWidth,
			searchAreaHeight
			);

	}

	*level2BestSad *= 2; // Multiply by 2 because considered only every other line
	*xLevel2SearchCenter += xSearchAreaOrigin;
	*yLevel2SearchCenter += ySearchAreaOrigin;

	return;
}



static void SelectBuffer(
	EB_U32                 puIndex,                         //[IN]
	EB_U8                  fracPosition,                    //[IN]
	EB_U32                 puWidth,                         //[IN] Refrence picture list index
	EB_U32                 puHeight,                        //[IN] Refrence picture index in the list
	EB_U8                 *pos_Full,                        //[IN] 
	EB_U8                 *pos_b,                           //[IN]
	EB_U8                 *pos_h,                           //[IN]
	EB_U8                 *pos_j,                           //[IN]
	EB_U32                 refHalfStride,                       //[IN]
	EB_U32                 refBufferFullStride,
	EB_U8                 **DstPtr,                             //[OUT]
	EB_U32                 *DstPtrStride)                       //[OUT]
{

	(void)puWidth;
	(void)puHeight;

	EB_U32 puShiftXIndex = puSearchIndexMap[puIndex][0];
	EB_U32 puShiftYIndex = puSearchIndexMap[puIndex][1];
	EB_U32 refStride = refHalfStride;

	//for each one of the 8 positions, we need to determine the 2 buffers to  do averaging
	EB_U8  *buf1 = pos_Full;

	switch (fracPosition)
	{
	case 0: // integer
		buf1 = pos_Full;
		refStride = refBufferFullStride;
		break;
	case 2: // b
		buf1 = pos_b;
		break;
	case 8: // h
		buf1 = pos_h;
		break;
	case 10: // j
		buf1 = pos_j;
		break;
	default:
		break;
	}

	buf1 = buf1 + puShiftXIndex + puShiftYIndex * refStride;

	*DstPtr = buf1;
	*DstPtrStride = refStride;


	return;
}


static void QuarterPelCompensation(
	EB_U32                 puIndex,                         //[IN]
	EB_U8                  fracPosition,                    //[IN]
	EB_U32                 puWidth,                         //[IN] Refrence picture list index
	EB_U32                 puHeight,                        //[IN] Refrence picture index in the list
	EB_U8                 *pos_Full,                        //[IN] 
	EB_U8                 *pos_b,                           //[IN]
	EB_U8                 *pos_h,                           //[IN]
	EB_U8                 *pos_j,                           //[IN]
	EB_U32                 refHalfStride,                       //[IN]
	EB_U32                 refBufferFullStride,
	EB_U8                 *Dst,                             //[IN]
	EB_U32                 DstStride)                       //[IN]
{


	EB_U32 puShiftXIndex = puSearchIndexMap[puIndex][0];
	EB_U32 puShiftYIndex = puSearchIndexMap[puIndex][1];
	EB_U32 refStride1 = refHalfStride;
	EB_U32 refStride2 = refHalfStride;

	//for each one of the 8 positions, we need to determine the 2 buffers to  do averaging
	EB_U8  *buf1 = pos_Full;
	EB_U8  *buf2 = pos_Full;

	switch (fracPosition)
	{
	case 1: // a
		buf1 = pos_Full;
		buf2 = pos_b;
		refStride1 = refBufferFullStride;
		break;

	case 3: // c
		buf1 = pos_b;
		buf2 = pos_Full + 1;
		refStride2 = refBufferFullStride;
		break;

	case 4: // d
		buf1 = pos_Full;
		buf2 = pos_h;
		refStride1 = refBufferFullStride;
		break;

	case 5: // e
		buf1 = pos_b;
		buf2 = pos_h;
		break;

	case 6: // f
		buf1 = pos_b;
		buf2 = pos_j;
		break;

	case 7: // g
		buf1 = pos_b;
		buf2 = pos_h + 1;
		break;

	case 9: // i
		buf1 = pos_h;
		buf2 = pos_j;
		break;

	case 11: // k
		buf1 = pos_j;
		buf2 = pos_h + 1;
		break;

	case 12: // L
		buf1 = pos_h;
		buf2 = pos_Full + refBufferFullStride;
		refStride2 = refBufferFullStride;
		break;

	case 13: // m
		buf1 = pos_h;
		buf2 = pos_b + refHalfStride;
		break;

	case 14: // n
		buf1 = pos_j;
		buf2 = pos_b + refHalfStride;
		break;
	case 15: // 0
		buf1 = pos_h + 1;
		buf2 = pos_b + refHalfStride;
		break;
	default:
		break;
	}



	buf1 = buf1 + puShiftXIndex + puShiftYIndex * refStride1;
	buf2 = buf2 + puShiftXIndex + puShiftYIndex * refStride2;

	PictureAverageArray[!!(ASM_TYPES & PREAVX2_MASK)](buf1, refStride1, buf2, refStride2, Dst, DstStride, puWidth, puHeight);

	return;
}

/*******************************************************************************
 * Requirement: puWidth      = 8, 16, 24, 32, 48 or 64
 * Requirement: puHeight % 2 = 0
 * Requirement: skip         = 0 or 1
 * Requirement (x86 only): tempBuf % 16 = 0
 * Requirement (x86 only): (dst->bufferY  + dstLumaIndex  ) % 16 = 0 when puWidth %16 = 0
 * Requirement (x86 only): (dst->bufferCb + dstChromaIndex) % 16 = 0 when puWidth %32 = 0
 * Requirement (x86 only): (dst->bufferCr + dstChromaIndex) % 16 = 0 when puWidth %32 = 0
 * Requirement (x86 only): dst->strideY   % 16 = 0 when puWidth %16 = 0
 * Requirement (x86 only): dst->chromaStride % 16 = 0 when puWidth %32 = 0
 *******************************************************************************/
EB_U32 BiPredAverging(
    MeContext_t           *contextPtr,
	MePredUnit_t          *meCandidate,
	EB_U32                 puIndex,
	EB_U8                 *sourcePic,
	EB_U32                 lumaStride,
	EB_U8                  firstFracPos,
	EB_U8                  secondFracPos,
	EB_U32                 puWidth,
	EB_U32                 puHeight,
	EB_U8                  *firstRefInteger,
	EB_U8                  *firstRefPosB,
	EB_U8                  *firstRefPosH,
	EB_U8                  *firstRefPosJ,
	EB_U8                  *secondRefInteger,
	EB_U8                  *secondRefPosB,
	EB_U8                  *secondRefPosH,
	EB_U8                  *secondRefPosJ,
	EB_U32                 refBufferStride,
	EB_U32                 refBufferFullList0Stride,
	EB_U32                 refBufferFullList1Stride,
	EB_U8                  *firstRefTempDst,
	EB_U8                  *secondRefTempDst)
{

	EB_U8                  *ptrList0, *ptrList1;
	EB_U32                  ptrList0Stride, ptrList1Stride;

	// Buffer Selection and quater-pel compensation on the fly
	if (subPositionType[firstFracPos] != 2){

		SelectBuffer(
			puIndex,
			firstFracPos,
			puWidth,
			puHeight,
			firstRefInteger,
			firstRefPosB,
			firstRefPosH,
			firstRefPosJ,
			refBufferStride,
			refBufferFullList0Stride,
			&ptrList0,
			&ptrList0Stride);

	}
	else{

		QuarterPelCompensation(
			puIndex,
			firstFracPos,
			puWidth,
			puHeight,
			firstRefInteger,
			firstRefPosB,
			firstRefPosH,
			firstRefPosJ,
			refBufferStride,
			refBufferFullList0Stride,
			firstRefTempDst,
			MAX_LCU_SIZE);

		ptrList0 = firstRefTempDst;
		ptrList0Stride = MAX_LCU_SIZE;
	}

	if (subPositionType[secondFracPos] != 2){

		SelectBuffer(
			puIndex,
			secondFracPos,
			puWidth,
			puHeight,
			secondRefInteger,
			secondRefPosB,
			secondRefPosH,
			secondRefPosJ,
			refBufferStride,
			refBufferFullList1Stride,
			&ptrList1,
			&ptrList1Stride);

	}
	else{
		//uni-prediction List1 luma
		//doing the luma interpolation
		QuarterPelCompensation(
			puIndex,
			secondFracPos,
			puWidth,
			puHeight,
			secondRefInteger,
			secondRefPosB,
			secondRefPosH,
			secondRefPosJ,
			refBufferStride,
			refBufferFullList1Stride,
			secondRefTempDst,
			MAX_LCU_SIZE);

		ptrList1 = secondRefTempDst;
		ptrList1Stride = MAX_LCU_SIZE;

	}

	// bi-pred luma
    meCandidate->distortion = (contextPtr->fractionalSearchMethod == SUB_SAD_SEARCH) ?
        NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](sourcePic,
            lumaStride << 1,
            ptrList0,
            ptrList0Stride << 1,
            ptrList1,
            ptrList1Stride << 1,
            puHeight >> 1,
            puWidth) << 1 :
        NxMSadAveragingKernel_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][puWidth >> 3](sourcePic,
		    lumaStride,
		    ptrList0,
		    ptrList0Stride,
		    ptrList1,
		    ptrList1Stride,
		    puHeight,
		    puWidth);

	return meCandidate->distortion;
}


/*******************************************
 * BiPredictionComponsation
 *   performs componsation fro List 0 and
 *   List1 Candidates and then compute the
 *   average
 *******************************************/
EB_ERRORTYPE  BiPredictionCompensation(
	MeContext_t             *contextPtr,
	EB_U32                  puIndex,
	MePredUnit_t            *meCandidate,
	EB_U32                  firstList,
	EB_U32                  firstRefMv,
	EB_U32                  secondList,
	EB_U32                  secondRefMv)
{
	EB_ERRORTYPE                    return_error = EB_ErrorNone;

	EB_S16                           firstRefPosX;
	EB_S16                           firstRefPosY;
	EB_S16                           firstRefIntegPosx;
	EB_S16                           firstRefIntegPosy;
	EB_U8                            firstRefFracPosx;
	EB_U8                            firstRefFracPosy;
	EB_U8                            firstRefFracPos;
	EB_S32                           xfirstSearchIndex;
	EB_S32                           yfirstSearchIndex;
	EB_S32                           firstSearchRegionIndexPosInteg;
	EB_S32                           firstSearchRegionIndexPosb;
	EB_S32                           firstSearchRegionIndexPosh;
	EB_S32                           firstSearchRegionIndexPosj;

	EB_S16                           secondRefPosX;
	EB_S16                           secondRefPosY;
	EB_S16                           secondRefIntegPosx;
	EB_S16                           secondRefIntegPosy;
	EB_U8                            secondRefFracPosx;
	EB_U8                            secondRefFracPosy;
	EB_U8                            secondRefFracPos;
	EB_S32                           xsecondSearchIndex;
	EB_S32                           ysecondSearchIndex;
	EB_S32                           secondSearchRegionIndexPosInteg;
	EB_S32                           secondSearchRegionIndexPosb;
	EB_S32                           secondSearchRegionIndexPosh;
	EB_S32                           secondSearchRegionIndexPosj;


	EB_U32 puShiftXIndex = puSearchIndexMap[puIndex][0];
	EB_U32 puShiftYIndex = puSearchIndexMap[puIndex][1];

	const EB_U32 puLcuBufferIndex = puShiftXIndex + puShiftYIndex * contextPtr->lcuSrcStride;

	meCandidate->predictionDirection = BI_PRED;


	// First refrence
	// Set Candidate information

	firstRefPosX = _MVXT(firstRefMv),
		firstRefPosY = _MVYT(firstRefMv),
		meCandidate->Mv[0] = firstRefMv;

	firstRefIntegPosx = (firstRefPosX >> 2);
	firstRefIntegPosy = (firstRefPosY >> 2);
	firstRefFracPosx = (EB_U8)firstRefPosX & 0x03;
	firstRefFracPosy = (EB_U8)firstRefPosY & 0x03;

	firstRefFracPos = (EB_U8)(firstRefFracPosx + (firstRefFracPosy << 2));

	xfirstSearchIndex = (EB_S32)firstRefIntegPosx - contextPtr->xSearchAreaOrigin[firstList][0];
	yfirstSearchIndex = (EB_S32)firstRefIntegPosy - contextPtr->ySearchAreaOrigin[firstList][0];

	firstSearchRegionIndexPosInteg = (EB_S32)(xfirstSearchIndex + (ME_FILTER_TAP >> 1)) + (EB_S32)contextPtr->interpolatedFullStride[firstList][0] * (EB_S32)(yfirstSearchIndex + (ME_FILTER_TAP >> 1));
	firstSearchRegionIndexPosb = (EB_S32)(xfirstSearchIndex + (ME_FILTER_TAP >> 1) - 1) + (EB_S32)contextPtr->interpolatedStride * (EB_S32)(yfirstSearchIndex + (ME_FILTER_TAP >> 1));
	firstSearchRegionIndexPosh = (EB_S32)(xfirstSearchIndex + (ME_FILTER_TAP >> 1) - 1) + (EB_S32)contextPtr->interpolatedStride * (EB_S32)(yfirstSearchIndex + (ME_FILTER_TAP >> 1) - 1);
	firstSearchRegionIndexPosj = (EB_S32)(xfirstSearchIndex + (ME_FILTER_TAP >> 1) - 1) + (EB_S32)contextPtr->interpolatedStride * (EB_S32)(yfirstSearchIndex + (ME_FILTER_TAP >> 1) - 1);

	// Second refrence

	// Set Candidate information
	secondRefPosX = _MVXT(secondRefMv),
		secondRefPosY = _MVYT(secondRefMv),
		meCandidate->Mv[1] = secondRefMv;

	secondRefIntegPosx = (secondRefPosX >> 2);
	secondRefIntegPosy = (secondRefPosY >> 2);
	secondRefFracPosx = (EB_U8)secondRefPosX & 0x03;
	secondRefFracPosy = (EB_U8)secondRefPosY & 0x03;

	secondRefFracPos = (EB_U8)(secondRefFracPosx + (secondRefFracPosy << 2));

	xsecondSearchIndex = secondRefIntegPosx - contextPtr->xSearchAreaOrigin[secondList][0];
	ysecondSearchIndex = secondRefIntegPosy - contextPtr->ySearchAreaOrigin[secondList][0];

	secondSearchRegionIndexPosInteg = (EB_S32)(xsecondSearchIndex + (ME_FILTER_TAP >> 1)) + (EB_S32)contextPtr->interpolatedFullStride[secondList][0] * (EB_S32)(ysecondSearchIndex + (ME_FILTER_TAP >> 1));
	secondSearchRegionIndexPosb     = (EB_S32)(xsecondSearchIndex + (ME_FILTER_TAP >> 1) - 1) + (EB_S32)contextPtr->interpolatedStride * (EB_S32)(ysecondSearchIndex + (ME_FILTER_TAP >> 1));
	secondSearchRegionIndexPosh     = (EB_S32)(xsecondSearchIndex + (ME_FILTER_TAP >> 1) - 1) + (EB_S32)contextPtr->interpolatedStride * (EB_S32)(ysecondSearchIndex + (ME_FILTER_TAP >> 1) - 1);
	secondSearchRegionIndexPosj     = (EB_S32)(xsecondSearchIndex + (ME_FILTER_TAP >> 1) - 1) + (EB_S32)contextPtr->interpolatedStride * (EB_S32)(ysecondSearchIndex + (ME_FILTER_TAP >> 1) - 1);

	EB_U32 nIndex =   puIndex >  20   ?   tab8x8[puIndex-21]  + 21   :
		puIndex >   4   ?   tab32x32[puIndex-5] + 5    :  puIndex;
	contextPtr->pLcuBipredSad[nIndex] = 

		BiPredAverging(
            contextPtr,
		    meCandidate,
		    puIndex,
		    &(contextPtr->lcuSrcPtr[puLcuBufferIndex]),
		    contextPtr->lcuSrcStride,
		    firstRefFracPos,
		    secondRefFracPos,
		    partitionWidth[puIndex],
		    partitionHeight[puIndex],
		    &(contextPtr->integerBufferPtr[firstList][0][firstSearchRegionIndexPosInteg]),
		    &(contextPtr->posbBuffer[firstList][0][firstSearchRegionIndexPosb]),
		    &(contextPtr->poshBuffer[firstList][0][firstSearchRegionIndexPosh]),
		    &(contextPtr->posjBuffer[firstList][0][firstSearchRegionIndexPosj]),
		    &(contextPtr->integerBufferPtr[secondList][0][secondSearchRegionIndexPosInteg]),
		    &(contextPtr->posbBuffer[secondList][0][secondSearchRegionIndexPosb]),
		    &(contextPtr->poshBuffer[secondList][0][secondSearchRegionIndexPosh]),
		    &(contextPtr->posjBuffer[secondList][0][secondSearchRegionIndexPosj]),
		    contextPtr->interpolatedStride,
		    contextPtr->interpolatedFullStride[firstList][0],
		    contextPtr->interpolatedFullStride[secondList][0],
		    &(contextPtr->oneDIntermediateResultsBuf0[0]),
		    &(contextPtr->oneDIntermediateResultsBuf1[0]));

	return return_error;
}

/*******************************************
 * BiPredictionSearch
 *   performs Bi-Prediction Search (LCU)
 *******************************************/
// This function enables all 16 Bipred candidates when MRP is ON
EB_ERRORTYPE  BiPredictionSearch(
	MeContext_t					*contextPtr,
	EB_U32						puIndex,
	EB_U8						candidateIndex,
	EB_U32						activeRefPicFirstLisNum,
	EB_U32						activeRefPicSecondLisNum,
	EB_U8						*totalMeCandidateIndex,
	PictureParentControlSet_t   *pictureControlSetPtr)
{
	EB_ERRORTYPE                 return_error = EB_ErrorNone;

	EB_U32 firstListRefPictdx;
	EB_U32 secondListRefPictdx;

    (void)pictureControlSetPtr;
	EB_U32 nIndex =   puIndex >  20   ?   tab8x8[puIndex-21]  + 21   :
		puIndex >   4   ?   tab32x32[puIndex-5] + 5    :  puIndex; 
	for (firstListRefPictdx = 0; firstListRefPictdx < activeRefPicFirstLisNum; firstListRefPictdx++) {
		for (secondListRefPictdx = 0; secondListRefPictdx < activeRefPicSecondLisNum; secondListRefPictdx++) {

			{

				BiPredictionCompensation(
					contextPtr,
					puIndex,
					&(contextPtr->meCandidate[candidateIndex].pu[puIndex]),
					REFERENCE_PIC_LIST_0,
					contextPtr->pLcuBestMV[REFERENCE_PIC_LIST_0][firstListRefPictdx][nIndex],
					REFERENCE_PIC_LIST_1,
					contextPtr->pLcuBestMV[REFERENCE_PIC_LIST_1][secondListRefPictdx][nIndex]);

				candidateIndex++;

			}
		}
	}

	*totalMeCandidateIndex = candidateIndex;

	return return_error;
}



#define NSET_CAND(mePuResult, num, dist, dir) \
     (mePuResult)->distortionDirection[(num)].distortion = (dist); \
	 (mePuResult)->distortionDirection[(num)].direction = (dir)  ;


EB_S8 Sort3Elements(EB_U32 a, EB_U32 b, EB_U32 c){

    EB_U8 sortCode = 0;
	if (a <= b && a <= c){ 
		if (b <= c) {
			sortCode =   a_b_c;
		}else{                         
			sortCode =   a_c_b;
		}
	}else if (b <= a &&  b <= c) {
		if (a <= c) {             
			sortCode =   b_a_c;
		}else {                       
			sortCode =   b_c_a;
		}

	}else if (a <= b){ 
		sortCode =  c_a_b;
	} else{                        
		sortCode =  c_b_a;
	}


	return sortCode;
}


EB_ERRORTYPE CheckZeroZeroCenter(
	EbPictureBufferDesc_t        *refPicPtr,
	MeContext_t                  *contextPtr,
	EB_U32                       lcuOriginX,
	EB_U32                       lcuOriginY,
	EB_U32                       lcuWidth,
	EB_U32                       lcuHeight,
	EB_S16                       *xSearchCenter,
	EB_S16                       *ySearchCenter)

{
	EB_ERRORTYPE return_error       = EB_ErrorNone;
	EB_U32       searchRegionIndex, zeroMvSad, hmeMvSad, hmeMvdRate;
	EB_U64       hmeMvCost, zeroMvCost, searchCenterCost;
	EB_S16       originX            = (EB_S16)lcuOriginX;
	EB_S16       originY            = (EB_S16)lcuOriginY;
    EB_U32       subsampleSad       = 1;
    EB_S16       padWidth = (EB_S16)MAX_LCU_SIZE - 1;
    EB_S16       padHeight = (EB_S16)MAX_LCU_SIZE - 1;

	searchRegionIndex = (EB_S16)refPicPtr->originX + originX +
		((EB_S16)refPicPtr->originY + originY) * refPicPtr->strideY;

	zeroMvSad = NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][lcuWidth >> 3](
		contextPtr->lcuSrcPtr,
		contextPtr->lcuSrcStride << subsampleSad,
		&(refPicPtr->bufferY[searchRegionIndex]),
		refPicPtr->strideY << subsampleSad,
		lcuHeight >> subsampleSad,
		lcuWidth);

    zeroMvSad = zeroMvSad << subsampleSad;

    if (contextPtr->updateHmeSearchCenter) {
        // FIX
        // Correct the left edge of the Search Area if it is not on the reference Picture
        *xSearchCenter = ((originX + *xSearchCenter) < -padWidth) ?
            -padWidth - originX :
            *xSearchCenter;
        // Correct the right edge of the Search Area if its not on the reference Picture
        *xSearchCenter = ((originX + *xSearchCenter) > (EB_S16)refPicPtr->width - 1) ?
            *xSearchCenter - ((originX + *xSearchCenter) - ((EB_S16)refPicPtr->width - 1)) :
            *xSearchCenter;
        // Correct the top edge of the Search Area if it is not on the reference Picture
        *ySearchCenter = ((originY + *ySearchCenter) < -padHeight) ?
            -padHeight - originY :
            *ySearchCenter;
        // Correct the bottom edge of the Search Area if its not on the reference Picture
        *ySearchCenter = ((originY + *ySearchCenter) > (EB_S16)refPicPtr->height - 1) ?
            *ySearchCenter - ((originY + *ySearchCenter) - ((EB_S16)refPicPtr->height - 1)) :
            *ySearchCenter;
        ///
    }
    else {
        (void)padWidth;
        (void)padHeight;
    }

	zeroMvCost = zeroMvSad << COST_PRECISION;
	searchRegionIndex = (EB_S16)(refPicPtr->originX + originX) + *xSearchCenter +
		((EB_S16)(refPicPtr->originY + originY) + *ySearchCenter) * refPicPtr->strideY;

	hmeMvSad = NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][lcuWidth >> 3](
		contextPtr->lcuSrcPtr,
		contextPtr->lcuSrcStride << subsampleSad,
		&(refPicPtr->bufferY[searchRegionIndex]),
		refPicPtr->strideY << subsampleSad,
		lcuHeight >> subsampleSad,
		lcuWidth);

    hmeMvSad = hmeMvSad << subsampleSad;


	hmeMvdRate = 0;
	MeGetMvdFractionBits(
		ABS(*xSearchCenter << 2),
		ABS(*ySearchCenter << 2),
		contextPtr->mvdBitsArray,
		&hmeMvdRate);

	hmeMvCost = (hmeMvSad << COST_PRECISION) + (((contextPtr->lambda * hmeMvdRate) + MD_OFFSET) >> MD_SHIFT);
	searchCenterCost = MIN(zeroMvCost, hmeMvCost);

	*xSearchCenter = (searchCenterCost == zeroMvCost) ? 0 : *xSearchCenter;
	*ySearchCenter = (searchCenterCost == zeroMvCost) ? 0 : *ySearchCenter;

	return return_error;
}

EB_ERRORTYPE SuPelEnable(
	MeContext_t                 *contextPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
	EB_U32 listIndex,
	EB_U32 refPicIndex,
	EB_BOOL *enableHalfPel32x32,
	EB_BOOL *enableHalfPel16x16,
	EB_BOOL *enableHalfPel8x8)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 mvMag32x32 = 0;
	EB_U32 mvMag16x16 = 0;
	EB_U32 mvMag8x8 = 0;
	EB_U32 avgSad32x32 = 0;
	EB_U32 avgSad16x16 = 0;
	EB_U32 avgSad8x8 = 0;
	EB_U32 avgMvx32x32 = 0;
	EB_U32 avgMvy32x32 = 0;
	EB_U32 avgMvx16x16 = 0;
	EB_U32 avgMvy16x16 = 0;
	EB_U32 avgMvx8x8 = 0;
	EB_U32 avgMvy8x8 = 0;


	avgMvx32x32 = (_MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_0]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_1]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_2]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_3])) >> 2;
	avgMvy32x32 = (_MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_0]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_1]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_2]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_3])) >> 2;
	mvMag32x32 = SQR(avgMvx32x32) + SQR(avgMvy32x32);

	avgMvx16x16 = (_MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_0]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_1]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_2]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_3])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_4]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_5]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_6]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_7])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_8]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_9]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_10]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_11])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_12]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_13]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_14]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_15])
		) >> 4;
	avgMvy16x16 = (_MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_0]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_1]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_2]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_3])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_4]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_5]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_6]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_7])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_8]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_9]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_10]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_11])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_12]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_13]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_14]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_15])
		) >> 4;
	mvMag16x16 = SQR(avgMvx16x16) + SQR(avgMvy16x16);

	avgMvx8x8 = (_MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_0]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_1]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_2]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_3])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_4]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_5]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_6]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_7])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_8]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_9]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_10]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_11])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_12]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_13]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_14]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_15])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_16]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_17]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_18]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_19])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_20]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_21]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_22]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_23])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_24]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_25]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_26]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_27])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_28]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_29]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_30]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_31])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_32]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_33]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_34]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_35])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_36]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_37]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_38]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_39])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_40]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_41]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_42]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_43])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_44]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_45]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_46]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_47])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_48]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_49]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_50]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_51])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_52]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_53]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_54]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_55])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_56]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_57]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_58]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_59])
		+ _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_60]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_61]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_62]) + _MVXT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_63])
		) >> 6;
	avgMvy8x8 = (_MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_0]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_1]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_2]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_3])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_4]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_5]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_6]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_7])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_8]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_9]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_10]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_11])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_12]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_13]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_14]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_15])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_16]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_17]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_18]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_19])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_20]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_21]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_22]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_23])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_24]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_25]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_26]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_27])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_28]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_29]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_30]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_31])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_32]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_33]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_34]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_35])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_36]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_37]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_38]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_39])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_40]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_41]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_42]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_43])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_44]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_45]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_46]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_47])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_48]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_49]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_50]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_51])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_52]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_53]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_54]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_55])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_56]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_57]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_58]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_59])
		+ _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_60]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_61]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_62]) + _MVYT(contextPtr->pLcuBestMV[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_63])
		) >> 6;
	mvMag8x8 = SQR(avgMvx8x8) + SQR(avgMvy8x8);

	avgSad32x32 = (contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_0] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_1] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_2] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_32x32_3]) >> 2;
	avgSad16x16 = (contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_0] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_1] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_2] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_3]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_4] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_5] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_6] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_7]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_8] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_9] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_10] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_11]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_12] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_13] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_14] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_16x16_15]
		) >> 4;
	avgSad8x8 = (contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_0] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_1] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_2] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_3]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_4] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_5] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_6] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_7]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_8] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_9] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_10] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_11]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_12] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_13] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_14] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_15]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_16] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_17] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_18] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_19]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_20] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_21] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_22] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_23]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_24] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_25] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_26] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_27]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_28] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_29] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_30] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_31]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_32] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_33] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_34] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_35]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_36] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_37] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_38] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_39]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_40] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_41] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_42] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_43]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_44] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_45] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_46] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_47]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_48] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_49] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_50] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_51]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_52] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_53] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_54] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_55]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_56] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_57] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_58] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_59]
		+ contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_60] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_61] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_62] + contextPtr->pLcuBestSad[listIndex][refPicIndex][ME_TIER_ZERO_PU_8x8_63]
		) >> 6;


		if (pictureControlSetPtr->temporalLayerIndex == 0)
		{
			//32x32
			if ((mvMag32x32 < SQR(48)) && (avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_TRUE; //CLASS_0
			}
			else if ((mvMag32x32 < SQR(48)) && !(avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_FALSE; //CLASS_1
			}
			else if (!(mvMag32x32 < SQR(48)) && (avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_TRUE; //CLASS_2
			}
			else
			{
				*enableHalfPel32x32 = EB_FALSE; //CLASS_3
			}
			//16x16
			if ((mvMag16x16 < SQR(48)) && (avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_FALSE; //CLASS_0
			}
			else if ((mvMag16x16 < SQR(48)) && !(avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_TRUE; //CLASS_1
			}
			else if (!(mvMag16x16 < SQR(48)) && (avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_FALSE; //CLASS_2
			}
			else
			{
				*enableHalfPel16x16 = EB_TRUE; //CLASS_3
			}
			//8x8
			if ((mvMag8x8 < SQR(48)) && (avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_FALSE; //CLASS_0
			}
			else if ((mvMag8x8 < SQR(48)) && !(avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_TRUE; //CLASS_1
			}
			else if (!(mvMag8x8 < SQR(48)) && (avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_FALSE; //CLASS_2
			}
			else
			{
				*enableHalfPel8x8 = EB_TRUE; //CLASS_3
			}

		}

		else if (pictureControlSetPtr->temporalLayerIndex == 1)
		{
			//32x32
			if ((mvMag32x32 < SQR(32)) && (avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_TRUE; //CLASS_0
			}
			else if ((mvMag32x32 < SQR(32)) && !(avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_FALSE; //CLASS_1
			}
			else if (!(mvMag32x32 < SQR(32)) && (avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_TRUE; //CLASS_2
			}
			else
			{
				*enableHalfPel32x32 = EB_TRUE; //CLASS_3
			}
			//16x16
			if ((mvMag16x16 < SQR(32)) && (avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_FALSE; //CLASS_0
			}
			else if ((mvMag16x16 < SQR(32)) && !(avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_TRUE; //CLASS_1
			}
			else if (!(mvMag16x16 < SQR(32)) && (avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_FALSE; //CLASS_2
			}
			else
			{
				*enableHalfPel16x16 = EB_TRUE; //CLASS_3
			}
			//8x8
			if ((mvMag8x8 < SQR(32)) && (avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_FALSE; //CLASS_0
			}
			else if ((mvMag8x8 < SQR(32)) && !(avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_TRUE; //CLASS_1
			}
			else if (!(mvMag8x8 < SQR(32)) && (avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_FALSE; //CLASS_2
			}
			else
			{
				*enableHalfPel8x8 = EB_TRUE; //CLASS_3
			}

		}
		else if (pictureControlSetPtr->temporalLayerIndex == 2)
		{
			//32x32
			if ((mvMag32x32 < SQR(80)) && (avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_TRUE; //CLASS_0
			}
			else if ((mvMag32x32 < SQR(80)) && !(avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_FALSE; //CLASS_1

			}
			else if (!(mvMag32x32 < SQR(80)) && (avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_TRUE; //CLASS_2
			}
			else
			{
				*enableHalfPel32x32 = EB_FALSE; //CLASS_3
			}
			//16x16
			if ((mvMag16x16 < SQR(80)) && (avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_FALSE; //CLASS_0
			}
			else if ((mvMag16x16 < SQR(80)) && !(avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_TRUE; //CLASS_1
			}
			else if (!(mvMag16x16 < SQR(80)) && (avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_FALSE; //CLASS_2
			}
			else
			{
				*enableHalfPel16x16 = EB_TRUE; //CLASS_3
			}
			//8x8
			if ((mvMag8x8 < SQR(80)) && (avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_FALSE; //CLASS_0
			}
			else if ((mvMag8x8 < SQR(80)) && !(avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_TRUE; //CLASS_1
			}
			else if (!(mvMag8x8 < SQR(80)) && (avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_FALSE; //CLASS_2
			}
			else
			{
				*enableHalfPel8x8 = EB_TRUE; //CLASS_3
			}

		}
		else
		{
			//32x32
			if ((mvMag32x32 < SQR(48)) && (avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_TRUE; //CLASS_0
			}
			else if ((mvMag32x32 < SQR(48)) && !(avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_TRUE; //CLASS_1
			}
			else if (!(mvMag32x32 < SQR(48)) && (avgSad32x32 < 32 * 32 * 6))
			{
				*enableHalfPel32x32 = EB_TRUE; //CLASS_2
			}
			else
			{
				*enableHalfPel32x32 = EB_FALSE; //CLASS_3
			}
			//16x16
			if ((mvMag16x16 < SQR(48)) && (avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_FALSE; //CLASS_0
			}
			else if ((mvMag16x16 < SQR(48)) && !(avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_TRUE; //CLASS_1
			}
			else if (!(mvMag16x16 < SQR(48)) && (avgSad16x16 < 16 * 16 * 2))
			{
				*enableHalfPel16x16 = EB_FALSE; //CLASS_2
			}
			else
			{
				*enableHalfPel16x16 = EB_TRUE; //CLASS_3
			}
			//8x8
			if ((mvMag8x8 < SQR(48)) && (avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_FALSE; //CLASS_0
			}
			else if ((mvMag8x8 < SQR(48)) && !(avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_TRUE; //CLASS_1
			}
			else if (!(mvMag8x8 < SQR(48)) && (avgSad8x8 < 8 * 8 * 2))
			{
				*enableHalfPel8x8 = EB_FALSE; //CLASS_2
			}
			else
			{
				*enableHalfPel8x8 = EB_FALSE;// EB_TRUE; //CLASS_3
			}

		}

	return return_error;
}

static void TestSearchAreaBounds(
    EbPictureBufferDesc_t       *refPicPtr,
    MeContext_t					*contextPtr,
    EB_S16                      *xsc,
    EB_S16                      *ysc,
    EB_U32                       listIndex,
    EB_S16                       originX,
    EB_S16                       originY,
    EB_U32                       lcuWidth,
    EB_U32                       lcuHeight)
{
    // Search for (-srx/2, 0),  (+srx/2, 0),  (0, -sry/2), (0, +sry/2),
    /*
    |------------C-------------|
    |--------------------------|
    |--------------------------|
    A            0             B
    |--------------------------|
    |--------------------------|
    |------------D-------------|
    */
    EB_U32 searchRegionIndex;
    EB_S16 xSearchCenter = *xsc;
    EB_S16 ySearchCenter = *ysc;
    EB_U64 bestCost;
    EB_U64 directMvCost = 0xFFFFFFFFFFFFF;
    EB_U8  sparce_scale = 1;
    EB_S16 padWidth = (EB_S16)MAX_LCU_SIZE - 1;
    EB_S16 padHeight = (EB_S16)MAX_LCU_SIZE - 1;
    // O pos

    searchRegionIndex = (EB_S16)refPicPtr->originX + originX +
        ((EB_S16)refPicPtr->originY + originY) * refPicPtr->strideY;

    EB_U32 subsampleSad = 1;
    EB_U64 zeroMvSad = NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][lcuWidth >> 3](
        contextPtr->lcuSrcPtr,
        contextPtr->lcuSrcStride << subsampleSad,
        &(refPicPtr->bufferY[searchRegionIndex]),
        refPicPtr->strideY << subsampleSad,
        lcuHeight >> subsampleSad,
        lcuWidth);

    zeroMvSad = zeroMvSad << subsampleSad;

    EB_U64 zeroMvCost = zeroMvSad << COST_PRECISION;

    //A pos
    xSearchCenter = 0 - (contextPtr->hmeLevel0TotalSearchAreaWidth*sparce_scale);
    ySearchCenter = 0;

    // Correct the left edge of the Search Area if it is not on the reference Picture
    xSearchCenter = ((originX + xSearchCenter) < -padWidth) ?
        -padWidth - originX :
        xSearchCenter;
    // Correct the right edge of the Search Area if its not on the reference Picture
    xSearchCenter = ((originX + xSearchCenter) > (EB_S16)refPicPtr->width - 1) ?
        xSearchCenter - ((originX + xSearchCenter) - ((EB_S16)refPicPtr->width - 1)) :
        xSearchCenter;
    // Correct the top edge of the Search Area if it is not on the reference Picture
    ySearchCenter = ((originY + ySearchCenter) < -padHeight) ?
        -padHeight - originY :
        ySearchCenter;

    // Correct the bottom edge of the Search Area if its not on the reference Picture
    ySearchCenter = ((originY + ySearchCenter) > (EB_S16)refPicPtr->height - 1) ?
        ySearchCenter - ((originY + ySearchCenter) - ((EB_S16)refPicPtr->height - 1)) :
        ySearchCenter;


    EB_U64 MvASad = NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][lcuWidth >> 3](
        contextPtr->lcuSrcPtr,
        contextPtr->lcuSrcStride << subsampleSad,
        &(refPicPtr->bufferY[searchRegionIndex]),
        refPicPtr->strideY << subsampleSad,
        lcuHeight >> subsampleSad,
        lcuWidth);

    MvASad = MvASad << subsampleSad;

    EB_U32 MvAdRate = 0;
    MeGetMvdFractionBits(
        ABS(xSearchCenter << 2),
        ABS(ySearchCenter << 2),
        contextPtr->mvdBitsArray,
        &MvAdRate);

    EB_U64 MvACost = (MvASad << COST_PRECISION) + (MD_OFFSET >> MD_SHIFT);


    //B pos
    xSearchCenter = (contextPtr->hmeLevel0TotalSearchAreaWidth*sparce_scale);
    ySearchCenter = 0;
    ///////////////// correct
    // Correct the left edge of the Search Area if it is not on the reference Picture
    xSearchCenter = ((originX + xSearchCenter) < -padWidth) ?
        -padWidth - originX :
        xSearchCenter;
    // Correct the right edge of the Search Area if its not on the reference Picture
    xSearchCenter = ((originX + xSearchCenter) > (EB_S16)refPicPtr->width - 1) ?
        xSearchCenter - ((originX + xSearchCenter) - ((EB_S16)refPicPtr->width - 1)) :
        xSearchCenter;
    // Correct the top edge of the Search Area if it is not on the reference Picture
    ySearchCenter = ((originY + ySearchCenter) < -padHeight) ?
        -padHeight - originY :
        ySearchCenter;
    // Correct the bottom edge of the Search Area if its not on the reference Picture
    ySearchCenter = ((originY + ySearchCenter) > (EB_S16)refPicPtr->height - 1) ?
        ySearchCenter - ((originY + ySearchCenter) - ((EB_S16)refPicPtr->height - 1)) :
        ySearchCenter;


    searchRegionIndex = (EB_S16)(refPicPtr->originX + originX) + xSearchCenter +
        ((EB_S16)(refPicPtr->originY + originY) + ySearchCenter) * refPicPtr->strideY;

    EB_U64 MvBSad = NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][lcuWidth >> 3](
        contextPtr->lcuSrcPtr,
        contextPtr->lcuSrcStride << subsampleSad,
        &(refPicPtr->bufferY[searchRegionIndex]),
        refPicPtr->strideY << subsampleSad,
        lcuHeight >> subsampleSad,
        lcuWidth);

    MvBSad = MvBSad << subsampleSad;


    EB_U32 MvBdRate = 0;
    MeGetMvdFractionBits(
        ABS(xSearchCenter << 2),
        ABS(ySearchCenter << 2),
        contextPtr->mvdBitsArray,
        &MvBdRate);

    EB_U64 MvBCost = (MvBSad << COST_PRECISION) + (MD_OFFSET >> MD_SHIFT);
    //C pos
    xSearchCenter = 0;
    ySearchCenter = 0 - (contextPtr->hmeLevel0TotalSearchAreaHeight * sparce_scale);
    ///////////////// correct
    // Correct the left edge of the Search Area if it is not on the reference Picture
    xSearchCenter = ((originX + xSearchCenter) < -padWidth) ?
        -padWidth - originX :
        xSearchCenter;

    // Correct the right edge of the Search Area if its not on the reference Picture
    xSearchCenter = ((originX + xSearchCenter) > (EB_S16)refPicPtr->width - 1) ?
        xSearchCenter - ((originX + xSearchCenter) - ((EB_S16)refPicPtr->width - 1)) :
        xSearchCenter;

    // Correct the top edge of the Search Area if it is not on the reference Picture
    ySearchCenter = ((originY + ySearchCenter) < -padHeight) ?
        -padHeight - originY :
        ySearchCenter;

    // Correct the bottom edge of the Search Area if its not on the reference Picture
    ySearchCenter = ((originY + ySearchCenter) > (EB_S16)refPicPtr->height - 1) ?
        ySearchCenter - ((originY + ySearchCenter) - ((EB_S16)refPicPtr->height - 1)) :
        ySearchCenter;

    searchRegionIndex = (EB_S16)(refPicPtr->originX + originX) + xSearchCenter +
        ((EB_S16)(refPicPtr->originY + originY) + ySearchCenter) * refPicPtr->strideY;

    EB_U64 MvCSad = NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][lcuWidth >> 3](
        contextPtr->lcuSrcPtr,
        contextPtr->lcuSrcStride << subsampleSad,
        &(refPicPtr->bufferY[searchRegionIndex]),
        refPicPtr->strideY << subsampleSad,
        lcuHeight >> subsampleSad,
        lcuWidth);

    MvCSad = MvCSad << subsampleSad;


    EB_U32 MvCdRate = 0;
    MeGetMvdFractionBits(
        ABS(xSearchCenter << 2),
        ABS(ySearchCenter << 2),
        contextPtr->mvdBitsArray,
        &MvCdRate);

    EB_U64 MvCCost = (MvCSad << COST_PRECISION) + (MD_OFFSET >> MD_SHIFT);
    //D pos
    xSearchCenter = 0;
    ySearchCenter = (contextPtr->hmeLevel0TotalSearchAreaHeight * sparce_scale);
    // Correct the left edge of the Search Area if it is not on the reference Picture
    xSearchCenter = ((originX + xSearchCenter) < -padWidth) ?
        -padWidth - originX :
        xSearchCenter;
    // Correct the right edge of the Search Area if its not on the reference Picture
    xSearchCenter = ((originX + xSearchCenter) > (EB_S16)refPicPtr->width - 1) ?
        xSearchCenter - ((originX + xSearchCenter) - ((EB_S16)refPicPtr->width - 1)) :
        xSearchCenter;
    // Correct the top edge of the Search Area if it is not on the reference Picture
    ySearchCenter = ((originY + ySearchCenter) < -padHeight) ?
        -padHeight - originY :
        ySearchCenter;
    // Correct the bottom edge of the Search Area if its not on the reference Picture
    ySearchCenter = ((originY + ySearchCenter) > (EB_S16)refPicPtr->height - 1) ?
        ySearchCenter - ((originY + ySearchCenter) - ((EB_S16)refPicPtr->height - 1)) :
        ySearchCenter;
    searchRegionIndex = (EB_S16)(refPicPtr->originX + originX) + xSearchCenter +
        ((EB_S16)(refPicPtr->originY + originY) + ySearchCenter) * refPicPtr->strideY;
    EB_U64 MvDSad = NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][lcuWidth >> 3](
        contextPtr->lcuSrcPtr,
        contextPtr->lcuSrcStride << subsampleSad,
        &(refPicPtr->bufferY[searchRegionIndex]),
        refPicPtr->strideY << subsampleSad,
        lcuHeight >> subsampleSad,
        lcuWidth);
    MvDSad = MvDSad << subsampleSad;

    EB_U32 MvDdRate = 0;
    MeGetMvdFractionBits(
        ABS(xSearchCenter << 2),
        ABS(ySearchCenter << 2),
        contextPtr->mvdBitsArray,
        &MvDdRate);

    EB_U64 MvDCost = (MvDSad << COST_PRECISION) + (MD_OFFSET >> MD_SHIFT);

    if (listIndex == 1) {

        xSearchCenter = listIndex ? 0 - (_MVXT(contextPtr->pLcuBestMV[0][0][0]) >> 2) : 0;
        ySearchCenter = listIndex ? 0 - (_MVYT(contextPtr->pLcuBestMV[0][0][0]) >> 2) : 0;
        ///////////////// correct
        // Correct the left edge of the Search Area if it is not on the reference Picture
        xSearchCenter = ((originX + xSearchCenter) < -padWidth) ?
            -padWidth - originX :
            xSearchCenter;
        // Correct the right edge of the Search Area if its not on the reference Picture
        xSearchCenter = ((originX + xSearchCenter) > (EB_S16)refPicPtr->width - 1) ?
            xSearchCenter - ((originX + xSearchCenter) - ((EB_S16)refPicPtr->width - 1)) :
            xSearchCenter;
        // Correct the top edge of the Search Area if it is not on the reference Picture
        ySearchCenter = ((originY + ySearchCenter) < -padHeight) ?
            -padHeight - originY :
            ySearchCenter;
        // Correct the bottom edge of the Search Area if its not on the reference Picture
        ySearchCenter = ((originY + ySearchCenter) > (EB_S16)refPicPtr->height - 1) ?
            ySearchCenter - ((originY + ySearchCenter) - ((EB_S16)refPicPtr->height - 1)) :
            ySearchCenter;

        searchRegionIndex = (EB_S16)(refPicPtr->originX + originX) + xSearchCenter +
            ((EB_S16)(refPicPtr->originY + originY) + ySearchCenter) * refPicPtr->strideY;

        EB_U64 directMvSad = NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][lcuWidth >> 3](
            contextPtr->lcuSrcPtr,
            contextPtr->lcuSrcStride << subsampleSad,
            &(refPicPtr->bufferY[searchRegionIndex]),
            refPicPtr->strideY << subsampleSad,
            lcuHeight >> subsampleSad,
            lcuWidth);

        directMvSad = directMvSad << subsampleSad;


        EB_U32 direcMvdRate = 0;
        MeGetMvdFractionBits(
            ABS(xSearchCenter << 2),
            ABS(ySearchCenter << 2),
            contextPtr->mvdBitsArray,
            &direcMvdRate);

        directMvCost = (directMvSad << COST_PRECISION) + (MD_OFFSET >> MD_SHIFT);
    }

    bestCost = MIN(zeroMvCost, MIN(MvACost, MIN(MvBCost, MIN(MvCCost, MIN(MvDCost, directMvCost)))));

    if (bestCost == zeroMvCost) {
        xSearchCenter = 0;
        ySearchCenter = 0;
    }
    else if (bestCost == MvACost) {
        xSearchCenter = 0 - (contextPtr->hmeLevel0TotalSearchAreaWidth*sparce_scale);
        ySearchCenter = 0;
    }
    else if (bestCost == MvBCost) {
        xSearchCenter = (contextPtr->hmeLevel0TotalSearchAreaWidth*sparce_scale);
        ySearchCenter = 0;
    }
    else if (bestCost == MvCCost) {
        xSearchCenter = 0;
        ySearchCenter = 0 - (contextPtr->hmeLevel0TotalSearchAreaHeight * sparce_scale);
    }
    else if (bestCost == directMvCost) {
        xSearchCenter = listIndex ? 0 - (_MVXT(contextPtr->pLcuBestMV[0][0][0]) >> 2) : 0;
        ySearchCenter = listIndex ? 0 - (_MVYT(contextPtr->pLcuBestMV[0][0][0]) >> 2) : 0;
    }
    else if (bestCost == MvDCost) {
        xSearchCenter = 0;
        ySearchCenter = (contextPtr->hmeLevel0TotalSearchAreaHeight * sparce_scale);
    }

    else {
        printf("error no center selected");
    }

    *xsc = xSearchCenter;
    *ysc = ySearchCenter;


}

/*******************************************
 * MotionEstimateLcu
 *   performs ME (LCU)
 *******************************************/
EB_ERRORTYPE MotionEstimateLcu(
	PictureParentControlSet_t   *pictureControlSetPtr,  // input parameter, Picture Control Set Ptr
	EB_U32                       lcuIndex,              // input parameter, LCU Index      
	EB_U32                       lcuOriginX,            // input parameter, LCU Origin X
	EB_U32                       lcuOriginY,            // input parameter, LCU Origin X
	MeContext_t					*contextPtr,                        // input parameter, ME Context Ptr, used to store decimated/interpolated LCU/SR
	EbPictureBufferDesc_t       *inputPtr)              // input parameter, source Picture Ptr
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	SequenceControlSet_t    *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;

	EB_S16                  xTopLeftSearchRegion;
	EB_S16                  yTopLeftSearchRegion;
	EB_U32                  searchRegionIndex;
	EB_S16                  pictureWidth = (EB_S16)((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaWidth;
	EB_S16                  pictureHeight = (EB_S16)((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaHeight;
	EB_U32                  lcuWidth = (inputPtr->width - lcuOriginX) < MAX_LCU_SIZE ? inputPtr->width - lcuOriginX : MAX_LCU_SIZE;
	EB_U32                  lcuHeight = (inputPtr->height - lcuOriginY) < MAX_LCU_SIZE ? inputPtr->height - lcuOriginY : MAX_LCU_SIZE;
	EB_S16                  padWidth = (EB_S16)MAX_LCU_SIZE - 1;
	EB_S16                  padHeight = (EB_S16)MAX_LCU_SIZE - 1;
	EB_S16                  searchAreaWidth;
	EB_S16                  searchAreaHeight;
	EB_S16                  xSearchAreaOrigin;
	EB_S16                  ySearchAreaOrigin;
	EB_S16                  originX = (EB_S16)lcuOriginX;
	EB_S16                  originY = (EB_S16)lcuOriginY;

	// HME
	EB_U32                  searchRegionNumberInWidth = 0;
	EB_U32                  searchRegionNumberInHeight = 0;
	EB_S16                  xHmeLevel0SearchCenter[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
	EB_S16                  yHmeLevel0SearchCenter[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
	EB_U64                  hmeLevel0Sad[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
	EB_S16                  xHmeLevel1SearchCenter[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
	EB_S16                  yHmeLevel1SearchCenter[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
	EB_U64                  hmeLevel1Sad[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
	EB_S16                  xHmeLevel2SearchCenter[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
	EB_S16                  yHmeLevel2SearchCenter[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
	EB_U64                  hmeLevel2Sad[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT][EB_HME_SEARCH_AREA_ROW_MAX_COUNT];

	// Hierarchical ME Search Center
	EB_S16                  xHmeSearchCenter = 0;
	EB_S16                  yHmeSearchCenter = 0;

	// Final ME Search Center
	EB_S16                  xSearchCenter = 0;
	EB_S16                  ySearchCenter = 0;

	// Search Center SADs
	EB_U64                  hmeMvSad = 0;

	EB_U32                  puIndex;

	EB_U32                  maxNumberOfPusPerLcu = pictureControlSetPtr->maxNumberOfPusPerLcu;

	EB_U32                  numOfListToSearch;
	EB_U32                  listIndex;
	EB_U8                   candidateIndex = 0;
	EB_U8                   totalMeCandidateIndex = 0;
	EbPaReferenceObject_t  *referenceObject;  // input parameter, reference Object Ptr

	EbPictureBufferDesc_t  *refPicPtr;
	EbPictureBufferDesc_t  *quarterRefPicPtr;
	EbPictureBufferDesc_t  *sixteenthRefPicPtr;

	EB_S16                  tempXHmeSearchCenter = 0;
	EB_S16                  tempYHmeSearchCenter = 0;
                            
	EB_U32                  numQuadInWidth;
	EB_U32                  totalMeQuad;
	EB_U32                  quadIndex;
	EB_U32                  nextQuadIndex;
	EB_U64                  tempXHmeSad;
                            
	EB_U64                  ref0Poc = 0;
	EB_U64                  ref1Poc = 0;
                            
	EB_U64                  i;

	EB_S16                  hmeLevel1SearchAreaInWidth;
	EB_S16                  hmeLevel1SearchAreaInHeight;

	EB_U32                  adjustSearchAreaDirection = 0;

    // Configure HME level 0, level 1 and level 2 from static config parameters
    EB_BOOL                 enableHmeLevel0Flag = pictureControlSetPtr->enableHmeLevel0Flag;
    EB_BOOL                 enableHmeLevel1Flag = pictureControlSetPtr->enableHmeLevel1Flag;
    EB_BOOL                 enableHmeLevel2Flag = pictureControlSetPtr->enableHmeLevel2Flag;
	EB_BOOL					enableHalfPel32x32  = EB_FALSE;
	EB_BOOL					enableHalfPel16x16  = EB_FALSE;
	EB_BOOL					enableHalfPel8x8    = EB_FALSE;
	EB_BOOL					enableQuarterPel    = EB_FALSE;


    numOfListToSearch = (pictureControlSetPtr->sliceType == EB_P_PICTURE) ? (EB_U32)REF_LIST_0 : (EB_U32)REF_LIST_1;

	referenceObject             = (EbPaReferenceObject_t*)pictureControlSetPtr->refPaPicPtrArray[0]->objectPtr;
	ref0Poc                     = pictureControlSetPtr->refPicPocArray[0];

	if (numOfListToSearch){
		referenceObject = (EbPaReferenceObject_t*)pictureControlSetPtr->refPaPicPtrArray[1]->objectPtr;
		ref1Poc = pictureControlSetPtr->refPicPocArray[1];
	}

	// Uni-Prediction motion estimation loop
	// List Loop 
	for (listIndex = REF_LIST_0; listIndex <= numOfListToSearch; ++listIndex) {

        // Ref Picture Loop
        referenceObject     = (EbPaReferenceObject_t*)pictureControlSetPtr->refPaPicPtrArray[listIndex]->objectPtr;
        refPicPtr           = (EbPictureBufferDesc_t*)referenceObject->inputPaddedPicturePtr;
        quarterRefPicPtr    = (EbPictureBufferDesc_t*)referenceObject->quarterDecimatedPicturePtr;
        sixteenthRefPicPtr  = (EbPictureBufferDesc_t*)referenceObject->sixteenthDecimatedPicturePtr;

        if (pictureControlSetPtr->temporalLayerIndex > 0 || listIndex == 0) {
            // A - The MV center for Tier0 search could be either (0,0), or HME
            // A - Set HME MV Center
            if (contextPtr->updateHmeSearchCenter) {
                TestSearchAreaBounds(
                        refPicPtr,
                        contextPtr,
                        &xSearchCenter,
                        &ySearchCenter,
                        listIndex,
                        originX,
                        originY,
                        lcuWidth,
                        lcuHeight);
            } else {
                xSearchCenter = 0;
                ySearchCenter = 0;
            }

            // B - NO HME in boundaries
            // C - Skip HME
            if (pictureControlSetPtr->enableHmeFlag && lcuHeight == MAX_LCU_SIZE) {
                while (searchRegionNumberInHeight < EB_MIN(contextPtr->numberHmeSearchRegionInHeight, EB_HME_SEARCH_AREA_ROW_MAX_COUNT)) {
                    while (searchRegionNumberInWidth < EB_MIN(contextPtr->numberHmeSearchRegionInWidth, EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT)) {
                        if (contextPtr->updateHmeSearchCenter) {
                            xHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] = xSearchCenter >> 2;
                            yHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] = ySearchCenter >> 2;

                            xHmeLevel1SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] = xSearchCenter >> 1;
                            yHmeLevel1SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] = ySearchCenter >> 1;
                        }
                        else {
                            xHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] = xSearchCenter;
                            yHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] = ySearchCenter;

                            xHmeLevel1SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] = xSearchCenter;
                            yHmeLevel1SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] = ySearchCenter;
                        }
                        xHmeLevel2SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] = xSearchCenter;
                        yHmeLevel2SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] = ySearchCenter;

                        searchRegionNumberInWidth++;
                    }
                    searchRegionNumberInWidth = 0;
                    searchRegionNumberInHeight++;
                }

                // HME: Level0 search
                if (enableHmeLevel0Flag) {

                    if (contextPtr->oneQuadrantHME && !enableHmeLevel1Flag && !enableHmeLevel2Flag) {

                        searchRegionNumberInHeight = 0;
                        searchRegionNumberInWidth = 0;

                        HmeOneQuadrantLevel0(
                                contextPtr,
                                originX >> 2,
                                originY >> 2,
                                lcuWidth >> 2,
                                lcuHeight >> 2,
                                xSearchCenter >> 2,
                                ySearchCenter >> 2,
                                sixteenthRefPicPtr,								
                                &(hmeLevel0Sad[searchRegionNumberInWidth][searchRegionNumberInHeight]),
                                &(xHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight]),
                                &(yHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight]),
                                HME_LEVEL_0_SEARCH_AREA_MULTIPLIER_X[pictureControlSetPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex],
                                HME_LEVEL_0_SEARCH_AREA_MULTIPLIER_Y[pictureControlSetPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex]);



                    } else {

                        searchRegionNumberInHeight = 0;
                        searchRegionNumberInWidth = 0;

                        while (searchRegionNumberInHeight < contextPtr->numberHmeSearchRegionInHeight) {
                            while (searchRegionNumberInWidth < contextPtr->numberHmeSearchRegionInWidth) {

                                HmeLevel0(
                                        contextPtr,
                                        originX >> 2,
                                        originY >> 2,
                                        lcuWidth >> 2,
                                        lcuHeight >> 2,
                                        xSearchCenter >> 2,
                                        ySearchCenter >> 2,
                                        sixteenthRefPicPtr,
                                        searchRegionNumberInWidth,
                                        searchRegionNumberInHeight,
                                        &(hmeLevel0Sad[searchRegionNumberInWidth][searchRegionNumberInHeight]),
                                        &(xHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight]),
                                        &(yHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight]),
                                        HME_LEVEL_0_SEARCH_AREA_MULTIPLIER_X[pictureControlSetPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex],
                                        HME_LEVEL_0_SEARCH_AREA_MULTIPLIER_Y[pictureControlSetPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex]);

                                searchRegionNumberInWidth++;
                            }
                            searchRegionNumberInWidth = 0;
                            searchRegionNumberInHeight++;
                        }                          
                    }
                }

                // HME: Level1 search
                if (enableHmeLevel1Flag) {
                    searchRegionNumberInHeight = 0;
                    searchRegionNumberInWidth = 0;

                    while (searchRegionNumberInHeight < contextPtr->numberHmeSearchRegionInHeight) {
                        while (searchRegionNumberInWidth < contextPtr->numberHmeSearchRegionInWidth) {

                            // When HME level 0 has been disabled, increase the search area width and height for level 1 to (32x12)
                            hmeLevel1SearchAreaInWidth = (EB_S16)contextPtr->hmeLevel1SearchAreaInWidthArray[searchRegionNumberInWidth];
                            hmeLevel1SearchAreaInHeight = (EB_S16)contextPtr->hmeLevel1SearchAreaInHeightArray[searchRegionNumberInHeight];
                            HmeLevel1(
                                    contextPtr,
                                    originX >> 1,
                                    originY >> 1,
                                    lcuWidth >> 1,
                                    lcuHeight >> 1,
                                    quarterRefPicPtr,
                                    hmeLevel1SearchAreaInWidth,
                                    hmeLevel1SearchAreaInHeight,
                                    xHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] >> 1,
                                    yHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] >> 1,
                                    &(hmeLevel1Sad[searchRegionNumberInWidth][searchRegionNumberInHeight]),
                                    &(xHmeLevel1SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight]),
                                    &(yHmeLevel1SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight]));


                            searchRegionNumberInWidth++;
                        }
                        searchRegionNumberInWidth = 0;
                        searchRegionNumberInHeight++;
                    }
                }

                // HME: Level2 search
                if (enableHmeLevel2Flag) {

                    searchRegionNumberInHeight = 0;
                    searchRegionNumberInWidth = 0;

                    while (searchRegionNumberInHeight < contextPtr->numberHmeSearchRegionInHeight) {
                        while (searchRegionNumberInWidth < contextPtr->numberHmeSearchRegionInWidth) {


                            HmeLevel2(
                                    contextPtr,
                                    originX,
                                    originY,
                                    lcuWidth,
                                    lcuHeight,
                                    refPicPtr,
                                    searchRegionNumberInWidth,
                                    searchRegionNumberInHeight,
                                    xHmeLevel1SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight],
                                    yHmeLevel1SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight],
                                    &(hmeLevel2Sad[searchRegionNumberInWidth][searchRegionNumberInHeight]),
                                    &(xHmeLevel2SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight]),
                                    &(yHmeLevel2SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight]));


                            searchRegionNumberInWidth++;
                        }
                        searchRegionNumberInWidth = 0;
                        searchRegionNumberInHeight++;
                    }
                }

                // Hierarchical ME - Search Center
                if (enableHmeLevel0Flag && !enableHmeLevel1Flag && !enableHmeLevel2Flag) {

                    if (contextPtr->oneQuadrantHME)
                    {
                        xHmeSearchCenter = xHmeLevel0SearchCenter[0][0];
                        yHmeSearchCenter = yHmeLevel0SearchCenter[0][0];
                        hmeMvSad = hmeLevel0Sad[0][0];
                    }
                    else {

                        xHmeSearchCenter = xHmeLevel0SearchCenter[0][0];
                        yHmeSearchCenter = yHmeLevel0SearchCenter[0][0];
                        hmeMvSad = hmeLevel0Sad[0][0];

                        searchRegionNumberInWidth = 1;
                        searchRegionNumberInHeight = 0;

                        while (searchRegionNumberInHeight < contextPtr->numberHmeSearchRegionInHeight) {
                            while (searchRegionNumberInWidth < contextPtr->numberHmeSearchRegionInWidth) {

                                xHmeSearchCenter = (hmeLevel0Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] < hmeMvSad) ? xHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] : xHmeSearchCenter;
                                yHmeSearchCenter = (hmeLevel0Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] < hmeMvSad) ? yHmeLevel0SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] : yHmeSearchCenter;
                                hmeMvSad = (hmeLevel0Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] < hmeMvSad) ? hmeLevel0Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] : hmeMvSad;
                                searchRegionNumberInWidth++;
                            }
                            searchRegionNumberInWidth = 0;
                            searchRegionNumberInHeight++;
                        }
                    }

                }

                if (enableHmeLevel1Flag && !enableHmeLevel2Flag) {
                    xHmeSearchCenter = xHmeLevel1SearchCenter[0][0];
                    yHmeSearchCenter = yHmeLevel1SearchCenter[0][0];
                    hmeMvSad = hmeLevel1Sad[0][0];

                    searchRegionNumberInWidth = 1;
                    searchRegionNumberInHeight = 0;

                    while (searchRegionNumberInHeight < contextPtr->numberHmeSearchRegionInHeight) {
                        while (searchRegionNumberInWidth < contextPtr->numberHmeSearchRegionInWidth) {

                            xHmeSearchCenter = (hmeLevel1Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] < hmeMvSad) ? xHmeLevel1SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] : xHmeSearchCenter;
                            yHmeSearchCenter = (hmeLevel1Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] < hmeMvSad) ? yHmeLevel1SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] : yHmeSearchCenter;
                            hmeMvSad = (hmeLevel1Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] < hmeMvSad) ? hmeLevel1Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] : hmeMvSad;
                            searchRegionNumberInWidth++;
                        }
                        searchRegionNumberInWidth = 0;
                        searchRegionNumberInHeight++;
                    }
                }

                if (enableHmeLevel2Flag) {
                    xHmeSearchCenter = xHmeLevel2SearchCenter[0][0];
                    yHmeSearchCenter = yHmeLevel2SearchCenter[0][0];
                    hmeMvSad = hmeLevel2Sad[0][0];

                    searchRegionNumberInWidth = 1;
                    searchRegionNumberInHeight = 0;

                    while (searchRegionNumberInHeight < contextPtr->numberHmeSearchRegionInHeight) {
                        while (searchRegionNumberInWidth < contextPtr->numberHmeSearchRegionInWidth) {

                            xHmeSearchCenter = (hmeLevel2Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] < hmeMvSad) ? xHmeLevel2SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] : xHmeSearchCenter;
                            yHmeSearchCenter = (hmeLevel2Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] < hmeMvSad) ? yHmeLevel2SearchCenter[searchRegionNumberInWidth][searchRegionNumberInHeight] : yHmeSearchCenter;
                            hmeMvSad = (hmeLevel2Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] < hmeMvSad) ? hmeLevel2Sad[searchRegionNumberInWidth][searchRegionNumberInHeight] : hmeMvSad;
                            searchRegionNumberInWidth++;
                        }
                        searchRegionNumberInWidth = 0;
                        searchRegionNumberInHeight++;
                    }


                    numQuadInWidth = contextPtr->numberHmeSearchRegionInWidth;
                    totalMeQuad = contextPtr->numberHmeSearchRegionInHeight * contextPtr->numberHmeSearchRegionInWidth;

                    if ((ref0Poc == ref1Poc) && (listIndex == 1) && (totalMeQuad > 1)){

                        for (quadIndex = 0; quadIndex < totalMeQuad - 1; ++quadIndex) {
                            for (nextQuadIndex = quadIndex + 1; nextQuadIndex < totalMeQuad; ++nextQuadIndex) {


                                if (hmeLevel2Sad[quadIndex / numQuadInWidth][quadIndex%numQuadInWidth] > hmeLevel2Sad[nextQuadIndex / numQuadInWidth][nextQuadIndex%numQuadInWidth]) {

                                    tempXHmeSearchCenter = xHmeLevel2SearchCenter[quadIndex / numQuadInWidth][quadIndex%numQuadInWidth];
                                    tempYHmeSearchCenter = yHmeLevel2SearchCenter[quadIndex / numQuadInWidth][quadIndex%numQuadInWidth];
                                    tempXHmeSad = hmeLevel2Sad[quadIndex / numQuadInWidth][quadIndex%numQuadInWidth];

                                    xHmeLevel2SearchCenter[quadIndex / numQuadInWidth][quadIndex%numQuadInWidth] = xHmeLevel2SearchCenter[nextQuadIndex / numQuadInWidth][nextQuadIndex%numQuadInWidth];
                                    yHmeLevel2SearchCenter[quadIndex / numQuadInWidth][quadIndex%numQuadInWidth] = yHmeLevel2SearchCenter[nextQuadIndex / numQuadInWidth][nextQuadIndex%numQuadInWidth];
                                    hmeLevel2Sad[quadIndex / numQuadInWidth][quadIndex%numQuadInWidth] = hmeLevel2Sad[nextQuadIndex / numQuadInWidth][nextQuadIndex%numQuadInWidth];

                                    xHmeLevel2SearchCenter[nextQuadIndex / numQuadInWidth][nextQuadIndex%numQuadInWidth] = tempXHmeSearchCenter;
                                    yHmeLevel2SearchCenter[nextQuadIndex / numQuadInWidth][nextQuadIndex%numQuadInWidth] = tempYHmeSearchCenter;
                                    hmeLevel2Sad[nextQuadIndex / numQuadInWidth][nextQuadIndex%numQuadInWidth] = tempXHmeSad;
                                }
                            }
                        }

                        xHmeSearchCenter = xHmeLevel2SearchCenter[0][1];
                        yHmeSearchCenter = yHmeLevel2SearchCenter[0][1];
                    }
                }

                xSearchCenter = xHmeSearchCenter;
                ySearchCenter = yHmeSearchCenter;
            }
        } else {
            xSearchCenter = 0;
            ySearchCenter = 0;
        }

        searchAreaWidth = (EB_S16)MIN(contextPtr->searchAreaWidth, 127);
        searchAreaHeight = (EB_S16)MIN(contextPtr->searchAreaHeight, 127);

        if (xSearchCenter != 0 || ySearchCenter != 0) {
            CheckZeroZeroCenter(
                    refPicPtr,
                    contextPtr,
                    lcuOriginX,
                    lcuOriginY,
                    lcuWidth,
                    lcuHeight,
                    &xSearchCenter,
                    &ySearchCenter);
        }

        xSearchAreaOrigin = xSearchCenter - (searchAreaWidth >> 1);
        ySearchAreaOrigin = ySearchCenter - (searchAreaHeight >> 1);

        if (listIndex == 1 && lcuWidth == MAX_LCU_SIZE && lcuHeight == MAX_LCU_SIZE)
        {
            {
                if (adjustSearchAreaDirection == 1){
                    searchAreaWidth = (EB_S16)8;
                    searchAreaHeight = (EB_S16)5;
                }
                else if (adjustSearchAreaDirection == 2){
                    searchAreaWidth = (EB_S16)16;
                    searchAreaHeight = (EB_S16)9;
                }

                xSearchAreaOrigin = xSearchCenter - (searchAreaWidth >> 1);
                ySearchAreaOrigin = ySearchCenter - (searchAreaHeight >> 1);

            }
        }

        //Jing: Get tile index where this LCU belongs
        //Jing: TODO
        //Clean up this mess 
        if (sequenceControlSetPtr->staticConfig.unrestrictedMotionVector == 0 && (sequenceControlSetPtr->staticConfig.tileRowCount * sequenceControlSetPtr->staticConfig.tileColumnCount) > 1)
        {
            EB_U16 lcuTileIdx = pictureControlSetPtr->lcuEdgeInfoArray[lcuIndex].tileIndexInRaster;
            int tileStartX = pictureControlSetPtr->tileInfoArray[lcuTileIdx].tilePxlOriginX;
            int tileEndX   = pictureControlSetPtr->tileInfoArray[lcuTileIdx].tilePxlEndX;

            // Correct the left edge of the Search Area if it is not on the reference Picture
            xSearchAreaOrigin = ((originX + xSearchAreaOrigin) < tileStartX) ?
                tileStartX - originX :
                xSearchAreaOrigin;

            searchAreaWidth = ((originX + xSearchAreaOrigin) < tileStartX) ?
                searchAreaWidth - (tileStartX - (originX + xSearchAreaOrigin)) :
                searchAreaWidth;

            // Correct the right edge of the Search Area if its not on the reference Picture
            xSearchAreaOrigin = ((originX + xSearchAreaOrigin) > tileEndX - 1) ?
                xSearchAreaOrigin - ((originX + xSearchAreaOrigin) - (tileEndX - 1)) :
                xSearchAreaOrigin;

            searchAreaWidth = ((originX + xSearchAreaOrigin + searchAreaWidth) > tileEndX) ?
                MAX(1, searchAreaWidth - ((originX + xSearchAreaOrigin + searchAreaWidth) - tileEndX)) :
                searchAreaWidth;
        } else {
            // Correct the left edge of the Search Area if it is not on the reference Picture
            xSearchAreaOrigin = ((originX + xSearchAreaOrigin) < -padWidth) ?
                -padWidth - originX :
                xSearchAreaOrigin;

            searchAreaWidth = ((originX + xSearchAreaOrigin) < -padWidth) ?
                searchAreaWidth - (-padWidth - (originX + xSearchAreaOrigin)) :
                searchAreaWidth;

            // Correct the right edge of the Search Area if its not on the reference Picture
            xSearchAreaOrigin = ((originX + xSearchAreaOrigin) > pictureWidth - 1) ?
                xSearchAreaOrigin - ((originX + xSearchAreaOrigin) - (pictureWidth - 1)) :
                xSearchAreaOrigin;

            searchAreaWidth = ((originX + xSearchAreaOrigin + searchAreaWidth) > pictureWidth) ?
                MAX(1, searchAreaWidth - ((originX + xSearchAreaOrigin + searchAreaWidth) - pictureWidth)) :
                searchAreaWidth;
        }

        if (sequenceControlSetPtr->staticConfig.unrestrictedMotionVector == 0 && (sequenceControlSetPtr->staticConfig.tileRowCount * sequenceControlSetPtr->staticConfig.tileColumnCount) > 1)
        {
            EB_U16 lcuTileIdx = pictureControlSetPtr->lcuEdgeInfoArray[lcuIndex].tileIndexInRaster;
            int tileStartY = pictureControlSetPtr->tileInfoArray[lcuTileIdx].tilePxlOriginY;
            int tileEndY   = pictureControlSetPtr->tileInfoArray[lcuTileIdx].tilePxlEndY;

            // Correct the top edge of the Search Area if it is not on the reference Picture
            ySearchAreaOrigin = ((originY + ySearchAreaOrigin) < tileStartY) ?
                tileStartY - originY :
                ySearchAreaOrigin;

            searchAreaHeight = ((originY + ySearchAreaOrigin) < tileStartY) ?
                searchAreaHeight - (tileStartY - (originY + ySearchAreaOrigin)) :
                searchAreaHeight;

            // Correct the bottom edge of the Search Area if its not on the reference Picture
            ySearchAreaOrigin = ((originY + ySearchAreaOrigin) > tileEndY - 1) ?
                ySearchAreaOrigin - ((originY + ySearchAreaOrigin) - (tileEndY - 1)) :
                ySearchAreaOrigin;

            searchAreaHeight = (originY + ySearchAreaOrigin + searchAreaHeight > tileEndY) ?
                MAX(1, searchAreaHeight - ((originY + ySearchAreaOrigin + searchAreaHeight) - tileEndY)) :
                searchAreaHeight;
        } else {
            // Correct the top edge of the Search Area if it is not on the reference Picture
            ySearchAreaOrigin = ((originY + ySearchAreaOrigin) < -padHeight) ?
                -padHeight - originY :
                ySearchAreaOrigin;

            searchAreaHeight = ((originY + ySearchAreaOrigin) < -padHeight) ?
                searchAreaHeight - (-padHeight - (originY + ySearchAreaOrigin)) :
                searchAreaHeight;

            // Correct the bottom edge of the Search Area if its not on the reference Picture
            ySearchAreaOrigin = ((originY + ySearchAreaOrigin) > pictureHeight - 1) ?
                ySearchAreaOrigin - ((originY + ySearchAreaOrigin) - (pictureHeight - 1)) :
                ySearchAreaOrigin;

            searchAreaHeight = (originY + ySearchAreaOrigin + searchAreaHeight > pictureHeight) ?
                MAX(1, searchAreaHeight - ((originY + ySearchAreaOrigin + searchAreaHeight) - pictureHeight)) :
                searchAreaHeight;
        }

        contextPtr->xSearchAreaOrigin[listIndex][0] = xSearchAreaOrigin;
        contextPtr->ySearchAreaOrigin[listIndex][0] = ySearchAreaOrigin;

        xTopLeftSearchRegion    = (EB_S16)(refPicPtr->originX + lcuOriginX) - (ME_FILTER_TAP >> 1) + xSearchAreaOrigin;
        yTopLeftSearchRegion    = (EB_S16)(refPicPtr->originY + lcuOriginY) - (ME_FILTER_TAP >> 1) + ySearchAreaOrigin;
        searchRegionIndex       = (xTopLeftSearchRegion)+(yTopLeftSearchRegion)* refPicPtr->strideY;

        contextPtr->integerBufferPtr[listIndex][0] = &(refPicPtr->bufferY[searchRegionIndex]);
        contextPtr->interpolatedFullStride[listIndex][0] = refPicPtr->strideY;

        // Move to the top left of the search region
        xTopLeftSearchRegion = (EB_S16)(refPicPtr->originX + lcuOriginX) + xSearchAreaOrigin;
        yTopLeftSearchRegion = (EB_S16)(refPicPtr->originY + lcuOriginY) + ySearchAreaOrigin;
        searchRegionIndex = xTopLeftSearchRegion + yTopLeftSearchRegion * refPicPtr->strideY;

        {
            {

                InitializeBuffer_32bits_funcPtrArray[!!(ASM_TYPES & PREAVX2_MASK)](contextPtr->pLcuBestSad[listIndex][0], 21, 1, MAX_SAD_VALUE);
                contextPtr->pBestSad64x64 = &(contextPtr->pLcuBestSad[listIndex][0][ME_TIER_ZERO_PU_64x64]);
                contextPtr->pBestSad32x32 = &(contextPtr->pLcuBestSad[listIndex][0][ME_TIER_ZERO_PU_32x32_0]);
                contextPtr->pBestSad16x16 = &(contextPtr->pLcuBestSad[listIndex][0][ME_TIER_ZERO_PU_16x16_0]);
                contextPtr->pBestSad8x8 = &(contextPtr->pLcuBestSad[listIndex][0][ME_TIER_ZERO_PU_8x8_0]);

                contextPtr->pBestMV64x64 = &(contextPtr->pLcuBestMV[listIndex][0][ME_TIER_ZERO_PU_64x64]);
                contextPtr->pBestMV32x32 = &(contextPtr->pLcuBestMV[listIndex][0][ME_TIER_ZERO_PU_32x32_0]);
                contextPtr->pBestMV16x16 = &(contextPtr->pLcuBestMV[listIndex][0][ME_TIER_ZERO_PU_16x16_0]);
                contextPtr->pBestMV8x8 = &(contextPtr->pLcuBestMV[listIndex][0][ME_TIER_ZERO_PU_8x8_0]);

                contextPtr->pBestSsd64x64 = &(contextPtr->pLcuBestSsd[listIndex][0][ME_TIER_ZERO_PU_64x64]);
                contextPtr->pBestSsd32x32 = &(contextPtr->pLcuBestSsd[listIndex][0][ME_TIER_ZERO_PU_32x32_0]);
                contextPtr->pBestSsd16x16 = &(contextPtr->pLcuBestSsd[listIndex][0][ME_TIER_ZERO_PU_16x16_0]);
                contextPtr->pBestSsd8x8   = &(contextPtr->pLcuBestSsd[listIndex][0][ME_TIER_ZERO_PU_8x8_0]);

                FullPelSearch_LCU(
                        contextPtr,
                        listIndex,
                        xSearchAreaOrigin,
                        ySearchAreaOrigin,
                        searchAreaWidth,
                        searchAreaHeight
                        );

            }

            if (contextPtr->fractionalSearchModel == 0) {
                enableHalfPel32x32 = EB_TRUE;
                enableHalfPel16x16 = EB_TRUE;
                enableHalfPel8x8   = EB_TRUE;
                enableQuarterPel   = EB_TRUE;
            }
            else if (contextPtr->fractionalSearchModel == 1) {
                SuPelEnable(
                        contextPtr,
                        pictureControlSetPtr,
                        listIndex,
                        0,
                        &enableHalfPel32x32,
                        &enableHalfPel16x16,
                        &enableHalfPel8x8);
                enableQuarterPel = EB_TRUE;


            }
            else {
                enableHalfPel32x32 = EB_FALSE;
                enableHalfPel16x16 = EB_FALSE;
                enableHalfPel8x8   = EB_FALSE;
                enableQuarterPel   = EB_FALSE;

            }

            if (enableHalfPel32x32 || enableHalfPel16x16 || enableHalfPel8x8 || enableQuarterPel){

                // Move to the top left of the search region
                xTopLeftSearchRegion = (EB_S16)(refPicPtr->originX + lcuOriginX) + xSearchAreaOrigin;
                yTopLeftSearchRegion = (EB_S16)(refPicPtr->originY + lcuOriginY) + ySearchAreaOrigin;
                searchRegionIndex = xTopLeftSearchRegion + yTopLeftSearchRegion * refPicPtr->strideY;

                // Interpolate the search region for Half-Pel Refinements 
                // H - AVC Style
                InterpolateSearchRegionAVC(
                        contextPtr,
                        listIndex,
                        contextPtr->integerBufferPtr[listIndex][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * contextPtr->interpolatedFullStride[listIndex][0]),
                        contextPtr->interpolatedFullStride[listIndex][0],
                        (EB_U32)searchAreaWidth + (MAX_LCU_SIZE - 1),
                        (EB_U32)searchAreaHeight + (MAX_LCU_SIZE - 1));

                // Half-Pel Refinement [8 search positions]
                HalfPelSearch_LCU(
                        sequenceControlSetPtr,
                        contextPtr,
                        contextPtr->integerBufferPtr[listIndex][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * contextPtr->interpolatedFullStride[listIndex][0]),
                        contextPtr->interpolatedFullStride[listIndex][0],
                        &(contextPtr->posbBuffer[listIndex][0][(ME_FILTER_TAP >> 1) * contextPtr->interpolatedStride]),
                        &(contextPtr->poshBuffer[listIndex][0][1]),
                        &(contextPtr->posjBuffer[listIndex][0][0]),
                        xSearchAreaOrigin,
                        ySearchAreaOrigin,
                        pictureControlSetPtr->cu8x8Mode == CU_8x8_MODE_1,
                        enableHalfPel32x32,
                        enableHalfPel16x16 && pictureControlSetPtr->cu16x16Mode == CU_16x16_MODE_0,
                        enableHalfPel8x8);

                // Quarter-Pel Refinement [8 search positions]
                QuarterPelSearch_LCU(
                        contextPtr,
                        contextPtr->integerBufferPtr[listIndex][0] + (ME_FILTER_TAP >> 1) + ((ME_FILTER_TAP >> 1) * contextPtr->interpolatedFullStride[listIndex][0]),
                        contextPtr->interpolatedFullStride[listIndex][0],
                        &(contextPtr->posbBuffer[listIndex][0][(ME_FILTER_TAP >> 1) * contextPtr->interpolatedStride]),  //points to b position of the figure above
                        &(contextPtr->poshBuffer[listIndex][0][1]),                                                      //points to h position of the figure above
                        &(contextPtr->posjBuffer[listIndex][0][0]),                                                      //points to j position of the figure above
                        xSearchAreaOrigin,
                        ySearchAreaOrigin,
                        pictureControlSetPtr->cu8x8Mode == CU_8x8_MODE_1,
                        enableHalfPel32x32,
                        enableHalfPel16x16 && pictureControlSetPtr->cu16x16Mode == CU_16x16_MODE_0,
                        enableHalfPel8x8,
                        enableQuarterPel);
            }				
        }
	}

	// Bi-Prediction motion estimation loop
	for (puIndex = 0; puIndex < maxNumberOfPusPerLcu; ++puIndex) {
		candidateIndex = 0;
		EB_U32 nIdx = puIndex >  20   ?   tab8x8[puIndex-21]  + 21   :
			puIndex >   4   ?   tab32x32[puIndex-5] + 5    :  puIndex;                     

		for (listIndex = REF_LIST_0; listIndex <= numOfListToSearch; ++listIndex) {
				candidateIndex++;
		}

		totalMeCandidateIndex = candidateIndex;

		if (numOfListToSearch){
			EB_BOOL condition = (EB_BOOL)((pictureControlSetPtr->cu8x8Mode == CU_8x8_MODE_0 || puIndex < 21) && (pictureControlSetPtr->cu16x16Mode == CU_16x16_MODE_0 || puIndex < 5));

			if (condition) {
				BiPredictionSearch(
					contextPtr,
					puIndex,
					candidateIndex,
					1,
					1,
					&totalMeCandidateIndex,
					pictureControlSetPtr);
			}
		}

		MeCuResults_t * mePuResult = &pictureControlSetPtr->meResults[lcuIndex][puIndex];
		mePuResult->totalMeCandidateIndex = totalMeCandidateIndex;

		if (totalMeCandidateIndex == 3){

			EB_U32 L0Sad = contextPtr->pLcuBestSad[0][0][nIdx];
			EB_U32 L1Sad = contextPtr->pLcuBestSad[1][0][nIdx];
			EB_U32 biSad = contextPtr->pLcuBipredSad[nIdx];

			mePuResult->xMvL0 = _MVXT(contextPtr->pLcuBestMV[0][0][nIdx]);
			mePuResult->yMvL0 = _MVYT(contextPtr->pLcuBestMV[0][0][nIdx]);
			mePuResult->xMvL1 = _MVXT(contextPtr->pLcuBestMV[1][0][nIdx]);
			mePuResult->yMvL1 = _MVYT(contextPtr->pLcuBestMV[1][0][nIdx]);

			EB_U32 order = Sort3Elements(L0Sad, L1Sad, biSad);

			switch (order){
				// a = l0Sad, b= l1Sad, c= biSad
			case a_b_c:


				NSET_CAND(mePuResult, 0, L0Sad, UNI_PRED_LIST_0)
					NSET_CAND(mePuResult, 1, L1Sad, UNI_PRED_LIST_1)
					NSET_CAND(mePuResult, 2, biSad, BI_PRED)
					break;

			case a_c_b:

				NSET_CAND(mePuResult, 0, L0Sad, UNI_PRED_LIST_0)
					NSET_CAND(mePuResult, 1, biSad, BI_PRED)
					NSET_CAND(mePuResult, 2, L1Sad, UNI_PRED_LIST_1)
					break;

			case b_a_c:

				NSET_CAND(mePuResult, 0, L1Sad, UNI_PRED_LIST_1)
					NSET_CAND(mePuResult, 1, L0Sad, UNI_PRED_LIST_0)
					NSET_CAND(mePuResult, 2, biSad, BI_PRED)
					break;

			case b_c_a:

				NSET_CAND(mePuResult, 0, L1Sad, UNI_PRED_LIST_1)
					NSET_CAND(mePuResult, 1, biSad, BI_PRED)
					NSET_CAND(mePuResult, 2, L0Sad, UNI_PRED_LIST_0)
					break;

			case c_a_b:

				NSET_CAND(mePuResult, 0, biSad, BI_PRED)
					NSET_CAND(mePuResult, 1, L0Sad, UNI_PRED_LIST_0)
					NSET_CAND(mePuResult, 2, L1Sad, UNI_PRED_LIST_1)
					break;

			case c_b_a:

				NSET_CAND(mePuResult, 0, biSad, BI_PRED)
					NSET_CAND(mePuResult, 1, L1Sad, UNI_PRED_LIST_1)
					NSET_CAND(mePuResult, 2, L0Sad, UNI_PRED_LIST_0)
					break;

			default:
				SVT_LOG("Err in sorting");
				break;
			}
		} else if (totalMeCandidateIndex == 2){

			EB_U32 L0Sad = contextPtr->pLcuBestSad[0][0][nIdx];
			EB_U32 L1Sad = contextPtr->pLcuBestSad[1][0][nIdx];

			mePuResult->xMvL0 = _MVXT(contextPtr->pLcuBestMV[0][0][nIdx]);
			mePuResult->yMvL0 = _MVYT(contextPtr->pLcuBestMV[0][0][nIdx]);
			mePuResult->xMvL1 = _MVXT(contextPtr->pLcuBestMV[1][0][nIdx]);
			mePuResult->yMvL1 = _MVYT(contextPtr->pLcuBestMV[1][0][nIdx]);

			if (L0Sad <= L1Sad){
				NSET_CAND(mePuResult, 0, L0Sad, UNI_PRED_LIST_0)
					NSET_CAND(mePuResult, 1, L1Sad, UNI_PRED_LIST_1)
			}
			else{
				NSET_CAND(mePuResult, 0, L1Sad, UNI_PRED_LIST_1)
					NSET_CAND(mePuResult, 1, L0Sad, UNI_PRED_LIST_0)
			}
		} else{
			EB_U32 L0Sad = contextPtr->pLcuBestSad[0][0][nIdx];
			mePuResult->xMvL0 = _MVXT(contextPtr->pLcuBestMV[0][0][nIdx]);
			mePuResult->yMvL0 = _MVYT(contextPtr->pLcuBestMV[0][0][nIdx]);
			mePuResult->xMvL1 = _MVXT(contextPtr->pLcuBestMV[1][0][nIdx]);
			mePuResult->yMvL1 = _MVYT(contextPtr->pLcuBestMV[1][0][nIdx]);
			NSET_CAND(mePuResult, 0, L0Sad, UNI_PRED_LIST_0)
		}
	}

	if (sequenceControlSetPtr->staticConfig.rateControlMode){

		// Compute the sum of the distortion of all 16 16x16 (best) blocks in the LCU
		pictureControlSetPtr->rcMEdistortion[lcuIndex] = 0;
		for (i = 0; i < 16; i++) {
			pictureControlSetPtr->rcMEdistortion[lcuIndex] += pictureControlSetPtr->meResults[lcuIndex][5 + i].distortionDirection[0].distortion;
		}

	}


	return return_error;
}

/** IntraOpenLoopSearchTheseModesOutputBest

 */
void IntraOpenLoopSearchTheseModesOutputBest(
	EB_U32                     cuSize,
	MotionEstimationContext_t *contextPtr,
	EB_U8                     *src,
	EB_U32                     srcStride,
	EB_U8                      NumOfModesToTest,
	EB_U32                    *stage1ModesArray,
	EB_U32                    *sadArray,
	EB_U32                    *bestMode
	)
{
	EB_U32   candidateIndex;
	EB_U32   mode;
	EB_U32   bestSAD = 32 * 32 * 255;

	for (candidateIndex = 0; candidateIndex < NumOfModesToTest; candidateIndex++) {

		mode = stage1ModesArray[candidateIndex];

		// Intra Prediction
		IntraPredictionOpenLoop(
			cuSize,
			contextPtr,
			(EB_U32)mode);

		//Distortion
		sadArray[candidateIndex] = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][cuSize >> 3]( // Always SAD without weighting 
			src,
			srcStride,
			&(contextPtr->meContextPtr->lcuBuffer[0]),
			MAX_LCU_SIZE,
			cuSize,
			cuSize);

		//kepp track of best SAD
		if (sadArray[candidateIndex] < bestSAD){
			*bestMode = (EB_U32)mode;
			bestSAD = sadArray[candidateIndex];
		}

	}

}

void InjectIntraCandidatesBasedOnBestModeIslice(
    OisCandidate_t              *oisCuPtr,
    EB_U32	                    *stage1SadArray,
    EB_U32                       bestMode,
    EB_U8                       *count)
{
   
    oisCuPtr[(*count)].validDistortion = EB_TRUE;
    oisCuPtr[(*count)].distortion = stage1SadArray[0];
    oisCuPtr[(*count)++].intraMode = EB_INTRA_PLANAR;
    oisCuPtr[(*count)++].intraMode = EB_INTRA_DC;

    switch (bestMode){
        
    case EB_INTRA_PLANAR:
    case EB_INTRA_DC:

        break;
    case EB_INTRA_MODE_2:

        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_2;
        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_4;
        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_6;

        break;

    case EB_INTRA_HORIZONTAL:

        oisCuPtr[(*count)++].intraMode = EB_INTRA_HORIZONTAL;
        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_6;
        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_14;

        break;

    case EB_INTRA_MODE_18:

        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_18;
        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_14;
        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_22;

        break;
    case EB_INTRA_VERTICAL:

        oisCuPtr[(*count)++].intraMode = EB_INTRA_VERTICAL;
        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_22;
        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_30;

        break;

    case EB_INTRA_MODE_34:
    default:

        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_34;
        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_32;
        oisCuPtr[(*count)++].intraMode = EB_INTRA_MODE_30;

        break;
    }
}

void InjectIntraCandidatesBasedOnBestMode(
    MotionEstimationContext_t   *contextPtr,
    OisCandidate_t              *oisCuPtr,
    EB_U32	                    *stage1SadArray,
    EB_U32                       bestMode)
{
    EB_U32 count = 0;

    switch (bestMode){

    case EB_INTRA_MODE_2:
        oisCuPtr[count].distortion = stage1SadArray[2];


        if (contextPtr->setBestOisDistortionToValid){
            oisCuPtr[count].validDistortion = EB_TRUE;
        }
        else{
            oisCuPtr[count].validDistortion = EB_FALSE;
        }

        oisCuPtr[count++].intraMode = EB_INTRA_MODE_2;
        oisCuPtr[count++].intraMode = EB_INTRA_DC;
        oisCuPtr[count++].intraMode = EB_INTRA_PLANAR;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_3;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_4;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_5;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_7;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_8;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_9;

        break;

    case EB_INTRA_HORIZONTAL:

        oisCuPtr[count].distortion = stage1SadArray[0];

        if (contextPtr->setBestOisDistortionToValid){
            oisCuPtr[count].validDistortion = EB_TRUE;
        }
        else{
            oisCuPtr[count].validDistortion = EB_FALSE;
        }

        oisCuPtr[count++].intraMode = EB_INTRA_HORIZONTAL;
        oisCuPtr[count++].intraMode = EB_INTRA_DC;
        oisCuPtr[count++].intraMode = EB_INTRA_PLANAR;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_9;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_11;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_8;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_12;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_7;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_13;


        break;

    case EB_INTRA_MODE_18:

        oisCuPtr[count].distortion = stage1SadArray[3];

        if (contextPtr->setBestOisDistortionToValid)
            oisCuPtr[count].validDistortion = EB_TRUE;
        else
            oisCuPtr[count].validDistortion = EB_FALSE;


        oisCuPtr[count++].intraMode = EB_INTRA_MODE_18;
        oisCuPtr[count++].intraMode = EB_INTRA_DC;
        oisCuPtr[count++].intraMode = EB_INTRA_PLANAR;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_17;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_19;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_16;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_20;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_15;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_21;

        break;
    case EB_INTRA_VERTICAL:

        oisCuPtr[count].distortion = stage1SadArray[1];

        if (contextPtr->setBestOisDistortionToValid)
            oisCuPtr[count].validDistortion = EB_TRUE;
        else
            oisCuPtr[count].validDistortion = EB_FALSE;

        oisCuPtr[count++].intraMode = EB_INTRA_VERTICAL;
        oisCuPtr[count++].intraMode = EB_INTRA_DC;
        oisCuPtr[count++].intraMode = EB_INTRA_PLANAR;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_25;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_27;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_24;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_28;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_23;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_29;



        break;

    case EB_INTRA_MODE_34:

        oisCuPtr[count].distortion = stage1SadArray[4];

        if (contextPtr->setBestOisDistortionToValid)
            oisCuPtr[count].validDistortion = EB_TRUE;
        else
            oisCuPtr[count].validDistortion = EB_FALSE;

        oisCuPtr[count++].intraMode = EB_INTRA_MODE_34;
        oisCuPtr[count++].intraMode = EB_INTRA_DC;
        oisCuPtr[count++].intraMode = EB_INTRA_PLANAR;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_33;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_32;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_29;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_31;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_27;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_28;

        break;

    case EB_INTRA_MODE_6:

        oisCuPtr[count].distortion = stage1SadArray[5];

        if (contextPtr->setBestOisDistortionToValid)
            oisCuPtr[count].validDistortion = EB_TRUE;
        else
            oisCuPtr[count].validDistortion = EB_FALSE;


        oisCuPtr[count++].intraMode = EB_INTRA_MODE_6;
        oisCuPtr[count++].intraMode = EB_INTRA_DC;
        oisCuPtr[count++].intraMode = EB_INTRA_PLANAR;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_7;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_5;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_4;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_8;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_3;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_9;

        break;

    case EB_INTRA_MODE_14:

        oisCuPtr[count].distortion = stage1SadArray[6];

        if (contextPtr->setBestOisDistortionToValid)
            oisCuPtr[count].validDistortion = EB_TRUE;
        else
            oisCuPtr[count].validDistortion = EB_FALSE;


        oisCuPtr[count++].intraMode = EB_INTRA_MODE_14;
        oisCuPtr[count++].intraMode = EB_INTRA_DC;
        oisCuPtr[count++].intraMode = EB_INTRA_PLANAR;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_13;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_15;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_12;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_16;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_11;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_17;

        break;

    case EB_INTRA_MODE_22:

        oisCuPtr[count].distortion = stage1SadArray[7];

        if (contextPtr->setBestOisDistortionToValid)
            oisCuPtr[count].validDistortion = EB_TRUE;
        else
            oisCuPtr[count].validDistortion = EB_FALSE;

        oisCuPtr[count++].intraMode = EB_INTRA_MODE_22;
        oisCuPtr[count++].intraMode = EB_INTRA_DC;
        oisCuPtr[count++].intraMode = EB_INTRA_PLANAR;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_21;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_23;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_20;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_24;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_19;
        oisCuPtr[count++].intraMode = EB_INTRA_MODE_25;

        break;

    case EB_INTRA_MODE_30:
    default:

        oisCuPtr[count].distortion = stage1SadArray[8];

        if (contextPtr->setBestOisDistortionToValid){
            oisCuPtr[count].validDistortion = EB_TRUE;
        }
        else{
            oisCuPtr[count].validDistortion = EB_FALSE;
        }

			oisCuPtr[count++].intraMode = EB_INTRA_MODE_30;
			oisCuPtr[count++].intraMode = EB_INTRA_DC;
			oisCuPtr[count++].intraMode = EB_INTRA_PLANAR;
			oisCuPtr[count++].intraMode = EB_INTRA_MODE_29;
			oisCuPtr[count++].intraMode = EB_INTRA_MODE_31;
			oisCuPtr[count++].intraMode = EB_INTRA_MODE_28;
			oisCuPtr[count++].intraMode = EB_INTRA_MODE_32;
			oisCuPtr[count++].intraMode = EB_INTRA_MODE_27;
			oisCuPtr[count++].intraMode = EB_INTRA_MODE_33;

			break;


		}
}

EB_S32 GetInterIntraSadDistance(
    MotionEstimationContext_t   *contextPtr,
    EbPictureBufferDesc_t       *inputPtr,
    EB_U32                       cuSize,
    EB_U32                      *stage1SadArray,
    EB_U32                       meSad,
    EB_U32                       cuOriginX,
    EB_U32                       cuOriginY
    )

{
    EB_S32   sadDiff = 0;
    EB_U8   *src = &(inputPtr->bufferY[(inputPtr->originY + cuOriginY) * inputPtr->strideY + (inputPtr->originX + cuOriginX)]);
    // Compute Prediction & SAD for Intra Planer

    // Intra Prediction
    IntraPredictionOpenLoop(
        cuSize,
        contextPtr,
        (EB_U32)1);

    //Distortion
	stage1SadArray[0] = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][cuSize >> 3]( // Always SAD without weighting 
        src,
        inputPtr->strideY,
        &(contextPtr->meContextPtr->lcuBuffer[0]),
        MAX_LCU_SIZE,
        cuSize,
        cuSize);

    // Perform Inter-Intra Comparision
    sadDiff = (EB_S32)(meSad - stage1SadArray[0]) * 100;

    return (stage1SadArray[0] ? (sadDiff / (EB_S32)stage1SadArray[0]) : 0);

}

EB_IOS_POINT GetOisPoint(
    EB_U8           oisThSet,
    EB_U32          meSad,
    EB_U8           temporalLayerIndex,
    EB_S32          interIntraSadDistance,
    EB_U32         *stage1SadArray)

{
    EB_IOS_POINT            oisPoint = OIS_VERY_COMPLEX_MODE;
    // Intra points switcher
    if (stage1SadArray[0] == 0 || meSad == 0 || interIntraSadDistance <= OisPointTh[oisThSet][temporalLayerIndex][0]){
        oisPoint = OIS_VERY_FAST_MODE;
    }
    else
    {
        if (interIntraSadDistance <= OisPointTh[oisThSet][temporalLayerIndex][1]){
            oisPoint = OIS_FAST_MODE;
        }
        else if (interIntraSadDistance <= OisPointTh[oisThSet][temporalLayerIndex][2]){
            oisPoint = OIS_MEDUIM_MODE;
        }
        else if (interIntraSadDistance <= OisPointTh[oisThSet][temporalLayerIndex][3]){
            oisPoint = OIS_COMPLEX_MODE;
        }
    }

    return oisPoint;
}

EB_ERRORTYPE SortOisCandidateOpenLoop(
    OisCandidate_t                 *oisCandidate)   // input OIS candidate array
{
    EB_ERRORTYPE                return_error = EB_ErrorNone;
    EB_U32   index1;
    EB_U32   index2;
    EB_U32   intraCandidateMode;
    EB_U64   intraSadDistortion;

    for (index1 = 0; index1 < MAX_OIS_2; ++index1)
    {
        for (index2 = index1; index2 < MAX_OIS_2; ++index2)
        {
            if (oisCandidate[index1].distortion > oisCandidate[index2].distortion)
            {
                intraCandidateMode = oisCandidate[index1].intraMode;
                oisCandidate[index1].intraMode = oisCandidate[index2].intraMode;
                oisCandidate[index2].intraMode = intraCandidateMode;

                intraSadDistortion = oisCandidate[index1].distortion;
                oisCandidate[index1].distortion = oisCandidate[index2].distortion;
                oisCandidate[index2].distortion = (EB_U32)intraSadDistortion;
            }
        }
    }
    return return_error;
}

EB_ERRORTYPE SortIntraModesOpenLoop(
	PictureParentControlSet_t   *pictureControlSetPtr,          // input parameter, pointer to the current lcu 
	EB_U32                       lcuIndex,                      // input parameter, lcu Index
	EB_U32                       cuIndex,                       // input parameter, cu index
	EB_U32                       sadDistortion,                 // input parameter, SAD 
	EB_U32           openLoopIntraCandidateIndex)   // input parameter, intra mode
{
	EB_ERRORTYPE                return_error = EB_ErrorNone;

	EB_U32  worstIntraCandidateIndex;
	EB_U32  bestIntraCandidateIndex;
	EB_U64	worstSadDistortion; // could be EB_U32


	OisCu32Cu16Results_t	        *oisCu32Cu16ResultsPtr = pictureControlSetPtr->oisCu32Cu16Results[lcuIndex];
	OisCu8Results_t   	            *oisCu8ResultsPtr = pictureControlSetPtr->oisCu8Results[lcuIndex];

	OisCandidate_t * oisCuPtr = cuIndex < RASTER_SCAN_CU_INDEX_8x8_0 ?
		oisCu32Cu16ResultsPtr->sortedOisCandidate[cuIndex] : oisCu8ResultsPtr->sortedOisCandidate[cuIndex - RASTER_SCAN_CU_INDEX_8x8_0];


	// Set the best MAX_OIS_2 modes
	if (openLoopIntraCandidateIndex < MAX_OIS_2) {

		oisCuPtr[openLoopIntraCandidateIndex].distortion = sadDistortion;
		oisCuPtr[openLoopIntraCandidateIndex].intraMode = openLoopIntraCandidateIndex;

		// Get a copy of the OIS SAD and mode - This array will be sorted         
		// This array is not used in RC and DeltaQP
		oisCuPtr[openLoopIntraCandidateIndex].distortion = sadDistortion;
		oisCuPtr[openLoopIntraCandidateIndex].intraMode = openLoopIntraCandidateIndex;

	}
	else {
		EB_U32 intraIndex;
		// Determine max SAD distortion 
		worstSadDistortion = oisCuPtr[0].distortion;
		worstIntraCandidateIndex = EB_INTRA_PLANAR;

		for (intraIndex = 1; intraIndex < MAX_OIS_2; ++intraIndex) {
			bestIntraCandidateIndex = (EB_U32)intraIndex;
			if (oisCuPtr[bestIntraCandidateIndex].distortion > worstSadDistortion) {

				worstSadDistortion = oisCuPtr[bestIntraCandidateIndex].distortion;
				worstIntraCandidateIndex = bestIntraCandidateIndex;
			}
		}

		// Update the best MAX_OIS_2 modes
		if (sadDistortion < worstSadDistortion) {

			oisCuPtr[worstIntraCandidateIndex].distortion = sadDistortion;
			oisCuPtr[worstIntraCandidateIndex].intraMode = openLoopIntraCandidateIndex;
		}
	}

	return return_error;
}

EB_U32 UpdateNeighborDcIntraPred(
	MotionEstimationContext_t       *contextPtr,
	EbPictureBufferDesc_t           *inputPtr,
	EB_U32                           cuOriginX,
	EB_U32                           cuOriginY,
	EB_U32                           cuSize)
{
	EB_U32 distortion;
	//	SVT_LOG("cuSize=%i  x=%i  y=%i  rasterScanCuIndex=%i   mdScanCuIndex=%i \n", cuSize, RASTER_SCAN_CU_X[rasterScanCuIndex], RASTER_SCAN_CU_Y[rasterScanCuIndex],rasterScanCuIndex, mdScanCuIndex );
	// Fill Neighbor Arrays
	UpdateNeighborSamplesArrayOpenLoop(
		contextPtr->intraRefPtr,
		inputPtr,
		inputPtr->strideY,
		cuOriginX,
		cuOriginY,
		cuSize);

	// Intra Prediction
	IntraPredictionOpenLoop(
		cuSize,
		contextPtr,
		(EB_U32)INTRA_DC_MODE);

	distortion = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][cuSize >> 3]( // Always SAD without weighting 
		&(inputPtr->bufferY[(inputPtr->originY + cuOriginY) * inputPtr->strideY + (inputPtr->originX + cuOriginX)]),
		inputPtr->strideY,
		&(contextPtr->meContextPtr->lcuBuffer[0]),
		MAX_LCU_SIZE,
		cuSize,
		cuSize);
	return(distortion);
}

EB_ERRORTYPE OpenLoopIntraDC(
	PictureParentControlSet_t   *pictureControlSetPtr,
	EB_U32                       lcuIndex,
	MotionEstimationContext_t   *contextPtr,
	EbPictureBufferDesc_t       *inputPtr,
	EB_U32						 cuOriginX,
	EB_U32						 cuOriginY,
	EB_U32						 rasterScanCuIndex)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	OisCu32Cu16Results_t	        *oisCu32Cu16ResultsPtr = pictureControlSetPtr->oisCu32Cu16Results[lcuIndex];
	OisCu8Results_t   	            *oisCu8ResultsPtr = pictureControlSetPtr->oisCu8Results[lcuIndex];
	OisCandidate_t *oisCuPtr = rasterScanCuIndex < RASTER_SCAN_CU_INDEX_8x8_0 ?
		oisCu32Cu16ResultsPtr->sortedOisCandidate[rasterScanCuIndex] : oisCu8ResultsPtr->sortedOisCandidate[rasterScanCuIndex - RASTER_SCAN_CU_INDEX_8x8_0];
	EB_U32 cuSize = RASTER_SCAN_CU_SIZE[rasterScanCuIndex];

	if ((cuSize == 32) || (cuSize == 16) || (cuSize == 8))
	{
        if (!!(ASM_TYPES & AVX2_MASK))
        {
            oisCuPtr[0].distortion = (EB_U32)UpdateNeighborDcIntraPred_AVX2_INTRIN(
                contextPtr->intraRefPtr->yIntraReferenceArrayReverse,
                inputPtr->height,
                inputPtr->strideY,
                inputPtr->bufferY,
                inputPtr->originY,
                inputPtr->originX,
                cuOriginX,
                cuOriginY,
                cuSize);
        }
        else
        {
            oisCuPtr[0].distortion = (EB_U32)UpdateNeighborDcIntraPred(
                contextPtr,
                inputPtr,
                cuOriginX,
                cuOriginY,
                cuSize);
        }

		oisCuPtr[0].intraMode = INTRA_DC_MODE;
	}
	else{

		oisCuPtr[0].distortion = UpdateNeighborDcIntraPred(
			contextPtr,
			inputPtr,
			cuOriginX,
			cuOriginY,
			cuSize);

		oisCuPtr[0].intraMode = INTRA_DC_MODE;
	}

    oisCuPtr[0].validDistortion = 1;

	if (rasterScanCuIndex < RASTER_SCAN_CU_INDEX_8x8_0)
		oisCu32Cu16ResultsPtr->totalIntraLumaMode[rasterScanCuIndex] = 1;
	else
		oisCu8ResultsPtr->totalIntraLumaMode[rasterScanCuIndex - RASTER_SCAN_CU_INDEX_8x8_0] = 1;


	return return_error;
}

EB_U8 GetNumOfIntraModesFromOisPoint(
    PictureParentControlSet_t   *pictureControlSetPtr,
    EB_U32                       meSad,
    EB_U32                       oisDcSad   
)
{   
  
    EB_S32 sadDiff = (EB_S32)(meSad - oisDcSad) * 100;
    EB_S32 interIntraSadDistance = oisDcSad ? (sadDiff / (EB_S32)oisDcSad) : 0;
     
    EB_U8 oisPoint = GetOisPoint(
        0,
        meSad,
        pictureControlSetPtr->temporalLayerIndex,
        interIntraSadDistance,
        &oisDcSad);
         

  return  numberOfOisModePoints[oisPoint];   

}

EB_ERRORTYPE OpenLoopIntraSearchLcu(
	PictureParentControlSet_t   *pictureControlSetPtr,
	EB_U32                       lcuIndex,
	MotionEstimationContext_t   *contextPtr,
	EbPictureBufferDesc_t       *inputPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	SequenceControlSet_t    *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	EB_U32                   rasterScanCuIndex;
	EB_U32                   meSad = 0xFFFFFFFF;
	EB_U8                    stage1NumOfModes = 0;
	EB_S32                   interIntraSadDistance = 0;
	EB_U32	                 cuOriginX;
	EB_U32                   cuOriginY;
	EB_U32                   cuSize;
	EB_U32                   cuDepth;
	EB_U32	                 stage1SadArray[11] = { 0 };
	EB_U32	                 openLoopIntraCandidateIndex;
	EB_U32	                 sadDistortion;
	EB_U32	                 intraCandidateIndex;
	EB_U32                   bestMode = EB_INTRA_PLANAR;
	
	LcuParams_t             *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];


	OisCu32Cu16Results_t    *oisCu32Cu16ResultsPtr = pictureControlSetPtr->oisCu32Cu16Results[lcuIndex];
	OisCu8Results_t   	    *oisCu8ResultsPtr = pictureControlSetPtr->oisCu8Results[lcuIndex];

	if (pictureControlSetPtr->sliceType == EB_I_PICTURE){

		for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_32x32_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_8x8_63; rasterScanCuIndex++) {

			cuSize = RASTER_SCAN_CU_SIZE[rasterScanCuIndex];

			OisCandidate_t *oisCuPtr = rasterScanCuIndex < RASTER_SCAN_CU_INDEX_8x8_0 ?
				oisCu32Cu16ResultsPtr->sortedOisCandidate[rasterScanCuIndex] : oisCu8ResultsPtr->sortedOisCandidate[rasterScanCuIndex - RASTER_SCAN_CU_INDEX_8x8_0];

            // Initialize valid distortion flag 
            for (intraCandidateIndex = 0; intraCandidateIndex < MAX_OIS_0; intraCandidateIndex++) {
                oisCuPtr[intraCandidateIndex].validDistortion = EB_FALSE;
            }

			if (lcuParams->rasterScanCuValidity[rasterScanCuIndex]) {

				cuOriginX = lcuParams->originX + RASTER_SCAN_CU_X[rasterScanCuIndex];
				cuOriginY = lcuParams->originY + RASTER_SCAN_CU_Y[rasterScanCuIndex];
				// Fill Neighbor Arrays
				UpdateNeighborSamplesArrayOpenLoop(
					contextPtr->intraRefPtr,
					inputPtr,
					inputPtr->strideY,
					cuOriginX,
					cuOriginY,
					cuSize);

				if (cuSize == 32) {

					// Intra Prediction
					IntraPredictionOpenLoop(
						cuSize,
						contextPtr,
						(EB_U32)EB_INTRA_PLANAR);

					//Distortion
					oisCuPtr[0].distortion = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][cuSize >> 3]( // Always SAD without weighting 
						&(inputPtr->bufferY[(inputPtr->originY + cuOriginY) * inputPtr->strideY + (inputPtr->originX + cuOriginX)]),
						inputPtr->strideY,
						&(contextPtr->meContextPtr->lcuBuffer[0]),
						MAX_LCU_SIZE,
						cuSize,
						cuSize);


					oisCuPtr[0].intraMode = EB_INTRA_PLANAR;
					oisCuPtr[0].validDistortion = EB_TRUE;

				}
				else{
					EB_U8 count = 0;
					IntraOpenLoopSearchTheseModesOutputBest(cuSize,
						contextPtr,
						&(inputPtr->bufferY[(inputPtr->originY + cuOriginY) * inputPtr->strideY + (inputPtr->originX + cuOriginX)]),
						inputPtr->strideY,
                        MAX_OIS_0, // PL ,DC ,  2 , H , 18, V , 34
						iSliceModesArray,
						stage1SadArray,
						&bestMode);

					InjectIntraCandidatesBasedOnBestModeIslice(
						oisCuPtr,
						stage1SadArray,
						bestMode,
						&count);


					if (rasterScanCuIndex < RASTER_SCAN_CU_INDEX_8x8_0)
						oisCu32Cu16ResultsPtr->totalIntraLumaMode[rasterScanCuIndex] = count;
					else
						oisCu8ResultsPtr->totalIntraLumaMode[rasterScanCuIndex - RASTER_SCAN_CU_INDEX_8x8_0] = count;


				}
			}
		}
	}
	else{

        EB_BOOL skipOis8x8 = pictureControlSetPtr->skipOis8x8;

		EB_U32 maxCuIndex = (skipOis8x8 || pictureControlSetPtr->cu8x8Mode == CU_8x8_MODE_1) ? RASTER_SCAN_CU_INDEX_16x16_15 : RASTER_SCAN_CU_INDEX_8x8_63;

		for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_32x32_0; rasterScanCuIndex <= maxCuIndex; rasterScanCuIndex++) {
	
			EB_IOS_POINT            oisPoint = OIS_VERY_COMPLEX_MODE;

			cuSize = RASTER_SCAN_CU_SIZE[rasterScanCuIndex];
			cuDepth = RASTER_SCAN_CU_DEPTH[rasterScanCuIndex];

			OisCandidate_t * oisCuPtr = rasterScanCuIndex < RASTER_SCAN_CU_INDEX_8x8_0 ?
				oisCu32Cu16ResultsPtr->sortedOisCandidate[rasterScanCuIndex] : oisCu8ResultsPtr->sortedOisCandidate[rasterScanCuIndex - RASTER_SCAN_CU_INDEX_8x8_0];

			if (lcuParams->rasterScanCuValidity[rasterScanCuIndex]) {
				cuOriginX = lcuParams->originX + RASTER_SCAN_CU_X[rasterScanCuIndex];
				cuOriginY = lcuParams->originY + RASTER_SCAN_CU_Y[rasterScanCuIndex];
				if (pictureControlSetPtr->limitOisToDcModeFlag == EB_FALSE){

					// Fill Neighbor Arrays
					UpdateNeighborSamplesArrayOpenLoop(
						contextPtr->intraRefPtr,
						inputPtr,
						inputPtr->strideY,
						cuOriginX,
						cuOriginY,
						cuSize);
				}

				if (contextPtr->oisKernelLevel) {

                    // Initialize valid distortion flag 
					for (intraCandidateIndex = 0; intraCandidateIndex < MAX_OIS_2; intraCandidateIndex++) {
						oisCuPtr[intraCandidateIndex].validDistortion = EB_FALSE;
					}

					EB_U32 oisIndex;
					for (oisIndex = 0; oisIndex < MAX_INTRA_MODES; ++oisIndex) {

						openLoopIntraCandidateIndex = (EB_U32)oisIndex;
						// Intra Prediction
						IntraPredictionOpenLoop(
							cuSize,
							contextPtr,
							openLoopIntraCandidateIndex);

						//Distortion
						sadDistortion = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][cuSize >> 3](
							&(inputPtr->bufferY[(inputPtr->originY + cuOriginY) * inputPtr->strideY + (inputPtr->originX + cuOriginX)]),
							inputPtr->strideY,
							&(contextPtr->meContextPtr->lcuBuffer[0]),
							MAX_LCU_SIZE,
							cuSize,
							cuSize);

						// BEST MAX_OIS_2
						SortIntraModesOpenLoop(
							pictureControlSetPtr,
							lcuIndex,
							rasterScanCuIndex,
							sadDistortion,
							openLoopIntraCandidateIndex);
					}

					// The sorted array is not used in RC and DeltaQP
					SortOisCandidateOpenLoop(oisCuPtr);

					if (rasterScanCuIndex < RASTER_SCAN_CU_INDEX_8x8_0)
						oisCu32Cu16ResultsPtr->totalIntraLumaMode[rasterScanCuIndex] = MAX_OIS_2;
					else
						oisCu8ResultsPtr->totalIntraLumaMode[rasterScanCuIndex - RASTER_SCAN_CU_INDEX_8x8_0] = MAX_OIS_2;

				}
				else
				{

                    // Initialize valid distortion flag 
                    for (intraCandidateIndex = 0; intraCandidateIndex < MAX_OIS_1; intraCandidateIndex++) {
                        oisCuPtr[intraCandidateIndex].validDistortion = EB_FALSE;
                    }

					if (pictureControlSetPtr->limitOisToDcModeFlag == EB_TRUE){

						OpenLoopIntraDC(
							pictureControlSetPtr,
							lcuIndex,
							contextPtr,
							inputPtr,
							cuOriginX,
							cuOriginY,
							rasterScanCuIndex);
					}
					else{
						// Set ME distortion

						meSad = pictureControlSetPtr->meResults[lcuIndex][rasterScanCuIndex].distortionDirection[0].distortion;

						interIntraSadDistance = GetInterIntraSadDistance(
							contextPtr,
							inputPtr,
							cuSize,
							stage1SadArray,
							meSad,
							cuOriginX,
							cuOriginY);

						oisPoint = GetOisPoint(
                            contextPtr->oisThSet,
							meSad,
							pictureControlSetPtr->temporalLayerIndex,
							interIntraSadDistance,
							stage1SadArray);



						if (oisPoint == OIS_VERY_FAST_MODE){

							oisCuPtr[0].intraMode = INTRA_DC_MODE;
							oisCuPtr[0].distortion = stage1SadArray[0];


							if (rasterScanCuIndex < RASTER_SCAN_CU_INDEX_8x8_0)
								oisCu32Cu16ResultsPtr->totalIntraLumaMode[rasterScanCuIndex] = 1;
							else
								oisCu8ResultsPtr->totalIntraLumaMode[rasterScanCuIndex - RASTER_SCAN_CU_INDEX_8x8_0] = 1;


						}
						else{
							stage1NumOfModes = numberOfOisModePoints[oisPoint];

							IntraOpenLoopSearchTheseModesOutputBest(cuSize,
								contextPtr,
								&(inputPtr->bufferY[(inputPtr->originY + cuOriginY) * inputPtr->strideY + (inputPtr->originX + cuOriginX)]),
								inputPtr->strideY,
								stage1NumOfModes,
								stage1ModesArray,
								stage1SadArray,
								&bestMode);


							InjectIntraCandidatesBasedOnBestMode(
                                contextPtr,
								oisCuPtr,
								stage1SadArray,
								bestMode);


							if (rasterScanCuIndex < RASTER_SCAN_CU_INDEX_8x8_0)
								oisCu32Cu16ResultsPtr->totalIntraLumaMode[rasterScanCuIndex] = totalIntraLumaMode[oisPoint][cuDepth];
							else
								oisCu8ResultsPtr->totalIntraLumaMode[rasterScanCuIndex - RASTER_SCAN_CU_INDEX_8x8_0] = totalIntraLumaMode[oisPoint][cuDepth];

						}
					}
				}

			}
		}
	}

	return return_error;
}





