/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMotionEstimationResults_h
#define EbMotionEstimationResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Process Results
 **************************************/
typedef struct MotionEstimationResults_s 
{
    EbObjectWrapper_t   *pictureControlSetWrapperPtr;
    EB_U32               segmentIndex;
} MotionEstimationResults_t;

typedef struct MotionEstimationResultsInitData_s
{
    int junk;
} MotionEstimationResultsInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE MotionEstimationResultsCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr,
    EB_HANDLE encHandle);


#ifdef __cplusplus
}
#endif
#endif // EbMotionEstimationResults_h