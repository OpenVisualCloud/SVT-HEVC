/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/***************************************
 * Includes
 ***************************************/

#include <stdlib.h>

#include "EbTypes.h"
#include "EbSimpleAppContext.h"
#include "EbSimpleAppConfig.h"

#define EB_OUTPUTSTREAMBUFFERSIZE_MACRO(ResolutionSize)                ((ResolutionSize) < (INPUT_SIZE_1080i_TH) ? 0x1E8480 : (ResolutionSize) < (INPUT_SIZE_1080p_TH) ? 0x2DC6C0 : (ResolutionSize) < (INPUT_SIZE_4K_TH) ? 0x2DC6C0 : 0x2DC6C0  )   

EB_ERRORTYPE AllocateFrameBuffer(
    EbConfig_t        *config,
    unsigned char     *pBuffer)
{
    EB_ERRORTYPE   return_error = EB_ErrorNone;

    const int tenBitPackedMode = (config->encoderBitDepth > 8) && (config->compressedTenBitFormat == 0) ? 1 : 0;

    // Determine size of each plane
    const size_t luma8bitSize =

        config->inputPaddedWidth    *
        config->inputPaddedHeight   *

        (1 << tenBitPackedMode);

    const size_t chroma8bitSize = luma8bitSize >> 2;
    const size_t luma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? luma8bitSize : 0;
    const size_t chroma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? chroma8bitSize : 0;

    // Determine  
    EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)pBuffer;

    if (luma8bitSize) {
        inputPtr->luma = (unsigned char*)malloc(luma8bitSize);
        if (!inputPtr->luma) return EB_ErrorInsufficientResources;
    }
    else {
        inputPtr->luma = 0;
    }
    if (chroma8bitSize) {
        inputPtr->cb = (unsigned char*)malloc(chroma8bitSize);
        if (!inputPtr->cb) return EB_ErrorInsufficientResources;
    }
    else {
        inputPtr->cb = 0;
    }
    if (chroma8bitSize) {
        inputPtr->cr = (unsigned char*)malloc(chroma8bitSize);
        if (!inputPtr->cr) return EB_ErrorInsufficientResources;
    }
    else {
        inputPtr->cr = 0;
    }
    if (luma10bitSize) {
        inputPtr->lumaExt = (unsigned char*)malloc(luma10bitSize);
        if (!inputPtr->lumaExt) return EB_ErrorInsufficientResources;
    }
    else {
        inputPtr->lumaExt = 0;
    }
    if (chroma10bitSize) {
        inputPtr->cbExt = (unsigned char*)malloc(chroma10bitSize);
        if (!inputPtr->cbExt) return EB_ErrorInsufficientResources;
    }
    else {
        inputPtr->cbExt = 0;
    }
    if (chroma10bitSize) {
        inputPtr->crExt = (unsigned char*)malloc(chroma10bitSize);
        if (!inputPtr->crExt) return EB_ErrorInsufficientResources;
    }
    else {
        inputPtr->crExt = 0;
    }
    return return_error;
}
/***********************************
 * AppContext Constructor
 ***********************************/
EB_ERRORTYPE EbAppContextCtor(EbAppContext_t *contextPtr, EbConfig_t *config)
{
    EB_ERRORTYPE   return_error = EB_ErrorInsufficientResources;

    // Input Buffer
    contextPtr->inputPictureBuffer = (EB_BUFFERHEADERTYPE*)malloc(sizeof(EB_BUFFERHEADERTYPE));
    if (!contextPtr->inputPictureBuffer) return return_error;

    contextPtr->inputPictureBuffer->pBuffer = (unsigned char*)malloc(sizeof(EB_H265_ENC_INPUT));
    if (!contextPtr->inputPictureBuffer->pBuffer) return return_error;

    contextPtr->inputPictureBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);
    contextPtr->inputPictureBuffer->pAppPrivate = NULL;
    // Allocate frame buffer for the pBuffer
    AllocateFrameBuffer(
        config,
        contextPtr->inputPictureBuffer->pBuffer);
    // output buffer
    contextPtr->outputStreamBuffer = (EB_BUFFERHEADERTYPE*)malloc(sizeof(EB_BUFFERHEADERTYPE));
    if (!contextPtr->outputStreamBuffer) return return_error;

    contextPtr->outputStreamBuffer->pBuffer = (unsigned char*)malloc(EB_OUTPUTSTREAMBUFFERSIZE_MACRO(config->sourceWidth*config->sourceHeight));
    if (!contextPtr->outputStreamBuffer->pBuffer) return return_error;

    contextPtr->outputStreamBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);
    contextPtr->outputStreamBuffer->nAllocLen = EB_OUTPUTSTREAMBUFFERSIZE_MACRO(config->sourceWidth*config->sourceHeight);
    contextPtr->outputStreamBuffer->pAppPrivate = (EB_PTR)contextPtr;

    return return_error;
}

/***********************************
 * AppContext Destructor
 ***********************************/
void EbAppContextDtor(EbAppContext_t *contextPtr)
{
    EB_H265_ENC_INPUT *inputPtr = (EB_H265_ENC_INPUT*)contextPtr->inputPictureBuffer->pBuffer;
    free(inputPtr->luma);
    free(inputPtr->cb);
    free(inputPtr->cr);
    free(inputPtr->lumaExt);
    free(inputPtr->cbExt);
    free(inputPtr->crExt);
    free(contextPtr->inputPictureBuffer->pBuffer);
    free(contextPtr->outputStreamBuffer->pBuffer);
    free(contextPtr->inputPictureBuffer);
    free(contextPtr->outputStreamBuffer);
}

/***********************************************
* Copy configuration parameters from 
*  The config structure, to the 
*  callback structure to send to the library
***********************************************/
EB_ERRORTYPE CopyConfigurationParameters(
    EbConfig_t				*config,
    EbAppContext_t			*callbackData,
    unsigned int         instanceIdx)
{
    EB_ERRORTYPE   return_error = EB_ErrorNone;

    // Assign Instance index to the library
    callbackData->instanceIdx = (unsigned char)instanceIdx;

    // Initialize Port Activity Flags
    callbackData->ebEncParameters.sourceWidth = config->sourceWidth;
    callbackData->ebEncParameters.sourceHeight = config->sourceHeight;
    callbackData->ebEncParameters.encoderBitDepth = config->encoderBitDepth;

    return return_error;

}
/***********************************
 * Initialize Core & Component
 ***********************************/
EB_ERRORTYPE InitEncoder(
    EbConfig_t				*config,
    EbAppContext_t			*callbackData,
    unsigned int        instanceIdx)
{
    EB_ERRORTYPE        return_error = EB_ErrorNone;
    
    ///************************* LIBRARY INIT [START] *********************///
    // STEP 1: Call the library to construct a Component Handle
    return_error = EbInitHandle(&callbackData->svtEncoderHandle, callbackData);
    if (return_error != EB_ErrorNone) {return return_error;}

    // STEP 2: Get the Default parameters from the library
    return_error = EbH265EncInitParameter(
        &callbackData->ebEncParameters);
    if (return_error != EB_ErrorNone) { return return_error; }

    // STEP 3: Copy all configuration parameters into the callback structure
    return_error = CopyConfigurationParameters(config,callbackData,instanceIdx);
    if (return_error != EB_ErrorNone) { return return_error; }
    
    // STEP 4: Send over all configuration parameters
    return_error = EbH265EncSetParameter(callbackData->svtEncoderHandle,&callbackData->ebEncParameters);
    if (return_error != EB_ErrorNone) { return return_error; }

    // STEP 5: Init Encoder
    return_error = EbInitEncoder(callbackData->svtEncoderHandle);

    ///************************* LIBRARY INIT [END] *********************///
    return return_error;
}

/***********************************
 * Deinit Components
 ***********************************/
EB_ERRORTYPE DeInitEncoder(
    EbAppContext_t *callbackDataPtr,
    unsigned int instanceIndex,
    EB_ERRORTYPE   libExitError)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    (void)instanceIndex;

    if (((EB_COMPONENTTYPE*)(callbackDataPtr->svtEncoderHandle)) != NULL) {
        if (libExitError == EB_ErrorInsufficientResources)
            return_error = EbStopEncoder(callbackDataPtr->svtEncoderHandle, 0);
        else 
            return_error = EbDeinitEncoder(callbackDataPtr->svtEncoderHandle);
    }

    // Destruct the buffer memory pool
    if (return_error != EB_ErrorNone) { return return_error; }

    // Destruct the component
    EbDeinitHandle(callbackDataPtr->svtEncoderHandle);
    
    return return_error;
}
