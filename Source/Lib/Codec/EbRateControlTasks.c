/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbRateControlTasks.h"

EB_ERRORTYPE RateControlTasksCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    RateControlTasks_t *contextPtr;
    EB_MALLOC(RateControlTasks_t*, contextPtr, sizeof(RateControlTasks_t), EB_N_PTR);

    *objectDblPtr = (EB_PTR) contextPtr;
    objectInitDataPtr = EB_NULL;

    (void) objectInitDataPtr;

    return EB_ErrorNone;
}

