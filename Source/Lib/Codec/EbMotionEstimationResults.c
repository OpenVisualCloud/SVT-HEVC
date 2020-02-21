/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbMotionEstimationResults.h"

EB_ERRORTYPE MotionEstimationResultsCtor(
    MotionEstimationResults_t *contexPtr,
    EB_PTR objectInitDataPtr)
{
    (void)contexPtr;
    (void)objectInitDataPtr;
    return EB_ErrorNone;
}

EB_ERRORTYPE MotionEstimationResultsCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    MotionEstimationResults_t *obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, MotionEstimationResultsCtor, objectInitDataPtr);
    *objectDblPtr = obj;
    return EB_ErrorNone;
}
