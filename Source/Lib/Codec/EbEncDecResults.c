/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbDefinitions.h"
#include "EbEncDecResults.h"

EB_ERRORTYPE EncDecResultsCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    EncDecResults_t *contextPtr;
    EB_MALLOC(EncDecResults_t*, contextPtr, sizeof(EncDecResults_t), EB_N_PTR);

    *objectDblPtr = (EB_PTR) contextPtr;

    (void) objectInitDataPtr;

    return EB_ErrorNone;
}

