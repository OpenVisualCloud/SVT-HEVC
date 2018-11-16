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
// Windows Compiler
#pragma warning( disable : 4127 )
#pragma warning( disable : 4201 )
#pragma warning( disable : 4702 )
#pragma warning( disable : 4456 )  
#pragma warning( disable : 4457 )
#pragma warning( disable : 4459 )
#pragma warning( disable : 4334 ) 

#elif _WIN32 // MinGW
#else 
    // *Note -- fseeko and ftello are already defined for linux
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wunused-function"
#ifndef __cplusplus
#define __USE_LARGEFILE
#endif
#endif

#ifndef _RSIZE_T_DEFINED
typedef size_t rsize_t;
#define _RSIZE_T_DEFINED
#endif  /* _RSIZE_T_DEFINED */

#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int errno_t;
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
#ifdef _MSC_VER

	/** EB_U8 is an 8 bit unsigned quantity that is byte aligned */
	typedef unsigned __int8     EB_U8;
	/** EB_U16 is an 16 bit unsigned quantity that is byte aligned */
	typedef unsigned __int16    EB_U16;
	/** EB_U32 is an 32 bit unsigned quantity that is byte aligned */
	typedef unsigned __int32    EB_U32;
	/** EB_U64 is an 64 bit unsigned quantity that is byte aligned */
	typedef unsigned __int64    EB_U64;

	/** EB_S8 is an 8 bit signed quantity that is byte aligned */
	typedef signed __int8         EB_S8;
	/** EB_S16 is an 16 bit signed quantity that is byte aligned */
	typedef signed __int16         EB_S16;
	/** EB_S32 is an 32 bit signed quantity that is byte aligned */
	typedef signed __int32        EB_S32;
	/** EB_S64 is an 64 bit signed quantity that is byte aligned */
	typedef signed __int64        EB_S64;

#else

#include <stdint.h>

	/** EB_U8 is an 8 bit unsigned quantity that is byte aligned */
	typedef uint8_t             EB_U8;
	/** EB_U16 is an 16 bit unsigned quantity that is byte aligned */
	typedef uint16_t            EB_U16;
	/** EB_U32 is an 32 bit unsigned quantity that is byte aligned */
	typedef uint32_t            EB_U32;
	/** EB_U64 is an 64 bit unsigned quantity that is byte aligned */
	typedef uint64_t            EB_U64;

	/** EB_S8 is an 8 bit signed quantity that is byte aligned */
	typedef int8_t                 EB_S8;
	/** EB_S16 is an 16 bit signed quantity that is byte aligned */
	typedef int16_t                EB_S16;
	/** EB_S32 is an 32 bit signed quantity that is byte aligned */
	typedef int32_t                EB_S32;
	/** EB_S64 is an 64 bit signed quantity that is byte aligned */
	typedef int64_t             EB_S64;

#endif // _WIN32
    
/** The EB_BOOL type is intended to be used to represent a true or a false
value when passing parameters to and from the svt API.  The
EB_BOOL is a 32 bit quantity and is aligned on a 32 bit word boundary.
*/

#define EB_BOOL   EB_U8
#define EB_FALSE  0
#define EB_TRUE   1
    
extern    EbMemoryMapEntry        *appMemoryMap;            // App Memory table
extern    EB_U32                  *appMemoryMapIndex;       // App Memory index
extern    EB_U64                  *totalAppMemory;          // App Memory malloc'd
extern    EB_U32                   appMallocCount;

typedef struct EB_PARAM_PORTDEFINITIONTYPE {
    EB_U32 nFrameWidth;
    EB_U32 nFrameHeight;
    EB_S32 nStride;
    EB_U32 nSize;
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

    unsigned long long  processedFrameCount;
    unsigned long long  processedByteCount;
    
} EbConfig_t;

extern void EbConfigCtor(EbConfig_t *configPtr);
extern void EbConfigDtor(EbConfig_t *configPtr);

extern EB_ERRORTYPE	ReadCommandLine(int argc, char *const argv[], EbConfig_t **config, unsigned int  numChannels,	EB_ERRORTYPE *return_errors);
extern unsigned int     GetHelp(int argc, char *const argv[]);
extern unsigned int		GetNumberOfChannels(int argc, char *const argv[]);

#endif //EbAppConfig_h
