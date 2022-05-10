/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEntropyCodingResults_h
#define EbEntropyCodingResults_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * Process Results
 **************************************/
typedef struct
{
    EbDctor                 dctor;
    EbObjectWrapper_t      *pictureControlSetWrapperPtr;
} EntropyCodingResults_t;

typedef struct
{
    EB_U32         junk;
} EntropyCodingResultsInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EntropyCodingResultsCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr);


#ifdef __cplusplus
}
#endif
#endif // EbEntropyCodingResults_h