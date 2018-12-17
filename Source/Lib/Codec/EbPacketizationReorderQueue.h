/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPacketizationReorderQueue_h
#define EbPacketizationReorderQueue_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#ifdef __cplusplus
extern "C" {
#endif
/************************************************
 * Packetization Reorder Queue Entry
 ************************************************/
typedef struct PacketizationReorderEntry_s {
    EB_U64                          pictureNumber;    
    EbObjectWrapper_t              *outputStreamWrapperPtr;

    EB_U64                          startTimeSeconds;
    EB_U64                          startTimeuSeconds;
} PacketizationReorderEntry_t;   

extern EB_ERRORTYPE PacketizationReorderEntryCtor(   
    PacketizationReorderEntry_t   **entryDblPtr,
    EB_U32                          pictureNumber);

  
#ifdef __cplusplus
}
#endif
#endif //EbPacketizationReorderQueue_h
