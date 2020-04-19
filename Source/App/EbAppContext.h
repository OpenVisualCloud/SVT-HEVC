/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAppContext_h
#define EbAppContext_h

#include "EbApi.h"
#include "EbAppConfig.h"

#include <sys/queue.h>

// Close to the Input FIFO size, and can be tuned.
#define INPUT_BUFFER_POOL_SIZE 100

#define MAX(x, y)                       ((x)>(y)?(x):(y))
#define MIN(x, y)                       ((x)<(y)?(x):(y))

/***************************************

 * App Callback data struct
 ***************************************/
typedef struct EbAppInputFrame_s
{
    EB_BUFFERHEADERTYPE *inputFrame;
    LIST_ENTRY(EbAppInputFrame_s) list;
} EbAppInputFrame_t;

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
    EbAppInputFrame_t                  **inputBufferPool;
    uint16_t                            inputBufferPoolSize;
    LIST_HEAD(pool_list, EbAppInputFrame_s)       poolList;
    LIST_HEAD(encoding_list, EbAppInputFrame_s)   encodingList;
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
