/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEntropyCodingObject_h
#define EbEntropyCodingObject_h

#include "EbDefinitions.h"
#include "EbCabacContextModel.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct Bitstream_s {
    EbDctor dctor;
    EB_PTR  outputBitstreamPtr;
} Bitstream_t;

typedef struct EntropyCoder_s {
    EbDctor dctor;
    EB_PTR  cabacEncodeContextPtr;
} EntropyCoder_t;

extern EB_ERRORTYPE BitstreamCtor(
    Bitstream_t *bitstreamPtr,
    EB_U32 bufferSize);

extern EB_ERRORTYPE EntropyCoderCtor(
    EntropyCoder_t *entropyCoderPtr,
    EB_U32 bufferSize);

extern EB_PTR EntropyCoderGetBitstreamPtr(
    EntropyCoder_t *entropyCoderPtr);
#ifdef __cplusplus
}
#endif
#endif // EbEntropyCodingObject_h