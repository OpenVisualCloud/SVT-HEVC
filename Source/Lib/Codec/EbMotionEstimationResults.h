/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMotionEstimationResults_h
#define EbMotionEstimationResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Process Results
 **************************************/
typedef struct MotionEstimationResults_s
{
    EbDctor              dctor;
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
extern EB_ERRORTYPE MotionEstimationResultsCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr);


#ifdef __cplusplus
}
#endif
#endif // EbMotionEstimationResults_h