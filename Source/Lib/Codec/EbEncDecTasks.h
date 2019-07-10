/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEncDecTasks_h
#define EbEncDecTasks_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif
#define ENCDEC_TASKS_MDC_INPUT      0
#define ENCDEC_TASKS_ENCDEC_INPUT   1
#define ENCDEC_TASKS_CONTINUE       2

/**************************************
 * Process Results
 **************************************/
typedef struct EncDecTasks_s 
{
    EbObjectWrapper_t            *pictureControlSetWrapperPtr;
    EB_U32                        inputType;
    EB_S16                        encDecSegmentRow;

    EB_U32                        tileRowIndex;
} EncDecTasks_t;

typedef struct EncDecTasksInitData_s
{
    unsigned encDecSegmentRowCount;
} EncDecTasksInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EncDecTasksCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);


#ifdef __cplusplus
}
#endif
#endif // EbEncDecTasks_h
