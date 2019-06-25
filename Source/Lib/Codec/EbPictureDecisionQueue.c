/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureDecisionQueue.h"


EB_ERRORTYPE PaReferenceQueueEntryCtor(
    PaReferenceQueueEntry_t   **entryDblPtr)
{
    PaReferenceQueueEntry_t *entryPtr;
    EB_MALLOC(PaReferenceQueueEntry_t*, entryPtr, sizeof(PaReferenceQueueEntry_t), EB_N_PTR);
    *entryDblPtr = entryPtr;

    entryPtr->inputObjectPtr        = (EbObjectWrapper_t*) EB_NULL;
    entryPtr->pictureNumber         = 0;
    entryPtr->referenceEntryIndex   = 0;
    entryPtr->dependentCount        = 0;
    entryPtr->list0Ptr              = (ReferenceList_t*) EB_NULL;
    entryPtr->list1Ptr              = (ReferenceList_t*) EB_NULL;
    EB_MALLOC(EB_S32*, entryPtr->list0.list, sizeof(EB_S32) * (1 << MAX_TEMPORAL_LAYERS) , EB_N_PTR);

    EB_MALLOC(EB_S32*, entryPtr->list1.list, sizeof(EB_S32) * (1 << MAX_TEMPORAL_LAYERS) , EB_N_PTR);

    return EB_ErrorNone;
}


