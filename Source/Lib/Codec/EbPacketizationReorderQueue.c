/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPacketizationReorderQueue.h"
#include "EbPictureControlSet.h"

static void PacketizationReorderEntryDctor(EB_PTR p)
{
    PacketizationReorderEntry_t *obj = (PacketizationReorderEntry_t*)p;
    EB_FREE(obj->picTimingEntry);
    EB_DELETE(obj->bitStreamPtr2);
}

EB_ERRORTYPE PacketizationReorderEntryCtor(
    PacketizationReorderEntry_t    *entryPtr,
    EB_U32                          pictureNumber)
{
    entryPtr->dctor = PacketizationReorderEntryDctor;
    EB_CALLOC(entryPtr->picTimingEntry, 1, sizeof(PicTimingEntry_t));

    EB_NEW(
        entryPtr->bitStreamPtr2,
        BitstreamCtor,
        PACKETIZATION_PROCESS_BUFFER_SIZE);

    entryPtr->pictureNumber = pictureNumber;
    return EB_ErrorNone;
}

