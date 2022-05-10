/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbEncDecTasks.h"

EB_ERRORTYPE EncDecTasksCtor(
    EncDecTasks_t *contextPtr,
    EB_PTR objectInitDataPtr)
{
    (void)contextPtr;
    (void)objectInitDataPtr;

    return EB_ErrorNone;
}

EB_ERRORTYPE EncDecTasksCreator(
    EB_PTR *objectDblPtr,
    EB_PTR  objectInitDataPtr)
{
    EncDecTasks_t *obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, EncDecTasksCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}
