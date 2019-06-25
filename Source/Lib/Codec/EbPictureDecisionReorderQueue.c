/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureManagerReorderQueue.h"

EB_ERRORTYPE PictureManagerReorderEntryCtor(
    PictureManagerReorderEntry_t   **entryDblPtr,
    EB_U32                           pictureNumber)
{
    EB_MALLOC(PictureManagerReorderEntry_t*, *entryDblPtr, sizeof(PictureManagerReorderEntry_t), EB_N_PTR);

    (*entryDblPtr)->pictureNumber       = pictureNumber;
    (*entryDblPtr)->parentPcsWrapperPtr = (EbObjectWrapper_t *)EB_NULL;

    return EB_ErrorNone;
}
