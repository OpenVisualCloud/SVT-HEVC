/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbInitialRateControlResults_h
#define EbInitialRateControlResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Process Results
 **************************************/
typedef struct InitialRateControlResults_s
{
    EbDctor              dctor;
    EbObjectWrapper_t   *pictureControlSetWrapperPtr;
} InitialRateControlResults_t;

typedef struct InitialRateControlResultInitData_s
{
    int junk;
} InitialRateControlResultInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE InitialRateControlResultsCreator(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);

#ifdef __cplusplus
}
#endif
#endif //EbInitialRateControlResults_h