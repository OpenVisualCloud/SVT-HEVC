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
    size_t sizeCopy = (size_t)((EB_U64)(&dstVuiPtr->hrdParametersPtr) -
        (EB_U64)(&dstVuiPtr->aspectRatioInfoPresentFlag));

    EB_MEMCPY(dstVuiPtr, srcVuiPtr, sizeCopy);
    EB_MEMCPY(dstVuiPtr->hrdParametersPtr, srcVuiPtr->hrdParametersPtr, sizeof(AppHrdParameters_t));

    return;
}

static void EbVideoUsabilityInfoDctor(EB_PTR p)
{
    AppVideoUsabilityInfo_t *obj = (AppVideoUsabilityInfo_t*)p;
    EB_FREE(obj->hrdParametersPtr);
}

EB_ERRORTYPE EbVideoUsabilityInfoCtor(
    AppVideoUsabilityInfo_t *vuiPtr)
{
    EB_CALLOC(vuiPtr->hrdParametersPtr, 1, sizeof(AppHrdParameters_t));
    // Initialize vui variables
    vuiPtr->dctor = EbVideoUsabilityInfoDctor;
    vuiPtr->aspectRatioInfoPresentFlag = EB_TRUE;
    vuiPtr->defaultDisplayWindowFlag = EB_TRUE;
    vuiPtr->hrdParametersPtr->cpbDpbDelaysPresentFlag = (EB_BOOL)((vuiPtr->hrdParametersPtr->nalHrdParametersPresentFlag || vuiPtr->hrdParametersPtr->vclHrdParametersPresentFlag)
                                                                   && vuiPtr->vuiHrdParametersPresentFlag);

    return EB_ErrorNone;
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
        seiLength += vuiPtr->hrdParametersPtr->auCpbRemovalDelayLengthMinus1 + 1;

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
        seiLength += vuiPtr->hrdParametersPtr->auCpbRemovalDelayLengthMinus1 + 1;

        // dpb_delay_offset
        seiLength += vuiPtr->hrdParametersPtr->dpbOutputDelayDuLengthMinus1 + 1;
    }

    // concatenation_flag
    seiLength += 1;

    // au_cpb_removal_delay_delta_minus1
    seiLength += vuiPtr->hrdParametersPtr->auCpbRemovalDelayLengthMinus1 + 1;

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

EB_U32 GetActiveParameterSetSEILength(
    AppActiveparameterSetSei_t    *activeParameterSet)
{
    EB_U32       seiLength = 0;

    // active_video_parameter_set_id
    seiLength += 4;

    // self_contained_cvs_flag
    seiLength += 1;

    // no_param_set_update_flag
    seiLength += 1;

    //num_sps_ids_minus1
    seiLength += GetUvlcCodeLength(activeParameterSet->numSpsIdsMinus1);

    //active_seq_param_set_id
    seiLength += GetUvlcCodeLength(activeParameterSet->activeSeqParameterSetId);

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
