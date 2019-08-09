/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbMotionEstimationResults.h"

EB_ERRORTYPE MotionEstimationResultsCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    MotionEstimationResults_t *contextPtr;
    EB_MALLOC(MotionEstimationResults_t*, contextPtr, sizeof(MotionEstimationResults_t), EB_N_PTR);

    *objectDblPtr = (EB_PTR) contextPtr;
    objectInitDataPtr = 0;

    (void)(objectInitDataPtr);
    return EB_ErrorNone;
}


