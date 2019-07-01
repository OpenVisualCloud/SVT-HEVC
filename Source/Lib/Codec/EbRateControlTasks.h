/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbRateControlTasks_h
#define EbRateControlTasks_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"


/**************************************
 * Tasks Types
 **************************************/
typedef enum RATE_CONTROL_TASKTYPES {
    RC_PICTURE_MANAGER_RESULT,
    RC_PACKETIZATION_FEEDBACK_RESULT,
    RC_ENTROPY_CODING_ROW_FEEDBACK_RESULT,
    RC_INVALID_TASK
} RATE_CONTROL_TASKTYPES;

/**************************************
 * Process Results
 **************************************/
typedef struct RateControlTasks_s
{
    RATE_CONTROL_TASKTYPES              taskType;
    EbObjectWrapper_t                  *pictureControlSetWrapperPtr;
    EB_U32                              segmentIndex;

    // Following are valid for RC_ENTROPY_CODING_ROW_FEEDBACK_RESULT only
    EB_U64                              pictureNumber;
    EB_U16                              tileIndex;
    EB_U32                              rowNumber;
    EB_U32                              bitCount;
    
} RateControlTasks_t;

typedef struct RateControlTasksInitData_s
{
    int junk;
} RateControlTasksInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE RateControlTasksCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);


#endif // EbRateControlTasks_h
