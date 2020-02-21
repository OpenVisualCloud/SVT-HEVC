/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbPictureDecisionResults.h"

EB_ERRORTYPE PictureDecisionResultCtor(
    PictureDecisionResults_t *objectPtr,
    EB_PTR objectInitDataPtr)
{
    (void)objectPtr;
    (void)objectInitDataPtr;
    return EB_ErrorNone;
}

EB_ERRORTYPE PictureDecisionResultCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    PictureDecisionResults_t* obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, PictureDecisionResultCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}


