/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEncDecResults_h
#define EbEncDecResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Process Results
 **************************************/
typedef struct EncDecResults_s 
{
    EbObjectWrapper_t      *pictureControlSetWrapperPtr;
    EB_U32                  completedLcuRowIndexStart;
    EB_U32                  completedLcuRowCount;

    EB_U32                  tileIndex;
} EncDecResults_t;

typedef struct EncDecResultsInitData_s
{
    EB_U32         junk;
} EncDecResultsInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EncDecResultsCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);


#ifdef __cplusplus
}
#endif
#endif // EbEncDecResults_h
