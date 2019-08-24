/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureAnalysisResults_h
#define EbPictureAnalysisResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif

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

   
#ifdef __cplusplus
}
#endif
#endif //EbPictureAnalysisResults_h