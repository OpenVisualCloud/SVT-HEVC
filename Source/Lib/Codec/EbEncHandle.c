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
#include <inttypes.h>

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
#define EB_OUTPUTSTREAMBUFFERSIZE_MACRO(ResolutionSize)                ((ResolutionSize) < (INPUT_SIZE_1080i_TH) ? 0x1E8480 : (ResolutionSize) < (INPUT_SIZE_1080p_TH) ? 0x2DC6C0 : (ResolutionSize) < (INPUT_SIZE_4K_TH) ? 0x2DC6C0 : (ResolutionSize) < (INPUT_SIZE_8K_TH) ? 0x2DC6C0:0x5B8D80)

static EB_U64 maxLumaPictureSize[TOTAL_LEVEL_COUNT] = { 36864U, 122880U, 245760U, 552960U, 983040U, 2228224U, 2228224U, 8912896U, 8912896U, 8912896U, 35651584U, 35651584U, 35651584U };
static EB_U64 maxLumaSampleRate[TOTAL_LEVEL_COUNT] = { 552960U, 3686400U, 7372800U, 16588800U, 33177600U, 66846720U, 133693440U, 267386880U, 534773760U, 1069547520U, 1069547520U, 2139095040U, 4278190080U };
static EB_U32 mainTierCPB[TOTAL_LEVEL_COUNT] = { 350000, 1500000, 3000000, 6000000, 10000000, 12000000, 20000000, 25000000, 40000000, 60000000, 60000000, 120000000, 240000000 };
static EB_U32 highTierCPB[TOTAL_LEVEL_COUNT] = { 350000, 1500000, 3000000, 6000000, 10000000, 30000000, 50000000, 100000000, 160000000, 240000000, 240000000, 480000000, 800000000 };
static EB_U32 mainTierMaxBitRate[TOTAL_LEVEL_COUNT] = { 128000, 1500000, 3000000, 6000000, 10000000, 12000000, 20000000, 25000000, 40000000, 60000000, 60000000, 120000000, 240000000 };
static EB_U32 highTierMaxBitRate[TOTAL_LEVEL_COUNT] = { 128000, 1500000, 3000000, 6000000, 10000000, 30000000, 50000000, 100000000, 160000000, 240000000, 240000000, 480000000, 800000000 };
static EB_U32 maxTileColumn[TOTAL_LEVEL_COUNT] = { 1, 1, 1, 2, 3, 5, 5, 10, 10, 10, 20, 20, 20 };
static EB_U32 maxTileRow[TOTAL_LEVEL_COUNT]    = { 1, 1, 1, 2, 3, 5, 5, 11, 11, 11, 22, 22, 22 };

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

EB_U8                           numGroups = 0;
#ifdef _WIN32
GROUP_AFFINITY                  groupAffinity;
EB_BOOL                         alternateGroups = 0;
#elif defined(__linux__)
cpu_set_t                       groupAffinity;
typedef struct logicalProcessorGroup {
    uint32_t num;
    uint32_t group[1024];
}processorGroup;
#define MAX_PROCESSOR_GROUP 16
processorGroup                   lpGroup[MAX_PROCESSOR_GROUP];
#endif

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
#ifdef NON_AVX512_SUPPORT
        asmType = 3; // bit-field
#else
        asmType = 7; // bit-field
#endif
    else
    if (CanUseIntelCore4thGenFeatures() == 1){
        asmType = 3; // bit-field
    }
    else{
        asmType = 1; // bit-field
    }
	return asmType;
}

//Get Number of logical processors
EB_U32 GetNumProcessors() {
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return numGroups == 1 ? sysinfo.dwNumberOfProcessors : sysinfo.dwNumberOfProcessors << 1;
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

EB_ERRORTYPE InitThreadManagmentParams(){
#ifdef _WIN32
    // Initialize groupAffinity structure with Current thread info
    GetThreadGroupAffinity(GetCurrentThread(),&groupAffinity);
    numGroups = (EB_U8) GetActiveProcessorGroupCount();
#elif defined(__linux__)
    const char* PROCESSORID = "processor";
    const char* PHYSICALID = "physical id";
    int processor_id_len = strnlen_ss(PROCESSORID, 128);
    int physical_id_len = strnlen_ss(PHYSICALID, 128);
    if (processor_id_len < 0 || processor_id_len >= 128) return EB_ErrorInsufficientResources;
    if (physical_id_len < 0 || physical_id_len >= 128) return EB_ErrorInsufficientResources;
    memset(lpGroup, 0, 16* sizeof(processorGroup));

    FILE *fin = fopen("/proc/cpuinfo", "r");
    if (fin) {
        int processor_id = 0, socket_id = 0;
        char line[1024];
        while (fgets(line, sizeof(line), fin)) {
            if(strncmp(line, PROCESSORID, processor_id_len) == 0) {
                char* p = line + processor_id_len;
                while(*p < '0' || *p > '9') p++;
                processor_id = strtol(p, NULL, 0);
            }
            if(strncmp(line, PHYSICALID, physical_id_len) == 0) {
                char* p = line + physical_id_len;
                while(*p < '0' || *p > '9') p++;
                socket_id = strtol(p, NULL, 0);
                if (socket_id < 0 || socket_id > 15) {
                    fclose(fin);
                    return EB_ErrorInsufficientResources;
                }
                if (socket_id + 1 > numGroups)
                    numGroups = socket_id + 1;
                lpGroup[socket_id].group[lpGroup[socket_id].num++] = processor_id;
            }
        }
        fclose(fin);
    }
#endif
    return EB_ErrorNone;
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

    return_error = InitThreadManagmentParams();
    if (return_error == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

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

#ifdef _WIN32
EB_U64 GetAffinityMask(EB_U32 lpnum) {
    EB_U64 mask = 0x1;
    for (EB_U32 i = lpnum - 1; i > 0; i--)
        mask += (EB_U64)1 << i;
    return mask;
}
#endif

void SwitchToRealTime()
{

#if  __linux__

    struct sched_param schedParam = {
        .sched_priority = sched_get_priority_max(SCHED_FIFO)
    };

    int retValue = pthread_setschedparam(pthread_self(), SCHED_FIFO, &schedParam);
    if (retValue == EPERM)
        SVT_LOG("\nSVT [WARNING] Elevated privileges required to run with real-time policies! Check Linux Best Known Configuration in User Guide to run application in real-time without elevated privileges!\n\n");

#endif
}

void EbSetThreadManagementParameters(
    EB_H265_ENC_CONFIGURATION   *configPtr)
{
    EB_U32 numLogicProcessors = GetNumProcessors();

    if (configPtr->switchThreadsToRtPriority == 1) {
        SwitchToRealTime();
    }

#ifdef _WIN32
    // For system with a single processor group(no more than 64 logic processors all together)
    // Affinity of the thread can be set to one or more logical processors
    if (numGroups == 1) {
        EB_U32 lps = configPtr->logicalProcessors == 0 ? numLogicProcessors:
            configPtr->logicalProcessors < numLogicProcessors ? configPtr->logicalProcessors : numLogicProcessors;
        groupAffinity.Mask = GetAffinityMask(lps);
    }
    else if (numGroups > 1) { // For system with multiple processor group
        if (configPtr->logicalProcessors == 0) {
            if (configPtr->targetSocket != -1) {
                groupAffinity.Group = configPtr->targetSocket;
            }
        }
        else {
            EB_U32 numLpPerGroup = numLogicProcessors / numGroups;
            if (configPtr->targetSocket == -1) {
                if (configPtr->logicalProcessors > numLpPerGroup) {
                    alternateGroups = TRUE;
                    SVT_LOG("SVT [WARNING]: -lp(logical processors) setting is ignored. Run on both sockets. \n");
                }
                else {
                    groupAffinity.Mask = GetAffinityMask(configPtr->logicalProcessors);
                }
            }
            else {
                EB_U32 lps = configPtr->logicalProcessors == 0 ? numLpPerGroup :
                    configPtr->logicalProcessors < numLpPerGroup ? configPtr->logicalProcessors : numLpPerGroup;
                groupAffinity.Mask = GetAffinityMask(lps);
                groupAffinity.Group = configPtr->targetSocket;
            }
        }
    }
#elif defined(__linux__)
    CPU_ZERO(&groupAffinity);
    if (numGroups == 1) {
        EB_U32 lps = configPtr->logicalProcessors == 0 ? numLogicProcessors:
            configPtr->logicalProcessors < numLogicProcessors ? configPtr->logicalProcessors : numLogicProcessors;
        for(EB_U32 i=0; i<lps; i++)
            CPU_SET(lpGroup[0].group[i], &groupAffinity);
    }
    else if (numGroups > 1) {
        EB_U32 numLpPerGroup = numLogicProcessors / numGroups;
        if (configPtr->logicalProcessors == 0) {
            if (configPtr->targetSocket != -1) {
                for(EB_U32 i=0; i<lpGroup[configPtr->targetSocket].num; i++)
                    CPU_SET(lpGroup[configPtr->targetSocket].group[i], &groupAffinity);
            }
        }
        else {
            if (configPtr->targetSocket == -1) {
                EB_U32 lps = configPtr->logicalProcessors == 0 ? numLogicProcessors:
                    configPtr->logicalProcessors < numLogicProcessors ? configPtr->logicalProcessors : numLogicProcessors;
                if(lps > numLpPerGroup) {
                    for(EB_U32 i=0; i<lpGroup[0].num; i++)
                        CPU_SET(lpGroup[0].group[i], &groupAffinity);
                    for(EB_U32 i=0; i< (lps -lpGroup[0].num); i++)
                        CPU_SET(lpGroup[1].group[i], &groupAffinity);
                }
                else {
                    for(EB_U32 i=0; i<lps; i++)
                        CPU_SET(lpGroup[0].group[i], &groupAffinity);
                }
            }
            else {
                EB_U32 lps = configPtr->logicalProcessors == 0 ? numLpPerGroup :
                    configPtr->logicalProcessors < numLpPerGroup ? configPtr->logicalProcessors : numLpPerGroup;
                for(EB_U32 i=0; i<lps; i++)
                    CPU_SET(lpGroup[configPtr->targetSocket].group[i], &groupAffinity);
            }
        }
    }
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

    EB_BOOL is16bit = (EB_BOOL) (encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);

    /************************************
    * Plateform detection
    ************************************/
    if (encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.asmType == EB_ASM_AUTO) {
        ASM_TYPES = GetCpuAsmType(); // Use highest assembly
    }
    else if (encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.asmType == EB_ASM_C) {
        ASM_TYPES = EB_ASM_C; // Use C_only
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
        inputData.colorFormat           = (EB_COLOR_FORMAT)encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->chromaFormatIdc;
        inputData.lcuSize               = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize;
        inputData.maxDepth              = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxLcuDepth;
        inputData.is16bit               = is16bit;
		inputData.compressedTenBitFormat = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.compressedTenBitFormat;

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
        inputData.colorFormat       = (EB_COLOR_FORMAT)encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->chromaFormatIdc;
        inputData.lcuSize           = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize;
        inputData.maxDepth          = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxLcuDepth;
        inputData.is16bit           = is16bit;
        inputData.tileRowCount      = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.tileRowCount;
        inputData.tileColumnCount   = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.tileColumnCount;

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
        referencePictureBufferDescInitData.colorFormat            =  (EB_COLOR_FORMAT)encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->chromaFormatIdc;
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
        referencePictureBufferDescInitData.colorFormat            = EB_YUV420;
        referencePictureBufferDescInitData.bufferEnableMask = 0;
		referencePictureBufferDescInitData.leftPadding            = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize + ME_FILTER_TAP;
		referencePictureBufferDescInitData.rightPadding           = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize + ME_FILTER_TAP;
		referencePictureBufferDescInitData.topPadding             = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize + ME_FILTER_TAP;
		referencePictureBufferDescInitData.botPadding             = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize + ME_FILTER_TAP;
        referencePictureBufferDescInitData.splitMode              = EB_FALSE;

        quarterDecimPictureBufferDescInitData.maxWidth              = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth  >> 1;
        quarterDecimPictureBufferDescInitData.maxHeight             = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight >> 1;
        quarterDecimPictureBufferDescInitData.bitDepth              = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->inputBitdepth;
        quarterDecimPictureBufferDescInitData.colorFormat           = EB_YUV420;
        quarterDecimPictureBufferDescInitData.bufferEnableMask      = PICTURE_BUFFER_DESC_LUMA_MASK;
		quarterDecimPictureBufferDescInitData.leftPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 1;
		quarterDecimPictureBufferDescInitData.rightPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 1;
		quarterDecimPictureBufferDescInitData.topPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 1;
		quarterDecimPictureBufferDescInitData.botPadding			= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize >> 1;
        quarterDecimPictureBufferDescInitData.splitMode             = EB_FALSE;

        sixteenthDecimPictureBufferDescInitData.maxWidth            = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth  >> 2;
        sixteenthDecimPictureBufferDescInitData.maxHeight           = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight >> 2;
        sixteenthDecimPictureBufferDescInitData.bitDepth            = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->inputBitdepth;
        sixteenthDecimPictureBufferDescInitData.colorFormat         = EB_YUV420;
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
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->inputOutputBufferFifoInitCount,
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
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->inputOutputBufferFifoInitCount + 4, // to accommodate output error + vps + eos
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
		encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->streamOutputFifoPtr  = (encHandlePtr->outputStreamBufferProducerFifoPtrDblArray[instanceIndex])[0];
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
        pictureBufferDescConf.colorFormat = (EB_COLOR_FORMAT)encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->chromaFormatIdc;
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
            is16bit,
            (EB_COLOR_FORMAT)encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->chromaFormatIdc);
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
#ifdef _WIN32
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
        //SVT_LOG("Error: Component Struct Malloc Failed\n");
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
EB_U32 SetParentPcs(EB_H265_ENC_CONFIGURATION*   config)
{

    EB_U32 inputPic = 100;
    EB_U32 fps = (EB_U32)((config->frameRate > 1000) ? config->frameRate >> 16 : config->frameRate);

    fps = fps > 120 ? 120 : fps;
    fps = fps < 24 ? 24 : fps;

    if (((EB_U32)(config->intraPeriodLength) > (fps << 1)) && ((config->sourceWidth * config->sourceHeight) < INPUT_SIZE_4K_TH))
        fps = config->intraPeriodLength;

    EB_U32     lowLatencyInput = (config->encMode < 6 || config->speedControlFlag == 1) ? fps :
        (config->encMode < 8) ? fps >> 1 : (EB_U32)((2 << config->hierarchicalLevels) + SCD_LAD);

    EB_U32     normalLatencyInput = (fps * 3) >> 1;

    if ((config->sourceWidth * config->sourceHeight) > INPUT_SIZE_4K_TH)
        normalLatencyInput = (normalLatencyInput * 3) >> 1;

    if (config->latencyMode == 0)
        inputPic = (normalLatencyInput + config->lookAheadDistance);
    else
        inputPic = (EB_U32)(lowLatencyInput + config->lookAheadDistance);

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

    unsigned int lpCount = GetNumProcessors();
    unsigned int coreCount = lpCount;
#if defined(_WIN32) || defined(__linux__)
    if (sequenceControlSetPtr->staticConfig.targetSocket != -1)
        coreCount /= numGroups;
    if (sequenceControlSetPtr->staticConfig.logicalProcessors != 0)
        coreCount = sequenceControlSetPtr->staticConfig.logicalProcessors < coreCount ?
            sequenceControlSetPtr->staticConfig.logicalProcessors: coreCount;
#endif

#ifdef _WIN32
    //Handle special case on Windows
    //By default, on Windows an application is constrained to a single group
    if (sequenceControlSetPtr->staticConfig.targetSocket == -1 &&
        sequenceControlSetPtr->staticConfig.logicalProcessors == 0)
        coreCount /= numGroups;

    //Affininty can only be set by group on Windows.
    //Run on both sockets if -lp is larger than logical processor per group.
    if (sequenceControlSetPtr->staticConfig.targetSocket == -1 &&
        sequenceControlSetPtr->staticConfig.logicalProcessors > lpCount / numGroups)
        coreCount = lpCount;
#endif

    sequenceControlSetPtr->inputOutputBufferFifoInitCount = inputPic + SCD_LAD;

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
    sequenceControlSetPtr->referencePictureBufferInitCount      = inputPic;//MAX((EB_U32)(sequenceControlSetPtr->inputOutputBufferFifoInitCount >> 1), (EB_U32)((1 << sequenceControlSetPtr->staticConfig.hierarchicalLevels) + 2));
    sequenceControlSetPtr->paReferencePictureBufferInitCount    = inputPic;//MAX((EB_U32)(sequenceControlSetPtr->inputOutputBufferFifoInitCount >> 1), (EB_U32)((1 << sequenceControlSetPtr->staticConfig.hierarchicalLevels) + 2));
    sequenceControlSetPtr->reconBufferFifoInitCount             = sequenceControlSetPtr->referencePictureBufferInitCount;

    //#====================== Inter process Fifos ======================
    sequenceControlSetPtr->resourceCoordinationFifoInitCount = 300;
    sequenceControlSetPtr->pictureAnalysisFifoInitCount = 301;
    sequenceControlSetPtr->pictureDecisionFifoInitCount = 302;
    sequenceControlSetPtr->initialRateControlFifoInitCount = 303;
    sequenceControlSetPtr->pictureDemuxFifoInitCount = 304;
    sequenceControlSetPtr->rateControlTasksFifoInitCount = 305;
    sequenceControlSetPtr->rateControlFifoInitCount = 306;
    //sequenceControlSetPtr->modeDecisionFifoInitCount = 307;
    sequenceControlSetPtr->modeDecisionConfigurationFifoInitCount = (300 * sequenceControlSetPtr->tileRowCount);
    sequenceControlSetPtr->motionEstimationFifoInitCount = 308;
    sequenceControlSetPtr->entropyCodingFifoInitCount = 309;
    sequenceControlSetPtr->encDecFifoInitCount = 900;

    //#====================== Processes number ======================
    sequenceControlSetPtr->totalProcessInitCount = 0;
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->pictureAnalysisProcessInitCount              = MAX(15, coreCount / 6);
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->motionEstimationProcessInitCount             = MAX(20, coreCount / 3);
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->sourceBasedOperationsProcessInitCount        = MAX(3, coreCount / 12);
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount    = MAX(3, coreCount / 12);
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->encDecProcessInitCount                       = MAX(40, coreCount);
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->entropyCodingProcessInitCount                = MAX(3, coreCount / 12);

    sequenceControlSetPtr->totalProcessInitCount += 6; // single processes count
    SVT_LOG("Number of logical cores available: %u\nNumber of PPCS %u\n", coreCount, inputPic);

    return;

}

// Sets the default intra period the closest possible to 1 second without breaking the minigop
static EB_S32 ComputeIntraPeriod(
    SequenceControlSet_t       *sequenceControlSetPtr)
{
    EB_S32 intraPeriod = 0;
    EB_H265_ENC_CONFIGURATION   *config = &sequenceControlSetPtr->staticConfig;
    EB_S32 fps = config->frameRate <= (240) ? config->frameRate : config->frameRate >> 16;
    EB_S32 miniGopSize = (1 << (config->hierarchicalLevels));
    EB_S32 minIp = ((int)((fps) / miniGopSize)*(miniGopSize));
    EB_S32 maxIp = ((int)((fps + miniGopSize) / miniGopSize)*(miniGopSize));

    intraPeriod = (ABS((fps - maxIp)) > ABS((fps - minIp))) ? minIp : maxIp;

    if(config->intraRefreshType == 1)
        intraPeriod -= 1;

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
        //sequenceControlSetPtr->maxInputChromaWidth = sequenceControlSetPtr->maxInputLumaWidth >> 1; //TODO: change here
    }
    else {

        sequenceControlSetPtr->maxInputPadRight = 0;
    }
    if (sequenceControlSetPtr->maxInputLumaHeight % MIN_CU_SIZE) {

        sequenceControlSetPtr->maxInputPadBottom = MIN_CU_SIZE - (sequenceControlSetPtr->maxInputLumaHeight % MIN_CU_SIZE);
        sequenceControlSetPtr->maxInputLumaHeight = sequenceControlSetPtr->maxInputLumaHeight + sequenceControlSetPtr->maxInputPadBottom;
        //sequenceControlSetPtr->maxInputChromaHeight = sequenceControlSetPtr->maxInputLumaHeight >> 1; //ditto
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
)
{

    EB_MEMCPY(&sequenceControlSetPtr->staticConfig, pComponentParameterStructure, sizeof(EB_H265_ENC_CONFIGURATION));

    sequenceControlSetPtr->maxInputLumaWidth  = (EB_U16)((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->sourceWidth;
    sequenceControlSetPtr->maxInputLumaHeight = (EB_U16)((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->sourceHeight;

    if (sequenceControlSetPtr->staticConfig.tune >= 1) {
        sequenceControlSetPtr->staticConfig.bitRateReduction = 0;
        sequenceControlSetPtr->staticConfig.improveSharpness = 0;
    }

    sequenceControlSetPtr->intraPeriodLength = sequenceControlSetPtr->staticConfig.intraPeriodLength;
    sequenceControlSetPtr->intraRefreshType = sequenceControlSetPtr->staticConfig.intraRefreshType;
    sequenceControlSetPtr->maxTemporalLayers = sequenceControlSetPtr->staticConfig.hierarchicalLevels;
    sequenceControlSetPtr->maxRefCount = 1;
    sequenceControlSetPtr->tileRowCount = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->tileRowCount;
    sequenceControlSetPtr->tileColumnCount = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->tileColumnCount;
    sequenceControlSetPtr->tileSliceMode = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->tileSliceMode;


    // Quantization
    sequenceControlSetPtr->qp = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->qp;

    if (((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->frameRate > 1000)
        sequenceControlSetPtr->frameRate = sequenceControlSetPtr->staticConfig.frameRate = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->frameRate;
    else
        sequenceControlSetPtr->frameRate = sequenceControlSetPtr->staticConfig.frameRate = (((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->frameRate << 16);
    sequenceControlSetPtr->staticConfig.targetBitRate = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->targetBitRate;
    sequenceControlSetPtr->encodeContextPtr->availableTargetBitRate = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->targetBitRate;

    sequenceControlSetPtr->staticConfig.maxQpAllowed = (sequenceControlSetPtr->staticConfig.rateControlMode) ?
        ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->maxQpAllowed :
        51;

    sequenceControlSetPtr->staticConfig.minQpAllowed = (sequenceControlSetPtr->staticConfig.rateControlMode) ?
        ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->minQpAllowed :
        0;

    // Misc
    if (sequenceControlSetPtr->staticConfig.encoderColorFormat == EB_YUV400) {
        SVT_LOG("SVT [Warning]: Color format EB_YUV400 not supported, set to EB_YUV420\n");
        sequenceControlSetPtr->staticConfig.encoderColorFormat = EB_YUV420;
    }
    sequenceControlSetPtr->chromaFormatIdc = (EB_U32)(sequenceControlSetPtr->staticConfig.encoderColorFormat);
    sequenceControlSetPtr->enableTmvpSps = sequenceControlSetPtr->staticConfig.unrestrictedMotionVector;

    // Copying to masteringDisplayColorVolume structure
    sequenceControlSetPtr->masteringDisplayColorVolume.displayPrimaryX[0] = sequenceControlSetPtr->staticConfig.displayPrimaryX[0];
    sequenceControlSetPtr->masteringDisplayColorVolume.displayPrimaryX[1] = sequenceControlSetPtr->staticConfig.displayPrimaryX[1];
    sequenceControlSetPtr->masteringDisplayColorVolume.displayPrimaryX[2] = sequenceControlSetPtr->staticConfig.displayPrimaryX[2];
    sequenceControlSetPtr->masteringDisplayColorVolume.displayPrimaryY[0] = sequenceControlSetPtr->staticConfig.displayPrimaryY[0];
    sequenceControlSetPtr->masteringDisplayColorVolume.displayPrimaryY[1] = sequenceControlSetPtr->staticConfig.displayPrimaryY[1];
    sequenceControlSetPtr->masteringDisplayColorVolume.displayPrimaryY[2] = sequenceControlSetPtr->staticConfig.displayPrimaryY[2];
    sequenceControlSetPtr->masteringDisplayColorVolume.whitePointX = sequenceControlSetPtr->staticConfig.whitePointX;
    sequenceControlSetPtr->masteringDisplayColorVolume.whitePointY = sequenceControlSetPtr->staticConfig.whitePointY;
    sequenceControlSetPtr->masteringDisplayColorVolume.maxDisplayMasteringLuminance = sequenceControlSetPtr->staticConfig.maxDisplayMasteringLuminance;
    sequenceControlSetPtr->masteringDisplayColorVolume.minDisplayMasteringLuminance = sequenceControlSetPtr->staticConfig.minDisplayMasteringLuminance;

    // if dolby Profile is set HDR should be set to 1
    if (sequenceControlSetPtr->staticConfig.dolbyVisionProfile == 81) {
        sequenceControlSetPtr->staticConfig.highDynamicRangeInput = 1;
    }

    // if HDR is set videoUsabilityInfo should be set to 1
    if (sequenceControlSetPtr->staticConfig.highDynamicRangeInput == 1) {
        sequenceControlSetPtr->staticConfig.videoUsabilityInfo = 1;
    }

    // Extract frame rate from Numerator and Denominator if not 0
    if (sequenceControlSetPtr->staticConfig.frameRateNumerator != 0 && sequenceControlSetPtr->staticConfig.frameRateDenominator != 0) {
        sequenceControlSetPtr->frameRate = sequenceControlSetPtr->staticConfig.frameRate =
            (EB_U32)((double)sequenceControlSetPtr->staticConfig.frameRateNumerator/(double)sequenceControlSetPtr->staticConfig.frameRateDenominator)<<16;
    }

    // Get Default Intra Period if not specified
    if (sequenceControlSetPtr->staticConfig.intraPeriodLength == -2) {
        sequenceControlSetPtr->intraPeriodLength = sequenceControlSetPtr->staticConfig.intraPeriodLength = ComputeIntraPeriod(sequenceControlSetPtr);
    }

    if (sequenceControlSetPtr->staticConfig.lookAheadDistance == (EB_U32)~0) {
        sequenceControlSetPtr->staticConfig.lookAheadDistance = ComputeDefaultLookAhead(&sequenceControlSetPtr->staticConfig);
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
        SVT_LOG("SVT [Error]: Instance %u: Invalid  HME Total Search Area. \n", index);
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
        SVT_LOG("SVT [Error]: Instance %u: Invalid  HME Total Search Area. Must be [1 - 256].\n", index);
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
        SVT_LOG("SVT [Error]: Instance %u: Tier must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}

	// For levels below level 4 (exclusive), only the main tier is allowed
    if(config->level < 40 && config->tier != 0){
        SVT_LOG("SVT [Error]: Instance %u: For levels below level 4 (exclusive), only the main tier is allowed\n",channelNumber+1);
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
        SVT_LOG("SVT [Error]: Instance %u: Unsupported level\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

    if (sequenceControlSetPtr->maxInputLumaWidth < 64) {
        SVT_LOG("SVT [Error]: Instance %u: Source Width must be at least 64\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}
    if (sequenceControlSetPtr->maxInputLumaHeight < 64) {
        SVT_LOG("SVT [Error]: Instance %u: Source Width must be at least 64\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->predStructure > 2) {
        SVT_LOG("SVT [Error]: Instance %u: Pred Structure must be [0-2]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->baseLayerSwitchMode == 1 && config->predStructure != 2) {
        SVT_LOG("SVT [Error]: Instance %u: Base Layer Switch Mode 1 only when Prediction Structure is Random Access\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }
    if (sequenceControlSetPtr->maxInputLumaWidth % 8 && sequenceControlSetPtr->staticConfig.compressedTenBitFormat == 1) {
        SVT_LOG("SVT [Error]: Instance %u: Only multiple of 8 width is supported for compressed 10-bit inputs \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

	if (sequenceControlSetPtr->maxInputLumaWidth % 2) {
        SVT_LOG("SVT [Error]: Instance %u: Source Width must be even for YUV_420 colorspace\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

    if (sequenceControlSetPtr->maxInputLumaHeight % 2) {
        SVT_LOG("SVT [Error]: Instance %u: Source Height must be even for YUV_420 colorspace\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }
    if (sequenceControlSetPtr->maxInputLumaWidth > 8192) {
        SVT_LOG("SVT [Error]: Instance %u: Source Width must be less than 8192\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}

    if (sequenceControlSetPtr->maxInputLumaHeight > 4320) {
        SVT_LOG("SVT [Error]: Instance %u: Source Height must be less than 4320\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	EB_U32 inputSize = (EB_U32)sequenceControlSetPtr->maxInputLumaWidth * (EB_U32)sequenceControlSetPtr->maxInputLumaHeight;

	EB_U8 inputResolution = (inputSize < INPUT_SIZE_1080i_TH)	?	INPUT_SIZE_576p_RANGE_OR_LOWER :
							(inputSize < INPUT_SIZE_1080p_TH)	?	INPUT_SIZE_1080i_RANGE :
							(inputSize < INPUT_SIZE_4K_TH)		?	INPUT_SIZE_1080p_RANGE :
																	INPUT_SIZE_4K_RANGE;

    if (inputResolution <= INPUT_SIZE_1080i_RANGE) {
        if (config->encMode > 9) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - 9] for this resolution\n", channelNumber + 1);
            return_error = EB_ErrorBadParameter;
        }

    }
    else if (inputResolution == INPUT_SIZE_1080p_RANGE) {
        if (config->encMode > 10) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - 10] for this resolution\n", channelNumber + 1);
            return_error = EB_ErrorBadParameter;
        }

    }
    else {
        if (config->encMode > 12 && config->tune == 0) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - 12] for this resolution\n", channelNumber + 1);
            return_error = EB_ErrorBadParameter;
        }
        else if (config->encMode > 10 && config->tune >= 1) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - 10] for this resolution\n", channelNumber + 1);
            return_error = EB_ErrorBadParameter;
        }
    }

    // encMode
    sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES;
    if (inputResolution <= INPUT_SIZE_1080i_RANGE){
        sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_SUB1080P - 1;
        if (config->encMode > MAX_SUPPORTED_MODES_SUB1080P -1) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_SUB1080P-1);
			return_error = EB_ErrorBadParameter;
		}
	}else if (inputResolution == INPUT_SIZE_1080p_RANGE){
        sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_1080P - 1;
        if (config->encMode > MAX_SUPPORTED_MODES_1080P - 1) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_1080P - 1);
			return_error = EB_ErrorBadParameter;
		}
	}else {
        if (config->tune == 0)
            sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_4K_SQ - 1;
        else
            sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_4K_OQ - 1;

        if (config->encMode > MAX_SUPPORTED_MODES_4K_SQ - 1 && config->tune == 0) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_4K_SQ-1);
			return_error = EB_ErrorBadParameter;
        }else if (config->encMode > MAX_SUPPORTED_MODES_4K_OQ - 1 && config->tune >= 1) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_4K_OQ-1);
			return_error = EB_ErrorBadParameter;
		}
	}

	if(config->qp > 51) {
        SVT_LOG("SVT [Error]: Instance %u: QP must be [0 - 51]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}

    if (config->hierarchicalLevels > 3) {
        SVT_LOG("SVT [Error]: Instance %u: Hierarchical Levels supported [0-3]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}

    if (config->intraPeriodLength < -2 || config->intraPeriodLength > 255) {
        SVT_LOG("SVT [Error]: Instance %u: The intra period must be [-2 - 255] \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

	if( config->intraRefreshType > 2 || config->intraRefreshType < 1) {
        SVT_LOG("SVT [Error]: Instance %u: Invalid intra Refresh Type [1-2]\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
	}
    if (config->baseLayerSwitchMode > 1) {
        SVT_LOG("SVT [Error]: Instance %u: Invalid Base Layer Switch Mode [0-1] \n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->interlacedVideo > 1) {
        SVT_LOG("SVT [Error]: Instance %u: Invalid Interlaced Video\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

	if ( config->disableDlfFlag > 1) {
       SVT_LOG("SVT [Error]: Instance %u: Invalid LoopFilterDisable. LoopFilterDisable must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;
    }

	if ( config->enableSaoFlag > 1) {
       SVT_LOG("SVT [Error]: Instance %u: Invalid SAO. SAO range must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;
    }
	if ( config->useDefaultMeHme > 1 ){
       SVT_LOG("SVT [Error]: Instance %u: invalid useDefaultMeHme. useDefaultMeHme must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;
	}
    if ( config->enableHmeFlag > 1 ){
       SVT_LOG("SVT [Error]: Instance %u: invalid HME. HME must be [0 - 1]\n",channelNumber+1);
	   return_error = EB_ErrorBadParameter;
	}
	if ((config->searchAreaWidth > 256) || (config->searchAreaWidth == 0)){
        SVT_LOG("SVT [Error]: Instance %u: Invalid SearchAreaWidth. SearchAreaWidth must be [1 - 256]\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;

    }

	 if((config->searchAreaHeight > 256) || (config->searchAreaHeight == 0)) {
        SVT_LOG("SVT [Error]: Instance %u: Invalid SearchAreaHeight. SearchAreaHeight must be [1 - 256]\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;

    }

    if (levelIdx < 13) {
    // Check if the current input video is conformant with the Level constraint
    if(config->level != 0 && (((EB_U64)sequenceControlSetPtr->maxInputLumaWidth * (EB_U64)sequenceControlSetPtr->maxInputLumaHeight) > maxLumaPictureSize[levelIdx])){
        SVT_LOG("SVT [Error]: Instance %u: The input luma picture size exceeds the maximum luma picture size allowed for level %s\n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }

    // Check if the current input video is conformant with the Level constraint
    if(config->level != 0 && ((EB_U64)config->frameRate * (EB_U64)sequenceControlSetPtr->maxInputLumaWidth * (EB_U64)sequenceControlSetPtr->maxInputLumaHeight > (maxLumaSampleRate[levelIdx]<<16))){
        SVT_LOG("SVT [Error]: Instance %u: The input luma sample rate exceeds the maximum input sample rate allowed for level %s\n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }

	if ((config->level != 0) && (config->rateControlMode) && (config->tier == 0) && ((config->targetBitRate*2) > mainTierMaxBitRate[levelIdx])){
        SVT_LOG("SVT [Error]: Instance %u: Allowed MaxBitRate exceeded for level %s and tier 0 \n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
	if ((config->level != 0) && (config->rateControlMode) && (config->tier == 1) && ((config->targetBitRate*2) > highTierMaxBitRate[levelIdx])){
        SVT_LOG("SVT [Error]: Instance %u: Allowed MaxBitRate exceeded for level %s and tier 1 \n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
	if ((config->level != 0) && (config->rateControlMode) && (config->tier == 0) && ((config->targetBitRate * 3) > mainTierCPB[levelIdx])) {
        SVT_LOG("SVT [Error]: Instance %u: Out of bound maxBufferSize for level %s and tier 0 \n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
	if ((config->level != 0) && (config->rateControlMode) && (config->tier == 1) && ((config->targetBitRate * 3) > highTierCPB[levelIdx])) {
        SVT_LOG("SVT [Error]: Instance %u: Out of bound maxBufferSize for level %s and tier 1 \n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
    // Table A.6 General tier and level limits
	if ((config->level != 0) && (config->tileColumnCount > maxTileColumn[levelIdx])) {
        SVT_LOG("SVT [Error]: Instance %u: Out of bound maxTileColumn for level %s\n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
	if ((config->level != 0) && (config->tileRowCount > maxTileRow[levelIdx])) {
        SVT_LOG("SVT [Error]: Instance %u: Out of bound maxTileRow for level %s\n",channelNumber+1, levelIdc);
        return_error = EB_ErrorBadParameter;
    }
    }

	if(config->profile > 4){
        SVT_LOG("SVT [Error]: Instance %u: The maximum allowed Profile number is 4 or MAINEXT \n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

	if (config->profile == 0){
        SVT_LOG("SVT [Error]: Instance %u: The minimum allowed Profile number is 1 \n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}

	if(config->profile == 3) {
        SVT_LOG("SVT [Error]: Instance %u: The Main Still Picture Profile is not supported \n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}

    if(config->encoderColorFormat >= EB_YUV422 && config->profile != 4)
    {
        SVT_LOG("SVT [Error]: Instance %u: The input profile is not correct, should be 4 or MainREXT for YUV422 or YUV444 cases\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }

    // Check if the current input video is conformant with the Level constraint
    if(config->frameRate > (240<<16)){
        SVT_LOG("SVT [Error]: Instance %u: The maximum allowed frame rate is 240 fps\n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }
    // Check that the frameRate is non-zero
    if(config->frameRate <= 0) {
        SVT_LOG("SVT [Error]: Instance %u: The frame rate should be greater than 0 fps \n",channelNumber+1);
        return_error = EB_ErrorBadParameter;
    }
    if (config->intraPeriodLength < -2 || config->intraPeriodLength > 255 ) {
        SVT_LOG("SVT [Error]: Instance %u: The intra period must be [-2 - 255] \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }
	if (config->constrainedIntra > 1) {
        SVT_LOG("SVT [Error]: Instance %u: The constrained intra must be [0 - 1] \n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}
	if (config->rateControlMode > 1) {
        SVT_LOG("SVT [Error]: Instance %u: The rate control mode must be [0 - 1] \n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}

    if (config->tune > 0 && config->bitRateReduction == 1){
        SVT_LOG("SVT [Error]: Instance %u: Bit Rate Reduction is not supported for OQ mode (Tune = 1 ) and VMAF mode (Tune = 2)\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->tune > 0 && config->improveSharpness == 1){
        SVT_LOG("SVT [Error]: Instance %u: Improve sharpness is not supported for OQ mode (Tune = 1 ) and VMAF mode (Tune = 2)\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->lookAheadDistance > 250 && config->lookAheadDistance != (EB_U32)~0) {
        SVT_LOG("SVT [Error]: Instance %u: The lookahead distance must be [0 - 250] \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }
	if (config->sceneChangeDetection > 1) {
        SVT_LOG("SVT [Error]: Instance %u: The scene change detection must be [0 - 1] \n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}
	if ( config->maxQpAllowed > 51) {
        SVT_LOG("SVT [Error]: Instance %u: MaxQpAllowed must be [0 - 51]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}
	else if ( config->minQpAllowed > 50 ) {
        SVT_LOG("SVT [Error]: Instance %u: MinQpAllowed must be [0 - 50]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}
	else if ( (config->minQpAllowed) > (config->maxQpAllowed))  {
        SVT_LOG("SVT [Error]: Instance %u:  MinQpAllowed must be smaller than MaxQpAllowed\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
	}

	if (config->videoUsabilityInfo > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid VideoUsabilityInfo. VideoUsabilityInfo must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

    if (config->tune > 2) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid Tune. Tune must be [0 - 2]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }
	if (config->bitRateReduction > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid BitRateReduction. BitRateReduction must be [0 - 1]\n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}
    if (config->improveSharpness > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid ImproveSharpness. ImproveSharpness must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

    if (config->highDynamicRangeInput > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid HighDynamicRangeInput. HighDynamicRangeInput must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }
	if (config->accessUnitDelimiter > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid AccessUnitDelimiter. AccessUnitDelimiter must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	if (config->bufferingPeriodSEI > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid BufferingPeriod. BufferingPeriod must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	if (config->pictureTimingSEI > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid PictureTiming. PictureTiming must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	if (config->registeredUserDataSeiFlag > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid RegisteredUserData. RegisteredUserData must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	if (config->unregisteredUserDataSeiFlag > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid UnregisteredUserData. UnregisteredUserData must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

	if (config->recoveryPointSeiFlag > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid RecoveryPoint. RecoveryPoint must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

    if (config->useMasteringDisplayColorVolume > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid useMasterDisplay. useMasterDisplay must be [0 - 1]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->useNaluFile > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid useNaluFile. useNaluFile must be [0 - 1]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

	if ((config->maxCLL && !config->highDynamicRangeInput) || (config->maxFALL && !config->highDynamicRangeInput)) {
		SVT_LOG("Error Instance %u: maxCLL or maxFALL should be used only with high dynamic range input; set highDynamicRangeInput to 1\n", channelNumber);
		return_error = EB_ErrorBadParameter;
	}

	if (config->useMasteringDisplayColorVolume && !config->highDynamicRangeInput) {
		SVT_LOG("Error Instance %u: MasterDisplay should be used only with high dynamic range input; set highDynamicRangeInput to 1\n", channelNumber);
		return_error = EB_ErrorBadParameter;
	}

	if (config->dolbyVisionProfile != 0 && config->dolbyVisionProfile != 81) {
		SVT_LOG("Error Instance %u: Only Dolby Vision Profile 8.1 is supported \n", channelNumber);
		return_error = EB_ErrorBadParameter;
	}

	if (config->dolbyVisionProfile == 81 && config->encoderBitDepth != 10) {
		SVT_LOG("Error Instance %u: Dolby Vision Profile 8.1 work only with main10 input \n", channelNumber);
		return_error = EB_ErrorBadParameter;
	}

	if (config->dolbyVisionProfile == 81 && !config->useMasteringDisplayColorVolume) {
		SVT_LOG("Error Instance %u: Dolby Vision Profile 8.1 requires mastering display color volume information \n", channelNumber);
		return_error = EB_ErrorBadParameter;
	}

 	if (config->enableTemporalId > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid TemporalId. TemporalId must be [0 - 1]\n",channelNumber+1);
		return_error = EB_ErrorBadParameter;
    }

    if (config->pictureTimingSEI && !config->videoUsabilityInfo){
        SVT_LOG("SVT [Error]: Instance %u: If pictureTimingSEI is set, VideoUsabilityInfo should be also set to 1\n",channelNumber);
        return_error = EB_ErrorBadParameter;

    }
	  if ( (config->encoderBitDepth !=8 )  &&
		(config->encoderBitDepth !=10 )
		) {
          SVT_LOG("SVT [Error]: Instance %u: Encoder Bit Depth shall be only 8 or 10 \n",channelNumber+1);
			return_error = EB_ErrorBadParameter;
	}
	// Check if the EncoderBitDepth is conformant with the Profile constraint
	if(config->profile == 1 && config->encoderBitDepth == 10) {
        SVT_LOG("SVT [Error]: Instance %u: The encoder bit depth shall be equal to 8 for Main Profile\n",channelNumber+1);
			return_error = EB_ErrorBadParameter;
	}

	if (config->compressedTenBitFormat > 1)
	{
        SVT_LOG("SVT [Error]: Instance %u: Invalid Compressed Ten Bit Format shall be only [0 - 1] \n", channelNumber + 1);
		return_error = EB_ErrorBadParameter;
	}

    if (config->speedControlFlag > 1) {
        SVT_LOG("SVT [Error]: Instance %u: Invalid Speed Control flag [0 - 1]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->latencyMode > 0) {
        SVT_LOG("SVT [Error]: Instance %u: Latency Mode flag deprecated\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (((EB_S32)(config->asmType) < 0) || ((EB_S32)(config->asmType) > 1)){
        SVT_LOG("SVT [Error]: Instance %u: Invalid asm type value [0: C Only, 1: Auto] .\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->targetSocket != -1 && config->targetSocket != 0 && config->targetSocket != 1) {
        SVT_LOG("SVT [Error]: Instance %u: Invalid TargetSocket [-1 - 1] \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->switchThreadsToRtPriority > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid Switch Threads To Real Time Priority flag [0 - 1]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->fpsInVps > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid FPS in VPS flag [0 - 1]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->tileColumnCount < 1 || config->tileColumnCount > 20) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid tile column count\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->tileRowCount < 1 || config->tileRowCount > 22) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid tile row count\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->tileSliceMode > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid tile slice mode\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if ((config->tileColumnCount * config->tileRowCount) <= 1 && config->tileSliceMode) {
        SVT_LOG("SVT [Error]: Instance %u: Invalid TileSliceMode. TileSliceMode could be 1 for multi tile mode, please set it to 0 for single tile mode\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->unrestrictedMotionVector > 1) {
        SVT_LOG("SVT [Error]: Instance %u : Invalid Unrestricted Motion Vector flag [0 - 1]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    //Check tiles
    //TODO: Check maxTileCol/maxTileRow according to profile/level later
    uint32_t pictureWidthInLcu = (config->sourceWidth + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE;
    uint32_t pictureHeightInLcu = (config->sourceHeight + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE;
    for (int horizontalTileIndex = 0; horizontalTileIndex < (config->tileColumnCount - 1); ++horizontalTileIndex) {
        if (((horizontalTileIndex + 1) * pictureWidthInLcu / config->tileColumnCount -
                    horizontalTileIndex * pictureWidthInLcu / config->tileColumnCount) * MAX_LCU_SIZE < 256) { // 256 samples
            SVT_LOG("SVT [Error]: Instance %u: TileColumnCount must be chosen such that each tile has a minimum width of 256 luma samples\n", channelNumber + 1);
            return_error = EB_ErrorBadParameter;
            break;
        }
    }
    for (int verticalTileIndex = 0; verticalTileIndex < (config->tileRowCount - 1); ++verticalTileIndex) {
        if (((verticalTileIndex + 1) * pictureHeightInLcu / config->tileRowCount -
                    verticalTileIndex * pictureHeightInLcu / config->tileRowCount) * MAX_LCU_SIZE < 64) { // 64 samples
            SVT_LOG("SVT [Error]: Instance %u: TileRowCount must be chosen such that each tile has a minimum height of 64 luma samples\n", channelNumber + 1);
            return_error = EB_ErrorBadParameter;
            break;
        }
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
        SVT_LOG("The EB_H265_ENC_CONFIGURATION structure is empty! \n");
        return EB_ErrorBadParameter;
    }

    configPtr->frameRate = 60 << 16;
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
    configPtr->tileRowCount = 1;
    configPtr->tileColumnCount = 1;
    configPtr->tileSliceMode = 0;
    configPtr->sceneChangeDetection = 1;
    configPtr->rateControlMode = 0;
    configPtr->lookAheadDistance = (EB_U32)~0;
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
    configPtr->searchAreaWidth = 16;
    configPtr->searchAreaHeight = 7;
    configPtr->constrainedIntra = EB_FALSE;
    configPtr->tune = 1;
    configPtr->bitRateReduction = EB_TRUE;
    configPtr->improveSharpness = EB_TRUE;

    // Bitstream options
    configPtr->codeVpsSpsPps = 0;
    configPtr->codeEosNal    = 0;
    configPtr->switchThreadsToRtPriority = EB_TRUE;
    configPtr->fpsInVps      = EB_TRUE;
    configPtr->unrestrictedMotionVector = EB_TRUE;

    configPtr->videoUsabilityInfo = 0;
    configPtr->highDynamicRangeInput = 0;
    configPtr->accessUnitDelimiter = 0;
    configPtr->bufferingPeriodSEI = 0;
    configPtr->pictureTimingSEI = 0;
    configPtr->registeredUserDataSeiFlag = EB_FALSE;
    configPtr->unregisteredUserDataSeiFlag = EB_FALSE;
    configPtr->recoveryPointSeiFlag = EB_FALSE;
    configPtr->enableTemporalId = 1;

    // Annex A parameters
    configPtr->profile = 2;
    configPtr->tier = 0;
    configPtr->level = 0;

    // Latency
    configPtr->injectorFrameRate = 60 << 16;
    configPtr->speedControlFlag = 0;
    configPtr->latencyMode = 0;

    // ASM Type
    configPtr->asmType = 1;


    // Channel info
    configPtr->logicalProcessors = 0;
    configPtr->targetSocket = -1;
    configPtr->channelId = 0;
    configPtr->activeChannelCount   = 1;

    //SEI
    configPtr->maxCLL = 0;
    configPtr->maxFALL = 0;
    configPtr->dolbyVisionProfile = 0;
    configPtr->useMasteringDisplayColorVolume = EB_FALSE;
    configPtr->useNaluFile = EB_FALSE;

    // Master Display
    configPtr->displayPrimaryX[0] = 0;
    configPtr->displayPrimaryX[1] = 0;
    configPtr->displayPrimaryX[2] = 0;
    configPtr->displayPrimaryY[0] = 0;
    configPtr->displayPrimaryY[1] = 0;
    configPtr->displayPrimaryY[2] = 0;
    configPtr->whitePointX = 0;
    configPtr->whitePointY = 0;
    configPtr->maxDisplayMasteringLuminance = 0;
    configPtr->minDisplayMasteringLuminance = 0;

    // Debug info
    configPtr->reconEnabled = 0;

    return return_error;
}
static void PrintLibParams(
    EB_H265_ENC_CONFIGURATION*   config)
{
    SVT_LOG("------------------------------------------- ");
    if (config->profile == 1)
        SVT_LOG("\nSVT [config]: Main Profile\t");
    else if (config->profile == 2)
        SVT_LOG("\nSVT [config]: Main10 Profile\t");
    else
        SVT_LOG("\nSVT [config]: MainEXT Profile\t");

    if (config->tier != 0 && config->level !=0)
        SVT_LOG("Tier %d\tLevel %.1f\t", config->tier, (float)(config->level / 10));
    else {
        if (config->tier == 0 )
            SVT_LOG("Tier (auto)\t");
        else
            SVT_LOG("Tier %d\t", config->tier);

        if (config->level == 0 )
            SVT_LOG("Level (auto)\t");
        else
            SVT_LOG("Level %.1f\t", (float)(config->level / 10));


    }

    SVT_LOG("\nSVT [config]: EncoderMode / Tune\t\t\t\t\t: %d / %d ", config->encMode, config->tune);
    SVT_LOG("\nSVT [config]: EncoderBitDepth / CompressedTenBitFormat / EncoderColorFormat \t: %d / %d / %d", config->encoderBitDepth, config->compressedTenBitFormat, config->encoderColorFormat);
    SVT_LOG("\nSVT [config]: SourceWidth / SourceHeight\t\t\t\t\t: %d / %d ", config->sourceWidth, config->sourceHeight);
    if (config->frameRateDenominator != 0 && config->frameRateNumerator != 0)
        SVT_LOG("\nSVT [config]: Fps_Numerator / Fps_Denominator / Gop Size / IntraRefreshType \t: %d / %d / %d / %d", config->frameRateNumerator > (1<<16) ? config->frameRateNumerator >> 16: config->frameRateNumerator,
            config->frameRateDenominator > (1<<16) ? config->frameRateDenominator >> 16: config->frameRateDenominator,
            config->intraPeriodLength + 1,
            config->intraRefreshType);
    else
        SVT_LOG("\nSVT [config]: FrameRate / Gop Size\t\t\t\t\t\t: %d / %d ", config->frameRate > 1000 ? config->frameRate >> 16 : config->frameRate, config->intraPeriodLength + 1);
    SVT_LOG("\nSVT [config]: HierarchicalLevels / BaseLayerSwitchMode / PredStructure\t\t: %d / %d / %d ", config->hierarchicalLevels, config->baseLayerSwitchMode, config->predStructure);
    if (config->rateControlMode == 1)
        SVT_LOG("\nSVT [config]: RCMode / TargetBitrate / LookaheadDistance / SceneChange\t\t: VBR / %d / %d / %d ", config->targetBitRate, config->lookAheadDistance, config->sceneChangeDetection);
    else
        SVT_LOG("\nSVT [config]: BRC Mode / QP  / LookaheadDistance / SceneChange\t\t\t: CQP / %d / %d / %d ", config->qp, config->lookAheadDistance, config->sceneChangeDetection);

    if (config->tune == 0)
        SVT_LOG("\nSVT [config]: BitRateReduction / ImproveSharpness\t\t\t\t: %d / %d ", config->bitRateReduction, config->improveSharpness);
    SVT_LOG("\n------------------------------------------- ");
    SVT_LOG("\n");

    fflush(stdout);
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
    EB_ERRORTYPE            return_error      = EB_ErrorNone;
    EB_U32                  instanceIndex     = 0;
    EbEncHandle_t          *pEncCompData;

    if (h265EncComponent == (EB_COMPONENTTYPE*)EB_NULL) {
        return EB_ErrorBadParameter;
    }

    pEncCompData = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;

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

    PrintLibParams(
        &pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig);

    // Release Config Mutex
    EbReleaseMutex(pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->configMutex);

    return return_error;
}
#if __linux
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265EncStreamHeader(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE        **outputStreamPtr
)
{
    EB_ERRORTYPE           return_error = EB_ErrorNone;
    EbEncHandle_t          *pEncCompData = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    Bitstream_t            *bitstreamPtr;
    SequenceControlSet_t   *sequenceControlSetPtr = pEncCompData->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr;
    EncodeContext_t        *encodeContextPtr = sequenceControlSetPtr->encodeContextPtr;
    EbPPSConfig_t          *ppsConfig;
    EB_BUFFERHEADERTYPE    *outputStreamBuffer;

    // Output buffer Allocation
    EB_MALLOC(EB_BUFFERHEADERTYPE*, outputStreamBuffer, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR);
    EB_MALLOC(EB_U8*, outputStreamBuffer->pBuffer, sizeof(EB_U8) * PACKETIZATION_PROCESS_BUFFER_SIZE, EB_N_PTR);
    outputStreamBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);
    outputStreamBuffer->nAllocLen = PACKETIZATION_PROCESS_BUFFER_SIZE;
    outputStreamBuffer->pAppPrivate = NULL;
    outputStreamBuffer->sliceType = EB_INVALID_PICTURE;
    outputStreamBuffer->nFilledLen = 0;

    // Intermediate buffers
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
            EB_I_PICTURE,
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
        outputStreamBuffer->pBuffer,
        (EB_U32*) &(outputStreamBuffer->nFilledLen),
        (EB_U32*) &(outputStreamBuffer->nAllocLen),
        encodeContextPtr,
		NAL_UNIT_INVALID);

    *outputStreamPtr = outputStreamBuffer;

    return return_error;
}

#if __linux
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265EncEosNal(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE       **outputStreamPtr
)
{
    EB_ERRORTYPE           return_error = EB_ErrorNone;
    EbEncHandle_t          *pEncCompData = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    Bitstream_t            *bitstreamPtr;
    SequenceControlSet_t  *sequenceControlSetPtr = pEncCompData->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr;
    EncodeContext_t        *encodeContextPtr = sequenceControlSetPtr->encodeContextPtr;
    EB_BUFFERHEADERTYPE    *outputStreamBuffer;

    // Output buffer Allocation
    EB_MALLOC(EB_BUFFERHEADERTYPE*, outputStreamBuffer, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR);
    EB_MALLOC(EB_U8*, outputStreamBuffer->pBuffer, sizeof(EB_U8) * PACKETIZATION_PROCESS_BUFFER_SIZE, EB_N_PTR);
    outputStreamBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);
    outputStreamBuffer->nAllocLen = PACKETIZATION_PROCESS_BUFFER_SIZE;
    outputStreamBuffer->pAppPrivate = NULL;
    outputStreamBuffer->sliceType = EB_INVALID_PICTURE;
    outputStreamBuffer->nFilledLen = 0;

    //
    EB_MALLOC(Bitstream_t*, bitstreamPtr, sizeof(Bitstream_t), EB_N_PTR);
    EB_MALLOC(OutputBitstreamUnit_t*, bitstreamPtr->outputBitstreamPtr, sizeof(OutputBitstreamUnit_t), EB_N_PTR);

    return_error = OutputBitstreamUnitCtor(
        (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr,
        EOS_NAL_BUFFER_SIZE);

    // Reset the bitstream before writing to it
    ResetBitstream(
        (OutputBitstreamUnit_t*)bitstreamPtr->outputBitstreamPtr);

    CodeEndOfSequenceNalUnit(bitstreamPtr);

    // Flush the Bitstream
    FlushBitstream(
        bitstreamPtr->outputBitstreamPtr);

    // Copy SPS & PPS to the Output Bitstream
    CopyRbspBitstreamToPayload(
        bitstreamPtr,
        outputStreamBuffer->pBuffer,
        (EB_U32*) &(outputStreamBuffer->nFilledLen),
        (EB_U32*) &(outputStreamBuffer->nAllocLen),
        encodeContextPtr,
		NAL_UNIT_INVALID);

    *outputStreamPtr = outputStreamBuffer;

    return return_error;
}

/* charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" */
static EB_ERRORTYPE BaseDecodeFunction(EB_U8* encodedString, EB_U32 base64EncodeLength, EB_U8* decodedString )
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_U32 i, j, k = 0;
    EB_U32 bitstream = 0;
    EB_U32 countBits = 0;
    if (encodedString == NULL || decodedString == NULL) {
        return_error = EB_ErrorBadParameter;
    }
    if (return_error == EB_ErrorNone) {
        // selects 4 characters from encodedString at a time
        for (i = 0; i < base64EncodeLength; i += 4) {
            bitstream = 0, countBits = 0;
            for (j = 0; j < 4; j++) {
                // make space for 6 bits
                if (encodedString[i + j] != '=') {
                    bitstream = bitstream << 6;
                    countBits += 6;
                }
                // Finding the position of each encoded character in charSet and storing in bitstream
                if (encodedString[i + j] >= 'A' && encodedString[i + j] <= 'Z')
                    bitstream = bitstream | (encodedString[i + j] - 'A');

                else if (encodedString[i + j] >= 'a' && encodedString[i + j] <= 'z')
                    bitstream = bitstream | (encodedString[i + j] - 'a' + 26);

                else if (encodedString[i + j] >= '0' && encodedString[i + j] <= '9')
                    bitstream = bitstream | (encodedString[i + j] - '0' + 52);

                // '+' occurs in 62nd position in charSet
                else if (encodedString[i + j] == '+')
                    bitstream = bitstream | 62;

                // '/' occurs in 63rd position in charSet
                else if (encodedString[i + j] == '/')
                    bitstream = bitstream | 63;

                else {
                    bitstream = bitstream >> 2;
                    countBits -= 2;
                }
            }

            while (countBits != 0) {
                countBits -= 8;
                if (k >= sizeof(decodedString)) {
                    return EB_ErrorBadParameter;
                }
                decodedString[k++] = (bitstream >> countBits) & 255;
            }
        }
    }
    return return_error;
}

static EB_ERRORTYPE ParseSeiMetaData(
    EB_BUFFERHEADERTYPE         *dst,
    EB_BUFFERHEADERTYPE         *src)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    EbPictureBufferDesc_t *headerPtr = (EbPictureBufferDesc_t*)dst->pBuffer;
    EB_U8 *base64Encode;
    EB_U32 base64EncodeLength;
    EB_U8 *base64Decode;

    if (src->naluFound == EB_FALSE) {
        return EB_ErrorBadParameter;
    }

    base64Encode = src->naluBase64Encode;
    base64EncodeLength = (uint32_t)strlen((char*)base64Encode);
    EB_MALLOC(EB_U8*, base64Decode, (base64EncodeLength / 4) * 3, EB_N_PTR);
    return_error = BaseDecodeFunction(base64Encode, base64EncodeLength, base64Decode);

    if (return_error != EB_ErrorNone) {
        src->naluFound = EB_FALSE;
        SVT_LOG("\nSVT [WARNING]: SEI encoded message cannot be decoded \n ");
        return EB_ErrorBadParameter;
    }

    if (src->naluNalType == NAL_UNIT_PREFIX_SEI && src->naluPrefix == 0) {
        EB_U64 currentPOC = src->pts;
        if (currentPOC == src->naluPOC) {
            headerPtr->userSeiMsg.payloadSize = (base64EncodeLength / 4) * 3;
            EB_MALLOC(EB_U8*, headerPtr->userSeiMsg.payload, headerPtr->userSeiMsg.payloadSize, EB_N_PTR);
            if (src->naluPayloadType == 4)
                headerPtr->userSeiMsg.payloadType = USER_DATA_REGISTERED_ITU_T_T35;
            else if (src->naluPayloadType == 5)
                headerPtr->userSeiMsg.payloadType = USER_DATA_UNREGISTERED;
            else {
                src->naluFound = EB_FALSE;
                SVT_LOG("\nSVT [WARNING]: Unsupported SEI payload Type for frame %u\n ", src->naluPOC);
                return EB_ErrorBadParameter;
            }
            EB_MEMCPY(headerPtr->userSeiMsg.payload, base64Decode, headerPtr->userSeiMsg.payloadSize);
        }
        else {
            src->naluFound = EB_FALSE;
            SVT_LOG("\nSVT [WARNING]: User SEI frame number %u doesn't match input frame number %" PRId64 "\n ", src->naluPOC, currentPOC);
            return EB_ErrorBadParameter;
        }
    }
    else {
        src->naluFound = EB_FALSE;
        SVT_LOG("\nSVT [WARNING]: SEI message for frame %u is not inserted. Will support only PREFIX SEI message \n ", src->naluPOC);
        return EB_ErrorBadParameter;
    }
    return return_error;
}

static EB_ERRORTYPE CopyUserSei(
    SequenceControlSet_t*    sequenceControlSetPtr,
    EB_BUFFERHEADERTYPE*     dst,
    EB_BUFFERHEADERTYPE*     src)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_H265_ENC_CONFIGURATION   *config = &sequenceControlSetPtr->staticConfig;
    EbPictureBufferDesc_t       *dstPicturePtr = (EbPictureBufferDesc_t*)dst->pBuffer;

    if (config->useNaluFile == EB_TRUE && src->naluFound == EB_FALSE) {
        config->useNaluFile = EB_FALSE;
    }

    // Copy User SEI metadata from input
    if (config->useNaluFile) {
        return_error = ParseSeiMetaData(dst, src);
    }
    else {
        dstPicturePtr->userSeiMsg.payloadSize = 0;
        dstPicturePtr->userSeiMsg.payload = NULL;
    }
    return return_error;
}

/***********************************************
**** Copy the input buffer from the
**** sample application to the library buffers
************************************************/
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

    EB_U16                           colorFormat = (EB_U16)(config->encoderColorFormat);
    EB_U16                           subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16                           subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
    // Need to include for Interlacing on the fly with pictureScanType = 1

    if (!is16BitInput) {

        EB_U32                           lumaBufferOffset = inputPicturePtr->strideY*sequenceControlSetPtr->topPadding + sequenceControlSetPtr->leftPadding;
        EB_U32                           chromaBufferOffset = inputPicturePtr->strideCr*(sequenceControlSetPtr->topPadding >> subHeightCMinus1) + (sequenceControlSetPtr->leftPadding >> subWidthCMinus1);
        EB_U16                           lumaStride = inputPicturePtr->strideY;
        EB_U16                           chromaStride = inputPicturePtr->strideCb;
        EB_U16                           lumaWidth = (EB_U16)(inputPicturePtr->width - sequenceControlSetPtr->maxInputPadRight);
        EB_U16                           chromaWidth = lumaWidth >> subWidthCMinus1;
        EB_U16                           lumaHeight = (EB_U16)(inputPicturePtr->height - sequenceControlSetPtr->maxInputPadBottom);
        EB_U16                           chromaHeight = lumaHeight >> subHeightCMinus1;

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
        for (inputRowIndex = 0; inputRowIndex < chromaHeight; inputRowIndex++) {
            EB_MEMCPY((inputPicturePtr->bufferCb + chromaBufferOffset + chromaStride * inputRowIndex),
                (inputPtr->cb + (sourceCbStride*inputRowIndex)),
                chromaWidth);
        }

        // V
        for (inputRowIndex = 0; inputRowIndex < chromaHeight; inputRowIndex++) {
            EB_MEMCPY((inputPicturePtr->bufferCr + chromaBufferOffset + chromaStride * inputRowIndex),
                (inputPtr->cr + (sourceCrStride*inputRowIndex)),
                chromaWidth);
        }

    }
    else if (is16BitInput && config->compressedTenBitFormat == 1)
    {
        {
            EB_U32  lumaBufferOffset = (inputPicturePtr->strideY*sequenceControlSetPtr->topPadding + sequenceControlSetPtr->leftPadding);
            EB_U32  chromaBufferOffset = (inputPicturePtr->strideCr*(sequenceControlSetPtr->topPadding >> subHeightCMinus1) + (sequenceControlSetPtr->leftPadding >> subWidthCMinus1));
            EB_U16  lumaStride = inputPicturePtr->strideY;
            EB_U16  chromaStride = inputPicturePtr->strideCb;
            EB_U16  lumaWidth = (EB_U16)(inputPicturePtr->width - sequenceControlSetPtr->maxInputPadRight);
            EB_U16  chromaWidth = (lumaWidth >> subWidthCMinus1);
            EB_U16  lumaHeight = (EB_U16)(inputPicturePtr->height - sequenceControlSetPtr->maxInputPadBottom);
            EB_U16  chromaHeight = lumaHeight >> subHeightCMinus1;

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
            for (inputRowIndex = 0; inputRowIndex < chromaHeight; inputRowIndex++) {

                EB_MEMCPY((inputPicturePtr->bufferCb + chromaBufferOffset + chromaStride * inputRowIndex),
                    (inputPtr->cb + (sourceCbStride*inputRowIndex)),
                    chromaWidth);

            }

            // V 8bit
            for (inputRowIndex = 0; inputRowIndex < chromaHeight; inputRowIndex++) {

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
                for (inputRowIndex = 0; inputRowIndex < chromaHeight; inputRowIndex++) {
                    EB_MEMCPY(inputPicturePtr->bufferBitIncCb + (luma2BitWidth >> 1)*inputRowIndex, inputPtr->cbExt + sourceChroma2BitStride * inputRowIndex, luma2BitWidth >> 1);
                }
                for (inputRowIndex = 0; inputRowIndex < chromaHeight; inputRowIndex++) {
                    EB_MEMCPY(inputPicturePtr->bufferBitIncCr + (luma2BitWidth >> 1)*inputRowIndex, inputPtr->crExt + sourceChroma2BitStride * inputRowIndex, luma2BitWidth >> 1);
                }
            }

        }

    }
    else { // 10bit packed

        EB_U32 lumaOffset = 0, chromaOffset = 0;
        EB_U32 lumaBufferOffset = (inputPicturePtr->strideY*sequenceControlSetPtr->topPadding + sequenceControlSetPtr->leftPadding);
        EB_U32 chromaBufferOffset = (inputPicturePtr->strideCr*(sequenceControlSetPtr->topPadding >> subHeightCMinus1) + (sequenceControlSetPtr->leftPadding >> subWidthCMinus1));
        EB_U16 lumaWidth = (EB_U16)(inputPicturePtr->width - sequenceControlSetPtr->maxInputPadRight);
        EB_U16 chromaWidth = (lumaWidth >> subWidthCMinus1);
        EB_U16 lumaHeight = (EB_U16)(inputPicturePtr->height - sequenceControlSetPtr->maxInputPadBottom);
        EB_U16 chromaHeight = lumaHeight >> subHeightCMinus1;

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
            chromaHeight);

        UnPack2D(
            (EB_U16*)(inputPtr->cr + chromaOffset),
            sourceCrStride,
            inputPicturePtr->bufferCr + chromaBufferOffset,
            inputPicturePtr->strideCr,
            inputPicturePtr->bufferBitIncCr + chromaBufferOffset,
            inputPicturePtr->strideBitIncCr,
            chromaWidth,
            chromaHeight);
    }

    // Copy Dolby Vision RPU metadata from input
    if (inputPtr->dolbyVisionRpu.payloadSize) {
        inputPicturePtr->dolbyVisionRpu.payloadSize = inputPtr->dolbyVisionRpu.payloadSize;
        EB_MALLOC(EB_U8*, inputPicturePtr->dolbyVisionRpu.payload, inputPtr->dolbyVisionRpu.payloadSize, EB_N_PTR);
        EB_MEMCPY(inputPicturePtr->dolbyVisionRpu.payload, inputPtr->dolbyVisionRpu.payload, inputPtr->dolbyVisionRpu.payloadSize);
    }
    else {
        inputPicturePtr->dolbyVisionRpu.payloadSize = 0;
        inputPicturePtr->dolbyVisionRpu.payload = NULL;
    }

    return return_error;
}
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
    dst->nTickCount = src->nTickCount;
    dst->nSize      = src->nSize;
    dst->qpValue    = src->qpValue;
    dst->sliceType  = src->sliceType;

    // Copy the picture buffer
    if(src->pBuffer != NULL)
        CopyFrameBuffer(sequenceControlSet, dst->pBuffer, src->pBuffer);

    // Copy User SEI
    if (src->pBuffer != NULL)
        CopyUserSei(sequenceControlSet, dst, src);

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
    dst->pAppPrivate   = src->pAppPrivate;
    dst->nTickCount    = src->nTickCount;
    dst->pts           = src->pts;
    dst->dts           = src->dts;
    dst->nFlags        = src->nFlags;
    dst->sliceType     = src->sliceType;
    if (src->pBuffer)
        EB_MEMCPY(dst->pBuffer, src->pBuffer, src->nFilledLen);
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
    EB_BUFFERHEADERTYPE  **pBuffer,
    unsigned char          picSendDone)
{
    EB_ERRORTYPE           return_error = EB_ErrorNone;
    EbEncHandle_t          *pEncCompData = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    EbObjectWrapper_t      *ebWrapperPtr = NULL;
    EB_BUFFERHEADERTYPE    *packet;
    if (picSendDone)
        EbGetFullObject(
        (pEncCompData->outputStreamBufferConsumerFifoPtrDblArray[0])[0],
            &ebWrapperPtr);
    else
        EbGetFullObjectNonBlocking(
        (pEncCompData->outputStreamBufferConsumerFifoPtrDblArray[0])[0],
            &ebWrapperPtr);

    if (ebWrapperPtr) {

        packet = (EB_BUFFERHEADERTYPE*)ebWrapperPtr->objectPtr;

        if (packet->nFlags != EB_BUFFERFLAG_EOS && packet->nFlags != 0) {
            return_error = EB_ErrorMax;
        }

        // return the output stream buffer
        *pBuffer = packet;

        // save the wrapper pointer for the release
        (*pBuffer)->wrapperPtr = (void*)ebWrapperPtr;
    }
    else {
        return_error = EB_NoErrorEmptyQueue;
    }

    return return_error;
}

#if __linux
__attribute__((visibility("default")))
#endif
EB_API void EbH265ReleaseOutBuffer(
    EB_BUFFERHEADERTYPE  **pBuffer)
{
    if ((*pBuffer)->wrapperPtr)
        // Release out put buffer back into the pool
        EbReleaseObject((EbObjectWrapper_t  *)(*pBuffer)->wrapperPtr);

    return;
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

    printf("SVT [version]:\tSVT-HEVC Encoder Lib v%d.%d.%d\n", SVT_VERSION_MAJOR, SVT_VERSION_MINOR,SVT_VERSION_PATCHLEVEL);
#if ( defined( _MSC_VER ) && (_MSC_VER < 1910) )
    printf("SVT [build]  : Visual Studio 2013");
#elif ( defined( _MSC_VER ) && (_MSC_VER >= 1910) )
    printf("SVT [build]  :\tVisual Studio 2017");
#elif defined(__GNUC__)
    printf("SVT [build]  :\tGCC %s\t", __VERSION__);
#else
    printf("SVT [build]  :\tunknown compiler");
#endif
    printf(" %u bit\n", (unsigned) sizeof(void*) * 8);
    printf("LIB Build date: %s %s\n",__DATE__,__TIME__);
    printf("-------------------------------------------\n");

    // Set Component Size & Version
    h265EncComponent->nSize                     = sizeof(EB_COMPONENTTYPE);

    // Encoder Private Handle Ctor
    return_error = (EB_ERRORTYPE) EbEncHandleCtor(
        (EbEncHandle_t**) &(h265EncComponent->pComponentPrivate),
        h265EncComponent);

    return return_error;
}
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
    inputPictureBufferDescInitData.colorFormat = (EB_COLOR_FORMAT)config->encoderColorFormat;

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


/**************************************
* EB_BUFFERHEADERTYPE Constructor
**************************************/
EB_ERRORTYPE EbInputBufferHeaderCtor(
    EB_PTR *objectDblPtr,
    EB_PTR  objectInitDataPtr)
{
    EB_BUFFERHEADERTYPE* inputBuffer;
    SequenceControlSet_t        *sequenceControlSetPtr = (SequenceControlSet_t*)objectInitDataPtr;
    EB_MALLOC(EB_BUFFERHEADERTYPE*, inputBuffer, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR);
    *objectDblPtr = (EB_PTR)inputBuffer;
    // Initialize Header
    inputBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);

    AllocateFrameBuffer(
        sequenceControlSetPtr,
        inputBuffer);

    inputBuffer->pAppPrivate = NULL;

    return EB_ErrorNone;
}

/**************************************
 * EB_BUFFERHEADERTYPE Constructor
 **************************************/
EB_ERRORTYPE EbOutputBufferHeaderCtor(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    EB_H265_ENC_CONFIGURATION   * config = (EB_H265_ENC_CONFIGURATION*)objectInitDataPtr;
    EB_U32 nStride = (EB_U32)(EB_OUTPUTSTREAMBUFFERSIZE_MACRO(config->sourceWidth * config->sourceHeight));  //TBC
	EB_BUFFERHEADERTYPE* outBufPtr;

	EB_MALLOC(EB_BUFFERHEADERTYPE*, outBufPtr, sizeof(EB_BUFFERHEADERTYPE), EB_N_PTR);
	*objectDblPtr = (EB_PTR)outBufPtr;

	// Initialize Header
	outBufPtr->nSize = sizeof(EB_BUFFERHEADERTYPE);

	EB_MALLOC(EB_U8*, outBufPtr->pBuffer, nStride, EB_N_PTR);

	outBufPtr->nAllocLen =  nStride;
	outBufPtr->pAppPrivate = NULL;

	    (void)objectInitDataPtr;

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
    const EB_COLOR_FORMAT colorFormat = (EB_COLOR_FORMAT)sequenceControlSetPtr->chromaFormatIdc;
    const EB_U32 lumaSize = sequenceControlSetPtr->lumaWidth * sequenceControlSetPtr->lumaHeight;
    // both u and v
    const EB_U32 chromaSize = lumaSize >> (3 - colorFormat);
    const EB_U32 tenBit = (sequenceControlSetPtr->staticConfig.encoderBitDepth > 8);
    const EB_U32 frameSize = (lumaSize + 2 * chromaSize) << tenBit;

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
