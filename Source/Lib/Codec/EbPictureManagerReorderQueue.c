/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureDecisionReorderQueue.h"

EB_ERRORTYPE PictureDecisionReorderEntryCtor(   
    PictureDecisionReorderEntry_t   **entryDblPtr,
    EB_U32                            pictureNumber,
    EB_HANDLE                         encHandle)
{
    EB_MALLOC(PictureDecisionReorderEntry_t*, *entryDblPtr, sizeof(PictureDecisionReorderEntry_t), EB_N_PTR, encHandle);

    (*entryDblPtr)->pictureNumber       = pictureNumber;
    (*entryDblPtr)->parentPcsWrapperPtr = (EbObjectWrapper_t *)EB_NULL;
    
    return EB_ErrorNone;
}
