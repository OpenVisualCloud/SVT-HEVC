/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbResourceCoordinationResults.h"

EB_ERRORTYPE ResourceCoordinationResultCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr,
    EB_HANDLE encHandle)
{
    ResourceCoordinationResults_t *objectPtr;
    EB_MALLOC(ResourceCoordinationResults_t*, objectPtr, sizeof(ResourceCoordinationResults_t), EB_N_PTR, encHandle);

    *objectDblPtr = objectPtr;

    objectInitDataPtr = 0;
    (void)objectInitDataPtr;


    return EB_ErrorNone;
}

