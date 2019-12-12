/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAppContext_h
#define EbAppContext_h

#include "EbApi.h"
#include "EbAppConfig.h"

// For compressed 10-bit format, the Y/U/V 2-bit samples of each pixel are packed
// into 1 byte for any YUV format.
#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height, format, is16bit, compressedTenBitFormat) \
    (compressedTenBitFormat ? \
    ((width) * (height) + 2 * (((width) * (height)) >> (3 - format)) + (width) * (height)) : \
    ((((width) * (height)) + 2 * (((width) * (height)) >> (3 - format))) << (is16bit)))

/***************************************

 * App Callback data struct
 ***************************************/
typedef struct EbAppContext_s {
    void                               *cmdSemaphoreHandle;
    void                               *inputSemaphoreHandle;
    void                               *streamSemaphoreHandle;
    EB_H265_ENC_CONFIGURATION           ebEncParameters;

    // Output Ports Active Flags
    APPPORTACTIVETYPE                   outputStreamPortActive;

    // Component Handle
    EB_COMPONENTTYPE*                   svtEncoderHandle;

    // Buffer Pools
    EB_BUFFERHEADERTYPE                *inputBufferPool;
    EB_BUFFERHEADERTYPE                *streamBufferPool;
    EB_BUFFERHEADERTYPE                *reconBuffer;

	// Instance Index
	uint8_t								instanceIdx;

} EbAppContext_t;


/********************************
 * External Function
 ********************************/
extern EB_ERRORTYPE InitEncoder(EbConfig_t *config, EbAppContext_t *callbackData, uint32_t instanceIdx);
extern EB_ERRORTYPE DeInitEncoder(EbAppContext_t *callbackDataPtr, uint32_t instanceIndex);

#endif // EbAppContext_h
