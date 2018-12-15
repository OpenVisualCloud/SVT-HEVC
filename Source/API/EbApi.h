/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbApi_h
#define EbApi_h

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// API Version
#define SVT_VERSION_MAJOR       1
#define SVT_VERSION_MINOR       3
#define SVT_VERSION_PATCHLEVEL  0

#define EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT     2
#define EB_HME_SEARCH_AREA_ROW_MAX_COUNT        2
    
#ifdef _WIN32
#define EB_API __declspec(dllexport)
#else
#define EB_API
#endif

/********************************
* Defines
********************************/
    
/** Assembly Types
*/
typedef enum EB_ASM {
	ASM_NON_AVX2,
	ASM_AVX2,
	ASM_TYPE_TOTAL,
	ASM_AVX512,
	ASM_TYPE_INVALID = ~0
} EB_ASM;
    
/***************************************
* Generic linked list data structure for passing data into/out from the library
***************************************/

#define EB_SLICE        unsigned int
#define B_SLICE         0
#define P_SLICE         1
#define I_SLICE         2
#define IDR_SLICE       3
#define NON_REF_SLICE   4
#define INVALID_SLICE   0xFF

typedef struct EB_BUFFERHEADERTYPE
{
    // EB_BUFFERHEADERTYPE size
    unsigned int nSize;
    
    // picture (input or output) buffer 
    unsigned char* pBuffer;
    unsigned int nFilledLen;
    unsigned int nAllocLen;

    // pic private data
    void* pAppPrivate;

    // pic timing param
    unsigned int nTickCount;
    signed long long dts;
    signed long long pts;

    // pic info
    unsigned int qpValue;
    unsigned int sliceType;

    // pic flags
    unsigned int nFlags;
} EB_BUFFERHEADERTYPE;

typedef struct EB_COMPONENTTYPE
{
    unsigned int nSize;
    void* pComponentPrivate;
    void* pApplicationPrivate;
} EB_COMPONENTTYPE;
    
typedef enum EB_ERRORTYPE
{
    EB_ErrorNone = 0,
    EB_ErrorInsufficientResources               = (signed int) 0x80001000,
    EB_ErrorUndefined                           = (signed int) 0x80001001,
    EB_ErrorInvalidComponent                    = (signed int) 0x80001004,
    EB_ErrorBadParameter                        = (signed int) 0x80001005,
    EB_ErrorDestroyThreadFailed                 = (signed int) 0x80002012,
    EB_ErrorSemaphoreUnresponsive               = (signed int) 0x80002021,
    EB_ErrorDestroySemaphoreFailed              = (signed int) 0x80002022,
    EB_ErrorCreateMutexFailed                   = (signed int) 0x80002030,
    EB_ErrorMutexUnresponsive                   = (signed int) 0x80002031,
    EB_ErrorDestroyMutexFailed                  = (signed int) 0x80002032,
    EB_NoErrorEmptyQueue                        = (signed int) 0x80002033,
    EB_ErrorMax                                 = 0x7FFFFFFF
} EB_ERRORTYPE;

#define EB_BUFFERFLAG_EOS 0x00000001 

// Display Total Memory at the end of the memory allocations
#define DISPLAY_MEMORY                                  0

// For 8-bit and 10-bit packed inputs, the luma, cb, and cr fields should be used
//   for the three input picture planes.  However, for 10-bit unpacked planes the
//   lumaExt, cbExt, and crExt fields should be used hold the extra 2-bits of 
//   precision while the luma, cb, and cr fields hold the 8-bit data.
typedef struct EB_H265_ENC_INPUT 
{
    // Hosts 8 bit or 16 bit input YUV420p / YUV420p10le
    unsigned char *luma;
    unsigned char *cb;
    unsigned char *cr;

    // Hosts LSB 2 bits of 10bit input when the compressed 10bit format is used
    unsigned char *lumaExt;
    unsigned char *cbExt;
    unsigned char *crExt;

    unsigned int   yStride;
    unsigned int   crStride;
    unsigned int   cbStride;

} EB_H265_ENC_INPUT;

// Will contain the EbEncApi which will live in the EncHandle class
// Only modifiable during config-time.
typedef struct EB_H265_ENC_CONFIGURATION
{
    // Encoding preset
    unsigned char             encMode;     // [0, 12](for tune 0 and >= 4k resolution), [0, 10](for >= 1080p resolution), [0, 9](for all resolution and modes)
    unsigned char             tune;        // encoder tuning for Visual Quality [0], PSNR/SSIM [1] 
    unsigned char             latencyMode; // lossless change

    // GOP Structure
    signed int                intraPeriodLength;
    unsigned int              intraRefreshType;
    unsigned int              hierarchicalLevels;

    unsigned char             predStructure;
    unsigned int              baseLayerSwitchMode;

    // Input Info
    unsigned int              sourceWidth;
    unsigned int              sourceHeight;
    unsigned int              frameRate;
    signed int                frameRateNumerator;
    signed int                frameRateDenominator;
    unsigned int              encoderBitDepth;
    unsigned int              compressedTenBitFormat;
    unsigned long long        framesToBeEncoded;

    // Visual quality optimizations only applicable when tune = 1
    unsigned char             bitRateReduction;
    unsigned char             improveSharpness;

    // Interlaced Video 
    unsigned char             interlacedVideo;
        
    // Quantization
    unsigned int              qp;
    unsigned char             useQpFile;

    // Deblock Filter
    unsigned char             disableDlfFlag;
    
    // SAO
    unsigned char             enableSaoFlag;

    // Motion Estimation Tools
    unsigned char             useDefaultMeHme; 
    unsigned char             enableHmeFlag;
    unsigned char             enableHmeLevel0Flag;
    unsigned char             enableHmeLevel1Flag;
    unsigned char             enableHmeLevel2Flag;

    // ME Parameters
    unsigned int              searchAreaWidth;
    unsigned int              searchAreaHeight;

    // HME Parameters
    unsigned int              numberHmeSearchRegionInWidth;
    unsigned int              numberHmeSearchRegionInHeight;
    unsigned int              hmeLevel0TotalSearchAreaWidth;
    unsigned int              hmeLevel0TotalSearchAreaHeight;
    unsigned int              hmeLevel0SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    unsigned int              hmeLevel0SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    unsigned int              hmeLevel1SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    unsigned int              hmeLevel1SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];
    unsigned int              hmeLevel2SearchAreaInWidthArray[EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT];
    unsigned int              hmeLevel2SearchAreaInHeightArray[EB_HME_SEARCH_AREA_ROW_MAX_COUNT];

    // MD Parameters
    unsigned char             constrainedIntra;

    // Rate Control
    unsigned int              rateControlMode;
    unsigned int              sceneChangeDetection;
    unsigned int              lookAheadDistance;
    unsigned int              targetBitRate;
    unsigned int              maxQpAllowed;
    unsigned int              minQpAllowed;

    // bitstream options
    unsigned char             codeVpsSpsPps;
    unsigned char             codeEosNal;
    unsigned int              videoUsabilityInfo;
    unsigned int              highDynamicRangeInput;
    unsigned int              accessUnitDelimiter;
    unsigned int              bufferingPeriodSEI;
    unsigned int              pictureTimingSEI;
    unsigned int              registeredUserDataSeiFlag;
    unsigned int              unregisteredUserDataSeiFlag;
    unsigned int              recoveryPointSeiFlag;
    unsigned int              enableTemporalId;
    unsigned int              profile;
    unsigned int              tier;
    unsigned int              level;

    // Application Specific parameters
    unsigned int              channelId;                    // when multiple instances are running within the same application
    unsigned int              activeChannelCount;           // how many channels are active
    unsigned char             useRoundRobinThreadAssignment;// create one thread on each socket [windows only]
    
    // ASM Type
    EB_ASM			          asmType;                      // level of optimization to use.

    // Demo features
    unsigned int              speedControlFlag;             // dynamically change the encoding preset to meet the average speed defined in injectorFrameRate
    signed int                injectorFrameRate;

    // Debug tools
    unsigned int              reconEnabled;

} EB_H265_ENC_CONFIGURATION;

// API calls: 

/*****************************************/
/******* STEP 1: Init the Handle *********/
/*****************************************/
EB_API EB_ERRORTYPE EbInitHandle(
    EB_COMPONENTTYPE** pHandle,
    void* pAppData,
    EB_H265_ENC_CONFIGURATION  *configPtr); // configPtr will be loaded with default params from the library

/***************************************************/
/******* STEP 2: Update the encoder params *********/
/***************************************************/
EB_API EB_ERRORTYPE EbH265EncSetParameter(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_H265_ENC_CONFIGURATION  *pComponentParameterStructure); // pComponentParameterStructure contents will be copied to the library

/***************************************************/
/******* STEP 3: Init the encoder libray ***********/
/***************************************************/
EB_API EB_ERRORTYPE EbInitEncoder(
    EB_COMPONENTTYPE *h265EncComponent);

/***************************************************/
/****** OPTIONAL: Get the stream header NAL ********/
/***************************************************/
EB_API EB_ERRORTYPE EbH265EncStreamHeader(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE*        outputStreamPtr);

/***************************************************/
/******** OPTIONAL: Get the stream EOS NAL *********/
/***************************************************/
EB_API EB_ERRORTYPE EbH265EncEosNal(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE*        outputStreamPtr);

/***************************************************/
/***** STEP 4: Send input pictures to encode *******/
/***************************************************/
EB_API EB_ERRORTYPE EbH265EncSendPicture(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE   *pBuffer);

/***************************************************/
/****** STEP 5: Get output slices to encode ********/
/***************************************************/
EB_API EB_ERRORTYPE EbH265GetPacket(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE   *pBuffer,
    unsigned char          picSendDone);

/***************************************************/
/*** OPTIONAL: Get output reconstructed picture ****/
/***************************************************/
EB_API EB_ERRORTYPE EbH265GetRecon(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE   *pBuffer);

/***************************************************/
/******* STEP 6: De-Init the encoder libray ********/
/***************************************************/
EB_API EB_ERRORTYPE EbDeinitEncoder(
    EB_COMPONENTTYPE *h265EncComponent);

/***************************************************/
/******* STEP 7: De-Init the encoder libray ********/
/***************************************************/
EB_API EB_ERRORTYPE EbDeinitHandle(
    EB_COMPONENTTYPE  *h265EncComponent);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // EbApi_h
