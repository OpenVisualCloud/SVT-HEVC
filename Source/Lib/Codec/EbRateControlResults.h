/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbRateControlResults_h
#define EbRateControlResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Process Results
 **************************************/
typedef struct RateControlResults_s
{
    EbDctor              dctor;
    EbObjectWrapper_t   *pictureControlSetWrapperPtr;
} RateControlResults_t;

typedef struct RateControlResultsInitData_s
{
    int junk;
} RateControlResultsInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE RateControlResultsCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr);


#ifdef __cplusplus
}
#endif
#endif // EbRateControlResults_h