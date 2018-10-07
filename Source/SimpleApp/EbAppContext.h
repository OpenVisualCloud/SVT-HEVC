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
    EB_H265_ENC_CONFIGURATION           ebEncParameters;

    // Local Contexts
    InputBitstreamContext_t             inputContext;

    // Component Handle
    EB_HANDLETYPE                       svtEncoderHandle;

    // Buffer Pools
    EB_BUFFERHEADERTYPE                 *inputPictureBuffer;
    EB_BUFFERHEADERTYPE                 *outputStreamBuffer;

    EB_U32 instanceIdx;

} EbAppContext_t;


/********************************
 * External Function
 ********************************/
extern EB_ERRORTYPE EbAppContextCtor(EbAppContext_t *contextPtr, EbConfig_t *config);
extern EB_ERRORTYPE InitEncoder(EbConfig_t *config, EbAppContext_t *callbackData, EB_U32 instanceIdx);
extern EB_ERRORTYPE DeInitEncoder(EbAppContext_t *callbackDataPtr, EB_U32 instanceIndex, EB_ERRORTYPE   libExitError);
extern EB_ERRORTYPE StartEncoder(EbAppContext_t *callbackDataPtr);
extern EB_ERRORTYPE StopEncoder(EbAppContext_t *callbackDataPtr);

#endif // EbAppContext_h