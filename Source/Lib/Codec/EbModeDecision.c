/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/***************************************
* Includes
***************************************/
#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbSequenceControlSet.h"

#include "EbModeDecision.h"
#include "EbAdaptiveMotionVectorPrediction.h"
#include "EbMvMerge.h"
#include "EbTransformUnit.h"
#include "EbModeDecisionProcess.h"
#include "EbComputeSAD.h"
#include "EbModeDecisionConfiguration.h"

/********************************************
* Constants
********************************************/
static EB_U32  AntiContouringIntraMode[11] = { EB_INTRA_PLANAR, EB_INTRA_DC, EB_INTRA_HORIZONTAL, EB_INTRA_VERTICAL,
EB_INTRA_MODE_2, EB_INTRA_MODE_6, EB_INTRA_MODE_14, EB_INTRA_MODE_18, EB_INTRA_MODE_22, EB_INTRA_MODE_30, EB_INTRA_MODE_34 };

EB_BOOL AntiContouringIntraModeValidityPerDepth[35] =
{
	EB_TRUE, EB_TRUE,
    EB_TRUE, EB_FALSE, EB_FALSE, EB_FALSE,
    EB_TRUE, EB_FALSE, EB_FALSE, EB_FALSE,
    EB_TRUE, EB_FALSE, EB_FALSE, EB_FALSE,
    EB_TRUE, EB_FALSE, EB_FALSE, EB_FALSE,
    EB_TRUE, EB_FALSE, EB_FALSE, EB_FALSE,
	EB_TRUE, EB_FALSE, EB_FALSE, EB_FALSE,
    EB_TRUE, EB_FALSE, EB_FALSE, EB_FALSE,
    EB_TRUE, EB_FALSE, EB_FALSE, EB_FALSE,
    EB_TRUE

};


const EB_U32 parentIndex[85] = { 0, 0, 0, 2, 2, 2, 2, 0, 7, 7, 7, 7, 0, 12, 12, 12, 12, 0, 17, 17, 17, 17, 0, 0,
23, 23, 23, 23, 0, 28, 28, 28, 28, 0, 33, 33, 33, 33, 0, 38, 38, 38, 38, 0, 0,
44, 44, 44, 44, 0, 49, 49, 49, 49, 0, 54, 54, 54, 54, 0, 59, 59, 59, 59, 0, 0,
65, 65, 65, 65, 0, 70, 70, 70, 70, 0, 75, 75, 75, 75, 0, 80, 80, 80, 80 };

EB_ERRORTYPE IntraPredOnSrc(
    ModeDecisionContext_t                  *mdContextPtr,
    EB_U32                                  componentMask,
    PictureControlSet_t                    *pictureControlSetPtr,
    EbPictureBufferDesc_t                  * predictionPtr,
    EB_U32                                   intraMode);


EB_U8 GetNumOfIntraModesFromOisPoint(
    PictureParentControlSet_t   *pictureControlSetPtr,
    EB_U32                       meSad,
    EB_U32                       oisDcSad
);
extern EB_U32 stage1ModesArray[];

void intraSearchTheseModesOutputBest(   
    ModeDecisionContext_t                *contextPtr,
    PictureControlSet_t                 *pictureControlSetPtr,
    EB_U8                                *src,
    EB_U32                               srcStride,
    EB_U8                                NumOfModesToTest,   
    EB_U32                              *bestMode,  
    EB_U32                              *bestSADOut
)
{
    EB_U32   cuSize = contextPtr->cuSize;
    EB_U32   candidateIndex;
    EB_U32   mode;
    EB_U32   bestSAD = 32 * 32 * 255;
    EB_U32	 sadCurr;

  
    for (candidateIndex = 0; candidateIndex < NumOfModesToTest; candidateIndex++) {

        mode =  stage1ModesArray[candidateIndex];

        // Intra Prediction
        IntraPredOnSrc(
            contextPtr,
            PICTURE_BUFFER_DESC_LUMA_MASK,           
            pictureControlSetPtr,
            contextPtr->predictionBuffer,
            mode);

        const EB_U32 puOriginIndex = (contextPtr->cuOriginY & 63) * 64 + (contextPtr->cuOriginX & 63);

        //Distortion
        sadCurr  = (EB_U32)NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][cuSize >> 3](
            src,
            srcStride,
            &(contextPtr->predictionBuffer->bufferY[puOriginIndex]),
            MAX_LCU_SIZE,
            cuSize,
            cuSize);

        //keep track of best SAD
        if (sadCurr < bestSAD) {
            *bestMode = mode;
            bestSAD = sadCurr;
        }

    }
    *bestSADOut = bestSAD;
}



/***************************************
* Mode Decision Candidate Ctor
***************************************/
EB_ERRORTYPE ModeDecisionCandidateBufferCtor(
	ModeDecisionCandidateBuffer_t **bufferDblPtr,
	EB_U16                          lcuMaxSize,
	EB_BITDEPTH                     maxBitdepth,
	EB_U64                         *fastCostPtr,
	EB_U64                         *fullCostPtr,
	EB_U64                         *fullCostSkipPtr,
	EB_U64                         *fullCostMergePtr)
{
	EbPictureBufferDescInitData_t pictureBufferDescInitData;
	EbPictureBufferDescInitData_t doubleWidthPictureBufferDescInitData;
	EB_ERRORTYPE return_error = EB_ErrorNone;
	// Allocate Buffer
	ModeDecisionCandidateBuffer_t *bufferPtr;
	EB_MALLOC(ModeDecisionCandidateBuffer_t*, bufferPtr, sizeof(ModeDecisionCandidateBuffer_t), EB_N_PTR);
	*bufferDblPtr = bufferPtr;

	// Init Picture Data
	pictureBufferDescInitData.maxWidth = lcuMaxSize;
	pictureBufferDescInitData.maxHeight = lcuMaxSize;
	pictureBufferDescInitData.bitDepth = maxBitdepth;
	pictureBufferDescInitData.bufferEnableMask = PICTURE_BUFFER_DESC_FULL_MASK;
    pictureBufferDescInitData.colorFormat = EB_YUV420;
	pictureBufferDescInitData.leftPadding = 0;
	pictureBufferDescInitData.rightPadding = 0;
	pictureBufferDescInitData.topPadding = 0;
	pictureBufferDescInitData.botPadding = 0;
	pictureBufferDescInitData.splitMode = EB_FALSE;

	doubleWidthPictureBufferDescInitData.maxWidth = lcuMaxSize;
	doubleWidthPictureBufferDescInitData.maxHeight = lcuMaxSize;
	doubleWidthPictureBufferDescInitData.bitDepth = EB_16BIT;
	doubleWidthPictureBufferDescInitData.bufferEnableMask = PICTURE_BUFFER_DESC_FULL_MASK;
    doubleWidthPictureBufferDescInitData.colorFormat = EB_YUV420;
	doubleWidthPictureBufferDescInitData.leftPadding = 0;
	doubleWidthPictureBufferDescInitData.rightPadding = 0;
	doubleWidthPictureBufferDescInitData.topPadding = 0;
	doubleWidthPictureBufferDescInitData.botPadding = 0;
	doubleWidthPictureBufferDescInitData.splitMode = EB_FALSE;

	// Candidate Ptr
	bufferPtr->candidatePtr = (ModeDecisionCandidate_t*)EB_NULL;

	// Video Buffers
	return_error = EbPictureBufferDescCtor(
		(EB_PTR*)&(bufferPtr->predictionPtr),
		(EB_PTR)&pictureBufferDescInitData);

	if (return_error == EB_ErrorInsufficientResources){
		return EB_ErrorInsufficientResources;
	}
	return_error = EbPictureBufferDescCtor(
		(EB_PTR*)&(bufferPtr->residualQuantCoeffPtr),
		(EB_PTR)&doubleWidthPictureBufferDescInitData);

	if (return_error == EB_ErrorInsufficientResources){
		return EB_ErrorInsufficientResources;
	}

	return_error = EbPictureBufferDescCtor(
		(EB_PTR*)&(bufferPtr->reconCoeffPtr),
		(EB_PTR)&doubleWidthPictureBufferDescInitData);

	if (return_error == EB_ErrorInsufficientResources){
		return EB_ErrorInsufficientResources;
	}
	return_error = EbPictureBufferDescCtor(
		(EB_PTR*)&(bufferPtr->reconPtr),
		(EB_PTR)&pictureBufferDescInitData);

	if (return_error == EB_ErrorInsufficientResources){
		return EB_ErrorInsufficientResources;
	}
	//Distortion
	bufferPtr->residualLumaSad = 0;

	bufferPtr->fullLambdaRate = 0;


	// Costs
	bufferPtr->fastCostPtr = fastCostPtr;
	bufferPtr->fullCostPtr = fullCostPtr;
	bufferPtr->fullCostSkipPtr = fullCostSkipPtr;
	bufferPtr->fullCostMergePtr = fullCostMergePtr;
	return EB_ErrorNone;
}


// Function Declarations
void RoundMv(
    ModeDecisionCandidate_t	*candidateArray,
    EB_U32                   canTotalCnt)
{

    candidateArray[canTotalCnt].motionVector_x_L0 = (candidateArray[canTotalCnt].motionVector_x_L0 + 2)&~0x03;
    candidateArray[canTotalCnt].motionVector_y_L0 = (candidateArray[canTotalCnt].motionVector_y_L0 + 2)&~0x03;

    candidateArray[canTotalCnt].motionVector_x_L1 = (candidateArray[canTotalCnt].motionVector_x_L1 + 2)&~0x03;
    candidateArray[canTotalCnt].motionVector_y_L1 = (candidateArray[canTotalCnt].motionVector_y_L1 + 2)&~0x03;

    return;

}

EB_ERRORTYPE SetMvpClipMVs(
    ModeDecisionCandidate_t  *candidatePtr,
    EB_U32                    cuOriginX,
    EB_U32                    cuOriginY,
    EB_U32                    puIndex,
    EB_U32                    tbSize,
    PictureControlSet_t      *pictureControlSetPtr)
{
    EB_ERRORTYPE  return_error = EB_ErrorNone;

    EB_U32        pictureWidth = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaWidth;
    EB_U32        pictureHeight = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaHeight;

    candidatePtr->motionVectorPredIdx[REF_LIST_0] = 0;
    candidatePtr->motionVectorPred_x[REF_LIST_0]  = 0;
    candidatePtr->motionVectorPred_y[REF_LIST_0]  = 0;
    candidatePtr->motionVectorPredIdx[REF_LIST_1] = 0;
    candidatePtr->motionVectorPred_x[REF_LIST_1]  = 0;
    candidatePtr->motionVectorPred_y[REF_LIST_1]  = 0;

    switch(candidatePtr->predictionDirection[puIndex]) {
    case UNI_PRED_LIST_0:
        // Clip the input MV
        ClipMV(
            cuOriginX,
            cuOriginY,
            &candidatePtr->motionVector_x_L0,
            &candidatePtr->motionVector_y_L0,
            pictureWidth,
            pictureHeight,
            tbSize);

        break;

    case UNI_PRED_LIST_1:

        // Clip the input MV
       ClipMV(
            cuOriginX,
            cuOriginY,
            &candidatePtr->motionVector_x_L1,
            &candidatePtr->motionVector_y_L1,
            pictureWidth,
            pictureHeight,
            tbSize);

        break;

    case BI_PRED:

        // Choose the MVP in list0
        // Clip the input MV
        ClipMV(
            cuOriginX,
            cuOriginY,
            &candidatePtr->motionVector_x_L0,
            &candidatePtr->motionVector_y_L0,
            pictureWidth,
            pictureHeight,
            tbSize);

        // Choose the MVP in list1
        // Clip the input MV
        ClipMV(
            cuOriginX,
            cuOriginY,
            &candidatePtr->motionVector_x_L1,
            &candidatePtr->motionVector_y_L1,
            pictureWidth,
            pictureHeight,
            tbSize);
        break;

    default:
        break;
    }

    return return_error;
}

/***************************************
* Pre-Mode Decision
*   Selects which fast cost modes to
*   do full reconstruction on.
***************************************/
EB_ERRORTYPE PreModeDecision(
	CodingUnit_t                   *cuPtr,
	EB_U32                          bufferTotalCount,
	ModeDecisionCandidateBuffer_t **bufferPtrArray,
	EB_U32                         *fullCandidateTotalCountPtr,
	EB_U8                          *bestCandidateIndexArray,
	EB_U8                          *disableMergeIndex,
    EB_BOOL                         sameFastFullCandidate
    )
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U32 fullCandidateIndex;
	EB_U32 fullReconCandidateCount;
	EB_U32                          highestCostIndex;
	EB_U64                          highestCost;
	EB_U32                          candIndx = 0, i, j, index;

	*fullCandidateTotalCountPtr = bufferTotalCount;

	//Second,  We substract one, because with N buffers we can determine the best N-1 candidates.
	//Note/TODO: in the case number of fast candidate is less or equal to the number of buffers, N buffers would be enough
    if (sameFastFullCandidate){
        fullReconCandidateCount = MAX(1, (*fullCandidateTotalCountPtr));
    }
    else{
        fullReconCandidateCount = MAX(1, (*fullCandidateTotalCountPtr) - 1);
    }

	//With N buffers, we get here with the best N-1, plus the last candidate. We need to exclude the worst, and keep the best N-1.  
	highestCost = *(bufferPtrArray[0]->fastCostPtr);
	highestCostIndex = 0;

	if (bufferTotalCount > 1){
        if (sameFastFullCandidate){
            for (i = 0; i < bufferTotalCount; i++) {
                bestCandidateIndexArray[candIndx++] = (EB_U8)i;
            }
        }
        else{
            for (i = 1; i < bufferTotalCount; i++) {

                if (*(bufferPtrArray[i]->fastCostPtr) >= highestCost){
                    highestCost = *(bufferPtrArray[i]->fastCostPtr);
                    highestCostIndex = i;
                }
            }
            for (i = 0; i < bufferTotalCount; i++) {

                if (i != highestCostIndex){
                    bestCandidateIndexArray[candIndx++] = (EB_U8)i;
                }
            }
        }

	}
	else
		bestCandidateIndexArray[0] = 0;
	for(i = 0; i < fullReconCandidateCount - 1; ++i) {
		for (j = i + 1; j < fullReconCandidateCount; ++j) {
			if ((bufferPtrArray[bestCandidateIndexArray[i]]->candidatePtr->type == INTRA_MODE) &&
				(bufferPtrArray[bestCandidateIndexArray[j]]->candidatePtr->type == INTER_MODE)){
				index = bestCandidateIndexArray[i];
				bestCandidateIndexArray[i] = (EB_U8)bestCandidateIndexArray[j];
				bestCandidateIndexArray[j] = (EB_U8)index;
			}
		}
	}

	// Set (*fullCandidateTotalCountPtr) to fullReconCandidateCount
	(*fullCandidateTotalCountPtr) = fullReconCandidateCount;

	for (i = 0; i < fullReconCandidateCount; ++i) {
		fullCandidateIndex = bestCandidateIndexArray[i];

		// Next, Set the transformSize
		bufferPtrArray[fullCandidateIndex]->candidatePtr->transformSize       = (EB_U8)(CLIP3(TRANSFORM_MIN_SIZE, TRANSFORM_MAX_SIZE, GetCodedUnitStats(cuPtr->leafIndex)->size));
		bufferPtrArray[fullCandidateIndex]->candidatePtr->transformChromaSize = (EB_U8)(CLIP3(TRANSFORM_MIN_SIZE, TRANSFORM_MAX_SIZE, bufferPtrArray[fullCandidateIndex]->candidatePtr->transformSize >> 1));

        // Set disableMergeIndex
		*disableMergeIndex = bufferPtrArray[fullCandidateIndex]->candidatePtr->type == INTER_MODE ? 1 : *disableMergeIndex;
	}

	return return_error;
}

void LimitMvOverBound(
    EB_S16 *mvx,
    EB_S16 *mvy,
    ModeDecisionContext_t           *ctxtPtr,
    const SequenceControlSet_t      *sCSet)
{
    EB_S32 mvxF, mvyF;

    //L0
    mvxF = *mvx;
    mvyF = *mvy;

    EB_S32 cuOriginX = (EB_S32)ctxtPtr->cuOriginX << 2;
    EB_S32 cuOriginY = (EB_S32)ctxtPtr->cuOriginY << 2;
    EB_S32 startX = 0;
    EB_S32 startY = 0;
    EB_S32 endX   = (EB_S32)sCSet->lumaWidth << 2;
    EB_S32 endY   = (EB_S32)sCSet->lumaHeight << 2;
    EB_S32 cuSize = (EB_S32)ctxtPtr->cuStats->size << 2;
    EB_S32 pad = (4 << 2);

    if ((sCSet->tileRowCount * sCSet->tileColumnCount) > 1) {
        const unsigned lcuIndex = ctxtPtr->cuOriginX/sCSet->lcuSize + (ctxtPtr->cuOriginY/sCSet->lcuSize) * sCSet->pictureWidthInLcu;
        startX = (EB_S32)sCSet->lcuParamsArray[lcuIndex].tileStartX << 2;
        startY = (EB_S32)sCSet->lcuParamsArray[lcuIndex].tileStartY << 2;
        endX   = (EB_S32)sCSet->lcuParamsArray[lcuIndex].tileEndX << 2;
        endY   = (EB_S32)sCSet->lcuParamsArray[lcuIndex].tileEndY << 2;
    }
    //Jing: if MV is quarter/half, the 7,8 tap interpolation will cross the boundary
    //Just clamp the MV to integer

    // Horizontal
    if (((mvxF % 4) != 0) &&
            (cuOriginX + mvxF + cuSize > (endX - pad) || (cuOriginX + mvxF < (startX + pad)))) {
        //half/quarter interpolation, and cross the boundary, clamp to integer first 
        mvxF = ((mvxF >> 2) << 2);
    }

    if (cuOriginX + mvxF + cuSize > endX) {
        *mvx = endX - cuSize - cuOriginX;
    }

    if (cuOriginX + mvxF < startX) {
        *mvx = startX - cuOriginX;
    }

    // Vertical
    if (((mvyF % 4) != 0) &&
            (cuOriginY + mvyF + cuSize > (endY - pad) || (cuOriginY + mvyF < (startY + pad)))) {
        //half/quarter interpolation, and cross the boundary, clamp to integer first
        mvyF = ((mvyF >> 2) << 2);
    }

    if (cuOriginY + mvyF + cuSize > endY) {
        *mvy = endY - cuSize - cuOriginY;
    }

    if (cuOriginY + mvyF < startY) {
        *mvy = startY - cuOriginY;
    }
}

void Me2Nx2NCandidatesInjection(
    PictureControlSet_t            *pictureControlSetPtr,
    ModeDecisionContext_t          *contextPtr,
    const SequenceControlSet_t     *sequenceControlSetPtr,
    LargestCodingUnit_t            *lcuPtr,
    const EB_U32                    me2Nx2NTableOffset,
    EB_U32                         *candidateTotalCnt,
    EB_S16                          firstPuAMVPCandArray_x[MAX_NUM_OF_REF_PIC_LIST][2],
    EB_S16                          firstPuAMVPCandArray_y[MAX_NUM_OF_REF_PIC_LIST][2],
    EB_U32                          firstPuNumAvailableAMVPCand[MAX_NUM_OF_REF_PIC_LIST]
    )
{
    EB_U32                   meCandidateIndex;
    EB_U32                   canTotalCnt           = (*candidateTotalCnt);
    const EB_U32             lcuAddr               = lcuPtr->index;
    const EB_U32             cuOriginX             = contextPtr->cuOriginX;
	const EB_U32             cuOriginY             = contextPtr->cuOriginY;

	MeCuResults_t * mePuResult = &pictureControlSetPtr->ParentPcsPtr->meResults[lcuAddr][me2Nx2NTableOffset];
	ModeDecisionCandidate_t	*candidateArray = contextPtr->fastCandidateArray;
	const EB_U32             meTotalCnt = mePuResult->totalMeCandidateIndex;

	for (meCandidateIndex = 0; meCandidateIndex < meTotalCnt; ++meCandidateIndex)
	{



		const EB_U32 interDirection = mePuResult->distortionDirection[meCandidateIndex].direction;
        if (interDirection == BI_PRED &&  pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuPtr->index] == LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE)
            continue;
        candidateArray[canTotalCnt].motionVector_x_L0 = mePuResult->xMvL0;
        candidateArray[canTotalCnt].motionVector_y_L0 = mePuResult->yMvL0;
        candidateArray[canTotalCnt].motionVector_x_L1 = mePuResult->xMvL1;
        candidateArray[canTotalCnt].motionVector_y_L1 = mePuResult->yMvL1;

        if (pictureControlSetPtr->ParentPcsPtr->useSubpelFlag == 0){
            RoundMv(candidateArray,
                canTotalCnt);
        }

        //constrain mv
        if (sequenceControlSetPtr->staticConfig.unrestrictedMotionVector == 0) {
            if (interDirection == UNI_PRED_LIST_0) {
                LimitMvOverBound(
                    &candidateArray[canTotalCnt].motionVector_x_L0,
                    &candidateArray[canTotalCnt].motionVector_y_L0,
                    contextPtr,
                    sequenceControlSetPtr);
            }
            else if (interDirection == UNI_PRED_LIST_1) {
                LimitMvOverBound(
                    &candidateArray[canTotalCnt].motionVector_x_L1,
                    &candidateArray[canTotalCnt].motionVector_y_L1,
                    contextPtr,
                    sequenceControlSetPtr);
            }
            else {
                LimitMvOverBound(
                    &candidateArray[canTotalCnt].motionVector_x_L0,
                    &candidateArray[canTotalCnt].motionVector_y_L0,
                    contextPtr,
                    sequenceControlSetPtr);

                LimitMvOverBound(
                    &candidateArray[canTotalCnt].motionVector_x_L1,
                    &candidateArray[canTotalCnt].motionVector_y_L1,
                    contextPtr,
                    sequenceControlSetPtr);
            }
        }

		candidateArray[canTotalCnt].meDistortion = mePuResult->distortionDirection[meCandidateIndex].distortion;

        candidateArray[canTotalCnt].distortionReady         = 1;

        candidateArray[canTotalCnt].predictionDirection[0] = (EB_PREDDIRECTION)interDirection;

        candidateArray[canTotalCnt].type                   = INTER_MODE;
        candidateArray[canTotalCnt].mergeFlag              = EB_FALSE;
        if (contextPtr->generateAmvpTableMd == EB_FALSE)
        {
            // TODO: Clips mvs along with Limit mvs
            SetMvpClipMVs(
                &candidateArray[canTotalCnt],
                cuOriginX,
                cuOriginY,
                0,
                sequenceControlSetPtr->lcuSize,  
                pictureControlSetPtr
            );

        }
        else
        {

            ChooseMVPIdx_V2(
                &candidateArray[canTotalCnt],
                cuOriginX,
                cuOriginY,
                0,
                sequenceControlSetPtr->lcuSize,
                firstPuAMVPCandArray_x[REF_LIST_0],
                firstPuAMVPCandArray_y[REF_LIST_0],
                firstPuNumAvailableAMVPCand[REF_LIST_0],
                firstPuAMVPCandArray_x[REF_LIST_1],
                firstPuAMVPCandArray_y[REF_LIST_1],
                firstPuNumAvailableAMVPCand[REF_LIST_1],
                pictureControlSetPtr);

        }


        canTotalCnt++;
    }

    // update the total number of candidates injected
    (*candidateTotalCnt) = canTotalCnt;


 return;
}

void Amvp2Nx2NCandidatesInjection(
    PictureControlSet_t            *pictureControlSetPtr,
    ModeDecisionContext_t          *contextPtr,
    const SequenceControlSet_t     *sequenceControlSetPtr,
    EB_U32                         *candidateTotalCnt,
    EB_S16                          firstPuAMVPCandArray_x[MAX_NUM_OF_REF_PIC_LIST][2],
    EB_S16                          firstPuAMVPCandArray_y[MAX_NUM_OF_REF_PIC_LIST][2],
    EB_U32                          firstPuNumAvailableAMVPCand[MAX_NUM_OF_REF_PIC_LIST]
)
{
    ModeDecisionCandidate_t	*candidateArray = contextPtr->fastCandidateArray;



    EB_PREDDIRECTION interDirection;
    EB_U8            amvpCandidateIndex;
    EB_BOOL          isAmvpCandidateAvailable;

    const EB_U32     cuOriginX = contextPtr->cuOriginX;
    const EB_U32     cuOriginY = contextPtr->cuOriginY;
    EB_U32                   canTotalCnt = (*candidateTotalCnt);

    for (interDirection = UNI_PRED_LIST_0; interDirection <= (pictureControlSetPtr->sliceType == EB_B_PICTURE ? BI_PRED : UNI_PRED_LIST_0); ++interDirection)
    {

        const unsigned targetRefList = (interDirection == UNI_PRED_LIST_0 || interDirection == BI_PRED) ? REF_LIST_0 : REF_LIST_1;

        amvpCandidateIndex = 0;

        for (amvpCandidateIndex = 0; amvpCandidateIndex < MAX_AMVP_CANDIDATES_PER_REF_LIST; amvpCandidateIndex++)
        {
            isAmvpCandidateAvailable = EB_TRUE;

            switch (amvpCandidateIndex)
            {
            case ZERO:

                candidateArray[canTotalCnt].motionVector_x_L0 = 0;
                candidateArray[canTotalCnt].motionVector_y_L0 = 0;

                candidateArray[canTotalCnt].motionVector_x_L1 = 0;
                candidateArray[canTotalCnt].motionVector_y_L1 = 0;

                break;

            case AMVP0:
                candidateArray[canTotalCnt].motionVector_x_L0 = firstPuAMVPCandArray_x[targetRefList][0];
                candidateArray[canTotalCnt].motionVector_y_L0 = firstPuAMVPCandArray_y[targetRefList][0];

                candidateArray[canTotalCnt].motionVector_x_L1 = firstPuAMVPCandArray_x[1 - targetRefList][0];
                candidateArray[canTotalCnt].motionVector_y_L1 = firstPuAMVPCandArray_y[1 - targetRefList][0];

                break;

            case AMVP1:

                if (interDirection == BI_PRED) {
                    if (firstPuNumAvailableAMVPCand[REF_LIST_0] == 2 && firstPuNumAvailableAMVPCand[REF_LIST_1] == 2) {

                        candidateArray[canTotalCnt].motionVector_x_L0 = firstPuAMVPCandArray_x[REF_LIST_0][1];
                        candidateArray[canTotalCnt].motionVector_y_L0 = firstPuAMVPCandArray_y[REF_LIST_0][1];

                        candidateArray[canTotalCnt].motionVector_x_L1 = firstPuAMVPCandArray_x[REF_LIST_1][1];
                        candidateArray[canTotalCnt].motionVector_y_L1 = firstPuAMVPCandArray_y[REF_LIST_1][1];

                    }
                    else {
                        isAmvpCandidateAvailable = EB_FALSE;
                    }
                }
                else {
                    if (firstPuNumAvailableAMVPCand[targetRefList] == 2) {
                        candidateArray[canTotalCnt].motionVector_x_L0 = firstPuAMVPCandArray_x[targetRefList][1];
                        candidateArray[canTotalCnt].motionVector_y_L0 = firstPuAMVPCandArray_y[targetRefList][1];

                        candidateArray[canTotalCnt].motionVector_x_L1 = firstPuAMVPCandArray_x[targetRefList][1];
                        candidateArray[canTotalCnt].motionVector_y_L1 = firstPuAMVPCandArray_y[targetRefList][1];

                    }
                    else {
                        isAmvpCandidateAvailable = EB_FALSE;
                    }
                }

                break;
            }

            if (isAmvpCandidateAvailable) {
                candidateArray[canTotalCnt].distortionReady = 0;

                candidateArray[canTotalCnt].predictionDirection[0] = (EB_PREDDIRECTION)interDirection;

                candidateArray[canTotalCnt].type = INTER_MODE;
                candidateArray[canTotalCnt].mergeFlag = EB_FALSE;

                if (pictureControlSetPtr->ParentPcsPtr->useSubpelFlag == 0) {
                    RoundMv(candidateArray,
                        canTotalCnt);
                }


                if (contextPtr->generateAmvpTableMd == EB_FALSE)
                {
                    // TODO: Clips mvs along with Limit mvs
                    SetMvpClipMVs(
                        &candidateArray[canTotalCnt],
                        cuOriginX,
                        cuOriginY,
                        0,
                        sequenceControlSetPtr->lcuSize,
                        pictureControlSetPtr
                    );

                }
                else
                {

                    ChooseMVPIdx_V2(
                        &candidateArray[canTotalCnt],
                        cuOriginX,
                        cuOriginY,
                        0,
                        sequenceControlSetPtr->lcuSize,
                        firstPuAMVPCandArray_x[REF_LIST_0],
                        firstPuAMVPCandArray_y[REF_LIST_0],
                        firstPuNumAvailableAMVPCand[REF_LIST_0],
                        firstPuAMVPCandArray_x[REF_LIST_1],
                        firstPuAMVPCandArray_y[REF_LIST_1],
                        firstPuNumAvailableAMVPCand[REF_LIST_1],
                        pictureControlSetPtr);

                }

                canTotalCnt++;

            }
        }
    }

    // update the total number of candidates injected
    (*candidateTotalCnt) = canTotalCnt;


    return;
}

#define BIPRED_3x3_REFINMENT_POSITIONS 8

EB_S8 BIPRED_3x3_X_POS[BIPRED_3x3_REFINMENT_POSITIONS] = {-1, -1,  0, 1, 1,  1,  0, -1};
EB_S8 BIPRED_3x3_Y_POS[BIPRED_3x3_REFINMENT_POSITIONS] = { 0,  1,  1, 1, 0, -1, -1, -1};

void Unipred3x3CandidatesInjection(
	PictureControlSet_t            *pictureControlSetPtr,
	ModeDecisionContext_t          *contextPtr,
	const SequenceControlSet_t     *sequenceControlSetPtr,
	LargestCodingUnit_t            *lcuPtr,
	const EB_U32                    me2Nx2NTableOffset,
	EB_U32                         *candidateTotalCnt,
	EB_S16                          firstPuAMVPCandArray_x[MAX_NUM_OF_REF_PIC_LIST][2],
	EB_S16                          firstPuAMVPCandArray_y[MAX_NUM_OF_REF_PIC_LIST][2],
	EB_U32                          firstPuNumAvailableAMVPCand[MAX_NUM_OF_REF_PIC_LIST]
)
{
	EB_U32                   bipredIndex;
	EB_U32                   canTotalCnt = (*candidateTotalCnt);
	const EB_U32             lcuAddr = lcuPtr->index;
	const EB_U32             cuOriginX = contextPtr->cuOriginX;
	const EB_U32             cuOriginY = contextPtr->cuOriginY;

	MeCuResults_t * mePuResult = &pictureControlSetPtr->ParentPcsPtr->meResults[lcuAddr][me2Nx2NTableOffset];
	ModeDecisionCandidate_t	*candidateArray = contextPtr->fastCandidateArray;

	// (Best_L0, 8 Best_L1 neighbors)
	for (bipredIndex = 0; bipredIndex < BIPRED_3x3_REFINMENT_POSITIONS; ++bipredIndex)
	{

		const EB_U32 interDirection = UNI_PRED_LIST_1;

		candidateArray[canTotalCnt].motionVector_x_L0 = mePuResult->xMvL0;
		candidateArray[canTotalCnt].motionVector_y_L0 = mePuResult->yMvL0;

		candidateArray[canTotalCnt].motionVector_x_L1 = mePuResult->xMvL1 + BIPRED_3x3_X_POS[bipredIndex];
		candidateArray[canTotalCnt].motionVector_y_L1 = mePuResult->yMvL1 + BIPRED_3x3_Y_POS[bipredIndex];

		if (pictureControlSetPtr->ParentPcsPtr->useSubpelFlag == 0) {
			RoundMv(candidateArray,
				canTotalCnt);
		}

		candidateArray[canTotalCnt].distortionReady = 0;

		candidateArray[canTotalCnt].predictionDirection[0] = (EB_PREDDIRECTION)interDirection;

		candidateArray[canTotalCnt].type = INTER_MODE;
		candidateArray[canTotalCnt].mergeFlag = EB_FALSE;
		if (contextPtr->generateAmvpTableMd == EB_FALSE)
		{
			// TODO: Clips mvs along with Limit mvs
			SetMvpClipMVs(
				&candidateArray[canTotalCnt],
				cuOriginX,
				cuOriginY,
				0,
				sequenceControlSetPtr->lcuSize,
				pictureControlSetPtr
			);

		}
		else
		{

			ChooseMVPIdx_V2(
				&candidateArray[canTotalCnt],
				cuOriginX,
				cuOriginY,
				0,
				sequenceControlSetPtr->lcuSize,
				firstPuAMVPCandArray_x[REF_LIST_0],
				firstPuAMVPCandArray_y[REF_LIST_0],
				firstPuNumAvailableAMVPCand[REF_LIST_0],
				firstPuAMVPCandArray_x[REF_LIST_1],
				firstPuAMVPCandArray_y[REF_LIST_1],
				firstPuNumAvailableAMVPCand[REF_LIST_1],
				pictureControlSetPtr);

		}


		canTotalCnt++;
	}

	// (8 Best_L0 neighbors, Best_L1) :
	for (bipredIndex = 0; bipredIndex < BIPRED_3x3_REFINMENT_POSITIONS; ++bipredIndex)
	{

		const EB_U32 interDirection = UNI_PRED_LIST_0;
		candidateArray[canTotalCnt].motionVector_x_L0 = mePuResult->xMvL0 + BIPRED_3x3_X_POS[bipredIndex];
		candidateArray[canTotalCnt].motionVector_y_L0 = mePuResult->yMvL0 + BIPRED_3x3_Y_POS[bipredIndex];
		candidateArray[canTotalCnt].motionVector_x_L1 = mePuResult->xMvL1;
		candidateArray[canTotalCnt].motionVector_y_L1 = mePuResult->yMvL1;

		if (pictureControlSetPtr->ParentPcsPtr->useSubpelFlag == 0) {
			RoundMv(candidateArray,
				canTotalCnt);
		}

		candidateArray[canTotalCnt].distortionReady = 0;

		candidateArray[canTotalCnt].predictionDirection[0] = (EB_PREDDIRECTION)interDirection;

		candidateArray[canTotalCnt].type = INTER_MODE;
		candidateArray[canTotalCnt].mergeFlag = EB_FALSE;
		if (contextPtr->generateAmvpTableMd == EB_FALSE)
		{
			// TODO: Clips mvs along with Limit mvs
			SetMvpClipMVs(
				&candidateArray[canTotalCnt],
				cuOriginX,
				cuOriginY,
				0,
				sequenceControlSetPtr->lcuSize,
				pictureControlSetPtr
			);

		}
		else
		{

			ChooseMVPIdx_V2(
				&candidateArray[canTotalCnt],
				cuOriginX,
				cuOriginY,
				0,
				sequenceControlSetPtr->lcuSize,
				firstPuAMVPCandArray_x[REF_LIST_0],
				firstPuAMVPCandArray_y[REF_LIST_0],
				firstPuNumAvailableAMVPCand[REF_LIST_0],
				firstPuAMVPCandArray_x[REF_LIST_1],
				firstPuAMVPCandArray_y[REF_LIST_1],
				firstPuNumAvailableAMVPCand[REF_LIST_1],
				pictureControlSetPtr);

		}


		canTotalCnt++;
	}

	// update the total number of candidates injected
	(*candidateTotalCnt) = canTotalCnt;


	return;
}

void Bipred3x3CandidatesInjection(
	PictureControlSet_t            *pictureControlSetPtr,
	ModeDecisionContext_t          *contextPtr,
	const SequenceControlSet_t     *sequenceControlSetPtr,
	LargestCodingUnit_t            *lcuPtr,
	const EB_U32                    me2Nx2NTableOffset,
	EB_U32                         *candidateTotalCnt,
	EB_S16                          firstPuAMVPCandArray_x[MAX_NUM_OF_REF_PIC_LIST][2],
	EB_S16                          firstPuAMVPCandArray_y[MAX_NUM_OF_REF_PIC_LIST][2],
	EB_U32                          firstPuNumAvailableAMVPCand[MAX_NUM_OF_REF_PIC_LIST]
)
{
	EB_U32                   bipredIndex;
	EB_U32                   canTotalCnt = (*candidateTotalCnt);
	const EB_U32             lcuAddr = lcuPtr->index;
	const EB_U32             cuOriginX = contextPtr->cuOriginX;
	const EB_U32             cuOriginY = contextPtr->cuOriginY;

	MeCuResults_t * mePuResult = &pictureControlSetPtr->ParentPcsPtr->meResults[lcuAddr][me2Nx2NTableOffset];
	ModeDecisionCandidate_t	*candidateArray = contextPtr->fastCandidateArray;


	// (Best_L0, 8 Best_L1 neighbors)
	for (bipredIndex = 0; bipredIndex < BIPRED_3x3_REFINMENT_POSITIONS; ++bipredIndex)
	{

		const EB_U32 interDirection = BI_PRED;

		candidateArray[canTotalCnt].motionVector_x_L0 = mePuResult->xMvL0;
		candidateArray[canTotalCnt].motionVector_y_L0 = mePuResult->yMvL0;

		candidateArray[canTotalCnt].motionVector_x_L1 = mePuResult->xMvL1 + BIPRED_3x3_X_POS[bipredIndex];
		candidateArray[canTotalCnt].motionVector_y_L1 = mePuResult->yMvL1 + BIPRED_3x3_Y_POS[bipredIndex];


		if (pictureControlSetPtr->ParentPcsPtr->useSubpelFlag == 0) {
			RoundMv(candidateArray,
				canTotalCnt);
		}

		candidateArray[canTotalCnt].distortionReady = 0;

		candidateArray[canTotalCnt].predictionDirection[0] = (EB_PREDDIRECTION)interDirection;

		candidateArray[canTotalCnt].type = INTER_MODE;
		candidateArray[canTotalCnt].mergeFlag = EB_FALSE;
		if (contextPtr->generateAmvpTableMd == EB_FALSE)
		{
			// TODO: Clips mvs along with Limit mvs
			SetMvpClipMVs(
				&candidateArray[canTotalCnt],
				cuOriginX,
				cuOriginY,
				0,
				sequenceControlSetPtr->lcuSize,
				pictureControlSetPtr
			);

		}
		else
		{

			ChooseMVPIdx_V2(
				&candidateArray[canTotalCnt],
				cuOriginX,
				cuOriginY,
				0,
				sequenceControlSetPtr->lcuSize,
				firstPuAMVPCandArray_x[REF_LIST_0],
				firstPuAMVPCandArray_y[REF_LIST_0],
				firstPuNumAvailableAMVPCand[REF_LIST_0],
				firstPuAMVPCandArray_x[REF_LIST_1],
				firstPuAMVPCandArray_y[REF_LIST_1],
				firstPuNumAvailableAMVPCand[REF_LIST_1],
				pictureControlSetPtr);

		}


		canTotalCnt++;
	}	

	// (8 Best_L0 neighbors, Best_L1) :
	for (bipredIndex = 0; bipredIndex < BIPRED_3x3_REFINMENT_POSITIONS; ++bipredIndex)
	{

		const EB_U32 interDirection = BI_PRED;
		candidateArray[canTotalCnt].motionVector_x_L0 = mePuResult->xMvL0 + BIPRED_3x3_X_POS[bipredIndex];
		candidateArray[canTotalCnt].motionVector_y_L0 = mePuResult->yMvL0 + BIPRED_3x3_Y_POS[bipredIndex];
		candidateArray[canTotalCnt].motionVector_x_L1 = mePuResult->xMvL1;
		candidateArray[canTotalCnt].motionVector_y_L1 = mePuResult->yMvL1;


		if (pictureControlSetPtr->ParentPcsPtr->useSubpelFlag == 0) {
			RoundMv(candidateArray,
				canTotalCnt);
		}

		candidateArray[canTotalCnt].distortionReady = 0;

		candidateArray[canTotalCnt].predictionDirection[0] = (EB_PREDDIRECTION)interDirection;

		candidateArray[canTotalCnt].type = INTER_MODE;
		candidateArray[canTotalCnt].mergeFlag = EB_FALSE;
		if (contextPtr->generateAmvpTableMd == EB_FALSE)
		{
			// TODO: Clips mvs along with Limit mvs
			SetMvpClipMVs(
				&candidateArray[canTotalCnt],
				cuOriginX,
				cuOriginY,
				0,
				sequenceControlSetPtr->lcuSize,
				pictureControlSetPtr
			);

		}
		else
		{

			ChooseMVPIdx_V2(
				&candidateArray[canTotalCnt],
				cuOriginX,
				cuOriginY,
				0,
				sequenceControlSetPtr->lcuSize,
				firstPuAMVPCandArray_x[REF_LIST_0],
				firstPuAMVPCandArray_y[REF_LIST_0],
				firstPuNumAvailableAMVPCand[REF_LIST_0],
				firstPuAMVPCandArray_x[REF_LIST_1],
				firstPuAMVPCandArray_y[REF_LIST_1],
				firstPuNumAvailableAMVPCand[REF_LIST_1],
				pictureControlSetPtr);

		}


		canTotalCnt++;
	}

	// update the total number of candidates injected
	(*candidateTotalCnt) = canTotalCnt;

	return;
}


// END of Function Declarations
void  ProductIntraCandidateInjection(
    PictureControlSet_t            *pictureControlSetPtr,
    ModeDecisionContext_t          *contextPtr,
    const SequenceControlSet_t     *sequenceControlSetPtr,
    LargestCodingUnit_t            *lcuPtr,
    EB_U32                         *candidateTotalCnt,
    const EB_U32                    leafIndex
)


{
    EB_U32                   openLoopIntraCandidate;
    EB_U32                   canTotalCnt = 0;

    const EB_U32             lcuAddr = lcuPtr->index;
    const EB_U32             cuSize = contextPtr->cuStats->size;
    const EB_U32             cuDepth = contextPtr->cuStats->depth;
    const EB_PICTURE           sliceType = pictureControlSetPtr->sliceType;


    ModeDecisionCandidate_t	*candidateArray = contextPtr->fastCandidateArray;
    LcuParams_t				 *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuAddr];

    if (contextPtr->intraInjectionMethod == 2) {
        EB_U8 totModes = MAX_INTRA_MODES;
        for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totModes; ++openLoopIntraCandidate, ++canTotalCnt)
        {
            candidateArray[canTotalCnt].type = INTRA_MODE;
            candidateArray[canTotalCnt].intraLumaMode = openLoopIntraCandidate;
            candidateArray[canTotalCnt].distortionReady = 0;
        }
    }
    else {

        const EB_BOOL  isLeftCu = contextPtr->cuStats->originX == 0;
        const EB_BOOL  isTopCu = contextPtr->cuStats->originY == 0;
        const EB_BOOL  limitIntra = contextPtr->limitIntra;
        const EB_U8    limitLeftMode = cuSize < 32 ? EB_INTRA_MODE_27 : EB_INTRA_VERTICAL;
        const EB_U8    limitTopMode = cuSize < 32 ? EB_INTRA_MODE_9 : EB_INTRA_HORIZONTAL;
        
        EB_BOOL skipOis8x8 = (pictureControlSetPtr->ParentPcsPtr->skipOis8x8 && cuSize == 8);

        if (pictureControlSetPtr->ParentPcsPtr->complexLcuArray[lcuPtr->index] == LCU_COMPLEXITY_STATUS_2) {
            if (pictureControlSetPtr->ParentPcsPtr->cu8x8Mode == CU_8x8_MODE_1) {
                if (limitIntra == 0 || (isLeftCu == 0 && isTopCu == 0))
                {
                    candidateArray[canTotalCnt].type = INTRA_MODE;
                    candidateArray[canTotalCnt].intraLumaMode = EB_INTRA_DC;
                    candidateArray[canTotalCnt].distortionReady = 0;
                    canTotalCnt++;

                    candidateArray[canTotalCnt].type = INTRA_MODE;
                    candidateArray[canTotalCnt].intraLumaMode = EB_INTRA_PLANAR;
                    candidateArray[canTotalCnt].distortionReady = 0;
                }

            }
            else {
                if (skipOis8x8) {
                    if (limitIntra == 0 || (isLeftCu == 0 && isTopCu == 0))
                    {
                        candidateArray[canTotalCnt].type = INTRA_MODE;
                        candidateArray[canTotalCnt].intraLumaMode = EB_INTRA_PLANAR;
                        candidateArray[canTotalCnt].distortionReady = 0;
                        canTotalCnt++;
                    }

                }
                else {

                    const OisCandidate_t *oisCandidate;
                    EB_U32 totalIntraLumaMode;

                    if (cuSize > 8)
                    {
                        const EB_U32 me2Nx2NTableOffset = contextPtr->cuStats->cuNumInDepth + me2Nx2NOffset[contextPtr->cuStats->depth];
                        OisCu32Cu16Results_t	        *oisCu32Cu16ResultsPtr = pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuAddr];
                        oisCandidate = oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset];
                        totalIntraLumaMode = oisCu32Cu16ResultsPtr->totalIntraLumaMode[me2Nx2NTableOffset];

                    }
                    else {
                        OisCu8Results_t	        *oisCu8ResultsPtr = pictureControlSetPtr->ParentPcsPtr->oisCu8Results[lcuAddr];
                        oisCandidate = oisCu8ResultsPtr->sortedOisCandidate[contextPtr->cuStats->cuNumInDepth];
                        totalIntraLumaMode = oisCu8ResultsPtr->totalIntraLumaMode[contextPtr->cuStats->cuNumInDepth];
                    }


                    for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totalIntraLumaMode; ++openLoopIntraCandidate)
                    {
                        const OisCandidate_t *oisCandidatePtr = &oisCandidate[openLoopIntraCandidate];

                        if (limitIntra == 0 || (isLeftCu == 0 && isTopCu == 0))
                        {
                            if (oisCandidatePtr->intraMode == EB_INTRA_PLANAR || oisCandidatePtr->intraMode == EB_INTRA_DC) {
                                candidateArray[canTotalCnt].oisResults = oisCandidatePtr->oisResults;
                                candidateArray[canTotalCnt].type = INTRA_MODE;
                                canTotalCnt++;
                            }
                        }
                    }
                }
            }
        }
        else {
            // No Intra 64x64            
            if (cuDepth != 0)
            {
                //----------------------
                // I Slice
                //----------------------   
                if (sliceType == EB_I_PICTURE) {

                    if (cuSize == 32) {
                        if (contextPtr->intraInjectionMethod == 1 &&
                            DeriveContouringClass(
                                lcuPtr->pictureControlSetPtr->ParentPcsPtr,
                                lcuPtr->index,
                                (EB_U8)leafIndex) == 0) {
                            EB_U8 totModes = MAX_INTRA_MODES;
                            for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totModes; ++openLoopIntraCandidate, ++canTotalCnt)
                            {
                                candidateArray[canTotalCnt].type = INTRA_MODE;
                                candidateArray[canTotalCnt].intraLumaMode = openLoopIntraCandidate;
                                candidateArray[canTotalCnt].distortionReady = 0;
                            }
                        }
                        else {
                            EB_U8 totModes = 4;
                            for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totModes; ++openLoopIntraCandidate, ++canTotalCnt)
                            {
                                candidateArray[canTotalCnt].type = INTRA_MODE;
                                candidateArray[canTotalCnt].intraLumaMode = AntiContouringIntraMode[openLoopIntraCandidate];
                                candidateArray[canTotalCnt].distortionReady = 0;
                            }
                        }
                    }

                    else if (cuSize == 16) {

                        const EB_U32 me2Nx2NTableOffset = contextPtr->cuStats->cuNumInDepth + me2Nx2NOffset[contextPtr->cuStats->depth];
                        OisCu32Cu16Results_t	        *oisCu32Cu16ResultsPtr = pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuAddr];
                        const OisCandidate_t            *oisCandidate = oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset];
                        const EB_U32 totalIntraLumaMode = oisCu32Cu16ResultsPtr->totalIntraLumaMode[me2Nx2NTableOffset];


                        // Mainline
                        if (contextPtr->intraInjectionMethod == 1 &&
                            DeriveContouringClass(
                                lcuPtr->pictureControlSetPtr->ParentPcsPtr,
                                lcuPtr->index,
                                (EB_U8)leafIndex) == 0) {

                            EB_U8 totModes = MAX_INTRA_MODES;
                            for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totModes; ++openLoopIntraCandidate, ++canTotalCnt)
                            {
                                candidateArray[canTotalCnt].type = INTRA_MODE;
                                candidateArray[canTotalCnt].intraLumaMode = openLoopIntraCandidate;
                                candidateArray[canTotalCnt].distortionReady = 0;
                            }
                        }
                        else

                            for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totalIntraLumaMode; ++openLoopIntraCandidate)
                            {
                                const OisCandidate_t *oisCandidatePtr = &oisCandidate[openLoopIntraCandidate];

                                if (AntiContouringIntraModeValidityPerDepth[oisCandidatePtr->intraMode]) {
                                    candidateArray[canTotalCnt].oisResults = oisCandidatePtr->oisResults;
                                    candidateArray[canTotalCnt].type = INTRA_MODE;
                                    candidateArray[canTotalCnt].distortionReady = 0;
                                    canTotalCnt++;
                                }


                            }
                    }
                    else {

                        OisCu8Results_t	        *oisCu8ResultsPtr = pictureControlSetPtr->ParentPcsPtr->oisCu8Results[lcuAddr];
                        const OisCandidate_t    *oisCandidate = oisCu8ResultsPtr->sortedOisCandidate[contextPtr->cuStats->cuNumInDepth];
                        const EB_U32 totalIntraLumaMode = oisCu8ResultsPtr->totalIntraLumaMode[contextPtr->cuStats->cuNumInDepth];


                        // Mainline
                        if (contextPtr->intraInjectionMethod == 1) {
                            EB_U8 totModes = MAX_INTRA_MODES;
                            for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totModes; ++openLoopIntraCandidate, ++canTotalCnt)
                            {
                                candidateArray[canTotalCnt].type = INTRA_MODE;
                                candidateArray[canTotalCnt].intraLumaMode = openLoopIntraCandidate;
                                candidateArray[canTotalCnt].distortionReady = 0;
                            }
                        }
                        else {
                            for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totalIntraLumaMode; ++openLoopIntraCandidate, ++canTotalCnt)
                            {
                                const OisCandidate_t *oisCandidatePtr = &oisCandidate[openLoopIntraCandidate];

                                candidateArray[canTotalCnt].oisResults = oisCandidatePtr->oisResults;
                                candidateArray[canTotalCnt].type = INTRA_MODE;
                                candidateArray[canTotalCnt].distortionReady = 0;
                            }
                        }
                    }
                }

                //----------------------
                // P/B Slice
                //----------------------  
                else {
					if ((cuSize >= 16 && pictureControlSetPtr->ParentPcsPtr->cu16x16Mode == CU_16x16_MODE_0) || (cuSize == 32)) {

                        {
                            if (pictureControlSetPtr->ParentPcsPtr->limitOisToDcModeFlag)
                            {
                                const EB_U32 me2Nx2NTableOffset = contextPtr->cuStats->cuNumInDepth + me2Nx2NOffset[contextPtr->cuStats->depth];
                                const OisCandidate_t            *oisCandidate = pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuAddr]->sortedOisCandidate[me2Nx2NTableOffset];
                                EB_U32							bestSAD;
                                EB_U32                          bestAngMode = 0;

                                EB_U8 numOfModesToSearch = GetNumOfIntraModesFromOisPoint(
                                    pictureControlSetPtr->ParentPcsPtr,
                                    (EB_U32)pictureControlSetPtr->ParentPcsPtr->meResults[lcuAddr][me2Nx2NTableOffset].distortionDirection[0].distortion,
                                    oisCandidate[0].distortion
                                );


                                if (numOfModesToSearch == 1) {

                                    if (limitIntra == 0 || (isLeftCu == 0 && isTopCu == 0))
                                    {
                                        candidateArray[canTotalCnt].type = INTRA_MODE;
                                        candidateArray[canTotalCnt].oisResults = oisCandidate[0].oisResults;//DC
                                        canTotalCnt++;
                                    }
                                }
                                else {

                                    const EbPictureBufferDesc_t           *inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr;

                                    intraSearchTheseModesOutputBest(
                                        contextPtr,
                                        pictureControlSetPtr,
                                        inputPicturePtr->bufferY + (contextPtr->cuOriginY + inputPicturePtr->originY) * inputPicturePtr->strideY + (contextPtr->cuOriginX + inputPicturePtr->originX),
                                        inputPicturePtr->strideY,
                                        numOfModesToSearch,
                                        &bestAngMode,
                                        &bestSAD);


                                    if (AntiContouringIntraModeValidityPerDepth[bestAngMode])
                                    {
                                        candidateArray[canTotalCnt].type = INTRA_MODE;
                                        candidateArray[canTotalCnt].intraLumaMode = bestAngMode;
                                        candidateArray[canTotalCnt].meDistortion = bestSAD;
                                        candidateArray[canTotalCnt].distortionReady = 1;
                                        canTotalCnt++;

                                        if (limitIntra == 1)
                                            if ((isLeftCu  && bestAngMode != limitLeftMode) || (isTopCu && bestAngMode != limitTopMode))
                                                canTotalCnt--;

                                    }

                                    if (limitIntra == 0 || (isLeftCu == 0 && isTopCu == 0))
                                    {
                                        candidateArray[canTotalCnt].type = INTRA_MODE;
                                        candidateArray[canTotalCnt].intraLumaMode = EB_INTRA_DC;
                                        candidateArray[canTotalCnt].distortionReady = 0;
                                        canTotalCnt++;

                                        candidateArray[canTotalCnt].type = INTRA_MODE;
                                        candidateArray[canTotalCnt].intraLumaMode = EB_INTRA_PLANAR;
                                        candidateArray[canTotalCnt].distortionReady = 0;
                                        canTotalCnt++;
                                    }

                                }


                            }
                            else {


                                const EB_U32 me2Nx2NTableOffset = contextPtr->cuStats->cuNumInDepth + me2Nx2NOffset[contextPtr->cuStats->depth];
                                OisCu32Cu16Results_t	        *oisCu32Cu16ResultsPtr = pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuAddr];
                                const OisCandidate_t            *oisCandidate = oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset];

                                const EB_U32 totalIntraLumaMode = oisCu32Cu16ResultsPtr->totalIntraLumaMode[me2Nx2NTableOffset];

                                // Mainline
                                for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totalIntraLumaMode; ++openLoopIntraCandidate)
                                {
                                    const OisCandidate_t *oisCandidatePtr = &oisCandidate[openLoopIntraCandidate];

                                    if (AntiContouringIntraModeValidityPerDepth[oisCandidatePtr->intraMode]) {

                                        candidateArray[canTotalCnt].oisResults = oisCandidatePtr->oisResults;
                                        candidateArray[canTotalCnt].type = INTRA_MODE;
                                        canTotalCnt++;

                                        const EB_U8 iMode = oisCandidatePtr->intraMode;
                                        if (limitIntra == 1)
                                            if ((isLeftCu  && iMode != limitLeftMode) || (isTopCu && iMode != limitTopMode))
                                                canTotalCnt--;

                                    }

                                }
                            }
                        }
                    }

					else if (cuSize == 16) {
					    if (limitIntra == 0 || (isLeftCu == 0 && isTopCu == 0))
						{
							candidateArray[canTotalCnt].type = INTRA_MODE;
							candidateArray[canTotalCnt].intraLumaMode = EB_INTRA_DC;
							candidateArray[canTotalCnt].distortionReady = 0;
							canTotalCnt++;

							candidateArray[canTotalCnt].type = INTRA_MODE;
							candidateArray[canTotalCnt].intraLumaMode = EB_INTRA_PLANAR;
							candidateArray[canTotalCnt].distortionReady = 0;
							canTotalCnt++;
						}

					}

                    else if (pictureControlSetPtr->ParentPcsPtr->cu8x8Mode == CU_8x8_MODE_1) {

                        if (skipOis8x8) {
                            if (limitIntra == 0 || (isLeftCu == 0 && isTopCu == 0))
                            {
                                candidateArray[canTotalCnt].type = INTRA_MODE;
                                candidateArray[canTotalCnt].intraLumaMode = EB_INTRA_PLANAR;
                                candidateArray[canTotalCnt].distortionReady = 0;
                                canTotalCnt++;
                            }
                        }
                        else {


                            if (lcuParams->isCompleteLcu) {
                                const CodedUnitStats_t  *cuStats = GetCodedUnitStats(parentIndex[leafIndex]);
                                const EB_U32 me2Nx2NTableOffset = cuStats->cuNumInDepth + me2Nx2NOffset[cuStats->depth];
                                OisCu32Cu16Results_t	        *oisCu32Cu16ResultsPtr = pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuAddr];
                                const OisCandidate_t    *oisCandidate = oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset];
                                const EB_U32 totalIntraLumaMode = oisCu32Cu16ResultsPtr->totalIntraLumaMode[me2Nx2NTableOffset];

                                for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totalIntraLumaMode; ++openLoopIntraCandidate)
                                {
                                    const OisCandidate_t *oisCandidatePtr = &oisCandidate[openLoopIntraCandidate];
                                    if (AntiContouringIntraModeValidityPerDepth[oisCandidatePtr->intraMode]) {
                                        if (cuSize < 16 || oisCandidatePtr->intraMode != EB_INTRA_DC) {
                                            candidateArray[canTotalCnt].oisResults = oisCandidatePtr->oisResults;
                                            candidateArray[canTotalCnt].type = INTRA_MODE;
                                            candidateArray[canTotalCnt].distortionReady = 0;
                                            ++canTotalCnt;
                                            const EB_U8 iMode = oisCandidatePtr->intraMode;
                                            if (limitIntra == 1)
                                                if ((isLeftCu  && iMode != limitLeftMode) || (isTopCu && iMode != limitTopMode))
                                                    canTotalCnt--;
                                        }
                                    }
                                }
                            }
                            else {
                                if (limitIntra == 0 || (isLeftCu == 0 && isTopCu == 0))
                                {
                                    // DC
                                    candidateArray[canTotalCnt].type = INTRA_MODE;
                                    candidateArray[canTotalCnt].intraLumaMode = EB_INTRA_DC;
                                    candidateArray[canTotalCnt].distortionReady = 0;
                                    canTotalCnt++;
                                }
                            }
                        }
                    }
                    else {

                        if (skipOis8x8) {
                            if (limitIntra == 0 || (isLeftCu == 0 && isTopCu == 0))
                            {
                                candidateArray[canTotalCnt].type = INTRA_MODE;
                                candidateArray[canTotalCnt].intraLumaMode = EB_INTRA_PLANAR;
                                candidateArray[canTotalCnt].distortionReady = 0;
                                canTotalCnt++;

                            }
                        }
                        else {
                            OisCu8Results_t	        *oisCu8ResultsPtr = pictureControlSetPtr->ParentPcsPtr->oisCu8Results[lcuAddr];
                            const OisCandidate_t    *oisCandidate = oisCu8ResultsPtr->sortedOisCandidate[contextPtr->cuStats->cuNumInDepth];
                            const EB_U32 totalIntraLumaMode = oisCu8ResultsPtr->totalIntraLumaMode[contextPtr->cuStats->cuNumInDepth];


                            // Mainline
                            for (openLoopIntraCandidate = 0; openLoopIntraCandidate < totalIntraLumaMode; ++openLoopIntraCandidate)
                            {
                                const OisCandidate_t *oisCandidatePtr = &oisCandidate[openLoopIntraCandidate];

                                if ((contextPtr->intra8x8RestrictionInterSlice == EB_TRUE && AntiContouringIntraModeValidityPerDepth[oisCandidatePtr->intraMode]) || contextPtr->intra8x8RestrictionInterSlice == EB_FALSE) {

                                    if (cuSize < 16 || oisCandidatePtr->intraMode != EB_INTRA_DC) {
                                        candidateArray[canTotalCnt].oisResults = oisCandidatePtr->oisResults;
                                        candidateArray[canTotalCnt].type = INTRA_MODE;
                                        ++canTotalCnt;

                                        const EB_U8 iMode = oisCandidatePtr->intraMode;
                                        if (limitIntra == 1)
                                            if ((isLeftCu  && iMode != limitLeftMode) || (isTopCu && iMode != limitTopMode))
                                                canTotalCnt--;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // update the total number of candidates injected
    (*candidateTotalCnt) = canTotalCnt;


    return;
}

EB_BOOL CheckForMvOverBound(
    EB_S16 mvx,
    EB_S16 mvy,
    ModeDecisionContext_t           *ctxtPtr,
    const SequenceControlSet_t      *sCSet)
{
    EB_S32 mvxF, mvyF;

    //L0
    mvxF = (EB_S32)mvx;
    mvyF = (EB_S32)mvy;

    EB_S32 cuOriginX = (EB_S32)ctxtPtr->cuOriginX << 2;
    EB_S32 cuOriginY = (EB_S32)ctxtPtr->cuOriginY << 2;
    EB_S32 startX = 0;
    EB_S32 startY = 0;
    EB_S32 endX   = (EB_S32)sCSet->lumaWidth << 2;
    EB_S32 endY   = (EB_S32)sCSet->lumaHeight << 2;
    EB_S32 cuSize = (EB_S32)ctxtPtr->cuStats->size << 2;
    EB_S32 pad = 4 << 2;

    if ((sCSet->tileRowCount * sCSet->tileColumnCount) > 1) {
        const unsigned lcuIndex = ctxtPtr->cuOriginX/sCSet->lcuSize + (ctxtPtr->cuOriginY/sCSet->lcuSize) * sCSet->pictureWidthInLcu;
        startX = (EB_S32)sCSet->lcuParamsArray[lcuIndex].tileStartX << 2;
        startY = (EB_S32)sCSet->lcuParamsArray[lcuIndex].tileStartY << 2;
        endX   = (EB_S32)sCSet->lcuParamsArray[lcuIndex].tileEndX << 2;
        endY   = (EB_S32)sCSet->lcuParamsArray[lcuIndex].tileEndY << 2;
    }

    if (cuOriginX + mvxF + cuSize > (endX - pad)) {

        if (cuOriginX + mvxF + cuSize > endX) {
            return EB_TRUE;
        }
        else {
            if (mvxF % 4 != 0 || mvxF % 8 != 0) {
                return EB_TRUE;
            }
        }
    }

    if (cuOriginY + mvyF + cuSize > (endY - pad)) {
        if (cuOriginY + mvyF + cuSize > endY) {
            return EB_TRUE;
        }
        else {
            if (mvyF % 4 != 0 || mvyF % 8 != 0) {
                return EB_TRUE;
            }
        }
    }

    if (cuOriginX + mvxF < (startX + pad)) {
        if (cuOriginX + mvxF < startX) {
            return EB_TRUE;
        }
        else {
            if (mvxF % 4 != 0 || mvxF % 8 != 0) {
                return EB_TRUE;
            }
        }
    }

    if (cuOriginY + mvyF < (startY + pad)) {
        if (cuOriginY + mvyF < startY) {
            return EB_TRUE;
        }
        else {
            if (mvyF % 4 != 0 || mvyF % 8 != 0) {
                return EB_TRUE;
            }
        }
    }

    return EB_FALSE;
}

void ProductMergeSkip2Nx2NCandidatesInjection(
    ModeDecisionContext_t          *contextPtr,
    const SequenceControlSet_t     *sequenceControlSetPtr,
    InterPredictionContext_t       *interPredictionPtr,
    EB_U32                         *candidateTotalCnt,
    EB_U32                          mvMergeCandidateTotalCount

    )
{
    EB_U32                   canTotalCnt                = (*candidateTotalCnt);
    EB_U8                    modeDecisionCandidateIndex = 0;
    EB_U32                   duplicateIndex;
    EB_BOOL                  mvMergeDuplicateFlag       = EB_FALSE;
    EB_BOOL                  mvOutOfPicFlag             = EB_FALSE;
    (void)sequenceControlSetPtr;
    ModeDecisionCandidate_t	*candidateArray             = contextPtr->fastCandidateArray;

    while (mvMergeCandidateTotalCount)
    {
        if (modeDecisionCandidateIndex < interPredictionPtr->mvMergeCandidateCount)
        {
            const MvMergeCandidate_t *mvMergeCandidatePtr = &interPredictionPtr->mvMergeCandidateArray[modeDecisionCandidateIndex];
            const Mv_t *candidateMv = mvMergeCandidatePtr->mv;

            ModeDecisionCandidate_t *mdCandidatePtr = &candidateArray[canTotalCnt];

            // add a duplicate detector to mode decision array
            mvMergeDuplicateFlag    = EB_FALSE;
            mvOutOfPicFlag          = EB_FALSE;
            duplicateIndex          = modeDecisionCandidateIndex;
            EB_BOOL duplicateFlags[EB_PREDDIRECTION_TOTAL];
            while ((duplicateIndex > 0) && (mvMergeDuplicateFlag == EB_FALSE)) {
                const Mv_t *duplicateMv = interPredictionPtr->mvMergeCandidateArray[--duplicateIndex].mv;

                duplicateFlags[UNI_PRED_LIST_0] = (EB_BOOL)(candidateMv[REF_LIST_0].mvUnion == duplicateMv[REF_LIST_0].mvUnion);
                duplicateFlags[UNI_PRED_LIST_1] = (EB_BOOL)(mvMergeCandidatePtr->predictionDirection != UNI_PRED_LIST_0 && candidateMv[REF_LIST_1].mvUnion == duplicateMv[REF_LIST_1].mvUnion);
                duplicateFlags[BI_PRED] = (EB_BOOL)(duplicateFlags[UNI_PRED_LIST_0] && duplicateFlags[UNI_PRED_LIST_1]);

                mvMergeDuplicateFlag = (EB_BOOL)(mvMergeCandidatePtr->predictionDirection == interPredictionPtr->mvMergeCandidateArray[duplicateIndex].predictionDirection && duplicateFlags[mvMergeCandidatePtr->predictionDirection]);
            }

            if (sequenceControlSetPtr->staticConfig.unrestrictedMotionVector == 0) {
                if (mvMergeCandidatePtr->predictionDirection == UNI_PRED_LIST_0) {
                    mvOutOfPicFlag = CheckForMvOverBound(
                        candidateMv[REF_LIST_0].x,
                        candidateMv[REF_LIST_0].y,
                        contextPtr,
                        sequenceControlSetPtr);
                }
                else if (mvMergeCandidatePtr->predictionDirection == UNI_PRED_LIST_1) {
                    mvOutOfPicFlag = CheckForMvOverBound(
                        candidateMv[REF_LIST_1].x,
                        candidateMv[REF_LIST_1].y,
                        contextPtr,
                        sequenceControlSetPtr);
                }
                else {
                    mvOutOfPicFlag = CheckForMvOverBound(
                        candidateMv[REF_LIST_0].x,
                        candidateMv[REF_LIST_0].y,
                        contextPtr,
                        sequenceControlSetPtr);
                    mvOutOfPicFlag += CheckForMvOverBound(
                        candidateMv[REF_LIST_1].x,
                        candidateMv[REF_LIST_1].y,
                        contextPtr,
                        sequenceControlSetPtr);
                }
            }

            if ((sequenceControlSetPtr->staticConfig.unrestrictedMotionVector && mvMergeDuplicateFlag == EB_FALSE) ||
                (!sequenceControlSetPtr->staticConfig.unrestrictedMotionVector && mvMergeDuplicateFlag == EB_FALSE && mvOutOfPicFlag == EB_FALSE)) {
                mdCandidatePtr->type = INTER_MODE;
                mdCandidatePtr->distortionReady = 0;
                //EB_MEMCPY(&mdCandidatePtr->MVs, &candidateMv[REF_LIST_0].x, 8);
                mdCandidatePtr->motionVector_x_L0 = candidateMv[REF_LIST_0].x;
                mdCandidatePtr->motionVector_y_L0 = candidateMv[REF_LIST_0].y;
                mdCandidatePtr->motionVector_x_L1 = candidateMv[REF_LIST_1].x;
                mdCandidatePtr->motionVector_y_L1 = candidateMv[REF_LIST_1].y;
                mdCandidatePtr->mergeFlag = EB_TRUE;
                mdCandidatePtr->mergeIndex = modeDecisionCandidateIndex;
                mdCandidatePtr->mpmFlag = EB_FALSE;
                mdCandidatePtr->predictionDirection[0] = (EB_PREDDIRECTION)mvMergeCandidatePtr->predictionDirection;
                canTotalCnt++;
            }

        }

        --mvMergeCandidateTotalCount;
        ++modeDecisionCandidateIndex;
    }
    (*candidateTotalCnt) = canTotalCnt;
    return;
}

void ProductMpmCandidatesInjection(
    ModeDecisionContext_t          *contextPtr,
    EB_U32                         *candidateTotalCnt,
	EB_U32                         *bufferTotalCountPtr,
    EB_BOOL							mpmSearch,
	EB_U8	                        mpmSearchCandidate,
	EB_U32                         *mostProbableModeArray
    )

{
    const EB_U32             cuDepth            = contextPtr->cuStats->depth;
    EB_U32                   canTotalCnt        = (*candidateTotalCnt);
    EB_U32                   fastLoopCandidate  = 0;  
	EB_U32                   candidateIndex;
	EB_U32			         mostProbableModeCount;
    EB_BOOL                  mpmPresentFlag;

#ifdef LIMITINRA_MPM_PATCH
    const EB_BOOL            cuSize = contextPtr->cuStats->size;
    const EB_BOOL            isLeftCu = contextPtr->cuStats->originX == 0;
    const EB_BOOL            isTopCu = contextPtr->cuStats->originY == 0;
    const EB_BOOL            limitIntra = contextPtr->limitIntra;
    const EB_U8              limitLeftMode = cuSize < 32 ? EB_INTRA_MODE_27 : EB_INTRA_VERTICAL;
    const EB_U8              limitTopMode = cuSize < 32 ? EB_INTRA_MODE_9 : EB_INTRA_HORIZONTAL;
#endif

    ModeDecisionCandidate_t	*candidateArray     = contextPtr->fastCandidateArray;

    if (mpmSearch && cuDepth != 0){

        fastLoopCandidate = canTotalCnt;

        // Loop over fast loop candidates
        for (candidateIndex = 0; candidateIndex < fastLoopCandidate; ++candidateIndex) {
            candidateArray[candidateIndex].mpmFlag = EB_FALSE;
        }

        // Loop over fast loop candidates
        mostProbableModeCount = 0;

        while (mostProbableModeCount < mpmSearchCandidate) {

#ifdef LIMITINRA_MPM_PATCH
            if (!(limitIntra == 0 || (isLeftCu == 0 && isTopCu == 0))) {
                ++mostProbableModeCount;
                continue;
            }
#endif
            mpmPresentFlag = EB_FALSE;
            for (candidateIndex = 0; candidateIndex < fastLoopCandidate; ++candidateIndex) {

                // Mark MPM mode and add one full loop candidate
                if (candidateArray[candidateIndex].type == INTRA_MODE && mpmPresentFlag == EB_FALSE){

                    if (candidateArray[candidateIndex].intraLumaMode == mostProbableModeArray[mostProbableModeCount]) {

                        candidateArray[candidateIndex].mpmFlag = EB_TRUE;
                        ++(*bufferTotalCountPtr);
                        mpmPresentFlag = EB_TRUE;
                    }
                }
            }
            // add MPM mode to fast loop and add one full loop candidate
            if (mpmPresentFlag == EB_FALSE){
                candidateArray[canTotalCnt].type = INTRA_MODE;
                candidateArray[canTotalCnt].intraLumaMode = mostProbableModeArray[mostProbableModeCount];
                candidateArray[canTotalCnt].distortionReady = 0;
                candidateArray[canTotalCnt].mpmFlag = EB_TRUE;
                canTotalCnt++;
                ++(*bufferTotalCountPtr);
            }
            ++mostProbableModeCount;
        }
    }
    else {

        // Loop over fast loop candidates
        for (candidateIndex = 0; candidateIndex < canTotalCnt; ++candidateIndex) {
            candidateArray[candidateIndex].mpmFlag = EB_FALSE;
        }
    }

    (*candidateTotalCnt) = canTotalCnt;
    return;

}

/***************************************
* ProductGenerateAmvpMergeInterIntraMdCandidatesCU
*   Creates list of initial modes to
*   perform fast cost search on.
***************************************/
EB_ERRORTYPE ProductGenerateAmvpMergeInterIntraMdCandidatesCU(
	LargestCodingUnit_t				 *lcuPtr,
	ModeDecisionContext_t            *contextPtr,
	const EB_U32                      leafIndex,
	const EB_U32                      lcuAddr,
	EB_U32                           *bufferTotalCountPtr,
	EB_U32                           *candidateTotalCountPtr,
	EB_PTR							  interPredContextPtr,
	PictureControlSet_t              *pictureControlSetPtr,
	EB_BOOL							  mpmSearch,
	EB_U8	                          mpmSearchCandidate,
	EB_U32                           *mostProbableModeArray)
{
	InterPredictionContext_t       *interPredictionPtr  = (InterPredictionContext_t*)interPredContextPtr;
	const SequenceControlSet_t *sequenceControlSetPtr   = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	// const LargestCodingUnit_t  *lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuAddr];
	const EB_PICTURE sliceType = pictureControlSetPtr->sliceType;


	EB_U32 fullReconSearchCount = contextPtr->fullReconSearchCount;
	// AMVP
    EB_S16 firstPuAMVPCandArray_x[MAX_NUM_OF_REF_PIC_LIST][2]  ;
	EB_S16 firstPuAMVPCandArray_y[MAX_NUM_OF_REF_PIC_LIST][2]  ;
	EB_U32 firstPuNumAvailableAMVPCand[MAX_NUM_OF_REF_PIC_LIST];
	const EB_U32 cuNumberInDepth = contextPtr->cuStats->cuNumInDepth;
    const EB_U32             cuDepth            = contextPtr->cuStats->depth;
	const EB_U32 me2Nx2NTableOffset = cuNumberInDepth + me2Nx2NOffset[cuDepth];
	EB_U32       mvMergeCandidateTotalCount = contextPtr->mvMergeSkipModeCount;
	EB_S16       amvpArray_x[MAX_NUM_OF_REF_PIC_LIST][2];
	EB_S16       amvpArray_y[MAX_NUM_OF_REF_PIC_LIST][2];
	EB_U32       canTotalCnt = 0;


	//----------------------
	// Intra
	//---------------------- 
    if (cuDepth != 0 && (sliceType == EB_I_PICTURE || cuDepth == 3 || contextPtr->restrictIntraGlobalMotion == EB_FALSE)) {
		const EB_BOOL  isLeftCu = contextPtr->cuStats->originX == 0;
		const EB_BOOL  isTopCu = contextPtr->cuStats->originY == 0;
		EB_BOOL limitIntraLoptLeft = contextPtr->limitIntra == EB_TRUE && isLeftCu  &&  isTopCu;
		if (limitIntraLoptLeft == 0)

        ProductIntraCandidateInjection( // HT not much to do
            pictureControlSetPtr,
            contextPtr,
            sequenceControlSetPtr,
            lcuPtr,
            &canTotalCnt,
			leafIndex
			);
    }

    if (sliceType != EB_I_PICTURE)
    {

        const EB_BOOL tmvpEnableFlag = pictureControlSetPtr->ParentPcsPtr->disableTmvpFlag ? EB_FALSE : EB_TRUE;
        //Generate only this number of merge candidates
        interPredictionPtr->mvMergeCandidateCount = mvMergeCandidateTotalCount;

        if (contextPtr->conformantMvMergeTable == EB_FALSE && contextPtr->generateAmvpTableMd == EB_FALSE) {
            NonConformantGenerateL0L1AmvpMergeLists( // HT not much to do
                contextPtr,
                interPredictionPtr,
                pictureControlSetPtr,
                lcuAddr);
        }
        else
        {
            GenerateL0L1AmvpMergeLists(
                contextPtr,
                interPredictionPtr,
                pictureControlSetPtr,
                tmvpEnableFlag,
                lcuAddr,
                amvpArray_x,
                amvpArray_y,
                firstPuNumAvailableAMVPCand,
                firstPuAMVPCandArray_x,
                firstPuAMVPCandArray_y
            );
        }

        //----------------------
        // Me2Nx2N
        //----------------------    
        Me2Nx2NCandidatesInjection( // HT not much to do
            pictureControlSetPtr,
            contextPtr,
            sequenceControlSetPtr,
            lcuPtr,
            me2Nx2NTableOffset,
            &canTotalCnt,
            firstPuAMVPCandArray_x,
            firstPuAMVPCandArray_y,
            firstPuNumAvailableAMVPCand);

		if (contextPtr->amvpInjection) {

			//----------------------
			// Amvp2Nx2N
			//----------------------   
			Amvp2Nx2NCandidatesInjection(
				pictureControlSetPtr,
				contextPtr,
				sequenceControlSetPtr,
				&canTotalCnt,
				firstPuAMVPCandArray_x,
				firstPuAMVPCandArray_y,
				firstPuNumAvailableAMVPCand);
		}
         
        if (pictureControlSetPtr->sliceType == EB_B_PICTURE) {
            if (contextPtr->bipred3x3Injection) {
                //----------------------
                // Bipred2Nx2N
                //----------------------   

                Bipred3x3CandidatesInjection( // HT not much to do
                    pictureControlSetPtr,
                    contextPtr,
                    sequenceControlSetPtr,
                    lcuPtr,
                    me2Nx2NTableOffset,
                    &canTotalCnt,
                    firstPuAMVPCandArray_x,
                    firstPuAMVPCandArray_y,
                    firstPuNumAvailableAMVPCand);
            }

            if (contextPtr->unipred3x3Injection) {
                //----------------------
                // Unipred2Nx2N
                //---------------------- 
                Unipred3x3CandidatesInjection( // HT not much to do
                    pictureControlSetPtr,
                    contextPtr,
                    sequenceControlSetPtr,
                    lcuPtr,
                    me2Nx2NTableOffset,
                    &canTotalCnt,
                    firstPuAMVPCandArray_x,
                    firstPuAMVPCandArray_y,
                    firstPuNumAvailableAMVPCand);
            }
        }

		//----------------------
		// MergeSkip2Nx2N
		//----------------------
		if (mvMergeCandidateTotalCount) {
			ProductMergeSkip2Nx2NCandidatesInjection( // HT not much to do
				contextPtr,
				sequenceControlSetPtr,
				interPredictionPtr,
				&canTotalCnt,
				mvMergeCandidateTotalCount);
		}

	}

	// Set BufferTotalCount: determines the number of candidates to fully reconstruct
	*bufferTotalCountPtr = fullReconSearchCount;

	// Mark MPM candidates, and update the number of full recon - MPM candidates are going to get pushed to the full, 
	// however they still need to be tested in the fast loop where the predicted, and the fast rate are going to get computed
#ifdef LIMITINRA_MPM_PATCH
    const EB_BOOL  isLeftCu = contextPtr->cuStats->originX == 0;
    const EB_BOOL  isTopCu = contextPtr->cuStats->originY == 0;
    EB_BOOL limitIntraLoptLeft = contextPtr->limitIntra == EB_TRUE && isLeftCu  &&  isTopCu;
    if (limitIntraLoptLeft == 0)
	    ProductMpmCandidatesInjection(
		    contextPtr,
		    &canTotalCnt,
		    bufferTotalCountPtr,
		    mpmSearch,
		    mpmSearchCandidate,
		    mostProbableModeArray);

#else
    ProductMpmCandidatesInjection(
        contextPtr,
        &canTotalCnt,
        bufferTotalCountPtr,
        mpmSearch,
        mpmSearchCandidate,
        mostProbableModeArray);
#endif

	*candidateTotalCountPtr = canTotalCnt;

	// Make sure bufferTotalCount is not larger than the number of fast modes
	*bufferTotalCountPtr = MIN(*candidateTotalCountPtr, *bufferTotalCountPtr);

	return EB_ErrorNone;
}

/***************************************
* Full Mode Decision
***************************************/
EB_U8 ProductFullModeDecision(
    struct ModeDecisionContext_s   *contextPtr,
	CodingUnit_t                   *cuPtr,
	EB_U8                           cuSize,
	ModeDecisionCandidateBuffer_t **bufferPtrArray,
	EB_U32                          candidateTotalCount,
	EB_U8                          *bestCandidateIndexArray,
	EB_U32						   *bestIntraMode)
{

	EB_U8                   candidateIndex;
	EB_U64                  lowestCost = 0xFFFFFFFFFFFFFFFFull;
	EB_U64                  lowestIntraCost = 0xFFFFFFFFFFFFFFFFull;
	EB_U8                   lowestCostIndex = 0;
	PredictionUnit_t       *puPtr;
    EB_U32                   i;
	ModeDecisionCandidate_t       *candidatePtr;

	lowestCostIndex = bestCandidateIndexArray[0];

	// Find the candidate with the lowest cost
	for (i = 0; i < candidateTotalCount; ++i) {

		candidateIndex = bestCandidateIndexArray[i];
               
		// Compute fullCostBis 

           if (( *(bufferPtrArray[candidateIndex]->fullCostPtr)  < lowestIntraCost) && bufferPtrArray[candidateIndex]->candidatePtr->type == INTRA_MODE){
				*bestIntraMode = bufferPtrArray[candidateIndex]->candidatePtr->intraLumaMode;
				lowestIntraCost = *(bufferPtrArray[candidateIndex]->fullCostPtr);

			}

			if ( *(bufferPtrArray[candidateIndex]->fullCostPtr) < lowestCost){
				lowestCostIndex = candidateIndex;
				lowestCost = *(bufferPtrArray[candidateIndex]->fullCostPtr);
			}


	}

	candidatePtr    = bufferPtrArray[lowestCostIndex]->candidatePtr;

	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].cost = *(bufferPtrArray[lowestCostIndex]->fullCostPtr);
	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].cost      = (contextPtr->mdLocalCuUnit[cuPtr->leafIndex].cost - bufferPtrArray[lowestCostIndex]->candidatePtr->chromaDistortion) + bufferPtrArray[lowestCostIndex]->candidatePtr->chromaDistortionInterDepth;

    if(candidatePtr->type==INTRA_MODE)
		contextPtr->mdLocalCuUnit[cuPtr->leafIndex].costLuma = bufferPtrArray[lowestCostIndex]->fullCostLuma;
	contextPtr->mdEpPipeLcu[cuPtr->leafIndex].mergeCost = *bufferPtrArray[lowestCostIndex]->fullCostMergePtr;
	contextPtr->mdEpPipeLcu[cuPtr->leafIndex].skipCost = *bufferPtrArray[lowestCostIndex]->fullCostSkipPtr;

    if(candidatePtr->type == INTER_MODE && candidatePtr->mergeFlag == EB_TRUE){
        contextPtr->mdEpPipeLcu[cuPtr->leafIndex].chromaDistortion = bufferPtrArray[lowestCostIndex]->candidatePtr->chromaDistortion;
    }

	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].fullDistortion = bufferPtrArray[lowestCostIndex]->candidatePtr->fullDistortion;
	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].chromaDistortion = (EB_U32)bufferPtrArray[lowestCostIndex]->candidatePtr->chromaDistortion;
	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].chromaDistortionInterDepth = (EB_U32)bufferPtrArray[lowestCostIndex]->candidatePtr->chromaDistortionInterDepth;
	cuPtr->predictionModeFlag       = candidatePtr->type;
    cuPtr->skipFlag                 = candidatePtr->skipFlag; // note, the skip flag is re-checked in the ENCDEC process
    cuPtr->rootCbf                  = ((candidatePtr->rootCbf) > 0) ? EB_TRUE : EB_FALSE;

	contextPtr->mdLocalCuUnit[cuPtr->leafIndex].countNonZeroCoeffs       = candidatePtr->countNonZeroCoeffs;
	// Set the PU level variables

	{
		puPtr = cuPtr->predictionUnitArray;
		// Intra Prediction
		puPtr->intraLumaMode = 0x1F;
		if (cuPtr->predictionModeFlag == INTRA_MODE)
		{
			puPtr->intraLumaMode = candidatePtr->intraLumaMode;
		}

		// Inter Prediction
		puPtr->interPredDirectionIndex = candidatePtr->predictionDirection[0];
		puPtr->mergeFlag = candidatePtr->mergeFlag;
		if (cuPtr->predictionModeFlag != INTER_MODE)
		{
			puPtr->interPredDirectionIndex = 0x03;
			puPtr->mergeFlag = EB_FALSE;
		}
		puPtr->mergeIndex = candidatePtr->mergeIndex;
		puPtr->mv[REF_LIST_0].x = 0;
		puPtr->mv[REF_LIST_0].y = 0;

		puPtr->mv[REF_LIST_1].x = 0;
		puPtr->mv[REF_LIST_1].y = 0;

		if (puPtr->interPredDirectionIndex == UNI_PRED_LIST_0)
		{
			//EB_MEMCPY(&puPtr->mv[REF_LIST_0].x,&candidatePtr->MVsL0,4);
			puPtr->mv[REF_LIST_0].x = candidatePtr->motionVector_x_L0;
			puPtr->mv[REF_LIST_0].y = candidatePtr->motionVector_y_L0;
		}

		if (puPtr->interPredDirectionIndex == UNI_PRED_LIST_1)
		{
			//EB_MEMCPY(&puPtr->mv[REF_LIST_1].x,&candidatePtr->MVsL1,4);
			puPtr->mv[REF_LIST_1].x = candidatePtr->motionVector_x_L1;
			puPtr->mv[REF_LIST_1].y = candidatePtr->motionVector_y_L1;
		}

		if (puPtr->interPredDirectionIndex == BI_PRED)
		{
			//EB_MEMCPY(&puPtr->mv[REF_LIST_0].x,&candidatePtr->MVs,8);
			puPtr->mv[REF_LIST_0].x = candidatePtr->motionVector_x_L0;
			puPtr->mv[REF_LIST_0].y = candidatePtr->motionVector_y_L0;
			puPtr->mv[REF_LIST_1].x = candidatePtr->motionVector_x_L1;
			puPtr->mv[REF_LIST_1].y = candidatePtr->motionVector_y_L1;
		}

        // The MV prediction indicies are recalcated by the EncDec.
        puPtr->mvd[REF_LIST_0].predIdx = 0;
        puPtr->mvd[REF_LIST_1].predIdx = 0;

	}

    TransformUnit_t        *tuPtr;
	const TransformUnitStats_t   *tuStatPtr;
	EB_U32                  tuItr;
	EB_U32                  tuSize;
	EB_U32                  tuIndex;

	EB_U32                  parentTuIndex;

	EB_U32                  tuTotalCount;

    EB_U32  cuSizeLog2 = contextPtr->cuSizeLog2;


    if (cuSize == MAX_LCU_SIZE){
		tuTotalCount = 4;
		tuIndex = 1;
		tuItr = 0;
		tuPtr = &cuPtr->transformUnitArray[0];
		tuPtr->splitFlag = EB_TRUE;
		tuPtr->cbCbf = EB_FALSE;
		tuPtr->crCbf = EB_FALSE;

		// Set TU variables
        tuPtr->cbCbf2 = EB_FALSE;
        tuPtr->crCbf2 = EB_FALSE;
			tuPtr->chromaCbfContext = 0; //at TU level 
		}
		else {
			tuTotalCount = 1;
			tuIndex = 0;
			tuItr = 0;
	}

	//cuPtr->forceSmallTu = candidatePtr->forceSmallTu;

	// Set TU
	do {
		tuStatPtr = GetTransformUnitStats(tuIndex);
		tuSize = cuSize >> tuStatPtr->depth;
		tuPtr = &cuPtr->transformUnitArray[tuIndex];
		parentTuIndex = 0;
		if (tuStatPtr->depth > 0)
			parentTuIndex = tuIndexList[tuStatPtr->depth - 1][(tuItr >> 2)];

		tuPtr->splitFlag = EB_FALSE;
		tuPtr->lumaCbf = (EB_BOOL)(((candidatePtr->yCbf)  & (1 << tuIndex)) > 0);
		tuPtr->cbCbf = (EB_BOOL)(((candidatePtr->cbCbf) & (1 << (tuIndex))) > 0);
		tuPtr->crCbf = (EB_BOOL)(((candidatePtr->crCbf) & (1 << (tuIndex))) > 0);
		tuPtr->cbCbf2 = EB_FALSE;
		tuPtr->crCbf2 = EB_FALSE;
		
        //CHKN tuPtr->chromaCbfContext = (tuIndex == 0 || (cuPtr->partitionMode == SIZE_NxN)) ? 0 : (cuSizeLog2 - Log2f(tuSize)); //at TU level 
        tuPtr->chromaCbfContext = (tuIndex == 0 || (0)) ? 0 : (cuSizeLog2 - Log2f(tuSize)); //at TU level 
		
        tuPtr->lumaCbfContext = (cuSizeLog2 - Log2f(tuSize)) == 0 ? 1 : 0;

		if (tuPtr->cbCbf){
			cuPtr->transformUnitArray[0].cbCbf = EB_TRUE;
			cuPtr->transformUnitArray[parentTuIndex].cbCbf = EB_TRUE;
		}
		if (tuPtr->crCbf){
			cuPtr->transformUnitArray[0].crCbf = EB_TRUE;
			cuPtr->transformUnitArray[parentTuIndex].crCbf = EB_TRUE;
		}

		++tuItr;
		tuIndex = tuIndexList[tuStatPtr->depth][tuItr];

	} while (tuItr < tuTotalCount);

	return lowestCostIndex;
}






