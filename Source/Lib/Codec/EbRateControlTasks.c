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
    (void)objectDblPtr;
    (void)objectInitDataPtr;

    return EB_ErrorNone;
}

EB_ERRORTYPE RateControlTasksCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    RateControlTasks_t* obj;

    EB_NEW(obj, RateControlTasksCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}
