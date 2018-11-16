/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbInitialRateControlResults.h"

EB_ERRORTYPE InitialRateControlResultsCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr)
{
    InitialRateControlResults_t *objectPtr;
    EB_MALLOC(InitialRateControlResults_t *, objectPtr, sizeof(InitialRateControlResults_t), EB_N_PTR);

    *objectDblPtr = (EB_PTR) objectPtr;
    objectInitDataPtr = 0;
    (void) objectInitDataPtr;
    
    return EB_ErrorNone;
}

