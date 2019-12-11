/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbPictureDecisionResults.h"

EB_ERRORTYPE PictureDecisionResultCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr,
    EB_HANDLE encHandle)
{
    PictureDecisionResults_t *objectPtr;
    EB_MALLOC(PictureDecisionResults_t *, objectPtr, sizeof(PictureDecisionResults_t), EB_N_PTR, encHandle);

    *objectDblPtr = (EB_PTR) objectPtr;
    objectInitDataPtr = 0;
    (void) objectInitDataPtr;
    
    return EB_ErrorNone;
}


