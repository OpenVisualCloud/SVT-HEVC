/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbPictureAnalysisResults.h"

EB_ERRORTYPE PictureAnalysisResultCtor(
    PictureAnalysisResults_t *objectPtr,
    EB_PTR objectInitDataPtr)
{
    (void)objectPtr;
    (void)objectInitDataPtr;
    return EB_ErrorNone;
}

EB_ERRORTYPE PictureAnalysisResultCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    PictureAnalysisResults_t *obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, PictureAnalysisResultCtor, objectInitDataPtr);
    *objectDblPtr = (EB_PTR)obj;

    return EB_ErrorNone;
}
