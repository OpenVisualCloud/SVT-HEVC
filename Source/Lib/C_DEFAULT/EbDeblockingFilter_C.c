/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDeblockingFilter_C.h"
#include "EbTypes.h"
#include "EbUtility.h"


/** Luma4SampleEdgeDLFCore()
is used to conduct the deblocking filter upon the 4 luma
samples edge being filtered. The first and fourth sample rows of each 4
sample luma edge are used for the decision making process within this
function except for the weak filtering decision making where all rows
of the 4-sample-edge are used to take the decision.

@param reconPic  (input)
A pointer to the reconstructed picture being filtered
@param reconLumaPicStride (input)
@param originX (input)
the picutre-wise horizontal location of the first sample of the
4 sample luma edge.
@param originY (input)
the picutre-wise vertical location of the first sample of the
4 sample luma edge.
@param isVerticalEdge (input)
TRUE  -- vertical
FALSE -- horizontal
@param tc (input)
@param beta (input)
@param *PCMFlagArray (input)
PCMFlagArray[2] stores the PCM flag which will be used by the filter
on each chroma sample of the edge.
TRUE  -- DLF is disabled because of PCM.
FALSE -- DLF is enabled.
0 -- samples q0,0, q0,1; 1 -- samples p0,0, p0,1.
*/
void Luma4SampleEdgeDLFCore(
    EB_BYTE                edgeStartFilteredSamplePtr,  //please refer to the detailed explantion above.
    EB_U32                 reconLumaPicStride,          //please refer to the detailed explantion above.
    EB_BOOL                isVerticalEdge,              //please refer to the detailed explantion above.
    EB_S32                 tc,                          //please refer to the detailed explantion above.
    EB_S32                 beta)                        //please refer to the detailed explantion above.
{

    // Variables holding the luma values for the sample values for the filtered picture
    EB_S32           filteredQ0;
    EB_S32           filteredP0;
    EB_S32           filteredQ1;
    EB_S32           filteredP1;
    EB_S32           filteredQ2;
    EB_S32           filteredP2;
    EB_S32           filteredQ3;
    EB_S32           filteredP3;

    // thresholds
    EB_S32          delta;
    EB_S32          delta1;
    EB_S32          delta2;
    EB_S32          tc2;
    EB_S32          threshWeakFiltering;
    EB_S32          sideBlocksThresh;
    EB_S32          dq;
    EB_S32          dp;
    EB_S32          dq0;
    EB_S32          dp0;
    EB_S32          dq3;
    EB_S32          dp3;
    EB_S32          d;
    EB_S32          d0;
    EB_S32          d3;
    EB_BOOL         strongFiltering;
    EB_BOOL         strongFilteringLine0;
    EB_BOOL         strongFilteringLine3;

    // will be taking this off later on HT
    int sampleCounter = 0;

    // setting the distance between the neighboring samples at the same row depending on the type of the edge (vertical or horizontal)
    EB_S32          filterStride = isVerticalEdge ?
        1 :
        reconLumaPicStride;
    // setting the distance between the neighboring samples at the same column depending on the type of the edge (vertical or horizontal)
    EB_S32          distanceToNextRowSample = isVerticalEdge ?
        (EB_S32)reconLumaPicStride :
        1;

    // setting the offset for the lines taking the decision
    EB_S32          firstDecisionSampleRowOffset = 0 * distanceToNextRowSample;
    EB_S32          secondDecisionSampleRowOffset = 3 * distanceToNextRowSample;



    // dp = | p22 - 2*p12 + p02 |  +  | p25 - 2*p15 + p05 |
    dp0 = ABS(edgeStartFilteredSamplePtr[-(3 * filterStride) + firstDecisionSampleRowOffset]
        - 2 * edgeStartFilteredSamplePtr[-(filterStride << 1) + firstDecisionSampleRowOffset]
        + edgeStartFilteredSamplePtr[-filterStride + firstDecisionSampleRowOffset]
        );

    dp3 = ABS(edgeStartFilteredSamplePtr[-(3 * filterStride) + secondDecisionSampleRowOffset]
        - 2 * edgeStartFilteredSamplePtr[-(filterStride << 1) + secondDecisionSampleRowOffset]
        + edgeStartFilteredSamplePtr[-filterStride + secondDecisionSampleRowOffset]
        );

    // dp = | q22 - 2*q12 + q02 |  +  | q25 - 2*q15 + q05 |
    dq0 = ABS(edgeStartFilteredSamplePtr[(filterStride << 1) + firstDecisionSampleRowOffset]
        - 2 * edgeStartFilteredSamplePtr[filterStride + firstDecisionSampleRowOffset]
        + edgeStartFilteredSamplePtr[firstDecisionSampleRowOffset]
        );

    dq3 = ABS(edgeStartFilteredSamplePtr[(filterStride << 1) + secondDecisionSampleRowOffset]
        - 2 * edgeStartFilteredSamplePtr[filterStride + secondDecisionSampleRowOffset]
        + edgeStartFilteredSamplePtr[secondDecisionSampleRowOffset]
        );


    // calculate d
    dp = dp0 + dp3;
    dq = dq0 + dq3;
    d0 = dp0 + dq0;
    d3 = dp3 + dq3;
    d = d0 + d3;

    // decision if d > beta : no filtering
    if (d < beta)
    {
        // loading samples from the zeroth line used for the strong filtering decision
        filteredQ0 = (EB_S32)edgeStartFilteredSamplePtr[firstDecisionSampleRowOffset];
        filteredQ3 = (EB_S32)edgeStartFilteredSamplePtr[(3 * filterStride) + firstDecisionSampleRowOffset];

        filteredP0 = (EB_S32)edgeStartFilteredSamplePtr[-filterStride + firstDecisionSampleRowOffset];
        filteredP3 = (EB_S32)edgeStartFilteredSamplePtr[-(4 * filterStride) + firstDecisionSampleRowOffset];

        // strong filtering decision component based on the zeroth line of the 4 sample edge
        strongFilteringLine0 = (EB_BOOL)(((d0 << 1)<(beta >> 2)) &&
            (beta >> 3) > (ABS(filteredP3 - filteredP0) + ABS(filteredQ3 - filteredQ0)) &&
            ((5 * tc + 1) >> 1) > ABS(filteredP0 - filteredQ0));

        // loading samples from the third line used for the strong filtering decision
        filteredQ0 = (EB_S32)edgeStartFilteredSamplePtr[secondDecisionSampleRowOffset];
        filteredQ3 = (EB_S32)edgeStartFilteredSamplePtr[(3 * filterStride) + secondDecisionSampleRowOffset];

        filteredP0 = (EB_S32)edgeStartFilteredSamplePtr[-filterStride + secondDecisionSampleRowOffset];
        filteredP3 = (EB_S32)edgeStartFilteredSamplePtr[-(4 * filterStride) + secondDecisionSampleRowOffset];

        // strong filtering decision component based on the third line of the 4 sample edge
        strongFilteringLine3 = (EB_BOOL)(((d3 << 1)<(beta >> 2)) &&
            (beta >> 3) > (ABS(filteredP3 - filteredP0) + ABS(filteredQ3 - filteredQ0)) &&
            ((5 * tc + 1) >> 1) > ABS(filteredP0 - filteredQ0));

        // strong filtering decision for the 4 sample edge
        strongFiltering = (EB_BOOL)(strongFilteringLine0 && strongFilteringLine3);


        // testing for strong or weak filtering for each row
        for (sampleCounter = 0; sampleCounter < 4; ++sampleCounter)
        {
            // next row in the filtered picture
            filteredQ0 = (EB_S32)edgeStartFilteredSamplePtr[sampleCounter * distanceToNextRowSample];
            filteredQ1 = (EB_S32)edgeStartFilteredSamplePtr[filterStride + sampleCounter * distanceToNextRowSample];
            filteredQ2 = (EB_S32)edgeStartFilteredSamplePtr[(filterStride << 1) + sampleCounter * distanceToNextRowSample];
            filteredQ3 = (EB_S32)edgeStartFilteredSamplePtr[(3 * filterStride) + sampleCounter * distanceToNextRowSample];

            filteredP0 = (EB_S32)edgeStartFilteredSamplePtr[-filterStride + sampleCounter * distanceToNextRowSample];
            filteredP1 = (EB_S32)edgeStartFilteredSamplePtr[-(filterStride << 1) + sampleCounter * distanceToNextRowSample];
            filteredP2 = (EB_S32)edgeStartFilteredSamplePtr[-(3 * filterStride) + sampleCounter * distanceToNextRowSample];
            filteredP3 = (EB_S32)edgeStartFilteredSamplePtr[-(4 * filterStride) + sampleCounter * distanceToNextRowSample];

            if (strongFiltering)
            {
                edgeStartFilteredSamplePtr[sampleCounter * distanceToNextRowSample] =
                    (EB_S8)CLIP3((filteredQ0 - (tc << 1)), (filteredQ0 + (tc << 1)), (filteredP1 + (filteredP0 << 1) + (filteredQ0 << 1) + (filteredQ1 << 1) + filteredQ2 + 4) >> 3);  // filtering q0

                edgeStartFilteredSamplePtr[-filterStride + sampleCounter * distanceToNextRowSample] =
                    (EB_S8)CLIP3((filteredP0 - (tc << 1)), (filteredP0 + (tc << 1)), (filteredP2 + (filteredP1 << 1) + (filteredP0 << 1) + (filteredQ0 << 1) + filteredQ1 + 4) >> 3);  // filtering p0

                edgeStartFilteredSamplePtr[filterStride + sampleCounter * distanceToNextRowSample] =
                    (EB_S8)CLIP3((filteredQ1 - (tc << 1)), (filteredQ1 + (tc << 1)), (filteredP0 + filteredQ0 + filteredQ1 + filteredQ2 + 2) >> 2);                              // filtering q1

                edgeStartFilteredSamplePtr[-(filterStride << 1) + sampleCounter * distanceToNextRowSample] =
                    (EB_S8)CLIP3((filteredP1 - (tc << 1)), (filteredP1 + (tc << 1)), (filteredP2 + filteredP1 + filteredP0 + filteredQ0 + 2) >> 2);                              // filtering p1

                edgeStartFilteredSamplePtr[(filterStride << 1) + sampleCounter * distanceToNextRowSample] =
                    (EB_S8)CLIP3((filteredQ2 - (tc << 1)), (filteredQ2 + (tc << 1)), (filteredP0 + filteredQ0 + filteredQ1 + filteredQ2 * 3 + (filteredQ3 << 1) + 4) >> 3);         // filtering q2

                edgeStartFilteredSamplePtr[-(3 * filterStride) + sampleCounter * distanceToNextRowSample] =
                    (EB_S8)CLIP3((filteredP2 - (tc << 1)), (filteredP2 + (tc << 1)), ((filteredP3 << 1) + filteredP2 * 3 + filteredP1 + filteredP0 + filteredQ0 + 4) >> 3);            // filtering p2
            }


            else // weak filtering
            {
                // calculate delta, ithreshcut
                delta = ((filteredQ0 - filteredP0) * 9 - (filteredQ1 - filteredP1) * 3 + 8) >> 4;
                threshWeakFiltering = tc * 10;

                // if delta < 10*tc then no filtering is applied
                if (ABS(delta) < threshWeakFiltering)
                {
                    // calculate delta
                    delta = CLIP3(-tc, tc, delta);

                    // filter first two samples
                    edgeStartFilteredSamplePtr[sampleCounter * distanceToNextRowSample]
                        = (EB_S8)CLIP3(0, 255, (filteredQ0 - delta)); // filtering q0

                    edgeStartFilteredSamplePtr[-filterStride + sampleCounter * distanceToNextRowSample]
                        = (EB_S8)CLIP3(0, 255, (filteredP0 + delta)); // filtering p0

                    // calculate side thresholds and the new tc value tc2
                    sideBlocksThresh = (beta + (beta >> 1)) >> 3;
                    tc2 = tc >> 1;

                    // check sideP for weak filtering
                    if (sideBlocksThresh > dp)   // filter the second sample in P Side
                    {
                        delta1
                            = CLIP3(-tc2, tc2, ((((filteredP0 + filteredP2 + 1) >> 1) - filteredP1 + delta) >> 1));

                        edgeStartFilteredSamplePtr[-(filterStride << 1) + sampleCounter * distanceToNextRowSample]
                            = (EB_S8)CLIP3(0, 255, (filteredP1 + delta1));      //filtering p1
                    }

                    // check sideQ for weak filtering
                    if (sideBlocksThresh > dq)      // filter the second sample in Q Side
                    {
                        delta2
                            = CLIP3(-tc2, tc2, ((((filteredQ0 + filteredQ2 + 1) >> 1) - filteredQ1 - delta) >> 1));

                        edgeStartFilteredSamplePtr[filterStride + sampleCounter * distanceToNextRowSample]
                            = (EB_S8)CLIP3(0, 255, (filteredQ1 + delta2));      //filtering q1
                    }
                }
            }
        }
    }
}

void Luma4SampleEdgeDLFCore16bit(
    EB_U16         *edgeStartFilteredSamplePtr,
    EB_U32          reconLumaPicStride,
    EB_BOOL         isVerticalEdge,
    EB_S32          tc,
    EB_S32          beta)
{
    // Variables holding the luma values for the sample values for the filtered picture
    EB_S32           filteredQ0;
    EB_S32           filteredP0;
    EB_S32           filteredQ1;
    EB_S32           filteredP1;
    EB_S32           filteredQ2;
    EB_S32           filteredP2;
    EB_S32           filteredQ3;
    EB_S32           filteredP3;

    // thresholds
    EB_S32          delta;
    EB_S32          delta1;
    EB_S32          delta2;
    EB_S32          tc2;
    EB_S32          threshWeakFiltering;
    EB_S32          sideBlocksThresh;
    EB_S32          dq;
    EB_S32          dp;
    EB_S32          dq0;
    EB_S32          dp0;
    EB_S32          dq3;
    EB_S32          dp3;
    EB_S32          d;
    EB_S32          d0;
    EB_S32          d3;
    EB_BOOL         strongFiltering;
    EB_BOOL         strongFilteringLine0;
    EB_BOOL         strongFilteringLine3;

    // will be taking this off later on HT
    int sampleCounter = 0;

    // setting the distance between the neighboring samples at the same row depending on the type of the edge (vertical or horizontal)
    EB_S32          filterStride = isVerticalEdge ?
        1 :
        reconLumaPicStride;
    // setting the distance between the neighboring samples at the same column depending on the type of the edge (vertical or horizontal)
    EB_S32          distanceToNextRowSample = isVerticalEdge ?
        (EB_S32)reconLumaPicStride :
        1;

    // setting the offset for the lines taking the decision
    EB_S32          firstDecisionSampleRowOffset = 0 * distanceToNextRowSample;
    EB_S32          secondDecisionSampleRowOffset = 3 * distanceToNextRowSample;



    // dp = | p22 - 2*p12 + p02 |  +  | p25 - 2*p15 + p05 |
    dp0 = ABS(edgeStartFilteredSamplePtr[-(3 * filterStride) + firstDecisionSampleRowOffset]
        - 2 * edgeStartFilteredSamplePtr[-(filterStride << 1) + firstDecisionSampleRowOffset]
        + edgeStartFilteredSamplePtr[-filterStride + firstDecisionSampleRowOffset]
        );

    dp3 = ABS(edgeStartFilteredSamplePtr[-(3 * filterStride) + secondDecisionSampleRowOffset]
        - 2 * edgeStartFilteredSamplePtr[-(filterStride << 1) + secondDecisionSampleRowOffset]
        + edgeStartFilteredSamplePtr[-filterStride + secondDecisionSampleRowOffset]
        );

    // dp = | q22 - 2*q12 + q02 |  +  | q25 - 2*q15 + q05 |
    dq0 = ABS(edgeStartFilteredSamplePtr[(filterStride << 1) + firstDecisionSampleRowOffset]
        - 2 * edgeStartFilteredSamplePtr[filterStride + firstDecisionSampleRowOffset]
        + edgeStartFilteredSamplePtr[firstDecisionSampleRowOffset]
        );

    dq3 = ABS(edgeStartFilteredSamplePtr[(filterStride << 1) + secondDecisionSampleRowOffset]
        - 2 * edgeStartFilteredSamplePtr[filterStride + secondDecisionSampleRowOffset]
        + edgeStartFilteredSamplePtr[secondDecisionSampleRowOffset]
        );


    // calculate d
    dp = dp0 + dp3;
    dq = dq0 + dq3;
    d0 = dp0 + dq0;
    d3 = dp3 + dq3;
    d = d0 + d3;

    // decision if d > beta : no filtering
    if (d < beta)
    {
        // loading samples from the zeroth line used for the strong filtering decision
        filteredQ0 = (EB_S32)edgeStartFilteredSamplePtr[firstDecisionSampleRowOffset];
        filteredQ3 = (EB_S32)edgeStartFilteredSamplePtr[(3 * filterStride) + firstDecisionSampleRowOffset];

        filteredP0 = (EB_S32)edgeStartFilteredSamplePtr[-filterStride + firstDecisionSampleRowOffset];
        filteredP3 = (EB_S32)edgeStartFilteredSamplePtr[-(4 * filterStride) + firstDecisionSampleRowOffset];

        // strong filtering decision component based on the zeroth line of the 4 sample edge
        strongFilteringLine0 = (EB_BOOL)(((d0 << 1)<(beta >> 2)) &&
            (beta >> 3) > (ABS(filteredP3 - filteredP0) + ABS(filteredQ3 - filteredQ0)) &&
            ((5 * tc + 1) >> 1) > ABS(filteredP0 - filteredQ0));

        // loading samples from the third line used for the strong filtering decision
        filteredQ0 = (EB_S32)edgeStartFilteredSamplePtr[secondDecisionSampleRowOffset];
        filteredQ3 = (EB_S32)edgeStartFilteredSamplePtr[(3 * filterStride) + secondDecisionSampleRowOffset];

        filteredP0 = (EB_S32)edgeStartFilteredSamplePtr[-filterStride + secondDecisionSampleRowOffset];
        filteredP3 = (EB_S32)edgeStartFilteredSamplePtr[-(4 * filterStride) + secondDecisionSampleRowOffset];

        // strong filtering decision component based on the third line of the 4 sample edge
        strongFilteringLine3 = (EB_BOOL)(((d3 << 1)<(beta >> 2)) &&
            (beta >> 3) > (ABS(filteredP3 - filteredP0) + ABS(filteredQ3 - filteredQ0)) &&
            ((5 * tc + 1) >> 1) > ABS(filteredP0 - filteredQ0));

        // strong filtering decision for the 4 sample edge
        strongFiltering = (EB_BOOL)(strongFilteringLine0 && strongFilteringLine3);


        // testing for strong or weak filtering for each row
        for (sampleCounter = 0; sampleCounter < 4; ++sampleCounter)
        {
            // next row in the filtered picture
            filteredQ0 = (EB_S32)edgeStartFilteredSamplePtr[sampleCounter * distanceToNextRowSample];
            filteredQ1 = (EB_S32)edgeStartFilteredSamplePtr[filterStride + sampleCounter * distanceToNextRowSample];
            filteredQ2 = (EB_S32)edgeStartFilteredSamplePtr[(filterStride << 1) + sampleCounter * distanceToNextRowSample];
            filteredQ3 = (EB_S32)edgeStartFilteredSamplePtr[(3 * filterStride) + sampleCounter * distanceToNextRowSample];

            filteredP0 = (EB_S32)edgeStartFilteredSamplePtr[-filterStride + sampleCounter * distanceToNextRowSample];
            filteredP1 = (EB_S32)edgeStartFilteredSamplePtr[-(filterStride << 1) + sampleCounter * distanceToNextRowSample];
            filteredP2 = (EB_S32)edgeStartFilteredSamplePtr[-(3 * filterStride) + sampleCounter * distanceToNextRowSample];
            filteredP3 = (EB_S32)edgeStartFilteredSamplePtr[-(4 * filterStride) + sampleCounter * distanceToNextRowSample];

            if (strongFiltering)
            {
                edgeStartFilteredSamplePtr[sampleCounter * distanceToNextRowSample] =
                    (EB_U16)CLIP3((filteredQ0 - (tc << 1)), (filteredQ0 + (tc << 1)), (filteredP1 + (filteredP0 << 1) + (filteredQ0 << 1) + (filteredQ1 << 1) + filteredQ2 + 4) >> 3);  // filtering q0

                edgeStartFilteredSamplePtr[-filterStride + sampleCounter * distanceToNextRowSample] =
                    (EB_U16)CLIP3((filteredP0 - (tc << 1)), (filteredP0 + (tc << 1)), (filteredP2 + (filteredP1 << 1) + (filteredP0 << 1) + (filteredQ0 << 1) + filteredQ1 + 4) >> 3);  // filtering p0

                edgeStartFilteredSamplePtr[filterStride + sampleCounter * distanceToNextRowSample] =
                    (EB_U16)CLIP3((filteredQ1 - (tc << 1)), (filteredQ1 + (tc << 1)), (filteredP0 + filteredQ0 + filteredQ1 + filteredQ2 + 2) >> 2);                              // filtering q1

                edgeStartFilteredSamplePtr[-(filterStride << 1) + sampleCounter * distanceToNextRowSample] =
                    (EB_U16)CLIP3((filteredP1 - (tc << 1)), (filteredP1 + (tc << 1)), (filteredP2 + filteredP1 + filteredP0 + filteredQ0 + 2) >> 2);                              // filtering p1

                edgeStartFilteredSamplePtr[(filterStride << 1) + sampleCounter * distanceToNextRowSample] =
                    (EB_U16)CLIP3((filteredQ2 - (tc << 1)), (filteredQ2 + (tc << 1)), (filteredP0 + filteredQ0 + filteredQ1 + filteredQ2 * 3 + (filteredQ3 << 1) + 4) >> 3);         // filtering q2

                edgeStartFilteredSamplePtr[-(3 * filterStride) + sampleCounter * distanceToNextRowSample] =
                    (EB_U16)CLIP3((filteredP2 - (tc << 1)), (filteredP2 + (tc << 1)), ((filteredP3 << 1) + filteredP2 * 3 + filteredP1 + filteredP0 + filteredQ0 + 4) >> 3);            // filtering p2
            }


            else // weak filtering
            {
                // calculate delta, ithreshcut
                delta = ((filteredQ0 - filteredP0) * 9 - (filteredQ1 - filteredP1) * 3 + 8) >> 4;
                threshWeakFiltering = tc * 10;

                // if delta < 10*tc then no filtering is applied
                if (ABS(delta) < threshWeakFiltering)
                {
                    // calculate delta
                    delta = CLIP3(-tc, tc, delta);

                    // filter first two samples
                    edgeStartFilteredSamplePtr[sampleCounter * distanceToNextRowSample]
                        = (EB_U16)CLIP3(0, 1023, (filteredQ0 - delta)); // filtering q0

                    edgeStartFilteredSamplePtr[-filterStride + sampleCounter * distanceToNextRowSample]
                        = (EB_U16)CLIP3(0, 1023, (filteredP0 + delta)); // filtering p0

                    // calculate side thresholds and the new tc value tc2
                    sideBlocksThresh = (beta + (beta >> 1)) >> 3;
                    tc2 = tc >> 1;

                    // check sideP for weak filtering
                    if (sideBlocksThresh > dp)   // filter the second sample in P Side
                    {
                        delta1
                            = CLIP3(-tc2, tc2, ((((filteredP0 + filteredP2 + 1) >> 1) - filteredP1 + delta) >> 1));

                        edgeStartFilteredSamplePtr[-(filterStride << 1) + sampleCounter * distanceToNextRowSample]
                            = (EB_U16)CLIP3(0, 1023, (filteredP1 + delta1));      //filtering p1
                    }

                    // check sideQ for weak filtering
                    if (sideBlocksThresh > dq)      // filter the second sample in Q Side
                    {
                        delta2
                            = CLIP3(-tc2, tc2, ((((filteredQ0 + filteredQ2 + 1) >> 1) - filteredQ1 - delta) >> 1));

                        edgeStartFilteredSamplePtr[filterStride + sampleCounter * distanceToNextRowSample]
                            = (EB_U16)CLIP3(0, 1023, (filteredQ1 + delta2));      //filtering q1
                    }
                }
            }
        }
    }
}


/** Chroma2SampleEdgeDLFCore()
is used to conduct the deblocking filter upon a particular
2 sample chroma edge. This method doesn't care about whether the deblocking
fitler will be turned ON/OFF on this 2 sample chroma edge.

@param reconPic (input)
reconPic is the pointer to reconstructed picture to be filtered.
@param reconChromaPicStride (input)
@param edgeStartPointPos_x (input)
edgeStartPointPos_x is the picutre-wise horizontal location of the first sample of the
2 sample chroma edge.
@param edgeStartPointPos_y (input)
edgeStartPointPos_x is the picutre-wise vertical location of the first sample of the
2 sample chroma edge.
@param isVerticalEdge (input)
TRUE  -- vertical
FALSE -- horizontal
@param Tc (input)
@param chromaComponent (input)
0 -- Cb
1 -- Cr
@param *PCMFlagArray (input)
PCMFlagArray[2] stores the PCM flag which will be used by the filter
on each chroma sample of the edge.
TRUE  -- DLF is disabled because of PCM.
FALSE -- DLF is enabled.
0 -- samples q0,0, q0,1; 1 -- samples p0,0, p0,1.
*/
void Chroma2SampleEdgeDLFCore(
    EB_BYTE                edgeStartSampleCb,           //please refer to the detailed explanation above.
    EB_BYTE                edgeStartSampleCr,           //please refer to the detailed explanation above.
    EB_U32                 reconChromaPicStride,        //please refer to the detailed explanation above.
    EB_BOOL                isVerticalEdge,              //please refer to the detailed explanation above.
    EB_U8                  cbTc,                        //please refer to the detailed explanation above.
    EB_U8                  crTc)                        //please refer to the detailed explanation above.
{
    EB_U32   filterStride = isVerticalEdge ? 1 : reconChromaPicStride;
    EB_U32   NeighbourFilteringSampleDistance = isVerticalEdge ? reconChromaPicStride : 1;
    EB_BYTE  q0;
    EB_BYTE  p0;
    EB_BYTE  q1;
    EB_BYTE  p1;
    EB_S16   delta;

    // Cb
    // fitler the first sample
    q0 = edgeStartSampleCb;
    q1 = edgeStartSampleCb + filterStride;
    p0 = edgeStartSampleCb - filterStride;
    p1 = edgeStartSampleCb - (filterStride << 1);
    delta = CLIP3(-cbTc, cbTc, (((((*q0) - (*p0)) << 2) + (*p1) - (*q1) + 4) >> 3));
    (*p0) = (EB_U8)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE, (*p0) + delta);
    (*q0) = (EB_U8)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE, (*q0) - delta);
    // filter the second sample
    q0 = edgeStartSampleCb + NeighbourFilteringSampleDistance;
    q1 = edgeStartSampleCb + NeighbourFilteringSampleDistance + filterStride;
    p0 = edgeStartSampleCb + NeighbourFilteringSampleDistance - filterStride;
    p1 = edgeStartSampleCb + NeighbourFilteringSampleDistance - (filterStride << 1);
    delta = CLIP3(-cbTc, cbTc, (((((*q0) - (*p0)) << 2) + (*p1) - (*q1) + 4) >> 3));
    (*p0) = (EB_U8)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE, (*p0) + delta);
    (*q0) = (EB_U8)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE, (*q0) - delta);

    // Cr
    // fitler the first sample
    q0 = edgeStartSampleCr;
    q1 = edgeStartSampleCr + filterStride;
    p0 = edgeStartSampleCr - filterStride;
    p1 = edgeStartSampleCr - (filterStride << 1);
    delta = CLIP3(-crTc, crTc, (((((*q0) - (*p0)) << 2) + (*p1) - (*q1) + 4) >> 3));
    (*p0) = (EB_U8)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE, (*p0) + delta);
    (*q0) = (EB_U8)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE, (*q0) - delta);
    // filter the second sample
    q0 = edgeStartSampleCr + NeighbourFilteringSampleDistance;
    q1 = edgeStartSampleCr + NeighbourFilteringSampleDistance + filterStride;
    p0 = edgeStartSampleCr + NeighbourFilteringSampleDistance - filterStride;
    p1 = edgeStartSampleCr + NeighbourFilteringSampleDistance - (filterStride << 1);
    delta = CLIP3(-crTc, crTc, (((((*q0) - (*p0)) << 2) + (*p1) - (*q1) + 4) >> 3));
    (*p0) = (EB_U8)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE, (*p0) + delta);
    (*q0) = (EB_U8)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE, (*q0) - delta);

    return;
}

void Chroma2SampleEdgeDLFCore16bit(
    EB_U16                *edgeStartSampleCb,
    EB_U16                *edgeStartSampleCr,
    EB_U32                 reconChromaPicStride,
    EB_BOOL                isVerticalEdge,
    EB_U8                  cbTc,
    EB_U8                  crTc)
{
    EB_U32   filterStride = isVerticalEdge ? 1 : reconChromaPicStride;
    EB_U32   NeighbourFilteringSampleDistance = isVerticalEdge ? reconChromaPicStride : 1;
    EB_U16*  q0;
    EB_U16*  p0;
    EB_U16*  q1;
    EB_U16*  p1;
    EB_S16   delta;

    // Cb
    // fitler the first sample
    q0 = edgeStartSampleCb;
    q1 = edgeStartSampleCb + filterStride;
    p0 = edgeStartSampleCb - filterStride;
    p1 = edgeStartSampleCb - (filterStride << 1);
    delta = CLIP3(-cbTc, cbTc, (((((*q0) - (*p0)) << 2) + (*p1) - (*q1) + 4) >> 3));
    (*p0) = (EB_U16)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE_10BIT, (*p0) + delta);
    (*q0) = (EB_U16)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE_10BIT, (*q0) - delta);
    // filter the second sample
    q0 = edgeStartSampleCb + NeighbourFilteringSampleDistance;
    q1 = edgeStartSampleCb + NeighbourFilteringSampleDistance + filterStride;
    p0 = edgeStartSampleCb + NeighbourFilteringSampleDistance - filterStride;
    p1 = edgeStartSampleCb + NeighbourFilteringSampleDistance - (filterStride << 1);
    delta = CLIP3(-cbTc, cbTc, (((((*q0) - (*p0)) << 2) + (*p1) - (*q1) + 4) >> 3));
    (*p0) = (EB_U16)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE_10BIT, (*p0) + delta);
    (*q0) = (EB_U16)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE_10BIT, (*q0) - delta);

    // Cr
    // fitler the first sample
    q0 = edgeStartSampleCr;
    q1 = edgeStartSampleCr + filterStride;
    p0 = edgeStartSampleCr - filterStride;
    p1 = edgeStartSampleCr - (filterStride << 1);
    delta = CLIP3(-crTc, crTc, (((((*q0) - (*p0)) << 2) + (*p1) - (*q1) + 4) >> 3));
    (*p0) = (EB_U16)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE_10BIT, (*p0) + delta);
    (*q0) = (EB_U16)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE_10BIT, (*q0) - delta);
    // filter the second sample
    q0 = edgeStartSampleCr + NeighbourFilteringSampleDistance;
    q1 = edgeStartSampleCr + NeighbourFilteringSampleDistance + filterStride;
    p0 = edgeStartSampleCr + NeighbourFilteringSampleDistance - filterStride;
    p1 = edgeStartSampleCr + NeighbourFilteringSampleDistance - (filterStride << 1);
    delta = CLIP3(-crTc, crTc, (((((*q0) - (*p0)) << 2) + (*p1) - (*q1) + 4) >> 3));
    (*p0) = (EB_U16)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE_10BIT, (*p0) + delta);
    (*q0) = (EB_U16)CLIP3(0, MAX_CHROMA_SAMPLE_VALUE_10BIT, (*q0) - delta);

    return;
}
