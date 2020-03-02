/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureDecisionResults_h
#define EbPictureDecisionResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Process Results
 **************************************/
typedef struct PictureDecisionResults_s
{
    EbDctor              dctor;
    EbObjectWrapper_t   *pictureControlSetWrapperPtr;
    EB_U32               segmentIndex;
} PictureDecisionResults_t;

typedef struct PictureDecisionResultInitData_s
{
    int junk;
} PictureDecisionResultInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE PictureDecisionResultCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr);

#ifdef __cplusplus
}
#endif
#endif //EbPictureDecisionResults_h
