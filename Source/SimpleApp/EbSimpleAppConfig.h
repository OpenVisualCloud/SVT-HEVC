/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAppConfig_h
#define EbAppConfig_h

#include <stdio.h>

#include "EbApi.h"



#define MAX_CHANNEL_NUMBER      6
#define MAX_NUM_TOKENS          200


/****************************************
* Padding
****************************************/
#define LEFT_INPUT_PADDING 0
#define RIGHT_INPUT_PADDING 0
#define TOP_INPUT_PADDING 0
#define BOTTOM_INPUT_PADDING 0

   
typedef struct EbConfig_s
{        
    /****************************************
     * File I/O
     ****************************************/
    FILE                   *configFile;
    FILE                   *inputFile;
    FILE                   *bitstreamFile;
    FILE                   *errorLogFile;
    FILE                   *bufferFile;

    FILE                   *qpFile;
    
    unsigned char          useQpFile;

    signed long            frameRate;
    signed long            frameRateNumerator;
    signed long            frameRateDenominator;
    signed long            injectorFrameRate;
    unsigned int           injector;
    unsigned int           speedControlFlag;
    unsigned int           encoderBitDepth;
    unsigned int           compressedTenBitFormat;
    unsigned int           sourceWidth;
    unsigned int           sourceHeight;

    unsigned int           inputPaddedWidth;
    unsigned int           inputPaddedHeight;

    signed long            framesToBeEncoded;
    signed long            framesEncoded;
    signed long            bufferedInput;
    unsigned char         **sequenceBuffer;

    unsigned char          latencyMode;
    
    /****************************************
     * // Interlaced Video 
     ****************************************/
    unsigned char        interlacedVideo;
    unsigned char        separateFields;

    /*****************************************
     * Coding Structure
     *****************************************/
    unsigned int         baseLayerSwitchMode;
    unsigned char        encMode;
    signed long          intraPeriod;
    unsigned int         intraRefreshType;
    unsigned int         hierarchicalLevels;
    unsigned int         predStructure;


    /****************************************
     * Quantization
     ****************************************/
    unsigned int         qp;

    /****************************************
     * DLF
     ****************************************/
    unsigned char        disableDlfFlag;
    
    /****************************************
     * SAO
     ****************************************/
    unsigned char        enableSaoFlag;

    /****************************************
     * ME Tools
     ****************************************/
    unsigned char        useDefaultMeHme;
    unsigned char        enableHmeFlag;
    unsigned char        enableHmeLevel0Flag;
    unsigned char        enableHmeLevel1Flag;
    unsigned char        enableHmeLevel2Flag;

    /****************************************
     * ME Parameters
     ****************************************/
    unsigned int         searchAreaWidth;
    unsigned int         searchAreaHeight;

    /****************************************
     * HME Parameters
     ****************************************/
    unsigned int         numberHmeSearchRegionInWidth ;
    unsigned int         numberHmeSearchRegionInHeight;
    unsigned int         hmeLevel0TotalSearchAreaWidth;
    unsigned int         hmeLevel0TotalSearchAreaHeight;
    unsigned int         hmeLevel0ColumnIndex;
    unsigned int         hmeLevel0RowIndex;
    unsigned int         hmeLevel1ColumnIndex;
    unsigned int         hmeLevel1RowIndex;
    unsigned int         hmeLevel2ColumnIndex;
    unsigned int         hmeLevel2RowIndex;
    unsigned int         hmeLevel0SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    unsigned int         hmeLevel0SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    unsigned int         hmeLevel1SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    unsigned int         hmeLevel1SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    unsigned int         hmeLevel2SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    unsigned int         hmeLevel2SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];

    /****************************************
     * MD Parameters
     ****************************************/
    unsigned char        constrainedIntra;
      
    /****************************************
     * Rate Control
     ****************************************/
    unsigned int         sceneChangeDetection;
    unsigned int         rateControlMode;
    unsigned int         lookAheadDistance;
    unsigned int         targetBitRate;
    unsigned int         maxQpAllowed;
    unsigned int         minQpAllowed;

    /****************************************
    * TUNE
    ****************************************/
    unsigned char        tune;

    /****************************************
     * Optional Features
     ****************************************/

    unsigned char        bitRateReduction;
    unsigned char        improveSharpness;
    unsigned int         videoUsabilityInfo;
    unsigned int         highDynamicRangeInput;
    unsigned int         accessUnitDelimiter;
    unsigned int         bufferingPeriodSEI;
    unsigned int         pictureTimingSEI;
    unsigned char        registeredUserDataSeiFlag;
    unsigned char        unregisteredUserDataSeiFlag;
    unsigned char        recoveryPointSeiFlag;
    unsigned int         enableTemporalId;

    /****************************************
     * Annex A Parameters
     ****************************************/
    unsigned int         profile;
    unsigned int         tier;
    unsigned int         level;

	/****************************************
     * Buffers and Fifos
     ****************************************/
    unsigned int         inputOutputBufferFifoInitCount;

    /****************************************
     * On-the-fly Testing 
     ****************************************/
    unsigned int         testUserData;
    unsigned char        eosFlag;

    /****************************************
    * Optimization Type
    ****************************************/
    EB_ASM					asmType;

    // Channel info
    unsigned int      channelId;
    unsigned int      activeChannelCount;
    unsigned char     useRoundRobinThreadAssignment;
    unsigned char     targetSocket;
    unsigned char     stopEncoder;         // to signal CTRL+C Event, need to stop encoding.
} EbConfig_t;

extern void EbConfigCtor(EbConfig_t *configPtr);
extern void EbConfigDtor(EbConfig_t *configPtr);


#endif //EbAppConfig_h
