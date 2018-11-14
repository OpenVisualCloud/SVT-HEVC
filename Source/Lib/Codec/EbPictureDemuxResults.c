/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureDemuxResults.h"

EB_ERRORTYPE PictureResultsCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr)
{
    PictureDemuxResults_t *objectPtr;
    EB_MALLOC(PictureDemuxResults_t*, objectPtr, sizeof(PictureDemuxResults_t), EB_N_PTR);

    *objectDblPtr                    = objectPtr;
    
    objectPtr->pictureType           = EB_PIC_INVALID;
    objectPtr->pictureControlSetWrapperPtr = 0;
    objectPtr->referencePictureWrapperPtr = 0;
    objectPtr->pictureNumber = 0;
    
    (void) objectInitDataPtr;
        
    return EB_ErrorNone;
}


