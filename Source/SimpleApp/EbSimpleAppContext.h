/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSimpleAppContext_h
#define EbSimpleAppContext_h

#include "EbApi.h"
#include "EbSimpleAppConfig.h"

/***************************************
 * App Callback data struct
 ***************************************/
typedef struct EbAppContext_s {
    EB_H265_ENC_CONFIGURATION           ebEncParameters;

    // Local Contexts
    InputBitstreamContext_t             inputContext;

    // Component Handle
    EB_COMPONENTTYPE*                   svtEncoderHandle;

    // Buffer Pools
    EB_BUFFERHEADERTYPE                 *inputPictureBuffer;
    EB_BUFFERHEADERTYPE                 *outputStreamBuffer;

    unsigned int instanceIdx;

} EbAppContext_t;


/********************************
 * External Function
 ********************************/
extern EB_ERRORTYPE EbAppContextCtor(EbAppContext_t *contextPtr, EbConfig_t *config);
extern void EbAppContextDtor(EbAppContext_t *contextPtr);
extern EB_ERRORTYPE InitEncoder(EbConfig_t *config, EbAppContext_t *callbackData, unsigned int instanceIdx);
extern EB_ERRORTYPE DeInitEncoder(EbAppContext_t *callbackDataPtr, unsigned int instanceIndex, EB_ERRORTYPE   libExitError);
extern EB_ERRORTYPE StartEncoder(EbAppContext_t *callbackDataPtr);
extern EB_ERRORTYPE StopEncoder(EbAppContext_t *callbackDataPtr);

#endif // EbAppContext_h
