/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbEntropyCodingResults.h"

EB_ERRORTYPE EntropyCodingResultsCtor(
    EntropyCodingResults_t *contextPtr,
    EB_PTR objectInitDataPtr)
{
    (void)contextPtr;
    (void)objectInitDataPtr;

    return EB_ErrorNone;
}


EB_ERRORTYPE EntropyCodingResultsCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    EntropyCodingResults_t* obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, EntropyCodingResultsCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}
