/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbPerFramePrediction.h"

EB_U64 predictBits(SequenceControlSet_t *sequenceControlSetPtr, EncodeContext_t *encodeContextPtr, HlRateControlHistogramEntry_t *hlRateControlHistogramPtrTemp, EB_U32 qp)
{
    EB_U64 totalBits = 0;
    if (hlRateControlHistogramPtrTemp->isCoded) {
        // If the frame is already coded, use the actual number of bits
        totalBits = hlRateControlHistogramPtrTemp->totalNumBitsCoded;
    }
    else {
        RateControlTables_t *rateControlTablesPtr = &encodeContextPtr->rateControlTablesArray[qp];
        EB_Bit_Number *sadBitsArrayPtr = rateControlTablesPtr->sadBitsArray[hlRateControlHistogramPtrTemp->temporalLayerIndex];
        EB_Bit_Number *intraSadBitsArrayPtr = rateControlTablesPtr->intraSadBitsArray[0];
        EB_U32 predBitsRefQp = 0;
        EB_U32 numOfFullLcus = 0;
        EB_U32 areaInPixel = sequenceControlSetPtr->lumaWidth * sequenceControlSetPtr->lumaHeight;

        if (hlRateControlHistogramPtrTemp->sliceType == EB_I_PICTURE) {
            // Loop over block in the frame and calculated the predicted bits at reg QP
            EB_U32 i;
            EB_U32 accum = 0;
            for (i = 0; i < NUMBER_OF_INTRA_SAD_INTERVALS; ++i)
            {
                accum += (EB_U32)(hlRateControlHistogramPtrTemp->oisDistortionHistogram[i] * intraSadBitsArrayPtr[i]);
            }

            predBitsRefQp = accum;
            numOfFullLcus = hlRateControlHistogramPtrTemp->fullLcuCount;
            totalBits += predBitsRefQp;
        }
        else {
            EB_U32 i;
            EB_U32 accum = 0;
            EB_U32 accumIntra = 0;
            for (i = 0; i < NUMBER_OF_SAD_INTERVALS; ++i)
            {
                accum += (EB_U32)(hlRateControlHistogramPtrTemp->meDistortionHistogram[i] * sadBitsArrayPtr[i]);
                accumIntra += (EB_U32)(hlRateControlHistogramPtrTemp->oisDistortionHistogram[i] * intraSadBitsArrayPtr[i]);

            }
            if (accum > accumIntra * 3)
                predBitsRefQp = accumIntra;
            else
                predBitsRefQp = accum;
            numOfFullLcus = hlRateControlHistogramPtrTemp->fullLcuCount;
            totalBits += predBitsRefQp;
        }

        // Scale for in complete LCSs
        //  predBitsRefQp is normalized based on the area because of the LCUs at the picture boundries
        totalBits = totalBits * (EB_U64)areaInPixel / (numOfFullLcus << 12);
    }
    hlRateControlHistogramPtrTemp->predBitsRefQp[qp] = totalBits;
    return totalBits;
}