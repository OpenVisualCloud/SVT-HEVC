/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbSei.h"

void EbVideoUsabilityInfoCopy(
    AppVideoUsabilityInfo_t *dstVuiPtr,
    AppVideoUsabilityInfo_t *srcVuiPtr)
{

    dstVuiPtr->aspectRatioInfoPresentFlag = srcVuiPtr->aspectRatioInfoPresentFlag;
    dstVuiPtr->aspectRatioIdc = srcVuiPtr->aspectRatioIdc;
    dstVuiPtr->sarWidth = srcVuiPtr->sarWidth;
    dstVuiPtr->sarHeight = srcVuiPtr->sarHeight;

    dstVuiPtr->overscanInfoPresentFlag = srcVuiPtr->overscanInfoPresentFlag;
    dstVuiPtr->overscanApproriateFlag = srcVuiPtr->overscanApproriateFlag;
    dstVuiPtr->videoSignalTypePresentFlag = srcVuiPtr->videoSignalTypePresentFlag;

    dstVuiPtr->videoFormat = srcVuiPtr->videoFormat;
    dstVuiPtr->videoFullRangeFlag = srcVuiPtr->videoFullRangeFlag;

    dstVuiPtr->colorDescriptionPresentFlag =  srcVuiPtr->colorDescriptionPresentFlag;
    dstVuiPtr->colorPrimaries = srcVuiPtr->colorPrimaries;
    dstVuiPtr->transferCharacteristics = srcVuiPtr->transferCharacteristics;
    dstVuiPtr->matrixCoeffs = srcVuiPtr->matrixCoeffs;

    dstVuiPtr->chromaLocInfoPresentFlag = srcVuiPtr->chromaLocInfoPresentFlag;
    dstVuiPtr->chromaSampleLocTypeTopField = srcVuiPtr->chromaSampleLocTypeTopField;
    dstVuiPtr->chromaSampleLocTypeBottomField = srcVuiPtr->chromaSampleLocTypeBottomField;

    dstVuiPtr->neutralChromaIndicationFlag = srcVuiPtr->neutralChromaIndicationFlag;
    dstVuiPtr->fieldSeqFlag = srcVuiPtr->fieldSeqFlag;
    dstVuiPtr->frameFieldInfoPresentFlag = srcVuiPtr->frameFieldInfoPresentFlag;

    dstVuiPtr->defaultDisplayWindowFlag = srcVuiPtr->defaultDisplayWindowFlag;
    dstVuiPtr->defaultDisplayWinLeftOffset = srcVuiPtr->defaultDisplayWinLeftOffset;
    dstVuiPtr->defaultDisplayWinRightOffset = srcVuiPtr->defaultDisplayWinRightOffset;
    dstVuiPtr->defaultDisplayWinTopOffset =  srcVuiPtr->defaultDisplayWinTopOffset;
    dstVuiPtr->defaultDisplayWinBottomOffset = srcVuiPtr->defaultDisplayWinBottomOffset;

    dstVuiPtr->vuiTimingInfoPresentFlag = srcVuiPtr->vuiTimingInfoPresentFlag;
    dstVuiPtr->vuiNumUnitsInTick = srcVuiPtr->vuiNumUnitsInTick;
    dstVuiPtr->vuiTimeScale = srcVuiPtr->vuiTimeScale;

    dstVuiPtr->vuiPocPropotionalTimingFlag = srcVuiPtr->vuiPocPropotionalTimingFlag;
    dstVuiPtr->vuiNumTicksPocDiffOneMinus1 = srcVuiPtr->vuiNumTicksPocDiffOneMinus1;

    dstVuiPtr->vuiHrdParametersPresentFlag = srcVuiPtr->vuiHrdParametersPresentFlag;

    dstVuiPtr->bitstreamRestrictionFlag = srcVuiPtr->bitstreamRestrictionFlag;

    dstVuiPtr->motionVectorsOverPicBoundariesFlag = srcVuiPtr->motionVectorsOverPicBoundariesFlag;
    dstVuiPtr->restrictedRefPicListsFlag = srcVuiPtr->restrictedRefPicListsFlag;

    dstVuiPtr->minSpatialSegmentationIdc = srcVuiPtr->minSpatialSegmentationIdc;
    dstVuiPtr->maxBytesPerPicDenom = srcVuiPtr->maxBytesPerPicDenom;
    dstVuiPtr->maxBitsPerMinCuDenom = srcVuiPtr->maxBitsPerMinCuDenom;
    dstVuiPtr->log2MaxMvLengthHorizontal = srcVuiPtr->log2MaxMvLengthHorizontal;
    dstVuiPtr->log2MaxMvLengthVertical = srcVuiPtr->log2MaxMvLengthVertical;

    EB_MEMCPY(
        dstVuiPtr->hrdParametersPtr,
        srcVuiPtr->hrdParametersPtr,
        sizeof(AppHrdParameters_t));

    return;
}

EB_ERRORTYPE EbVideoUsabilityInfoCtor(
    AppVideoUsabilityInfo_t *vuiPtr)
{
    AppHrdParameters_t* hrdParamPtr;
    EB_MALLOC(AppHrdParameters_t*, vuiPtr->hrdParametersPtr, sizeof(AppHrdParameters_t), EB_N_PTR);

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
    hrdParamPtr->vclHrdParametersPresentFlag = EB_FALSE;
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




void EbPictureTimeingSeiCtor(
    AppPictureTimingSei_t   *picTimingPtr)
{
    picTimingPtr->picStruct = 0;
    picTimingPtr->sourceScanType = 0;
    picTimingPtr->duplicateFlag = EB_FALSE;
    picTimingPtr->auCpbRemovalDelayMinus1 = 0;
    picTimingPtr->picDpbOutputDelay = 0;
    picTimingPtr->picDpbOutputDuDelay = 0;
    picTimingPtr->numDecodingUnitsMinus1 = 0;//3;
    picTimingPtr->duCommonCpbRemovalDelayFlag = EB_FALSE;
    picTimingPtr->duCommonCpbRemovalDelayMinus1 = 0;
    picTimingPtr->numNalusInDuMinus1 = 0;

    EB_MEMSET(
        picTimingPtr->duCpbRemovalDelayMinus1,
        0,
        sizeof(EB_U32) * MAX_DECODING_UNIT_COUNT);

    return;
}

void EbBufferingPeriodSeiCtor(
    AppBufferingPeriodSei_t   *bufferingPeriodPtr)
{
    bufferingPeriodPtr->bpSeqParameterSetId = 0;
    bufferingPeriodPtr->rapCpbParamsPresentFlag = EB_FALSE;
    bufferingPeriodPtr->concatenationFlag = EB_FALSE;
    bufferingPeriodPtr->auCpbRemovalDelayDeltaMinus1 = 0;
    bufferingPeriodPtr->cpbDelayOffset = 0;
    bufferingPeriodPtr->dpbDelayOffset = 0;

    EB_MEMSET(
        bufferingPeriodPtr->initialCpbRemovalDelay,
        0,
        sizeof(EB_U32) * 2 * MAX_CPB_COUNT);
    EB_MEMSET(
        bufferingPeriodPtr->initialCpbRemovalDelayOffset,
        0,
        sizeof(EB_U32) * 2 * MAX_CPB_COUNT);
    EB_MEMSET(
        bufferingPeriodPtr->initialAltCpbRemovalDelay,
        0,
        sizeof(EB_U32) * 2 * MAX_CPB_COUNT);
    EB_MEMSET(
        bufferingPeriodPtr->initialAltCpbRemovalDelayOffset,
        0,
        sizeof(EB_U32) * 2 * MAX_CPB_COUNT);

    return;
}


void EbRecoveryPointSeiCtor(
    AppRecoveryPoint_t   *recoveryPointSeiPtr)
{
    recoveryPointSeiPtr->recoveryPocCnt = 0;

    recoveryPointSeiPtr->exactMatchingFlag = EB_FALSE;

    recoveryPointSeiPtr->brokenLinkFlag = EB_FALSE;

    return;
}

void EbContentLightLevelCtor(
    AppContentLightLevelSei_t    *contentLightLevelPtr)
{
    contentLightLevelPtr->maxContentLightLevel = 0;
    contentLightLevelPtr->maxPicAverageLightLevel = 0;
}

void EbMasteringDisplayColorVolumeCtor(
    AppMasteringDisplayColorVolumeSei_t    *masteringDisplayPtr)
{

    EB_MEMSET(
        masteringDisplayPtr->displayPrimaryX,
        0,
        sizeof(EB_U16) * 3);
    EB_MEMSET(
        masteringDisplayPtr->displayPrimaryY,
        0,
        sizeof(EB_U16) * 3);

    masteringDisplayPtr->whitePointX = 0;
    masteringDisplayPtr->whitePointY= 0 ;
    masteringDisplayPtr->maxDisplayMasteringLuminance = 0;
    masteringDisplayPtr->minDisplayMasteringLuminance = 0;
}

void EbRegUserDataSEICtor(
    RegistedUserData_t* regUserDataSeiPtr) {

    regUserDataSeiPtr->userData = NULL;
    regUserDataSeiPtr->userDataSize = 0;
}

void EbUnRegUserDataSEICtor(
    UnregistedUserData_t* UnRegUserDataPtr) {

    UnRegUserDataPtr->userData = NULL;
    UnRegUserDataPtr->userDataSize = 0;
    EB_MEMSET(
        UnRegUserDataPtr->uuidIsoIec_11578,
        0,
        sizeof(EB_U8) * 16);
}


/**************************************************
 * GetUvlcCodeLength
 **************************************************/
EB_U32 GetUvlcCodeLength(
    EB_U32      code)
{
    EB_U32 numberOfBits = 1;
    EB_U32 tempBits = ++code;

    while( 1 != tempBits )
    {
        tempBits >>= 1;
        numberOfBits += 2;
    }

    return numberOfBits;
}

/**************************************************
 * GetUvlcCodeLength
 **************************************************/
EB_U32 GetSvlcCodeLength(
    EB_S32      code)
{
    EB_U32 numberOfBits = 1;
    EB_U32 tempBits;
    EB_U32 bits;

    bits = (code <= 0) ? -code<<1 : (code<<1)-1;;

    tempBits = ++bits;

    while( 1 != tempBits )
    {
        tempBits >>= 1;
        numberOfBits += 2;
    }

    return numberOfBits;
}

EB_U32 GetPictureTimingSEILength(
    AppPictureTimingSei_t      *picTimingSeiPtr,
    AppVideoUsabilityInfo_t    *vuiPtr)
{
    EB_U32    seiLength = 0;
    EB_U32    decodingUnitIndex;

    if(vuiPtr->frameFieldInfoPresentFlag) {
        // pic_struct
        seiLength += 4;

        // source_scan_type
        seiLength += 2;

        // duplicate_flag
        seiLength += 1;
    }

    if(vuiPtr->hrdParametersPtr->cpbDpbDelaysPresentFlag) {
        // au_cpb_removal_delay_minus1
        seiLength += vuiPtr->hrdParametersPtr->duCpbRemovalDelayLengthMinus1 + 1;

        // pic_dpb_output_delay
        seiLength += vuiPtr->hrdParametersPtr->dpbOutputDelayLengthMinus1 + 1;

        if(vuiPtr->hrdParametersPtr->subPicCpbParamsPresentFlag) {
            // pic_dpb_output_du_delay
            seiLength += vuiPtr->hrdParametersPtr->dpbOutputDelayDuLengthMinus1 + 1;
        }

        if(vuiPtr->hrdParametersPtr->subPicCpbParamsPresentFlag && vuiPtr->hrdParametersPtr->subPicCpbParamsPicTimingSeiFlag) {
            // num_decoding_units_minus1
            seiLength += GetUvlcCodeLength(
                             picTimingSeiPtr->numDecodingUnitsMinus1);

            // du_common_cpb_removal_delay_flag
            seiLength += 1;

            if(picTimingSeiPtr->duCommonCpbRemovalDelayFlag) {
                // du_common_cpb_removal_delay_minus1
                seiLength += vuiPtr->hrdParametersPtr->duCpbRemovalDelayLengthMinus1 + 1;
            }

            for(decodingUnitIndex = 0; decodingUnitIndex <= picTimingSeiPtr->numDecodingUnitsMinus1; ++decodingUnitIndex) {
                // num_nalus_in_du_minus1
                seiLength += GetUvlcCodeLength(
                                 picTimingSeiPtr->numNalusInDuMinus1);

                if(!picTimingSeiPtr->duCommonCpbRemovalDelayFlag && decodingUnitIndex < picTimingSeiPtr->numDecodingUnitsMinus1) {
                    // du_cpb_removal_delay_minus1
                    seiLength += vuiPtr->hrdParametersPtr->duCpbRemovalDelayLengthMinus1 + 1;
                }
            }
        }
    }

    seiLength = (seiLength + 7) >> 3;

    return seiLength;
}

EB_U32 GetBufPeriodSEILength(
    AppBufferingPeriodSei_t    *bufferingPeriodPtr,
    AppVideoUsabilityInfo_t    *vuiPtr)
{
    EB_U32       seiLength = 0;
    EB_U32       nalVclIndex;
    EB_U32       cpbIndex;

    // bp_seq_parameter_set_id
    seiLength += GetUvlcCodeLength(
                     bufferingPeriodPtr->bpSeqParameterSetId);

    if(!vuiPtr->hrdParametersPtr->subPicCpbParamsPresentFlag) {
        // rap_cpb_params_present_flag
        seiLength += 1;
    }

    if(bufferingPeriodPtr->rapCpbParamsPresentFlag) {
        // cpb_delay_offset
        seiLength += vuiPtr->hrdParametersPtr->initialCpbRemovalDelayLengthMinus1 + 1;

        // dpb_delay_offset
        seiLength += vuiPtr->hrdParametersPtr->dpbOutputDelayDuLengthMinus1 + 1;
    }

    // concatenation_flag
    seiLength += 1;

    // au_cpb_removal_delay_delta_minus1
    seiLength += vuiPtr->hrdParametersPtr->initialCpbRemovalDelayLengthMinus1 + 1;

    for(nalVclIndex = 0; nalVclIndex < 2; ++nalVclIndex) {
        if((nalVclIndex == 0 && vuiPtr->hrdParametersPtr->nalHrdParametersPresentFlag) ||
                (nalVclIndex == 1 && vuiPtr->hrdParametersPtr->vclHrdParametersPresentFlag)) {

            for(cpbIndex = 0; cpbIndex < vuiPtr->hrdParametersPtr->cpbCountMinus1[0] + 1; ++cpbIndex) {
                // initial_cpb_removal_delay
                seiLength += vuiPtr->hrdParametersPtr->initialCpbRemovalDelayLengthMinus1 + 1;

                // initial_cpb_removal_delay_offset
                seiLength += vuiPtr->hrdParametersPtr->initialCpbRemovalDelayLengthMinus1 + 1;

                if(vuiPtr->hrdParametersPtr->subPicCpbParamsPresentFlag || bufferingPeriodPtr->rapCpbParamsPresentFlag) {
                    // initial_alt_cpb_removal_delay
                    seiLength += vuiPtr->hrdParametersPtr->initialCpbRemovalDelayLengthMinus1 + 1;

                    // initial_alt_cpb_removal_delay_offset
                    seiLength += vuiPtr->hrdParametersPtr->initialCpbRemovalDelayLengthMinus1 + 1;
                }
            }
        }
    }

    seiLength = (seiLength + 7) >> 3;

    return seiLength;
}

EB_U32 GetRecoveryPointSEILength(
    AppRecoveryPoint_t    *recoveryPointSeiPtr)
{
    EB_U32    seiLength = 0;

    // recovery_poc_cnt
    seiLength += GetSvlcCodeLength(
                     recoveryPointSeiPtr->recoveryPocCnt);

    // exact_matching_flag
    seiLength += 1;

    // broken_link_flag
    seiLength += 1;

    seiLength = (seiLength + 7) >> 3;

    return seiLength;
}

EB_U32 GetContentLightLevelSEILength()
{
    EB_U32    seiLength = 0;

    // max_content_light_level
    seiLength += 16;

    // max_pixel_average_light_level
    seiLength += 16;

    seiLength = (seiLength + 7) >> 3;

    return seiLength;
}

EB_U32 GetMasteringDisplayColorVolumeSEILength()
{
    EB_U32    seiLength = 0;

    // R, G, B Primaries
    seiLength += 2 * 16 + 2 * 16 + 2 * 16;

    // White Point Co-Ordinates
    seiLength += 2 * 16;

    // min & max luminance values
    seiLength += 2 * 32;

    seiLength = (seiLength + 7) >> 3;

    return seiLength;
}
