/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbEncDecTasks.h"

EB_ERRORTYPE EncDecTasksCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr,
    EB_HANDLE encHandle)
{
    EncDecTasks_t *contextPtr;
    EB_MALLOC(EncDecTasks_t*, contextPtr, sizeof(EncDecTasks_t), EB_N_PTR, encHandle);
    
    *objectDblPtr = (EB_PTR) contextPtr;

    (void) objectInitDataPtr;

    return EB_ErrorNone;
}
