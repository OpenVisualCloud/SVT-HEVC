/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbEntropyCodingResults.h"

EB_ERRORTYPE EntropyCodingResultsCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    EntropyCodingResults_t *contextPtr;
    EB_MALLOC(EntropyCodingResults_t*, contextPtr, sizeof(EntropyCodingResults_t), EB_N_PTR);

    *objectDblPtr = (EB_PTR) contextPtr;

    (void) objectInitDataPtr;

    return EB_ErrorNone;
}

