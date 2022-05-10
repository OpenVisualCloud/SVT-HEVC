/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbRateControlResults.h"

EB_ERRORTYPE RateControlResultsCtor(
    RateControlResults_t *objectPtr,
    EB_PTR objectInitDataPtr)
{
    (void)objectPtr;
    (void)objectInitDataPtr;

    return EB_ErrorNone;
}

EB_ERRORTYPE RateControlResultsCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    RateControlResults_t* obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, RateControlResultsCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}
