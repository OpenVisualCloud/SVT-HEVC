/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"

#include "EbMotionEstimationResults.h"
#include "EbInitialRateControlProcess.h"
#include "EbInitialRateControlResults.h"
#include "EbMotionEstimationContext.h"
#include "EbUtility.h"
#include "EbReferenceObject.h"
#include "EbMotionEstimation.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

/**************************************
* Macros
**************************************/
#define PAN_LCU_PERCENTAGE                    75
#define LOW_AMPLITUDE_TH                      64


EB_BOOL CheckMvForPanHighAmp(
	EB_U32   hierarchicalLevels,
	EB_U32	 temporalLayerIndex,
	EB_S32	*xCurrentMv,
	EB_S32	*xCandidateMv)
{
	if (*xCurrentMv * *xCandidateMv		> 0						// both negative or both positives and both different than 0 i.e. same direction and non Stationary) 
		&& ABS(*xCurrentMv) >= GLOBAL_MOTION_THRESHOLD[hierarchicalLevels][temporalLayerIndex]	// high amplitude 
		&& ABS(*xCandidateMv) >= GLOBAL_MOTION_THRESHOLD[hierarchicalLevels][temporalLayerIndex]	// high amplitude 
		&& ABS(*xCurrentMv - *xCandidateMv) < LOW_AMPLITUDE_TH) {	// close amplitude

		return(EB_TRUE);
	}

	else {
		return(EB_FALSE);
	}

}

EB_BOOL CheckMvForTiltHighAmp(
	EB_U32   hierarchicalLevels,
	EB_U32	 temporalLayerIndex,
	EB_S32	*yCurrentMv,
	EB_S32	*yCandidateMv)
{
	if (*yCurrentMv * *yCandidateMv > 0						// both negative or both positives and both different than 0 i.e. same direction and non Stationary) 
		&& ABS(*yCurrentMv) >= GLOBAL_MOTION_THRESHOLD[hierarchicalLevels][temporalLayerIndex]	// high amplitude 
		&& ABS(*yCandidateMv) >= GLOBAL_MOTION_THRESHOLD[hierarchicalLevels][temporalLayerIndex]	// high amplitude 
		&& ABS(*yCurrentMv - *yCandidateMv) < LOW_AMPLITUDE_TH) {	// close amplitude

		return(EB_TRUE);
	}

	else {
		return(EB_FALSE);
	}

}

EB_BOOL CheckMvForPan(
	EB_U32   hierarchicalLevels,
	EB_U32	 temporalLayerIndex,
	EB_S32	*xCurrentMv,
	EB_S32	*yCurrentMv,
	EB_S32	*xCandidateMv,
	EB_S32	*yCandidateMv)
{
	if (*yCurrentMv < LOW_AMPLITUDE_TH
		&& *yCandidateMv < LOW_AMPLITUDE_TH
		&& *xCurrentMv * *xCandidateMv		> 0						// both negative or both positives and both different than 0 i.e. same direction and non Stationary) 
		&& ABS(*xCurrentMv) >= GLOBAL_MOTION_THRESHOLD[hierarchicalLevels][temporalLayerIndex]	// high amplitude 
		&& ABS(*xCandidateMv) >= GLOBAL_MOTION_THRESHOLD[hierarchicalLevels][temporalLayerIndex]	// high amplitude 
		&& ABS(*xCurrentMv - *xCandidateMv) < LOW_AMPLITUDE_TH) {	// close amplitude

		return(EB_TRUE);
	}

	else {
		return(EB_FALSE);
	}

}

EB_BOOL CheckMvForTilt(
	EB_U32   hierarchicalLevels,
	EB_U32	 temporalLayerIndex,
	EB_S32	*xCurrentMv,
	EB_S32	*yCurrentMv,
	EB_S32	*xCandidateMv,
	EB_S32	*yCandidateMv)
{
	if (*xCurrentMv < LOW_AMPLITUDE_TH
		&& *xCandidateMv < LOW_AMPLITUDE_TH
		&& *yCurrentMv * *yCandidateMv		> 0						// both negative or both positives and both different than 0 i.e. same direction and non Stationary) 
		&& ABS(*yCurrentMv) >= GLOBAL_MOTION_THRESHOLD[hierarchicalLevels][temporalLayerIndex]	// high amplitude 
		&& ABS(*yCandidateMv) >= GLOBAL_MOTION_THRESHOLD[hierarchicalLevels][temporalLayerIndex]	// high amplitude 
		&& ABS(*yCurrentMv - *yCandidateMv) < LOW_AMPLITUDE_TH) {	// close amplitude

		return(EB_TRUE);
	}

	else {
		return(EB_FALSE);
	}

}

EB_BOOL CheckMvForNonUniformMotion(
	EB_S32	*xCurrentMv,
	EB_S32	*yCurrentMv,
	EB_S32	*xCandidateMv,
	EB_S32	*yCandidateMv)
{
	EB_S32 mvThreshold = 40;//LOW_AMPLITUDE_TH + 18;
	// Either the x or the y direction is greater than threshold
	if ((ABS(*xCurrentMv - *xCandidateMv) > mvThreshold) || (ABS(*yCurrentMv - *yCandidateMv) > mvThreshold)) {
		return(EB_TRUE);
	}
	else {
		return(EB_FALSE);
	}

}

void CheckForNonUniformMotionVectorField(
    SequenceControlSet_t        *sequenceControlSetPtr,
	PictureParentControlSet_t	*pictureControlSetPtr)
{
	EB_U32	lcuIndex;
	EB_U32	pictureWidthInLcu = (pictureControlSetPtr->enhancedPicturePtr->width + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE;
	EB_U32	lcuOriginX;
	EB_U32	lcuOriginY;

	EB_S32	xCurrentMv = 0;
	EB_S32	yCurrentMv = 0;
	EB_S32	xLeftMv = 0;
	EB_S32	yLeftMv = 0;
	EB_S32	xTopMv = 0;
	EB_S32	yTopMv = 0;
	EB_S32	xRightMv = 0;
	EB_S32	yRightMv = 0;
	EB_S32	xBottomMv = 0;
	EB_S32	yBottomMv = 0;
	EB_U32 countOfNonUniformNeighbors = 0;

    for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

        LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

        lcuOriginX = lcuParams->originX;
        lcuOriginY = lcuParams->originY;

		countOfNonUniformNeighbors = 0;

        if (lcuParams->isCompleteLcu) {

			// Current MV   
			GetMv(pictureControlSetPtr, lcuIndex, &xCurrentMv, &yCurrentMv);

			// Left MV   
			if (lcuOriginX == 0) {
				xLeftMv = 0;
				yLeftMv = 0;
			}
			else {
				GetMv(pictureControlSetPtr, lcuIndex - 1, &xLeftMv, &yLeftMv);
			}

			countOfNonUniformNeighbors += CheckMvForNonUniformMotion(&xCurrentMv, &yCurrentMv, &xLeftMv, &yLeftMv);

			// Top MV   
			if (lcuOriginY == 0) {
				xTopMv = 0;
				yTopMv = 0;
			}
			else {
				GetMv(pictureControlSetPtr, lcuIndex - pictureWidthInLcu, &xTopMv, &yTopMv);
			}

			countOfNonUniformNeighbors += CheckMvForNonUniformMotion(&xCurrentMv, &yCurrentMv, &xTopMv, &yTopMv);

			// Right MV   
			if ((lcuOriginX + (MAX_LCU_SIZE << 1)) > pictureControlSetPtr->enhancedPicturePtr->width) {
				xRightMv = 0;
				yRightMv = 0;
			}
			else {
				GetMv(pictureControlSetPtr, lcuIndex + 1, &xRightMv, &yRightMv);
			}

			countOfNonUniformNeighbors += CheckMvForNonUniformMotion(&xCurrentMv, &yCurrentMv, &xRightMv, &yRightMv);

			// Bottom MV   
			if ((lcuOriginY + (MAX_LCU_SIZE << 1)) > pictureControlSetPtr->enhancedPicturePtr->height) {
				xBottomMv = 0;
				yBottomMv = 0;
			}
			else {
				GetMv(pictureControlSetPtr, lcuIndex + pictureWidthInLcu, &xBottomMv, &yBottomMv);
			}

			countOfNonUniformNeighbors += CheckMvForNonUniformMotion(&xCurrentMv, &yCurrentMv, &xBottomMv, &yBottomMv);
		}
	}
}


void DetectGlobalMotion(
    SequenceControlSet_t        *sequenceControlSetPtr,
	PictureParentControlSet_t	*pictureControlSetPtr)
{
	EB_U32	lcuIndex;
	EB_U32	pictureWidthInLcu = (pictureControlSetPtr->enhancedPicturePtr->width + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE;
	EB_U32	lcuOriginX;
	EB_U32	lcuOriginY;

	EB_U32  totalCheckedLcus = 0;
	EB_U32  totalPanLcus = 0;

	EB_S32	xCurrentMv = 0;
	EB_S32	yCurrentMv = 0;
	EB_S32	xLeftMv = 0;
	EB_S32	yLeftMv = 0;
	EB_S32	xTopMv = 0;
	EB_S32	yTopMv = 0;
	EB_S32	xRightMv = 0;
	EB_S32	yRightMv = 0;
	EB_S32	xBottomMv = 0;
	EB_S32	yBottomMv = 0;

	EB_S64  xTiltMvSum = 0;
	EB_S64  yTiltMvSum = 0;
	EB_U32  totalTiltLcus = 0;

	EB_U32  totalTiltHighAmpLcus = 0;
	EB_U32  totalPanHighAmpLcus = 0;

	for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

        LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

        lcuOriginX = lcuParams->originX;
        lcuOriginY = lcuParams->originY;

        if (lcuParams->isCompleteLcu) { 

			// Current MV   
			GetMv(pictureControlSetPtr, lcuIndex, &xCurrentMv, &yCurrentMv);

			// Left MV   
			if (lcuOriginX == 0) {
				xLeftMv = 0;
				yLeftMv = 0;
			}
			else {
				GetMv(pictureControlSetPtr, lcuIndex - 1, &xLeftMv, &yLeftMv);
			}

			// Top MV   
			if (lcuOriginY == 0) {
				xTopMv = 0;
				yTopMv = 0;
			}
			else {
				GetMv(pictureControlSetPtr, lcuIndex - pictureWidthInLcu, &xTopMv, &yTopMv);
			}

			// Right MV   
			if ((lcuOriginX + (MAX_LCU_SIZE << 1)) > pictureControlSetPtr->enhancedPicturePtr->width) {
				xRightMv = 0;
				yRightMv = 0;
			}
			else {
				GetMv(pictureControlSetPtr, lcuIndex + 1, &xRightMv, &yRightMv);
			}

			// Bottom MV   
			if ((lcuOriginY + (MAX_LCU_SIZE << 1)) > pictureControlSetPtr->enhancedPicturePtr->height) {
				xBottomMv = 0;
				yBottomMv = 0;
			}
			else {
				GetMv(pictureControlSetPtr, lcuIndex + pictureWidthInLcu, &xBottomMv, &yBottomMv);
			}

			totalCheckedLcus++;

			if ((EB_BOOL)(CheckMvForPan(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &yCurrentMv, &xLeftMv, &yLeftMv)   ||
				          CheckMvForPan(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &yCurrentMv, &xTopMv, &yTopMv)     ||
				          CheckMvForPan(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &yCurrentMv, &xRightMv, &yRightMv) ||
				          CheckMvForPan(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &yCurrentMv, &xBottomMv, &yBottomMv))) {

				totalPanLcus++;


			}

			if ((EB_BOOL)(CheckMvForTilt(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &yCurrentMv, &xLeftMv, &yLeftMv)   ||
				          CheckMvForTilt(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &yCurrentMv, &xTopMv, &yTopMv)     ||
				          CheckMvForTilt(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &yCurrentMv, &xRightMv, &yRightMv) ||
				          CheckMvForTilt(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &yCurrentMv, &xBottomMv, &yBottomMv))) {


				totalTiltLcus++;

				xTiltMvSum += xCurrentMv;
				yTiltMvSum += yCurrentMv;
			}

			if ((EB_BOOL)(CheckMvForPanHighAmp(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &xLeftMv)   ||
				          CheckMvForPanHighAmp(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &xTopMv)    ||
				          CheckMvForPanHighAmp(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &xRightMv)  ||
				          CheckMvForPanHighAmp(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &xCurrentMv, &xBottomMv))) {

				totalPanHighAmpLcus++;


			}

			if ((EB_BOOL)(CheckMvForTiltHighAmp(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &yCurrentMv, &yLeftMv)  ||
				          CheckMvForTiltHighAmp(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &yCurrentMv, &yTopMv)   ||
				          CheckMvForTiltHighAmp(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &yCurrentMv, &yRightMv) ||
				          CheckMvForTiltHighAmp(pictureControlSetPtr->hierarchicalLevels, pictureControlSetPtr->temporalLayerIndex, &yCurrentMv, &yBottomMv))) {


				totalTiltHighAmpLcus++;

			}

		}
	}
	pictureControlSetPtr->isPan = EB_FALSE;
	pictureControlSetPtr->isTilt = EB_FALSE;

    if (totalCheckedLcus > 0) {
        // If more than PAN_LCU_PERCENTAGE % of LCUs are PAN
        if ((totalPanLcus * 100 / totalCheckedLcus) > PAN_LCU_PERCENTAGE) {
            pictureControlSetPtr->isPan = EB_TRUE;
        }
        if ((totalTiltLcus * 100 / totalCheckedLcus) > PAN_LCU_PERCENTAGE) {
            pictureControlSetPtr->isTilt = EB_TRUE;
        }
    }
}

/************************************************
* Initial Rate Control Context Constructor
************************************************/
EB_ERRORTYPE InitialRateControlContextCtor(
	InitialRateControlContext_t **contextDblPtr,
	EbFifo_t                     *motionEstimationResultsInputFifoPtr,
	EbFifo_t                     *initialrateControlResultsOutputFifoPtr)
{
	InitialRateControlContext_t *contextPtr;
	EB_MALLOC(InitialRateControlContext_t*, contextPtr, sizeof(InitialRateControlContext_t), EB_N_PTR);
	*contextDblPtr = contextPtr;
	contextPtr->motionEstimationResultsInputFifoPtr = motionEstimationResultsInputFifoPtr;
	contextPtr->initialrateControlResultsOutputFifoPtr = initialrateControlResultsOutputFifoPtr;

	return EB_ErrorNone;
}

/************************************************
* Release Pa Reference Objects
** Check if reference pictures are needed
** release them when appropriate
************************************************/
void ReleasePaReferenceObjects(
	PictureParentControlSet_t         *pictureControlSetPtr)
{
	// PA Reference Pictures
	EB_U32                             numOfListToSearch;
	EB_U32                             listIndex;
	if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {

		numOfListToSearch = (pictureControlSetPtr->sliceType == EB_P_PICTURE) ? REF_LIST_0 : REF_LIST_1;

		// List Loop 
		for (listIndex = REF_LIST_0; listIndex <= numOfListToSearch; ++listIndex) {

				// Release PA Reference Pictures
				if (pictureControlSetPtr->refPaPicPtrArray[listIndex] != EB_NULL) {

                    EbReleaseObject(((EbPaReferenceObject_t*)pictureControlSetPtr->refPaPicPtrArray[listIndex]->objectPtr)->pPcsPtr->pPcsWrapperPtr);
					EbReleaseObject(pictureControlSetPtr->refPaPicPtrArray[listIndex]);
				}
		}
	}

	if (pictureControlSetPtr->paReferencePictureWrapperPtr != EB_NULL) {
  
        EbReleaseObject(pictureControlSetPtr->pPcsWrapperPtr);
		EbReleaseObject(pictureControlSetPtr->paReferencePictureWrapperPtr);
	}
    
	return;
}

/************************************************
* Global Motion Detection Based on ME information
** Mark pictures for pan
** Mark pictures for tilt
** No lookahead information used in this function
************************************************/
void MeBasedGlobalMotionDetection(
    SequenceControlSet_t              *sequenceControlSetPtr,
	PictureParentControlSet_t         *pictureControlSetPtr)
{
	// PAN Generation
	pictureControlSetPtr->isPan                 = EB_FALSE;
	pictureControlSetPtr->isTilt                = EB_FALSE;

	if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {
        DetectGlobalMotion(
            sequenceControlSetPtr,
            pictureControlSetPtr);
	}

	return;
}


void StationaryEdgeCountLcu(
    SequenceControlSet_t        *sequenceControlSetPtr,
    PictureParentControlSet_t   *pictureControlSetPtr,
    PictureParentControlSet_t   *temporalPictureControlSetPtr,
    EB_U32                       totalLcuCount)
{

    EB_U32               lcuIndex;
    for (lcuIndex = 0; lcuIndex < totalLcuCount; lcuIndex++) {

        LcuParams_t *lcuParams  = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
        LcuStat_t   *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

        if (lcuParams->potentialLogoLcu && lcuParams->isCompleteLcu && lcuStatPtr->check1ForLogoStationaryEdgeOverTimeFlag && lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag){

            LcuStat_t *tempLcuStatPtr = &temporalPictureControlSetPtr->lcuStatArray[lcuIndex];
            EB_U32 rasterScanCuIndex;

            if (tempLcuStatPtr->check1ForLogoStationaryEdgeOverTimeFlag)
            {
                for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_16x16_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_16x16_15; rasterScanCuIndex++) {
                    lcuStatPtr->cuStatArray[rasterScanCuIndex].similarEdgeCount += tempLcuStatPtr->cuStatArray[rasterScanCuIndex].edgeCu;
                }
            }
        }

        if (lcuParams->potentialLogoLcu && lcuParams->isCompleteLcu && lcuStatPtr->pmCheck1ForLogoStationaryEdgeOverTimeFlag && lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag){

            LcuStat_t *tempLcuStatPtr = &temporalPictureControlSetPtr->lcuStatArray[lcuIndex];
            EB_U32 rasterScanCuIndex;

            if (tempLcuStatPtr->pmCheck1ForLogoStationaryEdgeOverTimeFlag)
            {
                for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_16x16_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_16x16_15; rasterScanCuIndex++) {
                    lcuStatPtr->cuStatArray[rasterScanCuIndex].pmSimilarEdgeCount += tempLcuStatPtr->cuStatArray[rasterScanCuIndex].edgeCu;
                }
            }
        }
    }
}

/************************************************
* Global Motion Detection Based on Lookahead
** Mark pictures for pan
** Mark pictures for tilt
** LAD Window: min (8 or sliding window size)
************************************************/
void UpdateGlobalMotionDetectionOverTime(
	EncodeContext_t                   *encodeContextPtr,
	SequenceControlSet_t              *sequenceControlSetPtr,
	PictureParentControlSet_t         *pictureControlSetPtr)
{

	InitialRateControlReorderEntry_t   *temporaryQueueEntryPtr;
	PictureParentControlSet_t          *temporaryPictureControlSetPtr;

	EB_U32								totalPanPictures = 0;
	EB_U32								totalCheckedPictures = 0;
	EB_U32								totalTiltPictures = 0;
	EB_U32                              totalTMVPEnableDetectedPictures = 0;
	EB_U32							    updateIsPanFramesToCheck;
	EB_U32								inputQueueIndex;
	EB_U32								framesToCheckIndex;


	(void) sequenceControlSetPtr;

	// Determine number of frames to check (8 frames)
	updateIsPanFramesToCheck = MIN(8, pictureControlSetPtr->framesInSw);

	// Walk the first N entries in the sliding window
	inputQueueIndex = encodeContextPtr->initialRateControlReorderQueueHeadIndex;
	EB_U32 updateFramesToCheck = updateIsPanFramesToCheck;
	for (framesToCheckIndex = 0; framesToCheckIndex < updateFramesToCheck; framesToCheckIndex++) {

		temporaryQueueEntryPtr = encodeContextPtr->initialRateControlReorderQueue[inputQueueIndex];
		temporaryPictureControlSetPtr = ((PictureParentControlSet_t*)(temporaryQueueEntryPtr->parentPcsWrapperPtr)->objectPtr);

		if (temporaryPictureControlSetPtr->sliceType != EB_I_PICTURE) {

			totalPanPictures += (temporaryPictureControlSetPtr->isPan == EB_TRUE);

			totalTiltPictures += (temporaryPictureControlSetPtr->isTilt == EB_TRUE);

			// Keep track of checked pictures
			totalCheckedPictures++;
		}

		// Increment the inputQueueIndex Iterator
		inputQueueIndex = (inputQueueIndex == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : inputQueueIndex + 1;

	}

	pictureControlSetPtr->isPan                 = EB_FALSE;
	pictureControlSetPtr->isTilt                = EB_FALSE;

	if (totalCheckedPictures) {
		if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {
			// If more than 75% of checked pictures are PAN then current picture is PAN
			if ((((pictureControlSetPtr->hierarchicalLevels == 3)) ||
				((pictureControlSetPtr->hierarchicalLevels == 4) && pictureControlSetPtr->temporalLayerIndex > 1) ||
				((pictureControlSetPtr->hierarchicalLevels == 5) && pictureControlSetPtr->temporalLayerIndex > 2))){

				const EB_U32 HOMOGENIOUS_MOTION_TH[6] = { 20, 40, 50, 55, 60, 60 };
				if ((totalTMVPEnableDetectedPictures * 100 / totalCheckedPictures) > HOMOGENIOUS_MOTION_TH[pictureControlSetPtr->temporalLayerIndex]) {
					pictureControlSetPtr->disableTmvpFlag = EB_FALSE;
				}
			}

			if ((totalPanPictures * 100 / totalCheckedPictures) > 75) {
				pictureControlSetPtr->isPan = EB_TRUE;
			}
		}

	}
	return;
}

/************************************************
* Update BEA Information Based on Lookahead
** Average zzCost of Collocated LCU throughout lookahead frames
** Set isMostOfPictureNonMoving based on number of non moving LCUs
** LAD Window: min (2xmgpos+1 or sliding window size)
************************************************/

void UpdateBeaInfoOverTime(
	EncodeContext_t                   *encodeContextPtr,
	PictureParentControlSet_t         *pictureControlSetPtr)
{
    InitialRateControlReorderEntry_t   *temporaryQueueEntryPtr;
    PictureParentControlSet_t          *temporaryPictureControlSetPtr;
    EB_U32							    updateNonMovingIndexArrayFramesToCheck;
    EB_U16                              lcuIdx;
    EB_U16								framesToCheckIndex;
    EB_U32								inputQueueIndex;


    SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
    // Update motionIndexArray of the current picture by averaging the motionIndexArray of the N future pictures
    // Determine number of frames to check N
    updateNonMovingIndexArrayFramesToCheck = MIN(MIN(((pictureControlSetPtr->predStructPtr->predStructPeriod << 1) + 1), pictureControlSetPtr->framesInSw), sequenceControlSetPtr->staticConfig.lookAheadDistance);

    // LCU Loop
    for (lcuIdx = 0; lcuIdx < pictureControlSetPtr->lcuTotalCount; ++lcuIdx) {

         EB_U32 zzCostOverSlidingWindow         = pictureControlSetPtr->zzCostArray[lcuIdx];
         EB_U16 nonMovingIndexOverSlidingWindow = pictureControlSetPtr->nonMovingIndexArray[lcuIdx];

        // Walk the first N entries in the sliding window starting picture + 1
        inputQueueIndex = (encodeContextPtr->initialRateControlReorderQueueHeadIndex == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->initialRateControlReorderQueueHeadIndex + 1;
        for (framesToCheckIndex = 0; framesToCheckIndex < updateNonMovingIndexArrayFramesToCheck - 1; framesToCheckIndex++) {


            temporaryQueueEntryPtr = encodeContextPtr->initialRateControlReorderQueue[inputQueueIndex];
            temporaryPictureControlSetPtr = ((PictureParentControlSet_t*)(temporaryQueueEntryPtr->parentPcsWrapperPtr)->objectPtr);


            if (temporaryPictureControlSetPtr->sliceType == EB_I_PICTURE || temporaryPictureControlSetPtr->endOfSequenceFlag) {
                break;
            }

            zzCostOverSlidingWindow         += temporaryPictureControlSetPtr->zzCostArray[lcuIdx];
            nonMovingIndexOverSlidingWindow += temporaryPictureControlSetPtr->nonMovingIndexArray[lcuIdx];

            // Increment the inputQueueIndex Iterator
            inputQueueIndex = (inputQueueIndex == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : inputQueueIndex + 1;
        }

        pictureControlSetPtr->zzCostArray[lcuIdx]           = (EB_U8) (zzCostOverSlidingWindow / (framesToCheckIndex + 1));
        pictureControlSetPtr->nonMovingIndexArray[lcuIdx]   = (EB_U8) (nonMovingIndexOverSlidingWindow / (framesToCheckIndex + 1));
    }

    return;
}

/****************************************
* Init ZZ Cost array to default values
** Used when no Lookahead is available
****************************************/
void InitZzCostInfo(
	PictureParentControlSet_t         *pictureControlSetPtr)
{
    EB_U16 lcuIdx;
	// LCU loop
	for (lcuIdx = 0; lcuIdx < pictureControlSetPtr->lcuTotalCount; ++lcuIdx) {
		pictureControlSetPtr->zzCostArray[lcuIdx] = INVALID_ZZ_COST;

	}
	pictureControlSetPtr->nonMovingIndexAverage = INVALID_ZZ_COST;

	// LCU Loop
	for (lcuIdx = 0; lcuIdx < pictureControlSetPtr->lcuTotalCount; ++lcuIdx) {
		pictureControlSetPtr->nonMovingIndexArray[lcuIdx] = INVALID_ZZ_COST;
	}

	return;
}

/************************************************
* Update uniform motion field
** Update Uniformly moving LCUs using
** collocated LCUs infor in lookahead pictures
** LAD Window: min (2xmgpos+1 or sliding window size)
************************************************/
void UpdateMotionFieldUniformityOverTime(
	EncodeContext_t                   *encodeContextPtr,
    SequenceControlSet_t              *sequenceControlSetPtr,
	PictureParentControlSet_t         *pictureControlSetPtr)
{
	InitialRateControlReorderEntry_t   *temporaryQueueEntryPtr;
	PictureParentControlSet_t          *temporaryPictureControlSetPtr;
	EB_U32								inputQueueIndex;
	EB_U32                              noFramesToCheck;
	EB_U32								framesToCheckIndex;
	//SVT_LOG("To update POC %d\tframesInSw = %d\n", pictureControlSetPtr->pictureNumber, pictureControlSetPtr->framesInSw);

	// Determine number of frames to check N
    noFramesToCheck = MIN(MIN(((pictureControlSetPtr->predStructPtr->predStructPeriod << 1) + 1), pictureControlSetPtr->framesInSw), sequenceControlSetPtr->staticConfig.lookAheadDistance);

	// Walk the first N entries in the sliding window starting picture + 1
	inputQueueIndex = (encodeContextPtr->initialRateControlReorderQueueHeadIndex == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->initialRateControlReorderQueueHeadIndex;
	for (framesToCheckIndex = 0; framesToCheckIndex < noFramesToCheck - 1; framesToCheckIndex++) {


		temporaryQueueEntryPtr = encodeContextPtr->initialRateControlReorderQueue[inputQueueIndex];
		temporaryPictureControlSetPtr = ((PictureParentControlSet_t*)(temporaryQueueEntryPtr->parentPcsWrapperPtr)->objectPtr);

		if (temporaryPictureControlSetPtr->endOfSequenceFlag) {
			break;
		}
        // The values are calculated for every 4th frame
        if ((temporaryPictureControlSetPtr->pictureNumber & 3) == 0){
            StationaryEdgeCountLcu(
                sequenceControlSetPtr,
                pictureControlSetPtr,
                temporaryPictureControlSetPtr,
                pictureControlSetPtr->lcuTotalCount);
        }
		// Increment the inputQueueIndex Iterator
		inputQueueIndex = (inputQueueIndex == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : inputQueueIndex + 1;

	}

	return;
}

/************************************************
* Update uniform motion field
** Update Uniformly moving LCUs using
** collocated LCUs infor in lookahead pictures
** LAD Window: min (2xmgpos+1 or sliding window size)
************************************************/
void UpdateHomogeneityOverTime(
	EncodeContext_t                   *encodeContextPtr,
	PictureParentControlSet_t         *pictureControlSetPtr)
{
	InitialRateControlReorderEntry_t   *temporaryQueueEntryPtr;
	PictureParentControlSet_t          *temporaryPictureControlSetPtr;
	EB_U32                              noFramesToCheck;

	EB_U16                             *variancePtr;

	EB_U64                              meanSqrvariance64x64Based = 0, meanvariance64x64Based = 0;
	EB_U16                              countOfHomogeneousOverTime = 0;
	EB_U32								inputQueueIndex;
	EB_U32								framesToCheckIndex;
	EB_U32                              lcuIdx;

    SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;

	pictureControlSetPtr->picHomogenousOverTimeLcuPercentage = 0;

	// LCU Loop
	for (lcuIdx = 0; lcuIdx < pictureControlSetPtr->lcuTotalCount; ++lcuIdx) {

		meanSqrvariance64x64Based = 0;
		meanvariance64x64Based = 0;

		// Initialize 
		pictureControlSetPtr->lcuVarianceOfVarianceOverTime[lcuIdx] = 0xFFFFFFFFFFFFFFFF;

		pictureControlSetPtr->isLcuHomogeneousOverTime[lcuIdx] = EB_FALSE;

		// Update motionIndexArray of the current picture by averaging the motionIndexArray of the N future pictures
		// Determine number of frames to check N
        noFramesToCheck = MIN(MIN(((pictureControlSetPtr->predStructPtr->predStructPeriod << 1) + 1), pictureControlSetPtr->framesInSw), sequenceControlSetPtr->staticConfig.lookAheadDistance);

		// Walk the first N entries in the sliding window starting picture + 1
		inputQueueIndex = (encodeContextPtr->initialRateControlReorderQueueHeadIndex == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->initialRateControlReorderQueueHeadIndex;
		for (framesToCheckIndex = 0; framesToCheckIndex < noFramesToCheck - 1; framesToCheckIndex++) {


			temporaryQueueEntryPtr = encodeContextPtr->initialRateControlReorderQueue[inputQueueIndex];
			temporaryPictureControlSetPtr = ((PictureParentControlSet_t*)(temporaryQueueEntryPtr->parentPcsWrapperPtr)->objectPtr);
			if (temporaryPictureControlSetPtr->sceneChangeFlag || temporaryPictureControlSetPtr->endOfSequenceFlag) {

				break;
			}

			variancePtr = temporaryPictureControlSetPtr->variance[lcuIdx];

			meanSqrvariance64x64Based += (variancePtr[ME_TIER_ZERO_PU_64x64])*(variancePtr[ME_TIER_ZERO_PU_64x64]);
			meanvariance64x64Based += (variancePtr[ME_TIER_ZERO_PU_64x64]);

			// Increment the inputQueueIndex Iterator
			inputQueueIndex = (inputQueueIndex == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : inputQueueIndex + 1;

		} // PCS loop

		meanSqrvariance64x64Based = meanSqrvariance64x64Based / (noFramesToCheck - 1);
		meanvariance64x64Based = meanvariance64x64Based / (noFramesToCheck - 1);

		// Compute variance
		pictureControlSetPtr->lcuVarianceOfVarianceOverTime[lcuIdx] = meanSqrvariance64x64Based - meanvariance64x64Based * meanvariance64x64Based;

		if (pictureControlSetPtr->lcuVarianceOfVarianceOverTime[lcuIdx] <= (VAR_BASED_DETAIL_PRESERVATION_SELECTOR_THRSLHD)) {
			pictureControlSetPtr->isLcuHomogeneousOverTime[lcuIdx] = EB_TRUE;
		}

		countOfHomogeneousOverTime += pictureControlSetPtr->isLcuHomogeneousOverTime[lcuIdx];


	} // LCU loop


	pictureControlSetPtr->picHomogenousOverTimeLcuPercentage = (EB_U8)(countOfHomogeneousOverTime * 100 / pictureControlSetPtr->lcuTotalCount);

	return;
}

void ResetHomogeneityStructures(
	PictureParentControlSet_t         *pictureControlSetPtr)
{
	EB_U32                              lcuIdx;

	pictureControlSetPtr->picHomogenousOverTimeLcuPercentage = 0;

	// Reset the structure 
	for (lcuIdx = 0; lcuIdx < pictureControlSetPtr->lcuTotalCount; ++lcuIdx) {
		pictureControlSetPtr->lcuVarianceOfVarianceOverTime[lcuIdx] = 0xFFFFFFFFFFFFFFFF;
		pictureControlSetPtr->isLcuHomogeneousOverTime[lcuIdx] = EB_FALSE;
	}

	return;
}

InitialRateControlReorderEntry_t  * DeterminePictureOffsetInQueue(
	EncodeContext_t                   *encodeContextPtr,
	PictureParentControlSet_t         *pictureControlSetPtr,
	MotionEstimationResults_t         *inputResultsPtr)
{

	InitialRateControlReorderEntry_t  *queueEntryPtr;
	EB_S32                             queueEntryIndex;

	queueEntryIndex = (EB_S32)(pictureControlSetPtr->pictureNumber - encodeContextPtr->initialRateControlReorderQueue[encodeContextPtr->initialRateControlReorderQueueHeadIndex]->pictureNumber);
	queueEntryIndex += encodeContextPtr->initialRateControlReorderQueueHeadIndex;
	queueEntryIndex = (queueEntryIndex > INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? queueEntryIndex - INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH : queueEntryIndex;
	queueEntryPtr = encodeContextPtr->initialRateControlReorderQueue[queueEntryIndex];
	queueEntryPtr->parentPcsWrapperPtr = inputResultsPtr->pictureControlSetWrapperPtr;
	queueEntryPtr->pictureNumber = pictureControlSetPtr->pictureNumber;



	return queueEntryPtr;
}

void GetHistogramQueueData(
	SequenceControlSet_t              *sequenceControlSetPtr,
	EncodeContext_t                   *encodeContextPtr,
	PictureParentControlSet_t         *pictureControlSetPtr)
{
	HlRateControlHistogramEntry_t     *histogramQueueEntryPtr;
	EB_S32                             histogramQueueEntryIndex;

	// Determine offset from the Head Ptr for HLRC histogram queue 
	EbBlockOnMutex(sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueMutex);
	histogramQueueEntryIndex = (EB_S32)(pictureControlSetPtr->pictureNumber - encodeContextPtr->hlRateControlHistorgramQueue[encodeContextPtr->hlRateControlHistorgramQueueHeadIndex]->pictureNumber);
	histogramQueueEntryIndex += encodeContextPtr->hlRateControlHistorgramQueueHeadIndex;
	histogramQueueEntryIndex = (histogramQueueEntryIndex > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
		histogramQueueEntryIndex - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
		histogramQueueEntryIndex;
	histogramQueueEntryPtr = encodeContextPtr->hlRateControlHistorgramQueue[histogramQueueEntryIndex];


	//histogramQueueEntryPtr->parentPcsWrapperPtr  = inputResultsPtr->pictureControlSetWrapperPtr;
	histogramQueueEntryPtr->pictureNumber = pictureControlSetPtr->pictureNumber;
	histogramQueueEntryPtr->endOfSequenceFlag = pictureControlSetPtr->endOfSequenceFlag;
	histogramQueueEntryPtr->sliceType = pictureControlSetPtr->sliceType;
	histogramQueueEntryPtr->temporalLayerIndex = pictureControlSetPtr->temporalLayerIndex;
	histogramQueueEntryPtr->fullLcuCount = pictureControlSetPtr->fullLcuCount;
	histogramQueueEntryPtr->lifeCount = 0;
	histogramQueueEntryPtr->passedToHlrc = EB_FALSE;
	histogramQueueEntryPtr->isCoded = EB_FALSE;
	histogramQueueEntryPtr->totalNumBitsCoded = 0;
    EB_MEMCPY(
        histogramQueueEntryPtr->meDistortionHistogram,
        pictureControlSetPtr->meDistortionHistogram,
        sizeof(EB_U16) * NUMBER_OF_SAD_INTERVALS);

    EB_MEMCPY(
        histogramQueueEntryPtr->oisDistortionHistogram,
        pictureControlSetPtr->oisDistortionHistogram,
        sizeof(EB_U16) * NUMBER_OF_INTRA_SAD_INTERVALS);

	EbReleaseMutex(sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueMutex);
	//SVT_LOG("Test1 POC: %d\t POC: %d\t LifeCount: %d\n", histogramQueueEntryPtr->pictureNumber, pictureControlSetPtr->pictureNumber,  histogramQueueEntryPtr->lifeCount);


	return;

}

void UpdateHistogramQueueEntry(
	SequenceControlSet_t              *sequenceControlSetPtr,
	EncodeContext_t                   *encodeContextPtr,
	PictureParentControlSet_t         *pictureControlSetPtr)
{

	HlRateControlHistogramEntry_t     *histogramQueueEntryPtr;
	EB_S32                             histogramQueueEntryIndex;

	EbBlockOnMutex(sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueMutex);

	histogramQueueEntryIndex = (EB_S32)(pictureControlSetPtr->pictureNumber - encodeContextPtr->hlRateControlHistorgramQueue[encodeContextPtr->hlRateControlHistorgramQueueHeadIndex]->pictureNumber);
	histogramQueueEntryIndex += encodeContextPtr->hlRateControlHistorgramQueueHeadIndex;
	histogramQueueEntryIndex = (histogramQueueEntryIndex > HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH - 1) ?
		histogramQueueEntryIndex - HIGH_LEVEL_RATE_CONTROL_HISTOGRAM_QUEUE_MAX_DEPTH :
		histogramQueueEntryIndex;
    CHECK_REPORT_ERROR(histogramQueueEntryIndex >= 0, encodeContextPtr->appCallbackPtr, EB_ENC_RC_ERROR8);
	histogramQueueEntryPtr = encodeContextPtr->hlRateControlHistorgramQueue[histogramQueueEntryIndex];
	histogramQueueEntryPtr->lifeCount += pictureControlSetPtr->historgramLifeCount;
	histogramQueueEntryPtr->passedToHlrc = EB_TRUE;

	EbReleaseMutex(sequenceControlSetPtr->encodeContextPtr->hlRateControlHistorgramQueueMutex);

	return;

}

EB_AURA_STATUS AuraDetection64x64(
	PictureControlSet_t           *pictureControlSetPtr,
	EB_U8                          pictureQp,
	EB_U32                         xLcuIndex,
	EB_U32                         yLcuIndex
	);

/************************************************
* Initial Rate Control Kernel
* The Initial Rate Control Process determines the initial bit budget for each
* picture depending on the data gathered in the Picture Analysis and Motion
* Analysis processes as well as the settings determined in the Picture Decision process.
* The Initial Rate Control process also employs a sliding window buffer to analyze multiple
* pictures if the delay is allowed.  Note that through this process, until the subsequent
* Picture Manager process, no reference picture data has been used.
* P.S. Temporal noise reduction is now performed in Initial Rate Control Process.
* In future we might decide to move it to Motion Analysis Process.
************************************************/
void* InitialRateControlKernel(void *inputPtr)
{
	InitialRateControlContext_t       *contextPtr = (InitialRateControlContext_t*)inputPtr;
	PictureParentControlSet_t         *pictureControlSetPtr;
	PictureParentControlSet_t         *pictureControlSetPtrTemp;
	EncodeContext_t                   *encodeContextPtr;
	SequenceControlSet_t              *sequenceControlSetPtr;

	EbObjectWrapper_t                 *inputResultsWrapperPtr;
	MotionEstimationResults_t         *inputResultsPtr;

	EbObjectWrapper_t                 *outputResultsWrapperPtr;
	InitialRateControlResults_t       *outputResultsPtr;

	// Queue variables
	EB_U32                             queueEntryIndexTemp;
	EB_U32                             queueEntryIndexTemp2;
	InitialRateControlReorderEntry_t  *queueEntryPtr;

	EB_BOOL                            moveSlideWondowFlag = EB_TRUE;
	EB_BOOL                            endOfSequenceFlag = EB_TRUE;
	EB_U8                               framesInSw;
    EB_U8                               temporalLayerIndex;
	EbObjectWrapper_t                  *referencePictureWrapperPtr;

	EbObjectWrapper_t                *outputStreamWrapperPtr;

	for (;;) {

		// Get Input Full Object
		EbGetFullObject(
			contextPtr->motionEstimationResultsInputFifoPtr,
			&inputResultsWrapperPtr);
        EB_CHECK_END_OBJ(inputResultsWrapperPtr);

		inputResultsPtr = (MotionEstimationResults_t*)inputResultsWrapperPtr->objectPtr;
		pictureControlSetPtr = (PictureParentControlSet_t*)inputResultsPtr->pictureControlSetWrapperPtr->objectPtr;
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld IRC IN \n", pictureControlSetPtr->pictureNumber);
#endif
        pictureControlSetPtr->meSegmentsCompletionMask++;
        if (pictureControlSetPtr->meSegmentsCompletionMask == pictureControlSetPtr->meSegmentsTotalCount) {
			sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
			encodeContextPtr = (EncodeContext_t*)sequenceControlSetPtr->encodeContextPtr;

            // Mark picture when global motion is detected using ME results
            //reset intraCodedEstimationLcu
            MeBasedGlobalMotionDetection(
                sequenceControlSetPtr,
                pictureControlSetPtr);

			// Release Pa Ref pictures when not needed 
			ReleasePaReferenceObjects(
				pictureControlSetPtr);

			//****************************************************
			// Input Motion Analysis Results into Reordering Queue
			//****************************************************

			// Determine offset from the Head Ptr          
			queueEntryPtr = DeterminePictureOffsetInQueue(
				encodeContextPtr,
				pictureControlSetPtr,
				inputResultsPtr);

			if (sequenceControlSetPtr->staticConfig.rateControlMode)
			{
				if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0){

					// Getting the Histogram Queue Data 
					GetHistogramQueueData(
						sequenceControlSetPtr,
						encodeContextPtr,
						pictureControlSetPtr);
				}
			}

			for (temporalLayerIndex = 0; temporalLayerIndex < EB_MAX_TEMPORAL_LAYERS; temporalLayerIndex++){
				pictureControlSetPtr->framesInInterval[temporalLayerIndex] = 0;
			}

			pictureControlSetPtr->framesInSw                 = 0;
			pictureControlSetPtr->historgramLifeCount        = 0;
            pictureControlSetPtr->sceneChangeInGop = EB_FALSE;
 
			moveSlideWondowFlag = EB_TRUE;
			while (moveSlideWondowFlag){

				// Check if the sliding window condition is valid
				queueEntryIndexTemp = encodeContextPtr->initialRateControlReorderQueueHeadIndex;
				if (encodeContextPtr->initialRateControlReorderQueue[queueEntryIndexTemp]->parentPcsWrapperPtr != EB_NULL){
					endOfSequenceFlag = (((PictureParentControlSet_t*)(encodeContextPtr->initialRateControlReorderQueue[queueEntryIndexTemp]->parentPcsWrapperPtr)->objectPtr))->endOfSequenceFlag;
				}
				else{
					endOfSequenceFlag = EB_FALSE;
				}
				framesInSw = 0;
				while (moveSlideWondowFlag && !endOfSequenceFlag &&
					queueEntryIndexTemp <= encodeContextPtr->initialRateControlReorderQueueHeadIndex + sequenceControlSetPtr->staticConfig.lookAheadDistance){
					// framesInSw <= sequenceControlSetPtr->staticConfig.lookAheadDistance){

					framesInSw++;

					queueEntryIndexTemp2 = (queueEntryIndexTemp > INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? queueEntryIndexTemp - INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH : queueEntryIndexTemp;

					moveSlideWondowFlag = (EB_BOOL)(moveSlideWondowFlag && (encodeContextPtr->initialRateControlReorderQueue[queueEntryIndexTemp2]->parentPcsWrapperPtr != EB_NULL));
					if (encodeContextPtr->initialRateControlReorderQueue[queueEntryIndexTemp2]->parentPcsWrapperPtr != EB_NULL){
						// check if it is the last frame. If we have reached the last frame, we would output the buffered frames in the Queue.
						endOfSequenceFlag = ((PictureParentControlSet_t*)(encodeContextPtr->initialRateControlReorderQueue[queueEntryIndexTemp2]->parentPcsWrapperPtr)->objectPtr)->endOfSequenceFlag;
					}
					else{
						endOfSequenceFlag = EB_FALSE;
					}
					queueEntryIndexTemp++;
				}


				if (moveSlideWondowFlag)  {

					//get a new entry spot
					queueEntryPtr = encodeContextPtr->initialRateControlReorderQueue[encodeContextPtr->initialRateControlReorderQueueHeadIndex];
					pictureControlSetPtr = ((PictureParentControlSet_t*)(queueEntryPtr->parentPcsWrapperPtr)->objectPtr);
					sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
					pictureControlSetPtr->framesInSw = framesInSw;
					queueEntryIndexTemp = encodeContextPtr->initialRateControlReorderQueueHeadIndex;
					endOfSequenceFlag = EB_FALSE;
					// find the framesInInterval for the peroid I frames
					while (!endOfSequenceFlag &&
						queueEntryIndexTemp <= encodeContextPtr->initialRateControlReorderQueueHeadIndex + sequenceControlSetPtr->staticConfig.lookAheadDistance){

						queueEntryIndexTemp2 = (queueEntryIndexTemp > INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? queueEntryIndexTemp - INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH : queueEntryIndexTemp;
						pictureControlSetPtrTemp = ((PictureParentControlSet_t*)(encodeContextPtr->initialRateControlReorderQueue[queueEntryIndexTemp2]->parentPcsWrapperPtr)->objectPtr);
						if (sequenceControlSetPtr->intraPeriodLength != -1){
							if (pictureControlSetPtr->pictureNumber % ((sequenceControlSetPtr->intraPeriodLength + 1)) == 0){
								pictureControlSetPtr->framesInInterval[pictureControlSetPtrTemp->temporalLayerIndex] ++;
								if (pictureControlSetPtrTemp->sceneChangeFlag)
									pictureControlSetPtr->sceneChangeInGop = EB_TRUE;
							}
						}

						pictureControlSetPtrTemp->historgramLifeCount++;
						endOfSequenceFlag = pictureControlSetPtrTemp->endOfSequenceFlag;
						queueEntryIndexTemp++;
					}

					

					if ((sequenceControlSetPtr->staticConfig.lookAheadDistance != 0) && (framesInSw < (sequenceControlSetPtr->staticConfig.lookAheadDistance + 1)))
						pictureControlSetPtr->endOfSequenceRegion = EB_TRUE;
					else
						pictureControlSetPtr->endOfSequenceRegion = EB_FALSE;

					if (sequenceControlSetPtr->staticConfig.rateControlMode)
					{
						// Determine offset from the Head Ptr for HLRC histogram queue and set the life count
						if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0){

							// Update Histogram Queue Entry Life count
							UpdateHistogramQueueEntry(
								sequenceControlSetPtr,
								encodeContextPtr,
								pictureControlSetPtr);
						}
					}

					// Mark each input picture as PAN or not
					// If a lookahead is present then check PAN for a period of time
					if (!pictureControlSetPtr->endOfSequenceFlag && sequenceControlSetPtr->staticConfig.lookAheadDistance != 0) {

						// Check for Pan,Tilt, Zoom and other global motion detectors over the future pictures in the lookahead
						UpdateGlobalMotionDetectionOverTime(
							encodeContextPtr,
							sequenceControlSetPtr,
							pictureControlSetPtr);
					}
					else {
						if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {
							DetectGlobalMotion(
                                sequenceControlSetPtr, 
                                pictureControlSetPtr);
						}
					}

					// BACKGROUND ENHANCEMENT PART II 
					if (!pictureControlSetPtr->endOfSequenceFlag && sequenceControlSetPtr->staticConfig.lookAheadDistance != 0) {
						// Update BEA information based on Lookahead information
						UpdateBeaInfoOverTime(
							encodeContextPtr,
							pictureControlSetPtr);

					}
					else {
						// Reset zzCost information to default When there's no lookahead available
						InitZzCostInfo(
							pictureControlSetPtr);
					}

					// Use the temporal layer 0 isLcuMotionFieldNonUniform array for all the other layer pictures in the mini GOP
					if (!pictureControlSetPtr->endOfSequenceFlag && sequenceControlSetPtr->staticConfig.lookAheadDistance != 0) {

						// Updat uniformly moving LCUs based on Collocated LCUs in LookAhead window
						UpdateMotionFieldUniformityOverTime(
							encodeContextPtr,
                            sequenceControlSetPtr,
							pictureControlSetPtr);
					}

					if (!pictureControlSetPtr->endOfSequenceFlag && sequenceControlSetPtr->staticConfig.lookAheadDistance != 0) {
						// Compute and store variance of LCus over time and determine homogenuity temporally
						UpdateHomogeneityOverTime(
							encodeContextPtr,
							pictureControlSetPtr);
					}
					else {
						// Reset Homogeneity Structures to default if no lookahead is detected
						ResetHomogeneityStructures(
							pictureControlSetPtr);
					}

					// Get Empty Reference Picture Object
					EbGetEmptyObject(
						sequenceControlSetPtr->encodeContextPtr->referencePicturePoolFifoPtr,
						&referencePictureWrapperPtr);
					((PictureParentControlSet_t*)(queueEntryPtr->parentPcsWrapperPtr->objectPtr))->referencePictureWrapperPtr = referencePictureWrapperPtr;

					// Give the new Reference a nominal liveCount of 1
					EbObjectIncLiveCount(
						((PictureParentControlSet_t*)(queueEntryPtr->parentPcsWrapperPtr->objectPtr))->referencePictureWrapperPtr,
						1);
					//OPTION 1:  get the buffer in resource coordination

					EbGetEmptyObject(
						sequenceControlSetPtr->encodeContextPtr->streamOutputFifoPtr,
						&outputStreamWrapperPtr);

					pictureControlSetPtr->outputStreamWrapperPtr = outputStreamWrapperPtr;					

                    // Get Empty Results Object
					EbGetEmptyObject(
						contextPtr->initialrateControlResultsOutputFifoPtr,
						&outputResultsWrapperPtr);

					outputResultsPtr = (InitialRateControlResults_t*)outputResultsWrapperPtr->objectPtr;
					outputResultsPtr->pictureControlSetWrapperPtr = queueEntryPtr->parentPcsWrapperPtr;
					/////////////////////////////
					// Post the Full Results Object
					EbPostFullObject(outputResultsWrapperPtr);
#if LATENCY_PROFILE
        double latency = 0.0;
        EB_U64 finishTimeSeconds = 0;
        EB_U64 finishTimeuSeconds = 0;
        EbFinishTime((uint64_t*)&finishTimeSeconds, (uint64_t*)&finishTimeuSeconds);

        EbComputeOverallElapsedTimeMs(
                pictureControlSetPtr->startTimeSeconds,
                pictureControlSetPtr->startTimeuSeconds,
                finishTimeSeconds,
                finishTimeuSeconds,
                &latency);

        SVT_LOG("[%lld]: POC %lld IRC OUT, decoder order %d, latency %3.3f \n",
                EbGetSysTimeMs(),
                pictureControlSetPtr->pictureNumber,
                pictureControlSetPtr->decodeOrder,
                latency);
#endif

					// Reset the Reorder Queue Entry
					queueEntryPtr->pictureNumber += INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH;
					queueEntryPtr->parentPcsWrapperPtr = (EbObjectWrapper_t *)EB_NULL;

					// Increment the Reorder Queue head Ptr
					encodeContextPtr->initialRateControlReorderQueueHeadIndex =
						(encodeContextPtr->initialRateControlReorderQueueHeadIndex == INITIAL_RATE_CONTROL_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->initialRateControlReorderQueueHeadIndex + 1;

					queueEntryPtr = encodeContextPtr->initialRateControlReorderQueue[encodeContextPtr->initialRateControlReorderQueueHeadIndex];
				}
			}
		}
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld IRC OUT \n", pictureControlSetPtr->pictureNumber);
#endif
		// Release the Input Results
		EbReleaseObject(inputResultsWrapperPtr);

	}
	return EB_NULL;
}

