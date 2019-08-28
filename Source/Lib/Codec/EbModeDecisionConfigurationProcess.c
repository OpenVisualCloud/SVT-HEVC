/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbUtility.h"

#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"

#include "EbModeDecisionConfigurationProcess.h"
#include "EbRateControlResults.h"
#include "EbEncDecTasks.h"
#include "EbModeDecisionConfiguration.h"
#include "EbLambdaRateTables.h"
#include "EbReferenceObject.h"
#include "EbRateControlProcess.h"

// Shooting states
#define UNDER_SHOOTING                        0
#define OVER_SHOOTING                         1
#define TBD_SHOOTING                          2
  

// Set a cost to each search method (could be modified)
// EB30 @ Revision 12879
#define PRED_OPEN_LOOP_1_NFL_COST    97 // PRED_OPEN_LOOP_1_NFL_COST is ~03% faster than PRED_OPEN_LOOP_COST  
#define U_099                        99
#define PRED_OPEN_LOOP_COST         100 // Let's assume PRED_OPEN_LOOP_COST costs ~100 U   
#define U_101                       101
#define U_102                       102
#define U_103                       103
#define U_104                       104  
#define U_105                       105
#define LIGHT_OPEN_LOOP_COST        106 // L_MDC is ~06% slower than PRED_OPEN_LOOP_COST       
#define U_107                       107  
#define U_108                       108  
#define U_109                       109
#define OPEN_LOOP_COST              110 // F_MDC is ~10% slower than PRED_OPEN_LOOP_COST
#define U_111                       111
#define U_112                       112
#define U_113                       113
#define U_114                       114
#define U_115                       115  
#define U_116                       116
#define U_117                       117
#define U_119                       119
#define U_120                       120
#define U_121                       121
#define U_122                       122
#define LIGHT_AVC_COST              122      
#define LIGHT_BDP_COST              123 // L_BDP is ~23% slower than PRED_OPEN_LOOP_COST
#define U_125                       125
#define U_127                       127
#define BDP_COST                    129 // F_BDP is ~29% slower than PRED_OPEN_LOOP_COST
#define U_130                       130
#define U_132                       132
#define U_133                       133
#define U_134                       134
#define AVC_COST                    138 // L_BDP is ~38% slower than PRED_OPEN_LOOP_COST
#define U_140                       140
#define U_145                       145
#define U_150                       150
#define U_152                       152
#define FULL_SEARCH_COST            155 // FS    is ~55% slower than PRED_OPEN_LOOP_COST


// ADP LCU score Manipulation
#define ADP_CLASS_SHIFT_DIST_0    50
#define ADP_CLASS_SHIFT_DIST_1    75

#define ADP_BLACK_AREA_PERCENTAGE 25
#define ADP_DARK_LCU_TH           25


#define ADP_CLASS_NON_MOVING_INDEX_TH_0 10
#define ADP_CLASS_NON_MOVING_INDEX_TH_1 25
#define ADP_CLASS_NON_MOVING_INDEX_TH_2 30

#define ADP_CONFIG_NON_MOVING_INDEX_TH_0 15
#define ADP_CONFIG_NON_MOVING_INDEX_TH_1 30

static const EB_U8 AdpLuminosityChangeThArray[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
    {  2 },
    {  2, 2 },
    {  3, 2, 2 },
    {  3, 3, 2, 2 },
    {  4, 3, 3, 2, 2 },
    {  4, 4, 3, 3, 2, 2 },
};

#define HIGH_HOMOGENOUS_PIC_TH               30

#define US_ADJ_STEP_TH0                      7
#define US_ADJ_STEP_TH1                      5
#define US_ADJ_STEP_TH2                      2
#define OS_ADJ_STEP_TH1                      2
#define OS_ADJ_STEP_TH2                      5
#define OS_ADJ_STEP_TH3                      7



#define VALID_SLOT_TH                        2


/******************************************************
* Compute picture and slice level chroma QP offsets 
******************************************************/
static void SetSliceAndPictureChromaQpOffsets(
	PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionConfigurationContext_t     *contextPtr)
{

	// This is a picture level chroma QP offset and is sent in the PPS
	pictureControlSetPtr->cbQpOffset = 0;
	pictureControlSetPtr->crQpOffset = 0;

	//In order to have QP offsets for chroma at a slice level set sliceLevelChromaQpFlag flag in pictureControlSetPtr (can be done in the PCS Ctor)

	// The below are slice level chroma QP offsets and is sent for each slice when sliceLevelChromaQpFlag is set

	// IMPORTANT: Lambda tables assume that the cb and cr have the same QP offsets.
	// However the offsets for each component can be very different for ENC DEC and we are conformant.
	pictureControlSetPtr->sliceCbQpOffset = 0;
	pictureControlSetPtr->sliceCrQpOffset = 0;

	if (pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= PIC_NOISE_CLASS_6){

		pictureControlSetPtr->sliceCbQpOffset = 10;
		pictureControlSetPtr->sliceCrQpOffset = 10;

	}
	else if (pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= PIC_NOISE_CLASS_4){

		pictureControlSetPtr->sliceCbQpOffset = 8;
		pictureControlSetPtr->sliceCrQpOffset = 8;

	}
	else{

        if (contextPtr->chromaQpOffsetLevel == 0) {

            if (pictureControlSetPtr->temporalLayerIndex == 1) {
                pictureControlSetPtr->sliceCbQpOffset = 2;
                pictureControlSetPtr->sliceCrQpOffset = 2;
            }
            else {
                pictureControlSetPtr->sliceCbQpOffset = 0;
                pictureControlSetPtr->sliceCrQpOffset = 0;
            }
        }
        else {

            int maxQpOffset = 3;
            if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
                pictureControlSetPtr->sliceCbQpOffset = -maxQpOffset;
                pictureControlSetPtr->sliceCrQpOffset = -maxQpOffset;
            }
            else {
                pictureControlSetPtr->sliceCbQpOffset = CLIP3(-12, 12, (int)MOD_QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex] - maxQpOffset);
                pictureControlSetPtr->sliceCrQpOffset = CLIP3(-12, 12, (int)MOD_QP_OFFSET_LAYER_ARRAY[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex] - maxQpOffset);
            }

        }
	}

}

/******************************************************
* Compute Tc, and Beta offsets for a given picture
******************************************************/
static void AdaptiveDlfParameterComputation(
    ModeDecisionConfigurationContext_t     *contextPtr,
    PictureControlSet_t                    *pictureControlSetPtr)
{
    EbReferenceObject_t  * refObjL0, *refObjL1;
    EB_U8                  highIntra = 0;

	EB_BOOL				   tcBetaOffsetManipulation = EB_FALSE;

    pictureControlSetPtr->highIntraSlection = highIntra;
    pictureControlSetPtr->tcOffset			= 0;
    pictureControlSetPtr->betaOffset		= 0;

    (void)contextPtr;

    if (pictureControlSetPtr->sliceType == EB_B_PICTURE){

        refObjL0 = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
        refObjL1 = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;

		if ((refObjL0->intraCodedArea > INTRA_AREA_TH_CLASS_1[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][refObjL0->tmpLayerIdx]) && (refObjL1->intraCodedArea > INTRA_AREA_TH_CLASS_1[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][refObjL1->tmpLayerIdx]))

            highIntra = 1;
        else
            highIntra = 0;
    }

    if (tcBetaOffsetManipulation){
        if (highIntra == 1) {
            pictureControlSetPtr->tcOffset = 12;
            pictureControlSetPtr->betaOffset = 12;
        }
        else {
            pictureControlSetPtr->tcOffset = 6;
            pictureControlSetPtr->betaOffset = 0;
        }
    }
    if (pictureControlSetPtr->sliceType == EB_B_PICTURE){

  
        refObjL0 = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
        refObjL1 = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;

        if (pictureControlSetPtr->temporalLayerIndex == 0){
			highIntra = (pictureControlSetPtr->ParentPcsPtr->intraCodedBlockProbability > INTRA_AREA_TH_CLASS_1[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex]) ? 1 : 0;
        } else{
            highIntra = (refObjL0->penalizeSkipflag || refObjL1->penalizeSkipflag) ? 1 : 0;
        }
        
    }

    if (tcBetaOffsetManipulation){

        if (pictureControlSetPtr->pictureQp < 19) {
            pictureControlSetPtr->tcOffset = 0;
            pictureControlSetPtr->betaOffset = 0;
        }
        else {
            if (pictureControlSetPtr->pictureQp > 35) {
                pictureControlSetPtr->tcOffset = 12;
                pictureControlSetPtr->betaOffset = 12;
            }

        }
    }
    pictureControlSetPtr->highIntraSlection = highIntra;


	if (pictureControlSetPtr->temporalLayerIndex == 0 &&
		pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= PIC_NOISE_CLASS_3_1 &&
		pictureControlSetPtr->pictureQp >= 37) {

		pictureControlSetPtr->tcOffset = 0;
		pictureControlSetPtr->betaOffset = 0;
	}

}


/******************************************************
 * Mode Decision Configuration Context Constructor
 ******************************************************/
EB_ERRORTYPE ModeDecisionConfigurationContextCtor(
    ModeDecisionConfigurationContext_t **contextDblPtr,
    EbFifo_t                            *rateControlInputFifoPtr,

    EbFifo_t                            *modeDecisionConfigurationOutputFifoPtr,
    EB_U16						         lcuTotalCount)
{
    ModeDecisionConfigurationContext_t *contextPtr;

    EB_MALLOC(ModeDecisionConfigurationContext_t*, contextPtr, sizeof(ModeDecisionConfigurationContext_t), EB_N_PTR);

    *contextDblPtr = contextPtr;
    
    // Input/Output System Resource Manager FIFOs
    contextPtr->rateControlInputFifoPtr                      = rateControlInputFifoPtr;
    contextPtr->modeDecisionConfigurationOutputFifoPtr       = modeDecisionConfigurationOutputFifoPtr;
    // Rate estimation
    EB_MALLOC(MdRateEstimationContext_t*, contextPtr->mdRateEstimationPtr, sizeof(MdRateEstimationContext_t), EB_N_PTR);


    // Budgeting  
    EB_MALLOC(EB_U32*,contextPtr->lcuScoreArray,sizeof(EB_U32) * lcuTotalCount, EB_N_PTR);
    EB_MALLOC(EB_U8 *,contextPtr->lcuCostArray ,sizeof(EB_U8 ) * lcuTotalCount, EB_N_PTR);


    return EB_ErrorNone;
}

/******************************************************
* Predict the LCU partitionning
******************************************************/
static void PerformEarlyLcuPartitionning(
	ModeDecisionConfigurationContext_t     *contextPtr,
	SequenceControlSet_t                   *sequenceControlSetPtr,
	PictureControlSet_t                    *pictureControlSetPtr)
{
	LargestCodingUnit_t			*lcuPtr;
	EB_U32						 lcuIndex;
	EB_U32						 sliceType;

	// MD Conf Rate Estimation Array from encodeContext
	MdRateEstimationContext_t	*mdConfRateEstimationArray;

	// Lambda Assignement

    if (pictureControlSetPtr->temporalLayerIndex == 0) {
        contextPtr->lambda = lambdaModeDecisionRaSadBase[contextPtr->qp];
    }
    else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
        contextPtr->lambda = lambdaModeDecisionRaSadRefNonBase[contextPtr->qp];
    }
    else {
        contextPtr->lambda = lambdaModeDecisionRaSadNonRef[contextPtr->qp];
    }


	// Slice Type
	sliceType =
		(pictureControlSetPtr->ParentPcsPtr->idrFlag == EB_TRUE) ? EB_I_PICTURE :
		pictureControlSetPtr->sliceType;

	// Increment the MD Rate Estimation array pointer to point to the right address based on the QP and slice type
	mdConfRateEstimationArray = (MdRateEstimationContext_t*)sequenceControlSetPtr->encodeContextPtr->mdRateEstimationArray;
	mdConfRateEstimationArray += sliceType * TOTAL_NUMBER_OF_QP_VALUES + contextPtr->qp;

	// Reset MD rate Estimation table to initial values by copying from mdRateEstimationArray
	EB_MEMCPY(&(contextPtr->mdRateEstimationPtr->splitFlagBits[0]), &(mdConfRateEstimationArray->splitFlagBits[0]), sizeof(EB_BitFraction)* MAX_SIZE_OF_MD_RATE_ESTIMATION_CASES);

    pictureControlSetPtr->ParentPcsPtr->averageQp = (EB_U8)pictureControlSetPtr->ParentPcsPtr->pictureQp;

	// LCU Loop : Partitionnig Decision
	for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

        lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];
        {
			lcuPtr->qp = (EB_U8)pictureControlSetPtr->ParentPcsPtr->pictureQp;			
		}

        EarlyModeDecisionLcu(
            sequenceControlSetPtr,
            pictureControlSetPtr,
            lcuPtr,
            lcuIndex,
            contextPtr);

	} // End of LCU Loop

}

static void PerformEarlyLcuPartitionningLcu(
	ModeDecisionConfigurationContext_t     *contextPtr,
	SequenceControlSet_t                   *sequenceControlSetPtr,
	PictureControlSet_t                    *pictureControlSetPtr,
    EB_U32						            lcuIndex)
{
	LargestCodingUnit_t			*lcuPtr;

	// LCU Loop : Partitionnig Decision
    lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];
    {
        lcuPtr->qp = (EB_U8)pictureControlSetPtr->ParentPcsPtr->pictureQp;
    }

    EarlyModeDecisionLcu(
        sequenceControlSetPtr,
        pictureControlSetPtr,
        lcuPtr,
        lcuIndex,
        contextPtr);
}


static void Forward85CuToModeDecisionLCU(
    SequenceControlSet_t  *sequenceControlSetPtr,
    PictureControlSet_t   *pictureControlSetPtr,
    EB_U32                 lcuIndex)
{
    const CodedUnitStats_t  *cuStatsPtr;
    EB_BOOL splitFlag;
    // LCU Loop : Partitionnig Decision

    LcuParams_t  *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
    MdcLcuData_t *resultsPtr = &pictureControlSetPtr->mdcLcuArray[lcuIndex];

    resultsPtr->leafCount = 0;
    EB_U8 cuIndex = 0;
    while (cuIndex < CU_MAX_COUNT)
    {
        splitFlag = EB_TRUE;
        cuStatsPtr = GetCodedUnitStats(cuIndex);
        if (lcuParams->rasterScanCuValidity[MD_SCAN_TO_RASTER_SCAN[cuIndex]])
        {
            switch (cuStatsPtr->depth){

            case 0:                
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                break;

            case 1:
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                break;

            case 2:
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                break;

            case 3:
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_FALSE;
                break;

            default:
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                break;
            }
        }

        cuIndex += (splitFlag == EB_TRUE) ? 1 : DepthOffset[cuStatsPtr->depth];

    } // End CU Loop
}

static void Forward84CuToModeDecisionLCU(
    SequenceControlSet_t  *sequenceControlSetPtr,
    PictureControlSet_t   *pictureControlSetPtr,
    EB_U32                 lcuIndex)
{
    const CodedUnitStats_t  *cuStatsPtr;
    EB_BOOL splitFlag;
    // LCU Loop : Partitionnig Decision

    LcuParams_t  *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
    MdcLcuData_t *resultsPtr = &pictureControlSetPtr->mdcLcuArray[lcuIndex];

    resultsPtr->leafCount = 0;
    EB_U8 cuIndex = 0;
    while (cuIndex < CU_MAX_COUNT)
    {
        splitFlag = EB_TRUE;
        cuStatsPtr = GetCodedUnitStats(cuIndex);
        if (lcuParams->rasterScanCuValidity[MD_SCAN_TO_RASTER_SCAN[cuIndex]])
        {
            switch (cuStatsPtr->depth){

            case 0:                
                splitFlag = EB_TRUE;
                break;

            case 1:
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                break;

            case 2:
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                break;

            case 3:
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_FALSE;
                break;

            default:
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                break;
            }
        }

        cuIndex += (splitFlag == EB_TRUE) ? 1 : DepthOffset[cuStatsPtr->depth];

    } // End CU Loop
}

static void Forward85CuToModeDecision(
    SequenceControlSet_t                   *sequenceControlSetPtr,
    PictureControlSet_t                    *pictureControlSetPtr)
{
    const CodedUnitStats_t  *cuStatsPtr;
    EB_U32                   lcuIndex;
    EB_BOOL splitFlag;
    // LCU Loop : Partitionnig Decision
    for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

        LcuParams_t  *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
        MdcLcuData_t *resultsPtr = &pictureControlSetPtr->mdcLcuArray[lcuIndex];

        resultsPtr->leafCount = 0;
        EB_U8 cuIndex = 0;
        while (cuIndex < CU_MAX_COUNT)
        {
            splitFlag = EB_TRUE;
            cuStatsPtr = GetCodedUnitStats(cuIndex);
            if (lcuParams->rasterScanCuValidity[MD_SCAN_TO_RASTER_SCAN[cuIndex]])
            {
                switch (cuStatsPtr->depth){

                case 0:
				    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
				    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                    break;

                case 1:
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                    break;

                case 2:
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                    break;

                case 3:
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_FALSE;
                    break;

                default:
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                    break;
                }
            }

            cuIndex += (splitFlag == EB_TRUE) ? 1 : DepthOffset[cuStatsPtr->depth];

        } // End CU Loop
    }

    pictureControlSetPtr->ParentPcsPtr->averageQp = (EB_U8)pictureControlSetPtr->ParentPcsPtr->pictureQp;

}

static void Forward84CuToModeDecision(
    SequenceControlSet_t                   *sequenceControlSetPtr,
    PictureControlSet_t                    *pictureControlSetPtr)
{
    const CodedUnitStats_t  *cuStatsPtr;
    EB_U32                   lcuIndex;
    EB_BOOL splitFlag;
    // LCU Loop : Partitionnig Decision
    for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

        LcuParams_t  *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
        MdcLcuData_t *resultsPtr = &pictureControlSetPtr->mdcLcuArray[lcuIndex];

        resultsPtr->leafCount = 0;
        EB_U8 cuIndex = 0;
        while (cuIndex < CU_MAX_COUNT)
        {
            splitFlag = EB_TRUE;
            cuStatsPtr = GetCodedUnitStats(cuIndex);
            if (lcuParams->rasterScanCuValidity[MD_SCAN_TO_RASTER_SCAN[cuIndex]])
            {
                switch (cuStatsPtr->depth){

                case 0:
				    splitFlag = EB_TRUE;
                    break;

                case 1:
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                    break;

                case 2:
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                    break;

                case 3:
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_FALSE;
                    break;

                default:
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                    break;
                }
            }

            cuIndex += (splitFlag == EB_TRUE) ? 1 : DepthOffset[cuStatsPtr->depth];

        } // End CU Loop
    }

    pictureControlSetPtr->ParentPcsPtr->averageQp = (EB_U8)pictureControlSetPtr->ParentPcsPtr->pictureQp;

}

void Forward8x816x16CuToModeDecisionLCU(
    SequenceControlSet_t  *sequenceControlSetPtr,
    PictureControlSet_t   *pictureControlSetPtr,
    EB_U32                 lcuIndex)
{
    const CodedUnitStats_t  *cuStatsPtr;
    EB_BOOL splitFlag;

    LcuParams_t  *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
    MdcLcuData_t *resultsPtr = &pictureControlSetPtr->mdcLcuArray[lcuIndex];

    resultsPtr->leafCount = 0;
    EB_U8 cuIndex = 0;
    while (cuIndex < CU_MAX_COUNT)
    {
        splitFlag = EB_TRUE;
        cuStatsPtr = GetCodedUnitStats(cuIndex);
        if (lcuParams->rasterScanCuValidity[MD_SCAN_TO_RASTER_SCAN[cuIndex]])
        {
            switch (cuStatsPtr->depth) {

            case 0:

                splitFlag = EB_TRUE;

                break;
            case 1:
                splitFlag = EB_TRUE;

                break;

            case 2:

                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;

                break;
            case 3:

                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_FALSE;

                break;

            default:
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                break;
            }
        }

        cuIndex += (splitFlag == EB_TRUE) ? 1 : DepthOffset[cuStatsPtr->depth];

    } // End CU Loop
}

static void Forward16x16CuToModeDecisionLCU(
    SequenceControlSet_t  *sequenceControlSetPtr,
    PictureControlSet_t   *pictureControlSetPtr,
    EB_U32                 lcuIndex)
{
    const CodedUnitStats_t  *cuStatsPtr;
    EB_BOOL splitFlag;

    LcuParams_t  *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
    MdcLcuData_t *resultsPtr = &pictureControlSetPtr->mdcLcuArray[lcuIndex];

    resultsPtr->leafCount = 0;
    EB_U8 cuIndex = 0;
    while (cuIndex < CU_MAX_COUNT)
    {
        splitFlag = EB_TRUE;
        cuStatsPtr = GetCodedUnitStats(cuIndex);
        if (lcuParams->rasterScanCuValidity[MD_SCAN_TO_RASTER_SCAN[cuIndex]])
        {
            switch (cuStatsPtr->depth) {

            case 0:

                splitFlag = EB_TRUE;

                break;
            case 1:
                splitFlag = EB_TRUE;

                break;

            case 2:

                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_FALSE;

                break;
            case 3:

                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_FALSE;

                break;

            default:
                resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;
                break;
            }
        }

        cuIndex += (splitFlag == EB_TRUE) ? 1 : DepthOffset[cuStatsPtr->depth];

    } // End CU Loop
}

static void PartitioningInitialization( 
    SequenceControlSet_t                   *sequenceControlSetPtr,
    PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionConfigurationContext_t     *contextPtr) {

    EB_U32						 sliceType;

    // MD Conf Rate Estimation Array from encodeContext
    MdRateEstimationContext_t	*mdConfRateEstimationArray;

    // Lambda Assignement
    if (pictureControlSetPtr->temporalLayerIndex == 0) {
        contextPtr->lambda = lambdaModeDecisionRaSadBase[contextPtr->qp];
    }
    else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
        contextPtr->lambda = lambdaModeDecisionRaSadRefNonBase[contextPtr->qp];
    }
    else {
        contextPtr->lambda = lambdaModeDecisionRaSadNonRef[contextPtr->qp];
    }

    // Slice Type
    sliceType =
        (pictureControlSetPtr->ParentPcsPtr->idrFlag == EB_TRUE) ? EB_I_PICTURE :
        pictureControlSetPtr->sliceType;

    // Increment the MD Rate Estimation array pointer to point to the right address based on the QP and slice type
    mdConfRateEstimationArray = (MdRateEstimationContext_t*)sequenceControlSetPtr->encodeContextPtr->mdRateEstimationArray;
    mdConfRateEstimationArray += sliceType * TOTAL_NUMBER_OF_QP_VALUES + contextPtr->qp;

    // Reset MD rate Estimation table to initial values by copying from mdRateEstimationArray
    EB_MEMCPY(&(contextPtr->mdRateEstimationPtr->splitFlagBits[0]), &(mdConfRateEstimationArray->splitFlagBits[0]), sizeof(EB_BitFraction)* MAX_SIZE_OF_MD_RATE_ESTIMATION_CASES);

    pictureControlSetPtr->ParentPcsPtr->averageQp = (EB_U8)pictureControlSetPtr->ParentPcsPtr->pictureQp;
}

/******************************************************
* Detect complex/non-flat/moving LCU in a non-complex area (used to refine MDC depth control)
******************************************************/
static void DetectComplexNonFlatMovingLcu(
    SequenceControlSet_t   *sequenceControlSetPtr,
	PictureControlSet_t	   *pictureControlSetPtr,
	EB_U32					pictureWidthInLcu) {

	LargestCodingUnit_t *lcuPtr;
	EB_U32               lcuIndex;

    if (pictureControlSetPtr->ParentPcsPtr->nonMovingIndexAverage != INVALID_ZZ_COST && pictureControlSetPtr->ParentPcsPtr->nonMovingIndexAverage >= 10 && pictureControlSetPtr->temporalLayerIndex <= 2) {

		// Determine deltaQP and assign QP value for each leaf
		for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

            LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

			lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];

			EB_BOOL condition = EB_FALSE;

			if (!pictureControlSetPtr->ParentPcsPtr->similarColocatedLcuArray[lcuIndex] &&
                lcuPtr->pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuIndex].edgeBlockNum > 0) {
				condition = EB_TRUE;
			}

			if (condition){
				EB_U32  counter = 0;

                if (!lcuParams->isEdgeLcu){  
                    // Top      
                    if (pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuIndex - pictureWidthInLcu].edgeBlockNum == 0)
                        counter++;
                    // Bottom
                    if (pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuIndex + pictureWidthInLcu].edgeBlockNum == 0)
                        counter++;
                    // Left                  
                    if (pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuIndex - 1].edgeBlockNum == 0)
                        counter++;
                    // right
                    if (pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuIndex + 1].edgeBlockNum == 0)
                        counter++;
				}
			}
		}    
	}
}

static EB_AURA_STATUS AuraDetection64x64(
	PictureControlSet_t           *pictureControlSetPtr,
	EB_U8                          pictureQp,
	EB_U32                         lcuIndex
	)
{
	SequenceControlSet_t  *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	EB_S32                 pictureWidthInLcu = (sequenceControlSetPtr->lumaWidth + MAX_LCU_SIZE - 1) >> MAX_LOG2_LCU_SIZE;
	EB_U32                 currDist;
	EB_U32                 topDist, topLDist, topRDist;
	EB_U32                 localAvgDist, distThresh0, distThresh1;
	EB_S32                 lcuOffset;

	EB_AURA_STATUS		   auraClass = AURA_STATUS_0;
	EB_U8				   auraClass1 = 0;
	EB_U8				   auraClass2 = 0;

	EB_S32				   xMv0 = 0;
	EB_S32				   yMv0 = 0;
	EB_S32				   xMv1 = 0;
	EB_S32				   yMv1 = 0;

	EB_U32                 leftDist, rightDist;

    LcuParams_t           *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

    distThresh0 = pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag ||sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE ? 15 : 14; 
	distThresh1 = 23;


	if (pictureQp > 38){
		distThresh0 = distThresh0 << 2;
		distThresh1 = distThresh1 << 2;
	}

    if (!lcuParams->isEdgeLcu){ 
        
        EB_U32 k;		

		MeCuResults_t * mePuResult = &pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex][0];

		//Curr Block  	

		for (k = 0; k < mePuResult->totalMeCandidateIndex; k++) {

			if (mePuResult->distortionDirection[k].direction == UNI_PRED_LIST_0) {
				// Get reference list 0 / reference index 0 MV
				xMv0 = mePuResult->xMvL0;
				yMv0 = mePuResult->yMvL0;
			}
			if (mePuResult->distortionDirection[k].direction == UNI_PRED_LIST_1) {
				// Get reference list  1 / reference index 0 MV
				xMv1 = mePuResult->xMvL1;
				yMv1 = mePuResult->yMvL1;
			}

		}
		currDist = mePuResult->distortionDirection[0].distortion;



		if ((currDist > 64 * 64) &&
			// Only mark a block as aura when it is moving (MV amplitude higher than X; X is temporal layer dependent)
			(abs(xMv0) > GLOBAL_MOTION_THRESHOLD[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex] ||
			abs(yMv0) > GLOBAL_MOTION_THRESHOLD[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex] ||
			abs(xMv1) > GLOBAL_MOTION_THRESHOLD[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex] ||
			abs(yMv1) > GLOBAL_MOTION_THRESHOLD[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex]))
		{

			//Top Distortion          
			lcuOffset = -pictureWidthInLcu;
			topDist = pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex + lcuOffset]->distortionDirection[0].distortion;


			//TopLeft Distortion          
			lcuOffset = -pictureWidthInLcu - 1;
			topLDist = pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex + lcuOffset]->distortionDirection[0].distortion;


			//TopRightDistortion          
			lcuOffset = -pictureWidthInLcu + 1;
			topRDist = pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex + lcuOffset]->distortionDirection[0].distortion;


            topRDist = (lcuParams->horizontalIndex < (EB_U32)(pictureWidthInLcu - 2)) ? topRDist : currDist;

			//left Distortion          
			lcuOffset = -1;
			leftDist = pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex + lcuOffset]->distortionDirection[0].distortion;



			//RightDistortion          
			lcuOffset = 1;
			rightDist = pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex + lcuOffset]->distortionDirection[0].distortion;


            rightDist = (lcuParams->horizontalIndex < (EB_U32)(pictureWidthInLcu - 2)) ? topRDist : currDist;

			localAvgDist = MIN(MIN(MIN(topLDist, MIN(topRDist, topDist)), leftDist), rightDist);

			if (10 * currDist > distThresh0*localAvgDist){
				if (10 * currDist > distThresh1*localAvgDist)
					auraClass2++;
				else
					auraClass1++;
			}
		}

	}

	auraClass = (auraClass2 > 0 || auraClass1 > 0) ? AURA_STATUS_1 : AURA_STATUS_0;

	return   auraClass;
}


/******************************************************
* Aura detection
******************************************************/
static void AuraDetection(
	SequenceControlSet_t         *sequenceControlSetPtr,
	PictureControlSet_t			 *pictureControlSetPtr,
	EB_U32					      pictureWidthInLcu,
	EB_U32					      pictureHeightInLcu)
{
	EB_U32                      lcuIndex;
    EB_U32                      lcu_X;
    EB_U32                      lcu_Y;
	for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {

        LcuParams_t             *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
		LargestCodingUnit_t     *lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];

		// Aura status intialization
        lcuPtr->auraStatus = INVALID_AURA_STATUS;
        lcu_X = lcuParams->horizontalIndex;
        lcu_Y = lcuParams->verticalIndex;

        if (pictureControlSetPtr->sliceType == EB_B_PICTURE){ 
            if ((lcu_X > 0) && (lcu_X < pictureWidthInLcu - 1) && (lcu_Y < pictureHeightInLcu - 1)){
                lcuPtr->auraStatus = AuraDetection64x64(
                    pictureControlSetPtr,
                    (EB_U8)pictureControlSetPtr->pictureQp,
                    lcuPtr->index);
            }
        }
	}
    return;
}

static EB_ERRORTYPE DeriveDefaultSegments(
    PictureControlSet_t                 *pictureControlSetPtr,
    ModeDecisionConfigurationContext_t  *contextPtr)
{
    EB_ERRORTYPE                    return_error = EB_ErrorNone;

    // @ BASE MDC is not considered as similar to BDP_L in term of speed
    if (pictureControlSetPtr->temporalLayerIndex == 0) {

        if (contextPtr->adpDepthSensitivePictureClass && contextPtr->budget >= (EB_U32)(pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * LIGHT_BDP_COST)) {

            if (contextPtr->budget > (EB_U32) (pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * BDP_COST)) {
                contextPtr->numberOfSegments = 2;
                contextPtr->scoreTh[0] = (EB_S8)((1 * 100) / contextPtr->numberOfSegments);
                contextPtr->intervalCost[0] = contextPtr->costDepthMode[LCU_BDP_DEPTH_MODE - 1];
                contextPtr->intervalCost[1] = contextPtr->costDepthMode[LCU_FULL84_DEPTH_MODE - 1];
            }
            else {
                contextPtr->numberOfSegments = 2;
                contextPtr->scoreTh[0] = (EB_S8)((1 * 100) / contextPtr->numberOfSegments);
                contextPtr->intervalCost[0] = contextPtr->costDepthMode[LCU_LIGHT_BDP_DEPTH_MODE - 1];
                contextPtr->intervalCost[1] = contextPtr->costDepthMode[LCU_BDP_DEPTH_MODE - 1];
            }

        } 
        else { 
            if (contextPtr->budget > (EB_U32) (pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * BDP_COST)) {
                
                contextPtr->numberOfSegments = 2;
                
                contextPtr->scoreTh[0] = (EB_S8)((1 * 100) / contextPtr->numberOfSegments);

                contextPtr->intervalCost[0] = contextPtr->costDepthMode[LCU_BDP_DEPTH_MODE - 1];
                contextPtr->intervalCost[1] = contextPtr->costDepthMode[LCU_FULL84_DEPTH_MODE - 1];
            } 

            else if (contextPtr->budget > (EB_U32)(pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * OPEN_LOOP_COST)) {

                contextPtr->numberOfSegments = 4;

                contextPtr->scoreTh[0] = (EB_S8)((1 * 100) / contextPtr->numberOfSegments);
                contextPtr->scoreTh[1] = (EB_S8)((2 * 100) / contextPtr->numberOfSegments);
                contextPtr->scoreTh[2] = (EB_S8)((3 * 100) / contextPtr->numberOfSegments);

                contextPtr->intervalCost[0] = contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_DEPTH_MODE - 1];
                contextPtr->intervalCost[1] = contextPtr->costDepthMode[LCU_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
                contextPtr->intervalCost[2] = contextPtr->costDepthMode[LCU_LIGHT_BDP_DEPTH_MODE - 1];
                contextPtr->intervalCost[3] = contextPtr->costDepthMode[LCU_BDP_DEPTH_MODE - 1];
            }
            else {
                contextPtr->numberOfSegments = 5;

                contextPtr->scoreTh[0] = (EB_S8)((1 * 100) / contextPtr->numberOfSegments);
                contextPtr->scoreTh[1] = (EB_S8)((2 * 100) / contextPtr->numberOfSegments);
                contextPtr->scoreTh[2] = (EB_S8)((3 * 100) / contextPtr->numberOfSegments);
                contextPtr->scoreTh[3] = (EB_S8)((4 * 100) / contextPtr->numberOfSegments);

                contextPtr->intervalCost[0] = contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE - 1];
                contextPtr->intervalCost[1] = contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_DEPTH_MODE - 1];
                contextPtr->intervalCost[2] = contextPtr->costDepthMode[LCU_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
                contextPtr->intervalCost[3] = contextPtr->costDepthMode[LCU_LIGHT_BDP_DEPTH_MODE - 1];
                contextPtr->intervalCost[4] = contextPtr->costDepthMode[LCU_BDP_DEPTH_MODE - 1];
            }
       
        }
    }
    else {

        if (contextPtr->budget > (EB_U32)(pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_120)) {

            contextPtr->numberOfSegments = 6;

            contextPtr->scoreTh[0] = (EB_S8)((1 * 100) / contextPtr->numberOfSegments);
            contextPtr->scoreTh[1] = (EB_S8)((2 * 100) / contextPtr->numberOfSegments);
            contextPtr->scoreTh[2] = (EB_S8)((3 * 100) / contextPtr->numberOfSegments);
            contextPtr->scoreTh[3] = (EB_S8)((4 * 100) / contextPtr->numberOfSegments);
            contextPtr->scoreTh[4] = (EB_S8)((5 * 100) / contextPtr->numberOfSegments);

            contextPtr->intervalCost[0] = contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_DEPTH_MODE - 1];
            contextPtr->intervalCost[1] = contextPtr->costDepthMode[LCU_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
            contextPtr->intervalCost[2] = contextPtr->costDepthMode[LCU_OPEN_LOOP_DEPTH_MODE - 1];
            contextPtr->intervalCost[3] = contextPtr->costDepthMode[LCU_LIGHT_BDP_DEPTH_MODE - 1];
            contextPtr->intervalCost[4] = contextPtr->costDepthMode[LCU_BDP_DEPTH_MODE - 1];
            contextPtr->intervalCost[5] = contextPtr->costDepthMode[LCU_FULL85_DEPTH_MODE - 1];
        } 
        else if (contextPtr->budget > (EB_U32)(pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_115)) {

                contextPtr->numberOfSegments = 5;

                contextPtr->scoreTh[0] = (EB_S8)((1 * 100) / contextPtr->numberOfSegments);
                contextPtr->scoreTh[1] = (EB_S8)((2 * 100) / contextPtr->numberOfSegments);
                contextPtr->scoreTh[2] = (EB_S8)((3 * 100) / contextPtr->numberOfSegments);
                contextPtr->scoreTh[3] = (EB_S8)((4 * 100) / contextPtr->numberOfSegments);

                contextPtr->intervalCost[0] = contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_DEPTH_MODE - 1];
                contextPtr->intervalCost[1] = contextPtr->costDepthMode[LCU_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
                contextPtr->intervalCost[2] = contextPtr->costDepthMode[LCU_OPEN_LOOP_DEPTH_MODE - 1];
                contextPtr->intervalCost[3] = contextPtr->costDepthMode[LCU_LIGHT_BDP_DEPTH_MODE - 1];
                contextPtr->intervalCost[4] = contextPtr->costDepthMode[LCU_BDP_DEPTH_MODE - 1];

        } else if (contextPtr->budget > (EB_U32)(pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * OPEN_LOOP_COST)) {

            contextPtr->numberOfSegments = 4;

            contextPtr->scoreTh[0] = (EB_S8)((1 * 100) / contextPtr->numberOfSegments);
            contextPtr->scoreTh[1] = (EB_S8)((2 * 100) / contextPtr->numberOfSegments);
            contextPtr->scoreTh[2] = (EB_S8)((3 * 100) / contextPtr->numberOfSegments);

            contextPtr->intervalCost[0] = contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_DEPTH_MODE - 1];
            contextPtr->intervalCost[1] = contextPtr->costDepthMode[LCU_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
            contextPtr->intervalCost[2] = contextPtr->costDepthMode[LCU_OPEN_LOOP_DEPTH_MODE - 1];
            contextPtr->intervalCost[3] = contextPtr->costDepthMode[LCU_LIGHT_BDP_DEPTH_MODE - 1];

        } else {

            contextPtr->numberOfSegments = 4;

            contextPtr->scoreTh[0] = (EB_S8)((1 * 100) / contextPtr->numberOfSegments);
            contextPtr->scoreTh[1] = (EB_S8)((2 * 100) / contextPtr->numberOfSegments);
            contextPtr->scoreTh[2] = (EB_S8)((3 * 100) / contextPtr->numberOfSegments);

            contextPtr->intervalCost[0] = contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE - 1];
            contextPtr->intervalCost[1] = contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_DEPTH_MODE - 1];
            contextPtr->intervalCost[2] = contextPtr->costDepthMode[LCU_LIGHT_OPEN_LOOP_DEPTH_MODE - 1];
            contextPtr->intervalCost[3] = contextPtr->costDepthMode[LCU_OPEN_LOOP_DEPTH_MODE - 1];
        }
    }
    
    return return_error;
}

/******************************************************
* Set the target budget
Input   : cost per depth
Output  : budget per picture
******************************************************/

static void SetTargetBudgetOq(
	SequenceControlSet_t                *sequenceControlSetPtr,
	PictureControlSet_t                 *pictureControlSetPtr,
	ModeDecisionConfigurationContext_t  *contextPtr)
{
	EB_U32 budget;
    
	if (contextPtr->adpLevel <= ENC_MODE_3) {
		if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_1080i_RANGE) {
			if (pictureControlSetPtr->temporalLayerIndex == 0)
				budget = pictureControlSetPtr->lcuTotalCount * FULL_SEARCH_COST;
			else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
				budget = (pictureControlSetPtr->ParentPcsPtr->isPan || pictureControlSetPtr->ParentPcsPtr->isTilt) ?
				pictureControlSetPtr->lcuTotalCount * U_152 :
				pictureControlSetPtr->lcuTotalCount * U_150;
			else
				budget = (pictureControlSetPtr->ParentPcsPtr->isPan || pictureControlSetPtr->ParentPcsPtr->isTilt) ?
				pictureControlSetPtr->lcuTotalCount * U_152 :
				pictureControlSetPtr->lcuTotalCount * U_145;
		}
		else if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_1080p_RANGE) {
			if (pictureControlSetPtr->temporalLayerIndex == 0)
				budget = pictureControlSetPtr->lcuTotalCount * FULL_SEARCH_COST;
			else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
				budget = pictureControlSetPtr->lcuTotalCount * AVC_COST;
			else
				budget = pictureControlSetPtr->lcuTotalCount * U_134;
		}
		else {
			if (pictureControlSetPtr->temporalLayerIndex == 0)
				budget = pictureControlSetPtr->lcuTotalCount * BDP_COST;
			else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
				budget = pictureControlSetPtr->lcuTotalCount * OPEN_LOOP_COST;
			else
				budget = pictureControlSetPtr->lcuTotalCount * U_109;
		}
	}
    else if (contextPtr->adpLevel <= ENC_MODE_5) {
        if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_1080i_RANGE) {
            if (pictureControlSetPtr->temporalLayerIndex == 0)
                budget = pictureControlSetPtr->lcuTotalCount * FULL_SEARCH_COST;
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
                budget = (pictureControlSetPtr->ParentPcsPtr->isPan || pictureControlSetPtr->ParentPcsPtr->isTilt) ?
                    pictureControlSetPtr->lcuTotalCount * U_152 :
                    pictureControlSetPtr->lcuTotalCount * U_150;
            else
                budget = (pictureControlSetPtr->ParentPcsPtr->isPan || pictureControlSetPtr->ParentPcsPtr->isTilt) ?
                    pictureControlSetPtr->lcuTotalCount * U_152 :
                    pictureControlSetPtr->lcuTotalCount * U_145;
        }
        else if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_1080p_RANGE) {
            if (pictureControlSetPtr->temporalLayerIndex == 0)
                budget = pictureControlSetPtr->lcuTotalCount * FULL_SEARCH_COST;
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
                budget = pictureControlSetPtr->lcuTotalCount * AVC_COST;
            else
                budget = pictureControlSetPtr->lcuTotalCount * U_134;
        }
        else {
            if (pictureControlSetPtr->temporalLayerIndex == 0)
                budget = pictureControlSetPtr->lcuTotalCount * FULL_SEARCH_COST;
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
                budget = pictureControlSetPtr->lcuTotalCount * U_125;
            else
                budget = pictureControlSetPtr->lcuTotalCount * U_121;
        }
	}
	else if (contextPtr->adpLevel <= ENC_MODE_7) {
		if (pictureControlSetPtr->temporalLayerIndex == 0)
			budget = pictureControlSetPtr->lcuTotalCount * BDP_COST;
		else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
			budget = pictureControlSetPtr->lcuTotalCount * OPEN_LOOP_COST;
		else
			budget = pictureControlSetPtr->lcuTotalCount * U_109;
	}
    else if (contextPtr->adpLevel <= ENC_MODE_8) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0)
				budget = (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_2) ?
				pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_127 :
				(contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_1) ?
				pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_125 :
				pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_121;
			else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag)
				budget = (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_2) ?
				pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * OPEN_LOOP_COST :
				(contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_1) ?
				pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100 :
				pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100;
			else
				budget = (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_2) ?
				pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100 :
				pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100;
		}
		else {
			if (pictureControlSetPtr->temporalLayerIndex == 0)
				budget = pictureControlSetPtr->lcuTotalCount * BDP_COST;
			else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
				budget = pictureControlSetPtr->lcuTotalCount * OPEN_LOOP_COST;
			else
				budget = pictureControlSetPtr->lcuTotalCount * U_109;
		}
	}
    else if (contextPtr->adpLevel <= ENC_MODE_10) {
        if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0)
            budget = (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_127 :
            (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_1) ?
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_125 :
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_121;
        else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag)
            budget = (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * OPEN_LOOP_COST :
            (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_1) ?
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100 :
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100;
        else
            budget = (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100 :
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100;
    }
    else {
        if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0)
            budget = (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_127 :
            (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_1) ?
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_125 :
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * U_121;
        else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag)
            budget = (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * OPEN_LOOP_COST :
            (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_1) ?
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100 :
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100;
        else
            budget = (contextPtr->adpDepthSensitivePictureClass == DEPTH_SENSITIVE_PIC_CLASS_2) ?
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100 :
            pictureControlSetPtr->ParentPcsPtr->lcuTotalCount * 100;
    }

    contextPtr->budget = budget;
}


/******************************************************
 * IsAvcPartitioningMode()
 * Returns TRUE for LCUs where only Depth2 & Depth3 
 * (AVC Partitioning) are goind to be tested by MD 
 * The LCU is marked if Sharpe Edge or Potential Aura/Grass
 * or B-Logo or S-Logo or Potential Blockiness Area
 * Input: Sharpe Edge, Potential Aura/Grass, B-Logo, S-Logo, Potential Blockiness Area signals
 * Output: TRUE if one of the above is TRUE
 ******************************************************/
static EB_BOOL IsAvcPartitioningMode(
    SequenceControlSet_t  *sequenceControlSetPtr,
    PictureControlSet_t   *pictureControlSetPtr,
    LargestCodingUnit_t   *lcuPtr)
{
	const EB_U32    lcuIndex = lcuPtr->index;
	LcuParams_t    *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
	EB_PICTURE        sliceType = pictureControlSetPtr->sliceType;
	EB_U8           edgeBlockNum = pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuIndex].edgeBlockNum;
	LcuStat_t      *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuIndex]);
	EB_U8           stationaryEdgeOverTimeFlag = lcuStatPtr->stationaryEdgeOverTimeFlag;

	EB_U8           auraStatus = lcuPtr->auraStatus;

	// No refinment for sub-1080p
	if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_1080i_RANGE)
		return EB_FALSE;

	// Sharpe Edge
	if (pictureControlSetPtr->ParentPcsPtr->highDarkLowLightAreaDensityFlag && pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex > 0 && pictureControlSetPtr->ParentPcsPtr->sharpEdgeLcuFlag[lcuIndex] && !pictureControlSetPtr->ParentPcsPtr->similarColocatedLcuArrayAllLayers[lcuIndex]) {
		return EB_TRUE;
	}

	// Potential Aura/Grass
	if (pictureControlSetPtr->sceneCaracteristicId == EB_FRAME_CARAC_0) {
		if (pictureControlSetPtr->ParentPcsPtr->grassPercentageInPicture > 60 && auraStatus == AURA_STATUS_1) {
			if ((sliceType != EB_I_PICTURE && pictureControlSetPtr->highIntraSlection == 0) && (lcuParams->isCompleteLcu)) {
				return EB_TRUE;
			}
		}
	}

	// B-Logo
	if (pictureControlSetPtr->ParentPcsPtr->logoPicFlag && edgeBlockNum)
		return EB_TRUE;

	// S-Logo           
	if (stationaryEdgeOverTimeFlag > 0)
		return EB_TRUE;

	// Potential Blockiness Area
	if (pictureControlSetPtr->ParentPcsPtr->complexLcuArray[lcuIndex] == LCU_COMPLEXITY_STATUS_2)
		return EB_TRUE;

    return EB_FALSE;
}

/******************************************************
* Load the cost of the different partitioning method into a local array and derive sensitive picture flag
    Input   : the offline derived cost per search method, detection signals
    Output  : valid costDepthMode and valid sensitivePicture
******************************************************/
static void ConfigureAdp(
    PictureControlSet_t                 *pictureControlSetPtr,
	ModeDecisionConfigurationContext_t  *contextPtr) 
{
    contextPtr->costDepthMode[LCU_FULL85_DEPTH_MODE - 1]               = FULL_SEARCH_COST;
    contextPtr->costDepthMode[LCU_FULL84_DEPTH_MODE - 1]               = FULL_SEARCH_COST;
    contextPtr->costDepthMode[LCU_BDP_DEPTH_MODE - 1]                  = BDP_COST;
    contextPtr->costDepthMode[LCU_LIGHT_BDP_DEPTH_MODE - 1]            = LIGHT_BDP_COST;
    contextPtr->costDepthMode[LCU_OPEN_LOOP_DEPTH_MODE - 1]            = OPEN_LOOP_COST;
    contextPtr->costDepthMode[LCU_LIGHT_OPEN_LOOP_DEPTH_MODE - 1]      = LIGHT_OPEN_LOOP_COST;
    contextPtr->costDepthMode[LCU_AVC_DEPTH_MODE - 1]                  = AVC_COST;
    contextPtr->costDepthMode[LCU_LIGHT_AVC_DEPTH_MODE - 1]            = LIGHT_AVC_COST;
    contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_DEPTH_MODE - 1]       = PRED_OPEN_LOOP_COST;
    contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE - 1] = PRED_OPEN_LOOP_1_NFL_COST;

    // Initialize the score based TH
    contextPtr->scoreTh[0] = ~0;
    contextPtr->scoreTh[1] = ~0;
    contextPtr->scoreTh[2] = ~0;
    contextPtr->scoreTh[3] = ~0;
    contextPtr->scoreTh[4] = ~0;
    contextPtr->scoreTh[5] = ~0;
    contextPtr->scoreTh[6] = ~0;

    // Initialize the predicted budget
    contextPtr->predictedCost = (EB_U32) ~0;

	// Initialize the predicted budget
	contextPtr->predictedCost = (EB_U32)~0;

    // Derive the sensitive picture flag 
    contextPtr->adpDepthSensitivePictureClass = DEPTH_SENSITIVE_PIC_CLASS_0;

    EB_BOOL luminosityChange = EB_FALSE;
    // Derived for REF P & B & kept false otherwise (for temporal distance equal to 1 luminosity changes are easier to handle)
    // Derived for P & B
    if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {
        if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
            EbReferenceObject_t  * refObjL0, *refObjL1;
            refObjL0 = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
            refObjL1 = (pictureControlSetPtr->ParentPcsPtr->sliceType == EB_B_PICTURE) ? (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr : (EbReferenceObject_t*)EB_NULL;
            luminosityChange = ((ABS(pictureControlSetPtr->ParentPcsPtr->averageIntensity[0] - refObjL0->averageIntensity) >= AdpLuminosityChangeThArray[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex]) || (refObjL1 != EB_NULL && ABS(pictureControlSetPtr->ParentPcsPtr->averageIntensity[0] - refObjL1->averageIntensity) >= AdpLuminosityChangeThArray[pictureControlSetPtr->ParentPcsPtr->hierarchicalLevels][pictureControlSetPtr->temporalLayerIndex]));
        }
    }

    if (pictureControlSetPtr->ParentPcsPtr->nonMovingIndexAverage != INVALID_ZZ_COST && pictureControlSetPtr->ParentPcsPtr->nonMovingIndexAverage < ADP_CONFIG_NON_MOVING_INDEX_TH_1) { // could not seen by the eye if very active   
        if (pictureControlSetPtr->ParentPcsPtr->picNoiseClass > PIC_NOISE_CLASS_3 || pictureControlSetPtr->ParentPcsPtr->highDarkLowLightAreaDensityFlag ||luminosityChange) { // potential complex picture: luminosity Change (e.g. fade, light..)
            contextPtr->adpDepthSensitivePictureClass = DEPTH_SENSITIVE_PIC_CLASS_2;
        }   
        // potential complex picture: light foreground and dark background(e.g.flash, light..) or moderate activity and high variance (noise or a lot of edge) 
        else if ( (pictureControlSetPtr->ParentPcsPtr->nonMovingIndexAverage >= ADP_CONFIG_NON_MOVING_INDEX_TH_0 && pictureControlSetPtr->ParentPcsPtr->picNoiseClass == PIC_NOISE_CLASS_3)) {
            contextPtr->adpDepthSensitivePictureClass = DEPTH_SENSITIVE_PIC_CLASS_1;
        }
    }


}

/******************************************************
* Assign a search method based on the allocated cost 
    Input   : allocated budget per LCU
    Output  : search method per LCU
******************************************************/
static void DeriveSearchMethod(
    PictureControlSet_t                 *pictureControlSetPtr,
    ModeDecisionConfigurationContext_t  *contextPtr)
{
    
    EB_U32 lcuIndex;


    pictureControlSetPtr->bdpPresentFlag = EB_FALSE;
    pictureControlSetPtr->mdPresentFlag  = EB_FALSE;

    for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->ParentPcsPtr->lcuTotalCount; lcuIndex++) {

        if (contextPtr->lcuCostArray[lcuIndex] == contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE - 1]) {
            pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] = LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE;
            pictureControlSetPtr->mdPresentFlag = EB_TRUE;
        }
        else
        if (contextPtr->lcuCostArray[lcuIndex] == contextPtr->costDepthMode[LCU_PRED_OPEN_LOOP_DEPTH_MODE - 1]) {
            pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] = LCU_PRED_OPEN_LOOP_DEPTH_MODE;
            pictureControlSetPtr->mdPresentFlag  = EB_TRUE;
        } else if (contextPtr->lcuCostArray[lcuIndex] == contextPtr->costDepthMode[LCU_LIGHT_OPEN_LOOP_DEPTH_MODE - 1]) {
            pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] = LCU_LIGHT_OPEN_LOOP_DEPTH_MODE;
            pictureControlSetPtr->mdPresentFlag  = EB_TRUE;
        } else if (contextPtr->lcuCostArray[lcuIndex] == contextPtr->costDepthMode[LCU_OPEN_LOOP_DEPTH_MODE - 1]) {
            pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] = LCU_OPEN_LOOP_DEPTH_MODE;
            pictureControlSetPtr->mdPresentFlag  = EB_TRUE;
        } else if (contextPtr->lcuCostArray[lcuIndex] == contextPtr->costDepthMode[LCU_LIGHT_BDP_DEPTH_MODE - 1]) {
            pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] = LCU_LIGHT_BDP_DEPTH_MODE;
            pictureControlSetPtr->bdpPresentFlag = EB_TRUE;
        } else if (contextPtr->lcuCostArray[lcuIndex] == contextPtr->costDepthMode[LCU_BDP_DEPTH_MODE - 1]) {
            pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] = LCU_BDP_DEPTH_MODE;
            pictureControlSetPtr->bdpPresentFlag = EB_TRUE;
        } else if (contextPtr->lcuCostArray[lcuIndex] == contextPtr->costDepthMode[LCU_AVC_DEPTH_MODE - 1]) {
            pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] = LCU_AVC_DEPTH_MODE;
            pictureControlSetPtr->mdPresentFlag  = EB_TRUE;
        } else if (contextPtr->lcuCostArray[lcuIndex] == contextPtr->costDepthMode[LCU_LIGHT_AVC_DEPTH_MODE - 1]) {
            pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] = LCU_LIGHT_AVC_DEPTH_MODE;
            pictureControlSetPtr->mdPresentFlag = EB_TRUE;
        } else if (pictureControlSetPtr->temporalLayerIndex == 0) {
            pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] = LCU_FULL84_DEPTH_MODE;
            pictureControlSetPtr->mdPresentFlag  = EB_TRUE;
        } else {
            pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] = LCU_FULL85_DEPTH_MODE;
            pictureControlSetPtr->mdPresentFlag  = EB_TRUE;
        }

    }
}

/******************************************************
* Set LCU budget
    Input   : LCU score, detection signals, iteration
    Output  : predicted budget for the LCU
******************************************************/
static void SetLcuBudget(
    SequenceControlSet_t                *sequenceControlSetPtr,
    PictureControlSet_t                 *pictureControlSetPtr,
    LargestCodingUnit_t                 *lcuPtr,
    ModeDecisionConfigurationContext_t  *contextPtr)
{
	const EB_U32      lcuIndex = lcuPtr->index;
    EB_U32      maxToMinScore, scoreToMin;

	const EB_BOOL isAvcPartitioningModeFlag = IsAvcPartitioningMode(sequenceControlSetPtr, pictureControlSetPtr, lcuPtr);


	if (contextPtr->adpRefinementMode == 2 && isAvcPartitioningModeFlag) {

        contextPtr->lcuCostArray[lcuIndex] = contextPtr->costDepthMode[LCU_AVC_DEPTH_MODE - 1];
        contextPtr->predictedCost += contextPtr->costDepthMode[LCU_AVC_DEPTH_MODE - 1];

    }
	else if (contextPtr->adpRefinementMode == 1 && isAvcPartitioningModeFlag) {

        contextPtr->lcuCostArray[lcuIndex] = contextPtr->costDepthMode[LCU_LIGHT_AVC_DEPTH_MODE - 1];
        contextPtr->predictedCost += contextPtr->costDepthMode[LCU_LIGHT_AVC_DEPTH_MODE - 1];

    }
    else {
        contextPtr->lcuScoreArray[lcuIndex] = CLIP3(contextPtr->lcuMinScore, contextPtr->lcuMaxScore, contextPtr->lcuScoreArray[lcuIndex]);
        scoreToMin = contextPtr->lcuScoreArray[lcuIndex] - contextPtr->lcuMinScore;
        maxToMinScore = contextPtr->lcuMaxScore - contextPtr->lcuMinScore;

        if ((scoreToMin <= (maxToMinScore * contextPtr->scoreTh[0]) / 100 && contextPtr->scoreTh[0] != 0) || contextPtr->numberOfSegments == 1 || contextPtr->scoreTh[1] == 100) {
            contextPtr->lcuCostArray[lcuIndex] = contextPtr->intervalCost[0];
            contextPtr->predictedCost += contextPtr->intervalCost[0];
        }
        else if ((scoreToMin <= (maxToMinScore * contextPtr->scoreTh[1]) / 100 && contextPtr->scoreTh[1] != 0) || contextPtr->numberOfSegments == 2 || contextPtr->scoreTh[2] == 100) {
            contextPtr->lcuCostArray[lcuIndex] = contextPtr->intervalCost[1];
            contextPtr->predictedCost += contextPtr->intervalCost[1];
        }
        else if ((scoreToMin <= (maxToMinScore * contextPtr->scoreTh[2]) / 100 && contextPtr->scoreTh[2] != 0) || contextPtr->numberOfSegments == 3 || contextPtr->scoreTh[3] == 100) {
            contextPtr->lcuCostArray[lcuIndex] = contextPtr->intervalCost[2];
            contextPtr->predictedCost += contextPtr->intervalCost[2];
        }
        else if ((scoreToMin <= (maxToMinScore * contextPtr->scoreTh[3]) / 100 && contextPtr->scoreTh[3] != 0) || contextPtr->numberOfSegments == 4 || contextPtr->scoreTh[4] == 100) {
            contextPtr->lcuCostArray[lcuIndex] = contextPtr->intervalCost[3];
            contextPtr->predictedCost += contextPtr->intervalCost[3];
        }
        else if ((scoreToMin <= (maxToMinScore * contextPtr->scoreTh[4]) / 100 && contextPtr->scoreTh[4] != 0) || contextPtr->numberOfSegments == 5 || contextPtr->scoreTh[5] == 100) {
            contextPtr->lcuCostArray[lcuIndex] = contextPtr->intervalCost[4];
            contextPtr->predictedCost += contextPtr->intervalCost[4];
        }
        else if ((scoreToMin <= (maxToMinScore * contextPtr->scoreTh[5]) / 100 && contextPtr->scoreTh[5] != 0) || contextPtr->numberOfSegments == 6 || contextPtr->scoreTh[6] == 100) {
            contextPtr->lcuCostArray[lcuIndex] = contextPtr->intervalCost[5];
            contextPtr->predictedCost += contextPtr->intervalCost[5];
        }
        else {
            contextPtr->lcuCostArray[lcuIndex] = contextPtr->intervalCost[6];
            contextPtr->predictedCost += contextPtr->intervalCost[6];
        }
        // Switch to AVC mode if the LCU cost is higher than the AVC cost and the the LCU is marked + adjust the current picture cost accordingly
		if (isAvcPartitioningModeFlag) {
            if (contextPtr->lcuCostArray[lcuIndex] > contextPtr->costDepthMode[LCU_AVC_DEPTH_MODE - 1]) {
                contextPtr->predictedCost -= (contextPtr->lcuCostArray[lcuIndex] - contextPtr->costDepthMode[LCU_AVC_DEPTH_MODE - 1]);
                contextPtr->lcuCostArray[lcuIndex] = contextPtr->costDepthMode[LCU_AVC_DEPTH_MODE - 1];
            }
            else if (contextPtr->lcuCostArray[lcuIndex] > contextPtr->costDepthMode[LCU_LIGHT_AVC_DEPTH_MODE - 1] && pictureControlSetPtr->temporalLayerIndex > 0) {
                contextPtr->predictedCost -= (contextPtr->lcuCostArray[lcuIndex] - contextPtr->costDepthMode[LCU_LIGHT_AVC_DEPTH_MODE - 1]);
                contextPtr->lcuCostArray[lcuIndex] = contextPtr->costDepthMode[LCU_LIGHT_AVC_DEPTH_MODE - 1];
            }
        }
    }
}

/******************************************************
* Loop multiple times over the LCUs in order to derive the optimal budget per LCU
    Input   : budget per picture, ditortion, detection signals, iteration
    Output  : optimal budget for each LCU
******************************************************/
static void  DeriveOptimalBudgetPerLcu(
    SequenceControlSet_t                *sequenceControlSetPtr,
    PictureControlSet_t                 *pictureControlSetPtr,
    ModeDecisionConfigurationContext_t  *contextPtr)
{

    EB_U32 lcuIndex;
    // Initialize the deviation between the picture predicted cost & the target budget to 100, 
    EB_U32 deviationToTarget    = 1000;    
    
    // Set the adjustment step to 1 (could be increased for faster convergence), 
    EB_S8  adjustementStep      =  1;
       
    // Set the initial shooting state & the final shooting state to TBD
    EB_U32 initialShooting  = TBD_SHOOTING;
    EB_U32 finalShooting    = TBD_SHOOTING;
   
    EB_U8 maxAdjustementIteration   = 100;
    EB_U8 adjustementIteration      =   0;

    while (deviationToTarget != 0 && (initialShooting == finalShooting) && adjustementIteration <= maxAdjustementIteration) {

        if (contextPtr->predictedCost < contextPtr->budget) {
            initialShooting =    UNDER_SHOOTING;
        }
        else {
            initialShooting =   OVER_SHOOTING;
        }

        // reset running cost
        contextPtr->predictedCost = 0;

		for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->ParentPcsPtr->lcuTotalCount; lcuIndex++) {

            LargestCodingUnit_t* lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];

            SetLcuBudget(
                sequenceControlSetPtr,
                pictureControlSetPtr,
                lcuPtr,
                contextPtr);
        }

        // Compute the deviation between the predicted budget & the target budget 
        deviationToTarget = (ABS((EB_S32)(contextPtr->predictedCost - contextPtr->budget)) * 1000) / contextPtr->budget;
        // Derive shooting status
        if (contextPtr->predictedCost < contextPtr->budget) {
            contextPtr->scoreTh[0] = MAX((contextPtr->scoreTh[0] - adjustementStep), 0);
            contextPtr->scoreTh[1] = MAX((contextPtr->scoreTh[1] - adjustementStep), 0);
            contextPtr->scoreTh[2] = MAX((contextPtr->scoreTh[2] - adjustementStep), 0);
            contextPtr->scoreTh[3] = MAX((contextPtr->scoreTh[3] - adjustementStep), 0);
            contextPtr->scoreTh[4] = MAX((contextPtr->scoreTh[4] - adjustementStep), 0);
            finalShooting = UNDER_SHOOTING;
        }
        else {
            contextPtr->scoreTh[0] = (contextPtr->scoreTh[0] == 0) ? 0 : MIN(contextPtr->scoreTh[0] + adjustementStep, 100);
            contextPtr->scoreTh[1] = (contextPtr->scoreTh[1] == 0) ? 0 : MIN(contextPtr->scoreTh[1] + adjustementStep, 100);
            contextPtr->scoreTh[2] = (contextPtr->scoreTh[2] == 0) ? 0 : MIN(contextPtr->scoreTh[2] + adjustementStep, 100);
            contextPtr->scoreTh[3] = (contextPtr->scoreTh[3] == 0) ? 0 : MIN(contextPtr->scoreTh[3] + adjustementStep, 100);
            contextPtr->scoreTh[4] = (contextPtr->scoreTh[4] == 0) ? 0 : MIN(contextPtr->scoreTh[4] + adjustementStep, 100);
            finalShooting = OVER_SHOOTING;
        }

        if (adjustementIteration == 0)
            initialShooting = finalShooting ;

        adjustementIteration ++;
    }
}


/******************************************************
* Compute the refinment cost
    Input   : budget per picture, and the cost of the refinment
    Output  : the refinment flag 
******************************************************/
static void ComputeRefinementCost(
    SequenceControlSet_t                *sequenceControlSetPtr,
    PictureControlSet_t                 *pictureControlSetPtr,
    ModeDecisionConfigurationContext_t  *contextPtr)
{
    
    EB_U32  lcuIndex;
    EB_U32  avcRefinementCost = 0;
    EB_U32  lightAvcRefinementCost = 0;

	for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->ParentPcsPtr->lcuTotalCount; lcuIndex++) {
        LargestCodingUnit_t* lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];
        if (IsAvcPartitioningMode(sequenceControlSetPtr, pictureControlSetPtr, lcuPtr)) {
            avcRefinementCost       += contextPtr->costDepthMode[LCU_AVC_DEPTH_MODE - 1];
            lightAvcRefinementCost  += contextPtr->costDepthMode[LCU_LIGHT_AVC_DEPTH_MODE - 1];

        }
        // assumes the fastest mode will be used otherwise
        else {
            avcRefinementCost       += contextPtr->intervalCost[0];
            lightAvcRefinementCost  += contextPtr->intervalCost[0];
        }
    }

    // Shut the refinement if the refinement cost is higher than the allocated budget
    if (avcRefinementCost <= contextPtr->budget && ((contextPtr->budget > (EB_U32)(LIGHT_BDP_COST * pictureControlSetPtr->ParentPcsPtr->lcuTotalCount)) || pictureControlSetPtr->temporalLayerIndex == 0 || (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE && pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE))) {
        contextPtr->adpRefinementMode = ADP_MODE_1;
    }
    else if (lightAvcRefinementCost <= contextPtr->budget && pictureControlSetPtr->temporalLayerIndex > 0) {
        contextPtr->adpRefinementMode = ADP_MODE_0;
    }
    else {
        contextPtr->adpRefinementMode = ADP_REFINMENT_OFF;
    }

}

/******************************************************
* Compute the score of each LCU
    Input   : distortion, detection signals
    Output  : LCU score
******************************************************/
static void DeriveLcuScore(
    SequenceControlSet_t               *sequenceControlSetPtr,
	PictureControlSet_t                *pictureControlSetPtr,
	ModeDecisionConfigurationContext_t *contextPtr)
{
	EB_U32  lcuIndex;
    EB_U32  lcuScore;
    EB_U32  distortion;

    contextPtr->lcuMinScore = ~0u;
    contextPtr->lcuMaxScore =  0u;

	for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; lcuIndex++) {

		LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            if (lcuParams->rasterScanCuValidity[RASTER_SCAN_CU_INDEX_64x64] == EB_FALSE) {

                EB_U8 cu8x8Index;
                EB_U8 validCu8x8Count = 0;
                distortion = 0;
                for (cu8x8Index = 0; cu8x8Index < 64; cu8x8Index++) {
                    if (lcuParams->rasterScanCuValidity[cu8x8Index]) {
                        distortion += pictureControlSetPtr->ParentPcsPtr->oisCu8Results[lcuIndex]->sortedOisCandidate[cu8x8Index][0].distortion;
                        validCu8x8Count++;
                    }
                }
                if (validCu8x8Count > 0)
                    distortion = CLIP3(pictureControlSetPtr->ParentPcsPtr->intraComplexityMinPre, pictureControlSetPtr->ParentPcsPtr->intraComplexityMaxPre, (distortion / validCu8x8Count) * 64);
            }
            else {
                distortion = pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[RASTER_SCAN_CU_INDEX_32x32_0][0].distortion +
                    pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[RASTER_SCAN_CU_INDEX_32x32_1][0].distortion +
                    pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[RASTER_SCAN_CU_INDEX_32x32_2][0].distortion +
                    pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuIndex]->sortedOisCandidate[RASTER_SCAN_CU_INDEX_32x32_3][0].distortion;
            }
            lcuScore = distortion;

		}
		else {
            if (lcuParams->rasterScanCuValidity[RASTER_SCAN_CU_INDEX_64x64] == EB_FALSE) {

                EB_U8 cu8x8Index;
                EB_U8 validCu8x8Count = 0;
                distortion = 0;
                for (cu8x8Index = RASTER_SCAN_CU_INDEX_8x8_0; cu8x8Index <= RASTER_SCAN_CU_INDEX_8x8_63; cu8x8Index++) {
                    if (lcuParams->rasterScanCuValidity[cu8x8Index]) {
                        distortion += pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex][cu8x8Index].distortionDirection[0].distortion;
                        validCu8x8Count++;
                    }
                }
                if (validCu8x8Count > 0)
                    distortion = CLIP3(pictureControlSetPtr->ParentPcsPtr->interComplexityMinPre, pictureControlSetPtr->ParentPcsPtr->interComplexityMaxPre, (distortion / validCu8x8Count) * 64);

                // Do not perform LCU score manipulation for incomplete LCUs as not valid signals
                lcuScore   = distortion;

            }
            else {
                distortion = pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex][RASTER_SCAN_CU_INDEX_64x64].distortionDirection[0].distortion;

                lcuScore = distortion;

                // Use uncovered area 
                if (pictureControlSetPtr->ParentPcsPtr->failingMotionLcuFlag[lcuIndex]) {
                    lcuScore = pictureControlSetPtr->ParentPcsPtr->interComplexityMaxPre;

                }
                // Active LCUs @ picture boundaries
                else if (
                    // LCU @ a picture boundary
                    lcuParams->isEdgeLcu
                    && pictureControlSetPtr->ParentPcsPtr->nonMovingIndexArray[lcuIndex] != INVALID_ZZ_COST 
                    && pictureControlSetPtr->ParentPcsPtr->nonMovingIndexAverage         != INVALID_ZZ_COST
                    // Active LCU
                    && pictureControlSetPtr->ParentPcsPtr->nonMovingIndexArray[lcuIndex] >= ADP_CLASS_NON_MOVING_INDEX_TH_0
                    // Active Picture or LCU belongs to the most active LCUs
                    && (pictureControlSetPtr->ParentPcsPtr->nonMovingIndexArray[lcuIndex] >= pictureControlSetPtr->ParentPcsPtr->nonMovingIndexAverage || pictureControlSetPtr->ParentPcsPtr->nonMovingIndexAverage > ADP_CLASS_NON_MOVING_INDEX_TH_1)
                    // Off for sub-4K (causes instability as % of picture boundary LCUs is 2x higher for 1080p than for 4K (9% vs. 18% ) => might hurt the non-boundary LCUs)
                    && sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {

                    lcuScore += (((pictureControlSetPtr->ParentPcsPtr->interComplexityMaxPre - lcuScore) * ADP_CLASS_SHIFT_DIST_1) / 100);

                }
                else {

                    // Use LCU variance & activity    
                    if (pictureControlSetPtr->ParentPcsPtr->nonMovingIndexArray[lcuIndex] == ADP_CLASS_NON_MOVING_INDEX_TH_2 && pictureControlSetPtr->ParentPcsPtr->variance[lcuIndex][RASTER_SCAN_CU_INDEX_64x64] > IS_COMPLEX_LCU_VARIANCE_TH && (sequenceControlSetPtr->staticConfig.frameRate >> 16) > 30)

                        lcuScore -= (((lcuScore - pictureControlSetPtr->ParentPcsPtr->interComplexityMinPre) * ADP_CLASS_SHIFT_DIST_0) / 100);
                    // Use LCU luminosity
                    if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE){
                        // Shift to the left dark LCUs & shift to the right otherwise ONLY if a high dark area is present
                        if (pictureControlSetPtr->ParentPcsPtr->blackAreaPercentage > ADP_BLACK_AREA_PERCENTAGE) {
                            if (pictureControlSetPtr->ParentPcsPtr->yMean[lcuIndex][RASTER_SCAN_CU_INDEX_64x64] < ADP_DARK_LCU_TH)
                                lcuScore -= (((lcuScore - pictureControlSetPtr->ParentPcsPtr->interComplexityMinPre) * ADP_CLASS_SHIFT_DIST_0) / 100);
                            else
                                lcuScore += (((pictureControlSetPtr->ParentPcsPtr->interComplexityMaxPre - lcuScore) * ADP_CLASS_SHIFT_DIST_0) / 100);
                        }
                    } else {
                        // Shift to the left dark LCUs & shift to the right otherwise
                        if (pictureControlSetPtr->ParentPcsPtr->yMean[lcuIndex][RASTER_SCAN_CU_INDEX_64x64] < ADP_DARK_LCU_TH )
                            lcuScore -= (((lcuScore - pictureControlSetPtr->ParentPcsPtr->interComplexityMinPre) * ADP_CLASS_SHIFT_DIST_0) / 100);
                        else
                            lcuScore += (((pictureControlSetPtr->ParentPcsPtr->interComplexityMaxPre - lcuScore) * ADP_CLASS_SHIFT_DIST_0) / 100);                        
                    }

                }

            }
		}
            
            contextPtr->lcuScoreArray[lcuIndex] = lcuScore;

            // Track MIN & MAX LCU scores
            contextPtr->lcuMinScore = MIN(contextPtr->lcuScoreArray[lcuIndex], contextPtr->lcuMinScore);
            contextPtr->lcuMaxScore = MAX(contextPtr->lcuScoreArray[lcuIndex], contextPtr->lcuMaxScore);
    }


   
}

/******************************************************
* BudgetingOutlierRemovalLcu
    Input   : LCU score histogram
    Output  : Adjusted min & max LCU score (to be used to clip the LCU score @ SetLcuBudget)
 Performs outlier removal by:
 1. dividing the total distance between the maximum lcuScore and the minimum lcuScore into NI intervals(NI = 10).
 For each lcuScore interval,
 2. computing the number of LCUs NV with lcuScore belonging to the subject lcuScore interval.
 3. marking the subject lcuScore interval as not valid if its NV represent less than validity threshold V_TH per - cent of the total number of the processed LCUs in the picture. (V_TH = 2)
 4. computing the distance MIN_D from 0 and the index of the first, in incremental way, lcuScore interval marked as valid in the prior step.
 5. computing the distance MAX_D from NI and the index of the first, in decreasing way, lcuScore interval marked as valid in the prior step.
 6. adjusting the minimum and maximum lcuScore as follows :
    MIN_SCORE = MIN_SCORE + MIN_D * I_Value.
    MAX_SCORE = MAX_SCORE - MAX_D * I_Value.
******************************************************/

static void PerformOutlierRemoval(
	SequenceControlSet_t                *sequenceControlSetPtr,
	PictureParentControlSet_t           *pictureControlSetPtr,
    ModeDecisionConfigurationContext_t  *contextPtr)
{

	EB_U32 maxInterval = 0;
	EB_U32 subInterval = 0;
	EB_U32 lcuScoreHistogram[10] = { 0 };
	EB_U32 lcuIndex;
    EB_U32 lcuScore;
	EB_U32 processedlcuCount = 0;
	EB_S32 slot = 0;

    maxInterval = contextPtr->lcuMaxScore - contextPtr->lcuMinScore;
    // Consider 10 bins
    subInterval = maxInterval / 10;

    // Count # of LCUs at each bin
	for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; lcuIndex++) {

		LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

		if (lcuParams->rasterScanCuValidity[RASTER_SCAN_CU_INDEX_64x64]) {

			processedlcuCount++;

            lcuScore = contextPtr->lcuScoreArray[lcuIndex] + contextPtr->lcuMinScore;
            if (lcuScore < (subInterval + contextPtr->lcuMinScore)){
                lcuScoreHistogram[0]++;
            }
            else if (lcuScore < ((2 * subInterval) + contextPtr->lcuMinScore)){
                lcuScoreHistogram[1]++;
            }
            else if (lcuScore < ((3 * subInterval) + contextPtr->lcuMinScore)){
                lcuScoreHistogram[2]++;
            }
            else if (lcuScore < ((4 * subInterval) + contextPtr->lcuMinScore)){
                lcuScoreHistogram[3]++;
            }
            else if (lcuScore < ((5 * subInterval) + contextPtr->lcuMinScore)){
                lcuScoreHistogram[4]++;
            }
            else if (lcuScore < ((6 * subInterval) + contextPtr->lcuMinScore)){
                lcuScoreHistogram[5]++;
            }
            else if (lcuScore < ((7 * subInterval) + contextPtr->lcuMinScore)){
                lcuScoreHistogram[6]++;
            }
            else if (lcuScore < ((8 * subInterval) + contextPtr->lcuMinScore)){
                lcuScoreHistogram[7]++;
            }
            else if (lcuScore < ((9 * subInterval) + contextPtr->lcuMinScore)){
                lcuScoreHistogram[8]++;
            }
            else if (lcuScore < ((10 * subInterval) + contextPtr->lcuMinScore)){
                lcuScoreHistogram[9]++;
            }
		}
	}
	
    // Zero-out the bin if percentage lower than VALID_SLOT_TH
	for (slot = 0; slot < 10; slot++){
        if (processedlcuCount > 0) {
            if ((lcuScoreHistogram[slot] * 100 / processedlcuCount) < VALID_SLOT_TH) {
                lcuScoreHistogram[slot] = 0;
            }
        }
	}

    // Ignore null bins
	for (slot = 0; slot < 10; slot++){
		if (lcuScoreHistogram[slot]){
			contextPtr->lcuMinScore = contextPtr->lcuMinScore+ (slot * subInterval);
			break;
		}
	}

	for (slot = 9; slot >= 0; slot--){
		if (lcuScoreHistogram[slot]){
			contextPtr->lcuMaxScore = contextPtr->lcuMaxScore - ((9 - slot) * subInterval);
			break;
		}
	}
}
/******************************************************
* Assign a search method for each LCU 
    Input   : LCU score, detection signals
    Output  : search method for each LCU
******************************************************/
static void DeriveLcuMdMode(
    SequenceControlSet_t                *sequenceControlSetPtr,
	PictureControlSet_t                 *pictureControlSetPtr,
    ModeDecisionConfigurationContext_t  *contextPtr) {

    // Configure ADP 
    ConfigureAdp(
        pictureControlSetPtr,
        contextPtr);

    // Set the target budget
    SetTargetBudgetOq(
            sequenceControlSetPtr,
            pictureControlSetPtr,
            contextPtr);

    // Set the percentage based thresholds
    DeriveDefaultSegments(
        pictureControlSetPtr,
        contextPtr);

    // Compute the cost of the refinements 
    ComputeRefinementCost(
        sequenceControlSetPtr,
        pictureControlSetPtr,
        contextPtr);

    // Derive LCU score
	DeriveLcuScore(
		sequenceControlSetPtr,
		pictureControlSetPtr,
        contextPtr);

    // Remove the outliers 
	PerformOutlierRemoval(
		sequenceControlSetPtr,
		pictureControlSetPtr->ParentPcsPtr,
        contextPtr);

    // Perform Budgetting
    DeriveOptimalBudgetPerLcu(
        sequenceControlSetPtr,
        pictureControlSetPtr,
        contextPtr);

    // Set the search method using the LCU cost (mapping)
    DeriveSearchMethod(
        pictureControlSetPtr,
        contextPtr);

}



/******************************************************
* Derive Mode Decision Config Settings for OQ
Input   : encoder mode and tune
Output  : EncDec Kernel signal(s)
******************************************************/
static EB_ERRORTYPE SignalDerivationModeDecisionConfigKernelOq(
    PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionConfigurationContext_t     *contextPtr) {

    EB_ERRORTYPE return_error = EB_ErrorNone;

    contextPtr->adpLevel = pictureControlSetPtr->ParentPcsPtr->encMode;
    
    // Derive chroma Qp Offset
    // 0 : 2 Layer1 0 OW 
    // 1 : MOD_QP_OFFSET -3
    contextPtr->chromaQpOffsetLevel  = 1;
    
    return return_error;
}


/******************************************************
 * Mode Decision Configuration Kernel
 ******************************************************/
void* ModeDecisionConfigurationKernel(void *inputPtr)
{
    // Context & SCS & PCS
    ModeDecisionConfigurationContext_t         *contextPtr = (ModeDecisionConfigurationContext_t*) inputPtr;
    PictureControlSet_t                        *pictureControlSetPtr;
    SequenceControlSet_t                       *sequenceControlSetPtr;

    // Input
    EbObjectWrapper_t                          *rateControlResultsWrapperPtr;
    RateControlResults_t                       *rateControlResultsPtr;

    // Output
    EbObjectWrapper_t                          *encDecTasksWrapperPtr;
    EncDecTasks_t                              *encDecTasksPtr;
    EB_U32                                      pictureWidthInLcu;
	EB_U32                                      pictureHeightInLcu;

    for(;;) {
		// Get RateControl Results
		EbGetFullObject(
			contextPtr->rateControlInputFifoPtr,
			&rateControlResultsWrapperPtr);
        EB_CHECK_END_OBJ(rateControlResultsWrapperPtr);

		rateControlResultsPtr = (RateControlResults_t*)rateControlResultsWrapperPtr->objectPtr;
		pictureControlSetPtr = (PictureControlSet_t*)rateControlResultsPtr->pictureControlSetWrapperPtr->objectPtr;
		sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld MDC IN \n", pictureControlSetPtr->pictureNumber);
#endif
        SignalDerivationModeDecisionConfigKernelOq(
                pictureControlSetPtr,
                contextPtr);

		pictureWidthInLcu = (sequenceControlSetPtr->lumaWidth + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;
		pictureHeightInLcu = (sequenceControlSetPtr->lumaHeight + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;

        contextPtr->qp = pictureControlSetPtr->pictureQp;

		pictureControlSetPtr->ParentPcsPtr->averageQp = 0;

		pictureControlSetPtr->intraCodedArea = 0;

        pictureControlSetPtr->sceneCaracteristicId = EB_FRAME_CARAC_0;

        EB_PICNOISE_CLASS picNoiseClassTH = (pictureControlSetPtr->ParentPcsPtr->noiseDetectionTh == 0) ? PIC_NOISE_CLASS_1 : PIC_NOISE_CLASS_3;

        pictureControlSetPtr->sceneCaracteristicId = (
            (!pictureControlSetPtr->ParentPcsPtr->isPan) &&
            (!pictureControlSetPtr->ParentPcsPtr->isTilt) &&
            (pictureControlSetPtr->ParentPcsPtr->grassPercentageInPicture > 0) &&
            (pictureControlSetPtr->ParentPcsPtr->grassPercentageInPicture <= 35) &&
            (pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= picNoiseClassTH) &&
            (pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage < 50)) ? EB_FRAME_CARAC_1 : pictureControlSetPtr->sceneCaracteristicId;
        
        pictureControlSetPtr->sceneCaracteristicId = (
			(pictureControlSetPtr->ParentPcsPtr->isPan) &&
			(!pictureControlSetPtr->ParentPcsPtr->isTilt) &&
			(pictureControlSetPtr->ParentPcsPtr->grassPercentageInPicture > 35) &&
			(pictureControlSetPtr->ParentPcsPtr->grassPercentageInPicture <= 70) &&
			(pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= picNoiseClassTH) &&
            (pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage < 50)) ? EB_FRAME_CARAC_2 : pictureControlSetPtr->sceneCaracteristicId;

        pictureControlSetPtr->adjustMinQPFlag = (EB_BOOL)((!pictureControlSetPtr->ParentPcsPtr->isPan) &&
            (!pictureControlSetPtr->ParentPcsPtr->isTilt) &&
            (pictureControlSetPtr->ParentPcsPtr->grassPercentageInPicture > 2) &&
            (pictureControlSetPtr->ParentPcsPtr->grassPercentageInPicture <= 35) &&
            (pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage < 70) &&
            (pictureControlSetPtr->ParentPcsPtr->zzCostAverage > 15) &&
            (pictureControlSetPtr->ParentPcsPtr->picNoiseClass >= picNoiseClassTH));


		// Aura Detection
		// Still is using the picture QP to derive aura thresholds, there fore it could not move to the open loop
		AuraDetection(  // HT done 
			sequenceControlSetPtr,
			pictureControlSetPtr,
			pictureWidthInLcu,
			pictureHeightInLcu);

		// Detect complex/non-flat/moving LCU in a non-complex area (used to refine MDC depth control)
		DetectComplexNonFlatMovingLcu( // HT done
            sequenceControlSetPtr,
			pictureControlSetPtr,
			pictureWidthInLcu);

		// Compute picture and slice level chroma QP offsets 
        SetSliceAndPictureChromaQpOffsets( // HT done 
            pictureControlSetPtr,
            contextPtr);

		// Compute Tc, and Beta offsets for a given picture
		AdaptiveDlfParameterComputation( // HT done 
			contextPtr,
			pictureControlSetPtr);


        if (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE) {

            DeriveLcuMdMode(
                sequenceControlSetPtr,
                pictureControlSetPtr,
                contextPtr);


            EB_U32 lcuIndex;

            // Rate estimation/QP
            PartitioningInitialization(
                sequenceControlSetPtr,
                pictureControlSetPtr,
                contextPtr);

            // LCU Loop : Partitionnig Decision
            for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex) {


                if (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_FULL85_DEPTH_MODE) {
                    Forward85CuToModeDecisionLCU(
                        sequenceControlSetPtr,
                        pictureControlSetPtr,
                        lcuIndex);
                }
                else if (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_FULL84_DEPTH_MODE) {
                    Forward84CuToModeDecisionLCU(
                        sequenceControlSetPtr,
                        pictureControlSetPtr,
                        lcuIndex);
                }
                else if (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_AVC_DEPTH_MODE) {
                    Forward8x816x16CuToModeDecisionLCU(
                        sequenceControlSetPtr,
                        pictureControlSetPtr,
                        lcuIndex);
                }
                else if (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_LIGHT_AVC_DEPTH_MODE) {
                    Forward16x16CuToModeDecisionLCU(
                        sequenceControlSetPtr,
                        pictureControlSetPtr,
                        lcuIndex);
                }
                else if (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_OPEN_LOOP_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_LIGHT_OPEN_LOOP_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_PRED_OPEN_LOOP_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE) {

                    // Predict the LCU partitionning
                    PerformEarlyLcuPartitionningLcu( // HT done 
                        contextPtr,
                        sequenceControlSetPtr,
                        pictureControlSetPtr,
                        lcuIndex);                 
                }
            }
        }
        else  if (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL85_DEPTH_MODE){

            Forward85CuToModeDecision( 
                sequenceControlSetPtr,
                pictureControlSetPtr);
        }
        else  if (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL84_DEPTH_MODE) {

            Forward84CuToModeDecision(
                sequenceControlSetPtr,
                pictureControlSetPtr);
        }
        else if (pictureControlSetPtr->ParentPcsPtr->depthMode >= PICT_OPEN_LOOP_DEPTH_MODE){

                // Predict the LCU partitionning
                PerformEarlyLcuPartitionning( // HT done 
                    contextPtr,
                    sequenceControlSetPtr,
                    pictureControlSetPtr);  
        }
        else {   // (pictureControlSetPtr->ParentPcsPtr->mdMode == PICT_BDP_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->mdMode == PICT_LIGHT_BDP_DEPTH_MODE )
            pictureControlSetPtr->ParentPcsPtr->averageQp = (EB_U8)pictureControlSetPtr->ParentPcsPtr->pictureQp; 
        }

#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld MDC OUT \n", pictureControlSetPtr->pictureNumber);
#endif
        // Post the results to the MD processes
        EB_U16 tileGroupRowCnt = sequenceControlSetPtr->tileGroupRowCountArray[pictureControlSetPtr->temporalLayerIndex];
        EB_U16 tileGroupColCnt = sequenceControlSetPtr->tileGroupColCountArray[pictureControlSetPtr->temporalLayerIndex]; 

        for (EB_U16 r = 0; r < tileGroupRowCnt; r++) {
            for (EB_U16 c = 0; c < tileGroupColCnt; c++) {
                unsigned tileGroupIdx = c + r * tileGroupColCnt;
                EbGetEmptyObject(
                        contextPtr->modeDecisionConfigurationOutputFifoPtr,
                        &encDecTasksWrapperPtr);
                encDecTasksPtr = (EncDecTasks_t*) encDecTasksWrapperPtr->objectPtr;
                encDecTasksPtr->pictureControlSetWrapperPtr = rateControlResultsPtr->pictureControlSetWrapperPtr;
                encDecTasksPtr->inputType = ENCDEC_TASKS_MDC_INPUT;
                encDecTasksPtr->tileGroupIndex = tileGroupIdx;

                // Post the Full Results Object
                EbPostFullObject(encDecTasksWrapperPtr);
            }
        }

#if LATENCY_PROFILE
        double latency = 0.0;
        EB_U64 finishTimeSeconds = 0;
        EB_U64 finishTimeuSeconds = 0;
        EbFinishTime((uint64_t*)&finishTimeSeconds, (uint64_t*)&finishTimeuSeconds);

        EbComputeOverallElapsedTimeMs(
                pictureControlSetPtr->ParentPcsPtr->startTimeSeconds,
                pictureControlSetPtr->ParentPcsPtr->startTimeuSeconds,
                finishTimeSeconds,
                finishTimeuSeconds,
                &latency);

        SVT_LOG("POC %lld MDC OUT, decoder order %d, latency %3.3f \n",
                pictureControlSetPtr->ParentPcsPtr->pictureNumber,
                pictureControlSetPtr->ParentPcsPtr->decodeOrder,
                latency);
#endif

        // Release Rate Control Results
        EbReleaseObject(rateControlResultsWrapperPtr);

	}

	return EB_NULL;
}
