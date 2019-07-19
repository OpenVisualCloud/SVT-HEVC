/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include "EbPacketizationReorderQueue.h"
#include "EbPictureControlSet.h"

EB_ERRORTYPE PicTimingCtor(
    PicTimingEntry_t*   *entryPtr)
{
    EB_MALLOC(PicTimingEntry_t*, *entryPtr, sizeof(PicTimingEntry_t), EB_N_PTR);
    (*entryPtr)->picStruct = 0;
    (*entryPtr)->temporalId = 0;
    (*entryPtr)->decodeOrder = 0;
    return EB_ErrorNone;
}
EB_ERRORTYPE PacketizationReorderEntryCtor(
    PacketizationReorderEntry_t   **entryDblPtr,
    EB_U32                          pictureNumber)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_MALLOC(PacketizationReorderEntry_t*, *entryDblPtr, sizeof(PacketizationReorderEntry_t), EB_N_PTR);
    return_error = PicTimingCtor(
        &(*entryDblPtr)->picTimingEntry);
    return_error = BitstreamCtor(
        &(*entryDblPtr)->bitStreamPtr2,
        PACKETIZATION_PROCESS_BUFFER_SIZE);
    (*entryDblPtr)->pictureNumber                   = pictureNumber;
    (*entryDblPtr)->outputStreamWrapperPtr          = (EbObjectWrapper_t *)EB_NULL;
    (*entryDblPtr)->startSplicing = 0;
    return return_error;
}

