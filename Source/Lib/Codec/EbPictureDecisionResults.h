/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureDecisionResults_h
#define EbPictureDecisionResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"

/**************************************
 * Process Results
 **************************************/
typedef struct PictureDecisionResults_s
{
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
extern EB_ERRORTYPE PictureDecisionResultCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr);


#endif //EbPictureDecisionResults_h
