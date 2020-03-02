/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbInitialRateControlResults.h"

EB_ERRORTYPE InitialRateControlResultsCtor(
    InitialRateControlResults_t *objectPtr,
    EB_PTR objectInitDataPtr)
{
    (void)objectPtr;
    (void)objectInitDataPtr;
    return EB_ErrorNone;
}

EB_ERRORTYPE InitialRateControlResultsCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    InitialRateControlResults_t* obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, InitialRateControlResultsCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}
