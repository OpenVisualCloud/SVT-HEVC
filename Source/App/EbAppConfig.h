/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbAppConfig_h
#define EbAppConfig_h

#include <stdio.h>

#include "EbApi.h"

#ifdef __GNUC__
#define fseeko64 fseek
#define ftello64 ftell
#endif
// Define Cross-Platform 64-bit fseek() and ftell()
#ifdef _MSC_VER
typedef __int64 off64_t;
#define fseeko64 _fseeki64
#define ftello64 _ftelli64

#elif _WIN32 // MinGW

#endif

#ifndef _RSIZE_T_DEFINED
typedef size_t rsize_t;
#define _RSIZE_T_DEFINED
#endif  /* _RSIZE_T_DEFINED */

#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int32_t errno_t;
#endif  /* _ERRNO_T_DEFINED */

/** The APPEXITCONDITIONTYPE type is used to define the App main loop exit
conditions.
*/
typedef enum APPEXITCONDITIONTYPE {
    APP_ExitConditionNone = 0,
    APP_ExitConditionFinished,
    APP_ExitConditionError
} APPEXITCONDITIONTYPE;

/** The APPPORTACTIVETYPE type is used to define the state of output ports in
the App.
*/
typedef enum APPPORTACTIVETYPE {
    APP_PortActive = 0,
    APP_PortInactive
} APPPORTACTIVETYPE;

typedef enum EbPtrType {
    EB_N_PTR = 0,                                   // malloc'd pointer
    EB_A_PTR = 1,                                   // malloc'd pointer aligned
    EB_MUTEX = 2,                                   // mutex
    EB_SEMAPHORE = 3,                                   // semaphore
    EB_THREAD = 4                                    // thread handle
}EbPtrType;

/** The EB_PTR type is intended to be used to pass pointers to and from the svt
API.  This is a 32 bit pointer and is aligned on a 32 bit word boundary.
*/
typedef void * EB_PTR;

/** The EB_NULL type is used to define the C style NULL pointer.
*/
#define EB_NULL ((void*) 0)

typedef struct EbMemoryMapEntry
{
    EB_PTR                    ptr;                       // points to a memory pointer
    EbPtrType                 ptrType;                   // pointer type
} EbMemoryMapEntry;

// *Note - This work around is needed for the windows visual studio compiler
//  (MSVC) because it doesn't support the C99 header file stdint.h.
//  All other compilers should support the stdint.h C99 standard types.

/** The EB_BOOL type is intended to be used to represent a true or a false
value when passing parameters to and from the svt API.  The
EB_BOOL is a 32 bit quantity and is aligned on a 32 bit word boundary.
*/

#define EB_BOOL   uint8_t
#define EB_FALSE  0
#define EB_TRUE   1

extern    EbMemoryMapEntry        *appMemoryMap;            // App Memory table
extern    uint32_t                  *appMemoryMapIndex;       // App Memory index
extern    uint64_t                  *totalAppMemory;          // App Memory malloc'd
extern    uint32_t                   appMallocCount;

typedef struct EB_PARAM_PORTDEFINITIONTYPE {
    uint32_t nFrameWidth;
    uint32_t nFrameHeight;
    int32_t nStride;
    uint32_t nSize;
} EB_PARAM_PORTDEFINITIONTYPE;

#define MAX_APP_NUM_PTR                             (0x186A0 << 2)             // Maximum number of pointers to be allocated for the app

#define EB_APP_MALLOC(type, pointer, nElements, pointerClass, returnType) \
    pointer = (type)malloc(nElements); \
    if (pointer == (type)EB_NULL){ \
        return returnType; \
		    } \
			    else { \
        appMemoryMap[*(appMemoryMapIndex)].ptrType = pointerClass; \
        appMemoryMap[(*(appMemoryMapIndex))++].ptr = pointer; \
		if (nElements % 8 == 0) { \
			*totalAppMemory += (nElements); \
						} \
								else { \
			*totalAppMemory += ((nElements) + (8 - ((nElements) % 8))); \
			} \
	    } \
    if (*(appMemoryMapIndex) >= MAX_APP_NUM_PTR) { \
        return returnType; \
		        } \
    appMallocCount++;

#define EB_APP_MALLOC_NR(type, pointer, nElements, pointerClass,returnType) \
    (void)returnType; \
    pointer = (type)malloc(nElements); \
    if (pointer == (type)EB_NULL){ \
        returnType = EB_ErrorInsufficientResources; \
        printf("Malloc has failed due to insuffucient resources"); \
        return; \
		    } \
			    else { \
        appMemoryMap[*(appMemoryMapIndex)].ptrType = pointerClass; \
        appMemoryMap[(*(appMemoryMapIndex))++].ptr = pointer; \
		if (nElements % 8 == 0) { \
			*totalAppMemory += (nElements); \
						} \
								else { \
			*totalAppMemory += ((nElements) + (8 - ((nElements) % 8))); \
			} \
	    } \
    if (*(appMemoryMapIndex) >= MAX_APP_NUM_PTR) { \
        returnType = EB_ErrorInsufficientResources; \
        printf("Malloc has failed due to insuffucient resources"); \
        return; \
		        } \
    appMallocCount++;

/* string copy */
extern errno_t strcpy_ss(char *dest, rsize_t dmax, const char *src);

/* fitted string copy */
extern errno_t strncpy_ss(char *dest, rsize_t dmax, const char *src, rsize_t slen);

/* string length */
extern rsize_t strnlen_ss(const char *s, rsize_t smax);

#define EB_STRNCPY(dst, src, count) \
	strncpy_ss(dst, sizeof(dst), src, count)

#define EB_STRCPY(dst, size, src) \
	strcpy_ss(dst, size, src)

#define EB_STRCMP(target,token) \
	strcmp(target,token)

#define EB_STRLEN(target, max_size) \
	strnlen_ss(target, max_size)

#define EB_APP_MEMORY() \
    printf("Total Number of Mallocs in App: %d\n", appMallocCount); \
    printf("Total App Memory: %.2lf KB\n\n",*totalAppMemory/(double)1024);

#define MAX_CHANNEL_NUMBER      6
#define MAX_NUM_TOKENS          200

#define MAX_STRING_LENGTH       1024

#ifdef _MSC_VER
#define FOPEN(f,s,m) fopen_s(&f,s,m)
#else
#define FOPEN(f,s,m) f=fopen(s,m)
#endif

#ifdef _MSC_VER
#define EB_STRTOK(str,delim,next) strtok_s((char*)str,(const char*)delim,(char**)next)
#else
#define EB_STRTOK(str,delim,next) strtok_r((char*)str,(const char*)delim,(char**)next)
#endif

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
    uint64_t                  libStartTime[2];       // [sec, micro_sec] including init time
    uint64_t                  encodeStartTime[2];    // [sec, micro_sec] first frame sent

    double                    totalExecutionTime;    // includes init
    double                    totalEncodeTime;       // not including init

    uint64_t                  totalLatency;
	uint32_t                  maxLatency;

    uint64_t                  startsTime;
    uint64_t                  startuTime;
    uint64_t                  frameCount;

    double                  averageSpeed;
    double                  averageLatency;

    uint64_t                  byteCount;

}EbPerformanceContext_t;

typedef struct EbConfig_s
{
    /****************************************
     * File I/O
     ****************************************/
    FILE                   *configFile;
    FILE                   *inputFile;
    FILE                   *bitstreamFile;
    FILE                   *reconFile;
    FILE                   *errorLogFile;
	FILE                   *bufferFile;

    FILE                   *qpFile;

    // y4m format support
    EB_BOOL                 y4m_input;
    unsigned char           y4m_buf[9];

    EB_BOOL                useQpFile;
    uint8_t                 tileColumnCount;
    uint8_t                 tileRowCount;
    uint8_t                 tileSliceMode;
    int32_t                 frameRate;
    int32_t                 frameRateNumerator;
    int32_t                 frameRateDenominator;
    int32_t                 injectorFrameRate;
    uint32_t                 injector;
    uint32_t                  speedControlFlag;
    uint32_t                 encoderBitDepth;
    uint32_t                 encoderColorFormat;
	uint32_t                 compressedTenBitFormat;
    uint32_t                 sourceWidth;
    uint32_t                 sourceHeight;

    uint32_t                 inputPaddedWidth;
    uint32_t                 inputPaddedHeight;

    int64_t                 framesToBeEncoded;
    int32_t                 framesEncoded;
    int32_t                 bufferedInput;
    uint8_t               **sequenceBuffer;

    uint8_t                  latencyMode;

    /****************************************
     * // Interlaced Video
     ****************************************/
    EB_BOOL                 interlacedVideo;
    EB_BOOL                 separateFields;

    /*****************************************
     * Coding Structure
     *****************************************/
    uint32_t                 baseLayerSwitchMode;
    uint8_t                  encMode;
    int32_t                 intraPeriod;
    uint32_t                 intraRefreshType;
	uint32_t                 hierarchicalLevels;
	uint32_t                 predStructure;


    /****************************************
     * Quantization
     ****************************************/
    uint32_t                 qp;

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

    /****************************************
     * ME Parameters
     ****************************************/
    uint32_t               searchAreaWidth;
    uint32_t               searchAreaHeight;
    
    /****************************************
     * MD Parameters
     ****************************************/
    EB_BOOL                constrainedIntra;

    /****************************************
     * Rate Control
     ****************************************/
    uint32_t                 sceneChangeDetection;
    uint32_t                 rateControlMode;
    uint32_t                 lookAheadDistance;
    uint32_t                 targetBitRate;
    uint32_t                 maxQpAllowed;
    uint32_t                 minQpAllowed;
    uint32_t                 vbvMaxRate;
    uint32_t                 vbvBufsize;
    uint64_t                 vbvBufInit;

    /****************************************
    * TUNE
    ****************************************/
    uint8_t                  tune;

    /****************************************
     * Optional Features
     ****************************************/

	EB_BOOL				   bitRateReduction;
    EB_BOOL                improveSharpness;
    uint32_t                 videoUsabilityInfo;
    uint32_t                 highDynamicRangeInput;
    uint32_t                 accessUnitDelimiter;
    uint32_t                 bufferingPeriodSEI;
    uint32_t                 pictureTimingSEI;
    EB_BOOL                registeredUserDataSeiFlag;
    EB_BOOL                unregisteredUserDataSeiFlag;
    EB_BOOL                recoveryPointSeiFlag;
    uint32_t                 enableTemporalId;
    EB_BOOL                switchThreadsToRtPriority;
    EB_BOOL                fpsInVps;
    uint32_t                 hrdFlag;
    EB_BOOL                unrestrictedMotionVector;

    /****************************************
     * Annex A Parameters
     ****************************************/
    uint32_t                 profile;
    uint32_t                 tier;
    uint32_t                 level;

    /****************************************
     * On-the-fly Testing
     ****************************************/
	uint32_t                 testUserData;
	EB_BOOL				   eosFlag;

    /****************************************
    * Optimization Type
    ****************************************/
    uint32_t					asmType;

    /****************************************
     * Computational Performance Data
     ****************************************/
    EbPerformanceContext_t  performanceContext;

    /****************************************
    * Instance Info
    ****************************************/
    uint32_t              channelId;
    uint32_t              activeChannelCount;
    uint32_t              logicalProcessors;
    int32_t              targetSocket;
    EB_BOOL             stopEncoder;         // to signal CTRL+C Event, need to stop encoding.

    uint64_t  processedFrameCount;
    uint64_t  processedByteCount;

    /****************************************
    * SEI parameters
    ****************************************/
    uint16_t     maxCLL;
    uint16_t     maxFALL;
    EB_BOOL      useMasteringDisplayColorVolume;
    char         masteringDisplayColorVolumeString[MAX_STRING_LENGTH];
    uint32_t     dolbyVisionProfile;
    FILE*        dolbyVisionRpuFile;
    EB_BOOL      useNaluFile;
    FILE*        naluFile;

    // Master Display Color Volume Parameters
    uint16_t     displayPrimaryX[3];
    uint16_t     displayPrimaryY[3];
    uint16_t     whitePointX, whitePointY;
    uint32_t     maxDisplayMasteringLuminance;
    uint32_t     minDisplayMasteringLuminance;

} EbConfig_t;

extern void EbConfigCtor(EbConfig_t *configPtr);
extern void EbConfigDtor(EbConfig_t *configPtr);

extern EB_ERRORTYPE	ReadCommandLine(int32_t argc, char *const argv[], EbConfig_t **config, uint32_t  numChannels,	EB_ERRORTYPE *return_errors);
extern uint32_t     GetHelp(int32_t argc, char *const argv[]);
extern uint32_t		GetNumberOfChannels(int32_t argc, char *const argv[]);

#endif //EbAppConfig_h
