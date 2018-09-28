/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAppConfig_h
#define EbAppConfig_h

#include <stdio.h>

#include "EbApi.h"

#define EB_NORMAL_LATENCY        0
#define EB_LOW_LATENCY           1

#define MAX_CHANNEL_NUMBER      6
#define MAX_NUM_TOKENS          200


/****************************************
* Padding
****************************************/
#define LEFT_INPUT_PADDING 0
#define RIGHT_INPUT_PADDING 0
#define TOP_INPUT_PADDING 0
#define BOTTOM_INPUT_PADDING 0


typedef struct EbPerformanceContext_s {
    
       
    /****************************************
     * Computational Performance Data
     ****************************************/
    EB_U64                  totalLatency;
	EB_U32                  maxLatency;

    EB_U64                  startsTime; 
    EB_U64                  startuTime;

    EB_U64                  frameCount;

    double                  averageSpeed;
    double                  averageLatency;

    EB_U64                  byteCount;

}EbPerformanceContext_t;
   
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
    
    EB_BOOL                useQpFile;

    EB_S32                 frameRate;
    EB_S32                 frameRateNumerator;
    EB_S32                 frameRateDenominator;
    EB_S32                 injectorFrameRate;
    EB_U32                 injector;
    EB_U32                  speedControlFlag;
    EB_U32                 encoderBitDepth;
	EB_U32                 compressedTenBitFormat;
    EB_U32                 sourceWidth;
    EB_U32                 sourceHeight;

    EB_U32                 inputPaddedWidth;
    EB_U32                 inputPaddedHeight;

    EB_S64                 framesToBeEncoded;
    EB_S32                 framesEncoded;
    EB_S32                 bufferedInput;
    unsigned char         **sequenceBuffer;

    EB_U8                  latencyMode;
    
    /****************************************
     * // Interlaced Video 
     ****************************************/
    EB_BOOL                 interlacedVideo;
    EB_BOOL                 separateFields;

    /*****************************************
     * Coding Structure
     *****************************************/
    EB_U32                 baseLayerSwitchMode;
    EB_U8                  encMode;
    EB_S32                 intraPeriod;
    EB_U32                 intraRefreshType;
	EB_U32                 hierarchicalLevels;
	EB_U32                 predStructure;


    /****************************************
     * Quantization
     ****************************************/
    EB_U32                 qp;

    /****************************************
     * DLF
     ****************************************/
    EB_BOOL                disableDlfFlag;
    
    /****************************************
     * SAO
     ****************************************/
    EB_BOOL                enableSaoFlag;

    /****************************************
     * ME Tools
     ****************************************/
    EB_BOOL                useDefaultMeHme;
    EB_BOOL                enableHmeFlag;
    EB_BOOL                enableHmeLevel0Flag;
    EB_BOOL                enableHmeLevel1Flag;
    EB_BOOL                enableHmeLevel2Flag;

    /****************************************
     * ME Parameters
     ****************************************/
    EB_U32                 searchAreaWidth;
    EB_U32                 searchAreaHeight;

    /****************************************
     * HME Parameters
     ****************************************/
    EB_U32                 numberHmeSearchRegionInWidth ;
    EB_U32                 numberHmeSearchRegionInHeight;
    EB_U32                 hmeLevel0TotalSearchAreaWidth;
    EB_U32                 hmeLevel0TotalSearchAreaHeight;
    EB_U32                 hmeLevel0ColumnIndex;
    EB_U32                 hmeLevel0RowIndex;
    EB_U32                 hmeLevel1ColumnIndex;
    EB_U32                 hmeLevel1RowIndex;
    EB_U32                 hmeLevel2ColumnIndex;
    EB_U32                 hmeLevel2RowIndex;
    EB_U32                 hmeLevel0SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    EB_U32                 hmeLevel0SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    EB_U32                 hmeLevel1SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    EB_U32                 hmeLevel1SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    EB_U32                 hmeLevel2SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    EB_U32                 hmeLevel2SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];

    /****************************************
     * MD Parameters
     ****************************************/
    EB_BOOL                constrainedIntra;
      
    /****************************************
     * Rate Control
     ****************************************/
    EB_U32                 sceneChangeDetection;
    EB_U32                 rateControlMode;
    EB_U32                 lookAheadDistance;
    EB_U32                 targetBitRate;
    EB_U32                 maxQpAllowed;
    EB_U32                 minQpAllowed;

    /****************************************
    * TUNE
    ****************************************/
    EB_U8                  tune;

    /****************************************
     * Optional Features
     ****************************************/

	EB_BOOL				   bitRateReduction;
    EB_BOOL                improveSharpness;
    EB_U32                 videoUsabilityInfo;
    EB_U32                 highDynamicRangeInput;
    EB_U32                 accessUnitDelimiter;
    EB_U32                 bufferingPeriodSEI;
    EB_U32                 pictureTimingSEI;
    EB_BOOL                registeredUserDataSeiFlag;
    EB_BOOL                unregisteredUserDataSeiFlag;
    EB_BOOL                recoveryPointSeiFlag;
    EB_U32                 enableTemporalId;

    /****************************************
     * Annex A Parameters
     ****************************************/
    EB_U32                 profile;
    EB_U32                 tier;
    EB_U32                 level;

	/****************************************
     * Buffers and Fifos
     ****************************************/
    EB_U32				   inputOutputBufferFifoInitCount;             

    /****************************************
     * On-the-fly Testing 
     ****************************************/
	EB_U32                 testUserData;
	EB_BOOL				   eosFlag;

    /****************************************
    * Optimization Type
    ****************************************/
    EB_ASM					asmType;

    /****************************************
     * Computational Performance Data
     ****************************************/
    EbPerformanceContext_t  performanceContext;

    // Channel info
    EB_U32              channelId;
    EB_U32              activeChannelCount;
    EB_BOOL             useRoundRobinThreadAssignment;
    EB_U8               targetSocket;
    EB_BOOL             stopEncoder;         // to signal CTRL+C Event, need to stop encoding.
} EbConfig_t;

extern void EbConfigCtor(EbConfig_t *configPtr);
extern void EbConfigDtor(EbConfig_t *configPtr);

extern EB_ERRORTYPE	ReadCommandLine(int argc, char *const argv[], EbConfig_t **config, unsigned int  numChannels,	EB_ERRORTYPE *return_errors);
extern unsigned int     GetHelp(int argc, char *const argv[]);
extern unsigned int		GetNumberOfChannels(int argc, char *const argv[]);

#endif //EbAppConfig_h
