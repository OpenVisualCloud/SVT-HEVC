/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPictureManagerQueue.h"

EB_ERRORTYPE InputQueueEntryCtor(
    InputQueueEntry_t  *entryPtr)
{
    (void)entryPtr;
    return EB_ErrorNone;
}

static void ReferenceQueueEntryDctor(EB_PTR p)
{
    ReferenceQueueEntry_t* obj = (ReferenceQueueEntry_t*)p;
    EB_FREE_ARRAY(obj->list0.list);
    EB_FREE_ARRAY(obj->list1.list);
}

EB_ERRORTYPE ReferenceQueueEntryCtor(   
    ReferenceQueueEntry_t  *entryPtr)
{
    entryPtr->dctor = ReferenceQueueEntryDctor;
    entryPtr->pictureNumber = ~0u;
    EB_MALLOC_ARRAY(entryPtr->list0.list, (1 << MAX_TEMPORAL_LAYERS));
    EB_MALLOC_ARRAY(entryPtr->list1.list, (1 << MAX_TEMPORAL_LAYERS));

    return EB_ErrorNone;
}
