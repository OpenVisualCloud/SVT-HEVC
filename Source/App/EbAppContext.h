/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAppContext_h
#define EbAppContext_h

#include "EbApi.h"
#include "EbAppConfig.h"

/***************************************

 * App Callback data struct
 ***************************************/
typedef struct EbAppContext_s {
    void                               *cmdSemaphoreHandle;
    void                               *inputSemaphoreHandle;
    void                               *streamSemaphoreHandle;
    EB_PARAM_PORTDEFINITIONTYPE         inputPortDefinition;
    EB_PARAM_PORTDEFINITIONTYPE         outputStreamPortDefinition;
    EB_H265_ENC_CONFIGURATION           ebEncParameters;

    // Output Ports Active Flags
    APPPORTACTIVETYPE                   outputStreamPortActive;

    // Component Handle
    EB_COMPONENTTYPE*                   svtEncoderHandle;

    // Buffer Pools
    EB_BUFFERHEADERTYPE              **inputBufferPool;
    EB_BUFFERHEADERTYPE              **streamBufferPool;
    EB_BUFFERHEADERTYPE               *reconBuffer;

	// Instance Index
	EB_U8								instanceIdx;

} EbAppContext_t;


/********************************
 * External Function
 ********************************/
extern EB_ERRORTYPE InitEncoder(EbConfig_t *config, EbAppContext_t *callbackData, EB_U32 instanceIdx);
extern EB_ERRORTYPE DeInitEncoder(EbAppContext_t *callbackDataPtr, EB_U32 instanceIndex);

#endif // EbAppContext_h