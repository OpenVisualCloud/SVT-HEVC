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

        // nalu info
        uint8_t  naluFound;
        uint32_t naluPOC;
        uint32_t naluPrefix;
        uint32_t naluNalType;
        uint32_t naluPayloadType;
        uint8_t* naluBase64Encode;

    } EB_BUFFERHEADERTYPE;

    typedef struct EB_COMPONENTTYPE
    {
        uint32_t nSize;
        void*    pComponentPrivate;
        void*    pApplicationPrivate;
    } EB_COMPONENTTYPE;

    typedef enum EB_ERRORTYPE
    {
        EB_ErrorNone =                      0,
        EB_ErrorInsufficientResources =     (int32_t)0x80001000,
        EB_ErrorUndefined =                 (int32_t)0x80001001,
        EB_ErrorInvalidComponent =          (int32_t)0x80001004,
        EB_ErrorBadParameter =              (int32_t)0x80001005,
        EB_ErrorDestroyThreadFailed =       (int32_t)0x80002012,
        EB_ErrorSemaphoreUnresponsive =     (int32_t)0x80002021,
        EB_ErrorDestroySemaphoreFailed =    (int32_t)0x80002022,
        EB_ErrorCreateMutexFailed =         (int32_t)0x80002030,
        EB_ErrorMutexUnresponsive =         (int32_t)0x80002031,
        EB_ErrorDestroyMutexFailed =        (int32_t)0x80002032,
        EB_NoErrorEmptyQueue =              (int32_t)0x80002033,
        EB_ErrorMax =                       0x7FFFFFFF
    } EB_ERRORTYPE;

#define EB_BUFFERFLAG_EOS 0x00000001

typedef struct EB_SEI_MESSAGE
{
    uint32_t  payloadSize;
    unsigned char  *payload;
    uint32_t  payloadType;

}EB_SEI_MESSAGE;

typedef enum EB_COLOR_FORMAT {
    EB_YUV400,
    EB_YUV420,
    EB_YUV422,
    EB_YUV444
} EB_COLOR_FORMAT;

/* For 8-bit and 10-bit packed inputs, the luma, cb, and cr fields should be used
 * for the three input picture planes. However, for 10-bit unpacked planes the
 * lumaExt, cbExt, and crExt fields should be used hold the extra 2-bits of
 * precision while the luma, cb, and cr fields hold the 8-bit data. */
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
    EB_SEI_MESSAGE    dolbyVisionRpu;

} EB_H265_ENC_INPUT;

/* Will contain the EbEncApi which will live in the EncHandle class.
 * Only modifiable during config-time. */
typedef struct EB_H265_ENC_CONFIGURATION
{
    // Encoding preset

    /* A preset defining the quality vs density tradeoff point that the encoding
     * is to be performed at. 0 is the highest quality mode, 12 is the highest
     * density mode. 
     * 
     * [0, 12] for tune 0 and >= 4k resolution.
     * [0, 10] for >= 1080p resolution.
     * [0,  9] for all resolution and modes.
     * 
     * Default is 9. */
    uint8_t                 encMode;

    /* Encoder tuning for specific aim.
     *
     * 0 = SQ - visually optimized mode.
     * 1 = OQ - PSNR / SSIM optimized mode.
     * 2 = VMAF - VMAF optimized mode. 
     * 
     * Default is 1. */
    uint8_t                 tune;

    /* Flag to enable for lower latency mode. The change is lossless. 
     * 
     * Default is 0. */
    uint8_t                 latencyMode;


    // GOP Structure

    /* The intra period defines the interval of frames after which you insert an
     * Intra refresh. It is strongly recommended to set the value to multiple of
     * 8 minus 1 the closest to 1 second (e.g. 55, 47, 31, 23 should be used for
     * 60, 50, 30, (24 or 25) respectively.
     *
     * -1 = no intra update.
     * -2 = auto.
     *
     * Deault is -2. */
    int32_t                 intraPeriodLength;

    /* Random access.
     *
     * 1 = CRA, open GOP.
     * 2 = IDR, closed GOP.
     *
     * Default is 1. */
    uint32_t                intraRefreshType;

    /* Number of hierarchical layers used to construct GOP.
     * Minigop size = 2^HierarchicalLevels.
     *
     * Default is 3. */
    uint32_t                hierarchicalLevels;

    /* Prediction structure used to construct GOP. There are two main structures
     * supported, which are: Low Delay (P or B) and Random Access.
     * 
     * In Low Delay structure, pictures within a mini GOP refer to the previously
     * encoded pictures in display order. In other words, pictures with display
     * order N can only be referenced by pictures with display order greater than
     * N, and it can only refer pictures with picture order lower than N. The Low
     * Delay structure can be flat structured (e.g. IPPPPPPP…) or hierarchically
     * structured. B/b pictures can be used instead of P/p pictures. However, the
     * reference picture list 0 and the reference picture list 1 will contain the
     * same reference picture.
     *
     * In Random Access structure, the B/b pictures can refer to reference pictures
     * from both directions (past and future).
     *
     * Default is 2. */
    uint8_t                 predStructure;

    /* Decides whether to use B picture or P picture in the base layer.
     *
     * 0 = B Picture.
     * 1 = P Picture.
     *
     * Default is 0. */
    uint32_t                baseLayerSwitchMode;


    // Input Info

    /* The width of input source in units of picture luma pixels.
     *
     * Default is 0. */
    uint32_t                sourceWidth;

    /* The height of input source in units of picture luma pixels.
     *
     * Default is 0. */
    uint32_t                sourceHeight;
 
    /* The frequecy of images being displayed. If the number is less than 1000,
     * the input frame rate is an integer number between 1 and 60, else the input
     * number is in Q16 format, shifted by 16 bits, where max allowed is 240 fps.
     * If FrameRateNumerator and FrameRateDenominator are both not equal to zero,
     * the encoder will ignore this parameter.
     *
     * Default is 25. */
    uint32_t                frameRate;

    /* Frame rate numerator. When zero, the encoder will use –fps if
     * FrameRateDenominator is also zero, otherwise an error is returned.
     *
     * Default is 0. */
    int32_t                 frameRateNumerator;

    /* Frame rate denominator. When zero, the encoder will use –fps if
     * FrameRateNumerator is also zero, otherwise an error is returned.
     *
     * Default is 0. */
    int32_t                 frameRateDenominator;

    /* Specifies the bit depth of input video.
     *
     * 8 = 8 bit.
     * 10 = 10 bit.
     *
     * Default is 8. */
    uint32_t                encoderBitDepth;

	EB_COLOR_FORMAT         encoderColorFormat;

    /* Offline packing of the 2bits: requires two bits packed input.
     *
     * Default is 0. */
    uint32_t                compressedTenBitFormat;

    /* Number of frames of sequence to be encoded. If number of frames is greater
     * than the number of frames in file, the encoder will loop to the beginning
     * and continue the encode.
     *
     * 0 = encodes the full clip.
     *
     * Default is 0. */
    uint64_t                framesToBeEncoded;


    // Visual quality optimizations only applicable when tune = 1

    /* Enables subjective quality algorithms to reduce the output bitrate with
     * minimal or no subjective visual quality impact. Only applicable to tune 0.
     * 
     * Default is 1. */
    uint8_t                 bitRateReduction;

    /* The visual quality knob that allows the use of adaptive quantization
     * within the picture and enables visual quality algorithms that improve the
     * sharpness of the background. Only available for 4k and 8k resolutions and
     * tune 0.
     *
     * Default is 1. */
    uint8_t                 improveSharpness;


    // Interlaced Video

    /* Supplemental enhancement information messages for interlaced video.
     *
     * 0 = progressive signal.
     * 1 = interlaced signal.
     *
     * Default is 0. */
    uint8_t                 interlacedVideo;


    // Quantization

    /* Initial quantization parameter for the Intra pictures used under constant
     * qp rate control mode.
     *
     * Default is 25. */
    uint32_t                qp;

    /* Path to file that contains qp values.
     * 
     * Default is null.*/
    uint8_t                 useQpFile;

#if 1//TILES
    uint8_t                 tileColumnCount;
    uint8_t                 tileRowCount;
#endif

    // Deblock Filter

    /* Flag to disable the Deblocking Loop Filtering.
     *
     * Default is 0. */
    uint8_t                 disableDlfFlag;


    // SAO

    /* Flag to enable the use of Sample Adaptive Offset Filtering.
     *
     * Default is 1. */
    uint8_t                 enableSaoFlag;


    // Motion Estimation Tools
    
    /* Flag to enable the use of default ME HME parameters.
     *
     * Default is 1. */
    uint8_t                 useDefaultMeHme;

    /* Flag to enable HME.
     *
     * Default is 1. */
    uint8_t                 enableHmeFlag;


    // ME Parameters

    /* Number of search positions in the horizontal direction.
     *
     * Default depends on input resolution. */
    uint32_t                searchAreaWidth;

    /* Number of search positions in the vertical direction.
     *
     * Default depends on input resolution. */
    uint32_t                searchAreaHeight;


    // MD Parameters

    /* Enable the use of Constrained Intra, which yields sending two picture
     * parameter sets in the elementary streams .
     *
     * Default is 0. */
    uint8_t                 constrainedIntra;


    // Rate Control
    
    /* Rate control mode.
     *
     * 0 = Constant QP.
     * 1 = Variable BitRate.
     *
     * Default is 0. */
    uint32_t                rateControlMode;

    /* Flag to enable the scene change detection algorithm.
     *
     * Default is 1. */
    uint32_t                sceneChangeDetection;

    /* When RateControlMode is set to 1 it's best to set this parameter to be
     * equal to the Intra period value (such is the default set by the encoder).
     * When CQP is chosen, then a (2 * minigopsize +1) look ahead is recommended.
     *
     * Default depends on rate control mode.*/
    uint32_t                lookAheadDistance;

    /* Target bitrate in bits/second, only apllicable when rate control mode is
     * set to 1.
     *
     * Default is 7000000. */
    uint32_t                targetBitRate;

    /* Maxium QP value allowed for rate control use, only apllicable when rate
     * control mode is set to 1. It has to be greater or equal to minQpAllowed.
     *
     * Default is 48. */
    uint32_t                maxQpAllowed;

    /* Minimum QP value allowed for rate control use, only apllicable when rate
     * control mode is set to 1. It has to be smaller or equal to maxQpAllowed.
     *
     * Default is 10. */
    uint32_t                minQpAllowed;


    // bitstream options

    /* Flag to code VPS / SPS / PPS.
     *
     * Default is 1. */
    uint8_t                 codeVpsSpsPps;

    /* Flag to code end of squence Network Abstraction Layer.
     *
     * Default is 1. */
    uint8_t                 codeEosNal;

    /* Flag to enable sending extra information to enhance the use of video for
     * display purposes.
     *
     * Default is 0. */
    uint32_t                videoUsabilityInfo;

    /* Flag to signal that the input yuv is HDR BT2020 using SMPTE ST2048, requires
     * VideoUsabilityInfo to be set to 1. Only applicable for 10bit input.
     *
     * Default is 0. */
    uint32_t                highDynamicRangeInput;

    /* Flag to simplify the detection of boundary between access units.
     * 
     * Default is 0. */
    uint32_t                accessUnitDelimiter;

    /* Flag to enable buffering period supplemental enhancement information.
     *
     * Default is 0. */
    uint32_t                bufferingPeriodSEI;

    /* Flag to enable picture timeing supplemental enhancement information.
     *
     * Default is 0. */
    uint32_t                pictureTimingSEI;

    /* Flag to enable registered user data supplemental enhancement information.
     *
     * Default is 0. */
    uint32_t                registeredUserDataSeiFlag;

    /* Flag to enable unregistered user data supplemental enhancement information.
     *
     * Default is 0. */
    uint32_t                unregisteredUserDataSeiFlag;

    /* Flag to enable recovery point supplemental enhancement information.
     *
     * Default is 0. */
    uint32_t                recoveryPointSeiFlag;

    /* Flag to insert temporal ID in Network Abstraction Layer units.
     *
     * Default is 1. */
    uint32_t                enableTemporalId;

    /* Defined set of coding tools to create bitstream.
     *
     * 1 = Main, allows bit depth of 8.
     * 2 = Main 10, allows bit depth of 8 to 10.
     *
     * Default is 2. */
    uint32_t                profile;

    /* Constraints for bitstream in terms of max bitrate and max buffer size.
     *
     * 0 = Main, for most applications.
     * 1 = High, for demanding applications.
     *
     * Default is 0. */
    uint32_t                tier;

    /* Constraints for bitstream in terms of max bitrate and max buffer size.
     *
     * 0 = auto determination.
     *
     * Default is 0. */
    uint32_t                level;

    /* Flag to enable VPS timing info.
     *
     * Default is 0. */
    uint8_t                 fpsInVps;


    // Application Specific parameters

    /* ID assigned to each channel when multiple instances are running within the
     * same application. */
    uint32_t                channelId;

    /* Active channel count. */
    uint32_t                activeChannelCount;


    // Threads management

    /* The number of logical processor which encoder threads run on. If
     * LogicalProcessorNumber and TargetSocket are not set, threads are managed by
     * OS thread scheduler. */
    uint32_t                logicalProcessors;

    /* Target socket to run on. For dual socket systems, this can specify which
     * socket the encoder runs on.
     *
     * -1 = Both Sockets.
     *  0 = Socket 0.
     *  1 = Socket 1.
     *
     * Default is -1. */
    int32_t                 targetSocket;

    /* Flag to enable threads to real time priority. Running with sudo privilege
     * utilizes full resource. Only applicable to Linux.
     *
     * Default is 1. */
    uint8_t                 switchThreadsToRtPriority;


    // ASM Type
    
    /* Assembly instruction set used by encoder.
     *
     * 0 = non-AVX2, C only.
     * 1 = up to AVX512, auto-select highest assembly insturction set supported.
     * 
     * Default is 1. */
    uint32_t                asmType;

    
    // Demo features

    /* Flag to enable the Speed Control functionality to achieve the real-time
     * encoding speed defined by dynamically changing the encoding preset to meet
     * the average speed defined in injectorFrameRate. When this parameter is set
     * to 1 it forces –inj to be 1 -inj-frm-rt to be set to the –fps.
     *
     * Default is 0. */
    uint32_t                speedControlFlag;

    /* Frame Rate used for the injector. Recommended to match the encoder speed.
     *
     * Default is 60. */
    int32_t                 injectorFrameRate;

    /* Flag to constrain motion vectors.
    *
    * 1: Motion vectors are allowed to point outside frame boundary.
    * 0: Motion vectors are NOT allowed to point outside frame boundary.
    *
    * Default is 1. */
    uint8_t                 unrestrictedMotionVector;

    // Debug tools

    /* Output reconstructed yuv used for debug purposes. The value is set through 
     * ReconFile token (-o) and using the feature will affect the speed of encoder.
     *
     * Default is 0. */
    uint32_t                reconEnabled;

    // SEI
    uint16_t                maxCLL;
    uint16_t                maxFALL;

    uint8_t                 useMasteringDisplayColorVolume;
    uint8_t                 useNaluFile;
    uint32_t                dolbyVisionProfile;

    // Master Display Color Volume Parameters
    uint16_t                displayPrimaryX[3];
    uint16_t                displayPrimaryY[3];
    uint16_t                whitePointX, whitePointY;
    uint32_t                maxDisplayMasteringLuminance;
    uint32_t                minDisplayMasteringLuminance;

} EB_H265_ENC_CONFIGURATION;


// API calls:

/* STEP 1: Call the library to construct a Component Handle.
 *
 * Parameter:
 * @ **pHandle      Handle to be called in the future for manipulating the
 *                  component.
 * @ *pAppData      Callback data.
 * @ *configPtr     Pointer passed back to the client during callbacks, it will be
 *                  loaded with default params from the library. */
EB_API EB_ERRORTYPE EbInitHandle(
    EB_COMPONENTTYPE          **pHandle,
    void                       *pAppData,
    EB_H265_ENC_CONFIGURATION  *configPtr);

/* STEP 2: Set all configuration parameters.
 *
 * Parameter:
 * @ *h265EncComponent              Encoder handler.           
 * @ *pComponentParameterStructure  Encoder and buffer configurations will be copied to the library. */
EB_API EB_ERRORTYPE EbH265EncSetParameter(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_H265_ENC_CONFIGURATION  *pComponentParameterStructure);

/* STEP 3: Initialize encoder and allocates memory to necessary buffers.
 *
 * Parameter:
 * @ *h265EncComponent  Encoder handler. */
EB_API EB_ERRORTYPE EbInitEncoder(
    EB_COMPONENTTYPE           *h265EncComponent);

/* OPTIONAL: Get VPS / SPS / PPS headers at init time.
 *
 * Parameter:
 * @ *h265EncComponent  Encoder handler. 
 * @ **outputStreamPtr  Output stream. */
EB_API EB_ERRORTYPE EbH265EncStreamHeader(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE       **outputStreamPtr);

/* OPTIONAL: Get the end of sequence Network Abstraction Layer.
 *
 * Parameter:
 * @ *h265EncComponent  Encoder handler.
 * @ **outputStreamPtr  Output stream. */
EB_API EB_ERRORTYPE EbH265EncEosNal(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE       **outputStreamPtr);

/* STEP 4: Send the picture.
 *
 * Parameter:
 * @ *h265EncComponent  Encoder handler.
 * @ *pBuffer           Header pointer, picture buffer. */
EB_API EB_ERRORTYPE EbH265EncSendPicture(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE        *pBuffer);

/* STEP 5: Receive packet.
 *
 * Parameter:
 * @ *h265EncComponent  Encoder handler.
 * @ **pBuffer          Header pointer to return packet with.
 * @ picSendDone        Flag to decide whether to call non blocking function. */
EB_API EB_ERRORTYPE EbH265GetPacket(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE       **pBuffer,
    uint8_t                     picSendDone);

/* STEP 5-1: Release output buffer back into the pool.
 *
 * Parameter:
 * @ **pBuffer          Header pointer that contains the output packet. */
EB_API void EbH265ReleaseOutBuffer(
    EB_BUFFERHEADERTYPE       **pBuffer);

/* OPTIONAL: Fill buffer with reconstructed picture.
 *
 * Parameter:
 * @ *h265EncComponent  Encoder handler.
 * @ *pBuffer           Output buffer. */
EB_API EB_ERRORTYPE EbH265GetRecon(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE        *pBuffer);

/* STEP 6: Deinitialize encoder library.
 *
 * Parameter:
 * @ *h265EncComponent  Encoder handler. */
EB_API EB_ERRORTYPE EbDeinitEncoder(
    EB_COMPONENTTYPE           *h265EncComponent);

/* STEP 7: Deconstruct encoder handler.
 *
 * Parameter:
 * @ *h265EncComponent  Encoder handler. */
EB_API EB_ERRORTYPE EbDeinitHandle(
    EB_COMPONENTTYPE           *h265EncComponent);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // EbApi_h
