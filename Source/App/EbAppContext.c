/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/***************************************
 * Includes
 ***************************************/

#include <stdlib.h>

#include "EbAppContext.h"
#include "EbAppConfig.h"


#define INPUT_SIZE_576p_TH				0x90000     // 0.58 Million
#define INPUT_SIZE_1080i_TH				0xB71B0     // 0.75 Million
#define INPUT_SIZE_1080p_TH				0x1AB3F0    // 1.75 Million
#define INPUT_SIZE_4K_TH				0x29F630    // 2.75 Million
#define INPUT_SIZE_8K_TH				0xB71B00    // 12 Million

#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height,is16bit) ( ( ((width)*(height)*3)>>1 )<<is16bit)
#define IS_16_BIT(bit_depth) (bit_depth==10?1:0)
#define EB_OUTPUTSTREAMBUFFERSIZE_MACRO(ResolutionSize)                ((ResolutionSize) < (INPUT_SIZE_1080i_TH) ? 0x1E8480 : (ResolutionSize) < (INPUT_SIZE_1080p_TH) ? 0x2DC6C0 : (ResolutionSize) < (INPUT_SIZE_4K_TH) ? 0x2DC6C0 : (ResolutionSize) < (INPUT_SIZE_8K_TH) ? 0x2DC6C0:0x5B8D80)

 /***************************************
 * Variables Defining a memory table
 *  hosting all allocated pointers
 ***************************************/
EbMemoryMapEntry                 *appMemoryMap;
uint32_t                         *appMemoryMapIndex;
uint64_t                         *totalAppMemory;
uint32_t                          appMallocCount = 0;
static EbMemoryMapEntry          *appMemoryMapAllChannels[MAX_CHANNEL_NUMBER];
static uint32_t                   appMemoryMapIndexAllChannels[MAX_CHANNEL_NUMBER];
static uint64_t                   appMemoryMallocdAllChannels[MAX_CHANNEL_NUMBER];

/***************************************
* Allocation and initializing a memory table
*  hosting all allocated pointers
***************************************/
void AllocateMemoryTable(
    uint32_t	instanceIdx)
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


/*************************************
**************************************
*** Helper functions Input / Output **
**************************************
**************************************/
/******************************************************
* Copy fields from the stream to the input buffer
    Input   : stream
    Output  : valid input buffer
******************************************************/
void ProcessInputFieldBufferingMode(
    uint64_t                      processedFrameCount,
    int32_t                      *filledLen,
    FILE                         *inputFile,
    uint8_t                        *lumaInputPtr,
    uint8_t                        *cbInputPtr,
    uint8_t                        *crInputPtr,
    uint32_t                      inputPaddedWidth,
    uint32_t                      inputPaddedHeight,
    uint8_t                       is16bit) {


    uint64_t  sourceLumaRowSize   = (uint64_t)(inputPaddedWidth << is16bit);
    uint64_t  sourceChromaRowSize = sourceLumaRowSize >> 1;

    uint8_t  *ebInputPtr;
    uint32_t  inputRowIndex;

    // Y
    ebInputPtr = lumaInputPtr;
    // Skip 1 luma row if bottom field (point to the bottom field)
    if (processedFrameCount % 2 != 0)
        fseeko64(inputFile, (long)sourceLumaRowSize, SEEK_CUR);

    for (inputRowIndex = 0; inputRowIndex < inputPaddedHeight; inputRowIndex++) {

        *filledLen += (uint32_t)fread(ebInputPtr, 1, sourceLumaRowSize, inputFile);
        // Skip 1 luma row (only fields)
        fseeko64(inputFile, (long)sourceLumaRowSize, SEEK_CUR);
        ebInputPtr += sourceLumaRowSize;
    }

    // U
    ebInputPtr = cbInputPtr;
    // Step back 1 luma row if bottom field (undo the previous jump), and skip 1 chroma row if bottom field (point to the bottom field)
    if (processedFrameCount % 2 != 0) {
        fseeko64(inputFile, -(long)sourceLumaRowSize, SEEK_CUR);
        fseeko64(inputFile, (long)sourceChromaRowSize, SEEK_CUR);
    }

    for (inputRowIndex = 0; inputRowIndex < inputPaddedHeight >> 1; inputRowIndex++) {

        *filledLen += (uint32_t)fread(ebInputPtr, 1, sourceChromaRowSize, inputFile);
        // Skip 1 chroma row (only fields)
        fseeko64(inputFile, (long)sourceChromaRowSize, SEEK_CUR);
        ebInputPtr += sourceChromaRowSize;
    }

    // V
    ebInputPtr = crInputPtr;
    // Step back 1 chroma row if bottom field (undo the previous jump), and skip 1 chroma row if bottom field (point to the bottom field)
    // => no action


    for (inputRowIndex = 0; inputRowIndex < inputPaddedHeight >> 1; inputRowIndex++) {

        *filledLen += (uint32_t)fread(ebInputPtr, 1, sourceChromaRowSize, inputFile);
        // Skip 1 chroma row (only fields)
        fseeko64(inputFile, (long)sourceChromaRowSize, SEEK_CUR);
        ebInputPtr += sourceChromaRowSize;
    }

    // Step back 1 chroma row if bottom field (undo the previous jump)
    if (processedFrameCount % 2 != 0) {
        fseeko64(inputFile, -(long)sourceChromaRowSize, SEEK_CUR);
    }
}


/***********************************************
* Copy configuration parameters from
*  The config structure, to the
*  callback structure to send to the library
***********************************************/
EB_ERRORTYPE CopyConfigurationParameters(
    EbConfig_t				*config,
    EbAppContext_t			*callbackData,
    uint32_t                 instanceIdx)
{
    EB_ERRORTYPE   return_error = EB_ErrorNone;

    // Assign Instance index to the library
    callbackData->instanceIdx = (uint8_t)instanceIdx;

    // Initialize Port Activity Flags
    callbackData->outputStreamPortActive = APP_PortActive;
    callbackData->ebEncParameters.sourceWidth = config->sourceWidth;
    callbackData->ebEncParameters.sourceHeight = config->sourceHeight;
    callbackData->ebEncParameters.interlacedVideo = (EB_BOOL)config->interlacedVideo;
    callbackData->ebEncParameters.intraPeriodLength = config->intraPeriod;
    callbackData->ebEncParameters.intraRefreshType = config->intraRefreshType;
    callbackData->ebEncParameters.baseLayerSwitchMode = config->baseLayerSwitchMode;
    callbackData->ebEncParameters.encMode = (EB_BOOL)config->encMode;
    callbackData->ebEncParameters.frameRate = config->frameRate;
    callbackData->ebEncParameters.frameRateDenominator = config->frameRateDenominator;
    callbackData->ebEncParameters.frameRateNumerator = config->frameRateNumerator;
	callbackData->ebEncParameters.hierarchicalLevels = config->hierarchicalLevels;
	callbackData->ebEncParameters.predStructure = (uint8_t)config->predStructure;
    callbackData->ebEncParameters.sceneChangeDetection = config->sceneChangeDetection;
    callbackData->ebEncParameters.lookAheadDistance = config->lookAheadDistance;
    callbackData->ebEncParameters.framesToBeEncoded = config->framesToBeEncoded;
    callbackData->ebEncParameters.rateControlMode = config->rateControlMode;
    callbackData->ebEncParameters.targetBitRate = config->targetBitRate;
    callbackData->ebEncParameters.maxQpAllowed = config->maxQpAllowed;
    callbackData->ebEncParameters.minQpAllowed = config->minQpAllowed;
    callbackData->ebEncParameters.qp = config->qp;
    callbackData->ebEncParameters.useQpFile = (EB_BOOL)config->useQpFile;
#if 1//TILES
    callbackData->ebEncParameters.tileColumnCount = (EB_BOOL)config->tileColumnCount;
    callbackData->ebEncParameters.tileRowCount = (EB_BOOL)config->tileRowCount;
    callbackData->ebEncParameters.tileSliceMode = (EB_BOOL)config->tileSliceMode;
#endif
    callbackData->ebEncParameters.disableDlfFlag = (EB_BOOL)config->disableDlfFlag;
    callbackData->ebEncParameters.enableSaoFlag = (EB_BOOL)config->enableSaoFlag;
    callbackData->ebEncParameters.useDefaultMeHme = (EB_BOOL)config->useDefaultMeHme;
    callbackData->ebEncParameters.enableHmeFlag = (EB_BOOL)config->enableHmeFlag;
    callbackData->ebEncParameters.searchAreaWidth = config->searchAreaWidth;
    callbackData->ebEncParameters.searchAreaHeight = config->searchAreaHeight;
    callbackData->ebEncParameters.constrainedIntra = (EB_BOOL)config->constrainedIntra;
    callbackData->ebEncParameters.tune = config->tune;
    callbackData->ebEncParameters.channelId = config->channelId;
    callbackData->ebEncParameters.activeChannelCount = config->activeChannelCount;
    callbackData->ebEncParameters.logicalProcessors = config->logicalProcessors;
    callbackData->ebEncParameters.targetSocket = config->targetSocket;
    callbackData->ebEncParameters.unrestrictedMotionVector = config->unrestrictedMotionVector;
	callbackData->ebEncParameters.bitRateReduction = (uint8_t)config->bitRateReduction;
	callbackData->ebEncParameters.improveSharpness = (uint8_t)config->improveSharpness;
    callbackData->ebEncParameters.videoUsabilityInfo = config->videoUsabilityInfo;
    callbackData->ebEncParameters.highDynamicRangeInput = config->highDynamicRangeInput;
    callbackData->ebEncParameters.accessUnitDelimiter = config->accessUnitDelimiter;
    callbackData->ebEncParameters.bufferingPeriodSEI = config->bufferingPeriodSEI;
    callbackData->ebEncParameters.pictureTimingSEI = config->pictureTimingSEI;
    callbackData->ebEncParameters.registeredUserDataSeiFlag = config->registeredUserDataSeiFlag;
    callbackData->ebEncParameters.unregisteredUserDataSeiFlag = config->unregisteredUserDataSeiFlag;
    callbackData->ebEncParameters.recoveryPointSeiFlag = config->recoveryPointSeiFlag;
    callbackData->ebEncParameters.enableTemporalId = config->enableTemporalId;
    callbackData->ebEncParameters.encoderBitDepth = config->encoderBitDepth;
    callbackData->ebEncParameters.encoderColorFormat = (EB_COLOR_FORMAT)config->encoderColorFormat;
    callbackData->ebEncParameters.compressedTenBitFormat = config->compressedTenBitFormat;
    callbackData->ebEncParameters.profile = config->profile;
    if(config->encoderColorFormat >= EB_YUV422 && config->profile != 4)
    {
        printf("\nWarning: input profile is not correct, force converting it from %d to MainREXT for YUV422 or YUV444 cases \n", config->profile);
        callbackData->ebEncParameters.profile = 4;
    }
    else if(config->encoderBitDepth > 8 && config->profile < 2)
    {
        printf("\nWarning: input profile is not correct, force converting it from %d to Main10 for 10 bits cases\n", config->profile);
        callbackData->ebEncParameters.profile = 2;
    }
    callbackData->ebEncParameters.tier = config->tier;
    callbackData->ebEncParameters.level = config->level;
    callbackData->ebEncParameters.injectorFrameRate = config->injectorFrameRate;
    callbackData->ebEncParameters.speedControlFlag = config->speedControlFlag;
    //callbackData->ebEncParameters.latencyMode = config->latencyMode;
    callbackData->ebEncParameters.asmType = config->asmType;
    callbackData->ebEncParameters.reconEnabled = config->reconFile ? EB_TRUE : EB_FALSE;
    callbackData->ebEncParameters.codeVpsSpsPps = 1;
    callbackData->ebEncParameters.fpsInVps = config->fpsInVps;
    callbackData->ebEncParameters.switchThreadsToRtPriority = config->switchThreadsToRtPriority;

    callbackData->ebEncParameters.maxCLL = config->maxCLL;
    callbackData->ebEncParameters.maxFALL = config->maxFALL;
    callbackData->ebEncParameters.useMasteringDisplayColorVolume = config->useMasteringDisplayColorVolume;
    callbackData->ebEncParameters.dolbyVisionProfile = config->dolbyVisionProfile;
    callbackData->ebEncParameters.useNaluFile = config->useNaluFile;

    callbackData->ebEncParameters.displayPrimaryX[0] = config->displayPrimaryX[0];
    callbackData->ebEncParameters.displayPrimaryX[1] = config->displayPrimaryX[1];
    callbackData->ebEncParameters.displayPrimaryX[2] = config->displayPrimaryX[2];
    callbackData->ebEncParameters.displayPrimaryY[0] = config->displayPrimaryY[0];
    callbackData->ebEncParameters.displayPrimaryY[1] = config->displayPrimaryY[1];
    callbackData->ebEncParameters.displayPrimaryY[2] = config->displayPrimaryY[2];
    callbackData->ebEncParameters.whitePointX = config->whitePointX;
    callbackData->ebEncParameters.whitePointY = config->whitePointY;
    callbackData->ebEncParameters.maxDisplayMasteringLuminance = config->maxDisplayMasteringLuminance;
    callbackData->ebEncParameters.minDisplayMasteringLuminance = config->minDisplayMasteringLuminance;

    return return_error;

}


EB_ERRORTYPE AllocateFrameBuffer(
    EbConfig_t          *config,
    uint8_t               *pBuffer)
{
    EB_ERRORTYPE   return_error = EB_ErrorNone;

    const int32_t tenBitPackedMode = (config->encoderBitDepth > 8) && (config->compressedTenBitFormat == 0) ? 1 : 0;
    const EB_COLOR_FORMAT colorFormat = (EB_COLOR_FORMAT)config->encoderColorFormat;    // Chroma subsampling
    const uint8_t subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;

    // Determine size of each plane
    const size_t luma8bitSize =

        config->inputPaddedWidth    *
        config->inputPaddedHeight   *

        (1 << tenBitPackedMode);

    const size_t chroma8bitSize = luma8bitSize >> (3 - colorFormat);
    const size_t luma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? luma8bitSize : 0;
    const size_t chroma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? chroma8bitSize : 0;

    // Determine
    EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)pBuffer;
    inputPtr->yStride = config->inputPaddedWidth;
    inputPtr->crStride = config->inputPaddedWidth >> subWidthCMinus1;
    inputPtr->cbStride = config->inputPaddedWidth >> subWidthCMinus1;
    if (luma8bitSize) {
        EB_APP_MALLOC(uint8_t*, inputPtr->luma, luma8bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        inputPtr->luma = 0;
    }
    if (chroma8bitSize) {
        EB_APP_MALLOC(uint8_t*, inputPtr->cb, chroma8bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        inputPtr->cb = 0;
    }

    if (chroma8bitSize) {
        EB_APP_MALLOC(uint8_t*, inputPtr->cr, chroma8bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        inputPtr->cr = 0;
    }

    if (luma10bitSize) {
        EB_APP_MALLOC(uint8_t*, inputPtr->lumaExt, luma10bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        inputPtr->lumaExt = 0;
    }

    if (chroma10bitSize) {
        EB_APP_MALLOC(uint8_t*, inputPtr->cbExt, chroma10bitSize, EB_N_PTR, EB_ErrorInsufficientResources);
    }
    else {
        inputPtr->cbExt = 0;
    }

    if (chroma10bitSize) {
        EB_APP_MALLOC(uint8_t*, inputPtr->crExt, chroma10bitSize, EB_N_PTR, EB_ErrorInsufficientResources);

    }
    else {
        inputPtr->crExt = 0;
    }

    if (config->dolbyVisionProfile == 81 && config->dolbyVisionRpuFile) {
        EB_APP_MALLOC(uint8_t*, inputPtr->dolbyVisionRpu.payload, 1024, EB_N_PTR, EB_ErrorInsufficientResources);
    }

    return return_error;
}


EB_ERRORTYPE AllocateInputBuffers(
    EbConfig_t				*config,
    EbAppContext_t			*callbackData)
{
    EB_ERRORTYPE   return_error = EB_ErrorNone;
    {
        EB_APP_MALLOC(EB_BUFFERHEADERTYPE*, callbackData->inputBufferPool, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR, EB_ErrorInsufficientResources);

        // Initialize Header
        callbackData->inputBufferPool->nSize                       = sizeof(EB_BUFFERHEADERTYPE);

        EB_APP_MALLOC(uint8_t*, callbackData->inputBufferPool->pBuffer, sizeof(EB_H265_ENC_INPUT), EB_N_PTR, EB_ErrorInsufficientResources);

        if (config->bufferedInput == -1) {

            // Allocate frame buffer for the pBuffer
            AllocateFrameBuffer(
                    config,
                    callbackData->inputBufferPool->pBuffer);
        }

        // Assign the variables
        callbackData->inputBufferPool->pAppPrivate = NULL;
        callbackData->inputBufferPool->sliceType   = EB_INVALID_PICTURE;
    }

    return return_error;
}
EB_ERRORTYPE AllocateOutputReconBuffers(
    EbConfig_t				*config,
    EbAppContext_t			*callbackData)
{

    EB_ERRORTYPE   return_error = EB_ErrorNone;
    const size_t lumaSize = config->inputPaddedWidth * config->inputPaddedHeight;
    // both u and v
    const size_t chromaSize = lumaSize >> (3 - config->encoderColorFormat);
    const size_t tenBit = (config->encoderBitDepth > 8);
    const size_t frameSize = (lumaSize + 2 * chromaSize) << tenBit;

// ... Recon Port
    EB_APP_MALLOC(EB_BUFFERHEADERTYPE*, callbackData->reconBuffer, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR, EB_ErrorInsufficientResources);

    // Initialize Header
    callbackData->reconBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);

    EB_APP_MALLOC(uint8_t*, callbackData->reconBuffer->pBuffer, frameSize, EB_N_PTR, EB_ErrorInsufficientResources);

    callbackData->reconBuffer->nAllocLen = (uint32_t)frameSize;
    callbackData->reconBuffer->pAppPrivate = NULL;
    return return_error;
}

EB_ERRORTYPE AllocateOutputBuffers(
    EbConfig_t				*config,
    EbAppContext_t			*callbackData)
{

    EB_ERRORTYPE   return_error = EB_ErrorNone;
    uint32_t		   outputStreamBufferSize = (uint32_t)(EB_OUTPUTSTREAMBUFFERSIZE_MACRO(config->inputPaddedHeight * config->inputPaddedWidth));;
    {
        EB_APP_MALLOC(EB_BUFFERHEADERTYPE*, callbackData->streamBufferPool, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR, EB_ErrorInsufficientResources);

        // Initialize Header
        callbackData->streamBufferPool->nSize = sizeof(EB_BUFFERHEADERTYPE);

        EB_APP_MALLOC(uint8_t*, callbackData->streamBufferPool->pBuffer, outputStreamBufferSize, EB_N_PTR, EB_ErrorInsufficientResources);

        callbackData->streamBufferPool->nAllocLen = outputStreamBufferSize;
        callbackData->streamBufferPool->pAppPrivate = NULL;
        callbackData->streamBufferPool->sliceType = EB_INVALID_PICTURE;
    }
    return return_error;
}

EB_ERRORTYPE PreloadFramesIntoRam(
    EbConfig_t				*config)
{
    EB_ERRORTYPE    return_error = EB_ErrorNone;
    int32_t processedFrameCount;
    int32_t filledLen;
    int32_t inputPaddedWidth = config->inputPaddedWidth;
    int32_t inputPaddedHeight = config->inputPaddedHeight;
    int32_t readSize;
    uint8_t *ebInputPtr;

    FILE *inputFile = config->inputFile;

    if (config->encoderBitDepth == 10 && config->compressedTenBitFormat == 1) {
        readSize = (inputPaddedWidth*inputPaddedHeight * 3) / 2 + (inputPaddedWidth / 4 * inputPaddedHeight * 3) / 2;
    } else {
        readSize = inputPaddedWidth * inputPaddedHeight; //Luma
        readSize += 2 * (readSize >> (3 - config->encoderColorFormat)); // Add Chroma
        readSize *= (config->encoderBitDepth > 8 ? 2 : 1); //10 bit
    }
    EB_APP_MALLOC(uint8_t **, config->sequenceBuffer, sizeof(uint8_t*) * config->bufferedInput, EB_N_PTR, EB_ErrorInsufficientResources);

    for (processedFrameCount = 0; processedFrameCount < config->bufferedInput; ++processedFrameCount) {
        EB_APP_MALLOC(uint8_t*, config->sequenceBuffer[processedFrameCount], readSize, EB_N_PTR, EB_ErrorInsufficientResources);
        // Interlaced Video
        if (config->separateFields) {
            EB_BOOL is16bit = config->encoderBitDepth > 8;
            if (is16bit == 0 || (is16bit == 1 && config->compressedTenBitFormat == 0)) {
                const int32_t tenBitPackedMode = (config->encoderBitDepth > 8) && (config->compressedTenBitFormat == 0) ? 1 : 0;
                const size_t luma8bitSize =
                    (config->inputPaddedWidth) *
                    (config->inputPaddedHeight) *
                    (1 << tenBitPackedMode);

                const size_t chroma8bitSize = luma8bitSize >> 2;
                filledLen = 0;

                ProcessInputFieldBufferingMode(
                    processedFrameCount,
                    &filledLen,
                    inputFile,
                    config->sequenceBuffer[processedFrameCount],
                    config->sequenceBuffer[processedFrameCount] + luma8bitSize,
                    config->sequenceBuffer[processedFrameCount] + luma8bitSize + chroma8bitSize,
                    (uint32_t)inputPaddedWidth,
                    (uint32_t)inputPaddedHeight,
                    is16bit);

                if (readSize != filledLen) {
                    fseek(inputFile, 0, SEEK_SET);
                    filledLen = 0;

                    ProcessInputFieldBufferingMode(
                        processedFrameCount,
                        &filledLen,
                        inputFile,
                        config->sequenceBuffer[processedFrameCount],
                        config->sequenceBuffer[processedFrameCount] + luma8bitSize,
                        config->sequenceBuffer[processedFrameCount] + luma8bitSize + chroma8bitSize,
                        (uint32_t)inputPaddedWidth,
                        (uint32_t)inputPaddedHeight,
                        is16bit);
                }

                // Reset the pointer position after a top field
                if (processedFrameCount % 2 == 0) {
                    fseek(inputFile, -(long)(readSize << 1), SEEK_CUR);
                }
            }
            // Unpacked 10 bit
            else {

                const int32_t tenBitPackedMode = (config->encoderBitDepth > 8) && (config->compressedTenBitFormat == 0) ? 1 : 0;

                const size_t luma8bitSize =
                    (config->inputPaddedWidth) *
                    (config->inputPaddedHeight) *
                    (1 << tenBitPackedMode);

                const size_t chroma8bitSize = luma8bitSize >> 2;

                const size_t luma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? luma8bitSize : 0;
                const size_t chroma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? chroma8bitSize : 0;

                filledLen = 0;

                ProcessInputFieldBufferingMode(
                    processedFrameCount,
                    &filledLen,
                    inputFile,
                    config->sequenceBuffer[processedFrameCount],
                    config->sequenceBuffer[processedFrameCount] + luma8bitSize,
                    config->sequenceBuffer[processedFrameCount] + luma8bitSize + chroma8bitSize,
                    (uint32_t)inputPaddedWidth,
                    (uint32_t)inputPaddedHeight,
                    0);

                ProcessInputFieldBufferingMode(
                    processedFrameCount,
                    &filledLen,
                    inputFile,
                    config->sequenceBuffer[processedFrameCount] + luma8bitSize + (chroma8bitSize << 1),
                    config->sequenceBuffer[processedFrameCount] + luma8bitSize + (chroma8bitSize << 1) + luma10bitSize,
                    config->sequenceBuffer[processedFrameCount] + luma8bitSize + (chroma8bitSize << 1) + luma10bitSize + chroma10bitSize,
                    (uint32_t)inputPaddedWidth,
                    (uint32_t)inputPaddedHeight,
                    0);

                if (readSize != filledLen) {

                    fseek(inputFile, 0, SEEK_SET);
                    filledLen = 0;

                    ProcessInputFieldBufferingMode(
                        processedFrameCount,
                        &filledLen,
                        inputFile,
                        config->sequenceBuffer[processedFrameCount],
                        config->sequenceBuffer[processedFrameCount] + luma8bitSize,
                        config->sequenceBuffer[processedFrameCount] + luma8bitSize + chroma8bitSize,
                        (uint32_t)inputPaddedWidth,
                        (uint32_t)inputPaddedHeight,
                        0);

                    ProcessInputFieldBufferingMode(
                        processedFrameCount,
                        &filledLen,
                        inputFile,
                        config->sequenceBuffer[processedFrameCount] + luma8bitSize + (chroma8bitSize << 1),
                        config->sequenceBuffer[processedFrameCount] + luma8bitSize + (chroma8bitSize << 1) + luma10bitSize,
                        config->sequenceBuffer[processedFrameCount] + luma8bitSize + (chroma8bitSize << 1) + luma10bitSize + chroma10bitSize,
                        (uint32_t)inputPaddedWidth,
                        (uint32_t)inputPaddedHeight,
                        0);

                }

                // Reset the pointer position after a top field
                if (processedFrameCount % 2 == 0) {
                    fseek(inputFile, -(long)(readSize << 1), SEEK_CUR);
                }
            }
        }
        else {

            // Fill the buffer with a complete frame
            filledLen = 0;
            ebInputPtr = config->sequenceBuffer[processedFrameCount];
            filledLen += (uint32_t)fread(ebInputPtr, 1, readSize, inputFile);

            if (readSize != filledLen) {

                fseek(config->inputFile, 0, SEEK_SET);

                // Fill the buffer with a complete frame
                filledLen = 0;
                ebInputPtr = config->sequenceBuffer[processedFrameCount];
                filledLen += (uint32_t)fread(ebInputPtr, 1, readSize, inputFile);
            }
        }
    }

    return return_error;
}

/***************************************
* Functions Implementation
***************************************/

/***********************************
 * Initialize Core & Component
 ***********************************/
EB_ERRORTYPE InitEncoder(
    EbConfig_t				*config,
    EbAppContext_t			*callbackData,
	uint32_t				 instanceIdx)
{
    EB_ERRORTYPE        return_error = EB_ErrorNone;

    // Allocate a memory table hosting all allocated pointers
    AllocateMemoryTable(instanceIdx);

    ///************************* LIBRARY INIT [START] *********************///
    // STEP 1: Call the library to construct a Component Handle
    return_error = EbInitHandle(&callbackData->svtEncoderHandle, callbackData, &callbackData->ebEncParameters);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // STEP 3: Copy all configuration parameters into the callback structure
    return_error = CopyConfigurationParameters(
                    config,
                    callbackData,
                    instanceIdx);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // STEP 4: Send over all configuration parameters
    // Set the Parameters
    return_error = EbH265EncSetParameter(
                       callbackData->svtEncoderHandle,
                       &callbackData->ebEncParameters);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // STEP 5: Init Encoder
    return_error = EbInitEncoder(callbackData->svtEncoderHandle);

    ///************************* LIBRARY INIT [END] *********************///

    ///********************** APPLICATION INIT [START] ******************///

    // STEP 6: Allocate input buffers carrying the yuv frames in
    return_error = AllocateInputBuffers(
        config,
        callbackData);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // STEP 7: Allocate output buffers carrying the bitstream out
    return_error = AllocateOutputBuffers(
        config,
        callbackData);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // STEP 8: Allocate output Recon Buffer
    return_error = AllocateOutputReconBuffers(
        config,
        callbackData);

    if (return_error != EB_ErrorNone) {
        return return_error;
    }

	// Allocate the Sequence Buffer
    if (config->bufferedInput != -1) {

        // Preload frames into the ram for a faster yuv access time
        PreloadFramesIntoRam(
            config);
    }
    else {
        config->sequenceBuffer = 0;
    }

    if (return_error != EB_ErrorNone) {
        return return_error;
    }


    ///********************** APPLICATION INIT [END] ******************////////

    return return_error;
}

/***********************************
 * Deinit Components
 ***********************************/
EB_ERRORTYPE DeInitEncoder(
    EbAppContext_t *callbackDataPtr,
    uint32_t        instanceIndex)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    int32_t              ptrIndex        = 0;
    EbMemoryMapEntry*   memoryEntry     = (EbMemoryMapEntry*)0;

    if (((EB_COMPONENTTYPE*)(callbackDataPtr->svtEncoderHandle)) != NULL) {
            return_error = EbDeinitEncoder(callbackDataPtr->svtEncoderHandle);
    }

    // Destruct the buffer memory pool
    if (return_error != EB_ErrorNone) {
        return return_error;
    }

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

    // Destruct the component
    EbDeinitHandle(callbackDataPtr->svtEncoderHandle);

    return return_error;
}
