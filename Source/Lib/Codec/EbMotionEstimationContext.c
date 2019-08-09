/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbMotionEstimationContext.h"


void MotionEstimetionPredUnitCtor(
    MePredUnit_t   *pu)
{

    pu->distortion = 0xFFFFFFFFull;

    pu->predictionDirection = UNI_PRED_LIST_0;

    pu->Mv[0]        =  0;

    pu->Mv[1]        =  0;

    return;
}


EB_ERRORTYPE MeContextCtor(
    MeContext_t     **objectDblPtr)
{
    EB_U32                   listIndex;
    EB_U32                   refPicIndex;
    EB_U32                   puIndex;
    EB_U32                   meCandidateIndex;

    EB_MALLOC(MeContext_t*, *objectDblPtr, sizeof(MeContext_t), EB_N_PTR);

    (*objectDblPtr)->updateHmeSearchCenter = EB_FALSE;
    (*objectDblPtr)->xMvOffset = 0;
    (*objectDblPtr)->yMvOffset = 0;

    // Intermediate LCU-sized buffer to retain the input samples
    (*objectDblPtr)->lcuBufferStride                = MAX_LCU_SIZE;
    EB_ALLIGN_MALLOC(EB_U8 *, (*objectDblPtr)->lcuBuffer, sizeof(EB_U8) * MAX_LCU_SIZE * (*objectDblPtr)->lcuBufferStride, EB_A_PTR);

    (*objectDblPtr)->hmeLcuBufferStride             = (MAX_LCU_SIZE + HME_DECIM_FILTER_TAP - 1);
    EB_MALLOC(EB_U8 *, (*objectDblPtr)->hmeLcuBuffer, sizeof(EB_U8) * (MAX_LCU_SIZE + HME_DECIM_FILTER_TAP - 1) * (*objectDblPtr)->hmeLcuBufferStride, EB_N_PTR);

    (*objectDblPtr)->quarterLcuBufferStride         = (MAX_LCU_SIZE >> 1);
    EB_MALLOC(EB_U8 *, (*objectDblPtr)->quarterLcuBuffer, sizeof(EB_U8) * (MAX_LCU_SIZE >> 1) * (*objectDblPtr)->quarterLcuBufferStride, EB_N_PTR);

    (*objectDblPtr)->sixteenthLcuBufferStride       = (MAX_LCU_SIZE >> 2);
    EB_ALLIGN_MALLOC(EB_U8 *, (*objectDblPtr)->sixteenthLcuBuffer, sizeof(EB_U8) * (MAX_LCU_SIZE >> 2) * (*objectDblPtr)->sixteenthLcuBufferStride, EB_A_PTR);

    (*objectDblPtr)->interpolatedStride             = MAX_SEARCH_AREA_WIDTH;
    EB_MALLOC(EB_U8 *, (*objectDblPtr)->hmeBuffer, sizeof(EB_U8) * (*objectDblPtr)->interpolatedStride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

    (*objectDblPtr)->hmeBufferStride                = MAX_SEARCH_AREA_WIDTH;

    EB_MEMSET((*objectDblPtr)->lcuBuffer, 0 , sizeof(EB_U8) * MAX_LCU_SIZE * (*objectDblPtr)->lcuBufferStride);
    EB_MEMSET((*objectDblPtr)->hmeLcuBuffer, 0 ,sizeof(EB_U8) * (MAX_LCU_SIZE + HME_DECIM_FILTER_TAP - 1) * (*objectDblPtr)->hmeLcuBufferStride);
    EB_MALLOC(EB_BitFraction *, (*objectDblPtr)->mvdBitsArray, sizeof(EB_BitFraction) * NUMBER_OF_MVD_CASES, EB_N_PTR);
    // 15 intermediate buffers to retain the interpolated reference samples

    //      0    1    2    3
    // 0    A    a    b    c
    // 1    d    e    f    g
    // 2    h    i    j    k
    // 3    n    p    q    r

    //                  _____________
    //                 |             |
    // --I samples --> |Interpolation|-- O samples -->
    //                 | ____________|

    // Before Interpolation: 2 x 3
    //   I   I
    //   I   I
    //   I   I

    // After 1-D Horizontal Interpolation: (2 + 1) x 3 - a, b, and c
    // O I O I O
    // O I O I O
    // O I O I O

    // After 1-D Vertical Interpolation: 2 x (3 + 1) - d, h, and n
    //   O   O
    //   I   I
    //   O   O
    //   I   I
    //   O   O
    //   I   I
    //   O   O

    // After 2-D (Horizontal/Vertical) Interpolation: (2 + 1) x (3 + 1) - e, f, g, i, j, k, n, p, q, and r
    // O   O   O
    //   I   I
    // O   O   O
    //   I   I
    // O   O   O
    //   I   I
    // O   O   O

    for( listIndex = 0; listIndex < MAX_NUM_OF_REF_PIC_LIST; listIndex++) {

        for( refPicIndex = 0; refPicIndex < MAX_REF_IDX; refPicIndex++) {

            EB_MALLOC(EB_U8 *, (*objectDblPtr)->integerBuffer[listIndex][refPicIndex], sizeof(EB_U8) * (*objectDblPtr)->interpolatedStride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

            EB_MALLOC(EB_U8 *, (*objectDblPtr)->posbBuffer[listIndex][refPicIndex], sizeof(EB_U8) * (*objectDblPtr)->interpolatedStride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

            EB_MALLOC(EB_U8 *, (*objectDblPtr)->poshBuffer[listIndex][refPicIndex], sizeof(EB_U8) * (*objectDblPtr)->interpolatedStride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

            EB_MALLOC(EB_U8 *, (*objectDblPtr)->posjBuffer[listIndex][refPicIndex], sizeof(EB_U8) * (*objectDblPtr)->interpolatedStride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

        }

    }

    EB_MALLOC(EB_BYTE, (*objectDblPtr)->oneDIntermediateResultsBuf0, sizeof(EB_U8)*MAX_LCU_SIZE*MAX_LCU_SIZE, EB_N_PTR);

    EB_MALLOC(EB_BYTE, (*objectDblPtr)->oneDIntermediateResultsBuf1, sizeof(EB_U8)*MAX_LCU_SIZE*MAX_LCU_SIZE, EB_N_PTR);

    for(puIndex= 0; puIndex < MAX_ME_PU_COUNT; puIndex++) {
        for( meCandidateIndex = 0; meCandidateIndex < MAX_ME_CANDIDATE_PER_PU; meCandidateIndex++) {
            MotionEstimetionPredUnitCtor(&((*objectDblPtr)->meCandidate[meCandidateIndex]).pu[puIndex]);
        }
    }

    EB_MALLOC(EB_U8 *, (*objectDblPtr)->avctempBuffer, sizeof(EB_U8) * (*objectDblPtr)->interpolatedStride * MAX_SEARCH_AREA_HEIGHT, EB_N_PTR);

    EB_MALLOC(EB_U16 *, (*objectDblPtr)->pEightPosSad16x16, sizeof(EB_U16) * 8 * 16, EB_N_PTR);//16= 16 16x16 blocks in a LCU.       8=8search points

    return EB_ErrorNone;
}
