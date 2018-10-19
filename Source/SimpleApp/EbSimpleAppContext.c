/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/***************************************
 * Includes
 ***************************************/

#include <stdlib.h>

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
    contextPtr->outputStreamBuffer->pAppPrivate = (void*)contextPtr;

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

/**********************************
* Set Parameter
**********************************/
EB_ERRORTYPE InitParameter(
    EB_H265_ENC_CONFIGURATION*   configPtr)
{
    EB_ERRORTYPE                  return_error = EB_ErrorNone;

    if (!configPtr) {
        printf("The EB_H265_ENC_CONFIGURATION structure is empty! \n");
        return EB_ErrorBadParameter;
    }

    configPtr->frameRate = 60;
    configPtr->frameRateNumerator = 0;
    configPtr->frameRateDenominator = 0;
    configPtr->encoderBitDepth = 8;
    configPtr->compressedTenBitFormat = 0;
    configPtr->sourceWidth = 0;
    configPtr->sourceHeight = 0;
    configPtr->inputPictureStride = 0;
    configPtr->framesToBeEncoded = 0;


    // Interlaced Video 
    configPtr->interlacedVideo = 0;
    configPtr->qp = 32;
    configPtr->useQpFile = 0;
    configPtr->sceneChangeDetection = 1;
    configPtr->rateControlMode = 0;
    configPtr->lookAheadDistance = 17;
    configPtr->targetBitRate = 7000000;
    configPtr->maxQpAllowed = 48;
    configPtr->minQpAllowed = 10;
    configPtr->baseLayerSwitchMode = 0;
    configPtr->encMode  = 9;
    configPtr->intraPeriodLength = -2;
    configPtr->intraRefreshType = 1;
    configPtr->hierarchicalLevels = 3;
    configPtr->predStructure = 2;
    configPtr->disableDlfFlag = 0;
    configPtr->enableSaoFlag = 1;
    configPtr->useDefaultMeHme = 1;
    configPtr->enableHmeFlag = 1;
    configPtr->enableHmeLevel0Flag = 1;
    configPtr->enableHmeLevel1Flag = 0;
    configPtr->enableHmeLevel2Flag = 0;
    configPtr->searchAreaWidth = 16;
    configPtr->searchAreaHeight = 7;
    configPtr->numberHmeSearchRegionInWidth = 2;
    configPtr->numberHmeSearchRegionInHeight = 2;
    configPtr->hmeLevel0TotalSearchAreaWidth = 64;
    configPtr->hmeLevel0TotalSearchAreaHeight = 25;
    configPtr->hmeLevel0SearchAreaInWidthArray[0] = 32;
    configPtr->hmeLevel0SearchAreaInWidthArray[1] = 32;
    configPtr->hmeLevel0SearchAreaInHeightArray[0] = 12;
    configPtr->hmeLevel0SearchAreaInHeightArray[1] = 13;
    configPtr->hmeLevel1SearchAreaInWidthArray[0] = 1;
    configPtr->hmeLevel1SearchAreaInWidthArray[1] = 1;
    configPtr->hmeLevel1SearchAreaInHeightArray[0] = 1;
    configPtr->hmeLevel1SearchAreaInHeightArray[1] = 1;
    configPtr->hmeLevel2SearchAreaInWidthArray[0] = 1;
    configPtr->hmeLevel2SearchAreaInWidthArray[1] = 1;
    configPtr->hmeLevel2SearchAreaInHeightArray[0] = 1;
    configPtr->hmeLevel2SearchAreaInHeightArray[1] = 1;
    configPtr->constrainedIntra = 0;
    configPtr->tune = 0;

    // Thresholds
    configPtr->videoUsabilityInfo = 0;
    configPtr->highDynamicRangeInput = 0;
    configPtr->accessUnitDelimiter = 0;
    configPtr->bufferingPeriodSEI = 0;
    configPtr->pictureTimingSEI = 0;

    configPtr->bitRateReduction = 1;
    configPtr->improveSharpness = 1;
    configPtr->registeredUserDataSeiFlag = 0;
    configPtr->unregisteredUserDataSeiFlag = 0;
    configPtr->recoveryPointSeiFlag = 0;
    configPtr->enableTemporalId = 1;
    configPtr->inputOutputBufferFifoInitCount = 50;

    // Annex A parameters
    configPtr->profile = 2;
    configPtr->tier = 0;
    configPtr->level = 0;

    // Latency
    configPtr->injectorFrameRate = 60 << 16;
    configPtr->speedControlFlag = 0;
    configPtr->latencyMode = EB_NORMAL_LATENCY;

    // ASM Type
    configPtr->asmType = ASM_AVX2; 
    configPtr->useRoundRobinThreadAssignment = 0;
    configPtr->channelId = 0;
    configPtr->activeChannelCount = 1;


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
    return_error = InitParameter(
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
