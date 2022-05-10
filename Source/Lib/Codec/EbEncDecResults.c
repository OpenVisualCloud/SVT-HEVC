/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbDefinitions.h"
#include "EbEncDecResults.h"

EB_ERRORTYPE EncDecResultsCtor(
    EncDecResults_t *contextPtr,
    EB_PTR objectInitDataPtr)
{
    (void)contextPtr;
    (void)objectInitDataPtr;

    return EB_ErrorNone;
}

EB_ERRORTYPE EncDecResultsCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    EncDecResults_t* obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, EncDecResultsCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}