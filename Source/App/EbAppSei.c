/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbApiSei.h"

EB_ERRORTYPE EbAppVideoUsabilityInfoCtor(
    AppVideoUsabilityInfo_t *vuiPtr)
{
    AppHrdParameters_t* hrdParamPtr;
    EB_APP_MALLOC(AppHrdParameters_t*, vuiPtr->hrdParametersPtr, sizeof(AppHrdParameters_t), EB_N_PTR, EB_ErrorInsufficientResources);

    hrdParamPtr = vuiPtr->hrdParametersPtr;
    // Initialize vui variables

    vuiPtr->aspectRatioInfoPresentFlag = EB_TRUE;
    vuiPtr->aspectRatioIdc = 0;
    vuiPtr->sarWidth = 0;
    vuiPtr->sarHeight = 0;

    vuiPtr->overscanInfoPresentFlag = EB_FALSE;
    vuiPtr->overscanApproriateFlag = EB_FALSE;
    vuiPtr->videoSignalTypePresentFlag = EB_FALSE;

    vuiPtr->videoFormat = 0;
    vuiPtr->videoFullRangeFlag = EB_FALSE;

    vuiPtr->colorDescriptionPresentFlag = EB_FALSE;
    vuiPtr->colorPrimaries = 0;
    vuiPtr->transferCharacteristics = 0;
    vuiPtr->matrixCoeffs = 0;

    vuiPtr->chromaLocInfoPresentFlag = EB_FALSE;
    vuiPtr->chromaSampleLocTypeTopField = 0;
    vuiPtr->chromaSampleLocTypeBottomField = 0;

    vuiPtr->neutralChromaIndicationFlag = EB_FALSE;
    vuiPtr->fieldSeqFlag = EB_FALSE;
    vuiPtr->frameFieldInfoPresentFlag = EB_FALSE;//EB_TRUE;

    vuiPtr->defaultDisplayWindowFlag = EB_TRUE;
    vuiPtr->defaultDisplayWinLeftOffset = 0;
    vuiPtr->defaultDisplayWinRightOffset = 0;
    vuiPtr->defaultDisplayWinTopOffset = 0;
    vuiPtr->defaultDisplayWinBottomOffset = 0;

    vuiPtr->vuiTimingInfoPresentFlag = EB_FALSE;//EB_TRUE;
    vuiPtr->vuiNumUnitsInTick = 0;
    vuiPtr->vuiTimeScale = 0;

    vuiPtr->vuiPocPropotionalTimingFlag = EB_FALSE;
    vuiPtr->vuiNumTicksPocDiffOneMinus1 = 0;

    vuiPtr->vuiHrdParametersPresentFlag = EB_FALSE;//EB_TRUE;

    vuiPtr->bitstreamRestrictionFlag = EB_FALSE;

    vuiPtr->motionVectorsOverPicBoundariesFlag = EB_FALSE;
    vuiPtr->restrictedRefPicListsFlag = EB_FALSE;

    vuiPtr->minSpatialSegmentationIdc = 0;
    vuiPtr->maxBytesPerPicDenom = 0;
    vuiPtr->maxBitsPerMinCuDenom = 0;
    vuiPtr->log2MaxMvLengthHorizontal = 0;
    vuiPtr->log2MaxMvLengthVertical = 0;

    // Initialize HRD parameters
    hrdParamPtr->nalHrdParametersPresentFlag = EB_FALSE;//EB_TRUE;
    hrdParamPtr->vclHrdParametersPresentFlag = EB_FALSE; //EB_TRUE;
    hrdParamPtr->subPicCpbParamsPresentFlag = EB_FALSE;//EB_TRUE;

    hrdParamPtr->tickDivisorMinus2 = 0;
    hrdParamPtr->duCpbRemovalDelayLengthMinus1 = 0;

    hrdParamPtr->subPicCpbParamsPicTimingSeiFlag = EB_FALSE;//EB_TRUE;

    hrdParamPtr->dpbOutputDelayDuLengthMinus1 = 0;

    hrdParamPtr->bitRateScale = 0;
    hrdParamPtr->cpbSizeScale = 0;
    hrdParamPtr->duCpbSizeScale = 0;

    hrdParamPtr->initialCpbRemovalDelayLengthMinus1 = 0;
    hrdParamPtr->auCpbRemovalDelayLengthMinus1 = 0;
    hrdParamPtr->dpbOutputDelayLengthMinus1 = 0;

    EB_MEMSET(
        hrdParamPtr->fixedPicRateGeneralFlag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrdParamPtr->fixedPicRateWithinCvsFlag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrdParamPtr->elementalDurationTcMinus1,
        EB_FALSE,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrdParamPtr->lowDelayHrdFlag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrdParamPtr->cpbCountMinus1,
        0,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS);
    //hrdParamPtr->cpbCountMinus1[0] = 2;

    EB_MEMSET(
        hrdParamPtr->bitRateValueMinus1,
        EB_FALSE,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS*2*MAX_CPB_COUNT);

    EB_MEMSET(
        hrdParamPtr->cpbSizeValueMinus1,
        EB_FALSE,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS*2*MAX_CPB_COUNT);

    EB_MEMSET(
        hrdParamPtr->bitRateDuValueMinus1,
        EB_FALSE,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS*2*MAX_CPB_COUNT);

    EB_MEMSET(
        hrdParamPtr->cpbSizeDuValueMinus1,
        EB_FALSE,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS*2*MAX_CPB_COUNT);

    EB_MEMSET(
        hrdParamPtr->cbrFlag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS*2*MAX_CPB_COUNT);

    hrdParamPtr->cpbDpbDelaysPresentFlag = (EB_BOOL)((hrdParamPtr->nalHrdParametersPresentFlag || hrdParamPtr->vclHrdParametersPresentFlag) && vuiPtr->vuiHrdParametersPresentFlag);

    return EB_ErrorNone;
}







