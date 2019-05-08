/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbBitstreamUnit_h
#define EbBitstreamUnit_h

#include "EbDefinitions.h"
#include "EbUtility.h"
#ifdef __cplusplus
extern "C" {
#endif
// Bistream Slice Buffer Size
#define EB_BITSTREAM_SLICE_BUFFER_SIZE          0x300000
// Table A.6 General tier and level limits
#define SLICE_HEADER_COUNT                      600

/**********************************
 * Bitstream Unit Types
 **********************************/
typedef struct OutputBitstreamUnit_s {
    EB_U32             size;                               // allocated buffer size
    EB_U32             byteHolder;                        // holds bytes and partial bytes
    EB_S32             validBitsCount;                     // count of valid bits in byteHolder
    EB_U32             writtenBitsCount;                   // count of written bits
    EB_U32             sliceNum;                           // Number of slices
    EB_U32             sliceLocation[SLICE_HEADER_COUNT];  // Location of each slice in byte
    EB_U32            *bufferBegin;                        // the byte buffer
    EB_U32            *buffer;                             // the byte buffer

} OutputBitstreamUnit_t;

/**********************************
 * Extern Function Declarations
 **********************************/
extern EB_ERRORTYPE OutputBitstreamUnitCtor(
    OutputBitstreamUnit_t   *bitstreamPtr,
    EB_U32                   bufferSize );


extern EB_ERRORTYPE OutputBitstreamReset(OutputBitstreamUnit_t *bitstreamPtr);

extern EB_ERRORTYPE OutputBitstreamFlushBuffer(OutputBitstreamUnit_t *bitstreamPtr);


extern EB_ERRORTYPE OutputBitstreamWrite   (
    OutputBitstreamUnit_t *bitstreamPtr,
    EB_U32                 bits,
    EB_U32                 numberOfBits );

extern EB_ERRORTYPE OutputBitstreamWriteByte(OutputBitstreamUnit_t *bitstreamPtr, EB_U32 bits);

extern EB_ERRORTYPE OutputBitstreamWriteAlignZero(OutputBitstreamUnit_t *bitstreamPtr);

extern EB_ERRORTYPE OutputBitstreamRBSPToPayload(
    OutputBitstreamUnit_t *bitstreamPtr,
    EB_BYTE                outputBuffer,
    EB_U32                *outputBufferIndex,
    EB_U32                *outputBufferSize,
    EB_U32                 startLocation,
    NalUnitType            nalType);
#ifdef __cplusplus
}
#endif

#endif // EbBitstreamUnit_h
