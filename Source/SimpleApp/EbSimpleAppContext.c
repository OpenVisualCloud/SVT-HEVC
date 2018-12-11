/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/
#include <stdlib.h>
#include "EbSimpleAppContext.h"
#define INPUT_SIZE_576p_TH				0x90000		// 0.58 Million   
#define INPUT_SIZE_1080i_TH				0xB71B0		// 0.75 Million
#define INPUT_SIZE_1080p_TH				0x1AB3F0	// 1.75 Million
#define INPUT_SIZE_4K_TH				0x29F630	// 2.75 Million  
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
    contextPtr->inputPictureBuffer->sliceType = INVALID_SLICE;
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
    contextPtr->outputStreamBuffer->pAppPrivate = (void*)contextPtr;
    contextPtr->outputStreamBuffer->sliceType = INVALID_SLICE;
    
    // recon buffer
    if (config->reconFile) {
        contextPtr->reconBuffer = (EB_BUFFERHEADERTYPE*)malloc(sizeof(EB_BUFFERHEADERTYPE));
        if (!contextPtr->reconBuffer) return return_error;
        const size_t lumaSize =
            config->inputPaddedWidth    *
            config->inputPaddedHeight;
        // both u and v
        const size_t chromaSize = lumaSize >> 1;
        const size_t tenBit = (config->encoderBitDepth > 8);
        const size_t frameSize = (lumaSize + chromaSize) << tenBit;

        // Initialize Header
        contextPtr->reconBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);

        contextPtr->reconBuffer->pBuffer = (unsigned char*)malloc(frameSize);
        if (!contextPtr->reconBuffer->pBuffer) return return_error;

        contextPtr->reconBuffer->nAllocLen = (unsigned int)frameSize;
        contextPtr->reconBuffer->pAppPrivate = NULL;
    }
    return EB_ErrorNone;
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
    if(contextPtr->reconBuffer)
        free(contextPtr->reconBuffer);
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
    callbackData->ebEncParameters.sourceWidth       = config->sourceWidth;
    callbackData->ebEncParameters.sourceHeight      = config->sourceHeight;
    callbackData->ebEncParameters.encoderBitDepth   = config->encoderBitDepth;
    callbackData->ebEncParameters.codeVpsSpsPps     = 0;
    callbackData->ebEncParameters.reconEnabled      = config->reconFile ? 1 : 0;
    
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
    return_error = EbInitHandle(&callbackData->svtEncoderHandle, callbackData, &callbackData->ebEncParameters);
    if (return_error != EB_ErrorNone) {return return_error;}

    // STEP 3: Copy all configuration parameters into the callback structure
    return_error = CopyConfigurationParameters(config,callbackData,instanceIdx);
    if (return_error != EB_ErrorNone) { return return_error; }
    
    // STEP 4: Send over all configuration parameters
    return_error = EbH265EncSetParameter(callbackData->svtEncoderHandle,&callbackData->ebEncParameters);
    if (return_error != EB_ErrorNone) { return return_error; }

    // STEP 5: Init Encoder
    return_error = EbInitEncoder(callbackData->svtEncoderHandle);
    if (callbackData->ebEncParameters.codeVpsSpsPps == 0) {
        callbackData->outputStreamBuffer->nFilledLen = 0;
        return_error = EbH265EncStreamHeader(callbackData->svtEncoderHandle, callbackData->outputStreamBuffer);
        if (return_error != EB_ErrorNone) {
            return return_error;
        }
        fwrite(callbackData->outputStreamBuffer->pBuffer, 1, callbackData->outputStreamBuffer->nFilledLen, config->bitstreamFile);
    }
    ///************************* LIBRARY INIT [END] *********************///
    return return_error;
}

/***********************************
 * Deinit Components
 ***********************************/
EB_ERRORTYPE DeInitEncoder(
    EbAppContext_t *callbackDataPtr,
    unsigned int    instanceIndex)
{
    (void)instanceIndex;
    EB_ERRORTYPE return_error = EB_ErrorNone;

    if (((EB_COMPONENTTYPE*)(callbackDataPtr->svtEncoderHandle)) != NULL) {
            return_error = EbDeinitEncoder(callbackDataPtr->svtEncoderHandle);
    }

    // Destruct the buffer memory pool
    if (return_error != EB_ErrorNone) { return return_error; }

    // Destruct the component
    EbDeinitHandle(callbackDataPtr->svtEncoderHandle);
    
    return return_error;
}
