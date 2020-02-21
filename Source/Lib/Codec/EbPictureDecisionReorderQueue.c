/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureDecisionReorderQueue.h"

EB_ERRORTYPE PictureDecisionReorderEntryCtor(
    PictureDecisionReorderEntry_t    *entryPtr,
    EB_U32                            pictureNumber)
{
    entryPtr->pictureNumber = pictureNumber;

    return EB_ErrorNone;
}
