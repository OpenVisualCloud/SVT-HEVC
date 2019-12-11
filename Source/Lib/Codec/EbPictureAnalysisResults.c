/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbPictureAnalysisResults.h"

EB_ERRORTYPE PictureAnalysisResultCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr,
    EB_HANDLE encHandle)
{
    PictureAnalysisResults_t *objectPtr;
    EB_MALLOC(PictureAnalysisResults_t *, objectPtr, sizeof(PictureAnalysisResults_t), EB_N_PTR, encHandle);

    *objectDblPtr = (EB_PTR) objectPtr;
    objectInitDataPtr = 0;
    (void) objectInitDataPtr;
    
    return EB_ErrorNone;
}

