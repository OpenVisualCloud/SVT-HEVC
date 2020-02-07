/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureDecisionReorderQueue_h
#define EbPictureDecisionReorderQueue_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 * Packetization Reorder Queue Entry
 ************************************************/
typedef struct PictureDecisionReorderEntry_s {
    EbDctor                               dctor;
    EB_U64                                pictureNumber;
    EbObjectWrapper_t                    *parentPcsWrapperPtr;
} PictureDecisionReorderEntry_t;   

extern EB_ERRORTYPE PictureDecisionReorderEntryCtor(   
    PictureDecisionReorderEntry_t        *entryPtr,
    EB_U32                                pictureNumber);

#ifdef __cplusplus
}
#endif
#endif //EbPictureDecisionReorderQueue_h
