/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"

#include "EbPictureAnalysisResults.h"
#include "EbPictureDecisionProcess.h"
#include "EbPictureDecisionResults.h"
#include "EbReferenceObject.h"
#include "EbComputeSAD.h"
#include "EbMeSadCalculation.h"

#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

/************************************************
 * Defines
 ************************************************/
#define POC_CIRCULAR_ADD(base, offset/*, bits*/)             (/*(((EB_S32) (base)) + ((EB_S32) (offset)) > ((EB_S32) (1 << (bits))))   ? ((base) + (offset) - (1 << (bits))) : \
                                                             (((EB_S32) (base)) + ((EB_S32) (offset)) < 0)                           ? ((base) + (offset) + (1 << (bits))) : \
                                                                                                                                       */((base) + (offset)))
#define FUTURE_WINDOW_WIDTH                 4
#define FLASH_TH                            5
#define FADE_TH                             3
#define SCENE_TH                            3000
#define NOISY_SCENE_TH                      4500	// SCD TH in presence of noise
#define HIGH_PICTURE_VARIANCE_TH			1500
#define NUM64x64INPIC(w,h)          ((w*h)>> (LOG2F(MAX_LCU_SIZE)<<1))
#define QUEUE_GET_PREVIOUS_SPOT(h)  ((h == 0) ? PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH - 1 : h - 1)
#define QUEUE_GET_NEXT_SPOT(h,off)  (( (h+off) >= PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH) ? h+off - PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH  : h + off)

/************************************************
 * Picture Analysis Context Constructor
 ************************************************/
EB_ERRORTYPE PictureDecisionContextCtor(
    PictureDecisionContext_t **contextDblPtr,
    EbFifo_t *pictureAnalysisResultsInputFifoPtr,
    EbFifo_t *pictureDecisionResultsOutputFifoPtr)
{
    PictureDecisionContext_t *contextPtr;
    EB_U32 arrayIndex;
    EB_U32 arrayRow , arrowColumn;
    EB_MALLOC(PictureDecisionContext_t*, contextPtr, sizeof(PictureDecisionContext_t), EB_N_PTR);
    *contextDblPtr = contextPtr;

    contextPtr->pictureAnalysisResultsInputFifoPtr  = pictureAnalysisResultsInputFifoPtr;
    contextPtr->pictureDecisionResultsOutputFifoPtr = pictureDecisionResultsOutputFifoPtr;
    
	EB_MALLOC(EB_U32**, contextPtr->ahdRunningAvgCb, sizeof(EB_U32*) * MAX_NUMBER_OF_REGIONS_IN_WIDTH, EB_N_PTR);

	EB_MALLOC(EB_U32**, contextPtr->ahdRunningAvgCr, sizeof(EB_U32*) * MAX_NUMBER_OF_REGIONS_IN_WIDTH, EB_N_PTR);

	EB_MALLOC(EB_U32**, contextPtr->ahdRunningAvg, sizeof(EB_U32*) * MAX_NUMBER_OF_REGIONS_IN_WIDTH, EB_N_PTR);

	for (arrayIndex = 0; arrayIndex < MAX_NUMBER_OF_REGIONS_IN_WIDTH; arrayIndex++)
	{
		EB_MALLOC(EB_U32*, contextPtr->ahdRunningAvgCb[arrayIndex], sizeof(EB_U32) * MAX_NUMBER_OF_REGIONS_IN_HEIGHT, EB_N_PTR);

		EB_MALLOC(EB_U32*, contextPtr->ahdRunningAvgCr[arrayIndex], sizeof(EB_U32) * MAX_NUMBER_OF_REGIONS_IN_HEIGHT, EB_N_PTR);

		EB_MALLOC(EB_U32*, contextPtr->ahdRunningAvg[arrayIndex], sizeof(EB_U32) * MAX_NUMBER_OF_REGIONS_IN_HEIGHT, EB_N_PTR);
	}

	for (arrayRow = 0; arrayRow < MAX_NUMBER_OF_REGIONS_IN_HEIGHT; arrayRow++)
	{
		for (arrowColumn = 0; arrowColumn < MAX_NUMBER_OF_REGIONS_IN_WIDTH; arrowColumn++) {
			contextPtr->ahdRunningAvgCb[arrowColumn][arrayRow] = 0;
			contextPtr->ahdRunningAvgCr[arrowColumn][arrayRow] = 0;
			contextPtr->ahdRunningAvg[arrowColumn][arrayRow] = 0;
		}
	}


    contextPtr->resetRunningAvg = EB_TRUE;

	contextPtr->isSceneChangeDetected = EB_FALSE;


    return EB_ErrorNone;
}

static EB_BOOL SceneTransitionDetector(
    PictureDecisionContext_t *contextPtr,
	SequenceControlSet_t				 *sequenceControlSetPtr,
	PictureParentControlSet_t           **ParentPcsWindow,
	EB_U32                                windowWidthFuture)
{
	PictureParentControlSet_t       *previousPictureControlSetPtr = ParentPcsWindow[0];
	PictureParentControlSet_t       *currentPictureControlSetPtr = ParentPcsWindow[1];
	PictureParentControlSet_t       *futurePictureControlSetPtr = ParentPcsWindow[2];

	// calculating the frame threshold based on the number of 64x64 blocks in the frame
	EB_U32  regionThreshHold;
	EB_U32  regionThreshHoldChroma;
	// this variable determines whether the running average should be reset to equal the ahd or not after detecting a scene change.
    //EB_BOOL resetRunningAvg = contextPtr->resetRunningAvg;

	EB_BOOL isAbruptChange; // this variable signals an abrubt change (scene change or flash)
	EB_BOOL isSceneChange; // this variable signals a frame representing a scene change
	EB_BOOL isFlash; // this variable signals a frame that contains a flash
	EB_BOOL isFade; // this variable signals a frame that contains a fade
	EB_BOOL gradualChange; // this signals the detection of a light scene change a small/localized flash or the start of a fade

	EB_U32  ahd; // accumulative histogram (absolute) differences between the past and current frame

	EB_U32  ahdCb;
	EB_U32  ahdCr;

	EB_U32  ahdErrorCb = 0;
	EB_U32  ahdErrorCr = 0;

    EB_U32 **ahdRunningAvgCb = contextPtr->ahdRunningAvgCb;
    EB_U32 **ahdRunningAvgCr = contextPtr->ahdRunningAvgCr;
    EB_U32 **ahdRunningAvg = contextPtr->ahdRunningAvg;
	
	EB_U32  ahdError = 0; // the difference between the ahd and the running average at the current frame.

	EB_U8   aidFuturePast = 0; // this variable denotes the average intensity difference between the next and the past frames
	EB_U8   aidFuturePresent = 0;
	EB_U8   aidPresentPast = 0;

	EB_U32  bin = 0; // variable used to iterate through the bins of the histograms

	EB_U32  regionInPictureWidthIndex;
	EB_U32  regionInPictureHeightIndex;

	EB_U32  regionWidth;
	EB_U32  regionHeight;
	EB_U32  regionWidthOffset;
	EB_U32  regionHeightOffset;

	EB_U32  isAbruptChangeCount = 0;
	EB_U32  isSceneChangeCount = 0;

	EB_U32  regionCountThreshold = (sequenceControlSetPtr->scdMode == SCD_MODE_2) ?
		(EB_U32)(((float)((sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth * sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight) * 75) / 100) + 0.5) :
		(EB_U32)(((float)((sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth * sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight) * 50) / 100) + 0.5) ;
	
	regionWidth = ParentPcsWindow[1]->enhancedPicturePtr->width / sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth;
	regionHeight = ParentPcsWindow[1]->enhancedPicturePtr->height / sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight;
	
	// Loop over regions inside the picture
	for (regionInPictureWidthIndex = 0; regionInPictureWidthIndex < sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth; regionInPictureWidthIndex++){  // loop over horizontal regions
		for (regionInPictureHeightIndex = 0; regionInPictureHeightIndex < sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight; regionInPictureHeightIndex++){ // loop over vertical regions

            isAbruptChange = EB_FALSE; 
            isSceneChange = EB_FALSE;
            isFlash = EB_FALSE;
            gradualChange = EB_FALSE; 
			
			// Reset accumulative histogram (absolute) differences between the past and current frame
			ahd = 0;
			ahdCb = 0;
			ahdCr = 0;

			regionWidthOffset = (regionInPictureWidthIndex == sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth - 1) ?
				ParentPcsWindow[1]->enhancedPicturePtr->width - (sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth * regionWidth) :
				0;

			regionHeightOffset = (regionInPictureHeightIndex == sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight - 1) ?
				ParentPcsWindow[1]->enhancedPicturePtr->height - (sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight * regionHeight) :
				0;

			regionWidth += regionWidthOffset;
			regionHeight += regionHeightOffset;

			regionThreshHold = (
				// Noise insertion/removal detection
				((ABS((EB_S64)currentPictureControlSetPtr->picAvgVariance - (EB_S64)previousPictureControlSetPtr->picAvgVariance)) > NOISE_VARIANCE_TH) &&
				(currentPictureControlSetPtr->picAvgVariance > HIGH_PICTURE_VARIANCE_TH || previousPictureControlSetPtr->picAvgVariance > HIGH_PICTURE_VARIANCE_TH)) ? 
				NOISY_SCENE_TH * NUM64x64INPIC(regionWidth, regionHeight) : // SCD TH function of noise insertion/removal.
				SCENE_TH * NUM64x64INPIC(regionWidth, regionHeight) ;

			regionThreshHoldChroma = regionThreshHold / 4;

			for (bin = 0; bin < HISTOGRAM_NUMBER_OF_BINS; ++bin) {
				ahd += ABS((EB_S32)currentPictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][0][bin] - (EB_S32)previousPictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][0][bin]);
				ahdCb += ABS((EB_S32)currentPictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][1][bin] - (EB_S32)previousPictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][1][bin]);
				ahdCr += ABS((EB_S32)currentPictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][2][bin] - (EB_S32)previousPictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][2][bin]);

			}

			if (contextPtr->resetRunningAvg){
				ahdRunningAvg[regionInPictureWidthIndex][regionInPictureHeightIndex] = ahd;
				ahdRunningAvgCb[regionInPictureWidthIndex][regionInPictureHeightIndex] = ahdCb;
				ahdRunningAvgCr[regionInPictureWidthIndex][regionInPictureHeightIndex] = ahdCr;
			}

			ahdError = ABS((EB_S32)ahdRunningAvg[regionInPictureWidthIndex][regionInPictureHeightIndex] - (EB_S32)ahd);
			ahdErrorCb = ABS((EB_S32)ahdRunningAvgCb[regionInPictureWidthIndex][regionInPictureHeightIndex] - (EB_S32)ahdCb);
			ahdErrorCr = ABS((EB_S32)ahdRunningAvgCr[regionInPictureWidthIndex][regionInPictureHeightIndex] - (EB_S32)ahdCr);


			if ((ahdError   > regionThreshHold       && ahd >= ahdError) ||
				(ahdErrorCb > regionThreshHoldChroma && ahdCb >= ahdErrorCb) ||
				(ahdErrorCr > regionThreshHoldChroma && ahdCr >= ahdErrorCr)){

				isAbruptChange = EB_TRUE;

			}
			else if ((ahdError > (regionThreshHold >> 1)) && ahd >= ahdError){
				gradualChange = EB_TRUE;
			}

			if (isAbruptChange)
			{
				aidFuturePast = (EB_U8) ABS((EB_S16)futurePictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][0] - (EB_S16)previousPictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][0]);
				aidFuturePresent = (EB_U8)ABS((EB_S16)futurePictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][0] - (EB_S16)currentPictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][0]);
				aidPresentPast = (EB_U8)ABS((EB_S16)currentPictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][0] - (EB_S16)previousPictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][0]);

                if (aidFuturePast < FLASH_TH && aidFuturePresent >= FLASH_TH && aidPresentPast >= FLASH_TH){
					isFlash = EB_TRUE;
					//SVT_LOG ("\nFlash in frame# %i , %i\n", currentPictureControlSetPtr->pictureNumber,aidFuturePast);
				}
				else if (aidFuturePresent < FADE_TH && aidPresentPast < FADE_TH){
					isFade = EB_TRUE;
					//SVT_LOG ("\nFlash in frame# %i , %i\n", currentPictureControlSetPtr->pictureNumber,aidFuturePast);
				} else {
					isSceneChange = EB_TRUE;
					//SVT_LOG ("\nScene Change in frame# %i , %i\n", currentPictureControlSetPtr->pictureNumber,aidFuturePast);
				}

			}
			else if (gradualChange){

				aidFuturePast = (EB_U8) ABS((EB_S16)futurePictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][0] - (EB_S16)previousPictureControlSetPtr->averageIntensityPerRegion[regionInPictureWidthIndex][regionInPictureHeightIndex][0]);
				if (aidFuturePast < FLASH_TH){
					// proper action to be signalled
					//SVT_LOG ("\nLight Flash in frame# %i , %i\n", currentPictureControlSetPtr->pictureNumber,aidFuturePast);
					ahdRunningAvg[regionInPictureWidthIndex][regionInPictureHeightIndex] = (3 * ahdRunningAvg[regionInPictureWidthIndex][regionInPictureHeightIndex] + ahd) / 4;
				}
				else{
					// proper action to be signalled
					//SVT_LOG ("\nLight Scene Change / fade detected in frame# %i , %i\n", currentPictureControlSetPtr->pictureNumber,aidFuturePast);
					ahdRunningAvg[regionInPictureWidthIndex][regionInPictureHeightIndex] = (3 * ahdRunningAvg[regionInPictureWidthIndex][regionInPictureHeightIndex] + ahd) / 4;
				}

			}
			else{
				ahdRunningAvg[regionInPictureWidthIndex][regionInPictureHeightIndex] = (3 * ahdRunningAvg[regionInPictureWidthIndex][regionInPictureHeightIndex] + ahd) / 4;
			}

			isAbruptChangeCount += isAbruptChange;
			isSceneChangeCount += isSceneChange;
		}
	}

	(void)windowWidthFuture;
	(void)isFlash;
	(void)isFade;

	if (isAbruptChangeCount >= regionCountThreshold) {
		contextPtr->resetRunningAvg = EB_TRUE;
	}
	else {
		contextPtr->resetRunningAvg = EB_FALSE;
	}

    if ((isSceneChangeCount >= regionCountThreshold)){

		return(EB_TRUE);
	} 
	else {
		return(EB_FALSE);
	}
	
}

/***************************************************************************************************
* ReleasePrevPictureFromReorderQueue
***************************************************************************************************/
EB_ERRORTYPE ReleasePrevPictureFromReorderQueue(
	EncodeContext_t                 *encodeContextPtr) {

	EB_ERRORTYPE return_error = EB_ErrorNone;

	PictureDecisionReorderEntry_t   *queuePreviousEntryPtr;
	EB_S32                           previousEntryIndex;

	
	// Get the previous entry from the Picture Decision Reordering Queue (Entry N-1)
	// P.S. The previous entry in display order is needed for Scene Change Detection
	previousEntryIndex = (encodeContextPtr->pictureDecisionReorderQueueHeadIndex == 0) ? PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH - 1 : encodeContextPtr->pictureDecisionReorderQueueHeadIndex - 1;
	queuePreviousEntryPtr = encodeContextPtr->pictureDecisionReorderQueue[previousEntryIndex];

	// LCU activity classification based on (0,0) SAD & picture activity derivation 
	if (queuePreviousEntryPtr->parentPcsWrapperPtr) {

		// Reset the Picture Decision Reordering Queue Entry
		// P.S. The reset of the Picture Decision Reordering Queue Entry could not be done before running the Scene Change Detector 
		queuePreviousEntryPtr->pictureNumber += PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH;
		queuePreviousEntryPtr->parentPcsWrapperPtr = (EbObjectWrapper_t *)EB_NULL;
	}

	return return_error;
}


/***************************************************************************************************
* Generates mini GOP RPSs
*
*   
***************************************************************************************************/
EB_ERRORTYPE GenerateMiniGopRps(
	PictureDecisionContext_t        *contextPtr,
	EncodeContext_t                 *encodeContextPtr) {

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_U32						 miniGopIndex;
	PictureParentControlSet_t	*pictureControlSetPtr;
	EB_U32						 pictureIndex;

	SequenceControlSet_t       *sequenceControlSetPtr;


	// Loop over all mini GOPs
	for (miniGopIndex = 0; miniGopIndex < contextPtr->totalNumberOfMiniGops; ++miniGopIndex) {

		// Loop over picture within the mini GOP
		for (pictureIndex = contextPtr->miniGopStartIndex[miniGopIndex]; pictureIndex <= contextPtr->miniGopEndIndex[miniGopIndex]; pictureIndex++) {

			pictureControlSetPtr	= (PictureParentControlSet_t*)	encodeContextPtr->preAssignmentBuffer[pictureIndex]->objectPtr;
			sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
			pictureControlSetPtr->predStructure = sequenceControlSetPtr->staticConfig.predStructure;

            pictureControlSetPtr->hierarchicalLevels = (EB_U8)contextPtr->miniGopHierarchicalLevels[miniGopIndex];

            pictureControlSetPtr->predStructPtr = GetPredictionStructure(
				encodeContextPtr->predictionStructureGroupPtr,
				pictureControlSetPtr->predStructure,
				1,
				pictureControlSetPtr->hierarchicalLevels);
		}
	}
	return return_error;
}

/***************************************************************************
* Set the default subPel enble/disable flag for each frame
****************************************************************************/

EB_U8 PictureLevelSubPelSettingsOq(
	EB_U8   inputResolution,
	EB_U8   encMode,
	EB_U8   temporalLayerIndex,
	EB_BOOL isUsedAsReferenceFlag) {

	EB_U8 subPelMode;
   
	if (encMode <= ENC_MODE_8) {
		subPelMode = 1;
	}
	else if (encMode <= ENC_MODE_9) {
		if (inputResolution >= INPUT_SIZE_4K_RANGE) {
			subPelMode = (temporalLayerIndex == 0) ? 1 : 0;
		}
		else {
			subPelMode = 1;
		}
    }
    else {
		if (inputResolution >= INPUT_SIZE_4K_RANGE) {
            if (encMode > ENC_MODE_10) {
                subPelMode = 0;
            }
            else {
                subPelMode = (temporalLayerIndex == 0) ? 1 : 0;
            }
		}
		else {
			subPelMode = isUsedAsReferenceFlag ? 1 : 0;
		}
    }

	return subPelMode;
}


/******************************************************
* Derive Multi-Processes Settings for OQ
Input   : encoder mode and tune
Output  : Multi-Processes signal(s)
******************************************************/
EB_ERRORTYPE SignalDerivationMultiProcessesOq(
    SequenceControlSet_t        *sequenceControlSetPtr,
    PictureParentControlSet_t   *pictureControlSetPtr ) {

    EB_ERRORTYPE return_error = EB_ErrorNone;

    // Set MD Partitioning Method
	if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			pictureControlSetPtr->depthMode = PICT_FULL84_DEPTH_MODE;
		}
		else {
			pictureControlSetPtr->depthMode = PICT_FULL85_DEPTH_MODE;
		}
	}
    else if (pictureControlSetPtr->encMode <= ENC_MODE_10) {
        if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            pictureControlSetPtr->depthMode = PICT_FULL84_DEPTH_MODE;
        }
        else {
            pictureControlSetPtr->depthMode = PICT_LCU_SWITCH_DEPTH_MODE;
        }
    }

    else {
        if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            pictureControlSetPtr->depthMode = PICT_BDP_DEPTH_MODE;
        }
        else {
            pictureControlSetPtr->depthMode = PICT_LCU_SWITCH_DEPTH_MODE;
        }
    }
    
    // Set the default settings of  subpel
    pictureControlSetPtr->useSubpelFlag = PictureLevelSubPelSettingsOq(
        sequenceControlSetPtr->inputResolution,
        pictureControlSetPtr->encMode,
        pictureControlSetPtr->temporalLayerIndex,
        pictureControlSetPtr->isUsedAsReferenceFlag);

    // Limit OIS to DC
    if (pictureControlSetPtr->encMode <= ENC_MODE_9) {
        pictureControlSetPtr->limitOisToDcModeFlag = EB_FALSE;
    }
    else {
        pictureControlSetPtr->limitOisToDcModeFlag = (pictureControlSetPtr->sliceType != EB_I_PICTURE) ? EB_TRUE : EB_FALSE;
    }

    // CU_8x8 Search Mode
    if (pictureControlSetPtr->encMode <= ENC_MODE_2 ) {
        pictureControlSetPtr->cu8x8Mode = CU_8x8_MODE_0;
    }
	else if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
		pictureControlSetPtr->cu8x8Mode = (pictureControlSetPtr->isUsedAsReferenceFlag) ? CU_8x8_MODE_0 : CU_8x8_MODE_1;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			pictureControlSetPtr->cu8x8Mode = (pictureControlSetPtr->temporalLayerIndex == 0) ? CU_8x8_MODE_0 : CU_8x8_MODE_1;
		}
		else {
			pictureControlSetPtr->cu8x8Mode = (pictureControlSetPtr->isUsedAsReferenceFlag) ? CU_8x8_MODE_0 : CU_8x8_MODE_1;
		}
	}
    else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			pictureControlSetPtr->cu8x8Mode = CU_8x8_MODE_1;
		}
		else {
			pictureControlSetPtr->cu8x8Mode = (pictureControlSetPtr->temporalLayerIndex == 0) ? CU_8x8_MODE_0 : CU_8x8_MODE_1;
		}
    }
	else {
		pictureControlSetPtr->cu8x8Mode = CU_8x8_MODE_1;
    }
   
    // CU_16x16 Search Mode
	  pictureControlSetPtr->cu16x16Mode = CU_16x16_MODE_0;

    // Set Skip OIS 8x8 Flag
    pictureControlSetPtr->skipOis8x8 = (pictureControlSetPtr->sliceType == EB_I_PICTURE && (pictureControlSetPtr->encMode <= ENC_MODE_10)) ? EB_FALSE : EB_TRUE;

    return return_error;
}


/***************************************************************************************************
 * Picture Decision Kernel
 *                                                                                                        
 * Notes on the Picture Decision:                                                                                                       
 *                                                                                                        
 * The Picture Decision process performs multi-picture level decisions, including setting of the prediction structure, 
 * setting the picture type and scene change detection.
 *
 * Inputs:
 * Input Picture                                                                                                       
 *   -Input Picture Data                                                                                
 *                                                                                                                         
 *  Outputs:                                                                                                      
 *   -Picture Control Set with fully available PA Reference List                                                                                              
 *                                                                                                                                                                                                                                                                                       
 *  For Low Delay Sequences, pictures are started into the encoder pipeline immediately.                                                                                                      
 *                                                                                                        
 *  For Random Access Sequences, pictures are held for up to a PredictionStructurePeriod
 *    in order to determine if a Scene Change or Intra Frame is forthcoming. Either of 
 *    those events (and additionally a End of Sequence Flag) will change the expected 
 *    prediction structure.
 *
 *  Below is an example worksheet for how Intra Flags and Scene Change Flags interact 
 *    together to affect the prediction structure.
 *
 *  The base prediction structure for this example is a 3-Level Hierarchical Random Access, 
 *    Single Reference Prediction Structure:                                                                                                        
 *                                                                                                        
 *        b   b                                                                                                      
 *       / \ / \                                                                                                     
 *      /   B   \                                                                                                    
 *     /   / \   \                                                                                                     
 *    I-----------B                                                                                                    
 *                                                                                                        
 *  From this base structure, the following RPS positions are derived:                                                                                                   
 *                                                                                                        
 *    p   p       b   b       p   p                                                                                                    
 *     \   \     / \ / \     /   /                                                                                                     
 *      P   \   /   B   \   /   P                                                                                                      
 *       \   \ /   / \   \ /   /                                                                                                       
 *        ----I-----------B----                                                                                                        
 *                                                                                                        
 *    L L L   I  [ Normal ]   T T T                                                                                              
 *    2 1 0   n               0 1 2                                                                           
 *            t                                                                                            
 *            r                                                                                            
 *            a                                                                                            
 *                                                                                                        
 *  The RPS is composed of Leading Picture [L2-L0], Intra (CRA), Base/Normal Pictures,
 *    and Trailing Pictures [T0-T2]. Generally speaking, Leading Pictures are useful
 *    for handling scene changes without adding extraneous I-pictures and the Trailing 
 *    pictures are useful for terminating GOPs.
 *
 *  Here is a table of possible combinations of pictures needed to handle intra and
 *    scene changes happening in quick succession.
 *                                                                                                     
 *        Distance to scene change ------------>                                                                                                      
 *                                                                                                        
 *                  0              1                 2                3+                                                                                            
 *   I                                                                                                                      
 *   n                                                                                                                     
 *   t   0        I   I           n/a               n/a              n/a                                                                      
 *   r                                                                                                                     
 *   a              p              p                                                                                             
 *                   \            /                                                                                              
 *   P   1        I   I          I   I              n/a              n/a                                                                               
 *   e                                                                                                             
 *   r               p                               p                                                                                                
 *   i                \                             /                                                                                                 
 *   o            p    \         p   p             /   p                                                                                           
 *   d             \    \       /     \           /   /                                                                                          
 *       2     I    -----I     I       I         I----    I          n/a
 *   |                                                                                               
 *   |            p   p           p   p            p   p            p   p                                                                                           
 *   |             \   \         /     \          /     \          /   /                                                                                       
 *   |              P   \       /   p   \        /   p   \        /   P                                                                                        
 *   |               \   \     /     \   \      /   /     \      /   /                                                                                         
 *   V   3+   I       ----I   I       ----I    I----       I    I----       I                                                                                 
 *                                                                                                
 *   The table is interpreted as follows:                                                                                             
 *                                                                                                
 *   If there are no SCs or Intras encountered for a PredPeriod, then the normal                                                                                             
 *     prediction structure is applied.                                                                                          
 *                                                                                              
 *   If there is an intra in the PredPeriod, then one of the above combinations of
 *     Leading and Trailing pictures is used.  If there is no scene change, the last
 *     valid column consisting of Trailing Pictures only is used.  However, if there
 *     is an upcoming scene change before the next intra, then one of the above patterns 
 *     is used. In the case of End of Sequence flags, only the last valid column of Trailing
 *     Pictures is used. The intention here is that any combination of Intra Flag and Scene
 *     Change flag can be coded.                                                                                            
 *                                                                                              
 ***************************************************************************************************/
void* PictureDecisionKernel(void *inputPtr)
{
    PictureDecisionContext_t        *contextPtr = (PictureDecisionContext_t*) inputPtr;     

    PictureParentControlSet_t       *pictureControlSetPtr;  

    EncodeContext_t                 *encodeContextPtr;
    SequenceControlSet_t            *sequenceControlSetPtr;
    
    EbObjectWrapper_t               *inputResultsWrapperPtr;
    PictureAnalysisResults_t        *inputResultsPtr;
    
    EbObjectWrapper_t               *outputResultsWrapperPtr;
    PictureDecisionResults_t        *outputResultsPtr;

    PredictionStructureEntry_t      *predPositionPtr;

    EB_BOOL                          preAssignmentBufferFirstPassFlag;
    EB_PICTURE                         pictureType;
    
    PictureDecisionReorderEntry_t   *queueEntryPtr; 
    EB_S32                           queueEntryIndex;

    EB_S32                           previousEntryIndex;  

    PaReferenceQueueEntry_t         *inputEntryPtr;
    EB_U32                           inputQueueIndex;

    PaReferenceQueueEntry_t         *paReferenceEntryPtr;
    EB_U32                           paReferenceQueueIndex;  

    EB_U64                           refPoc;

    EB_U32                           depIdx;
    EB_U64                           depPoc;

    EB_U32                           depListCount;

	// Dynamic GOP
	EB_U32                           miniGopIndex;
	EB_U32                           pictureIndex;

	EB_BOOL                          windowAvail,framePasseThru;
    EB_U32                           windowIndex;
    EB_U32                           entryIndex;
    PictureParentControlSet_t        *ParentPcsWindow[FUTURE_WINDOW_WIDTH+2];

    // Debug
    EB_U64                           loopCount = 0;
    
    for(;;) {
    
        // Get Input Full Object
        EbGetFullObject(
            contextPtr->pictureAnalysisResultsInputFifoPtr,
            &inputResultsWrapperPtr);
        EB_CHECK_END_OBJ(inputResultsWrapperPtr);
        
        inputResultsPtr         = (PictureAnalysisResults_t*)   inputResultsWrapperPtr->objectPtr;     
        pictureControlSetPtr    = (PictureParentControlSet_t*)  inputResultsPtr->pictureControlSetWrapperPtr->objectPtr;
        sequenceControlSetPtr   = (SequenceControlSet_t*)       pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
        encodeContextPtr        = (EncodeContext_t*)            sequenceControlSetPtr->encodeContextPtr;

#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld PD IN \n", pictureControlSetPtr->pictureNumber);
#endif

        loopCount ++;

        // Input Picture Analysis Results into the Picture Decision Reordering Queue
        // P.S. Since the prior Picture Analysis processes stage is multithreaded, inputs to the Picture Decision Process 
        // can arrive out-of-display-order, so a the Picture Decision Reordering Queue is used to enforce processing of 
        // pictures in display order

        queueEntryIndex                                         =   (EB_S32) (pictureControlSetPtr->pictureNumber - encodeContextPtr->pictureDecisionReorderQueue[encodeContextPtr->pictureDecisionReorderQueueHeadIndex]->pictureNumber);        
        queueEntryIndex                                         +=  encodeContextPtr->pictureDecisionReorderQueueHeadIndex;
        queueEntryIndex                                         =   (queueEntryIndex > PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH - 1) ? queueEntryIndex - PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH : queueEntryIndex;        
        queueEntryPtr                                           =   encodeContextPtr->pictureDecisionReorderQueue[queueEntryIndex];         
        if(queueEntryPtr->parentPcsWrapperPtr != NULL){
            CHECK_REPORT_ERROR_NC(
                encodeContextPtr->appCallbackPtr, 
                EB_ENC_PD_ERROR8);
        }else{
            queueEntryPtr->parentPcsWrapperPtr                      =   inputResultsPtr->pictureControlSetWrapperPtr;
            queueEntryPtr->pictureNumber                            =   pictureControlSetPtr->pictureNumber;
        }
        // Process the head of the Picture Decision Reordering Queue (Entry N) 
        // P.S. The Picture Decision Reordering Queue should be parsed in the display order to be able to construct a pred structure
        queueEntryPtr = encodeContextPtr->pictureDecisionReorderQueue[encodeContextPtr->pictureDecisionReorderQueueHeadIndex];
        
        while(queueEntryPtr->parentPcsWrapperPtr != EB_NULL) {
            
            if(queueEntryPtr->pictureNumber == 0 ||
                sequenceControlSetPtr->staticConfig.sceneChangeDetection == 0 ||
                ((PictureParentControlSet_t *)(queueEntryPtr->parentPcsWrapperPtr->objectPtr))->endOfSequenceFlag == EB_TRUE){
                    framePasseThru = EB_TRUE;
            }else{
                 framePasseThru = EB_FALSE;
            }
            windowAvail                = EB_TRUE; 
            previousEntryIndex         = QUEUE_GET_PREVIOUS_SPOT(encodeContextPtr->pictureDecisionReorderQueueHeadIndex); 

            if(encodeContextPtr->pictureDecisionReorderQueue[previousEntryIndex]->parentPcsWrapperPtr == NULL){
                windowAvail = EB_FALSE;
            }else{
                ParentPcsWindow[0] =(PictureParentControlSet_t *) encodeContextPtr->pictureDecisionReorderQueue[previousEntryIndex]->parentPcsWrapperPtr->objectPtr;
                ParentPcsWindow[1] =(PictureParentControlSet_t *) encodeContextPtr->pictureDecisionReorderQueue[encodeContextPtr->pictureDecisionReorderQueueHeadIndex]->parentPcsWrapperPtr->objectPtr;
                for(windowIndex=0;    windowIndex<FUTURE_WINDOW_WIDTH;     windowIndex++){
                    entryIndex = QUEUE_GET_NEXT_SPOT(encodeContextPtr->pictureDecisionReorderQueueHeadIndex , windowIndex+1 ) ;

                    if(encodeContextPtr->pictureDecisionReorderQueue[entryIndex]->parentPcsWrapperPtr == NULL ){
                        windowAvail = EB_FALSE;
                        break;
                    }
                    else if (((PictureParentControlSet_t *)(encodeContextPtr->pictureDecisionReorderQueue[entryIndex]->parentPcsWrapperPtr->objectPtr))->endOfSequenceFlag == EB_TRUE) {
                        windowAvail = EB_FALSE;
                        framePasseThru = EB_TRUE;
                        break;
                    }else{
                        ParentPcsWindow[2+windowIndex] =(PictureParentControlSet_t *) encodeContextPtr->pictureDecisionReorderQueue[entryIndex]->parentPcsWrapperPtr->objectPtr;
                    }
                }
            } 
            pictureControlSetPtr                        = (PictureParentControlSet_t*)  queueEntryPtr->parentPcsWrapperPtr->objectPtr;

            if(pictureControlSetPtr->idrFlag == EB_TRUE)
                contextPtr->lastSolidColorFramePoc = 0xFFFFFFFF;

            if(windowAvail == EB_TRUE) {
                if(sequenceControlSetPtr->staticConfig.sceneChangeDetection) {
                    pictureControlSetPtr->sceneChangeFlag = SceneTransitionDetector(
                        contextPtr,
                        sequenceControlSetPtr,
                        ParentPcsWindow,
                        FUTURE_WINDOW_WIDTH);
                } else {
                    pictureControlSetPtr->sceneChangeFlag = EB_FALSE;
                }

                // Store scene change in context
                contextPtr->isSceneChangeDetected = pictureControlSetPtr->sceneChangeFlag;
            }

            if(windowAvail == EB_TRUE ||framePasseThru == EB_TRUE)
			{
            // Place the PCS into the Pre-Assignment Buffer
            // P.S. The Pre-Assignment Buffer is used to store a whole pre-structure
            encodeContextPtr->preAssignmentBuffer[encodeContextPtr->preAssignmentBufferCount] = queueEntryPtr->parentPcsWrapperPtr;
            
            // Setup the PCS & SCS
            pictureControlSetPtr                        = (PictureParentControlSet_t*)  encodeContextPtr->preAssignmentBuffer[encodeContextPtr->preAssignmentBufferCount]->objectPtr;
            
            // Set the POC Number
            pictureControlSetPtr->pictureNumber    = (encodeContextPtr->currentInputPoc + 1) /*& ((1 << sequenceControlSetPtr->bitsForPictureOrderCount)-1)*/;
            encodeContextPtr->currentInputPoc      = pictureControlSetPtr->pictureNumber;


			pictureControlSetPtr->predStructure = sequenceControlSetPtr->staticConfig.predStructure;

			pictureControlSetPtr->hierarchicalLayersDiff = 0;
          
           pictureControlSetPtr->initPredStructPositionFlag    = EB_FALSE;
    
           pictureControlSetPtr->targetBitRate          = sequenceControlSetPtr->staticConfig.targetBitRate;
           pictureControlSetPtr->droppedFramesNumber    = 0;    

           ReleasePrevPictureFromReorderQueue(
               encodeContextPtr);

            pictureControlSetPtr->craFlag = EB_FALSE;
            pictureControlSetPtr->idrFlag = EB_FALSE;

            // If the Intra period length is 0, then introduce an intra for every picture
            if ((sequenceControlSetPtr->intraPeriodLength == 0) || (pictureControlSetPtr->pictureNumber == 0)) {
                if (sequenceControlSetPtr->intraRefreshType == CRA_REFRESH)
                    pictureControlSetPtr->craFlag = EB_TRUE;
                else
                    pictureControlSetPtr->idrFlag = EB_TRUE;
            }
            // If an #IntraPeriodLength has passed since the last Intra, then introduce a CRA or IDR based on Intra Refresh type
            else if (sequenceControlSetPtr->intraPeriodLength != -1) {
                if ((encodeContextPtr->intraPeriodPosition == (EB_U32) sequenceControlSetPtr->intraPeriodLength) ||
                    (pictureControlSetPtr->sceneChangeFlag == EB_TRUE)) {
                    if (sequenceControlSetPtr->intraRefreshType == CRA_REFRESH)
                        pictureControlSetPtr->craFlag = EB_TRUE;
                    else
                        pictureControlSetPtr->idrFlag = EB_TRUE;
                }
            }

            encodeContextPtr->preAssignmentBufferEosFlag             = (pictureControlSetPtr->endOfSequenceFlag) ? (EB_U32)EB_TRUE : encodeContextPtr->preAssignmentBufferEosFlag;

            // Increment the Pre-Assignment Buffer Intra Count
            encodeContextPtr->preAssignmentBufferIntraCount         += (pictureControlSetPtr->idrFlag || pictureControlSetPtr->craFlag);
            encodeContextPtr->preAssignmentBufferIdrCount           += pictureControlSetPtr->idrFlag;
            encodeContextPtr->preAssignmentBufferCount              += 1;

            if (sequenceControlSetPtr->staticConfig.rateControlMode)
            {
                // Increment the Intra Period Position
                encodeContextPtr->intraPeriodPosition = (encodeContextPtr->intraPeriodPosition == (EB_U32)sequenceControlSetPtr->intraPeriodLength) ?
                                                        0 : encodeContextPtr->intraPeriodPosition + 1;
            } else {
                // Increment the Intra Period Position
                encodeContextPtr->intraPeriodPosition = ((encodeContextPtr->intraPeriodPosition == (EB_U32)sequenceControlSetPtr->intraPeriodLength) ||
                                                        (pictureControlSetPtr->sceneChangeFlag == EB_TRUE)) ?
                                                        0 : encodeContextPtr->intraPeriodPosition + 1;
            }

			// Determine if Pictures can be released from the Pre-Assignment Buffer
			if ((encodeContextPtr->preAssignmentBufferIntraCount > 0) ||
				(encodeContextPtr->preAssignmentBufferCount == (EB_U32) (1 << sequenceControlSetPtr->staticConfig.hierarchicalLevels)) ||
				(encodeContextPtr->preAssignmentBufferEosFlag == EB_TRUE) ||
				(pictureControlSetPtr->predStructure == EB_PRED_LOW_DELAY_P) ||
				(pictureControlSetPtr->predStructure == EB_PRED_LOW_DELAY_B))
			{

				// Initialize Picture Block Params 
				contextPtr->miniGopStartIndex[0]		 = 0;
				contextPtr->miniGopEndIndex	 [0]		 = encodeContextPtr->preAssignmentBufferCount - 1;
				contextPtr->miniGopLenght	 [0]		 = encodeContextPtr->preAssignmentBufferCount;

				contextPtr->miniGopHierarchicalLevels[0] = sequenceControlSetPtr->staticConfig.hierarchicalLevels;
				contextPtr->miniGopIntraCount[0]		 = encodeContextPtr->preAssignmentBufferIntraCount;
				contextPtr->miniGopIdrCount  [0]		 = encodeContextPtr->preAssignmentBufferIdrCount;
				contextPtr->totalNumberOfMiniGops		 = 1;
				
				encodeContextPtr->previousMiniGopHierarchicalLevels = (pictureControlSetPtr->pictureNumber == 0) ?
					sequenceControlSetPtr->staticConfig.hierarchicalLevels :
					encodeContextPtr->previousMiniGopHierarchicalLevels;

				GenerateMiniGopRps(
					contextPtr,
					encodeContextPtr);
           
                // Loop over Mini GOPs 

				for (miniGopIndex = 0; miniGopIndex < contextPtr->totalNumberOfMiniGops; ++miniGopIndex) {

					preAssignmentBufferFirstPassFlag = EB_TRUE;

					// 1st Loop over Pictures in the Pre-Assignment Buffer
					for (pictureIndex = contextPtr->miniGopStartIndex[miniGopIndex]; pictureIndex <= contextPtr->miniGopEndIndex[miniGopIndex]; ++pictureIndex) {

						pictureControlSetPtr = (PictureParentControlSet_t*)encodeContextPtr->preAssignmentBuffer[pictureIndex]->objectPtr;
						sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;

						// Keep track of the mini GOP size to which the input picture belongs - needed @ PictureManagerProcess() 
						pictureControlSetPtr->preAssignmentBufferCount = contextPtr->miniGopLenght[miniGopIndex];

						// Update the Pred Structure if cutting short a Random Access period
						if ((contextPtr->miniGopLenght[miniGopIndex] < pictureControlSetPtr->predStructPtr->predStructPeriod || contextPtr->miniGopIdrCount[miniGopIndex] > 0) &&

							pictureControlSetPtr->predStructPtr->predType == EB_PRED_RANDOM_ACCESS &&
							pictureControlSetPtr->idrFlag == EB_FALSE &&
							pictureControlSetPtr->craFlag == EB_FALSE)
						{
							// Correct the Pred Index before switching structures
							if (preAssignmentBufferFirstPassFlag == EB_TRUE) {
								encodeContextPtr->predStructPosition -= pictureControlSetPtr->predStructPtr->initPicIndex;
							}

							pictureControlSetPtr->predStructPtr = GetPredictionStructure(
								encodeContextPtr->predictionStructureGroupPtr,
								EB_PRED_LOW_DELAY_P,
								1,
								pictureControlSetPtr->hierarchicalLevels);

							// Set the RPS Override Flag - this current only will convert a Random Access structure to a Low Delay structure
							pictureControlSetPtr->useRpsInSps = EB_FALSE;
							pictureControlSetPtr->openGopCraFlag = EB_FALSE;

							pictureType = EB_P_PICTURE;

						}
						// Open GOP CRA - adjust the RPS
						else if ((contextPtr->miniGopLenght[miniGopIndex] == pictureControlSetPtr->predStructPtr->predStructPeriod) &&

							(pictureControlSetPtr->predStructPtr->predType == EB_PRED_RANDOM_ACCESS || pictureControlSetPtr->predStructPtr->temporalLayerCount == 1) &&
							pictureControlSetPtr->idrFlag == EB_FALSE &&
							pictureControlSetPtr->craFlag == EB_TRUE)
						{
							pictureControlSetPtr->useRpsInSps = EB_FALSE;
							pictureControlSetPtr->openGopCraFlag = EB_TRUE;

							pictureType = EB_I_PICTURE;
						}
						else {

							pictureControlSetPtr->useRpsInSps = EB_FALSE;
							pictureControlSetPtr->openGopCraFlag = EB_FALSE;

							// Set the Picture Type
							pictureType =
								(pictureControlSetPtr->idrFlag) ? EB_I_PICTURE :
								(pictureControlSetPtr->craFlag) ? EB_I_PICTURE :
								(pictureControlSetPtr->predStructure == EB_PRED_LOW_DELAY_P) ? EB_P_PICTURE :
								(pictureControlSetPtr->predStructure == EB_PRED_LOW_DELAY_B) ? EB_B_PICTURE :
								(pictureControlSetPtr->preAssignmentBufferCount == pictureControlSetPtr->predStructPtr->predStructPeriod) ? ((pictureIndex == contextPtr->miniGopEndIndex[miniGopIndex] && sequenceControlSetPtr->staticConfig.baseLayerSwitchMode) ? EB_P_PICTURE : EB_B_PICTURE) :

								(encodeContextPtr->preAssignmentBufferEosFlag) ? EB_P_PICTURE :
								EB_B_PICTURE;
						}

						// If Intra, reset position
						if (pictureControlSetPtr->idrFlag == EB_TRUE) {
							encodeContextPtr->predStructPosition = pictureControlSetPtr->predStructPtr->initPicIndex;
						}

						else if (pictureControlSetPtr->craFlag == EB_TRUE && contextPtr->miniGopLenght[miniGopIndex] < pictureControlSetPtr->predStructPtr->predStructPeriod) {

							encodeContextPtr->predStructPosition = pictureControlSetPtr->predStructPtr->initPicIndex;
						}
						else if (encodeContextPtr->elapsedNonCraCount == 0) {
							// If we are the picture directly after a CRA, we have to not use references that violate the CRA
							encodeContextPtr->predStructPosition = pictureControlSetPtr->predStructPtr->initPicIndex + 1;
						}
						// Elif Scene Change, determine leading and trailing pictures
						//else if (encodeContextPtr->preAssignmentBufferSceneChangeCount > 0) {
						//    if(bufferIndex < encodeContextPtr->preAssignmentBufferSceneChangeIndex) {
						//        ++encodeContextPtr->predStructPosition;
						//        pictureType = EB_P_PICTURE;
						//    }
						//    else {
						//        encodeContextPtr->predStructPosition = pictureControlSetPtr->predStructPtr->initPicIndex + encodeContextPtr->preAssignmentBufferCount - bufferIndex - 1;
						//    }
						//}
						// Else, Increment the position normally
						else {
							++encodeContextPtr->predStructPosition;
						}

						// The poc number of the latest IDR picture is stored so that lastIdrPicture (present in PCS) for the incoming pictures can be updated.
						// The lastIdrPicture is used in reseting the poc (in entropy coding) whenever IDR is encountered. 
						// Note IMP: This logic only works when display and decode order are the same. Currently for Random Access, IDR is inserted (similar to CRA) by using trailing P pictures (low delay fashion) and breaking prediction structure. 
						// Note: When leading P pictures are implemented, this logic has to change.. 
						if (pictureControlSetPtr->idrFlag == EB_TRUE) {
							encodeContextPtr->lastIdrPicture = pictureControlSetPtr->pictureNumber;
						}
						else {
							pictureControlSetPtr->lastIdrPicture = encodeContextPtr->lastIdrPicture;
						}


						// Cycle the PredStructPosition if its overflowed
						encodeContextPtr->predStructPosition = (encodeContextPtr->predStructPosition == pictureControlSetPtr->predStructPtr->predStructEntryCount) ?
							encodeContextPtr->predStructPosition - pictureControlSetPtr->predStructPtr->predStructPeriod :
							encodeContextPtr->predStructPosition;

						predPositionPtr = pictureControlSetPtr->predStructPtr->predStructEntryPtrArray[encodeContextPtr->predStructPosition];

						// Set the NAL Unit
						if (pictureControlSetPtr->idrFlag == EB_TRUE) {
							pictureControlSetPtr->nalUnit = NAL_UNIT_CODED_SLICE_IDR_W_RADL;
						}
						else if (pictureControlSetPtr->craFlag == EB_TRUE) {
							pictureControlSetPtr->nalUnit = NAL_UNIT_CODED_SLICE_CRA;
						}
						// User specify of use of non-reference picture is OFF 
						else {
							// If we have an open GOP situation, where pictures are forward-referencing to a CRA, then those pictures have to be tagged as RASL.
							if ((contextPtr->miniGopIntraCount[miniGopIndex] > 0) && (contextPtr->miniGopIdrCount[miniGopIndex] == 0) &&
								(contextPtr->miniGopLenght[miniGopIndex] == pictureControlSetPtr->predStructPtr->predStructPeriod)) {

								if (pictureControlSetPtr->hierarchicalLevels > 0 && predPositionPtr->temporalLayerIndex == pictureControlSetPtr->hierarchicalLevels){
									pictureControlSetPtr->nalUnit = NAL_UNIT_CODED_SLICE_RASL_N;
								}
								else {
									pictureControlSetPtr->nalUnit = NAL_UNIT_CODED_SLICE_RASL_R;
								}
							}
							else if (pictureControlSetPtr->hierarchicalLevels > 0 && predPositionPtr->temporalLayerIndex == pictureControlSetPtr->hierarchicalLevels){
								pictureControlSetPtr->nalUnit = NAL_UNIT_CODED_SLICE_TRAIL_N;
							}
							else {
								pictureControlSetPtr->nalUnit = NAL_UNIT_CODED_SLICE_TRAIL_R;
							}
						}

						// Set the Slice type
						pictureControlSetPtr->sliceType = pictureType;
						((EbPaReferenceObject_t*)pictureControlSetPtr->paReferencePictureWrapperPtr->objectPtr)->sliceType = pictureControlSetPtr->sliceType;



						switch (pictureType) {

						case EB_I_PICTURE:

							// Reset Prediction Structure Position & Reference Struct Position 
							if (pictureControlSetPtr->pictureNumber == 0){
								encodeContextPtr->intraPeriodPosition = 0;
							}
							encodeContextPtr->elapsedNonCraCount = 0;

							//-------------------------------
							// IDR
							//-------------------------------
							if (pictureControlSetPtr->idrFlag == EB_TRUE) {

								// Set CRA flag
								pictureControlSetPtr->craFlag = EB_FALSE;

								// Reset the pictures since last IDR counter
								encodeContextPtr->elapsedNonIdrCount = 0;

							}
							//-------------------------------
							// CRA
							//-------------------------------
							else {

								// Set a Random Access Point if not an IDR
								pictureControlSetPtr->craFlag = EB_TRUE;
							}

							break;

						case EB_P_PICTURE:
						case EB_B_PICTURE:

							// Reset CRA and IDR Flag
							pictureControlSetPtr->craFlag = EB_FALSE;
							pictureControlSetPtr->idrFlag = EB_FALSE;

							// Increment & Clip the elapsed Non-IDR Counter. This is clipped rather than allowed to free-run
							// inorder to avoid rollover issues.  This assumes that any the GOP period is less than MAX_ELAPSED_IDR_COUNT
							encodeContextPtr->elapsedNonIdrCount = MIN(encodeContextPtr->elapsedNonIdrCount + 1, MAX_ELAPSED_IDR_COUNT);
							encodeContextPtr->elapsedNonCraCount = MIN(encodeContextPtr->elapsedNonCraCount + 1, MAX_ELAPSED_IDR_COUNT);

							CHECK_REPORT_ERROR(
								(pictureControlSetPtr->predStructPtr->predStructEntryCount < MAX_ELAPSED_IDR_COUNT),
								encodeContextPtr->appCallbackPtr,
								EB_ENC_PD_ERROR1);

							break;

						default:

							CHECK_REPORT_ERROR_NC(
								encodeContextPtr->appCallbackPtr,
								EB_ENC_PD_ERROR2);

							break;
						}
						pictureControlSetPtr->predStructIndex = (EB_U8)encodeContextPtr->predStructPosition;
						pictureControlSetPtr->temporalLayerIndex = (EB_U8)predPositionPtr->temporalLayerIndex;
						pictureControlSetPtr->isUsedAsReferenceFlag = predPositionPtr->isReferenced;

                        pictureControlSetPtr->disableTmvpFlag = sequenceControlSetPtr->staticConfig.unrestrictedMotionVector == 0 ? EB_TRUE : EB_FALSE;

                        pictureControlSetPtr->useSrcRef = (sequenceControlSetPtr->staticConfig.improveSharpness && pictureControlSetPtr->temporalLayerIndex > 0) ?
                            EB_TRUE :
                            EB_FALSE;

                        SignalDerivationMultiProcessesOq(
                                sequenceControlSetPtr,
                                pictureControlSetPtr);

                        // Set the Decode Order
						if ((sequenceControlSetPtr->staticConfig.predStructure == EB_PRED_RANDOM_ACCESS) && (contextPtr->miniGopIdrCount[miniGopIndex] == 0) &&

							(contextPtr->miniGopLenght[miniGopIndex] == pictureControlSetPtr->predStructPtr->predStructPeriod))

                        {
                            pictureControlSetPtr->decodeOrder = encodeContextPtr->decodeBaseNumber + predPositionPtr->decodeOrder;
                        }
                        else {
                            pictureControlSetPtr->decodeOrder = pictureControlSetPtr->pictureNumber;
                        }
                        EbBlockOnMutex(encodeContextPtr->terminatingConditionsMutex);

                        encodeContextPtr->terminatingSequenceFlagReceived = (pictureControlSetPtr->endOfSequenceFlag == EB_TRUE) ? 
                            EB_TRUE :  
                            encodeContextPtr->terminatingSequenceFlagReceived;

                        encodeContextPtr->terminatingPictureNumber = (pictureControlSetPtr->endOfSequenceFlag == EB_TRUE) ? 
                            pictureControlSetPtr->pictureNumber :
                            encodeContextPtr->terminatingPictureNumber;

                        EbReleaseMutex(encodeContextPtr->terminatingConditionsMutex);

                        preAssignmentBufferFirstPassFlag = EB_FALSE;
                    
                        // Update the Dependant List Count - If there was an I-frame or Scene Change, then cleanup the Picture Decision PA Reference Queue Dependent Counts
                        if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
                        {

                            inputQueueIndex = encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex;

                            while(inputQueueIndex != encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex) {
                        
                                inputEntryPtr = encodeContextPtr->pictureDecisionPaReferenceQueue[inputQueueIndex];
                            
                                // Modify Dependent List0
                                depListCount = inputEntryPtr->list0.listCount;
                                for(depIdx=0; depIdx < depListCount; ++depIdx) {

                        
                                    // Adjust the latest currentInputPoc in case we're in a POC rollover scenario 
                                    // currentInputPoc += (currentInputPoc < inputEntryPtr->pocNumber) ? (1 << sequenceControlSetPtr->bitsForPictureOrderCount) : 0;
                            
                                    depPoc = POC_CIRCULAR_ADD(
                                        inputEntryPtr->pictureNumber, // can't use a value that gets reset
                                        inputEntryPtr->list0.list[depIdx]/*,
                                        sequenceControlSetPtr->bitsForPictureOrderCount*/);
                            
                                    // If Dependent POC is greater or equal to the IDR POC
                                    if(depPoc >= pictureControlSetPtr->pictureNumber && inputEntryPtr->list0.list[depIdx]) {
                                
                                        inputEntryPtr->list0.list[depIdx] = 0;

                                        // Decrement the Reference's referenceCount
                                        --inputEntryPtr->dependentCount;

                                        CHECK_REPORT_ERROR(
                                            (inputEntryPtr->dependentCount != ~0u),
                                            encodeContextPtr->appCallbackPtr, 
                                            EB_ENC_PD_ERROR3);  
                                    }
                                }
                            
                                // Modify Dependent List1
                                depListCount = inputEntryPtr->list1.listCount;
                                for(depIdx=0; depIdx < depListCount; ++depIdx) {
                                
                                    // Adjust the latest currentInputPoc in case we're in a POC rollover scenario 
                                    // currentInputPoc += (currentInputPoc < inputEntryPtr->pocNumber) ? (1 << sequenceControlSetPtr->bitsForPictureOrderCount) : 0;
                            
                                    depPoc = POC_CIRCULAR_ADD(
                                        inputEntryPtr->pictureNumber,
                                        inputEntryPtr->list1.list[depIdx]/*,
                                        sequenceControlSetPtr->bitsForPictureOrderCount*/);
                            
                                    // If Dependent POC is greater or equal to the IDR POC
                                    if(((depPoc >= pictureControlSetPtr->pictureNumber) || (((pictureControlSetPtr->preAssignmentBufferCount != pictureControlSetPtr->predStructPtr->predStructPeriod) || (pictureControlSetPtr->idrFlag == EB_TRUE)) && (depPoc >  (pictureControlSetPtr->pictureNumber - pictureControlSetPtr->preAssignmentBufferCount)))) && inputEntryPtr->list1.list[depIdx]) {
                                        inputEntryPtr->list1.list[depIdx] = 0;

                                        // Decrement the Reference's referenceCount
                                        --inputEntryPtr->dependentCount;

                                        CHECK_REPORT_ERROR(
                                            (inputEntryPtr->dependentCount != ~0u),
                                            encodeContextPtr->appCallbackPtr, 
                                            EB_ENC_PD_ERROR3); 
                                    }
                                
                                }                                                                                                                                
                            
                                // Increment the inputQueueIndex Iterator
                                inputQueueIndex = (inputQueueIndex == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : inputQueueIndex + 1;
                            } 
                    
                        } else if(pictureControlSetPtr->idrFlag == EB_TRUE) {
                        
                            // Set the Picture Decision PA Reference Entry pointer
                            inputEntryPtr                           = (PaReferenceQueueEntry_t*) EB_NULL;
                        }

                        // Place Picture in Picture Decision PA Reference Queue
                        inputEntryPtr                                       = encodeContextPtr->pictureDecisionPaReferenceQueue[encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex];
                        inputEntryPtr->inputObjectPtr                       = pictureControlSetPtr->paReferencePictureWrapperPtr;
                        inputEntryPtr->pictureNumber                        = pictureControlSetPtr->pictureNumber;
                        inputEntryPtr->referenceEntryIndex                  = encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex;
                        inputEntryPtr->pPcsPtr = pictureControlSetPtr;                      
                        encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex     = 
                            (encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex + 1;
                    
                        // Check if the Picture Decision PA Reference is full
                        CHECK_REPORT_ERROR(
                            (((encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex != encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex) ||
                            (encodeContextPtr->pictureDecisionPaReferenceQueue[encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex]->inputObjectPtr == EB_NULL))),
                            encodeContextPtr->appCallbackPtr, 
                            EB_ENC_PD_ERROR4);

                        // Copy the reference lists into the inputEntry and 
                        // set the Reference Counts Based on Temporal Layer and how many frames are active
                        pictureControlSetPtr->refList0Count = (pictureType == EB_I_PICTURE) ? 0 : (EB_U8)predPositionPtr->refList0.referenceListCount;
                        pictureControlSetPtr->refList1Count = (pictureType == EB_I_PICTURE) ? 0 : (EB_U8)predPositionPtr->refList1.referenceListCount;

						inputEntryPtr->list0Ptr             = &predPositionPtr->refList0;
                        inputEntryPtr->list1Ptr             = &predPositionPtr->refList1;
                                
                        {

                            // Copy the Dependent Lists
                            // *Note - we are removing any leading picture dependencies for now
                            inputEntryPtr->list0.listCount = 0;
                            for(depIdx = 0; depIdx < predPositionPtr->depList0.listCount; ++depIdx) {
                                if(predPositionPtr->depList0.list[depIdx] >= 0) {
                                    inputEntryPtr->list0.list[inputEntryPtr->list0.listCount++] = predPositionPtr->depList0.list[depIdx];
                                }
                            }

                            inputEntryPtr->list1.listCount = predPositionPtr->depList1.listCount;
                            for(depIdx = 0; depIdx < predPositionPtr->depList1.listCount; ++depIdx) {
                                inputEntryPtr->list1.list[depIdx] = predPositionPtr->depList1.list[depIdx];
                            }

                            inputEntryPtr->depList0Count                = inputEntryPtr->list0.listCount;
                            inputEntryPtr->depList1Count                = inputEntryPtr->list1.listCount;
                            inputEntryPtr->dependentCount               = inputEntryPtr->depList0Count + inputEntryPtr->depList1Count;

                        }

						((EbPaReferenceObject_t*)pictureControlSetPtr->paReferencePictureWrapperPtr->objectPtr)->dependentPicturesCount = inputEntryPtr->dependentCount;

						/* EB_U32 depCnt = ((EbPaReferenceObject_t*)pictureControlSetPtr->paReferencePictureWrapperPtr->objectPtr)->dependentPicturesCount;
						if (pictureControlSetPtr->pictureNumber>0 && pictureControlSetPtr->sliceType==EB_I_PICTURE && depCnt!=8 )
						SVT_LOG("depCnt Error1  POC:%i  TL:%i   is needed:%i\n",pictureControlSetPtr->pictureNumber,pictureControlSetPtr->temporalLayerIndex,inputEntryPtr->dependentCount);
						else if (pictureControlSetPtr->sliceType==EB_B_PICTURE && pictureControlSetPtr->temporalLayerIndex == 0 && depCnt!=8)
						SVT_LOG("depCnt Error2  POC:%i  TL:%i   is needed:%i\n",pictureControlSetPtr->pictureNumber,pictureControlSetPtr->temporalLayerIndex,inputEntryPtr->dependentCount);
						else if (pictureControlSetPtr->sliceType==EB_B_PICTURE && pictureControlSetPtr->temporalLayerIndex == 1 && depCnt!=4)
						SVT_LOG("depCnt Error3  POC:%i  TL:%i   is needed:%i\n",pictureControlSetPtr->pictureNumber,pictureControlSetPtr->temporalLayerIndex,inputEntryPtr->dependentCount);
						else if (pictureControlSetPtr->sliceType==EB_B_PICTURE && pictureControlSetPtr->temporalLayerIndex == 2 && depCnt!=2)
						SVT_LOG("depCnt Error4  POC:%i  TL:%i   is needed:%i\n",pictureControlSetPtr->pictureNumber,pictureControlSetPtr->temporalLayerIndex,inputEntryPtr->dependentCount);
						else if (pictureControlSetPtr->sliceType==EB_B_PICTURE && pictureControlSetPtr->temporalLayerIndex == 3 && depCnt!=0)
						SVT_LOG("depCnt Error5  POC:%i  TL:%i   is needed:%i\n",pictureControlSetPtr->pictureNumber,pictureControlSetPtr->temporalLayerIndex,inputEntryPtr->dependentCount);*/
						//if (pictureControlSetPtr->sliceType==EB_P_PICTURE )
						//     SVT_LOG("POC:%i  TL:%i   is needed:%i\n",pictureControlSetPtr->pictureNumber,pictureControlSetPtr->temporalLayerIndex,inputEntryPtr->dependentCount);

                        CHECK_REPORT_ERROR(
                            (pictureControlSetPtr->predStructPtr->predStructPeriod < MAX_ELAPSED_IDR_COUNT),
                            encodeContextPtr->appCallbackPtr, 
                            EB_ENC_PD_ERROR5);

                        // Reset the PA Reference Lists
						EB_MEMSET(pictureControlSetPtr->refPaPicPtrArray, 0, 2 * sizeof(EbObjectWrapper_t*));
                    
						EB_MEMSET(pictureControlSetPtr->refPaPicPtrArray, 0, 2 * sizeof(EB_U32));
               
                    }
                
                    // 2nd Loop over Pictures in the Pre-Assignment Buffer
					for (pictureIndex = contextPtr->miniGopStartIndex[miniGopIndex]; pictureIndex <= contextPtr->miniGopEndIndex[miniGopIndex]; ++pictureIndex) {
						
						pictureControlSetPtr = (PictureParentControlSet_t*)	encodeContextPtr->preAssignmentBuffer[pictureIndex]->objectPtr;

                        // Find the Reference in the Picture Decision PA Reference Queue
                        inputQueueIndex = encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex;

                        do {
                        
                            // Setup the Picture Decision PA Reference Queue Entry
                            inputEntryPtr   = encodeContextPtr->pictureDecisionPaReferenceQueue[inputQueueIndex];
                        
                            // Increment the referenceQueueIndex Iterator
                            inputQueueIndex = (inputQueueIndex == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : inputQueueIndex + 1;
                    
                        } while ((inputQueueIndex != encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex) && (inputEntryPtr->pictureNumber != pictureControlSetPtr->pictureNumber));

                        CHECK_REPORT_ERROR(
                            (inputEntryPtr->pictureNumber == pictureControlSetPtr->pictureNumber),
                            encodeContextPtr->appCallbackPtr, 
                            EB_ENC_PD_ERROR7);

                        // Reset the PA Reference Lists
						EB_MEMSET(pictureControlSetPtr->refPaPicPtrArray, 0, 2 * sizeof(EbObjectWrapper_t*));
						
						EB_MEMSET(pictureControlSetPtr->refPicPocArray, 0, 2 * sizeof(EB_U64));
                    

                        // Configure List0
                        if ((pictureControlSetPtr->sliceType == EB_P_PICTURE) || (pictureControlSetPtr->sliceType == EB_B_PICTURE)) {
                    
							if (pictureControlSetPtr->refList0Count){
                                paReferenceQueueIndex = (EB_U32) CIRCULAR_ADD(
                                    ((EB_S32) inputEntryPtr->referenceEntryIndex) - inputEntryPtr->list0Ptr->referenceList,      
                                    PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH);                                                                                             // Max
                                
                                paReferenceEntryPtr = encodeContextPtr->pictureDecisionPaReferenceQueue[paReferenceQueueIndex];
                        
                                // Calculate the Ref POC
                                refPoc = POC_CIRCULAR_ADD(
                                    pictureControlSetPtr->pictureNumber,
                                    -inputEntryPtr->list0Ptr->referenceList/*,
                                    sequenceControlSetPtr->bitsForPictureOrderCount*/);
                       
                                // Set the Reference Object
                                pictureControlSetPtr->refPaPicPtrArray[REF_LIST_0] = paReferenceEntryPtr->inputObjectPtr;
                                pictureControlSetPtr->refPicPocArray[REF_LIST_0] = refPoc;
                                pictureControlSetPtr->refPaPcsArray[REF_LIST_0] = paReferenceEntryPtr->pPcsPtr;

                                // Increment the PA Reference's liveCount by the number of tiles in the input picture
                                EbObjectIncLiveCount(
                                    paReferenceEntryPtr->inputObjectPtr,
                                    1);

                                ((EbPaReferenceObject_t*)pictureControlSetPtr->refPaPicPtrArray[REF_LIST_0]->objectPtr)->pPcsPtr = paReferenceEntryPtr->pPcsPtr;

                                EbObjectIncLiveCount(
                                    paReferenceEntryPtr->pPcsPtr->pPcsWrapperPtr,
                                    1);
                                
                                --paReferenceEntryPtr->dependentCount;
                            }
                        }
                 
                        // Configure List1
                        if (pictureControlSetPtr->sliceType == EB_B_PICTURE) {
                        
							if (pictureControlSetPtr->refList1Count){
                                paReferenceQueueIndex = (EB_U32) CIRCULAR_ADD(
                                    ((EB_S32) inputEntryPtr->referenceEntryIndex) - inputEntryPtr->list1Ptr->referenceList,      
                                    PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH);                                                                                             // Max
                                
                                paReferenceEntryPtr = encodeContextPtr->pictureDecisionPaReferenceQueue[paReferenceQueueIndex];
                        
                                // Calculate the Ref POC
                                refPoc = POC_CIRCULAR_ADD(
                                    pictureControlSetPtr->pictureNumber,
                                    -inputEntryPtr->list1Ptr->referenceList/*,
                                    sequenceControlSetPtr->bitsForPictureOrderCount*/);
                                pictureControlSetPtr->refPaPcsArray[REF_LIST_1] = paReferenceEntryPtr->pPcsPtr;
                                // Set the Reference Object
                                pictureControlSetPtr->refPaPicPtrArray[REF_LIST_1] = paReferenceEntryPtr->inputObjectPtr;
                                pictureControlSetPtr->refPicPocArray[REF_LIST_1] = refPoc;
                                                            
                                // Increment the PA Reference's liveCount by the number of tiles in the input picture
                                EbObjectIncLiveCount(
                                    paReferenceEntryPtr->inputObjectPtr,
                                    1);

                                ((EbPaReferenceObject_t*)pictureControlSetPtr->refPaPicPtrArray[REF_LIST_1]->objectPtr)->pPcsPtr = paReferenceEntryPtr->pPcsPtr;

                                EbObjectIncLiveCount(
                                    paReferenceEntryPtr->pPcsPtr->pPcsWrapperPtr,
                                    1);
                        
                                --paReferenceEntryPtr->dependentCount;
                            }
                        }

   
                        // Initialize Segments
                        pictureControlSetPtr->meSegmentsColumnCount    =  (EB_U8)(sequenceControlSetPtr->meSegmentColumnCountArray[pictureControlSetPtr->temporalLayerIndex]);
                        pictureControlSetPtr->meSegmentsRowCount       =  (EB_U8)(sequenceControlSetPtr->meSegmentRowCountArray[pictureControlSetPtr->temporalLayerIndex]);
                        pictureControlSetPtr->meSegmentsTotalCount     =  (EB_U16)(pictureControlSetPtr->meSegmentsColumnCount  * pictureControlSetPtr->meSegmentsRowCount);
                        pictureControlSetPtr->meSegmentsCompletionMask = 0;

                        // Post the results to the ME processes                  
                        {
                            EB_U32 segmentIndex;

                            for(segmentIndex=0; segmentIndex < pictureControlSetPtr->meSegmentsTotalCount; ++segmentIndex)
                            {
                                // Get Empty Results Object
                                EbGetEmptyObject(
                                    contextPtr->pictureDecisionResultsOutputFifoPtr,
                                    &outputResultsWrapperPtr);

                                outputResultsPtr = (PictureDecisionResults_t*) outputResultsWrapperPtr->objectPtr;

								outputResultsPtr->pictureControlSetWrapperPtr = encodeContextPtr->preAssignmentBuffer[pictureIndex];

                                outputResultsPtr->segmentIndex = segmentIndex;  
                        
                                // Post the Full Results Object
                                EbPostFullObject(outputResultsWrapperPtr);
                            }
                        }

						if (pictureIndex == contextPtr->miniGopEndIndex[miniGopIndex]) {

							// Increment the Decode Base Number
							encodeContextPtr->decodeBaseNumber  += contextPtr->miniGopLenght[miniGopIndex];
						}

						if (pictureIndex == encodeContextPtr->preAssignmentBufferCount - 1) {
                                    
                            // Reset the Pre-Assignment Buffer
                            encodeContextPtr->preAssignmentBufferCount              = 0;
                            encodeContextPtr->preAssignmentBufferIdrCount           = 0;
                            encodeContextPtr->preAssignmentBufferIntraCount         = 0;
                            encodeContextPtr->preAssignmentBufferSceneChangeCount   = 0;
                            encodeContextPtr->preAssignmentBufferEosFlag            = EB_FALSE;
                            encodeContextPtr->numberOfActivePictures                = 0;
                        }
                    }

                } // End MINI GOPs loop
            }
            
            // Walk the pictureDecisionPaReferenceQueue and remove entries that have been completely referenced.
            inputQueueIndex = encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex;

            while(inputQueueIndex != encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex) {
        
                inputEntryPtr = encodeContextPtr->pictureDecisionPaReferenceQueue[inputQueueIndex];
            
                // Remove the entry
                if((inputEntryPtr->dependentCount == 0) &&
                   (inputEntryPtr->inputObjectPtr)) { 
                    EbReleaseObject(inputEntryPtr->pPcsPtr->pPcsWrapperPtr);
                       // Release the nominal liveCount value
                       EbReleaseObject(inputEntryPtr->inputObjectPtr);
                       inputEntryPtr->inputObjectPtr = (EbObjectWrapper_t*) EB_NULL;
                }
                
                // Increment the HeadIndex if the head is null
                encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex = 
                    (encodeContextPtr->pictureDecisionPaReferenceQueue[encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex]->inputObjectPtr)   ? encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex :
                    (encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1)                 ? 0
                                                                                                                                                        : encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex + 1;

                 CHECK_REPORT_ERROR(
                    (((encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex != encodeContextPtr->pictureDecisionPaReferenceQueueTailIndex) ||
                    (encodeContextPtr->pictureDecisionPaReferenceQueue[encodeContextPtr->pictureDecisionPaReferenceQueueHeadIndex]->inputObjectPtr == EB_NULL))),
                    encodeContextPtr->appCallbackPtr, 
                    EB_ENC_PD_ERROR4);

                // Increment the inputQueueIndex Iterator
                inputQueueIndex = (inputQueueIndex == PICTURE_DECISION_PA_REFERENCE_QUEUE_MAX_DEPTH - 1) ? 0 : inputQueueIndex + 1;
            }
            
            // Increment the Picture Decision Reordering Queue Head Ptr
            encodeContextPtr->pictureDecisionReorderQueueHeadIndex  =   (encodeContextPtr->pictureDecisionReorderQueueHeadIndex == PICTURE_DECISION_REORDER_QUEUE_MAX_DEPTH - 1) ? 0 : encodeContextPtr->pictureDecisionReorderQueueHeadIndex + 1;
            
            // Get the next entry from the Picture Decision Reordering Queue (Entry N+1) 
            queueEntryPtr                                           = encodeContextPtr->pictureDecisionReorderQueue[encodeContextPtr->pictureDecisionReorderQueueHeadIndex];
        }
            if(windowAvail == EB_FALSE  && framePasseThru == EB_FALSE)
                break;
        }        
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld PD OUT \n", pictureControlSetPtr->pictureNumber);
#endif        
        // Release the Input Results
        EbReleaseObject(inputResultsWrapperPtr);
    }
    
    return EB_NULL;
}

static void UnusedVariablevoidFunc_PicDecision()
{
    (void)NxMSadKernel_funcPtrArray;
	(void)NxMSadLoopKernel_funcPtrArray;
    (void)NxMSadAveragingKernel_funcPtrArray;
    (void)SadCalculation_8x8_16x16_funcPtrArray;
    (void)SadCalculation_32x32_64x64_funcPtrArray;
    (void)InitializeBuffer_32bits_funcPtrArray;
}
