/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbApi_h
#define EbApi_h

#include "EbTypes.h"
#include "EbApiSei.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT     2
#define EB_HME_SEARCH_AREA_ROW_MAX_COUNT        2

/********************************
* Defines
********************************/
#define EB_ENCODERINPUTPORT                     0
#define EB_ENCODERSTREAMPORT                    1
#define EB_ENCODERPORTCOUNT                     4 

    
/***************************************
* Input Bitstream Context
***************************************/
typedef struct InputBitstreamContext_s {

    EB_U64  processedByteCount;
    EB_U64  processedFrameCount;

    EB_S64  previousTimeSeconds;
    double  measuredFrameRate;

} InputBitstreamContext_t;
// For 8-bit and 10-bit packed inputs, the luma, cb, and cr fields should be used
//   for the three input picture planes.  However, for 10-bit unpacked planes the
//   lumaExt, cbExt, and crExt fields should be used hold the extra 2-bits of 
//   precision while the luma, cb, and cr fields hold the 8-bit data.
typedef struct EB_H265_ENC_INPUT 
{
    unsigned char *luma;
    unsigned char *cb;
    unsigned char *cr;
    unsigned char *lumaExt;
    unsigned char *cbExt;
    unsigned char *crExt;

    // Local Contexts
    InputBitstreamContext_t             inputContext;
} EB_H265_ENC_INPUT;

// Will contain the EbEncApi which will live in the EncHandle class
// Only modifiable during config-time.
typedef struct EB_H265_ENC_CONFIGURATION
{
    // Define the settings

    // Channel info
    EB_U32              channelId;
    EB_U32              activeChannelCount;
    EB_BOOL             useRoundRobinThreadAssignment;
    
    // GOP Structure
    EB_S32              intraPeriodLength;
    EB_U32              intraRefreshType;

    EB_PRED             predStructure;
    EB_U32              baseLayerSwitchMode;
    EB_U8               encMode;

    EB_U32              hierarchicalLevels;


    // Input Stride
    EB_U32             inputPictureStride; // Includes padding

    EB_U32             sourceWidth;
    EB_U32             sourceHeight;

    EB_U8              latencyMode;


    // Interlaced Video 
    EB_BOOL             interlacedVideo;
        
    // Quantization
    EB_U32              qp;
    EB_BOOL             useQpFile;

    // Deblock Filter
    EB_BOOL             disableDlfFlag;
    
    // SAO
    EB_BOOL             enableSaoFlag;

    // ME Tools
    EB_BOOL             useDefaultMeHme; 
    EB_BOOL             enableHmeFlag;
    EB_BOOL             enableHmeLevel0Flag;
    EB_BOOL             enableHmeLevel1Flag;
    EB_BOOL             enableHmeLevel2Flag;

    // ME Parameters
    EB_U32              searchAreaWidth;
    EB_U32              searchAreaHeight;

    // HME Parameters
    EB_U32              numberHmeSearchRegionInWidth;
    EB_U32              numberHmeSearchRegionInHeight;
    EB_U32              hmeLevel0TotalSearchAreaWidth;
    EB_U32              hmeLevel0TotalSearchAreaHeight;
    EB_U32              hmeLevel0SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    EB_U32              hmeLevel0SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    EB_U32              hmeLevel1SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    EB_U32              hmeLevel1SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    EB_U32              hmeLevel2SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    EB_U32              hmeLevel2SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];

    // MD Parameters
    EB_BOOL             constrainedIntra;

    // Rate Control
    EB_U32              frameRate;
    EB_S32              frameRateNumerator;
    EB_S32              frameRateDenominator;
    EB_U32              encoderBitDepth;
    EB_U32              compressedTenBitFormat;
    EB_U32              rateControlMode;
    EB_U32              sceneChangeDetection;
    EB_U32              lookAheadDistance;
    EB_U64              framesToBeEncoded;
    EB_U32              targetBitRate;
    EB_U32              maxQpAllowed;
    EB_U32              minQpAllowed;

    EB_U8               tune;

	EB_BOOL				bitRateReduction;
    // Tresholds
    EB_BOOL             improveSharpness;
    EB_U32              videoUsabilityInfo;
    EB_U32              highDynamicRangeInput;
    EB_U32              accessUnitDelimiter;
    EB_U32              bufferingPeriodSEI;
    EB_U32              pictureTimingSEI;
    EB_U32              registeredUserDataSeiFlag;
    EB_U32              unregisteredUserDataSeiFlag;
    EB_U32              recoveryPointSeiFlag;
    EB_U32              enableTemporalId;
    EB_U32              profile;
    EB_U32              tier;
    EB_U32              level;

	// Buffer Configuration
    EB_U32				inputOutputBufferFifoInitCount;              // add check for minimum 

    EB_S32              injectorFrameRate;
    EB_U32              speedControlFlag;
    
    // ASM Type
    EB_ASM			    asmType;

} EB_H265_ENC_CONFIGURATION;

#ifdef _WIN32
#ifdef __EB_EXPORTS
#define EB_API __declspec(dllexport)
#else
#define EB_API __declspec(dllimport)
#endif
#else
#ifdef __EB_EXPORTS
#define EB_API
#else
#define EB_API extern
#endif
#endif

EB_API EB_ERRORTYPE EbInitEncoder(
    EB_HANDLETYPE  hComponent);

EB_API EB_ERRORTYPE EbDeinitEncoder(
    EB_HANDLETYPE  hComponent);

EB_API EB_ERRORTYPE EbStartEncoder(
    EB_HANDLETYPE  hComponent,
    EB_U32         instanceIndex);

EB_API EB_ERRORTYPE EbStopEncoder(
    EB_HANDLETYPE  hComponent,
    EB_U32         instanceIndex);

EB_API EB_ERRORTYPE EbH265EncInitParameter(
    EB_PTR                 pComponentParameterStructure);

EB_API EB_ERRORTYPE EbH265EncSetParameter(
    EB_HANDLETYPE          hComponent,
    EB_PTR                 pComponentParameterStructure);

EB_API EB_ERRORTYPE EbH265EncSendPicture(
    EB_HANDLETYPE          hComponent,
    EB_BUFFERHEADERTYPE   *pBuffer);

EB_API EB_ERRORTYPE EbH265GetPacket(
    EB_HANDLETYPE          hComponent,
    EB_BUFFERHEADERTYPE   *pBuffer);

EB_API EB_ERRORTYPE EbInitHandle(
    EB_HANDLETYPE* pHandle,
    EB_PTR pAppData);

EB_API EB_ERRORTYPE EbDeinitHandle(
    EB_HANDLETYPE hComponent);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // EbApi_h
