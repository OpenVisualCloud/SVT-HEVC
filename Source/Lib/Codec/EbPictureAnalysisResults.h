/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureAnalysisResults_h
#define EbPictureAnalysisResults_h

#include "EbTypes.h"
#include "EbSystemResourceManager.h"

/**************************************
 * Process Results
 **************************************/
typedef struct PictureAnalysisResults_s
{
    EbObjectWrapper_t   *pictureControlSetWrapperPtr;
} PictureAnalysisResults_t;

typedef struct PictureAnalysisResultInitData_s
{
    int junk;
} PictureAnalysisResultInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE PictureAnalysisResultCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);

   
#endif //EbPictureAnalysisResults_h