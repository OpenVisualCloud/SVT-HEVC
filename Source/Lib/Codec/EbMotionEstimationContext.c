/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbMotionEstimationContext.h"


void EbHevcMotionEstimetionPredUnitInit(
    MePredUnit_t   *pu)
{
    pu->distortion = 0xFFFFFFFFull;
    pu->predictionDirection = UNI_PRED_LIST_0;
    pu->Mv[0]        =  0;
    pu->Mv[1]        =  0;

    return;
}

static void MeContextDctor(EB_PTR p)
{
    MeContext_t *obj = (MeContext_t*)p;

    EB_FREE_ARRAY(obj->hmeLcuBuffer);
    EB_FREE_ARRAY(obj->quarterLcuBuffer);
    EB_FREE_ARRAY(obj->hmeBuffer);
    EB_FREE_ARRAY(obj->mvdBitsArray);
    EB_FREE_ALIGNED_ARRAY(obj->sixteenthLcuBuffer);
    EB_FREE_ALIGNED_ARRAY(obj->lcuBuffer);

    for (EB_U32 listIndex = 0; listIndex < MAX_NUM_OF_REF_PIC_LIST; listIndex++) {
        for (EB_U32 refPicIndex = 0; refPicIndex < MAX_REF_IDX; refPicIndex++) {
            EB_FREE_ARRAY(obj->integerBuffer[listIndex][refPicIndex]);
            EB_FREE_ARRAY(obj->posbBuffer[listIndex][refPicIndex]);
            EB_FREE_ARRAY(obj->poshBuffer[listIndex][refPicIndex]);
            EB_FREE_ARRAY(obj->posjBuffer[listIndex][refPicIndex]);
        }
    }

    EB_FREE_ARRAY(obj->oneDIntermediateResultsBuf0);
    EB_FREE_ARRAY(obj->oneDIntermediateResultsBuf1);
    EB_FREE_ARRAY(obj->avctempBuffer);
    EB_FREE_ARRAY(obj->pEightPosSad16x16);
}

EB_ERRORTYPE MeContextCtor(
    MeContext_t     *objectPtr)
{
    objectPtr->dctor = MeContextDctor;

    // Intermediate LCU-sized buffer to retain the input samples
    objectPtr->lcuBufferStride                = MAX_LCU_SIZE;
    EB_CALLOC_ALIGNED_ARRAY(objectPtr->lcuBuffer, MAX_LCU_SIZE * objectPtr->lcuBufferStride);

    objectPtr->hmeLcuBufferStride             = (MAX_LCU_SIZE + HME_DECIM_FILTER_TAP - 1);
    EB_CALLOC_ARRAY(objectPtr->hmeLcuBuffer, (MAX_LCU_SIZE + HME_DECIM_FILTER_TAP - 1) * objectPtr->hmeLcuBufferStride);

    objectPtr->quarterLcuBufferStride         = (MAX_LCU_SIZE >> 1);
    EB_MALLOC_ARRAY(objectPtr->quarterLcuBuffer, (MAX_LCU_SIZE >> 1) * objectPtr->quarterLcuBufferStride);

    objectPtr->sixteenthLcuBufferStride       = (MAX_LCU_SIZE >> 2);
    EB_MALLOC_ALIGNED_ARRAY(objectPtr->sixteenthLcuBuffer, (MAX_LCU_SIZE >> 2) * objectPtr->sixteenthLcuBufferStride);

    objectPtr->interpolatedStride             = MAX_SEARCH_AREA_WIDTH;
    EB_MALLOC_ARRAY(objectPtr->hmeBuffer, objectPtr->interpolatedStride * MAX_SEARCH_AREA_HEIGHT);

    objectPtr->hmeBufferStride                = MAX_SEARCH_AREA_WIDTH;

    EB_MALLOC_ARRAY(objectPtr->mvdBitsArray, NUMBER_OF_MVD_CASES);
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

    for(EB_U32 listIndex = 0; listIndex < MAX_NUM_OF_REF_PIC_LIST; listIndex++) {
        for(EB_U32 refPicIndex = 0; refPicIndex < MAX_REF_IDX; refPicIndex++) {
            EB_MALLOC_ARRAY(objectPtr->integerBuffer[listIndex][refPicIndex], objectPtr->interpolatedStride * MAX_SEARCH_AREA_HEIGHT);
            EB_MALLOC_ARRAY(objectPtr->posbBuffer[listIndex][refPicIndex], objectPtr->interpolatedStride * MAX_SEARCH_AREA_HEIGHT);
            EB_MALLOC_ARRAY(objectPtr->poshBuffer[listIndex][refPicIndex], objectPtr->interpolatedStride * MAX_SEARCH_AREA_HEIGHT);
            EB_MALLOC_ARRAY(objectPtr->posjBuffer[listIndex][refPicIndex], objectPtr->interpolatedStride * MAX_SEARCH_AREA_HEIGHT);
        }
    }

    EB_MALLOC_ARRAY(objectPtr->oneDIntermediateResultsBuf0, MAX_LCU_SIZE*MAX_LCU_SIZE);
    EB_MALLOC_ARRAY(objectPtr->oneDIntermediateResultsBuf1, MAX_LCU_SIZE*MAX_LCU_SIZE);

    for(EB_U32 puIndex= 0; puIndex < MAX_ME_PU_COUNT; puIndex++) {
        for(EB_U32 meCandidateIndex = 0; meCandidateIndex < MAX_ME_CANDIDATE_PER_PU; meCandidateIndex++) {
            EbHevcMotionEstimetionPredUnitInit(&(objectPtr->meCandidate[meCandidateIndex]).pu[puIndex]);
        }
    }

    EB_MALLOC_ARRAY(objectPtr->avctempBuffer, objectPtr->interpolatedStride * MAX_SEARCH_AREA_HEIGHT);
    EB_MALLOC_ARRAY(objectPtr->pEightPosSad16x16, 8 * 16);//16= 16 16x16 blocks in a LCU.       8=8search points

    return EB_ErrorNone;
}
