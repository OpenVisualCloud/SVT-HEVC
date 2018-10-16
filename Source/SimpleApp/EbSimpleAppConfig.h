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
    
    unsigned __int8        useQpFile;

    signed __int64         frameRate;
    signed __int64         frameRateNumerator;
    signed __int64         frameRateDenominator;
    signed __int64         injectorFrameRate;
    unsigned __int32       injector;
    unsigned __int32       speedControlFlag;
    unsigned __int32       encoderBitDepth;
	unsigned __int32       compressedTenBitFormat;
    unsigned __int32       sourceWidth;
    unsigned __int32       sourceHeight;

    unsigned __int32       inputPaddedWidth;
    unsigned __int32       inputPaddedHeight;

    signed __int64         framesToBeEncoded;
    signed __int64         framesEncoded;
    signed __int64         bufferedInput;
    unsigned char         **sequenceBuffer;

    unsigned __int8        latencyMode;
    
    /****************************************
     * // Interlaced Video 
     ****************************************/
    unsigned __int8        interlacedVideo;
    unsigned __int8        separateFields;

    /*****************************************
     * Coding Structure
     *****************************************/
    unsigned __int32       baseLayerSwitchMode;
    unsigned __int8        encMode;
    signed __int64         intraPeriod;
    unsigned __int32       intraRefreshType;
	unsigned __int32       hierarchicalLevels;
	unsigned __int32       predStructure;


    /****************************************
     * Quantization
     ****************************************/
    unsigned __int32       qp;

    /****************************************
     * DLF
     ****************************************/
    unsigned __int8        disableDlfFlag;
    
    /****************************************
     * SAO
     ****************************************/
    unsigned __int8        enableSaoFlag;

    /****************************************
     * ME Tools
     ****************************************/
    unsigned __int8        useDefaultMeHme;
    unsigned __int8        enableHmeFlag;
    unsigned __int8        enableHmeLevel0Flag;
    unsigned __int8        enableHmeLevel1Flag;
    unsigned __int8        enableHmeLevel2Flag;

    /****************************************
     * ME Parameters
     ****************************************/
    unsigned __int32       searchAreaWidth;
    unsigned __int32       searchAreaHeight;

    /****************************************
     * HME Parameters
     ****************************************/
    unsigned __int32       numberHmeSearchRegionInWidth ;
    unsigned __int32       numberHmeSearchRegionInHeight;
    unsigned __int32       hmeLevel0TotalSearchAreaWidth;
    unsigned __int32       hmeLevel0TotalSearchAreaHeight;
    unsigned __int32       hmeLevel0ColumnIndex;
    unsigned __int32       hmeLevel0RowIndex;
    unsigned __int32       hmeLevel1ColumnIndex;
    unsigned __int32       hmeLevel1RowIndex;
    unsigned __int32       hmeLevel2ColumnIndex;
    unsigned __int32       hmeLevel2RowIndex;
    unsigned __int32       hmeLevel0SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    unsigned __int32       hmeLevel0SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    unsigned __int32       hmeLevel1SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    unsigned __int32       hmeLevel1SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    unsigned __int32       hmeLevel2SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    unsigned __int32       hmeLevel2SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];

    /****************************************
     * MD Parameters
     ****************************************/
    unsigned __int8        constrainedIntra;
      
    /****************************************
     * Rate Control
     ****************************************/
    unsigned __int32       sceneChangeDetection;
    unsigned __int32       rateControlMode;
    unsigned __int32       lookAheadDistance;
    unsigned __int32       targetBitRate;
    unsigned __int32       maxQpAllowed;
    unsigned __int32       minQpAllowed;

    /****************************************
    * TUNE
    ****************************************/
    unsigned __int8        tune;

    /****************************************
     * Optional Features
     ****************************************/

	unsigned __int8        bitRateReduction;
    unsigned __int8        improveSharpness;
    unsigned __int32       videoUsabilityInfo;
    unsigned __int32       highDynamicRangeInput;
    unsigned __int32       accessUnitDelimiter;
    unsigned __int32       bufferingPeriodSEI;
    unsigned __int32       pictureTimingSEI;
    unsigned __int8        registeredUserDataSeiFlag;
    unsigned __int8        unregisteredUserDataSeiFlag;
    unsigned __int8        recoveryPointSeiFlag;
    unsigned __int32       enableTemporalId;

    /****************************************
     * Annex A Parameters
     ****************************************/
    unsigned __int32       profile;
    unsigned __int32       tier;
    unsigned __int32       level;

	/****************************************
     * Buffers and Fifos
     ****************************************/
    unsigned __int32       inputOutputBufferFifoInitCount;

    /****************************************
     * On-the-fly Testing 
     ****************************************/
    unsigned __int32       testUserData;
	unsigned __int8        eosFlag;

    /****************************************
    * Optimization Type
    ****************************************/
    EB_ASM					asmType;

    // Channel info
    unsigned __int32    channelId;
    unsigned __int32    activeChannelCount;
    unsigned __int8     useRoundRobinThreadAssignment;
    unsigned __int8     targetSocket;
    unsigned __int8     stopEncoder;         // to signal CTRL+C Event, need to stop encoding.
} EbConfig_t;

extern void EbConfigCtor(EbConfig_t *configPtr);
extern void EbConfigDtor(EbConfig_t *configPtr);


#endif //EbAppConfig_h
