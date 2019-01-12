/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSimpleAppContext_h
#define EbSimpleAppContext_h

#include "EbApi.h"
#include <stdio.h>

typedef struct EbConfig_s
{
    /****************************************
     * File I/O
     ****************************************/
    FILE                        *configFile;
    FILE                        *inputFile;
    FILE                        *bitstreamFile;
    FILE                        *reconFile;
    FILE                        *errorLogFile;
    FILE                        *bufferFile;

    FILE                        *qpFile;

    uint8_t                      useQpFile;
#if 1//TILES
    uint8_t                      tileColumnCount;
    uint8_t                      tileRowCount ;
#endif
    int64_t                      frameRate;
    int64_t                      frameRateNumerator;
    int64_t                      frameRateDenominator;
    int64_t                      injectorFrameRate;
    uint32_t                     injector;
    uint32_t                     speedControlFlag;
    uint32_t                     encoderBitDepth;
    uint32_t                     encoderColorFormat;
    uint32_t                     compressedTenBitFormat;
    uint32_t                     sourceWidth;
    uint32_t                     sourceHeight;

    uint32_t                     inputPaddedWidth;
    uint32_t                     inputPaddedHeight;

    int64_t                      framesToBeEncoded;
    int64_t                      framesEncoded;
    int64_t                      bufferedInput;
    uint8_t                   **sequenceBuffer;

    uint8_t                     latencyMode;

    /****************************************
     * // Interlaced Video
     ****************************************/
    uint8_t                     interlacedVideo;
    uint8_t                     separateFields;

    /*****************************************
     * Coding Structure
     *****************************************/
    uint32_t                    baseLayerSwitchMode;
    uint8_t                     encMode;
    int64_t                     intraPeriod;
    uint32_t                    intraRefreshType;
    uint32_t                    hierarchicalLevels;
    uint32_t                    predStructure;


    /****************************************
     * Quantization
     ****************************************/
    uint32_t                    qp;

    /****************************************
     * DLF
     ****************************************/
    uint8_t                     disableDlfFlag;

    /****************************************
     * SAO
     ****************************************/
    uint8_t                     enableSaoFlag;

    /****************************************
     * ME Tools
     ****************************************/
    uint8_t                     useDefaultMeHme;
    uint8_t                     enableHmeFlag;

    /****************************************
     * ME Parameters
     ****************************************/
    uint32_t                    searchAreaWidth;
    uint32_t                    searchAreaHeight;

    /****************************************
     * MD Parameters
     ****************************************/
    uint8_t                     constrainedIntra;

    /****************************************
     * Rate Control
     ****************************************/
    uint32_t                    sceneChangeDetection;
    uint32_t                    rateControlMode;
    uint32_t                    lookAheadDistance;
    uint32_t                    targetBitRate;
    uint32_t                    maxQpAllowed;
    uint32_t                    minQpAllowed;

    /****************************************
    * TUNE
    ****************************************/
    uint8_t                     tune;

    /****************************************
     * Optional Features
     ****************************************/

    uint8_t                     bitRateReduction;
    uint8_t                     improveSharpness;
    uint32_t                    videoUsabilityInfo;
    uint32_t                    highDynamicRangeInput;
    uint32_t                    accessUnitDelimiter;
    uint32_t                    bufferingPeriodSEI;
    uint32_t                    pictureTimingSEI;
    uint8_t                     registeredUserDataSeiFlag;
    uint8_t                     unregisteredUserDataSeiFlag;
    uint8_t                     recoveryPointSeiFlag;
    uint32_t                    enableTemporalId;
    uint8_t                     switchThreadsToRtPriority;
    uint8_t                     fpsInVps;

    /****************************************
     * Annex A Parameters
     ****************************************/
    uint32_t                    profile;
    uint32_t                    tier;
    uint32_t                    level;

    /****************************************
     * On-the-fly Testing
     ****************************************/
    uint32_t                    testUserData;
    uint8_t                     eosFlag;

    /****************************************
    * Optimization Type
    ****************************************/
    uint32_t                    asmType;

    // Channel info
    uint32_t                    channelId;
    uint32_t                    activeChannelCount;
    uint32_t                    logicalProcessors;
    int32_t                     targetSocket;
    uint8_t                     stopEncoder;         // to signal CTRL+C Event, need to stop encoding.
} EbConfig_t;

/***************************************
 * App Callback data struct
 ***************************************/
typedef struct EbAppContext_s {
    EB_H265_ENC_CONFIGURATION           ebEncParameters;

    // Component Handle
    EB_COMPONENTTYPE*                   svtEncoderHandle;

    // Buffer Pools
    EB_BUFFERHEADERTYPE                 *inputPictureBuffer;
    EB_BUFFERHEADERTYPE                 *outputStreamBuffer;
    EB_BUFFERHEADERTYPE                 *reconBuffer;

    uint32_t instanceIdx;

} EbAppContext_t;


/********************************
 * External Function
 ********************************/
extern EB_ERRORTYPE EbAppContextCtor(EbAppContext_t *contextPtr, EbConfig_t *config);
extern void EbAppContextDtor(EbAppContext_t *contextPtr);
extern EB_ERRORTYPE InitEncoder(EbConfig_t *config, EbAppContext_t *callbackData, uint32_t instanceIdx);
extern EB_ERRORTYPE DeInitEncoder(EbAppContext_t *callbackDataPtr, uint32_t instanceIndex);

#endif // EbAppContext_h
