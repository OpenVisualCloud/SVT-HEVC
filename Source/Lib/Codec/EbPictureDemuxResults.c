/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureDemuxResults.h"

EB_ERRORTYPE PictureResultsCtor(
    PictureDemuxResults_t *objectPtr,
    EB_PTR objectInitDataPtr)
{
    objectPtr->pictureType = EB_PIC_INVALID;
    (void) objectInitDataPtr;
    return EB_ErrorNone;
}

EB_ERRORTYPE PictureResultsCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    PictureDemuxResults_t* obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, PictureResultsCtor, objectInitDataPtr);
    *objectDblPtr = obj;

    return EB_ErrorNone;
}


