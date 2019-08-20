/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbUtility.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbPictureDecisionResults.h"
#include "EbMotionEstimationProcess.h"
#include "EbMotionEstimationResults.h"
#include "EbReferenceObject.h"
#include "EbMotionEstimation.h"
#include "EbIntraPrediction.h"
#include "EbLambdaRateTables.h"
#include "EbComputeSAD.h"

#include "emmintrin.h"

#define SQUARE_PU_NUM  85
#define BUFF_CHECK_SIZE	128

#define DERIVE_INTRA_32_FROM_16   0 //CHKN 1

/* --32x32-
|00||01|
|02||03|
--------*/
/* ------16x16-----
|00||01||04||05|
|02||03||06||07|
|08||09||12||13|
|10||11||14||15|
----------------*/
/* ------8x8----------------------------
|00||01||04||05|     |16||17||20||21|
|02||03||06||07|     |18||19||22||23|
|08||09||12||13|     |24||25||28||29|
|10||11||14||15|     |26||27||30||31|

|32||33||36||37|     |48||49||52||53|
|34||35||38||39|     |50||51||54||55|
|40||41||44||45|     |56||57||60||61|
|42||43||46||47|     |58||59||62||63|
-------------------------------------*/

      
/************************************************
 * Set ME/HME Params
 ************************************************/
static void* SetMeHmeParamsOq(
    MeContext_t                     *meContextPtr,
	PictureParentControlSet_t       *pictureControlSetPtr,
	SequenceControlSet_t            *sequenceControlSetPtr,
	EB_INPUT_RESOLUTION				 inputResolution)
{

	EB_U8  hmeMeLevel = pictureControlSetPtr->encMode;

	EB_U32 inputRatio = sequenceControlSetPtr->lumaWidth / sequenceControlSetPtr->lumaHeight;

	EB_U8 resolutionIndex = inputResolution <= INPUT_SIZE_576p_RANGE_OR_LOWER   ?   0 : // 480P
		(inputResolution <= INPUT_SIZE_1080i_RANGE && inputRatio < 2)           ?   1 : // 720P
		(inputResolution <= INPUT_SIZE_1080i_RANGE && inputRatio > 3)           ?   2 : // 1080I
		(inputResolution <= INPUT_SIZE_1080p_RANGE)                             ?   3 : // 1080I
		                                                                            4;  // 4K
		   
    // HME/ME default settings
	meContextPtr->numberHmeSearchRegionInWidth          = EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT;
	meContextPtr->numberHmeSearchRegionInHeight         = EB_HME_SEARCH_AREA_ROW_MAX_COUNT;
    
    // HME Level0
	meContextPtr->hmeLevel0TotalSearchAreaWidth         = HmeLevel0TotalSearchAreaWidthOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel0TotalSearchAreaHeight        = HmeLevel0TotalSearchAreaHeightOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel0SearchAreaInWidthArray[0]    = HmeLevel0SearchAreaInWidthArrayRightOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel0SearchAreaInWidthArray[1]    = HmeLevel0SearchAreaInWidthArrayLeftOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel0SearchAreaInHeightArray[0]   = HmeLevel0SearchAreaInHeightArrayTopOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel0SearchAreaInHeightArray[1]   = HmeLevel0SearchAreaInHeightArrayBottomOq[resolutionIndex][hmeMeLevel];
    // HME Level1
	meContextPtr->hmeLevel1SearchAreaInWidthArray[0]    = HmeLevel1SearchAreaInWidthArrayRightOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel1SearchAreaInWidthArray[1]    = HmeLevel1SearchAreaInWidthArrayLeftOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel1SearchAreaInHeightArray[0]   = HmeLevel1SearchAreaInHeightArrayTopOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel1SearchAreaInHeightArray[1]   = HmeLevel1SearchAreaInHeightArrayBottomOq[resolutionIndex][hmeMeLevel];
    // HME Level2
	meContextPtr->hmeLevel2SearchAreaInWidthArray[0]    = HmeLevel2SearchAreaInWidthArrayRightOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel2SearchAreaInWidthArray[1]    = HmeLevel2SearchAreaInWidthArrayLeftOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel2SearchAreaInHeightArray[0]   = HmeLevel2SearchAreaInHeightArrayTopOq[resolutionIndex][hmeMeLevel];
	meContextPtr->hmeLevel2SearchAreaInHeightArray[1]   = HmeLevel2SearchAreaInHeightArrayBottomOq[resolutionIndex][hmeMeLevel];

    // ME
	meContextPtr->searchAreaWidth                       = SearchAreaWidthOq[resolutionIndex][hmeMeLevel];
	meContextPtr->searchAreaHeight                      = SearchAreaHeightOq[resolutionIndex][hmeMeLevel];


	// HME Level0 adjustment for low frame rate contents (frame rate <= 30)
    if (inputResolution == INPUT_SIZE_4K_RANGE) {
        if ((sequenceControlSetPtr->staticConfig.frameRate >> 16) <= 30) {

            if (hmeMeLevel == ENC_MODE_6 || hmeMeLevel == ENC_MODE_7) {
                meContextPtr->hmeLevel0TotalSearchAreaWidth         = MAX(96  , meContextPtr->hmeLevel0TotalSearchAreaWidth        );
                meContextPtr->hmeLevel0TotalSearchAreaHeight        = MAX(64  , meContextPtr->hmeLevel0TotalSearchAreaHeight       );
                meContextPtr->hmeLevel0SearchAreaInWidthArray[0]    = MAX(48  , meContextPtr->hmeLevel0SearchAreaInWidthArray[0]   ); 
                meContextPtr->hmeLevel0SearchAreaInWidthArray[1]    = MAX(48  , meContextPtr->hmeLevel0SearchAreaInWidthArray[1]   );
                meContextPtr->hmeLevel0SearchAreaInHeightArray[0]   = MAX(32  , meContextPtr->hmeLevel0SearchAreaInHeightArray[0]  );
                meContextPtr->hmeLevel0SearchAreaInHeightArray[1]   = MAX(32  , meContextPtr->hmeLevel0SearchAreaInHeightArray[1]  );
            }
            else if (hmeMeLevel >= ENC_MODE_8) {
                meContextPtr->hmeLevel0TotalSearchAreaWidth         = MAX(64  , meContextPtr->hmeLevel0TotalSearchAreaWidth        );
                meContextPtr->hmeLevel0TotalSearchAreaHeight        = MAX(48  , meContextPtr->hmeLevel0TotalSearchAreaHeight       );
                meContextPtr->hmeLevel0SearchAreaInWidthArray[0]    = MAX(32  , meContextPtr->hmeLevel0SearchAreaInWidthArray[0]   ); 
                meContextPtr->hmeLevel0SearchAreaInWidthArray[1]    = MAX(32  , meContextPtr->hmeLevel0SearchAreaInWidthArray[1]   );
                meContextPtr->hmeLevel0SearchAreaInHeightArray[0]   = MAX(24  , meContextPtr->hmeLevel0SearchAreaInHeightArray[0]  );
                meContextPtr->hmeLevel0SearchAreaInHeightArray[1]   = MAX(24  , meContextPtr->hmeLevel0SearchAreaInHeightArray[1]  );
            }
        }
    }

    if ((inputResolution > INPUT_SIZE_576p_RANGE_OR_LOWER) && (sequenceControlSetPtr->staticConfig.tune > 0)) {
        meContextPtr->updateHmeSearchCenter = EB_TRUE;
    }
	return EB_NULL;
};




/************************************************
 * Set ME/HME Params from Config
 ************************************************/
static void SetMeHmeParamsFromConfig(
    SequenceControlSet_t	    *sequenceControlSetPtr,
    MeContext_t                 *meContextPtr)
{

    meContextPtr->searchAreaWidth = (EB_U8)sequenceControlSetPtr->staticConfig.searchAreaWidth;
    meContextPtr->searchAreaHeight = (EB_U8)sequenceControlSetPtr->staticConfig.searchAreaHeight;
}

/************************************************
 * Motion Analysis Context Constructor
 ************************************************/

EB_ERRORTYPE MotionEstimationContextCtor(
	MotionEstimationContext_t   **contextDblPtr,
	EbFifo_t                     *pictureDecisionResultsInputFifoPtr,
	EbFifo_t                     *motionEstimationResultsOutputFifoPtr) {

	EB_ERRORTYPE return_error = EB_ErrorNone;
	MotionEstimationContext_t *contextPtr;
	EB_MALLOC(MotionEstimationContext_t*, contextPtr, sizeof(MotionEstimationContext_t), EB_N_PTR);

	*contextDblPtr = contextPtr;

	contextPtr->pictureDecisionResultsInputFifoPtr = pictureDecisionResultsInputFifoPtr;
	contextPtr->motionEstimationResultsOutputFifoPtr = motionEstimationResultsOutputFifoPtr;
	return_error = IntraOpenLoopReferenceSamplesCtor(&contextPtr->intraRefPtr);
	if (return_error == EB_ErrorInsufficientResources){
		return EB_ErrorInsufficientResources;
	}
	return_error = MeContextCtor(&(contextPtr->meContextPtr));
	if (return_error == EB_ErrorInsufficientResources){
		return EB_ErrorInsufficientResources;
	}

	return EB_ErrorNone;

}

/***************************************************************************************************
* ZZ Decimated SAD Computation
***************************************************************************************************/
static EB_ERRORTYPE ComputeDecimatedZzSad(
	MotionEstimationContext_t   *contextPtr,
	SequenceControlSet_t        *sequenceControlSetPtr,
	PictureParentControlSet_t   *pictureControlSetPtr,
	EbPictureBufferDesc_t       *sixteenthDecimatedPicturePtr,
	EB_U32						 xLcuStartIndex,
	EB_U32						 xLcuEndIndex,
	EB_U32						 yLcuStartIndex,
	EB_U32						 yLcuEndIndex) {

	EB_ERRORTYPE return_error = EB_ErrorNone;

	PictureParentControlSet_t	*previousPictureControlSetWrapperPtr = ((PictureParentControlSet_t*)pictureControlSetPtr->previousPictureControlSetWrapperPtr->objectPtr);
	EbPictureBufferDesc_t		*previousInputPictureFull = previousPictureControlSetWrapperPtr->enhancedPicturePtr;

	EB_U32 lcuIndex;

	EB_U32 lcuWidth;
	EB_U32 lcuHeight;

	EB_U32 decimatedLcuWidth;
	EB_U32 decimatedLcuHeight;

	EB_U32 lcuOriginX;
	EB_U32 lcuOriginY;

	EB_U32 blkDisplacementDecimated;
	EB_U32 blkDisplacementFull;

	EB_U32 decimatedLcuCollocatedSad;

	EB_U32 xLcuIndex;
	EB_U32 yLcuIndex;

	for (yLcuIndex = yLcuStartIndex; yLcuIndex < yLcuEndIndex; ++yLcuIndex) {
		for (xLcuIndex = xLcuStartIndex; xLcuIndex < xLcuEndIndex; ++xLcuIndex) {

			lcuIndex = xLcuIndex + yLcuIndex * sequenceControlSetPtr->pictureWidthInLcu;
            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

			lcuWidth = lcuParams->width;
			lcuHeight = lcuParams->height;

			lcuOriginX = lcuParams->originX;
			lcuOriginY = lcuParams->originY;

			lcuWidth = lcuParams->width;
			lcuHeight = lcuParams->height;


			decimatedLcuWidth = lcuWidth >> 2;
			decimatedLcuHeight = lcuHeight >> 2;

			decimatedLcuCollocatedSad = 0;

            if (lcuParams->isCompleteLcu)
			{

				blkDisplacementDecimated = (sixteenthDecimatedPicturePtr->originY + (lcuOriginY >> 2)) * sixteenthDecimatedPicturePtr->strideY + sixteenthDecimatedPicturePtr->originX + (lcuOriginX >> 2);
                blkDisplacementFull = (previousInputPictureFull->originY + lcuOriginY)* previousInputPictureFull->strideY + (previousInputPictureFull->originX + lcuOriginX);

				// 1/16 collocated LCU decimation 
				Decimation2D(
					&previousInputPictureFull->bufferY[blkDisplacementFull],
					previousInputPictureFull->strideY,
					MAX_LCU_SIZE,
					MAX_LCU_SIZE,
					contextPtr->meContextPtr->sixteenthLcuBuffer,
					contextPtr->meContextPtr->sixteenthLcuBufferStride,
					4);

				// ZZ SAD between 1/16 current & 1/16 collocated
				decimatedLcuCollocatedSad = NxMSadKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)][2](
					&(sixteenthDecimatedPicturePtr->bufferY[blkDisplacementDecimated]),
					sixteenthDecimatedPicturePtr->strideY,
					contextPtr->meContextPtr->sixteenthLcuBuffer,
					contextPtr->meContextPtr->sixteenthLcuBufferStride,
					16, 16);

				// Background Enhancement Algorithm
				// Classification is important to:
				// 1. Avoid improving moving objects.
				// 2. Do not modulate when all the picture is background
				// 3. Do give different importance to different regions 
				if (decimatedLcuCollocatedSad < BEA_CLASS_0_0_DEC_TH) {
					previousPictureControlSetWrapperPtr->zzCostArray[lcuIndex] = BEA_CLASS_0_ZZ_COST;
				}
				else if (decimatedLcuCollocatedSad < BEA_CLASS_0_DEC_TH) {
					previousPictureControlSetWrapperPtr->zzCostArray[lcuIndex] = BEA_CLASS_0_1_ZZ_COST;
				}
				else if (decimatedLcuCollocatedSad < BEA_CLASS_1_DEC_TH) {
					previousPictureControlSetWrapperPtr->zzCostArray[lcuIndex] = BEA_CLASS_1_ZZ_COST;
				}
				else if (decimatedLcuCollocatedSad < BEA_CLASS_2_DEC_TH) {
					previousPictureControlSetWrapperPtr->zzCostArray[lcuIndex] = BEA_CLASS_2_ZZ_COST;
				}
				else {
					previousPictureControlSetWrapperPtr->zzCostArray[lcuIndex] = BEA_CLASS_3_ZZ_COST;
				}


			}
			else {
				previousPictureControlSetWrapperPtr->zzCostArray[lcuIndex] = INVALID_ZZ_COST;
				decimatedLcuCollocatedSad = (EB_U32)~0;
			}


			// Keep track of non moving LCUs for QP modulation
			if (decimatedLcuCollocatedSad < ((decimatedLcuWidth * decimatedLcuHeight) * 2)) {
				previousPictureControlSetWrapperPtr->nonMovingIndexArray[lcuIndex] = BEA_CLASS_0_ZZ_COST;
			}
			else if (decimatedLcuCollocatedSad < ((decimatedLcuWidth * decimatedLcuHeight) * 4)) {
				previousPictureControlSetWrapperPtr->nonMovingIndexArray[lcuIndex] = BEA_CLASS_1_ZZ_COST;
			}
			else if (decimatedLcuCollocatedSad < ((decimatedLcuWidth * decimatedLcuHeight) * 8)) {
				previousPictureControlSetWrapperPtr->nonMovingIndexArray[lcuIndex] = BEA_CLASS_2_ZZ_COST;
			}
			else { //if (decimatedLcuCollocatedSad < ((decimatedLcuWidth * decimatedLcuHeight) * 4)) {
				previousPictureControlSetWrapperPtr->nonMovingIndexArray[lcuIndex] = BEA_CLASS_3_ZZ_COST;
			}
		}
	}

	return return_error;
}

/******************************************************
* Derive ME Settings for OQ
  Input   : encoder mode and tune
  Output  : ME Kernel signal(s)
******************************************************/
EB_ERRORTYPE SignalDerivationMeKernelOq(
    SequenceControlSet_t        *sequenceControlSetPtr,
    PictureParentControlSet_t   *pictureControlSetPtr,
    MotionEstimationContext_t   *contextPtr) {

    EB_ERRORTYPE return_error = EB_ErrorNone;

    // Set ME/HME search regions
    SetMeHmeParamsOq(
        contextPtr->meContextPtr,
        pictureControlSetPtr,
        sequenceControlSetPtr,
        sequenceControlSetPtr->inputResolution);
    if (!sequenceControlSetPtr->staticConfig.useDefaultMeHme) {
        SetMeHmeParamsFromConfig(
            sequenceControlSetPtr,
            contextPtr->meContextPtr);
    }

    // Set number of quadrant(s)
    if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
        contextPtr->meContextPtr->oneQuadrantHME = EB_FALSE;
    }
    else {
        if (sequenceControlSetPtr->inputResolution >= INPUT_SIZE_4K_RANGE) {
            contextPtr->meContextPtr->oneQuadrantHME = EB_TRUE;
        }
        else {
            contextPtr->meContextPtr->oneQuadrantHME = EB_FALSE;
        }
    }

    // Set ME Fractional Search Method
    if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
        contextPtr->meContextPtr->fractionalSearchMethod = SSD_SEARCH;
    }
    else {
        contextPtr->meContextPtr->fractionalSearchMethod = SUB_SAD_SEARCH;
    }

    // Set 64x64 Fractional Search Flag
	if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->meContextPtr->fractionalSearch64x64 = EB_TRUE;
	}
	else {
		contextPtr->meContextPtr->fractionalSearch64x64 = EB_FALSE;
	}

    // Set OIS Kernel
	if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
		if (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE) {
			contextPtr->oisKernelLevel = (pictureControlSetPtr->temporalLayerIndex == 0) ? EB_TRUE : EB_FALSE;
		}
		else {
			contextPtr->oisKernelLevel = EB_FALSE;
		}
	}
    else {
        contextPtr->oisKernelLevel = EB_FALSE;
    }
    
    // Set OIS TH
    // 0: Agressive 
    // 1: Default
    // 2: Conservative
    if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
        if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
            if (pictureControlSetPtr->isUsedAsReferenceFlag == EB_TRUE) {
                contextPtr->oisThSet = 2;  
            }
            else {
                contextPtr->oisThSet = 1;  
            }
        }
        else {
            contextPtr->oisThSet = 1;        
        }
    }
    else {
		if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
			contextPtr->oisThSet = 2;
		}
        else {
            contextPtr->oisThSet = 1; 
        }
    }
    
    // Set valid flag for the best OIS
	contextPtr->setBestOisDistortionToValid = EB_FALSE;

    // Set fractional search model
    // 0: search all blocks 
    // 1: selective based on Full-Search SAD & MV.
    // 2: off
    if (pictureControlSetPtr->useSubpelFlag == 1) {
        if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
            contextPtr->meContextPtr->fractionalSearchModel = 0;
        }
		else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
			if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
				contextPtr->meContextPtr->fractionalSearchModel = 1;
			}
			else {
				contextPtr->meContextPtr->fractionalSearchModel = 0;
			}
		}
        else {
            contextPtr->meContextPtr->fractionalSearchModel = 1;
        }
    }
    else {
        contextPtr->meContextPtr->fractionalSearchModel = 2;
    }              

    return return_error;
}


/******************************************************
* GetMv
  Input   : LCU Index
  Output  : List0 MV
******************************************************/
void GetMv(
    PictureParentControlSet_t	*pictureControlSetPtr,
    EB_U32						 lcuIndex,
    EB_S32						*xCurrentMv,
    EB_S32						*yCurrentMv)
{

    MeCuResults_t * cuResults = &pictureControlSetPtr->meResults[lcuIndex][0];

    *xCurrentMv = cuResults->xMvL0;
    *yCurrentMv = cuResults->yMvL0;
}

/******************************************************
* GetMeDist
 Input   : LCU Index
 Output  : Best ME Distortion
******************************************************/
void GetMeDist(
    PictureParentControlSet_t	*pictureControlSetPtr,
    EB_U32						 lcuIndex,
    EB_U32                      *distortion)
{

    *distortion = (EB_U32)(pictureControlSetPtr->meResults[lcuIndex][0].distortionDirection[0].distortion);

}

/******************************************************
* Derive Similar Collocated Flag
******************************************************/
static void DeriveSimilarCollocatedFlag(
    PictureParentControlSet_t    *pictureControlSetPtr,
    EB_U32	                      lcuIndex)
{
   // Similairty detector for collocated LCU
   pictureControlSetPtr->similarColocatedLcuArray[lcuIndex] = EB_FALSE;

   // Similairty detector for collocated LCU -- all layers
   pictureControlSetPtr->similarColocatedLcuArrayAllLayers[lcuIndex] = EB_FALSE;

   if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {

       EB_U8                   refMean, curMean;
       EB_U16                  refVar, curVar;

       EbPaReferenceObject_t    *refObjL0;

       refObjL0 = (EbPaReferenceObject_t*)pictureControlSetPtr->refPaPicPtrArray[REF_LIST_0]->objectPtr;
       refMean = refObjL0->yMean[lcuIndex];

       refVar = refObjL0->variance[lcuIndex];

       curMean = pictureControlSetPtr->yMean[lcuIndex][RASTER_SCAN_CU_INDEX_64x64];

       curVar = pictureControlSetPtr->variance[lcuIndex][RASTER_SCAN_CU_INDEX_64x64];

       refVar = MAX(refVar, 1);
       if ((ABS((EB_S64)curMean - (EB_S64)refMean) < MEAN_DIFF_THRSHOLD) &&
           ((ABS((EB_S64)curVar * 100 / (EB_S64)refVar - 100) < VAR_DIFF_THRSHOLD) || (ABS((EB_S64)curVar - (EB_S64)refVar) < VAR_DIFF_THRSHOLD))) {

           if (pictureControlSetPtr->isUsedAsReferenceFlag) {
               pictureControlSetPtr->similarColocatedLcuArray[lcuIndex] = EB_TRUE;
           }
           pictureControlSetPtr->similarColocatedLcuArrayAllLayers[lcuIndex] = EB_TRUE;
       }
   }

    return;
}

static void StationaryEdgeOverUpdateOverTimeLcuPart1(
    SequenceControlSet_t        *sequenceControlSetPtr,
    PictureParentControlSet_t   *pictureControlSetPtr,
    EB_U32                       lcuIndex)
{
    EB_S32	             xCurrentMv = 0;
    EB_S32	             yCurrentMv = 0;

    LcuParams_t *lcuParams  = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
    LcuStat_t   *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

    if (lcuParams->potentialLogoLcu && lcuParams->isCompleteLcu) {

        // Current MV   
        if (pictureControlSetPtr->temporalLayerIndex > 0)
            GetMv(pictureControlSetPtr, lcuIndex, &xCurrentMv, &yCurrentMv);

        EB_BOOL lowMotion = pictureControlSetPtr->temporalLayerIndex == 0 ? EB_TRUE : (ABS(xCurrentMv) < 16) && (ABS(yCurrentMv) < 16) ? EB_TRUE : EB_FALSE;
        EB_U16 *yVariancePtr = pictureControlSetPtr->variance[lcuIndex];
        EB_U64 var0 = yVariancePtr[ME_TIER_ZERO_PU_32x32_0];
        EB_U64 var1 = yVariancePtr[ME_TIER_ZERO_PU_32x32_1];
        EB_U64 var2 = yVariancePtr[ME_TIER_ZERO_PU_32x32_2];
        EB_U64 var3 = yVariancePtr[ME_TIER_ZERO_PU_32x32_3];

        EB_U64 averageVar = (var0 + var1 + var2 + var3) >> 2;
        EB_U64 varOfVar = (((EB_S32)(var0 - averageVar) * (EB_S32)(var0 - averageVar)) +
            ((EB_S32)(var1 - averageVar) * (EB_S32)(var1 - averageVar)) +
            ((EB_S32)(var2 - averageVar) * (EB_S32)(var2 - averageVar)) +
            ((EB_S32)(var3 - averageVar) * (EB_S32)(var3 - averageVar))) >> 2;

        if ((varOfVar <= 50000) || !lowMotion) {
            lcuStatPtr->check1ForLogoStationaryEdgeOverTimeFlag = 0;
        }
        else {
            lcuStatPtr->check1ForLogoStationaryEdgeOverTimeFlag = 1;
        }

        if ((varOfVar <= 1000)) {
            lcuStatPtr->pmCheck1ForLogoStationaryEdgeOverTimeFlag = 0;
        }
        else {
            lcuStatPtr->pmCheck1ForLogoStationaryEdgeOverTimeFlag = 1;
        }
    }
    else {
        lcuStatPtr->check1ForLogoStationaryEdgeOverTimeFlag = 0;

        lcuStatPtr->pmCheck1ForLogoStationaryEdgeOverTimeFlag = 0;

    }
}

static void StationaryEdgeOverUpdateOverTimeLcuPart2(
    SequenceControlSet_t        *sequenceControlSetPtr,
    PictureParentControlSet_t   *pictureControlSetPtr,
    EB_U32                       lcuIndex)
{
    EB_U32               lowSadTh = (sequenceControlSetPtr->inputResolution < INPUT_SIZE_1080p_RANGE) ? 5 : 2;

    LcuParams_t  *lcuParams  = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
    LcuStat_t    *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];
    
    if (lcuParams->potentialLogoLcu && lcuParams->isCompleteLcu) {
        EB_U32 meDist = 0;
    
        EB_BOOL lowSad = EB_FALSE;
    
        if (pictureControlSetPtr->sliceType == EB_B_PICTURE) {
            GetMeDist(pictureControlSetPtr, lcuIndex, &meDist);
        }
        lowSad = (pictureControlSetPtr->sliceType != EB_B_PICTURE) ?
    
            EB_FALSE : (meDist < 64 * 64 * lowSadTh) ? EB_TRUE : EB_FALSE;
    
        if (lowSad) {
            lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag = 0;
            lcuStatPtr->lowDistLogo = 1;
        }
        else {
            lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag = 1;
    
            lcuStatPtr->lowDistLogo = 0;
        }
    }
    else {
        lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag = 0;
    
        lcuStatPtr->lowDistLogo = 0;
    }
    lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag = 1;
    
}

/************************************************
 * Motion Analysis Kernel
 * The Motion Analysis performs  Motion Estimation
 * This process has access to the current input picture as well as
 * the input pictures, which the current picture references according
 * to the prediction structure pattern.  The Motion Analysis process is multithreaded,
 * so pictures can be processed out of order as long as all inputs are available.
 ************************************************/
void* MotionEstimationKernel(void *inputPtr)
{
	MotionEstimationContext_t   *contextPtr = (MotionEstimationContext_t*)inputPtr;

	PictureParentControlSet_t   *pictureControlSetPtr;
	SequenceControlSet_t        *sequenceControlSetPtr;

	EbObjectWrapper_t           *inputResultsWrapperPtr;
	PictureDecisionResults_t    *inputResultsPtr;

	EbObjectWrapper_t           *outputResultsWrapperPtr;
	MotionEstimationResults_t   *outputResultsPtr;

	EbPictureBufferDesc_t       *inputPicturePtr;

    EbPictureBufferDesc_t       *inputPaddedPicturePtr;

	EB_U32                       bufferIndex;

	EB_U32                       lcuIndex;
	EB_U32                       xLcuIndex;
	EB_U32                       yLcuIndex;
	EB_U32                       pictureWidthInLcu;
	EB_U32                       pictureHeightInLcu;
	EB_U32                       lcuOriginX;
	EB_U32                       lcuOriginY;
	EB_U32                       lcuWidth;
	EB_U32                       lcuHeight;
	EB_U32                       lcuRow;



	EbPaReferenceObject_t       *paReferenceObject;
	EbPictureBufferDesc_t       *quarterDecimatedPicturePtr;
	EbPictureBufferDesc_t       *sixteenthDecimatedPicturePtr;

	// Segments
	EB_U32                      segmentIndex;
	EB_U32                      xSegmentIndex;
	EB_U32                      ySegmentIndex;
	EB_U32                      xLcuStartIndex;
	EB_U32                      xLcuEndIndex;
	EB_U32                      yLcuStartIndex;
	EB_U32                      yLcuEndIndex;

	EB_U32                      intraSadIntervalIndex;

	MdRateEstimationContext_t   *mdRateEstimationArray;


	for (;;) {


		// Get Input Full Object
		EbGetFullObject(
			contextPtr->pictureDecisionResultsInputFifoPtr,
			&inputResultsWrapperPtr);
        EB_CHECK_END_OBJ(inputResultsWrapperPtr);

		inputResultsPtr = (PictureDecisionResults_t*)inputResultsWrapperPtr->objectPtr;
		pictureControlSetPtr = (PictureParentControlSet_t*)inputResultsPtr->pictureControlSetWrapperPtr->objectPtr;
		sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
		paReferenceObject = (EbPaReferenceObject_t*)pictureControlSetPtr->paReferencePictureWrapperPtr->objectPtr;
		quarterDecimatedPicturePtr = (EbPictureBufferDesc_t*)paReferenceObject->quarterDecimatedPicturePtr;
		sixteenthDecimatedPicturePtr = (EbPictureBufferDesc_t*)paReferenceObject->sixteenthDecimatedPicturePtr;
        inputPaddedPicturePtr = (EbPictureBufferDesc_t*)paReferenceObject->inputPaddedPicturePtr;
		inputPicturePtr = pictureControlSetPtr->enhancedPicturePtr;
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld ME IN \n", pictureControlSetPtr->pictureNumber);
#endif
		// Segments
		segmentIndex = inputResultsPtr->segmentIndex;
		pictureWidthInLcu = (sequenceControlSetPtr->lumaWidth + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;
		pictureHeightInLcu = (sequenceControlSetPtr->lumaHeight + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;
		SEGMENT_CONVERT_IDX_TO_XY(segmentIndex, xSegmentIndex, ySegmentIndex, pictureControlSetPtr->meSegmentsColumnCount);
		xLcuStartIndex = SEGMENT_START_IDX(xSegmentIndex, pictureWidthInLcu, pictureControlSetPtr->meSegmentsColumnCount);
		xLcuEndIndex = SEGMENT_END_IDX(xSegmentIndex, pictureWidthInLcu, pictureControlSetPtr->meSegmentsColumnCount);
		yLcuStartIndex = SEGMENT_START_IDX(ySegmentIndex, pictureHeightInLcu, pictureControlSetPtr->meSegmentsRowCount);
		yLcuEndIndex = SEGMENT_END_IDX(ySegmentIndex, pictureHeightInLcu, pictureControlSetPtr->meSegmentsRowCount);
		// Increment the MD Rate Estimation array pointer to point to the right address based on the QP and slice type 
		mdRateEstimationArray = (MdRateEstimationContext_t*)sequenceControlSetPtr->encodeContextPtr->mdRateEstimationArray;
		mdRateEstimationArray += pictureControlSetPtr->sliceType * TOTAL_NUMBER_OF_QP_VALUES + pictureControlSetPtr->pictureQp;
		// Reset MD rate Estimation table to initial values by copying from mdRateEstimationArray
		EB_MEMCPY(&(contextPtr->meContextPtr->mvdBitsArray[0]), &(mdRateEstimationArray->mvdBits[0]), sizeof(EB_BitFraction)*NUMBER_OF_MVD_CASES);

        SignalDerivationMeKernelOq(
                sequenceControlSetPtr,
                pictureControlSetPtr,
                contextPtr);     

		// Lambda Assignement
        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            contextPtr->meContextPtr->lambda = lambdaModeDecisionRaSadBase[pictureControlSetPtr->pictureQp];
        }
        else if (pictureControlSetPtr->isUsedAsReferenceFlag) {
            contextPtr->meContextPtr->lambda = lambdaModeDecisionRaSadRefNonBase[pictureControlSetPtr->pictureQp];
        }
        else {
            contextPtr->meContextPtr->lambda = lambdaModeDecisionRaSadNonRef[pictureControlSetPtr->pictureQp];
        }

        // Motion Estimation
        if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {

            // LCU Loop
            for (yLcuIndex = yLcuStartIndex; yLcuIndex < yLcuEndIndex; ++yLcuIndex) {
                for (xLcuIndex = xLcuStartIndex; xLcuIndex < xLcuEndIndex; ++xLcuIndex) {

                    lcuIndex = (EB_U16)(xLcuIndex + yLcuIndex * pictureWidthInLcu);
                    lcuOriginX = xLcuIndex * sequenceControlSetPtr->lcuSize;
                    lcuOriginY = yLcuIndex * sequenceControlSetPtr->lcuSize;

                    lcuWidth = (sequenceControlSetPtr->lumaWidth - lcuOriginX) < MAX_LCU_SIZE ? sequenceControlSetPtr->lumaWidth - lcuOriginX : MAX_LCU_SIZE;
                    lcuHeight = (sequenceControlSetPtr->lumaHeight - lcuOriginY) < MAX_LCU_SIZE ? sequenceControlSetPtr->lumaHeight - lcuOriginY : MAX_LCU_SIZE;

                    // Load the LCU from the input to the intermediate LCU buffer
                    bufferIndex = (inputPicturePtr->originY + lcuOriginY) * inputPicturePtr->strideY + inputPicturePtr->originX + lcuOriginX;

                    contextPtr->meContextPtr->hmeSearchType = HME_RECTANGULAR;

                    for (lcuRow = 0; lcuRow < MAX_LCU_SIZE; lcuRow++) {
                        EB_MEMCPY((&(contextPtr->meContextPtr->lcuBuffer[lcuRow * MAX_LCU_SIZE])), (&(inputPicturePtr->bufferY[bufferIndex + lcuRow * inputPicturePtr->strideY])), MAX_LCU_SIZE * sizeof(EB_U8));

                    }

                    EB_U8 * srcPtr = &inputPaddedPicturePtr->bufferY[bufferIndex];

                    //_MM_HINT_T0 	//_MM_HINT_T1	//_MM_HINT_T2//_MM_HINT_NTA
                    EB_U32 i;
                    for (i = 0; i < lcuHeight; i++)
                    {
                        char const* p = (char const*)(srcPtr + i*inputPaddedPicturePtr->strideY);
                        _mm_prefetch(p, _MM_HINT_T2);
                    }


                    contextPtr->meContextPtr->lcuSrcPtr = &inputPaddedPicturePtr->bufferY[bufferIndex];
                    contextPtr->meContextPtr->lcuSrcStride = inputPaddedPicturePtr->strideY;


                    // Load the 1/4 decimated LCU from the 1/4 decimated input to the 1/4 intermediate LCU buffer
                    if (pictureControlSetPtr->enableHmeLevel1Flag) {

                        bufferIndex = (quarterDecimatedPicturePtr->originY + (lcuOriginY >> 1)) * quarterDecimatedPicturePtr->strideY + quarterDecimatedPicturePtr->originX + (lcuOriginX >> 1);

                        for (lcuRow = 0; lcuRow < (lcuHeight >> 1); lcuRow++) {
                            EB_MEMCPY((&(contextPtr->meContextPtr->quarterLcuBuffer[lcuRow * contextPtr->meContextPtr->quarterLcuBufferStride])), (&(quarterDecimatedPicturePtr->bufferY[bufferIndex + lcuRow * quarterDecimatedPicturePtr->strideY])), (lcuWidth >> 1) * sizeof(EB_U8));

                        }
                    }

                    // Load the 1/16 decimated LCU from the 1/16 decimated input to the 1/16 intermediate LCU buffer
                    if (pictureControlSetPtr->enableHmeLevel0Flag) {

                        bufferIndex = (sixteenthDecimatedPicturePtr->originY + (lcuOriginY >> 2)) * sixteenthDecimatedPicturePtr->strideY + sixteenthDecimatedPicturePtr->originX + (lcuOriginX >> 2);

                        {
                            EB_U8  *framePtr = &sixteenthDecimatedPicturePtr->bufferY[bufferIndex];
                            EB_U8  *localPtr = contextPtr->meContextPtr->sixteenthLcuBuffer;

                            for (lcuRow = 0; lcuRow < (lcuHeight >> 2); lcuRow += 2) {
                                EB_MEMCPY(localPtr, framePtr, (lcuWidth >> 2) * sizeof(EB_U8));
                                localPtr += 16;
                                framePtr += sixteenthDecimatedPicturePtr->strideY << 1;
                            }
                        }
                    }

                    MotionEstimateLcu(
                        pictureControlSetPtr,
                        lcuIndex,
                        lcuOriginX,
                        lcuOriginY,
                        contextPtr->meContextPtr,
                        inputPicturePtr);
                }
            }
        }

	    // OIS + Similar Collocated Checks + Stationary Edge Over Time Check
        // LCU Loop
		for (yLcuIndex = yLcuStartIndex; yLcuIndex < yLcuEndIndex; ++yLcuIndex) {
			for (xLcuIndex = xLcuStartIndex; xLcuIndex < xLcuEndIndex; ++xLcuIndex) {

				lcuOriginX = xLcuIndex * sequenceControlSetPtr->lcuSize;
				lcuOriginY = yLcuIndex * sequenceControlSetPtr->lcuSize;
                lcuIndex = (EB_U16)(xLcuIndex + yLcuIndex * pictureWidthInLcu);

				OpenLoopIntraSearchLcu(
					pictureControlSetPtr,
					lcuIndex,
					contextPtr,
					inputPicturePtr);

                // Derive Similar Collocated Flag
                DeriveSimilarCollocatedFlag(
                    pictureControlSetPtr,
                    lcuIndex);

                //Check conditions for stationary edge over time Part 1
                StationaryEdgeOverUpdateOverTimeLcuPart1(
                    sequenceControlSetPtr,
                    pictureControlSetPtr,
                    lcuIndex);

                //Check conditions for stationary edge over time Part 2
                if (!pictureControlSetPtr->endOfSequenceFlag && sequenceControlSetPtr->staticConfig.lookAheadDistance != 0) {
                    StationaryEdgeOverUpdateOverTimeLcuPart2(
                        sequenceControlSetPtr,
                        pictureControlSetPtr,
                        lcuIndex);
                }
			}
		}

		// ZZ SADs Computation
		// 1 lookahead frame is needed to get valid (0,0) SAD
		if (sequenceControlSetPtr->staticConfig.lookAheadDistance != 0) {
			// when DG is ON, the ZZ SADs are computed @ the PD process
			{
				// ZZ SADs Computation using decimated picture
				if (pictureControlSetPtr->pictureNumber > 0) {

                    ComputeDecimatedZzSad(
                        contextPtr,
                        sequenceControlSetPtr,
                        pictureControlSetPtr,
                        sixteenthDecimatedPicturePtr,
                        xLcuStartIndex,
                        xLcuEndIndex,
                        yLcuStartIndex,
                        yLcuEndIndex);
					
				}
			}
		}


		// Calculate the ME Distortion and OIS Historgrams
        EbBlockOnMutex(pictureControlSetPtr->rcDistortionHistogramMutex);
		if (sequenceControlSetPtr->staticConfig.rateControlMode){
			if (pictureControlSetPtr->sliceType != EB_I_PICTURE){
				EB_U16 sadIntervalIndex;
				for (yLcuIndex = yLcuStartIndex; yLcuIndex < yLcuEndIndex; ++yLcuIndex) {
					for (xLcuIndex = xLcuStartIndex; xLcuIndex < xLcuEndIndex; ++xLcuIndex) {

						lcuOriginX = xLcuIndex * sequenceControlSetPtr->lcuSize;
						lcuOriginY = yLcuIndex * sequenceControlSetPtr->lcuSize;
						lcuWidth = (sequenceControlSetPtr->lumaWidth - lcuOriginX) < MAX_LCU_SIZE ? sequenceControlSetPtr->lumaWidth - lcuOriginX : MAX_LCU_SIZE;
						lcuHeight = (sequenceControlSetPtr->lumaHeight - lcuOriginY) < MAX_LCU_SIZE ? sequenceControlSetPtr->lumaHeight - lcuOriginY : MAX_LCU_SIZE;

                        lcuIndex = (EB_U16)(xLcuIndex + yLcuIndex * pictureWidthInLcu);                         
                        pictureControlSetPtr->interSadIntervalIndex[lcuIndex] = 0;
                        pictureControlSetPtr->intraSadIntervalIndex[lcuIndex] = 0;

						if (lcuWidth == MAX_LCU_SIZE && lcuHeight == MAX_LCU_SIZE) {


							sadIntervalIndex = (EB_U16)(pictureControlSetPtr->rcMEdistortion[lcuIndex] >> (12 - SAD_PRECISION_INTERVAL));//change 12 to 2*log2(64) 

                            sadIntervalIndex = (EB_U16)(sadIntervalIndex >> 2);
                            if (sadIntervalIndex > (NUMBER_OF_SAD_INTERVALS>>1) -1){
                                EB_U16 sadIntervalIndexTemp = sadIntervalIndex - ((NUMBER_OF_SAD_INTERVALS >> 1) - 1);

                                sadIntervalIndex = ((NUMBER_OF_SAD_INTERVALS >> 1) - 1) + (sadIntervalIndexTemp >> 3);

                            }
                            if (sadIntervalIndex >= NUMBER_OF_SAD_INTERVALS - 1)
                                sadIntervalIndex = NUMBER_OF_SAD_INTERVALS - 1;
                  

                         
                            pictureControlSetPtr->interSadIntervalIndex[lcuIndex] = sadIntervalIndex;   
                            pictureControlSetPtr->meDistortionHistogram[sadIntervalIndex] ++;
							
                            EB_U32                       bestOisCuIndex = 0;

							//DOUBLE CHECK THIS PIECE OF CODE
                            intraSadIntervalIndex = (EB_U32)
								(((pictureControlSetPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[1][bestOisCuIndex].distortion +
								pictureControlSetPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[2][bestOisCuIndex].distortion +
								pictureControlSetPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[3][bestOisCuIndex].distortion +
								pictureControlSetPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[4][bestOisCuIndex].distortion)) >> (12 - SAD_PRECISION_INTERVAL));//change 12 to 2*log2(64) ;

                            intraSadIntervalIndex = (EB_U16)(intraSadIntervalIndex >> 2);
                            if (intraSadIntervalIndex > (NUMBER_OF_SAD_INTERVALS >> 1) - 1){
                                EB_U32 sadIntervalIndexTemp = intraSadIntervalIndex - ((NUMBER_OF_SAD_INTERVALS >> 1) - 1);

                                intraSadIntervalIndex = ((NUMBER_OF_SAD_INTERVALS >> 1) - 1) + (sadIntervalIndexTemp >> 3);

                            }
                            if (intraSadIntervalIndex >= NUMBER_OF_SAD_INTERVALS - 1)
                                intraSadIntervalIndex = NUMBER_OF_SAD_INTERVALS - 1;

                   
                            pictureControlSetPtr->intraSadIntervalIndex[lcuIndex] = intraSadIntervalIndex; 
                            pictureControlSetPtr->oisDistortionHistogram[intraSadIntervalIndex] ++; 




							++pictureControlSetPtr->fullLcuCount;
						}

					}
				}
			}
			else{
				EB_U32                       bestOisCuIndex = 0;


				for (yLcuIndex = yLcuStartIndex; yLcuIndex < yLcuEndIndex; ++yLcuIndex) {
					for (xLcuIndex = xLcuStartIndex; xLcuIndex < xLcuEndIndex; ++xLcuIndex) {
						lcuOriginX = xLcuIndex * sequenceControlSetPtr->lcuSize;
						lcuOriginY = yLcuIndex * sequenceControlSetPtr->lcuSize;
						lcuWidth = (sequenceControlSetPtr->lumaWidth - lcuOriginX) < MAX_LCU_SIZE ? sequenceControlSetPtr->lumaWidth - lcuOriginX : MAX_LCU_SIZE;
						lcuHeight = (sequenceControlSetPtr->lumaHeight - lcuOriginY) < MAX_LCU_SIZE ? sequenceControlSetPtr->lumaHeight - lcuOriginY : MAX_LCU_SIZE;

                        lcuIndex = (EB_U16)(xLcuIndex + yLcuIndex * pictureWidthInLcu);
                       
                        pictureControlSetPtr->interSadIntervalIndex[lcuIndex] = 0;
                        pictureControlSetPtr->intraSadIntervalIndex[lcuIndex] = 0;

						if (lcuWidth == MAX_LCU_SIZE && lcuHeight == MAX_LCU_SIZE) {


							//DOUBLE CHECK THIS PIECE OF CODE
						
							intraSadIntervalIndex = (EB_U32)
								(((pictureControlSetPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[1][bestOisCuIndex].distortion +
								pictureControlSetPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[2][bestOisCuIndex].distortion +
								pictureControlSetPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[3][bestOisCuIndex].distortion +
								pictureControlSetPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[4][bestOisCuIndex].distortion)) >> (12 - SAD_PRECISION_INTERVAL));//change 12 to 2*log2(64) ;
 
                            intraSadIntervalIndex = (EB_U16)(intraSadIntervalIndex >> 2);
                            if (intraSadIntervalIndex > (NUMBER_OF_SAD_INTERVALS >> 1) - 1){
                                EB_U32 sadIntervalIndexTemp = intraSadIntervalIndex - ((NUMBER_OF_SAD_INTERVALS >> 1) - 1);

                                intraSadIntervalIndex = ((NUMBER_OF_SAD_INTERVALS >> 1) - 1) + (sadIntervalIndexTemp >> 3);

                            }
                            if (intraSadIntervalIndex >= NUMBER_OF_SAD_INTERVALS - 1)
                                intraSadIntervalIndex = NUMBER_OF_SAD_INTERVALS - 1;

                            pictureControlSetPtr->intraSadIntervalIndex[lcuIndex] = intraSadIntervalIndex;
							pictureControlSetPtr->oisDistortionHistogram[intraSadIntervalIndex] ++;
							++pictureControlSetPtr->fullLcuCount;
						}

					}
				}
			}
		}
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld ME OUT \n", pictureControlSetPtr->pictureNumber);
#endif
        EbReleaseMutex(pictureControlSetPtr->rcDistortionHistogramMutex);
		// Get Empty Results Object
		EbGetEmptyObject(
			contextPtr->motionEstimationResultsOutputFifoPtr,
			&outputResultsWrapperPtr);

		outputResultsPtr = (MotionEstimationResults_t*)outputResultsWrapperPtr->objectPtr;
		outputResultsPtr->pictureControlSetWrapperPtr = inputResultsPtr->pictureControlSetWrapperPtr;
		outputResultsPtr->segmentIndex = segmentIndex;

		// Release the Input Results
		EbReleaseObject(inputResultsWrapperPtr);

		// Post the Full Results Object
		EbPostFullObject(outputResultsWrapperPtr);
	}
	return EB_NULL;
}
