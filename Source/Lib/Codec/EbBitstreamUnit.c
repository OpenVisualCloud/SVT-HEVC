/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbBitstreamUnit.h"
#include "EbDefinitions.h"

static void OutputBitstreamUnitDctor(EB_PTR p)
{
    OutputBitstreamUnit_t *obj = (OutputBitstreamUnit_t*)p;
    EB_FREE_ARRAY(obj->bufferBegin);
}

/**********************************
 * Constructor
 **********************************/
EB_ERRORTYPE OutputBitstreamUnitCtor(
    OutputBitstreamUnit_t   *bitstreamPtr,
    EB_U32                   bufferSize )
{

    EB_U32       sliceIndex;
    bitstreamPtr->dctor = OutputBitstreamUnitDctor;

    if(bufferSize) {
        bitstreamPtr->size             = bufferSize / sizeof(unsigned int);
        EB_MALLOC_ARRAY(bitstreamPtr->bufferBegin, bitstreamPtr->size);
        bitstreamPtr->buffer           = bitstreamPtr->bufferBegin;
    }
    else {
        bitstreamPtr->size             = 0;
        bitstreamPtr->bufferBegin      = 0;
        bitstreamPtr->buffer           = 0;
    }

    bitstreamPtr->validBitsCount   = 32;
    bitstreamPtr->byteHolder       = 0;
    bitstreamPtr->writtenBitsCount = 0;
    bitstreamPtr->sliceNum         = 0;

    for(sliceIndex=0; sliceIndex < SLICE_HEADER_COUNT; ++sliceIndex) {
        bitstreamPtr->sliceLocation[sliceIndex] = 0;
    }

    return EB_ErrorNone;
}



/**********************************
 * Reset Bitstream
 **********************************/
EB_ERRORTYPE OutputBitstreamReset(
    OutputBitstreamUnit_t *bitstreamPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    EB_U32       sliceIndex;

    bitstreamPtr->validBitsCount   = 32;
    bitstreamPtr->byteHolder       = 0;
    bitstreamPtr->writtenBitsCount = 0;
    // Reset the write ptr to the beginning of the buffer
    bitstreamPtr->buffer           = bitstreamPtr->bufferBegin;

    bitstreamPtr->sliceNum         = 0;
    for(sliceIndex=0; sliceIndex < SLICE_HEADER_COUNT; ++sliceIndex) {
        bitstreamPtr->sliceLocation[sliceIndex] = 0;
    }

    return return_error;
}

/**********************************
 * Flush the Buffer
 **********************************/
EB_ERRORTYPE OutputBitstreamFlushBuffer(
    OutputBitstreamUnit_t   *bitstreamPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    if (bitstreamPtr->validBitsCount != 0) {
        *(bitstreamPtr->buffer) = EndianSwap( bitstreamPtr->byteHolder );
        bitstreamPtr->writtenBitsCount = ((bitstreamPtr->writtenBitsCount+7)>>3)<<3;
    }

    return return_error;
}

/**********************************
 * Write to bitstream
 *   Intended to be used in CABAC
 **********************************/
EB_ERRORTYPE OutputBitstreamWrite (
    OutputBitstreamUnit_t *bitstreamPtr,
    EB_U32                 bits,
    EB_U32                 numberOfBits)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_U32 shiftCount;

    bitstreamPtr->writtenBitsCount += numberOfBits;

    // If number of bits is less than Valid bits, one word
    if( (EB_S32)numberOfBits < bitstreamPtr->validBitsCount) {
        bitstreamPtr->validBitsCount -= numberOfBits;
        bitstreamPtr->byteHolder     |= bits << bitstreamPtr->validBitsCount;
    } else {
        shiftCount = numberOfBits - bitstreamPtr->validBitsCount;

        // add the last bits
        bitstreamPtr->byteHolder |= bits >> shiftCount;
        *bitstreamPtr->buffer++   = EndianSwap( bitstreamPtr->byteHolder );

        // note: there is a problem with left shift with 32
        bitstreamPtr->validBitsCount = 32 - shiftCount;
        bitstreamPtr->byteHolder  = ( 0 == shiftCount ) ?
                                    0 :
                                    (bits << bitstreamPtr->validBitsCount);

    }

    return return_error;
}

EB_ERRORTYPE OutputBitstreamWriteByte(OutputBitstreamUnit_t *bitstreamPtr, EB_U32 bits)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    bitstreamPtr->writtenBitsCount += 8;

    if (8 < bitstreamPtr->validBitsCount)
    {
        bitstreamPtr->validBitsCount -= 8;
        bitstreamPtr->byteHolder |= bits << bitstreamPtr->validBitsCount;
    }
    else
    {
        *bitstreamPtr->buffer++ = EndianSwap( bitstreamPtr->byteHolder | bits );

        bitstreamPtr->validBitsCount = 32;
        bitstreamPtr->byteHolder  = 0;
    }

    return return_error;
}

/**********************************
 * Write allign zero to bitstream
 *   Intended to be used in CABAC
 **********************************/
EB_ERRORTYPE OutputBitstreamWriteAlignZero(OutputBitstreamUnit_t *bitstreamPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    OutputBitstreamWrite(
        bitstreamPtr,
        0,
        bitstreamPtr->validBitsCount & 0x7 );

    return return_error;
}

/**********************************
 * Output RBSP to payload
 *   Intended to be used in CABAC
 **********************************/
EB_ERRORTYPE OutputBitstreamRBSPToPayload(
    OutputBitstreamUnit_t   *bitstreamPtr,
    EB_BYTE                  outputBuffer,
    EB_U32                  *outputBufferIndex,
    EB_U32                  *outputBufferSize,
    EB_U32                   startLocation,
    NalUnitType              nalType)
{
    EB_ERRORTYPE return_error      = EB_ErrorNone;

    EB_U32  bufferWrittenBytesCount = bitstreamPtr->writtenBitsCount>>3;
    EB_U32  zeroByteCount           = 0;
    EB_U32  writeLocation           = startLocation;
    EB_U32  readLocation            = startLocation;
    EB_U32  sliceIndex              = 0;
    EB_BYTE readBytePtr;
    EB_BYTE writeBytePtr;


    readBytePtr  = (EB_BYTE) bitstreamPtr->bufferBegin;
    writeBytePtr = &outputBuffer[*outputBufferIndex];

    while ( (readLocation < bufferWrittenBytesCount) && ((*outputBufferIndex) < ((*outputBufferSize)-5)) ) {
        // skip over start codes introduced before slice headers
        if (sliceIndex < bitstreamPtr->sliceNum &&
                readLocation == bitstreamPtr->sliceLocation[sliceIndex] &&
                (*outputBufferIndex) < ((*outputBufferSize)-3)) {
            writeBytePtr[writeLocation++] =  readBytePtr[readLocation++];
            writeBytePtr[writeLocation++] =  readBytePtr[readLocation++];
            writeBytePtr[writeLocation++] =  readBytePtr[readLocation++];
            writeBytePtr[writeLocation++] =  readBytePtr[readLocation++];

            *outputBufferIndex += 4;
            sliceIndex++;
        }

        // add emulation code
        if( (zeroByteCount == 2) && ((readBytePtr[readLocation] & 0xfc) == 0) && ((*outputBufferIndex) < (*outputBufferSize)) && nalType != NAL_UNIT_UNSPECIFIED_62 ) {
            writeBytePtr[writeLocation++] = 0x03;
            zeroByteCount = 0;
            *outputBufferIndex += 1;
        }

        if ((*outputBufferIndex) < (*outputBufferSize)) {
            writeBytePtr[writeLocation++] = readBytePtr[readLocation];
            *outputBufferIndex += 1;
        }

        // count the number of zeros for emulation code check
        zeroByteCount = (readBytePtr[readLocation] == 0 ) ? zeroByteCount + 1 : 0;
        readLocation++;
    }

    bitstreamPtr->writtenBitsCount = writeLocation << 3;

    return return_error;
}
