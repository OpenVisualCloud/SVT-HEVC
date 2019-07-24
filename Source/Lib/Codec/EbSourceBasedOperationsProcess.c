/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbPictureBufferDesc.h"

#include "EbSourceBasedOperationsProcess.h"
#include "EbInitialRateControlResults.h"
#include "EbPictureDemuxResults.h"
#include "EbPictureOperators.h"
#include "EbMotionEstimationContext.h"
#include "emmintrin.h"
/**************************************
* Macros
**************************************/

#define VARIANCE_PRECISION               16
#define PAN_LCU_PERCENTAGE               75
#define LOW_AMPLITUDE_TH                 16

#define Y_MEAN_RANGE_03					 52
#define Y_MEAN_RANGE_02                  70
#define Y_MEAN_RANGE_01                 130
#define CB_MEAN_RANGE_02                115
#define CR_MEAN_RANGE_00                110
#define CR_MEAN_RANGE_02                135

#define DARK_FRM_TH                      45
#define CB_MEAN_RANGE_00				 80


#define SAD_DEVIATION_LCU_TH_0           15
#define SAD_DEVIATION_LCU_TH_1           20

#define MAX_DELTA_QP_SHAPE_TH			  4
#define MIN_DELTA_QP_SHAPE_TH             1

#define MIN_BLACK_AREA_PERCENTAGE        20
#define LOW_MEAN_TH_0                    25 

#define MIN_WHITE_AREA_PERCENTAGE         1
#define LOW_MEAN_TH_1                    40 
#define HIGH_MEAN_TH                    210
#define NORM_FACTOR                      10 // Used ComplexityClassifier32x32

const EB_U32    THRESHOLD_NOISE[MAX_TEMPORAL_LAYERS] = { 33, 28, 27, 26, 26, 26 }; // [Temporal Layer Index]  // Used ComplexityClassifier32x32
// Outlier removal threshold per depth {2%, 2%, 4%, 4%}
const EB_S8  MinDeltaQPdefault[3] = {
	-4, -3, -2
};
const EB_U8 MaxDeltaQPdefault[3] = {
	4, 5, 6
};

/************************************************
* Initial Rate Control Context Constructor
************************************************/

EB_ERRORTYPE SourceBasedOperationsContextCtor(
    SourceBasedOperationsContext_t **contextDblPtr,
    EbFifo_t						*initialRateControlResultsInputFifoPtr,
    EbFifo_t						*pictureDemuxResultsOutputFifoPtr)
{
	SourceBasedOperationsContext_t *contextPtr;

	EB_MALLOC(SourceBasedOperationsContext_t*, contextPtr, sizeof(SourceBasedOperationsContext_t), EB_N_PTR);
	*contextDblPtr = contextPtr;
	contextPtr->initialrateControlResultsInputFifoPtr = initialRateControlResultsInputFifoPtr;
	contextPtr->pictureDemuxResultsOutputFifoPtr = pictureDemuxResultsOutputFifoPtr;

	return EB_ErrorNone;
}

/***************************************************
* Derives BEA statistics and set activity flags
***************************************************/

void DerivePictureActivityStatistics(
	SequenceControlSet_t            *sequenceControlSetPtr,
	PictureParentControlSet_t       *pictureControlSetPtr)

{

	EB_U64               nonMovingIndexSum      = 0;
	
	EB_U32               lcuIndex;

    EB_U32 zzSum                                = 0;
    EB_U32 completeLcuCount                     = 0;


    for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

        LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

        if (lcuParams->isCompleteLcu) {

            nonMovingIndexSum += pictureControlSetPtr->nonMovingIndexArray[lcuIndex];
            zzSum             += pictureControlSetPtr->zzCostArray[lcuIndex];

            completeLcuCount++;
        }

	}

    if (completeLcuCount > 0) {
        pictureControlSetPtr->nonMovingIndexAverage = (EB_U16)(nonMovingIndexSum / completeLcuCount);
        pictureControlSetPtr->zzCostAverage = zzSum / completeLcuCount;
    }
    pictureControlSetPtr->lowMotionContentFlag  = pictureControlSetPtr->zzCostAverage == 0 ? EB_TRUE : EB_FALSE;

	return;
}
/***************************************************
* complexity Classification
***************************************************/
void ComplexityClassifier32x32(
    SequenceControlSet_t      *sequenceControlSetPtr,
    PictureParentControlSet_t *pictureControlSetPtr) {


    //No need to have Threshold depending on TempLayer. Higher Temoral Layer would have smaller classes
    //TODO: add more classes using above threshold when needed.


    EB_U32          lcuIndex;
    LcuParams_t     *lcuParams;
    EB_U8           noiseClass;

    for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {


        pictureControlSetPtr->cmplxStatusLcu[lcuIndex] = CMPLX_LOW;

        if (pictureControlSetPtr->temporalLayerIndex >= 1 && pictureControlSetPtr->sliceType == EB_B_PICTURE) {

            lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];


            if (lcuParams->isCompleteLcu) {

                noiseClass = CMPLX_LOW;

                EB_U32         blkIt;

                for (blkIt = 0; blkIt < 4; blkIt++) {


                    EB_U32 distortion = pictureControlSetPtr->meResults[lcuIndex][1 + blkIt].distortionDirection[0].distortion;

                    if ((((EB_U32)(distortion)) >> NORM_FACTOR) > THRESHOLD_NOISE[pictureControlSetPtr->temporalLayerIndex])


                        noiseClass++;
                }


                pictureControlSetPtr->cmplxStatusLcu[lcuIndex] = noiseClass > 0 ? CMPLX_NOISE : CMPLX_LOW;
            }
        }
    }
}





/******************************************************
* Pre-MD Uncovered Area Detection
******************************************************/
void FailingMotionLcu(
    SequenceControlSet_t			*sequenceControlSetPtr,
    PictureParentControlSet_t		*pictureControlSetPtr,
    EB_U32							 lcuIndex) {

    EB_U32 rasterScanCuIndex;

    // LCU Loop : Failing motion detector for L2 only
    LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
    // Detection variables
    EB_U64                  sortedcuOisSAD = 0;
    EB_U64                  cuMeSAD = 0;
    EB_S64                  meToOisSadDeviation = 0;
    // LCU loop variables

    EB_S64 failingMotionLcuFlag = 0;

    if (pictureControlSetPtr->sliceType != EB_I_PICTURE && lcuParams->isCompleteLcu && (!pictureControlSetPtr->similarColocatedLcuArray[lcuIndex])){
        for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_64x64; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_32x32_3; rasterScanCuIndex++) {

            meToOisSadDeviation = 0;

            // Get ME SAD

			cuMeSAD = pictureControlSetPtr->meResults[lcuIndex][rasterScanCuIndex].distortionDirection[0].distortion;




			OisCu32Cu16Results_t *oisResultsPtr = pictureControlSetPtr->oisCu32Cu16Results[lcuIndex];
			if (RASTER_SCAN_CU_SIZE[rasterScanCuIndex] > 32){
				sortedcuOisSAD = oisResultsPtr->sortedOisCandidate[1][0].distortion +
					oisResultsPtr->sortedOisCandidate[2][0].distortion +
					oisResultsPtr->sortedOisCandidate[3][0].distortion +
					oisResultsPtr->sortedOisCandidate[4][0].distortion;
			}
			else { //32x32
				sortedcuOisSAD = oisResultsPtr->sortedOisCandidate[rasterScanCuIndex][0].distortion;
			}

      

            EB_S64  meToOisSadDiff = (EB_S32)cuMeSAD - (EB_S32)sortedcuOisSAD;
            meToOisSadDeviation = (sortedcuOisSAD == 0) || (meToOisSadDiff < 0) ? 0 : (meToOisSadDiff * 100) / sortedcuOisSAD;

            if (meToOisSadDeviation > SAD_DEVIATION_LCU_TH_0){
                failingMotionLcuFlag += 1;
            }
        }

        // Update failing motion flag
        pictureControlSetPtr->failingMotionLcuFlag[lcuIndex] = failingMotionLcuFlag ? EB_TRUE : EB_FALSE;
    }

}

/******************************************************
* Pre-MD Uncovered Area Detection
******************************************************/
void DetectUncoveredLcu(
	SequenceControlSet_t			*sequenceControlSetPtr,
	PictureParentControlSet_t		*pictureControlSetPtr,
	EB_U32							 lcuIndex) {

	EB_U32 rasterScanCuIndex;

	// LCU Loop : Uncovered area detector -- ON only for 4k
    LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
	// Detection variables
	EB_U64                  sortedcuOisSAD = 0;
	EB_U64                  cuMeSAD = 0;
	EB_S64                  meToOisSadDeviation = 0;
	// LCU loop variables

	EB_S64 uncoveredAreaLcuFlag = 0;


	if (pictureControlSetPtr->temporalLayerIndex == 0 && pictureControlSetPtr->sliceType != EB_I_PICTURE){
        if (lcuParams->isCompleteLcu && (!pictureControlSetPtr->similarColocatedLcuArray[lcuIndex])){


			for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_64x64; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_32x32_3; rasterScanCuIndex++) {


				meToOisSadDeviation = 0;

				// Get ME SAD

				cuMeSAD = pictureControlSetPtr->meResults[lcuIndex][rasterScanCuIndex].distortionDirection[0].distortion;




					OisCu32Cu16Results_t *oisResultsPtr = pictureControlSetPtr->oisCu32Cu16Results[lcuIndex];
					if (RASTER_SCAN_CU_SIZE[rasterScanCuIndex] > 32){
						sortedcuOisSAD = oisResultsPtr->sortedOisCandidate[1][0].distortion +
							oisResultsPtr->sortedOisCandidate[2][0].distortion +
							oisResultsPtr->sortedOisCandidate[3][0].distortion +
							oisResultsPtr->sortedOisCandidate[4][0].distortion;
					}
					else if (RASTER_SCAN_CU_SIZE[rasterScanCuIndex] == 32) {
						sortedcuOisSAD = oisResultsPtr->sortedOisCandidate[rasterScanCuIndex][0].distortion;
					}
					else {
						sortedcuOisSAD = oisResultsPtr->sortedOisCandidate[rasterScanCuIndex][0].distortion;
					}

       

				EB_S64  meToOisSadDiff = (EB_S32)cuMeSAD - (EB_S32)sortedcuOisSAD;
				meToOisSadDeviation = (sortedcuOisSAD == 0) || (meToOisSadDiff < 0) ? 0 : (meToOisSadDiff * 100) / sortedcuOisSAD;


				if (RASTER_SCAN_CU_SIZE[rasterScanCuIndex] > 16){

                    if (meToOisSadDeviation > SAD_DEVIATION_LCU_TH_1){
						uncoveredAreaLcuFlag += 1;
					}

				}
			}

			// Update Uncovered area flag
			pictureControlSetPtr->uncoveredAreaLcuFlag[lcuIndex] = uncoveredAreaLcuFlag ? EB_TRUE : EB_FALSE;
		}

	}
}


/******************************************************
* Calculates AC Energy
******************************************************/
void CalculateAcEnergy(
	SequenceControlSet_t	        *sequenceControlSetPtr,
	PictureParentControlSet_t		*pictureControlSetPtr,
	EB_U32							 lcuIndex) {

	EbPictureBufferDesc_t	*inputPicturePtr = pictureControlSetPtr->enhancedPicturePtr;
	EB_U32					 inputLumaStride = inputPicturePtr->strideY;
	EB_U32                   inputOriginIndex;
    LcuParams_t  *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

	EB_U8       *meanPtr = pictureControlSetPtr->yMean[lcuIndex];
	inputOriginIndex = (lcuParams->originY + inputPicturePtr->originY) * inputLumaStride + (lcuParams->originX + inputPicturePtr->originX);

	if (lcuParams->isCompleteLcu && pictureControlSetPtr->sliceType == EB_I_PICTURE){

		EB_U32 inputCuOriginIndex;
		EB_U32 cuNum, cuSize;
		EB_U16 cuH, cuW;


		pictureControlSetPtr->lcuYSrcEnergyCuArray[lcuIndex][0] = ComputeNxMSatdSadLCU(
			&(inputPicturePtr->bufferY[inputOriginIndex]),
			inputPicturePtr->strideY,
			lcuParams->width,
			lcuParams->height);

		//64 x 64
		pictureControlSetPtr->lcuYSrcMeanCuArray[lcuIndex][0] = meanPtr[0];

		// 32x32
		cuSize = 32;
		cuNum = 64 / cuSize;
		for (cuH = 0; cuH < cuNum; cuH++){
			for (cuW = 0; cuW < cuNum; cuW++){
				inputCuOriginIndex = inputOriginIndex + cuH*(64 / cuNum)*inputLumaStride + cuW*(64 / cuNum);

				pictureControlSetPtr->lcuYSrcEnergyCuArray[lcuIndex][1 + cuH*cuNum + cuW] = ComputeNxMSatdSadLCU(
					&(inputPicturePtr->bufferY[inputCuOriginIndex]),
					inputPicturePtr->strideY,
					cuSize,
					cuSize);
				pictureControlSetPtr->lcuYSrcMeanCuArray[lcuIndex][1 + cuH*cuNum + cuW] = meanPtr[1 + cuH*cuNum + cuW];
			}
		}


	}
	else{
		pictureControlSetPtr->lcuYSrcEnergyCuArray[lcuIndex][0] = 100000000;
		pictureControlSetPtr->lcuYSrcEnergyCuArray[lcuIndex][1] = 100000000;
		pictureControlSetPtr->lcuYSrcEnergyCuArray[lcuIndex][2] = 100000000;
		pictureControlSetPtr->lcuYSrcEnergyCuArray[lcuIndex][3] = 100000000;
		pictureControlSetPtr->lcuYSrcEnergyCuArray[lcuIndex][4] = 100000000;
		pictureControlSetPtr->lcuYSrcMeanCuArray[lcuIndex][0] = 100000000;
		pictureControlSetPtr->lcuYSrcMeanCuArray[lcuIndex][1] = 100000000;
		pictureControlSetPtr->lcuYSrcMeanCuArray[lcuIndex][2] = 100000000;
		pictureControlSetPtr->lcuYSrcMeanCuArray[lcuIndex][3] = 100000000;
		pictureControlSetPtr->lcuYSrcMeanCuArray[lcuIndex][4] = 100000000;
	}
}

void LumaContrastDetectorLcu(
	SourceBasedOperationsContext_t *contextPtr,
	SequenceControlSet_t           *sequenceControlSetPtr,
	PictureParentControlSet_t	   *pictureControlSetPtr,
	EB_U32							lcuIndex) {

	EB_U64                  cuOisSAD = 0;
	EB_U64                  cuMeSAD = 0;

	// Calculate Luma mean of the frame by averaging the mean of LCUs to Detect Dark Frames (On only for 4k and BQMode)
	EB_U8  *yMeanPtr = contextPtr->yMeanPtr;
    LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
	if (lcuParams->isCompleteLcu){
		if (pictureControlSetPtr->sliceType != EB_I_PICTURE && pictureControlSetPtr->temporalLayerIndex == 0) {




			OisCu32Cu16Results_t *oisResultsPtr = pictureControlSetPtr->oisCu32Cu16Results[lcuIndex];
			cuOisSAD = oisResultsPtr->sortedOisCandidate[1][0].distortion +
				oisResultsPtr->sortedOisCandidate[2][0].distortion +
				oisResultsPtr->sortedOisCandidate[3][0].distortion +
				oisResultsPtr->sortedOisCandidate[4][0].distortion;



			cuMeSAD = pictureControlSetPtr->meResults[lcuIndex][0].distortionDirection[0].distortion;


			contextPtr->toBeIntraCodedProbability += cuOisSAD < cuMeSAD ? 1 : 0;
			contextPtr->depth1BlockNum++;
		}
	}

	if (pictureControlSetPtr->nonMovingIndexArray[lcuIndex] < 10)
	{
		contextPtr->yNonMovingMean += yMeanPtr[0];
		contextPtr->countOfNonMovingLcus++;
	}
	else {
		contextPtr->yMovingMean += yMeanPtr[0];
		contextPtr->countOfMovingLcus++;
	}
}

void LumaContrastDetectorPicture(
	SourceBasedOperationsContext_t		*contextPtr,
	PictureParentControlSet_t			*pictureControlSetPtr) {

	contextPtr->yNonMovingMean = (contextPtr->countOfNonMovingLcus != 0) ? (contextPtr->yNonMovingMean / contextPtr->countOfNonMovingLcus) : 0;
	contextPtr->yMovingMean = (contextPtr->countOfMovingLcus != 0) ? (contextPtr->yMovingMean / contextPtr->countOfMovingLcus) : 0;

	pictureControlSetPtr->darkBackGroundlightForeGround = ((contextPtr->yMovingMean > (2 * contextPtr->yNonMovingMean)) && (contextPtr->yNonMovingMean < DARK_FRM_TH)) ?
		EB_TRUE :
		EB_FALSE;

	pictureControlSetPtr->intraCodedBlockProbability = 0;

	if (pictureControlSetPtr->sliceType != EB_I_PICTURE && pictureControlSetPtr->temporalLayerIndex == 0){
		pictureControlSetPtr->intraCodedBlockProbability = (EB_U8)(contextPtr->depth1BlockNum != 0 ? contextPtr->toBeIntraCodedProbability * 100 / contextPtr->depth1BlockNum : 0);
	}
}

void GrassSkinLcu(
	SourceBasedOperationsContext_t		*contextPtr,
	SequenceControlSet_t                *sequenceControlSetPtr,
	PictureParentControlSet_t			*pictureControlSetPtr,
	EB_U32								 lcuIndex) {

	EB_U32                  childIndex;

	EB_BOOL                 lcuGrassFlag = EB_FALSE;

	EB_U32 grassLcuInrange;
	EB_U32 processedCus;
	EB_U32  rasterScanCuIndex;

    LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
	LcuStat_t *lcuStatPtr = &(pictureControlSetPtr->lcuStatArray[lcuIndex]);

	_mm_prefetch((const char*)lcuStatPtr, _MM_HINT_T0);

	lcuGrassFlag = EB_FALSE;
	grassLcuInrange = 0;
	processedCus = 0;


	for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_16x16_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_16x16_15; rasterScanCuIndex++) {
        if (lcuParams->rasterScanCuValidity[rasterScanCuIndex]) {
			const EB_U32 mdScanCuIndex = RASTER_SCAN_TO_MD_SCAN[rasterScanCuIndex];
			const EB_U32 rasterScanParentCuIndex = RASTER_SCAN_CU_PARENT_INDEX[rasterScanCuIndex];
			const EB_U32 mdScanParentCuIndex = RASTER_SCAN_TO_MD_SCAN[rasterScanParentCuIndex];
			CuStat_t *cuStatPtr = &(lcuStatPtr->cuStatArray[mdScanCuIndex]);


			const EB_U32 perfectCondition = 7;
			const EB_U8 yMean = contextPtr->yMeanPtr[rasterScanCuIndex];
			const EB_U8 cbMean = contextPtr->cbMeanPtr[rasterScanCuIndex];
			const EB_U8 crMean = contextPtr->crMeanPtr[rasterScanCuIndex];
			EB_U32 grassCondition = 0;
			EB_U32 skinCondition = 0;

			EB_U32	highChromaCondition = 0;
			EB_U32 highLumaCondition = 0;

			// GRASS
			grassCondition += (yMean > Y_MEAN_RANGE_02 && yMean < Y_MEAN_RANGE_01) ? 1 : 0;
			grassCondition += (cbMean > CB_MEAN_RANGE_00 && cbMean < CB_MEAN_RANGE_02) ? 2 : 0;
			grassCondition += (crMean > CR_MEAN_RANGE_00 && crMean < CR_MEAN_RANGE_02) ? 4 : 0;


			grassLcuInrange += (grassCondition == perfectCondition) ? 1 : 0;
			processedCus++;

			lcuGrassFlag = grassCondition == perfectCondition ? EB_TRUE : lcuGrassFlag;

			cuStatPtr->grassArea = (EB_BOOL)(grassCondition == perfectCondition);
			// SKIN
			skinCondition += (yMean > Y_MEAN_RANGE_03 && yMean < Y_MEAN_RANGE_01) ? 1 : 0;
			skinCondition += (cbMean > 100 && cbMean < 120) ? 2 : 0;
			skinCondition += (crMean > 135 && crMean < 160) ? 4 : 0;

			cuStatPtr->skinArea = (EB_BOOL)(skinCondition == perfectCondition);

			highChromaCondition = (crMean >= 127 || cbMean > 150) ? 1 : 0;
			highLumaCondition = (crMean >= 80 && yMean > 180) ? 1 : 0;
			cuStatPtr->highLuma = (highLumaCondition == 1) ? EB_TRUE : EB_FALSE;
			cuStatPtr->highChroma = (highChromaCondition == 1) ? EB_TRUE : EB_FALSE;

			for (childIndex = mdScanCuIndex + 1; childIndex < mdScanCuIndex + 5; ++childIndex){
				lcuStatPtr->cuStatArray[childIndex].grassArea = cuStatPtr->grassArea;
				lcuStatPtr->cuStatArray[childIndex].skinArea = cuStatPtr->skinArea;
				lcuStatPtr->cuStatArray[childIndex].highChroma = cuStatPtr->highChroma;
				lcuStatPtr->cuStatArray[childIndex].highLuma = cuStatPtr->highLuma;

			}

			lcuStatPtr->cuStatArray[mdScanParentCuIndex].grassArea = cuStatPtr->grassArea ? EB_TRUE :
				lcuStatPtr->cuStatArray[mdScanParentCuIndex].grassArea;
			lcuStatPtr->cuStatArray[0].grassArea = cuStatPtr->grassArea ? EB_TRUE :
				lcuStatPtr->cuStatArray[0].grassArea;
			lcuStatPtr->cuStatArray[mdScanParentCuIndex].skinArea = cuStatPtr->skinArea ? EB_TRUE :
				lcuStatPtr->cuStatArray[mdScanParentCuIndex].skinArea;
			lcuStatPtr->cuStatArray[0].skinArea = cuStatPtr->skinArea ? EB_TRUE :
				lcuStatPtr->cuStatArray[0].skinArea;
			lcuStatPtr->cuStatArray[mdScanParentCuIndex].highChroma = cuStatPtr->highChroma ? EB_TRUE :
				lcuStatPtr->cuStatArray[mdScanParentCuIndex].highChroma;
			lcuStatPtr->cuStatArray[0].highChroma = cuStatPtr->highChroma ? EB_TRUE :
				lcuStatPtr->cuStatArray[0].highChroma;

			lcuStatPtr->cuStatArray[mdScanParentCuIndex].highLuma = cuStatPtr->highLuma ? EB_TRUE :
				lcuStatPtr->cuStatArray[mdScanParentCuIndex].highLuma;
			lcuStatPtr->cuStatArray[0].highLuma = cuStatPtr->highLuma ? EB_TRUE :
				lcuStatPtr->cuStatArray[0].highLuma;



		}
	}

	contextPtr->pictureNumGrassLcu += lcuGrassFlag ? 1 : 0;
}

void GrassSkinPicture(
	SourceBasedOperationsContext_t		*contextPtr,
	PictureParentControlSet_t			*pictureControlSetPtr) {
	pictureControlSetPtr->grassPercentageInPicture = (EB_U8)(contextPtr->pictureNumGrassLcu * 100 / pictureControlSetPtr->lcuTotalCount);
}

/******************************************************
* Detect and mark LCU and 32x32 CUs which belong to an isolated non-homogeneous region surrounding a homogenous and flat region
******************************************************/
static inline void DetermineIsolatedNonHomogeneousRegionInPicture(
	SequenceControlSet_t            *sequenceControlSetPtr,
	PictureParentControlSet_t       *pictureControlSetPtr)
{
	EB_U32 lcuIndex;
	EB_U32 cuuIndex;
	EB_S32 lcuHor, lcuVer, lcuVerOffset;
	EB_U32 lcuTotalCount = pictureControlSetPtr->lcuTotalCount;
	EB_U32 pictureWidthInLcu = sequenceControlSetPtr->pictureWidthInLcu;
	EB_U32 pictureHeightInLcu = sequenceControlSetPtr->pictureHeightInLcu;

	for (lcuIndex = 0; lcuIndex < lcuTotalCount; ++lcuIndex) {

        LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
		// Initialize
		pictureControlSetPtr->lcuIsolatedNonHomogeneousAreaArray[lcuIndex] = EB_FALSE;
        if ((lcuParams->horizontalIndex > 0) && (lcuParams->horizontalIndex < pictureWidthInLcu - 1) && (lcuParams->verticalIndex > 0) && (lcuParams->verticalIndex < pictureHeightInLcu - 1)) {
			EB_U32 countOfMedVarianceLcu;
			countOfMedVarianceLcu = 0;

            // top neighbors
			countOfMedVarianceLcu += ((pictureControlSetPtr->variance[lcuIndex - pictureWidthInLcu - 1][ME_TIER_ZERO_PU_64x64]) <= MEDIUM_LCU_VARIANCE) ? 1 : 0;
			countOfMedVarianceLcu += (pictureControlSetPtr->variance[lcuIndex - pictureWidthInLcu][ME_TIER_ZERO_PU_64x64] <= MEDIUM_LCU_VARIANCE) ? 1 : 0;
			countOfMedVarianceLcu += (pictureControlSetPtr->variance[lcuIndex - pictureWidthInLcu + 1][ME_TIER_ZERO_PU_64x64] <= MEDIUM_LCU_VARIANCE && sequenceControlSetPtr->lcuParamsArray[lcuIndex - pictureWidthInLcu + 1].isCompleteLcu) ? 1 : 0;
			// bottom
			countOfMedVarianceLcu += (pictureControlSetPtr->variance[lcuIndex + pictureWidthInLcu - 1][ME_TIER_ZERO_PU_64x64] <= MEDIUM_LCU_VARIANCE && sequenceControlSetPtr->lcuParamsArray[lcuIndex + pictureWidthInLcu - 1].isCompleteLcu) ? 1 : 0;
			countOfMedVarianceLcu += (pictureControlSetPtr->variance[lcuIndex + pictureWidthInLcu][ME_TIER_ZERO_PU_64x64] <= MEDIUM_LCU_VARIANCE && sequenceControlSetPtr->lcuParamsArray[lcuIndex + pictureWidthInLcu].isCompleteLcu) ? 1 : 0;
			countOfMedVarianceLcu += (pictureControlSetPtr->variance[lcuIndex + pictureWidthInLcu + 1][ME_TIER_ZERO_PU_64x64] <= MEDIUM_LCU_VARIANCE && sequenceControlSetPtr->lcuParamsArray[lcuIndex + pictureWidthInLcu + 1].isCompleteLcu) ? 1 : 0;
			// left right
			countOfMedVarianceLcu += (pictureControlSetPtr->variance[lcuIndex + 1][ME_TIER_ZERO_PU_64x64] <= MEDIUM_LCU_VARIANCE  && sequenceControlSetPtr->lcuParamsArray[lcuIndex + 1].isCompleteLcu) ? 1 : 0;
			countOfMedVarianceLcu += (pictureControlSetPtr->variance[lcuIndex - 1][ME_TIER_ZERO_PU_64x64] <= MEDIUM_LCU_VARIANCE) ? 1 : 0;

			// At least two neighbors are flat
			if ((countOfMedVarianceLcu > 2)|| countOfMedVarianceLcu > 1)
			{
				// Search within an LCU if any of the 32x32 CUs is non-homogeneous
				EB_U32 count32x32NonhomCusInLcu = 0;
				for (cuuIndex = 0; cuuIndex < 4; cuuIndex++)
				{
					if (pictureControlSetPtr->varOfVar32x32BasedLcuArray[lcuIndex][cuuIndex] > VAR_BASED_DETAIL_PRESERVATION_SELECTOR_THRSLHD)
						count32x32NonhomCusInLcu++;
				}
				// If atleast one is non-homogeneous, then check all its neighbors (top left, top, top right, left, right, btm left, btm, btm right)
				EB_U32 countOfHomogeneousNeighborLcus = 0;
				if (count32x32NonhomCusInLcu > 0) {
					for (lcuVer = -1; lcuVer <= 1; lcuVer++) {
						lcuVerOffset = lcuVer * (EB_S32)pictureWidthInLcu;
						for (lcuHor = -1; lcuHor <= 1; lcuHor++) {
							if (lcuVer != 0 && lcuHor != 0)
								countOfHomogeneousNeighborLcus += (pictureControlSetPtr->lcuHomogeneousAreaArray[lcuIndex + lcuVerOffset + lcuHor] == EB_TRUE);

						}
					}
				}

				// To determine current lcu is isolated non-homogeneous, at least 2 neighbors must be homogeneous 
				if (countOfHomogeneousNeighborLcus >= 2){
					for (cuuIndex = 0; cuuIndex < 4; cuuIndex++)
					{
						if (pictureControlSetPtr->varOfVar32x32BasedLcuArray[lcuIndex][cuuIndex] > VAR_BASED_DETAIL_PRESERVATION_SELECTOR_THRSLHD)
						{
							pictureControlSetPtr->lcuIsolatedNonHomogeneousAreaArray[lcuIndex] = EB_TRUE;
						}
					}
				}

			}
		}
	}
	return;
}


void SetDefaultDeltaQpRange(
	SourceBasedOperationsContext_t	*contextPtr,
	PictureParentControlSet_t		*pictureControlSetPtr) {

	EB_S8	minDeltaQP;
	EB_U8	maxDeltaQP;
	if (pictureControlSetPtr->temporalLayerIndex == 0) {
		minDeltaQP = MinDeltaQPdefault[0];
		maxDeltaQP = MaxDeltaQPdefault[0];
	}
	else if (pictureControlSetPtr->isUsedAsReferenceFlag) {
		minDeltaQP = MinDeltaQPdefault[1];
		maxDeltaQP = MaxDeltaQPdefault[1];
	}
	else {
		minDeltaQP = MinDeltaQPdefault[2];
		maxDeltaQP = MaxDeltaQPdefault[2];
	}

	// Shape the min degrade
	minDeltaQP = (((EB_S8)(minDeltaQP + MIN_DELTA_QP_SHAPE_TH) > 0) ? 0 : (minDeltaQP + MIN_DELTA_QP_SHAPE_TH));

	// Shape the max degrade
	maxDeltaQP = (((EB_S8)(maxDeltaQP - MAX_DELTA_QP_SHAPE_TH) < 0) ? 0 : (maxDeltaQP - MAX_DELTA_QP_SHAPE_TH));

	contextPtr->minDeltaQP = minDeltaQP;
	contextPtr->maxDeltaQP = maxDeltaQP;
}


static inline void DetermineMorePotentialAuraAreas(
	SequenceControlSet_t        *sequenceControlSetPtr,
	PictureParentControlSet_t	*pictureControlSetPtr)
{
	EB_U16 lcuIndex;
	EB_S32 lcuHor, lcuVer, lcuVerOffset;
	EB_U32 countOfEdgeBlocks = 0, countOfNonEdgeBlocks = 0;

	EB_U32 lightLumaValue = 150;

	EB_U16 lcuTotalCount = pictureControlSetPtr->lcuTotalCount;

	for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {
        LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

		// For all the internal LCUs
		if (!lcuParams->isEdgeLcu) {
			countOfNonEdgeBlocks = 0;
            if (pictureControlSetPtr->edgeResultsPtr[lcuIndex].edgeBlockNum
				&& pictureControlSetPtr->yMean[lcuIndex][ME_TIER_ZERO_PU_64x64] >= lightLumaValue) {

				for (lcuVer = -1; lcuVer <= 1; lcuVer++) {
					lcuVerOffset = lcuVer * (EB_S32)sequenceControlSetPtr->pictureWidthInLcu;
					for (lcuHor = -1; lcuHor <= 1; lcuHor++) {
                        countOfNonEdgeBlocks += (!pictureControlSetPtr->edgeResultsPtr[lcuIndex + lcuVerOffset + lcuHor].edgeBlockNum) &&
							(pictureControlSetPtr->nonMovingIndexArray[lcuIndex + lcuVerOffset + lcuHor] < 30);
					}
				}
			}

			if (countOfNonEdgeBlocks > 1) {
				countOfEdgeBlocks++;
			}
		}
	}


	// To check the percentage of potential aura in the picture.. If a large area is detected then this is not isolated
    pictureControlSetPtr->percentageOfEdgeinLightBackground = (EB_U8)(countOfEdgeBlocks * 100 / lcuTotalCount);

}

/***************************************************
* Detects the presence of dark area
***************************************************/
void DeriveHighDarkAreaDensityFlag(
	SequenceControlSet_t                *sequenceControlSetPtr,
	PictureParentControlSet_t           *pictureControlSetPtr) {


	EB_U32	regionInPictureWidthIndex;
	EB_U32	regionInPictureHeightIndex;
	EB_U32	lumaHistogramBin;
	EB_U32	blackSamplesCount = 0;
	EB_U32	blackAreaPercentage;
	// Loop over regions inside the picture
	for (regionInPictureWidthIndex = 0; regionInPictureWidthIndex < sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth; regionInPictureWidthIndex++){  // loop over horizontal regions
		for (regionInPictureHeightIndex = 0; regionInPictureHeightIndex < sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight; regionInPictureHeightIndex++){ // loop over vertical regions
			for (lumaHistogramBin = 0; lumaHistogramBin < LOW_MEAN_TH_0; lumaHistogramBin++){ // loop over the 1st LOW_MEAN_THLD bins
				blackSamplesCount += pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][0][lumaHistogramBin];
			}
		}
	}

	blackAreaPercentage = (blackSamplesCount * 100) / (sequenceControlSetPtr->lumaWidth * sequenceControlSetPtr->lumaHeight);
    pictureControlSetPtr->highDarkAreaDensityFlag = (EB_BOOL)(blackAreaPercentage >= MIN_BLACK_AREA_PERCENTAGE);

    blackSamplesCount = 0;
    EB_U32	whiteSamplesCount = 0;
    EB_U32	whiteAreaPercentage;
    // Loop over regions inside the picture
    for (regionInPictureWidthIndex = 0; regionInPictureWidthIndex < sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerWidth; regionInPictureWidthIndex++){  // loop over horizontal regions
        for (regionInPictureHeightIndex = 0; regionInPictureHeightIndex < sequenceControlSetPtr->pictureAnalysisNumberOfRegionsPerHeight; regionInPictureHeightIndex++){ // loop over vertical regions
            for (lumaHistogramBin = 0; lumaHistogramBin < LOW_MEAN_TH_1; lumaHistogramBin++){ // loop over the 1st LOW_MEAN_THLD bins
                blackSamplesCount += pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][0][lumaHistogramBin];
            }
            for (lumaHistogramBin = HIGH_MEAN_TH; lumaHistogramBin < HISTOGRAM_NUMBER_OF_BINS; lumaHistogramBin++){ 
                whiteSamplesCount += pictureControlSetPtr->pictureHistogram[regionInPictureWidthIndex][regionInPictureHeightIndex][0][lumaHistogramBin];
            }
        }
    }

    blackAreaPercentage = (blackSamplesCount * 100) / (sequenceControlSetPtr->lumaWidth * sequenceControlSetPtr->lumaHeight);
    whiteAreaPercentage = (whiteSamplesCount * 100) / (sequenceControlSetPtr->lumaWidth * sequenceControlSetPtr->lumaHeight);

    pictureControlSetPtr->blackAreaPercentage = (EB_U8) blackAreaPercentage;

    pictureControlSetPtr->highDarkLowLightAreaDensityFlag = (EB_BOOL)(blackAreaPercentage >= MIN_BLACK_AREA_PERCENTAGE) && (whiteAreaPercentage >= MIN_WHITE_AREA_PERCENTAGE);
}
#define NORM_FACTOR	10
#define VAR_MIN		10
#define VAR_MAX		300
#define MIN_Y		70
#define MAX_Y		145
#define MID_CB		140
#define MID_CR		115
#define TH_CB		10
#define TH_CR		15

/******************************************************
* High  contrast classifier
******************************************************/
void TemporalHighContrastClassifier(
	SourceBasedOperationsContext_t	*contextPtr,
	PictureParentControlSet_t       *pictureControlSetPtr,
	EB_U32                           lcuIndex)
{

	EB_U32 blkIt;
	EB_U32 nsadTable[] = { 10, 5, 5, 5, 5, 5 };
	EB_U32 thRes = 0;
	EB_U32 nsad;
	EB_U32 meDist = 0;

	if (pictureControlSetPtr->sliceType == EB_B_PICTURE){ 

			
			for (blkIt = 0; blkIt < 4; blkIt++) {
			
				nsad = ((EB_U32)pictureControlSetPtr->meResults[lcuIndex][1 + blkIt].distortionDirection[0].distortion) >> NORM_FACTOR;

				if (nsad >= nsadTable[pictureControlSetPtr->temporalLayerIndex] + thRes)
					meDist++;
			}
		
	}
	contextPtr->highDist = meDist>0 ? EB_TRUE : EB_FALSE;
}

void SpatialHighContrastClassifier(
	SourceBasedOperationsContext_t	*contextPtr,
	PictureParentControlSet_t       *pictureControlSetPtr,
	EB_U32                           lcuIndex)
{

	EB_U32                 blkIt;

	contextPtr->highContrastNum = 0;

	//16x16 blocks
	for (blkIt = 0; blkIt < 16; blkIt++) {

		EB_U8 ymean = contextPtr->yMeanPtr[5 + blkIt];
		EB_U8 umean = contextPtr->cbMeanPtr[5 + blkIt];
		EB_U8 vmean = contextPtr->crMeanPtr[5 + blkIt];

		EB_U16 var = pictureControlSetPtr->variance[lcuIndex][5 + blkIt];


		if (var>VAR_MIN && var<VAR_MAX          &&  //medium texture 
			ymean>MIN_Y && ymean < MAX_Y        &&  //medium brightness(not too dark and not too bright)
			ABS((EB_S64)umean - MID_CB) < TH_CB &&  //middle of the color plane
			ABS((EB_S64)vmean - MID_CR) < TH_CR     //middle of the color plane
			)
		{
			contextPtr->highContrastNum++;

		}

	}
}

/******************************************************
Input   : current LCU value
Output  : populate to neighbor LCUs
******************************************************/
void PopulateFromCurrentLcuToNeighborLcus(
    PictureParentControlSet_t   *pictureControlSetPtr,
    EB_BOOL	                     inputToPopulate,
    EB_BOOL	                    *outputBuffer,
    EB_U32                       lcuAdrr,
    EB_U32                       lcuOriginX,
    EB_U32                       lcuOriginY)
{
    EB_U32 pictureWidthInLcus = (pictureControlSetPtr->enhancedPicturePtr->width + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE;

    // Copy to left if present
    if (lcuOriginX != 0) {
        outputBuffer[lcuAdrr - 1] = inputToPopulate;
    }

    // Copy to right if present
    if ((lcuOriginX + MAX_LCU_SIZE) < pictureControlSetPtr->enhancedPicturePtr->width) {
        outputBuffer[lcuAdrr + 1] = inputToPopulate;
    }

    // Copy to top LCU if present
    if (lcuOriginY != 0) {
        outputBuffer[lcuAdrr - pictureWidthInLcus] = inputToPopulate;
    }

    // Copy to bottom LCU if present
    if ((lcuOriginY + MAX_LCU_SIZE) < pictureControlSetPtr->enhancedPicturePtr->height) {
        outputBuffer[lcuAdrr + pictureWidthInLcus] = inputToPopulate;
    }

    // Copy to top-left LCU if present
    if ((lcuOriginX >= MAX_LCU_SIZE) && (lcuOriginY >= MAX_LCU_SIZE)) {
        outputBuffer[lcuAdrr - pictureWidthInLcus - 1] = inputToPopulate;
    }

    // Copy to top-right LCU if present
    if ((lcuOriginX < pictureControlSetPtr->enhancedPicturePtr->width - MAX_LCU_SIZE) && (lcuOriginY >= MAX_LCU_SIZE)) {
        outputBuffer[lcuAdrr - pictureWidthInLcus + 1] = inputToPopulate;
    }

    // Copy to bottom-left LCU if present
    if ((lcuOriginX >= MAX_LCU_SIZE) && (lcuOriginY < pictureControlSetPtr->enhancedPicturePtr->height - MAX_LCU_SIZE)) {
        outputBuffer[lcuAdrr + pictureWidthInLcus - 1] = inputToPopulate;
    }

    // Copy to bottom-right LCU if present
    if ((lcuOriginX < pictureControlSetPtr->enhancedPicturePtr->width - MAX_LCU_SIZE) && (lcuOriginY < pictureControlSetPtr->enhancedPicturePtr->height - MAX_LCU_SIZE)) {
        outputBuffer[lcuAdrr + pictureWidthInLcus + 1] = inputToPopulate;
    }
}


/******************************************************
Input   : variance
Output  : true if current & neighbors are spatially complex
******************************************************/
EB_BOOL IsSpatiallyComplexArea(

    PictureParentControlSet_t	*pictureControlSetPtr,
    EB_U32                       lcuAdrr,
    EB_U32                       lcuOriginX,
    EB_U32                       lcuOriginY)
{

    EB_U32  availableLcusCount      = 0;
    EB_U32  highVarianceLcusCount   = 0;
    EB_U32  pictureWidthInLcus      = (pictureControlSetPtr->enhancedPicturePtr->width + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE ;


    // Check the variance of the current LCU
    if ((pictureControlSetPtr->variance[lcuAdrr][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_LCU_VARIANCE_TH) {
        availableLcusCount++;
        highVarianceLcusCount++;
    }

    // Check the variance of left LCU if available
    if (lcuOriginX != 0) {
        availableLcusCount++;
        if ((pictureControlSetPtr->variance[lcuAdrr - 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_LCU_VARIANCE_TH) {
            highVarianceLcusCount++;
        }
    }

    // Check the variance of right LCU if available
    if ((lcuOriginX + MAX_LCU_SIZE) < pictureControlSetPtr->enhancedPicturePtr->width) {
        availableLcusCount++;
        if ((pictureControlSetPtr->variance[lcuAdrr + 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_LCU_VARIANCE_TH) {
            highVarianceLcusCount++;
        }

    }

    // Check the variance of top LCU if available
    if (lcuOriginY != 0) {
        availableLcusCount++;
        if ((pictureControlSetPtr->variance[lcuAdrr - pictureWidthInLcus][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_LCU_VARIANCE_TH) {
            highVarianceLcusCount++;
        }
    }

    // Check the variance of bottom LCU
    if ((lcuOriginY + MAX_LCU_SIZE) < pictureControlSetPtr->enhancedPicturePtr->height) {
        availableLcusCount++;
        if ((pictureControlSetPtr->variance[lcuAdrr + pictureWidthInLcus][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_LCU_VARIANCE_TH) {
            highVarianceLcusCount++;
        }

    }

    // Check the variance of top-left LCU
    if ((lcuOriginX >= MAX_LCU_SIZE) && (lcuOriginY >= MAX_LCU_SIZE)) {
        availableLcusCount++;
        if ((pictureControlSetPtr->variance[lcuAdrr - pictureWidthInLcus - 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_LCU_VARIANCE_TH) {
            highVarianceLcusCount++;
        }

    }

    // Check the variance of top-right LCU
    if ((lcuOriginX < pictureControlSetPtr->enhancedPicturePtr->width - MAX_LCU_SIZE) && (lcuOriginY >= MAX_LCU_SIZE)) {
        availableLcusCount++;
        if ((pictureControlSetPtr->variance[lcuAdrr - pictureWidthInLcus + 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_LCU_VARIANCE_TH) {
            highVarianceLcusCount++;
        }
    }

    // Check the variance of bottom-left LCU
    if ((lcuOriginX >= MAX_LCU_SIZE) && (lcuOriginY < pictureControlSetPtr->enhancedPicturePtr->height - MAX_LCU_SIZE)) {
        availableLcusCount++;
        if ((pictureControlSetPtr->variance[lcuAdrr + pictureWidthInLcus - 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_LCU_VARIANCE_TH) {
            highVarianceLcusCount++;
        }

    }

    // Check the variance of bottom-right LCU
    if ((lcuOriginX < pictureControlSetPtr->enhancedPicturePtr->width - MAX_LCU_SIZE) && (lcuOriginY < pictureControlSetPtr->enhancedPicturePtr->height - MAX_LCU_SIZE)) {
        availableLcusCount++;
        if ((pictureControlSetPtr->variance[lcuAdrr + pictureWidthInLcus + 1][ME_TIER_ZERO_PU_64x64]) > IS_COMPLEX_LCU_VARIANCE_TH) {
            highVarianceLcusCount++;
        }
    }

    if (highVarianceLcusCount == availableLcusCount) {
        return EB_TRUE;
    }

    return EB_FALSE;

}

/******************************************************
Input   : activity, layer index
Output  : LCU complexity level
******************************************************/
void DeriveBlockinessPresentFlag(
    SequenceControlSet_t        *sequenceControlSetPtr,
    PictureParentControlSet_t   *pictureControlSetPtr)
{
    EB_U32                      lcuIndex;

    for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

        LcuParams_t         *lcuParamPtr = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
        pictureControlSetPtr->complexLcuArray[lcuIndex] = LCU_COMPLEXITY_STATUS_INVALID;

        // Spatially complex LCU within a spatially complex area
        if (IsSpatiallyComplexArea(
            pictureControlSetPtr,
            lcuIndex,
            lcuParamPtr->originX,
            lcuParamPtr->originY)
            && pictureControlSetPtr->nonMovingIndexArray[lcuIndex]  != INVALID_ZZ_COST
            && pictureControlSetPtr->nonMovingIndexAverage          != INVALID_ZZ_COST
        
            ) {

            // Active LCU within an active scene (added a check on 4K & non-BASE to restrict the action - could be generated for all resolutions/layers) 
            if (pictureControlSetPtr->nonMovingIndexArray[lcuIndex] == LCU_COMPLEXITY_NON_MOVING_INDEX_TH_0 && pictureControlSetPtr->nonMovingIndexAverage >= LCU_COMPLEXITY_NON_MOVING_INDEX_TH_1 && pictureControlSetPtr->temporalLayerIndex > 0 && sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
                pictureControlSetPtr->complexLcuArray[lcuIndex] = LCU_COMPLEXITY_STATUS_2;
            }
            // Active LCU within a scene with a moderate acitivity (eg. active foregroud & static background) 
            else if (pictureControlSetPtr->nonMovingIndexArray[lcuIndex] == LCU_COMPLEXITY_NON_MOVING_INDEX_TH_0 && pictureControlSetPtr->nonMovingIndexAverage >= LCU_COMPLEXITY_NON_MOVING_INDEX_TH_2 && pictureControlSetPtr->nonMovingIndexAverage < LCU_COMPLEXITY_NON_MOVING_INDEX_TH_1) {
                pictureControlSetPtr->complexLcuArray[lcuIndex] = LCU_COMPLEXITY_STATUS_1;
            }
            else {
                pictureControlSetPtr->complexLcuArray[lcuIndex] = LCU_COMPLEXITY_STATUS_0;
            }
        }
        else {
            pictureControlSetPtr->complexLcuArray[lcuIndex] = LCU_COMPLEXITY_STATUS_0;
        }

    }
}

void QpmGatherStatistics(
    SequenceControlSet_t		*sequenceControlSetPtr,
    PictureParentControlSet_t   *pictureControlSetPtr,
    EB_U32                       lcuIndex)
{

    EB_U32 rasterScanCuIndex;
    EB_U32 mdScanCuIndex;
    EB_U32 oisSad;
    EB_U32 meSad;


    EB_U8                   cuDepth;
    LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

    EB_BOOL skipOis8x8 = pictureControlSetPtr->skipOis8x8;

    if (skipOis8x8 == EB_FALSE)
    {

        // 8x8 block loop
        cuDepth = 3;
        for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_8x8_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_8x8_63; rasterScanCuIndex++) {
            if (lcuParams->rasterScanCuValidity[rasterScanCuIndex]) {
                mdScanCuIndex = RASTER_SCAN_TO_MD_SCAN[rasterScanCuIndex];

                OisCu8Results_t   	     *oisCu8ResultsPtr = pictureControlSetPtr->oisCu8Results[lcuIndex];

                if (pictureControlSetPtr->cu8x8Mode == CU_8x8_MODE_0 && oisCu8ResultsPtr->sortedOisCandidate[rasterScanCuIndex - RASTER_SCAN_CU_INDEX_8x8_0][0].validDistortion) {
                    oisSad = oisCu8ResultsPtr->sortedOisCandidate[rasterScanCuIndex - RASTER_SCAN_CU_INDEX_8x8_0][0].distortion;
                }
                else {

                    OisCu32Cu16Results_t	 *oisCu32Cu16ResultsPtr = pictureControlSetPtr->oisCu32Cu16Results[lcuIndex];
                    const CodedUnitStats_t  *cuStats = GetCodedUnitStats(ParentBlockIndex[mdScanCuIndex]);
                    const EB_U32 me2Nx2NTableOffset = cuStats->cuNumInDepth + me2Nx2NOffset[cuStats->depth];
                    if (oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].validDistortion) {
                        oisSad = oisCu32Cu16ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].distortion;
                    }
                    else {
                        oisSad = 0;
                    }
                }



                meSad = pictureControlSetPtr->meResults[lcuIndex][rasterScanCuIndex].distortionDirection[0].distortion;


                //Keep track of the min,max and sum.
                pictureControlSetPtr->intraComplexityMin[cuDepth] = oisSad < pictureControlSetPtr->intraComplexityMin[cuDepth] ? oisSad : pictureControlSetPtr->intraComplexityMin[cuDepth];
                pictureControlSetPtr->intraComplexityMax[cuDepth] = oisSad > pictureControlSetPtr->intraComplexityMax[cuDepth] ? oisSad : pictureControlSetPtr->intraComplexityMax[cuDepth];
                pictureControlSetPtr->intraComplexityAccum[cuDepth] += oisSad;

                pictureControlSetPtr->interComplexityMin[cuDepth] = meSad < pictureControlSetPtr->interComplexityMin[cuDepth] ? meSad : pictureControlSetPtr->interComplexityMin[cuDepth];
                pictureControlSetPtr->interComplexityMax[cuDepth] = meSad > pictureControlSetPtr->interComplexityMax[cuDepth] ? meSad : pictureControlSetPtr->interComplexityMax[cuDepth];
                pictureControlSetPtr->interComplexityAccum[cuDepth] += meSad;
                pictureControlSetPtr->processedleafCount[cuDepth]++;
            }

        }
    }

    // 16x16 block loop
    cuDepth = 2;
    for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_16x16_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_16x16_15; rasterScanCuIndex++) {
        if (lcuParams->rasterScanCuValidity[rasterScanCuIndex]) {


            OisCu32Cu16Results_t	        *oisCu32Cu16ResultsPtr = pictureControlSetPtr->oisCu32Cu16Results[lcuIndex];

            oisSad = oisCu32Cu16ResultsPtr->sortedOisCandidate[rasterScanCuIndex][0].distortion;

            meSad = pictureControlSetPtr->meResults[lcuIndex][rasterScanCuIndex].distortionDirection[0].distortion;

            //Keep track of the min,max and sum.
            pictureControlSetPtr->intraComplexityMin[cuDepth] = oisSad < pictureControlSetPtr->intraComplexityMin[cuDepth] ? oisSad : pictureControlSetPtr->intraComplexityMin[cuDepth];
            pictureControlSetPtr->intraComplexityMax[cuDepth] = oisSad > pictureControlSetPtr->intraComplexityMax[cuDepth] ? oisSad : pictureControlSetPtr->intraComplexityMax[cuDepth];
            pictureControlSetPtr->intraComplexityAccum[cuDepth] += oisSad;

            pictureControlSetPtr->interComplexityMin[cuDepth] = meSad < pictureControlSetPtr->interComplexityMin[cuDepth] ? meSad : pictureControlSetPtr->interComplexityMin[cuDepth];
            pictureControlSetPtr->interComplexityMax[cuDepth] = meSad > pictureControlSetPtr->interComplexityMax[cuDepth] ? meSad : pictureControlSetPtr->interComplexityMax[cuDepth];
            pictureControlSetPtr->interComplexityAccum[cuDepth] += meSad;
            pictureControlSetPtr->processedleafCount[cuDepth]++;
        }
    }

    // 32x32 block loop
    cuDepth = 1;
    for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_32x32_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_32x32_3; rasterScanCuIndex++) {
        if (lcuParams->rasterScanCuValidity[rasterScanCuIndex]) {



            OisCu32Cu16Results_t	        *oisCu32Cu16ResultsPtr = pictureControlSetPtr->oisCu32Cu16Results[lcuIndex];
            oisSad = oisCu32Cu16ResultsPtr->sortedOisCandidate[rasterScanCuIndex][0].distortion;


            meSad = pictureControlSetPtr->meResults[lcuIndex][rasterScanCuIndex].distortionDirection[0].distortion;


            //Keep track of the min,max and sum.
            pictureControlSetPtr->intraComplexityMin[cuDepth] = oisSad < pictureControlSetPtr->intraComplexityMin[cuDepth] ? oisSad : pictureControlSetPtr->intraComplexityMin[cuDepth];
            pictureControlSetPtr->intraComplexityMax[cuDepth] = oisSad > pictureControlSetPtr->intraComplexityMax[cuDepth] ? oisSad : pictureControlSetPtr->intraComplexityMax[cuDepth];
            pictureControlSetPtr->intraComplexityAccum[cuDepth] += oisSad;

            pictureControlSetPtr->interComplexityMin[cuDepth] = meSad < pictureControlSetPtr->interComplexityMin[cuDepth] ? meSad : pictureControlSetPtr->interComplexityMin[cuDepth];
            pictureControlSetPtr->interComplexityMax[cuDepth] = meSad > pictureControlSetPtr->interComplexityMax[cuDepth] ? meSad : pictureControlSetPtr->interComplexityMax[cuDepth];
            pictureControlSetPtr->interComplexityAccum[cuDepth] += meSad;
            pictureControlSetPtr->processedleafCount[cuDepth]++;
        }
    }

    // 64x64 block loop
    cuDepth = 0;
    if (lcuParams->rasterScanCuValidity[RASTER_SCAN_CU_INDEX_64x64]) {


        OisCu32Cu16Results_t	        *oisCu32Cu16ResultsPtr = pictureControlSetPtr->oisCu32Cu16Results[lcuIndex];



        oisSad = oisCu32Cu16ResultsPtr->sortedOisCandidate[1][0].distortion +
            oisCu32Cu16ResultsPtr->sortedOisCandidate[2][0].distortion +
            oisCu32Cu16ResultsPtr->sortedOisCandidate[3][0].distortion +
            oisCu32Cu16ResultsPtr->sortedOisCandidate[4][0].distortion;


        meSad = pictureControlSetPtr->meResults[lcuIndex][RASTER_SCAN_CU_INDEX_64x64].distortionDirection[0].distortion;


        //Keep track of the min,max and sum.
        pictureControlSetPtr->intraComplexityMin[cuDepth] = oisSad < pictureControlSetPtr->intraComplexityMin[cuDepth] ? oisSad : pictureControlSetPtr->intraComplexityMin[cuDepth];
        pictureControlSetPtr->intraComplexityMax[cuDepth] = oisSad > pictureControlSetPtr->intraComplexityMax[cuDepth] ? oisSad : pictureControlSetPtr->intraComplexityMax[cuDepth];
        pictureControlSetPtr->intraComplexityAccum[cuDepth] += oisSad;

        pictureControlSetPtr->interComplexityMin[cuDepth] = meSad < pictureControlSetPtr->interComplexityMin[cuDepth] ? meSad : pictureControlSetPtr->interComplexityMin[cuDepth];
        pictureControlSetPtr->interComplexityMax[cuDepth] = meSad > pictureControlSetPtr->interComplexityMax[cuDepth] ? meSad : pictureControlSetPtr->interComplexityMax[cuDepth];
        pictureControlSetPtr->interComplexityAccum[cuDepth] += meSad;
        pictureControlSetPtr->processedleafCount[cuDepth]++;
    }
}


void StationaryEdgeOverUpdateOverTimeLcu(
    SequenceControlSet_t        *sequenceControlSetPtr,
    EB_U32						totalCheckedPictures,
    PictureParentControlSet_t   *pictureControlSetPtr,
    EB_U32                       totalLcuCount)
{

    EB_U32               lcuIndex;

    const EB_U32         slideWindowTh = ((totalCheckedPictures / 4) - 1);

    for (lcuIndex = 0; lcuIndex < totalLcuCount; lcuIndex++) {

        LcuParams_t *lcuParams  = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
        LcuStat_t   *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

        lcuStatPtr->stationaryEdgeOverTimeFlag = EB_FALSE;
        if (lcuParams->potentialLogoLcu && lcuParams->isCompleteLcu && lcuStatPtr->check1ForLogoStationaryEdgeOverTimeFlag && lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag) {

            EB_U32 rasterScanCuIndex;
            EB_U32 similarEdgeCountLcu = 0;
            // CU Loop
            for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_16x16_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_16x16_15; rasterScanCuIndex++) {
                similarEdgeCountLcu += (lcuStatPtr->cuStatArray[rasterScanCuIndex].similarEdgeCount > slideWindowTh) ? 1 : 0;
            }

            lcuStatPtr->stationaryEdgeOverTimeFlag = (similarEdgeCountLcu >= 4) ? EB_TRUE : EB_FALSE;
        }

        lcuStatPtr->pmStationaryEdgeOverTimeFlag = EB_FALSE;
        if (lcuParams->potentialLogoLcu && lcuParams->isCompleteLcu && lcuStatPtr->pmCheck1ForLogoStationaryEdgeOverTimeFlag && lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag) {

            EB_U32 rasterScanCuIndex;
            EB_U32 similarEdgeCountLcu = 0;
            // CU Loop
            for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_16x16_0; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_16x16_15; rasterScanCuIndex++) {
                similarEdgeCountLcu += (lcuStatPtr->cuStatArray[rasterScanCuIndex].pmSimilarEdgeCount > slideWindowTh) ? 1 : 0;
            }

            lcuStatPtr->pmStationaryEdgeOverTimeFlag = (similarEdgeCountLcu >= 4) ? EB_TRUE : EB_FALSE;
        }

    }
    {
        EB_U32 lcuIndex;
        EB_U32 lcu_X, lcu_Y;
        EB_U32 countOfNeighbors = 0;

        EB_U32 countOfNeighborsPm = 0;

        EB_S32 lcuHor, lcuVer, lcuVerOffset;
        EB_S32 lcuHorS, lcuVerS, lcuHorE, lcuVerE;
        EB_U32 pictureWidthInLcu = sequenceControlSetPtr->pictureWidthInLcu;
        EB_U32 pictureHeightInLcu = sequenceControlSetPtr->pictureHeightInLcu;

        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t		*lcuParams  = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
            LcuStat_t       *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

            lcu_X = lcuParams->horizontalIndex;
            lcu_Y = lcuParams->verticalIndex;
            if (lcuParams->potentialLogoLcu && lcuParams->isCompleteLcu && lcuStatPtr->check1ForLogoStationaryEdgeOverTimeFlag && lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag) {
                {
                    lcuHorS = (lcu_X > 0) ? -1 : 0;
                    lcuHorE = (lcu_X < pictureWidthInLcu - 1) ? 1 : 0;
                    lcuVerS = (lcu_Y > 0) ? -1 : 0;
                    lcuVerE = (lcu_Y < pictureHeightInLcu - 1) ? 1 : 0;
                    countOfNeighbors = 0;
                    for (lcuVer = lcuVerS; lcuVer <= lcuVerE; lcuVer++) {
                        lcuVerOffset = lcuVer * (EB_S32)pictureWidthInLcu;
                        for (lcuHor = lcuHorS; lcuHor <= lcuHorE; lcuHor++) {
                            countOfNeighbors += (pictureControlSetPtr->lcuStatArray[lcuIndex + lcuVerOffset + lcuHor].stationaryEdgeOverTimeFlag == 1);

                        }
                    }
                    if (countOfNeighbors == 1) {
                        lcuStatPtr->stationaryEdgeOverTimeFlag = 0;
                    }
                }
            }

            if (lcuParams->potentialLogoLcu && lcuParams->isCompleteLcu && lcuStatPtr->pmCheck1ForLogoStationaryEdgeOverTimeFlag && lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag) {
                {
                    lcuHorS = (lcu_X > 0) ? -1 : 0;
                    lcuHorE = (lcu_X < pictureWidthInLcu - 1) ? 1 : 0;
                    lcuVerS = (lcu_Y > 0) ? -1 : 0;
                    lcuVerE = (lcu_Y < pictureHeightInLcu - 1) ? 1 : 0;
                    countOfNeighborsPm = 0;
                    for (lcuVer = lcuVerS; lcuVer <= lcuVerE; lcuVer++) {
                        lcuVerOffset = lcuVer * (EB_S32)pictureWidthInLcu;
                        for (lcuHor = lcuHorS; lcuHor <= lcuHorE; lcuHor++) {
                            countOfNeighborsPm += (pictureControlSetPtr->lcuStatArray[lcuIndex + lcuVerOffset + lcuHor].pmStationaryEdgeOverTimeFlag == 1);
                        }
                    }

                    if (countOfNeighborsPm == 1) {
                        lcuStatPtr->pmStationaryEdgeOverTimeFlag = 0;
                    }
                }
            }

        }
    }


    {

        EB_U32 lcuIndex;
        EB_U32 lcu_X, lcu_Y;
        EB_U32 countOfNeighbors = 0;
        EB_S32 lcuHor, lcuVer, lcuVerOffset;
        EB_S32 lcuHorS, lcuVerS, lcuHorE, lcuVerE;
        EB_U32 pictureWidthInLcu = sequenceControlSetPtr->pictureWidthInLcu;
        EB_U32 pictureHeightInLcu = sequenceControlSetPtr->pictureHeightInLcu;

        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t		*lcuParams  = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
            LcuStat_t       *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

            lcu_X = lcuParams->horizontalIndex;
            lcu_Y = lcuParams->verticalIndex;

            {
                if (lcuStatPtr->stationaryEdgeOverTimeFlag == 0 && lcuParams->potentialLogoLcu && (lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag || !lcuParams->isCompleteLcu)) {
                    lcuHorS = (lcu_X > 0) ? -1 : 0;
                    lcuHorE = (lcu_X < pictureWidthInLcu - 1) ? 1 : 0;
                    lcuVerS = (lcu_Y > 0) ? -1 : 0;
                    lcuVerE = (lcu_Y < pictureHeightInLcu - 1) ? 1 : 0;
                    countOfNeighbors = 0;
                    for (lcuVer = lcuVerS; lcuVer <= lcuVerE; lcuVer++) {
                        lcuVerOffset = lcuVer * (EB_S32)pictureWidthInLcu;
                        for (lcuHor = lcuHorS; lcuHor <= lcuHorE; lcuHor++) {
                            countOfNeighbors += (pictureControlSetPtr->lcuStatArray[lcuIndex + lcuVerOffset + lcuHor].stationaryEdgeOverTimeFlag == 1);

                        }
                    }
                    if (countOfNeighbors > 0) {
                        lcuStatPtr->stationaryEdgeOverTimeFlag = 2;
                    }

                }
            }
        }

        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t	    *lcuParams  = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
            LcuStat_t       *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

            lcu_X = lcuParams->horizontalIndex;
            lcu_Y = lcuParams->verticalIndex;

            {
                if (lcuStatPtr->stationaryEdgeOverTimeFlag == 0 && lcuParams->potentialLogoLcu && (lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag || !lcuParams->isCompleteLcu)) {

                    lcuHorS = (lcu_X > 0) ? -1 : 0;
                    lcuHorE = (lcu_X < pictureWidthInLcu - 1) ? 1 : 0;
                    lcuVerS = (lcu_Y > 0) ? -1 : 0;
                    lcuVerE = (lcu_Y < pictureHeightInLcu - 1) ? 1 : 0;
                    countOfNeighbors = 0;
                    for (lcuVer = lcuVerS; lcuVer <= lcuVerE; lcuVer++) {
                        lcuVerOffset = lcuVer * (EB_S32)pictureWidthInLcu;
                        for (lcuHor = lcuHorS; lcuHor <= lcuHorE; lcuHor++) {
                            countOfNeighbors += (pictureControlSetPtr->lcuStatArray[lcuIndex + lcuVerOffset + lcuHor].stationaryEdgeOverTimeFlag == 2);

                        }
                    }
                    if (countOfNeighbors > 3) {
                        lcuStatPtr->stationaryEdgeOverTimeFlag = 3;
                    }

                }
            }
        }
    }

    {

        EB_U32 lcuIndex;
        EB_U32 lcu_X, lcu_Y;
        EB_U32 countOfNeighbors = 0;
        EB_S32 lcuHor, lcuVer, lcuVerOffset;
        EB_S32 lcuHorS, lcuVerS, lcuHorE, lcuVerE;
        EB_U32 pictureWidthInLcu = sequenceControlSetPtr->pictureWidthInLcu;
        EB_U32 pictureHeightInLcu = sequenceControlSetPtr->pictureHeightInLcu;

        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t		*lcuParams  = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
            LcuStat_t       *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

            lcu_X = lcuParams->horizontalIndex;
            lcu_Y = lcuParams->verticalIndex;

            {
                if (lcuStatPtr->pmStationaryEdgeOverTimeFlag == 0 && lcuParams->potentialLogoLcu && (lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag || !lcuParams->isCompleteLcu)) {
                    lcuHorS = (lcu_X > 0) ? -1 : 0;
                    lcuHorE = (lcu_X < pictureWidthInLcu - 1) ? 1 : 0;
                    lcuVerS = (lcu_Y > 0) ? -1 : 0;
                    lcuVerE = (lcu_Y < pictureHeightInLcu - 1) ? 1 : 0;
                    countOfNeighbors = 0;
                    for (lcuVer = lcuVerS; lcuVer <= lcuVerE; lcuVer++) {
                        lcuVerOffset = lcuVer * (EB_S32)pictureWidthInLcu;
                        for (lcuHor = lcuHorS; lcuHor <= lcuHorE; lcuHor++) {
                            countOfNeighbors += (pictureControlSetPtr->lcuStatArray[lcuIndex + lcuVerOffset + lcuHor].pmStationaryEdgeOverTimeFlag == 1);

                        }
                    }
                    if (countOfNeighbors > 0) {
                        lcuStatPtr->pmStationaryEdgeOverTimeFlag = 2;
                    }

                }
            }
        }

        for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t		*lcuParams  = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
            LcuStat_t       *lcuStatPtr = &pictureControlSetPtr->lcuStatArray[lcuIndex];

            lcu_X = lcuParams->horizontalIndex;
            lcu_Y = lcuParams->verticalIndex;

            {
                if (lcuStatPtr->pmStationaryEdgeOverTimeFlag == 0 && lcuParams->potentialLogoLcu && (lcuStatPtr->check2ForLogoStationaryEdgeOverTimeFlag || !lcuParams->isCompleteLcu)) {

                    lcuHorS = (lcu_X > 0) ? -1 : 0;
                    lcuHorE = (lcu_X < pictureWidthInLcu - 1) ? 1 : 0;
                    lcuVerS = (lcu_Y > 0) ? -1 : 0;
                    lcuVerE = (lcu_Y < pictureHeightInLcu - 1) ? 1 : 0;
                    countOfNeighbors = 0;
                    for (lcuVer = lcuVerS; lcuVer <= lcuVerE; lcuVer++) {
                        lcuVerOffset = lcuVer * (EB_S32)pictureWidthInLcu;
                        for (lcuHor = lcuHorS; lcuHor <= lcuHorE; lcuHor++) {
                            countOfNeighbors += (pictureControlSetPtr->lcuStatArray[lcuIndex + lcuVerOffset + lcuHor].pmStationaryEdgeOverTimeFlag == 2);

                        }
                    }
                    if (countOfNeighbors > 3) {
                        lcuStatPtr->pmStationaryEdgeOverTimeFlag = 3;
                    }

                }
            }
        }
    }
}


/************************************************
 * Source Based Operations Kernel
 ************************************************/
void* SourceBasedOperationsKernel(void *inputPtr)
{
    SourceBasedOperationsContext_t	*contextPtr = (SourceBasedOperationsContext_t*)inputPtr;
    PictureParentControlSet_t       *pictureControlSetPtr;
	SequenceControlSet_t            *sequenceControlSetPtr;
    EbObjectWrapper_t               *inputResultsWrapperPtr;
	InitialRateControlResults_t	    *inputResultsPtr;
    EbObjectWrapper_t               *outputResultsWrapperPtr;
	PictureDemuxResults_t       	*outputResultsPtr;

    for (;;) {

        // Get Input Full Object
        EbGetFullObject(
            contextPtr->initialrateControlResultsInputFifoPtr,
            &inputResultsWrapperPtr);
        EB_CHECK_END_OBJ(inputResultsWrapperPtr);

		inputResultsPtr = (InitialRateControlResults_t*)inputResultsWrapperPtr->objectPtr;
        pictureControlSetPtr = (PictureParentControlSet_t*)inputResultsPtr->pictureControlSetWrapperPtr->objectPtr;
		sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;

#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld SRC IN \n", pictureControlSetPtr->pictureNumber);
#endif
		pictureControlSetPtr->darkBackGroundlightForeGround = EB_FALSE;
		contextPtr->pictureNumGrassLcu = 0;
		contextPtr->countOfMovingLcus = 0;
		contextPtr->countOfNonMovingLcus = 0;
		contextPtr->yNonMovingMean = 0;
		contextPtr->yMovingMean = 0;
		contextPtr->toBeIntraCodedProbability = 0;
		contextPtr->depth1BlockNum = 0;

		EB_U32 lcuTotalCount = pictureControlSetPtr->lcuTotalCount;
		EB_U32 lcuIndex;


		/***********************************************LCU-based operations************************************************************/
		for (lcuIndex = 0; lcuIndex < lcuTotalCount; ++lcuIndex) {
            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
			pictureControlSetPtr->lcuCmplxContrastArray[lcuIndex] = 0;
            EB_BOOL isCompleteLcu = lcuParams->isCompleteLcu;
			EB_U8  *yMeanPtr = pictureControlSetPtr->yMean[lcuIndex];

			_mm_prefetch((const char*)yMeanPtr, _MM_HINT_T0);

			EB_U8  *crMeanPtr = pictureControlSetPtr->crMean[lcuIndex];
			EB_U8  *cbMeanPtr = pictureControlSetPtr->cbMean[lcuIndex];

			_mm_prefetch((const char*)crMeanPtr, _MM_HINT_T0);
			_mm_prefetch((const char*)cbMeanPtr, _MM_HINT_T0);

			contextPtr->yMeanPtr = yMeanPtr;
			contextPtr->crMeanPtr = crMeanPtr;
			contextPtr->cbMeanPtr = cbMeanPtr;

			// Grass & Skin detection 
            GrassSkinLcu(
				contextPtr,
				sequenceControlSetPtr,
				pictureControlSetPtr,
				lcuIndex);

			// Spatial high contrast classifier
			if (isCompleteLcu) {
				SpatialHighContrastClassifier(
					contextPtr,
					pictureControlSetPtr,
					lcuIndex);
			}

			// Luma Contrast detection
			LumaContrastDetectorLcu(
				contextPtr,
				sequenceControlSetPtr,
				pictureControlSetPtr,
				lcuIndex);

			// AC energy computation
			CalculateAcEnergy(
				sequenceControlSetPtr,
				pictureControlSetPtr,
				lcuIndex);

			// Failing Motion Detection
            pictureControlSetPtr->failingMotionLcuFlag[lcuIndex] = EB_FALSE;

            if (pictureControlSetPtr->sliceType != EB_I_PICTURE && isCompleteLcu){

                FailingMotionLcu(
                    sequenceControlSetPtr,
                    pictureControlSetPtr,
                    lcuIndex);
            }

			pictureControlSetPtr->uncoveredAreaLcuFlag[lcuIndex] = EB_FALSE;
			if (pictureControlSetPtr->temporalLayerIndex == 0 && pictureControlSetPtr->sliceType != EB_I_PICTURE){

				if (isCompleteLcu && (!pictureControlSetPtr->similarColocatedLcuArray[lcuIndex])){
					DetectUncoveredLcu(
						sequenceControlSetPtr,
						pictureControlSetPtr,
						lcuIndex);
				}
			}

			// Uncovered area detection II
			// Temporal high contrast classifier
			if (isCompleteLcu) {
				TemporalHighContrastClassifier(
					contextPtr,
					pictureControlSetPtr,
					lcuIndex);

                if (contextPtr->highContrastNum && contextPtr->highDist) {
                    PopulateFromCurrentLcuToNeighborLcus(
                        pictureControlSetPtr,
                        (contextPtr->highContrastNum && contextPtr->highDist),
                        pictureControlSetPtr->lcuCmplxContrastArray,
                        lcuIndex,
                        lcuParams->originX,
                        lcuParams->originY);
                }

			}

		}

		/*********************************************Picture-based operations**********************************************************/
		LumaContrastDetectorPicture(
			contextPtr,
			pictureControlSetPtr);

		// Delta QP range adjustments 
		SetDefaultDeltaQpRange(
			contextPtr,
			pictureControlSetPtr);

		// Dark density derivation (histograms not available when no SCD)
		DeriveHighDarkAreaDensityFlag(
			sequenceControlSetPtr,
			pictureControlSetPtr);


		// Detect and mark LCU and 32x32 CUs which belong to an isolated non-homogeneous region surrounding a homogenous and flat region.
		DetermineIsolatedNonHomogeneousRegionInPicture(
			sequenceControlSetPtr,
			pictureControlSetPtr);

		// Detect aura areas in lighter background when subject is moving similar to background
		DetermineMorePotentialAuraAreas(
			sequenceControlSetPtr,
			pictureControlSetPtr);

		// Activity statistics derivation
		DerivePictureActivityStatistics(
			sequenceControlSetPtr,
			pictureControlSetPtr);

        // Derive blockinessPresentFlag
        DeriveBlockinessPresentFlag(
            sequenceControlSetPtr,
            pictureControlSetPtr);

		// Skin & Grass detection 
		GrassSkinPicture(
			contextPtr,
			pictureControlSetPtr);

        // Complexity Classification
        ComplexityClassifier32x32(
            sequenceControlSetPtr,
            pictureControlSetPtr);


        // Stationary edge over time (final stage) 
        if (!pictureControlSetPtr->endOfSequenceFlag && sequenceControlSetPtr->staticConfig.lookAheadDistance != 0) {
            StationaryEdgeOverUpdateOverTimeLcu(
                sequenceControlSetPtr,
                MIN(MIN(((pictureControlSetPtr->predStructPtr->predStructPeriod << 1) + 1), pictureControlSetPtr->framesInSw), sequenceControlSetPtr->staticConfig.lookAheadDistance),
                pictureControlSetPtr,
                pictureControlSetPtr->lcuTotalCount);
        }


        // Outlier removal is needed for ADP classifier
        if (sequenceControlSetPtr->staticConfig.improveSharpness || sequenceControlSetPtr->staticConfig.bitRateReduction || pictureControlSetPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE) {

            EB_U32 lcuIndex;
            EB_U8 cuDepth;

            // QPM statistics init
            for (cuDepth = 0; cuDepth < 4; ++cuDepth) {
                pictureControlSetPtr->intraComplexityMin[cuDepth] = ~0u;
                pictureControlSetPtr->intraComplexityMax[cuDepth] = 0;
                pictureControlSetPtr->intraComplexityAccum[cuDepth] = 0;
                pictureControlSetPtr->intraComplexityAvg[cuDepth] = 0;

                pictureControlSetPtr->interComplexityMin[cuDepth] = ~0u;
                pictureControlSetPtr->interComplexityMax[cuDepth] = 0;
                pictureControlSetPtr->interComplexityAccum[cuDepth] = 0;
                pictureControlSetPtr->interComplexityAvg[cuDepth] = 0;

                pictureControlSetPtr->processedleafCount[cuDepth] = 0;
            }



            for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; lcuIndex++) {

                QpmGatherStatistics(
                    sequenceControlSetPtr,
                    pictureControlSetPtr,
                    lcuIndex);
            }


            pictureControlSetPtr->intraComplexityMinPre = pictureControlSetPtr->intraComplexityMin[0];
            pictureControlSetPtr->intraComplexityMaxPre = pictureControlSetPtr->intraComplexityMax[0];
            pictureControlSetPtr->interComplexityMinPre = pictureControlSetPtr->interComplexityMin[0];
            pictureControlSetPtr->interComplexityMaxPre = pictureControlSetPtr->interComplexityMax[0];

            EB_BOOL skipOis8x8 = pictureControlSetPtr->skipOis8x8;

            EB_U32 totDepths = skipOis8x8 ? 3 : 4;

            for (cuDepth = 0; cuDepth < totDepths; ++cuDepth) {

                pictureControlSetPtr->intraComplexityAvg[cuDepth] = pictureControlSetPtr->intraComplexityAccum[cuDepth] / pictureControlSetPtr->processedleafCount[cuDepth];
                pictureControlSetPtr->interComplexityAvg[cuDepth] = pictureControlSetPtr->interComplexityAccum[cuDepth] / pictureControlSetPtr->processedleafCount[cuDepth];

                EB_S32 intraMinDistance = ABS(((EB_S32)pictureControlSetPtr->intraComplexityMin[cuDepth] - (EB_S32)pictureControlSetPtr->intraComplexityAvg[cuDepth]));
                EB_S32 intraMaxDistance = ((EB_S32)pictureControlSetPtr->intraComplexityMax[cuDepth] - (EB_S32)pictureControlSetPtr->intraComplexityAvg[cuDepth]);

                // Adjust complexity bounds.
                if (intraMinDistance < intraMaxDistance) {
                    intraMaxDistance = intraMinDistance;
                    pictureControlSetPtr->intraComplexityMax[cuDepth] = pictureControlSetPtr->intraComplexityAvg[cuDepth] + intraMinDistance;
                    intraMinDistance = 0 - intraMinDistance;
                }
                else {
                    intraMinDistance = 0 - intraMaxDistance;
                    pictureControlSetPtr->intraComplexityMin[cuDepth] = pictureControlSetPtr->intraComplexityAvg[cuDepth] - intraMaxDistance;
                }

                EB_S32 interMinDistance = 0;
                EB_S32 interMaxDistance = 0;
                if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {
                    interMinDistance = ABS(((EB_S32)pictureControlSetPtr->interComplexityMin[cuDepth] - (EB_S32)pictureControlSetPtr->interComplexityAvg[cuDepth]));
                    interMaxDistance = ((EB_S32)pictureControlSetPtr->interComplexityMax[cuDepth] - (EB_S32)pictureControlSetPtr->interComplexityAvg[cuDepth]);
                }

                // Adjust complexity bounds.
                if (interMinDistance < interMaxDistance) {
                    interMaxDistance = interMinDistance;
                    pictureControlSetPtr->interComplexityMax[cuDepth] = pictureControlSetPtr->interComplexityAvg[cuDepth] + interMinDistance;
                    interMinDistance = 0 - interMinDistance;
                }
                else {
                    interMinDistance = 0 - interMaxDistance;
                    pictureControlSetPtr->interComplexityMin[cuDepth] = pictureControlSetPtr->interComplexityAvg[cuDepth] - interMaxDistance;
                }

                pictureControlSetPtr->intraMinDistance[cuDepth] = intraMinDistance;
                pictureControlSetPtr->intraMaxDistance[cuDepth] = intraMaxDistance;
                pictureControlSetPtr->interMinDistance[cuDepth] = interMinDistance;
                pictureControlSetPtr->interMaxDistance[cuDepth] = interMaxDistance;
            }
        }

#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld SRC OUT \n", pictureControlSetPtr->pictureNumber);
#endif

        // Get Empty Results Object
        EbGetEmptyObject(
            contextPtr->pictureDemuxResultsOutputFifoPtr,
            &outputResultsWrapperPtr);

		outputResultsPtr = (PictureDemuxResults_t*)outputResultsWrapperPtr->objectPtr;
        outputResultsPtr->pictureControlSetWrapperPtr = inputResultsPtr->pictureControlSetWrapperPtr;
		outputResultsPtr->pictureType = EB_PIC_INPUT;

        // Release the Input Results
        EbReleaseObject(inputResultsWrapperPtr);

        // Post the Full Results Object
        EbPostFullObject(outputResultsWrapperPtr);

    }
    return EB_NULL;
}
