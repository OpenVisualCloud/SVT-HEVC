/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureDecisionQueue.h"

static void PaReferenceQueueEntryDctor(EB_PTR p)
{
    PaReferenceQueueEntry_t* obj = (PaReferenceQueueEntry_t*)p;
    EB_FREE_ARRAY(obj->list0.list);
    EB_FREE_ARRAY(obj->list1.list);
}

EB_ERRORTYPE PaReferenceQueueEntryCtor(   
    PaReferenceQueueEntry_t   *entryPtr)
{
    entryPtr->dctor = PaReferenceQueueEntryDctor;
    EB_MALLOC_ARRAY(entryPtr->list0.list, (1 << MAX_TEMPORAL_LAYERS));
    EB_MALLOC_ARRAY(entryPtr->list1.list, (1 << MAX_TEMPORAL_LAYERS));

    return EB_ErrorNone;
}
