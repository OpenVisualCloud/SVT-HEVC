/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbResourceCoordinationResults.h"


EB_ERRORTYPE ResourceCoordinationResultCtor(
    ResourceCoordinationResults_t *objectPtr,
    EB_PTR objectInitDataPtr)
{
    (void)objectPtr;
    (void)objectInitDataPtr;

    return EB_ErrorNone;
}


EB_ERRORTYPE ResourceCoordinationResultCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    ResourceCoordinationResults_t* obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, ResourceCoordinationResultCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}

