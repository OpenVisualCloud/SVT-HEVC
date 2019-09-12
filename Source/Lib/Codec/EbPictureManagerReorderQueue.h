/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureManagerReorderQueue_h
#define EbPictureManagerReorderQueue_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 * Packetization Reorder Queue Entry
 ************************************************/
typedef struct PictureManagerReorderEntry_s {
    EB_U64                                pictureNumber;    
    EbObjectWrapper_t                    *parentPcsWrapperPtr;
} PictureManagerReorderEntry_t;   

extern EB_ERRORTYPE PictureManagerReorderEntryCtor(   
    PictureManagerReorderEntry_t       **entryDblPtr,
    EB_U32                                pictureNumber);

#ifdef __cplusplus
}
#endif
#endif //EbPictureManagerReorderQueue_h
