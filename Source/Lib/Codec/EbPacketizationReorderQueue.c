/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPacketizationReorderQueue.h"

EB_ERRORTYPE PacketizationReorderEntryCtor(
    PacketizationReorderEntry_t   **entryDblPtr,
    EB_U32                          pictureNumber)
{
    EB_MALLOC(PacketizationReorderEntry_t*, *entryDblPtr, sizeof(PacketizationReorderEntry_t), EB_N_PTR);

    (*entryDblPtr)->pictureNumber                   = pictureNumber;
    (*entryDblPtr)->outputStreamWrapperPtr          = (EbObjectWrapper_t *)EB_NULL;

    return EB_ErrorNone;
}

