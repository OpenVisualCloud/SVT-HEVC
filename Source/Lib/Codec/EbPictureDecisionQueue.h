/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureDecisionQueue_h
#define EbPictureDecisionQueue_h

#include "EbSei.h"
#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPredictionStructure.h"
#include"EbPictureControlSet.h"  
#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 * PA Reference Queue Entry
 ************************************************/
typedef struct PaReferenceQueueEntry_s {
    EbObjectWrapper_t              *inputObjectPtr;
    EB_U64                          pictureNumber;
    EB_U32                          dependentCount;
    EB_U32                          referenceEntryIndex;
    ReferenceList_t                *list0Ptr;
    ReferenceList_t                *list1Ptr;  
    EB_U32                          depList0Count;
    EB_U32                          depList1Count;
    DependentList_t                 list0;
    DependentList_t                 list1;

	PictureParentControlSet_t       *pPcsPtr;
} PaReferenceQueueEntry_t;

extern EB_ERRORTYPE PaReferenceQueueEntryCtor(   
    PaReferenceQueueEntry_t  **entryDblPtr);

#ifdef __cplusplus
}
#endif
#endif // EbPictureDecisionQueue_h