/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbEntropyCoding.h"
#include "EbEntropyCodingUtil.h"
#include "EbUtility.h"
#include "EbCodingUnit.h"
#include "EbSequenceControlSet.h"
#include "EbTransformUnit.h"
#include "EbDeblockingFilter.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

// SSE2 Intrinsics 
#include "emmintrin.h"

/*****************************
* Defines
*****************************/
#define RPS_DEFAULT_VALUE   ~0u
#define ONE_BIT             32768 // Fixed-point representation of 1.0 bits

/*****************************
* Enums
*****************************/

enum COEFF_SCAN_TYPE
{
	SCAN_ZIGZAG = 0,      // zigzag scan
	SCAN_HOR,             // first scan is horizontal 
	SCAN_VER,             // first scan is vertical 
	SCAN_DIAG             // up-right diagonal scan
};


/*****************************
* Static consts
*****************************/
//LUT used for LPSxRange calculation 

static EB_U64 maxLumaPictureSize[TOTAL_LEVEL_COUNT] =
{ 36864U, 122880U, 245760U, 552960U, 983040U, 2228224U, 2228224U, 8912896U, 8912896U, 8912896U, 35651584U, 35651584U, 35651584U };

static EB_U64 maxLumaSampleRate[TOTAL_LEVEL_COUNT] =
{ 552960U, 3686400U, 7372800U, 16588800U, 33177600U, 66846720U, 133693440U, 267386880U, 534773760U, 1069547520U, 1069547520U, 2139095040U, 4278190080U };

static EB_U32 mainTierMaxBitrate[TOTAL_LEVEL_COUNT] =
{ 128000, 1500000, 3000000, 6000000, 10000000, 12000000, 20000000, 25000000, 40000000, 60000000, 60000000, 120000000, 240000000 };

static EB_U32 highTierMaxBitrate[TOTAL_LEVEL_COUNT] =
{ 128000, 1500000, 3000000, 6000000, 10000000, 30000000, 50000000, 100000000, 160000000, 240000000, 240000000, 480000000, 800000000 };

static EB_U32 mainTierMaxCPBsize[TOTAL_LEVEL_COUNT] =
{ 350000, 150000, 3000000, 6000000, 10000000, 12000000, 20000000, 25000000, 40000000, 60000000, 60000000, 120000000, 240000000 };

static EB_U32 highTierMaxCPBsize[TOTAL_LEVEL_COUNT] =
{ 350000, 1500000, 3000000, 6000000, 10000000, 30000000, 50000000, 100000000, 160000000, 240000000, 240000000, 480000000, 800000000 };
static EB_U32 maxTileColumn[TOTAL_LEVEL_COUNT] = { 1, 1, 1, 2, 3, 5, 5, 10, 10, 10, 20, 20, 20 };
static EB_U32 maxTileRow[TOTAL_LEVEL_COUNT]    = { 1, 1, 1, 2, 3, 5, 5, 11, 11, 11, 22, 22, 22 };

/************************************************
* Bac Encoder Context:Finish Function
* It is the Finish function called in finish CU
*
* input: bacEncContext pointer
*
************************************************/
static void BacEncContextFinish(BacEncContext_t *bacEncContextPtr)
{
	EB_U32 carry;

	carry = bacEncContextPtr->intervalLowValue >> (32 - bacEncContextPtr->bitsRemainingNum);
	bacEncContextPtr->intervalLowValue &= 0xffffffffu >> bacEncContextPtr->bitsRemainingNum;
    if (carry > 0 || bacEncContextPtr->tempBufferedBytesNum > 0) {
        OutputBitstreamWriteByte(&(bacEncContextPtr->m_pcTComBitIf), (bacEncContextPtr->tempBufferedByte + carry) & 0xff);
    }

	while (bacEncContextPtr->tempBufferedBytesNum > 1)
	{
		OutputBitstreamWriteByte(&(bacEncContextPtr->m_pcTComBitIf), (0xff + carry) & 0xff);

		bacEncContextPtr->tempBufferedBytesNum--;
	}

	OutputBitstreamWrite(
		&(bacEncContextPtr->m_pcTComBitIf),
		bacEncContextPtr->intervalLowValue >> 8,
		24 - bacEncContextPtr->bitsRemainingNum);

}

/************************************************
* CABAC Encoder Constructor
************************************************/
void CabacCtor(
	CabacEncodeContext_t *cabacEncContextPtr)
{
	EB_MEMSET(cabacEncContextPtr, 0, sizeof(CabacEncodeContext_t));

	return;
}

/************************************************
* Bac Encoder Context Reset Function
************************************************/
inline static EB_ERRORTYPE ResetBacEnc(BacEncContext_t *bacEncContextPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	bacEncContextPtr->intervalLowValue = 0;
	bacEncContextPtr->intervalRangeValue = 510;
	bacEncContextPtr->bitsRemainingNum = 23;
	bacEncContextPtr->tempBufferedBytesNum = 0;
	bacEncContextPtr->tempBufferedByte = 0xff;

	return return_error;
}

/*********************************************************************
* EncodeSplitFlag
*   Encodes the split flag
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
static void EncodeSplitFlag(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	EB_U32                  cuDepth,
	EB_U32                  maxCuDepth,
	EB_BOOL                 splitFlag,
	EB_U32                  cuOriginX,
	EB_U32                  cuOriginY,
	NeighborArrayUnit_t    *modeTypeNeighborArray,
	NeighborArrayUnit_t    *leafDepthNeighborArray)
{
	EB_U32 modeTypeLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		modeTypeNeighborArray,
		cuOriginY);
	EB_U32 modeTypeTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		modeTypeNeighborArray,
		cuOriginX);
	EB_U32 leafDepthLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		leafDepthNeighborArray,
		cuOriginY);
	EB_U32 leafDepthTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		leafDepthNeighborArray,
		cuOriginX);

	EB_U32 contextIndex;

	if (cuDepth < (maxCuDepth - 1)) {

		contextIndex =
			(modeTypeNeighborArray->leftArray[modeTypeLeftNeighborIndex] == (EB_U8)INVALID_MODE) ? 0 :
			(leafDepthNeighborArray->leftArray[leafDepthLeftNeighborIndex] > cuDepth) ? 1 : 0;
		contextIndex +=
			(modeTypeNeighborArray->topArray[modeTypeTopNeighborIndex] == (EB_U8)INVALID_MODE) ? 0 :
			(leafDepthNeighborArray->topArray[leafDepthTopNeighborIndex] > cuDepth) ? 1 : 0;

		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			splitFlag,
			&(cabacEncodeCtxPtr->contextModelEncContext.splitFlagContextModel[contextIndex]));
	}

	return;
}

/*********************************************************************
* EncodeSkipFlag
*   Encodes the skip flag
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
static void EncodeSkipFlag(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	EB_BOOL                 skipFlag,
	EB_U32                  cuOriginX,
	EB_U32                  cuOriginY,
	NeighborArrayUnit_t    *modeTypeNeighborArray,
	NeighborArrayUnit_t    *skipFlagNeighborArray)
{
	EB_U32 modeTypeLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		modeTypeNeighborArray,
		cuOriginY);
	EB_U32 modeTypeTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		modeTypeNeighborArray,
		cuOriginX);
	EB_U32 skipFlagLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		skipFlagNeighborArray,
		cuOriginY);
	EB_U32 skipFlagTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		skipFlagNeighborArray,
		cuOriginX);

	EB_U32 contextIndex;

	contextIndex =
		(modeTypeNeighborArray->leftArray[modeTypeLeftNeighborIndex] == (EB_U8)INVALID_MODE) ? 0 :
		(skipFlagNeighborArray->leftArray[skipFlagLeftNeighborIndex] == EB_TRUE) ? 1 : 0;
	contextIndex +=
		(modeTypeNeighborArray->topArray[modeTypeTopNeighborIndex] == (EB_U8)INVALID_MODE) ? 0 :
		(skipFlagNeighborArray->topArray[skipFlagTopNeighborIndex] == EB_TRUE) ? 1 : 0;

	EncodeOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
		skipFlag,
		&(cabacEncodeCtxPtr->contextModelEncContext.skipFlagContextModel[contextIndex]));

}

/*********************************************************************
* EncodeMergeFlag
*   Encodes the merge flag
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
EB_ERRORTYPE EncodeMergeFlag(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	PredictionUnit_t    *puPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 mergeFlag = (puPtr->mergeFlag) ? 1 : 0;

	EncodeOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
		mergeFlag,
		&(cabacEncodeCtxPtr->contextModelEncContext.mergeFlagContextModel[0]));

	return return_error;
}

/*********************************************************************
* EncodeMergeIndex
*   Encodes the merge index
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
EB_ERRORTYPE EncodeMergeIndex(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	PredictionUnit_t    *puPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 mergeIndex = puPtr->mergeIndex;


	switch (mergeIndex){
	case 4:
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1,
			&(cabacEncodeCtxPtr->contextModelEncContext.mergeIndexContextModel[0]));

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1);

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1);

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1);

		break;

	case 3:
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1,
			&(cabacEncodeCtxPtr->contextModelEncContext.mergeIndexContextModel[0]));

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1);

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1);

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			0);

		break;

	case 2:
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1,
			&(cabacEncodeCtxPtr->contextModelEncContext.mergeIndexContextModel[0]));

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1);

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			0);

		break;

	case 1:
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1,
			&(cabacEncodeCtxPtr->contextModelEncContext.mergeIndexContextModel[0]));

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			0);

		break;

	case 0:
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			0,
			&(cabacEncodeCtxPtr->contextModelEncContext.mergeIndexContextModel[0]));

		break;

	default:
		break;

	}

	return return_error;
}

/*********************************************************************
* EncodeMvpIndex
*   Encodes the motion vector prediction index
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*
*  refList
*   input pointer to the prediction unit
*********************************************************************/
EB_ERRORTYPE EncodeMvpIndex(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	PredictionUnit_t    *puPtr,
	EB_REFLIST              refList)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 mvpIndex = refList == REF_LIST_0 ? puPtr->mvd[REF_LIST_0].predIdx : puPtr->mvd[REF_LIST_1].predIdx;

	EncodeOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
		mvpIndex,
		&(cabacEncodeCtxPtr->contextModelEncContext.mvpIndexContextModel[0]));

	return return_error;
}

/*********************************************************************
* EncodePartitionSize
*   Encodes the partition size of a PU
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
EB_ERRORTYPE EncodePartitionSize(
	CabacEncodeContext_t	*cabacEncodeCtxPtr,
	CodingUnit_t			*cuPtr,
	EB_U32                   tbMaxDepth)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	if (cuPtr->predictionModeFlag == INTRA_MODE) {
		if (GetCodedUnitStats(cuPtr->leafIndex)->depth == (tbMaxDepth - 1)) {
			EncodeOneBin(
				&(cabacEncodeCtxPtr->bacEncContext),
				1, // SIZE_2Nx2N
				&(cabacEncodeCtxPtr->contextModelEncContext.partSizeContextModel[0]));
		}
	}
	// only true for P-mode and 2Nx2N partition
	else {
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1,
			&(cabacEncodeCtxPtr->contextModelEncContext.partSizeContextModel[0]));
	}
	return return_error;
}

EB_ERRORTYPE EncodeIntra4x4PartitionSize(
	CabacEncodeContext_t	*cabacEncodeCtxPtr,
	CodingUnit_t			*cuPtr,
	EB_U32                   tbMaxDepth)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	if (GetCodedUnitStats(cuPtr->leafIndex)->depth == (tbMaxDepth - 1)) {
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			0, // SIZE_NxN
			&(cabacEncodeCtxPtr->contextModelEncContext.partSizeContextModel[0]));
	}
	return return_error;
}

/*********************************************************************
* EncodePredictionMode
*   Encodes the prediction mode of a PU
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
EB_ERRORTYPE EncodePredictionMode(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	CodingUnit_t        *cuPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EncodeOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
		(cuPtr->predictionModeFlag == INTER_MODE) ? 0 : 1,
		&(cabacEncodeCtxPtr->contextModelEncContext.predModeContextModel[0]));

	return return_error;
}
/*********************************************************************
* EncodeIntraLumaModeFirstStage
*   Encodes the intra luma mode
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
static void EncodeIntraLumaModeFirstStage(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	EB_U32                  puOriginX,
	EB_U32                  puOriginY,
	EB_U32                  lcuSize,
    EB_U8                  *intraLumaLeftMode,
    EB_U8                  *intraLumaTopMode,
    EB_U32                  lumaMode,
	NeighborArrayUnit_t    *modeTypeNeighborArray,
	NeighborArrayUnit_t    *intraLumaModeNeighborArray)
{

	EB_U32 modeTypeLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		modeTypeNeighborArray,
		puOriginY);
	EB_U32 modeTypeTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		modeTypeNeighborArray,
		puOriginX);
	EB_U32 intraLumaModeLeftNeighborIndex = GetNeighborArrayUnitLeftIndex(
		intraLumaModeNeighborArray,
		puOriginY);
	EB_U32 intraLumaModeTopNeighborIndex = GetNeighborArrayUnitTopIndex(
		intraLumaModeNeighborArray,
		puOriginX);

	EB_S32 predictionIndex;
	EB_U32 lumaPredictionArray[3];

	EB_U32 leftNeighborMode = (EB_U32)(
		(modeTypeNeighborArray->leftArray[modeTypeLeftNeighborIndex] != INTRA_MODE) ? (EB_U32)EB_INTRA_DC :
		intraLumaModeNeighborArray->leftArray[intraLumaModeLeftNeighborIndex]);

	EB_U32 topNeighborMode = (EB_U32)(
		(modeTypeNeighborArray->topArray[modeTypeTopNeighborIndex] != INTRA_MODE) ? (EB_U32)EB_INTRA_DC :
		((puOriginY & (lcuSize - 1)) == 0) ? (EB_U32)EB_INTRA_DC :
		intraLumaModeNeighborArray->topArray[intraLumaModeTopNeighborIndex]);

    *intraLumaLeftMode = (EB_U8)leftNeighborMode;
    *intraLumaTopMode  = (EB_U8)topNeighborMode;

	if (leftNeighborMode == topNeighborMode) {

		if (leftNeighborMode > 1) { // For angular modes
			lumaPredictionArray[0] = leftNeighborMode;
			lumaPredictionArray[1] = ((leftNeighborMode + 29) & 0x1F) + 2;
			lumaPredictionArray[2] = ((leftNeighborMode - 1) & 0x1F) + 2;
		}
		else { // Non Angular modes
			lumaPredictionArray[0] = EB_INTRA_PLANAR;
			lumaPredictionArray[1] = EB_INTRA_DC;
			lumaPredictionArray[2] = EB_INTRA_VERTICAL;
		}
	}
	else {
		lumaPredictionArray[0] = leftNeighborMode;
		lumaPredictionArray[1] = topNeighborMode;

		if (leftNeighborMode && topNeighborMode) {
			lumaPredictionArray[2] = EB_INTRA_PLANAR; // when both modes are non planar 
		}
		else {
			lumaPredictionArray[2] = (leftNeighborMode + topNeighborMode) < 2 ? EB_INTRA_VERTICAL : EB_INTRA_DC;
		}
	}

	predictionIndex = (lumaMode == lumaPredictionArray[0]) ? 0 :
		(lumaMode == lumaPredictionArray[1]) ? 1 :
		(lumaMode == lumaPredictionArray[2]) ? 2 :
		-1; // luma mode is not equal to any of the predictors

	if (predictionIndex != -1) {
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1,
			&(cabacEncodeCtxPtr->contextModelEncContext.intraLumaContextModel[0]));

	}
	else {
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			0,
			&(cabacEncodeCtxPtr->contextModelEncContext.intraLumaContextModel[0]));
	}
}

/*********************************************************************
* EncodeIntraLumaModeFirstStage
*   Encodes the intra luma mode
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
static void EncodeIntraLumaModeSecondStage(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
    EB_U8                   leftNeighborMode,
    EB_U8                   topNeighborMode,
    EB_U32                  lumaMode)
{
	EB_S32 predictionIndex;

	EB_U32 lumaPredictionArray[3];

	if (leftNeighborMode == topNeighborMode) {

		if (leftNeighborMode > 1) { // For angular modes
			lumaPredictionArray[0] = leftNeighborMode;
			lumaPredictionArray[1] = ((leftNeighborMode + 29) & 0x1F) + 2;
			lumaPredictionArray[2] = ((leftNeighborMode - 1) & 0x1F) + 2;
		}
		else { // Non Angular modes
			lumaPredictionArray[0] = EB_INTRA_PLANAR;
			lumaPredictionArray[1] = EB_INTRA_DC;
			lumaPredictionArray[2] = EB_INTRA_VERTICAL;
		}
	}
	else {
		lumaPredictionArray[0] = leftNeighborMode;
		lumaPredictionArray[1] = topNeighborMode;

		if (leftNeighborMode && topNeighborMode) {
			lumaPredictionArray[2] = EB_INTRA_PLANAR; // when both modes are non planar 
		}
		else {
			lumaPredictionArray[2] = (leftNeighborMode + topNeighborMode) < 2 ? EB_INTRA_VERTICAL : EB_INTRA_DC;
		}
	}

	predictionIndex = (lumaMode == lumaPredictionArray[0]) ? 0 :
		(lumaMode == lumaPredictionArray[1]) ? 1 :
		(lumaMode == lumaPredictionArray[2]) ? 2 :
		-1; // luma mode is not equal to any of the predictors

	if (predictionIndex != -1) {

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			predictionIndex ? 1 : 0);

		if (predictionIndex) {
			EncodeBypassOneBin(
				&(cabacEncodeCtxPtr->bacEncContext),
				predictionIndex - 1);
		}
	}
	else {

		if (lumaPredictionArray[0] > lumaPredictionArray[1]) {
			SWAP(lumaPredictionArray[0], lumaPredictionArray[1]);
		}

		if (lumaPredictionArray[0] > lumaPredictionArray[2]) {
			SWAP(lumaPredictionArray[0], lumaPredictionArray[2]);
		}

		if (lumaPredictionArray[1] > lumaPredictionArray[2]) {
			SWAP(lumaPredictionArray[1], lumaPredictionArray[2]);
		}

		lumaMode =
			(lumaMode > lumaPredictionArray[2]) ? lumaMode - 1 :
			lumaMode;
		lumaMode =
			(lumaMode > lumaPredictionArray[1]) ? lumaMode - 1 :
			lumaMode;
		lumaMode =
			(lumaMode > lumaPredictionArray[0]) ? lumaMode - 1 :
			lumaMode;

		EncodeBypassBins(
			&(cabacEncodeCtxPtr->bacEncContext),
			lumaMode,
			5);
	}

}

/*********************************************************************
* EncodeIntraChromaMode
*   Encodes the intra chroma mode
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
EB_ERRORTYPE EncodeIntraChromaMode(
	CabacEncodeContext_t   *cabacEncodeCtxPtr)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;

    // EB_INTRA_CHROMA_DM
    EncodeOneBin(
        &(cabacEncodeCtxPtr->bacEncContext),
        0,
        &(cabacEncodeCtxPtr->contextModelEncContext.intraChromaContextModel[0]));

	return return_error;
}

/*********************************************************************
* ExponentialGolombCode
*   Encodes the symbol using Exponential Golomb Code
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  symbol
*   input to be coded using exponential golomb code
*
*  count
*   input count for exponential golomb code
*********************************************************************/
EB_ERRORTYPE ExponentialGolombCode(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	EB_U32 symbol,
	EB_U32 count)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 bins = 0;
	EB_S32 numOfBins = 0;

	while (symbol >= (EB_U32)(1 << count))
	{
		bins = (bins << 1) + 1;
		numOfBins++;
		symbol -= 1 << count;
		count++;
	}
	bins = (bins << 1);
	numOfBins++;

	bins = (bins << count) | symbol;
	numOfBins += count;

	EncodeBypassBins(
		&(cabacEncodeCtxPtr->bacEncContext),
		bins,
		numOfBins);

	return return_error;
}

/*********************************************************************
* EncodeMvd
*   Encodes the motion vector difference
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*
*  refList
*   input pointer to the prediction unit
*********************************************************************/
EB_ERRORTYPE EncodeMvd(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	PredictionUnit_t       *puPtr,
	EB_REFLIST              refList)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	const EB_S32 mvdX = refList == REF_LIST_0 ? puPtr->mvd[REF_LIST_0].mvdX : puPtr->mvd[REF_LIST_1].mvdX;
	const EB_S32 mvdY = refList == REF_LIST_0 ? puPtr->mvd[REF_LIST_0].mvdY : puPtr->mvd[REF_LIST_1].mvdY;

	EB_U32 xSign = (0 > mvdX);
	EB_U32 ySign = (0 > mvdY);

	EB_U32 absMvdX = ABS(mvdX);
	EB_U32 absMvdY = ABS(mvdY);

	EB_U32 mvdXneq0 = (mvdX != 0);
	EB_U32 mvdYneq0 = (mvdY != 0);

	EB_U32 absmvdXgt1 = (absMvdX > 1);
	EB_U32 absmvdYgt1 = (absMvdY > 1);

	EncodeOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
		mvdXneq0,
		&(cabacEncodeCtxPtr->contextModelEncContext.mvdContextModel[0]));

	EncodeOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
		mvdYneq0,
		&(cabacEncodeCtxPtr->contextModelEncContext.mvdContextModel[0]));

	if (mvdXneq0){
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			absmvdXgt1,
			&(cabacEncodeCtxPtr->contextModelEncContext.mvdContextModel[1]));
	}

	if (mvdYneq0) {
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			absmvdYgt1,
			&(cabacEncodeCtxPtr->contextModelEncContext.mvdContextModel[1]));
	}

	switch (mvdXneq0) {
	case 1:
		switch (absmvdXgt1) {
		case 1:
			ExponentialGolombCode(
				cabacEncodeCtxPtr,
				(EB_S32)absMvdX - 2,
				1);
		case 0:
			// Sign of mvdX
			EncodeBypassOneBin(
				&(cabacEncodeCtxPtr->bacEncContext),
				xSign);
            break;
		default:
			break;
		}

	case 0:
        break;
	default:
		break;
	}

	switch (mvdYneq0) {
	case 1:
		switch (absmvdYgt1){
		case 1:
			ExponentialGolombCode(
				cabacEncodeCtxPtr,
				(EB_S32)absMvdY - 2,
				1);
		case 0:
			// Sign of mvdY
			EncodeBypassOneBin(
				&(cabacEncodeCtxPtr->bacEncContext),
				ySign);
		default:
			break;
		}

	case 0:
	default:
		break;
	}

	return return_error;
}

/*********************************************************************
* EncodePredictionDirection
*   Encodes the inter prediction direction
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
EB_ERRORTYPE EncodePredictionDirection(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	PredictionUnit_t    *puPtr,
	CodingUnit_t        *cuPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32  predictionDirection = puPtr->interPredDirectionIndex;
	{
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			predictionDirection == BI_PRED ? 1 : 0,
			&(cabacEncodeCtxPtr->contextModelEncContext.interDirContextModel[GetCodedUnitStats(cuPtr->leafIndex)->depth]));
	}

	if (predictionDirection != BI_PRED){
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			predictionDirection,
			&(cabacEncodeCtxPtr->contextModelEncContext.interDirContextModel[4]));
	}

	return return_error;
}

/*********************************************************************
* EncodeReferencePictureIndex
*   Encodes the reference picture index
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*
*  refList
*   input pointer to the prediction unit
*********************************************************************/
EB_ERRORTYPE EncodeReferencePictureIndex(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	EB_REFLIST              refList,
	PictureControlSet_t    *pictureControlSetPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_S32 refListsCount = (refList == REF_LIST_0) ?
		pictureControlSetPtr->ParentPcsPtr->refList0Count :
		pictureControlSetPtr->ParentPcsPtr->refList1Count;

	if (refListsCount > 1) {

		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			0,
			&(cabacEncodeCtxPtr->contextModelEncContext.refPicContextModel[0]));

	}

	return return_error;
}
/*********************************************************************
* EncodeSaoMerge
*   Encodes the SAO Merge
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  mergeLeftFlag
*   input sao merge flag
*
*  componentType
*   Y, Cb, or Cr
*********************************************************************/
EB_ERRORTYPE EncodeSaoMerge(
	CabacEncodeContext_t    *cabacEncodeCtxPtr,
	EB_U32                   mergeFlag)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EncodeOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
		mergeFlag ? 1 : 0,
		&(cabacEncodeCtxPtr->contextModelEncContext.saoMergeFlagContextModel[0]));

	return return_error;
}

/*********************************************************************
* EncodeSaoType
*   Encodes the SAO type
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  saoType
*   input sao type
*
*********************************************************************/
EB_ERRORTYPE EncodeSaoType(
	CabacEncodeContext_t    *cabacEncodeCtxPtr,
	EB_U32                   saoType)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	if (saoType == 0) {
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			0,
			&(cabacEncodeCtxPtr->contextModelEncContext.saoTypeIndexContextModel[0]));
	}
	else {
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			1,
			&(cabacEncodeCtxPtr->contextModelEncContext.saoTypeIndexContextModel[0]));

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			(saoType <= 4) ? 1 : 0);
	}

	return return_error;
}
/*********************************************************************
* EncodeOffsetTrunUnary
*   Encodes SAO offset
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  offset
*   input offset value
*
*  maxValue
*   input maximum value of the offsets
*********************************************************************/
EB_ERRORTYPE EncodeOffsetTrunUnary(
	CabacEncodeContext_t    *cabacEncodeCtxPtr,
	EB_U32                   offset,
	EB_U32                   maxValue)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32 index;
	EB_BOOL codeLastBin = (maxValue > offset) ? EB_TRUE : EB_FALSE;

	EncodeBypassOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
		(offset == 0) ? 0 : 1);

	if (offset != 0) {

		for (index = 0; index < (offset - 1); index++) {
			EncodeBypassOneBin(
				&(cabacEncodeCtxPtr->bacEncContext),
				1);
		}

		if (codeLastBin) {
			EncodeBypassOneBin(
				&(cabacEncodeCtxPtr->bacEncContext),
				0);
		}
	}

	return return_error;
}

/*********************************************************************
* EncodeSaoOffsets
*   Encodes SAO offsets
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  *offset
*   input pointer to offset values
*
*  bandPosition
*   input position of the band
*********************************************************************/
EB_ERRORTYPE EncodeSaoOffsets(
	CabacEncodeContext_t    *cabacEncodeCtxPtr,
	EB_U32                   componentIdx,
	EB_U32                  *saoType,
	EB_S32                  *offset,
    EB_U32                   bandPosition,
	
	EB_U8                    bitdepth)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32 saoTypeIdx, eoType;

	EB_U32 maxSaoOffsetValue = (1 << (MIN(bitdepth, 10) - 5)) - 1;

	// Code SAO type
	if (componentIdx != 2) {

		saoTypeIdx = saoType[componentIdx];
		EncodeSaoType(
			cabacEncodeCtxPtr,
			saoTypeIdx);
	}
	else {
		saoTypeIdx = saoType[1];
	}

	if (saoTypeIdx != 0) {

		// Band Offset
		if (saoTypeIdx == 5) {

			// code abs value of offsets
			EncodeOffsetTrunUnary(
				cabacEncodeCtxPtr,
				ABS(offset[0]),
				maxSaoOffsetValue);

			EncodeOffsetTrunUnary(
				cabacEncodeCtxPtr,
				ABS(offset[1]),
				maxSaoOffsetValue);

			EncodeOffsetTrunUnary(
				cabacEncodeCtxPtr,
				ABS(offset[2]),
				maxSaoOffsetValue);

			EncodeOffsetTrunUnary(
				cabacEncodeCtxPtr,
				ABS(offset[3]),
				maxSaoOffsetValue);

			// code the sign of offsets
			if (offset[0] != 0) {
				EncodeBypassOneBin(
					&(cabacEncodeCtxPtr->bacEncContext),
					(offset[0] < 0) ? 1 : 0);
			}

			if (offset[1] != 0) {
				EncodeBypassOneBin(
					&(cabacEncodeCtxPtr->bacEncContext),
					(offset[1] < 0) ? 1 : 0);
			}

			if (offset[2] != 0) {
				EncodeBypassOneBin(
					&(cabacEncodeCtxPtr->bacEncContext),
					(offset[2] < 0) ? 1 : 0);
			}

			if (offset[3] != 0) {
				EncodeBypassOneBin(
					&(cabacEncodeCtxPtr->bacEncContext),
					(offset[3] < 0) ? 1 : 0);
			}

			// code band position
			EncodeBypassBins(
				&(cabacEncodeCtxPtr->bacEncContext),
				bandPosition,
				5);
		}
		// Edge Offset
		else {

			// code the offset values
			EncodeOffsetTrunUnary(
				cabacEncodeCtxPtr,
				offset[0],
				maxSaoOffsetValue);

			EncodeOffsetTrunUnary(
				cabacEncodeCtxPtr,
				offset[1],
				maxSaoOffsetValue);

			EncodeOffsetTrunUnary(
				cabacEncodeCtxPtr,
				(EB_U32)-offset[2],
				maxSaoOffsetValue);

			EncodeOffsetTrunUnary(
				cabacEncodeCtxPtr,
				(EB_U32)-offset[3],
				maxSaoOffsetValue);

			// Derive EO type from sao type
			// 0: EO - 0 degrees
			// 1: EO - 90 degrees
			// 2: EO - 135 degrees
			// 3: EO - 45 degrees
			if (componentIdx != 2) {
				eoType = saoTypeIdx - 1;
				EncodeBypassBins(
					&(cabacEncodeCtxPtr->bacEncContext),
					eoType,
					2);
			}
		}

	}

	return return_error;

}

/*********************************************************************
* ComputeNumofSigCoefficients
*   Computes the number of significant quantized coefficients
*
*  coeffBufferPtr
*   input pointer to the quantized coefficient buffer
*
*  coeffStride
*   stride of coefficient buffer
*
*  size
*   size of the PU
*
*  numOfCoeffsPtr
*   is the output pointer to the number of significant coefficients
*********************************************************************/
EB_ERRORTYPE ComputeNumofSigCoefficients(
	EB_S16       *coeffBufferPtr,
	const EB_U32  coeffStride,
	const EB_U32  size,
	EB_U32       *numOfCoeffsPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 colIndex, rowIndex;
	*numOfCoeffsPtr = 0;

	for (rowIndex = 0; rowIndex < size; rowIndex++) {
		for (colIndex = 0; colIndex < size; colIndex++) {
			(*numOfCoeffsPtr) += (coeffBufferPtr[colIndex] != 0);
		}
		coeffBufferPtr += coeffStride;
	}
	return return_error;
}

// Table of subblock scans
// Note: for 4x4, only one entry with value 0 is required (hence reusing 8x8 scan)
static const EB_U8 *sbScans[] =
{
	sbScan8, sbScan8, sbScan16, sbScan32
};

void EncodeQuantizedCoefficients_generic(
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
    TransformUnit_t              *tuPtr)
{
	EB_S32 blockSizeOffset;
	EB_S32 absVal;
	EB_U32 symbolValue;
	EB_U32 contextOffset;
	EB_U32 contextOffset2;
	EB_U32 scanIndex;

	EB_S16 linearCoeff[MAX_TU_SIZE * MAX_TU_SIZE];
	EB_S16 *linearCoeffBufferPtr;

	// Significance map for each 4x4 subblock
	// 1 bit per coefficient
	// i-th bit corresponds to i-th coefficient in forward scan order
	EB_U16 subblockSigmap[MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4)];

	EB_S32 lastScanSet = -1;
	EB_S32 subSetIndex;
	EB_U32 coeffGroupPosition;
	EB_S32 coeffGroupPositionY;
	EB_S32 coeffGroupPositionX;

	EB_S16 *subblockPtr;

	EB_S16 k;
	EB_S32 sigmap;
	EB_U32 num;
	EB_S32 val;
	EB_S32 sign;
	EB_U32 isNonZero;

	EB_S32 sbDiagIdx;

	EB_S32 isChroma = componentType != COMPONENT_LUMA;
	EB_U32 logBlockSize = Log2f(size);

	EB_U32 posLast;
	EB_S32 scanPosLast;
	EB_S32 lastSigXPos;
	EB_S32 lastSigYPos;

	EB_U32 significantFlagContextOffset;
	EB_U32 contextOffset1;
	EB_U32 sbSigMemory;
	EB_S32 sbPrevDiagIdx;

	EB_S32 numNonZero; // Number of nonzero coefficients in current subblock
    EB_S32 absCoeff[16] = {0}; // Array containing list of nonzero coefficients (size given by numNonZero)
	EB_U32 signFlags;

	EB_U32 tempOffset;
	const EB_U8 *contextIndexMapPtr;

	EB_U32 golombRiceParam;
	EB_S32 index, index2;
	EB_U32 contextSet;
	EB_S32 numCoeffWithCodedGt1Flag; // Number of coefficients for which >1 flag is coded


	 EB_U32      numNonZeroCoeffs = tuPtr->nzCoefCount[ (componentType == COMPONENT_LUMA)      ? 0 : 
                                                        (componentType == COMPONENT_CHROMA_CB) ? 1 : 2
                                                      ];
                 numNonZeroCoeffs = (componentType == COMPONENT_CHROMA_CB2) ? tuPtr->nzCoefCount2[0] :
                                    (componentType == COMPONENT_CHROMA_CR2) ? tuPtr->nzCoefCount2[1] : numNonZeroCoeffs;

    EB_BOOL secondChroma = componentType == COMPONENT_CHROMA_CB2 || componentType == COMPONENT_CHROMA_CR2;
    // zerout the buffer to support N2_SHAPE & N4_SHAPE
    EB_U32  transCoeffShape = ((componentType == COMPONENT_LUMA) ? tuPtr->transCoeffShapeLuma    :
                                                    secondChroma ? tuPtr->transCoeffShapeChroma2 : tuPtr->transCoeffShapeChroma);
    EB_U32  isOnlyDc = (componentType == COMPONENT_LUMA) ? tuPtr->isOnlyDc[0] :
                                            secondChroma ? tuPtr->isOnlyDc2[(componentType == COMPONENT_CHROMA_CB2) ? 0 : 1] :
                                                           tuPtr->isOnlyDc[(componentType == COMPONENT_CHROMA_CB) ? 1 : 2];

    if (transCoeffShape && isOnlyDc == EB_FALSE) {
        PicZeroOutCoef_funcPtrArray[(EB_ASM_C & PREAVX2_MASK) && 1][(size >> 1) >> 3](
            coeffBufferPtr,
            coeffStride,
            (size >> 1),
            (size >> 1),
            (size >> 1));

        PicZeroOutCoef_funcPtrArray[(EB_ASM_C & PREAVX2_MASK) && 1][(size >> 1) >> 3](
            coeffBufferPtr,
            coeffStride,
            (size >> 1) * coeffStride,
            (size >> 1),
            (size >> 1));

        PicZeroOutCoef_funcPtrArray[(EB_ASM_C & PREAVX2_MASK) && 1][(size >> 1) >> 3](
            coeffBufferPtr,
            coeffStride,
            (size >> 1) * coeffStride + (size >> 1),
            (size >> 1),
            (size >> 1));

        if (transCoeffShape == N4_SHAPE) {
            PicZeroOutCoef_funcPtrArray[(EB_ASM_C & PREAVX2_MASK) && 1][(size >> 2) >> 3](
                coeffBufferPtr,
                coeffStride,
                (size >> 2),
                (size >> 2),
                (size >> 2));

            PicZeroOutCoef_funcPtrArray[(EB_ASM_C & PREAVX2_MASK) && 1][(size >> 2) >> 3](
                coeffBufferPtr,
                coeffStride,
                (size >> 2) * coeffStride,
                (size >> 2),
                (size >> 2));

            PicZeroOutCoef_funcPtrArray[(EB_ASM_C & PREAVX2_MASK) && 1][(size >> 2) >> 3](
                coeffBufferPtr,
                coeffStride,
                (size >> 2) * coeffStride + (size >> 2),
                (size >> 2),
                (size >> 2));

        }
    }


	// DC-only fast track
	if (numNonZeroCoeffs == 1 && coeffBufferPtr[0] != 0)
	{

		blockSizeOffset = isChroma ? NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize - 2) * 3 + ((logBlockSize - 1) >> 2));

		EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), 0,
			&(cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[blockSizeOffset]));
		EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), 0,
			&(cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[blockSizeOffset]));

		absVal = ABS(coeffBufferPtr[0]);
		symbolValue = absVal > 1;

		contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS;
		contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS;

		// Add bits for coeff_abs_level_greater1_flag
		EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), symbolValue,
			&(cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset + 1]));

		if (symbolValue)
		{
			symbolValue = absVal > 2;

			// Add bits for coeff_abs_level_greater2_flag
			EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), symbolValue,
				&(cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[contextOffset2]));
		}

		EncodeBypassOneBin(&(cabacEncodeCtxPtr->bacEncContext), coeffBufferPtr[0] < 0);

		if (absVal > 2)
		{
			// Golomb Rice parameter is known to be 0 here
			RemainingCoeffExponentialGolombCode(cabacEncodeCtxPtr, absVal - 3, 0);
		}

		return;
	}

	// Compute the scanning type
	// optimized this if statement later
	scanIndex = SCAN_DIAG2;

	if (type == INTRA_MODE)
	{
		// The test on partition size should be commented out to get conformance for Intra 4x4 !
		// if (partitionSize == SIZE_2Nx2N) To do - update asm 
		{
			// note that for Intra 2Nx2N, each CU is one PU. So this mode is the same as
			// the mode of upper-left corner of current CU
			//intraLumaMode   = candidatePtr->intraLumaMode[0];
			//intraChromaMode = candidatePtr->intraChromaMode[0];

			if ((((EB_S32)logBlockSize) <= 3 - isChroma) ||
                    (((EB_S32)logBlockSize) == 3 && cabacEncodeCtxPtr->colorFormat == EB_YUV444)) {
				EB_U32 tempIntraChromaMode = chromaMappingTable[intraChromaMode];
				EB_S32 intraMode = (!isChroma || tempIntraChromaMode == EB_INTRA_CHROMA_DM) ? intraLumaMode : tempIntraChromaMode;
                if (cabacEncodeCtxPtr->colorFormat == EB_YUV422 && isChroma && tempIntraChromaMode == EB_INTRA_CHROMA_DM) {
                   intraMode = intra422PredModeMap[intraLumaMode];
                }

				if (ABS(8 - ((intraMode - 2) & 15)) <= 4) {
					scanIndex = (intraMode & 16) ? SCAN_HOR2 : SCAN_VER2;
				}
			}
		}
	}

	//-------------------------------------------------------------------------------------------------------------------
	// Coefficients ordered according to scan order (absolute values)
	linearCoeffBufferPtr = linearCoeff;

	// Loop through subblocks to reorder coefficients according to scan order
	// Also derive significance map for each subblock, and determine last subblock that contains nonzero coefficients
	subSetIndex = 0;
	while (1)
	{
		// determine position of subblock within transform block
		coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
		coeffGroupPositionY = coeffGroupPosition >> 4;
		coeffGroupPositionX = coeffGroupPosition & 15;

		if (scanIndex == SCAN_HOR2)
		{
			// Subblock scan is mirrored for horizontal scan
			SWAP(coeffGroupPositionX, coeffGroupPositionY);
		}

		subblockPtr = coeffBufferPtr + 4 * coeffGroupPositionY * coeffStride + 4 * coeffGroupPositionX;

		sigmap = 0;
		num = 0;

		for (k = 0; k < 16; k++)
		{
			EB_U32 position = scans4[scanIndex != SCAN_DIAG2][k];
			EB_U32 positionY = position >> 2;
			EB_U32 positionX = position & 3;
			if (scanIndex == SCAN_HOR2)
			{
				// Subblock scan is mirrored for horizontal scan
				SWAP(positionX, positionY);
			}

			val = subblockPtr[coeffStride * positionY + positionX];
			linearCoeff[16 * subSetIndex + k] = (EB_S16)val;
			isNonZero = val != 0;
			num += isNonZero;
			sigmap |= isNonZero << k;
		}

		subblockSigmap[subSetIndex] = (EB_U16)sigmap;

		if (sigmap != 0)
		{
			lastScanSet = subSetIndex;

			numNonZeroCoeffs -= num;
			if (numNonZeroCoeffs == 0)
			{
				break;
			}
		}

		subSetIndex++;

	}

	//-------------------------------------------------------------------------------------------------------------------
	// Obtain the last significant X and Y positions and compute their bit cost

	// subblock position
	coeffGroupPosition = sbScans[logBlockSize - 2][lastScanSet];
	coeffGroupPositionY = coeffGroupPosition >> 4;
	coeffGroupPositionX = coeffGroupPosition & 15;
	lastSigYPos = 4 * coeffGroupPositionY;
	lastSigXPos = 4 * coeffGroupPositionX;
	scanPosLast = 16 * lastScanSet;

	// position within subblock
	posLast = Log2f(subblockSigmap[lastScanSet]);
	coeffGroupPosition = scans4[scanIndex != SCAN_DIAG2][posLast];
	lastSigYPos += coeffGroupPosition >> 2;
	lastSigXPos += coeffGroupPosition & 3;
	scanPosLast += posLast;

	// Should swap row/col for SCAN_HOR and SCAN_VER:
	// - SCAN_HOR because the subscan order is mirrored (compared to SCAN_DIAG)
	// - SCAN_VER because of equation (7-66) in H.265 (04/2013)
	// Note that the scans4 array is adjusted to reflect this
	if (scanIndex != SCAN_DIAG2)
	{
		SWAP(lastSigXPos, lastSigYPos);
	}

	// Encode the position of last significant coefficient
	EncodeLastSignificantXY(cabacEncodeCtxPtr, lastSigXPos, lastSigYPos, size, logBlockSize, isChroma);

	//-------------------------------------------------------------------------------------------------------------------
	// Encode Coefficient levels

	significantFlagContextOffset = (!isChroma) ? 0 : NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS;

	contextOffset1 = 1;

	// Memory for 4x4 subblock-level significance flag
	// Bits 0-7 are for current diagonal
	// Bits 16-23 are for previous diagonal
	sbSigMemory = 0;

	// Subblock diagonal index for previous 4x4 subblock (diagonal index is row+col)
	sbPrevDiagIdx = -1;

	// Loop over subblocks
	subSetIndex = lastScanSet;
	do
	{
		// 1. Subblock-level significance flag

		// Assign default value that works for first and last subblock
		EB_S32 significantFlagContextPattern = sbSigMemory & 3;

		if (subSetIndex != 0)
		{
			coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
			coeffGroupPositionY = coeffGroupPosition >> 4;
			coeffGroupPositionX = coeffGroupPosition & 15;

			sbDiagIdx = coeffGroupPositionY + coeffGroupPositionX;
			if (sbDiagIdx != sbPrevDiagIdx)
			{
				sbSigMemory = sbSigMemory << 16;
				sbPrevDiagIdx = sbDiagIdx;
			}

			if (subSetIndex != lastScanSet)
			{
				EB_U32 sigCoeffGroupContextIndex;
				EB_U32 significanceFlag;

				significantFlagContextPattern = (sbSigMemory >> (16 + coeffGroupPositionY)) & 3;
				sigCoeffGroupContextIndex = significantFlagContextPattern != 0;
				sigCoeffGroupContextIndex += isChroma * NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS;

				significanceFlag = (subblockSigmap[subSetIndex] != 0);

				// Add bits for coded_sub_block_flag
				EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), significanceFlag,
					&(cabacEncodeCtxPtr->contextModelEncContext.coeffGroupSigFlagContextModel[sigCoeffGroupContextIndex]));

				if (!significanceFlag)
				{
					// Nothing else to do for this subblock since all coefficients in it are zero
					continue;
				}
			}

			// Memorize that current subblock is nonzero
			sbSigMemory += 1 << coeffGroupPositionY;
		}

		// 2. Coefficient significance flags

		numNonZero = 0; // Number of nonzero coefficients in current subblock
		signFlags = 0;

		// Use do {} while(0) loop to avoid goto statement (early exit)
		do
		{
			EB_S32 scanPosSig;
			EB_S32 sigMap = subblockSigmap[subSetIndex];
			EB_S32 subPosition = subSetIndex << LOG2_SCAN_SET_SIZE;
			EB_S32 subPosition2 = subPosition;

			if (subSetIndex == lastScanSet)
			{
				int val = linearCoeffBufferPtr[scanPosLast];
				int sign = val >> 31;
				signFlags = -sign;
				absCoeff[0] = (val ^ sign) - sign;

				numNonZero = 1;
				if (sigMap == 1)
				{
					// Nothing else to do here (only "DC" coeff within subblock)
					break;
				}
				scanPosSig = scanPosLast - 1;
				sigMap <<= 31 - (scanPosSig & 15);
			}
			else
			{
				if (sigMap == 1 && subSetIndex != 0)
				{
					subPosition2++;
					val = linearCoeffBufferPtr[subPosition];
					sign = val >> 31;
					signFlags = -sign;
					absCoeff[0] = (val ^ sign) - sign;
					numNonZero = 1;
				}
				scanPosSig = subPosition + 15;
				sigMap <<= 16;
			}

            // Jing: change here for 444
			if (logBlockSize == 2)
			{
				tempOffset = 0;
				contextIndexMapPtr = contextIndexMap4[scanIndex];
			}
			else
			{
				tempOffset = (logBlockSize == 3) ? ((scanIndex == SCAN_DIAG2 || isChroma) ? 9 : 15) : (!isChroma ? 21 : 12);
				tempOffset += (!isChroma && subSetIndex != 0) ? 3 : 0;
				contextIndexMapPtr = contextIndexMap8[scanIndex != SCAN_DIAG2][significantFlagContextPattern] - subPosition;
			}
            /////////////

			// Loop over coefficients
			do
			{
				EB_U32 sigContextIndex;
				EB_S32 sigCoeffFlag = sigMap < 0;

				sigContextIndex = (scanPosSig == 0) ? 0 : contextIndexMapPtr[scanPosSig] + tempOffset;

				// Add bits for sig_coeff_flag
				EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), sigCoeffFlag,
					&(cabacEncodeCtxPtr->contextModelEncContext.significanceFlagContextModel[significantFlagContextOffset + sigContextIndex]));

				if (sigCoeffFlag)
				{
					int val = scanPosSig >= 0 ? linearCoeffBufferPtr[scanPosSig] : 0;
					int sign = val >> 31;
					signFlags += signFlags - sign;
					absCoeff[numNonZero] = (val ^ sign) - sign;
					numNonZero++;
				}

				sigMap <<= 1;
				scanPosSig--;
			} while (scanPosSig >= subPosition2);
		} while (0);

		// 3. Coefficient level values
		golombRiceParam = 0;

		contextSet = (subSetIndex != 0 && !isChroma) ? 2 : 0;
		contextSet += (contextOffset1 == 0);
		contextOffset1 = 1;
		contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS + 4 * contextSet;
		contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS + contextSet;

		numCoeffWithCodedGt1Flag = MIN(GREATER_THAN1_MAX_NUMBER, numNonZero);

		// Loop over coefficients until base value of Exp-Golomb coding changes
		// Base value changes after either
		// - 8th coefficient
		// - a coefficient larger than 1
		index2 = numCoeffWithCodedGt1Flag;

		for (index = 0; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for coeff_abs_level_greater1_flag
			EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), symbolValue,
				&(cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset + contextOffset1]));

			if (symbolValue)
			{
				index2 = index;
				contextOffset1 = 0; // Need to set because used in next subblock
				index++;
				break;
			}

			if (contextOffset1 < 3)
			{
				contextOffset1++;
			}
		}

		for (; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for >1 flag
			EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), symbolValue,
				&(cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset]));
		}

		index = index2;
		if (index < numCoeffWithCodedGt1Flag)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 2;

			// Add bits for coeff_abs_level_greater2_flag
			EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), symbolValue,
				&(cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[contextOffset2]));
		}

		{
			EncodeBypassBins(&(cabacEncodeCtxPtr->bacEncContext), signFlags, numNonZero);
		}

		if (index < numCoeffWithCodedGt1Flag)
		{
			EB_S32 absVal = absCoeff[index];

			if (absVal >= 3)
			{
				// Golomb Rice parameter is known to be 0 here
				RemainingCoeffExponentialGolombCode(cabacEncodeCtxPtr, absVal - 3, 0);
				golombRiceParam = (absVal > 3);
			}
			index++;

			// Loop over coefficients after first one that was > 1 but before 8th one
			// Base value is know to be equal to 2
			for (; index < numCoeffWithCodedGt1Flag; index++)
			{
				EB_S32 absVal = absCoeff[index];
				if (absVal >= 2)
				{
					RemainingCoeffExponentialGolombCode(cabacEncodeCtxPtr, absVal - 2, golombRiceParam);
					if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
				}
			}
		}

		// Loop over remaining coefficients (8th and beyond)
		// Base value is known to be equal to 1
		for (; index < numNonZero; index++)
		{
			EB_S32 absVal = absCoeff[index];

			RemainingCoeffExponentialGolombCode(cabacEncodeCtxPtr, absVal - 1, golombRiceParam);

			// Update Golomb-Rice parameter
			if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
		}
	} while (--subSetIndex >= 0);

	return;
}

void EncodeQuantizedCoefficients_SSE2(
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
    TransformUnit_t               *tuPtr)

{
	EB_S32 absVal;
	EB_U32 symbolValue;
	EB_U32 contextOffset;
	EB_U32 contextOffset2;
	EB_U32 scanIndex;

    EB_U32 numNonZeroCoeffs = tuPtr->nzCoefCount[(componentType == COMPONENT_LUMA) ?
        0 : (componentType == COMPONENT_CHROMA_CB) ? 1 : 2];
    numNonZeroCoeffs = (componentType == COMPONENT_CHROMA_CB2) ? tuPtr->nzCoefCount2[0] :
        (componentType == COMPONENT_CHROMA_CR2) ? tuPtr->nzCoefCount2[1] : numNonZeroCoeffs;

	__m128i linearCoeff[MAX_TU_SIZE * MAX_TU_SIZE / (sizeof(__m128i) / sizeof(EB_S16))];
	EB_S16 *linearCoeffBufferPtr;

	// Significance map for each 4x4 subblock
	// 1 bit per coefficient
	// i-th bit corresponds to i-th coefficient in forward scan order
	EB_U16 subblockSigmap[MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4)];

	EB_S32 lastScanSet = -1;
	EB_S32 subSetIndex;
	EB_U32 coeffGroupPosition;
	EB_S32 coeffGroupPositionY;
	EB_S32 coeffGroupPositionX;

	EB_S16 *subblockPtr;

	EB_S32 sigmap;
	EB_S32 val;
	EB_S32 sign;

	EB_S32 sbDiagIdx;

	EB_S32 isChroma = componentType != COMPONENT_LUMA;
	EB_U32 logBlockSize = Log2f(size);

	EB_U32 posLast;
	EB_S32 scanPosLast;
	EB_S32 lastSigXPos;
	EB_S32 lastSigYPos;

	EB_U32 significantFlagContextOffset;
	EB_U32 contextOffset1;
	EB_U32 sbSigMemory;
	EB_S32 sbPrevDiagIdx;

	EB_S32 numNonZero; // Number of nonzero coefficients in current subblock
	EB_S32 absCoeff[16] = { 0 }; // Array containing list of nonzero coefficients (size given by numNonZero)
	EB_U32 signFlags;

	EB_U32 tempOffset;
	const EB_U8 *contextIndexMapPtr;

	EB_U32 sigCoeffGroupContextIndex;
	EB_U32 significanceFlag;

	EB_U32 golombRiceParam;
	EB_S32 index, index2;
	EB_U32 contextSet;
	EB_S32 numCoeffWithCodedGt1Flag; // Number of coefficients for which >1 flag is coded
    EB_U32  transCoeffShapeChroma = (componentType == COMPONENT_CHROMA_CB2 || componentType == COMPONENT_CHROMA_CR2) ? tuPtr->transCoeffShapeChroma2 : tuPtr->transCoeffShapeChroma;


	// DC-only fast track
	if (numNonZeroCoeffs == 1 && coeffBufferPtr[0] != 0)
	{
		EB_S32 blockSizeOffset;
		blockSizeOffset = isChroma ? NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize - 2) * 3 + ((logBlockSize - 1) >> 2));

		EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), 0,
			&(cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[blockSizeOffset]));
		EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), 0,
			&(cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[blockSizeOffset]));

		absVal = ABS(coeffBufferPtr[0]);
		symbolValue = absVal > 1;

		contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS;
		contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS;

		// Add bits for coeff_abs_level_greater1_flag
		EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), symbolValue,
			&(cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset + 1]));

		if (symbolValue)
		{
			symbolValue = absVal > 2;

			// Add bits for coeff_abs_level_greater2_flag
			EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), symbolValue,
				&(cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[contextOffset2]));
		}

		EncodeBypassOneBin(&(cabacEncodeCtxPtr->bacEncContext), coeffBufferPtr[0] < 0);

		if (absVal > 2)
		{
			// Golomb Rice parameter is known to be 0 here
			RemainingCoeffExponentialGolombCode(cabacEncodeCtxPtr, absVal - 3, 0);
		}

		return;
	}

	// Compute the scanning type
	// optimized this if statement later
	scanIndex = SCAN_DIAG2;

	if (type == INTRA_MODE)
	{
		// The test on partition size should be commented out to get conformance for Intra 4x4 !
		//if (partitionSize == SIZE_2Nx2N)
		{
			// note that for Intra 2Nx2N, each CU is one PU. So this mode is the same as
			// the mode of upper-left corner of current CU
			//intraLumaMode   = candidatePtr->intraLumaMode[0];
			//intraChromaMode = candidatePtr->intraChromaMode[0];

            if ((((EB_S32)logBlockSize) <= 3 - isChroma) ||
                    (((EB_S32)logBlockSize) == 3 && cabacEncodeCtxPtr->colorFormat == EB_YUV444)) {
				EB_U32 tempIntraChromaMode = chromaMappingTable[intraChromaMode];
				EB_S32 intraMode = (!isChroma || tempIntraChromaMode == EB_INTRA_CHROMA_DM) ? intraLumaMode : tempIntraChromaMode;

                if (cabacEncodeCtxPtr->colorFormat == EB_YUV422 && isChroma && tempIntraChromaMode == EB_INTRA_CHROMA_DM) {
                   intraMode = intra422PredModeMap[intraLumaMode];
                }

				if (ABS(8 - ((intraMode - 2) & 15)) <= 4) {
					scanIndex = (intraMode & 16) ? SCAN_HOR2 : SCAN_VER2;
				}
			}
		}
	}

	//-------------------------------------------------------------------------------------------------------------------
	// Coefficients ordered according to scan order (signed values)
	linearCoeffBufferPtr = (EB_S16 *)linearCoeff;

	lastScanSet = -1;

	// Loop through subblocks to reorder coefficients according to scan order
	// Also derive significance map for each subblock, and determine last subblock that contains nonzero coefficients
	subSetIndex = 0;
	while (1)
	{
		__m128i a0, a1, a2, a3;
		__m128i b0, b1, c0, c1, d0, d1;
		__m128i z0;

		// determine position of subblock within transform block
		coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
		coeffGroupPositionY = coeffGroupPosition >> 4;
		coeffGroupPositionX = coeffGroupPosition & 15;

		if (scanIndex == SCAN_HOR2)
		{
			// Subblock scan is mirrored for horizontal scan
			SWAP(coeffGroupPositionX, coeffGroupPositionY);
		}

		subblockPtr = coeffBufferPtr + 4 * coeffGroupPositionY * coeffStride + 4 * coeffGroupPositionX;

        EB_BOOL isCGin;

        if(isChroma==EB_FALSE){        
            isCGin  =  ((EB_U32)coeffGroupPositionY < (size >>(tuPtr->transCoeffShapeLuma+2))) && ((EB_U32)coeffGroupPositionX < (size >>(tuPtr->transCoeffShapeLuma+2)));
        }else{
            isCGin  =  ((EB_U32)coeffGroupPositionY < (size >>(transCoeffShapeChroma+2))) && ((EB_U32)coeffGroupPositionX < (size >>(transCoeffShapeChroma+2)));
        }
 
        if(isCGin == EB_FALSE){
        
            a0 = _mm_setzero_si128();
		    a1 = _mm_setzero_si128();
		    a2 = _mm_setzero_si128();
		    a3 = _mm_setzero_si128();

        }
        else{

            a0 = _mm_loadl_epi64((__m128i *)(subblockPtr + 0 * coeffStride)); // 00 01 02 03 -- -- -- --
		    a1 = _mm_loadl_epi64((__m128i *)(subblockPtr + 1 * coeffStride)); // 10 11 12 13 -- -- -- --
		    a2 = _mm_loadl_epi64((__m128i *)(subblockPtr + 2 * coeffStride)); // 20 21 22 23 -- -- -- --
		    a3 = _mm_loadl_epi64((__m128i *)(subblockPtr + 3 * coeffStride)); // 30 31 32 33 -- -- -- --
        }

		if (scanIndex == SCAN_DIAG2)
		{
			int v03;
			int v30;

			b0 = _mm_unpacklo_epi64(a0, a3); // 00 01 02 03 30 31 32 33
			b1 = _mm_unpacklo_epi16(a1, a2); // 10 20 11 21 12 22 13 23

			c0 = _mm_unpacklo_epi16(b0, b1); // 00 10 01 20 02 11 03 21
			c1 = _mm_unpackhi_epi16(b1, b0); // 12 30 22 31 13 32 23 33

			v03 = _mm_extract_epi16(a0, 3);
			v30 = _mm_extract_epi16(a3, 0);

			d0 = _mm_shufflehi_epi16(c0, 0xe1); // 00 10 01 20 11 02 03 21
			d1 = _mm_shufflelo_epi16(c1, 0xb4); // 12 30 31 22 13 32 23 33

			d0 = _mm_insert_epi16(d0, v30, 6); // 00 10 01 20 11 02 30 21
			d1 = _mm_insert_epi16(d1, v03, 1); // 12 03 31 22 13 32 23 33
		}
		else if (scanIndex == SCAN_HOR2)
		{
			d0 = _mm_unpacklo_epi64(a0, a1); // 00 01 02 03 10 11 12 13
			d1 = _mm_unpacklo_epi64(a2, a3); // 20 21 22 23 30 31 32 33
		}
		else
		{
			b0 = _mm_unpacklo_epi16(a0, a2); // 00 20 01 21 02 22 03 23
			b1 = _mm_unpacklo_epi16(a1, a3); // 10 30 11 31 12 32 13 33

			d0 = _mm_unpacklo_epi16(b0, b1); // 00 10 20 30 01 11 21 31
			d1 = _mm_unpackhi_epi16(b0, b1); // 02 12 22 32 03 13 23 33
		}

		z0 = _mm_packs_epi16(d0, d1);
		z0 = _mm_cmpeq_epi8(z0, _mm_setzero_si128());

		sigmap = _mm_movemask_epi8(z0) ^ 0xffff;
		subblockSigmap[subSetIndex] = (EB_U16)sigmap;

		if (sigmap != 0)
		{
			EB_U32 num;
			lastScanSet = subSetIndex;
			linearCoeff[2 * subSetIndex + 0] = d0;
			linearCoeff[2 * subSetIndex + 1] = d1;

			// Count number of bits set in sigmap (Hamming weight)
			num = sigmap;
			num = (num)-((num >> 1) & 0x5555);
			num = (num & 0x3333) + ((num >> 2) & 0x3333);
			num = (num + (num >> 4)) & 0x0f0f;
			num = (num + (num >> 8)) & 0x1f;

			numNonZeroCoeffs -= num;
			if (numNonZeroCoeffs == 0)
			{
				break;
			}
		}

		subSetIndex++;

	}

	//-------------------------------------------------------------------------------------------------------------------
	// Obtain the last significant X and Y positions and compute their bit cost

	// subblock position
	coeffGroupPosition = sbScans[logBlockSize - 2][lastScanSet];
	coeffGroupPositionY = coeffGroupPosition >> 4;
	coeffGroupPositionX = coeffGroupPosition & 15;
	lastSigYPos = 4 * coeffGroupPositionY;
	lastSigXPos = 4 * coeffGroupPositionX;
	scanPosLast = 16 * lastScanSet;

	// position within subblock
	posLast = Log2f(subblockSigmap[lastScanSet]);
	coeffGroupPosition = scans4[scanIndex != SCAN_DIAG2][posLast];
	lastSigYPos += coeffGroupPosition >> 2;
	lastSigXPos += coeffGroupPosition & 3;
	scanPosLast += posLast;

	// Should swap row/col for SCAN_HOR and SCAN_VER:
	// - SCAN_HOR because the subscan order is mirrored (compared to SCAN_DIAG)
	// - SCAN_VER because of equation (7-66) in H.265 (04/2013)
	// Note that the scans4 array is adjusted to reflect this
	if (scanIndex != SCAN_DIAG2)
	{
		SWAP(lastSigXPos, lastSigYPos);
	}

	// Encode the position of last significant coefficient
	EncodeLastSignificantXY(cabacEncodeCtxPtr, lastSigXPos, lastSigYPos, size, logBlockSize, isChroma);

	//-------------------------------------------------------------------------------------------------------------------
	// Encode Coefficient levels

	significantFlagContextOffset = (!isChroma) ? 0 : NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS;

	contextOffset1 = 1;

	// Memory for 4x4 subblock-level significance flag
	// Bits 0-7 are for current diagonal
	// Bits 16-23 are for previous diagonal
	sbSigMemory = 0;

	// Subblock diagonal index for previous 4x4 subblock (diagonal index is row+col)
	sbPrevDiagIdx = -1;

	// Loop over subblocks
	subSetIndex = lastScanSet;
	do
	{
		// 1. Subblock-level significance flag

		// Assign default value that works for first and last subblock
		EB_S32 significantFlagContextPattern = sbSigMemory & 3;

		if (subSetIndex != 0)
		{
			coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
			coeffGroupPositionY = coeffGroupPosition >> 4;
			coeffGroupPositionX = coeffGroupPosition & 15;

			sbDiagIdx = coeffGroupPositionY + coeffGroupPositionX;
			if (sbDiagIdx != sbPrevDiagIdx)
			{
				sbSigMemory = sbSigMemory << 16;
				sbPrevDiagIdx = sbDiagIdx;
			}

			if (subSetIndex != lastScanSet)
			{
				significantFlagContextPattern = (sbSigMemory >> (16 + coeffGroupPositionY)) & 3;
				sigCoeffGroupContextIndex = significantFlagContextPattern != 0;
				sigCoeffGroupContextIndex += isChroma * NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS;

				significanceFlag = (subblockSigmap[subSetIndex] != 0);

				// Add bits for coded_sub_block_flag
				EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), significanceFlag,
					&(cabacEncodeCtxPtr->contextModelEncContext.coeffGroupSigFlagContextModel[sigCoeffGroupContextIndex]));

				if (!significanceFlag)
				{
					// Nothing else to do for this subblock since all coefficients in it are zero
					continue;
				}
			}

			// Memorize that current subblock is nonzero
			sbSigMemory += 1 << coeffGroupPositionY;
		}

		// 2. Coefficient significance flags
		numNonZero = 0; // Number of nonzero coefficients in current subblock
		signFlags = 0;

		// Use do {} while(0) loop to avoid goto statement (early exit)
		do
		{
			EB_S32 scanPosSig;
			EB_S32 sigMap = subblockSigmap[subSetIndex];
			EB_S32 subPosition = subSetIndex << LOG2_SCAN_SET_SIZE;
			EB_S32 subPosition2 = subPosition;

			if (subSetIndex == lastScanSet)
			{
				int val = linearCoeffBufferPtr[scanPosLast];
				int sign = val >> 31;
				signFlags = -sign;
				absCoeff[0] = (val ^ sign) - sign;

				numNonZero = 1;
				if (sigMap == 1)
				{
					// Nothing else to do here (only "DC" coeff within subblock)
					break;
				}
				scanPosSig = scanPosLast - 1;
				sigMap <<= 31 - (scanPosSig & 15);
			}
			else
			{
				if (sigMap == 1 && subSetIndex != 0)
				{
					subPosition2++;
					val = linearCoeffBufferPtr[subPosition];
					sign = val >> 31;
					signFlags = -sign;
					absCoeff[0] = (val ^ sign) - sign;
					numNonZero = 1;
				}
				scanPosSig = subPosition + 15;
				sigMap <<= 16;
			}

			if (logBlockSize == 2)
			{
				tempOffset = 0;
				contextIndexMapPtr = contextIndexMap4[scanIndex];
			}
			else
			{
				tempOffset = (logBlockSize == 3) ? ((scanIndex == SCAN_DIAG2 || isChroma) ? 9 : 15) : (!isChroma ? 21 : 12);
				tempOffset += (!isChroma && subSetIndex != 0) ? 3 : 0;
				contextIndexMapPtr = contextIndexMap8[scanIndex != SCAN_DIAG2][significantFlagContextPattern] - subPosition;
			}

			// Loop over coefficients
			do
			{
				EB_U32 sigContextIndex;
				EB_S32 sigCoeffFlag = sigMap < 0;

				sigContextIndex = (scanPosSig == 0) ? 0 : contextIndexMapPtr[scanPosSig] + tempOffset;

				// Add bits for sig_coeff_flag
				EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), sigCoeffFlag,
					&(cabacEncodeCtxPtr->contextModelEncContext.significanceFlagContextModel[significantFlagContextOffset + sigContextIndex]));

				if (sigCoeffFlag)
				{
					int val = scanPosSig >= 0 ? linearCoeffBufferPtr[scanPosSig] : 0;
					int sign = val >> 31;
					signFlags += signFlags - sign;
					absCoeff[numNonZero] = (val ^ sign) - sign;
					numNonZero++;
				}

				sigMap <<= 1;
				scanPosSig--;
			} while (scanPosSig >= subPosition2);
		} while (0);

		// 3. Coefficient level values

		golombRiceParam = 0;

		contextSet = (subSetIndex != 0 && !isChroma) ? 2 : 0;
		contextSet += (contextOffset1 == 0);
		contextOffset1 = 1;
		contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS + 4 * contextSet;
		contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS + contextSet;

		numCoeffWithCodedGt1Flag = MIN(GREATER_THAN1_MAX_NUMBER, numNonZero);

		// Loop over coefficients until base value of Exp-Golomb coding changes
		// Base value changes after either
		// - 8th coefficient
		// - a coefficient larger than 1
		index2 = numCoeffWithCodedGt1Flag;

		for (index = 0; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for coeff_abs_level_greater1_flag
			EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), symbolValue,
				&(cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset + contextOffset1]));

			if (symbolValue)
			{
				index2 = index;
				contextOffset1 = 0; // Need to set because used in next subblock
				index++;
				break;
			}

			if (contextOffset1 < 3)
			{
				contextOffset1++;
			}
		}

		for (; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for >1 flag
			EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), symbolValue,
				&(cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset]));
		}

		index = index2;
		if (index < numCoeffWithCodedGt1Flag)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 2;

			// Add bits for coeff_abs_level_greater2_flag
			EncodeOneBin(&(cabacEncodeCtxPtr->bacEncContext), symbolValue,
				&(cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[contextOffset2]));
		}

		{
			EncodeBypassBins(&(cabacEncodeCtxPtr->bacEncContext), signFlags, numNonZero);
		}

		if (index < numCoeffWithCodedGt1Flag)
		{
			EB_S32 absVal = absCoeff[index];

			if (absVal >= 3)
			{
				// Golomb Rice parameter is known to be 0 here
				RemainingCoeffExponentialGolombCode(cabacEncodeCtxPtr, absVal - 3, 0);
				golombRiceParam = (absVal > 3);
			}
			index++;

			// Loop over coefficients after first one that was > 1 but before 8th one
			// Base value is know to be equal to 2
			for (; index < numCoeffWithCodedGt1Flag; index++)
			{
				EB_S32 absVal = absCoeff[index];
				if (absVal >= 2)
				{
					RemainingCoeffExponentialGolombCode(cabacEncodeCtxPtr, absVal - 2, golombRiceParam);
					if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
				}
			}
		}

		// Loop over remaining coefficients (8th and beyond)
		// Base value is known to be equal to 1
		for (; index < numNonZero; index++)
		{
			EB_S32 absVal = absCoeff[index];

			RemainingCoeffExponentialGolombCode(cabacEncodeCtxPtr, absVal - 1, golombRiceParam);

			// Update Golomb-Rice parameter
			if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
		}
	} while (--subSetIndex >= 0);

	return;
}


EB_ERRORTYPE EncodeLastSignificantXYTemp(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	EB_U32                  lastSigXPos,
	EB_U32                  lastSigYPos,
	const EB_U32            width,
	const EB_U32            height,
	const EB_U32            widthLog2,
	const EB_U32            heightLog2,
	const EB_U32            componentType,  // 0 = Luma, 1 = Chroma
	const EB_U32            scanIndex,
	EB_U64                 *coeffBits)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 xGroupIndex, yGroupIndex;
	EB_S32 xBlockSizeOffset, yBlockSizeOffset;
	EB_S32 xShift, yShift;
	EB_U32 contextIndex;
	EB_S32 index, groupCount;
	EB_U32 contextOffset = componentType*NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS;

	xBlockSizeOffset = componentType ? 0 : ((widthLog2 - 2) * 3 + ((widthLog2 - 1) >> 2));
	yBlockSizeOffset = componentType ? 0 : ((heightLog2 - 2) * 3 + ((heightLog2 - 1) >> 2));
	xShift = componentType ? (widthLog2 - 2) : ((widthLog2 + 1) >> 2);
	yShift = componentType ? (heightLog2 - 2) : ((heightLog2 + 1) >> 2);

	if (scanIndex == SCAN_VER2) {
		SWAP(lastSigXPos, lastSigYPos);
	}
	xGroupIndex = lastSigXYGroupIndex[lastSigXPos];
	yGroupIndex = lastSigXYGroupIndex[lastSigYPos];

	// X position
	for (contextIndex = 0; contextIndex < xGroupIndex; contextIndex++) {

		*coeffBits += CabacEstimatedBits[1 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[contextOffset + xBlockSizeOffset + (contextIndex >> xShift)]];
	}

	if (xGroupIndex < lastSigXYGroupIndex[width - 1]) {

		*coeffBits += CabacEstimatedBits[0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[contextOffset + xBlockSizeOffset + (contextIndex >> xShift)]];
	}

	// Y position
	for (contextIndex = 0; contextIndex < yGroupIndex; contextIndex++) {

		*coeffBits += CabacEstimatedBits[1 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[contextOffset + yBlockSizeOffset + (contextIndex >> yShift)]];
	}

	if (yGroupIndex < lastSigXYGroupIndex[height - 1]) {

		*coeffBits += CabacEstimatedBits[0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[contextOffset + yBlockSizeOffset + (contextIndex >> yShift)]];
	}

	if (xGroupIndex > 3) {
		groupCount = (xGroupIndex - 2) >> 1;
		lastSigXPos = lastSigXPos - minInSigXYGroup[xGroupIndex];

		for (index = groupCount - 1; index >= 0; index--){

			*coeffBits += 32768;
		}
	}

	if (yGroupIndex > 3) {
		groupCount = (yGroupIndex - 2) >> 1;
		lastSigYPos = lastSigYPos - minInSigXYGroup[yGroupIndex];

		for (index = groupCount - 1; index >= 0; index--){

			*coeffBits += 32768;
		}
	}

	return return_error;
}

EB_ERRORTYPE RemainingCoeffExponentialGolombCodeTemp(
	EB_U32                  symbolValue,
	EB_U32                 *golombParamPtr,
	EB_U64                *coeffBits)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_S32 codeWord = (EB_S32)symbolValue;
	EB_U32 numberOfBins;

	//if (codeWord < (8 << (*golombParamPtr) ))
	if (codeWord < (COEF_REMAIN_BIN_REDUCTION << (*golombParamPtr)))
	{
		numberOfBins = codeWord >> (*golombParamPtr);
		*coeffBits += 32768 * (numberOfBins + 1);

		*coeffBits += 32768 * (*golombParamPtr);

	}
	else
	{
		numberOfBins = (*golombParamPtr);
		//codeWord  = codeWord - ( 8 << ((*golombParamPtr)));
		codeWord = codeWord - (COEF_REMAIN_BIN_REDUCTION << ((*golombParamPtr)));
		while (codeWord >= (1 << numberOfBins))
		{
			codeWord -= (1 << (numberOfBins++));
		}

		//*coeffBits += 32768*(8+numberOfBins+1-*golombParamPtr);
		*coeffBits += 32768 * (COEF_REMAIN_BIN_REDUCTION + numberOfBins + 1 - *golombParamPtr);

		*coeffBits += 32768 * numberOfBins;

	}

	return return_error;
}

EB_U32 EstimateLastSignificantXY_UPDATE(
    CoeffCtxtMdl_t         *updatedCoeffCtxModel,
    CabacEncodeContext_t   *cabacEncodeCtxPtr,
    EB_U32                  lastSigXPos,
    EB_U32                  lastSigYPos,
    const EB_U32            size,
    const EB_U32            logBlockSize,
    const EB_U32            isChroma)
{
    EB_U32 coeffBits = 0;
    EB_U32 xGroupIndex, yGroupIndex;
    EB_S32 blockSizeOffset;
    EB_S32 shift;
    EB_U32 contextIndex;
    EB_S32 groupCount;

    EB_U32 * ctxModelPtr;

    (void)cabacEncodeCtxPtr;

    blockSizeOffset = isChroma ? NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize - 2) * 3 + ((logBlockSize - 1) >> 2));
    shift = isChroma ? (logBlockSize - 2) : ((logBlockSize + 1) >> 2);

    /*
    ctxModelPtr   = &(updatedCoeffCtxModel->xxxxxxxxxxxxx[ xxxxxxxxx ] );
    coeffBits += CabacEstimatedBits[ xxxxxxxxxxx ^ ctxModelPtr[0] ];
    ctxModelPtr[0]   = UPDATE_CONTEXT_MODEL(xxxxxxxxxx, ctxModelPtr );

    */


    // X position
    xGroupIndex = lastSigXYGroupIndex[lastSigXPos];

    for (contextIndex = 0; contextIndex < xGroupIndex; contextIndex++)
    {
        //CHKN coeffBits += CabacEstimatedBits[ 1 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[ blockSizeOffset + (contextIndex>>shift) ] ];

        ctxModelPtr = &(updatedCoeffCtxModel->lastSigXContextModel[blockSizeOffset + (contextIndex >> shift)]);
        coeffBits += CabacEstimatedBits[1 ^ ctxModelPtr[0]];
        ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(1, ctxModelPtr);
    }

    if (xGroupIndex < lastSigXYGroupIndex[size - 1])
    {
        //CHKN coeffBits += CabacEstimatedBits[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[ blockSizeOffset + (contextIndex>>shift) ] ];

        ctxModelPtr = &(updatedCoeffCtxModel->lastSigXContextModel[blockSizeOffset + (contextIndex >> shift)]);
        coeffBits += CabacEstimatedBits[0 ^ ctxModelPtr[0]];
        ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(0, ctxModelPtr);
    }

    if (xGroupIndex > 3)
    {
        groupCount = (xGroupIndex - 2) >> 1;
        coeffBits += groupCount * ONE_BIT;
    }


    // Y position
    yGroupIndex = lastSigXYGroupIndex[lastSigYPos];

    for (contextIndex = 0; contextIndex < yGroupIndex; contextIndex++)
    {
        //CHKN coeffBits += CabacEstimatedBits[ 1 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[ blockSizeOffset + (contextIndex>>shift) ] ];

        ctxModelPtr = &(updatedCoeffCtxModel->lastSigYContextModel[blockSizeOffset + (contextIndex >> shift)]);
        coeffBits += CabacEstimatedBits[1 ^ ctxModelPtr[0]];
        ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(1, ctxModelPtr);
    }

    if (yGroupIndex < lastSigXYGroupIndex[size - 1])
    {
        //CHKN coeffBits += CabacEstimatedBits[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[ blockSizeOffset + (contextIndex>>shift) ] ];

        ctxModelPtr = &(updatedCoeffCtxModel->lastSigYContextModel[blockSizeOffset + (contextIndex >> shift)]);
        coeffBits += CabacEstimatedBits[0 ^ ctxModelPtr[0]];
        ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(0, ctxModelPtr);
    }

    if (yGroupIndex > 3)
    {
        groupCount = (yGroupIndex - 2) >> 1;
        coeffBits += groupCount * ONE_BIT;
    }

    return coeffBits;
}

/*********************************************************************
* EstimateQuantizedCoefficients
*   Estimate quantized coefficients rate
*   This function is designed for square quantized coefficient blocks.
*   So, in NSQT is implemented, this function  should be modified
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to prediction unit
*
*  coeffBufferPtr
*   input pointer to the quantized coefficinet buffer
*
*  coeffStride
*   stride of the coefficient buffer
*
*  componentType
*   component type indicated by  0 = Luma, 1 = Chroma, 2 = Cb, 3 = Cr, 4 = ALL
*
*  numNonZeroCoeffs
*   indicates the number of non zero coefficients in the coefficient buffer
*********************************************************************/

/**********************************************************************
*
**********************************************************************/

EB_ERRORTYPE EstimateQuantizedCoefficients_SSE2(
	CabacCost_t                  *CabacCost,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
	EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 isChroma = componentType != COMPONENT_LUMA;
	EB_U32 logBlockSize = Log2f(size);
	EB_U32 coeffBits = 0;

	// DC-only fast track
	if (numNonZeroCoeffs == 1 && coeffBufferPtr[0] != 0)
	{
		EB_S32 blockSizeOffset;
		blockSizeOffset = isChroma ? NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize - 2) * 3 + ((logBlockSize - 1) >> 2));

		coeffBits += CabacEstimatedBits[0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[blockSizeOffset]];
		coeffBits += CabacEstimatedBits[0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[blockSizeOffset]];

		{
			EB_S32 absVal = ABS(coeffBufferPtr[0]);
			EB_U32 symbolValue = absVal > 1;
			EB_U32 contextOffset;
			EB_U32 contextOffset2;

			contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS;
			contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS;

			// Add bits for coeff_abs_level_greater1_flag
			coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset + 1]];

			if (symbolValue)
			{
				symbolValue = absVal > 2;

				// Add bits for coeff_abs_level_greater2_flag
				coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[contextOffset2]];

				if (symbolValue)
				{
					// Golomb Rice parameter is known to be 0 here
					coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 3, 0);
				}
			}
		}

		coeffBits += ONE_BIT; // Sign bit
		// Add local bit counter to global bit counter
		*coeffBitsLong += coeffBits;

		return return_error;
	}

	// Compute the scanning type
	// optimized this if statement later
  {
	  EB_U32 scanIndex = SCAN_DIAG2;

	  if (type == INTRA_MODE)
	  {
		  // The test on partition size should be commented out to get conformance for Intra 4x4 !
		  //if (partitionSize == SIZE_2Nx2N)
		  {
			  // note that for Intra 2Nx2N, each CU is one PU. So this mode is the same as
			  // the mode of upper-left corner of current CU
			  //intraLumaMode   = candidatePtr->intraLumaMode[0];
			  //intraChromaMode = candidatePtr->intraChromaMode[0];

			  if (logBlockSize <= 3 - isChroma)
			  {
				  EB_U32 tempIntraChromaMode = chromaMappingTable[(EB_U32)intraChromaMode];
				  EB_S32 intraMode = (!isChroma || tempIntraChromaMode == EB_INTRA_CHROMA_DM) ? (EB_S32)intraLumaMode : (EB_S32)tempIntraChromaMode;

				  if (ABS(8 - ((intraMode - 2) & 15)) <= 4)
				  {
					  scanIndex = (intraMode & 16) ? SCAN_HOR2 : SCAN_VER2;
				  }
			  }
		  }
	  }

	  //-------------------------------------------------------------------------------------------------------------------
	  // Coefficients ordered according to scan order (absolute values)
	{
        __m128i linearCoeff[MAX_TU_SIZE * MAX_TU_SIZE / (sizeof(__m128i) / sizeof(EB_U16))] = {{0}};
		EB_U16 *linearCoeffBufferPtr = (EB_U16 *)linearCoeff;

		// Significance map for each 4x4 subblock
		// 1 bit per coefficient
		// i-th bit corresponds to i-th coefficient in forward scan order
		EB_U16 subblockSigmap[MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4)];

		EB_S32 lastScanSet = -1;
		EB_S32 subSetIndex;
		EB_U32 coeffGroupPosition;
		EB_S32 coeffGroupPositionY;
		EB_S32 coeffGroupPositionX;

		// Loop through subblocks to reorder coefficients according to scan order
		// Also derive significance map for each subblock, and determine last subblock that contains nonzero coefficients
		subSetIndex = 0;
		while (1)
		{
			// determine position of subblock within transform block
			coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
			coeffGroupPositionY = coeffGroupPosition >> 4;
			coeffGroupPositionX = coeffGroupPosition & 15;

			if (scanIndex == SCAN_HOR2)
			{
				// Subblock scan is mirrored for horizontal scan
				SWAP(coeffGroupPositionX, coeffGroupPositionY);
			}

		{
			EB_S16 *subblockPtr = coeffBufferPtr + 4 * coeffGroupPositionY * coeffStride + 4 * coeffGroupPositionX;
			__m128i a0, a1, a2, a3;
			__m128i b0, b1, c0, c1, d0, d1;

			a0 = _mm_loadl_epi64((__m128i *)(subblockPtr + 0 * coeffStride)); // 00 01 02 03 -- -- -- --
			a1 = _mm_loadl_epi64((__m128i *)(subblockPtr + 1 * coeffStride)); // 10 11 12 13 -- -- -- --
			a2 = _mm_loadl_epi64((__m128i *)(subblockPtr + 2 * coeffStride)); // 20 21 22 23 -- -- -- --
			a3 = _mm_loadl_epi64((__m128i *)(subblockPtr + 3 * coeffStride)); // 30 31 32 33 -- -- -- --

			if (scanIndex == SCAN_DIAG2)
			{
				b0 = _mm_unpacklo_epi64(a0, a3); // 00 01 02 03 30 31 32 33
				b1 = _mm_unpacklo_epi16(a1, a2); // 10 20 11 21 12 22 13 23

				c0 = _mm_unpacklo_epi16(b0, b1); // 00 10 01 20 02 11 03 21
				c1 = _mm_unpackhi_epi16(b1, b0); // 12 30 22 31 13 32 23 33

				{
					int v03 = _mm_extract_epi16(a0, 3);
					int v30 = _mm_extract_epi16(a3, 0);

					d0 = _mm_shufflehi_epi16(c0, 0xe1); // 00 10 01 20 11 02 03 21
					d1 = _mm_shufflelo_epi16(c1, 0xb4); // 12 30 31 22 13 32 23 33

					d0 = _mm_insert_epi16(d0, v30, 6); // 00 10 01 20 11 02 30 21
					d1 = _mm_insert_epi16(d1, v03, 1); // 12 03 31 22 13 32 23 33
				}
			}
			else if (scanIndex == SCAN_HOR2)
			{
				d0 = _mm_unpacklo_epi64(a0, a1); // 00 01 02 03 10 11 12 13
				d1 = _mm_unpacklo_epi64(a2, a3); // 20 21 22 23 30 31 32 33
			}
			else
			{
				b0 = _mm_unpacklo_epi16(a0, a2); // 00 20 01 21 02 22 03 23
				b1 = _mm_unpacklo_epi16(a1, a3); // 10 30 11 31 12 32 13 33

				d0 = _mm_unpacklo_epi16(b0, b1); // 00 10 20 30 01 11 21 31
				d1 = _mm_unpackhi_epi16(b0, b1); // 02 12 22 32 03 13 23 33
			}

		  {
			  __m128i z0;
			  z0 = _mm_packs_epi16(d0, d1);
			  z0 = _mm_cmpeq_epi8(z0, _mm_setzero_si128());

			  {
				  __m128i s0, s1;
				  // Absolute value (note: _mm_abs_epi16 requires SSSE3)
				  s0 = _mm_srai_epi16(d0, 15);
				  s1 = _mm_srai_epi16(d1, 15);
				  d0 = _mm_sub_epi16(_mm_xor_si128(d0, s0), s0);
				  d1 = _mm_sub_epi16(_mm_xor_si128(d1, s1), s1);
			  }

			{
				EB_S32 sigmap = _mm_movemask_epi8(z0) ^ 0xffff;
				subblockSigmap[subSetIndex] = (EB_U16)sigmap;

				if (sigmap != 0)
				{
					lastScanSet = subSetIndex;
					linearCoeff[2 * subSetIndex + 0] = d0;
					linearCoeff[2 * subSetIndex + 1] = d1;

					// Count number of bits set in sigmap (Hamming weight)
					{
						EB_U32 num;
						num = sigmap;
						num = (num)-((num >> 1) & 0x5555);
						num = (num & 0x3333) + ((num >> 2) & 0x3333);
						num = (num + (num >> 4)) & 0x0f0f;
						num = (num + (num >> 8)) & 0x1f;

						numNonZeroCoeffs -= num;
						if (numNonZeroCoeffs == 0)
						{
							break;
						}
					}
				}
			}
		  }
		}

		subSetIndex++;

		}

		//-------------------------------------------------------------------------------------------------------------------
		// Obtain the last significant X and Y positions and compute their bit cost
	  {
		  EB_U32 posLast;
		  EB_S32 scanPosLast;
		  EB_S32 lastSigXPos;
		  EB_S32 lastSigYPos;

		  // subblock position
		  coeffGroupPosition = sbScans[logBlockSize - 2][lastScanSet];
		  coeffGroupPositionY = coeffGroupPosition >> 4;
		  coeffGroupPositionX = coeffGroupPosition & 15;
		  lastSigYPos = 4 * coeffGroupPositionY;
		  lastSigXPos = 4 * coeffGroupPositionX;
		  scanPosLast = 16 * lastScanSet;

		  // position within subblock
		  posLast = Log2f(subblockSigmap[lastScanSet]);
		  coeffGroupPosition = scans4[scanIndex != SCAN_DIAG2][posLast];
		  lastSigYPos += coeffGroupPosition >> 2;
		  lastSigXPos += coeffGroupPosition & 3;
		  scanPosLast += posLast;

		  // Should swap row/col for SCAN_HOR and SCAN_VER:
		  // - SCAN_HOR because the subscan order is mirrored (compared to SCAN_DIAG)
		  // - SCAN_VER because of equation (7-66) in H.265 (04/2013)
		  // Note that the scans4 array is adjusted to reflect this
		  if (scanIndex != SCAN_DIAG2)
		  {
			  SWAP(lastSigXPos, lastSigYPos);
		  }

		  // Encode the position of last significant coefficient
		  coeffBits += EstimateLastSignificantXY(cabacEncodeCtxPtr, lastSigXPos, lastSigYPos, size, logBlockSize, isChroma);

		  //-------------------------------------------------------------------------------------------------------------------
		  // Encode Coefficient levels
		  {
			  EB_U32 significantFlagContextOffset = (!isChroma) ? 0 : NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS;

			  EB_U32 contextOffset1 = 1;

			  // Memory for 4x4 subblock-level significance flag
			  // Bits 0-7 are for current diagonal
			  // Bits 16-23 are for previous diagonal
			  EB_U32 sbSigMemory = 0;

			  // Subblock diagonal index for previous 4x4 subblock (diagonal index is row+col)
			  EB_S32 sbPrevDiagIdx = -1;

			  // Loop over subblocks
			  subSetIndex = lastScanSet;
			  do
			  {
				  // 1. Subblock-level significance flag

				  // Assign default value that works for first and last subblock
				  EB_S32 significantFlagContextPattern = sbSigMemory & 3;

				  if (subSetIndex != 0)
				  {
					  coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
					  coeffGroupPositionY = coeffGroupPosition >> 4;
					  coeffGroupPositionX = coeffGroupPosition & 15;

					  {
						  EB_S32 sbDiagIdx = coeffGroupPositionY + coeffGroupPositionX;
						  if (sbDiagIdx != sbPrevDiagIdx)
						  {
							  sbSigMemory = sbSigMemory << 16;
							  sbPrevDiagIdx = sbDiagIdx;
						  }

						  if (subSetIndex != lastScanSet)
						  {
							  EB_U32 sigCoeffGroupContextIndex;
							  EB_U32 significanceFlag;

							  significantFlagContextPattern = (sbSigMemory >> (16 + coeffGroupPositionY)) & 3;
							  sigCoeffGroupContextIndex = significantFlagContextPattern != 0;
							  sigCoeffGroupContextIndex += isChroma * NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS;

							  significanceFlag = (subblockSigmap[subSetIndex] != 0);

							  // Add bits for coded_sub_block_flag
							  coeffBits += CabacEstimatedBits[significanceFlag ^ cabacEncodeCtxPtr->contextModelEncContext.coeffGroupSigFlagContextModel[sigCoeffGroupContextIndex]];

							  if (!significanceFlag)
							  {
								  // Nothing else to do for this subblock since all coefficients in it are zero
								  continue;
							  }
						  }

						  // Memorize that current subblock is nonzero
						  sbSigMemory += 1 << coeffGroupPositionY;
					  }
				  }

				  // 2. Coefficient significance flags
			{
				EB_S32 numNonZero = 0; // Number of nonzero coefficients in current subblock
				EB_S32 absCoeff[16] = { 0 }; // Array containing list of nonzero coefficients (size given by numNonZero)

				// Use do {} while(0) loop to avoid goto statement (early exit)
				do
				{
					EB_S32 scanPosSig;
					EB_S32 sigMap = subblockSigmap[subSetIndex];
					EB_S32 subPosition = subSetIndex << LOG2_SCAN_SET_SIZE;
					EB_S32 subPosition2 = subPosition;

					if (subSetIndex == lastScanSet)
					{
						absCoeff[0] = linearCoeffBufferPtr[scanPosLast];
						numNonZero = 1;
						if (sigMap == 1)
						{
							// Nothing else to do here (only "DC" coeff within subblock)
							break;
						}
						scanPosSig = scanPosLast - 1;
						sigMap <<= 31 - (scanPosSig & 15);
					}
					else
					{
						if (sigMap == 1 && subSetIndex != 0)
						{
							subPosition2++;
							absCoeff[0] = linearCoeffBufferPtr[subPosition];
							numNonZero = 1;
						}
						scanPosSig = subPosition + 15;
						sigMap <<= 16;
					}

				{
					EB_U32 tempOffset;
					const EB_U8 *contextIndexMapPtr;

					if (logBlockSize == 2)
					{
						tempOffset = 0;
						contextIndexMapPtr = contextIndexMap4[scanIndex];
					}
					else
					{
						tempOffset = (logBlockSize == 3) ? (scanIndex == SCAN_DIAG2 ? 9 : 15) : (!isChroma ? 21 : 12);
						tempOffset += (!isChroma && subSetIndex != 0) ? 3 : 0;
						contextIndexMapPtr = contextIndexMap8[scanIndex != SCAN_DIAG2][significantFlagContextPattern] - subPosition;
					}

					// Loop over coefficients
					do
					{
						EB_U32 sigContextIndex;
						EB_S32 sigCoeffFlag = sigMap < 0;

						sigContextIndex = (scanPosSig == 0) ? 0 : contextIndexMapPtr[scanPosSig] + tempOffset;

						// Add bits for sig_coeff_flag
						coeffBits += CabacEstimatedBits[sigCoeffFlag ^ cabacEncodeCtxPtr->contextModelEncContext.significanceFlagContextModel[significantFlagContextOffset + sigContextIndex]];

						if (sigCoeffFlag)
						{
							absCoeff[numNonZero] = scanPosSig >= 0 ? linearCoeffBufferPtr[scanPosSig] : 0;
							numNonZero++;
						}

						sigMap <<= 1;
						scanPosSig--;
					} while (scanPosSig >= subPosition2);
				}
				} while (0);

				// 3. Coefficient level values
				{
					EB_U32 golombRiceParam = 0;
					EB_S32 index;
					EB_U32 contextSet;
					EB_S32 numCoeffWithCodedGt1Flag; // Number of coefficients for which >1 flag is coded
					EB_U32 contextOffset;
					EB_U32 contextOffset2;

					contextSet = (subSetIndex != 0 && !isChroma) ? 2 : 0;
					contextSet += (contextOffset1 == 0);
					contextOffset1 = 1;
					contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS + 4 * contextSet;
					contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS + contextSet;

					numCoeffWithCodedGt1Flag = MIN(GREATER_THAN1_MAX_NUMBER, numNonZero);

					{
						coeffBits += ONE_BIT * numNonZero; // Add bits for coeff_sign_flag (all coefficients in subblock)
					}

					// Loop over coefficients until base value of Exp-Golomb coding changes
					// Base value changes after either
					// - 8th coefficient
					// - a coefficient larger than 1                

					for (index = 0; index < numCoeffWithCodedGt1Flag; index++)
					{
						EB_S32 absVal = absCoeff[index];
						EB_U32 symbolValue = absVal > 1;

						// Add bits for coeff_abs_level_greater1_flag
						coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset + contextOffset1]];

						if (symbolValue)
						{
							symbolValue = absVal > 2;

							// Add bits for coeff_abs_level_greater2_flag
							coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[contextOffset2]];

							if (symbolValue)
							{
								// Golomb Rice parameter is known to be 0 here
								coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 3, 0);
								golombRiceParam = (absVal > 3);
							}

							index++;
							contextOffset1 = 0;

							// Exit loop early as remaining coefficients are coded differently
							break;
						}

						if (contextOffset1 < 3)
						{
							contextOffset1++;
						}
					}

					// Loop over coefficients after first one that was > 1 but before 8th one
					// Base value is know to be equal to 2
					for (; index < numCoeffWithCodedGt1Flag; index++)
					{
						EB_S32 absVal = absCoeff[index];
						EB_U32 symbolValue = absVal > 1;

						// Add bits for >1 flag
						coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset]];

						if (symbolValue)
						{
							coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 2, golombRiceParam);

							// Update Golomb-Rice parameter
							if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
						}
					}

					// Loop over remaining coefficients (8th and beyond)
					// Base value is known to be equal to 1
					for (; index < numNonZero; index++)
					{
						EB_S32 absVal = absCoeff[index];

						coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 1, golombRiceParam);

						// Update Golomb-Rice parameter
						if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
					}
				}
			}
			  } while (--subSetIndex >= 0);
		  }
	  }
	}
  }

  // Add local bit counter to global bit counter
  *coeffBitsLong += coeffBits;
  (void)CabacCost;
  return return_error;
}

EB_ERRORTYPE EstimateQuantizedCoefficients_generic_Update(
	CoeffCtxtMdl_t  *UpdatedCoeffCtxModel,
	CabacCost_t                  *CabacCost,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
	EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 isChroma = componentType != COMPONENT_LUMA;
	EB_U32 logBlockSize = Log2f(size);
	EB_U32 coeffBits = 0;

	EB_S32 absVal;
	EB_U32 symbolValue;
	EB_U32 contextOffset;
	EB_U32 contextOffset2;

	EB_U32 scanIndex;

	EB_U16 linearCoeff[MAX_TU_SIZE * MAX_TU_SIZE];
	EB_U16 *linearCoeffBufferPtr = linearCoeff;

	EB_U16 subblockSigmap[MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4)];

	EB_S32 lastScanSet = -1;
	EB_S32 subSetIndex;
	EB_U32 coeffGroupPosition;
	EB_S32 coeffGroupPositionY;
	EB_S32 coeffGroupPositionX;

	EB_S16 *subblockPtr;

	EB_S16 k;
	EB_S32 sigmap;
	EB_U32 num;

	EB_U32 val;
	EB_U32 isNonZero;

	EB_U32 posLast;
	EB_S32 scanPosLast;
	EB_S32 lastSigXPos;
	EB_S32 lastSigYPos;

	EB_U32 significantFlagContextOffset;

	EB_U32 contextOffset1;

	EB_S32 numNonZero; // Number of nonzero coefficients in current subblock
	EB_S32 absCoeff[16] = { 0 }; // Array containing list of nonzero coefficients (size given by numNonZero)

	// Memory for 4x4 subblock-level significance flag
	// Bits 0-7 are for current diagonal
	// Bits 16-23 are for previous diagonal
	EB_U32 sbSigMemory;

	// Subblock diagonal index for previous 4x4 subblock (diagonal index is row+col)
	EB_S32 sbPrevDiagIdx;
	EB_S32 sbDiagIdx;

	EB_U32 tempOffset;
	const EB_U8 *contextIndexMapPtr;

	EB_U32 golombRiceParam;
	EB_S32 index;
	EB_U32 contextSet;
	EB_S32 numCoeffWithCodedGt1Flag; // Number of coefficients for which >1 flag is coded

	EB_U32 * ctxModelPtr;

	// DC-only fast track
	if (numNonZeroCoeffs == 1 && coeffBufferPtr[0] != 0)
	{
		EB_S32 blockSizeOffset;
		blockSizeOffset = isChroma ? NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize - 2) * 3 + ((logBlockSize - 1) >> 2));


		//CHKN coeffBits += CabacEstimatedBits[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[ blockSizeOffset ] ];

		ctxModelPtr = &(UpdatedCoeffCtxModel->lastSigXContextModel[blockSizeOffset]);
		coeffBits += CabacEstimatedBits[0 ^ ctxModelPtr[0]];
		ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(0, ctxModelPtr);

		//CHKN coeffBits += CabacEstimatedBits[ 0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[ blockSizeOffset ] ];
		ctxModelPtr = &(UpdatedCoeffCtxModel->lastSigYContextModel[blockSizeOffset]);
		coeffBits += CabacEstimatedBits[0 ^ ctxModelPtr[0]];
		ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(0, ctxModelPtr);


		absVal = ABS(coeffBufferPtr[0]);
		symbolValue = absVal > 1;

		contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS;
		contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS;

		// Add bits for coeff_abs_level_greater1_flag
		//CHKN coeffBits += CabacEstimatedBits[ symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[ contextOffset+1 ] ];
		ctxModelPtr = &(UpdatedCoeffCtxModel->greaterThanOneContextModel[contextOffset + 1]);
		coeffBits += CabacEstimatedBits[symbolValue ^ ctxModelPtr[0]];
		ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(symbolValue, ctxModelPtr);

		/*********************************************************************************

		ctxModelPtr      = &(UpdatedCoeffCtxModel->xxxxxxxxxxxxx[ xxxxxxxxx ] );
		coeffBits       += CabacEstimatedBits[ xxxxxxxxxxx ^ ctxModelPtr[0] ];
		ctxModelPtr[0]   = UPDATE_CONTEXT_MODEL(xxxxxxxxxx, ctxModelPtr );

		*********************************************************************************/

		if (symbolValue)
		{
			symbolValue = absVal > 2;

			// Add bits for coeff_abs_level_greater2_flag
			//CHKN coeffBits += CabacEstimatedBits[ symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[ contextOffset2 ] ];

			ctxModelPtr = &(UpdatedCoeffCtxModel->greaterThanTwoContextModel[contextOffset2]);
			coeffBits += CabacEstimatedBits[symbolValue ^ ctxModelPtr[0]];
			ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(symbolValue, ctxModelPtr);

			if (symbolValue)
			{
				// Golomb Rice parameter is known to be 0 here
				coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 3, 0);
			}
		}

		coeffBits += ONE_BIT; // Sign bit
		// Add local bit counter to global bit counter
		*coeffBitsLong += coeffBits;

		return return_error;
	}

	// Compute the scanning type
	// optimized this if statement later
	scanIndex = SCAN_DIAG2;

	if (type == INTRA_MODE)
	{
		// The test on partition size should be commented out to get conformance for Intra 4x4 !
		//if (partitionSize == SIZE_2Nx2N)
		{
			// note that for Intra 2Nx2N, each CU is one PU. So this mode is the same as
			// the mode of upper-left corner of current CU
			//intraLumaMode   = candidatePtr->intraLumaMode[0];
			//intraChromaMode = candidatePtr->intraChromaMode[0];

			if (logBlockSize <= 3 - isChroma)
			{
				EB_S32 tempIntraChromaMode = chromaMappingTable[intraChromaMode];
				EB_S32 intraMode = (!isChroma || tempIntraChromaMode == EB_INTRA_CHROMA_DM) ? (EB_S32)intraLumaMode : tempIntraChromaMode;

				if (ABS(8 - ((intraMode - 2) & 15)) <= 4)
				{
					scanIndex = (intraMode & 16) ? SCAN_HOR2 : SCAN_VER2;
				}
			}
		}
	}

	//-------------------------------------------------------------------------------------------------------------------
	// Coefficients ordered according to scan order (absolute values)
	// Significance map for each 4x4 subblock
	// 1 bit per coefficient
	// i-th bit corresponds to i-th coefficient in forward scan order

	// Loop through subblocks to reorder coefficients according to scan order
	// Also derive significance map for each subblock, and determine last subblock that contains nonzero coefficients
	subSetIndex = 0;
	while (1)
	{
		// determine position of subblock within transform block
		coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
		coeffGroupPositionY = coeffGroupPosition >> 4;
		coeffGroupPositionX = coeffGroupPosition & 15;

		if (scanIndex == SCAN_HOR2)
		{
			// Subblock scan is mirrored for horizontal scan
			SWAP(coeffGroupPositionX, coeffGroupPositionY);
		}

		subblockPtr = coeffBufferPtr + 4 * coeffGroupPositionY * coeffStride + 4 * coeffGroupPositionX;

		sigmap = 0;
		num = 0;

		for (k = 0; k < 16; k++)
		{
			EB_U32 position = scans4[scanIndex != SCAN_DIAG2][k];
			EB_U32 positionY = position >> 2;
			EB_U32 positionX = position & 3;
			if (scanIndex == SCAN_HOR2)
			{
				// Subblock scan is mirrored for horizontal scan
				SWAP(positionX, positionY);
			}

			val = ABS(subblockPtr[coeffStride * positionY + positionX]);
			linearCoeff[16 * subSetIndex + k] = (EB_U16)val;
			isNonZero = val != 0;
			num += isNonZero;
			sigmap |= isNonZero << k;
		}

		subblockSigmap[subSetIndex] = (EB_U16)sigmap;

		if (sigmap != 0)
		{
			lastScanSet = subSetIndex;

			numNonZeroCoeffs -= num;
			if (numNonZeroCoeffs == 0)
			{
				break;
			}
		}

		subSetIndex++;

	}

	//-------------------------------------------------------------------------------------------------------------------
	// Obtain the last significant X and Y positions and compute their bit cost

	// subblock position
	coeffGroupPosition = sbScans[logBlockSize - 2][lastScanSet];
	coeffGroupPositionY = coeffGroupPosition >> 4;
	coeffGroupPositionX = coeffGroupPosition & 15;
	lastSigYPos = 4 * coeffGroupPositionY;
	lastSigXPos = 4 * coeffGroupPositionX;
	scanPosLast = 16 * lastScanSet;

	// position within subblock
	posLast = Log2f(subblockSigmap[lastScanSet]);
	coeffGroupPosition = scans4[scanIndex != SCAN_DIAG2][posLast];
	lastSigYPos += coeffGroupPosition >> 2;
	lastSigXPos += coeffGroupPosition & 3;
	scanPosLast += posLast;

	// Should swap row/col for SCAN_HOR and SCAN_VER:
	// - SCAN_HOR because the subscan order is mirrored (compared to SCAN_DIAG)
	// - SCAN_VER because of equation (7-66) in H.265 (04/2013)
	// Note that the scans4 array is adjusted to reflect this
	if (scanIndex != SCAN_DIAG2)
	{
		SWAP(lastSigXPos, lastSigYPos);
	}

	// Encode the position of last significant coefficient
	//CHKN coeffBits += EstimateLastSignificantXY(cabacEncodeCtxPtr, lastSigXPos, lastSigYPos, size, logBlockSize, isChroma);
	coeffBits += EstimateLastSignificantXY_UPDATE(UpdatedCoeffCtxModel, cabacEncodeCtxPtr, lastSigXPos, lastSigYPos, size, logBlockSize, isChroma);

	//-------------------------------------------------------------------------------------------------------------------
	// Encode Coefficient levels

	significantFlagContextOffset = (!isChroma) ? 0 : NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS;

	contextOffset1 = 1;

	// Memory for 4x4 subblock-level significance flag
	// Bits 0-7 are for current diagonal
	// Bits 16-23 are for previous diagonal
	sbSigMemory = 0;

	// Subblock diagonal index for previous 4x4 subblock (diagonal index is row+col)
	sbPrevDiagIdx = -1;

	// Loop over subblocks
	subSetIndex = lastScanSet;
	do
	{
		// 1. Subblock-level significance flag

		// Assign default value that works for first and last subblock
		EB_S32 significantFlagContextPattern = sbSigMemory & 3;

		if (subSetIndex != 0)
		{
			coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
			coeffGroupPositionY = coeffGroupPosition >> 4;
			coeffGroupPositionX = coeffGroupPosition & 15;

			sbDiagIdx = coeffGroupPositionY + coeffGroupPositionX;
			if (sbDiagIdx != sbPrevDiagIdx)
			{
				sbSigMemory = sbSigMemory << 16;
				sbPrevDiagIdx = sbDiagIdx;
			}

			if (subSetIndex != lastScanSet)
			{
				EB_U32 sigCoeffGroupContextIndex;
				EB_U32 significanceFlag;

				significantFlagContextPattern = (sbSigMemory >> (16 + coeffGroupPositionY)) & 3;
				sigCoeffGroupContextIndex = significantFlagContextPattern != 0;
				sigCoeffGroupContextIndex += isChroma * NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS;

				significanceFlag = (subblockSigmap[subSetIndex] != 0);

				// Add bits for coded_sub_block_flag
				//CHKN  coeffBits += CabacEstimatedBits[ significanceFlag ^ cabacEncodeCtxPtr->contextModelEncContext.coeffGroupSigFlagContextModel[ sigCoeffGroupContextIndex ] ];

				ctxModelPtr = &(UpdatedCoeffCtxModel->coeffGroupSigFlagContextModel[sigCoeffGroupContextIndex]);
				coeffBits += CabacEstimatedBits[significanceFlag ^ ctxModelPtr[0]];
				ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(significanceFlag, ctxModelPtr);


				if (!significanceFlag)
				{
					// Nothing else to do for this subblock since all coefficients in it are zero
					continue;
				}
			}

			// Memorize that current subblock is nonzero
			sbSigMemory += 1 << coeffGroupPositionY;
		}

		// 2. Coefficient significance flags

		numNonZero = 0; // Number of nonzero coefficients in current subblock

		// Use do {} while(0) loop to avoid goto statement (early exit)
		do
		{
			EB_S32 scanPosSig;
			EB_S32 sigMap = subblockSigmap[subSetIndex];
			EB_S32 subPosition = subSetIndex << LOG2_SCAN_SET_SIZE;
			EB_S32 subPosition2 = subPosition;

			if (subSetIndex == lastScanSet)
			{
				absCoeff[0] = linearCoeffBufferPtr[scanPosLast];
				numNonZero = 1;
				if (sigMap == 1)
				{
					// Nothing else to do here (only "DC" coeff within subblock)
					break;
				}
				scanPosSig = scanPosLast - 1;
				sigMap <<= 31 - (scanPosSig & 15);
			}
			else
			{
				if (sigMap == 1 && subSetIndex != 0)
				{
					subPosition2++;
					absCoeff[0] = linearCoeffBufferPtr[subPosition];
					numNonZero = 1;
				}
				scanPosSig = subPosition + 15;
				sigMap <<= 16;
			}

			if (logBlockSize == 2)
			{
				tempOffset = 0;
				contextIndexMapPtr = contextIndexMap4[scanIndex];
			}
			else
			{
				tempOffset = (logBlockSize == 3) ? (scanIndex == SCAN_DIAG2 ? 9 : 15) : (!isChroma ? 21 : 12);
				tempOffset += (!isChroma && subSetIndex != 0) ? 3 : 0;
				contextIndexMapPtr = contextIndexMap8[scanIndex != SCAN_DIAG2][significantFlagContextPattern] - subPosition;
			}

			// Loop over coefficients
			do
			{
				EB_U32 sigContextIndex;
				EB_S32 sigCoeffFlag = sigMap < 0;

				sigContextIndex = (scanPosSig == 0) ? 0 : contextIndexMapPtr[scanPosSig] + tempOffset;

				// Add bits for sig_coeff_flag
				//CHKN coeffBits += CabacEstimatedBits[ sigCoeffFlag ^ cabacEncodeCtxPtr->contextModelEncContext.significanceFlagContextModel[ significantFlagContextOffset+sigContextIndex ] ];

				ctxModelPtr = &(UpdatedCoeffCtxModel->significanceFlagContextModel[significantFlagContextOffset + sigContextIndex]);
				coeffBits += CabacEstimatedBits[sigCoeffFlag ^ ctxModelPtr[0]];
				ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(sigCoeffFlag, ctxModelPtr);

				if (sigCoeffFlag)
				{
					absCoeff[numNonZero] = scanPosSig >= 0 ? linearCoeffBufferPtr[scanPosSig] : 0;
					numNonZero++;
				}

				sigMap <<= 1;
				scanPosSig--;
			} while (scanPosSig >= subPosition2);
		} while (0);

		// 3. Coefficient level values
		golombRiceParam = 0;

		contextSet = (subSetIndex != 0 && !isChroma) ? 2 : 0;
		contextSet += (contextOffset1 == 0);
		contextOffset1 = 1;
		contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS + 4 * contextSet;
		contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS + contextSet;

		numCoeffWithCodedGt1Flag = MIN(GREATER_THAN1_MAX_NUMBER, numNonZero);

		{
			coeffBits += ONE_BIT * numNonZero; // Add bits for coeff_sign_flag (all coefficients in subblock)
		}

		// Loop over coefficients until base value of Exp-Golomb coding changes
		// Base value changes after either
		// - 8th coefficient
		// - a coefficient larger than 1

		for (index = 0; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for coeff_abs_level_greater1_flag
			//CHKN coeffBits += CabacEstimatedBits[ symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[ contextOffset+contextOffset1 ] ];


			ctxModelPtr = &(UpdatedCoeffCtxModel->greaterThanOneContextModel[contextOffset + contextOffset1]);
			coeffBits += CabacEstimatedBits[symbolValue ^ ctxModelPtr[0]];
			ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(symbolValue, ctxModelPtr);


			if (symbolValue)
			{
				symbolValue = absVal > 2;

				// Add bits for coeff_abs_level_greater2_flag
				//CHKN coeffBits += CabacEstimatedBits[ symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[ contextOffset2 ] ];

				ctxModelPtr = &(UpdatedCoeffCtxModel->greaterThanTwoContextModel[contextOffset2]);
				coeffBits += CabacEstimatedBits[symbolValue ^ ctxModelPtr[0]];
				ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(symbolValue, ctxModelPtr);


				if (symbolValue)
				{
					// Golomb Rice parameter is known to be 0 here
					coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 3, 0);
					golombRiceParam = (absVal > 3);
				}

				index++;
				contextOffset1 = 0;

				// Exit loop early as remaining coefficients are coded differently
				break;
			}

			if (contextOffset1 < 3)
			{
				contextOffset1++;
			}
		}

		// Loop over coefficients after first one that was > 1 but before 8th one
		// Base value is know to be equal to 2
		for (; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for >1 flag
			//CHKN  coeffBits += CabacEstimatedBits[ symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[ contextOffset ] ];

			ctxModelPtr = &(UpdatedCoeffCtxModel->greaterThanOneContextModel[contextOffset]);
			coeffBits += CabacEstimatedBits[symbolValue ^ ctxModelPtr[0]];
			ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(symbolValue, ctxModelPtr);


			if (symbolValue)
			{
				coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 2, golombRiceParam);

				// Update Golomb-Rice parameter
				if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
			}
		}

		// Loop over remaining coefficients (8th and beyond)
		// Base value is known to be equal to 1
		for (; index < numNonZero; index++)
		{
			EB_S32 absVal = absCoeff[index];

			coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 1, golombRiceParam);

			// Update Golomb-Rice parameter
			if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
		}
	} while (--subSetIndex >= 0);

	// Add local bit counter to global bit counter
	*coeffBitsLong += coeffBits;
	(void)CabacCost;
	return return_error;
}

EB_ERRORTYPE EstimateQuantizedCoefficients_generic(
	CabacCost_t                  *CabacCost,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
	EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32 isChroma = componentType != COMPONENT_LUMA;
	EB_U32 logBlockSize = Log2f(size);
	EB_U32 coeffBits = 0;

	EB_S32 absVal;
	EB_U32 symbolValue;
	EB_U32 contextOffset;
	EB_U32 contextOffset2;

	EB_U32 scanIndex;

	EB_U16 linearCoeff[MAX_TU_SIZE * MAX_TU_SIZE];
	EB_U16 *linearCoeffBufferPtr = linearCoeff;

	EB_U16 subblockSigmap[MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4)];

	EB_S32 lastScanSet = -1;
	EB_S32 subSetIndex;
	EB_U32 coeffGroupPosition;
	EB_S32 coeffGroupPositionY;
	EB_S32 coeffGroupPositionX;

	EB_S16 *subblockPtr;

	EB_S16 k;
	EB_S32 sigmap;
	EB_U32 num;

	EB_U32 val;
	EB_U32 isNonZero;

	EB_U32 posLast;
	EB_S32 scanPosLast;
	EB_S32 lastSigXPos;
	EB_S32 lastSigYPos;

	EB_U32 significantFlagContextOffset;

	EB_U32 contextOffset1;

	EB_S32 numNonZero; // Number of nonzero coefficients in current subblock
	EB_S32 absCoeff[16] = { 0 }; // Array containing list of nonzero coefficients (size given by numNonZero)

	// Memory for 4x4 subblock-level significance flag
	// Bits 0-7 are for current diagonal
	// Bits 16-23 are for previous diagonal
	EB_U32 sbSigMemory;

	// Subblock diagonal index for previous 4x4 subblock (diagonal index is row+col)
	EB_S32 sbPrevDiagIdx;
	EB_S32 sbDiagIdx;

	EB_U32 tempOffset;
	const EB_U8 *contextIndexMapPtr;

	EB_U32 golombRiceParam;
	EB_S32 index;
	EB_U32 contextSet;
	EB_S32 numCoeffWithCodedGt1Flag; // Number of coefficients for which >1 flag is coded

	// DC-only fast track
	if (numNonZeroCoeffs == 1 && coeffBufferPtr[0] != 0)
	{
		EB_S32 blockSizeOffset;
		blockSizeOffset = isChroma ? NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize - 2) * 3 + ((logBlockSize - 1) >> 2));

		coeffBits += CabacEstimatedBits[0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[blockSizeOffset]];
		coeffBits += CabacEstimatedBits[0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[blockSizeOffset]];

		absVal = ABS(coeffBufferPtr[0]);
		symbolValue = absVal > 1;

		contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS;
		contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS;

		// Add bits for coeff_abs_level_greater1_flag
		coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset + 1]];

		if (symbolValue)
		{
			symbolValue = absVal > 2;

			// Add bits for coeff_abs_level_greater2_flag
			coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[contextOffset2]];

			if (symbolValue)
			{
				// Golomb Rice parameter is known to be 0 here
				coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 3, 0);
			}
		}

		coeffBits += ONE_BIT; // Sign bit
		// Add local bit counter to global bit counter
		*coeffBitsLong += coeffBits;

		return return_error;
	}

	// Compute the scanning type
	// optimized this if statement later
	scanIndex = SCAN_DIAG2;

	if (type == INTRA_MODE)
	{
		// The test on partition size should be commented out to get conformance for Intra 4x4 !
		//if (partitionSize == SIZE_2Nx2N)
		{
			// note that for Intra 2Nx2N, each CU is one PU. So this mode is the same as
			// the mode of upper-left corner of current CU
			//intraLumaMode   = candidatePtr->intraLumaMode[0];
			//intraChromaMode = candidatePtr->intraChromaMode[0];

			if (logBlockSize <= 3 - isChroma)
			{
				EB_S32 tempIntraChromaMode = chromaMappingTable[intraChromaMode];
				EB_S32 intraMode = (!isChroma || tempIntraChromaMode == EB_INTRA_CHROMA_DM) ? (EB_S32)intraLumaMode : tempIntraChromaMode;

				if (ABS(8 - ((intraMode - 2) & 15)) <= 4)
				{
					scanIndex = (intraMode & 16) ? SCAN_HOR2 : SCAN_VER2;
				}
			}
		}
	}

	//-------------------------------------------------------------------------------------------------------------------
	// Coefficients ordered according to scan order (absolute values)
	// Significance map for each 4x4 subblock
	// 1 bit per coefficient
	// i-th bit corresponds to i-th coefficient in forward scan order

	// Loop through subblocks to reorder coefficients according to scan order
	// Also derive significance map for each subblock, and determine last subblock that contains nonzero coefficients
	subSetIndex = 0;
	while (1)
	{
		// determine position of subblock within transform block
		coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
		coeffGroupPositionY = coeffGroupPosition >> 4;
		coeffGroupPositionX = coeffGroupPosition & 15;

		if (scanIndex == SCAN_HOR2)
		{
			// Subblock scan is mirrored for horizontal scan
			SWAP(coeffGroupPositionX, coeffGroupPositionY);
		}

		subblockPtr = coeffBufferPtr + 4 * coeffGroupPositionY * coeffStride + 4 * coeffGroupPositionX;

		sigmap = 0;
		num = 0;

		for (k = 0; k < 16; k++)
		{
			EB_U32 position = scans4[scanIndex != SCAN_DIAG2][k];
			EB_U32 positionY = position >> 2;
			EB_U32 positionX = position & 3;
			if (scanIndex == SCAN_HOR2)
			{
				// Subblock scan is mirrored for horizontal scan
				SWAP(positionX, positionY);
			}

			val = ABS(subblockPtr[coeffStride * positionY + positionX]);
			linearCoeff[16 * subSetIndex + k] = (EB_U16)val;
			isNonZero = val != 0;
			num += isNonZero;
			sigmap |= isNonZero << k;
		}

		subblockSigmap[subSetIndex] = (EB_U16)sigmap;

		if (sigmap != 0)
		{
			lastScanSet = subSetIndex;

			numNonZeroCoeffs -= num;
			if (numNonZeroCoeffs == 0)
			{
				break;
			}
		}

		subSetIndex++;

	}

	//-------------------------------------------------------------------------------------------------------------------
	// Obtain the last significant X and Y positions and compute their bit cost

	// subblock position
	coeffGroupPosition = sbScans[logBlockSize - 2][lastScanSet];
	coeffGroupPositionY = coeffGroupPosition >> 4;
	coeffGroupPositionX = coeffGroupPosition & 15;
	lastSigYPos = 4 * coeffGroupPositionY;
	lastSigXPos = 4 * coeffGroupPositionX;
	scanPosLast = 16 * lastScanSet;

	// position within subblock
	posLast = Log2f(subblockSigmap[lastScanSet]);
	coeffGroupPosition = scans4[scanIndex != SCAN_DIAG2][posLast];
	lastSigYPos += coeffGroupPosition >> 2;
	lastSigXPos += coeffGroupPosition & 3;
	scanPosLast += posLast;

	// Should swap row/col for SCAN_HOR and SCAN_VER:
	// - SCAN_HOR because the subscan order is mirrored (compared to SCAN_DIAG)
	// - SCAN_VER because of equation (7-66) in H.265 (04/2013)
	// Note that the scans4 array is adjusted to reflect this
	if (scanIndex != SCAN_DIAG2)
	{
		SWAP(lastSigXPos, lastSigYPos);
	}

	// Encode the position of last significant coefficient
	coeffBits += EstimateLastSignificantXY(cabacEncodeCtxPtr, lastSigXPos, lastSigYPos, size, logBlockSize, isChroma);

	//-------------------------------------------------------------------------------------------------------------------
	// Encode Coefficient levels

	significantFlagContextOffset = (!isChroma) ? 0 : NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS;

	contextOffset1 = 1;

	// Memory for 4x4 subblock-level significance flag
	// Bits 0-7 are for current diagonal
	// Bits 16-23 are for previous diagonal
	sbSigMemory = 0;

	// Subblock diagonal index for previous 4x4 subblock (diagonal index is row+col)
	sbPrevDiagIdx = -1;

	// Loop over subblocks
	subSetIndex = lastScanSet;
	do
	{
		// 1. Subblock-level significance flag

		// Assign default value that works for first and last subblock
		EB_S32 significantFlagContextPattern = sbSigMemory & 3;

		if (subSetIndex != 0)
		{
			coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
			coeffGroupPositionY = coeffGroupPosition >> 4;
			coeffGroupPositionX = coeffGroupPosition & 15;

			sbDiagIdx = coeffGroupPositionY + coeffGroupPositionX;
			if (sbDiagIdx != sbPrevDiagIdx)
			{
				sbSigMemory = sbSigMemory << 16;
				sbPrevDiagIdx = sbDiagIdx;
			}

			if (subSetIndex != lastScanSet)
			{
				EB_U32 sigCoeffGroupContextIndex;
				EB_U32 significanceFlag;

				significantFlagContextPattern = (sbSigMemory >> (16 + coeffGroupPositionY)) & 3;
				sigCoeffGroupContextIndex = significantFlagContextPattern != 0;
				sigCoeffGroupContextIndex += isChroma * NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS;

				significanceFlag = (subblockSigmap[subSetIndex] != 0);

				// Add bits for coded_sub_block_flag
				coeffBits += CabacEstimatedBits[significanceFlag ^ cabacEncodeCtxPtr->contextModelEncContext.coeffGroupSigFlagContextModel[sigCoeffGroupContextIndex]];

				if (!significanceFlag)
				{
					// Nothing else to do for this subblock since all coefficients in it are zero
					continue;
				}
			}

			// Memorize that current subblock is nonzero
			sbSigMemory += 1 << coeffGroupPositionY;
		}

		// 2. Coefficient significance flags

		numNonZero = 0; // Number of nonzero coefficients in current subblock

		// Use do {} while(0) loop to avoid goto statement (early exit)
		do
		{
			EB_S32 scanPosSig;
			EB_S32 sigMap = subblockSigmap[subSetIndex];
			EB_S32 subPosition = subSetIndex << LOG2_SCAN_SET_SIZE;
			EB_S32 subPosition2 = subPosition;

			if (subSetIndex == lastScanSet)
			{
				absCoeff[0] = linearCoeffBufferPtr[scanPosLast];
				numNonZero = 1;
				if (sigMap == 1)
				{
					// Nothing else to do here (only "DC" coeff within subblock)
					break;
				}
				scanPosSig = scanPosLast - 1;
				sigMap <<= 31 - (scanPosSig & 15);
			}
			else
			{
				if (sigMap == 1 && subSetIndex != 0)
				{
					subPosition2++;
					absCoeff[0] = linearCoeffBufferPtr[subPosition];
					numNonZero = 1;
				}
				scanPosSig = subPosition + 15;
				sigMap <<= 16;
			}

			if (logBlockSize == 2)
			{
				tempOffset = 0;
				contextIndexMapPtr = contextIndexMap4[scanIndex];
			}
			else
			{
				tempOffset = (logBlockSize == 3) ? (scanIndex == SCAN_DIAG2 ? 9 : 15) : (!isChroma ? 21 : 12);
				tempOffset += (!isChroma && subSetIndex != 0) ? 3 : 0;
				contextIndexMapPtr = contextIndexMap8[scanIndex != SCAN_DIAG2][significantFlagContextPattern] - subPosition;
			}

			// Loop over coefficients
			do
			{
				EB_U32 sigContextIndex;
				EB_S32 sigCoeffFlag = sigMap < 0;

				sigContextIndex = (scanPosSig == 0) ? 0 : contextIndexMapPtr[scanPosSig] + tempOffset;

				// Add bits for sig_coeff_flag
				coeffBits += CabacEstimatedBits[sigCoeffFlag ^ cabacEncodeCtxPtr->contextModelEncContext.significanceFlagContextModel[significantFlagContextOffset + sigContextIndex]];

				if (sigCoeffFlag)
				{
					absCoeff[numNonZero] = scanPosSig >= 0 ? linearCoeffBufferPtr[scanPosSig] : 0;
					numNonZero++;
				}

				sigMap <<= 1;
				scanPosSig--;
			} while (scanPosSig >= subPosition2);
		} while (0);

		// 3. Coefficient level values
		golombRiceParam = 0;

		contextSet = (subSetIndex != 0 && !isChroma) ? 2 : 0;
		contextSet += (contextOffset1 == 0);
		contextOffset1 = 1;
		contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS + 4 * contextSet;
		contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS + contextSet;

		numCoeffWithCodedGt1Flag = MIN(GREATER_THAN1_MAX_NUMBER, numNonZero);

		{
			coeffBits += ONE_BIT * numNonZero; // Add bits for coeff_sign_flag (all coefficients in subblock)
		}

		// Loop over coefficients until base value of Exp-Golomb coding changes
		// Base value changes after either
		// - 8th coefficient
		// - a coefficient larger than 1

		for (index = 0; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for coeff_abs_level_greater1_flag
			coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset + contextOffset1]];

			if (symbolValue)
			{
				symbolValue = absVal > 2;

				// Add bits for coeff_abs_level_greater2_flag
				coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[contextOffset2]];

				if (symbolValue)
				{
					// Golomb Rice parameter is known to be 0 here
					coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 3, 0);
					golombRiceParam = (absVal > 3);
				}

				index++;
				contextOffset1 = 0;

				// Exit loop early as remaining coefficients are coded differently
				break;
			}

			if (contextOffset1 < 3)
			{
				contextOffset1++;
			}
		}

		// Loop over coefficients after first one that was > 1 but before 8th one
		// Base value is know to be equal to 2
		for (; index < numCoeffWithCodedGt1Flag; index++)
		{
			EB_S32 absVal = absCoeff[index];
			EB_U32 symbolValue = absVal > 1;

			// Add bits for >1 flag
			coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset]];

			if (symbolValue)
			{
				coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 2, golombRiceParam);

				// Update Golomb-Rice parameter
				if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
			}
		}

		// Loop over remaining coefficients (8th and beyond)
		// Base value is known to be equal to 1
		for (; index < numNonZero; index++)
		{
			EB_S32 absVal = absCoeff[index];

			coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 1, golombRiceParam);

			// Update Golomb-Rice parameter
			if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
		}
	} while (--subSetIndex >= 0);

	// Add local bit counter to global bit counter
	*coeffBitsLong += coeffBits;
	(void)CabacCost;
	return return_error;
}

/*********************************************************************
* EncodeDeltaQp
*   Encodes the change in the QP indicated by delta QP
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*********************************************************************/
// The functionality of coding delta Qp is implemented for HM 7.0 and needs to be updated for HM 8.0 
EB_ERRORTYPE EncodeDeltaQp(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	EB_S32                  deltaQp)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_BOOL lastBin;
	EB_U32  absDeltaQp;
	EB_U32  tuValue;
	EB_U32  tuValueTemp;
	EB_U32  signDeltaQp = 0;

	absDeltaQp = ABS(deltaQp);
	tuValue = MIN((EB_S32)absDeltaQp, CU_DELTA_QP_CMAX);
	tuValueTemp = tuValue;

	EncodeOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
		tuValueTemp ? 1 : 0,
		&(cabacEncodeCtxPtr->contextModelEncContext.deltaQpContextModel[0]));


	if (tuValueTemp) {
		lastBin =
			(CU_DELTA_QP_CMAX > tuValueTemp) ? EB_TRUE : EB_FALSE;

		while (--tuValueTemp) {
			EncodeOneBin(
				&(cabacEncodeCtxPtr->bacEncContext),
				1,
				&(cabacEncodeCtxPtr->contextModelEncContext.deltaQpContextModel[1]));
		}

		if (lastBin) {
			EncodeOneBin(
				&(cabacEncodeCtxPtr->bacEncContext),
				0,
				&(cabacEncodeCtxPtr->contextModelEncContext.deltaQpContextModel[1]));
		}
	}

	if (absDeltaQp >= CU_DELTA_QP_CMAX){
		ExponentialGolombCode(
			cabacEncodeCtxPtr,
			absDeltaQp - CU_DELTA_QP_CMAX,
			CU_DELTA_QP_EGK);
	}

	if (absDeltaQp > 0){
		signDeltaQp = (deltaQp > 0 ? 0 : 1);

		EncodeBypassOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			signDeltaQp);
	}

	return return_error;
}

/*********************************************************************
* CheckAndCodeDeltaQp
*   Check the conditions and codes the change in the QP indicated by delta QP
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  cuPtr
*   input pointer to the coding unit
*
*  tuPtr
*   input pointer to the transform unit
*********************************************************************/
EB_ERRORTYPE CheckAndCodeDeltaQp(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	CodingUnit_t        *cuPtr,
	TransformUnit_t     *tuPtr,
	EB_BOOL                 isDeltaQpEnable,
	EB_BOOL                *isdeltaQpNotCoded)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;

	if (isDeltaQpEnable) {
		if (tuPtr->lumaCbf || tuPtr->cbCbf || tuPtr->crCbf || tuPtr->cbCbf2 || tuPtr->crCbf2){
			if (*isdeltaQpNotCoded){
				EB_S32  deltaQp;
				deltaQp = cuPtr->qp - cuPtr->refQp;

				EncodeDeltaQp(
					cabacEncodeCtxPtr,
					deltaQp);

				*isdeltaQpNotCoded = EB_FALSE;
			}
		}
	}

	return return_error;
}

/************************************
******* EncodeCoeff
**************************************/
static EB_ERRORTYPE EncodeCoeff(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	CodingUnit_t           *cuPtr,
	TransformUnit_t        *tuPtr,
	EB_U32                  tuSize,
	EB_U32                  tuOriginX,
	EB_U32                  tuOriginY,
	EbPictureBufferDesc_t  *coeffPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_S16 *coeffBuffer;
	EB_U32  coeffLocation;
    const EB_U16 subWidthCMinus1 = (cabacEncodeCtxPtr->colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (cabacEncodeCtxPtr->colorFormat >= EB_YUV422 ? 1 : 2) - 1;
	EB_U32 tuChromaSize = (tuSize == 4) ? 4 : (tuSize >> subWidthCMinus1);
	coeffLocation = tuOriginX + (tuOriginY * coeffPtr->strideY);
	coeffBuffer = (EB_S16*)&coeffPtr->bufferY[coeffLocation * sizeof(EB_S16)];

	if (tuPtr->lumaCbf) {
		EncodeQuantizedCoefficientsFuncArray[!!(ASM_TYPES & PREAVX2_MASK)](
			cabacEncodeCtxPtr,
			tuSize,
			(EB_MODETYPE)cuPtr->predictionModeFlag,
			(&cuPtr->predictionUnitArray[0])->intraLumaMode,
            EB_INTRA_CHROMA_DM,
			coeffBuffer,
			coeffPtr->strideY,
			COMPONENT_LUMA,
            tuPtr);//tuPtr->nzCoefCount[0]);
	}

	// Chroma case when cusize 8x8 and tudepth == 1 OR 16x16 and tudepth == 2
	//tuOriginX = (cuPtr->size == MIN_CU_SIZE ) || ( (cuPtr->size == 16 ) && ((Log2f(cuPtr->size) - tuSizeLog2) == 2))? tuPtr->tuNode->originX: tuOriginX;
	//tuOriginY = (cuPtr->size == MIN_CU_SIZE ) || ( (cuPtr->size == 16 ) && ((Log2f(cuPtr->size) - tuSizeLog2) == 2))? tuPtr->tuNode->originY: tuOriginY;

	// cb
	coeffLocation = (tuOriginX >> subWidthCMinus1) + ((tuOriginY * coeffPtr->strideCb) >> subHeightCMinus1);
	coeffBuffer = (EB_S16*)&coeffPtr->bufferCb[coeffLocation * sizeof(EB_S16)];

	if (tuSize > 4){
		if (tuPtr->cbCbf) {
			EncodeQuantizedCoefficientsFuncArray[!!(ASM_TYPES & PREAVX2_MASK)](
				cabacEncodeCtxPtr,
				tuChromaSize,
				(EB_MODETYPE)cuPtr->predictionModeFlag,
				(&cuPtr->predictionUnitArray[0])->intraLumaMode,
                EB_INTRA_CHROMA_DM,
				coeffBuffer,
				coeffPtr->strideCb,
				COMPONENT_CHROMA_CB,
                tuPtr);//tuPtr->nzCoefCount[1]);
		}

        if (cabacEncodeCtxPtr->colorFormat == EB_YUV422 && tuPtr->cbCbf2) {
            coeffLocation = (tuOriginX >> 1) + ((tuOriginY+tuChromaSize) * coeffPtr->strideCb);
	        coeffBuffer = (EB_S16*)&coeffPtr->bufferCb[coeffLocation * sizeof(EB_S16)];
			EncodeQuantizedCoefficientsFuncArray[!!(ASM_TYPES & PREAVX2_MASK)](
				cabacEncodeCtxPtr,
				tuChromaSize,
				(EB_MODETYPE)cuPtr->predictionModeFlag,
				(&cuPtr->predictionUnitArray[0])->intraLumaMode,
                EB_INTRA_CHROMA_DM,
				coeffBuffer,// Jing: check here
				coeffPtr->strideCb,
				COMPONENT_CHROMA_CB2,
                tuPtr);//tuPtr->nzCoefCount[1]);
        }
	} else if (tuPtr->tuIndex - ((tuPtr->tuIndex >> 2) << 2) == 0) {
        // Never be here
		if (tuPtr->cbCbf) {
			EncodeQuantizedCoefficientsFuncArray[!!(ASM_TYPES & PREAVX2_MASK)](
				cabacEncodeCtxPtr,
				tuChromaSize,
				(EB_MODETYPE)cuPtr->predictionModeFlag,
				(&cuPtr->predictionUnitArray[0])->intraLumaMode,
                EB_INTRA_CHROMA_DM,
				coeffBuffer,
				coeffPtr->strideCb,
				COMPONENT_CHROMA_CB,
                 tuPtr);//tuPtr->nzCoefCount[1]);

		}
	}

	// cr
	coeffLocation = (tuOriginX >> subWidthCMinus1) + ((tuOriginY * coeffPtr->strideCr) >> subHeightCMinus1);
	coeffBuffer = (EB_S16*)&coeffPtr->bufferCr[coeffLocation * sizeof(EB_S16)];

	if (tuSize > 4){
		if (tuPtr->crCbf) {
			EncodeQuantizedCoefficientsFuncArray[!!(ASM_TYPES & PREAVX2_MASK)](
				cabacEncodeCtxPtr,
				tuChromaSize,
				(EB_MODETYPE)cuPtr->predictionModeFlag,
				(&cuPtr->predictionUnitArray[0])->intraLumaMode,
                EB_INTRA_CHROMA_DM,
				coeffBuffer,
				coeffPtr->strideCr,
				COMPONENT_CHROMA_CR,
                tuPtr);//tuPtr->nzCoefCount[2]);

		}
        if (cabacEncodeCtxPtr->colorFormat == EB_YUV422 && tuPtr->crCbf2) {
            coeffLocation = (tuOriginX >> 1) + ((tuOriginY+tuChromaSize) * coeffPtr->strideCr);
	        coeffBuffer = (EB_S16*)&coeffPtr->bufferCr[coeffLocation * sizeof(EB_S16)];
			EncodeQuantizedCoefficientsFuncArray[!!(ASM_TYPES & PREAVX2_MASK)](
				cabacEncodeCtxPtr,
				tuChromaSize,
				(EB_MODETYPE)cuPtr->predictionModeFlag,
				(&cuPtr->predictionUnitArray[0])->intraLumaMode,
                EB_INTRA_CHROMA_DM,
				coeffBuffer,
				coeffPtr->strideCr,
				COMPONENT_CHROMA_CR2,
                tuPtr);//tuPtr->nzCoefCount[2]);
        }
	}
	else if (tuPtr->tuIndex - ((tuPtr->tuIndex >> 2) << 2) == 0) {

		if (tuPtr->crCbf) {
			EncodeQuantizedCoefficientsFuncArray[!!(ASM_TYPES & PREAVX2_MASK)](
				cabacEncodeCtxPtr,
				tuChromaSize,
				(EB_MODETYPE)cuPtr->predictionModeFlag,
				(&cuPtr->predictionUnitArray[0])->intraLumaMode,
                EB_INTRA_CHROMA_DM,
				coeffBuffer,
				coeffPtr->strideCr,
				COMPONENT_CHROMA_CR,
                 tuPtr);//tuPtr->nzCoefCount[2]);

		}
	}

	return return_error;
}

/************************************
******* EncodeTuCoeff
**************************************/
static EB_ERRORTYPE EncodeTuCoeff(
	CabacEncodeContext_t    *cabacEncodeCtxPtr,
	CodingUnit_t            *cuPtr,
	const CodedUnitStats_t  *cuStatsPtr,
	EbPictureBufferDesc_t	*coeffPtr,
	EB_BOOL                  isDeltaQpEnable,
	EB_BOOL                 *isdeltaQpNotCoded)
{
	EB_ERRORTYPE        return_error = EB_ErrorNone;
	EB_U32              subDivContext;
	EB_U32              cbfContext;
	TransformUnit_t *tuPtr;
	const TransformUnitStats_t  *tuStatsPtr;
	EB_U32              tuSize;
	EB_U32              tuOriginX;
	EB_U32              tuOriginY;
	EB_U32              tuIndex;
	EB_U32              tuIndexDepth2;
	EB_U32              tuSizeDepth2;

	tuPtr = &cuPtr->transformUnitArray[0];
	tuStatsPtr = GetTransformUnitStats(0);
	tuSize = cuStatsPtr->size >> tuStatsPtr->depth;
	cbfContext = tuPtr->chromaCbfContext;
	subDivContext = 5 - Log2f(tuSize);

	if (GetCodedUnitStats(cuPtr->leafIndex)->size != 64) {
		// Encode split flag 
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			tuPtr->splitFlag,
			&(cabacEncodeCtxPtr->contextModelEncContext.transSubDivFlagContextModel[subDivContext]));
	}

	if (tuPtr->splitFlag) {
        // Jing: only comes here for inter 64x64

		// Cb CBF  
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			(tuPtr->cbCbf | tuPtr->cbCbf2),
			&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));

		// Cr CBF  
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			(tuPtr->crCbf | tuPtr->crCbf2),
			&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));

		//for(tuIndex = 1; tuIndex < 5; tuIndex++) {
		for (tuIndex = 1; tuIndex < 5; tuIndex ++ ) {
			tuPtr = &cuPtr->transformUnitArray[tuIndex];
			tuStatsPtr = GetTransformUnitStats(tuIndex);
			tuSize = cuStatsPtr->size >> tuStatsPtr->depth;

			tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatsPtr->offsetX);
			tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatsPtr->offsetY);

			if (GetCodedUnitStats(cuPtr->leafIndex)->size != 8) {
				subDivContext = 5 - Log2f(tuSize);
				// Encode split flag 
				EncodeOneBin(
					&(cabacEncodeCtxPtr->bacEncContext),
					tuPtr->splitFlag,
					&(cabacEncodeCtxPtr->contextModelEncContext.transSubDivFlagContextModel[subDivContext]));
			}

			if (tuPtr->splitFlag) {
                // Jing: seems never comes here for now
				cbfContext = tuPtr->chromaCbfContext;

				if (cuPtr->transformUnitArray[0].cbCbf | cuPtr->transformUnitArray[0].cbCbf2) {
					// Cb CBF  
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->cbCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
				}

				if (cuPtr->transformUnitArray[0].crCbf |  cuPtr->transformUnitArray[0].crCbf2){
					// Cr CBF  
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->crCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
				}

				// Transform depth 2
				//tuIndexDepth2 = tuIndex*4 + 1;
				tuIndexDepth2 = tuIndex + 1;

				tuPtr = (tuIndexDepth2 < TRANSFORM_UNIT_MAX_COUNT) ? &cuPtr->transformUnitArray[tuIndexDepth2] : tuPtr;
				cbfContext = tuPtr->chromaCbfContext;

				tuStatsPtr = GetTransformUnitStats(tuIndexDepth2);
				tuSizeDepth2 = cuStatsPtr->size >> tuStatsPtr->depth;
				tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatsPtr->offsetX);
				tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatsPtr->offsetY);

				// Cb CBF  
				if ((cuPtr->transformUnitArray[tuIndex].cbCbf) && (tuSize != 8)){
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->cbCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
				}

				// Cr CBF  
				if ((cuPtr->transformUnitArray[tuIndex].crCbf) && (tuSize != 8)){
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->crCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
				}

				cbfContext = tuPtr->lumaCbfContext;
				// Luma CBF
				EncodeOneBin(
					&(cabacEncodeCtxPtr->bacEncContext),
					tuPtr->lumaCbf,
					&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext]));


				//EncodeDeltaQp
				CheckAndCodeDeltaQp(
					cabacEncodeCtxPtr,
					cuPtr,
					tuPtr,
					isDeltaQpEnable,
					isdeltaQpNotCoded);

				EncodeCoeff(
					cabacEncodeCtxPtr,
					cuPtr,
					tuPtr,
					tuSizeDepth2,
					tuOriginX,
					tuOriginY,
					coeffPtr);

				tuIndexDepth2++;

				tuPtr = (tuIndexDepth2 < TRANSFORM_UNIT_MAX_COUNT) ? &cuPtr->transformUnitArray[tuIndexDepth2] : tuPtr;
				cbfContext = tuPtr->chromaCbfContext;

				tuStatsPtr = GetTransformUnitStats(tuIndexDepth2);
				tuSizeDepth2 = cuStatsPtr->size >> tuStatsPtr->depth;
				tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatsPtr->offsetX);
				tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatsPtr->offsetY);

				// Cb CBF  
				if ((cuPtr->transformUnitArray[tuIndex].cbCbf) && (tuSize != 8)){
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->cbCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
				}

				// Cr CBF  
				if ((cuPtr->transformUnitArray[tuIndex].crCbf) && (tuSize != 8)){
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->crCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
				}

				cbfContext = tuPtr->lumaCbfContext;

				// Luma CBF
				EncodeOneBin(
					&(cabacEncodeCtxPtr->bacEncContext),
					tuPtr->lumaCbf,
					&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext]));

				//EncodeDeltaQp
				CheckAndCodeDeltaQp(
					cabacEncodeCtxPtr,
					cuPtr,
					tuPtr,
					isDeltaQpEnable,
					isdeltaQpNotCoded);

				EncodeCoeff(
					cabacEncodeCtxPtr,
					cuPtr,
					tuPtr,
					tuSizeDepth2,
					tuOriginX,
					tuOriginY,
					coeffPtr);


				tuIndexDepth2++;
				tuPtr = (tuIndexDepth2 < TRANSFORM_UNIT_MAX_COUNT) ? &cuPtr->transformUnitArray[tuIndexDepth2] : tuPtr;
				cbfContext = tuPtr->chromaCbfContext;

				tuStatsPtr = GetTransformUnitStats(tuIndexDepth2);
				tuSizeDepth2 = cuStatsPtr->size >> tuStatsPtr->depth;
				tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatsPtr->offsetX);
				tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatsPtr->offsetY);

				// Cb CBF  
				if ((cuPtr->transformUnitArray[tuIndex].cbCbf) && (tuSize != 8)){
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->cbCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
				}

				// Cr CBF  
				if ((cuPtr->transformUnitArray[tuIndex].crCbf) && (tuSize != 8)){
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->crCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
				}

				cbfContext = tuPtr->lumaCbfContext;
				// Luma CBF
				EncodeOneBin(
					&(cabacEncodeCtxPtr->bacEncContext),
					tuPtr->lumaCbf,
					&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext]));

				//EncodeDeltaQp
				CheckAndCodeDeltaQp(
					cabacEncodeCtxPtr,
					cuPtr,
					tuPtr,
					isDeltaQpEnable,
					isdeltaQpNotCoded);

				EncodeCoeff(
					cabacEncodeCtxPtr,
					cuPtr,
					tuPtr,
					tuSizeDepth2,
					tuOriginX,
					tuOriginY,
					coeffPtr);


				tuIndexDepth2++;
				
				tuPtr = (tuIndexDepth2 < TRANSFORM_UNIT_MAX_COUNT) ? &cuPtr->transformUnitArray[tuIndexDepth2] : tuPtr;
				cbfContext = tuPtr->chromaCbfContext;

				tuStatsPtr = GetTransformUnitStats(tuIndexDepth2);
				tuSizeDepth2 = cuStatsPtr->size >> tuStatsPtr->depth;
				tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatsPtr->offsetX);
				tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatsPtr->offsetY);

				// Cb CBF  
				if ((cuPtr->transformUnitArray[tuIndex].cbCbf) && (tuSize != 8)) {
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->cbCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
				}

				// Cr CBF  
				if ((cuPtr->transformUnitArray[tuIndex].crCbf) && (tuSize != 8)){
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->crCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
				}

				cbfContext = tuPtr->lumaCbfContext;
				// Luma CBF
				EncodeOneBin(
					&(cabacEncodeCtxPtr->bacEncContext),
					tuPtr->lumaCbf,
					&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext]));


				//EncodeDeltaQp
				CheckAndCodeDeltaQp(
					cabacEncodeCtxPtr,
					cuPtr,
					tuPtr,
					isDeltaQpEnable,
					isdeltaQpNotCoded);

				EncodeCoeff(
					cabacEncodeCtxPtr,
					cuPtr,
					tuPtr,
					tuSizeDepth2,
					tuOriginX,
					tuOriginY,
					coeffPtr);

			}
			else {

				cbfContext = tuPtr->chromaCbfContext;

				// Cb CBF  
                if (cuPtr->transformUnitArray[0].cbCbf | cuPtr->transformUnitArray[0].cbCbf2) {
                    EncodeOneBin(
                            &(cabacEncodeCtxPtr->bacEncContext),
                            tuPtr->cbCbf,
                            &(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
                    if (cabacEncodeCtxPtr->colorFormat == EB_YUV422) {
                        EncodeOneBin(
                                &(cabacEncodeCtxPtr->bacEncContext),
                                tuPtr->cbCbf2,
                                &(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
                    }
                }

				// Cr CBF  
                if (cuPtr->transformUnitArray[0].crCbf | cuPtr->transformUnitArray[0].crCbf2) {
					EncodeOneBin(
						&(cabacEncodeCtxPtr->bacEncContext),
						tuPtr->crCbf,
						&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
                    if (cabacEncodeCtxPtr->colorFormat == EB_YUV422) {
                        EncodeOneBin(
                            &(cabacEncodeCtxPtr->bacEncContext),
                            tuPtr->crCbf2,
                            &(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
                    }
				}

				cbfContext = tuPtr->lumaCbfContext;

				// Luma CBF
				EncodeOneBin(
					&(cabacEncodeCtxPtr->bacEncContext),
					tuPtr->lumaCbf,
					&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext]));


				//EncodeDeltaQp

				CheckAndCodeDeltaQp(
					cabacEncodeCtxPtr,
					cuPtr,
					tuPtr,
					isDeltaQpEnable,
					isdeltaQpNotCoded);

				EncodeCoeff(
					cabacEncodeCtxPtr,
					cuPtr,
					tuPtr,
					tuSize,
					tuOriginX,
					tuOriginY,
					coeffPtr);

			}
		}

	}
	else {
		tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatsPtr->offsetX);
		tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatsPtr->offsetY);

		//  Cb CBF
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			tuPtr->cbCbf,
			&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
        if (cabacEncodeCtxPtr->colorFormat == EB_YUV422) {
		    EncodeOneBin(
		    	&(cabacEncodeCtxPtr->bacEncContext),
		    	tuPtr->cbCbf2,
		    	&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
        }

		// Cr CBF  
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			tuPtr->crCbf,
			&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));

        if (cabacEncodeCtxPtr->colorFormat == EB_YUV422) {
		    EncodeOneBin(
		    	&(cabacEncodeCtxPtr->bacEncContext),
		    	tuPtr->crCbf2,
		    	&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
        }
		// Luma CBF

		// In the Inter case, if the RootCbf is 1 and the Chroma Cbfs are 0, then we can infer that the
		// luma Cbf is true, so there is no need to code it.
		if ((cuPtr->predictionModeFlag == INTRA_MODE) || tuPtr->cbCbf || tuPtr->crCbf ||
                (cabacEncodeCtxPtr->colorFormat == EB_YUV422 && (tuPtr->cbCbf2 || tuPtr->crCbf2))) {

			//cbfContext = ((cuPtr->size == tuPtr->size) || (tuPtr->size == TRANSFORM_MAX_SIZE));
			cbfContext = tuPtr->lumaCbfContext;
			EncodeOneBin(
				&(cabacEncodeCtxPtr->bacEncContext),
				tuPtr->lumaCbf,
				&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext]));
		}

		//EncodeDeltaQp
		CheckAndCodeDeltaQp(
			cabacEncodeCtxPtr,
			cuPtr,
			tuPtr,
			isDeltaQpEnable,
			isdeltaQpNotCoded);

		EncodeCoeff(
			cabacEncodeCtxPtr,
			cuPtr,
			tuPtr,
			tuSize,
			tuOriginX,
			tuOriginY,
			coeffPtr);

	}

	return return_error;
}

/*********************************************************************
* EncodeTuSplitCoeff
*   Encodes the TU split flags and coded block flags
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  puPtr
*   input pointer to the prediction unit
*********************************************************************/
static EB_ERRORTYPE EncodeTuSplitCoeff(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	CodingUnit_t           *cuPtr,
	const CodedUnitStats_t *cuStatsPtr,
	EbPictureBufferDesc_t  *coeffPtr,
	EB_U32                 *cuQuantizedCoeffsBits,
	EB_BOOL                 isDeltaQpEnable,
	EB_BOOL                *isdeltaQpNotCoded)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32 rootCbf;

	//rate Control
	EB_U32 writtenBitsBeforeQuantizedCoeff;
	EB_U32 writtenBitsAfterQuantizedCoeff;

	//store the number of written bits before coding quantized coeffs (flush is not called yet): 
	// The total number of bits is 
	// number of written bits
	// + 32  - bits remaining in interval Low Value
	// + number of buffered byte * 8
	writtenBitsBeforeQuantizedCoeff = cabacEncodeCtxPtr->bacEncContext.m_pcTComBitIf.writtenBitsCount +
		32 - cabacEncodeCtxPtr->bacEncContext.bitsRemainingNum +
		(cabacEncodeCtxPtr->bacEncContext.tempBufferedBytesNum << 3);
	// Root CBF
	rootCbf = cuPtr->rootCbf;
	if (cuPtr->predictionModeFlag != INTRA_MODE &&
            !((&cuPtr->predictionUnitArray[0])->mergeFlag)) {
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			rootCbf,
			&(cabacEncodeCtxPtr->contextModelEncContext.rootCbfContextModel[0]));
	}

	if ((cuPtr->predictionModeFlag == INTRA_MODE) ||
            ((cuPtr->predictionModeFlag == INTER_MODE) && (rootCbf > 0))) {
		EncodeTuCoeff(
			cabacEncodeCtxPtr,
			cuPtr,
			cuStatsPtr,
			coeffPtr,
			isDeltaQpEnable,
			isdeltaQpNotCoded);
	}

	//store the number of written bits after coding quantized coeffs (flush is not called yet): 
	// The total number of bits is 
	// number of written bits
	// + 32  - bits remaining in interval Low Value
	// + number of buffered byte * 8
	writtenBitsAfterQuantizedCoeff = cabacEncodeCtxPtr->bacEncContext.m_pcTComBitIf.writtenBitsCount +
		32 - cabacEncodeCtxPtr->bacEncContext.bitsRemainingNum +
		(cabacEncodeCtxPtr->bacEncContext.tempBufferedBytesNum << 3);

	*cuQuantizedCoeffsBits = writtenBitsAfterQuantizedCoeff - writtenBitsBeforeQuantizedCoeff;


	return return_error;
}

/**************************************************
* ConvertToSigned32
**************************************************/
inline static EB_U32 ConvertToSigned32(
	EB_S32 signedNum)
{
	return (signedNum <= 0) ? -signedNum << 1 : (signedNum << 1) - 1;
}

/**************************************************
* WriteCodeCavlc
**************************************************/
inline static void WriteCodeCavlc(
	OutputBitstreamUnit_t *bitstreamPtr,
	EB_U32                 bits,
	EB_U32                 numberOfBits)
{
	OutputBitstreamWrite(
		bitstreamPtr,
		bits,
		numberOfBits);
}


/**************************************************
* WriteUvlc
**************************************************/
static void WriteUvlc(
	OutputBitstreamUnit_t *bitstreamPtr,
	EB_U32                 bits)
{
	EB_U32 numberOfBits = 1;
	EB_U32 tempBits = ++bits;

	while (tempBits > 1)
	{
		tempBits >>= 1;
		numberOfBits += 2;
	}

    if(numberOfBits<32) {
	    OutputBitstreamWrite(
		    bitstreamPtr,
		    bits,
		    numberOfBits);
    } else
    {
	    OutputBitstreamWrite(
		    bitstreamPtr,
		    0,
		    numberOfBits>>1);
	    OutputBitstreamWrite(
		    bitstreamPtr,
		    bits,
		    (numberOfBits+1)>>1);
    }
}

/**************************************************
* WriteSvlc
**************************************************/
inline static void WriteSvlc(
	OutputBitstreamUnit_t *bitstreamPtr,
	EB_S32                 signedBits)
{
	EB_U32 bits;

	bits = ConvertToSigned32(signedBits);
	WriteUvlc(
		bitstreamPtr,
		bits);

}

/**************************************************
* WriteFlagCavlc
**************************************************/
inline static void WriteFlagCavlc(
	OutputBitstreamUnit_t *bitstreamPtr,
	EB_U32                 bits)
{
	OutputBitstreamWrite(
		bitstreamPtr,
		bits,
		1);
}

static void CodeNALUnitHeader(
	OutputBitstreamUnit_t *bitstreamPtr,
	NalUnitType eNalUnitType,
	unsigned int TemporalId)
{
	bitstreamPtr->sliceLocation[bitstreamPtr->sliceNum++] = (bitstreamPtr->writtenBitsCount >> 3);

	// Write Start Code
	WriteCodeCavlc(
		bitstreamPtr,
		1,
		32);

	//bsNALUHeader.write(0,1);                    // forbidden_zero_bit
	//bsNALUHeader.write(nalu.m_nalUnitType, 6);  // nal_unit_type
	//bsNALUHeader.write(0, 6);                   // nuh_reserved_zero_6bits
	//bsNALUHeader.write(nalu.m_temporalId+1, 3); // nuh_temporal_id_plus1

	WriteCodeCavlc(bitstreamPtr, 0, 1);    // forbidden_zero_bit
	WriteCodeCavlc(bitstreamPtr, eNalUnitType, 6); // nal_unit_type 
	WriteCodeCavlc(bitstreamPtr, 0, 6); // nuh_reserved_zero_6bits
	WriteCodeCavlc(bitstreamPtr, TemporalId + 1, 3); // nuh_temporal_id_plus1

}
/**************************************************
* CodeShortTermRPS
*   Note - A value of -1 used for numberOfNegativeReferences
*     denotes that the default value should be used.
**************************************************/
static void CodeShortTermRPS(
	OutputBitstreamUnit_t         *bitstreamPtr,
	PredictionStructureEntry_t    *predStructEntryPtr,
	EB_U32                         numberOfNegativeReferences,
	EB_U32                         numberOfPositiveReferences,
	EB_U32                         rpsCurrentIndex,
	EB_BOOL                        openGopCraFlag)
{
	// difference between poc value of current picture and the picture used for inter prediction
	//EB_S32 deltaPocRps;
	EB_U32 rpsSyntaxIndex;
	EB_BOOL usedByCurrFlag;

	if (rpsCurrentIndex > 0) {

		// "inter_ref_pic_set_prediction_flag"
		WriteFlagCavlc(
			bitstreamPtr,
			predStructEntryPtr->interRpsPredictionFlag);
	}

	{
		// Update Number of Negative and Positive References

		numberOfNegativeReferences = (numberOfNegativeReferences == RPS_DEFAULT_VALUE || numberOfNegativeReferences > predStructEntryPtr->negativeRefPicsTotalCount) ?
			predStructEntryPtr->negativeRefPicsTotalCount :
			numberOfNegativeReferences;
		numberOfPositiveReferences = (numberOfPositiveReferences == RPS_DEFAULT_VALUE || numberOfPositiveReferences > predStructEntryPtr->positiveRefPicsTotalCount) ?
			predStructEntryPtr->positiveRefPicsTotalCount :
			numberOfPositiveReferences;

		// "num_negative_pics"
		WriteUvlc(
			bitstreamPtr,
			numberOfNegativeReferences);

		// "num_positive_pics"
		WriteUvlc(
			bitstreamPtr,
			numberOfPositiveReferences);

		for (rpsSyntaxIndex = 0; rpsSyntaxIndex < numberOfNegativeReferences; ++rpsSyntaxIndex) {

			//"delta_poc_s0_minus1"
			WriteUvlc(
				bitstreamPtr,
				predStructEntryPtr->deltaNegativeGopPosMinus1[rpsSyntaxIndex]);

			usedByCurrFlag = (openGopCraFlag == EB_TRUE) ?
			EB_FALSE :
					 predStructEntryPtr->usedByNegativeCurrPicFlag[rpsSyntaxIndex];

			// "used_by_curr_pic_s0_flag"
			WriteFlagCavlc(
				bitstreamPtr,
				usedByCurrFlag);
		}

		for (rpsSyntaxIndex = 0; rpsSyntaxIndex < numberOfPositiveReferences; ++rpsSyntaxIndex) {

			// "delta_poc_s0_minus1"
			WriteUvlc(
				bitstreamPtr,
				predStructEntryPtr->deltaPositiveGopPosMinus1[rpsSyntaxIndex]);

			// "used_by_curr_pic_s0_flag"
			WriteFlagCavlc(
				bitstreamPtr,
				predStructEntryPtr->usedByPositiveCurrPicFlag[rpsSyntaxIndex]);
		}
	}

	return;
}

/**************************************************
* CodeProfileTier
**************************************************/
static void CodeProfileTier(
	OutputBitstreamUnit_t   *bitstreamPtr,
	SequenceControlSet_t    *scsPtr)
{
	EB_U32 index;

	// "XXX_profile_space[]"
	WriteCodeCavlc(
		bitstreamPtr,
		0,
		2);

	//"XXX_tier_flag[]"
	WriteFlagCavlc(
		bitstreamPtr,
		scsPtr->tierIdc);

	// "XXX_profile_idc[]"
	WriteCodeCavlc(
		bitstreamPtr,
		scsPtr->profileIdc,
		5);

	// "XXX_profile_compatibility_flag[][j]"
	for (index = 0; index < 32; index++) {

		// "XXX_profile_compatibility_flag[][j]"
		WriteFlagCavlc(
			bitstreamPtr,
			(EB_U32)(index == scsPtr->profileIdc));
	}

	// "general_progressive_source_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		scsPtr->generalProgressiveSourceFlag);

	// "general_interlaced_source_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		scsPtr->generalInterlacedSourceFlag);

	// "general_non_packed_constraint_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "general_frame_only_constraint_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		scsPtr->generalFrameOnlyConstraintFlag);

    if(scsPtr->profileIdc < 4)
    {
	    // "XXX_reserved_zero_44bits[0..15]"
	    WriteCodeCavlc(
	        bitstreamPtr,
	        0,
	        16);
    } else
    {
        // "general_max_12bit_constraint_flag"
        WriteFlagCavlc(
           bitstreamPtr,
           1);

        // "general_max_10bit_constraint_flag"
        if(scsPtr->encoderBitDepth <= EB_10BIT || scsPtr->staticConfig.constrainedIntra == EB_TRUE)
        {
            WriteFlagCavlc(
               bitstreamPtr,
               1);
        } else
        {
            WriteFlagCavlc(
               bitstreamPtr,
               0);
        }

        // "general_max_8bit_constraint_flag"
        //if(scsPtr->encoderBitDepth == EB_8BIT)
        if(scsPtr->encoderBitDepth == EB_8BIT && (scsPtr->chromaFormatIdc == EB_YUV444 || scsPtr->staticConfig.constrainedIntra == EB_TRUE))
        {
            WriteFlagCavlc(
               bitstreamPtr,
               1);
        } else
        {
            WriteFlagCavlc(
               bitstreamPtr,
               0);
        }

        // "general_max_422chroma_constraint_flag"
        if(scsPtr->chromaFormatIdc == EB_YUV422 || (scsPtr->chromaFormatIdc == EB_YUV420 && scsPtr->staticConfig.constrainedIntra == EB_TRUE))
        {
            WriteFlagCavlc(
               bitstreamPtr,
               1);
        } else
        {
            WriteFlagCavlc(
               bitstreamPtr,
               0);
        }

        // "general_max_420chroma_constraint_flag"
        if(scsPtr->chromaFormatIdc == EB_YUV420)
        {
            WriteFlagCavlc(
               bitstreamPtr,
               1);
        } else
        {
            WriteFlagCavlc(
               bitstreamPtr,
               0);
        }

        // "general_max_monochrome_constraint_flag"
        WriteFlagCavlc(
           bitstreamPtr,
           0);

        // "general_intra_constraint_flag"
        WriteFlagCavlc(
           bitstreamPtr,
           (scsPtr->staticConfig.constrainedIntra == EB_TRUE));

        // "general_one_picture_only_constraint_flag"
        WriteFlagCavlc(
           bitstreamPtr,
           0);

        // "general_lower_bit_rate_constraint_flag"
        WriteFlagCavlc(
           bitstreamPtr,
           1);

        // "XXX_reserved_zero_44bits[9..15]"
        WriteCodeCavlc(
           bitstreamPtr,
           0,
           7);
    }

	// "XXX_reserved_zero_44bits[16..31]"
	WriteCodeCavlc(
		bitstreamPtr,
		0,
		16);

	// "XXX_reserved_zero_44bits[32..43]"
	WriteCodeCavlc(
		bitstreamPtr,
		0,
		12);

	return;
}
/**************************************************
* ComputeProfileTierLevelInfo
**************************************************/
EB_ERRORTYPE ComputeProfileTierLevelInfo(
	SequenceControlSet_t    *scsPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	// Profile, level, tier calculation

	EB_U64 lumaPictureSize;
	EB_U64 lumaWidthSquare;
	EB_U64 lumaHeightSquare;
	EB_U64 lumaSampleRate;

	lumaWidthSquare = scsPtr->lumaWidth* scsPtr->lumaWidth;
	lumaHeightSquare = scsPtr->lumaHeight* scsPtr->lumaHeight;
	lumaPictureSize = (((scsPtr->lumaWidth + (MAX_LCU_SIZE - 1)) >> MAX_LOG2_LCU_SIZE) << MAX_LOG2_LCU_SIZE) * (((scsPtr->lumaHeight + (MAX_LCU_SIZE - 1)) >> MAX_LOG2_LCU_SIZE) << MAX_LOG2_LCU_SIZE);
	lumaSampleRate = (lumaPictureSize*scsPtr->frameRate) >> 16;

	// profile calculation
	// profile is obatined from config file (needs to be modified later)
	scsPtr->profileIdc = scsPtr->staticConfig.profile;


	if (scsPtr->staticConfig.rateControlMode == 0){
		// level calculation
		if ((lumaSampleRate <= maxLumaSampleRate[0]) && (lumaPictureSize <= maxLumaPictureSize[0]) && (lumaWidthSquare <= maxLumaPictureSize[0] * 8) && (lumaHeightSquare <= maxLumaPictureSize[0] * 8))
			scsPtr->levelIdc = 30; //1*30
		else if ((lumaSampleRate <= maxLumaSampleRate[1]) && (lumaPictureSize <= maxLumaPictureSize[1]) && (lumaWidthSquare <= maxLumaPictureSize[1] * 8) && (lumaHeightSquare <= maxLumaPictureSize[1] * 8))
			scsPtr->levelIdc = 60;//2*30
		else if ((lumaSampleRate <= maxLumaSampleRate[2]) && (lumaPictureSize <= maxLumaPictureSize[2]) && (lumaWidthSquare <= maxLumaPictureSize[2] * 8) && (lumaHeightSquare <= maxLumaPictureSize[2] * 8))
			scsPtr->levelIdc = 63;//2.1*30
		else if ((lumaSampleRate <= maxLumaSampleRate[3]) && (lumaPictureSize <= maxLumaPictureSize[3]) && (lumaWidthSquare <= maxLumaPictureSize[3] * 8) && (lumaHeightSquare <= maxLumaPictureSize[3] * 8))
			scsPtr->levelIdc = 90;//3*30 
		else if ((lumaSampleRate <= maxLumaSampleRate[4]) && (lumaPictureSize <= maxLumaPictureSize[4]) && (lumaWidthSquare <= maxLumaPictureSize[4] * 8) && (lumaHeightSquare <= maxLumaPictureSize[4] * 8))
			scsPtr->levelIdc = 93;//3.1*30
		else if ((lumaSampleRate <= maxLumaSampleRate[5]) && (lumaPictureSize <= maxLumaPictureSize[5]) && (lumaWidthSquare <= maxLumaPictureSize[5] * 8) && (lumaHeightSquare <= maxLumaPictureSize[5] * 8))
			scsPtr->levelIdc = 120;//4*30
		else if ((lumaSampleRate <= maxLumaSampleRate[6]) && (lumaPictureSize <= maxLumaPictureSize[6]) && (lumaWidthSquare <= maxLumaPictureSize[6] * 8) && (lumaHeightSquare <= maxLumaPictureSize[6] * 8))
			scsPtr->levelIdc = 123;//4.1*30
		else if ((lumaSampleRate <= maxLumaSampleRate[7]) && (lumaPictureSize <= maxLumaPictureSize[7]) && (lumaWidthSquare <= maxLumaPictureSize[7] * 8) && (lumaHeightSquare <= maxLumaPictureSize[7] * 8))
			scsPtr->levelIdc = 150;//5*30
		else if ((lumaSampleRate <= maxLumaSampleRate[8]) && (lumaPictureSize <= maxLumaPictureSize[8]) && (lumaWidthSquare <= maxLumaPictureSize[8] * 8) && (lumaHeightSquare <= maxLumaPictureSize[8] * 8))
			scsPtr->levelIdc = 153;//5.1*30
		else if ((lumaSampleRate <= maxLumaSampleRate[9]) && (lumaPictureSize <= maxLumaPictureSize[9]) && (lumaWidthSquare <= maxLumaPictureSize[9] * 8) && (lumaHeightSquare <= maxLumaPictureSize[9] * 8))
			scsPtr->levelIdc = 156;//5.2*30
		else if ((lumaSampleRate <= maxLumaSampleRate[10]) && (lumaPictureSize <= maxLumaPictureSize[10]) && (lumaWidthSquare <= maxLumaPictureSize[10] * 8) && (lumaHeightSquare <= maxLumaPictureSize[10] * 8))
			scsPtr->levelIdc = 180;//6*30
		else if ((lumaSampleRate <= maxLumaSampleRate[11]) && (lumaPictureSize <= maxLumaPictureSize[11]) && (lumaWidthSquare <= maxLumaPictureSize[11] * 8) && (lumaHeightSquare <= maxLumaPictureSize[11] * 8))
			scsPtr->levelIdc = 183;//6.1*30
		else if ((lumaSampleRate <= maxLumaSampleRate[12]) && (lumaPictureSize <= maxLumaPictureSize[12]) && (lumaWidthSquare <= maxLumaPictureSize[12] * 8) && (lumaHeightSquare <= maxLumaPictureSize[12] * 8))
			scsPtr->levelIdc = 186;///6.2*30

		// Tier calculation
		// tier is hardcoded to 0
		scsPtr->tierIdc = 0;
	}
	else{
		// level calculation
		if ((lumaSampleRate <= maxLumaSampleRate[0]) && (lumaPictureSize <= maxLumaPictureSize[0]) && (lumaWidthSquare <= maxLumaPictureSize[0] * 8) && (lumaHeightSquare <= maxLumaPictureSize[0] * 8)
			&& ((scsPtr->staticConfig.targetBitRate*2) <= highTierMaxBitrate[0]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[0])){
			scsPtr->levelIdc = 30; //1*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[0]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[0]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[1]) && (lumaPictureSize <= maxLumaPictureSize[1]) && (lumaWidthSquare <= maxLumaPictureSize[1] * 8) && (lumaHeightSquare <= maxLumaPictureSize[1] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[1]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[1])){
			scsPtr->levelIdc = 60;//2*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[1]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[1]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[2]) && (lumaPictureSize <= maxLumaPictureSize[2]) && (lumaWidthSquare <= maxLumaPictureSize[2] * 8) && (lumaHeightSquare <= maxLumaPictureSize[2] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[2]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[2])){
			scsPtr->levelIdc = 63;//2.1*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[2]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[2]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[3]) && (lumaPictureSize <= maxLumaPictureSize[3]) && (lumaWidthSquare <= maxLumaPictureSize[3] * 8) && (lumaHeightSquare <= maxLumaPictureSize[3] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[3]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[3])){
			scsPtr->levelIdc = 90;//3*30 

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[3]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[3]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[4]) && (lumaPictureSize <= maxLumaPictureSize[4]) && (lumaWidthSquare <= maxLumaPictureSize[4] * 8) && (lumaHeightSquare <= maxLumaPictureSize[4] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[4]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[4])){
			scsPtr->levelIdc = 93;//3.1*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[4]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[4]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[5]) && (lumaPictureSize <= maxLumaPictureSize[5]) && (lumaWidthSquare <= maxLumaPictureSize[5] * 8) && (lumaHeightSquare <= maxLumaPictureSize[5] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[5]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[5])){
			scsPtr->levelIdc = 120;//4*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[5]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[5]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[6]) && (lumaPictureSize <= maxLumaPictureSize[6]) && (lumaWidthSquare <= maxLumaPictureSize[6] * 8) && (lumaHeightSquare <= maxLumaPictureSize[6] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[6]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[6])){
			scsPtr->levelIdc = 123;//4.1*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[6]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[6]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[7]) && (lumaPictureSize <= maxLumaPictureSize[7]) && (lumaWidthSquare <= maxLumaPictureSize[7] * 8) && (lumaHeightSquare <= maxLumaPictureSize[7] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[7]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[7])){
			scsPtr->levelIdc = 150;//5*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[7]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[7]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[8]) && (lumaPictureSize <= maxLumaPictureSize[8]) && (lumaWidthSquare <= maxLumaPictureSize[8] * 8) && (lumaHeightSquare <= maxLumaPictureSize[8] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[8]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[8])){
			scsPtr->levelIdc = 153;//5.1*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[8]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[8]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[9]) && (lumaPictureSize <= maxLumaPictureSize[9]) && (lumaWidthSquare <= maxLumaPictureSize[9] * 8) && (lumaHeightSquare <= maxLumaPictureSize[9] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[9]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[9])){
			scsPtr->levelIdc = 156;//5.2*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[9]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[9]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[10]) && (lumaPictureSize <= maxLumaPictureSize[10]) && (lumaWidthSquare <= maxLumaPictureSize[10] * 8) && (lumaHeightSquare <= maxLumaPictureSize[10] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[10]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[10])){
			scsPtr->levelIdc = 180;//6*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[10]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[10]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[11]) && (lumaPictureSize <= maxLumaPictureSize[11]) && (lumaWidthSquare <= maxLumaPictureSize[11] * 8) && (lumaHeightSquare <= maxLumaPictureSize[11] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[11]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[11])){
			scsPtr->levelIdc = 183;//6.1*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[11]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[11]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}
		else if ((lumaSampleRate <= maxLumaSampleRate[12]) && (lumaPictureSize <= maxLumaPictureSize[12]) && (lumaWidthSquare <= maxLumaPictureSize[12] * 8) && (lumaHeightSquare <= maxLumaPictureSize[12] * 8)
			&& ((scsPtr->staticConfig.targetBitRate * 2) <= highTierMaxBitrate[12]) && ((scsPtr->staticConfig.targetBitRate * 3) <= highTierMaxCPBsize[12])){
			scsPtr->levelIdc = 186;///6.2*30

			if (((scsPtr->staticConfig.targetBitRate * 2) <= mainTierMaxBitrate[12]) && ((scsPtr->staticConfig.targetBitRate * 3) <= mainTierMaxCPBsize[12]))
				scsPtr->tierIdc = 0;
			else
				scsPtr->tierIdc = 1;
		}

	}

    if(scsPtr->staticConfig.tileColumnCount > 1 || scsPtr->staticConfig.tileRowCount > 1) {
        unsigned int levelIdx = 0;
        const unsigned int general_level_idc[13] = {30, 60, 63, 90, 93, 120, 123, 150, 153, 156, 180, 183, 186};
        while (scsPtr->levelIdc != general_level_idc[levelIdx]) levelIdx++;
        while(scsPtr->staticConfig.tileColumnCount > maxTileColumn[levelIdx] || scsPtr->staticConfig.tileRowCount > maxTileRow[levelIdx]) levelIdx++;
        if (levelIdx>12) {
            return_error = EB_ErrorBadParameter;
            return return_error;
        }
        scsPtr->levelIdc = general_level_idc[levelIdx];
    }

	// Use Level and Tier info if set in config
	if (scsPtr->staticConfig.level != 0) {
		scsPtr->levelIdc = scsPtr->staticConfig.level * 3;
	}
	// This needs to be modified later
	scsPtr->tierIdc = scsPtr->staticConfig.tier;

	return return_error;
}
/**************************************************
* ComputeMaxDpbBuffer
**************************************************/
EB_ERRORTYPE ComputeMaxDpbBuffer(
	SequenceControlSet_t    *scsPtr){

	EB_U64 maxLumaPicSize = 0, frameSize = (scsPtr->lumaWidth * scsPtr->lumaHeight);
	EB_ERRORTYPE return_error = EB_ErrorNone;

	switch (scsPtr->levelIdc){
	case 30:
		maxLumaPicSize = maxLumaPictureSize[0];
		break;
	case 60:
		maxLumaPicSize = maxLumaPictureSize[1];
		break;
	case 63:
		maxLumaPicSize = maxLumaPictureSize[2];
		break;
	case 90:
		maxLumaPicSize = maxLumaPictureSize[3];
		break;
	case 93:
		maxLumaPicSize = maxLumaPictureSize[4];
		break;
	case 120:
		maxLumaPicSize = maxLumaPictureSize[5];
		break;
	case 123:
		maxLumaPicSize = maxLumaPictureSize[6];
		break;
	case 150:
		maxLumaPicSize = maxLumaPictureSize[7];
		break;
	case 153:
		maxLumaPicSize = maxLumaPictureSize[8];
		break;
	case 156:
		maxLumaPicSize = maxLumaPictureSize[9];
		break;
	case 180:
		maxLumaPicSize = maxLumaPictureSize[10];
		break;
	case 183:
		maxLumaPicSize = maxLumaPictureSize[11];
		break;
	case 186:
		maxLumaPicSize = maxLumaPictureSize[12];
		break;
	default:
		CHECK_REPORT_ERROR_NC(scsPtr->encodeContextPtr->appCallbackPtr, EB_ENC_EC_ERROR26)
			break;
	}

	if (frameSize <= maxLumaPicSize >> 2)
		scsPtr->maxDpbSize = 16; // MaxDpbSize = Min( 4 * maxDpbPicBuf, 16 ) \ maxDpbPicBuf = 6
	else if (frameSize <= maxLumaPicSize >> 1)
		scsPtr->maxDpbSize = 12; // MaxDpbSize = Min( 2 * maxDpbPicBuf, 16 ) \ maxDpbPicBuf = 6
	else if (frameSize <= (3 * maxLumaPicSize) >> 2)
		scsPtr->maxDpbSize = 8;  // MaxDpbSize = Min( ( 4 * maxDpbPicBuf ) / 3, 16 ) \ maxDpbPicBuf = 6
	else
		scsPtr->maxDpbSize = 6;  // MaxDpbSize = maxDpbPicBuf \ maxDpbPicBuf = 6

	return return_error;
}
/**************************************************
* CodeProfileTierLevel
**************************************************/
static void CodeProfileTierLevel(
	OutputBitstreamUnit_t   *bitstreamPtr,
	SequenceControlSet_t    *scsPtr,
	EB_BOOL                  profilePresentFlag,
	EB_U32                   maxSubLayersMinus1)
{
	EB_U32 index;

	if (profilePresentFlag) {
		CodeProfileTier(bitstreamPtr, scsPtr); // general profile tier
	}

	//"general_level_idc"
	WriteCodeCavlc(
		bitstreamPtr,
		scsPtr->levelIdc,
		8);

	for (index = 0; index < maxSubLayersMinus1; index++) {
		if (profilePresentFlag) {
			// sub_layer_profile_present_flag[i]
			WriteFlagCavlc(
				bitstreamPtr,
				0);
		}
		//sub_layer_level_present_flag[i]
		WriteFlagCavlc(
			bitstreamPtr,
			0);
	}

	if (maxSubLayersMinus1 > 0) {
		for (index = maxSubLayersMinus1; index < 8; index++) {
			// reserved_zero_2bits
			WriteCodeCavlc(
				bitstreamPtr,
				0,
				2);
		}
	}

	return;
}

/**************************************************
* CodeVPS
**************************************************/
static void CodeVPS(
	OutputBitstreamUnit_t   *bitstreamPtr,
	SequenceControlSet_t    *scsPtr)
{

	EB_U32 temporalLayersMinus1 = scsPtr->staticConfig.enableTemporalId ? scsPtr->maxTemporalLayers
		: 0;

	// uiFirstByte
	//codeNALUnitHeader( NAL_UNIT_SPS, NAL_REF_IDC_PRIORITY_HIGHEST );
	CodeNALUnitHeader(
		bitstreamPtr,
		NAL_UNIT_VPS,
		0);

	// Note: This function is currently hard coded, needs to have a proper structure

	// "vps_video_parameter_set_id"
	WriteCodeCavlc(
		bitstreamPtr,
		scsPtr->vpsId,
		4);

	// "vps_reserved_three_2bits"
	WriteCodeCavlc(
		bitstreamPtr,
		3,
		2);

	// "vps_reserved_zero_6bits"
	WriteCodeCavlc(
		bitstreamPtr,
		0,
		6);

	// "vps_max_sub_layers_minus1"
	WriteCodeCavlc(
		bitstreamPtr,
		temporalLayersMinus1,//scsPtr->maxTemporalLayers - 1,
		3);

	// "vps_temporal_id_nesting_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		temporalLayersMinus1 == 0 ? 1 : 0);

	// "vps_reserved_ffff_16bits"
	WriteCodeCavlc(
		bitstreamPtr,
		0xffff,
		16);

	CodeProfileTierLevel(
		bitstreamPtr,
		scsPtr,
		EB_TRUE,
		temporalLayersMinus1);

	// "vps_sub_layer_ordering_info_present_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	{
		// Supports Main Profile Level 5 (Max Picture resolution 4Kx2K, up to 6 Hierarchy Levels)
		// Max Dec Pic Buffering [i]
		WriteUvlc(
			bitstreamPtr,
			scsPtr->maxDpbSize - 1/*4*/);//(1 << (scsPtr->maxTemporalLayers-syntaxItr)));

		WriteUvlc(
			bitstreamPtr,
			scsPtr->maxDpbSize - 1/*3*/);//(1 << (scsPtr->maxTemporalLayers-syntaxItr)));

		WriteUvlc(
			bitstreamPtr,
			0);
	}

	// "vps_max_nuh_reserved_zero_layer_id"
	WriteCodeCavlc(
		bitstreamPtr,
		0,
		6);

	// "vps_max_op_sets_minus1"
	WriteUvlc(
		bitstreamPtr,
		0);

    // "vps_timing_info_present_flag"
    WriteFlagCavlc(
        bitstreamPtr,
        scsPtr->staticConfig.fpsInVps == 1 ? EB_TRUE : EB_FALSE);

    if (scsPtr->staticConfig.fpsInVps == 1) {
        if (scsPtr->staticConfig.frameRateDenominator != 0 && scsPtr->staticConfig.frameRateNumerator != 0) {

            // vps_num_units_in_tick
            WriteCodeCavlc(
                    bitstreamPtr,
                    scsPtr->staticConfig.frameRateDenominator,
                    32);

            // vps_time_scale
            WriteCodeCavlc(
                    bitstreamPtr,
                    scsPtr->staticConfig.frameRateNumerator,
                    32);
        }
        else {
            // vps_num_units_in_tick
            WriteCodeCavlc(
                    bitstreamPtr,
                    1 << 16,
                    32);

            // vps_time_scale
            WriteCodeCavlc(
                    bitstreamPtr,
                    scsPtr->frameRate > 1000 ? scsPtr->frameRate : scsPtr->frameRate << 16,
                    32);
        }

        // vps_poc_proportional_to_timing_flag 
        WriteFlagCavlc(
                bitstreamPtr,
                0);

        // vps_num_hrd_parameters 
        WriteUvlc(
                bitstreamPtr,
                0);
    }


	// "vps_extension_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	return;

}
static void CodeHrdParameters(
	OutputBitstreamUnit_t   *bitstreamPtr,
	AppHrdParameters_t         *hrdParamterPtr,
	EB_BOOL                  commonInfoPresentFlag,
	EB_U32                   maxTemporalLayersM1,
	EncodeContext_t         *encodeContextPtr)
{
	EB_U32   layerIndex;
	EB_U32   cpbIndex;
	EB_U32   nalOrVclIndex;   // 0 --- Nal; 1 --- VCL

	(void)encodeContextPtr;

	if (commonInfoPresentFlag){
		// nal_hrd_parameters_present_flag
		WriteFlagCavlc(
			bitstreamPtr,
			hrdParamterPtr->nalHrdParametersPresentFlag);

		// vcl_hrd_parameters_present_flag
		WriteFlagCavlc(
			bitstreamPtr,
			hrdParamterPtr->vclHrdParametersPresentFlag);

		if (hrdParamterPtr->nalHrdParametersPresentFlag || hrdParamterPtr->vclHrdParametersPresentFlag){
			// sub_pic_cpb_params_present_flag
			WriteFlagCavlc(
				bitstreamPtr,
				hrdParamterPtr->subPicCpbParamsPresentFlag);

			if (hrdParamterPtr->subPicCpbParamsPresentFlag){
				// tick_divisor_minus2
				WriteCodeCavlc(
					bitstreamPtr,
					hrdParamterPtr->tickDivisorMinus2,
					8);

				// du_cpb_removal_delay_length_minus1
				WriteCodeCavlc(
					bitstreamPtr,
					hrdParamterPtr->duCpbRemovalDelayLengthMinus1,
					5);

				// sub_pic_cpb_params_in_pic_timing_sei_flag
				WriteFlagCavlc(
					bitstreamPtr,
					hrdParamterPtr->subPicCpbParamsPicTimingSeiFlag);

				// dpb_output_delay_du_length_minus1
				WriteCodeCavlc(
					bitstreamPtr,
					hrdParamterPtr->dpbOutputDelayDuLengthMinus1,
					5);
			}

			// bit_rate_scale
			WriteCodeCavlc(
				bitstreamPtr,
				hrdParamterPtr->bitRateScale,
				4);

			// cpb_size_scale
			WriteCodeCavlc(
				bitstreamPtr,
				hrdParamterPtr->cpbSizeScale,
				4);

			if (hrdParamterPtr->subPicCpbParamsPresentFlag){
				// du_cpb_size_scale
				WriteCodeCavlc(
					bitstreamPtr,
					hrdParamterPtr->duCpbSizeScale,
					4);
			}

			// initial_cpb_removal_delay_length_minus1
			WriteCodeCavlc(
				bitstreamPtr,
				hrdParamterPtr->initialCpbRemovalDelayLengthMinus1,
				5);

			// au_cpb_removal_delay_length_minus1
			WriteCodeCavlc(
				bitstreamPtr,
				hrdParamterPtr->auCpbRemovalDelayLengthMinus1,
				5);

			// dpb_output_delay_length_minus1
			WriteCodeCavlc(
				bitstreamPtr,
				hrdParamterPtr->dpbOutputDelayLengthMinus1,
				5);

		}
	}

	for (layerIndex = 0; layerIndex <= maxTemporalLayersM1; ++layerIndex){
		// fixed_pic_rate_general_flag
		WriteFlagCavlc(
			bitstreamPtr,
			hrdParamterPtr->fixedPicRateGeneralFlag[layerIndex]);

		if (!hrdParamterPtr->fixedPicRateGeneralFlag[layerIndex]){
			// fixed_pic_rate_within_cvs_flag
			WriteFlagCavlc(
				bitstreamPtr,
				hrdParamterPtr->fixedPicRateWithinCvsFlag[layerIndex]);
		}

		if (hrdParamterPtr->fixedPicRateWithinCvsFlag[layerIndex]){
			// elemental_duration_in_tc_minus1
			WriteUvlc(
				bitstreamPtr,
				hrdParamterPtr->elementalDurationTcMinus1[layerIndex]);
		}
		else{
			// low_delay_hrd_flag
			WriteFlagCavlc(
				bitstreamPtr,
				hrdParamterPtr->lowDelayHrdFlag[layerIndex]);
		}

		if (!hrdParamterPtr->lowDelayHrdFlag[layerIndex]){
			// cpb_cnt_minus1
			WriteUvlc(
				bitstreamPtr,
				hrdParamterPtr->cpbCountMinus1[layerIndex]);
		}

		for (nalOrVclIndex = 0; nalOrVclIndex < 2; ++nalOrVclIndex){
			if ((nalOrVclIndex == 0 && hrdParamterPtr->nalHrdParametersPresentFlag) ||
				(nalOrVclIndex == 1 && hrdParamterPtr->vclHrdParametersPresentFlag)){

				//CHECK_REPORT_ERROR(
				//    (hrdParamterPtr->cpbCountMinus1[layerIndex] < MAX_CPB_COUNT),
				//    encodeContextPtr->appCallbackPtr, 
				//    EB_ENC_EC_ERROR14);

				for (cpbIndex = 0; cpbIndex <= hrdParamterPtr->cpbCountMinus1[layerIndex]; ++cpbIndex){
					// bit_rate_value_minus1
					WriteUvlc(
						bitstreamPtr,
						hrdParamterPtr->bitRateValueMinus1[layerIndex][nalOrVclIndex][cpbIndex]);

					// cpb_size_value_minus1
					WriteUvlc(
						bitstreamPtr,
						hrdParamterPtr->cpbSizeValueMinus1[layerIndex][nalOrVclIndex][cpbIndex]);

					if (hrdParamterPtr->subPicCpbParamsPresentFlag){
						// bit_rate_du_value_minus1
						WriteUvlc(
							bitstreamPtr,
							hrdParamterPtr->bitRateDuValueMinus1[layerIndex][nalOrVclIndex][cpbIndex]);

						// cpb_size_du_value_minus1
						WriteUvlc(
							bitstreamPtr,
							hrdParamterPtr->cpbSizeDuValueMinus1[layerIndex][nalOrVclIndex][cpbIndex]);
					}

					// cbr_flag
					WriteFlagCavlc(
						bitstreamPtr,
						hrdParamterPtr->cbrFlag[layerIndex][nalOrVclIndex][cpbIndex]);
				}
			}
		}
	}

	return;
}


/**************************************************
* CodeVUI
**************************************************/
static void CodeVUI(
	OutputBitstreamUnit_t   *bitstreamPtr,
	AppVideoUsabilityInfo_t    *vuiPtr,
	EB_U32                   maxTemporalLayersM1,
	EncodeContext_t        *encodeContextPtr)
{
	// aspect_ratio_info_present_flag
	WriteFlagCavlc(
		bitstreamPtr,
		vuiPtr->aspectRatioInfoPresentFlag);

	if (vuiPtr->aspectRatioInfoPresentFlag)
	{
		// aspect_ratio_idc
		WriteCodeCavlc(
			bitstreamPtr,
			vuiPtr->aspectRatioIdc,
			8);

		if (vuiPtr->aspectRatioIdc == 255) {
			// sar_width
			WriteCodeCavlc(
				bitstreamPtr,
				vuiPtr->sarWidth,
				16);

			// sar_height
			WriteCodeCavlc(
				bitstreamPtr,
				vuiPtr->sarHeight,
				16);
		}
	}

	// overscan_info_present_flag
	WriteFlagCavlc(
		bitstreamPtr,
		vuiPtr->overscanInfoPresentFlag);

	if (vuiPtr->overscanInfoPresentFlag) {
		// overscan_appropriate_flag
		WriteFlagCavlc(
			bitstreamPtr,
			vuiPtr->overscanApproriateFlag);
	}

	// video_signal_type_present_flag
	WriteFlagCavlc(
		bitstreamPtr,
		vuiPtr->videoSignalTypePresentFlag);

	if (vuiPtr->videoSignalTypePresentFlag) {

		// video_format
		WriteCodeCavlc(
			bitstreamPtr,
			vuiPtr->videoFormat,
			3);

		// video_full_range_flag
		WriteFlagCavlc(
			bitstreamPtr,
			vuiPtr->videoFullRangeFlag);

		// colour_description_present_flag
		WriteFlagCavlc(
			bitstreamPtr,
			vuiPtr->colorDescriptionPresentFlag);

		if (vuiPtr->colorDescriptionPresentFlag) {

			// colour_primaries
			WriteCodeCavlc(
				bitstreamPtr,
				vuiPtr->colorPrimaries,
				8);

			// transfer_characteristics
			WriteCodeCavlc(
				bitstreamPtr,
				vuiPtr->transferCharacteristics,
				8);

			// matrix_coefficients
			WriteCodeCavlc(
				bitstreamPtr,
				vuiPtr->matrixCoeffs,
				8);
		}
	}

	// chroma_loc_info_present_flag
	WriteFlagCavlc(
		bitstreamPtr,
		vuiPtr->chromaLocInfoPresentFlag);

	if (vuiPtr->chromaLocInfoPresentFlag) {

		// chroma_sample_loc_type_top_field
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->chromaSampleLocTypeTopField);

		// chroma_sample_loc_type_bottom_field
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->chromaSampleLocTypeBottomField);
	}

	// neutral_chroma_indication_flag
	WriteFlagCavlc(
		bitstreamPtr,
		vuiPtr->neutralChromaIndicationFlag);

	// field_seq_flag
	WriteFlagCavlc(
		bitstreamPtr,
		vuiPtr->fieldSeqFlag);

	// frame_field_info_present_flag
	WriteFlagCavlc(
		bitstreamPtr,
		vuiPtr->frameFieldInfoPresentFlag);

	// default_display_window_flag
	WriteFlagCavlc(
		bitstreamPtr,
		vuiPtr->defaultDisplayWindowFlag);

	if (vuiPtr->defaultDisplayWindowFlag)
	{
		// def_disp_win_left_offset
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->defaultDisplayWinLeftOffset);

		// def_disp_win_right_offset
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->defaultDisplayWinRightOffset);

		// def_disp_win_top_offset
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->defaultDisplayWinTopOffset);

		// def_disp_win_bottom_offset
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->defaultDisplayWinBottomOffset);
	}

	// vui_timing_info_present_flag
	WriteFlagCavlc(
		bitstreamPtr,
		vuiPtr->vuiTimingInfoPresentFlag);

	if (vuiPtr->vuiTimingInfoPresentFlag) {

		// vui_num_units_in_tick
		WriteCodeCavlc(
			bitstreamPtr,
			vuiPtr->vuiNumUnitsInTick,
			32);

		// vui_time_scale
		WriteCodeCavlc(
			bitstreamPtr,
			vuiPtr->vuiTimeScale,
			32);

		// vui_poc_proportional_to_timing_flag
		WriteFlagCavlc(
			bitstreamPtr,
			vuiPtr->vuiPocPropotionalTimingFlag);

		if (vuiPtr->vuiPocPropotionalTimingFlag) {
			// vui_num_ticks_poc_diff_one_minus1
			WriteUvlc(
				bitstreamPtr,
				vuiPtr->vuiNumTicksPocDiffOneMinus1);
		}

		// hrd_parameters_present_flag
		WriteFlagCavlc(
			bitstreamPtr,
			vuiPtr->vuiHrdParametersPresentFlag);

		if (vuiPtr->vuiHrdParametersPresentFlag) {
			CodeHrdParameters(
				bitstreamPtr,
				vuiPtr->hrdParametersPtr,
				EB_TRUE,
				maxTemporalLayersM1,
				encodeContextPtr);
		}
	}


	// bitstream_restriction_flag
	WriteFlagCavlc(
		bitstreamPtr,
		vuiPtr->bitstreamRestrictionFlag);

	if (vuiPtr->bitstreamRestrictionFlag) {
		// tiles_fixed_structure_flag
		WriteFlagCavlc(
			bitstreamPtr,
			0);

		// motion_vectors_over_pic_boundaries_flag
		WriteFlagCavlc(
			bitstreamPtr,
			vuiPtr->motionVectorsOverPicBoundariesFlag);

		// restricted_ref_pic_lists_flag
		WriteFlagCavlc(
			bitstreamPtr,
			vuiPtr->restrictedRefPicListsFlag);

		// min_spatial_segmentation_idc
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->minSpatialSegmentationIdc);

		// max_bytes_per_pic_denom
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->maxBytesPerPicDenom);

		// max_bits_per_mincu_denom
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->maxBitsPerMinCuDenom);

		// log2_max_mv_length_horizontal
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->log2MaxMvLengthHorizontal);

		// log2_max_mv_length_vertical
		WriteUvlc(
			bitstreamPtr,
			vuiPtr->log2MaxMvLengthVertical);
	}
}

/**************************************************
* CodeSPS
**************************************************/
static void CodeSPS(
	OutputBitstreamUnit_t   *bitstreamPtr,
	SequenceControlSet_t    *scsPtr)
{

	EncodeContext_t        *encodeContextPtr = scsPtr->encodeContextPtr;

	EB_U32 temporalLayersMinus1 = scsPtr->staticConfig.enableTemporalId ? scsPtr->maxTemporalLayers
		: 0;
	CodeNALUnitHeader(
		bitstreamPtr,
		NAL_UNIT_SPS,
		0);

	// "sps_video_parameter_set_id"
	WriteCodeCavlc(
		bitstreamPtr,
		scsPtr->vpsId,
		4);

	// "sps_max_sub_layers_minus1"
	WriteCodeCavlc(
		bitstreamPtr,
		temporalLayersMinus1,//scsPtr->maxTemporalLayers - 1,
		3);

	// "sps_temporal_id_nesting_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		temporalLayersMinus1 == 0 ? 1 : 0);

	CodeProfileTierLevel(
		bitstreamPtr,
		scsPtr,
		EB_TRUE,
		temporalLayersMinus1);

	// "sps_seq_parameter_set_id"
	WriteUvlc(
		bitstreamPtr,
		scsPtr->spsId);

	// "chroma_format_idc"
	WriteUvlc(
		bitstreamPtr,
		scsPtr->chromaFormatIdc);

    if (scsPtr->chromaFormatIdc == EB_YUV444) {
        WriteFlagCavlc(bitstreamPtr, 0); //separate_colour_plane_flag=0
    }

	// "pic_width_in_luma_samples"
	WriteUvlc(
		bitstreamPtr,
		scsPtr->lumaWidth);

	// "pic_height_in_luma_samples"
	WriteUvlc(
		bitstreamPtr,
		scsPtr->lumaHeight);

	// "conformance_window_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		scsPtr->conformanceWindowFlag);


	if (scsPtr->conformanceWindowFlag) {

		// "conf_win_left_offset"
		WriteUvlc(
			bitstreamPtr,
			scsPtr->croppingLeftOffset >> 1);

		// "conf_win_right_offset"
		WriteUvlc(
			bitstreamPtr,
			scsPtr->croppingRightOffset >> 1);

		// "conf_win_top_offset"
		WriteUvlc(
			bitstreamPtr,
			scsPtr->croppingTopOffset >> 1);

		// "conf_win_bottom_offset"
		WriteUvlc(
			bitstreamPtr,
			scsPtr->croppingBottomOffset >> 1);
	}


	// Luma Bit Increment 
	// "bit_depth_luma_minus8"
	WriteUvlc(
		bitstreamPtr,
		scsPtr->staticConfig.encoderBitDepth - 8);//2);//

	// Chroma Bit Increment
	// "bit_depth_chroma_minus8"
	WriteUvlc(
		bitstreamPtr,
		scsPtr->staticConfig.encoderBitDepth - 8);// 2);//

	// "log2_max_pic_order_cnt_lsb_minus4"
	WriteUvlc(
		bitstreamPtr,
		(EB_U32)(scsPtr->bitsForPictureOrderCount - 4));

	// "sps_sub_layer_ordering_info_present_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);


	{

		// Supports Main Profile Level 5 (Max Picture resolution 4Kx2K, up to 6 Hierarchy Levels)
		WriteUvlc(
			bitstreamPtr,
			scsPtr->maxDpbSize - 1/*4*/);//(1 << (scsPtr->maxTemporalLayers-syntaxItr)));

		WriteUvlc(
			bitstreamPtr,
			scsPtr->maxDpbSize - 1/*3*/);//(1 << (scsPtr->maxTemporalLayers-syntaxItr)));

		WriteUvlc(
			bitstreamPtr,
			0);
	}

	// "log2_min_coding_block_size_minus3"
	// Log2(Min CU Size) - 3
	WriteUvlc(
		bitstreamPtr,
		Log2f(scsPtr->lcuSize) - (scsPtr->maxLcuDepth - 1) - 3);

	// Max CU Depth
	// "log2_diff_max_min_coding_block_size"
	WriteUvlc(
		bitstreamPtr,
		scsPtr->maxLcuDepth - 1);

	// "log2_min_transform_block_size_minus2"
	WriteUvlc(
		bitstreamPtr,
		2 - 2);

	// "log2_diff_max_min_transform_block_size"
	WriteUvlc(
		bitstreamPtr,
		5 - 2);

	// "max_transform_hierarchy_depth_inter"
	WriteUvlc(
		bitstreamPtr,
		3 - 1);

	// "max_transform_hierarchy_depth_intra"
	WriteUvlc(
		bitstreamPtr,
		3 - 1);

	// "scaling_list_enabled_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "asymmetric_motion_partitions_enabled_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		EB_FALSE);

	//// SAO
	// "sample_adaptive_offset_enabled_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		scsPtr->staticConfig.enableSaoFlag);

	// "pcm_enabled_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// RPS related
	// No RPS to send in SPS when dynamic GOP is ON
	WriteUvlc(
		bitstreamPtr,
		0);

	// "long_term_ref_pics_present_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		scsPtr->predStructPtr->longTermEnableFlag);

	if (scsPtr->predStructPtr->longTermEnableFlag) {
		// Note: Add code here related to Long term reference pics

		CHECK_REPORT_ERROR_NC(
			encodeContextPtr->appCallbackPtr,
			EB_ENC_EC_ERROR11);

	}

	// "sps_temporal_mvp_enable_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		scsPtr->enableTmvpSps);

	// "sps_strong_intra_smoothing_enable_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		scsPtr->enableStrongIntraSmoothing);

	// "vui_parameters_present_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		scsPtr->staticConfig.videoUsabilityInfo);

	if (scsPtr->staticConfig.videoUsabilityInfo) {
		CodeVUI(
			bitstreamPtr,
			scsPtr->videoUsabilityInfoPtr,
			scsPtr->maxTemporalLayers,
			NULL /*encodeContextPtr*/);
	}

	// "sps_extension_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	return;
}

static void CodePPS(
	OutputBitstreamUnit_t *bitstreamPtr,
    SequenceControlSet_t *scsPtr,
	EbPPSConfig_t       *ppsConfig)
{
	//SequenceControlSet_t    *scsPtr = (SequenceControlSet_t*)pcsPtr->sequenceControlSetWrapperPtr->objectPtr;

	EB_BOOL disableDlfFlag = scsPtr->staticConfig.disableDlfFlag;
    EB_BOOL tileMode = (scsPtr->staticConfig.tileColumnCount > 1 || scsPtr->staticConfig.tileRowCount > 1) ? EB_TRUE : EB_FALSE;

	// uiFirstByte
	//codeNALUnitHeader( NAL_UNIT_PPS, NAL_REF_IDC_PRIORITY_HIGHEST );
	CodeNALUnitHeader(
		bitstreamPtr,
		NAL_UNIT_PPS,
		0);


	// "pps_pic_parameter_set_id"

	// Code PPS ID
	WriteUvlc(
		bitstreamPtr,
		ppsConfig->ppsId);

	// "pps_seq_parameter_set_id"

	// Code SPS ID
	WriteUvlc(
		bitstreamPtr,
		0);

	// "dependent_slice_segments_enabled_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "output_flag_present_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "num_extra_slice_header_bits"
	WriteCodeCavlc(
		bitstreamPtr,
		0,
		3);

	// "sign_data_hiding_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		EB_FALSE);

	// "cabac_init_present_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "num_ref_idx_l0_default_active_minus1"
    WriteUvlc(
        bitstreamPtr,
        0);// pcsPtr->ParentPcsPtr->predStructPtr->defaultRefPicsList0TotalCountMinus1);

	// "num_ref_idx_l1_default_active_minus1"
	WriteUvlc(
		bitstreamPtr,
        0);//pcsPtr->ParentPcsPtr->predStructPtr->defaultRefPicsList1TotalCountMinus1);


	// "pic_init_qp_minus26"
	WriteSvlc(
		bitstreamPtr,
		0);

	// "constrained_intra_pred_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		ppsConfig->constrainedFlag);

	//constrained = 1;

	// "transform_skip_enabled_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "cu_qp_delta_enabled_flag"
	WriteFlagCavlc(
		bitstreamPtr,
        scsPtr->staticConfig.improveSharpness || scsPtr->staticConfig.bitRateReduction);// pcsPtr->useDeltaQp);

	if (scsPtr->staticConfig.improveSharpness || scsPtr->staticConfig.bitRateReduction) { //pcsPtr->useDeltaQp) {
		// "diff_cu_qp_delta_depth"
		WriteUvlc(
			bitstreamPtr,
            scsPtr->inputResolution < INPUT_SIZE_1080p_RANGE ? 2 : 3);// pcsPtr->difCuDeltaQpDepth);
		//0); // max cu deltaqp depth
	}

	// "pps_cb_qp_offset"
	WriteSvlc(
		bitstreamPtr,
        0);// pcsPtr->cbQpOffset);

	// "pps_cr_qp_offset"
	WriteSvlc(
		bitstreamPtr,
        0);//pcsPtr->crQpOffset);

	// "pps_slice_chroma_qp_offsets_present_flag"
    WriteFlagCavlc(
        bitstreamPtr,
        EB_TRUE);// pcsPtr->sliceLevelChromaQpFlag);

	// "weighted_pred_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "weighted_bipred_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "transquant_bypass_enable_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);
	// "tiles_enabled_flag"
    WriteFlagCavlc(
        bitstreamPtr,
        tileMode);

	// "entropy_coding_sync_enabled_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

    if (tileMode == EB_TRUE) {

        // Tiles Number of Columns
        WriteUvlc(
            bitstreamPtr,
            scsPtr->staticConfig.tileColumnCount - 1);

        // Tiles Number of Rows
        WriteUvlc(
            bitstreamPtr,
            scsPtr->staticConfig.tileRowCount - 1);

        // Tiles Uniform Spacing Flag
        WriteCodeCavlc(
            bitstreamPtr,
//            scsPtr->tileUniformSpacing,
            1,
            1);

#if 0
        if (scsPtr->staticConfig.tileUniformSpacing == 0) {
            int syntaxItr;

            // Tile Column Width
            for (syntaxItr = 0; syntaxItr < (scsPtr->staticConfig.tileColumnCount - 1); ++syntaxItr) {
                // "column_width_minus1"
                WriteUvlc(
                    bitstreamPtr,
                    scsPtr->tileColumnWidthArray[syntaxItr] - 1);
            }

            // Tile Row Height
            for (syntaxItr = 0; syntaxItr < (scsPtr->tileRowCount - 1); ++syntaxItr) {
                // "row_height_minus1"
                WriteUvlc(
                    bitstreamPtr,
                    scsPtr->tileRowHeightArray[syntaxItr] - 1);
            }

        }
#endif
        // Loop filter across tiles
        //if(scsPtr->staticConfig.tileColumnCount != 1 || scsPtr->staticConfig.tileRowCount > 1) {
        WriteFlagCavlc(
            bitstreamPtr,
            0);
        //}
    }

	// "loop_filter_across_slices_enabled_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "deblocking_filter_control_present_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		1); 

	//if(pcPPS->getDeblockingFilterControlPresentFlag())
	{

		// "deblocking_filter_override_enabled_flag"
		WriteFlagCavlc(
			bitstreamPtr,
			1);

		// "pps_disable_deblocking_filter_flag"
		WriteFlagCavlc(
			bitstreamPtr,
			disableDlfFlag);

		if (!disableDlfFlag) {
			// "pps_beta_offset_div2"
			WriteSvlc(
				bitstreamPtr,
				0);

			// "pps_tc_offset_div2"
			WriteSvlc(
				bitstreamPtr,
				0);
		}
	}

	// "pps_scaling_list_data_present_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "lists_modification_present_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "log2_parallel_merge_level_minus2"
	WriteUvlc(
		bitstreamPtr,
		0); // Note: related to mv merge/skip parallel processing

	// "slice_header_extension_present_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	// "pps_extension_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		0);

	return;
}

static EB_U32 GetEntropyCoderGetBitstreamSize(EntropyCoder_t *entropyCoderPtr)
{
	CabacEncodeContext_t *cabacEncCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;
    OutputBitstreamUnit_t *bitstreamPtr = &cabacEncCtxPtr->bacEncContext.m_pcTComBitIf;
    unsigned payloadBytes = (cabacEncCtxPtr->bacEncContext.m_pcTComBitIf.writtenBitsCount) >> 3;
    FlushBitstream(bitstreamPtr);

    unsigned char *buf = (unsigned char *)bitstreamPtr->bufferBegin;
    unsigned int emulationCount = 0;

    //Count emulation counts 0x000001 and 0x000002
    if (payloadBytes >= 3) {
        for (unsigned i = 0; i <= payloadBytes - 3; i++) {
            if (buf[i] == 0 && buf[i+1] == 0 && (buf[i+2] <= 3)) {
                emulationCount++;
                i++;
            }
        }
    }

	return (payloadBytes + emulationCount);
}

static void CodeSliceHeader(
	EB_U32         firstLcuAddr,
	EB_U32         pictureQp,
	OutputBitstreamUnit_t *bitstreamPtr,
	PictureControlSet_t *pcsPtr)
{
	EB_S32  numBitesUsedByLcuAddr;
	EB_S32  lcuTotalCount = pcsPtr->lcuTotalCount;

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pcsPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

	SequenceControlSet_t     *sequenceControlSetPtr = (SequenceControlSet_t*)pcsPtr->sequenceControlSetWrapperPtr->objectPtr;

	EB_BOOL disableDlfFlag = sequenceControlSetPtr->staticConfig.disableDlfFlag;
    PictureParentControlSet_t *ppcsPtr = pcsPtr->ParentPcsPtr;

	EB_U32 sliceType = (pcsPtr->ParentPcsPtr->idrFlag == EB_TRUE) ? EB_I_PICTURE : pcsPtr->sliceType;
    EB_BOOL tileMode = (ppcsPtr->tileColumnCount > 1 || ppcsPtr->tileRowCount > 1) ? EB_TRUE : EB_FALSE;

	EB_U32 refPicsTotalCount =
		pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->negativeRefPicsTotalCount +
		pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->positiveRefPicsTotalCount;

	EB_U32 rpsSyntaxIndex;
	EB_S32 numBits;

	NalUnitType nalUnit = pcsPtr->ParentPcsPtr->nalUnit;

	// here someone can add an appropriated NalRefIdc type 
	//CodeNALUnitHeader (pcSlice->getNalUnitType(), NAL_REF_IDC_PRIORITY_HIGHEST, 1, true);

	// *Note - there seems to be a bug with the temporal layers in the NALU or I don't 
	// understand what the issue is.  Hardcoding to 0 for now...

	// Note - The NAL unit type has to be more sophisticated than this
	CodeNALUnitHeader(
		bitstreamPtr,
		nalUnit,
		pcsPtr->temporalId);

	// "first_slice_segment_in_pic_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		firstLcuAddr == 0);

	if (nalUnit == NAL_UNIT_CODED_SLICE_IDR_W_RADL
		|| nalUnit == NAL_UNIT_CODED_SLICE_IDR_N_LP
		|| nalUnit == NAL_UNIT_CODED_SLICE_BLA_N_LP
		|| nalUnit == NAL_UNIT_CODED_SLICE_BLA_W_RADL
		|| nalUnit == NAL_UNIT_CODED_SLICE_BLA_W_LP
		|| nalUnit == NAL_UNIT_CODED_SLICE_CRA)
	{

		// "no_output_of_prior_pics_flag"
		WriteFlagCavlc(
			bitstreamPtr,
			0);
	}

	// "slice_pic_parameter_set_id"
	// PPS ID
	WriteUvlc(
		bitstreamPtr,
		(pcsPtr->constrainedIntraFlag == EB_FALSE ? 0 : 1));

	if (firstLcuAddr > 0) {
		numBitesUsedByLcuAddr = 0;
		while (lcuTotalCount > (1 << numBitesUsedByLcuAddr))
		{
			++numBitesUsedByLcuAddr;
		}

		// "slice_address"
		WriteCodeCavlc(
			bitstreamPtr,
			firstLcuAddr,
			numBitesUsedByLcuAddr);
	}

	// "slice_type"

	// Slice Type 
	WriteUvlc(
		bitstreamPtr,
		sliceType);

	if (pcsPtr->ParentPcsPtr->idrFlag != EB_TRUE) {

		EB_U64 pocNumber = (EB_U32)(((EB_S32)(pcsPtr->ParentPcsPtr->pictureNumber - pcsPtr->ParentPcsPtr->lastIdrPicture) + (1 << sequenceControlSetPtr->bitsForPictureOrderCount)) & ((1 << sequenceControlSetPtr->bitsForPictureOrderCount) - 1));
		WriteCodeCavlc(
			bitstreamPtr,
			(EB_U32)pocNumber,
			(EB_U32)sequenceControlSetPtr->bitsForPictureOrderCount);

		// "short_term_ref_pic_set_sps_flag"
		WriteFlagCavlc(
			bitstreamPtr,
			pcsPtr->ParentPcsPtr->useRpsInSps);

		// Short Term RPS
		if (pcsPtr->ParentPcsPtr->useRpsInSps == EB_FALSE) {

			CodeShortTermRPS(
				bitstreamPtr,
				pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex],
				RPS_DEFAULT_VALUE,
				RPS_DEFAULT_VALUE,
				0,  // No RPS to send in SPS when dynamic GOP is ON
				pcsPtr->ParentPcsPtr->openGopCraFlag);


		}
		else {

			numBits = 0;
			while ((1 << numBits) < (EB_S32)sequenceControlSetPtr->predStructPtr->predStructEntryCount) {
				numBits++;
			}

			if (numBits > 0) {
				// "short_term_ref_pic_set_idx"
				WriteCodeCavlc(
					bitstreamPtr,
					pcsPtr->ParentPcsPtr->predStructIndex,
					numBits);
			}
		}

		// Long Term RPS
		if (pcsPtr->ParentPcsPtr->predStructPtr->longTermEnableFlag) {

			//NOTE: long term references are not handled properly. This below code needs to be modified

			CHECK_REPORT_ERROR_NC(
				encodeContextPtr->appCallbackPtr,
				EB_ENC_EC_ERROR11);

		}

		if (sequenceControlSetPtr->enableTmvpSps) {
			// "slice_temporal_mvp_enable_flag"
			WriteFlagCavlc(
				bitstreamPtr,
				!pcsPtr->ParentPcsPtr->disableTmvpFlag);
		}
	}

	if (sequenceControlSetPtr->staticConfig.enableSaoFlag) // Note: SAO is enabled
	{
		// "slice_sao_luma_flag"
		WriteFlagCavlc(
			bitstreamPtr,
			pcsPtr->saoFlag[0]);

		// "slice_sao_chroma_flag"
		WriteFlagCavlc(
			bitstreamPtr,
			pcsPtr->saoFlag[1]);
	}

	// RPS List Construction
	if (sliceType != EB_I_PICTURE) {

		// "num_ref_idx_active_override_flag"
		WriteFlagCavlc(
			bitstreamPtr,
			pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->refPicsOverrideTotalCountFlag);

		if (pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->refPicsOverrideTotalCountFlag) {

			// "num_ref_idx_l0_active_minus1"
			WriteUvlc(
				bitstreamPtr,
				pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->refPicsList0TotalCountMinus1);

			if (sliceType == EB_B_PICTURE){
				// "num_ref_idx_l1_active_minus1"
				WriteUvlc(
					bitstreamPtr,
					pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->refPicsList1TotalCountMinus1);
			}
		}
	}

	// RPS List Modification
	if (sliceType != EB_I_PICTURE) { // rd, Note: This needs to be changed

		// if( pcSlice->getPPS()->getListsModificationPresentFlag() && pcSlice->getNumRpsCurrTempList() > 1)
		if (pcsPtr->ParentPcsPtr->predStructPtr->listsModificationEnableFlag == EB_TRUE && refPicsTotalCount > 1){

			// "ref_pic_list_modification_flag_l0"
			WriteFlagCavlc(
				bitstreamPtr,
				pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->list0ModificationFlag);

			if (pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->list0ModificationFlag) {

				for (rpsSyntaxIndex = 0; rpsSyntaxIndex < refPicsTotalCount; rpsSyntaxIndex++) {
					// "list_entry_l0"
					WriteCodeCavlc(
						bitstreamPtr,
						pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->list0ModIndex[rpsSyntaxIndex],
						refPicsTotalCount);
				}
			}

			if (sliceType == EB_B_PICTURE){
				// "ref_pic_list_modification_flag_l1"
				WriteFlagCavlc(
					bitstreamPtr,
					pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->list1ModificationFlag);

				if (pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->list1ModificationFlag) {

					for (rpsSyntaxIndex = 0; rpsSyntaxIndex < refPicsTotalCount; rpsSyntaxIndex++) {
						// "list_entry_l1"
						WriteCodeCavlc(
							bitstreamPtr,
							pcsPtr->ParentPcsPtr->predStructPtr->predStructEntryPtrArray[pcsPtr->ParentPcsPtr->predStructIndex]->list1ModIndex[rpsSyntaxIndex],
							refPicsTotalCount);
					}
				}
			}
		}
	}

	if (sliceType == EB_B_PICTURE){
		// "mvd_l1_zero_flag"
		WriteFlagCavlc(
			bitstreamPtr,
			0);
	}

	if (!pcsPtr->ParentPcsPtr->disableTmvpFlag){
		if (pcsPtr->sliceType == EB_B_PICTURE){
			WriteFlagCavlc(
				bitstreamPtr,
				pcsPtr->colocatedPuRefList == REF_LIST_0);
		}
	}

	if (sliceType != EB_I_PICTURE) {
		// "five_minus_max_num_merge_cand"
		WriteUvlc(
			bitstreamPtr,
			0); // This has to be handled properly with mev merge being present
	}

	//Int iCode = pcSlice->getSliceQp() - ( pcSlice->getPPS()->getPicInitQPMinus26() + 26 );
	//WRITE_SVLC( iCode, "slice_qp_delta" );
	WriteSvlc(
		bitstreamPtr,
		pictureQp - 26);

	if (pcsPtr->sliceLevelChromaQpFlag) {
		// WRITE_SVLC( iCode, "slice_qp_delta_cb" );
		WriteSvlc(
			bitstreamPtr,
			pcsPtr->sliceCbQpOffset);
		// WRITE_SVLC( iCode, "slice_qp_delta_cr" );
		WriteSvlc(
			bitstreamPtr,
			pcsPtr->sliceCrQpOffset);
	}

	// Note: dlf override enable flag has been disabled (set to 0)
	//"deblocking_filter_override_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		1);
	//"slice_deblocking_filter_disabled_flag"
	WriteFlagCavlc(
		bitstreamPtr,
		disableDlfFlag);


	if (!disableDlfFlag){

		//"slice_beta_offset_div2"
		WriteSvlc(
			bitstreamPtr,
			(pcsPtr->betaOffset >> 1));

		//"slice_tc_offset_div2"
		WriteSvlc(
			bitstreamPtr,
			(pcsPtr->tcOffset >> 1));
	}

	//if (!disableDlfFlag || (sequenceControlSetPtr->staticConfig.enableSaoFlag && (pcsPtr->saoFlag[0] || pcsPtr->saoFlag[1]))) {
	//	// "slice_loop_filter_across_slices_enabled_flag"
	//	WriteFlagCavlc(
	//		bitstreamPtr,
	//		1);
	//}

    if (tileMode) {
        unsigned tileColumnNumMinus1 = ppcsPtr->tileColumnCount - 1;
        unsigned tileRowNumMinus1 = ppcsPtr->tileRowCount - 1;
        unsigned num_entry_point_offsets = sequenceControlSetPtr->staticConfig.tileSliceMode == 0 ? (ppcsPtr->tileColumnCount * ppcsPtr->tileRowCount - 1) : 0;

        if (tileColumnNumMinus1 > 0 || tileRowNumMinus1 > 0) {
            EB_U32 maxOffset = 0;
            EB_U32 offset[EB_TILE_MAX_COUNT];
            for (unsigned tileIdx = 0; tileIdx < num_entry_point_offsets; tileIdx++) {
                offset[tileIdx] = GetEntropyCoderGetBitstreamSize(pcsPtr->entropyCodingInfo[tileIdx]->entropyCoderPtr);
                if (offset[tileIdx] > maxOffset) {
                    maxOffset = offset[tileIdx];
                }
                //printf("tile %d, size %d\n", tileIdx, offset[tileIdx]);
            }

            EB_U32 offsetLenMinus1 = 0;
            while (maxOffset >= (1u << (offsetLenMinus1 + 1))) {
                offsetLenMinus1++;
                //assert(offsetLenMinus1 + 1 < 32);
            }

            // "num_entry_point_offsets"
            WriteUvlc(
                bitstreamPtr,
                num_entry_point_offsets);

            if (num_entry_point_offsets > 0) {
                WriteUvlc(
                        bitstreamPtr,
                        offsetLenMinus1);

                for (unsigned tileIdx = 0; tileIdx < num_entry_point_offsets; tileIdx++) {
                    //EB_U32 offset = GetEntropyCoderGetBitstreamSize(pcsPtr->entropyCodingInfo[tileIdx]->entropyCoderPtr);
                    WriteCodeCavlc(bitstreamPtr, offset[tileIdx] - 1, offsetLenMinus1 + 1);
                }
            }
        }
    }

	// Byte Alignment

	//pcBitstreamOut->write( 1, 1 );
	OutputBitstreamWrite(
		bitstreamPtr,
		1,
		1);

	//pcBitstreamOut->writeAlignZero();
	OutputBitstreamWriteAlignZero(
		bitstreamPtr);

}

EB_ERRORTYPE EncodeTileFinish(
    EntropyCoder_t        *entropyCoderPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    CabacEncodeContext_t *cabacEncodeCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;

    // Add tile terminate bit (0x1)
    BacEncContextTerminate(
        &(cabacEncodeCtxPtr->bacEncContext),
        1);

    BacEncContextFinish(&(cabacEncodeCtxPtr->bacEncContext));

    OutputBitstreamWrite(
        &(cabacEncodeCtxPtr->bacEncContext.m_pcTComBitIf),
        1,
        1);

    OutputBitstreamWriteAlignZero(
        &(cabacEncodeCtxPtr->bacEncContext.m_pcTComBitIf));

    return return_error;
}

EB_ERRORTYPE EncodeLcuSaoParameters(
	LargestCodingUnit_t   *tbPtr,
	EntropyCoder_t        *entropyCoderPtr,
	EB_BOOL                saoLumaSliceEnable,
	EB_BOOL                saoChromaSliceEnable,
	EB_U8                  bitdepth)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	CabacEncodeContext_t *cabacEncodeCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;

	// Note: Merge should be disabled across tiles and slices
	// This needs to be revisited when there is more than one slice per tile
	// Code Luma SAO parameters
	// Code Luma SAO parameters
    if (tbPtr->lcuEdgeInfoPtr->tileLeftEdgeFlag == EB_FALSE) {
		EncodeSaoMerge(
			cabacEncodeCtxPtr,
			tbPtr->saoParams.saoMergeLeftFlag);
	} else {
		tbPtr->saoParams.saoMergeLeftFlag = EB_FALSE;
	}

	if (tbPtr->saoParams.saoMergeLeftFlag == 0) {
        if (tbPtr->lcuEdgeInfoPtr->tileTopEdgeFlag == EB_FALSE) {
			EncodeSaoMerge(
				cabacEncodeCtxPtr,
				tbPtr->saoParams.saoMergeUpFlag);
		} else {
			tbPtr->saoParams.saoMergeUpFlag = EB_FALSE;
		}
	}

	if (tbPtr->saoParams.saoMergeLeftFlag == 0 && tbPtr->saoParams.saoMergeUpFlag == 0) {

		// Luma
		if (saoLumaSliceEnable) {
			EncodeSaoOffsets(
				cabacEncodeCtxPtr,
				0, // Y
				tbPtr->saoParams.saoTypeIndex,
				tbPtr->saoParams.saoOffset[0],
				tbPtr->saoParams.saoBandPosition[0],
				bitdepth);
		}

		// Chroma
		if (saoChromaSliceEnable) {
			// Cb
			EncodeSaoOffsets(
				cabacEncodeCtxPtr,
				1, // Cb
				tbPtr->saoParams.saoTypeIndex,
				tbPtr->saoParams.saoOffset[1],
				tbPtr->saoParams.saoBandPosition[1],
				bitdepth);
			// Cr
			EncodeSaoOffsets(
				cabacEncodeCtxPtr,
				2, // Cr
				tbPtr->saoParams.saoTypeIndex,
				tbPtr->saoParams.saoOffset[2],
				tbPtr->saoParams.saoBandPosition[2],
				bitdepth);
		}

	}

	return return_error;
}

/*******************************************
* Entropy Coding - Assign Delta Qp
*******************************************/
static void EntropyCodingUpdateQp(
	CodingUnit_t           *cuPtr,
	EB_BOOL                 availableCoeff,
	EB_U32                  cuOriginX,
	EB_U32                  cuOriginY,
	EB_U32                  cuSize,
	EB_U32                  lcuSize,
	EB_BOOL                 isDeltaQpEnable,
	EB_BOOL                *isDeltaQpNotCoded,
	EB_U32                  difCuDeltaQpDepth,
	EB_U8                  *prevCodedQp,
	EB_U8                  *prevQuantGroupCodedQp,
    EB_U8                   lcuQp,
	
	PictureControlSet_t     *pictureControlSetPtr,
	EB_U32                  pictureOriginX,
	EB_U32                  pictureOriginY)
{
    EB_U8  refQp;
	EB_U8  qp;
	EB_U32 log2MinCuQpDeltaSize = Log2f(lcuSize) - difCuDeltaQpDepth;
	EB_S32 qpTopNeighbor = 0;
	EB_S32 qpLeftNeighbor = 0;
	EB_BOOL newQuantGroup;
	EB_U32 quantGroupX = cuOriginX - (cuOriginX & ((1 << log2MinCuQpDeltaSize) - 1));
	EB_U32 quantGroupY = cuOriginY - (cuOriginY & ((1 << log2MinCuQpDeltaSize) - 1));
	EB_BOOL sameLcuCheckTop = (((quantGroupY - 1) >> Log2f(MAX_LCU_SIZE)) == ((quantGroupY) >> Log2f(MAX_LCU_SIZE))) ? EB_TRUE : EB_FALSE;
	EB_BOOL sameLcuCheckLeft = (((quantGroupX - 1) >> Log2f(MAX_LCU_SIZE)) == ((quantGroupX) >> Log2f(MAX_LCU_SIZE))) ? EB_TRUE : EB_FALSE;

	// Neighbor Array
	EB_U32 qpLeftNeighborIndex = 0;
	EB_U32 qpTopNeighborIndex = 0;

	// CU larger than the quantization group
	if (Log2f(cuSize) >= log2MinCuQpDeltaSize){
		*isDeltaQpNotCoded = EB_TRUE;
	}

	// At the beginning of a new quantization group
	if (((cuOriginX & ((1 << log2MinCuQpDeltaSize) - 1)) == 0) &&
		((cuOriginY & ((1 << log2MinCuQpDeltaSize) - 1)) == 0))
	{
		*isDeltaQpNotCoded = EB_TRUE;
		newQuantGroup = EB_TRUE;
	}
	else {
		newQuantGroup = EB_FALSE;
	}

	// setting the previous Quantization Group QP
	if (newQuantGroup == EB_TRUE) {
		*prevCodedQp = *prevQuantGroupCodedQp;
	}

	if ((quantGroupY>pictureOriginY) && sameLcuCheckTop)  {
		qpTopNeighborIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			quantGroupX,
			quantGroupY - 1,
			pictureControlSetPtr->qpArrayStride);
		qpTopNeighbor = pictureControlSetPtr->entropyQpArray[qpTopNeighborIndex];
	}
	else {
		qpTopNeighbor = *prevCodedQp;
	}

	if ((quantGroupX > pictureOriginX) && sameLcuCheckLeft)  {
		qpLeftNeighborIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			quantGroupX - 1,
			quantGroupY,
			pictureControlSetPtr->qpArrayStride);

		qpLeftNeighbor = pictureControlSetPtr->entropyQpArray[qpLeftNeighborIndex];
	}
	else {
		qpLeftNeighbor = *prevCodedQp;
	}
	refQp = (EB_U8)(qpLeftNeighbor + qpTopNeighbor + 1) >> 1;

	qp = cuPtr->qp;

	// Update the State info
	if (isDeltaQpEnable) {
		if (*isDeltaQpNotCoded) {
			if (availableCoeff){
				qp = cuPtr->qp;
				*prevCodedQp = qp;
				*prevQuantGroupCodedQp = qp;
				*isDeltaQpNotCoded = EB_FALSE;
			}
			else{
				qp = refQp;
				*prevQuantGroupCodedQp = qp;
			}
		}
	}
	else{
		qp = lcuQp;
	}
	cuPtr->qp = qp;
	cuPtr->refQp = refQp;

	return;
}

/*********************************************************************
* Intra4x4CheckAndCodeDeltaQp
*   Check the conditions and codes the change in the QP indicated by delta QP
*
*  cabacEncContextPtr
*   pointer to the CABAC structure passed as input
*
*  cuPtr
*   input pointer to the coding unit
*
*  tuPtr
*   input pointer to the transform unit
*********************************************************************/
EB_ERRORTYPE Intra4x4CheckAndCodeDeltaQp(
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	CodingUnit_t           *cuPtr,
	TransformUnit_t        *tuPtr,
	EB_BOOL                 isDeltaQpEnable,
	EB_BOOL                *isdeltaQpNotCoded)
{

	EB_ERRORTYPE return_error = EB_ErrorNone;

	if (isDeltaQpEnable) {
        EB_BOOL cbfChroma = 0;
        if (cabacEncodeCtxPtr->colorFormat == EB_YUV444) {
            cbfChroma = tuPtr->cbCbf || tuPtr->crCbf;
        } else if (cabacEncodeCtxPtr->colorFormat == EB_YUV422) {
            cbfChroma = (cuPtr->transformUnitArray[1].cbCbf ||
                        cuPtr->transformUnitArray[1].crCbf ||
                        cuPtr->transformUnitArray[3].cbCbf ||
                        cuPtr->transformUnitArray[3].crCbf);
        } else {
            cbfChroma = (cuPtr->transformUnitArray[1].cbCbf ||
                        cuPtr->transformUnitArray[1].crCbf);
        }
		if (tuPtr->lumaCbf || cbfChroma) {
			if (*isdeltaQpNotCoded){
				EB_S32  deltaQp;
				deltaQp = cuPtr->qp - cuPtr->refQp;

				EncodeDeltaQp(
					cabacEncodeCtxPtr,
					deltaQp);

				*isdeltaQpNotCoded = EB_FALSE;
			}
		}
	}

	return return_error;
}

static EB_ERRORTYPE Intra4x4EncodeLumaCoeff(
    EB_U8                    intraLumaMode,
	CabacEncodeContext_t    *cabacEncodeCtxPtr,
	CodingUnit_t            *cuPtr,
	TransformUnit_t         *tuPtr,
	EB_U32                   tuOriginX,
	EB_U32                   tuOriginY,
	EbPictureBufferDesc_t   *coeffPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_S16  *coeffBuffer;
	EB_U32   coeffLocation;
	EB_U32   countNonZeroCoeffs = 0;

	coeffLocation = tuOriginX + (tuOriginY * coeffPtr->strideY);
	coeffBuffer = (EB_S16*)&coeffPtr->bufferY[coeffLocation * sizeof(EB_S16)];

	if (tuPtr->lumaCbf){

		ComputeNumofSigCoefficients(
			coeffBuffer,
			coeffPtr->strideY,
			MIN_PU_SIZE,
			&countNonZeroCoeffs);

		EncodeQuantizedCoefficientsFuncArray[!!(ASM_TYPES & PREAVX2_MASK)](
			cabacEncodeCtxPtr,
			MIN_PU_SIZE,
			(EB_MODETYPE)cuPtr->predictionModeFlag,
			intraLumaMode,
            EB_INTRA_CHROMA_DM,
			coeffBuffer,
			coeffPtr->strideY,
			COMPONENT_LUMA,
			tuPtr);
	}

	return return_error;
}

static EB_ERRORTYPE Intra4x4EncodeChromaCoeff(
    EB_U8                    intraLumaMode,
	CabacEncodeContext_t    *cabacEncodeCtxPtr,
	CodingUnit_t            *cuPtr,
	EB_U32                   tuOriginX,
	EB_U32                   tuOriginY,
    EB_U32                   tuIndex, //For 444 case, 422/420 can ignore this flag
	EbPictureBufferDesc_t   *coeffPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	TransformUnit_t         *tuPtr = NULL;
	EB_S16  *coeffBuffer;
	EB_U32   coeffLocation;
	EB_U32   countNonZeroCoeffs = 0;
    const EB_U16 subWidthCMinus1 = (cabacEncodeCtxPtr->colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (cabacEncodeCtxPtr->colorFormat >= EB_YUV422 ? 1 : 2) - 1;

	// cb
    for (int tIdx = 0; tIdx < (cabacEncodeCtxPtr->colorFormat == EB_YUV422 ? 2 : 1); tIdx++) {
        // Get the correct TU block for 444, not always the 1st one
        tuPtr=&cuPtr->transformUnitArray[tuIndex + 1 + 2 * tIdx]; //1,3 for 422 chroma
        coeffLocation = (tuOriginX >> subWidthCMinus1) +
            (((tuOriginY + MIN_PU_SIZE * tIdx) * coeffPtr->strideCb) >> subHeightCMinus1);
        coeffBuffer = (EB_S16*)&coeffPtr->bufferCb[coeffLocation * sizeof(EB_S16)];

        if (tuPtr->cbCbf){
            ComputeNumofSigCoefficients(
                    coeffBuffer,
                    coeffPtr->strideCb,
                    MIN_PU_SIZE,
                    &countNonZeroCoeffs);

            EncodeQuantizedCoefficientsFuncArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    cabacEncodeCtxPtr,
                    MIN_PU_SIZE,
                    (EB_MODETYPE)cuPtr->predictionModeFlag,
                    intraLumaMode,
                    EB_INTRA_CHROMA_DM,
                    coeffBuffer,
                    coeffPtr->strideCb,
                    COMPONENT_CHROMA_CB,
                    tuPtr);
        }
    }

	// cr
    for (int tIdx=0; tIdx<(cabacEncodeCtxPtr->colorFormat==EB_YUV422?2:1); tIdx++) {
        tuPtr=&cuPtr->transformUnitArray[tuIndex + 1 + 2 * tIdx]; //1,3 for 422 chroma
        //coeffLocation = ((tuOriginX + ((tuOriginY+MIN_PU_SIZE*tIdx) * coeffPtr->strideCb)) >> 1);
        coeffLocation = (tuOriginX >> subWidthCMinus1) +
            (((tuOriginY + MIN_PU_SIZE * tIdx) * coeffPtr->strideCb) >> subHeightCMinus1);
        coeffBuffer = (EB_S16*)&coeffPtr->bufferCr[coeffLocation * sizeof(EB_S16)];

        if (tuPtr->crCbf){
            ComputeNumofSigCoefficients(
                    coeffBuffer,
                    coeffPtr->strideCr,
                    MIN_PU_SIZE,
                    &countNonZeroCoeffs);

            EncodeQuantizedCoefficientsFuncArray[!!(ASM_TYPES & PREAVX2_MASK)](
                    cabacEncodeCtxPtr,
                    MIN_PU_SIZE,
                    (EB_MODETYPE)cuPtr->predictionModeFlag,
                    intraLumaMode,
                    EB_INTRA_CHROMA_DM,
                    coeffBuffer,
                    coeffPtr->strideCr,
                    COMPONENT_CHROMA_CR,
                    tuPtr);
        }
    }

	return return_error;
}

static EB_ERRORTYPE Intra4x4EncodeCoeff(
    LargestCodingUnit_t    *tbPtr,
	CabacEncodeContext_t   *cabacEncodeCtxPtr,
	CodingUnit_t           *cuPtr,
	const CodedUnitStats_t *cuStatsPtr,
	EbPictureBufferDesc_t  *coeffPtr,
	EB_U32                 *cuQuantizedCoeffsBits,
	EB_BOOL                 isDeltaQpEnable,
	EB_BOOL                *isdeltaQpNotCoded)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	TransformUnit_t             *tuPtr;
	const TransformUnitStats_t  *tuStatsPtr;
	EB_U32                       tuIndex;
	EB_U32                       tuOriginX;
	EB_U32                       tuOriginY;

	EB_U32  cbfContext;
	EB_U32  puIndex;

	//rate Control
	EB_U32  writtenBitsBeforeQuantizedCoeff;
	EB_U32  writtenBitsAfterQuantizedCoeff;
    EB_BOOL sum_cbCbf;
    EB_BOOL sum_crCbf;

	//store the number of written bits before coding quantized coeffs (flush is not called yet): 
	// The total number of bits is 
	// number of written bits
	// + 32  - bits remaining in interval Low Value
	// + number of buffered byte * 8
	// This should be only for coeffs not any flag
	writtenBitsBeforeQuantizedCoeff = cabacEncodeCtxPtr->bacEncContext.m_pcTComBitIf.writtenBitsCount +
		32 - cabacEncodeCtxPtr->bacEncContext.bitsRemainingNum +
		(cabacEncodeCtxPtr->bacEncContext.tempBufferedBytesNum << 3);

	// Get Chroma Cbf context
	cbfContext = 0;

    sum_cbCbf = (cabacEncodeCtxPtr->colorFormat != EB_YUV444) ?
        (&cuPtr->transformUnitArray[1])->cbCbf :
        ((&cuPtr->transformUnitArray[1])->cbCbf |
         (&cuPtr->transformUnitArray[2])->cbCbf |
         (&cuPtr->transformUnitArray[3])->cbCbf |
         (&cuPtr->transformUnitArray[4])->cbCbf);

    sum_crCbf = (cabacEncodeCtxPtr->colorFormat != EB_YUV444) ?
        (&cuPtr->transformUnitArray[1])->crCbf :
        ((&cuPtr->transformUnitArray[1])->crCbf |
         (&cuPtr->transformUnitArray[2])->crCbf |
         (&cuPtr->transformUnitArray[3])->crCbf |
         (&cuPtr->transformUnitArray[4])->crCbf);

	//  Cb CBF
	EncodeOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
        sum_cbCbf,
		&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));

    if (cabacEncodeCtxPtr->colorFormat == EB_YUV422) {
        EncodeOneBin(
                &(cabacEncodeCtxPtr->bacEncContext),
                (&cuPtr->transformUnitArray[3])->cbCbf,
                &(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
    }
	// Cr CBF  
	EncodeOneBin(
		&(cabacEncodeCtxPtr->bacEncContext),
        sum_crCbf,
		&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
    if (cabacEncodeCtxPtr->colorFormat == EB_YUV422) {
        EncodeOneBin(
                &(cabacEncodeCtxPtr->bacEncContext),
                (&cuPtr->transformUnitArray[3])->crCbf,
                &(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
    }

	// Get Luma Cbf context

	// Encode Intra 4x4 data
	for (puIndex = 0; puIndex < 4; puIndex++) {
		tuIndex = puIndex + 1;
		tuPtr = &cuPtr->transformUnitArray[tuIndex];
		tuStatsPtr = GetTransformUnitStats(tuIndex);
		tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatsPtr->offsetX);
		tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatsPtr->offsetY);

        if (cabacEncodeCtxPtr->colorFormat == EB_YUV444) {
            cbfContext = 1;
            if (sum_cbCbf) {
                EncodeOneBin(
                        &(cabacEncodeCtxPtr->bacEncContext),
                        tuPtr->cbCbf,
                        &(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
            }

            if (sum_crCbf) {
                EncodeOneBin(
                        &(cabacEncodeCtxPtr->bacEncContext),
                        tuPtr->crCbf,
                        &(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext + NUMBER_OF_CBF_CONTEXT_MODELS]));
            }
        }

	    cbfContext = 0;
		EncodeOneBin(
			&(cabacEncodeCtxPtr->bacEncContext),
			tuPtr->lumaCbf,
			&(cabacEncodeCtxPtr->contextModelEncContext.cbfContextModel[cbfContext]));

		//EncodeDeltaQp
		Intra4x4CheckAndCodeDeltaQp(
			cabacEncodeCtxPtr,
			cuPtr,
			tuPtr,
			isDeltaQpEnable,
			isdeltaQpNotCoded);

		// Encode Luma coeff
		Intra4x4EncodeLumaCoeff(
            tbPtr->intra4x4Mode[((MD_SCAN_TO_RASTER_SCAN[cuPtr->leafIndex] - 21) << 2) + puIndex],
			cabacEncodeCtxPtr,
			cuPtr,
			tuPtr,
			tuOriginX,
			tuOriginY,
			coeffPtr);

        if (cabacEncodeCtxPtr->colorFormat == EB_YUV444) {
            // residual coding for Cb/Cr
            Intra4x4EncodeChromaCoeff(
                    tbPtr->intra4x4Mode[((MD_SCAN_TO_RASTER_SCAN[cuPtr->leafIndex] - 21) << 2) + puIndex],
                    cabacEncodeCtxPtr,
                    cuPtr,
                    tuOriginX,
                    tuOriginY,
                    puIndex,
                    coeffPtr);
        }
	}

    if (cabacEncodeCtxPtr->colorFormat != EB_YUV444) {
        // Encode Chroma coeff for non-444 case, 
        // Jing TODO: see if can move to above loop
        tuStatsPtr = GetTransformUnitStats(1);
        tuOriginX = TU_ORIGIN_ADJUST(cuStatsPtr->originX, cuStatsPtr->size, tuStatsPtr->offsetX);
        tuOriginY = TU_ORIGIN_ADJUST(cuStatsPtr->originY, cuStatsPtr->size, tuStatsPtr->offsetY);

        Intra4x4EncodeChromaCoeff(
                tbPtr->intra4x4Mode[((MD_SCAN_TO_RASTER_SCAN[cuPtr->leafIndex] - 21) << 2)],
                cabacEncodeCtxPtr,
                cuPtr,
                tuOriginX,
                tuOriginY,
                0,
                coeffPtr);
    }

	//store the number of written bits after coding quantized coeffs (flush is not called yet): 
	// The total number of bits is 
	// number of written bits
	// + 32  - bits remaining in interval Low Value
	// + number of buffered byte * 8
	writtenBitsAfterQuantizedCoeff = cabacEncodeCtxPtr->bacEncContext.m_pcTComBitIf.writtenBitsCount +
		32 - cabacEncodeCtxPtr->bacEncContext.bitsRemainingNum +
		(cabacEncodeCtxPtr->bacEncContext.tempBufferedBytesNum << 3);

	*cuQuantizedCoeffsBits = writtenBitsAfterQuantizedCoeff - writtenBitsBeforeQuantizedCoeff;


	return return_error;
}

/**********************************************
* Encode Lcu
**********************************************/
EB_ERRORTYPE EncodeLcu(
	LargestCodingUnit_t     *tbPtr,
	EB_U32                   lcuOriginX,
	EB_U32                   lcuOriginY,
	PictureControlSet_t     *pictureControlSetPtr,
	EB_U32                   lcuSize,
	EntropyCoder_t          *entropyCoderPtr,
	EbPictureBufferDesc_t   *coeffPtr,
	NeighborArrayUnit_t     *modeTypeNeighborArray,
	NeighborArrayUnit_t     *leafDepthNeighborArray,
	NeighborArrayUnit_t     *intraLumaModeNeighborArray,
	NeighborArrayUnit_t     *skipFlagNeighborArray,
	EB_U16                   tileIdx,
	EB_U32                   pictureOriginX,
	EB_U32                   pictureOriginY)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    CabacEncodeContext_t     *cabacEncodeCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;
    SequenceControlSet_t     *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
    EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    // CU Varaiables
    const CodedUnitStats_t   *cuStatsPtr;
    CodingUnit_t             *cuPtr;
    EB_U32                    cuIndex = 0;
    EB_REFLIST                refList;
    EB_U32                    maxRefList;
    EB_U32                    cuOriginX;
    EB_U32                    cuOriginY;
    EB_U32                    cuSize;
    EB_U8                     cuDepth;
    EB_BOOL                   availableCoeff;
    cabacEncodeCtxPtr->colorFormat = pictureControlSetPtr->colorFormat;

    // PU Varaiables
    PredictionUnit_t         *puPtr;
    EB_U32                    cuQuantizedCoeffsBits;
    EB_U32                    log2MinCuQpDeltaSize = Log2f(MAX_LCU_SIZE) - pictureControlSetPtr->difCuDeltaQpDepth;

    tbPtr->quantizedCoeffsBits = 0;
    EB_BOOL  entropyDeltaQpNotCoded = EB_TRUE;
    EB_BOOL  deltaQpNotCoded = EB_TRUE;
    EB_BOOL checkCuOutOfBound = EB_FALSE;
    LcuParams_t * lcuParam = &sequenceControlSetPtr->lcuParamsArray[tbPtr->index];
    if (!(lcuParam->isCompleteLcu)){
        checkCuOutOfBound = EB_TRUE;
    }
    do {
        EB_BOOL codeCuCond = EB_TRUE; // Code cu only if it is inside the picture
        cuPtr = tbPtr->codedLeafArrayPtr[cuIndex];

        if (checkCuOutOfBound)
            codeCuCond = (EB_BOOL)lcuParam->rasterScanCuValidity[MD_SCAN_TO_RASTER_SCAN[cuIndex]]; // check if cu is inside the picture

        if (codeCuCond){
            // CU Stats
            cuStatsPtr = GetCodedUnitStats(cuIndex);
            cuSize = cuStatsPtr->size;
            cuOriginX = lcuOriginX + cuStatsPtr->originX;
            cuOriginY = lcuOriginY + cuStatsPtr->originY;
            cuDepth = (EB_U8)cuStatsPtr->depth;

            // Code Split Flag         
            EncodeSplitFlag(
                cabacEncodeCtxPtr,
                cuDepth,
                pictureControlSetPtr->lcuMaxDepth,
                (EB_BOOL)cuPtr->splitFlag,
                cuOriginX,
                cuOriginY,
                modeTypeNeighborArray,
                leafDepthNeighborArray);
            if (cuStatsPtr->sizeLog2 >= log2MinCuQpDeltaSize){
                deltaQpNotCoded = EB_TRUE;
            }
            if (((cuStatsPtr->originY & ((1 << log2MinCuQpDeltaSize) - 1)) == 0) &&
                ((cuStatsPtr->originX & ((1 << log2MinCuQpDeltaSize) - 1)) == 0)){
                deltaQpNotCoded = EB_TRUE;
            }

            if (cuPtr->splitFlag == EB_FALSE){
                if (cuPtr->predictionModeFlag == INTRA_MODE &&
                        cuPtr->predictionUnitArray->intraLumaMode == EB_INTRA_MODE_4x4) {
                    availableCoeff = (
					cuPtr->transformUnitArray[1].lumaCbf ||
					cuPtr->transformUnitArray[2].lumaCbf ||
					cuPtr->transformUnitArray[3].lumaCbf ||
					cuPtr->transformUnitArray[4].lumaCbf ||
                        cuPtr->transformUnitArray[1].crCbf ||
                        cuPtr->transformUnitArray[1].cbCbf ||
                        cuPtr->transformUnitArray[2].crCbf ||
                        cuPtr->transformUnitArray[2].cbCbf ||
                        cuPtr->transformUnitArray[3].crCbf ||
                        cuPtr->transformUnitArray[3].cbCbf ||
                        cuPtr->transformUnitArray[4].crCbf || // 422 case will use 3rd 4x4 for the 2nd chroma
                        cuPtr->transformUnitArray[4].cbCbf) ? EB_TRUE : EB_FALSE;
                } else {
                    availableCoeff = (cuPtr->predictionModeFlag == INTER_MODE) ? (EB_BOOL)cuPtr->rootCbf :
                        (cuPtr->transformUnitArray[cuSize == sequenceControlSetPtr->lcuSize ? 1 : 0].lumaCbf ||
                        cuPtr->transformUnitArray[cuSize == sequenceControlSetPtr->lcuSize ? 1 : 0].crCbf ||
                        cuPtr->transformUnitArray[cuSize == sequenceControlSetPtr->lcuSize ? 1 : 0].crCbf2 ||
                        cuPtr->transformUnitArray[cuSize == sequenceControlSetPtr->lcuSize ? 1 : 0].cbCbf ||
                        cuPtr->transformUnitArray[cuSize == sequenceControlSetPtr->lcuSize ? 1 : 0].cbCbf2) ? EB_TRUE : EB_FALSE;
                }

                EntropyCodingUpdateQp(
                    cuPtr,
                    availableCoeff,
                    cuOriginX,
                    cuOriginY,
                    cuSize,
                    sequenceControlSetPtr->lcuSize,
					sequenceControlSetPtr->staticConfig.improveSharpness || sequenceControlSetPtr->staticConfig.bitRateReduction ? EB_TRUE : EB_FALSE,
                    &entropyDeltaQpNotCoded,
                    pictureControlSetPtr->difCuDeltaQpDepth,
                    &pictureControlSetPtr->prevCodedQp[tileIdx],
                    &pictureControlSetPtr->prevQuantGroupCodedQp[tileIdx],
                    tbPtr->qp,
                    pictureControlSetPtr,
					pictureOriginX,
					pictureOriginY);

                // Assign DLF QP
                entropySetQpArrayBasedOnCU(
                    pictureControlSetPtr,
                    cuOriginX,
                    cuOriginY,
                    cuSize,
                    cuSize,
                    cuPtr->qp);

                // Code the skip flag
                if (pictureControlSetPtr->sliceType == EB_P_PICTURE || pictureControlSetPtr->sliceType == EB_B_PICTURE)
                {
                    EncodeSkipFlag(
                        cabacEncodeCtxPtr,
                        (EB_BOOL)cuPtr->skipFlag,
                        cuOriginX,
                        cuOriginY,
                        modeTypeNeighborArray,
                        skipFlagNeighborArray);
                }

                if (cuPtr->skipFlag)
                {
                    // Merge Index
                    EncodeMergeIndex(
                        cabacEncodeCtxPtr,
                        &cuPtr->predictionUnitArray[0]);
                }
                else
                {
                    // Code CU pred mode (I, P, B, etc.)
                    // (not needed for Intra Slice)
                    if (pictureControlSetPtr->sliceType == EB_P_PICTURE || pictureControlSetPtr->sliceType == EB_B_PICTURE)
                    {
                        EncodePredictionMode(
                            cabacEncodeCtxPtr,
                            cuPtr);
                    }

                    switch (cuPtr->predictionModeFlag) {
                    case INTRA_MODE: 
                        if (cuPtr->predictionModeFlag == INTRA_MODE &&
                                cuPtr->predictionUnitArray->intraLumaMode == EB_INTRA_MODE_4x4) {
                            // Code Partition Size
                            EncodeIntra4x4PartitionSize(
                                cabacEncodeCtxPtr,
                                cuPtr,
                                pictureControlSetPtr->lcuMaxDepth);

                            // Get the PU Ptr
                            puPtr = cuPtr->predictionUnitArray;

              
                            EB_U8 partitionIndex;

                            EB_U8 intraLumaLeftModeArray[4];
                            EB_U8 intraLumaTopModeArray[4];

                            EB_U8 intraLumaLeftMode;
                            EB_U8 intraLumaTopMode;

                            // Partition Loop
                            for (partitionIndex = 0; partitionIndex < 4; partitionIndex++) {

                                EB_U8 intraLumaMode      = tbPtr->intra4x4Mode[((MD_SCAN_TO_RASTER_SCAN[cuIndex] - 21) << 2) + partitionIndex];
                                EB_U8 predictionModeFlag = INTRA_MODE;

                                EB_U32 partitionOriginX = cuOriginX + INTRA_4x4_OFFSET_X[partitionIndex];
                                EB_U32 partitionOriginY = cuOriginY + INTRA_4x4_OFFSET_Y[partitionIndex];

                                // Code Luma Mode for Intra First Stage
                                EncodeIntraLumaModeFirstStage(
                                    cabacEncodeCtxPtr,
                                    partitionOriginX,
                                    partitionOriginY,
                                    lcuSize,
                                    &intraLumaLeftMode,
                                    &intraLumaTopMode,
                                    intraLumaMode,
                                    modeTypeNeighborArray,
                                    intraLumaModeNeighborArray);
                                
                                intraLumaLeftModeArray[partitionIndex] = intraLumaLeftMode;
                                intraLumaTopModeArray[partitionIndex]  = intraLumaTopMode;

                                NeighborArrayUnitModeWrite(
                                    intraLumaModeNeighborArray,
                                    (EB_U8*)&intraLumaMode,
                                    partitionOriginX,
                                    partitionOriginY,
                                    MIN_PU_SIZE,
                                    MIN_PU_SIZE,
                                    NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);
             
                                NeighborArrayUnitModeWrite(
                                    modeTypeNeighborArray,
                                    &predictionModeFlag,
                                    partitionOriginX,
                                    partitionOriginY,
                                    MIN_PU_SIZE,
                                    MIN_PU_SIZE,
                                    NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

                            }

                            for (partitionIndex = 0; partitionIndex < 4; partitionIndex++) {
                                EB_U8 intraLumaMode = tbPtr->intra4x4Mode[((MD_SCAN_TO_RASTER_SCAN[cuIndex] - 21) << 2) + partitionIndex];

                                // Code Luma Mode for Intra Second Stage
                                EncodeIntraLumaModeSecondStage(
                                    cabacEncodeCtxPtr,
                                    intraLumaLeftModeArray[partitionIndex],
                                    intraLumaTopModeArray[partitionIndex],
                                    intraLumaMode);
                            }
                            
                            // Code Chroma Mode for Intra
                            for (partitionIndex = 0;
                                    partitionIndex < ((cabacEncodeCtxPtr->colorFormat == EB_YUV444) ? 4 : 1);
                                    partitionIndex++) {
                                EncodeIntraChromaMode(cabacEncodeCtxPtr);
                            }

                            // Encode Transform Unit Split & CBFs
							Intra4x4EncodeCoeff(
                                tbPtr,
								cabacEncodeCtxPtr,
								cuPtr,
								cuStatsPtr,
								coeffPtr,
								&cuQuantizedCoeffsBits,
								(EB_BOOL)pictureControlSetPtr->useDeltaQp,
								&deltaQpNotCoded);

                            tbPtr->quantizedCoeffsBits += cuQuantizedCoeffsBits;
                        } else {
                            // Code Partition Size
                            EncodePartitionSize(
                                    cabacEncodeCtxPtr,
                                    cuPtr,
                                    pictureControlSetPtr->lcuMaxDepth);

                            EB_U8 intraLumaLeftMode;
                            EB_U8 intraLumaTopMode;
                            EB_U8 intraLumaMode;

                            // Get the PU Ptr
                            puPtr = cuPtr->predictionUnitArray;
                            // Code Luma Mode for Intra First Stage
                            EncodeIntraLumaModeFirstStage(
                                    cabacEncodeCtxPtr,
                                    cuOriginX,
                                    cuOriginY,
                                    lcuSize,
                                    &intraLumaLeftMode,
                                    &intraLumaTopMode,
                                    puPtr->intraLumaMode,
                                    modeTypeNeighborArray,
                                    intraLumaModeNeighborArray);

                            intraLumaMode = (EB_U8)puPtr->intraLumaMode;

                            NeighborArrayUnitModeWrite(
                                    intraLumaModeNeighborArray,
                                    (EB_U8*)&intraLumaMode,
                                    cuOriginX,
                                    cuOriginY,
                                    cuSize,
                                    cuSize,
                                    NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

                            {
                                EB_U8 predictionModeFlag = (EB_U8)cuPtr->predictionModeFlag;
                                NeighborArrayUnitModeWrite(
                                        modeTypeNeighborArray,
                                        &predictionModeFlag,
                                        cuOriginX,
                                        cuOriginY,
                                        cuSize,
                                        cuSize,
                                        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);
                            }

                            // Get PU Ptr
                            puPtr = &cuPtr->predictionUnitArray[0];

                            // Code Luma Mode for Intra Second Stage
                            EncodeIntraLumaModeSecondStage(
                                    cabacEncodeCtxPtr,
                                    intraLumaLeftMode,
                                    intraLumaTopMode,
                                    puPtr->intraLumaMode);

                            // Code Chroma Mode for Intra
                            EncodeIntraChromaMode(
                                    cabacEncodeCtxPtr);
                            EncodeTuSplitCoeff(
                                    cabacEncodeCtxPtr,
                                    cuPtr,
                                    cuStatsPtr,
                                    coeffPtr,
                                    &cuQuantizedCoeffsBits,
                                    (EB_BOOL)pictureControlSetPtr->useDeltaQp,
                                    &deltaQpNotCoded);

                            tbPtr->quantizedCoeffsBits += cuQuantizedCoeffsBits;
                        }
                        break;

                    case INTER_MODE:
                    {

                        // Code Partition Size
                        EncodePartitionSize(
                            cabacEncodeCtxPtr,
                            cuPtr,
                            pictureControlSetPtr->lcuMaxDepth);

                        puPtr = cuPtr->predictionUnitArray;
                            // mv merge Flag
                            EncodeMergeFlag(
                                cabacEncodeCtxPtr,
                                puPtr);

                            if (puPtr->mergeFlag) {
                                // mv merge index
                                EncodeMergeIndex(
                                    cabacEncodeCtxPtr,
                                    cuPtr->predictionUnitArray);
                            }
                            else {
                                // Inter Prediction Direction
                                if (pictureControlSetPtr->sliceType == EB_B_PICTURE){
                                    EncodePredictionDirection(
                                        cabacEncodeCtxPtr,
                                        puPtr,
                                        cuPtr);
                                }

                                refList = puPtr->interPredDirectionIndex == UNI_PRED_LIST_1 ? REF_LIST_1 : REF_LIST_0;
                                maxRefList = (EB_U32)refList + (puPtr->interPredDirectionIndex == BI_PRED ? 2 : 1);
                                {
                                    EB_U32	refIndex = refList;
                                    for (; (EB_U32)refIndex < maxRefList; ++refIndex){
                                        // Reference Index
                                        refList = (EB_REFLIST)refIndex;
                                        // Reference Index
                                        EncodeReferencePictureIndex(
                                            cabacEncodeCtxPtr,
                                            refList,
                                            pictureControlSetPtr);

                                        // Motion Vector Difference
                                        EncodeMvd(
                                            cabacEncodeCtxPtr,
                                            puPtr,
                                            refList);

                                        // Motion Vector Prediction Index
                                        EncodeMvpIndex(
                                            cabacEncodeCtxPtr,
                                            puPtr,
                                            refList);
                                    }
                                }
                            }
                    }

                        // Encode Transform Unit Split & CBFs
                        EncodeTuSplitCoeff(
                            cabacEncodeCtxPtr,
                            cuPtr,
                            cuStatsPtr,
                            coeffPtr,
                            &cuQuantizedCoeffsBits,
                            (EB_BOOL)pictureControlSetPtr->useDeltaQp,
                            &deltaQpNotCoded);

                        tbPtr->quantizedCoeffsBits += cuQuantizedCoeffsBits;

                        break;
                    default:
                        CHECK_REPORT_ERROR_NC(
                            encodeContextPtr->appCallbackPtr,
                            EB_ENC_EC_ERROR3);
                        break;
                    }
                }

                // Update the Leaf Depth Neighbor Array
                NeighborArrayUnitModeWrite(
                    leafDepthNeighborArray,
                    &cuDepth,
                    cuOriginX,
                    cuOriginY,
                    cuSize,
                    cuSize,
                    NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);

                // Update the Mode Type Neighbor Array
                {
                    EB_U8 predictionModeFlag = (EB_U8)cuPtr->predictionModeFlag;
                    NeighborArrayUnitModeWrite(
                        modeTypeNeighborArray,
                        &predictionModeFlag,
                        cuOriginX,
                        cuOriginY,
                        cuSize,
                        cuSize,
                        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);
                }

                // Update the Skip Flag Neighbor Array
                {
                    EB_U8 skipFlag = (EB_U8)cuPtr->skipFlag;
                    NeighborArrayUnitModeWrite(
                        skipFlagNeighborArray,
                        (EB_U8*)&skipFlag,
                        cuOriginX,
                        cuOriginY,
                        cuSize,
                        cuSize,
                        NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK);
                }

            }
            if (cuPtr->splitFlag == EB_FALSE)
                cuIndex += DepthOffset[cuDepth];
            else
                ++cuIndex;
        }
        else
            ++cuIndex;

    } while (cuIndex < CU_MAX_COUNT);


    return return_error;
}

EB_ERRORTYPE TuEstimateCoeffBitsEncDec(
	EB_U32                     tuOriginIndex,
	EB_U32                     tuChromaOriginIndex,
	EntropyCoder_t            *entropyCoderPtr,
	EbPictureBufferDesc_t     *coeffBufferTB,
    EB_U32                     countNonZeroCoeffs[3],
	EB_U64                    *yTuCoeffBits,
	EB_U64                    *cbTuCoeffBits,
	EB_U64                    *crTuCoeffBits,
	EB_U32                     transformSize,
	EB_U32                     transformChromaSize,
	EB_MODETYPE                type,
	CabacCost_t               *CabacCost)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	CabacEncodeContext_t *cabacEncodeCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;

	EB_S16 *coeffBuffer;
	// Estimate Y Coeff bits
	{

		coeffBuffer = (EB_S16*)&coeffBufferTB->bufferY[tuOriginIndex * sizeof(EB_S16)];

		if (countNonZeroCoeffs[0]) {

			EstimateQuantizedCoefficients[1][!!(ASM_TYPES & PREAVX2_MASK)](
				CabacCost,
				cabacEncodeCtxPtr,
				transformSize,
				type,
				EB_INTRA_MODE_INVALID,
				EB_INTRA_CHROMA_INVALID,
				coeffBuffer,
				MAX_LCU_SIZE,
				COMPONENT_LUMA,
				countNonZeroCoeffs[0],
				yTuCoeffBits);

		}

		*yTuCoeffBits = (*yTuCoeffBits >> 15);
	}

	// Estimate U Coeff bits
	{
		coeffBuffer = (EB_S16*)&coeffBufferTB->bufferCb[tuChromaOriginIndex * sizeof(EB_S16)];

		if (countNonZeroCoeffs[1]) {

			EstimateQuantizedCoefficients[1][!!(ASM_TYPES & PREAVX2_MASK)](
				CabacCost,
				cabacEncodeCtxPtr,
				transformChromaSize,
				type,
				EB_INTRA_MODE_INVALID,
				EB_INTRA_CHROMA_INVALID,
				coeffBuffer,
				MAX_LCU_SIZE >> 1,
				COMPONENT_CHROMA_CB,
				countNonZeroCoeffs[1],
				cbTuCoeffBits);
		}

		*cbTuCoeffBits = (*cbTuCoeffBits >> 15);
	}

	// Estimate V Coeff bits
	{
		coeffBuffer = (EB_S16*)&coeffBufferTB->bufferCr[tuChromaOriginIndex * sizeof(EB_S16)];

		if (countNonZeroCoeffs[2]) {

			EstimateQuantizedCoefficients[1][!!(ASM_TYPES & PREAVX2_MASK)](
				CabacCost,
				cabacEncodeCtxPtr,
				transformChromaSize,
				type,
				EB_INTRA_MODE_INVALID,
				EB_INTRA_CHROMA_INVALID,
				coeffBuffer,
				MAX_LCU_SIZE>>1,
				COMPONENT_CHROMA_CR,
				countNonZeroCoeffs[2],
				crTuCoeffBits);
		}

		*crTuCoeffBits = (*crTuCoeffBits >> 15);
	}

	return return_error;
}


EB_ERRORTYPE TuEstimateCoeffBitsLuma(
	EB_U32                     tuOriginIndex,
	EntropyCoder_t            *entropyCoderPtr,
	EbPictureBufferDesc_t     *coeffPtr,
	EB_U32                     yCountNonZeroCoeffs,
	EB_U64                    *yTuCoeffBits,
	EB_U32                     transformSize,
	EB_MODETYPE                type,
	EB_U32                     intraLumaMode,
	EB_U32                     partialFrequencyN2Flag,
    EB_BOOL                    coeffCabacUpdate,
    CoeffCtxtMdl_t            *updatedCoeffCtxModel,
	CabacCost_t               *CabacCost)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	CabacEncodeContext_t *cabacEncodeCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;

	EB_S16 *coeffBuffer;

	// Estimate Y Coeff bits

	coeffBuffer = (EB_S16*)&coeffPtr->bufferY[tuOriginIndex * sizeof(EB_S16)];

	if (yCountNonZeroCoeffs) {

        if(coeffCabacUpdate)
            EstimateQuantizedCoefficientsUpdate[!!(ASM_TYPES & PREAVX2_MASK)](
                updatedCoeffCtxModel,
                CabacCost,
                cabacEncodeCtxPtr,
                (transformSize >> partialFrequencyN2Flag),
                type,
                intraLumaMode,
                EB_INTRA_CHROMA_DM,
                coeffBuffer,
                coeffPtr->strideY,
                COMPONENT_LUMA,
                yCountNonZeroCoeffs,
                yTuCoeffBits);
        else
		    EstimateQuantizedCoefficients[1][!!(ASM_TYPES & PREAVX2_MASK)](
			    CabacCost,
			    cabacEncodeCtxPtr,
			    (transformSize >> partialFrequencyN2Flag),
			    type,
			    intraLumaMode,
                EB_INTRA_CHROMA_DM,
			    coeffBuffer,
			    coeffPtr->strideY,
			    COMPONENT_LUMA,
			    yCountNonZeroCoeffs,
			    yTuCoeffBits);
	}

	*yTuCoeffBits = (*yTuCoeffBits >> 15);


	return return_error;
}



EB_ERRORTYPE TuEstimateCoeffBits_R(
	EB_U32                     tuOriginIndex,
	EB_U32                     tuChromaOriginIndex,
	EB_U32                     componentMask,
	EntropyCoder_t            *entropyCoderPtr,
	EbPictureBufferDesc_t     *coeffPtr,
	EB_U32                     yCountNonZeroCoeffs,
	EB_U32                     cbCountNonZeroCoeffs,
	EB_U32                     crCountNonZeroCoeffs,
	EB_U64                    *yTuCoeffBits,
	EB_U64                    *cbTuCoeffBits,
	EB_U64                    *crTuCoeffBits,
	EB_U32                     transformSize,
	EB_U32                     transformChromaSize,
	EB_MODETYPE                type,
	EB_U32					   intraLumaMode,
	EB_U32                     intraChromaMode,
	EB_U32                     partialFrequencyN2Flag,
    EB_BOOL                    coeffCabacUpdate,
    CoeffCtxtMdl_t            *updatedCoeffCtxModel,

	CabacCost_t               *CabacCost)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	CabacEncodeContext_t *cabacEncodeCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;

	EB_S16 *coeffBuffer;

	//CHKN EB_U8 encoderModeIndex = (pictureControlSetPtr->ParentPcsPtr->encoderLevel == ENCODER_LEVEL_7) ? 0 : 1;
    	EB_U8 encoderModeIndex = 1;

	// Estimate Y Coeff bits
	if (componentMask & PICTURE_BUFFER_DESC_Y_FLAG) {

		coeffBuffer = (EB_S16*)&coeffPtr->bufferY[tuOriginIndex * sizeof(EB_S16)];

		if (yCountNonZeroCoeffs) {

			if (coeffCabacUpdate)
				EstimateQuantizedCoefficientsUpdate[!!(ASM_TYPES & PREAVX2_MASK)](
					updatedCoeffCtxModel,
					CabacCost,
					cabacEncodeCtxPtr,
					(transformSize >> partialFrequencyN2Flag),
					type,
					intraLumaMode,
					intraChromaMode,
					coeffBuffer,
					coeffPtr->strideY,
					COMPONENT_LUMA,
					yCountNonZeroCoeffs,
					yTuCoeffBits);

            else

			    EstimateQuantizedCoefficients[encoderModeIndex][!!(ASM_TYPES & PREAVX2_MASK)](
				    CabacCost,
				    cabacEncodeCtxPtr,
				    (transformSize >> partialFrequencyN2Flag),
				    type,
				    intraLumaMode,
				    intraChromaMode,
				    coeffBuffer,
				    coeffPtr->strideY,
				    COMPONENT_LUMA,
				    yCountNonZeroCoeffs,
				    yTuCoeffBits);

		}

		*yTuCoeffBits = (*yTuCoeffBits >> 15);
	}

	// Estimate U Coeff bits
	if (componentMask & PICTURE_BUFFER_DESC_Cb_FLAG) {
		coeffBuffer = (EB_S16*)&coeffPtr->bufferCb[tuChromaOriginIndex * sizeof(EB_S16)];

		if (cbCountNonZeroCoeffs) {

			if (coeffCabacUpdate)
				EstimateQuantizedCoefficientsUpdate[!!(ASM_TYPES & PREAVX2_MASK)](
					updatedCoeffCtxModel,
					CabacCost,
					cabacEncodeCtxPtr,
					(transformChromaSize >> partialFrequencyN2Flag),
					type,
					intraLumaMode,
					intraChromaMode,
					coeffBuffer,
					coeffPtr->strideCb,
					COMPONENT_CHROMA_CB,
					cbCountNonZeroCoeffs,
					cbTuCoeffBits);
            else

			    EstimateQuantizedCoefficients[encoderModeIndex][!!(ASM_TYPES & PREAVX2_MASK)](
				    CabacCost,
				    cabacEncodeCtxPtr,
				    (transformChromaSize >> partialFrequencyN2Flag),
				    type,
				    intraLumaMode,
				    intraChromaMode,
				    coeffBuffer,
				    coeffPtr->strideCb,
				    COMPONENT_CHROMA_CB,
				    cbCountNonZeroCoeffs,
					cbTuCoeffBits);

		}

		*cbTuCoeffBits = (*cbTuCoeffBits >> 15);
	}

	// Estimate V Coeff bits
	if (componentMask & PICTURE_BUFFER_DESC_Cr_FLAG) {
		coeffBuffer = (EB_S16*)&coeffPtr->bufferCr[tuChromaOriginIndex * sizeof(EB_S16)];

		if (crCountNonZeroCoeffs) {

            if (coeffCabacUpdate) 
                EstimateQuantizedCoefficientsUpdate[!!(ASM_TYPES & PREAVX2_MASK)](
                    updatedCoeffCtxModel,
                    CabacCost,
                    cabacEncodeCtxPtr,
                    (transformChromaSize >> partialFrequencyN2Flag),
                    type,
                    intraLumaMode,
                    intraChromaMode,
                    coeffBuffer,
                    coeffPtr->strideCr,
                    COMPONENT_CHROMA_CR,
                    crCountNonZeroCoeffs,
                    crTuCoeffBits);

            else

			    EstimateQuantizedCoefficients[encoderModeIndex][!!(ASM_TYPES & PREAVX2_MASK)](
				    CabacCost,
				    cabacEncodeCtxPtr,
                    (transformChromaSize >> partialFrequencyN2Flag),
				    type,
				    intraLumaMode,
				    intraChromaMode,
				    coeffBuffer,
				    coeffPtr->strideCr,
				    COMPONENT_CHROMA_CR,
				    crCountNonZeroCoeffs,
				    crTuCoeffBits);

		}

		*crTuCoeffBits = (*crTuCoeffBits >> 15);
	}

	return return_error;
}

EB_ERRORTYPE EncodeSliceHeader(
	EB_U32               firstLcuAddr,
	EB_U32               pictureQp,
	PictureControlSet_t *pcsPtr,
	OutputBitstreamUnit_t  *bitstreamPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	CodeSliceHeader(
		firstLcuAddr,
		pictureQp,
		bitstreamPtr,
		pcsPtr);

	return return_error;
}

EB_ERRORTYPE EncodeSliceFinish(
	EntropyCoder_t        *entropyCoderPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	CabacEncodeContext_t *cabacEncodeCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;

	BacEncContextFinish(&(cabacEncodeCtxPtr->bacEncContext));
	//  End of bitstream & byte align

	//pcBitstreamOut->write( 1, 1 );
	OutputBitstreamWrite(
		&(cabacEncodeCtxPtr->bacEncContext.m_pcTComBitIf),
		1,
		1);

	//pcBitstreamOut->writeAlignZero();
	OutputBitstreamWriteAlignZero(
		&(cabacEncodeCtxPtr->bacEncContext.m_pcTComBitIf));

	return return_error;
}

EB_ERRORTYPE EncodeTerminateLcu(
	EntropyCoder_t        *entropyCoderPtr,
	EB_U32                 lastLcuInSlice)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	CabacEncodeContext_t *cabacEncodeCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;


	BacEncContextTerminate(
		&(cabacEncodeCtxPtr->bacEncContext),
		lastLcuInSlice);

	return return_error;
}

EB_ERRORTYPE ResetBitstream(
	EB_PTR bitstreamPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr;

	OutputBitstreamReset(
		outputBitstreamPtr);

	return return_error;
}

EB_ERRORTYPE ResetEntropyCoder(
	EncodeContext_t            *encodeContextPtr,
	EntropyCoder_t             *entropyCoderPtr,
	EB_U32                      qp,
	EB_PICTURE                    sliceType)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	CabacEncodeContext_t      *cabacEncCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;
	ContextModelEncContext_t  *cabacCtxModelArray = (ContextModelEncContext_t*)encodeContextPtr->cabacContextModelArray;

	// Increment the context model array pointer to point to the right address based on the QP and slice type 
	cabacCtxModelArray += sliceType * TOTAL_NUMBER_OF_QP_VALUES + qp;

	// Reset context models to initial values by copying from cabacContextModelArray
	EB_MEMCPY(&(cabacEncCtxPtr->contextModelEncContext.splitFlagContextModel[0]), &(cabacCtxModelArray->splitFlagContextModel[0]), sizeof(EB_ContextModel)* TOTAL_NUMBER_OF_CABAC_CONTEXT_MODELS);

	// Reset Binary Arithmetic Coder (BAC) to initial values 
	ResetBacEnc(
		&(cabacEncCtxPtr->bacEncContext));

	return return_error;
}

EB_ERRORTYPE FlushBitstream(
	EB_PTR outputBitstreamPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	OutputBitstreamFlushBuffer((OutputBitstreamUnit_t *)outputBitstreamPtr);

	return return_error;
}

/**************************************************
* EncodeAUD
**************************************************/
EB_ERRORTYPE EncodeAUD(
	Bitstream_t *bitstreamPtr,
	EB_PICTURE     sliceType,
	EB_U32       temporalId)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32 picType;
	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

	picType = (sliceType == EB_I_PICTURE) ? 0 :
		(sliceType == EB_P_PICTURE) ? 1 :
		2;

	CodeNALUnitHeader(
		outputBitstreamPtr,
		NAL_UNIT_ACCESS_UNIT_DELIMITER,
		temporalId);

	// picture type
	WriteCodeCavlc(
		outputBitstreamPtr,
		picType,
		3);

	// Byte Align the Bitstream: rbsp_trailing_bits
	OutputBitstreamWrite(
		outputBitstreamPtr,
		1,
		1);

	OutputBitstreamWriteAlignZero(
		outputBitstreamPtr);

	return return_error;
}

/**************************************************
* EncodeVPS
**************************************************/
EB_ERRORTYPE EncodeVPS(
	Bitstream_t *bitstreamPtr,
	SequenceControlSet_t *scsPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

	CodeVPS(
		outputBitstreamPtr,
		scsPtr);

	// Byte Align the Bitstream

	//bs_SPS_PPS_SEI.write( 1, 1 );
	OutputBitstreamWrite(
		outputBitstreamPtr,
		1,
		1);

	//bs_SPS_PPS_SEI.writeAlignZero();
	OutputBitstreamWriteAlignZero(
		outputBitstreamPtr);

	return return_error;
}

/**************************************************
* EncodeSPS
**************************************************/
EB_ERRORTYPE EncodeSPS(
	Bitstream_t *bitstreamPtr,
	SequenceControlSet_t *scsPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

	CodeSPS(
		outputBitstreamPtr,
		scsPtr);

	// Byte Align the Bitstream

	//bs_SPS_PPS_SEI.write( 1, 1 );
	OutputBitstreamWrite(
		outputBitstreamPtr,
		1,
		1);

	//bs_SPS_PPS_SEI.writeAlignZero();
	OutputBitstreamWriteAlignZero(
		outputBitstreamPtr);

	return return_error;
}
/**************************************************
* EncodePPS
**************************************************/
EB_ERRORTYPE EncodePPS(
	Bitstream_t *bitstreamPtr,
    SequenceControlSet_t *scsPtr,
	EbPPSConfig_t       *ppsConfig)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

	CodePPS(
		outputBitstreamPtr,
        scsPtr,
		ppsConfig);

	// Byte Align the Bitstream

	//bs_SPS_PPS_SEI.write( 1, 1 );
	OutputBitstreamWrite(
		outputBitstreamPtr,
		1,
		1);

	//bs_SPS_PPS_SEI.writeAlignZero();
	OutputBitstreamWriteAlignZero(
		outputBitstreamPtr);

	return return_error;
}

EB_ERRORTYPE CodeBufferingPeriodSEI(
	OutputBitstreamUnit_t   *bitstreamPtr,
	AppBufferingPeriodSei_t    *bufferingPeriodPtr,
	AppVideoUsabilityInfo_t    *vuiPtr,
	EncodeContext_t         *encodeContextPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32       nalVclIndex;
	EB_U32       cpbIndex;

	// bp_seq_parameter_set_id
	WriteUvlc(
		bitstreamPtr,
		bufferingPeriodPtr->bpSeqParameterSetId);

	if (!vuiPtr->hrdParametersPtr->subPicCpbParamsPresentFlag){
		// rap_cpb_params_present_flag
		WriteFlagCavlc(
			bitstreamPtr,
			bufferingPeriodPtr->rapCpbParamsPresentFlag);
	}

	if (bufferingPeriodPtr->rapCpbParamsPresentFlag){
		// cpb_delay_offset
		WriteCodeCavlc(
			bitstreamPtr,
			bufferingPeriodPtr->cpbDelayOffset,
			vuiPtr->hrdParametersPtr->auCpbRemovalDelayLengthMinus1 + 1);
		// dpb_delay_offset
		WriteCodeCavlc(
			bitstreamPtr,
			bufferingPeriodPtr->dpbDelayOffset,
			vuiPtr->hrdParametersPtr->dpbOutputDelayDuLengthMinus1 + 1);
	}

	// concatenation_flag
	WriteFlagCavlc(
		bitstreamPtr,
		bufferingPeriodPtr->concatenationFlag);

	// au_cpb_removal_delay_delta_minus1
	WriteCodeCavlc(
		bitstreamPtr,
		bufferingPeriodPtr->auCpbRemovalDelayDeltaMinus1,
		vuiPtr->hrdParametersPtr->auCpbRemovalDelayLengthMinus1 + 1);

	for (nalVclIndex = 0; nalVclIndex < 2; ++nalVclIndex){
		if ((nalVclIndex == 0 && vuiPtr->hrdParametersPtr->nalHrdParametersPresentFlag) ||
			(nalVclIndex == 1 && vuiPtr->hrdParametersPtr->vclHrdParametersPresentFlag)){

			CHECK_REPORT_ERROR(
				(vuiPtr->hrdParametersPtr->cpbCountMinus1[0] < MAX_CPB_COUNT),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_EC_ERROR14);

			for (cpbIndex = 0; cpbIndex < vuiPtr->hrdParametersPtr->cpbCountMinus1[0] + 1; ++cpbIndex){
				// initial_cpb_removal_delay
				WriteCodeCavlc(
					bitstreamPtr,
					bufferingPeriodPtr->initialCpbRemovalDelay[nalVclIndex][cpbIndex],
					vuiPtr->hrdParametersPtr->initialCpbRemovalDelayLengthMinus1 + 1);

				// initial_cpb_removal_delay_offset
				WriteCodeCavlc(
					bitstreamPtr,
					bufferingPeriodPtr->initialCpbRemovalDelayOffset[nalVclIndex][cpbIndex],
					vuiPtr->hrdParametersPtr->initialCpbRemovalDelayLengthMinus1 + 1);

				if (vuiPtr->hrdParametersPtr->subPicCpbParamsPresentFlag || bufferingPeriodPtr->rapCpbParamsPresentFlag){
					// initial_alt_cpb_removal_delay
					WriteCodeCavlc(
						bitstreamPtr,
						bufferingPeriodPtr->initialAltCpbRemovalDelay[nalVclIndex][cpbIndex],
						vuiPtr->hrdParametersPtr->initialCpbRemovalDelayLengthMinus1 + 1);

					// initial_alt_cpb_removal_delay_offset
					WriteCodeCavlc(
						bitstreamPtr,
						bufferingPeriodPtr->initialAltCpbRemovalDelayOffset[nalVclIndex][cpbIndex],
						vuiPtr->hrdParametersPtr->initialCpbRemovalDelayLengthMinus1 + 1);
				}
			}
		}
	}

	if (bitstreamPtr->writtenBitsCount % 8 != 0) {
		// bit_equal_to_one
		WriteFlagCavlc(
			bitstreamPtr,
			1);

		while (bitstreamPtr->writtenBitsCount % 8 != 0) {
			// bit_equal_to_zero
			WriteFlagCavlc(
				bitstreamPtr,
				0);
		}
	}

	return return_error;
}

EB_ERRORTYPE CodeActiveParameterSetSEI(
    OutputBitstreamUnit_t   *bitstreamPtr,
    AppActiveparameterSetSei_t    *activeParameterSet)
{
	EB_U32 i;
    EB_ERRORTYPE return_error = EB_ErrorNone;
    //active_vps_id
    WriteCodeCavlc(bitstreamPtr, activeParameterSet->activeVideoParameterSetid, 4);
    //self_contained_flag
    WriteFlagCavlc(bitstreamPtr, activeParameterSet->selfContainedCvsFlag);
    //no_param_set_update_flag
    WriteFlagCavlc(bitstreamPtr, activeParameterSet->noParameterSetUpdateFlag);
    //num_sps_ids_minus1
    WriteUvlc(bitstreamPtr, activeParameterSet->numSpsIdsMinus1);
    //active_seq_param_set_id
    for (i = 0; i <= activeParameterSet->numSpsIdsMinus1; i++)
    {
        WriteUvlc(bitstreamPtr, activeParameterSet->activeSeqParameterSetId);
    }
    if (bitstreamPtr->writtenBitsCount % 8 != 0) {
        // bit_equal_to_one
        WriteFlagCavlc(
            bitstreamPtr,
            1);

        while (bitstreamPtr->writtenBitsCount % 8 != 0) {
            // bit_equal_to_zero
            WriteFlagCavlc(
                bitstreamPtr,
                0);
        }
    }

    return return_error;
}

EB_ERRORTYPE CodePictureTimingSEI(
	OutputBitstreamUnit_t   *bitstreamPtr,
	AppPictureTimingSei_t   *picTimingSeiPtr,
	AppVideoUsabilityInfo_t *vuiPtr,
	EncodeContext_t         *encodeContextPtr,
    EB_U8                    pictStruct)

{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32    decodingUnitIndex;

	if (vuiPtr->frameFieldInfoPresentFlag){
		// pic_struct
		WriteCodeCavlc(
			bitstreamPtr,

            pictStruct,

			4);

		// source_scan_type
		WriteCodeCavlc(
			bitstreamPtr,
			picTimingSeiPtr->sourceScanType,
			2);

		// duplicate_flag
		WriteFlagCavlc(
			bitstreamPtr,
			picTimingSeiPtr->duplicateFlag);
	}

	if (vuiPtr->hrdParametersPtr->cpbDpbDelaysPresentFlag){
		// au_cpb_removal_delay_minus1
		WriteCodeCavlc(
			bitstreamPtr,
			picTimingSeiPtr->auCpbRemovalDelayMinus1,
			vuiPtr->hrdParametersPtr->auCpbRemovalDelayLengthMinus1 + 1);

		// pic_dpb_output_delay
		WriteCodeCavlc(
			bitstreamPtr,
			picTimingSeiPtr->picDpbOutputDelay,
			vuiPtr->hrdParametersPtr->dpbOutputDelayLengthMinus1 + 1);

		if (vuiPtr->hrdParametersPtr->subPicCpbParamsPresentFlag){
			// pic_dpb_output_du_delay
			WriteCodeCavlc(
				bitstreamPtr,
				picTimingSeiPtr->picDpbOutputDuDelay,
				vuiPtr->hrdParametersPtr->dpbOutputDelayDuLengthMinus1 + 1);
		}

		if (vuiPtr->hrdParametersPtr->subPicCpbParamsPresentFlag && vuiPtr->hrdParametersPtr->subPicCpbParamsPicTimingSeiFlag){
			// num_decoding_units_minus1
			WriteUvlc(
				bitstreamPtr,
				picTimingSeiPtr->numDecodingUnitsMinus1);

			// du_common_cpb_removal_delay_flag
			WriteFlagCavlc(
				bitstreamPtr,
				picTimingSeiPtr->duCommonCpbRemovalDelayFlag);

			if (picTimingSeiPtr->duCommonCpbRemovalDelayFlag){
				// du_common_cpb_removal_delay_minus1
				WriteCodeCavlc(
					bitstreamPtr,
					picTimingSeiPtr->duCommonCpbRemovalDelayMinus1,
					vuiPtr->hrdParametersPtr->duCpbRemovalDelayLengthMinus1 + 1);
			}

			CHECK_REPORT_ERROR(
				(picTimingSeiPtr->numDecodingUnitsMinus1 < MAX_CPB_COUNT),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_EC_ERROR15);

			for (decodingUnitIndex = 0; decodingUnitIndex <= picTimingSeiPtr->numDecodingUnitsMinus1; ++decodingUnitIndex){
				// num_nalus_in_du_minus1
				WriteUvlc(
					bitstreamPtr,
					picTimingSeiPtr->numNalusInDuMinus1);

				if (!picTimingSeiPtr->duCommonCpbRemovalDelayFlag && decodingUnitIndex < picTimingSeiPtr->numDecodingUnitsMinus1){
					// du_cpb_removal_delay_minus1
					WriteCodeCavlc(
						bitstreamPtr,
						picTimingSeiPtr->duCpbRemovalDelayMinus1[decodingUnitIndex],
						vuiPtr->hrdParametersPtr->duCpbRemovalDelayLengthMinus1 + 1);
				}
			}
		}
	}

	if (bitstreamPtr->writtenBitsCount % 8 != 0) {
		// bit_equal_to_one
		WriteFlagCavlc(
			bitstreamPtr,
			1);

		while (bitstreamPtr->writtenBitsCount % 8 != 0) {
			// bit_equal_to_zero
			WriteFlagCavlc(
				bitstreamPtr,
				0);
		}
	}

	return return_error;
}


//////////////////////////////////////
//// ADD NEW SEI messages BELOW
//// Use the example below
/////////////////////////////////////

EB_ERRORTYPE EncodePictureTimingSEI(
	Bitstream_t             *bitstreamPtr,
	AppPictureTimingSei_t   *picTimingSeiPtr,
	AppVideoUsabilityInfo_t *vuiPtr,
	EncodeContext_t         *encodeContextPtr,
    EB_U8                    pictStruct,
    EB_U8                    temporalId)

{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	unsigned payloadType = PICTURE_TIMING;

	// Note: payloadSize is sent as 0 as we are currently not populating picture timing message, this will change soon
	unsigned payloadSize;

	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

	CodeNALUnitHeader(
		outputBitstreamPtr,
		NAL_UNIT_PREFIX_SEI,
		temporalId);

	payloadSize = GetPictureTimingSEILength(
		picTimingSeiPtr,
		vuiPtr);

	for (; payloadType >= 0xff; payloadType -= 0xff) {
		OutputBitstreamWrite(
			outputBitstreamPtr,
			0xff,
			8);
	}
	OutputBitstreamWrite(
		outputBitstreamPtr,
		payloadType,
		8);

	for (; payloadSize >= 0xff; payloadSize -= 0xff) {
		OutputBitstreamWrite(
			outputBitstreamPtr,
			0xff,
			8);
	}
	OutputBitstreamWrite(
		outputBitstreamPtr,
		payloadSize,
		8);

	// Picture Timing SEI data

	CodePictureTimingSEI(
		outputBitstreamPtr,
		picTimingSeiPtr,
		vuiPtr,
		encodeContextPtr,
        pictStruct);


	// Byte Align the Bitstream    
	OutputBitstreamWrite(
		outputBitstreamPtr,
		1,
		1);

	OutputBitstreamWriteAlignZero(
		outputBitstreamPtr);

	return return_error;
}

EB_ERRORTYPE EncodeBufferingPeriodSEI(
	Bitstream_t             *bitstreamPtr,
	AppBufferingPeriodSei_t    *bufferingPeriodPtr,
	AppVideoUsabilityInfo_t    *vuiPtr,
	EncodeContext_t         *encodeContextPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	unsigned payloadType = BUFFERING_PERIOD;

	// Note: payloadSize is fixed temporarily, this may change in future based on what is sent in CodeBufferingPeriod
	unsigned payloadSize;

	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

	CodeNALUnitHeader(
		outputBitstreamPtr,
		NAL_UNIT_PREFIX_SEI,
		0);

	payloadSize = GetBufPeriodSEILength(
		bufferingPeriodPtr,
		vuiPtr);

	for (; payloadType >= 0xff; payloadType -= 0xff) {
		OutputBitstreamWrite(
			outputBitstreamPtr,
			0xff,
			8);
	}
	OutputBitstreamWrite(
		outputBitstreamPtr,
		payloadType,
		8);

	for (; payloadSize >= 0xff; payloadSize -= 0xff) {
		OutputBitstreamWrite(
			outputBitstreamPtr,
			0xff,
			8);
	}
	OutputBitstreamWrite(
		outputBitstreamPtr,
		payloadSize,
		8);

	// Buffering Period SEI data
	CodeBufferingPeriodSEI(
		outputBitstreamPtr,
		bufferingPeriodPtr,
		vuiPtr,
		encodeContextPtr);

	// Byte Align the Bitstream    
	OutputBitstreamWrite(
		outputBitstreamPtr,
		1,
		1);

	OutputBitstreamWriteAlignZero(
		outputBitstreamPtr);

	return return_error;
}

EB_ERRORTYPE EncodeActiveParameterSetsSEI(
    Bitstream_t             *bitstreamPtr,
    AppActiveparameterSetSei_t    *activeParameterSet)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    unsigned payloadType = ACTIVE_PARAMETER_SETS;

    // Note: payloadSize is fixed temporarily, this may change in future based on what is sent in CodeActiveParameterSet
    unsigned payloadSize;

    OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

    CodeNALUnitHeader(
        outputBitstreamPtr,
        NAL_UNIT_PREFIX_SEI,
        0);

    payloadSize = GetActiveParameterSetSEILength(
        activeParameterSet);

    for (; payloadType >= 0xff; payloadType -= 0xff) {
        OutputBitstreamWrite(
            outputBitstreamPtr,
            0xff,
            8);
    }
    OutputBitstreamWrite(
        outputBitstreamPtr,
        payloadType,
        8);

    for (; payloadSize >= 0xff; payloadSize -= 0xff) {
        OutputBitstreamWrite(
            outputBitstreamPtr,
            0xff,
            8);
    }
    OutputBitstreamWrite(
        outputBitstreamPtr,
        payloadSize,
        8);

    // Active Parameter Set SEI data
    CodeActiveParameterSetSEI(
        outputBitstreamPtr,
        activeParameterSet);

    // Byte Align the Bitstream
    OutputBitstreamWrite(
        outputBitstreamPtr,
        1,
        1);

    OutputBitstreamWriteAlignZero(
        outputBitstreamPtr);

    return return_error;
}

EB_ERRORTYPE EncodeFillerData(
    Bitstream_t             *bitstreamPtr,
    EB_U8                    temporalId)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

    CodeNALUnitHeader(
        outputBitstreamPtr,
        NAL_UNIT_FILLER_DATA,
        temporalId);
    return return_error;
}

EB_ERRORTYPE EncodeRegUserDataSEI(
	Bitstream_t             *bitstreamPtr,
	RegistedUserData_t      *regUserDataSeiPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32       index;
	unsigned     payloadType = REG_USER_DATA;

	unsigned     payloadSize = regUserDataSeiPtr->userDataSize;

	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

	CodeNALUnitHeader(
		outputBitstreamPtr,
		NAL_UNIT_PREFIX_SEI,
		0);

	for (; payloadType >= 0xff; payloadType -= 0xff) {
		OutputBitstreamWrite(
			outputBitstreamPtr,
			0xff,
			8);
	}
	OutputBitstreamWrite(
		outputBitstreamPtr,
		payloadType,
		8);

	for (; payloadSize >= 0xff; payloadSize -= 0xff) {
		OutputBitstreamWrite(
			outputBitstreamPtr,
			0xff,
			8);
	}
	OutputBitstreamWrite(
		outputBitstreamPtr,
		payloadSize,
		8);

	for (index = 0; index < payloadSize; ++index){
		// user_data
		WriteCodeCavlc(
			outputBitstreamPtr,
			regUserDataSeiPtr->userData[index],
			8);
	}

	if (outputBitstreamPtr->writtenBitsCount % 8 != 0) {
		// bit_equal_to_one
		WriteFlagCavlc(
			outputBitstreamPtr,
			1);

		while (outputBitstreamPtr->writtenBitsCount % 8 != 0) {
			// bit_equal_to_zero
			WriteFlagCavlc(
				outputBitstreamPtr,
				0);
		}
	}

	// Byte Align the Bitstream    
	OutputBitstreamWrite(
		outputBitstreamPtr,
		1,
		1);

	OutputBitstreamWriteAlignZero(
		outputBitstreamPtr);

	return return_error;
}

EB_ERRORTYPE EncodeUnregUserDataSEI(
	Bitstream_t             *bitstreamPtr,
	UnregistedUserData_t    *unregUserDataSeiPtr,
	EncodeContext_t         *encodeContextPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32       index;
	unsigned     payloadType = UNREG_USER_DATA;

	unsigned     payloadSize = unregUserDataSeiPtr->userDataSize + 16 ;

	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

	CHECK_REPORT_ERROR(
		(payloadSize >= 16),
		encodeContextPtr->appCallbackPtr,
		EB_ENC_EC_ERROR16);


	CodeNALUnitHeader(
		outputBitstreamPtr,
		NAL_UNIT_PREFIX_SEI,
		0);

	for (; payloadType >= 0xff; payloadType -= 0xff) {
		OutputBitstreamWrite(
			outputBitstreamPtr,
			0xff,
			8);
	}
	OutputBitstreamWrite(
		outputBitstreamPtr,
		payloadType,
		8);

	for (; payloadSize >= 0xff; payloadSize -= 0xff) {
		OutputBitstreamWrite(
			outputBitstreamPtr,
			0xff,
			8);
	}
	OutputBitstreamWrite(
		outputBitstreamPtr,
		payloadSize,
		8);

	for (index = 0; index < 16; ++index){
		// sei.uuid_iso_iec_11578[i]
		WriteCodeCavlc(
			outputBitstreamPtr,
			unregUserDataSeiPtr->uuidIsoIec_11578[index],
			8);
	}

	for (index = 0; index < payloadSize - 16; ++index){
		// user_data
		WriteCodeCavlc(
			outputBitstreamPtr,
			unregUserDataSeiPtr->userData[index],
			8);
	}

	if (outputBitstreamPtr->writtenBitsCount % 8 != 0) {
		// bit_equal_to_one
		WriteFlagCavlc(
			outputBitstreamPtr,
			1);

		while (outputBitstreamPtr->writtenBitsCount % 8 != 0) {
			// bit_equal_to_zero
			WriteFlagCavlc(
				outputBitstreamPtr,
				0);
		}
	}

	// Byte Align the Bitstream    
	OutputBitstreamWrite(
		outputBitstreamPtr,
		1,
		1);

	OutputBitstreamWriteAlignZero(
		outputBitstreamPtr);

	return return_error;
}

EB_ERRORTYPE EncodeRecoveryPointSEI(
	Bitstream_t             *bitstreamPtr,
	AppRecoveryPoint_t         *recoveryPointSeiPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	unsigned     payloadType = RECOVERY_POINT;

	// Note: payloadSize is fixed temporarily, this may change in future based on what is sent in CodeBufferingPeriod
	unsigned     payloadSize;

	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

	CodeNALUnitHeader(
		outputBitstreamPtr,
		NAL_UNIT_PREFIX_SEI,
		0);

	payloadSize = GetRecoveryPointSEILength(
		recoveryPointSeiPtr);

	for (; payloadType >= 0xff; payloadType -= 0xff) {
		OutputBitstreamWrite(
			outputBitstreamPtr,
			0xff,
			8);
	}
	OutputBitstreamWrite(
		outputBitstreamPtr,
		payloadType,
		8);

	for (; payloadSize >= 0xff; payloadSize -= 0xff) {
		OutputBitstreamWrite(
			outputBitstreamPtr,
			0xff,
			8);
	}
	OutputBitstreamWrite(
		outputBitstreamPtr,
		payloadSize,
		8);

	// recovery_poc_cnt
	WriteSvlc(
		outputBitstreamPtr,
		recoveryPointSeiPtr->recoveryPocCnt);

	// exact_matching_flag
	WriteFlagCavlc(
		outputBitstreamPtr,
		recoveryPointSeiPtr->exactMatchingFlag);

	// broken_link_flag
	WriteFlagCavlc(
		outputBitstreamPtr,
		recoveryPointSeiPtr->brokenLinkFlag);

	if (outputBitstreamPtr->writtenBitsCount % 8 != 0) {
		// bit_equal_to_one
		WriteFlagCavlc(
			outputBitstreamPtr,
			1);

		while (outputBitstreamPtr->writtenBitsCount % 8 != 0) {
			// bit_equal_to_zero
			WriteFlagCavlc(
				outputBitstreamPtr,
				0);
		}
	}

	// Byte Align the Bitstream    
	OutputBitstreamWrite(
		outputBitstreamPtr,
		1,
		1);

	OutputBitstreamWriteAlignZero(
		outputBitstreamPtr);

	return return_error;
}

EB_ERRORTYPE CodeEndOfSequenceNalUnit(
	Bitstream_t             *bitstreamPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;

	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

	CodeNALUnitHeader(
		outputBitstreamPtr,
		NAL_UNIT_EOS,
		0);

	return return_error;
}

EB_ERRORTYPE EncodeContentLightLevelSEI(
    Bitstream_t             *bitstreamPtr,
    AppContentLightLevelSei_t   *contentLightLevelPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    unsigned payloadType = CONTENT_LIGHT_LEVEL_INFO;
    unsigned payloadSize = GetContentLightLevelSEILength();

    OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

    CodeNALUnitHeader(
        outputBitstreamPtr,
        NAL_UNIT_PREFIX_SEI,
        0);

    for (; payloadType >= 0xff; payloadType -= 0xff) {
        OutputBitstreamWrite(
            outputBitstreamPtr,
            0xff,
            8);
    }
    return_error = OutputBitstreamWrite(
        outputBitstreamPtr,
        payloadType,
        8);

    for (; payloadSize >= 0xff; payloadSize -= 0xff) {
        OutputBitstreamWrite(
            outputBitstreamPtr,
            0xff,
            8);
    }
    return_error = OutputBitstreamWrite(
        outputBitstreamPtr,
        payloadSize,
        8);

    //max_content_light_level
    WriteCodeCavlc(
        outputBitstreamPtr,
        contentLightLevelPtr->maxContentLightLevel,
        16);

    // max_pixel_average_light_level
    WriteCodeCavlc(
        outputBitstreamPtr,
        contentLightLevelPtr->maxPicAverageLightLevel,
        16);

    if (outputBitstreamPtr->writtenBitsCount % 8 != 0) {
        // bit_equal_to_one
        WriteFlagCavlc(
            outputBitstreamPtr,
            1);

        while (outputBitstreamPtr->writtenBitsCount % 8 != 0) {
            // bit_equal_to_zero
            WriteFlagCavlc(
                outputBitstreamPtr,
                0);
        }
    }

    // Byte Align the Bitstream
    OutputBitstreamWrite(
        outputBitstreamPtr,
        1,
        1);

    OutputBitstreamWriteAlignZero(
        outputBitstreamPtr);

    return return_error;
}

EB_ERRORTYPE EncodeMasteringDisplayColorVolumeSEI(
    Bitstream_t             *bitstreamPtr,
    AppMasteringDisplayColorVolumeSei_t   *masterDisplayPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_U32 payloadType = MASTERING_DISPLAY_INFO;
    EB_U32 payloadSize = 0;

    OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;

    payloadSize = GetMasteringDisplayColorVolumeSEILength();

    CodeNALUnitHeader(
        outputBitstreamPtr,
        NAL_UNIT_PREFIX_SEI,
        0);

    for (; payloadType >= 0xff; payloadType -= 0xff) {
        OutputBitstreamWrite(
            outputBitstreamPtr,
            0xff,
            8);
    }
    OutputBitstreamWrite(
        outputBitstreamPtr,
        payloadType,
        8);

    for (; payloadSize >= 0xff; payloadSize -= 0xff) {
        OutputBitstreamWrite(
            outputBitstreamPtr,
            0xff,
            8);
    }
    OutputBitstreamWrite(
        outputBitstreamPtr,
        payloadSize,
        8);

    // R, G, B Primaries
    for (int i = 0; i < 3; i++)
    {
        WriteCodeCavlc(
            outputBitstreamPtr,
            masterDisplayPtr->displayPrimaryX[i],
            16);
        WriteCodeCavlc(
            outputBitstreamPtr,
            masterDisplayPtr->displayPrimaryY[i],
            16);
    }

    // White Point Co-ordinates
    WriteCodeCavlc(
        outputBitstreamPtr,
        masterDisplayPtr->whitePointX,
        16);
    WriteCodeCavlc(
        outputBitstreamPtr,
        masterDisplayPtr->whitePointY,
        16);

    // Min & max Luminance
    WriteCodeCavlc(
        outputBitstreamPtr,
        masterDisplayPtr->maxDisplayMasteringLuminance,
        32);
    WriteCodeCavlc(
        outputBitstreamPtr,
        masterDisplayPtr->minDisplayMasteringLuminance,
        32);

    if (outputBitstreamPtr->writtenBitsCount % 8 != 0) {
        // bit_equal_to_one
        WriteFlagCavlc(
            outputBitstreamPtr,
            1);

        while (outputBitstreamPtr->writtenBitsCount % 8 != 0) {
            // bit_equal_to_zero
            WriteFlagCavlc(
                outputBitstreamPtr,
                0);
        }
    }

    // Byte Align the Bitstream
    OutputBitstreamWrite(
        outputBitstreamPtr,
        1,
        1);

    OutputBitstreamWriteAlignZero(
        outputBitstreamPtr);

    return return_error;
}

EB_ERRORTYPE CodeDolbyVisionRpuMetadata(
    Bitstream_t  *bitstreamPtr,
    PictureControlSet_t *pictureControlSetPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;
    EB_SEI_MESSAGE *rpu = &pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->dolbyVisionRpu;

    CodeNALUnitHeader(
        outputBitstreamPtr,
        NAL_UNIT_UNSPECIFIED_62,
        0);

    for (EB_U32 i = 0; i < rpu->payloadSize; i++)
        WriteCodeCavlc(outputBitstreamPtr, rpu->payload[i], 8);

    return return_error;
}

EB_ERRORTYPE CopyRbspBitstreamToPayload(
	Bitstream_t *bitstreamPtr,
	EB_BYTE      outputBuffer,
	EB_U32      *outputBufferIndex,
	EB_U32      *outputBufferSize,
	EncodeContext_t         *encodeContextPtr,
	NalUnitType naltype)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	OutputBitstreamUnit_t *outputBitstreamPtr = (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr;


	CHECK_REPORT_ERROR(
		((outputBitstreamPtr->writtenBitsCount >> 3) + (*outputBufferIndex) < (*outputBufferSize)),
		encodeContextPtr->appCallbackPtr,
		EB_ENC_EC_ERROR2);



	OutputBitstreamRBSPToPayload(
		outputBitstreamPtr,
		outputBuffer,
		outputBufferIndex,
		outputBufferSize,
		0,
		naltype);

	return return_error;
}


EB_ERRORTYPE BitstreamCtor(
	Bitstream_t **bitstreamDblPtr,
	EB_U32 bufferSize)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_MALLOC(Bitstream_t*, *bitstreamDblPtr, sizeof(Bitstream_t), EB_N_PTR);

    EB_MALLOC(EB_PTR, (*bitstreamDblPtr)->outputBitstreamPtr, sizeof(OutputBitstreamUnit_t), EB_N_PTR);

    return_error = OutputBitstreamUnitCtor(
		(OutputBitstreamUnit_t *)(*bitstreamDblPtr)->outputBitstreamPtr,
		bufferSize);

    return return_error;
}


    

EB_ERRORTYPE EntropyCoderCtor(
	EntropyCoder_t **entropyCoderDblPtr,
	EB_U32 bufferSize)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_MALLOC(EntropyCoder_t*, *entropyCoderDblPtr, sizeof(EntropyCoder_t), EB_N_PTR);

    EB_MALLOC(EB_PTR, (*entropyCoderDblPtr)->cabacEncodeContextPtr, sizeof(CabacEncodeContext_t), EB_N_PTR);

	CabacCtor(
		(CabacEncodeContext_t *)(*entropyCoderDblPtr)->cabacEncodeContextPtr);


    return_error = OutputBitstreamUnitCtor(
		&((((CabacEncodeContext_t*)(*entropyCoderDblPtr)->cabacEncodeContextPtr)->bacEncContext).m_pcTComBitIf),
		bufferSize);

    return return_error;
}




EB_PTR EntropyCoderGetBitstreamPtr(
	EntropyCoder_t *entropyCoderPtr)
{
	CabacEncodeContext_t *cabacEncCtxPtr = (CabacEncodeContext_t*)entropyCoderPtr->cabacEncodeContextPtr;
	EB_PTR bitstreamPtr = (EB_PTR)&(cabacEncCtxPtr->bacEncContext.m_pcTComBitIf);

	return bitstreamPtr;
}

EB_ERRORTYPE EstimateQuantizedCoefficients_Update_SSE2(
    CoeffCtxtMdl_t               *updatedCoeffCtxModel,
    CabacCost_t                  *CabacCost,
    CabacEncodeContext_t         *cabacEncodeCtxPtr,
    EB_U32                        size,                 // Input: TU size
    EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
    EB_U32                        intraLumaMode,
    EB_U32                        intraChromaMode,
    EB_S16                       *coeffBufferPtr,
    const EB_U32                  coeffStride,
    EB_U32                        componentType,
    EB_U32                        numNonZeroCoeffs,
    EB_U64                       *coeffBitsLong)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    EB_U32 isChroma = componentType != COMPONENT_LUMA;
    EB_U32 logBlockSize = Log2f(size);
    EB_U32 coeffBits = 0;
    EB_U32 * ctxModelPtr;

    // DC-only fast track
    if (numNonZeroCoeffs == 1 && coeffBufferPtr[0] != 0)
    {
        EB_S32 blockSizeOffset;
        blockSizeOffset = isChroma ? NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS : ((logBlockSize - 2) * 3 + ((logBlockSize - 1) >> 2));

        //coeffBits += CabacEstimatedBits[0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigXContextModel[blockSizeOffset]];

        ctxModelPtr = &(updatedCoeffCtxModel->lastSigXContextModel[blockSizeOffset]);
        coeffBits += CabacEstimatedBits[0 ^ ctxModelPtr[0]];
        ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(0, ctxModelPtr);

        //coeffBits += CabacEstimatedBits[0 ^ cabacEncodeCtxPtr->contextModelEncContext.lastSigYContextModel[blockSizeOffset]];
        ctxModelPtr = &(updatedCoeffCtxModel->lastSigYContextModel[blockSizeOffset]);
        coeffBits += CabacEstimatedBits[0 ^ ctxModelPtr[0]];
        ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(0, ctxModelPtr);

        {
            EB_S32 absVal = ABS(coeffBufferPtr[0]);
            EB_U32 symbolValue = absVal > 1;
            EB_U32 contextOffset;
            EB_U32 contextOffset2;

            contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS;
            contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS;

            // Add bits for coeff_abs_level_greater1_flag
            //coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset + 1]];
            ctxModelPtr = &(updatedCoeffCtxModel->greaterThanOneContextModel[contextOffset + 1]);
            coeffBits += CabacEstimatedBits[symbolValue ^ ctxModelPtr[0]];
            ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(symbolValue, ctxModelPtr);

            if (symbolValue)
            {
                symbolValue = absVal > 2;

                // Add bits for coeff_abs_level_greater2_flag
                //coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[contextOffset2]];
                ctxModelPtr = &(updatedCoeffCtxModel->greaterThanTwoContextModel[contextOffset2]);
                coeffBits += CabacEstimatedBits[symbolValue ^ ctxModelPtr[0]];
                ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(symbolValue, ctxModelPtr);

                if (symbolValue)
                {
                    // Golomb Rice parameter is known to be 0 here
                    coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 3, 0);
                }
            }
        }

        coeffBits += ONE_BIT; // Sign bit
                              // Add local bit counter to global bit counter
        *coeffBitsLong += coeffBits;

        return return_error;
    }

    // Compute the scanning type
    // optimized this if statement later
    {
        EB_U32 scanIndex = SCAN_DIAG2;

        if (type == INTRA_MODE)
        {
            // The test on partition size should be commented out to get conformance for Intra 4x4 !
            //if (partitionSize == SIZE_2Nx2N)
            {
                // note that for Intra 2Nx2N, each CU is one PU. So this mode is the same as
                // the mode of upper-left corner of current CU
                //intraLumaMode   = candidatePtr->intraLumaMode[0];
                //intraChromaMode = candidatePtr->intraChromaMode[0];

                if (logBlockSize <= 3 - isChroma)
                {
                    EB_U32 tempIntraChromaMode = chromaMappingTable[(EB_U32)intraChromaMode];
                    EB_S32 intraMode = (!isChroma || tempIntraChromaMode == EB_INTRA_CHROMA_DM) ? (EB_S32)intraLumaMode : (EB_S32)tempIntraChromaMode;

                    if (ABS(8 - ((intraMode - 2) & 15)) <= 4)
                    {
                        scanIndex = (intraMode & 16) ? SCAN_HOR2 : SCAN_VER2;
                    }
                }
            }
        }

        //-------------------------------------------------------------------------------------------------------------------
        // Coefficients ordered according to scan order (absolute values)
        {
            __m128i linearCoeff[MAX_TU_SIZE * MAX_TU_SIZE / (sizeof(__m128i) / sizeof(EB_U16))];
            EB_U16 *linearCoeffBufferPtr = (EB_U16 *)linearCoeff;

            // Significance map for each 4x4 subblock
            // 1 bit per coefficient
            // i-th bit corresponds to i-th coefficient in forward scan order
            EB_U16 subblockSigmap[MAX_TU_SIZE * MAX_TU_SIZE / (4 * 4)];

            EB_S32 lastScanSet = -1;
            EB_S32 subSetIndex;
            EB_U32 coeffGroupPosition;
            EB_S32 coeffGroupPositionY;
            EB_S32 coeffGroupPositionX;

            // Loop through subblocks to reorder coefficients according to scan order
            // Also derive significance map for each subblock, and determine last subblock that contains nonzero coefficients
            subSetIndex = 0;
            while (1)
            {
                // determine position of subblock within transform block
                coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
                coeffGroupPositionY = coeffGroupPosition >> 4;
                coeffGroupPositionX = coeffGroupPosition & 15;

                if (scanIndex == SCAN_HOR2)
                {
                    // Subblock scan is mirrored for horizontal scan
                    SWAP(coeffGroupPositionX, coeffGroupPositionY);
                }

                {
                    EB_S16 *subblockPtr = coeffBufferPtr + 4 * coeffGroupPositionY * coeffStride + 4 * coeffGroupPositionX;
                    __m128i a0, a1, a2, a3;
                    __m128i b0, b1, c0, c1, d0, d1;

                    a0 = _mm_loadl_epi64((__m128i *)(subblockPtr + 0 * coeffStride)); // 00 01 02 03 -- -- -- --
                    a1 = _mm_loadl_epi64((__m128i *)(subblockPtr + 1 * coeffStride)); // 10 11 12 13 -- -- -- --
                    a2 = _mm_loadl_epi64((__m128i *)(subblockPtr + 2 * coeffStride)); // 20 21 22 23 -- -- -- --
                    a3 = _mm_loadl_epi64((__m128i *)(subblockPtr + 3 * coeffStride)); // 30 31 32 33 -- -- -- --

                    if (scanIndex == SCAN_DIAG2)
                    {
                        b0 = _mm_unpacklo_epi64(a0, a3); // 00 01 02 03 30 31 32 33
                        b1 = _mm_unpacklo_epi16(a1, a2); // 10 20 11 21 12 22 13 23

                        c0 = _mm_unpacklo_epi16(b0, b1); // 00 10 01 20 02 11 03 21
                        c1 = _mm_unpackhi_epi16(b1, b0); // 12 30 22 31 13 32 23 33

                        {
                            int v03 = _mm_extract_epi16(a0, 3);
                            int v30 = _mm_extract_epi16(a3, 0);

                            d0 = _mm_shufflehi_epi16(c0, 0xe1); // 00 10 01 20 11 02 03 21
                            d1 = _mm_shufflelo_epi16(c1, 0xb4); // 12 30 31 22 13 32 23 33

                            d0 = _mm_insert_epi16(d0, v30, 6); // 00 10 01 20 11 02 30 21
                            d1 = _mm_insert_epi16(d1, v03, 1); // 12 03 31 22 13 32 23 33
                        }
                    }
                    else if (scanIndex == SCAN_HOR2)
                    {
                        d0 = _mm_unpacklo_epi64(a0, a1); // 00 01 02 03 10 11 12 13
                        d1 = _mm_unpacklo_epi64(a2, a3); // 20 21 22 23 30 31 32 33
                    }
                    else
                    {
                        b0 = _mm_unpacklo_epi16(a0, a2); // 00 20 01 21 02 22 03 23
                        b1 = _mm_unpacklo_epi16(a1, a3); // 10 30 11 31 12 32 13 33

                        d0 = _mm_unpacklo_epi16(b0, b1); // 00 10 20 30 01 11 21 31
                        d1 = _mm_unpackhi_epi16(b0, b1); // 02 12 22 32 03 13 23 33
                    }

                    {
                        __m128i z0;
                        z0 = _mm_packs_epi16(d0, d1);
                        z0 = _mm_cmpeq_epi8(z0, _mm_setzero_si128());

                        {
                            __m128i s0, s1;
                            // Absolute value (note: _mm_abs_epi16 requires SSSE3)
                            s0 = _mm_srai_epi16(d0, 15);
                            s1 = _mm_srai_epi16(d1, 15);
                            d0 = _mm_sub_epi16(_mm_xor_si128(d0, s0), s0);
                            d1 = _mm_sub_epi16(_mm_xor_si128(d1, s1), s1);
                        }

                        {
                            EB_S32 sigmap = _mm_movemask_epi8(z0) ^ 0xffff;
                            subblockSigmap[subSetIndex] = (EB_U16)sigmap;

                            if (sigmap != 0)
                            {
                                lastScanSet = subSetIndex;
                                linearCoeff[2 * subSetIndex + 0] = d0;
                                linearCoeff[2 * subSetIndex + 1] = d1;

                                // Count number of bits set in sigmap (Hamming weight)
                                {
                                    EB_U32 num;
                                    num = sigmap;
                                    num = (num)-((num >> 1) & 0x5555);
                                    num = (num & 0x3333) + ((num >> 2) & 0x3333);
                                    num = (num + (num >> 4)) & 0x0f0f;
                                    num = (num + (num >> 8)) & 0x1f;

                                    numNonZeroCoeffs -= num;
                                    if (numNonZeroCoeffs == 0)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                subSetIndex++;

            }

            //-------------------------------------------------------------------------------------------------------------------
            // Obtain the last significant X and Y positions and compute their bit cost
            {
                EB_U32 posLast;
                EB_S32 scanPosLast;
                EB_S32 lastSigXPos;
                EB_S32 lastSigYPos;

                // subblock position
                coeffGroupPosition = sbScans[logBlockSize - 2][lastScanSet];
                coeffGroupPositionY = coeffGroupPosition >> 4;
                coeffGroupPositionX = coeffGroupPosition & 15;
                lastSigYPos = 4 * coeffGroupPositionY;
                lastSigXPos = 4 * coeffGroupPositionX;
                scanPosLast = 16 * lastScanSet;

                // position within subblock
                posLast = Log2f(subblockSigmap[lastScanSet]);
                coeffGroupPosition = scans4[scanIndex != SCAN_DIAG2][posLast];
                lastSigYPos += coeffGroupPosition >> 2;
                lastSigXPos += coeffGroupPosition & 3;
                scanPosLast += posLast;

                // Should swap row/col for SCAN_HOR and SCAN_VER:
                // - SCAN_HOR because the subscan order is mirrored (compared to SCAN_DIAG)
                // - SCAN_VER because of equation (7-66) in H.265 (04/2013)
                // Note that the scans4 array is adjusted to reflect this
                if (scanIndex != SCAN_DIAG2)
                {
                    SWAP(lastSigXPos, lastSigYPos);
                }

                // Encode the position of last significant coefficient
                //coeffBits += EstimateLastSignificantXY                             (cabacEncodeCtxPtr, lastSigXPos, lastSigYPos, size, logBlockSize, isChroma);
                coeffBits += EstimateLastSignificantXY_UPDATE(updatedCoeffCtxModel, cabacEncodeCtxPtr, lastSigXPos, lastSigYPos, size, logBlockSize, isChroma);

                //-------------------------------------------------------------------------------------------------------------------
                // Encode Coefficient levels
                {
                    EB_U32 significantFlagContextOffset = (!isChroma) ? 0 : NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS;

                    EB_U32 contextOffset1 = 1;

                    // Memory for 4x4 subblock-level significance flag
                    // Bits 0-7 are for current diagonal
                    // Bits 16-23 are for previous diagonal
                    EB_U32 sbSigMemory = 0;

                    // Subblock diagonal index for previous 4x4 subblock (diagonal index is row+col)
                    EB_S32 sbPrevDiagIdx = -1;

                    // Loop over subblocks
                    subSetIndex = lastScanSet;
                    do
                    {
                        // 1. Subblock-level significance flag

                        // Assign default value that works for first and last subblock
                        EB_S32 significantFlagContextPattern = sbSigMemory & 3;

                        if (subSetIndex != 0)
                        {
                            coeffGroupPosition = sbScans[logBlockSize - 2][subSetIndex];
                            coeffGroupPositionY = coeffGroupPosition >> 4;
                            coeffGroupPositionX = coeffGroupPosition & 15;

                            {
                                EB_S32 sbDiagIdx = coeffGroupPositionY + coeffGroupPositionX;
                                if (sbDiagIdx != sbPrevDiagIdx)
                                {
                                    sbSigMemory = sbSigMemory << 16;
                                    sbPrevDiagIdx = sbDiagIdx;
                                }

                                if (subSetIndex != lastScanSet)
                                {
                                    EB_U32 sigCoeffGroupContextIndex;
                                    EB_U32 significanceFlag;

                                    significantFlagContextPattern = (sbSigMemory >> (16 + coeffGroupPositionY)) & 3;
                                    sigCoeffGroupContextIndex = significantFlagContextPattern != 0;
                                    sigCoeffGroupContextIndex += isChroma * NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS;

                                    significanceFlag = (subblockSigmap[subSetIndex] != 0);

                                    // Add bits for coded_sub_block_flag
                                    //coeffBits += CabacEstimatedBits[significanceFlag ^ cabacEncodeCtxPtr->contextModelEncContext.coeffGroupSigFlagContextModel[sigCoeffGroupContextIndex]];
                                    ctxModelPtr = &(updatedCoeffCtxModel->coeffGroupSigFlagContextModel[sigCoeffGroupContextIndex]);
                                    coeffBits += CabacEstimatedBits[significanceFlag ^ ctxModelPtr[0]];
                                    ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(significanceFlag, ctxModelPtr);
                                    if (!significanceFlag)
                                    {
                                        // Nothing else to do for this subblock since all coefficients in it are zero
                                        continue;
                                    }
                                }

                                // Memorize that current subblock is nonzero
                                sbSigMemory += 1 << coeffGroupPositionY;
                            }
                        }

                        // 2. Coefficient significance flags
                        {
                            EB_S32 numNonZero = 0; // Number of nonzero coefficients in current subblock
                            EB_S32 absCoeff[16]; // Array containing list of nonzero coefficients (size given by numNonZero)

                                                 // Use do {} while(0) loop to avoid goto statement (early exit)
                            do
                            {
                                EB_S32 scanPosSig;
                                EB_S32 sigMap = subblockSigmap[subSetIndex];
                                EB_S32 subPosition = subSetIndex << LOG2_SCAN_SET_SIZE;
                                EB_S32 subPosition2 = subPosition;

                                if (subSetIndex == lastScanSet)
                                {
                                    absCoeff[0] = linearCoeffBufferPtr[scanPosLast];
                                    numNonZero = 1;
                                    if (sigMap == 1)
                                    {
                                        // Nothing else to do here (only "DC" coeff within subblock)
                                        break;
                                    }
                                    scanPosSig = scanPosLast - 1;
                                    sigMap <<= 31 - (scanPosSig & 15);
                                }
                                else
                                {
                                    if (sigMap == 1 && subSetIndex != 0)
                                    {
                                        subPosition2++;
                                        absCoeff[0] = linearCoeffBufferPtr[subPosition];
                                        numNonZero = 1;
                                    }
                                    scanPosSig = subPosition + 15;
                                    sigMap <<= 16;
                                }

                                {
                                    EB_U32 tempOffset;
                                    const EB_U8 *contextIndexMapPtr;

                                    if (logBlockSize == 2)
                                    {
                                        tempOffset = 0;
                                        contextIndexMapPtr = contextIndexMap4[scanIndex];
                                    }
                                    else
                                    {
                                        tempOffset = (logBlockSize == 3) ? (scanIndex == SCAN_DIAG2 ? 9 : 15) : (!isChroma ? 21 : 12);
                                        tempOffset += (!isChroma && subSetIndex != 0) ? 3 : 0;
                                        contextIndexMapPtr = contextIndexMap8[scanIndex != SCAN_DIAG2][significantFlagContextPattern] - subPosition;
                                    }

                                    // Loop over coefficients
                                    do
                                    {
                                        EB_U32 sigContextIndex;
                                        EB_S32 sigCoeffFlag = sigMap < 0;

                                        sigContextIndex = (scanPosSig == 0) ? 0 : contextIndexMapPtr[scanPosSig] + tempOffset;

                                        // Add bits for sig_coeff_flag
                                        //coeffBits += CabacEstimatedBits[sigCoeffFlag ^ cabacEncodeCtxPtr->contextModelEncContext.significanceFlagContextModel[significantFlagContextOffset + sigContextIndex]];
                                        ctxModelPtr = &(updatedCoeffCtxModel->significanceFlagContextModel[significantFlagContextOffset + sigContextIndex]);
                                        coeffBits += CabacEstimatedBits[sigCoeffFlag ^ ctxModelPtr[0]];
                                        ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(sigCoeffFlag, ctxModelPtr);

                                        if (sigCoeffFlag)
                                        {
                                            if (scanPosSig >= 0) {
                                                absCoeff[numNonZero] = linearCoeffBufferPtr[scanPosSig];
                                            }
                                            numNonZero++;
                                        }

                                        sigMap <<= 1;
                                        scanPosSig--;
                                    } while (scanPosSig >= subPosition2);
                                }
                            } while (0);

                            // 3. Coefficient level values
                            {
                                EB_U32 golombRiceParam = 0;
                                EB_S32 index;
                                EB_U32 contextSet;
                                EB_S32 numCoeffWithCodedGt1Flag; // Number of coefficients for which >1 flag is coded
                                EB_U32 contextOffset;
                                EB_U32 contextOffset2;

                                contextSet = (subSetIndex != 0 && !isChroma) ? 2 : 0;
                                contextSet += (contextOffset1 == 0);
                                contextOffset1 = 1;
                                contextOffset = isChroma * NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS + 4 * contextSet;
                                contextOffset2 = isChroma * NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS + contextSet;

                                numCoeffWithCodedGt1Flag = MIN(GREATER_THAN1_MAX_NUMBER, numNonZero);

                                coeffBits += ONE_BIT * numNonZero; // Add bits for coeff_sign_flag (all coefficients in subblock)
                                
                                // Loop over coefficients until base value of Exp-Golomb coding changes
                                // Base value changes after either
                                // - 8th coefficient
                                // - a coefficient larger than 1                

                                for (index = 0; index < numCoeffWithCodedGt1Flag; index++)
                                {
                                    EB_S32 absVal = absCoeff[index];
                                    EB_U32 symbolValue = absVal > 1;

                                    // Add bits for coeff_abs_level_greater1_flag
                                    //coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset + contextOffset1]];
                                    ctxModelPtr = &(updatedCoeffCtxModel->greaterThanOneContextModel[contextOffset + contextOffset1]);
                                    coeffBits += CabacEstimatedBits[symbolValue ^ ctxModelPtr[0]];
                                    ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(symbolValue, ctxModelPtr);
                                    if (symbolValue)
                                    {
                                        symbolValue = absVal > 2;

                                        // Add bits for coeff_abs_level_greater2_flag
                                        //coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanTwoContextModel[contextOffset2]];
                                        ctxModelPtr = &(updatedCoeffCtxModel->greaterThanTwoContextModel[contextOffset2]);
                                        coeffBits += CabacEstimatedBits[symbolValue ^ ctxModelPtr[0]];
                                        ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(symbolValue, ctxModelPtr);
                                        if (symbolValue)
                                        {
                                            // Golomb Rice parameter is known to be 0 here
                                            coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 3, 0);
                                            golombRiceParam = (absVal > 3);
                                        }

                                        index++;
                                        contextOffset1 = 0;

                                        // Exit loop early as remaining coefficients are coded differently
                                        break;
                                    }

                                    if (contextOffset1 < 3)
                                    {
                                        contextOffset1++;
                                    }
                                }

                                // Loop over coefficients after first one that was > 1 but before 8th one
                                // Base value is know to be equal to 2
                                for (; index < numCoeffWithCodedGt1Flag; index++)
                                {
                                    EB_S32 absVal = absCoeff[index];
                                    EB_U32 symbolValue = absVal > 1;

                                    // Add bits for >1 flag
                                    //coeffBits += CabacEstimatedBits[symbolValue ^ cabacEncodeCtxPtr->contextModelEncContext.greaterThanOneContextModel[contextOffset]];
                                    ctxModelPtr = &(updatedCoeffCtxModel->greaterThanOneContextModel[contextOffset]);
                                    coeffBits += CabacEstimatedBits[symbolValue ^ ctxModelPtr[0]];
                                    ctxModelPtr[0] = UPDATE_CONTEXT_MODEL(symbolValue, ctxModelPtr);
                                    if (symbolValue)
                                    {
                                        coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 2, golombRiceParam);

                                        // Update Golomb-Rice parameter
                                        if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
                                    }
                                }

                                // Loop over remaining coefficients (8th and beyond)
                                // Base value is known to be equal to 1
                                for (; index < numNonZero; index++)
                                {
                                    EB_S32 absVal = absCoeff[index];

                                    coeffBits += EstimateRemainingCoeffExponentialGolombCode(absVal - 1, golombRiceParam);

                                    // Update Golomb-Rice parameter
                                    if (golombRiceParam < 4 && absVal >(3 << golombRiceParam)) golombRiceParam++;
                                }
                            }
                        }
                    } while (--subSetIndex >= 0);
                }
            }
        }
    }

    // Add local bit counter to global bit counter
    *coeffBitsLong += coeffBits;
    (void)CabacCost;
    return return_error;
}
