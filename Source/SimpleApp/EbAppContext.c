/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/***************************************
 * Includes
 ***************************************/

#include <stdlib.h>

#include "EbTypes.h"
#include "EbAppContext.h"
#include "EbAppConfig.h"

 /***************************************
 * Variables Defining a memory table 
 *  hosting all allocated pointers 
 ***************************************/
EbMemoryMapEntry               *appMemoryMap; 
EB_U32                         *appMemoryMapIndex;
EB_U64                         *totalAppMemory;
EB_U32                         appMallocCount = 0;
static EbMemoryMapEntry        *appMemoryMapAllChannels[MAX_CHANNEL_NUMBER]; 
static EB_U32                   appMemoryMapIndexAllChannels[MAX_CHANNEL_NUMBER];
static EB_U64                   appMemoryMallocdAllChannels[MAX_CHANNEL_NUMBER];

/***************************************
* Allocation and initializing a memory table
*  hosting all allocated pointers
***************************************/
void AllocateMemoryTable(
    EB_U32	instanceIdx)
{
    // Malloc Memory Table for the instance @ instanceIdx
    appMemoryMapAllChannels[instanceIdx]        = (EbMemoryMapEntry*)malloc(sizeof(EbMemoryMapEntry) * MAX_APP_NUM_PTR);

    // Init the table index
    appMemoryMapIndexAllChannels[instanceIdx]   = 0;

    // Size of the table
    appMemoryMallocdAllChannels[instanceIdx]    = sizeof(EbMemoryMapEntry) * MAX_APP_NUM_PTR;
    totalAppMemory = &appMemoryMallocdAllChannels[instanceIdx];

    // Set pointer to the first entry
    appMemoryMap                                = appMemoryMapAllChannels[instanceIdx];

    // Set index to the first entry
    appMemoryMapIndex                           = &appMemoryMapIndexAllChannels[instanceIdx];

    // Init Number of pointers
    appMallocCount = 0;

    return;
}

EB_ERRORTYPE AllocateFrameBuffer(
    EbConfig_t				*config,
    EB_U8      			    *pBuffer)
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
        EB_APP_MALLOC(unsigned char*, inputPtr->luma, luma8bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        inputPtr->luma = 0;
    }
    if (chroma8bitSize) {
        EB_APP_MALLOC(unsigned char*, inputPtr->cb, chroma8bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        inputPtr->cb = 0;
    }
    if (chroma8bitSize) {
        EB_APP_MALLOC(unsigned char*, inputPtr->cr, chroma8bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        inputPtr->cr = 0;
    }
    if (luma10bitSize) {
        EB_APP_MALLOC(unsigned char*, inputPtr->lumaExt, luma10bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        inputPtr->lumaExt = 0;
    }
    if (chroma10bitSize) {
        EB_APP_MALLOC(unsigned char*, inputPtr->cbExt, chroma10bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        inputPtr->cbExt = 0;
    }
    if (chroma10bitSize) {
        EB_APP_MALLOC(unsigned char*, inputPtr->crExt, chroma10bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
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
    EB_ERRORTYPE   return_error = EB_ErrorNone;

    // Allocate a memory table hosting all allocated pointers
    AllocateMemoryTable(0);

    // Input Buffer
    EB_APP_MALLOC(EB_BUFFERHEADERTYPE*, contextPtr->inputPictureBuffer, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR, return_error);
    EB_APP_MALLOC(EB_U8*, contextPtr->inputPictureBuffer->pBuffer, sizeof(EB_H265_ENC_INPUT), EB_N_PTR, return_error);
    contextPtr->inputPictureBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);
    contextPtr->inputPictureBuffer->pAppPrivate = NULL;
    // Allocate frame buffer for the pBuffer
    AllocateFrameBuffer(
        config,
        contextPtr->inputPictureBuffer->pBuffer);
    // output buffer
    EB_APP_MALLOC(EB_BUFFERHEADERTYPE*, contextPtr->outputStreamBuffer, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR, return_error);
    EB_APP_MALLOC(EB_U8*, contextPtr->outputStreamBuffer->pBuffer, 0x2DC6C0, EB_N_PTR, return_error);
    
    contextPtr->outputStreamBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);
    contextPtr->outputStreamBuffer->nAllocLen = 0x2DC6C0;
    contextPtr->outputStreamBuffer->pAppPrivate = (EB_PTR)contextPtr;

    return return_error;
}

/***********************************************
* Copy configuration parameters from 
*  The config structure, to the 
*  callback structure to send to the library
***********************************************/
EB_ERRORTYPE CopyConfigurationParameters(
    EbConfig_t				*config,
    EbAppContext_t			*callbackData,
    EB_U32                   instanceIdx)
{
    EB_ERRORTYPE   return_error = EB_ErrorNone;

    // Assign Instance index to the library
    callbackData->instanceIdx = (EB_U8)instanceIdx;

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
	EB_U32					instanceIdx)
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
    EB_U32          instanceIndex,
    EB_ERRORTYPE   libExitError)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_S32              ptrIndex        = 0;
    EbMemoryMapEntry*   memoryEntry     = (EbMemoryMapEntry*)EB_NULL;
    
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

    // De-init App memory table
    // Loop through the ptr table and free all malloc'd pointers per channel
    for (ptrIndex = appMemoryMapIndexAllChannels[instanceIndex] - 1; ptrIndex >= 0; --ptrIndex) {
        memoryEntry = &appMemoryMapAllChannels[instanceIndex][ptrIndex];
        switch (memoryEntry->ptrType) {
        case EB_N_PTR:
            free(memoryEntry->ptr);
            break;
        default:
            return_error = EB_ErrorMax;
            break;
        }
    }
    free(appMemoryMapAllChannels[instanceIndex]);


    return return_error;
}
