/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbResourceCoordinationResults_h
#define EbResourceCoordinationResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Process Results
 **************************************/
typedef struct ResourceCoordinationResults_s
{
    EbObjectWrapper_t *pictureControlSetWrapperPtr;
    
} ResourceCoordinationResults_t;

typedef struct ResourceCoordinationResultInitData_s
{
    int junk;
} ResourceCoordinationResultInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE ResourceCoordinationResultCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr,
    EB_HANDLE encHandle);


#ifdef __cplusplus
}
#endif    
#endif //EbResourceCoordinationResults_h