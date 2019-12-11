/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbRateControlResults.h"

EB_ERRORTYPE RateControlResultsCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr,
    EB_HANDLE encHandle)
{
    RateControlResults_t *contextPtr;
    EB_MALLOC(RateControlResults_t*, contextPtr, sizeof(RateControlResults_t), EB_N_PTR, encHandle);
    *objectDblPtr = (EB_PTR) contextPtr;

    objectInitDataPtr = 0;

    (void) objectInitDataPtr;

    return EB_ErrorNone;
}

