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
#define EB_PICTURE           uint32_t
#define EB_B_PICTURE         0
#define EB_P_PICTURE         1
#define EB_I_PICTURE         2
#define EB_IDR_PICTURE       3
#define EB_NON_REF_PICTURE   4
#define EB_INVALID_PICTURE   0xFF

    typedef struct EB_BUFFERHEADERTYPE
    {
        // EB_BUFFERHEADERTYPE size
        uint32_t nSize;

        // picture (input or output) buffer
        uint8_t* pBuffer;
        uint32_t nFilledLen;
        uint32_t nAllocLen;

        // pic private data
        void*    pAppPrivate;
        void*    wrapperPtr;

        // pic timing param
        uint32_t nTickCount;
        int64_t  dts;
        int64_t  pts;

        // pic info
        uint32_t qpValue;
        uint32_t sliceType;

        // pic flags
        uint32_t nFlags;
    } EB_BUFFERHEADERTYPE;

    typedef struct EB_COMPONENTTYPE
    {
        uint32_t nSize;
        void* pComponentPrivate;
        void* pApplicationPrivate;
    } EB_COMPONENTTYPE;

    typedef enum EB_ERRORTYPE
    {
        EB_ErrorNone = 0,
        EB_ErrorInsufficientResources = (int32_t)0x80001000,
        EB_ErrorUndefined = (int32_t)0x80001001,
        EB_ErrorInvalidComponent = (int32_t)0x80001004,
        EB_ErrorBadParameter = (int32_t)0x80001005,
        EB_ErrorDestroyThreadFailed = (int32_t)0x80002012,
        EB_ErrorSemaphoreUnresponsive = (int32_t)0x80002021,
        EB_ErrorDestroySemaphoreFailed = (int32_t)0x80002022,
        EB_ErrorCreateMutexFailed = (int32_t)0x80002030,
        EB_ErrorMutexUnresponsive = (int32_t)0x80002031,
        EB_ErrorDestroyMutexFailed = (int32_t)0x80002032,
        EB_NoErrorEmptyQueue = (int32_t)0x80002033,
        EB_ErrorMax = 0x7FFFFFFF
    } EB_ERRORTYPE;

#define EB_BUFFERFLAG_EOS 0x00000001
// For 8-bit and 10-bit packed inputs, the luma, cb, and cr fields should be used
//   for the three input picture planes.  However, for 10-bit unpacked planes the
//   lumaExt, cbExt, and crExt fields should be used hold the extra 2-bits of
//   precision while the luma, cb, and cr fields hold the 8-bit data.
typedef struct EB_H265_ENC_INPUT
{
    // Hosts 8 bit or 16 bit input YUV420p / YUV420p10le
    uint8_t *luma;
    uint8_t *cb;
    uint8_t *cr;

    // Hosts LSB 2 bits of 10bit input when the compressed 10bit format is used
    uint8_t *lumaExt;
    uint8_t *cbExt;
    uint8_t *crExt;

    uint32_t yStride;
    uint32_t crStride;
    uint32_t cbStride;

} EB_H265_ENC_INPUT;

// Will contain the EbEncApi which will live in the EncHandle class
// Only modifiable during config-time.
typedef struct EB_H265_ENC_CONFIGURATION
{
    // Encoding preset
    uint8_t                 encMode;     // [0, 12](for tune 0 and >= 4k resolution), [0, 10](for >= 1080p resolution), [0, 9](for all resolution and modes)
    uint8_t                 tune;        // encoder tuning for Visual Quality [0], PSNR/SSIM [1]
    uint8_t                 latencyMode; // lossless change

    // GOP Structure
    int32_t                 intraPeriodLength;
    uint32_t                intraRefreshType;
    uint32_t                hierarchicalLevels;

    uint8_t                 predStructure;
    uint32_t                baseLayerSwitchMode;

    // Input Info
    uint32_t                sourceWidth;
    uint32_t                sourceHeight;
    uint32_t                frameRate;
    int32_t                 frameRateNumerator;
    int32_t                 frameRateDenominator;
    uint32_t                encoderBitDepth;
    uint32_t                compressedTenBitFormat;
    uint64_t                framesToBeEncoded;

    // Visual quality optimizations only applicable when tune = 1
    uint8_t                 bitRateReduction;
    uint8_t                 improveSharpness;

    // Interlaced Video
    uint8_t                 interlacedVideo;

    // Quantization
    uint32_t                qp;
    uint8_t                 useQpFile;

    // Deblock Filter
    uint8_t                 disableDlfFlag;

    // SAO
    uint8_t                 enableSaoFlag;

    // Motion Estimation Tools
    uint8_t                 useDefaultMeHme;
    uint8_t                 enableHmeFlag;

    // ME Parameters
    uint32_t                searchAreaWidth;
    uint32_t                searchAreaHeight;

    // MD Parameters
    uint8_t                 constrainedIntra;

    // Rate Control
    uint32_t                rateControlMode;
    uint32_t                sceneChangeDetection;
    uint32_t                lookAheadDistance;
    uint32_t                targetBitRate;
    uint32_t                maxQpAllowed;
    uint32_t                minQpAllowed;

    // bitstream options
    uint8_t                 codeVpsSpsPps;
    uint8_t                 codeEosNal;
    uint32_t                videoUsabilityInfo;
    uint32_t                highDynamicRangeInput;
    uint32_t                accessUnitDelimiter;
    uint32_t                bufferingPeriodSEI;
    uint32_t                pictureTimingSEI;
    uint32_t                registeredUserDataSeiFlag;
    uint32_t                unregisteredUserDataSeiFlag;
    uint32_t                recoveryPointSeiFlag;
    uint32_t                enableTemporalId;
    uint32_t                profile;
    uint32_t                tier;
    uint32_t                level;
    uint8_t                 fpsInVps;

    // Application Specific parameters
    uint32_t                channelId;                    // when multiple instances are running within the same application
    uint32_t                activeChannelCount;           // how many channels are active

  // Threads management
    uint32_t                logicalProcessors;             // number of logical processor to run on
    int32_t                 targetSocket;                  // target socket to run on

    // ASM Type
    uint32_t                asmType;                      // level of optimization to use.

    // Demo features
    uint32_t                speedControlFlag;             // dynamically change the encoding preset to meet the average speed defined in injectorFrameRate
    int32_t                 injectorFrameRate;

    // Debug tools
    uint32_t                reconEnabled;

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
    EB_BUFFERHEADERTYPE       **outputStreamPtr);

/***************************************************/
/******** OPTIONAL: Get the stream EOS NAL *********/
/***************************************************/
EB_API EB_ERRORTYPE EbH265EncEosNal(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE       **outputStreamPtr);

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
    EB_BUFFERHEADERTYPE  **pBuffer,
    uint8_t          picSendDone);

/***************************************************/
/******* STEP 5-1: Release output buffer ***********/
/***************************************************/
EB_API void EbH265ReleaseOutBuffer(
    EB_BUFFERHEADERTYPE  **pBuffer);

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
