/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPacketizationReorderQueue_h
#define EbPacketizationReorderQueue_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbEntropyCodingObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/************************************************
 * Packetization Reorder Queue Entry
 ************************************************/
    typedef struct PicTimingEntry_s
    {
        EB_PICT_STRUCT                  picStruct;
        EB_U8                           temporalId;
        EB_U64                          decodeOrder;
        EB_U64                          poc;
    } PicTimingEntry_t;

typedef struct PacketizationReorderEntry_s {
    EB_U64                          pictureNumber;    
    EbObjectWrapper_t              *outputStreamWrapperPtr;

    EB_U64                          startTimeSeconds;
    EB_U64                          startTimeuSeconds;
    EB_U64                          actualBits;
    EB_PICTURE                      sliceType;
    PicTimingEntry_t                *picTimingEntry;
    Bitstream_t                     *bitStreamPtr2;
    EB_U32                          startSplicing;
    EB_U64                          fillerBitsSent;
    EB_U64                          fillerBitsFinal;
    EB_BOOL                         isUsedAsReferenceFlag;
} PacketizationReorderEntry_t;   

extern EB_ERRORTYPE PacketizationReorderEntryCtor(   
    PacketizationReorderEntry_t   **entryDblPtr,
    EB_U32                          pictureNumber);

  
#ifdef __cplusplus
}
#endif
#endif //EbPacketizationReorderQueue_h
