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
    
#define CHKN_OMX        1
#define DEADLOCK_DEBUG  0
#define CHKN_EOS        1
#define ONE_MEMCPY      1

#define EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT     2
#define EB_HME_SEARCH_AREA_ROW_MAX_COUNT        2
    
#ifdef _WIN32
#define EB_API __declspec(dllexport)
#else
#ifdef __EB_EXPORTS
#define EB_API
#else
#define EB_API extern
#endif
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
#define       EB_CONFIG_ON_FLY_PIC_QP        219

typedef int   EB_LINKED_LIST_TYPE;
typedef struct EbLinkedListNode
{
	void*                     app;                       // points to an application object this node is associated
	// with. this is an opaque pointer to the encoder lib, but
	// releaseCbFncPtr may need to access it.
	EB_LINKED_LIST_TYPE       type;                      // type of data pointed by "data" member variable
	unsigned int                    size;                      // size of (data)
	unsigned char                   passthrough;               // whether this is passthrough data from application
	void(*releaseCbFncPtr)(struct EbLinkedListNode*); // callback to be executed by encoder when picture reaches end of pipeline, or
	// when aborting. However, at end of pipeline encoder shall
	// NOT invoke this callback if passthrough is TRUE (but
	// still needs to do so when aborting)
	void                     *data;                      // pointer to application's data
	struct EbLinkedListNode  *next;                      // pointer to next node (null when last)

} EbLinkedListNode;

#define EB_SLICE        unsigned int
#define B_SLICE         0
#define P_SLICE         1
#define I_SLICE         2
#define IDR_SLICE       3
#define NON_REF_SLICE   4
#define INVALID_SLICE   0xFF

typedef struct EB_BUFFERHEADERTYPE
{
    unsigned int nSize;
    unsigned char* pBuffer;
    unsigned int nAllocLen;
    unsigned int nFilledLen;
    unsigned int nOffset;
    void* pAppPrivate;
    unsigned int nTickCount;
    signed long long dts;
    signed long long pts;
    unsigned int nFlags;
    unsigned int qpValue;
    unsigned int sliceType;
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
    unsigned char *luma;
    unsigned char *cb;
    unsigned char *cr;
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
    // Define the settings

    // Channel info
    unsigned int              channelId;
    unsigned int              activeChannelCount;
    unsigned char             useRoundRobinThreadAssignment;
    
    // GOP Structure
    signed int              intraPeriodLength;
    unsigned int              intraRefreshType;

    unsigned char               predStructure;
    unsigned int              baseLayerSwitchMode;
    unsigned char               encMode;

    unsigned int              hierarchicalLevels;


    unsigned int             sourceWidth;
    unsigned int             sourceHeight;

    unsigned char              latencyMode;


    // Interlaced Video 
    unsigned char             interlacedVideo;
        
    // Quantization
    unsigned int              qp;
    unsigned char             useQpFile;

    // Deblock Filter
    unsigned char             disableDlfFlag;
    
    // SAO
    unsigned char             enableSaoFlag;

    // ME Tools
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
    unsigned int              frameRate;
    signed int              frameRateNumerator;
    signed int              frameRateDenominator;
    unsigned int              encoderBitDepth;
    unsigned int              compressedTenBitFormat;
    unsigned int              rateControlMode;
    unsigned int              sceneChangeDetection;
    unsigned int              lookAheadDistance;
    unsigned long long        framesToBeEncoded;
    unsigned int              targetBitRate;
    unsigned int              maxQpAllowed;
    unsigned int              minQpAllowed;

    unsigned char               tune;

	unsigned char				bitRateReduction;
    // Tresholds
    unsigned char             improveSharpness;
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

    // Debug tools
    unsigned int              reconEnabled;

	// Buffer Configuration
    unsigned int			  inputOutputBufferFifoInitCount;              // add check for minimum 

    signed int                injectorFrameRate;
    unsigned int              speedControlFlag;
    
    // ASM Type
    EB_ASM			          asmType;

    unsigned char             codeVpsSpsPps;

} EB_H265_ENC_CONFIGURATION;

EB_API EB_ERRORTYPE EbInitEncoder(
    EB_COMPONENTTYPE *h265EncComponent);

EB_API EB_ERRORTYPE EbDeinitEncoder(
    EB_COMPONENTTYPE *h265EncComponent);

EB_API EB_ERRORTYPE EbH265EncSetParameter(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_H265_ENC_CONFIGURATION  *pComponentParameterStructure);

EB_API EB_ERRORTYPE EbH265EncStreamHeader(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE*        outputStreamPtr);

EB_API EB_ERRORTYPE EbH265EncSendPicture(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE   *pBuffer);

EB_API EB_ERRORTYPE EbH265GetPacket(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE   *pBuffer,
    unsigned char          picSendDone);

EB_API EB_ERRORTYPE EbH265GetRecon(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE   *pBuffer);

EB_API EB_ERRORTYPE EbInitHandle(
    EB_COMPONENTTYPE** pHandle,
    void* pAppData,
    EB_H265_ENC_CONFIGURATION  *configPtr);

EB_API EB_ERRORTYPE EbDeinitHandle(
    EB_COMPONENTTYPE  *h265EncComponent);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // EbApi_h
