/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


// SUMMARY
//   Contains the API component functions

/**************************************
 * Includes
 **************************************/
#include <stdlib.h>
#include <stdio.h>

#include "EbDefinitions.h"
#include "EbApi.h"
#include "EbThreads.h"
#include "EbUtility.h"
#include "EbEncHandle.h"

#include "EbSystemResourceManager.h"
#include "EbPictureControlSet.h"
#include "EbSequenceControlSet.h"
#include "EbPictureBufferDesc.h"
#include "EbReferenceObject.h"

#include "EbResourceCoordinationProcess.h"
#include "EbPictureAnalysisProcess.h"
#include "EbPictureDecisionProcess.h"
#include "EbMotionEstimationProcess.h"
#include "EbInitialRateControlProcess.h"
#include "EbSourceBasedOperationsProcess.h"
#include "EbPictureManagerProcess.h"
#include "EbRateControlProcess.h"
#include "EbModeDecisionConfigurationProcess.h"
#include "EbEncDecProcess.h"
#include "EbEntropyCodingProcess.h"
#include "EbPacketizationProcess.h"

#include "EbResourceCoordinationResults.h"
#include "EbPictureAnalysisResults.h"
#include "EbPictureDecisionResults.h"
#include "EbMotionEstimationResults.h"
#include "EbInitialRateControlResults.h"
#include "EbPictureDemuxResults.h"
#include "EbRateControlTasks.h"
#include "EbRateControlResults.h"
#include "EbEncDecTasks.h"
#include "EbEncDecResults.h"
#include "EbEntropyCodingResults.h"

#include "EbPredictionStructure.h"
#include "EbBitstreamUnit.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#if __linux__
#include <pthread.h>
#include <errno.h>
#endif

/**************************************
 * Defines
 **************************************/

#define EB_EncodeInstancesTotalCount                    1
#define EB_CandidateInitCount                           1
#define EB_ComputeSegmentInitCount                      1

// Config Set Initial Count
#define EB_SequenceControlSetPoolInitCount              3
      
// Process Instantiation Initial Counts
#define EB_ResourceCoordinationProcessInitCount         1
#define EB_PictureDecisionProcessInitCount              1
#define EB_InitialRateControlProcessInitCount           1
#define EB_PictureManagerProcessInitCount               1
#define EB_RateControlProcessInitCount                  1
#define EB_PacketizationProcessInitCount                1

// Buffer Transfer Parameters
#define EB_INPUTVIDEOBUFFERSIZE                         0x10000//   832*480*3//      // Input Slice Size , must me a multiple of 2 in case of 10 bit video.
#define EB_OUTPUTSTREAMBUFFERSIZE                       0x2DC6C0   //0x7D00        // match MTU Size
#define EB_OUTPUTRECONBUFFERSIZE                        (MAX_PICTURE_WIDTH_SIZE*MAX_PICTURE_HEIGHT_SIZE*2)   // Recon Slice Size
#define EB_OUTPUTSTREAMQUANT                            27
#define EB_OUTPUTSTATISTICSBUFFERSIZE                   0x30            // 6X8 (8 Bytes for Y, U, V, number of bits, picture number, QP)
#define EB_OUTPUTSTREAMBUFFERSIZE_MACRO(ResolutionSize)                ((ResolutionSize) < (INPUT_SIZE_1080i_TH) ? 0x1E8480 : (ResolutionSize) < (INPUT_SIZE_1080p_TH) ? 0x2DC6C0 : (ResolutionSize) < (INPUT_SIZE_4K_TH) ? 0x2DC6C0 : 0x2DC6C0  )   

static EB_U64 maxLumaPictureSize[TOTAL_LEVEL_COUNT] = { 36864U, 122880U, 245760U, 552960U, 983040U, 2228224U, 2228224U, 8912896U, 8912896U, 8912896U, 35651584U, 35651584U, 35651584U };
static EB_U64 maxLumaSampleRate[TOTAL_LEVEL_COUNT] = { 552960U, 3686400U, 7372800U, 16588800U, 33177600U, 66846720U, 133693440U, 267386880U, 534773760U, 1069547520U, 1069547520U, 2139095040U, 4278190080U };
static EB_U32 mainTierCPB[TOTAL_LEVEL_COUNT] = { 350000, 1500000, 3000000, 6000000, 10000000, 12000000, 20000000, 25000000, 40000000, 60000000, 60000000, 120000000, 240000000 };
static EB_U32 highTierCPB[TOTAL_LEVEL_COUNT] = { 350000, 1500000, 3000000, 6000000, 10000000, 30000000, 50000000, 100000000, 160000000, 240000000, 240000000, 480000000, 800000000 };
static EB_U32 mainTierMaxBitRate[TOTAL_LEVEL_COUNT] = { 128000, 1500000, 3000000, 6000000, 10000000, 12000000, 20000000, 25000000, 40000000, 60000000, 60000000, 120000000, 240000000 };
static EB_U32 highTierMaxBitRate[TOTAL_LEVEL_COUNT] = { 128000, 1500000, 3000000, 6000000, 10000000, 30000000, 50000000, 100000000, 160000000, 240000000, 240000000, 480000000, 800000000 };

EB_U32 ASM_TYPES;
/**************************************
 * External Functions
 **************************************/
static EB_ERRORTYPE InitH265EncoderHandle(EB_HANDLETYPE hComponent); 
#include <immintrin.h>

/**************************************
 * Globals
 **************************************/

EbMemoryMapEntry               *memoryMap; 
EB_U32                         *memoryMapIndex;
EB_U64                         *totalLibMemory;

EB_U32                         libMallocCount = 0;
EB_U32                         libThreadCount = 0;
EB_U32                         libSemaphoreCount = 0;
EB_U32                         libMutexCount = 0;

#ifdef _MSC_VER
GROUP_AFFINITY                  groupAffinity;
#endif
EB_U8                           numGroups;
EB_BOOL                         alternateGroups;

/**************************************
* Instruction Set Support
**************************************/
#include <stdint.h>
#if defined(_MSC_VER)
# include <intrin.h>
#endif
void RunCpuid(EB_U32 eax, EB_U32 ecx, int* abcd)
{
#if defined(_MSC_VER)
    __cpuidex(abcd, eax, ecx);
#else
    uint32_t ebx, edx;
# if defined( __i386__ ) && defined ( __PIC__ )
     /* in case of PIC under 32-bit EBX cannot be clobbered */
    __asm__ ( "movl %%ebx, %%edi \n\t cpuid \n\t xchgl %%ebx, %%edi" : "=D" (ebx),
# else
    __asm__ ( "cpuid" : "=b" (ebx),
# endif
              "+a" (eax), "+c" (ecx), "=d" (edx) );
    abcd[0] = eax; abcd[1] = ebx; abcd[2] = ecx; abcd[3] = edx;
#endif
}

int CheckXcr0Ymm() 
{
    uint32_t xcr0;
#if defined(_MSC_VER)
    xcr0 = (uint32_t)_xgetbv(0);  /* min VS2010 SP1 compiler is required */
#else
    __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
#endif
    return ((xcr0 & 6) == 6); /* checking if xmm and ymm state are enabled in XCR0 */
}
EB_S32 Check4thGenIntelCoreFeatures()
{
    int abcd[4];
    int fma_movbe_osxsave_mask = ((1 << 12) | (1 << 22) | (1 << 27));
    int avx2_bmi12_mask = (1 << 5) | (1 << 3) | (1 << 8);
 
    /* CPUID.(EAX=01H, ECX=0H):ECX.FMA[bit 12]==1   && 
       CPUID.(EAX=01H, ECX=0H):ECX.MOVBE[bit 22]==1 && 
       CPUID.(EAX=01H, ECX=0H):ECX.OSXSAVE[bit 27]==1 */
    RunCpuid( 1, 0, abcd );
    if ( (abcd[2] & fma_movbe_osxsave_mask) != fma_movbe_osxsave_mask ) 
        return 0;
 
    if ( ! CheckXcr0Ymm() )
        return 0;
 
    /*  CPUID.(EAX=07H, ECX=0H):EBX.AVX2[bit 5]==1  &&
        CPUID.(EAX=07H, ECX=0H):EBX.BMI1[bit 3]==1  &&
        CPUID.(EAX=07H, ECX=0H):EBX.BMI2[bit 8]==1  */
    RunCpuid( 7, 0, abcd );
    if ( (abcd[1] & avx2_bmi12_mask) != avx2_bmi12_mask ) 
        return 0;
    /* CPUID.(EAX=80000001H):ECX.LZCNT[bit 5]==1 */
    RunCpuid( 0x80000001, 0, abcd );
    if ( (abcd[2] & (1 << 5)) == 0)
        return 0;
    return 1;
}
static EB_S32 CanUseIntelCore4thGenFeatures()
{
    static int the_4th_gen_features_available = -1;
    /* test is performed once */
    if (the_4th_gen_features_available < 0 )
        the_4th_gen_features_available = Check4thGenIntelCoreFeatures();
    return the_4th_gen_features_available;
}

int CheckXcr0Zmm() 
{
    uint32_t xcr0;
    uint32_t zmm_ymm_xmm = (7 << 5) | (1 << 2) | (1 << 1);
#if defined(_MSC_VER)
    xcr0 = (uint32_t)_xgetbv(0);  /* min VS2010 SP1 compiler is required */
#else
    __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
#endif
    return ((xcr0 & zmm_ymm_xmm) == zmm_ymm_xmm); /* check if xmm, ymm and zmm state are enabled in XCR0 */
}

static EB_S32 CanUseIntelAVX512()
{
    int abcd[4];

    /*  CPUID.(EAX=07H, ECX=0):EBX[bit 16]==1 AVX512F 
        CPUID.(EAX=07H, ECX=0):EBX[bit 17] AVX512DQ
        CPUID.(EAX=07H, ECX=0):EBX[bit 28] AVX512CD
        CPUID.(EAX=07H, ECX=0):EBX[bit 30] AVX512BW
        CPUID.(EAX=07H, ECX=0):EBX[bit 31] AVX512VL */

    int avx512_ebx_mask = 
          (1 << 16)  // AVX-512F
        | (1 << 17)  // AVX-512DQ
        | (1 << 28)  // AVX-512CD
        | (1 << 30)  // AVX-512BW
        | (1 << 31); // AVX-512VL

    // ensure OS supports ZMM registers (and YMM, and XMM)
    if ( ! CheckXcr0Zmm() )
        return 0;

    if ( ! Check4thGenIntelCoreFeatures() )
        return 0;

    RunCpuid( 7, 0, abcd );
    if ( (abcd[1] & avx512_ebx_mask) != avx512_ebx_mask )
        return 0;

    return 1;
}


// Returns ASM Type based on system configuration. AVX512 - 111, AVX2 - 011, NONAVX2 - 001, C - 000
// Using bit-fields, the fastest function will always be selected based on the available functions in the function arrays
EB_U32 GetCpuAsmType()
{
	EB_U32 asmType = 0;

    if (CanUseIntelAVX512() == 1)
        asmType = 7; // bit-field
    else
    if (CanUseIntelCore4thGenFeatures() == 1){
        asmType = 3; // bit-field
    }
    else{
        asmType = 1; // bit-field
    }
	return asmType;
}

//Get Number of processes
EB_U32 GetNumCores() {
#ifdef WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors<<1;
#else
	return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}
/*****************************************
 * Process Input Ports Config
 *****************************************/

// Rate Control
static RateControlPorts_t rateControlPorts[] = {
    {RATE_CONTROL_INPUT_PORT_PICTURE_MANAGER,       0},
    {RATE_CONTROL_INPUT_PORT_PACKETIZATION,         0},
    {RATE_CONTROL_INPUT_PORT_ENTROPY_CODING,        0},
    {RATE_CONTROL_INPUT_PORT_INVALID,               0}
};

typedef struct {
    EB_S32  type;
    EB_U32  count;
} EncDecPorts_t;

#define ENCDEC_INPUT_PORT_MDC      0
#define ENCDEC_INPUT_PORT_ENCDEC   1
#define ENCDEC_INPUT_PORT_INVALID -1

// EncDec 
static EncDecPorts_t encDecPorts[] = {
    {ENCDEC_INPUT_PORT_MDC,        0},
    {ENCDEC_INPUT_PORT_ENCDEC,     0},
    {ENCDEC_INPUT_PORT_INVALID,    0}
};

/*****************************************
 * Input Port Lookup
 *****************************************/

// Rate Control
static EB_U32 RateControlPortLookup(
    RATE_CONTROL_INPUT_PORT_TYPES           type,
    EB_U32                                  portTypeIndex)
{
    EB_U32 portIndex = 0;
    EB_U32 portCount = 0;

    while((type != rateControlPorts[portIndex].type) && (type != RATE_CONTROL_INPUT_PORT_INVALID)) {
        portCount += rateControlPorts[portIndex++].count;
    }

    return (portCount + portTypeIndex);
}

// EncDec 
static EB_U32 EncDecPortLookup(
    EB_S32  type,
    EB_U32  portTypeIndex)
{
    EB_U32 portIndex = 0;
    EB_U32 portCount = 0;

    while((type != encDecPorts[portIndex].type) && (type != ENCDEC_INPUT_PORT_INVALID)) {
        portCount += encDecPorts[portIndex++].count;
    }

    return (portCount + portTypeIndex);
}

/*****************************************
 * Input Port Total Count
 *****************************************/

// Rate Control
static EB_U32 RateControlPortTotalCount(void)
{
    EB_U32 portIndex    = 0;
    EB_U32 totalCount   = 0;

    while(rateControlPorts[portIndex].type != RATE_CONTROL_INPUT_PORT_INVALID) {
        totalCount += rateControlPorts[portIndex++].count;
    }

    return totalCount;
}
  
// EncDec
static EB_U32 EncDecPortTotalCount(void)
{
    EB_U32 portIndex    = 0;
    EB_U32 totalCount   = 0;

    while(encDecPorts[portIndex].type != ENCDEC_INPUT_PORT_INVALID) {
        totalCount += encDecPorts[portIndex++].count;
    }

    return totalCount;
}

void InitThreadManagmentParams(){
#ifdef _MSC_VER 
    // Initialize groupAffinity structure with Current thread info
    GetThreadGroupAffinity(GetCurrentThread(),&groupAffinity);
    numGroups = (EB_U8) GetActiveProcessorGroupCount();
#else
    return;
#endif
}
void libSvtEncoderSendErrorExit(
    EB_HANDLETYPE          hComponent,
    EB_U32                 errorCode);

/**********************************
 * Encoder Library Handle Constructor
 **********************************/
static EB_ERRORTYPE EbEncHandleCtor(
    EbEncHandle_t **encHandleDblPtr,
    EB_HANDLETYPE ebHandlePtr)
{
    EB_U32  instanceIndex;
    EB_ERRORTYPE return_error = EB_ErrorNone;
    // Allocate Memory
    EbEncHandle_t *encHandlePtr = (EbEncHandle_t*) malloc(sizeof(EbEncHandle_t));
    *encHandleDblPtr = encHandlePtr;
    if (encHandlePtr == (EbEncHandle_t*) EB_NULL){
        return EB_ErrorInsufficientResources;
    }
    encHandlePtr->memoryMap             = (EbMemoryMapEntry*) malloc(sizeof(EbMemoryMapEntry) * MAX_NUM_PTR);
    encHandlePtr->memoryMapIndex        = 0;
	encHandlePtr->totalLibMemory		= sizeof(EbEncHandle_t) + sizeof(EbMemoryMapEntry) * MAX_NUM_PTR;

    // Save Memory Map Pointers 
    totalLibMemory                      = &encHandlePtr->totalLibMemory;
    memoryMap                           =  encHandlePtr->memoryMap;
    memoryMapIndex                      = &encHandlePtr->memoryMapIndex;
    libMallocCount                      = 0;
    libThreadCount                      = 0;
    libMutexCount                       = 0;
    libSemaphoreCount                   = 0;

    if (memoryMap == (EbMemoryMapEntry*) EB_NULL){
        return EB_ErrorInsufficientResources;
    }

    InitThreadManagmentParams();
    
    encHandlePtr->encodeInstanceTotalCount                          = EB_EncodeInstancesTotalCount;

    EB_MALLOC(EB_U32*, encHandlePtr->computeSegmentsTotalCountArray, sizeof(EB_U32) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);

    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        encHandlePtr->computeSegmentsTotalCountArray[instanceIndex] = EB_ComputeSegmentInitCount;
    }

    // Config Set Count
    encHandlePtr->sequenceControlSetPoolTotalCount                  = EB_SequenceControlSetPoolInitCount;

    // Sequence Control Set Buffers
    encHandlePtr->sequenceControlSetPoolPtr                         = (EbSystemResource_t*) EB_NULL;
    encHandlePtr->sequenceControlSetPoolProducerFifoPtrArray        = (EbFifo_t**) EB_NULL;

    // Picture Buffers
    encHandlePtr->referencePicturePoolPtrArray                      = (EbSystemResource_t**) EB_NULL;
    encHandlePtr->paReferencePicturePoolPtrArray                    = (EbSystemResource_t**) EB_NULL;

    // Picture Buffer Producer Fifos  
    encHandlePtr->referencePicturePoolProducerFifoPtrDblArray       = (EbFifo_t***) EB_NULL;
    encHandlePtr->paReferencePicturePoolProducerFifoPtrDblArray     = (EbFifo_t***) EB_NULL;
    
    // Threads
    encHandlePtr->resourceCoordinationThreadHandle                  = (EB_HANDLE) EB_NULL;
    encHandlePtr->pictureAnalysisThreadHandleArray                  = (EB_HANDLE*)  EB_NULL;
    encHandlePtr->pictureDecisionThreadHandle                       = (EB_HANDLE)   EB_NULL;
    encHandlePtr->motionEstimationThreadHandleArray                 = (EB_HANDLE*) EB_NULL;
    encHandlePtr->initialRateControlThreadHandle                    = (EB_HANDLE)   EB_NULL;
	encHandlePtr->sourceBasedOperationsThreadHandleArray			= (EB_HANDLE*)EB_NULL;
    encHandlePtr->pictureManagerThreadHandle                        = (EB_HANDLE)   EB_NULL;
    encHandlePtr->rateControlThreadHandle                           = (EB_HANDLE) EB_NULL;
    encHandlePtr->modeDecisionConfigurationThreadHandleArray        = (EB_HANDLE*) EB_NULL;
    encHandlePtr->encDecThreadHandleArray                           = (EB_HANDLE*) EB_NULL;
    encHandlePtr->entropyCodingThreadHandleArray                    = (EB_HANDLE*) EB_NULL;
    encHandlePtr->packetizationThreadHandle                         = (EB_HANDLE) EB_NULL;

    // Contexts
    encHandlePtr->resourceCoordinationContextPtr                    = (EB_PTR) EB_NULL;
    encHandlePtr->pictureAnalysisContextPtrArray                    = (EB_PTR*) EB_NULL;
    encHandlePtr->pictureDecisionContextPtr                         = (EB_PTR)  EB_NULL;
    encHandlePtr->motionEstimationContextPtrArray                   = (EB_PTR*) EB_NULL;
    encHandlePtr->initialRateControlContextPtr                      = (EB_PTR)  EB_NULL;
	encHandlePtr->sourceBasedOperationsContextPtrArray				= (EB_PTR*)EB_NULL;
    encHandlePtr->pictureManagerContextPtr                          = (EB_PTR)  EB_NULL;
    encHandlePtr->rateControlContextPtr                             = (EB_PTR) EB_NULL;
    encHandlePtr->modeDecisionConfigurationContextPtrArray          = (EB_PTR*) EB_NULL;
    encHandlePtr->encDecContextPtrArray                             = (EB_PTR*) EB_NULL;
    encHandlePtr->entropyCodingContextPtrArray                      = (EB_PTR*) EB_NULL;
    encHandlePtr->packetizationContextPtr                           = (EB_PTR) EB_NULL;

    // System Resource Managers
    encHandlePtr->inputBufferResourcePtr                         = (EbSystemResource_t*) EB_NULL;
    encHandlePtr->outputStreamBufferResourcePtrArray             = (EbSystemResource_t**) EB_NULL;
    encHandlePtr->resourceCoordinationResultsResourcePtr            = (EbSystemResource_t*) EB_NULL;
    encHandlePtr->pictureAnalysisResultsResourcePtr                 = (EbSystemResource_t*) EB_NULL;
    encHandlePtr->pictureDecisionResultsResourcePtr                 = (EbSystemResource_t*) EB_NULL;
    encHandlePtr->motionEstimationResultsResourcePtr                = (EbSystemResource_t*) EB_NULL;
	encHandlePtr->initialRateControlResultsResourcePtr				= (EbSystemResource_t*)EB_NULL;
    encHandlePtr->pictureDemuxResultsResourcePtr                    = (EbSystemResource_t*) EB_NULL;
    encHandlePtr->rateControlTasksResourcePtr                       = (EbSystemResource_t*) EB_NULL;
    encHandlePtr->rateControlResultsResourcePtr                     = (EbSystemResource_t*) EB_NULL;
    encHandlePtr->encDecTasksResourcePtr                            = (EbSystemResource_t*) EB_NULL;
    encHandlePtr->encDecResultsResourcePtr                          = (EbSystemResource_t*) EB_NULL;
    encHandlePtr->entropyCodingResultsResourcePtr                   = (EbSystemResource_t*) EB_NULL;

    // Inter-Process Producer Fifos
    encHandlePtr->inputBufferProducerFifoPtrArray                         = (EbFifo_t**) EB_NULL;
    encHandlePtr->outputStreamBufferProducerFifoPtrDblArray               = (EbFifo_t***) EB_NULL;
    encHandlePtr->resourceCoordinationResultsProducerFifoPtrArray            = (EbFifo_t**) EB_NULL;
    encHandlePtr->pictureDemuxResultsProducerFifoPtrArray                    = (EbFifo_t**) EB_NULL;
    encHandlePtr->pictureManagerResultsProducerFifoPtrArray                  = (EbFifo_t**) EB_NULL;
    encHandlePtr->rateControlTasksProducerFifoPtrArray                       = (EbFifo_t**) EB_NULL;
    encHandlePtr->rateControlResultsProducerFifoPtrArray                     = (EbFifo_t**) EB_NULL;
    encHandlePtr->encDecTasksProducerFifoPtrArray                            = (EbFifo_t**) EB_NULL;
    encHandlePtr->encDecResultsProducerFifoPtrArray                          = (EbFifo_t**) EB_NULL;
    encHandlePtr->entropyCodingResultsProducerFifoPtrArray                   = (EbFifo_t**) EB_NULL;

    // Inter-Process Consumer Fifos
    encHandlePtr->inputBufferConsumerFifoPtrArray                = (EbFifo_t**) EB_NULL;
    encHandlePtr->outputStreamBufferConsumerFifoPtrDblArray      = (EbFifo_t***) EB_NULL;
    encHandlePtr->resourceCoordinationResultsConsumerFifoPtrArray   = (EbFifo_t**) EB_NULL;
    encHandlePtr->pictureDemuxResultsConsumerFifoPtrArray           = (EbFifo_t**) EB_NULL;
    encHandlePtr->rateControlTasksConsumerFifoPtrArray              = (EbFifo_t**) EB_NULL;
    encHandlePtr->rateControlResultsConsumerFifoPtrArray            = (EbFifo_t**) EB_NULL;
    encHandlePtr->encDecTasksConsumerFifoPtrArray                   = (EbFifo_t**) EB_NULL;
    encHandlePtr->encDecResultsConsumerFifoPtrArray                 = (EbFifo_t**) EB_NULL;
    encHandlePtr->entropyCodingResultsConsumerFifoPtrArray          = (EbFifo_t**) EB_NULL;

    // Initialize Callbacks
    EB_MALLOC(EbCallback_t**, encHandlePtr->appCallbackPtrArray, sizeof(EbCallback_t*) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);

    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        EB_MALLOC(EbCallback_t*, encHandlePtr->appCallbackPtrArray[instanceIndex], sizeof(EbCallback_t), EB_N_PTR);
        encHandlePtr->appCallbackPtrArray[instanceIndex]->ErrorHandler                          = libSvtEncoderSendErrorExit;
        encHandlePtr->appCallbackPtrArray[instanceIndex]->handle                                = ebHandlePtr;
    }

    // Initialize Input Video Port
    EB_MALLOC(EB_PARAM_PORTDEFINITIONTYPE**, encHandlePtr->inputVideoPortPtrArray, sizeof(EB_PARAM_PORTDEFINITIONTYPE*) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        EB_MALLOC(EB_PARAM_PORTDEFINITIONTYPE*, encHandlePtr->inputVideoPortPtrArray[instanceIndex], sizeof(EB_PARAM_PORTDEFINITIONTYPE) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
        
        encHandlePtr->inputVideoPortPtrArray[instanceIndex]->nSize                              = sizeof(EB_PARAM_PORTDEFINITIONTYPE);
        encHandlePtr->inputVideoPortPtrArray[instanceIndex]->nFrameWidth                        = 0;
        encHandlePtr->inputVideoPortPtrArray[instanceIndex]->nFrameHeight                       = 0;
        encHandlePtr->inputVideoPortPtrArray[instanceIndex]->nStride                            = 0;
    }

    // Initialize Output Bitstream Port
    EB_MALLOC(EB_PARAM_PORTDEFINITIONTYPE**, encHandlePtr->outputStreamPortPtrArray, sizeof(EB_PARAM_PORTDEFINITIONTYPE*) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        EB_MALLOC(EB_PARAM_PORTDEFINITIONTYPE*, encHandlePtr->outputStreamPortPtrArray[instanceIndex], sizeof(EB_PARAM_PORTDEFINITIONTYPE) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
        
        encHandlePtr->outputStreamPortPtrArray[instanceIndex]->nSize                            = sizeof(EB_PARAM_PORTDEFINITIONTYPE);
        encHandlePtr->outputStreamPortPtrArray[instanceIndex]->nStride                          = EB_OUTPUTSTREAMBUFFERSIZE;
    }


    // Initialize Sequence Control Set Instance Array
    EB_MALLOC(EbSequenceControlSetInstance_t**, encHandlePtr->sequenceControlSetInstanceArray, sizeof(EbSequenceControlSetInstance_t*) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        return_error = EbSequenceControlSetInstanceCtor(&encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    return EB_ErrorNone;
}

void EbSetThreadManagementParameters(EB_H265_ENC_CONFIGURATION   *configPtr){
#ifdef _MSC_VER 
    alternateGroups = 0;
    if (configPtr->useRoundRobinThreadAssignment == EB_TRUE){
        if (numGroups == 2 && configPtr->activeChannelCount > 1){
            if ((configPtr->activeChannelCount % 2) && (configPtr->activeChannelCount - 1 == configPtr->channelId)){
                alternateGroups = 1;
                groupAffinity.Group = 0;
            }
            else if (configPtr->channelId % 2){
                alternateGroups = 0;
                groupAffinity.Group = 1;
            }
            else {
                alternateGroups = 0;
                groupAffinity.Group = 0;
            }
        }
        else if (numGroups == 2 && configPtr->activeChannelCount == 1){
            alternateGroups = 1;
            groupAffinity.Group = 0;
        }
        else{
            alternateGroups = 0;
            numGroups = 1;
        }
    }
    else{
        alternateGroups = 0;
        numGroups = 1;
    }
#else
    alternateGroups = 0;
    numGroups = 1;
    (void) configPtr;
#endif
}

/**********************************
 * Initialize Encoder Library
 **********************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbInitEncoder(EB_COMPONENTTYPE *h265EncComponent)
{
    EbEncHandle_t *encHandlePtr = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_U32 instanceIndex;
    EB_U32 processIndex;
    EB_U32 maxPictureWidth;
    EB_U32 maxLookAheadDistance  = 0;

    EB_BOOL is16bit = (EB_BOOL) (encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);

    /************************************
    * Plateform detection
    ************************************/

    if (encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.asmType == ASM_AVX2) {
        ASM_TYPES = GetCpuAsmType();
    }
    else if (encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.asmType == ASM_NON_AVX2) {
        ASM_TYPES = 0;
    }

    /************************************
     * Sequence Control Set
     ************************************/
    return_error = EbSystemResourceCtor(
        &encHandlePtr->sequenceControlSetPoolPtr,
        encHandlePtr->sequenceControlSetPoolTotalCount,
        1,
        0,
        &encHandlePtr->sequenceControlSetPoolProducerFifoPtrArray,
        (EbFifo_t ***)EB_NULL,
        EB_FALSE,
        EbSequenceControlSetCtor,
        EB_NULL);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    /************************************
     * Picture Control Set: Parent
     ************************************/
    EB_MALLOC(EbSystemResource_t**, encHandlePtr->pictureParentControlSetPoolPtrArray, sizeof(EbSystemResource_t*)  * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    EB_MALLOC(EbFifo_t***, encHandlePtr->pictureParentControlSetPoolProducerFifoPtrDblArray, sizeof(EbSystemResource_t**) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    	
	// Updating the pictureControlSetPoolTotalCount based on the maximum look ahead distance
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        maxLookAheadDistance    = MAX(maxLookAheadDistance, encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.lookAheadDistance);
    }   
    
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
    
        // The segment Width & Height Arrays are in units of LCUs, not samples
        PictureControlSetInitData_t inputData;

        inputData.pictureWidth          = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth;
        inputData.pictureHeight         = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight;
		inputData.leftPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->leftPadding;
		inputData.rightPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->rightPadding;
		inputData.topPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->topPadding;
		inputData.botPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->botPadding;
        inputData.bitDepth              = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->outputBitdepth;
        inputData.lcuSize               = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize;
        inputData.maxDepth              = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxLcuDepth;
        inputData.is16bit               = is16bit;
		inputData.compressedTenBitFormat = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.compressedTenBitFormat;
		encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->pictureControlSetPoolInitCount += maxLookAheadDistance;

		inputData.encMode = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.encMode;
		inputData.speedControl = (EB_U8)encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.speedControlFlag;
        inputData.tune = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.tune;
		return_error = EbSystemResourceCtor(
            &(encHandlePtr->pictureParentControlSetPoolPtrArray[instanceIndex]),
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->pictureControlSetPoolInitCount,//encHandlePtr->pictureControlSetPoolTotalCount,
            1,
            0,
            &encHandlePtr->pictureParentControlSetPoolProducerFifoPtrDblArray[instanceIndex],
            (EbFifo_t ***)EB_NULL,
            EB_FALSE,
            PictureParentControlSetCtor,
            &inputData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
    /************************************
     * Picture Control Set: Child
     ************************************/
    EB_MALLOC(EbSystemResource_t**, encHandlePtr->pictureControlSetPoolPtrArray, sizeof(EbSystemResource_t*)  * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    EB_MALLOC(EbFifo_t***, encHandlePtr->pictureControlSetPoolProducerFifoPtrDblArray, sizeof(EbSystemResource_t**) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
    
        // The segment Width & Height Arrays are in units of LCUs, not samples
        PictureControlSetInitData_t inputData;
        unsigned i;
        
        inputData.encDecSegmentCol = 0;
        inputData.encDecSegmentRow = 0;
        for(i=0; i <= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.hierarchicalLevels; ++i) {
            inputData.encDecSegmentCol = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->encDecSegmentColCountArray[i] > inputData.encDecSegmentCol ?
                (EB_U16) encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->encDecSegmentColCountArray[i] :
                inputData.encDecSegmentCol;
            inputData.encDecSegmentRow = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->encDecSegmentRowCountArray[i] > inputData.encDecSegmentRow ?
                (EB_U16) encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->encDecSegmentRowCountArray[i] :
                inputData.encDecSegmentRow;
        }

        inputData.pictureWidth      = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth;
        inputData.pictureHeight     = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight;
		inputData.leftPadding		= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->leftPadding;
		inputData.rightPadding		= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->rightPadding;
		inputData.topPadding		= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->topPadding;
		inputData.botPadding		= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->botPadding;
        inputData.bitDepth          = EB_8BIT;
        inputData.lcuSize           = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize;
        inputData.maxDepth          = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxLcuDepth;
        inputData.is16bit           = is16bit;
        return_error = EbSystemResourceCtor(
            &(encHandlePtr->pictureControlSetPoolPtrArray[instanceIndex]),
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->pictureControlSetPoolInitCountChild, //EB_PictureControlSetPoolInitCountChild,
            1,
            0,
            &encHandlePtr->pictureControlSetPoolProducerFifoPtrDblArray[instanceIndex],
            (EbFifo_t ***)EB_NULL,
            EB_FALSE,
            PictureControlSetCtor,
            &inputData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
    /************************************
     * Picture Buffers
     ************************************/
     
    // Allocate Resource Arrays 
    EB_MALLOC(EbSystemResource_t**, encHandlePtr->referencePicturePoolPtrArray, sizeof(EbSystemResource_t*) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    EB_MALLOC(EbSystemResource_t**, encHandlePtr->paReferencePicturePoolPtrArray, sizeof(EbSystemResource_t*) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
        
    // Allocate Producer Fifo Arrays   
    EB_MALLOC(EbFifo_t***, encHandlePtr->referencePicturePoolProducerFifoPtrDblArray, sizeof(EbFifo_t**) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    EB_MALLOC(EbFifo_t***, encHandlePtr->paReferencePicturePoolProducerFifoPtrDblArray, sizeof(EbFifo_t**) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    
    // Rate Control
    rateControlPorts[0].count = EB_PictureManagerProcessInitCount;
    rateControlPorts[1].count = EB_PacketizationProcessInitCount;
    rateControlPorts[2].count = encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount;
    rateControlPorts[3].count = 0;

    encDecPorts[ENCDEC_INPUT_PORT_MDC].count = encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount;
    encDecPorts[ENCDEC_INPUT_PORT_ENCDEC].count = encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount;

    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
    
        EbReferenceObjectDescInitData_t     EbReferenceObjectDescInitDataStructure;
        EbPaReferenceObjectDescInitData_t   EbPaReferenceObjectDescInitDataStructure;
        EbPictureBufferDescInitData_t       referencePictureBufferDescInitData;
        EbPictureBufferDescInitData_t       quarterDecimPictureBufferDescInitData;
        EbPictureBufferDescInitData_t       sixteenthDecimPictureBufferDescInitData;

        // Initialize the various Picture types
        referencePictureBufferDescInitData.maxWidth               =  encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth;
        referencePictureBufferDescInitData.maxHeight              =  encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight;
        referencePictureBufferDescInitData.bitDepth               =  encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->inputBitdepth;
        referencePictureBufferDescInitData.bufferEnableMask       =  PICTURE_BUFFER_DESC_FULL_MASK;
		referencePictureBufferDescInitData.leftPadding			  =  MAX_LCU_SIZE + MCPXPaddingOffset;
		referencePictureBufferDescInitData.rightPadding			  =  MAX_LCU_SIZE + MCPXPaddingOffset;
		referencePictureBufferDescInitData.topPadding			  =  MAX_LCU_SIZE + MCPYPaddingOffset;
		referencePictureBufferDescInitData.botPadding			  =  MAX_LCU_SIZE + MCPYPaddingOffset;
        referencePictureBufferDescInitData.splitMode              =  EB_FALSE;

        if (is16bit){
            referencePictureBufferDescInitData.bitDepth  = EB_10BIT;
        }

        EbReferenceObjectDescInitDataStructure.referencePictureDescInitData = referencePictureBufferDescInitData;
        
        // Reference Picture Buffers
        return_error = EbSystemResourceCtor(
            &encHandlePtr->referencePicturePoolPtrArray[instanceIndex],
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->referencePictureBufferInitCount,//encHandlePtr->referencePicturePoolTotalCount,
            EB_PictureManagerProcessInitCount,
            0,
            &encHandlePtr->referencePicturePoolProducerFifoPtrDblArray[instanceIndex],
            (EbFifo_t ***)EB_NULL,
            EB_FALSE,
            EbReferenceObjectCtor,
            &(EbReferenceObjectDescInitDataStructure));

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

        // PA Reference Picture Buffers
        // Currently, only Luma samples are needed in the PA
        referencePictureBufferDescInitData.maxWidth               = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth;
        referencePictureBufferDescInitData.maxHeight              = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight;
        referencePictureBufferDescInitData.bitDepth               = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->inputBitdepth;
        referencePictureBufferDescInitData.bufferEnableMask = 0;
		referencePictureBufferDescInitData.leftPadding            = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize + ME_FILTER_TAP;
		referencePictureBufferDescInitData.rightPadding           = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize + ME_FILTER_TAP;
		referencePictureBufferDescInitData.topPadding             = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize + ME_FILTER_TAP;
		referencePictureBufferDescInitData.botPadding             = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize + ME_FILTER_TAP;
        referencePictureBufferDescInitData.splitMode              = EB_FALSE;
        
        quarterDecimPictureBufferDescInitData.maxWidth              = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth  >> 1;
        quarterDecimPictureBufferDescInitData.maxHeight             = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight >> 1;
        quarterDecimPictureBufferDescInitData.bitDepth              = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->inputBitdepth;
        quarterDecimPictureBufferDescInitData.bufferEnableMask      = PICTURE_BUFFER_DESC_LUMA_MASK;
		quarterDecimPictureBufferDescInitData.leftPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 1;
		quarterDecimPictureBufferDescInitData.rightPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 1;
		quarterDecimPictureBufferDescInitData.topPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 1;
		quarterDecimPictureBufferDescInitData.botPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 1;
        quarterDecimPictureBufferDescInitData.splitMode             = EB_FALSE;

        sixteenthDecimPictureBufferDescInitData.maxWidth            = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth  >> 2;
        sixteenthDecimPictureBufferDescInitData.maxHeight           = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight >> 2;
        sixteenthDecimPictureBufferDescInitData.bitDepth            = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->inputBitdepth;
        sixteenthDecimPictureBufferDescInitData.bufferEnableMask    = PICTURE_BUFFER_DESC_LUMA_MASK;
		sixteenthDecimPictureBufferDescInitData.leftPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 2;
		sixteenthDecimPictureBufferDescInitData.rightPadding		= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 2;
		sixteenthDecimPictureBufferDescInitData.topPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 2;
		sixteenthDecimPictureBufferDescInitData.botPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 2;
        sixteenthDecimPictureBufferDescInitData.splitMode           = EB_FALSE;
    
        EbPaReferenceObjectDescInitDataStructure.referencePictureDescInitData   = referencePictureBufferDescInitData;
        EbPaReferenceObjectDescInitDataStructure.quarterPictureDescInitData     = quarterDecimPictureBufferDescInitData;
        EbPaReferenceObjectDescInitDataStructure.sixteenthPictureDescInitData   = sixteenthDecimPictureBufferDescInitData;


        // Reference Picture Buffers
        return_error = EbSystemResourceCtor(
            &encHandlePtr->paReferencePicturePoolPtrArray[instanceIndex],
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->paReferencePictureBufferInitCount,
            EB_PictureDecisionProcessInitCount,
            0,
            &encHandlePtr->paReferencePicturePoolProducerFifoPtrDblArray[instanceIndex],
            (EbFifo_t ***)EB_NULL,
            EB_FALSE,
            EbPaReferenceObjectCtor,
            &(EbPaReferenceObjectDescInitDataStructure));
		if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
		}

        // Set the SequenceControlSet Picture Pool Fifo Ptrs      
        encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->referencePicturePoolFifoPtr     = (encHandlePtr->referencePicturePoolProducerFifoPtrDblArray[instanceIndex])[0];
        encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->paReferencePicturePoolFifoPtr   = (encHandlePtr->paReferencePicturePoolProducerFifoPtrDblArray[instanceIndex])[0];
}
    
    /************************************
     * System Resource Managers & Fifos
     ************************************/
    
    // EB_BUFFERHEADERTYPE Input
    return_error = EbSystemResourceCtor(
        &encHandlePtr->inputBufferResourcePtr,
#if !ONE_MEMCPY 
        2,// encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.inputOutputBufferFifoInitCount,
#else
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.inputOutputBufferFifoInitCount,
#endif
        1,
        EB_ResourceCoordinationProcessInitCount,
        &encHandlePtr->inputBufferProducerFifoPtrArray,
        &encHandlePtr->inputBufferConsumerFifoPtrArray,
        EB_TRUE,
        EbInputBufferHeaderCtor,
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr);
    
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // EB_BUFFERHEADERTYPE Output Stream    
    EB_MALLOC(EbSystemResource_t**, encHandlePtr->outputStreamBufferResourcePtrArray, sizeof(EbSystemResource_t*) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    EB_MALLOC(EbFifo_t***, encHandlePtr->outputStreamBufferProducerFifoPtrDblArray, sizeof(EbFifo_t**)          * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    EB_MALLOC(EbFifo_t***, encHandlePtr->outputStreamBufferConsumerFifoPtrDblArray, sizeof(EbFifo_t**)          * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
    
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        return_error = EbSystemResourceCtor(
            &encHandlePtr->outputStreamBufferResourcePtrArray[instanceIndex],
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.inputOutputBufferFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->totalProcessInitCount,// EB_PacketizationProcessInitCount,
            1,
            &encHandlePtr->outputStreamBufferProducerFifoPtrDblArray[instanceIndex],
            &encHandlePtr->outputStreamBufferConsumerFifoPtrDblArray[instanceIndex],
            EB_TRUE,
            EbOutputBufferHeaderCtor,
            &encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    if (encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.reconEnabled) {
        // EB_BUFFERHEADERTYPE Output Recon
        EB_MALLOC(EbSystemResource_t**, encHandlePtr->outputReconBufferResourcePtrArray, sizeof(EbSystemResource_t*) * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
        EB_MALLOC(EbFifo_t***, encHandlePtr->outputReconBufferProducerFifoPtrDblArray, sizeof(EbFifo_t**)          * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
        EB_MALLOC(EbFifo_t***, encHandlePtr->outputReconBufferConsumerFifoPtrDblArray, sizeof(EbFifo_t**)          * encHandlePtr->encodeInstanceTotalCount, EB_N_PTR);
        for (instanceIndex = 0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
            return_error = EbSystemResourceCtor(
                &encHandlePtr->outputReconBufferResourcePtrArray[instanceIndex],
                encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->reconBufferFifoInitCount,
                encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->encDecProcessInitCount,
                1,
                &encHandlePtr->outputReconBufferProducerFifoPtrDblArray[instanceIndex],
                &encHandlePtr->outputReconBufferConsumerFifoPtrDblArray[instanceIndex],
                EB_TRUE,
                EbOutputReconBufferHeaderCtor,
                encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr);
            if (return_error == EB_ErrorInsufficientResources) {
                return EB_ErrorInsufficientResources;
            }
        }
    }
        
    // Resource Coordination Results
    {
        ResourceCoordinationResultInitData_t resourceCoordinationResultInitData;
        
        return_error = EbSystemResourceCtor(
            &encHandlePtr->resourceCoordinationResultsResourcePtr,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->resourceCoordinationFifoInitCount,
            EB_ResourceCoordinationProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount,
            &encHandlePtr->resourceCoordinationResultsProducerFifoPtrArray,
            &encHandlePtr->resourceCoordinationResultsConsumerFifoPtrArray,
            EB_TRUE,
            ResourceCoordinationResultCtor,
            &resourceCoordinationResultInitData);
			
		if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    

    // Picture Analysis Results
    {
        PictureAnalysisResultInitData_t pictureAnalysisResultInitData;
        
        return_error = EbSystemResourceCtor(
            &encHandlePtr->pictureAnalysisResultsResourcePtr,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount,
            EB_PictureDecisionProcessInitCount,
            &encHandlePtr->pictureAnalysisResultsProducerFifoPtrArray,
            &encHandlePtr->pictureAnalysisResultsConsumerFifoPtrArray,
            EB_TRUE,
            PictureAnalysisResultCtor,
            &pictureAnalysisResultInitData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
    // Picture Decision Results
    {
        PictureDecisionResultInitData_t pictureDecisionResultInitData;
        
        return_error = EbSystemResourceCtor(
            &encHandlePtr->pictureDecisionResultsResourcePtr,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureDecisionFifoInitCount,
            EB_PictureDecisionProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount,
            &encHandlePtr->pictureDecisionResultsProducerFifoPtrArray,
            &encHandlePtr->pictureDecisionResultsConsumerFifoPtrArray,
            EB_TRUE,
            PictureDecisionResultCtor,
            &pictureDecisionResultInitData);
	    if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // Motion Estimation Results
    {
        MotionEstimationResultsInitData_t motionEstimationResultInitData;
        
        return_error = EbSystemResourceCtor(
            &encHandlePtr->motionEstimationResultsResourcePtr,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount,
            EB_InitialRateControlProcessInitCount,
            &encHandlePtr->motionEstimationResultsProducerFifoPtrArray,
            &encHandlePtr->motionEstimationResultsConsumerFifoPtrArray,
            EB_TRUE,
            MotionEstimationResultsCtor,
            &motionEstimationResultInitData);
		if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

	// Initial Rate Control Results
	{
		InitialRateControlResultInitData_t initialRateControlResultInitData;

		return_error = EbSystemResourceCtor(
			&encHandlePtr->initialRateControlResultsResourcePtr,
			encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->initialRateControlFifoInitCount,
			EB_InitialRateControlProcessInitCount,
			encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount,
			&encHandlePtr->initialRateControlResultsProducerFifoPtrArray,
			&encHandlePtr->initialRateControlResultsConsumerFifoPtrArray,
			EB_TRUE,
			InitialRateControlResultsCtor,
			&initialRateControlResultInitData);

		if (return_error == EB_ErrorInsufficientResources){
			return EB_ErrorInsufficientResources;
		}
	}

    // Picture Demux Results
    {
        PictureResultInitData_t pictureResultInitData;
        
        return_error = EbSystemResourceCtor(
            &encHandlePtr->pictureDemuxResultsResourcePtr,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureDemuxFifoInitCount,
			encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount + encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount,
            EB_PictureManagerProcessInitCount,
            &encHandlePtr->pictureDemuxResultsProducerFifoPtrArray,
            &encHandlePtr->pictureDemuxResultsConsumerFifoPtrArray,
            EB_TRUE,
            PictureResultsCtor,
            &pictureResultInitData);
		if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
    // Rate Control Tasks 
    {
        RateControlTasksInitData_t rateControlTasksInitData;
        
        return_error = EbSystemResourceCtor(
            &encHandlePtr->rateControlTasksResourcePtr,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->rateControlTasksFifoInitCount,
            RateControlPortTotalCount(),
            EB_RateControlProcessInitCount,
            &encHandlePtr->rateControlTasksProducerFifoPtrArray,
            &encHandlePtr->rateControlTasksConsumerFifoPtrArray,
            EB_TRUE,
            RateControlTasksCtor,
            &rateControlTasksInitData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
    // Rate Control Results
    {
        RateControlResultsInitData_t rateControlResultInitData;

        return_error = EbSystemResourceCtor(
            &encHandlePtr->rateControlResultsResourcePtr,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->rateControlFifoInitCount,
            EB_RateControlProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount,
            &encHandlePtr->rateControlResultsProducerFifoPtrArray,
            &encHandlePtr->rateControlResultsConsumerFifoPtrArray,
            EB_TRUE,
            RateControlResultsCtor,
            &rateControlResultInitData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }


    // EncDec Tasks
    {
        EncDecTasksInitData_t ModeDecisionResultInitData;
        unsigned i;

        ModeDecisionResultInitData.encDecSegmentRowCount = 0;

        for(i=0; i <= encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.hierarchicalLevels; ++i) {
            ModeDecisionResultInitData.encDecSegmentRowCount = MAX(
                ModeDecisionResultInitData.encDecSegmentRowCount,
                encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecSegmentRowCountArray[i]);
        }

        return_error = EbSystemResourceCtor(
            &encHandlePtr->encDecTasksResourcePtr,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationFifoInitCount,
            EncDecPortTotalCount(),
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount,
            &encHandlePtr->encDecTasksProducerFifoPtrArray,
            &encHandlePtr->encDecTasksConsumerFifoPtrArray,
            EB_TRUE,
            EncDecTasksCtor,
            &ModeDecisionResultInitData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // EncDec Results
    {
        EncDecResultsInitData_t encDecResultInitData;

        return_error = EbSystemResourceCtor(
            &encHandlePtr->encDecResultsResourcePtr,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount,
            &encHandlePtr->encDecResultsProducerFifoPtrArray,
            &encHandlePtr->encDecResultsConsumerFifoPtrArray,
            EB_TRUE,
            EncDecResultsCtor,
            &encDecResultInitData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // Entropy Coding Results
    {
        EntropyCodingResultsInitData_t entropyCodingResultInitData;

        return_error = EbSystemResourceCtor(
            &encHandlePtr->entropyCodingResultsResourcePtr,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount,
            EB_PacketizationProcessInitCount,
            &encHandlePtr->entropyCodingResultsProducerFifoPtrArray,
            &encHandlePtr->entropyCodingResultsConsumerFifoPtrArray,
            EB_TRUE,
            EntropyCodingResultsCtor,
            &entropyCodingResultInitData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    /************************************
     * App Callbacks
     ************************************/
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->appCallbackPtr          = encHandlePtr->appCallbackPtrArray[instanceIndex];
    }

    // Output Buffer Fifo Ptrs
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
#if CHKN_OMX
		encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->streamOutputFifoPtr  = (encHandlePtr->outputStreamBufferProducerFifoPtrDblArray[instanceIndex])[0];
#else
        encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->streamOutputFifoPtr  = (encHandlePtr->outputStreamBufferConsumerFifoPtrDblArray[instanceIndex])[0];
#endif
        if (encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.reconEnabled) {
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->reconOutputFifoPtr = (encHandlePtr->outputReconBufferProducerFifoPtrDblArray[instanceIndex])[0];
        }

	}

    /************************************
     * Contexts
     ************************************/

    // Resource Coordination Context
    return_error = ResourceCoordinationContextCtor(
        (ResourceCoordinationContext_t**) &encHandlePtr->resourceCoordinationContextPtr,
        encHandlePtr->inputBufferConsumerFifoPtrArray[0],
        encHandlePtr->resourceCoordinationResultsProducerFifoPtrArray[0],
        encHandlePtr->pictureParentControlSetPoolProducerFifoPtrDblArray[0],//ResourceCoordination works with ParentPCS
        encHandlePtr->sequenceControlSetInstanceArray,
        encHandlePtr->sequenceControlSetPoolProducerFifoPtrArray[0],
        encHandlePtr->appCallbackPtrArray,
        encHandlePtr->computeSegmentsTotalCountArray,
        encHandlePtr->encodeInstanceTotalCount);
    
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Picture Analysis Context
    EB_MALLOC(EB_PTR*, encHandlePtr->pictureAnalysisContextPtrArray, sizeof(EB_PTR) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount, EB_N_PTR);
    
	for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount; ++processIndex) {

		EbPictureBufferDescInitData_t  pictureBufferDescConf;
		pictureBufferDescConf.maxWidth = encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->maxInputLumaWidth;
		pictureBufferDescConf.maxHeight = encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->maxInputLumaHeight;
		pictureBufferDescConf.bitDepth = EB_8BIT;
		pictureBufferDescConf.bufferEnableMask = PICTURE_BUFFER_DESC_Y_FLAG;
		pictureBufferDescConf.leftPadding = 0;
		pictureBufferDescConf.rightPadding = 0;
		pictureBufferDescConf.topPadding = 0;
		pictureBufferDescConf.botPadding = 0;
		pictureBufferDescConf.splitMode = EB_FALSE;

		return_error = PictureAnalysisContextCtor(
			&pictureBufferDescConf,
			EB_TRUE,
            (PictureAnalysisContext_t**) &encHandlePtr->pictureAnalysisContextPtrArray[processIndex],
            encHandlePtr->resourceCoordinationResultsConsumerFifoPtrArray[processIndex],
            encHandlePtr->pictureAnalysisResultsProducerFifoPtrArray[processIndex],
            ((encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->maxInputLumaWidth  + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE) *
            ((encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->maxInputLumaHeight + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE));

			
		if (return_error == EB_ErrorInsufficientResources){
        	return EB_ErrorInsufficientResources;
    	}
    }
    
    // Picture Decision Context
    {
        instanceIndex = 0;

        return_error = PictureDecisionContextCtor(
            (PictureDecisionContext_t**) &encHandlePtr->pictureDecisionContextPtr,
            encHandlePtr->pictureAnalysisResultsConsumerFifoPtrArray[0],
            encHandlePtr->pictureDecisionResultsProducerFifoPtrArray[0]);
		if (return_error == EB_ErrorInsufficientResources){
        	return EB_ErrorInsufficientResources;
    	}
    }

    // Motion Analysis Context
    EB_MALLOC(EB_PTR*, encHandlePtr->motionEstimationContextPtrArray, sizeof(EB_PTR) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount, EB_N_PTR);
    
    for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount; ++processIndex) {

		return_error = MotionEstimationContextCtor(
			(MotionEstimationContext_t**) &encHandlePtr->motionEstimationContextPtrArray[processIndex],
			encHandlePtr->pictureDecisionResultsConsumerFifoPtrArray[processIndex],
			encHandlePtr->motionEstimationResultsProducerFifoPtrArray[processIndex]);

        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }
    
    // Initial Rate Control Context
    return_error = InitialRateControlContextCtor(
        (InitialRateControlContext_t**) &encHandlePtr->initialRateControlContextPtr,
        encHandlePtr->motionEstimationResultsConsumerFifoPtrArray[0],
		encHandlePtr->initialRateControlResultsProducerFifoPtrArray[0]);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    
	// Source Based Operations Context
	EB_MALLOC(EB_PTR*, encHandlePtr->sourceBasedOperationsContextPtrArray, sizeof(EB_PTR) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount, EB_N_PTR);

	for (processIndex = 0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount; ++processIndex) {
        return_error = SourceBasedOperationsContextCtor(
            (SourceBasedOperationsContext_t**)&encHandlePtr->sourceBasedOperationsContextPtrArray[processIndex],
            encHandlePtr->initialRateControlResultsConsumerFifoPtrArray[processIndex],
            encHandlePtr->pictureDemuxResultsProducerFifoPtrArray[processIndex]);
		if (return_error == EB_ErrorInsufficientResources){
			return EB_ErrorInsufficientResources;
		}
	}


    // Picture Manager Context
    return_error = PictureManagerContextCtor(
        (PictureManagerContext_t**) &encHandlePtr->pictureManagerContextPtr,
        encHandlePtr->pictureDemuxResultsConsumerFifoPtrArray[0],
        encHandlePtr->rateControlTasksProducerFifoPtrArray[RateControlPortLookup(RATE_CONTROL_INPUT_PORT_PICTURE_MANAGER, 0)],
        encHandlePtr->pictureControlSetPoolProducerFifoPtrDblArray[0]);//The Child PCS Pool here
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }    
    // Rate Control Context
    return_error = RateControlContextCtor(
        (RateControlContext_t**) &encHandlePtr->rateControlContextPtr,
        encHandlePtr->rateControlTasksConsumerFifoPtrArray[0],
        encHandlePtr->rateControlResultsProducerFifoPtrArray[0],
		encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->intraPeriodLength);
	if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    
	
	// Mode Decision Configuration Contexts
    {
		// Mode Decision Configuration Contexts
        EB_MALLOC(EB_PTR*, encHandlePtr->modeDecisionConfigurationContextPtrArray, sizeof(EB_PTR) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount, EB_N_PTR);

        for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount; ++processIndex) {
        	return_error = ModeDecisionConfigurationContextCtor(
                (ModeDecisionConfigurationContext_t**) &encHandlePtr->modeDecisionConfigurationContextPtrArray[processIndex],
                encHandlePtr->rateControlResultsConsumerFifoPtrArray[processIndex],

                encHandlePtr->encDecTasksProducerFifoPtrArray[EncDecPortLookup(ENCDEC_INPUT_PORT_MDC, processIndex)],
                ((encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->maxInputLumaWidth  + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE) *
                ((encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->maxInputLumaHeight + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE) );

            
		    if (return_error == EB_ErrorInsufficientResources){
            	return EB_ErrorInsufficientResources;
        	}
        }
    }

    maxPictureWidth = 0;
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        if(maxPictureWidth < encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth) {
            maxPictureWidth = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth;
        }
    }

    // EncDec Contexts
    EB_MALLOC(EB_PTR*, encHandlePtr->encDecContextPtrArray, sizeof(EB_PTR) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount, EB_N_PTR);
    
    for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount; ++processIndex) {
        return_error = EncDecContextCtor(
            (EncDecContext_t**) &encHandlePtr->encDecContextPtrArray[processIndex],
            encHandlePtr->encDecTasksConsumerFifoPtrArray[processIndex],
            encHandlePtr->encDecResultsProducerFifoPtrArray[processIndex],
            encHandlePtr->encDecTasksProducerFifoPtrArray[EncDecPortLookup(ENCDEC_INPUT_PORT_ENCDEC, processIndex)],
            encHandlePtr->pictureDemuxResultsProducerFifoPtrArray[1 + processIndex], // Add port lookup logic here JMJ
            is16bit);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // Entropy Coding Contexts
    EB_MALLOC(EB_PTR*, encHandlePtr->entropyCodingContextPtrArray, sizeof(EB_PTR) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount, EB_N_PTR);
    
    for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount; ++processIndex) {
        return_error = EntropyCodingContextCtor(
            (EntropyCodingContext_t**) &encHandlePtr->entropyCodingContextPtrArray[processIndex],
            encHandlePtr->encDecResultsConsumerFifoPtrArray[processIndex],
            encHandlePtr->entropyCodingResultsProducerFifoPtrArray[processIndex],
            encHandlePtr->rateControlTasksProducerFifoPtrArray[RateControlPortLookup(RATE_CONTROL_INPUT_PORT_ENTROPY_CODING, processIndex)],
            is16bit);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // Packetization Context
    return_error = PacketizationContextCtor(
        (PacketizationContext_t**) &encHandlePtr->packetizationContextPtr,
        encHandlePtr->entropyCodingResultsConsumerFifoPtrArray[0],
        encHandlePtr->rateControlTasksProducerFifoPtrArray[RateControlPortLookup(RATE_CONTROL_INPUT_PORT_PACKETIZATION, 0)]);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    /************************************
     * Thread Handles
     ************************************/
    EB_H265_ENC_CONFIGURATION   *configPtr = &encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig;
    EbSetThreadManagementParameters(configPtr);    

    // Resource Coordination
    EB_CREATETHREAD(EB_HANDLE, encHandlePtr->resourceCoordinationThreadHandle, sizeof(EB_HANDLE), EB_THREAD, ResourceCoordinationKernel, encHandlePtr->resourceCoordinationContextPtr);

    // Picture Analysis
    EB_MALLOC(EB_HANDLE*, encHandlePtr->pictureAnalysisThreadHandleArray, sizeof(EB_HANDLE) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount, EB_N_PTR);

    for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount; ++processIndex) {
        EB_CREATETHREAD(EB_HANDLE, encHandlePtr->pictureAnalysisThreadHandleArray[processIndex], sizeof(EB_HANDLE), EB_THREAD, PictureAnalysisKernel, encHandlePtr->pictureAnalysisContextPtrArray[processIndex]);
    }
    
    // Picture Decision
    EB_CREATETHREAD(EB_HANDLE, encHandlePtr->pictureDecisionThreadHandle, sizeof(EB_HANDLE), EB_THREAD, PictureDecisionKernel, encHandlePtr->pictureDecisionContextPtr);

    // Motion Estimation
    EB_MALLOC(EB_HANDLE*, encHandlePtr->motionEstimationThreadHandleArray, sizeof(EB_HANDLE) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount, EB_N_PTR);

    for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount; ++processIndex) {
        EB_CREATETHREAD(EB_HANDLE, encHandlePtr->motionEstimationThreadHandleArray[processIndex], sizeof(EB_HANDLE), EB_THREAD, MotionEstimationKernel, encHandlePtr->motionEstimationContextPtrArray[processIndex]);
    }
    
    // Initial Rate Control
    EB_CREATETHREAD(EB_HANDLE, encHandlePtr->initialRateControlThreadHandle, sizeof(EB_HANDLE), EB_THREAD, InitialRateControlKernel, encHandlePtr->initialRateControlContextPtr);

	// Source Based Oprations 
	EB_MALLOC(EB_HANDLE*, encHandlePtr->sourceBasedOperationsThreadHandleArray, sizeof(EB_HANDLE) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount, EB_N_PTR);

	for (processIndex = 0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount; ++processIndex) {
		EB_CREATETHREAD(EB_HANDLE, encHandlePtr->sourceBasedOperationsThreadHandleArray[processIndex], sizeof(EB_HANDLE), EB_THREAD, SourceBasedOperationsKernel, encHandlePtr->sourceBasedOperationsContextPtrArray[processIndex]);
	}

    // Picture Manager
    EB_CREATETHREAD(EB_HANDLE, encHandlePtr->pictureManagerThreadHandle, sizeof(EB_HANDLE), EB_THREAD, PictureManagerKernel, encHandlePtr->pictureManagerContextPtr);

    // Rate Control
    EB_CREATETHREAD(EB_HANDLE, encHandlePtr->rateControlThreadHandle, sizeof(EB_HANDLE), EB_THREAD, RateControlKernel, encHandlePtr->rateControlContextPtr);

    // Mode Decision Configuration Process
    EB_MALLOC(EB_HANDLE*, encHandlePtr->modeDecisionConfigurationThreadHandleArray, sizeof(EB_HANDLE) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount, EB_N_PTR);

    for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount; ++processIndex) {
        EB_CREATETHREAD(EB_HANDLE, encHandlePtr->modeDecisionConfigurationThreadHandleArray[processIndex], sizeof(EB_HANDLE), EB_THREAD, ModeDecisionConfigurationKernel, encHandlePtr->modeDecisionConfigurationContextPtrArray[processIndex]);
    }

    // EncDec Process
    EB_MALLOC(EB_HANDLE*, encHandlePtr->encDecThreadHandleArray, sizeof(EB_HANDLE) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount, EB_N_PTR);

    for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount; ++processIndex) {
        EB_CREATETHREAD(EB_HANDLE, encHandlePtr->encDecThreadHandleArray[processIndex], sizeof(EB_HANDLE), EB_THREAD, EncDecKernel, encHandlePtr->encDecContextPtrArray[processIndex]);
    }

    // Entropy Coding Process
    EB_MALLOC(EB_HANDLE*, encHandlePtr->entropyCodingThreadHandleArray, sizeof(EB_HANDLE) * encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount, EB_N_PTR);

    for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount; ++processIndex) {
        EB_CREATETHREAD(EB_HANDLE, encHandlePtr->entropyCodingThreadHandleArray[processIndex], sizeof(EB_HANDLE), EB_THREAD, EntropyCodingKernel, encHandlePtr->entropyCodingContextPtrArray[processIndex]);
    }

    // Packetization
    EB_CREATETHREAD(EB_HANDLE, encHandlePtr->packetizationThreadHandle, sizeof(EB_HANDLE), EB_THREAD, PacketizationKernel, encHandlePtr->packetizationContextPtr);

#if DISPLAY_MEMORY
    EB_MEMORY();
#endif
    return return_error;
}

/**********************************
 * DeInitialize Encoder Library
 **********************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbDeinitEncoder(EB_COMPONENTTYPE *h265EncComponent)
{
    EbEncHandle_t *encHandlePtr = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_S32              ptrIndex     = 0 ;
    EbMemoryMapEntry*   memoryEntry  = (EbMemoryMapEntry*)EB_NULL;
    if (encHandlePtr){
        if (encHandlePtr->memoryMapIndex){
    // Loop through the ptr table and free all malloc'd pointers per channel
            for (ptrIndex = (encHandlePtr->memoryMapIndex) - 1; ptrIndex >= 0; --ptrIndex){
                memoryEntry = &encHandlePtr->memoryMap[ptrIndex];
        switch (memoryEntry->ptrType){
        case EB_N_PTR:
            free(memoryEntry->ptr);
            break;
        case EB_A_PTR:
#ifdef _MSC_VER
            _aligned_free(memoryEntry->ptr);
#else
            free(memoryEntry->ptr);
#endif
            break;
        case EB_SEMAPHORE:
            EbDestroySemaphore(memoryEntry->ptr);
            break;
        case EB_THREAD:
            EbDestroyThread(memoryEntry->ptr);
            break;
        case EB_MUTEX:
            EbDestroyMutex(memoryEntry->ptr);
            break;
        default:
            return_error = EB_ErrorMax;
            break;           
        }
    }
            if (encHandlePtr->memoryMap != (EbMemoryMapEntry*) NULL) {
                free(encHandlePtr->memoryMap);
	}
    
            //(void)(encHandlePtr);
        }
    }   
    return return_error;
}

EB_ERRORTYPE EbH265EncInitParameter(
    EB_H265_ENC_CONFIGURATION * configPtr);

/**********************************
 * GetHandle
 **********************************/
#if __linux
__attribute__((visibility("default"))) 
#endif
EB_API EB_ERRORTYPE EbInitHandle(
    EB_COMPONENTTYPE** pHandle,               // Function to be called in the future for manipulating the component
    void*              pAppData,
    EB_H265_ENC_CONFIGURATION  *configPtr)              // Pointer passed back to the client during callbacks
            
{
    EB_ERRORTYPE           return_error = EB_ErrorNone;

    *pHandle = (EB_COMPONENTTYPE*) malloc(sizeof(EB_COMPONENTTYPE));
    if (*pHandle != (EB_HANDLETYPE) NULL) {
        
        // Init Component OS objects (threads, semaphores, etc.)
        // also links the various Component control functions
        return_error = InitH265EncoderHandle(*pHandle);

        if (return_error == EB_ErrorNone) {
            ((EB_COMPONENTTYPE*)(*pHandle))->pApplicationPrivate = pAppData;

        }else if (return_error == EB_ErrorInsufficientResources){
            EbDeinitEncoder((EB_COMPONENTTYPE*)NULL);
            *pHandle     = (EB_COMPONENTTYPE*) NULL;
        }else {
            return_error = EB_ErrorInvalidComponent;
        }
    }
    else {
        printf("Error: Component Struct Malloc Failed\n");
        return_error = EB_ErrorInsufficientResources;
    }
    return_error = EbH265EncInitParameter(configPtr);

    return return_error;
}

/**********************************
 * Encoder Componenet DeInit
 **********************************/
EB_ERRORTYPE EbH265EncComponentDeInit(EB_COMPONENTTYPE  *h265EncComponent)
{
    EB_ERRORTYPE       return_error        = EB_ErrorNone;

    if (h265EncComponent->pComponentPrivate) {
        free((EbEncHandle_t *)h265EncComponent->pComponentPrivate);
    }
    else {
        return_error = EB_ErrorUndefined;
    }

    return return_error;
}

/**********************************
 * EbDeinitHandle
 **********************************/
#if __linux
__attribute__((visibility("default"))) 
#endif
EB_API EB_ERRORTYPE EbDeinitHandle(
    EB_COMPONENTTYPE  *h265EncComponent)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    if (h265EncComponent) {
        return_error = EbH265EncComponentDeInit(h265EncComponent);

        free(h265EncComponent);
    }
    else {
        return_error = EB_ErrorInvalidComponent;
    }

    return return_error;
}

#define SCD_LAD 6
#define INPUT_SIZE_4K_TH				0x29F630	// 2.75 Million  
EB_U32 SetParentPcs(EB_H265_ENC_CONFIGURATION*   config)
{

    EB_U32 inputPic = 100;
    EB_U32 fps = (EB_U32)((config->frameRate > 1000) ? config->frameRate >> 16 : config->frameRate);

    fps = fps > 120 ? 120 : fps;
    fps = fps < 24 ? 24 : fps;

    EB_U32     lowLatencyInput = (config->encMode < 6 || config->speedControlFlag == 1) ? fps :
        (config->encMode < 8) ? fps >> 1 : (EB_U32)((2 << config->hierarchicalLevels) + SCD_LAD);

    EB_U32     normalLatencyInput = (fps * 3) >> 1;

    if ((config->sourceWidth * config->sourceHeight) > INPUT_SIZE_4K_TH)
        normalLatencyInput = (normalLatencyInput * 3) >> 1;

    if (config->latencyMode == 0)
        inputPic = (normalLatencyInput /*+ config->lookAheadDistance*/);
    else
        inputPic = (EB_U32)(lowLatencyInput /*+ config->lookAheadDistance*/);
    
    return inputPic;
}

void LoadDefaultBufferConfigurationSettings(
    SequenceControlSet_t       *sequenceControlSetPtr
)
{
    EB_U32 encDecSegH = ((sequenceControlSetPtr->maxInputLumaHeight + 32) / MAX_LCU_SIZE);
    EB_U32 encDecSegW = ((sequenceControlSetPtr->maxInputLumaWidth + 32) / MAX_LCU_SIZE);

    EB_U32 meSegH = (((sequenceControlSetPtr->maxInputLumaHeight + 32) / MAX_LCU_SIZE) < 6) ? 1 : 6;
    EB_U32 meSegW = (((sequenceControlSetPtr->maxInputLumaWidth + 32) / MAX_LCU_SIZE) < 10) ? 1 : 10;

    EB_U32 inputPic = SetParentPcs(&sequenceControlSetPtr->staticConfig);

    unsigned int coreCount = GetNumCores();

    sequenceControlSetPtr->staticConfig.inputOutputBufferFifoInitCount = inputPic + sequenceControlSetPtr->staticConfig.lookAheadDistance + SCD_LAD;
    
    // ME segments
    sequenceControlSetPtr->meSegmentRowCountArray[0] = meSegH;
    sequenceControlSetPtr->meSegmentRowCountArray[1] = meSegH;
    sequenceControlSetPtr->meSegmentRowCountArray[2] = meSegH;
    sequenceControlSetPtr->meSegmentRowCountArray[3] = meSegH;
    sequenceControlSetPtr->meSegmentRowCountArray[4] = meSegH;
    sequenceControlSetPtr->meSegmentRowCountArray[5] = meSegH;

    sequenceControlSetPtr->meSegmentColumnCountArray[0] = meSegW;
    sequenceControlSetPtr->meSegmentColumnCountArray[1] = meSegW;
    sequenceControlSetPtr->meSegmentColumnCountArray[2] = meSegW;
    sequenceControlSetPtr->meSegmentColumnCountArray[3] = meSegW;
    sequenceControlSetPtr->meSegmentColumnCountArray[4] = meSegW;
    sequenceControlSetPtr->meSegmentColumnCountArray[5] = meSegW;

    // EncDec segments     
    sequenceControlSetPtr->encDecSegmentRowCountArray[0] = encDecSegH;
    sequenceControlSetPtr->encDecSegmentRowCountArray[1] = encDecSegH;
    sequenceControlSetPtr->encDecSegmentRowCountArray[2] = encDecSegH;
    sequenceControlSetPtr->encDecSegmentRowCountArray[3] = encDecSegH;
    sequenceControlSetPtr->encDecSegmentRowCountArray[4] = encDecSegH;
    sequenceControlSetPtr->encDecSegmentRowCountArray[5] = encDecSegH;

    sequenceControlSetPtr->encDecSegmentColCountArray[0] = encDecSegW;
    sequenceControlSetPtr->encDecSegmentColCountArray[1] = encDecSegW;
    sequenceControlSetPtr->encDecSegmentColCountArray[2] = encDecSegW;
    sequenceControlSetPtr->encDecSegmentColCountArray[3] = encDecSegW;
    sequenceControlSetPtr->encDecSegmentColCountArray[4] = encDecSegW;
    sequenceControlSetPtr->encDecSegmentColCountArray[5] = encDecSegW;

    //#====================== Data Structures and Picture Buffers ======================
    sequenceControlSetPtr->pictureControlSetPoolInitCount       = inputPic;
    sequenceControlSetPtr->pictureControlSetPoolInitCountChild  = MAX(4, coreCount / 6);
    sequenceControlSetPtr->referencePictureBufferInitCount      = sequenceControlSetPtr->staticConfig.inputOutputBufferFifoInitCount;//MAX((EB_U32)(sequenceControlSetPtr->staticConfig.inputOutputBufferFifoInitCount >> 1), (EB_U32)((1 << sequenceControlSetPtr->staticConfig.hierarchicalLevels) + 2));
    sequenceControlSetPtr->paReferencePictureBufferInitCount    = sequenceControlSetPtr->staticConfig.inputOutputBufferFifoInitCount;//MAX((EB_U32)(sequenceControlSetPtr->staticConfig.inputOutputBufferFifoInitCount >> 1), (EB_U32)((1 << sequenceControlSetPtr->staticConfig.hierarchicalLevels) + 2));
    sequenceControlSetPtr->reconBufferFifoInitCount             = sequenceControlSetPtr->referencePictureBufferInitCount;
    
    //#====================== Inter process Fifos ======================
    sequenceControlSetPtr->resourceCoordinationFifoInitCount = 300;
    sequenceControlSetPtr->pictureAnalysisFifoInitCount = 300;
    sequenceControlSetPtr->pictureDecisionFifoInitCount = 300;
    sequenceControlSetPtr->initialRateControlFifoInitCount = 300;
    sequenceControlSetPtr->pictureDemuxFifoInitCount = 300;
    sequenceControlSetPtr->rateControlTasksFifoInitCount = 300;
    sequenceControlSetPtr->rateControlFifoInitCount = 301;
    sequenceControlSetPtr->modeDecisionFifoInitCount = 300;
    sequenceControlSetPtr->modeDecisionConfigurationFifoInitCount = 300;
    sequenceControlSetPtr->motionEstimationFifoInitCount = 300;
    sequenceControlSetPtr->entropyCodingFifoInitCount = 300;
    sequenceControlSetPtr->encDecFifoInitCount = 300;

    //#====================== Processes number ======================
    sequenceControlSetPtr->totalProcessInitCount = 0;
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->pictureAnalysisProcessInitCount              = MAX(15, coreCount / 6);
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->motionEstimationProcessInitCount             = MAX(20, coreCount / 3);
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->sourceBasedOperationsProcessInitCount        = MAX(3, coreCount / 12);
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount    = MAX(3, coreCount / 12);
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->encDecProcessInitCount                       = MAX(40, coreCount);
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->entropyCodingProcessInitCount                = MAX(3, coreCount / 12);

    sequenceControlSetPtr->totalProcessInitCount += 6; // single processes count
    printf("Number of cores available: %u\nNumber of PPCS %u\n", coreCount >> 1 , inputPic);

    return;

}

// Sets the default intra period the closest possible to 1 second without breaking the minigop
static EB_S32 ComputeIntraPeriod(
    SequenceControlSet_t       *sequenceControlSetPtr)
{
    EB_S32 intraPeriod = 0;
    EB_H265_ENC_CONFIGURATION   *config = &sequenceControlSetPtr->staticConfig;
    EB_S32 fps = config->frameRate < 1000 ? config->frameRate : config->frameRate >> 16;

    if (fps == 30) {
        intraPeriod = 31;
    }
    else {
        intraPeriod = ((int)((fps) / (1 << (config->hierarchicalLevels))))*(1 << (config->hierarchicalLevels));
        intraPeriod -= 1;
    }

    return intraPeriod;

}

EB_ERRORTYPE EbAppVideoUsabilityInfoInit(
    AppVideoUsabilityInfo_t *vuiPtr)
{
    AppHrdParameters_t* hrdParamPtr;
    
    hrdParamPtr = vuiPtr->hrdParametersPtr;
    // Initialize vui variables

    vuiPtr->aspectRatioInfoPresentFlag = EB_TRUE;
    vuiPtr->aspectRatioIdc = 0;
    vuiPtr->sarWidth = 0;
    vuiPtr->sarHeight = 0;

    vuiPtr->overscanInfoPresentFlag = EB_FALSE;
    vuiPtr->overscanApproriateFlag = EB_FALSE;
    vuiPtr->videoSignalTypePresentFlag = EB_FALSE;

    vuiPtr->videoFormat = 0;
    vuiPtr->videoFullRangeFlag = EB_FALSE;

    vuiPtr->colorDescriptionPresentFlag = EB_FALSE;
    vuiPtr->colorPrimaries = 0;
    vuiPtr->transferCharacteristics = 0;
    vuiPtr->matrixCoeffs = 0;

    vuiPtr->chromaLocInfoPresentFlag = EB_FALSE;
    vuiPtr->chromaSampleLocTypeTopField = 0;
    vuiPtr->chromaSampleLocTypeBottomField = 0;

    vuiPtr->neutralChromaIndicationFlag = EB_FALSE;
    vuiPtr->fieldSeqFlag = EB_FALSE;
    vuiPtr->frameFieldInfoPresentFlag = EB_FALSE;//EB_TRUE;

    vuiPtr->defaultDisplayWindowFlag = EB_TRUE;
    vuiPtr->defaultDisplayWinLeftOffset = 0;
    vuiPtr->defaultDisplayWinRightOffset = 0;
    vuiPtr->defaultDisplayWinTopOffset = 0;
    vuiPtr->defaultDisplayWinBottomOffset = 0;

    vuiPtr->vuiTimingInfoPresentFlag = EB_FALSE;//EB_TRUE;
    vuiPtr->vuiNumUnitsInTick = 0;
    vuiPtr->vuiTimeScale = 0;

    vuiPtr->vuiPocPropotionalTimingFlag = EB_FALSE;
    vuiPtr->vuiNumTicksPocDiffOneMinus1 = 0;

    vuiPtr->vuiHrdParametersPresentFlag = EB_FALSE;//EB_TRUE;

    vuiPtr->bitstreamRestrictionFlag = EB_FALSE;

    vuiPtr->motionVectorsOverPicBoundariesFlag = EB_FALSE;
    vuiPtr->restrictedRefPicListsFlag = EB_FALSE;

    vuiPtr->minSpatialSegmentationIdc = 0;
    vuiPtr->maxBytesPerPicDenom = 0;
    vuiPtr->maxBitsPerMinCuDenom = 0;
    vuiPtr->log2MaxMvLengthHorizontal = 0;
    vuiPtr->log2MaxMvLengthVertical = 0;

    // Initialize HRD parameters
    hrdParamPtr->nalHrdParametersPresentFlag = EB_FALSE;//EB_TRUE;
    hrdParamPtr->vclHrdParametersPresentFlag = EB_FALSE; //EB_TRUE;
    hrdParamPtr->subPicCpbParamsPresentFlag = EB_FALSE;//EB_TRUE;

    hrdParamPtr->tickDivisorMinus2 = 0;
    hrdParamPtr->duCpbRemovalDelayLengthMinus1 = 0;

    hrdParamPtr->subPicCpbParamsPicTimingSeiFlag = EB_FALSE;//EB_TRUE;

    hrdParamPtr->dpbOutputDelayDuLengthMinus1 = 0;

    hrdParamPtr->bitRateScale = 0;
    hrdParamPtr->cpbSizeScale = 0;
    hrdParamPtr->duCpbSizeScale = 0;

    hrdParamPtr->initialCpbRemovalDelayLengthMinus1 = 0;
    hrdParamPtr->auCpbRemovalDelayLengthMinus1 = 0;
    hrdParamPtr->dpbOutputDelayLengthMinus1 = 0;

    EB_MEMSET(
        hrdParamPtr->fixedPicRateGeneralFlag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrdParamPtr->fixedPicRateWithinCvsFlag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrdParamPtr->elementalDurationTcMinus1,
        EB_FALSE,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrdParamPtr->lowDelayHrdFlag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS);

    EB_MEMSET(
        hrdParamPtr->cpbCountMinus1,
        0,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS);
    //hrdParamPtr->cpbCountMinus1[0] = 2;

    EB_MEMSET(
        hrdParamPtr->bitRateValueMinus1,
        EB_FALSE,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS * 2 * MAX_CPB_COUNT);

    EB_MEMSET(
        hrdParamPtr->cpbSizeValueMinus1,
        EB_FALSE,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS * 2 * MAX_CPB_COUNT);

    EB_MEMSET(
        hrdParamPtr->bitRateDuValueMinus1,
        EB_FALSE,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS * 2 * MAX_CPB_COUNT);

    EB_MEMSET(
        hrdParamPtr->cpbSizeDuValueMinus1,
        EB_FALSE,
        sizeof(EB_U32)*MAX_TEMPORAL_LAYERS * 2 * MAX_CPB_COUNT);

    EB_MEMSET(
        hrdParamPtr->cbrFlag,
        EB_FALSE,
        sizeof(EB_BOOL)*MAX_TEMPORAL_LAYERS * 2 * MAX_CPB_COUNT);

    hrdParamPtr->cpbDpbDelaysPresentFlag = (EB_BOOL)((hrdParamPtr->nalHrdParametersPresentFlag || hrdParamPtr->vclHrdParametersPresentFlag) && vuiPtr->vuiHrdParametersPresentFlag);

    return EB_ErrorNone;
}

// Set configurations for the hardcoded parameters
void SetDefaultConfigurationParameters(
    SequenceControlSet_t       *sequenceControlSetPtr)
{

    // LCU Definitions
    sequenceControlSetPtr->lcuSize = MAX_LCU_SIZE;
    sequenceControlSetPtr->maxLcuDepth = (EB_U8)EB_MAX_LCU_DEPTH;

    // No Cropping Window
    sequenceControlSetPtr->conformanceWindowFlag = 0;
    sequenceControlSetPtr->croppingLeftOffset = 0;
    sequenceControlSetPtr->croppingRightOffset = 0;
    sequenceControlSetPtr->croppingTopOffset = 0;
    sequenceControlSetPtr->croppingBottomOffset = 0;

    // Coding Structure
    sequenceControlSetPtr->enableQpScalingFlag = EB_TRUE;

    //Denoise
    sequenceControlSetPtr->enableDenoiseFlag = EB_TRUE;

    return;
}

EB_U32 ComputeDefaultLookAhead(
    EB_H265_ENC_CONFIGURATION*   config)
{
    EB_S32 lad = 0;
    if (config->rateControlMode == 0)
        lad = 17;
    else
        lad = config->intraPeriodLength;


    return lad;
}

void SetParamBasedOnInput(
    SequenceControlSet_t       *sequenceControlSetPtr)

{
    if (sequenceControlSetPtr->interlacedVideo == EB_FALSE) {

        sequenceControlSetPtr->generalFrameOnlyConstraintFlag = 0;
        sequenceControlSetPtr->generalProgressiveSourceFlag = 1;
        sequenceControlSetPtr->generalInterlacedSourceFlag = 0;
        sequenceControlSetPtr->videoUsabilityInfoPtr->fieldSeqFlag = EB_FALSE;
        sequenceControlSetPtr->videoUsabilityInfoPtr->frameFieldInfoPresentFlag = EB_FALSE;
    }
    else {
        sequenceControlSetPtr->generalFrameOnlyConstraintFlag = 0;
        sequenceControlSetPtr->generalProgressiveSourceFlag = 0;
        sequenceControlSetPtr->generalInterlacedSourceFlag = 1;
        sequenceControlSetPtr->videoUsabilityInfoPtr->fieldSeqFlag = EB_TRUE;
        sequenceControlSetPtr->videoUsabilityInfoPtr->frameFieldInfoPresentFlag = EB_TRUE;
    }

    // Update picture width, and picture height
    if (sequenceControlSetPtr->maxInputLumaWidth % MIN_CU_SIZE) {

        sequenceControlSetPtr->maxInputPadRight = MIN_CU_SIZE - (sequenceControlSetPtr->maxInputLumaWidth % MIN_CU_SIZE);
        sequenceControlSetPtr->maxInputLumaWidth = sequenceControlSetPtr->maxInputLumaWidth + sequenceControlSetPtr->maxInputPadRight;
        sequenceControlSetPtr->maxInputChromaWidth = sequenceControlSetPtr->maxInputLumaWidth >> 1;
    }
    else {

        sequenceControlSetPtr->maxInputPadRight = 0;
    }
    if (sequenceControlSetPtr->maxInputLumaHeight % MIN_CU_SIZE) {

        sequenceControlSetPtr->maxInputPadBottom = MIN_CU_SIZE - (sequenceControlSetPtr->maxInputLumaHeight % MIN_CU_SIZE);
        sequenceControlSetPtr->maxInputLumaHeight = sequenceControlSetPtr->maxInputLumaHeight + sequenceControlSetPtr->maxInputPadBottom;
        sequenceControlSetPtr->maxInputChromaHeight = sequenceControlSetPtr->maxInputLumaHeight >> 1;
    }
    else {
        sequenceControlSetPtr->maxInputPadBottom = 0;
    }

    // Configure the padding
    sequenceControlSetPtr->leftPadding  = MAX_LCU_SIZE + 4;
    sequenceControlSetPtr->topPadding   = MAX_LCU_SIZE + 4;
    sequenceControlSetPtr->rightPadding = MAX_LCU_SIZE + 4;
    sequenceControlSetPtr->botPadding   = MAX_LCU_SIZE + 4;
    sequenceControlSetPtr->chromaWidth  = sequenceControlSetPtr->maxInputLumaWidth >> 1;
    sequenceControlSetPtr->chromaHeight = sequenceControlSetPtr->maxInputLumaHeight >> 1;
    sequenceControlSetPtr->lumaWidth    = sequenceControlSetPtr->maxInputLumaWidth;
    sequenceControlSetPtr->lumaHeight   = sequenceControlSetPtr->maxInputLumaHeight;

    DeriveInputResolution(
        sequenceControlSetPtr,
        sequenceControlSetPtr->lumaWidth*sequenceControlSetPtr->lumaHeight);

}

void CopyApiFromApp(
    SequenceControlSet_t       *sequenceControlSetPtr,
    EB_H265_ENC_CONFIGURATION* pComponentParameterStructure
) {

    EB_U32                  hmeRegionIndex = 0;

    sequenceControlSetPtr->staticConfig.sourceWidth  = (EB_U16)((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->sourceWidth;
    sequenceControlSetPtr->staticConfig.sourceHeight = (EB_U16)((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->sourceHeight;
    sequenceControlSetPtr->maxInputLumaWidth  = (EB_U16)((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->sourceWidth;
    sequenceControlSetPtr->maxInputLumaHeight = (EB_U16)((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->sourceHeight;

    // Interlaced Video
    sequenceControlSetPtr->staticConfig.interlacedVideo = sequenceControlSetPtr->interlacedVideo = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->interlacedVideo;

    // Coding Structure
    sequenceControlSetPtr->staticConfig.intraPeriodLength = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->intraPeriodLength;
    sequenceControlSetPtr->staticConfig.intraRefreshType = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->intraRefreshType;
	sequenceControlSetPtr->staticConfig.predStructure = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->predStructure;
    sequenceControlSetPtr->staticConfig.baseLayerSwitchMode = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->baseLayerSwitchMode;
    sequenceControlSetPtr->staticConfig.hierarchicalLevels = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->hierarchicalLevels;
	
    sequenceControlSetPtr->staticConfig.tune = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->tune;
    sequenceControlSetPtr->staticConfig.encMode = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->encMode;
    sequenceControlSetPtr->staticConfig.codeVpsSpsPps = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->codeVpsSpsPps;
    if (sequenceControlSetPtr->staticConfig.tune == 1) {
        sequenceControlSetPtr->staticConfig.bitRateReduction = 0;
        sequenceControlSetPtr->staticConfig.improveSharpness = 0;
    }
    else {
        sequenceControlSetPtr->staticConfig.bitRateReduction = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->bitRateReduction;
        sequenceControlSetPtr->staticConfig.improveSharpness = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->improveSharpness;
    }

    sequenceControlSetPtr->intraPeriodLength = sequenceControlSetPtr->staticConfig.intraPeriodLength;
    sequenceControlSetPtr->intraRefreshType = sequenceControlSetPtr->staticConfig.intraRefreshType;
    sequenceControlSetPtr->maxTemporalLayers = sequenceControlSetPtr->staticConfig.hierarchicalLevels;
    sequenceControlSetPtr->maxRefCount = 1;


    // Quantization
    sequenceControlSetPtr->qp = sequenceControlSetPtr->staticConfig.qp = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->qp;
    sequenceControlSetPtr->staticConfig.useQpFile = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->useQpFile;

    // Deblock Filter
    sequenceControlSetPtr->staticConfig.disableDlfFlag = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->disableDlfFlag;

    // SAO
    sequenceControlSetPtr->staticConfig.enableSaoFlag = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->enableSaoFlag;

    // ME Tools
    sequenceControlSetPtr->staticConfig.useDefaultMeHme = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->useDefaultMeHme;
    
    // Default HME/ME settings
    sequenceControlSetPtr->staticConfig.enableHmeFlag = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->enableHmeFlag;
    sequenceControlSetPtr->staticConfig.enableHmeLevel0Flag = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->enableHmeLevel0Flag;
    sequenceControlSetPtr->staticConfig.enableHmeLevel1Flag = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->enableHmeLevel1Flag;
    sequenceControlSetPtr->staticConfig.enableHmeLevel2Flag = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->enableHmeLevel2Flag;
    sequenceControlSetPtr->staticConfig.searchAreaWidth = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->searchAreaWidth;
    sequenceControlSetPtr->staticConfig.searchAreaHeight = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->searchAreaHeight;
    sequenceControlSetPtr->staticConfig.numberHmeSearchRegionInWidth = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->numberHmeSearchRegionInWidth;
    sequenceControlSetPtr->staticConfig.numberHmeSearchRegionInHeight = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->numberHmeSearchRegionInHeight;
    sequenceControlSetPtr->staticConfig.hmeLevel0TotalSearchAreaWidth = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->hmeLevel0TotalSearchAreaWidth;
    sequenceControlSetPtr->staticConfig.hmeLevel0TotalSearchAreaHeight = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->hmeLevel0TotalSearchAreaHeight;
    
    for (hmeRegionIndex = 0; hmeRegionIndex < sequenceControlSetPtr->staticConfig.numberHmeSearchRegionInWidth; ++hmeRegionIndex) {
        sequenceControlSetPtr->staticConfig.hmeLevel0SearchAreaInWidthArray[hmeRegionIndex] = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->hmeLevel0SearchAreaInWidthArray[hmeRegionIndex];
        sequenceControlSetPtr->staticConfig.hmeLevel1SearchAreaInWidthArray[hmeRegionIndex] = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->hmeLevel1SearchAreaInWidthArray[hmeRegionIndex];
        sequenceControlSetPtr->staticConfig.hmeLevel2SearchAreaInWidthArray[hmeRegionIndex] = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->hmeLevel2SearchAreaInWidthArray[hmeRegionIndex];
    }
    
    for (hmeRegionIndex = 0; hmeRegionIndex < sequenceControlSetPtr->staticConfig.numberHmeSearchRegionInHeight; ++hmeRegionIndex) {
        sequenceControlSetPtr->staticConfig.hmeLevel0SearchAreaInHeightArray[hmeRegionIndex] = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->hmeLevel0SearchAreaInHeightArray[hmeRegionIndex];
        sequenceControlSetPtr->staticConfig.hmeLevel1SearchAreaInHeightArray[hmeRegionIndex] = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->hmeLevel1SearchAreaInHeightArray[hmeRegionIndex];
        sequenceControlSetPtr->staticConfig.hmeLevel2SearchAreaInHeightArray[hmeRegionIndex] = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->hmeLevel2SearchAreaInHeightArray[hmeRegionIndex];
    }

    // MD Parameters
    sequenceControlSetPtr->staticConfig.constrainedIntra = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->constrainedIntra;

    // Rate Control
    sequenceControlSetPtr->staticConfig.sceneChangeDetection = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->sceneChangeDetection;
    sequenceControlSetPtr->staticConfig.rateControlMode = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->rateControlMode;
    sequenceControlSetPtr->staticConfig.lookAheadDistance = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->lookAheadDistance;
    sequenceControlSetPtr->staticConfig.framesToBeEncoded = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->framesToBeEncoded;
    sequenceControlSetPtr->frameRate = sequenceControlSetPtr->staticConfig.frameRate = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->frameRate;
    sequenceControlSetPtr->staticConfig.targetBitRate = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->targetBitRate;
    sequenceControlSetPtr->encodeContextPtr->availableTargetBitRate = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->targetBitRate;

    sequenceControlSetPtr->staticConfig.maxQpAllowed = (sequenceControlSetPtr->staticConfig.rateControlMode) ?
        ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->maxQpAllowed :
        51;

    sequenceControlSetPtr->staticConfig.minQpAllowed = (sequenceControlSetPtr->staticConfig.rateControlMode) ?
        ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->minQpAllowed :
        0;

    // Misc
    sequenceControlSetPtr->staticConfig.encoderBitDepth = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->encoderBitDepth;
    sequenceControlSetPtr->staticConfig.compressedTenBitFormat = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->compressedTenBitFormat;
	sequenceControlSetPtr->staticConfig.videoUsabilityInfo = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->videoUsabilityInfo;
    sequenceControlSetPtr->staticConfig.highDynamicRangeInput = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->highDynamicRangeInput;
    sequenceControlSetPtr->staticConfig.accessUnitDelimiter = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->accessUnitDelimiter;
    sequenceControlSetPtr->staticConfig.bufferingPeriodSEI = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->bufferingPeriodSEI;
    sequenceControlSetPtr->staticConfig.pictureTimingSEI = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->pictureTimingSEI;
    sequenceControlSetPtr->staticConfig.registeredUserDataSeiFlag = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->registeredUserDataSeiFlag;
    sequenceControlSetPtr->staticConfig.unregisteredUserDataSeiFlag = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->unregisteredUserDataSeiFlag;
    sequenceControlSetPtr->staticConfig.recoveryPointSeiFlag = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->recoveryPointSeiFlag;
    sequenceControlSetPtr->staticConfig.enableTemporalId = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->enableTemporalId;

    // Annex A parameters
    sequenceControlSetPtr->staticConfig.profile = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->profile;
    sequenceControlSetPtr->staticConfig.tier = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->tier;
    sequenceControlSetPtr->staticConfig.level = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->level;

    sequenceControlSetPtr->staticConfig.injectorFrameRate   = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->injectorFrameRate;
    sequenceControlSetPtr->staticConfig.speedControlFlag    = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->speedControlFlag;
    sequenceControlSetPtr->staticConfig.latencyMode         = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->latencyMode;    
    sequenceControlSetPtr->staticConfig.asmType = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->asmType;
    sequenceControlSetPtr->staticConfig.channelId = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->channelId;
    sequenceControlSetPtr->staticConfig.activeChannelCount = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->activeChannelCount;
    sequenceControlSetPtr->staticConfig.useRoundRobinThreadAssignment = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->useRoundRobinThreadAssignment;

    sequenceControlSetPtr->staticConfig.frameRateDenominator = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->frameRateDenominator;
    sequenceControlSetPtr->staticConfig.frameRateNumerator = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->frameRateNumerator;

    sequenceControlSetPtr->staticConfig.reconEnabled = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->reconEnabled;

    // if HDR is set videoUsabilityInfo should be set to 1
    if (sequenceControlSetPtr->staticConfig.highDynamicRangeInput == 1) {
        sequenceControlSetPtr->staticConfig.videoUsabilityInfo = 1;
    }
    

    if (sequenceControlSetPtr->staticConfig.lookAheadDistance == (EB_U32)~0) {
        sequenceControlSetPtr->staticConfig.lookAheadDistance = ComputeDefaultLookAhead(&sequenceControlSetPtr->staticConfig);
    }

    // Extract frame rate from Numerator and Denominator if not 0
    if (sequenceControlSetPtr->staticConfig.frameRateNumerator != 0 && sequenceControlSetPtr->staticConfig.frameRateDenominator != 0) {
        sequenceControlSetPtr->staticConfig.frameRate = (sequenceControlSetPtr->staticConfig.frameRateNumerator << 16) / (sequenceControlSetPtr->staticConfig.frameRateDenominator);
    }

    // Get Default Intra Period if not specified
    if (sequenceControlSetPtr->staticConfig.intraPeriodLength == -2) {
        sequenceControlSetPtr->intraPeriodLength = sequenceControlSetPtr->staticConfig.intraPeriodLength = ComputeIntraPeriod(sequenceControlSetPtr);
    }
    return;
}

/******************************************
* Verify Settings
******************************************/
#define PowerOfTwoCheck(x) (((x) != 0) && (((x) & (~(x) + 1)) == (x)))

static int VerifyHmeDimention(unsigned int index,unsigned int HmeLevel0SearchAreaInWidth, EB_U32 NumberHmeSearchRegionInWidth[EB_HME_SEARCH_AREA_ROW_MAX_COUNT], unsigned int numberHmeSearchRegionInWidth )
{    
	int           return_error = 0;
	EB_U32        i;
    EB_U32        totalSearchWidth = 0;
	
    for (i=0 ; i < numberHmeSearchRegionInWidth; i++){
        totalSearchWidth += NumberHmeSearchRegionInWidth[i] ;
    }
    if ((totalSearchWidth) != (HmeLevel0SearchAreaInWidth)) {
		 printf("Error Instance %u: Invalid  HME Total Search Area. \n", index);
		 return_error = -1;
		 return return_error;
	 }
    
	return return_error;
}

static int VerifyHmeDimentionL1L2(unsigned int index, EB_U32 NumberHmeSearchRegionInWidth[EB_HME_SEARCH_AREA_ROW_MAX_COUNT], unsigned int numberHmeSearchRegionInWidth)
{
    int           return_error = 0;
    EB_U32        i;
    EB_U32        totalSearchWidth = 0;

    for (i = 0; i < numberHmeSearchRegionInWidth; i++) {
        totalSearchWidth += NumberHmeSearchRegionInWidth[i];
    }
    if ((totalSearchWidth > 256) || (totalSearchWidth == 0)) {
        printf("Error Instance %u: Invalid  HME Total Search Area. Must be [1 - 256].\n", index);
        return_error = -1;
        return return_error;
    }

    return return_error;
}

static EB_ERRORTYPE VerifySettings(\
    SequenceControlSet_t       *sequenceControlSetPtr)
{
	EB_ERRORTYPE return_error = EB_ErrorNone;
    const char   *levelIdc;
    unsigned int  levelIdx;
    EB_H265_ENC_CONFIGURATION *config = &sequenceControlSetPtr->staticConfig;
    unsigned int channelNumber = config->channelId;


	if ( config->tier > 1 ) {
		printf("Error instance %u: Tier must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;    
	}

	// For levels below level 4 (exclusive), only the main tier is allowed
    if(config->level < 40 && config->tier != 0){
        printf("Error Instance %u: For levels below level 4 (exclusive), only the main tier is allowed\n",channelNumber+1);
        return_error = EB_ErrorBadParameter; 
    }

    switch(config->level){
        case 10:
            levelIdc = "1";
            levelIdx = 0;

            break;
        case 20:
            levelIdc = "2";
            levelIdx = 1;

            break;
        case 21:
            levelIdc = "2.1";
            levelIdx = 2;
        
            break;
        case 30:
            levelIdc = "3";
            levelIdx = 3;
        
            break;
        case 31:
            levelIdc = "3.1";
            levelIdx = 4;
        
            break;
        case 40:
            levelIdc = "4";
            levelIdx = 5;
        
            break;
        case 41:
            levelIdc = "4.1";
            levelIdx = 6;
        
            break;
        case 50:
            levelIdc = "5";
            levelIdx = 7;
        
            break;
        case 51:
            levelIdc = "5.1";
            levelIdx = 8;
        
            break;
        case 52:
            levelIdc = "5.2";
            levelIdx = 9;
        
            break;
        case 60:
            levelIdc = "6";
            levelIdx = 10;
        
            break;
        case 61:
            levelIdc = "6.1";
            levelIdx = 11;
        
            break;
        case 62:
            levelIdc = "6.2";
            levelIdx = 12;
        
            break;

        case 0: // Level determined by the encoder
            levelIdc = "0";
            levelIdx = TOTAL_LEVEL_COUNT;
        
            break;

        default:
            levelIdc = "unknown";
            levelIdx = TOTAL_LEVEL_COUNT + 1;

            break;
    }
    
	if(levelIdx > TOTAL_LEVEL_COUNT){
        printf("Error Instance %u: Unsupported level\n",channelNumber+1);
		return_error = EB_ErrorBadParameter; 
    }	 

    if (sequenceControlSetPtr->maxInputLumaWidth < 64) {
		printf("Error instance %u: Source Width must be at least 64\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}
    if (sequenceControlSetPtr->maxInputLumaHeight < 64) {
        printf("Error instance %u: Source Width must be at least 64\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->predStructure > 2) {
        printf("Error instance %u: Pred Structure must be [0-2]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->baseLayerSwitchMode == 1 && config->predStructure != 2) {
        printf( "Error Instance %u: Base Layer Switch Mode 1 only when Prediction Structure is Random Access\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (sequenceControlSetPtr->maxInputLumaWidth % 8 && sequenceControlSetPtr->staticConfig.compressedTenBitFormat == 1) {
        printf("Error Instance %u: Only multiple of 8 width is supported for compressed 10-bit inputs \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

	if (sequenceControlSetPtr->maxInputLumaWidth % 2) {
        printf("Error Instance %u: Source Width must be even for YUV_420 colorspace\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    } 
    
    if (sequenceControlSetPtr->maxInputLumaHeight % 2) {
        printf("Error Instance %u: Source Height must be even for YUV_420 colorspace\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    } 
    if (sequenceControlSetPtr->maxInputLumaWidth > 8192) {
		printf("Error instance %u: Source Width must be less than 8192\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	} 

    if (sequenceControlSetPtr->maxInputLumaHeight > 4320) {
		printf("Error instance %u: Source Height must be less than 4320\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	EB_U32 inputSize = sequenceControlSetPtr->maxInputLumaWidth * sequenceControlSetPtr->maxInputLumaHeight;

	EB_U8 inputResolution = (inputSize < INPUT_SIZE_1080i_TH)	?	INPUT_SIZE_576p_RANGE_OR_LOWER :
							(inputSize < INPUT_SIZE_1080p_TH)	?	INPUT_SIZE_1080i_RANGE :
							(inputSize < INPUT_SIZE_4K_TH)		?	INPUT_SIZE_1080p_RANGE : 
																	INPUT_SIZE_4K_RANGE;

    if (inputResolution <= INPUT_SIZE_1080i_RANGE) {
        if (config->encMode > 9) {
            printf("Error instance %u: encMode must be [0 - 9] for this resolution\n", channelNumber + 1);
            return_error = EB_ErrorBadParameter;
        }

    }
    else if (inputResolution == INPUT_SIZE_1080p_RANGE) {
        if (config->encMode > 10) {
            printf("Error instance %u: encMode must be [0 - 10] for this resolution\n", channelNumber + 1);
            return_error = EB_ErrorBadParameter;
        }

    }
    else {
        if (config->encMode > 12 && config->tune == 0) {
            printf("Error instance %u: encMode must be [0 - 12] for this resolution\n", channelNumber + 1);
            return_error = EB_ErrorBadParameter;
        }
        else if (config->encMode > 10 && config->tune == 1) {
            printf("Error instance %u: encMode must be [0 - 10] for this resolution\n", channelNumber + 1);
            return_error = EB_ErrorBadParameter;
        }
    }

    // encMode
    sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES;
    if (inputResolution <= INPUT_SIZE_1080i_RANGE){
        sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_SUB1080P - 1;
        if (config->encMode > MAX_SUPPORTED_MODES_SUB1080P -1) {
            printf("Error instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_SUB1080P-1);
			return_error = EB_ErrorBadParameter;
		}
	}else if (inputResolution == INPUT_SIZE_1080p_RANGE){
        sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_1080P - 1;
        if (config->encMode > MAX_SUPPORTED_MODES_1080P - 1) {
            printf("Error instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_1080P - 1);
			return_error = EB_ErrorBadParameter;
		}
	}else {
        if (config->tune == 0)
            sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_4K_SQ - 1;
        else
            sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_4K_OQ - 1;

        if (config->encMode > MAX_SUPPORTED_MODES_4K_SQ - 1 && config->tune == 0) {
            printf("Error instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_4K_SQ-1);
			return_error = EB_ErrorBadParameter;
        }else if (config->encMode > MAX_SUPPORTED_MODES_4K_OQ - 1 && config->tune == 1) {
            printf("Error instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_4K_OQ-1);
			return_error = EB_ErrorBadParameter;
		}
	}

	if(config->qp > 51) {
		printf("Error instance %u: QP must be [0 - 51]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;	
	}
    
    if (config->hierarchicalLevels > 3) {
		printf("Error instance %u: Hierarchical Levels supported [0-3]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	} 

    if (config->intraPeriodLength < -2 || config->intraPeriodLength > 255) {
        printf("Error Instance %u: The intra period must be [-2 - 255] \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

	if( config->intraRefreshType > 2 || config->intraRefreshType < 1) {
        printf("Error Instance %u: Invalid intra Refresh Type [1-2]\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
	}
    if (config->baseLayerSwitchMode > 1) {
        printf("Error Instance %u: Invalid Base Layer Switch Mode [0-1] \n",channelNumber+1);
        return_error = EB_ErrorBadParameter; 
    }
    if (config->interlacedVideo > 1) {
        printf("Error Instance %u: Invalid Interlaced Video\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }  

	if ( config->disableDlfFlag > 1) {
       printf("Error Instance %u: Invalid LoopFilterDisable. LoopFilterDisable must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;	
    } 

	if ( config->enableSaoFlag > 1) {
       printf("Error Instance %u: Invalid SAO. SAO range must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;	
    }
	if ( config->useDefaultMeHme > 1 ){
        printf("Error Instance %u: invalid useDefaultMeHme. useDefaultMeHme must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;	
	}
    if ( config->enableHmeFlag > 1 ){
        printf("Error Instance %u: invalid HME. HME must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;	
	}

	if ( config->enableHmeLevel0Flag > 1 ) {
        printf("Error Instance %u: invalid enable HMELevel0. HMELevel0 must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;	
	}

	if ( config->enableHmeLevel1Flag > 1 ) {
        printf("Error Instance %u: invalid enable HMELevel1. HMELevel1 must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;	
	}

	if ( config->enableHmeLevel2Flag > 1 ){
        printf("Error Instance %u: invalid enable HMELevel2. HMELevel2 must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;	
	}	
	
	if ((config->searchAreaWidth > 256) || (config->searchAreaWidth == 0)){
        printf("Error Instance %u: Invalid SearchAreaWidth. SearchAreaWidth must be [1 - 256]\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;	

    }
	 
	 if((config->searchAreaHeight > 256) || (config->searchAreaHeight == 0)) {
         printf("Error Instance %u: Invalid SearchAreaHeight. SearchAreaHeight must be [1 - 256]\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;	

    }
	 
	 if (config->enableHmeFlag){

		 if ((config->numberHmeSearchRegionInWidth > (EB_U32)EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT) || (config->numberHmeSearchRegionInWidth == 0)){
             printf("Error Instance %u: Invalid NumberHmeSearchRegionInWidth. NumberHmeSearchRegionInWidth must be [1 - %d]\n",channelNumber+1,EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT);
			return_error = EB_ErrorBadParameter;	
		 }
	 
		 if ((config->numberHmeSearchRegionInHeight > (EB_U32)EB_HME_SEARCH_AREA_ROW_MAX_COUNT) || (config->numberHmeSearchRegionInHeight == 0)){
             printf("Error Instance %u: Invalid NumberHmeSearchRegionInHeight. NumberHmeSearchRegionInHeight must be [1 - %d]\n",channelNumber+1,EB_HME_SEARCH_AREA_ROW_MAX_COUNT);
			return_error = EB_ErrorBadParameter;	
		 }

		 if ((config->hmeLevel0TotalSearchAreaHeight > 256) || (config->hmeLevel0TotalSearchAreaHeight == 0)) {
			 printf("Error Instance %u: Invalid hmeLevel0TotalSearchAreaHeight. hmeLevel0TotalSearchAreaHeight must be [1 - 256]\n", channelNumber + 1);
			 return_error = EB_ErrorBadParameter;
		 }
		 if ((config->hmeLevel0TotalSearchAreaWidth > 256) || (config->hmeLevel0TotalSearchAreaWidth == 0)) {
			 printf("Error Instance %u: Invalid hmeLevel0TotalSearchAreaWidth. hmeLevel0TotalSearchAreaWidth must be [1 - 256]\n", channelNumber + 1);
			 return_error = EB_ErrorBadParameter;
		 }
		 if ( VerifyHmeDimention(channelNumber+1, config->hmeLevel0TotalSearchAreaHeight, config->hmeLevel0SearchAreaInHeightArray, config->numberHmeSearchRegionInHeight) ) {
			 return_error = EB_ErrorBadParameter;	
		 }
		 
		 if ( VerifyHmeDimention(channelNumber+1, config->hmeLevel0TotalSearchAreaWidth, config->hmeLevel0SearchAreaInWidthArray, config->numberHmeSearchRegionInWidth) ) {
			 return_error = EB_ErrorBadParameter;	
		 }
		 if (VerifyHmeDimentionL1L2(channelNumber + 1, config->hmeLevel1SearchAreaInWidthArray , config->numberHmeSearchRegionInWidth)) {
			 return_error = EB_ErrorBadParameter;
		 }
		 if (VerifyHmeDimentionL1L2(channelNumber + 1, config->hmeLevel1SearchAreaInHeightArray, config->numberHmeSearchRegionInWidth)) {
			 return_error = EB_ErrorBadParameter;
		 }
		 if (VerifyHmeDimentionL1L2(channelNumber + 1, config->hmeLevel2SearchAreaInWidthArray, config->numberHmeSearchRegionInWidth)) {
			 return_error = EB_ErrorBadParameter;
		 }
		 if (VerifyHmeDimentionL1L2(channelNumber + 1, config->hmeLevel2SearchAreaInHeightArray, config->numberHmeSearchRegionInWidth)) {
			 return_error = EB_ErrorBadParameter;	
		 }
	 }


    if (levelIdx < 13) {
    // Check if the current input video is conformant with the Level constraint
    if(config->level != 0 && ((EB_U64)(sequenceControlSetPtr->maxInputLumaWidth * sequenceControlSetPtr->maxInputLumaHeight) > maxLumaPictureSize[levelIdx])){
        printf("Error Instance %u: The input luma picture size exceeds the maximum luma picture size allowed for level %s\n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }

    // Check if the current input video is conformant with the Level constraint
    if(config->level != 0 && ((EB_U64)config->frameRate * (EB_U64)sequenceControlSetPtr->maxInputLumaWidth * (EB_U64)sequenceControlSetPtr->maxInputLumaHeight > (maxLumaSampleRate[levelIdx]<<16))){
        printf("Error Instance %u: The input luma sample rate exceeds the maximum input sample rate allowed for level %s\n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
	
	if ((config->level != 0) && (config->rateControlMode) && (config->tier == 0) && ((config->targetBitRate*2) > mainTierMaxBitRate[levelIdx])){
		printf("Error Instance %u: Allowed MaxBitRate exceeded for level %s and tier 0 \n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
	if ((config->level != 0) && (config->rateControlMode) && (config->tier == 1) && ((config->targetBitRate*2) > highTierMaxBitRate[levelIdx])){
		printf("Error Instance %u: Allowed MaxBitRate exceeded for level %s and tier 1 \n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
	if ((config->level != 0) && (config->rateControlMode) && (config->tier == 0) && ((config->targetBitRate * 3) > mainTierCPB[levelIdx])) {
		printf("Error Instance %u: Out of bound maxBufferSize for level %s and tier 0 \n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
	if ((config->level != 0) && (config->rateControlMode) && (config->tier == 1) && ((config->targetBitRate * 3) > highTierCPB[levelIdx])) {
		printf("Error Instance %u: Out of bound maxBufferSize for level %s and tier 1 \n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
    }

	if(config->profile > 3){
        printf("Error Instance %u: The maximum allowed Profile number is 3 \n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

	if (config->profile == 0){
		printf("Error Instance %u: The minimum allowed Profile number is 1 \n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}

	if(config->profile == 3) {
		printf("Error instance %u: The Main Still Picture Profile is not supported \n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	} 

    // Check if the current input video is conformant with the Level constraint
    if(config->frameRate > (240<<16)){
        printf("Error Instance %u: The maximum allowed frame rate is 240 fps\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }
    // Check that the frameRate is non-zero
    if(config->frameRate <= 0) {
        printf("Error Instance %u: The frame rate should be greater than 0 fps \n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->intraPeriodLength < -2 || config->intraPeriodLength > 255 ) {
        printf("Error Instance %u: The intra period must be [-2 - 255] \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }
	if (config->constrainedIntra > 1) {
		printf("Error Instance %u: The constrained intra must be [0 - 1] \n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}
	if (config->rateControlMode > 1) {
		printf("Error Instance %u: The rate control mode must be [0 - 1] \n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}
    if (config->rateControlMode == 1 && config->tune > 0) {
        printf("Error Instance %u: The rate control is not supported for OQ mode (Tune = 1 )\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->tune > 0 && config->bitRateReduction == 1){
        printf("Error Instance %u: Bit Rate Reduction is not supported for OQ mode (Tune = 1 )\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->tune > 0 && config->improveSharpness == 1){
        printf("Error Instance %u: Improve sharpness is not supported for OQ mode (Tune = 1 )\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->lookAheadDistance > 256 && config->lookAheadDistance != (EB_U32)~0) {
        printf("Error Instance %u: The lookahead distance must be [0 - 256] \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }
	if (config->sceneChangeDetection > 1) {
		printf("Error Instance %u: The scene change detection must be [0 - 1] \n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}
	if ( config->maxQpAllowed > 51) {
		printf("Error instance %u: MaxQpAllowed must be [0 - 51]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;    
	}
	else if ( config->minQpAllowed > 50 ) {
		printf("Error instance %u: MinQpAllowed must be [0 - 50]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;   
	}
	else if ( (config->minQpAllowed) > (config->maxQpAllowed))  {
        printf("Error Instance %u:  MinQpAllowed must be smaller than MaxQpAllowed\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;  
	}

	if (config->videoUsabilityInfo > 1) {
		printf("Error instance %u : Invalid VideoUsabilityInfo. VideoUsabilityInfo must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

    if (config->tune > 1) {
        printf("Error instance %u : Invalid Tune. Tune must be [0 - 1]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }
	if (config->bitRateReduction > 1) {
		printf("Error instance %u : Invalid BitRateReduction. BitRateReduction must be [0 - 1]\n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}
    if (config->improveSharpness > 1) {
		printf("Error instance %u : Invalid ImproveSharpness. ImproveSharpness must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

    if (config->highDynamicRangeInput > 1) {
		printf("Error instance %u : Invalid HighDynamicRangeInput. HighDynamicRangeInput must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }
	if (config->accessUnitDelimiter > 1) {
		printf("Error instance %u : Invalid AccessUnitDelimiter. AccessUnitDelimiter must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	if (config->bufferingPeriodSEI > 1) {
		printf("Error instance %u : Invalid BufferingPeriod. BufferingPeriod must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	if (config->pictureTimingSEI > 1) {
		printf("Error instance %u : Invalid PictureTiming. PictureTiming must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	if (config->registeredUserDataSeiFlag > 1) {
		printf("Error instance %u : Invalid RegisteredUserData. RegisteredUserData must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }
 
	if (config->unregisteredUserDataSeiFlag > 1) {
		printf("Error instance %u : Invalid UnregisteredUserData. UnregisteredUserData must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	if (config->recoveryPointSeiFlag > 1) {
		printf("Error instance %u : Invalid RecoveryPoint. RecoveryPoint must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

 	if (config->enableTemporalId > 1) {
		printf("Error instance %u : Invalid TemporalId. TemporalId must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

    if (config->pictureTimingSEI && !config->videoUsabilityInfo){
        printf("Error Instance %u: If pictureTimingSEI is set, VideoUsabilityInfo should be also set to 1\n",channelNumber);
        return_error = EB_ErrorBadParameter;

    }
	  if ( (config->encoderBitDepth !=8 )  && 
		(config->encoderBitDepth !=10 ) 
		) {
			printf("Error instance %u: Encoder Bit Depth shall be only 8 or 10 \n",channelNumber+1);
			return_error = EB_ErrorBadParameter;
	}
	// Check if the EncoderBitDepth is conformant with the Profile constraint
	if(config->profile == 1 && config->encoderBitDepth == 10) {
		printf("Error instance %u: The encoder bit depth shall be equal to 8 for Main Profile\n",channelNumber+1);
			return_error = EB_ErrorBadParameter;
	}

	if (config->compressedTenBitFormat > 1)
	{
		printf("Error instance %u: Invalid Compressed Ten Bit Format shall be only [0 - 1] \n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}

    if (config->speedControlFlag > 1) {
        printf("Error Instance %u: Invalid Speed Control flag [0 - 1]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (((EB_S32)(config->asmType) < 0) || ((EB_S32)(config->asmType) > 1)){
        printf("Error Instance %u: Invalid asm type value [0: C Only, 1: Auto] .\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

	// UseRoundRobinThreadAssignment
	if (config->useRoundRobinThreadAssignment != 0 && config->useRoundRobinThreadAssignment != 1) {
		printf("Error instance %u: Invalid UseRoundRobinThreadAssignment [0 - 1]\n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}
 

    return return_error;
}

/**********************************
 Set Parameter
**********************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_ERRORTYPE EbH265EncInitParameter(
    EB_H265_ENC_CONFIGURATION * configPtr)
{
    EB_ERRORTYPE                  return_error = EB_ErrorNone;

    if (!configPtr) {
        printf("The EB_H265_ENC_CONFIGURATION structure is empty! \n");
        return EB_ErrorBadParameter;
    }

    configPtr->frameRate = 60;
    configPtr->frameRateNumerator = 0;
    configPtr->frameRateDenominator = 0;
    configPtr->encoderBitDepth = 8;
    configPtr->compressedTenBitFormat = 0;
    configPtr->sourceWidth = 0;
    configPtr->sourceHeight = 0;
    configPtr->framesToBeEncoded = 0;


    // Interlaced Video 
    configPtr->interlacedVideo = EB_FALSE;
    configPtr->qp = 32;
    configPtr->useQpFile = EB_FALSE;
    configPtr->sceneChangeDetection = 1;
    configPtr->rateControlMode = 0;
    configPtr->lookAheadDistance = 17;
    configPtr->targetBitRate = 7000000;
    configPtr->maxQpAllowed = 48;
    configPtr->minQpAllowed = 10;
    configPtr->baseLayerSwitchMode = 0;
    configPtr->encMode  = 9;
    configPtr->intraPeriodLength = -2;
    configPtr->intraRefreshType = 1;
    configPtr->hierarchicalLevels = 3;
    configPtr->predStructure = EB_PRED_RANDOM_ACCESS;
    configPtr->disableDlfFlag = EB_FALSE;
    configPtr->enableSaoFlag = EB_TRUE;
    configPtr->useDefaultMeHme = EB_TRUE;
    configPtr->enableHmeFlag = EB_TRUE;
    configPtr->enableHmeLevel0Flag = EB_TRUE;
    configPtr->enableHmeLevel1Flag = EB_FALSE;
    configPtr->enableHmeLevel2Flag = EB_FALSE;
    configPtr->searchAreaWidth = 16;
    configPtr->searchAreaHeight = 7;
    configPtr->numberHmeSearchRegionInWidth = 2;
    configPtr->numberHmeSearchRegionInHeight = 2;
    configPtr->hmeLevel0TotalSearchAreaWidth = 64;
    configPtr->hmeLevel0TotalSearchAreaHeight = 25;
    configPtr->hmeLevel0SearchAreaInWidthArray[0] = 32;
    configPtr->hmeLevel0SearchAreaInWidthArray[1] = 32;
    configPtr->hmeLevel0SearchAreaInHeightArray[0] = 12;
    configPtr->hmeLevel0SearchAreaInHeightArray[1] = 13;
    configPtr->hmeLevel1SearchAreaInWidthArray[0] = 1;
    configPtr->hmeLevel1SearchAreaInWidthArray[1] = 1;
    configPtr->hmeLevel1SearchAreaInHeightArray[0] = 1;
    configPtr->hmeLevel1SearchAreaInHeightArray[1] = 1;
    configPtr->hmeLevel2SearchAreaInWidthArray[0] = 1;
    configPtr->hmeLevel2SearchAreaInWidthArray[1] = 1;
    configPtr->hmeLevel2SearchAreaInHeightArray[0] = 1;
    configPtr->hmeLevel2SearchAreaInHeightArray[1] = 1;
    configPtr->constrainedIntra = EB_FALSE;
    configPtr->tune = 0;
    configPtr->bitRateReduction = EB_TRUE;
    configPtr->improveSharpness = EB_TRUE;

    // Bitstream options
    configPtr->codeVpsSpsPps = 1;
    configPtr->videoUsabilityInfo = 0;
    configPtr->highDynamicRangeInput = 0;
    configPtr->accessUnitDelimiter = 0;
    configPtr->bufferingPeriodSEI = 0;
    configPtr->pictureTimingSEI = 0;
    configPtr->registeredUserDataSeiFlag = EB_FALSE;
    configPtr->unregisteredUserDataSeiFlag = EB_FALSE;
    configPtr->recoveryPointSeiFlag = EB_FALSE;
    configPtr->enableTemporalId = 1;
    
    // HT - To be removed
    configPtr->inputOutputBufferFifoInitCount = 50;

    // Annex A parameters
    configPtr->profile = 2;
    configPtr->tier = 0;
    configPtr->level = 0;

    // Latency
    configPtr->injectorFrameRate = 60 << 16;
    configPtr->speedControlFlag = 0;
    configPtr->latencyMode = 0;

    // ASM Type
    configPtr->asmType = ASM_AVX2; 
    
    // Channel info
    configPtr->useRoundRobinThreadAssignment = EB_FALSE;
    configPtr->channelId = 0;
    configPtr->activeChannelCount   = 1;
    
    // Debug info
    configPtr->reconEnabled = 0;

    return return_error;
}

/**********************************

 * Set Parameter
 **********************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265EncSetParameter(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_H265_ENC_CONFIGURATION  *pComponentParameterStructure)
{
    EB_ERRORTYPE           return_error      = EB_ErrorNone;
    EbEncHandle_t          *pEncCompData      = (EbEncHandle_t*) h265EncComponent->pComponentPrivate;
    EB_U32                  instanceIndex     = 0;

    // Acquire Config Mutex
    EbBlockOnMutex(pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->configMutex);

    SetDefaultConfigurationParameters(
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr);

    CopyApiFromApp(
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr,
        (EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure);

    return_error = (EB_ERRORTYPE)VerifySettings(
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr);

    if (return_error == EB_ErrorBadParameter) {
        return EB_ErrorBadParameter;
    }

    SetParamBasedOnInput(
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr);

    // Initialize the Prediction Structure Group
    return_error = (EB_ERRORTYPE)PredictionStructureGroupCtor(
        &pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->predictionStructureGroupPtr,
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.baseLayerSwitchMode);
    
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    // Set the Prediction Structure
    pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->predStructPtr = GetPredictionStructure(
		pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->predictionStructureGroupPtr,
		pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.predStructure,
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxRefCount,
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxTemporalLayers);

    LoadDefaultBufferConfigurationSettings(
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr);

    // Release Config Mutex
    EbReleaseMutex(pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->configMutex);

    return return_error;
}    
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265EncStreamHeader(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE*        outputStreamPtr
)
{
    EB_ERRORTYPE           return_error = EB_ErrorNone;
    EbEncHandle_t          *pEncCompData = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    Bitstream_t            *bitstreamPtr;
    SequenceControlSet_t  *sequenceControlSetPtr = pEncCompData->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr;
    EncodeContext_t        *encodeContextPtr = sequenceControlSetPtr->encodeContextPtr;
    EbPPSConfig_t          *ppsConfig;

    EB_MALLOC(EbPPSConfig_t*, ppsConfig, sizeof(EbPPSConfig_t), EB_N_PTR);
    EB_MALLOC(Bitstream_t*, bitstreamPtr, sizeof(Bitstream_t), EB_N_PTR);
    EB_MALLOC(OutputBitstreamUnit_t*, bitstreamPtr->outputBitstreamPtr, sizeof(OutputBitstreamUnit_t), EB_N_PTR);

    return_error = OutputBitstreamUnitCtor(
        (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr,
        PACKETIZATION_PROCESS_BUFFER_SIZE);

    // Reset the bitstream before writing to it
    ResetBitstream(
        (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr);

    if (sequenceControlSetPtr->staticConfig.accessUnitDelimiter) {

        EncodeAUD(
            bitstreamPtr,
            I_SLICE,
            0);
    }

    // Compute Profile Tier and Level Information
    ComputeProfileTierLevelInfo(
        sequenceControlSetPtr);

    ComputeMaxDpbBuffer(
        sequenceControlSetPtr);

    // Code the VPS
    EncodeVPS(
        bitstreamPtr,
        sequenceControlSetPtr);

    // Code the SPS
    EncodeSPS(
        bitstreamPtr,
        sequenceControlSetPtr);

    ppsConfig->ppsId = 0;
    ppsConfig->constrainedFlag = 0;
    EncodePPS(
        bitstreamPtr,
        sequenceControlSetPtr,
        ppsConfig);

    if (sequenceControlSetPtr->staticConfig.constrainedIntra == EB_TRUE) {
        // Configure second pps
        ppsConfig->ppsId = 1;
        ppsConfig->constrainedFlag = 1;

        EncodePPS(
            bitstreamPtr,
            sequenceControlSetPtr,
            ppsConfig);
    }

    // Flush the Bitstream
    FlushBitstream(
        bitstreamPtr->outputBitstreamPtr);

    // Copy SPS & PPS to the Output Bitstream
    CopyRbspBitstreamToPayload(
        bitstreamPtr,
        outputStreamPtr->pBuffer,
        (EB_U32*) &(outputStreamPtr->nFilledLen),
        (EB_U32*) &(outputStreamPtr->nAllocLen),
        encodeContextPtr);

    return return_error;
}


/***********************************************
**** Copy the input buffer from the 
**** sample application to the library buffers
************************************************/
#if !ONE_MEMCPY
EB_ERRORTYPE CopyFrameBuffer(
    SequenceControlSet_t        *sequenceControlSet,
    EB_U8      			        *dst,
    EB_U8      			        *src)
{
    EB_H265_ENC_CONFIGURATION   *config = &sequenceControlSet->staticConfig;
    EB_ERRORTYPE   return_error = EB_ErrorNone;
    const int tenBitPackedMode = (config->encoderBitDepth > 8) && (config->compressedTenBitFormat == 0) ? 1 : 0;

    // Determine size of each plane
    const size_t luma8bitSize =
        config->sourceWidth    *
        config->sourceHeight   *
        (1 << tenBitPackedMode);

    const size_t chroma8bitSize = luma8bitSize >> 2;
    const size_t luma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? (luma8bitSize >> 2) : 0;
    const size_t chroma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? (luma10bitSize >> 2) : 0;

    // Determine  
    EB_H265_ENC_INPUT* inputPtr     = (EB_H265_ENC_INPUT*)src;
    EB_H265_ENC_INPUT* libInputPtr  = (EB_H265_ENC_INPUT*)dst;
    if (luma8bitSize) {
        EB_MEMCPY(libInputPtr->luma, inputPtr->luma, luma8bitSize);
    }
    else {
        libInputPtr->luma = 0;
    }
    if (chroma8bitSize) {
        EB_MEMCPY(libInputPtr->cb, inputPtr->cb, chroma8bitSize);
    }
    else {
        libInputPtr->cb = 0;
    }

    if (chroma8bitSize) {
        EB_MEMCPY(libInputPtr->cr, inputPtr->cr, chroma8bitSize);
    }
    else {
        libInputPtr->cr = 0;
    }

    if (luma10bitSize) {
        EB_MEMCPY(libInputPtr->lumaExt, inputPtr->lumaExt, luma10bitSize);
    }
    else {
        libInputPtr->lumaExt = 0;
    }

    if (chroma10bitSize) {
        EB_MEMCPY(libInputPtr->cbExt, inputPtr->cbExt, chroma10bitSize);
    }
    else {
        libInputPtr->cbExt = 0;
    }

    if (chroma10bitSize) {
        EB_MEMCPY(libInputPtr->crExt, inputPtr->crExt, chroma10bitSize);
    }
    else {
        libInputPtr->crExt = 0;
    }

    return return_error;
}
#else
static EB_ERRORTYPE CopyFrameBuffer(
    SequenceControlSet_t        *sequenceControlSetPtr,
    EB_U8      			        *dst,
    EB_U8      			        *src)
{
    EB_H265_ENC_CONFIGURATION   *config = &sequenceControlSetPtr->staticConfig;
    EB_ERRORTYPE   return_error = EB_ErrorNone;



    EbPictureBufferDesc_t           *inputPicturePtr = (EbPictureBufferDesc_t*)dst;
    EB_H265_ENC_INPUT               *inputPtr = (EB_H265_ENC_INPUT*)src;
    EB_U16                           inputRowIndex;
    EB_BOOL                          is16BitInput = (EB_BOOL)(config->encoderBitDepth > EB_8BIT);

    // Need to include for Interlacing on the fly with pictureScanType = 1

    if (!is16BitInput) {

        EB_U32                           lumaBufferOffset = (inputPicturePtr->strideY*sequenceControlSetPtr->topPadding + sequenceControlSetPtr->leftPadding) << is16BitInput;
        EB_U32                           chromaBufferOffset = (inputPicturePtr->strideCr*(sequenceControlSetPtr->topPadding >> 1) + (sequenceControlSetPtr->leftPadding >> 1)) << is16BitInput;
        EB_U16                           lumaStride = inputPicturePtr->strideY << is16BitInput;
        EB_U16                           chromaStride = inputPicturePtr->strideCb << is16BitInput;
        EB_U16                           lumaWidth = (EB_U16)(inputPicturePtr->width - sequenceControlSetPtr->maxInputPadRight) << is16BitInput;
        EB_U16                           chromaWidth = (lumaWidth >> 1) << is16BitInput;
        EB_U16                           lumaHeight = (EB_U16)(inputPicturePtr->height - sequenceControlSetPtr->maxInputPadBottom);

        EB_U16                           sourceLumaStride = (EB_U16)(inputPtr->yStride);
        EB_U16                           sourceCrStride   = (EB_U16)(inputPtr->crStride);
        EB_U16                           sourceCbStride   = (EB_U16)(inputPtr->cbStride);

        //EB_U16                           lumaHeight  = inputPicturePtr->maxHeight;
        // Y
        for (inputRowIndex = 0; inputRowIndex < lumaHeight; inputRowIndex++) {

            EB_MEMCPY((inputPicturePtr->bufferY + lumaBufferOffset + lumaStride * inputRowIndex),
                (inputPtr->luma + sourceLumaStride * inputRowIndex),
                lumaWidth);
        }

        // U
        for (inputRowIndex = 0; inputRowIndex < lumaHeight >> 1; inputRowIndex++) {
            EB_MEMCPY((inputPicturePtr->bufferCb + chromaBufferOffset + chromaStride * inputRowIndex),
                (inputPtr->cb + (sourceCbStride*inputRowIndex)),
                chromaWidth);
        }

        // V
        for (inputRowIndex = 0; inputRowIndex < lumaHeight >> 1; inputRowIndex++) {
            EB_MEMCPY((inputPicturePtr->bufferCr + chromaBufferOffset + chromaStride * inputRowIndex),
                (inputPtr->cr + (sourceCrStride*inputRowIndex)),
                chromaWidth);
        }

    }
    else if (is16BitInput && config->compressedTenBitFormat == 1)
    {
        {
            EB_U32  lumaBufferOffset = (inputPicturePtr->strideY*sequenceControlSetPtr->topPadding + sequenceControlSetPtr->leftPadding);
            EB_U32  chromaBufferOffset = (inputPicturePtr->strideCr*(sequenceControlSetPtr->topPadding >> 1) + (sequenceControlSetPtr->leftPadding >> 1));
            EB_U16  lumaStride = inputPicturePtr->strideY;
            EB_U16  chromaStride = inputPicturePtr->strideCb;
            EB_U16  lumaWidth = (EB_U16)(inputPicturePtr->width - sequenceControlSetPtr->maxInputPadRight);
            EB_U16  chromaWidth = (lumaWidth >> 1);
            EB_U16  lumaHeight = (EB_U16)(inputPicturePtr->height - sequenceControlSetPtr->maxInputPadBottom);

            EB_U16 sourceLumaStride = (EB_U16)(inputPtr->yStride);
            EB_U16 sourceCrStride   = (EB_U16)(inputPtr->crStride);
            EB_U16 sourceCbStride   = (EB_U16)(inputPtr->cbStride);

            // Y 8bit
            for (inputRowIndex = 0; inputRowIndex < lumaHeight; inputRowIndex++) {

                EB_MEMCPY((inputPicturePtr->bufferY + lumaBufferOffset + lumaStride * inputRowIndex),
                    (inputPtr->luma + sourceLumaStride * inputRowIndex),
                    lumaWidth);

            }

            // U 8bit
            for (inputRowIndex = 0; inputRowIndex < lumaHeight >> 1; inputRowIndex++) {

                EB_MEMCPY((inputPicturePtr->bufferCb + chromaBufferOffset + chromaStride * inputRowIndex),
                    (inputPtr->cb + (sourceCbStride*inputRowIndex)),
                    chromaWidth);

            }

            // V 8bit
            for (inputRowIndex = 0; inputRowIndex < lumaHeight >> 1; inputRowIndex++) {

                EB_MEMCPY((inputPicturePtr->bufferCr + chromaBufferOffset + chromaStride * inputRowIndex),
                    (inputPtr->cr + (sourceCrStride*inputRowIndex)),
                    chromaWidth);

            }

            //efficient copy - final
            //compressed 2Bit in 1D format
            {
                EB_U16 luma2BitWidth = sequenceControlSetPtr->maxInputLumaWidth / 4;
                EB_U16 lumaHeight = sequenceControlSetPtr->maxInputLumaHeight;

                EB_U16 sourceLuma2BitStride = sourceLumaStride / 4;
                EB_U16 sourceChroma2BitStride = sourceLuma2BitStride >> 1;

                for (inputRowIndex = 0; inputRowIndex < lumaHeight; inputRowIndex++) {
                    EB_MEMCPY(inputPicturePtr->bufferBitIncY + luma2BitWidth * inputRowIndex, inputPtr->lumaExt + sourceLuma2BitStride * inputRowIndex, luma2BitWidth);
                }
                for (inputRowIndex = 0; inputRowIndex < lumaHeight >> 1; inputRowIndex++) {
                    EB_MEMCPY(inputPicturePtr->bufferBitIncCb + (luma2BitWidth >> 1)*inputRowIndex, inputPtr->cbExt + sourceChroma2BitStride * inputRowIndex, luma2BitWidth >> 1);
                }
                for (inputRowIndex = 0; inputRowIndex < lumaHeight >> 1; inputRowIndex++) {
                    EB_MEMCPY(inputPicturePtr->bufferBitIncCr + (luma2BitWidth >> 1)*inputRowIndex, inputPtr->crExt + sourceChroma2BitStride * inputRowIndex, luma2BitWidth >> 1);
                }
            }

        }

    }
    else { // 10bit packed 

        EB_U32 lumaOffset = 0, chromaOffset = 0;
        EB_U32 lumaBufferOffset = (inputPicturePtr->strideY*sequenceControlSetPtr->topPadding + sequenceControlSetPtr->leftPadding);
        EB_U32 chromaBufferOffset = (inputPicturePtr->strideCr*(sequenceControlSetPtr->topPadding >> 1) + (sequenceControlSetPtr->leftPadding >> 1));
        EB_U16 lumaWidth = (EB_U16)(inputPicturePtr->width - sequenceControlSetPtr->maxInputPadRight);
        EB_U16 chromaWidth = (lumaWidth >> 1);
        EB_U16 lumaHeight = (EB_U16)(inputPicturePtr->height - sequenceControlSetPtr->maxInputPadBottom);

        EB_U16 sourceLumaStride = (EB_U16)(inputPtr->yStride);
        EB_U16 sourceCrStride = (EB_U16)(inputPtr->crStride);
        EB_U16 sourceCbStride = (EB_U16)(inputPtr->cbStride);

        UnPack2D(
            (EB_U16*)(inputPtr->luma + lumaOffset),
            sourceLumaStride,
            inputPicturePtr->bufferY + lumaBufferOffset,
            inputPicturePtr->strideY,
            inputPicturePtr->bufferBitIncY + lumaBufferOffset,
            inputPicturePtr->strideBitIncY,
            lumaWidth,
            lumaHeight);

        UnPack2D(
            (EB_U16*)(inputPtr->cb + chromaOffset),
            sourceCbStride,
            inputPicturePtr->bufferCb + chromaBufferOffset,
            inputPicturePtr->strideCb,
            inputPicturePtr->bufferBitIncCb + chromaBufferOffset,
            inputPicturePtr->strideBitIncCb,
            chromaWidth,
            (lumaHeight >> 1));

        UnPack2D(
            (EB_U16*)(inputPtr->cr + chromaOffset),
            sourceCrStride,
            inputPicturePtr->bufferCr + chromaBufferOffset,
            inputPicturePtr->strideCr,
            inputPicturePtr->bufferBitIncCr + chromaBufferOffset,
            inputPicturePtr->strideBitIncCr,
            chromaWidth,
            (lumaHeight >> 1));
    }
    return return_error;
}
#endif
static void CopyInputBuffer(
    SequenceControlSet_t*    sequenceControlSet,
    EB_BUFFERHEADERTYPE*     dst,
    EB_BUFFERHEADERTYPE*     src
)
{
    // Copy the higher level structure
    dst->nAllocLen  = src->nAllocLen;
    dst->nFilledLen = src->nFilledLen;
    dst->nFlags     = src->nFlags;
    dst->pts        = src->pts;
    dst->nOffset    = src->nOffset;
    dst->nTickCount = src->nTickCount;
    dst->nSize      = src->nSize;
    dst->qpValue    = src->qpValue;
    dst->sliceType  = src->sliceType;

    // Copy the picture buffer
    if(src->pBuffer != NULL)
#if ONE_MEMCPY
        CopyFrameBuffer(sequenceControlSet, dst->pBuffer, src->pBuffer);
#else
        CopyFrameBuffer(sequenceControlSet, dst->pBuffer, src->pBuffer);
#endif
    
}

/**********************************
 * Empty This Buffer
 **********************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265EncSendPicture(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE   *pBuffer)
{
    EbEncHandle_t          *encHandlePtr = (EbEncHandle_t*) h265EncComponent->pComponentPrivate;
    EbObjectWrapper_t      *ebWrapperPtr;
    
    // Take the buffer and put it into our internal queue structure
    EbGetEmptyObject(
        encHandlePtr->inputBufferProducerFifoPtrArray[0],
        &ebWrapperPtr);

    if (pBuffer != NULL) {
        CopyInputBuffer(
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr,
            (EB_BUFFERHEADERTYPE*)ebWrapperPtr->objectPtr,
            pBuffer);    
    }

    EbPostFullObject(ebWrapperPtr);

    return EB_ErrorNone;
}            

static void CopyOutputBuffer(
    EB_BUFFERHEADERTYPE   *dst,
    EB_BUFFERHEADERTYPE   *src
) 
{
    // copy output bitstream fileds
    dst->nSize         = src->nSize;
    dst->nAllocLen     = src->nAllocLen;
    dst->nFilledLen    = src->nFilledLen;
    dst->nOffset       = src->nOffset;
    dst->pAppPrivate   = src->pAppPrivate;
    dst->nTickCount    = src->nTickCount;
    dst->pts           = src->pts;
    dst->dts           = src->dts;
    dst->nFlags        = src->nFlags;
    dst->sliceType     = src->sliceType;
    if (src->pBuffer)
        EB_MEMCPY(dst->pBuffer, src->pBuffer, src->nFilledLen);
    if (src->pAppPrivate)
        EB_MEMCPY(dst->pAppPrivate, src->pAppPrivate, sizeof(EbLinkedListNode));

    return;
}

static void CopyOutputReconBuffer(
    EB_BUFFERHEADERTYPE   *dst,
    EB_BUFFERHEADERTYPE   *src
)
{
    // copy output bitstream fileds
    dst->nSize = src->nSize;
    dst->nAllocLen = src->nAllocLen;
    dst->nFilledLen = src->nFilledLen;
    dst->nOffset = src->nOffset;
    dst->pAppPrivate = src->pAppPrivate;
    dst->nTickCount = src->nTickCount;
    dst->pts = src->pts;
    dst->dts = src->dts;
    dst->nFlags = src->nFlags;
    dst->sliceType = src->sliceType;
    if (src->pBuffer)
        EB_MEMCPY(dst->pBuffer, src->pBuffer, src->nFilledLen);
    
    return;
}

/**********************************
 * EbH265GetPacket sends out packet
 **********************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265GetPacket(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE   *pBuffer,
    unsigned char          picSendDone)
{
    EB_ERRORTYPE           return_error = EB_ErrorNone;

    EbEncHandle_t          *pEncCompData = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    EbObjectWrapper_t      *ebWrapperPtr = NULL;

    if (picSendDone)
        EbGetFullObject(
            (pEncCompData->outputStreamBufferConsumerFifoPtrDblArray[0])[0],
            &ebWrapperPtr);
    else
        EbGetFullObjectNonBlocking(
            (pEncCompData->outputStreamBufferConsumerFifoPtrDblArray[0])[0],
            &ebWrapperPtr);

    if (ebWrapperPtr) {
        
        EB_BUFFERHEADERTYPE* objPtr = (EB_BUFFERHEADERTYPE*)ebWrapperPtr->objectPtr;
        CopyOutputBuffer(
            pBuffer,
            objPtr);
                
        if (pBuffer->nFlags != EB_BUFFERFLAG_EOS && pBuffer->nFlags != 0) {
            return_error = EB_ErrorMax;
        }
        EbReleaseObject((EbObjectWrapper_t  *)ebWrapperPtr);
    }
    else {
        return_error =  EB_NoErrorEmptyQueue;
    }
        
    return return_error;
} 


/**********************************
* Fill This Buffer
**********************************/
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265GetRecon(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE   *pBuffer)
{
    EB_ERRORTYPE           return_error = EB_ErrorNone;
    EbEncHandle_t          *pEncCompData = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    EbObjectWrapper_t      *ebWrapperPtr = NULL;

    if (pEncCompData->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.reconEnabled) {

        EbGetFullObjectNonBlocking(
            (pEncCompData->outputReconBufferConsumerFifoPtrDblArray[0])[0],
            &ebWrapperPtr);

        if (ebWrapperPtr) {

            EB_BUFFERHEADERTYPE* objPtr = (EB_BUFFERHEADERTYPE*)ebWrapperPtr->objectPtr;
            CopyOutputReconBuffer(
                pBuffer,
                objPtr);

            if (pBuffer->nFlags != EB_BUFFERFLAG_EOS && pBuffer->nFlags != 0) {
                return_error = EB_ErrorMax;
            }
            EbReleaseObject((EbObjectWrapper_t  *)ebWrapperPtr);
        }
        else {
            return_error = EB_NoErrorEmptyQueue;
        }
    }
    else {
        // recon is not enabled
        return_error = EB_ErrorMax;
    }

    return return_error;
}

void SwitchToRealTime()
{

#if  __linux__

    struct sched_param schedParam = {
        .sched_priority = 99
    };

    int retValue = pthread_setschedparam(pthread_self(), SCHED_FIFO, &schedParam);
    if (retValue == EPERM)
        printf("\n[WARNING] For best speed performance, run with sudo privileges !\n\n");

#endif
}
/**********************************
* Encoder Error Handling
**********************************/
void libSvtEncoderSendErrorExit(
    EB_HANDLETYPE          hComponent,
    EB_U32                 errorCode)
{
    EB_COMPONENTTYPE      *h265EncComponent = (EB_COMPONENTTYPE*)hComponent;
    EbEncHandle_t          *pEncCompData = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    EbObjectWrapper_t      *ebWrapperPtr = NULL;
    EB_BUFFERHEADERTYPE    *outputPacket;

    EbGetEmptyObject(
        (pEncCompData->outputStreamBufferProducerFifoPtrDblArray[0])[0],
        &ebWrapperPtr);

    outputPacket = (EB_BUFFERHEADERTYPE*)ebWrapperPtr->objectPtr;

    outputPacket->nSize     = 0;
    outputPacket->nFlags    = errorCode;
    outputPacket->pBuffer   = NULL;

    EbPostFullObject(ebWrapperPtr);
}

/**********************************
 * Encoder Handle Initialization
 **********************************/
EB_ERRORTYPE InitH265EncoderHandle(
    EB_HANDLETYPE hComponent) 
{
    EB_ERRORTYPE       return_error            = EB_ErrorNone;
    EB_COMPONENTTYPE  *h265EncComponent        = (EB_COMPONENTTYPE*) hComponent;

    printf("LIB Build date: %s %s\n",__DATE__,__TIME__);
    printf("-------------------------------------\n");
    
    SwitchToRealTime();

    // Set Component Size & Version
    h265EncComponent->nSize                     = sizeof(EB_COMPONENTTYPE);
    
    // Encoder Private Handle Ctor
    return_error = (EB_ERRORTYPE) EbEncHandleCtor(
        (EbEncHandle_t**) &(h265EncComponent->pComponentPrivate),
        h265EncComponent);
    
    return return_error;
}
#if ONE_MEMCPY
EB_ERRORTYPE AllocateFrameBuffer(
    SequenceControlSet_t       *sequenceControlSetPtr,
    EB_BUFFERHEADERTYPE        *inputBuffer)
{
    EB_ERRORTYPE   return_error = EB_ErrorNone;
    EbPictureBufferDescInitData_t inputPictureBufferDescInitData;
    EB_H265_ENC_CONFIGURATION   * config = &sequenceControlSetPtr->staticConfig;
    EB_U8 is16bit = config->encoderBitDepth > 8 ? 1 : 0;
    // Init Picture Init data
    inputPictureBufferDescInitData.maxWidth  = (EB_U16)sequenceControlSetPtr->maxInputLumaWidth;
    inputPictureBufferDescInitData.maxHeight = (EB_U16)sequenceControlSetPtr->maxInputLumaHeight;
    inputPictureBufferDescInitData.bitDepth = (EB_BITDEPTH)config->encoderBitDepth;

    if (config->compressedTenBitFormat == 1) {
        inputPictureBufferDescInitData.bufferEnableMask = 0;
    }
    else {
        inputPictureBufferDescInitData.bufferEnableMask = is16bit ? PICTURE_BUFFER_DESC_FULL_MASK : 0;
    }

    inputPictureBufferDescInitData.leftPadding = sequenceControlSetPtr->leftPadding;
    inputPictureBufferDescInitData.rightPadding = sequenceControlSetPtr->rightPadding;
    inputPictureBufferDescInitData.topPadding = sequenceControlSetPtr->topPadding;
    inputPictureBufferDescInitData.botPadding = sequenceControlSetPtr->botPadding;

    inputPictureBufferDescInitData.splitMode = is16bit ? EB_TRUE : EB_FALSE;

    inputPictureBufferDescInitData.bufferEnableMask = PICTURE_BUFFER_DESC_FULL_MASK;

    if (is16bit && config->compressedTenBitFormat == 1) {
        inputPictureBufferDescInitData.splitMode = EB_FALSE;  //do special allocation for 2bit data down below.		
    }

    // Enhanced Picture Buffer
    return_error = EbPictureBufferDescCtor(
        (EB_PTR*) &(inputBuffer->pBuffer),
        (EB_PTR)&inputPictureBufferDescInitData);

    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    if (is16bit && config->compressedTenBitFormat == 1) {
        //pack 4 2bit pixels into 1Byte
        EB_ALLIGN_MALLOC(EB_U8*, ((EbPictureBufferDesc_t*)(inputBuffer->pBuffer))->bufferBitIncY, sizeof(EB_U8) * (inputPictureBufferDescInitData.maxWidth / 4)*(inputPictureBufferDescInitData.maxHeight), EB_A_PTR);
        EB_ALLIGN_MALLOC(EB_U8*, ((EbPictureBufferDesc_t*)(inputBuffer->pBuffer))->bufferBitIncCb, sizeof(EB_U8) * (inputPictureBufferDescInitData.maxWidth / 8)*(inputPictureBufferDescInitData.maxHeight / 2), EB_A_PTR);
        EB_ALLIGN_MALLOC(EB_U8*, ((EbPictureBufferDesc_t*)(inputBuffer->pBuffer))->bufferBitIncCr, sizeof(EB_U8) * (inputPictureBufferDescInitData.maxWidth / 8)*(inputPictureBufferDescInitData.maxHeight / 2), EB_A_PTR);
    }

    return return_error;
}
#else
EB_ERRORTYPE AllocateFrameBuffer(
    EB_H265_ENC_CONFIGURATION   * config,
    EB_BUFFERHEADERTYPE       	* inputBuffer)
{
    EB_ERRORTYPE   return_error = EB_ErrorNone;

    const int tenBitPackedMode = (config->encoderBitDepth > 8) && (config->compressedTenBitFormat == 0) ? 1 : 0;

    // Determine size of each plane
    const size_t luma8bitSize =

        config->sourceWidth    *
        config->sourceHeight   *

        (1 << tenBitPackedMode);

    const size_t chroma8bitSize = luma8bitSize >> 2;
    const size_t luma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? (luma8bitSize >> 2) : 0;
    const size_t chroma10bitSize = (config->encoderBitDepth > 8 && tenBitPackedMode == 0) ? (luma10bitSize >> 2) : 0;

    EB_MALLOC(EB_U8*, inputBuffer->pBuffer, sizeof(EB_H265_ENC_INPUT), EB_N_PTR);

    // Determine  
    EB_H265_ENC_INPUT* inputPtr = (EB_H265_ENC_INPUT*)inputBuffer->pBuffer;

    if (luma8bitSize) {
        EB_MALLOC(unsigned char*, inputPtr->luma, luma8bitSize, EB_N_PTR);
    }
    else {
        inputPtr->luma = 0;
    }
    if (chroma8bitSize) {
        EB_MALLOC(unsigned char*, inputPtr->cb, chroma8bitSize, EB_N_PTR);
    }
    else {
        inputPtr->cb = 0;
    }

    if (chroma8bitSize) {
        EB_MALLOC(unsigned char*, inputPtr->cr, chroma8bitSize, EB_N_PTR);
    }
    else {
        inputPtr->cr = 0;
    }

    if (luma10bitSize) {
        EB_MALLOC(unsigned char*, inputPtr->lumaExt, luma10bitSize, EB_N_PTR);
    }
    else {
        inputPtr->lumaExt = 0;
    }

    if (chroma10bitSize) {
        EB_MALLOC(unsigned char*, inputPtr->cbExt, chroma10bitSize, EB_N_PTR);
    }
    else {
        inputPtr->cbExt = 0;
    }

    if (chroma10bitSize) {
        EB_MALLOC(unsigned char*, inputPtr->crExt, chroma10bitSize, EB_N_PTR);

    }
    else {
        inputPtr->crExt = 0;
    }

    return return_error;
}
#endif

/**************************************
* EB_BUFFERHEADERTYPE Constructor
**************************************/
EB_ERRORTYPE EbInputBufferHeaderCtor(
    EB_PTR *objectDblPtr,
    EB_PTR  objectInitDataPtr)
{
    EB_BUFFERHEADERTYPE* inputBuffer;
    SequenceControlSet_t        *sequenceControlSetPtr = (SequenceControlSet_t*)objectInitDataPtr;
#if !ONE_MEMCPY 
    EB_H265_ENC_CONFIGURATION   *config = &sequenceControlSetPtr->staticConfig;
#endif
    EB_MALLOC(EB_BUFFERHEADERTYPE*, inputBuffer, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR);
    *objectDblPtr = (EB_PTR)inputBuffer;
    // Initialize Header
    inputBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);
#if !ONE_MEMCPY 
    // Allocate frame buffer for the pBuffer
    AllocateFrameBuffer(
        config,
        inputBuffer);
#else
    AllocateFrameBuffer(
        sequenceControlSetPtr,
        inputBuffer);
#endif
    // Assign the variables 
    EB_MALLOC(EbLinkedListNode*, inputBuffer->pAppPrivate, sizeof(EbLinkedListNode), EB_N_PTR);
    ((EbLinkedListNode*)(inputBuffer->pAppPrivate))->type = EB_FALSE;

    return EB_ErrorNone;
}

/**************************************
 * EB_BUFFERHEADERTYPE Constructor
 **************************************/  
EB_ERRORTYPE EbOutputBufferHeaderCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr) 
{
#if CHKN_OMX
    EB_H265_ENC_CONFIGURATION   * config = (EB_H265_ENC_CONFIGURATION*)objectInitDataPtr;
    EB_U32 nStride = (EB_U32)(EB_OUTPUTSTREAMBUFFERSIZE_MACRO(config->sourceWidth * config->sourceHeight));  //TBC
	EB_BUFFERHEADERTYPE* outBufPtr;

	EB_MALLOC(EB_BUFFERHEADERTYPE*, outBufPtr, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR);
	*objectDblPtr = (EB_PTR)outBufPtr;

	// Initialize Header
	outBufPtr->nSize = sizeof(EB_BUFFERHEADERTYPE);

	EB_MALLOC(EB_U8*, outBufPtr->pBuffer, nStride, EB_N_PTR);

	outBufPtr->nAllocLen =  nStride;
	outBufPtr->pAppPrivate = 0;// (EB_PTR)callbackData;
	
	    (void)objectInitDataPtr;
#else
    *objectDblPtr = (EB_PTR)EB_NULL;
    objectInitDataPtr = (EB_PTR)EB_NULL;

    (void)objectDblPtr;
    (void)objectInitDataPtr;
#endif
    return EB_ErrorNone;
}

/**************************************
* EB_BUFFERHEADERTYPE Constructor
**************************************/
EB_ERRORTYPE EbOutputReconBufferHeaderCtor(
    EB_PTR *objectDblPtr,
    EB_PTR  objectInitDataPtr)
{
    EB_BUFFERHEADERTYPE         *reconBuffer;
    SequenceControlSet_t        *sequenceControlSetPtr = (SequenceControlSet_t*)objectInitDataPtr;
    const EB_U32 lumaSize =
        sequenceControlSetPtr->lumaWidth    *
        sequenceControlSetPtr->lumaHeight;
    // both u and v
    const EB_U32 chromaSize = lumaSize >> 1;
    const EB_U32 tenBit = (sequenceControlSetPtr->staticConfig.encoderBitDepth > 8);
    const EB_U32 frameSize = (lumaSize + chromaSize) << tenBit;

    EB_MALLOC(EB_BUFFERHEADERTYPE*, reconBuffer, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR);
    *objectDblPtr = (EB_PTR)reconBuffer;

    // Initialize Header
    reconBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);

    // Assign the variables 
    EB_MALLOC(EB_U8*, reconBuffer->pBuffer, frameSize, EB_N_PTR);

    reconBuffer->nAllocLen   = frameSize;
    reconBuffer->pAppPrivate = NULL;

    return EB_ErrorNone;
}


/* SAFE STRING LIBRARY */

#ifndef EOK
#define EOK             ( 0 )
#endif

#ifndef ESZEROL
#define ESZEROL         ( 401 )       /* length is zero              */
#endif

#ifndef ESLEMIN
#define ESLEMIN         ( 402 )       /* length is below min         */
#endif

#ifndef ESLEMAX
#define ESLEMAX         ( 403 )       /* length exceeds max          */
#endif

#ifndef ESNULLP
#define ESNULLP         ( 400 )       /* null ptr                    */
#endif

#ifndef ESOVRLP
#define ESOVRLP         ( 404 )       /* overlap undefined           */
#endif

#ifndef ESEMPTY
#define ESEMPTY         ( 405 )       /* empty string                */
#endif

#ifndef ESNOSPC
#define ESNOSPC         ( 406 )       /* not enough space for s2     */
#endif

#ifndef ESUNTERM
#define ESUNTERM        ( 407 )       /* unterminated string         */
#endif

#ifndef ESNODIFF
#define ESNODIFF        ( 408 )       /* no difference               */
#endif

#ifndef ESNOTFND
#define ESNOTFND        ( 409 )       /* not found                   */
#endif

#define RSIZE_MAX_MEM      ( 256UL << 20 )     /* 256MB */

#define RCNEGATE(x)  (x)
#define RSIZE_MAX_STR      ( 4UL << 10 )      /* 4KB */
#define sl_default_handler ignore_handler_s
#define EXPORT_SYMBOL(sym)

#ifndef sldebug_printf
#define sldebug_printf(...)
#endif

#ifndef _RSIZE_T_DEFINED
typedef size_t rsize_t;
#define _RSIZE_T_DEFINED
#endif  /* _RSIZE_T_DEFINED */

#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int errno_t;
#endif  /* _ERRNO_T_DEFINED */

/*
* Function used by the libraries to invoke the registered
* runtime-constraint handler. Always needed.
*/

typedef void(*constraint_handler_t) (const char * /* msg */,
    void *       /* ptr */,
    errno_t      /* error */);
extern void ignore_handler_s(const char *msg, void *ptr, errno_t error);

/*
* Function used by the libraries to invoke the registered
* runtime-constraint handler. Always needed.
*/
extern void invoke_safe_str_constraint_handler(
    const char *msg,
    void *ptr,
    errno_t error);


static inline void handle_error(char *orig_dest, rsize_t orig_dmax,
    char *err_msg, errno_t err_code)
{
    (void)orig_dmax;
    *orig_dest = '\0';

    invoke_safe_str_constraint_handler(err_msg, NULL, err_code);
    return;
}
static constraint_handler_t str_handler = NULL;

void
invoke_safe_str_constraint_handler(const char *msg,
void *ptr,
errno_t error)
{
	if (NULL != str_handler) {
		str_handler(msg, ptr, error);
	}
	else {
		sl_default_handler(msg, ptr, error);
	}
}

void ignore_handler_s(const char *msg, void *ptr, errno_t error)
{
	(void)msg;
	(void)ptr;
	(void)error;
	sldebug_printf("IGNORE CONSTRAINT HANDLER: (%u) %s\n", error,
		(msg) ? msg : "Null message");
	return;
}
EXPORT_SYMBOL(ignore_handler_s)

errno_t
strncpy_ss(char *dest, rsize_t dmax, const char *src, rsize_t slen)
{
	rsize_t orig_dmax;
	char *orig_dest;
	const char *overlap_bumper;

	if (dest == NULL) {
		invoke_safe_str_constraint_handler((char*) ("strncpy_ss: dest is null"),
			NULL, ESNULLP);
		return RCNEGATE(ESNULLP);
	}

	if (dmax == 0) {
		invoke_safe_str_constraint_handler((char*)("strncpy_ss: dmax is 0"),
			NULL, ESZEROL);
		return RCNEGATE(ESZEROL);
	}

	if (dmax > RSIZE_MAX_STR) {
		invoke_safe_str_constraint_handler((char*)("strncpy_ss: dmax exceeds max"),
			NULL, ESLEMAX);
		return RCNEGATE(ESLEMAX);
	}

	/* hold base in case src was not copied */
	orig_dmax = dmax;
	orig_dest = dest;

	if (src == NULL) {
		handle_error(orig_dest, orig_dmax, (char*)("strncpy_ss: "
			"src is null"),
			ESNULLP);
		return RCNEGATE(ESNULLP);
	}

	if (slen == 0) {
		handle_error(orig_dest, orig_dmax, (char*)("strncpy_ss: "
			"slen is zero"),
			ESZEROL);
		return RCNEGATE(ESZEROL);
	}

	if (slen > RSIZE_MAX_STR) {
		handle_error(orig_dest, orig_dmax, (char*)("strncpy_ss: "
			"slen exceeds max"),
			ESLEMAX);
		return RCNEGATE(ESLEMAX);
	}


	if (dest < src) {
		overlap_bumper = src;

		while (dmax > 0) {
			if (dest == overlap_bumper) {
				handle_error(orig_dest, orig_dmax, (char*)("strncpy_ss: "
					"overlapping objects"),
					ESOVRLP);
				return RCNEGATE(ESOVRLP);
			}

			if (slen == 0) {
				/*
				* Copying truncated to slen chars.  Note that the TR says to
				* copy slen chars plus the null char.  We null the slack.
				*/
				*dest = '\0';
				return RCNEGATE(EOK);
			}

			*dest = *src;
			if (*dest == '\0') {
				return RCNEGATE(EOK);
			}

			dmax--;
			slen--;
			dest++;
			src++;
		}

	}
	else {
		overlap_bumper = dest;

		while (dmax > 0) {
			if (src == overlap_bumper) {
				handle_error(orig_dest, orig_dmax, (char*)("strncpy_s: "
					"overlapping objects"),
					ESOVRLP);
				return RCNEGATE(ESOVRLP);
			}

			if (slen == 0) {
				/*
				* Copying truncated to slen chars.  Note that the TR says to
				* copy slen chars plus the null char.  We null the slack.
				*/
				*dest = '\0';
				return RCNEGATE(EOK);
			}

			*dest = *src;
			if (*dest == '\0') {
				return RCNEGATE(EOK);
			}

			dmax--;
			slen--;
			dest++;
			src++;
		}
	}

	/*
	* the entire src was not copied, so zero the string
	*/
	handle_error(orig_dest, orig_dmax, (char*)("strncpy_ss: not enough "
		"space for src"),
		ESNOSPC);
	return RCNEGATE(ESNOSPC);
}
EXPORT_SYMBOL(strncpy_ss)

errno_t
strcpy_ss(char *dest, rsize_t dmax, const char *src)
{
	rsize_t orig_dmax;
	char *orig_dest;
	const char *overlap_bumper;

	if (dest == NULL) {
		invoke_safe_str_constraint_handler((char*)("strcpy_ss: dest is null"),
			NULL, ESNULLP);
		return RCNEGATE(ESNULLP);
	}

	if (dmax == 0) {
		invoke_safe_str_constraint_handler((char*)("strcpy_ss: dmax is 0"),
			NULL, ESZEROL);
		return RCNEGATE(ESZEROL);
	}

	if (dmax > RSIZE_MAX_STR) {
		invoke_safe_str_constraint_handler((char*)("strcpy_ss: dmax exceeds max"),
			NULL, ESLEMAX);
		return RCNEGATE(ESLEMAX);
	}

	if (src == NULL) {
		*dest = '\0';
		invoke_safe_str_constraint_handler((char*)("strcpy_ss: src is null"),
			NULL, ESNULLP);
		return RCNEGATE(ESNULLP);
	}

	if (dest == src) {
		return RCNEGATE(EOK);
	}

	/* hold base of dest in case src was not copied */
	orig_dmax = dmax;
	orig_dest = dest;

	if (dest < src) {
		overlap_bumper = src;

		while (dmax > 0) {
			if (dest == overlap_bumper) {
				handle_error(orig_dest, orig_dmax, (char*)("strcpy_ss: "
					"overlapping objects"),
					ESOVRLP);
				return RCNEGATE(ESOVRLP);
			}

			*dest = *src;
			if (*dest == '\0') {
				return RCNEGATE(EOK);
			}

			dmax--;
			dest++;
			src++;
		}

	}
	else {
		overlap_bumper = dest;

		while (dmax > 0) {
			if (src == overlap_bumper) {
				handle_error(orig_dest, orig_dmax, (char*)("strcpy_ss: "
					"overlapping objects"),
					ESOVRLP);
				return RCNEGATE(ESOVRLP);
			}

			*dest = *src;
			if (*dest == '\0') {
				return RCNEGATE(EOK);
			}

			dmax--;
			dest++;
			src++;
		}
	}

	/*
	* the entire src must have been copied, if not reset dest
	* to null the string.
	*/
	handle_error(orig_dest, orig_dmax, (char*)("strcpy_ss: not "
		"enough space for src"),
		ESNOSPC);
	return RCNEGATE(ESNOSPC);
}
EXPORT_SYMBOL(strcpy_ss)

rsize_t
strnlen_ss(const char *dest, rsize_t dmax)
{
	rsize_t count;

	if (dest == NULL) {
		return RCNEGATE(0);
	}

	if (dmax == 0) {
		invoke_safe_str_constraint_handler("strnlen_ss: dmax is 0",
			NULL, ESZEROL);
		return RCNEGATE(0);
	}

	if (dmax > RSIZE_MAX_STR) {
		invoke_safe_str_constraint_handler("strnlen_ss: dmax exceeds max",
			NULL, ESLEMAX);
		return RCNEGATE(0);
	}

	count = 0;
	while (*dest && dmax) {
		count++;
		dmax--;
		dest++;
	}

	return RCNEGATE(count);
}
EXPORT_SYMBOL(strnlen_ss)

/* SAFE STRING LIBRARY */