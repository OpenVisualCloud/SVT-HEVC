/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureControlSet.h"
#include "EbCodingUnit.h"
#include "EbMdRateEstimation.h"
#include "EbSampleAdaptiveOffset.h"
#include "EbLambdaRateTables.h"
#include "EbReferenceObject.h"
static EB_S32 MinSaoOffsetvalueEO[2 /*8bit+10bit*/][SAO_EO_CATEGORIES] = { {0, 0, -7, -7},{0,  0, -31, -31} };
static EB_S32 MaxSaoOffsetvalueEO[2 /*8bit+10bit*/][SAO_EO_CATEGORIES] = { {7, 7,  0,  0},{31, 31, 0,    0} };
static EB_S32 MaxSaoOffsetvalueBO[2 /*8bit+10bit*/] = { 7 , 31 };
static EB_S32 MinSaoOffsetvalueBO[2 /*8bit+10bit*/] = { -7 ,-31 };

/********************************************
 * Sao Stats Ctor
 ********************************************/
EB_ERRORTYPE SaoStatsCtor(SaoStats_t **saoStatsPtr)
{
    EB_U32 videoComponent; 

    SaoStats_t *saoStats;
    EB_MALLOC(SaoStats_t*, saoStats, sizeof(SaoStats_t), EB_N_PTR);
    *saoStatsPtr = saoStats;

    EB_MALLOC(EB_S32**, saoStats->boDiff, sizeof(EB_S32*) * 3, EB_N_PTR);

    EB_MALLOC(EB_U16**, saoStats->boCount, sizeof(EB_U32*) * 3, EB_N_PTR);

    for(videoComponent = 0; videoComponent < 3; ++videoComponent) {
        EB_MALLOC(EB_S32*, saoStats->boDiff[videoComponent], sizeof(EB_S32) * SAO_BO_INTERVALS, EB_N_PTR);
        EB_MALLOC(EB_U16*, saoStats->boCount[videoComponent], sizeof(EB_U32) * SAO_BO_INTERVALS, EB_N_PTR);
    }
    return EB_ErrorNone;
}

/********************************************
* DetermineSaoLumaModeOffsets
*   generates Sao Luma Offsets
********************************************/
static void DetermineSaoLumaModeOffsets(
    SaoStats_t                 *saoStats,
    SaoParameters_t            *saoParams,
    EB_U64                      lambda,                 // input parameter, lambda, used to compute the cost
    MdRateEstimationContext_t  *mdRateEstimationPtr,    // input parameter, MD table bits, used to compute the cost
    EB_S64                     *saoLumaBestCost,        // output parameter, best SAO Luma cost
    EB_BOOL                     is10bit)
{
    EB_U32          boIndex, boInterval, bestSaoBandPosition = 0, eoType, eoIndex, bestEotype = 0, saoOffset;
    EB_U64 maxCost = (EB_U64)~0;

    EB_S32 boOffset[SAO_BO_INTERVALS];
    EB_S64 boIntervalDistortion[SAO_BO_INTERVALS];
    EB_S64 boDistortion;
    EB_S64 boRate;
    EB_S64 boCost;
    EB_S64 boBestCost = (maxCost >> 1);

    EB_S32 eoOffset[SAO_EO_TYPES][SAO_EO_CATEGORIES];
    EB_S64 eoCategoryDistortion[SAO_EO_TYPES][SAO_EO_CATEGORIES];
    EB_S64 eoTypeDistortion[SAO_EO_TYPES] = { 0 };
    EB_S64 eoTypeRate[SAO_EO_TYPES] = { 0 };
    EB_S64 eoTypeCost[SAO_EO_TYPES];
    EB_S64 eoBestCost = (maxCost >> 1);

    EB_U64 saoOffsetFlagBitsNum;
    EB_S64 saoOffRate;
    EB_S64 saoOffCost;
    EB_U32 distortionShift = is10bit ? 4 : 0;

    // *Note: in all Cost Computations we are not including the Cost of the Merge Left & Up Costs - they will be added later in TestSaoCopyModes()
    saoOffRate = mdRateEstimationPtr->saoTypeIndexBits[0];
    saoOffCost = ((saoOffRate *lambda) + MD_OFFSET) >> MD_SHIFT;

    // BO Candidates
    for (boIndex = 0; boIndex < SAO_BO_INTERVALS; ++boIndex) {
        boOffset[boIndex] = (saoStats->boCount[0][boIndex] == 0) ?
            0 :
            ROUND(saoStats->boDiff[0][boIndex] / (EB_S32)saoStats->boCount[0][boIndex]);
        //TODO fix this rounding here . it is useless since no float varibles in it.

        boOffset[boIndex] = CLIP3(MinSaoOffsetvalueBO[is10bit], MaxSaoOffsetvalueBO[is10bit], boOffset[boIndex]); // ramdas change I0066

                                                                                                                  //boIntervalDistortion[boIndex] = - (2 * boOffset[boIndex] * saoStats->boDiff[0][boIndex]) + ((EB_S32)saoStats->boCount[0][boIndex] * SQR(boOffset[boIndex]));
        boIntervalDistortion[boIndex] = (-(2 * boOffset[boIndex] * saoStats->boDiff[0][boIndex]) + ((EB_S32)saoStats->boCount[0][boIndex] * SQR(boOffset[boIndex]))) >> distortionShift;

    }


    for (boInterval = 0; boInterval< SAO_BO_INTERVALS - SAO_BO_LEN + 1; ++boInterval) {
        boDistortion = 0;

        for (boIndex = boInterval; boIndex < boInterval + SAO_BO_LEN; ++boIndex) {
            boDistortion += boIntervalDistortion[boIndex];
        }

        GetSaoOffsetsFractionBits(
            5,
            &(boOffset[boInterval]),
            mdRateEstimationPtr,
            &saoOffsetFlagBitsNum);

        boRate = saoOffsetFlagBitsNum;
        boCost = (boDistortion << COST_PRECISION) + (((boRate *lambda) + MD_OFFSET) >> MD_SHIFT);

        if (boCost < boBestCost) {
            boBestCost = boCost;
            bestSaoBandPosition = boInterval;
        }
    }

    // EO Candidates
    for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {

        for (eoIndex = 0; eoIndex < SAO_EO_CATEGORIES; ++eoIndex) {
            eoOffset[eoType][eoIndex] = (saoStats->eoCount[0][eoType][eoIndex] == 0) ?
                0 :
                ROUND(saoStats->eoDiff[0][eoType][eoIndex] / (EB_S32)saoStats->eoCount[0][eoType][eoIndex]);

            eoOffset[eoType][eoIndex] = CLIP3(MinSaoOffsetvalueEO[is10bit][eoIndex], MaxSaoOffsetvalueEO[is10bit][eoIndex], eoOffset[eoType][eoIndex]);

            //eoCategoryDistortion[eoType][eoIndex] = - (2 * eoOffset[eoType][eoIndex] * saoStats->eoDiff[0][eoType][eoIndex]) + ((EB_S32)saoStats->eoCount[0][eoType][eoIndex] * SQR(eoOffset[eoType][eoIndex]));
            eoCategoryDistortion[eoType][eoIndex] = (-(2 * eoOffset[eoType][eoIndex] * saoStats->eoDiff[0][eoType][eoIndex]) + ((EB_S32)saoStats->eoCount[0][eoType][eoIndex] * SQR(eoOffset[eoType][eoIndex]))) >> distortionShift;

            eoTypeDistortion[eoType] += eoCategoryDistortion[eoType][eoIndex];

        }

        GetSaoOffsetsFractionBits(
            eoType + 1,
            &(eoOffset[eoType][0]),
            mdRateEstimationPtr,
            &saoOffsetFlagBitsNum);

        eoTypeRate[eoType] = saoOffsetFlagBitsNum;
        eoTypeCost[eoType] = (eoTypeDistortion[eoType] << COST_PRECISION) + (((eoTypeRate[eoType] * lambda) + MD_OFFSET) >> MD_SHIFT);

        if (eoTypeCost[eoType] < eoBestCost) {

            eoBestCost = eoTypeCost[eoType];
            bestEotype = eoType;
        }
    }

    // Add SAO Type Cost
    boBestCost += (((mdRateEstimationPtr->saoTypeIndexBits[5] * lambda) + MD_OFFSET) >> MD_SHIFT);
    eoBestCost += (((mdRateEstimationPtr->saoTypeIndexBits[bestEotype + 1] * lambda) + MD_OFFSET) >> MD_SHIFT);

    if ((boBestCost < saoOffCost) || (eoBestCost < saoOffCost)) {

        *saoLumaBestCost = MIN(boBestCost, eoBestCost);

        if (*saoLumaBestCost == boBestCost) {
            saoParams->saoTypeIndex[SAO_COMPONENT_LUMA] = 5;
            saoParams->saoBandPosition[SAO_COMPONENT_LUMA] = bestSaoBandPosition;
            for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                saoParams->saoOffset[SAO_COMPONENT_LUMA][saoOffset] = boOffset[bestSaoBandPosition + saoOffset];
            }
        }
        else if (*saoLumaBestCost == eoBestCost) {
            saoParams->saoTypeIndex[SAO_COMPONENT_LUMA] = bestEotype + 1;
            for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                saoParams->saoOffset[SAO_COMPONENT_LUMA][saoOffset] = eoOffset[bestEotype][saoOffset];
            }
        }
    }
    else {

        saoParams->saoTypeIndex[SAO_COMPONENT_LUMA] = 0;
        *saoLumaBestCost = saoOffCost;
    }

    return;
}
/********************************************
* DetermineSaoChromaModeOffsets
*   Generates Sao Chroma Offsets
********************************************/
static void DetermineSaoChromaModeOffsets(
    SaoStats_t                 *saoStats,
    SaoParameters_t            *saoParams,
    EB_U64                      lambda,                 // input parameter, lambda, used to compute the cost
    MdRateEstimationContext_t  *mdRateEstimationPtr,    // input parameter, MD table bits, used to compute the cost
    EB_S64                     *saoChromaBestCost,      // output parameter, best SAO Chroma cost
    EB_BOOL                     is10bit)
{
#if 0 // BO for CHROMA hurts BD-Rate
    EB_S64 boIntervalDistortion[SAO_BO_INTERVALS];
    EB_S64 boDistortion;
    EB_S64 boRate = 0;
    EB_S64 boCost;
    EB_S64 boBestCost;
    EB_U32 boIndex;
    EB_U32 boInterval;
#endif
    EB_S32 boOffset[SAO_COMPONENT_CHROMA_CB][SAO_BO_INTERVALS];
    EB_S64 boChromaBestCost = 0;
    EB_U32 chromaComponent, eoType, eoIndex, bestEotype = 1, saoOffset;
    EB_U64 maxCost = (EB_U64)~0;

    EB_U32 bestSaoBandPosition[SAO_COMPONENT_CHROMA_CB] = { 0 };

    EB_S32 eoOffset[SAO_COMPONENT_CHROMA_CB][SAO_EO_TYPES][SAO_EO_CATEGORIES];
    EB_S64 eoCategoryDistortion[SAO_EO_TYPES][SAO_EO_CATEGORIES];
    EB_S64 eoTypeDistortion[SAO_EO_TYPES] = { 0 };
    EB_S64 eoTypeRate[SAO_EO_TYPES];
    EB_S64 eoTypeCost[SAO_EO_TYPES] = { 0 };
    EB_S64 eoChromaBestCost;

    EB_U64 saoOffsetFlagBitsNum;
    EB_S64 saoOffRate;
    EB_S64 saoOffCost;
    EB_U32 distortionShift = is10bit ? 4 : 0;

    // *Note: in all Cost Computations we are not including the Cost of the Merge Left & Up Costs - they will be added later in TestSaoCopyModes()
    saoOffRate = mdRateEstimationPtr->saoTypeIndexBits[0];
    saoOffCost = ((saoOffRate *lambda) + MD_OFFSET) >> MD_SHIFT;


#if 1 // BO for CHROMA hurts BD-Rate
    boChromaBestCost = (maxCost >> 1);
#else
    // BO Candidates
    for (chromaComponent = SAO_COMPONENT_CHROMA; chromaComponent < SAO_COMPONENT_CHROMA_CR; ++chromaComponent) {
        boBestCost  = (maxCost >> 1); 
        for (boIndex = 0; boIndex < SAO_BO_INTERVALS; ++boIndex) {
            boOffset[chromaComponent - 1][boIndex] = (saoStats->boCount[chromaComponent][boIndex] == 0) ?
                0 :
                ROUND(saoStats->boDiff[chromaComponent][boIndex] / (EB_S32)saoStats->boCount[chromaComponent][boIndex]);
            //TODO fix this rounding here . it is useless since no float varibles in it.

            boOffset[chromaComponent - 1][boIndex] = CLIP3(MinSaoOffsetvalueBO[is10bit], MaxSaoOffsetvalueBO[is10bit], boOffset[chromaComponent - 1][boIndex]); // ramdas change I0066

                                                                                                                      //boIntervalDistortion[boIndex] = - (2 * boOffset[boIndex] * saoStats->boDiff[0][boIndex]) + ((EB_S32)saoStats->boCount[0][boIndex] * SQR(boOffset[boIndex]));
            boIntervalDistortion[boIndex] = (-(2 * boOffset[chromaComponent - 1][boIndex] * saoStats->boDiff[0][boIndex]) + ((EB_S32)saoStats->boCount[chromaComponent][boIndex] * SQR(boOffset[chromaComponent - 1][boIndex]))) >> distortionShift;

        }


        for (boInterval = 0; boInterval < SAO_BO_INTERVALS - SAO_BO_LEN + 1; ++boInterval) {
            boDistortion = 0;

            for (boIndex = boInterval; boIndex < boInterval + SAO_BO_LEN; ++boIndex) {
                boDistortion += boIntervalDistortion[boIndex];
            }

            GetSaoOffsetsFractionBits(
                5,
                &(boOffset[chromaComponent - 1][boInterval]),
                mdRateEstimationPtr,
                &saoOffsetFlagBitsNum);

            boRate = saoOffsetFlagBitsNum;
            boCost = (boDistortion << COST_PRECISION) + (((boRate *lambda) + MD_OFFSET) >> MD_SHIFT);

            if (boCost < boBestCost) {
                boBestCost = boCost;
                bestSaoBandPosition[chromaComponent - 1] = boInterval;
            }
        }
        boChromaBestCost += boBestCost;
    }
#endif
    // EO Candidates 

    eoChromaBestCost = (maxCost >> 1);

    for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {

        for (chromaComponent = SAO_COMPONENT_CHROMA; chromaComponent < SAO_COMPONENT_CHROMA_CR; ++chromaComponent) {

            for (eoIndex = 0; eoIndex < SAO_EO_CATEGORIES; ++eoIndex) {

                eoOffset[chromaComponent - 1][eoType][eoIndex] = (saoStats->eoCount[chromaComponent][eoType][eoIndex] == 0) ?
                    0 :
                    ROUND(saoStats->eoDiff[chromaComponent][eoType][eoIndex] / (EB_S32)saoStats->eoCount[chromaComponent][eoType][eoIndex]);

                eoOffset[chromaComponent - 1][eoType][eoIndex] = CLIP3(MinSaoOffsetvalueEO[is10bit][eoIndex], MaxSaoOffsetvalueEO[is10bit][eoIndex], eoOffset[chromaComponent - 1][eoType][eoIndex]);

                //eoCategoryDistortion[eoType][eoIndex] = - (2 * eoOffset[chromaComponent - 1][eoType][eoIndex] * saoStats->eoDiff[chromaComponent][eoType][eoIndex]) + ((EB_S32)saoStats->eoCount[chromaComponent][eoType][eoIndex] * SQR(eoOffset[chromaComponent - 1][eoType][eoIndex]));
                eoCategoryDistortion[eoType][eoIndex] = (-(2 * eoOffset[chromaComponent - 1][eoType][eoIndex] * saoStats->eoDiff[chromaComponent][eoType][eoIndex]) + ((EB_S32)saoStats->eoCount[chromaComponent][eoType][eoIndex] * SQR(eoOffset[chromaComponent - 1][eoType][eoIndex]))) >> distortionShift;

                eoTypeDistortion[eoType] += eoCategoryDistortion[eoType][eoIndex];
            }

            GetSaoOffsetsFractionBits(eoType + 1, &(eoOffset[chromaComponent - 1][eoType][0]), mdRateEstimationPtr, &saoOffsetFlagBitsNum);
            eoTypeRate[eoType] = saoOffsetFlagBitsNum;
            eoTypeCost[eoType] += (eoTypeDistortion[eoType] << COST_PRECISION) + (((eoTypeRate[eoType] * lambda) + MD_OFFSET) >> MD_SHIFT);
        }

        if (eoTypeCost[eoType] < eoChromaBestCost) {

            eoChromaBestCost = eoTypeCost[eoType];
            bestEotype = eoType;
        }
    }


    // Add SAO Type Cost
#if 0 // BO for CHROMA hurts BD-Rate
    boChromaBestCost += (((mdRateEstimationPtr->saoTypeIndexBits[5] * lambda) + MD_OFFSET) >> MD_SHIFT);
#endif
    eoChromaBestCost += (((mdRateEstimationPtr->saoTypeIndexBits[bestEotype + 1] * lambda) + MD_OFFSET) >> MD_SHIFT);


    if ((boChromaBestCost < saoOffCost) || (eoChromaBestCost < saoOffCost)) {
        *saoChromaBestCost = MIN(boChromaBestCost, eoChromaBestCost);

        if (*saoChromaBestCost == boChromaBestCost) {

            saoParams->saoTypeIndex[SAO_COMPONENT_CHROMA] = 5;

            for (chromaComponent = SAO_COMPONENT_CHROMA; chromaComponent < SAO_COMPONENT_CHROMA_CR; ++chromaComponent) {

                saoParams->saoBandPosition[chromaComponent] = bestSaoBandPosition[chromaComponent - 1];
                for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                    saoParams->saoOffset[chromaComponent][saoOffset] = boOffset[chromaComponent - 1][bestSaoBandPosition[chromaComponent - 1] + saoOffset];
                }
            }
        }
        else {
            saoParams->saoTypeIndex[SAO_COMPONENT_CHROMA] = bestEotype + 1;
            for (chromaComponent = SAO_COMPONENT_CHROMA; chromaComponent < SAO_COMPONENT_CHROMA_CR; ++chromaComponent) {
                for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                    saoParams->saoOffset[chromaComponent][saoOffset] = eoOffset[chromaComponent - 1][bestEotype][saoOffset];
                }
            }
        }
    }
    else {

        saoParams->saoTypeIndex[SAO_COMPONENT_CHROMA] = 0;
        *saoChromaBestCost = saoOffCost;
    }

    return;
}


/********************************************
 * DetermineSaoLumaModeOffsets
 *   generates Sao Luma Offsets
 ********************************************/
static void DetermineSaoLumaModeOffsets_OnlyEo_90_45_135(
    SaoStats_t                 *saoStats,
    SaoParameters_t            *saoParams,
    EB_U64                      lambda,                 // input parameter, lambda, used to compute the cost
    MdRateEstimationContext_t  *mdRateEstimationPtr,    // input parameter, MD table bits, used to compute the cost
    EB_S64                     *saoLumaBestCost,        // output parameter, best SAO Luma cost
    EB_BOOL                     is10bit      )  
{
    EB_U32 eoType, eoIndex, bestEotype=0, saoOffset;
    EB_U64 maxCost = (EB_U64) ~0;

    EB_S32 eoOffset[SAO_EO_TYPES][SAO_EO_CATEGORIES];
    EB_S64 eoCategoryDistortion[SAO_EO_TYPES][SAO_EO_CATEGORIES];
    EB_S64 eoTypeDistortion[SAO_EO_TYPES] = {0};  
    EB_S64 eoTypeRate[SAO_EO_TYPES] = {0};
    EB_S64 eoTypeCost[SAO_EO_TYPES];
    EB_S64 eoBestCost = (maxCost >> 1);  

    EB_U64 saoOffsetFlagBitsNum;
    EB_S64 saoOffRate;
    EB_S64 saoOffCost; 
    EB_U32 distortionShift = is10bit ? 4 : 0;

    // *Note: in all Cost Computations we are not including the Cost of the Merge Left & Up Costs - they will be added later in TestSaoCopyModes()
    saoOffRate = mdRateEstimationPtr->saoTypeIndexBits[0];
    saoOffCost = ((saoOffRate *lambda) + MD_OFFSET) >> MD_SHIFT;

    saoParams->saoTypeIndex[SAO_COMPONENT_LUMA]    = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][0]    = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][1]    = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][2]    = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][3]    = 0;
    saoParams->saoBandPosition[SAO_COMPONENT_LUMA] = 0;
    
    // EO Candidates
    for (eoType = 1; eoType < SAO_EO_TYPES; ++eoType ) {

        for (eoIndex = 0; eoIndex < SAO_EO_CATEGORIES; ++eoIndex ) {
            eoOffset[eoType][eoIndex] = (saoStats->eoCount[0][eoType][eoIndex] == 0) ?
                                         0 :
                                         ROUND(saoStats->eoDiff[0][eoType][eoIndex] / (EB_S32)saoStats->eoCount[0][eoType][eoIndex]);

            eoOffset[eoType][eoIndex] = CLIP3(MinSaoOffsetvalueEO[is10bit][eoIndex], MaxSaoOffsetvalueEO[is10bit][eoIndex], eoOffset[eoType][eoIndex]);

            //eoCategoryDistortion[eoType][eoIndex] = - (2 * eoOffset[eoType][eoIndex] * saoStats->eoDiff[0][eoType][eoIndex]) + ((EB_S32)saoStats->eoCount[0][eoType][eoIndex] * SQR(eoOffset[eoType][eoIndex]));
            eoCategoryDistortion[eoType][eoIndex] = ( - (2 * eoOffset[eoType][eoIndex] * saoStats->eoDiff[0][eoType][eoIndex]) + ((EB_S32)saoStats->eoCount[0][eoType][eoIndex] * SQR(eoOffset[eoType][eoIndex])) )>>distortionShift;

            eoTypeDistortion[eoType] += eoCategoryDistortion[eoType][eoIndex];
        }

        GetSaoOffsetsFractionBits(
            eoType + 1,
            &(eoOffset[eoType][0]),
            mdRateEstimationPtr,
            &saoOffsetFlagBitsNum);

        eoTypeRate[eoType] = saoOffsetFlagBitsNum;
        eoTypeCost[eoType] = (eoTypeDistortion[eoType] << COST_PRECISION) + (((eoTypeRate[eoType] *lambda) + MD_OFFSET) >> MD_SHIFT);

        if(eoTypeCost[eoType] < eoBestCost) {

            eoBestCost = eoTypeCost[eoType];
            bestEotype = eoType;
        }  
    }


    eoBestCost += (((mdRateEstimationPtr->saoTypeIndexBits[bestEotype + 1] *lambda) + MD_OFFSET) >> MD_SHIFT); 

    if ( eoBestCost < saoOffCost) {
        
        *saoLumaBestCost =  eoBestCost;
         saoParams->saoTypeIndex[SAO_COMPONENT_LUMA] = bestEotype + 1;
         for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
             saoParams->saoOffset[SAO_COMPONENT_LUMA][saoOffset] = eoOffset[bestEotype][saoOffset];
         }
    }
    else {

        saoParams->saoTypeIndex[SAO_COMPONENT_LUMA] = 0;
        *saoLumaBestCost = saoOffCost;
    }

    return;
}

/********************************************
 * TestSaoCopyModes
 *   tests Sao Copy Modes
 ********************************************/
static void TestSaoCopyModes(
    EB_S32                    **boDiff,              // output parameter, used to store Band Offset diff, boDiff[SAO_BO_INTERVALS]
    EB_U16                    **boCount,             // output parameter, used to store Band Offset count, boCount[SAO_BO_INTERVALS] 
	EB_S32                      eoDiff[3][SAO_EO_TYPES][SAO_EO_CATEGORIES+1],
    EB_U16                      eoCount[3][SAO_EO_TYPES][SAO_EO_CATEGORIES+1],
    SaoParameters_t            *saoPtr,              // input/output parameter, LCU Ptr, used to store SAO results
    SaoParameters_t            *leftSaoPtr,          // input parameter, left LCU Ptr, used to detect the availability of the left neighbor candidate 
    SaoParameters_t            *upSaoPtr,            // input parameter, up LCU Ptr, used to detect the availability of the top neighbor candidate 
    EB_U64                      lambda,              // input parameter, lambda, used to compute the cost
    MdRateEstimationContext_t  *mdRateEstimationPtr, // input parameter, MD table bits, used to compute the cost
    EB_S64                     *saoLumaBestCost,     // input/output parameter, best SAO Luma cost
    EB_S64                     *saoChromaBestCost,   // input/output parameter, best SAO Chroma cost
    EB_BOOL                     is10bit
    )   
{
    EB_U32 chromaComponent, videoComponent, isChromaComponent, saoOffset;
    EB_U64 maxCost = (EB_U64)~0;

    EB_S64 saoLumaLeftMergeDistortion   = 0;    
    EB_S64 saoChromaLeftMergeDistortion = 0;  
    EB_U64 saoMergeLeftFlagBitsNum;
    EB_S64 saoMergeLeftRate;
    EB_S64 saoLumaMergeLeftCost         = 0; 
    EB_S64 saoChromaMergeLeftCost       = 0;
    EB_S64 saoMergeLeftCost             = (maxCost >> 1);

    EB_S64 saoLumaUpMergeDistortion     = 0;   
    EB_S64 saoChromaUpMergeDistortion   = 0;  
    EB_U64 saoMergeUpFlagBitsNum; 
    EB_S64 saoMergeUpRate;
    EB_S64 saoLumaMergeUpCost           = 0; 
    EB_S64 saoChromaMergeUpCost         = 0;  
    EB_S64 saoMergeUpCost               = (maxCost >> 1);

    EB_S64 saoBestCost;
    EB_U32 distortionShift = is10bit ? 4 : 0;
    
    // Add Left Flag & Up Flag costs to the best Luma & Chroma Costs in order to get the best SAO Cost
    // The best SAO Cost is either SAO Off Cost or BO Cost or EO Cost 
    saoMergeLeftFlagBitsNum = (leftSaoPtr) ?  
        mdRateEstimationPtr->saoMergeFlagBits[0] :
        0;
    
    saoMergeUpFlagBitsNum  = (upSaoPtr) ? 
        mdRateEstimationPtr->saoMergeFlagBits[0] :
        0; 

    saoBestCost        = *saoLumaBestCost + *saoChromaBestCost + ((((saoMergeLeftFlagBitsNum + saoMergeUpFlagBitsNum) * lambda) + MD_OFFSET) >> MD_SHIFT); 
    *saoLumaBestCost   = *saoLumaBestCost   + ((((saoMergeLeftFlagBitsNum + saoMergeUpFlagBitsNum) * lambda) + MD_OFFSET) >> MD_SHIFT); 
    *saoChromaBestCost = *saoChromaBestCost + ((((saoMergeLeftFlagBitsNum + saoMergeUpFlagBitsNum) * lambda) + MD_OFFSET) >> MD_SHIFT); 

    // Left-Neighbor Candidate
    if(leftSaoPtr) { 

        // Luma            
        if(leftSaoPtr->saoTypeIndex[SAO_COMPONENT_LUMA] > 0) {
            
            if (leftSaoPtr->saoTypeIndex[SAO_COMPONENT_LUMA] == 5) { // BO
                
                for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                    
                    saoLumaLeftMergeDistortion += - (2 * leftSaoPtr->saoOffset[SAO_COMPONENT_LUMA][saoOffset] * boDiff[SAO_COMPONENT_LUMA][leftSaoPtr->saoBandPosition[SAO_COMPONENT_LUMA] + saoOffset]) + ((EB_S32)boCount[SAO_COMPONENT_LUMA][leftSaoPtr->saoBandPosition[SAO_COMPONENT_LUMA] + saoOffset] * SQR(leftSaoPtr->saoOffset[SAO_COMPONENT_LUMA][saoOffset]));
                }
            } 
            else { // EO
                
                for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                    
                    saoLumaLeftMergeDistortion += - (2 * leftSaoPtr->saoOffset[SAO_COMPONENT_LUMA][saoOffset] * eoDiff[SAO_COMPONENT_LUMA][leftSaoPtr->saoTypeIndex[SAO_COMPONENT_LUMA] - 1][saoOffset]) + ((EB_S32)eoCount[SAO_COMPONENT_LUMA][leftSaoPtr->saoTypeIndex[SAO_COMPONENT_LUMA] - 1][saoOffset] * SQR(leftSaoPtr->saoOffset[SAO_COMPONENT_LUMA][saoOffset]));
                }
            }
        }

        // Chroma
        if(leftSaoPtr->saoTypeIndex[SAO_COMPONENT_CHROMA] > 0) {
                
            if (leftSaoPtr->saoTypeIndex[SAO_COMPONENT_CHROMA] == 5) { // BO
                
                for (chromaComponent = SAO_COMPONENT_CHROMA; chromaComponent < SAO_COMPONENT_CHROMA_CR; ++chromaComponent) {
                    
                    for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                        
                        saoChromaLeftMergeDistortion += - (2 * leftSaoPtr->saoOffset[chromaComponent][saoOffset] * boDiff[chromaComponent][leftSaoPtr->saoBandPosition[chromaComponent] + saoOffset]) + ((EB_S32)boCount[chromaComponent][leftSaoPtr->saoBandPosition[chromaComponent] + saoOffset] * SQR(leftSaoPtr->saoOffset[chromaComponent][saoOffset]));
                    }
                } 
            }
            else { // EO
                
                for (chromaComponent = SAO_COMPONENT_CHROMA; chromaComponent < SAO_COMPONENT_CHROMA_CR; ++chromaComponent) {
                    
                    for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                        
                        saoChromaLeftMergeDistortion += - (2 * leftSaoPtr->saoOffset[chromaComponent][saoOffset] * eoDiff[chromaComponent][leftSaoPtr->saoTypeIndex[SAO_COMPONENT_CHROMA] - 1][saoOffset]) + ((EB_S32)eoCount[chromaComponent][leftSaoPtr->saoTypeIndex[SAO_COMPONENT_CHROMA] - 1][saoOffset] * SQR(leftSaoPtr->saoOffset[chromaComponent][saoOffset]));
                    }
                }
            }
        }
        
        saoMergeLeftRate       =  mdRateEstimationPtr->saoMergeFlagBits[1];

        //CHKN
        saoLumaLeftMergeDistortion   = saoLumaLeftMergeDistortion   >> distortionShift;
        saoChromaLeftMergeDistortion = saoChromaLeftMergeDistortion >> distortionShift;

        saoLumaMergeLeftCost   = (saoLumaLeftMergeDistortion << COST_PRECISION)   + (((saoMergeLeftRate *lambda) + MD_OFFSET) >> MD_SHIFT);
        saoChromaMergeLeftCost = (saoChromaLeftMergeDistortion << COST_PRECISION) + (((saoMergeLeftRate *lambda) + MD_OFFSET) >> MD_SHIFT);
        saoMergeLeftCost       = (saoLumaLeftMergeDistortion << COST_PRECISION)   + (saoChromaLeftMergeDistortion << COST_PRECISION) + (((saoMergeLeftRate *lambda) + MD_OFFSET) >> MD_SHIFT);
    }


    // Top-Neighbor Candidate
    if(upSaoPtr) {

        // Luma            
        if(upSaoPtr->saoTypeIndex[SAO_COMPONENT_LUMA] > 0) {
            
            if (upSaoPtr->saoTypeIndex[SAO_COMPONENT_LUMA] == 5) { // BO

                for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                    
                    saoLumaUpMergeDistortion += - (2 * upSaoPtr->saoOffset[SAO_COMPONENT_LUMA][saoOffset] * boDiff[SAO_COMPONENT_LUMA][upSaoPtr->saoBandPosition[SAO_COMPONENT_LUMA] + saoOffset]) + ((EB_S32)boCount[SAO_COMPONENT_LUMA][upSaoPtr->saoBandPosition[SAO_COMPONENT_LUMA] + saoOffset] * SQR(upSaoPtr->saoOffset[SAO_COMPONENT_LUMA][saoOffset]));
                }
            } else { // EO

                for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                    
                    saoLumaUpMergeDistortion += - (2 * upSaoPtr->saoOffset[SAO_COMPONENT_LUMA][saoOffset] * eoDiff[SAO_COMPONENT_LUMA][upSaoPtr->saoTypeIndex[SAO_COMPONENT_LUMA] - 1][saoOffset]) + ((EB_S32)eoCount[SAO_COMPONENT_LUMA][upSaoPtr->saoTypeIndex[SAO_COMPONENT_LUMA] - 1][saoOffset] * SQR(upSaoPtr->saoOffset[SAO_COMPONENT_LUMA][saoOffset]));
                }
            }
        }

        // Chroma
        if(upSaoPtr->saoTypeIndex[SAO_COMPONENT_CHROMA] > 0) {
                
            if (upSaoPtr->saoTypeIndex[SAO_COMPONENT_CHROMA] == 5) { // BO
                
                for (chromaComponent = SAO_COMPONENT_CHROMA; chromaComponent < SAO_COMPONENT_CHROMA_CR; ++chromaComponent) {
                    
                    for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                        
                        saoChromaUpMergeDistortion += - (2 * upSaoPtr->saoOffset[chromaComponent][saoOffset] * boDiff[chromaComponent][upSaoPtr->saoBandPosition[chromaComponent] + saoOffset]) + ((EB_S32)boCount[chromaComponent][upSaoPtr->saoBandPosition[chromaComponent] + saoOffset] * SQR(upSaoPtr->saoOffset[chromaComponent][saoOffset]));
                    }
                } 
            }
            else { // EO
                
                for (chromaComponent = SAO_COMPONENT_CHROMA; chromaComponent < SAO_COMPONENT_CHROMA_CR; ++chromaComponent) {
                    
                    for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
                        
                        saoChromaUpMergeDistortion += - (2 * upSaoPtr->saoOffset[chromaComponent][saoOffset] * eoDiff[chromaComponent][upSaoPtr->saoTypeIndex[SAO_COMPONENT_CHROMA] - 1][saoOffset]) + ((EB_S32)eoCount[chromaComponent][upSaoPtr->saoTypeIndex[SAO_COMPONENT_CHROMA] - 1][saoOffset] * SQR(upSaoPtr->saoOffset[chromaComponent][saoOffset]));
                    }
                }
            }
        }

         //CHKN
        saoLumaUpMergeDistortion   = saoLumaUpMergeDistortion   >> distortionShift;
        saoChromaUpMergeDistortion = saoChromaUpMergeDistortion >> distortionShift;
        
        saoMergeUpFlagBitsNum = mdRateEstimationPtr->saoMergeFlagBits[1];
        saoMergeUpRate        = saoMergeLeftFlagBitsNum + saoMergeUpFlagBitsNum;
        saoLumaMergeUpCost    = (saoLumaUpMergeDistortion << COST_PRECISION)   + (((saoMergeUpRate *lambda) + MD_OFFSET) >> MD_SHIFT);
        saoChromaMergeUpCost  = (saoChromaUpMergeDistortion << COST_PRECISION) + (((saoMergeUpRate *lambda) + MD_OFFSET) >> MD_SHIFT);
        saoMergeUpCost        = (saoLumaUpMergeDistortion << COST_PRECISION)   + (saoChromaUpMergeDistortion << COST_PRECISION) + (((saoMergeUpRate *lambda) + MD_OFFSET) >> MD_SHIFT);
        
    }

    if ((saoMergeLeftCost < saoBestCost) || (saoMergeUpCost < saoBestCost) ) {

        saoBestCost = MIN(saoMergeLeftCost, saoMergeUpCost);

        if (saoBestCost == saoMergeLeftCost && leftSaoPtr) {
			
            saoPtr->saoMergeLeftFlag = EB_TRUE;
            
            for (videoComponent = SAO_COMPONENT_LUMA; videoComponent < SAO_COMPONENT_CHROMA_CR; ++videoComponent) {
                
                isChromaComponent = (videoComponent == SAO_COMPONENT_LUMA) ? SAO_COMPONENT_LUMA : SAO_COMPONENT_CHROMA;
                saoPtr->saoTypeIndex[isChromaComponent] = leftSaoPtr->saoTypeIndex[isChromaComponent];
                saoPtr->saoBandPosition[videoComponent] = leftSaoPtr->saoBandPosition[videoComponent];
                EB_MEMCPY(saoPtr->saoOffset[videoComponent], leftSaoPtr->saoOffset[videoComponent], NUMBER_SAO_OFFSETS * sizeof(EB_S32)); 
                *saoLumaBestCost   = saoLumaMergeLeftCost;
                *saoChromaBestCost = saoChromaMergeLeftCost; 
            }

        }
        else if (saoBestCost == saoMergeUpCost && upSaoPtr) {
			
            saoPtr->saoMergeUpFlag = EB_TRUE;

            for (videoComponent = SAO_COMPONENT_LUMA; videoComponent < SAO_COMPONENT_CHROMA_CR; ++videoComponent) {
                
                isChromaComponent = (videoComponent == SAO_COMPONENT_LUMA) ? SAO_COMPONENT_LUMA : SAO_COMPONENT_CHROMA;
                saoPtr->saoTypeIndex[isChromaComponent] = upSaoPtr->saoTypeIndex[isChromaComponent];
                saoPtr->saoBandPosition[videoComponent] = upSaoPtr->saoBandPosition[videoComponent];
                EB_MEMCPY(saoPtr->saoOffset[videoComponent], upSaoPtr->saoOffset[videoComponent], NUMBER_SAO_OFFSETS * sizeof(EB_S32));
                *saoLumaBestCost   = saoLumaMergeUpCost;
                *saoChromaBestCost = saoChromaMergeUpCost; 
            }
        }
    }

    return;
}

/********************************************
 * SaoGenerationDecision
 *   generates SAO offsets and chooses best SAO mode
 ********************************************/
EB_ERRORTYPE SaoGenerationDecision(
    SaoStats_t                 *saoStats,     
    SaoParameters_t            *saoParams,
    MdRateEstimationContext_t  *mdRateEstimationPtr,
    EB_U64                      fullLambda,
    EB_U64                      fullChromaLambdaSao,
    EB_BOOL                     mmSao,
    PictureControlSet_t        *pictureControlSetPtr,
    EB_U32                      tbOriginX,
    EB_U32                      tbOriginY,
    EB_U32                      lcuWidth,
    EB_U32                      lcuHeight,
    SaoParameters_t            *saoPtr,
    SaoParameters_t            *leftSaoPtr,
    SaoParameters_t            *upSaoPtr,
    EB_S64                     *saoLumaBestCost,
    EB_S64                     *saoChromaBestCost)

{
    EB_ERRORTYPE             return_error    = EB_ErrorNone;

	EbPictureBufferDesc_t  *inputPicturePtr;

    inputPicturePtr = pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr;

     EbPictureBufferDesc_t   *reconPicturePtr;
     if(pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
        reconPicturePtr = ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->referencePicture;
     else
		reconPicturePtr = pictureControlSetPtr->reconPicturePtr;

    const EB_COLOR_FORMAT colorFormat = reconPicturePtr->colorFormat;
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    // Intialise SAO  Parameters         
    saoParams->saoTypeIndex[SAO_COMPONENT_LUMA]     = 0;
    saoParams->saoTypeIndex[SAO_COMPONENT_CHROMA]   = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][0]     = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][1]     = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][2]     = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][3]     = 0;
    saoParams->saoBandPosition[SAO_COMPONENT_LUMA]  = 0;
    saoParams->saoMergeLeftFlag                     = EB_FALSE;
    saoParams->saoMergeUpFlag                       = EB_FALSE;
    *saoLumaBestCost                                = 0;
    *saoChromaBestCost                              = 0;


    if (mmSao) {
        // Y
        SaoGatherFunctionTableLossy[!!(ASM_TYPES & PREAVX2_MASK)](
            &(inputPicturePtr->bufferY[(inputPicturePtr->originY + tbOriginY) * inputPicturePtr->strideY + inputPicturePtr->originX + tbOriginX]),
            inputPicturePtr->strideY,
            &(reconPicturePtr->bufferY[(reconPicturePtr->originY + tbOriginY) * reconPicturePtr->strideY + reconPicturePtr->originX + tbOriginX]),
            reconPicturePtr->strideY,
            lcuWidth,
            lcuHeight,
            saoStats->boDiff[0],
            saoStats->boCount[0],
            saoStats->eoDiff[0],
            saoStats->eoCount[0]);

        // U
        SaoGatherFunctionTableLossy[!!(ASM_TYPES & PREAVX2_MASK)](
            &(inputPicturePtr->bufferCb[(((inputPicturePtr->originY + tbOriginY) * inputPicturePtr->strideCb) >> subHeightCMinus1) + ((inputPicturePtr->originX + tbOriginX) >> subWidthCMinus1)]),
            inputPicturePtr->strideCb,
            &(reconPicturePtr->bufferCb[(((reconPicturePtr->originY + tbOriginY) * reconPicturePtr->strideCb) >> subHeightCMinus1) + ((reconPicturePtr->originX + tbOriginX) >> subWidthCMinus1)]),
            reconPicturePtr->strideCb,
            lcuWidth >> subWidthCMinus1,
            lcuHeight >> subHeightCMinus1,
            saoStats->boDiff[1],
            saoStats->boCount[1],
            saoStats->eoDiff[1],
            saoStats->eoCount[1]);

        // V
        SaoGatherFunctionTableLossy[!!(ASM_TYPES & PREAVX2_MASK)](
            &(inputPicturePtr->bufferCr[(((inputPicturePtr->originY + tbOriginY) * inputPicturePtr->strideCr) >> subHeightCMinus1) + ((inputPicturePtr->originX + tbOriginX) >> subWidthCMinus1)]),
            inputPicturePtr->strideCr,
            &(reconPicturePtr->bufferCr[(((reconPicturePtr->originY + tbOriginY) * reconPicturePtr->strideCr) >> subHeightCMinus1) + ((reconPicturePtr->originX + tbOriginX) >> subHeightCMinus1)]),
            reconPicturePtr->strideCr,
            lcuWidth >> subWidthCMinus1,
            lcuHeight >> subHeightCMinus1,
            saoStats->boDiff[2],
            saoStats->boCount[2],
            saoStats->eoDiff[2],
            saoStats->eoCount[2]);
        // Y
        DetermineSaoLumaModeOffsets(
            saoStats,
            saoPtr,
            fullLambda,
            mdRateEstimationPtr,
            saoLumaBestCost,
            EB_FALSE);

        // U + V
        DetermineSaoChromaModeOffsets(
            saoStats,
            saoPtr,
            fullChromaLambdaSao,
            mdRateEstimationPtr,
            saoChromaBestCost,
            EB_FALSE);

        // Check Merge Mode Y + U + V    
        TestSaoCopyModes(
            saoStats->boDiff,
            saoStats->boCount,
            saoStats->eoDiff,
            saoStats->eoCount,
            saoPtr,
            leftSaoPtr,
            upSaoPtr,
            fullLambda,
            mdRateEstimationPtr,
            saoLumaBestCost,
            saoChromaBestCost,
            EB_FALSE);
    }
    else {

        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            // Y
            {
                SaoGatherFunctionTableLossy_90_45_135[!!(ASM_TYPES & PREAVX2_MASK)](
                    &(inputPicturePtr->bufferY[(inputPicturePtr->originY + tbOriginY) * inputPicturePtr->strideY + inputPicturePtr->originX + tbOriginX]),
                    inputPicturePtr->strideY,
                    &(reconPicturePtr->bufferY[(reconPicturePtr->originY + tbOriginY) * reconPicturePtr->strideY + reconPicturePtr->originX + tbOriginX]),
                    reconPicturePtr->strideY,
                    lcuWidth,
                    lcuHeight,
                    saoStats->eoDiff[0],
                    saoStats->eoCount[0]);
            }

        }

        if (pictureControlSetPtr->temporalLayerIndex == 1) {
            // Y
            SaoGatherFunctionTableLossy_90_45_135[!!(ASM_TYPES & PREAVX2_MASK)](
                &(inputPicturePtr->bufferY[(inputPicturePtr->originY + tbOriginY) * inputPicturePtr->strideY + inputPicturePtr->originX + tbOriginX]),
                inputPicturePtr->strideY,
                &(reconPicturePtr->bufferY[(reconPicturePtr->originY + tbOriginY) * reconPicturePtr->strideY + reconPicturePtr->originX + tbOriginX]),
                reconPicturePtr->strideY,
                lcuWidth,
                lcuHeight,
                saoStats->eoDiff[0],
                saoStats->eoCount[0]);

        }

        // Generate SAO Offsets
        //Luma
        if (pictureControlSetPtr->temporalLayerIndex == 0) {

            {
                DetermineSaoLumaModeOffsets_OnlyEo_90_45_135(
                    saoStats,
                    saoPtr,
                    fullLambda,
                    mdRateEstimationPtr,
                    saoLumaBestCost,
                    EB_FALSE);
            }

        }

        if (pictureControlSetPtr->temporalLayerIndex == 1) {
            DetermineSaoLumaModeOffsets_OnlyEo_90_45_135(
                saoStats,
                saoPtr,
                fullLambda,
                mdRateEstimationPtr,
                saoLumaBestCost,
                EB_FALSE);
        }


        //Check Merge Mode (Luma+Chroma)   
        if (pictureControlSetPtr->temporalLayerIndex < 2) {

            TestSaoCopyModes(
                saoStats->boDiff,
                saoStats->boCount,
                saoStats->eoDiff,
                saoStats->eoCount,
                saoPtr,
                leftSaoPtr,
                upSaoPtr,
                fullLambda,
                mdRateEstimationPtr,
                saoLumaBestCost,
                saoChromaBestCost,
                EB_FALSE);
        }
    }

    return return_error;
}

/********************************************
* DetermineSaoLumaModeOffsets
*   generates Sao Luma Offsets
********************************************/
static void DetermineSaoEOLumaModeOffsets(
	SaoStats_t                 *saoStats,
	SaoParameters_t            *saoParams,
	EB_U64                      lambda,                 // input parameter, lambda, used to compute the cost
	MdRateEstimationContext_t  *mdRateEstimationPtr,    // input parameter, MD table bits, used to compute the cost
	EB_S64                     *saoLumaBestCost,        // output parameter, best SAO Luma cost
	EB_BOOL                     is10bit)
{
	EB_U32          eoType, eoIndex, bestEotype = 0, saoOffset;
	EB_U64 maxCost = (EB_U64)~0;

	EB_S32 eoOffset[SAO_EO_TYPES][SAO_EO_CATEGORIES];
	EB_S64 eoCategoryDistortion[SAO_EO_TYPES][SAO_EO_CATEGORIES];
	EB_S64 eoTypeDistortion[SAO_EO_TYPES] = { 0 };
	EB_S64 eoTypeRate[SAO_EO_TYPES] = { 0 };
	EB_S64 eoTypeCost[SAO_EO_TYPES];
	EB_S64 eoBestCost = (maxCost >> 1);

	EB_U64 saoOffsetFlagBitsNum;
	EB_S64 saoOffRate;
	EB_S64 saoOffCost;
	EB_U32 distortionShift = is10bit ? 4 : 0;

	// *Note: in all Cost Computations we are not including the Cost of the Merge Left & Up Costs - they will be added later in TestSaoCopyModes()
	saoOffRate = mdRateEstimationPtr->saoTypeIndexBits[0];
	saoOffCost = ((saoOffRate *lambda) + MD_OFFSET) >> MD_SHIFT;

	// EO Candidates
	for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {

		for (eoIndex = 0; eoIndex < SAO_EO_CATEGORIES; ++eoIndex) {
			eoOffset[eoType][eoIndex] = (saoStats->eoCount[0][eoType][eoIndex] == 0) ?
				0 :
				ROUND(saoStats->eoDiff[0][eoType][eoIndex] / (EB_S32)saoStats->eoCount[0][eoType][eoIndex]);

			eoOffset[eoType][eoIndex] = CLIP3(MinSaoOffsetvalueEO[is10bit][eoIndex], MaxSaoOffsetvalueEO[is10bit][eoIndex], eoOffset[eoType][eoIndex]);

			//eoCategoryDistortion[eoType][eoIndex] = - (2 * eoOffset[eoType][eoIndex] * saoStats->eoDiff[0][eoType][eoIndex]) + ((EB_S32)saoStats->eoCount[0][eoType][eoIndex] * SQR(eoOffset[eoType][eoIndex]));
			eoCategoryDistortion[eoType][eoIndex] = (-(2 * eoOffset[eoType][eoIndex] * saoStats->eoDiff[0][eoType][eoIndex]) + ((EB_S32)saoStats->eoCount[0][eoType][eoIndex] * SQR(eoOffset[eoType][eoIndex]))) >> distortionShift;

			eoTypeDistortion[eoType] += eoCategoryDistortion[eoType][eoIndex];

		}

		GetSaoOffsetsFractionBits(
			eoType + 1,
			&(eoOffset[eoType][0]),
			mdRateEstimationPtr,
			&saoOffsetFlagBitsNum);

		eoTypeRate[eoType] = saoOffsetFlagBitsNum;
		eoTypeCost[eoType] = (eoTypeDistortion[eoType] << COST_PRECISION) + (((eoTypeRate[eoType] * lambda) + MD_OFFSET) >> MD_SHIFT);

		if (eoTypeCost[eoType] < eoBestCost) {

			eoBestCost = eoTypeCost[eoType];
			bestEotype = eoType;
		}
	}

	// Add SAO Type Cost
	eoBestCost += (((mdRateEstimationPtr->saoTypeIndexBits[bestEotype + 1] * lambda) + MD_OFFSET) >> MD_SHIFT);
	if ((eoBestCost < saoOffCost)) {

		*saoLumaBestCost = eoBestCost;
		saoParams->saoTypeIndex[SAO_COMPONENT_LUMA] = bestEotype + 1;
		for (saoOffset = 0; saoOffset < NUMBER_SAO_OFFSETS; ++saoOffset) {
			saoParams->saoOffset[SAO_COMPONENT_LUMA][saoOffset] = eoOffset[bestEotype][saoOffset];
		}
	}
	else {

		saoParams->saoTypeIndex[SAO_COMPONENT_LUMA] = 0;
		*saoLumaBestCost = saoOffCost;
	}

	return;
}

/********************************************
 * SaoGenerationDecision16bit
 *   generates SAO offsets and chooses best SAO mode
 ********************************************/
EB_ERRORTYPE SaoGenerationDecision16bit(
    EbPictureBufferDesc_t      *inputLcuPtr,
    SaoStats_t                 *saoStats,
    SaoParameters_t            *saoParams,
    MdRateEstimationContext_t  *mdRateEstimationPtr,
    EB_U64                      fullLambda,

	EB_U64                      fullChromaLambdaSao,
	EB_BOOL                     mmSao,

    PictureControlSet_t        *pictureControlSetPtr,
    EB_U32                      tbOriginX,
    EB_U32                      tbOriginY,
    EB_U32                      lcuWidth,
    EB_U32                      lcuHeight,
    SaoParameters_t            *saoPtr,
    SaoParameters_t            *leftSaoPtr,
    SaoParameters_t            *upSaoPtr,
    EB_S64                     *saoLumaBestCost,
    EB_S64                     *saoChromaBestCost)
{
    EB_ERRORTYPE             return_error = EB_ErrorNone;
     EbPictureBufferDesc_t   *recon16;
     if(pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
        recon16 = ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->referencePicture16bit;
     else
		recon16 = pictureControlSetPtr->reconPicture16bitPtr;

    const EB_COLOR_FORMAT colorFormat = recon16->colorFormat;
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
        
    // Intialise SAO  Parameters         
    saoParams->saoTypeIndex[SAO_COMPONENT_LUMA] = 0;
    saoParams->saoTypeIndex[SAO_COMPONENT_CHROMA] = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][0] = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][1] = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][2] = 0;
    saoParams->saoOffset[SAO_COMPONENT_LUMA][3] = 0;
    saoParams->saoBandPosition[SAO_COMPONENT_LUMA] = 0;
    saoParams->saoMergeLeftFlag = EB_FALSE;
    saoParams->saoMergeUpFlag = EB_FALSE;
    *saoLumaBestCost = 0;
    *saoChromaBestCost = 0;

	if (mmSao) {

		EB_U32       lcuChromaWidth = lcuWidth >> subWidthCMinus1;
		EB_U32       lcuChromaHeight = lcuHeight >> subHeightCMinus1;

		// Y
		// Requirement: lcuWidth = 28, 56 or lcuWidth % 16 = 0
		// Requirement: lcuHeight > 2

		// This function is only written in C. To be implemented in ASM
		SaoGatherFunctionTabl_16bit[((lcuWidth & 15) == 0) || (lcuWidth == 28) || (lcuWidth == 56)](
			(EB_U16*)inputLcuPtr->bufferY,
			inputLcuPtr->strideY,
			(EB_U16*)(recon16->bufferY) + (recon16->originY + tbOriginY)*recon16->strideY + (recon16->originX + tbOriginX),
			recon16->strideY,
			lcuWidth,
			lcuHeight,
			saoStats->boDiff[0],
			saoStats->boCount[0],
			saoStats->eoDiff[0],
			saoStats->eoCount[0]);

		// U
		SaoGatherFunctionTabl_16bit[((lcuChromaWidth & 15) == 0) || (lcuChromaWidth == 28) || (lcuChromaWidth == 56)](
			(EB_U16*)inputLcuPtr->bufferCb,
			inputLcuPtr->strideCb,
			(EB_U16*)(recon16->bufferCb) + ((((recon16->originY + tbOriginY) * recon16->strideCb) >> subHeightCMinus1) + ((recon16->originX + tbOriginX) >> subWidthCMinus1)),
			recon16->strideCb,
			lcuChromaWidth,
			lcuChromaHeight,
			saoStats->boDiff[1],
			saoStats->boCount[1],
			saoStats->eoDiff[1],
			saoStats->eoCount[1]);

		// V
		SaoGatherFunctionTabl_16bit[((lcuChromaWidth & 15) == 0) || (lcuChromaWidth == 28) || (lcuChromaWidth == 56)](
			(EB_U16*)inputLcuPtr->bufferCr,
			inputLcuPtr->strideCr,
			(EB_U16*)(recon16->bufferCr) + ((((recon16->originY + tbOriginY) * recon16->strideCb) >> subHeightCMinus1) + ((recon16->originX + tbOriginX) >> subWidthCMinus1)),
			recon16->strideCr,
			lcuChromaWidth,
			lcuChromaHeight,
			saoStats->boDiff[2],
			saoStats->boCount[2],
			saoStats->eoDiff[2],
			saoStats->eoCount[2]);

		// Y
		DetermineSaoEOLumaModeOffsets(
			saoStats,
			saoPtr,
			fullLambda,
			mdRateEstimationPtr,
			saoLumaBestCost,
			EB_TRUE);

		//Chroma U+V
		DetermineSaoChromaModeOffsets(
			saoStats,
			saoPtr,
			fullChromaLambdaSao,
			mdRateEstimationPtr,
			saoChromaBestCost,
			EB_TRUE);

		TestSaoCopyModes(
			saoStats->boDiff,
			saoStats->boCount,
			saoStats->eoDiff,
			saoStats->eoCount,
			saoPtr,
			leftSaoPtr,
			upSaoPtr,
			fullLambda,
			mdRateEstimationPtr,
			saoLumaBestCost,
			saoChromaBestCost,
			EB_TRUE);

	}
	else {
		// Y
		if (pictureControlSetPtr->temporalLayerIndex == 0) {
			//; Requirement: lcuWidth = 28, 56 or lcuWidth % 16 = 0
			//; Requirement: lcuHeight > 2

			{
                SaoGatherFunctionTable_90_45_135_16bit_SSE2[!!(ASM_TYPES & PREAVX2_MASK)][((lcuWidth & 15) == 0) || (lcuWidth == 28) || (lcuWidth == 56)](
                    (EB_U16*)inputLcuPtr->bufferY,
                    inputLcuPtr->strideY,
                    (EB_U16*)(recon16->bufferY) + (recon16->originY + tbOriginY)*recon16->strideY + (recon16->originX + tbOriginX),
                    recon16->strideY,
                    lcuWidth,
                    lcuHeight,
                    saoStats->eoDiff[0],
                    saoStats->eoCount[0]);
			}
		}

		if (pictureControlSetPtr->temporalLayerIndex == 1) {
            SaoGatherFunctionTable_90_45_135_16bit_SSE2[!!(ASM_TYPES & PREAVX2_MASK)][((lcuWidth & 15) == 0) || (lcuWidth == 28) || (lcuWidth == 56)](
                (EB_U16*)inputLcuPtr->bufferY,
                inputLcuPtr->strideY,
                (EB_U16*)(recon16->bufferY) + (recon16->originY + tbOriginY)*recon16->strideY + (recon16->originX + tbOriginX),
                recon16->strideY,
                lcuWidth,
                lcuHeight,
                saoStats->eoDiff[0],
                saoStats->eoCount[0]);
		}

		// Generate SAO Offsets
		//Luma
		if (pictureControlSetPtr->temporalLayerIndex == 0) {

			{
				DetermineSaoLumaModeOffsets_OnlyEo_90_45_135(
					saoStats,
					saoPtr,
					fullLambda,
					mdRateEstimationPtr,
					saoLumaBestCost,
					EB_TRUE);

			}
		}

		if (pictureControlSetPtr->temporalLayerIndex == 1) {

			DetermineSaoLumaModeOffsets_OnlyEo_90_45_135(
				saoStats,
				saoPtr,
				fullLambda,
				mdRateEstimationPtr,
				saoLumaBestCost,
				EB_TRUE);
		}

		//Check Merge Mode (Luma+Chroma)   
		if (pictureControlSetPtr->temporalLayerIndex < 2) {

			TestSaoCopyModes(
				saoStats->boDiff,
				saoStats->boCount,
				saoStats->eoDiff,
				saoStats->eoCount,
				saoPtr,
				leftSaoPtr,
				upSaoPtr,
				fullLambda,
				mdRateEstimationPtr,
				saoLumaBestCost,
				saoChromaBestCost,
				EB_TRUE);
		}
	}

    return return_error;
}