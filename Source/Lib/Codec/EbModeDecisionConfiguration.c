/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#include "EbModeDecisionConfiguration.h"
#include "EbLambdaRateTables.h"
#include "EbUtility.h"
#include "EbModeDecisionProcess.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"


/********************************************
 * Constants
 ********************************************/
#define ADD_CU_STOP_SPLIT             0   // Take into account & Stop Splitting
#define ADD_CU_CONTINUE_SPLIT         1   // Take into account & Continue Splitting
#define DO_NOT_ADD_CU_CONTINUE_SPLIT  2   // Do not take into account & Continue Splitting

#define DEPTH_64                      0   // Depth corresponding to the CU size
#define DEPTH_32                      1   // Depth corresponding to the CU size
#define DEPTH_16                      2   // Depth corresponding to the CU size
#define DEPTH_8                       3   // Depth corresponding to the CU size


#define PREDICTED           0
#define PREDICTED_PLUS_1    1
#define PREDICTED_PLUS_2    2
#define PREDICTED_PLUS_3    3
#define ALL_DEPTH           4
#define PREDICTED_MINUS_1   5

EB_U8 INVALID_QP = 0xFF;
static const EB_U8 parentCuIndex[85] =
{
    0,
    0, 0, 0, 1, 2, 3, 5, 0, 1, 2, 3, 10, 0, 1, 2, 3, 15, 0, 1, 2, 3,
    21, 0, 0, 1, 2, 3, 5, 0, 1, 2, 3, 10, 0, 1, 2, 3, 15, 0, 1, 2, 3,
    42, 0, 0, 1, 2, 3, 5, 0, 1, 2, 3, 10, 0, 1, 2, 3, 15, 0, 1, 2, 3,
    36, 0, 0, 1, 2, 3, 5, 0, 1, 2, 3, 10, 0, 1, 2, 3, 15, 0, 1, 2, 3,
};

const EB_U8 incrementalCount[85] = {

    //64x64
    0,
    //32x32
    4, 4,
    4, 4,
    //16x16
    0, 0, 0, 0,
    0, 4, 0, 4,
    0, 0, 0, 0,
    0, 4, 0, 4,
    //8x8
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 4, 0, 0, 0, 4,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 4, 0, 0, 0, 4

};

/*******************************************
mdcSetDepth : set depth to be tested
*******************************************/
#define REFINEMENT_P        0x01
#define REFINEMENT_Pp1      0x02
#define REFINEMENT_Pp2      0x04
#define REFINEMENT_Pp3      0x08
#define REFINEMENT_Pm1      0x10
#define REFINEMENT_Pm2      0x20
#define REFINEMENT_Pm3      0x40



EB_ERRORTYPE MdcRefinement(
    MdcpLocalCodingUnit_t                   *localCuArray,
    EB_U32                                  cuIndex,
    EB_U32                                  depth,
    EB_U8                                   refinementLevel,
    EB_U8                                   lowestLevel)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;


    if (refinementLevel & REFINEMENT_P){
        if (lowestLevel == REFINEMENT_P){
            localCuArray[cuIndex].stopSplit = EB_TRUE;
        }

    }
    else{
        localCuArray[cuIndex].slectedCu = EB_FALSE;
    }

    if (refinementLevel & REFINEMENT_Pp1){
		
        if (depth < 3 && cuIndex < 81){
            localCuArray[cuIndex + 1].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + DepthOffset[depth + 1]].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 2 * DepthOffset[depth + 1]].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 3 * DepthOffset[depth + 1]].slectedCu = EB_TRUE;
        }
        if (lowestLevel == REFINEMENT_Pp1){
			if (depth < 3 && cuIndex < 81){
                localCuArray[cuIndex + 1].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + DepthOffset[depth + 1]].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 2 * DepthOffset[depth + 1]].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 3 * DepthOffset[depth + 1]].stopSplit = EB_TRUE;
            }
        }
    }

    if (refinementLevel & REFINEMENT_Pp2){
        if (depth < 2 && cuIndex < 65){
            localCuArray[cuIndex + 1 + 1].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 1 + DepthOffset[depth + 2]].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 1 + 2 * DepthOffset[depth + 2]].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 1 + 3 * DepthOffset[depth + 2]].slectedCu = EB_TRUE;

            localCuArray[cuIndex + 1 + DepthOffset[depth + 1] + 1].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + DepthOffset[depth + 1] + 1 + DepthOffset[depth + 2]].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + DepthOffset[depth + 1] + 1 + 2 * DepthOffset[depth + 2]].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + DepthOffset[depth + 1] + 1 + 3 * DepthOffset[depth + 2]].slectedCu = EB_TRUE;

            localCuArray[cuIndex + 1 + 2 * DepthOffset[depth + 1] + 1].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 2 * DepthOffset[depth + 1] + 1 + DepthOffset[depth + 2]].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 2 * DepthOffset[depth + 1] + 1 + 2 * DepthOffset[depth + 2]].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 2 * DepthOffset[depth + 1] + 1 + 3 * DepthOffset[depth + 2]].slectedCu = EB_TRUE;

            localCuArray[cuIndex + 1 + 3 * DepthOffset[depth + 1] + 1].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 3 * DepthOffset[depth + 1] + 1 + DepthOffset[depth + 2]].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 3 * DepthOffset[depth + 1] + 1 + 2 * DepthOffset[depth + 2]].slectedCu = EB_TRUE;
            localCuArray[cuIndex + 1 + 3 * DepthOffset[depth + 1] + 1 + 3 * DepthOffset[depth + 2]].slectedCu = EB_TRUE;
        }
        if (lowestLevel == REFINEMENT_Pp2){
            if (depth < 2 && cuIndex < 65){
                localCuArray[cuIndex + 1 + 1].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 1 + DepthOffset[depth + 2]].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 1 + 2 * DepthOffset[depth + 2]].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 1 + 3 * DepthOffset[depth + 2]].stopSplit = EB_TRUE;

                localCuArray[cuIndex + 1 + DepthOffset[depth + 1] + 1].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + DepthOffset[depth + 1] + 1 + DepthOffset[depth + 2]].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + DepthOffset[depth + 1] + 1 + 2 * DepthOffset[depth + 2]].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + DepthOffset[depth + 1] + 1 + 3 * DepthOffset[depth + 2]].stopSplit = EB_TRUE;

                localCuArray[cuIndex + 1 + 2 * DepthOffset[depth + 1] + 1].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 2 * DepthOffset[depth + 1] + 1 + DepthOffset[depth + 2]].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 2 * DepthOffset[depth + 1] + 1 + 2 * DepthOffset[depth + 2]].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 2 * DepthOffset[depth + 1] + 1 + 3 * DepthOffset[depth + 2]].stopSplit = EB_TRUE;

                localCuArray[cuIndex + 1 + 3 * DepthOffset[depth + 1] + 1].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 3 * DepthOffset[depth + 1] + 1 + DepthOffset[depth + 2]].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 3 * DepthOffset[depth + 1] + 1 + 2 * DepthOffset[depth + 2]].stopSplit = EB_TRUE;
                localCuArray[cuIndex + 1 + 3 * DepthOffset[depth + 1] + 1 + 3 * DepthOffset[depth + 2]].stopSplit = EB_TRUE;
            }
        }
    }

    if (refinementLevel & REFINEMENT_Pp3){
        EB_U8 inLoop;
        EB_U8 outLoop;
        EB_U8 cuIndex = 2;
        if (depth == 0){

            for (outLoop = 0; outLoop < 16; ++outLoop){
                for (inLoop = 0; inLoop < 4; ++inLoop){
                    localCuArray[++cuIndex].slectedCu = EB_TRUE;

                }
                cuIndex += cuIndex == 21 ? 2 : cuIndex == 42 ? 2 : cuIndex == 63 ? 2 : 1;

            }
            if (lowestLevel == REFINEMENT_Pp3){
                cuIndex = 2;
                for (outLoop = 0; outLoop < 16; ++outLoop){
                    for (inLoop = 0; inLoop < 4; ++inLoop){
                        localCuArray[++cuIndex].stopSplit = EB_TRUE;
                    }
                    cuIndex += cuIndex == 21 ? 2 : cuIndex == 42 ? 2 : cuIndex == 63 ? 2 : 1;
                }
            }
        }

    }

    if (refinementLevel & REFINEMENT_Pm1){
        if (depth > 0){
            localCuArray[cuIndex - 1 - parentCuIndex[cuIndex]].slectedCu = EB_TRUE;
        }
        if (lowestLevel == REFINEMENT_Pm1){
            if (depth > 0){
                localCuArray[cuIndex - 1 - parentCuIndex[cuIndex]].stopSplit = EB_TRUE;
            }
        }
    }

    if (refinementLevel & REFINEMENT_Pm2){
        if (depth == 2){
            localCuArray[0].slectedCu = EB_TRUE;
        }
        if (depth == 3){
            localCuArray[1].slectedCu = EB_TRUE;
            localCuArray[22].slectedCu = EB_TRUE;
            localCuArray[43].slectedCu = EB_TRUE;
            localCuArray[64].slectedCu = EB_TRUE;
        }
        if (lowestLevel == REFINEMENT_Pm2){
            if (depth == 2){
                localCuArray[0].stopSplit = EB_TRUE;
            }
            if (depth == 3){
                localCuArray[1].stopSplit = EB_TRUE;
                localCuArray[22].stopSplit = EB_TRUE;
                localCuArray[43].stopSplit = EB_TRUE;
                localCuArray[64].stopSplit = EB_TRUE;
            }
        }
    }

    if (refinementLevel & REFINEMENT_Pm3){
        if (depth == 3){
            localCuArray[0].slectedCu = EB_TRUE;
        }
        if (lowestLevel == REFINEMENT_Pm2){
            if (depth == 3){
                localCuArray[0].stopSplit = EB_TRUE;
            }
        }
    }

    return return_error;
}

/*******************************************
Cost Computation for intra CU
*******************************************/

EB_ERRORTYPE MdcIntraCuRate(
    EB_U32                                  cuDepth,
    MdRateEstimationContext_t               *mdRateEstimationPtr,
    PictureControlSet_t                    *pictureControlSetPtr,
    EB_U64                                  *mdcIntraRate)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;


    EB_U32 chromaMode = EB_INTRA_CHROMA_DM;
    EB_PART_MODE partitionMode = SIZE_2Nx2N;
    EB_S32 predictionIndex = -1;

    // Number of bits for each synatax element
    EB_U64 partSizeIntraBitsNum = 0;
    EB_U64 intraLumaModeBitsNum = 0;

    // Luma and chroma rate
    EB_U64 lumaRate;
    EB_U64 chromaRate;

    EncodeContext_t *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;
    
    CHECK_REPORT_ERROR(
        (partitionMode == SIZE_2Nx2N),
        encodeContextPtr->appCallbackPtr,
        EB_ENC_RD_COST_ERROR3);

    // Estimate Chroma Mode Bits
    chromaRate = mdRateEstimationPtr->intraChromaBits[chromaMode];

    // Estimate Partition Size Bits :
    // *Note - Intra is implicitly 2Nx2N
    partSizeIntraBitsNum = ((EB_U8)cuDepth == (((SequenceControlSet_t *)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->maxLcuDepth - 1)) ?
        mdRateEstimationPtr->intraPartSizeBits[partitionMode] :
        ZERO_COST;

    // Estimate Luma Mode Bits for Intra
    /*IntraLumaModeContext(
        cuPtr,
        lumaMode,
        &predictionIndex);*/

    intraLumaModeBitsNum = (predictionIndex != -1) ?
        mdRateEstimationPtr->intraLumaBits[predictionIndex] :
        mdRateEstimationPtr->intraLumaBits[3];

    // Rate of the candiadate mode is equal to the sum of the rate of independent syntax element
    lumaRate =
        partSizeIntraBitsNum +
        intraLumaModeBitsNum;

    // Assign fast cost
    *(mdcIntraRate) = chromaRate + lumaRate;

    return return_error;
}

/*******************************************
Cost Computation for inter CU
*******************************************/
EB_U64 MdcInterCuRate(
    EB_U32                                   direction,
    EB_S16                                   xMvL0,
    EB_S16                                   yMvL0,
    EB_S16                                   xMvL1,
    EB_S16                                   yMvL1)
{

    signed int MVs_0, MVs_1, MVs_2, MVs_3;

    EB_U64 rate = 86440;


    switch (direction)
    {
    default:
    case UNI_PRED_LIST_0:

        // Estimate the Motion Vector Prediction Index Bits
        rate += 23196;

        // Estimate the Motion Vector Difference Bits   
        MVs_0 = ABS(xMvL0);
        MVs_1 = ABS(yMvL0);
        MVs_0 = MVs_0 > 499 ? 499 : MVs_0;
        MVs_1 = MVs_1 > 499 ? 499 : MVs_1;
        rate += mvBitTable[MVs_0][MVs_1];
        break;

    case UNI_PRED_LIST_1:

        // Estimate the Motion Vector Prediction Index Bits
        rate += 23196;

        // Estimate the Motion Vector Difference Bits  

        MVs_2 = ABS(xMvL1);
        MVs_3 = ABS(yMvL1);
        MVs_2 = MVs_2 > 499 ? 499 : MVs_2;
        MVs_3 = MVs_3 > 499 ? 499 : MVs_3;


        rate += mvBitTable[MVs_2][MVs_3];




        break;

    case BI_PRED:
        //LIST 0 RATE ESTIMATION

        // Estimate the Motion Vector Prediction Index Bits

        rate += 46392;

        // Estimate the Motion Vector Difference Bits   

        MVs_0 = ABS(xMvL0);
        MVs_1 = ABS(yMvL0);
        MVs_0 = MVs_0 > 499 ? 499 : MVs_0;
        MVs_1 = MVs_1 > 499 ? 499 : MVs_1;


        rate += mvBitTable[MVs_0][MVs_1];

        // Estimate the Motion Vector Difference Bits    
        MVs_2 = ABS(xMvL1);
        MVs_3 = ABS(yMvL1);
        MVs_2 = MVs_2 > 499 ? 499 : MVs_2;
        MVs_3 = MVs_3 > 499 ? 499 : MVs_3;


        rate += mvBitTable[MVs_2][MVs_3];

        break;

    }

    return rate;
}

/*******************************************
Derive the contouring class
If (AC energy < 32 * 32) then apply aggressive action (Class 1),
else if (AC energy < 32 * 32 * 1.6) OR (32 * 32 * 3.5 < AC energy < 32 * 32 * 4.5 AND non-8x8) then moderate action (Class 2),
else no action
*******************************************/
EB_U8 DeriveContouringClass(
    PictureParentControlSet_t   *parentPcsPtr,
    EB_U16                       lcuIndex,
    EB_U8                        leafIndex)
{
    EB_U8 contouringClass = 0;

    SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)parentPcsPtr->sequenceControlSetWrapperPtr->objectPtr;

    if (parentPcsPtr->isLcuHomogeneousOverTime[lcuIndex]) {
        if (leafIndex > 0){
            LcuParams_t            *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
            if (lcuParams->isEdgeLcu){

                if (parentPcsPtr->lcuYSrcEnergyCuArray[lcuIndex][(leafIndex - 1) / 21 + 1] < ANTI_CONTOURING_TH_1) {
                    contouringClass = 2;
                }
                else if (parentPcsPtr->lcuYSrcEnergyCuArray[lcuIndex][(leafIndex - 1) / 21 + 1] < ANTI_CONTOURING_TH_2) {
                    contouringClass = 3;
                }
                else if (parentPcsPtr->lcuYSrcEnergyCuArray[lcuIndex][(leafIndex - 1) / 21 + 1] < (ANTI_CONTOURING_TH_1+ANTI_CONTOURING_TH_2)) {
                    contouringClass = 3;
                }
            }
            else{
                if (parentPcsPtr->lcuYSrcEnergyCuArray[lcuIndex][(leafIndex - 1) / 21 + 1] < ANTI_CONTOURING_TH_0) {
                    contouringClass = 1;
                }
                else if (parentPcsPtr->lcuYSrcEnergyCuArray[lcuIndex][(leafIndex - 1) / 21 + 1] < ANTI_CONTOURING_TH_1) {
                    contouringClass = 2;
                }
                else if (parentPcsPtr->lcuYSrcEnergyCuArray[lcuIndex][(leafIndex - 1) / 21 + 1] < ANTI_CONTOURING_TH_2) {
                    contouringClass = 3;
                }
            }
        }
    }
    return(contouringClass);
}


void RefinementPredictionLoop(
    SequenceControlSet_t                   *sequenceControlSetPtr,
    PictureControlSet_t                    *pictureControlSetPtr,
    LargestCodingUnit_t                    *lcuPtr,
    EB_U32                                  lcuIndex,
    ModeDecisionConfigurationContext_t     *contextPtr)
{

    MdcpLocalCodingUnit_t  *localCuArray = contextPtr->localCuArray;
    LcuParams_t            *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
    EB_U32                  temporalLayerIndex = pictureControlSetPtr->temporalLayerIndex;
    EB_U32                  cuIndex = 0;
    EB_U8                   stationaryEdgeOverTimeFlag = (&(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuIndex]))->stationaryEdgeOverTimeFlag;

    lcuPtr->pred64 = EB_FALSE;
    while (cuIndex < CU_MAX_COUNT)
    {
        if (lcuParams->rasterScanCuValidity[MD_SCAN_TO_RASTER_SCAN[cuIndex]] && (localCuArray[cuIndex].earlySplitFlag == EB_FALSE))
        {
            localCuArray[cuIndex].slectedCu = EB_TRUE;
            lcuPtr->pred64 = (cuIndex == 0) ? EB_TRUE : lcuPtr->pred64;
            EB_U32 depth = GetCodedUnitStats(cuIndex)->depth;
            EB_U8 refinementLevel;

            if (lcuPtr->pictureControlSetPtr->sliceType == EB_I_SLICE){

                {
                    EB_U8 lowestLevel = 0x00;

                    if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE)
                        refinementLevel = NdpRefinementControl_ISLICE[depth]; 
                    else
                        refinementLevel = NdpRefinementControl_ISLICE_Sub4K[depth];

                    if (depth <= 1 && stationaryEdgeOverTimeFlag > 0){
                        if (depth == 0)
                            refinementLevel = Predp1 + Predp2 + Predp3;
                        else
                            refinementLevel = Pred + Predp1 + Predp2;

                    }
                    lowestLevel = (refinementLevel & REFINEMENT_Pp3) ? REFINEMENT_Pp3 : (refinementLevel & REFINEMENT_Pp2) ? REFINEMENT_Pp2 : (refinementLevel & REFINEMENT_Pp1) ? REFINEMENT_Pp1 :
                        (refinementLevel & REFINEMENT_P) ? REFINEMENT_P :
                        (refinementLevel & REFINEMENT_Pm1) ? REFINEMENT_Pm1 : (refinementLevel & REFINEMENT_Pm2) ? REFINEMENT_Pm2 : (refinementLevel & REFINEMENT_Pm3) ? REFINEMENT_Pm3 : 0x00;

                    MdcRefinement(
                        &(*contextPtr->localCuArray),
                        cuIndex,
                        depth,
                        refinementLevel,
                        lowestLevel);
                }
            }
            else{
                if (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_PRED_OPEN_LOOP_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE)) {
                    refinementLevel = Pred;
                }
                else 
            
				if  (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_OPEN_LOOP_DEPTH_MODE ||
                    (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_OPEN_LOOP_DEPTH_MODE))

                    refinementLevel =  NdpRefinementControlNREF[temporalLayerIndex][depth];
                else
                    refinementLevel =  NdpRefinementControl_FAST[temporalLayerIndex][depth];

				if (pictureControlSetPtr->ParentPcsPtr->cu8x8Mode == CU_8x8_MODE_1) {
                    refinementLevel = ((refinementLevel & REFINEMENT_Pp1) && depth == 2) ? refinementLevel - REFINEMENT_Pp1 :
                        ((refinementLevel & REFINEMENT_Pp2) && depth == 1) ? refinementLevel - REFINEMENT_Pp2 :
                        ((refinementLevel & REFINEMENT_Pp3) && depth == 0) ? refinementLevel - REFINEMENT_Pp3 : refinementLevel;
                }

                EB_U8 lowestLevel = 0x00;

                lowestLevel = (refinementLevel & REFINEMENT_Pp3) ? REFINEMENT_Pp3 : (refinementLevel & REFINEMENT_Pp2) ? REFINEMENT_Pp2 : (refinementLevel & REFINEMENT_Pp1) ? REFINEMENT_Pp1 :
                    (refinementLevel & REFINEMENT_P) ? REFINEMENT_P :
                    (refinementLevel & REFINEMENT_Pm1) ? REFINEMENT_Pm1 : (refinementLevel & REFINEMENT_Pm2) ? REFINEMENT_Pm2 : (refinementLevel & REFINEMENT_Pm3) ? REFINEMENT_Pm3 : 0x00;

                MdcRefinement(
                    &(*contextPtr->localCuArray),
                    cuIndex,
                    depth,
                    refinementLevel,
                    lowestLevel);
            }

            cuIndex += DepthOffset[depth];

        }
        else{

            cuIndex++;
        }
    } // End while 1 CU Loop
}


void PrePredictionRefinement(
    SequenceControlSet_t                   *sequenceControlSetPtr,
    PictureControlSet_t                    *pictureControlSetPtr,
    LargestCodingUnit_t                    *lcuPtr,
    EB_U32                                  lcuIndex,
    EB_U32                                 *startDepth,
    EB_U32                                 *endDepth
    )
{
    LcuParams_t    *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

    EB_SLICE        sliceType = pictureControlSetPtr->sliceType;

    EB_U8           edgeBlockNum = pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuIndex].edgeBlockNum;

    LcuStat_t      *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuIndex]);
    EB_U8           stationaryEdgeOverTimeFlag = lcuStatPtr->stationaryEdgeOverTimeFlag;
    EB_U8           auraStatus = lcuPtr->auraStatus;

    if (pictureControlSetPtr->ParentPcsPtr->highDarkLowLightAreaDensityFlag && pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex >0 && pictureControlSetPtr->ParentPcsPtr->sharpEdgeLcuFlag[lcuIndex] && !pictureControlSetPtr->ParentPcsPtr->similarColocatedLcuArrayAllLayers[lcuIndex]){
        *startDepth = DEPTH_16;
    }

    if ((sliceType != EB_I_SLICE && pictureControlSetPtr->highIntraSlection == 0) && (lcuParams->isCompleteLcu)){

        if (pictureControlSetPtr->sceneCaracteristicId == EB_FRAME_CARAC_0){
            if (pictureControlSetPtr->ParentPcsPtr->grassPercentageInPicture > 60 && auraStatus == AURA_STATUS_1) {
                *startDepth = DEPTH_16;
            }
        }
    }

    if (pictureControlSetPtr->ParentPcsPtr->logoPicFlag && edgeBlockNum)
    {
        *startDepth = DEPTH_16;
    }


    // S-LOGO           

    if (stationaryEdgeOverTimeFlag > 0){

        *startDepth = DEPTH_16;
        *endDepth = DEPTH_16;

    }

    if (pictureControlSetPtr->ParentPcsPtr->complexLcuArray[lcuPtr->index] == LCU_COMPLEXITY_STATUS_2) {
        *startDepth = DEPTH_16;
    }
}


void ForwardCuToModeDecision(
    SequenceControlSet_t                   *sequenceControlSetPtr,
    PictureControlSet_t                    *pictureControlSetPtr,

    EB_U32                                  lcuIndex,
    ModeDecisionConfigurationContext_t     *contextPtr
    )
{

    EB_U8                   cuIndex = 0;
    EB_U32                  cuClass = DO_NOT_ADD_CU_CONTINUE_SPLIT;
    EB_BOOL                 splitFlag = EB_TRUE;
    MdcLcuData_t           *resultsPtr = &pictureControlSetPtr->mdcLcuArray[lcuIndex];
    LcuParams_t            *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
    MdcpLocalCodingUnit_t  *localCuArray = contextPtr->localCuArray;
    EB_SLICE                sliceType = pictureControlSetPtr->sliceType;


    // CU Loop
    const CodedUnitStats_t *cuStatsPtr = GetCodedUnitStats(0);

    LcuStat_t *lcuStatPtr = &(pictureControlSetPtr->ParentPcsPtr->lcuStatArray[lcuIndex]);

    EB_BOOL    testAllDepthIntraSliceFlag = EB_FALSE;


    testAllDepthIntraSliceFlag = sliceType == EB_I_SLICE &&
        (lcuStatPtr->stationaryEdgeOverTimeFlag || pictureControlSetPtr->ParentPcsPtr->logoPicFlag ||
        (pictureControlSetPtr->ParentPcsPtr->veryLowVarPicFlag && pictureControlSetPtr->ParentPcsPtr->lowMotionContentFlag)) ?
    EB_TRUE : testAllDepthIntraSliceFlag;


    resultsPtr->leafCount = 0;

    cuIndex = 0;

    while (cuIndex < CU_MAX_COUNT)
    {
        splitFlag = EB_TRUE;
        if (lcuParams->rasterScanCuValidity[MD_SCAN_TO_RASTER_SCAN[cuIndex]])
        {
            cuStatsPtr = GetCodedUnitStats(cuIndex);

            switch (cuStatsPtr->depth){

            case 0:
            case 1:
            case 2:

                cuClass = DO_NOT_ADD_CU_CONTINUE_SPLIT;


                if (sliceType == EB_I_SLICE){
                    if (testAllDepthIntraSliceFlag){
                        cuClass = ADD_CU_CONTINUE_SPLIT;
                    }
                    else{
                        cuClass = localCuArray[cuIndex].slectedCu == EB_TRUE ? ADD_CU_CONTINUE_SPLIT : cuClass;
                        cuClass = localCuArray[cuIndex].stopSplit == EB_TRUE ? ADD_CU_STOP_SPLIT : cuClass;
                    }
                }
                else{
                    cuClass = localCuArray[cuIndex].slectedCu == EB_TRUE ? ADD_CU_CONTINUE_SPLIT : cuClass;
                    cuClass = localCuArray[cuIndex].stopSplit == EB_TRUE ? ADD_CU_STOP_SPLIT : cuClass;

                }

                // Take into account MAX CU size & MAX intra size (from the API)   
                cuClass = (cuStatsPtr->size > MAX_CU_SIZE || (sliceType == EB_I_SLICE && cuStatsPtr->size > MAX_INTRA_SIZE)) ?
                DO_NOT_ADD_CU_CONTINUE_SPLIT :
                                             cuClass;

                // Take into account MIN CU size & Min intra size(from the API)
                cuClass = (cuStatsPtr->size == MIN_CU_SIZE || (sliceType == EB_I_SLICE && cuStatsPtr->size == MIN_INTRA_SIZE)) ?
                ADD_CU_STOP_SPLIT :
                                  cuClass;

                switch (cuClass){

                case ADD_CU_STOP_SPLIT:
                    // Stop
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_FALSE;

                    break;

                case ADD_CU_CONTINUE_SPLIT:
                    // Go Down + consider the current CU as candidate
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;

                    break;

                case DO_NOT_ADD_CU_CONTINUE_SPLIT:
                    // Go Down + do not consider the current CU as candidate
                    splitFlag = EB_TRUE;

                    break;

                default:
                    resultsPtr->leafDataArray[resultsPtr->leafCount].leafIndex = cuIndex;
                    resultsPtr->leafDataArray[resultsPtr->leafCount++].splitFlag = splitFlag = EB_TRUE;

                    break;
                }

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



void MdcInterDepthDecision(
    ModeDecisionConfigurationContext_t     *contextPtr,
    EB_U32                                 originX,
    EB_U32                                 originY,
    EB_U32                                 endDepth,
    EB_U32                                 splitFlagBits0,
    EB_U32                                 splitFlagBits1,
    EB_U64                                 lambda,

    EB_U32                                 cuIndex
    )
{

    EB_U32               leftCuIndex;
    EB_U32               topCuIndex;
    EB_U32               topLeftCuIndex;
    EB_U32               depthZeroCandidateCuIndex;
    EB_U32               depthOneCandidateCuIndex = cuIndex;
    EB_U32               depthTwoCandidateCuIndex = cuIndex;
    EB_U64               depthNRate = 0;
    EB_U64               depthNPlusOneRate = 0;
    EB_U64               depthNCost = 0;
    EB_U64               depthNPlusOneCost = 0;
    MdcpLocalCodingUnit_t *localCuArray = contextPtr->localCuArray;
    /*** Stage 0: Inter depth decision: depth 2 vs depth 3 ***/
    // Walks to the last coded 8x8 block for merging
    EB_U8  groupOf8x8BlocksCount = contextPtr->groupOf8x8BlocksCount;
    EB_U8  groupOf16x16BlocksCount = contextPtr->groupOf16x16BlocksCount;
    if ((GROUP_OF_4_8x8_BLOCKS(originX, originY))) {

        groupOf8x8BlocksCount++;

        // From the last coded cu index, get the indices of the left, top, and top left cus
        leftCuIndex = cuIndex - DEPTH_THREE_STEP;
        topCuIndex = leftCuIndex - DEPTH_THREE_STEP;
        topLeftCuIndex = topCuIndex - DEPTH_THREE_STEP;

        // From the top left index, get the index of the candidate pu for merging
        depthTwoCandidateCuIndex = topLeftCuIndex - 1;

        // Compute depth N cost
        localCuArray[depthTwoCandidateCuIndex].splitContext = 0;

        depthNRate = (((lambda  * splitFlagBits0) + MD_OFFSET) >> MD_SHIFT);

        depthNCost = (localCuArray[depthTwoCandidateCuIndex]).earlyCost + depthNRate;

        if (endDepth < 3) {

            (localCuArray[depthTwoCandidateCuIndex]).earlySplitFlag = EB_FALSE;
            (localCuArray[depthTwoCandidateCuIndex]).earlyCost = depthNCost;

        }
        else{

            // Assign rate
            depthNPlusOneRate = (((lambda  * splitFlagBits1) + MD_OFFSET) >> MD_SHIFT);

            depthNPlusOneCost = (localCuArray[cuIndex]).earlyCost + (localCuArray[leftCuIndex]).earlyCost + (localCuArray[topCuIndex]).earlyCost + (localCuArray[topLeftCuIndex]).earlyCost + depthNPlusOneRate;

            if (depthNCost <= depthNPlusOneCost) {

                // If the cost is low enough to warrant not spliting further:
                // 1. set the split flag of the candidate pu for merging to false
                // 2. update the last pu index
                (localCuArray[depthTwoCandidateCuIndex]).earlySplitFlag = EB_FALSE;
                (localCuArray[depthTwoCandidateCuIndex]).earlyCost = depthNCost;

            }
            else {
                // If the cost is not low enough:
                // update the cost of the candidate pu for merging
                // this update is required for the next inter depth decision
                (&localCuArray[depthTwoCandidateCuIndex])->earlyCost = depthNPlusOneCost;
            }

        }
    }

    // Walks to the last coded 16x16 block for merging
    if (GROUP_OF_4_16x16_BLOCKS(GetCodedUnitStats(depthTwoCandidateCuIndex)->originX, GetCodedUnitStats(depthTwoCandidateCuIndex)->originY) &&
        (groupOf8x8BlocksCount == 4)){

        groupOf8x8BlocksCount = 0;
        groupOf16x16BlocksCount++;

        // From the last coded pu index, get the indices of the left, top, and top left pus
        leftCuIndex = depthTwoCandidateCuIndex - DEPTH_TWO_STEP;
        topCuIndex = leftCuIndex - DEPTH_TWO_STEP;
        topLeftCuIndex = topCuIndex - DEPTH_TWO_STEP;

        // From the top left index, get the index of the candidate pu for merging
        depthOneCandidateCuIndex = topLeftCuIndex - 1;

        if (GetCodedUnitStats(depthOneCandidateCuIndex)->depth == 1) {
            depthNRate = (((lambda  *splitFlagBits0) + MD_OFFSET) >> MD_SHIFT);

            depthNCost = localCuArray[depthOneCandidateCuIndex].earlyCost + depthNRate;
            if (endDepth < 2) {

                localCuArray[depthOneCandidateCuIndex].earlySplitFlag = EB_FALSE;
                localCuArray[depthOneCandidateCuIndex].earlyCost = depthNCost;

            }
            else{
                // Compute depth N+1 cost
                // Assign rate
                depthNPlusOneRate = (((lambda  *splitFlagBits1) + MD_OFFSET) >> MD_SHIFT);

                depthNPlusOneCost = localCuArray[depthTwoCandidateCuIndex].earlyCost +
                    localCuArray[leftCuIndex].earlyCost +
                    localCuArray[topCuIndex].earlyCost +
                    localCuArray[topLeftCuIndex].earlyCost +
                    depthNPlusOneRate;

                // Inter depth comparison: depth 1 vs depth 2
                if (depthNCost <= depthNPlusOneCost) {
                    // If the cost is low enough to warrant not spliting further:
                    // 1. set the split flag of the candidate pu for merging to false
                    // 2. update the last pu index
                    localCuArray[depthOneCandidateCuIndex].earlySplitFlag = EB_FALSE;
                    localCuArray[depthOneCandidateCuIndex].earlyCost = depthNCost;
                }
                else {
                    // If the cost is not low enough:
                    // update the cost of the candidate pu for merging
                    // this update is required for the next inter depth decision
                    localCuArray[depthOneCandidateCuIndex].earlyCost = depthNPlusOneCost;
                }
            }
        }
    }

    // Stage 2: Inter depth decision: depth 0 vs depth 1 

    // Walks to the last coded 32x32 block for merging
    // Stage 2 isn't performed in I slices since the abcense of 64x64 candidates
    if (GROUP_OF_4_32x32_BLOCKS(GetCodedUnitStats(depthOneCandidateCuIndex)->originX, GetCodedUnitStats(depthOneCandidateCuIndex)->originY) &&
        (groupOf16x16BlocksCount == 4)) {

        groupOf16x16BlocksCount = 0;

        // From the last coded pu index, get the indices of the left, top, and top left pus
        leftCuIndex = depthOneCandidateCuIndex - DEPTH_ONE_STEP;
        topCuIndex = leftCuIndex - DEPTH_ONE_STEP;
        topLeftCuIndex = topCuIndex - DEPTH_ONE_STEP;

        // From the top left index, get the index of the candidate pu for merging
        depthZeroCandidateCuIndex = topLeftCuIndex - 1;

        if (GetCodedUnitStats(depthZeroCandidateCuIndex)->depth == 0) {

            // Compute depth N cost
            depthNRate = (((lambda  *splitFlagBits0) + MD_OFFSET) >> MD_SHIFT);

            depthNCost = (&localCuArray[depthZeroCandidateCuIndex])->earlyCost + depthNRate;
            if (endDepth < 1) {

                (&localCuArray[depthZeroCandidateCuIndex])->earlySplitFlag = EB_FALSE;

            }
            else{
                // Compute depth N+1 cost

                // Assign rate
                depthNPlusOneRate = (((lambda  *splitFlagBits1) + MD_OFFSET) >> MD_SHIFT);

                depthNPlusOneCost = localCuArray[depthOneCandidateCuIndex].earlyCost +
                    localCuArray[leftCuIndex].earlyCost +
                    localCuArray[topCuIndex].earlyCost +
                    localCuArray[topLeftCuIndex].earlyCost +
                    depthNPlusOneRate;

                // Inter depth comparison: depth 0 vs depth 1
                if (depthNCost <= depthNPlusOneCost) {
                    // If the cost is low enough to warrant not spliting further:
                    // 1. set the split flag of the candidate pu for merging to false
                    // 2. update the last pu index
                    (&localCuArray[depthZeroCandidateCuIndex])->earlySplitFlag = EB_FALSE;
                }
            }
        }
    }

    contextPtr->groupOf8x8BlocksCount = groupOf8x8BlocksCount;
    contextPtr->groupOf16x16BlocksCount = groupOf16x16BlocksCount;
}

void PredictionPartitionLoop(
    SequenceControlSet_t                   *sequenceControlSetPtr,
    PictureControlSet_t                    *pictureControlSetPtr,
    EB_U32                                  lcuIndex,
    EB_U32                                  tbOriginX,
    EB_U32                                  tbOriginY,
    EB_U32                                  startDepth,
    EB_U32                                  endDepth,
    ModeDecisionConfigurationContext_t     *contextPtr
    )
{

	MdRateEstimationContext_t *mdRateEstimationPtr = contextPtr->mdRateEstimationPtr;
    MdcpLocalCodingUnit_t *localCuArray = contextPtr->localCuArray;
    MdcpLocalCodingUnit_t   *cuPtr;

    LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];
    EB_U32      cuInterSad = 0;
    EB_U64      cuInterRate = 0;
    EB_U32      cuIntraSad = 0;
    EB_U64      cuIntraRate = 0;
    EB_U64      cuIntraCost = 0;
    EB_U32     cuIndexInRaterScan;
    EB_U64      cuInterCost = 0;
    EB_U32 cuIndex = 0;
    EB_U32 startIndex = 0;

    (void)tbOriginX;
    (void)tbOriginY;

    const CodedUnitStats_t *cuStatsPtr;

    for (cuIndex = startIndex; cuIndex < CU_MAX_COUNT; ++cuIndex)

    {

        localCuArray[cuIndex].slectedCu = EB_FALSE;
        localCuArray[cuIndex].stopSplit = EB_FALSE;

        cuPtr = &localCuArray[cuIndex];
        cuIndexInRaterScan = MD_SCAN_TO_RASTER_SCAN[cuIndex];
        if (lcuParams->rasterScanCuValidity[cuIndexInRaterScan])
        {
            EB_U32 depth;
            EB_U32 size;
            cuStatsPtr = GetCodedUnitStats(cuIndex);

            depth = cuStatsPtr->depth;
            size = cuStatsPtr->size;
            cuPtr->earlySplitFlag = (depth < endDepth) ? EB_TRUE : EB_FALSE;

            if (depth >= startDepth && depth <= endDepth){
                //reset the flags here:   all CU splitFalg=TRUE. default: we always split. interDepthDecision will select where  to stop splitting(ie setting the flag to False)

                {
                    MdcIntraCuRate(
                        depth,
                        mdRateEstimationPtr,
                        pictureControlSetPtr,
                        &cuIntraRate);


					OisCu32Cu16Results_t	        *oisCu32Cu16ResultsPtr = pictureControlSetPtr->ParentPcsPtr->oisCu32Cu16Results[lcuIndex];
					OisCu8Results_t   	            *oisCu8ResultsPtr = pictureControlSetPtr->ParentPcsPtr->oisCu8Results[lcuIndex];
					OisCandidate_t * oisCuPtr = cuIndexInRaterScan < RASTER_SCAN_CU_INDEX_8x8_0 ?
						oisCu32Cu16ResultsPtr->sortedOisCandidate[cuIndexInRaterScan] : oisCu8ResultsPtr->sortedOisCandidate[cuIndexInRaterScan - RASTER_SCAN_CU_INDEX_8x8_0];


					if (size > 32){
						cuIntraSad =
							oisCu32Cu16ResultsPtr->sortedOisCandidate[1][0].distortion +
							oisCu32Cu16ResultsPtr->sortedOisCandidate[2][0].distortion +
							oisCu32Cu16ResultsPtr->sortedOisCandidate[3][0].distortion +
							oisCu32Cu16ResultsPtr->sortedOisCandidate[4][0].distortion;
					}
					else if (size == 32) {
						cuIntraSad = oisCu32Cu16ResultsPtr->sortedOisCandidate[cuIndexInRaterScan][0].distortion;
					}
					else{
						if (size > 8){
							cuIntraSad = oisCu32Cu16ResultsPtr->sortedOisCandidate[cuIndexInRaterScan][0].distortion;
						}
						else{
							if (oisCuPtr[0].validDistortion){
								cuIntraSad = oisCuPtr[0].distortion;
							}
							else{

								const CodedUnitStats_t  *cuStats = GetCodedUnitStats(ParentBlockIndex[cuIndex]);
								const EB_U32 me2Nx2NTableOffset = cuStats->cuNumInDepth + me2Nx2NOffset[cuStats->depth];


								if (oisCu8ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].validDistortion){
									cuIntraSad = oisCu8ResultsPtr->sortedOisCandidate[me2Nx2NTableOffset][0].distortion;
								}
								else {
									cuIntraSad = 0;
								}
							}
						}
					}
            

                    cuIntraCost = (cuIntraSad << COST_PRECISION) + ((contextPtr->lambda * cuIntraRate + MD_OFFSET) >> MD_SHIFT);
                    cuPtr->earlyCost = cuIntraCost;

                }

                if (pictureControlSetPtr->sliceType != EB_I_SLICE){

		

					MeCuResults_t * mePuResult = &pictureControlSetPtr->ParentPcsPtr->meResults[lcuIndex][cuIndexInRaterScan];
					cuInterRate = MdcInterCuRate(
						mePuResult->distortionDirection[0].direction,
						mePuResult->xMvL0,
						mePuResult->yMvL0,
						mePuResult->xMvL1,
						mePuResult->yMvL1);
					cuInterSad = mePuResult->distortionDirection[0].distortion;


                    cuInterCost = (cuInterSad << COST_PRECISION) + ((contextPtr->lambda * cuInterRate + MD_OFFSET) >> MD_SHIFT);
                    cuPtr->earlyCost = cuInterCost;

                }

                cuPtr->earlyCost = pictureControlSetPtr->sliceType == EB_I_SLICE ? cuIntraCost : cuInterCost;

                if (endDepth == 2){
                    contextPtr->groupOf8x8BlocksCount = depth == 2 ? incrementalCount[cuIndexInRaterScan] : 0;
                }

                if (endDepth == 1){
                    contextPtr->groupOf16x16BlocksCount = depth == 1 ? incrementalCount[cuIndexInRaterScan] : 0;
                }
                MdcInterDepthDecision(
                    contextPtr,
                    cuStatsPtr->originX,
                    cuStatsPtr->originY,
                    endDepth,
                    mdRateEstimationPtr->splitFlagBits[0],
                    mdRateEstimationPtr->splitFlagBits[3],
                    contextPtr->lambda,
                    cuIndex);
            }
            else{
                cuPtr->earlyCost = ~0u;
            }

        }

    }// End CU Loop

}

EB_ERRORTYPE EarlyModeDecisionLcu(
    SequenceControlSet_t                   *sequenceControlSetPtr,
    PictureControlSet_t                    *pictureControlSetPtr,
    LargestCodingUnit_t                    *lcuPtr,
    EB_U32                                  lcuIndex,
    ModeDecisionConfigurationContext_t     *contextPtr)
{

    EB_ERRORTYPE    return_error = EB_ErrorNone;

    EB_U32          tbOriginX = lcuPtr->originX;
    EB_U32          tbOriginY = lcuPtr->originY;
    EB_SLICE        sliceType = pictureControlSetPtr->sliceType;

    EB_U32      startDepth = (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0) ? DEPTH_32 : DEPTH_64 ;

    EB_U32      endDepth = (sliceType == EB_I_SLICE) ? DEPTH_8 : DEPTH_16;

    contextPtr->groupOf8x8BlocksCount = 0;
    contextPtr->groupOf16x16BlocksCount = 0;


    // The MDC refinements had been taken into account at the budgeting algorithm & therefore could be skipped for the ADP case
    if (pictureControlSetPtr->ParentPcsPtr->depthMode != PICT_LCU_SWITCH_DEPTH_MODE) {
        PrePredictionRefinement(
            sequenceControlSetPtr,
            pictureControlSetPtr,
            lcuPtr,
            lcuIndex,
            &startDepth,
            &endDepth);
    }

    PredictionPartitionLoop(
        sequenceControlSetPtr,
        pictureControlSetPtr,
        lcuIndex,
        tbOriginX,
        tbOriginY,
        startDepth,
        endDepth,
        contextPtr
        );

    RefinementPredictionLoop(
        sequenceControlSetPtr,
        pictureControlSetPtr,
        lcuPtr,
        lcuIndex,
        contextPtr);

    ForwardCuToModeDecision(
        sequenceControlSetPtr,
        pictureControlSetPtr,

        lcuIndex,
        contextPtr);

    return return_error;

}



