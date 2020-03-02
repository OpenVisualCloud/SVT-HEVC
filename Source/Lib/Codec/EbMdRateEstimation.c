/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbUtility.h"
#include "EbMdRateEstimation.h"
#include "EbDefinitions.h"

/*********************************************************************
 * ExponentialGolombBits
 *   Gives the number of bits neede for Exponential Golomb Code of a symbol
 *
 *  symbol
 *   input to be coded using exponential golomb code
 *
 *  count
 *   input count for exponential golomb code
 *
 *  bitNum
 *   output for the number of bits needed for exponential golomb code
 *********************************************************************/
static inline EB_ERRORTYPE ExponentialGolombBits(
    EB_U32  symbol,
    EB_U32  count,
    EB_U32 *bitNum)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    *bitNum = 0;

    while( symbol >= (EB_U32)(1<<count) ) {
        (*bitNum)++;
        symbol -= 1 << count;
        count++;
    }
    (*bitNum)++;
    (*bitNum) += count;
    return return_error;
}
/*********************************************************************
 * GetRefIndexFractionBits
 *   Gets the reference index Fraction Bits
 *
 *  refIndex
 *   reference index
 *
 *  mdRateEstimationArray
 *    MD Rate Estimation array
 *
 *  fractionBitNum
 *   output for the fraction number of bits needed for reference index
 *********************************************************************/
EB_ERRORTYPE GetRefIndexFractionBits(
    MdRateEstimationContext_t  *mdRateEstimationArray,
    EB_U64                     *fractionBitNum)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    *fractionBitNum = 0;
    *fractionBitNum += mdRateEstimationArray->refPicBits[0];

    return return_error;
}
/*********************************************************************
 * EbHevcGetMvdFractionBits
 *   Gets the motion vector difference Fraction Bits
 *
 *  mvdX
 *   MV X difference
 *
 *  mvdY
 *   MV Y difference
 *
 *  mdRateEstimationArray
 *    MD Rate Estimation array
 *
 *  fractionBitNum
 *   output for the fraction number of bits needed for motion vector difference
 *********************************************************************/
EB_ERRORTYPE EbHevcGetMvdFractionBits(
    EB_S32                      mvdX,
    EB_S32                      mvdY,
    MdRateEstimationContext_t  *mdRateEstimationArray,
    EB_U64                     *fractionBitNum)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    EB_U32 bypassBitNum;

    EB_U32 absMvdX    = ABS(mvdX);
    EB_U32 absMvdY    = ABS(mvdY);

    EB_U32 mvdXneq0   = (mvdX !=0);
    EB_U32 mvdYneq0   = (mvdY !=0);

    EB_U32 absmvdXgt1 = (absMvdX > 1);
    EB_U32 absmvdYgt1 = (absMvdY > 1);

    *fractionBitNum = 0;

    *fractionBitNum += mdRateEstimationArray->mvdBits[mvdXneq0];

    *fractionBitNum += mdRateEstimationArray->mvdBits[mvdYneq0+(2<<mvdXneq0)];

    if(mvdXneq0) {
        *fractionBitNum += mdRateEstimationArray->mvdBits[absmvdXgt1+6];
    }

    if(mvdYneq0) {
        *fractionBitNum += mdRateEstimationArray->mvdBits[absmvdYgt1+6+(2<<absmvdXgt1)];
    }

    if(mvdXneq0) {
        switch(absmvdXgt1) {
        case 1:
            ExponentialGolombBits(
                (EB_S32)absMvdX - 2,
                1,
                &bypassBitNum);

            *fractionBitNum += bypassBitNum * 32768;

        case 0:
            // Sign of mvdX
            *fractionBitNum += 32768;

        default:
            break;
        }
    }

    if(mvdYneq0) {
        switch(absmvdYgt1) {
        case 1:
            ExponentialGolombBits(
                (EB_S32)absMvdY - 2,
                1,
                &bypassBitNum);

            *fractionBitNum += bypassBitNum * 32768;

        case 0:
            // Sign of mvdY
            *fractionBitNum += 32768;

        default:
            break;
        }
    }

    return return_error;
}
/*********************************************************************
 * MeEbHevcGetMvdFractionBits
 *   Gets the motion vector difference Fraction Bits for ME
 *
 *  mvdX
 *   MV X difference
 *
 *  mvdY
 *   MV Y difference
 *
 *  mvdBitsPtr
 *    MVD bits Pointer
 *
 *  fractionBitNum
 *   output for the fraction number of bits needed for motion vector difference
 *********************************************************************/
EB_ERRORTYPE MeEbHevcGetMvdFractionBits(
    EB_S32                      mvdX,
    EB_S32                      mvdY,
    EB_BitFraction             *mvdBitsPtr,
    EB_U32                     *fractionBitNum)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    EB_U32 bypassBitNum;

    EB_U32 absMvdX    = ABS(mvdX);
    EB_U32 absMvdY    = ABS(mvdY);

    EB_U32 mvdXneq0   = (mvdX !=0);
    EB_U32 mvdYneq0   = (mvdY !=0);

    EB_U32 absmvdXgt1 = (absMvdX > 1);
    EB_U32 absmvdYgt1 = (absMvdY > 1);

    *fractionBitNum = 0;

    *fractionBitNum += mvdBitsPtr[mvdXneq0];

    *fractionBitNum += mvdBitsPtr[mvdYneq0+(2<<mvdXneq0)];

    if(mvdXneq0) {
        *fractionBitNum += mvdBitsPtr[absmvdXgt1+6];
    }

    if(mvdYneq0) {
        *fractionBitNum += mvdBitsPtr[absmvdYgt1+6+(2<<absmvdXgt1)];
    }

    if(mvdXneq0) {
        switch(absmvdXgt1) {
        case 1:
            ExponentialGolombBits(
                (EB_S32)absMvdX - 2,
                1,
                &bypassBitNum);

            *fractionBitNum += bypassBitNum * 32768;

        case 0:
            // Sign of mvdX
            *fractionBitNum += 32768;

        default:
            break;
        }
    }

    if(mvdYneq0) {
        switch(absmvdYgt1) {
        case 1:
            ExponentialGolombBits(
                (EB_S32)absMvdY - 2,
                1,
                &bypassBitNum);

            *fractionBitNum += bypassBitNum * 32768;

        case 0:
            // Sign of mvdY
            *fractionBitNum += 32768;

        default:
            break;
        }
    }

    return return_error;
}

/********************************************
 * GetSaoOffsetsFractionBits
 *   fets the SAO Luma Offset Fraction Bits
 ********************************************/
EB_ERRORTYPE GetSaoOffsetsFractionBits(
    EB_U32                      saoType,               // input parameter, SAO Type
    EB_S32                     *offset,                // input parameter, offset pointer
    MdRateEstimationContext_t  *mdRateEstimationArray, // input parameter, MD Rate Estimation array
    EB_U64                     *fractionBitNum)        // output parameter,  fraction number of bits
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    *fractionBitNum = 0;

    if(saoType != 0) {
        EB_U32 offset0 = MIN(NUMBER_OF_SAO_OFFSET_TRUNUNARY_CASES - 1, ABS(offset[0]));
        EB_U32 offset1 = MIN(NUMBER_OF_SAO_OFFSET_TRUNUNARY_CASES - 1, ABS(offset[1]));
        EB_U32 offset2 = MIN(NUMBER_OF_SAO_OFFSET_TRUNUNARY_CASES - 1, ABS(offset[2]));
        EB_U32 offset3 = MIN(NUMBER_OF_SAO_OFFSET_TRUNUNARY_CASES - 1, ABS(offset[3]));

        // Band Offset
        if(saoType == 5) { // Add macros

            // code band position
            *fractionBitNum += 163840; //32768*5

            // code abs value of offsets
            *fractionBitNum += mdRateEstimationArray->saoOffsetTrunUnaryBits[offset0];
            *fractionBitNum += mdRateEstimationArray->saoOffsetTrunUnaryBits[offset1];
            *fractionBitNum += mdRateEstimationArray->saoOffsetTrunUnaryBits[offset2];
            *fractionBitNum += mdRateEstimationArray->saoOffsetTrunUnaryBits[offset3];

            // code the sign of offsets
            *fractionBitNum += (offset0 != 0) ? 32768: 0;
            *fractionBitNum += (offset1 != 0) ? 32768: 0;
            *fractionBitNum += (offset2 != 0) ? 32768: 0;
            *fractionBitNum += (offset3 != 0) ? 32768: 0;
        }
        // Edge Offset
        else {
            // code the offset values
            *fractionBitNum += mdRateEstimationArray->saoOffsetTrunUnaryBits[offset0];
            *fractionBitNum += mdRateEstimationArray->saoOffsetTrunUnaryBits[offset1];
            *fractionBitNum += mdRateEstimationArray->saoOffsetTrunUnaryBits[offset2];
            *fractionBitNum += mdRateEstimationArray->saoOffsetTrunUnaryBits[offset3];
        }
    }
    return return_error;
}

EB_ERRORTYPE MdRateEstimationContextInit(MdRateEstimationContext_t *mdRateEstimationArray,
                                 ContextModelEncContext_t  *cabacContextModelArray)
{
    EB_U32                      caseIndex1;
    EB_U32                      caseIndex2;
    EB_U32                      sliceIndex;
    EB_U32                      qpIndex;
    MdRateEstimationContext_t  *mdRateEstimationTemp;

    EB_U32                      cabacContextModelArrayOffset1=0;
    EB_U32                      cabacContextModelArrayOffset2;
    ContextModelEncContext_t   *cabacContextModelPtr;
    EB_ContextModel             contextModel;

    // Loop over all slice types
    for(sliceIndex = 0; sliceIndex < TOTAL_NUMBER_OF_SLICE_TYPES; sliceIndex++) {

        cabacContextModelArrayOffset1 = sliceIndex * TOTAL_NUMBER_OF_QP_VALUES;
        // Loop over all Qps
        for(qpIndex = 0; qpIndex < TOTAL_NUMBER_OF_QP_VALUES; qpIndex++) {

            cabacContextModelArrayOffset2 = qpIndex ;
            cabacContextModelPtr          = (&cabacContextModelArray[cabacContextModelArrayOffset1+cabacContextModelArrayOffset2]);
            mdRateEstimationTemp          = (&mdRateEstimationArray[cabacContextModelArrayOffset1+cabacContextModelArrayOffset2]);

            // Split Flag Bits Table
            // 0: Symbol = 0 & CtxSplit = 0
            // 1: Symbol = 0 & CtxSplit = 1
            // 2: Symbol = 0 & CtxSplit = 2
            // 3: Symbol = 1 & CtxSplit = 0
            // 4: Symbol = 1 & CtxSplit = 1
            // 5: Symbol = 1 & CtxSplit = 2
            for (caseIndex1 = 0; caseIndex1<2 ; caseIndex1++) {
                for (caseIndex2 = 0; caseIndex2<(NUMBER_OF_SPLIT_FLAG_CASES>>1) ; caseIndex2++) {
                    mdRateEstimationTemp->splitFlagBits[(NUMBER_OF_SPLIT_FLAG_CASES>>1)*caseIndex1+caseIndex2] = CabacEstimatedBits[cabacContextModelPtr->splitFlagContextModel[caseIndex2] ^ caseIndex1];
                }
            }

            // Skip Flag Bits Table not for I SLICE
            for (caseIndex1 = 0; caseIndex1<2 ; caseIndex1++) {
                for (caseIndex2 = 0; caseIndex2<(NUMBER_OF_SKIP_FLAG_CASES>>1) ; caseIndex2++) {
                    mdRateEstimationTemp->skipFlagBits[(NUMBER_OF_SKIP_FLAG_CASES>>1)*caseIndex1+caseIndex2] = CabacEstimatedBits[cabacContextModelPtr->skipFlagContextModel[caseIndex2] ^ caseIndex1];
                }
            }

            // Transform Subdivision Flag Bits Table
            // there are 10 context but why only 5(4) of them are used?
            for (caseIndex1 = 0; caseIndex1<2 ; caseIndex1++) {
                for (caseIndex2 = 0; caseIndex2<(NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES>>1) ; caseIndex2++) {
                    mdRateEstimationTemp->transSubDivFlagBits[(NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES>>1)*caseIndex1+caseIndex2] = 0;
                }
            }

            // Pred Mode Bits Table
            // 0 for Inter
            // 1 for Intra
            mdRateEstimationTemp->predModeBits[0] = CabacEstimatedBits[cabacContextModelPtr->predModeContextModel[0] ^ 0];
            mdRateEstimationTemp->predModeBits[1] = CabacEstimatedBits[cabacContextModelPtr->predModeContextModel[0] ^ 1];

            //NUMBER_OF_INTRA_PART_SIZE_CASES
            //2Nx2N is index 0
            mdRateEstimationTemp->intraPartSizeBits[SIZE_2Nx2N] = CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[0] ^ 1];
            //NxN is index 1
            mdRateEstimationTemp->intraPartSizeBits[1] = CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[0] ^ 0];

            //NUMBER_OF_INTER_PART_SIZE_CASES
            //2Nx2N is index 0
            mdRateEstimationTemp->interPartSizeBits[SIZE_2Nx2N]  = CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[0] ^ 1];
            //2NxN
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxN]   = CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[0] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxN]  += CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[1] ^ 1];
            //**Note** a condition on AMP enabling shall be set for the following line
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxN]  += CabacEstimatedBits[cabacContextModelPtr->cuAmpContextModel[0] ^ 1];
            //Nx2N
            mdRateEstimationTemp->interPartSizeBits[SIZE_Nx2N]   = CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[0] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_Nx2N]  += CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[1] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_Nx2N]  += CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[2] ^ 1];
            //**Note** a condition on AMP enabling shall be set for the following line
            mdRateEstimationTemp->interPartSizeBits[SIZE_Nx2N]  += CabacEstimatedBits[cabacContextModelPtr->cuAmpContextModel[0] ^ 1];
            //NxN
            mdRateEstimationTemp->interPartSizeBits[SIZE_NxN]  = 0;
            // 2NxnU
            //**Note** a condition on AMP enabling shall be set for these lines
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxnU]   = CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[0] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxnU]  += CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[1] ^ 1];
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxnU]  += CabacEstimatedBits[cabacContextModelPtr->cuAmpContextModel[0] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxnU]  += 32768;
            // 2NxnD
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxnD]   = CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[0] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxnD]  += CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[1] ^ 1];
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxnD]  += CabacEstimatedBits[cabacContextModelPtr->cuAmpContextModel[0] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_2NxnD]  += 32768;
            // nLx2N
            mdRateEstimationTemp->interPartSizeBits[SIZE_nLx2N]   = CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[0] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_nLx2N]  += CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[1] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_nLx2N]  += CabacEstimatedBits[cabacContextModelPtr->cuAmpContextModel[0] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_nLx2N]  += 32768;
            // nRx2N
            mdRateEstimationTemp->interPartSizeBits[SIZE_nRx2N]   = CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[0] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_nRx2N]  += CabacEstimatedBits[cabacContextModelPtr->partSizeContextModel[1] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_nRx2N]  += CabacEstimatedBits[cabacContextModelPtr->cuAmpContextModel[0] ^ 0];
            mdRateEstimationTemp->interPartSizeBits[SIZE_nRx2N]  += 32768;

            //NUMBER_OF_REF_PIC_CASES not for I SLICE

            mdRateEstimationTemp->refPicBits[0]  = CabacEstimatedBits[cabacContextModelPtr->refPicContextModel[0] ^ 0];

            mdRateEstimationTemp->refPicBits[1]  = CabacEstimatedBits[cabacContextModelPtr->refPicContextModel[0] ^ 1];
            mdRateEstimationTemp->refPicBits[1] += CabacEstimatedBits[cabacContextModelPtr->refPicContextModel[1] ^ 0];

            mdRateEstimationTemp->refPicBits[2]  = CabacEstimatedBits[cabacContextModelPtr->refPicContextModel[0] ^ 1];
            mdRateEstimationTemp->refPicBits[2] += CabacEstimatedBits[cabacContextModelPtr->refPicContextModel[1] ^ 1];

            // AMVP Index Bits Table
            // assumption is only P pictures and refPicList0 is used
            for (caseIndex1 = 0; caseIndex1<NUMBER_OF_MVP_INDEX_CASES ; caseIndex1++) {
                mdRateEstimationTemp->mvpIndexBits[caseIndex1] = CabacEstimatedBits[cabacContextModelPtr->mvpIndexContextModel[0] ^ caseIndex1];
            }

            // NUMBER_OF_ROOT_CBF_CASES
            for (caseIndex1 = 0;  caseIndex1<NUMBER_OF_ROOT_CBF_CASES ; caseIndex1++) {
                mdRateEstimationTemp->rootCbfBits[caseIndex1] = CabacEstimatedBits[cabacContextModelPtr->rootCbfContextModel[0] ^ caseIndex1];
                //  mdRateEstimationTemp->rootCbfBits[caseIndex1] = 0;
            }

            // Luma, cb and cr CBF there are 5 context but why Nader is using only three of them
            for (caseIndex1 = 0; caseIndex1<2 ; caseIndex1++) {
                for (caseIndex2 = 0; caseIndex2<(NUMBER_OF_CBF_CASES>>1) ; caseIndex2++) {
                    mdRateEstimationTemp->lumaCbfBits[(NUMBER_OF_CBF_CASES>>1)*caseIndex1+caseIndex2]    = CabacEstimatedBits[cabacContextModelPtr->cbfContextModel[caseIndex2] ^ caseIndex1];
                    mdRateEstimationTemp->chromaCbfBits[(NUMBER_OF_CBF_CASES>>1)*caseIndex1+caseIndex2]  = CabacEstimatedBits[cabacContextModelPtr->cbfContextModel[caseIndex2+NUMBER_OF_CBF_CONTEXT_MODELS] ^ caseIndex1];
                }
            }
            // Intra LUMA Mode Bits Table
            // Col 0 : Same as Neighbor index 0
            // Col 1 : Same as Neighbor index 1
            // Col 2 : Same as Neighbor index 2
            // Col 3 : Not equal to any of the neighbors
            mdRateEstimationTemp->intraLumaBits[0]   = CabacEstimatedBits[cabacContextModelPtr->intraLumaContextModel[0] ^ 1];
            mdRateEstimationTemp->intraLumaBits[0]  += 32768;

            mdRateEstimationTemp->intraLumaBits[1]   = CabacEstimatedBits[cabacContextModelPtr->intraLumaContextModel[0] ^ 1];
            mdRateEstimationTemp->intraLumaBits[1]  += 65536;

            mdRateEstimationTemp->intraLumaBits[2]   = CabacEstimatedBits[cabacContextModelPtr->intraLumaContextModel[0] ^ 1];
            mdRateEstimationTemp->intraLumaBits[2]  += 65536;

            mdRateEstimationTemp->intraLumaBits[3]   = CabacEstimatedBits[cabacContextModelPtr->intraLumaContextModel[0] ^ 0];
            mdRateEstimationTemp->intraLumaBits[3]  += 163840;

            // Intra Chroma Mode Bits Table
            // Col 0 : Planar
            // Col 1 : Ver
            // Col 2 : Hor
            // Col 3 : DC
            // Col 4 : DM
            mdRateEstimationTemp->intraChromaBits[4]  = CabacEstimatedBits[cabacContextModelPtr->intraChromaContextModel[0] ^ 0];
            // Col 3 : DC
            mdRateEstimationTemp->intraChromaBits[3]  = CabacEstimatedBits[cabacContextModelPtr->intraChromaContextModel[0] ^ 1];
            mdRateEstimationTemp->intraChromaBits[3] += 65536;
            // Col 2 : Hor
            mdRateEstimationTemp->intraChromaBits[2]  = CabacEstimatedBits[cabacContextModelPtr->intraChromaContextModel[0] ^ 1];
            mdRateEstimationTemp->intraChromaBits[2] += 65536;
            // Col 1 : Ver
            mdRateEstimationTemp->intraChromaBits[1]  = CabacEstimatedBits[cabacContextModelPtr->intraChromaContextModel[0] ^ 1];
            mdRateEstimationTemp->intraChromaBits[1] += 65536;
            // Col 0 : Planar
            mdRateEstimationTemp->intraChromaBits[0]  = CabacEstimatedBits[cabacContextModelPtr->intraChromaContextModel[0] ^ 1];
            mdRateEstimationTemp->intraChromaBits[0] += 65536;


            // Mvd Bits Table
            // 0: first bit fraction for X and mvdXneq0=0
            // 1: first bit fraction for X and mvdXneq0=1
            // 2: first bit fraction for Y and mvdYneq0=0 and mvdXneq0=0
            // 3: first bit fraction for Y and mvdYneq0=1 and mvdXneq0=0
            // 4: first bit fraction for Y and mvdYneq0=0 and mvdXneq0=1
            // 5: first bit fraction for Y and mvdYneq0=1 and mvdXneq0=1
            // 6: second bit fraction for X and mvdXneq0=1 and absmvdXgt1=0
            // 7: second bit fraction for X and mvdXneq0=1 and absmvdXgt1=1
            // 8: second bit fraction for Y and mvdYneq0=1 and absmvdYgt1=0 and mvdXneq0=1 and absmvdXgt1=0
            // 9: second bit fraction for Y and mvdYneq0=1 and absmvdYgt1=1 and mvdXneq0=1 and absmvdXgt1=0
            //10: second bit fraction for Y and mvdYneq0=1 and absmvdYgt1=0 and mvdXneq0=1 and absmvdXgt1=1
            //11: second bit fraction for Y and mvdYneq0=1 and absmvdYgt1=1 and mvdXneq0=1 and absmvdXgt1=1

            mdRateEstimationTemp->mvdBits[0]  = CabacEstimatedBits[cabacContextModelPtr->mvdContextModel[0] ^ 0];
            mdRateEstimationTemp->mvdBits[1]  = CabacEstimatedBits[cabacContextModelPtr->mvdContextModel[0] ^ 1];

            contextModel                      = cabacContextModelPtr->mvdContextModel[0];
            contextModel                      = UPDATE_CONTEXT_MODEL(0, &contextModel);
            mdRateEstimationTemp->mvdBits[2]  = CabacEstimatedBits[contextModel ^ 0];
            mdRateEstimationTemp->mvdBits[3]  = CabacEstimatedBits[contextModel ^ 1];

            contextModel                      = cabacContextModelPtr->mvdContextModel[0];
            contextModel                      = UPDATE_CONTEXT_MODEL(1, &contextModel);
            mdRateEstimationTemp->mvdBits[4]  = CabacEstimatedBits[contextModel ^ 0];
            mdRateEstimationTemp->mvdBits[5]  = CabacEstimatedBits[contextModel ^ 1];

            contextModel                      = cabacContextModelPtr->mvdContextModel[1];
            mdRateEstimationTemp->mvdBits[6]  = CabacEstimatedBits[contextModel ^ 0];
            mdRateEstimationTemp->mvdBits[7]  = CabacEstimatedBits[contextModel ^ 1];

            contextModel                      = cabacContextModelPtr->mvdContextModel[1];
            contextModel                      = UPDATE_CONTEXT_MODEL(0, &contextModel);
            mdRateEstimationTemp->mvdBits[8]  = CabacEstimatedBits[contextModel ^ 0];
            mdRateEstimationTemp->mvdBits[9]  = CabacEstimatedBits[contextModel ^ 1];

            contextModel                      = cabacContextModelPtr->mvdContextModel[1];
            contextModel                      = UPDATE_CONTEXT_MODEL(1, &contextModel);
            mdRateEstimationTemp->mvdBits[10] = CabacEstimatedBits[contextModel ^ 0];
            mdRateEstimationTemp->mvdBits[11] = CabacEstimatedBits[contextModel ^ 1];

            // Merge Flag Bits Table
            // 0: Symbol = 0 & Ctx = 0
            // 1: Symbol = 0 & Ctx = 0
            mdRateEstimationTemp->mergeFlagBits[0] = CabacEstimatedBits[cabacContextModelPtr->mergeFlagContextModel[0] ^ 0];
            mdRateEstimationTemp->mergeFlagBits[1] = CabacEstimatedBits[cabacContextModelPtr->mergeFlagContextModel[0] ^ 1];

            // Merge Flag Bits Table
            // 0: Merge index = 0
            // 1: Merge index = 1
            // 2: Merge index = 2
            // 3: Merge index = 3
            // 4: Merge index = 4
            mdRateEstimationTemp->mergeIndexBits[0]  = CabacEstimatedBits[cabacContextModelPtr->mergeIndexContextModel[0] ^ 0];

            mdRateEstimationTemp->mergeIndexBits[1]  = CabacEstimatedBits[cabacContextModelPtr->mergeIndexContextModel[0] ^ 1];
            mdRateEstimationTemp->mergeIndexBits[1] += 32768;

            mdRateEstimationTemp->mergeIndexBits[2]  = CabacEstimatedBits[cabacContextModelPtr->mergeIndexContextModel[0] ^ 1];
            mdRateEstimationTemp->mergeIndexBits[2] += 65536;

            mdRateEstimationTemp->mergeIndexBits[3]  = CabacEstimatedBits[cabacContextModelPtr->mergeIndexContextModel[0] ^ 1];
            mdRateEstimationTemp->mergeIndexBits[3] += 98304;

            mdRateEstimationTemp->mergeIndexBits[4]  = CabacEstimatedBits[cabacContextModelPtr->mergeIndexContextModel[0] ^ 1];
            mdRateEstimationTemp->mergeIndexBits[4] += 98304;

            // SAO Merge Flag
            mdRateEstimationTemp->saoMergeFlagBits[0] = CabacEstimatedBits[cabacContextModelPtr->saoMergeFlagContextModel[0] ^ 0];
            mdRateEstimationTemp->saoMergeFlagBits[1] = CabacEstimatedBits[cabacContextModelPtr->saoMergeFlagContextModel[0] ^ 1];


            // Sao Type Index
            // index in the table is the type index
            // 0 is for mode 0
            // 1 2 3 4 are for EO
            // 5 is for rest of the BO mode

            mdRateEstimationTemp->saoTypeIndexBits[0]  = CabacEstimatedBits[cabacContextModelPtr->saoTypeIndexContextModel[0] ^ 0];

            mdRateEstimationTemp->saoTypeIndexBits[1]  = CabacEstimatedBits[cabacContextModelPtr->saoTypeIndexContextModel[0] ^ 1];
            mdRateEstimationTemp->saoTypeIndexBits[1] += 98304;

            mdRateEstimationTemp->saoTypeIndexBits[2]  = CabacEstimatedBits[cabacContextModelPtr->saoTypeIndexContextModel[0] ^ 1];
            mdRateEstimationTemp->saoTypeIndexBits[2] += 98304;

            mdRateEstimationTemp->saoTypeIndexBits[3]  = CabacEstimatedBits[cabacContextModelPtr->saoTypeIndexContextModel[0] ^ 1];
            mdRateEstimationTemp->saoTypeIndexBits[3] += 98304;

            mdRateEstimationTemp->saoTypeIndexBits[4]  = CabacEstimatedBits[cabacContextModelPtr->saoTypeIndexContextModel[0] ^ 1];
            mdRateEstimationTemp->saoTypeIndexBits[4] += 98304;

            mdRateEstimationTemp->saoTypeIndexBits[5]  = CabacEstimatedBits[cabacContextModelPtr->saoTypeIndexContextModel[0] ^ 1];
            mdRateEstimationTemp->saoTypeIndexBits[5] += 32768;


            // Sao Offset Trun Unary Bits
            // index in the table is the offset passed to EncodeOffsetTrunUnary
            // It is assumed that Max Value is always 7
            mdRateEstimationTemp->saoOffsetTrunUnaryBits[0]  = 32768;

            mdRateEstimationTemp->saoOffsetTrunUnaryBits[1]  = 65536;

            mdRateEstimationTemp->saoOffsetTrunUnaryBits[2]  = 98304;

            mdRateEstimationTemp->saoOffsetTrunUnaryBits[3]  = 131072;

            mdRateEstimationTemp->saoOffsetTrunUnaryBits[4]  = 163840;

            mdRateEstimationTemp->saoOffsetTrunUnaryBits[5]  = 196608;

            mdRateEstimationTemp->saoOffsetTrunUnaryBits[6]  = 229376;

            mdRateEstimationTemp->saoOffsetTrunUnaryBits[7]  = 229376;

            // Inter Prediction Direction
            //CabacEstimatedBits[cabacContextModelPtr->mergeIndexContextModel[0] ^ 0];
            mdRateEstimationTemp->interBiDirBits[0] = CabacEstimatedBits[cabacContextModelPtr->interDirContextModel[0] ^ 0];      // depth 0, non-bi pred
            mdRateEstimationTemp->interBiDirBits[1] = CabacEstimatedBits[cabacContextModelPtr->interDirContextModel[0] ^ 1];      // depth 0, bi pred
            mdRateEstimationTemp->interBiDirBits[2] = CabacEstimatedBits[cabacContextModelPtr->interDirContextModel[1] ^ 0];      // depth 1, non-bi pred
            mdRateEstimationTemp->interBiDirBits[3] = CabacEstimatedBits[cabacContextModelPtr->interDirContextModel[1] ^ 1];      // depth 1, bi pred
            mdRateEstimationTemp->interBiDirBits[4] = CabacEstimatedBits[cabacContextModelPtr->interDirContextModel[2] ^ 0];      // depth 2, non-bi pred
            mdRateEstimationTemp->interBiDirBits[5] = CabacEstimatedBits[cabacContextModelPtr->interDirContextModel[2] ^ 1];      // depth 2, bi pred
            mdRateEstimationTemp->interBiDirBits[6] = CabacEstimatedBits[cabacContextModelPtr->interDirContextModel[3] ^ 0];      // depth 3, non-bi pred
            mdRateEstimationTemp->interBiDirBits[7] = CabacEstimatedBits[cabacContextModelPtr->interDirContextModel[3] ^ 1];      // depth 3, bi pred

            mdRateEstimationTemp->interUniDirBits[0] = CabacEstimatedBits[cabacContextModelPtr->interDirContextModel[4] ^ 0];      // uni pred list0
            mdRateEstimationTemp->interUniDirBits[1] = CabacEstimatedBits[cabacContextModelPtr->interDirContextModel[4] ^ 1];      // uni pred list1

            //Setting PAD Junk Buffer to zeros
            for (caseIndex1 = 0; caseIndex1<NUMBER_OF_PAD_VALUES_IN_MD_RATE_ESTIMATION ; caseIndex1++) {
                mdRateEstimationTemp->padJunkBuffer[caseIndex1] = 0;
            }
        }
    }

    return EB_ErrorNone;
}
