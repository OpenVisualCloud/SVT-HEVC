/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


// SUMMARY
//   Contains the API component functions

/**************************************
 * Includes
 **************************************/
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "EbDefinitions.h"
#include "EbApi.h"
#include "EbThreads.h"
#include "EbUtility.h"
#include "EbString.h"
#include "EbEncHandle.h"

#include "EbPictureControlSet.h"
#include "EbReferenceObject.h"

#include "EbPictureAnalysisResults.h"
#include "EbPictureDecisionResults.h"
#include "EbMotionEstimationResults.h"
#include "EbInitialRateControlResults.h"
#include "EbRateControlTasks.h"
#include "EbEncDecTasks.h"
#include "EbEncDecResults.h"
#include "EbEntropyCodingResults.h"
#include "EbPredictionStructure.h"
#include "EbBitstreamUnit.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
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
#define EB_OUTPUTBUFFERCOUNT                            5000
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
EB_U32                          nbLumaThreads = 1;
EB_U32                          nbChromaThreads = 1;

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
#define INITIAL_PROCESSOR_GROUP 16
processorGroup                  *lpGroup = EB_NULL;
#endif

/**************************************
* Instruction Set Support
**************************************/
#include <stdint.h>
#ifdef _WIN32
# include <intrin.h>
#endif
void RunCpuid(EB_U32 eax, EB_U32 ecx, int* abcd)
{
#ifdef _WIN32
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
#ifdef _WIN32
    xcr0 = (uint32_t)_xgetbv(0);  /* min VS2010 SP1 compiler is required */
#else
    __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
#endif
    return ((xcr0 & 6) == 6); /* checking if xmm and ymm state are enabled in XCR0 */
}
static EB_S32 Check4thGenIntelCoreFeatures()
{
    int abcd[4];
#define ECX_REG_FMA     BIT(12)
#define ECX_REG_MOVBE   BIT(22)
#define ECX_REG_XSAVE   BIT(26)
#define ECX_REG_OSXSAVE BIT(27)
    int ecx_reg_mask = ECX_REG_FMA | ECX_REG_MOVBE | ECX_REG_XSAVE | ECX_REG_OSXSAVE;
    int avx2_bmi12_mask = (1 << 5) | (1 << 3) | (1 << 8);

    /* CPUID.(EAX=01H, ECX=0H):ECX.FMA[bit 12]==1   &&
       CPUID.(EAX=01H, ECX=0H):ECX.MOVBE[bit 22]==1 &&
       CPUID.(EAX=01H, ECX=0H):ECX.XSAVE[bit 26]==1 &&
       CPUID.(EAX=01H, ECX=0H):ECX.OSXSAVE[bit 27]==1 */
    RunCpuid( 1, 0, abcd );
    /* To make sure the processor supports XGETBV instruction, and OS has enabled it. */
    if ((abcd[2] & ecx_reg_mask) != ecx_reg_mask)
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
#ifdef _WIN32
    xcr0 = (uint32_t)_xgetbv(0);  /* min VS2010 SP1 compiler is required */
#else
    __asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx" );
#endif
    return ((xcr0 & zmm_ymm_xmm) == zmm_ymm_xmm); /* check if xmm, ymm and zmm state are enabled in XCR0 */
}

static EB_S32 CanUseIntelAVX512()
{
    int abcd[4];
    int avx512_ebx_mask = 0;

#define ECX_REG_FMA     BIT(12)
#define ECX_REG_MOVBE   BIT(22)
#define ECX_REG_XSAVE   BIT(26)
#define ECX_REG_OSXSAVE BIT(27)
    int ecx_reg_mask = ECX_REG_FMA | ECX_REG_MOVBE | ECX_REG_XSAVE | ECX_REG_OSXSAVE;

    /* CPUID.(EAX=01H, ECX=0H):ECX.FMA[bit 12]==1   &&
       CPUID.(EAX=01H, ECX=0H):ECX.MOVBE[bit 22]==1 &&
       CPUID.(EAX=01H, ECX=0H):ECX.XSAVE[bit 26]==1 &&
       CPUID.(EAX=01H, ECX=0H):ECX.OSXSAVE[bit 27]==1 */
    RunCpuid( 1, 0, abcd );
    /* To make sure the processor supports XGETBV instruction, and OS has enabled it. */
    if ((abcd[2] & ecx_reg_mask) != ecx_reg_mask)
        return 0;

    // ensure OS supports ZMM registers (and YMM, and XMM)
    if ( ! CheckXcr0Zmm() )
        return 0;

    if ( ! Check4thGenIntelCoreFeatures() )
        return 0;

    /* CPUID.(EAX=07H, ECX=0):EBX[bit 16]==1 AVX512F
       CPUID.(EAX=07H, ECX=0):EBX[bit 17] AVX512DQ
       CPUID.(EAX=07H, ECX=0):EBX[bit 28] AVX512CD
       CPUID.(EAX=07H, ECX=0):EBX[bit 30] AVX512BW
       CPUID.(EAX=07H, ECX=0):EBX[bit 31] AVX512VL */

    avx512_ebx_mask =
        (1 << 16)  // AVX-512F
        | (1 << 17)  // AVX-512DQ
        | (1 << 28)  // AVX-512CD
        | (1 << 30)  // AVX-512BW
        | (1 << 31); // AVX-512VL

    RunCpuid( 7, 0, abcd );
    if ( (abcd[1] & avx512_ebx_mask) != avx512_ebx_mask )
        return 0;

    return 1;
}


// Returns ASM Type based on system configuration. AVX512 - 111, AVX2 - 011, NONAVX2 - 001, C - 000
// Using bit-fields, the fastest function will always be selected based on the available functions in the function arrays
EB_U32 EbHevcGetCpuAsmType()
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
EB_U32 EbHevcGetNumProcessors() {
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

EB_ERRORTYPE EbHevcInitThreadManagmentParams(){
#ifdef _WIN32
    // Initialize groupAffinity structure with Current thread info
    GetThreadGroupAffinity(GetCurrentThread(),&groupAffinity);
    numGroups = (EB_U8) GetActiveProcessorGroupCount();
#elif defined(__linux__)
    const char* PROCESSORID = "processor";
    const char* PHYSICALID = "physical id";
    int processor_id_len = EB_STRLEN(PROCESSORID, 128);
    int physical_id_len = EB_STRLEN(PHYSICALID, 128);
    int maxSize = INITIAL_PROCESSOR_GROUP;
    if (processor_id_len < 0 || processor_id_len >= 128) return EB_ErrorInsufficientResources;
    if (physical_id_len < 0 || physical_id_len >= 128) return EB_ErrorInsufficientResources;
    memset(lpGroup, 0, INITIAL_PROCESSOR_GROUP * sizeof(processorGroup));

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
                if (socket_id < 0) {
                    fclose(fin);
                    return EB_ErrorInsufficientResources;
                }
                if (socket_id + 1 > numGroups)
                    numGroups = socket_id + 1;
                if (socket_id >= maxSize) {
                    maxSize = maxSize * 2;
                    lpGroup = (processorGroup*)realloc(lpGroup,maxSize * sizeof(processorGroup));
                    if (lpGroup == (processorGroup*) EB_NULL) {
                        fclose(fin);
                        return EB_ErrorInsufficientResources;
                    }
                }
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

static void EBEncHandleStopThreads(EbEncHandle_t *encHandlePtr)
{
    SequenceControlSet_t*  controlSetPtr = encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr;
    // Resource Coordination
    EB_DESTROY_THREAD(encHandlePtr->resourceCoordinationThreadHandle);
    EB_DESTROY_THREAD_ARRAY(encHandlePtr->pictureAnalysisThreadHandleArray, controlSetPtr->pictureAnalysisProcessInitCount);

    // Picture Decision
    EB_DESTROY_THREAD(encHandlePtr->pictureDecisionThreadHandle);

    // Motion Estimation
    EB_DESTROY_THREAD_ARRAY(encHandlePtr->motionEstimationThreadHandleArray, controlSetPtr->motionEstimationProcessInitCount);

    // Initial Rate Control
    EB_DESTROY_THREAD(encHandlePtr->initialRateControlThreadHandle);

    // Source Based Oprations
    EB_DESTROY_THREAD_ARRAY(encHandlePtr->sourceBasedOperationsThreadHandleArray, controlSetPtr->sourceBasedOperationsProcessInitCount);

    // Picture Manager
    EB_DESTROY_THREAD(encHandlePtr->pictureManagerThreadHandle);

    // Rate Control
    EB_DESTROY_THREAD(encHandlePtr->rateControlThreadHandle);

    // Mode Decision Configuration Process
    EB_DESTROY_THREAD_ARRAY(encHandlePtr->modeDecisionConfigurationThreadHandleArray, controlSetPtr->modeDecisionConfigurationProcessInitCount);

    // EncDec Process
    EB_DESTROY_THREAD_ARRAY(encHandlePtr->encDecThreadHandleArray, controlSetPtr->encDecProcessInitCount);

    // Entropy Coding Process
    EB_DESTROY_THREAD_ARRAY(encHandlePtr->entropyCodingThreadHandleArray, controlSetPtr->entropyCodingProcessInitCount);

    // Packetization
    EB_DESTROY_THREAD(encHandlePtr->packetizationThreadHandle);

    // UnPack
    EB_DESTROY_THREAD_ARRAY(encHandlePtr->unpackThreadHandleArray, controlSetPtr->unpackProcessInitCount);
}

/**********************************
* Encoder Library Handle Deonstructor
**********************************/
static void EbEncHandleDctor(EB_PTR p)
{
    EbEncHandle_t *obj = (EbEncHandle_t *)p;
    // from EbInitEncoder
    EBEncHandleStopThreads(obj);
    EB_DELETE(obj->sequenceControlSetPoolPtr);
    EB_DELETE_PTR_ARRAY(obj->pictureParentControlSetPoolPtrArray, obj->encodeInstanceTotalCount);
    EB_DELETE_PTR_ARRAY(obj->pictureControlSetPoolPtrArray, obj->encodeInstanceTotalCount);
    EB_DELETE_PTR_ARRAY(obj->referencePicturePoolPtrArray, obj->encodeInstanceTotalCount);
    EB_DELETE_PTR_ARRAY(obj->paReferencePicturePoolPtrArray, obj->encodeInstanceTotalCount);
    EB_DELETE(obj->inputBufferResourcePtr);
    EB_DELETE_PTR_ARRAY(obj->outputStreamBufferResourcePtrArray, obj->encodeInstanceTotalCount);
    EB_DELETE_PTR_ARRAY(obj->outputReconBufferResourcePtrArray, obj->encodeInstanceTotalCount);
    EB_DELETE(obj->resourceCoordinationResultsResourcePtr);
    EB_DELETE(obj->pictureAnalysisResultsResourcePtr);
    EB_DELETE(obj->pictureDecisionResultsResourcePtr);
    EB_DELETE(obj->motionEstimationResultsResourcePtr);
    EB_DELETE(obj->initialRateControlResultsResourcePtr);
    EB_DELETE(obj->pictureDemuxResultsResourcePtr);
    EB_DELETE(obj->rateControlTasksResourcePtr);
    EB_DELETE(obj->rateControlResultsResourcePtr);
    EB_DELETE(obj->encDecTasksResourcePtr);
    EB_DELETE(obj->encDecResultsResourcePtr);
    EB_DELETE(obj->entropyCodingResultsResourcePtr);
    EB_DELETE(obj->resourceCoordinationContextPtr);
    EB_DELETE(obj->unpackTasksResourcePtr);
    EB_DELETE(obj->unpackSyncResourcePtr);
    EB_DELETE_PTR_ARRAY(obj->pictureAnalysisContextPtrArray, obj->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount);
    EB_DELETE_PTR_ARRAY(obj->motionEstimationContextPtrArray, obj->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount);
    EB_DELETE_PTR_ARRAY(obj->sourceBasedOperationsContextPtrArray, obj->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount);
    EB_DELETE_PTR_ARRAY(obj->modeDecisionConfigurationContextPtrArray, obj->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount);
    EB_DELETE_PTR_ARRAY(obj->encDecContextPtrArray, obj->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount);
    EB_DELETE_PTR_ARRAY(obj->entropyCodingContextPtrArray, obj->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount);
    EB_DELETE_PTR_ARRAY(obj->unpackContextPtrArray, obj->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount);
    EB_DELETE(obj->pictureDecisionContextPtr);
    EB_DELETE(obj->initialRateControlContextPtr);
    EB_DELETE(obj->pictureManagerContextPtr);
    EB_DELETE(obj->rateControlContextPtr);
    EB_DELETE(obj->packetizationContextPtr);

    EB_FREE_ARRAY(obj->pictureParentControlSetPoolProducerFifoPtrDblArray);
    EB_FREE_ARRAY(obj->pictureControlSetPoolProducerFifoPtrDblArray);
    EB_FREE_ARRAY(obj->referencePicturePoolProducerFifoPtrDblArray);
    EB_FREE_ARRAY(obj->paReferencePicturePoolProducerFifoPtrDblArray);
    EB_FREE_ARRAY(obj->outputStreamBufferProducerFifoPtrDblArray);
    EB_FREE_ARRAY(obj->outputStreamBufferConsumerFifoPtrDblArray);
    EB_FREE_ARRAY(obj->outputReconBufferProducerFifoPtrDblArray);
    EB_FREE_ARRAY(obj->outputReconBufferConsumerFifoPtrDblArray);

    // from EbEncHandleCtor
    EB_FREE_PTR_ARRAY(obj->appCallbackPtrArray, obj->encodeInstanceTotalCount);
    EB_DELETE_PTR_ARRAY(obj->sequenceControlSetInstanceArray, obj->encodeInstanceTotalCount);
    EB_FREE_ARRAY(obj->computeSegmentsTotalCountArray);
}
/**********************************
 * Encoder Library Handle Constructor
 **********************************/
static EB_ERRORTYPE EbEncHandleCtor(
    EbEncHandle_t *encHandlePtr,
    EB_HANDLETYPE ebHandlePtr)
{
    EB_U32  instanceIndex;
    encHandlePtr->dctor = EbEncHandleDctor;

    if (EbHevcInitThreadManagmentParams() == EB_ErrorInsufficientResources) {
        return EB_ErrorInsufficientResources;
    }

    encHandlePtr->encodeInstanceTotalCount = EB_EncodeInstancesTotalCount;

    // Config Set Count
    encHandlePtr->sequenceControlSetPoolTotalCount = EB_SequenceControlSetPoolInitCount;

    EB_MALLOC_ARRAY(encHandlePtr->computeSegmentsTotalCountArray, encHandlePtr->encodeInstanceTotalCount);
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        encHandlePtr->computeSegmentsTotalCountArray[instanceIndex] = EB_ComputeSegmentInitCount;
    }

    // Initialize Callbacks
    EB_ALLOC_PTR_ARRAY(encHandlePtr->appCallbackPtrArray, encHandlePtr->encodeInstanceTotalCount);

    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        EB_MALLOC(encHandlePtr->appCallbackPtrArray[instanceIndex], sizeof(EbCallback_t));
        encHandlePtr->appCallbackPtrArray[instanceIndex]->ErrorHandler                          = libSvtEncoderSendErrorExit;
        encHandlePtr->appCallbackPtrArray[instanceIndex]->handle                                = ebHandlePtr;
    }

    // Initialize Sequence Control Set Instance Array
    EB_ALLOC_PTR_ARRAY(encHandlePtr->sequenceControlSetInstanceArray, encHandlePtr->encodeInstanceTotalCount);

    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        EB_NEW(
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex],
            EbSequenceControlSetInstanceCtor);
    }

    return EB_ErrorNone;
}

#ifdef _WIN32
static EB_U64 EbHevcGetAffinityMask(EB_U32 lpnum) {
    EB_U64 mask = 0x1;
    for (EB_U32 i = lpnum - 1; i > 0; i--)
        mask += (EB_U64)1 << i;
    return mask;
}
#endif

void EbHevcSwitchToRealTime()
{
#ifndef _WIN32

    struct sched_param schedParam = {
        .sched_priority = sched_get_priority_max(SCHED_FIFO)
    };

    int retValue = pthread_setschedparam(pthread_self(), SCHED_FIFO, &schedParam);
    if (retValue == EPERM)
        SVT_LOG("\nSVT [WARNING] Elevated privileges required to run with real-time policies! Check Linux Best Known Configuration in User Guide to run application in real-time without elevated privileges!\n\n");

#endif
}

void EbHevcSetThreadManagementParameters(
    EB_H265_ENC_CONFIGURATION   *configPtr)
{
    if (configPtr->switchThreadsToRtPriority == 1)
        EbHevcSwitchToRealTime();

#ifdef _WIN32
    EB_U32 numLogicProcessors = EbHevcGetNumProcessors();
    // For system with a single processor group(no more than 64 logic processors all together)
    // Affinity of the thread can be set to one or more logical processors
    if (numGroups == 1) {
        EB_U32 lps = configPtr->logicalProcessors == 0 ? numLogicProcessors:
            configPtr->logicalProcessors < numLogicProcessors ? configPtr->logicalProcessors : numLogicProcessors;
        groupAffinity.Mask = EbHevcGetAffinityMask(lps);
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
                    groupAffinity.Mask = EbHevcGetAffinityMask(configPtr->logicalProcessors);
                }
            }
            else {
                EB_U32 lps = configPtr->logicalProcessors == 0 ? numLpPerGroup :
                    configPtr->logicalProcessors < numLpPerGroup ? configPtr->logicalProcessors : numLpPerGroup;
                groupAffinity.Mask = EbHevcGetAffinityMask(lps);
                groupAffinity.Group = configPtr->targetSocket;
            }
        }
    }
#elif defined(__linux__)
    EB_U32 numLogicProcessors = EbHevcGetNumProcessors();
    CPU_ZERO(&groupAffinity);
    if (numGroups == 1) {
        EB_U32 lps = configPtr->logicalProcessors == 0 ? numLogicProcessors:
            configPtr->logicalProcessors < numLogicProcessors ? configPtr->logicalProcessors : numLogicProcessors;
            if (configPtr->targetSocket != -1) {
                for(EB_U32 i=0; i<lps; i++)
                    CPU_SET(lpGroup[0].group[i], &groupAffinity);
            }
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
#ifdef __GNUC__
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
        ASM_TYPES = EbHevcGetCpuAsmType(); // Use highest assembly
    }
    else if (encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.asmType == EB_ASM_C) {
        ASM_TYPES = EB_ASM_C; // Use C_only
    }

    /************************************
     * Sequence Control Set
     ************************************/
    EB_NEW(
        encHandlePtr->sequenceControlSetPoolPtr,
        EbSystemResourceCtor,
        encHandlePtr->sequenceControlSetPoolTotalCount,
        1,
        0,
        &encHandlePtr->sequenceControlSetPoolProducerFifoPtrArray,
        (EbFifo_t ***)EB_NULL,
        EB_FALSE,
        EbSequenceControlSetCreator,
        EB_NULL,
        NULL);

    /************************************
     * Picture Control Set: Parent
     ************************************/
    EB_ALLOC_PTR_ARRAY(encHandlePtr->pictureParentControlSetPoolPtrArray, encHandlePtr->encodeInstanceTotalCount);
    EB_MALLOC_ARRAY(encHandlePtr->pictureParentControlSetPoolProducerFifoPtrDblArray, encHandlePtr->encodeInstanceTotalCount);

    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {

        // The segment Width & Height Arrays are in units of LCUs, not samples
        PictureControlSetInitData_t inputData;

        inputData.pictureWidth  = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth;
        inputData.pictureHeight = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight;
        inputData.leftPadding   = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->leftPadding;
        inputData.rightPadding  = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->rightPadding;
        inputData.topPadding    = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->topPadding;
        inputData.botPadding    = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->botPadding;
        inputData.bitDepth      = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->outputBitdepth;
        inputData.colorFormat   = (EB_COLOR_FORMAT)encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->chromaFormatIdc;
        inputData.lcuSize       = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->lcuSize;
        inputData.maxDepth      = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxLcuDepth;
        inputData.is16bit       = is16bit;
        inputData.compressedTenBitFormat = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.compressedTenBitFormat;
        inputData.tileRowCount = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.tileRowCount;
        inputData.tileColumnCount = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.tileColumnCount;

        inputData.encMode = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.encMode;
        inputData.speedControl = (EB_U8)encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.speedControlFlag;
        inputData.segmentOvEnabled = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.segmentOvEnabled;
        //inputData.tune = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.tune;

        EB_NEW(
            encHandlePtr->pictureParentControlSetPoolPtrArray[instanceIndex],
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->pictureControlSetPoolInitCount,//encHandlePtr->pictureControlSetPoolTotalCount,
            1,
            0,
            &encHandlePtr->pictureParentControlSetPoolProducerFifoPtrDblArray[instanceIndex],
            (EbFifo_t ***)EB_NULL,
            EB_FALSE,
            PictureParentControlSetCreator,
            &inputData,
            NULL);
    }

    /************************************
     * Picture Control Set: Child
     ************************************/
    EB_ALLOC_PTR_ARRAY(encHandlePtr->pictureControlSetPoolPtrArray, encHandlePtr->encodeInstanceTotalCount);
    EB_MALLOC_ARRAY(encHandlePtr->pictureControlSetPoolProducerFifoPtrDblArray, encHandlePtr->encodeInstanceTotalCount);

    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {

        // The segment Width & Height Arrays are in units of LCUs, not samples
        PictureControlSetInitData_t inputData;
        unsigned i;

        inputData.encDecSegmentCol = 0;
        inputData.encDecSegmentRow = 0;
        inputData.tileGroupCol = 0;
        inputData.tileGroupRow = 0;
        for(i=0; i <= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.hierarchicalLevels; ++i) {
            inputData.encDecSegmentCol = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->encDecSegmentColCountArray[i] > inputData.encDecSegmentCol ?
                (EB_U16) encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->encDecSegmentColCountArray[i] :
                inputData.encDecSegmentCol;
            inputData.encDecSegmentRow = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->encDecSegmentRowCountArray[i] > inputData.encDecSegmentRow ?
                (EB_U16) encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->encDecSegmentRowCountArray[i] :
                inputData.encDecSegmentRow;
            inputData.tileGroupCol= encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->tileGroupColCountArray[i] > inputData.tileGroupCol ?
                (EB_U16) encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->tileGroupColCountArray[i] :
                inputData.tileGroupCol;
            inputData.tileGroupRow = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->tileGroupRowCountArray[i] > inputData.tileGroupRow ?
                (EB_U16) encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->tileGroupRowCountArray[i] :
                inputData.tileGroupRow;
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
        inputData.compressedTenBitFormat = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.compressedTenBitFormat;
        inputData.tileRowCount = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.tileRowCount;
        inputData.tileColumnCount = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.tileColumnCount;

        inputData.encMode = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.encMode;
        inputData.speedControl = (EB_U8)encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.speedControlFlag;
        EB_NEW(
            encHandlePtr->pictureControlSetPoolPtrArray[instanceIndex],
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->pictureControlSetPoolInitCountChild, //EB_PictureControlSetPoolInitCountChild,
            1,
            0,
            &encHandlePtr->pictureControlSetPoolProducerFifoPtrDblArray[instanceIndex],
            (EbFifo_t ***)EB_NULL,
            EB_FALSE,
            PictureControlSetCreator,
            &inputData,
            NULL);
    }

    /************************************
     * Picture Buffers
     ************************************/

    // Allocate Resource Arrays
    EB_ALLOC_PTR_ARRAY(encHandlePtr->referencePicturePoolPtrArray,   encHandlePtr->encodeInstanceTotalCount);
    EB_ALLOC_PTR_ARRAY(encHandlePtr->paReferencePicturePoolPtrArray, encHandlePtr->encodeInstanceTotalCount);

    // Allocate Producer Fifo Arrays
    EB_MALLOC_ARRAY(encHandlePtr->referencePicturePoolProducerFifoPtrDblArray,   encHandlePtr->encodeInstanceTotalCount);
    EB_MALLOC_ARRAY(encHandlePtr->paReferencePicturePoolProducerFifoPtrDblArray, encHandlePtr->encodeInstanceTotalCount);

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
        EB_NEW(
            encHandlePtr->referencePicturePoolPtrArray[instanceIndex],
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->referencePictureBufferInitCount,//encHandlePtr->referencePicturePoolTotalCount,
            EB_PictureManagerProcessInitCount,
            0,
            &encHandlePtr->referencePicturePoolProducerFifoPtrDblArray[instanceIndex],
            (EbFifo_t ***)EB_NULL,
            EB_FALSE,
            EbReferenceObjectCreator,
            &(EbReferenceObjectDescInitDataStructure),
            NULL);

        // PA Reference Picture Buffers
        // Currently, only Luma samples are needed in the PA
        referencePictureBufferDescInitData.maxWidth               = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth;
        referencePictureBufferDescInitData.maxHeight              = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaHeight;
        referencePictureBufferDescInitData.bitDepth               = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->inputBitdepth;
        referencePictureBufferDescInitData.colorFormat            = EB_YUV420;
        referencePictureBufferDescInitData.bufferEnableMask       = PICTURE_BUFFER_DESC_LUMA_MASK;

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
        EB_NEW(
            encHandlePtr->paReferencePicturePoolPtrArray[instanceIndex],
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->paReferencePictureBufferInitCount,
            EB_PictureDecisionProcessInitCount,
            0,
            &encHandlePtr->paReferencePicturePoolProducerFifoPtrDblArray[instanceIndex],
            (EbFifo_t ***)EB_NULL,
            EB_FALSE,
            EbPaReferenceObjectCreator,
            &(EbPaReferenceObjectDescInitDataStructure),
            NULL);

        // Set the SequenceControlSet Picture Pool Fifo Ptrs
        encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->referencePicturePoolFifoPtr     = (encHandlePtr->referencePicturePoolProducerFifoPtrDblArray[instanceIndex])[0];
        encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->paReferencePicturePoolFifoPtr   = (encHandlePtr->paReferencePicturePoolProducerFifoPtrDblArray[instanceIndex])[0];
}

    /************************************
     * System Resource Managers & Fifos
     ************************************/

    // EB_BUFFERHEADERTYPE Input
    EB_NEW(
        encHandlePtr->inputBufferResourcePtr,
        EbSystemResourceCtor,
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->inputBufferFifoInitCount,
        1,
        EB_ResourceCoordinationProcessInitCount,
        &encHandlePtr->inputBufferProducerFifoPtrArray,
        &encHandlePtr->inputBufferConsumerFifoPtrArray,
        EB_TRUE,
        EbInputBufferHeaderCreator,
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr,
        EbInputBufferHeaderDestroyer);

    // EB_BUFFERHEADERTYPE Output Stream
    EB_ALLOC_PTR_ARRAY(encHandlePtr->outputStreamBufferResourcePtrArray, encHandlePtr->encodeInstanceTotalCount);
    EB_MALLOC_ARRAY(encHandlePtr->outputStreamBufferProducerFifoPtrDblArray, encHandlePtr->encodeInstanceTotalCount);
    EB_MALLOC_ARRAY(encHandlePtr->outputStreamBufferConsumerFifoPtrDblArray, encHandlePtr->encodeInstanceTotalCount);

    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        EB_NEW(
            encHandlePtr->outputStreamBufferResourcePtrArray[instanceIndex],
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->outputBufferFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->totalProcessInitCount,// EB_PacketizationProcessInitCount,
            1,
            &encHandlePtr->outputStreamBufferProducerFifoPtrDblArray[instanceIndex],
            &encHandlePtr->outputStreamBufferConsumerFifoPtrDblArray[instanceIndex],
            EB_TRUE,
            EbOutputBufferHeaderCreator,
            &encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig,
            EbOutputBufferHeaderDestroyer);
    }
    if (encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig.reconEnabled) {
        // EB_BUFFERHEADERTYPE Output Recon
        EB_ALLOC_PTR_ARRAY(encHandlePtr->outputReconBufferResourcePtrArray, encHandlePtr->encodeInstanceTotalCount);
        EB_MALLOC_ARRAY(encHandlePtr->outputReconBufferProducerFifoPtrDblArray, encHandlePtr->encodeInstanceTotalCount);
        EB_MALLOC_ARRAY(encHandlePtr->outputReconBufferConsumerFifoPtrDblArray, encHandlePtr->encodeInstanceTotalCount);
        for (instanceIndex = 0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
            EB_NEW(
                encHandlePtr->outputReconBufferResourcePtrArray[instanceIndex],
                EbSystemResourceCtor,
                encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->reconBufferFifoInitCount,
                encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->encDecProcessInitCount,
                1,
                &encHandlePtr->outputReconBufferProducerFifoPtrDblArray[instanceIndex],
                &encHandlePtr->outputReconBufferConsumerFifoPtrDblArray[instanceIndex],
                EB_TRUE,
                EbOutputReconBufferHeaderCreator,
                encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr,
                EbOutputReconBufferHeaderDestroyer);
        }
    }

    // Resource Coordination Results
    {
        ResourceCoordinationResultInitData_t resourceCoordinationResultInitData;
        EB_NEW(
            encHandlePtr->resourceCoordinationResultsResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->resourceCoordinationFifoInitCount,
            EB_ResourceCoordinationProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount,
            &encHandlePtr->resourceCoordinationResultsProducerFifoPtrArray,
            &encHandlePtr->resourceCoordinationResultsConsumerFifoPtrArray,
            EB_TRUE,
            ResourceCoordinationResultCreator,
            &resourceCoordinationResultInitData,
            NULL);
    }


    // Picture Analysis Results
    {
        PictureAnalysisResultInitData_t pictureAnalysisResultInitData;

        EB_NEW(
            encHandlePtr->pictureAnalysisResultsResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount,
            EB_PictureDecisionProcessInitCount,
            &encHandlePtr->pictureAnalysisResultsProducerFifoPtrArray,
            &encHandlePtr->pictureAnalysisResultsConsumerFifoPtrArray,
            EB_TRUE,
            PictureAnalysisResultCreator,
            &pictureAnalysisResultInitData,
            NULL);
    }

    // Picture Decision Results
    {
        PictureDecisionResultInitData_t pictureDecisionResultInitData;

        EB_NEW(
            encHandlePtr->pictureDecisionResultsResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureDecisionFifoInitCount,
            EB_PictureDecisionProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount,
            &encHandlePtr->pictureDecisionResultsProducerFifoPtrArray,
            &encHandlePtr->pictureDecisionResultsConsumerFifoPtrArray,
            EB_TRUE,
            PictureDecisionResultCreator,
            &pictureDecisionResultInitData,
            NULL);
    }

    // Motion Estimation Results
    {
        MotionEstimationResultsInitData_t motionEstimationResultInitData;

        EB_NEW(
            encHandlePtr->motionEstimationResultsResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount,
            EB_InitialRateControlProcessInitCount,
            &encHandlePtr->motionEstimationResultsProducerFifoPtrArray,
            &encHandlePtr->motionEstimationResultsConsumerFifoPtrArray,
            EB_TRUE,
            MotionEstimationResultsCreator,
            &motionEstimationResultInitData,
            NULL);
    }

    // Initial Rate Control Results
    {
        InitialRateControlResultInitData_t initialRateControlResultInitData;

        EB_NEW(
            encHandlePtr->initialRateControlResultsResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->initialRateControlFifoInitCount,
            EB_InitialRateControlProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount,
            &encHandlePtr->initialRateControlResultsProducerFifoPtrArray,
            &encHandlePtr->initialRateControlResultsConsumerFifoPtrArray,
            EB_TRUE,
            InitialRateControlResultsCreator,
            &initialRateControlResultInitData,
            NULL);
    }

    // Picture Demux Results
    {
        PictureResultInitData_t pictureResultInitData;

        EB_NEW(
            encHandlePtr->pictureDemuxResultsResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureDemuxFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount + encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount + 1, // 1 for packetization
            EB_PictureManagerProcessInitCount,
            &encHandlePtr->pictureDemuxResultsProducerFifoPtrArray,
            &encHandlePtr->pictureDemuxResultsConsumerFifoPtrArray,
            EB_TRUE,
            PictureResultsCreator,
            &pictureResultInitData,
            NULL);
    }

    // Rate Control Tasks
    {
        RateControlTasksInitData_t rateControlTasksInitData;

        EB_NEW(
            encHandlePtr->rateControlTasksResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->rateControlTasksFifoInitCount,
            RateControlPortTotalCount(),
            EB_RateControlProcessInitCount,
            &encHandlePtr->rateControlTasksProducerFifoPtrArray,
            &encHandlePtr->rateControlTasksConsumerFifoPtrArray,
            EB_TRUE,
            RateControlTasksCreator,
            &rateControlTasksInitData,
            NULL);
    }

    // Rate Control Results
    {
        RateControlResultsInitData_t rateControlResultInitData;

        EB_NEW(
            encHandlePtr->rateControlResultsResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->rateControlFifoInitCount,
            EB_RateControlProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount,
            &encHandlePtr->rateControlResultsProducerFifoPtrArray,
            &encHandlePtr->rateControlResultsConsumerFifoPtrArray,
            EB_TRUE,
            RateControlResultsCreator,
            &rateControlResultInitData,
            NULL);
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

        EB_NEW(
            encHandlePtr->encDecTasksResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationFifoInitCount,
            EncDecPortTotalCount(),
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount,
            &encHandlePtr->encDecTasksProducerFifoPtrArray,
            &encHandlePtr->encDecTasksConsumerFifoPtrArray,
            EB_TRUE,
            EncDecTasksCreator,
            &ModeDecisionResultInitData,
            NULL);
    }

    // EncDec Results
    {
        EncDecResultsInitData_t encDecResultInitData;

        EB_NEW(
            encHandlePtr->encDecResultsResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount,
            &encHandlePtr->encDecResultsProducerFifoPtrArray,
            &encHandlePtr->encDecResultsConsumerFifoPtrArray,
            EB_TRUE,
            EncDecResultsCreator,
            &encDecResultInitData,
            NULL);
    }

    // Entropy Coding Results
    {
        EntropyCodingResultsInitData_t entropyCodingResultInitData;

        EB_NEW(
            encHandlePtr->entropyCodingResultsResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingFifoInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount,
            EB_PacketizationProcessInitCount,
            &encHandlePtr->entropyCodingResultsProducerFifoPtrArray,
            &encHandlePtr->entropyCodingResultsConsumerFifoPtrArray,
            EB_TRUE,
            EntropyCodingResultsCreator,
            &entropyCodingResultInitData,
            NULL);
    }

    // UnPack Tasks
    {
        EB_NEW(
            encHandlePtr->unpackTasksResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount,
            &encHandlePtr->unpackTasksProducerFifoPtrArray,
            &encHandlePtr->unpackTasksConsumerFifoPtrArray,
            EB_TRUE,
            UnPackCreator,
            EB_NULL,
            UnPackDestoryer);
    }

    // UnPack Sync
    {
        EB_NEW(
            encHandlePtr->unpackSyncResourcePtr,
            EbSystemResourceCtor,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount,
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount,
            &encHandlePtr->unpackSyncProducerFifoPtrArray,
            &encHandlePtr->unpackSyncConsumerFifoPtrArray,
            EB_TRUE,
            UnPackCreator,
            EB_NULL,
            UnPackDestoryer);
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
    EB_NEW(
        encHandlePtr->resourceCoordinationContextPtr,
        ResourceCoordinationContextCtor,
        encHandlePtr->inputBufferConsumerFifoPtrArray[0],
        encHandlePtr->resourceCoordinationResultsProducerFifoPtrArray[0],
        encHandlePtr->pictureParentControlSetPoolProducerFifoPtrDblArray[0],//ResourceCoordination works with ParentPCS
        encHandlePtr->sequenceControlSetInstanceArray,
        encHandlePtr->sequenceControlSetPoolProducerFifoPtrArray[0],
        encHandlePtr->appCallbackPtrArray,
        encHandlePtr->computeSegmentsTotalCountArray,
        encHandlePtr->encodeInstanceTotalCount);

    // Picture Analysis Context
    EB_ALLOC_PTR_ARRAY(encHandlePtr->pictureAnalysisContextPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount);

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

        EB_NEW(
            encHandlePtr->pictureAnalysisContextPtrArray[processIndex],
            PictureAnalysisContextCtor,
            &pictureBufferDescConf,
            EB_TRUE,
            encHandlePtr->resourceCoordinationResultsConsumerFifoPtrArray[processIndex],
            encHandlePtr->pictureAnalysisResultsProducerFifoPtrArray[processIndex],
            ((encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->maxInputLumaWidth + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE) *
            ((encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->maxInputLumaHeight + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE));
    }

    // Picture Decision Context
    {
        instanceIndex = 0;

        EB_NEW(
            encHandlePtr->pictureDecisionContextPtr,
            PictureDecisionContextCtor,
            encHandlePtr->pictureAnalysisResultsConsumerFifoPtrArray[0],
            encHandlePtr->pictureDecisionResultsProducerFifoPtrArray[0]);
    }

    // Motion Analysis Context
    EB_ALLOC_PTR_ARRAY(encHandlePtr->motionEstimationContextPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount);

    for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount; ++processIndex) {
        EB_NEW(
            encHandlePtr->motionEstimationContextPtrArray[processIndex],
            MotionEstimationContextCtor,
            encHandlePtr->pictureDecisionResultsConsumerFifoPtrArray[processIndex],
            encHandlePtr->motionEstimationResultsProducerFifoPtrArray[processIndex]);
    }

    // Initial Rate Control Context
    EB_NEW(
        encHandlePtr->initialRateControlContextPtr,
        InitialRateControlContextCtor,
        encHandlePtr->motionEstimationResultsConsumerFifoPtrArray[0],
        encHandlePtr->initialRateControlResultsProducerFifoPtrArray[0]);


	// Source Based Operations Context
    EB_ALLOC_PTR_ARRAY(encHandlePtr->sourceBasedOperationsContextPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount);

    for (processIndex = 0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount; ++processIndex) {
        EB_NEW(
            encHandlePtr->sourceBasedOperationsContextPtrArray[processIndex],
            SourceBasedOperationsContextCtor,
            encHandlePtr->initialRateControlResultsConsumerFifoPtrArray[processIndex],
            encHandlePtr->pictureDemuxResultsProducerFifoPtrArray[processIndex]);
    }

    // Picture Manager Context
    EB_NEW(
        encHandlePtr->pictureManagerContextPtr,
        PictureManagerContextCtor,
        encHandlePtr->pictureDemuxResultsConsumerFifoPtrArray[0],
        encHandlePtr->rateControlTasksProducerFifoPtrArray[RateControlPortLookup(RATE_CONTROL_INPUT_PORT_PICTURE_MANAGER, 0)],
        encHandlePtr->pictureControlSetPoolProducerFifoPtrDblArray[0]);//The Child PCS Pool here

    // Rate Control Context
    EB_NEW(
        encHandlePtr->rateControlContextPtr,
        RateControlContextCtor,
        encHandlePtr->rateControlTasksConsumerFifoPtrArray[0],
        encHandlePtr->rateControlResultsProducerFifoPtrArray[0],
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->intraPeriodLength);

	// Mode Decision Configuration Contexts
    {
		// Mode Decision Configuration Contexts
        EB_ALLOC_PTR_ARRAY(encHandlePtr->modeDecisionConfigurationContextPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount);

        for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount; ++processIndex) {
            EB_NEW(
                encHandlePtr->modeDecisionConfigurationContextPtrArray[processIndex],
                ModeDecisionConfigurationContextCtor,
                encHandlePtr->rateControlResultsConsumerFifoPtrArray[processIndex],
                encHandlePtr->encDecTasksProducerFifoPtrArray[EncDecPortLookup(ENCDEC_INPUT_PORT_MDC, processIndex)],
                ((encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->maxInputLumaWidth + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE) *
                ((encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->maxInputLumaHeight + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE));
        }
    }

    maxPictureWidth = 0;
    for(instanceIndex=0; instanceIndex < encHandlePtr->encodeInstanceTotalCount; ++instanceIndex) {
        if(maxPictureWidth < encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth) {
            maxPictureWidth = encHandlePtr->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->maxInputLumaWidth;
        }
    }

    // EncDec Contexts
    EB_ALLOC_PTR_ARRAY(encHandlePtr->encDecContextPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount);

    for (processIndex = 0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount; ++processIndex) {
        EB_NEW(
            encHandlePtr->encDecContextPtrArray[processIndex],
            EncDecContextCtor,
            encHandlePtr->encDecTasksConsumerFifoPtrArray[processIndex],
            encHandlePtr->encDecResultsProducerFifoPtrArray[processIndex],
            encHandlePtr->encDecTasksProducerFifoPtrArray[EncDecPortLookup(ENCDEC_INPUT_PORT_ENCDEC, processIndex)],
            encHandlePtr->pictureDemuxResultsProducerFifoPtrArray[encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount + processIndex], // Add port lookup logic here JMJ
            is16bit,
            (EB_COLOR_FORMAT)encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->chromaFormatIdc);
    }

    // Entropy Coding Contexts
    EB_ALLOC_PTR_ARRAY(encHandlePtr->entropyCodingContextPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount);

    for(processIndex=0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount; ++processIndex) {
        EB_NEW(
            encHandlePtr->entropyCodingContextPtrArray[processIndex],
            EntropyCodingContextCtor,
            encHandlePtr->encDecResultsConsumerFifoPtrArray[processIndex],
            encHandlePtr->entropyCodingResultsProducerFifoPtrArray[processIndex],
            encHandlePtr->rateControlTasksProducerFifoPtrArray[RateControlPortLookup(RATE_CONTROL_INPUT_PORT_ENTROPY_CODING, processIndex)],
            is16bit);
    }

    // Packetization Context
    EB_NEW(
        encHandlePtr->packetizationContextPtr,
        PacketizationContextCtor,
        encHandlePtr->entropyCodingResultsConsumerFifoPtrArray[0],
        encHandlePtr->rateControlTasksProducerFifoPtrArray[RateControlPortLookup(RATE_CONTROL_INPUT_PORT_PACKETIZATION, 0)],
        encHandlePtr->pictureDemuxResultsProducerFifoPtrArray[encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount
        + encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount] // Add port lookup logic here JMJ
    );

    // UnPack Context
    EB_ALLOC_PTR_ARRAY(encHandlePtr->unpackContextPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount);

    for (processIndex = 0; processIndex < encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount; ++processIndex) {
        EB_NEW(
            encHandlePtr->unpackContextPtrArray[processIndex],
            UnPackContextCtor,
            &encHandlePtr->unpackTasksProducerFifoPtrArray[0][0],
            &encHandlePtr->unpackTasksConsumerFifoPtrArray[0][0],
            &encHandlePtr->unpackSyncProducerFifoPtrArray[0][0],
            &encHandlePtr->unpackSyncConsumerFifoPtrArray[0][0],
            nbLumaThreads,
            nbChromaThreads);
    }

    /************************************
     * Thread Handles
     ************************************/
    EB_H265_ENC_CONFIGURATION   *configPtr = &encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->staticConfig;

    EbHevcSetThreadManagementParameters(configPtr);

    // Resource Coordination
    EB_CREATE_THREAD(encHandlePtr->resourceCoordinationThreadHandle, ResourceCoordinationKernel, encHandlePtr->resourceCoordinationContextPtr);

    // Picture Analysis
    EB_CREATE_THREAD_ARRAY(encHandlePtr->pictureAnalysisThreadHandleArray,
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount,
        PictureAnalysisKernel,
        encHandlePtr->pictureAnalysisContextPtrArray);

    // Picture Decision
    EB_CREATE_THREAD(encHandlePtr->pictureDecisionThreadHandle, PictureDecisionKernel, encHandlePtr->pictureDecisionContextPtr);

    // Motion Estimation
    EB_CREATE_THREAD_ARRAY(encHandlePtr->motionEstimationThreadHandleArray,
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount,
        MotionEstimationKernel,
        encHandlePtr->motionEstimationContextPtrArray);

    // Initial Rate Control
    EB_CREATE_THREAD(encHandlePtr->initialRateControlThreadHandle, InitialRateControlKernel, encHandlePtr->initialRateControlContextPtr);

	// Source Based Oprations
    EB_CREATE_THREAD_ARRAY(encHandlePtr->sourceBasedOperationsThreadHandleArray,
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount,
        SourceBasedOperationsKernel,
        encHandlePtr->sourceBasedOperationsContextPtrArray);

    // Picture Manager
    EB_CREATE_THREAD(encHandlePtr->pictureManagerThreadHandle, PictureManagerKernel, encHandlePtr->pictureManagerContextPtr);

    // Rate Control
    EB_CREATE_THREAD(encHandlePtr->rateControlThreadHandle, RateControlKernel, encHandlePtr->rateControlContextPtr);

    // Mode Decision Configuration Process
    EB_CREATE_THREAD_ARRAY(encHandlePtr->modeDecisionConfigurationThreadHandleArray,
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount,
        ModeDecisionConfigurationKernel,
        encHandlePtr->modeDecisionConfigurationContextPtrArray);

    // EncDec Process
    EB_CREATE_THREAD_ARRAY(encHandlePtr->encDecThreadHandleArray,
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount,
        EncDecKernel,
        encHandlePtr->encDecContextPtrArray);

    // Entropy Coding Process
    EB_CREATE_THREAD_ARRAY(encHandlePtr->entropyCodingThreadHandleArray,
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount,
        EntropyCodingKernel,
        encHandlePtr->entropyCodingContextPtrArray);

    // Packetization
    EB_CREATE_THREAD(encHandlePtr->packetizationThreadHandle, PacketizationKernel, encHandlePtr->packetizationContextPtr);

    // UnPack
    EB_CREATE_THREAD_ARRAY(encHandlePtr->unpackThreadHandleArray,
        encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount,
        UnPack2D, encHandlePtr->unpackContextPtrArray);

    EbPrintMemoryUsage();

    return return_error;
}

/**********************************
 * DeInitialize Encoder Library
 **********************************/
#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbDeinitEncoder(EB_COMPONENTTYPE *h265EncComponent)
{
    if (h265EncComponent == NULL)
        return EB_ErrorBadParameter;
    EbEncHandle_t *encHandlePtr = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    if (encHandlePtr) {
        //Jing: Send signal to quit thread
        EB_SEND_END_OBJ(encHandlePtr->inputBufferProducerFifoPtrArray, EB_ResourceCoordinationProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->resourceCoordinationResultsProducerFifoPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->pictureAnalysisProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->pictureAnalysisResultsProducerFifoPtrArray, EB_PictureDecisionProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->pictureDecisionResultsProducerFifoPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->motionEstimationProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->motionEstimationResultsProducerFifoPtrArray, EB_InitialRateControlProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->initialRateControlResultsProducerFifoPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->sourceBasedOperationsProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->pictureDemuxResultsProducerFifoPtrArray, EB_PictureManagerProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->rateControlTasksProducerFifoPtrArray, EB_RateControlProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->rateControlResultsProducerFifoPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->encDecTasksProducerFifoPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->encDecProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->encDecResultsProducerFifoPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->entropyCodingProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->entropyCodingResultsProducerFifoPtrArray, EB_PacketizationProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->unpackTasksProducerFifoPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount);
        EB_SEND_END_OBJ(encHandlePtr->unpackSyncProducerFifoPtrArray, encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr->unpackProcessInitCount);
    }

    return EB_ErrorNone;
}

EB_ERRORTYPE EbH265EncInitParameter(
    EB_H265_ENC_CONFIGURATION * configPtr);

/**********************************
 * GetHandle
 **********************************/
#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbInitHandle(
    EB_COMPONENTTYPE** pHandle,               // Function to be called in the future for manipulating the component
    void*              pAppData,
    EB_H265_ENC_CONFIGURATION  *configPtr)              // Pointer passed back to the client during callbacks

{
    EB_ERRORTYPE           return_error = EB_ErrorNone;

    #if  defined(__linux__)
    if(lpGroup == EB_NULL) {
        lpGroup = (processorGroup*) malloc(sizeof(processorGroup) * INITIAL_PROCESSOR_GROUP);
        if (lpGroup == (processorGroup*) EB_NULL)
            return EB_ErrorInsufficientResources;
    }
    #endif

    *pHandle = (EB_COMPONENTTYPE*) malloc(sizeof(EB_COMPONENTTYPE));
    if (*pHandle == (EB_COMPONENTTYPE*) NULL) {
        SVT_LOG("Error: Component Struct Malloc Failed\n");
        return EB_ErrorInsufficientResources;
    }
    // Init Component OS objects (threads, semaphores, etc.)
    // also links the various Component control functions
    return_error = InitH265EncoderHandle(*pHandle);

    if (return_error == EB_ErrorNone) {
        ((EB_COMPONENTTYPE*)(*pHandle))->pApplicationPrivate = pAppData;
        return_error = EbH265EncInitParameter(configPtr);
    }
    if (return_error != EB_ErrorNone) {
        EbDeinitEncoder(*pHandle);
        free(*pHandle);
        *pHandle = NULL;
        return return_error;
    }
    EbIncreaseComponentCount();
    return return_error;
}

/**********************************
 * Encoder Componenet DeInit
 **********************************/
EB_ERRORTYPE EbH265EncComponentDeInit(EB_COMPONENTTYPE  *h265EncComponent)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    if (h265EncComponent->pComponentPrivate) {
        EbEncHandle_t *handle;
        handle = (EbEncHandle_t*)(h265EncComponent->pComponentPrivate);
        EB_DELETE(handle);
    }
    else {
        return_error = EB_ErrorUndefined;
    }

    return return_error;
}

/**********************************
 * EbDeinitHandle
 **********************************/
#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbDeinitHandle(
    EB_COMPONENTTYPE  *h265EncComponent)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    if (h265EncComponent) {
        return_error = EbH265EncComponentDeInit(h265EncComponent);
        free(h265EncComponent);
        EbDecreaseComponentCount();
    }
    else {
        return_error = EB_ErrorInvalidComponent;
    }

    #if  defined(__linux__)
    if(lpGroup != EB_NULL) {
        free(lpGroup);
        lpGroup = EB_NULL;
    }
    #endif

    return return_error;
}

#define SCD_LAD 6
EB_U32 SetParentPcs(EB_H265_ENC_CONFIGURATION*   config)
{

    EB_U32 inputPic = 100;
    EB_U32 fps = (EB_U32)((config->frameRate > 1000) ? config->frameRate >> 16 : config->frameRate);

    fps = fps > 120 ? 120 : fps;
    fps = fps < 24 ? 24 : fps;

    if (config->intraPeriodLength > 0 && ((EB_U32)(config->intraPeriodLength) > (fps << 1)) && ((config->sourceWidth * config->sourceHeight) < INPUT_SIZE_4K_TH))
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
    EB_U32 encDecSegH = ((sequenceControlSetPtr->maxInputLumaHeight + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE);
    EB_U32 encDecSegW = ((sequenceControlSetPtr->maxInputLumaWidth + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE);

    EB_U32 meSegH = (((sequenceControlSetPtr->maxInputLumaHeight + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE) < 6) ? 1 : 6;
    EB_U32 meSegW = (((sequenceControlSetPtr->maxInputLumaWidth + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE) < 10) ? 1 : 10;

    EB_U16 tileColCount = sequenceControlSetPtr->staticConfig.tileColumnCount;
    EB_U16 tileRowCount = sequenceControlSetPtr->staticConfig.tileRowCount;

    EB_U32 inputPic = SetParentPcs(&sequenceControlSetPtr->staticConfig);

    unsigned int lpCount = EbHevcGetNumProcessors();
    unsigned int coreCount = lpCount;

    unsigned int totalThreadCount;
    unsigned int threadUnit;

    EB_U32 inputSize = (EB_U32)sequenceControlSetPtr->maxInputLumaWidth * (EB_U32)sequenceControlSetPtr->maxInputLumaHeight;

    EB_U8 inputResolution = (inputSize < INPUT_SIZE_1080i_TH) ? INPUT_SIZE_576p_RANGE_OR_LOWER :
        (inputSize < INPUT_SIZE_1080p_TH) ? INPUT_SIZE_1080i_RANGE :
        (inputSize < INPUT_SIZE_4K_TH) ? INPUT_SIZE_1080p_RANGE :
        INPUT_SIZE_4K_RANGE;

    const EB_U8 lowResInputFactor = 2;

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

    // Thread count computation
    if (sequenceControlSetPtr->staticConfig.threadCount != 0)
        totalThreadCount = sequenceControlSetPtr->staticConfig.threadCount;
    else
        totalThreadCount = coreCount * EB_THREAD_COUNT_FACTOR;

    if (totalThreadCount < EB_THREAD_COUNT_MIN_CORE * EB_THREAD_COUNT_FACTOR) {
        totalThreadCount = EB_THREAD_COUNT_MIN_CORE * EB_THREAD_COUNT_FACTOR;
    }

    if (totalThreadCount % EB_THREAD_COUNT_MIN_CORE) {
        totalThreadCount = (totalThreadCount + EB_THREAD_COUNT_MIN_CORE - 1)
                           / EB_THREAD_COUNT_MIN_CORE * EB_THREAD_COUNT_MIN_CORE;
    }
    threadUnit = totalThreadCount / EB_THREAD_COUNT_MIN_CORE;

    sequenceControlSetPtr->inputBufferFifoInitCount = inputPic + SCD_LAD;
    sequenceControlSetPtr->outputBufferFifoInitCount = EB_OUTPUTBUFFERCOUNT;

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

    // Jing: TODO:
    // Tune it later, different layer may have different Tile Group
    EB_U16 tileGroupColCount = 1;//1 col will have better perf for segments
    EB_U16 tileGroupRowCount = tileRowCount;// > 1 ? (tileRowCount / 2) : 1;

    // Tile group
    sequenceControlSetPtr->tileGroupColCountArray[0] = tileGroupColCount;
    sequenceControlSetPtr->tileGroupColCountArray[1] = tileGroupColCount;
    sequenceControlSetPtr->tileGroupColCountArray[2] = tileGroupColCount;
    sequenceControlSetPtr->tileGroupColCountArray[3] = tileGroupColCount;
    sequenceControlSetPtr->tileGroupColCountArray[4] = tileGroupColCount;
    sequenceControlSetPtr->tileGroupColCountArray[5] = tileGroupColCount;

    sequenceControlSetPtr->tileGroupRowCountArray[0] = tileGroupRowCount;
    sequenceControlSetPtr->tileGroupRowCountArray[1] = tileGroupRowCount;
    sequenceControlSetPtr->tileGroupRowCountArray[2] = tileGroupRowCount;
    sequenceControlSetPtr->tileGroupRowCountArray[3] = tileGroupRowCount;
    sequenceControlSetPtr->tileGroupRowCountArray[4] = tileGroupRowCount;
    sequenceControlSetPtr->tileGroupRowCountArray[5] = tileGroupRowCount;

    //#====================== Data Structures and Picture Buffers ======================
    if (inputResolution <= INPUT_SIZE_1080p_RANGE)
        sequenceControlSetPtr->pictureControlSetPoolInitCount       = inputPic * lowResInputFactor;
    else
        sequenceControlSetPtr->pictureControlSetPoolInitCount       = inputPic;

    sequenceControlSetPtr->pictureControlSetPoolInitCountChild  = MAX(4, coreCount / 6);
    sequenceControlSetPtr->referencePictureBufferInitCount      = inputPic;//MAX((EB_U32)(sequenceControlSetPtr->inputBufferFifoInitCount >> 1), (EB_U32)((1 << sequenceControlSetPtr->staticConfig.hierarchicalLevels) + 2));

    if (inputResolution <= INPUT_SIZE_1080p_RANGE)
        sequenceControlSetPtr->paReferencePictureBufferInitCount    = inputPic * lowResInputFactor;
    else
        sequenceControlSetPtr->paReferencePictureBufferInitCount    = inputPic;

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
    sequenceControlSetPtr->modeDecisionConfigurationFifoInitCount = (300 * sequenceControlSetPtr->staticConfig.tileRowCount);
    sequenceControlSetPtr->motionEstimationFifoInitCount = 308;
    sequenceControlSetPtr->entropyCodingFifoInitCount = 309;
    sequenceControlSetPtr->encDecFifoInitCount = 900;

    //#====================== Processes number ======================
    sequenceControlSetPtr->totalProcessInitCount = 0;
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->pictureAnalysisProcessInitCount           = threadUnit * 4;
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->motionEstimationProcessInitCount          = threadUnit * 8;
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->sourceBasedOperationsProcessInitCount     = threadUnit * 2;
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->modeDecisionConfigurationProcessInitCount = threadUnit * 2;
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->entropyCodingProcessInitCount             = threadUnit * 4;
    sequenceControlSetPtr->totalProcessInitCount += 6; // single processes count
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->unpackProcessInitCount =
                                                    nbLumaThreads + nbChromaThreads * 2;
    sequenceControlSetPtr->totalProcessInitCount += sequenceControlSetPtr->encDecProcessInitCount =
                                                    totalThreadCount - sequenceControlSetPtr->totalProcessInitCount;

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

    if (config->intraRefreshType == CRA_REFRESH)
        intraPeriod -= 1;

    return intraPeriod;
}

// Set configurations for the hardcoded parameters
void EbHevcSetDefaultConfigurationParameters(
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

void EbHevcSetParamBasedOnInput(
    SequenceControlSet_t       *sequenceControlSetPtr)

{
    EB_U32 chromaFormat = EB_YUV420;
    EB_U32 subWidthCMinus1 = 1;
    EB_U32 subHeightCMinus1 = 1;

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

    sequenceControlSetPtr->lumaWidth    = sequenceControlSetPtr->maxInputLumaWidth;
    sequenceControlSetPtr->lumaHeight   = sequenceControlSetPtr->maxInputLumaHeight;

    chromaFormat = sequenceControlSetPtr->chromaFormatIdc;
    subWidthCMinus1 = (chromaFormat == EB_YUV444 ? 1 : 2) - 1;
    subHeightCMinus1 = (chromaFormat >= EB_YUV422 ? 1 : 2) - 1;

    sequenceControlSetPtr->chromaWidth  = sequenceControlSetPtr->maxInputLumaWidth >> subWidthCMinus1;
    sequenceControlSetPtr->chromaHeight = sequenceControlSetPtr->maxInputLumaHeight >> subHeightCMinus1;

    sequenceControlSetPtr->padRight             = sequenceControlSetPtr->maxInputPadRight;
    sequenceControlSetPtr->croppingRightOffset  = sequenceControlSetPtr->padRight;
    sequenceControlSetPtr->padBottom            = sequenceControlSetPtr->maxInputPadBottom;
    sequenceControlSetPtr->croppingBottomOffset = sequenceControlSetPtr->padBottom;

    if (sequenceControlSetPtr->padRight != 0 || sequenceControlSetPtr->padBottom != 0)
        sequenceControlSetPtr->conformanceWindowFlag = 1;
    else
        sequenceControlSetPtr->conformanceWindowFlag = 0;

    DeriveInputResolution(
        sequenceControlSetPtr,
        sequenceControlSetPtr->lumaWidth*sequenceControlSetPtr->lumaHeight);

	sequenceControlSetPtr->pictureWidthInLcu = (EB_U8)((sequenceControlSetPtr->lumaWidth + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize);
	sequenceControlSetPtr->pictureHeightInLcu = (EB_U8)((sequenceControlSetPtr->lumaHeight + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize);
	sequenceControlSetPtr->lcuTotalCount = sequenceControlSetPtr->pictureWidthInLcu * sequenceControlSetPtr->pictureHeightInLcu;

}

void EbHevcCopyApiFromApp(
    SequenceControlSet_t       *sequenceControlSetPtr,
    EB_H265_ENC_CONFIGURATION* pComponentParameterStructure
)
{

    EB_MEMCPY(&sequenceControlSetPtr->staticConfig, pComponentParameterStructure, sizeof(EB_H265_ENC_CONFIGURATION));

    sequenceControlSetPtr->maxInputLumaWidth  = (EB_U16)((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->sourceWidth;
    sequenceControlSetPtr->maxInputLumaHeight = (EB_U16)((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->sourceHeight;

    sequenceControlSetPtr->interlacedVideo = sequenceControlSetPtr->staticConfig.interlacedVideo;

    sequenceControlSetPtr->intraPeriodLength = sequenceControlSetPtr->staticConfig.intraPeriodLength;
    sequenceControlSetPtr->intraRefreshType = sequenceControlSetPtr->staticConfig.intraRefreshType;
    sequenceControlSetPtr->maxTemporalLayers = sequenceControlSetPtr->staticConfig.hierarchicalLevels;
    sequenceControlSetPtr->maxRefCount = 1;
    //Jing: put these to pcs
    //sequenceControlSetPtr->tileRowCount = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->tileRowCount;
    //sequenceControlSetPtr->tileColumnCount = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->tileColumnCount;
    //sequenceControlSetPtr->tileSliceMode = ((EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure)->tileSliceMode;


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
    sequenceControlSetPtr->encoderBitDepth = (EB_U32)(sequenceControlSetPtr->staticConfig.encoderBitDepth);
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

    //Set required flags to signal vbv status when hrd is enabled
    if (sequenceControlSetPtr->staticConfig.hrdFlag == 1) {
        sequenceControlSetPtr->staticConfig.videoUsabilityInfo = 1;
        sequenceControlSetPtr->videoUsabilityInfoPtr->vuiHrdParametersPresentFlag = 1;
        sequenceControlSetPtr->staticConfig.bufferingPeriodSEI = 1;
        sequenceControlSetPtr->staticConfig.pictureTimingSEI = 1;
        sequenceControlSetPtr->videoUsabilityInfoPtr->hrdParametersPtr->nalHrdParametersPresentFlag = 1;
        sequenceControlSetPtr->videoUsabilityInfoPtr->hrdParametersPtr->cpbDpbDelaysPresentFlag = 1;
    }

    if (sequenceControlSetPtr->staticConfig.segmentOvEnabled) {
        if (sequenceControlSetPtr->staticConfig.improveSharpness) {
            SVT_LOG("SVT [Warning]: improveSharpness does not work with segment override, set to false\n");
            sequenceControlSetPtr->staticConfig.improveSharpness = EB_FALSE;
        }

        if (sequenceControlSetPtr->staticConfig.bitRateReduction) {
            SVT_LOG("SVT [Warning]: bitRateReduction does not work with segment override, set to false\n");
            sequenceControlSetPtr->staticConfig.bitRateReduction = EB_FALSE;
        }
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
    if(config->level > 0 && config->level < 40 && config->tier != 0) {
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

    EB_U8 inputResolution = (inputSize < INPUT_SIZE_1080i_TH) ? INPUT_SIZE_576p_RANGE_OR_LOWER :
      (inputSize < INPUT_SIZE_1080p_TH) ? INPUT_SIZE_1080i_RANGE :
      (inputSize < INPUT_SIZE_4K_TH) ? INPUT_SIZE_1080p_RANGE :
      INPUT_SIZE_4K_RANGE;

    // Set number of UnPack2D threads in function of the resolution and the format
    // for resolutions >= 4K need more than 3 threads (best perf at 12 for 8K)
    if(inputResolution >= INPUT_SIZE_4K_RANGE){
        nbLumaThreads = 8;
        nbChromaThreads = sequenceControlSetPtr->chromaFormatIdc == EB_YUV420 ? nbLumaThreads/4 : sequenceControlSetPtr->chromaFormatIdc == EB_YUV422 ? nbLumaThreads/2 : nbLumaThreads;
    }

    // encMode
    sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES;
    if (inputResolution <= INPUT_SIZE_1080i_RANGE) {
        sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_SUB1080P - 1;
        if (config->encMode > MAX_SUPPORTED_MODES_SUB1080P -1) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_SUB1080P-1);
            return_error = EB_ErrorBadParameter;
        }
    } else if (inputResolution == INPUT_SIZE_1080p_RANGE) {
        sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_1080P - 1;
        if (config->encMode > MAX_SUPPORTED_MODES_1080P - 1) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_1080P - 1);
            return_error = EB_ErrorBadParameter;
        }
    } else {
        sequenceControlSetPtr->maxEncMode = MAX_SUPPORTED_MODES_4K_OQ - 1;
        // Incase deprecated tune 0 M12
        if (config->encMode == MAX_SUPPORTED_MODES_4K_OQ) {
            SVT_LOG("SVT [WARNING]: M12 is deprecated. -encMode is set to %d\n", MAX_SUPPORTED_MODES_4K_OQ-1);
            config->encMode--;
        }
        if (config->encMode > MAX_SUPPORTED_MODES_4K_OQ - 1) {
            SVT_LOG("SVT [Error]: Instance %u: encMode must be [0 - %d]\n", channelNumber + 1, MAX_SUPPORTED_MODES_4K_OQ-1);
            return_error = EB_ErrorBadParameter;
        }
    }

    if (config->qp > 51) {
        SVT_LOG("SVT [Error]: Instance %u: QP must be [0 - 51]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->hierarchicalLevels > 3) {
      SVT_LOG("SVT [Error]: Instance %u: Hierarchical Levels supported [0-3]\n", channelNumber + 1);
      return_error = EB_ErrorBadParameter;
    }

    if (config->intraPeriodLength < -2 || config->intraPeriodLength > 255) {
        SVT_LOG("SVT [Error]: Instance %u: The intra period must be [-2 - 255] \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->intraRefreshType < CRA_REFRESH) {
        SVT_LOG("SVT [Error]: Instance %u: Intra refresh type must be -1 (CRA) or >=0 (IDR)\n",channelNumber+1);
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

    if (config->hrdFlag > 1) {
        SVT_LOG("SVT [Error]: Instance %u: hrdFlag must be [0 - 1]\n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->hrdFlag == 1 && ((config->vbvBufsize <= 0) || (config->vbvMaxrate <= 0))) {
        SVT_LOG("SVT [Error]: Instance %u: hrd requires vbv max rate and vbv bufsize to be greater than 0 \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if ((config->rateControlMode == 0) && ((config->vbvBufsize > 0) || (config->vbvMaxrate > 0))) {
        SVT_LOG("SVT [Error]: Instance %u: VBV options can not be used when RateControlMode is 1 (CQP) \n", channelNumber + 1);
        return_error = EB_ErrorBadParameter;
    }

    if (config->vbvBufInit > 100) {
        SVT_LOG("SVT [Error]: Instance %u: Invalid vbvBufInit [0 - 100]\n", channelNumber + 1);
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

    if (levelIdx < TOTAL_LEVEL_COUNT) {
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

    if (config->tune != 1) {
        SVT_LOG("SVT [WARNING]: -tune is deprecated and will be ignored.\n");
        config->tune = 1;
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
	    SVT_LOG("SVT [Error]: Instance %u: maxCLL or maxFALL should be used only with high dynamic range input; set highDynamicRangeInput to 1\n", channelNumber);
        return_error = EB_ErrorBadParameter;
    }

    if (config->useMasteringDisplayColorVolume && !config->highDynamicRangeInput) {
	    SVT_LOG("SVT [Error]: Instance %u: MasterDisplay should be used only with high dynamic range input; set highDynamicRangeInput to 1\n", channelNumber);
        return_error = EB_ErrorBadParameter;
    }

    if (config->dolbyVisionProfile != 0 && config->dolbyVisionProfile != 81) {
	    SVT_LOG("SVT [Error]: Instance %u: Only Dolby Vision Profile 8.1 is supported \n", channelNumber);
        return_error = EB_ErrorBadParameter;
    }

    if (config->dolbyVisionProfile == 81 && config->encoderBitDepth != 10) {
	    SVT_LOG("SVT [Error]: Instance %u: Dolby Vision Profile 8.1 work only with main10 input \n", channelNumber);
        return_error = EB_ErrorBadParameter;
    }

    if (config->dolbyVisionProfile == 81 && !config->useMasteringDisplayColorVolume) {
	    SVT_LOG("SVT [Error]: Instance %u: Dolby Vision Profile 8.1 requires mastering display color volume information \n", channelNumber);
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

    if (config->compressedTenBitFormat > 1)	{
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
#ifdef __GNUC__
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
    configPtr->encMode  = 7;
    configPtr->intraPeriodLength = -2;
    configPtr->intraRefreshType = CRA_REFRESH;
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
    configPtr->bitRateReduction = EB_FALSE;
    configPtr->improveSharpness = EB_FALSE;

    // Bitstream options
    configPtr->codeVpsSpsPps = 1;
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
    configPtr->threadCount = 0;
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

    // VBV
    configPtr->vbvMaxrate = 0;
    configPtr->vbvBufsize = 0;
    configPtr->vbvBufInit = 90;
    configPtr->hrdFlag = 0;

    //segmentOv
    configPtr->segmentOvEnabled = 0;
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
        SVT_LOG("Tier %d\tLevel %.1f\t", config->tier, (float)(config->level) / 10);
    else {
        if (config->tier == 0 )
            SVT_LOG("Tier (auto)\t");
        else
            SVT_LOG("Tier %d\t", config->tier);

        if (config->level == 0 )
            SVT_LOG("Level (auto)\t");
        else
            SVT_LOG("Level %.1f\t", (float)(config->level) / 10);


    }

    SVT_LOG("\nSVT [config]: EncoderMode / Tune\t\t\t\t\t\t\t: %d / %d ", config->encMode, config->tune);
    SVT_LOG("\nSVT [config]: EncoderBitDepth / CompressedTenBitFormat / EncoderColorFormat \t\t: %d / %d / %d", config->encoderBitDepth, config->compressedTenBitFormat, config->encoderColorFormat);
    SVT_LOG("\nSVT [config]: SourceWidth / SourceHeight / InterlacedVideo\t\t\t\t: %d / %d / %d", config->sourceWidth, config->sourceHeight, config->interlacedVideo);

    if (config->frameRateDenominator != 0 && config->frameRateNumerator != 0)
        SVT_LOG("\nSVT [config]: Fps_Numerator / Fps_Denominator / Gop Size / IntraRefreshType \t\t: %d / %d / %d / %d", config->frameRateNumerator > (1<<16) ? config->frameRateNumerator >> 16: config->frameRateNumerator,
            config->frameRateDenominator > (1<<16) ? config->frameRateDenominator >> 16: config->frameRateDenominator,
            config->intraPeriodLength + 1,
            config->intraRefreshType);
    else
        SVT_LOG("\nSVT [config]: FrameRate / Gop Size\t\t\t\t\t\t\t: %d / %d ", config->frameRate > 1000 ? config->frameRate >> 16 : config->frameRate, config->intraPeriodLength + 1);

    SVT_LOG("\nSVT [config]: HierarchicalLevels / BaseLayerSwitchMode / PredStructure\t\t\t: %d / %d / %d ", config->hierarchicalLevels, config->baseLayerSwitchMode, config->predStructure);

    if (config->rateControlMode == 1)
        SVT_LOG("\nSVT [config]: RCMode / TargetBitrate / LAD / SceneChange / QP Range [%u ~ %u]\t\t: VBR / %d / %d / %d ", config->minQpAllowed, config->maxQpAllowed, config->targetBitRate, config->lookAheadDistance, config->sceneChangeDetection);
    else
        SVT_LOG("\nSVT [config]: BRC Mode / QP / LookaheadDistance / SceneChange\t\t\t\t: CQP / %d / %d / %d ", config->qp, config->lookAheadDistance, config->sceneChangeDetection);

    if (config->tune <= 1)
        SVT_LOG("\nSVT [config]: BitRateReduction / ImproveSharpness\t\t\t\t\t: %d / %d ", config->bitRateReduction, config->improveSharpness);

    SVT_LOG("\nSVT [config]: tileColumnCount / tileRowCount / tileSliceMode / Constraint MV \t\t: %d / %d / %d / %d", config->tileColumnCount, config->tileRowCount, config->tileSliceMode, !config->unrestrictedMotionVector);
    SVT_LOG("\nSVT [config]: De-blocking Filter / SAO Filter\t\t\t\t\t\t: %d / %d ", !config->disableDlfFlag, config->enableSaoFlag);
    SVT_LOG("\nSVT [config]: HME / UseDefaultHME\t\t\t\t\t\t\t: %d / %d ", config->enableHmeFlag, config->useDefaultMeHme);
    SVT_LOG("\nSVT [config]: MV Search Area Width / Height \t\t\t\t\t\t: %d / %d ", config->searchAreaWidth, config->searchAreaHeight);
    SVT_LOG("\nSVT [config]: HRD / VBV MaxRate / BufSize / BufInit\t\t\t\t\t: %d / %d / %d / %" PRIu64, config->hrdFlag, config->vbvMaxrate, config->vbvBufsize, config->vbvBufInit);

#ifndef NDEBUG
    SVT_LOG("\nSVT [config]: More configurations for debugging:");
    SVT_LOG("\nSVT [config]: Channel ID / ActiveChannelCount\t\t\t\t\t\t: %d / %d", config->channelId, config->activeChannelCount);
    SVT_LOG("\nSVT [config]: Number of Logical Processors / Target Socket\t\t\t\t: %d / %d", config->logicalProcessors, config->targetSocket);
    SVT_LOG("\nSVT [config]: Threads To RT / Thread Count / ASM Type\t\t\t\t\t: %d / %d / %d", config->switchThreadsToRtPriority, config->threadCount, config->asmType);
    SVT_LOG("\nSVT [config]: Speed Control / Injector Frame Rate\t\t\t\t\t: %d / %d", config->speedControlFlag, (config->injectorFrameRate >> 16));
    SVT_LOG("\nSVT [config]: MaxCLL / MaxFALL / Output Reconstructed YUV\t\t\t\t: %d / %d / %d", config->maxCLL, config->maxFALL, config->reconEnabled);
    SVT_LOG("\nSVT [config]: MasterDisplayColorVolume / DolbyVisionProfile\t\t\t\t: %d / %d", config->useMasteringDisplayColorVolume, config->dolbyVisionProfile);
    SVT_LOG("\nSVT [config]: DisplayPrimaryX[0], DisplayPrimaryX[1], DisplayPrimaryX[2]\t\t: %d / %d / %d", config->displayPrimaryX[0], config->displayPrimaryX[1], config->displayPrimaryX[2]);
    SVT_LOG("\nSVT [config]: DisplayPrimaryY[0], DisplayPrimaryY[1], DisplayPrimaryY[2]\t\t: %d / %d / %d", config->displayPrimaryY[0], config->displayPrimaryY[1], config->displayPrimaryY[2]);
    SVT_LOG("\nSVT [config]: WhitePointX (%d, %d) / DisplayMasteringLuminance Range [%d ~ %d]\t\t\t", config->whitePointX, config->whitePointY, config->maxDisplayMasteringLuminance, config->minDisplayMasteringLuminance);
    SVT_LOG("\nSVT [config]: Constrained Intra / HDR / Code VPS SPS PPS / Code EOS\t\t\t: %d / %d / %d / %d", config->constrainedIntra, config->highDynamicRangeInput, config->codeVpsSpsPps, config->codeEosNal);
    SVT_LOG("\nSVT [config]: Sending VUI / Temporal ID / VPS Timing Info\t\t\t\t: %d / %d / %d", config->videoUsabilityInfo, config->enableTemporalId, config->fpsInVps);
    SVT_LOG("\nSVT [config]: SEI Message:");
    SVT_LOG("\nSVT [config]: AccessUnitDelimiter / BufferingPeriod / PictureTiming\t\t\t: %d / %d / %d", config->accessUnitDelimiter, config->bufferingPeriodSEI, config->pictureTimingSEI);
    SVT_LOG("\nSVT [config]: RegisteredUserData / UnregisteredUserData / RecoveryPoint\t\t\t: %d / %d / %d", config->registeredUserDataSeiFlag, config->unregisteredUserDataSeiFlag, config->recoveryPointSeiFlag);
#endif
    SVT_LOG("\n------------------------------------------- ");
    SVT_LOG("\n");


    fflush(stdout);
}
/**********************************

 * Set Parameter
 **********************************/
#ifdef __GNUC__
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

    EbHevcSetDefaultConfigurationParameters(
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr);

    EbHevcCopyApiFromApp(
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr,
        (EB_H265_ENC_CONFIGURATION*)pComponentParameterStructure);

    return_error = (EB_ERRORTYPE)VerifySettings(
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr);

    if (return_error == EB_ErrorBadParameter) {
        return EB_ErrorBadParameter;
    }

    EbHevcSetParamBasedOnInput(
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr);

    // Initialize the Prediction Structure Group
    EB_NO_THROW_NEW(
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->predictionStructureGroupPtr,
        PredictionStructureGroupCtor,
        pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->sequenceControlSetPtr->staticConfig.baseLayerSwitchMode);

    if (!pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->encodeContextPtr->predictionStructureGroupPtr) {
        EbReleaseMutex(pEncCompData->sequenceControlSetInstanceArray[instanceIndex]->configMutex);
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
#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265EncStreamHeader(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE       **outputStreamPtr)
{
    EB_ERRORTYPE           return_error = EB_ErrorNone;
    EbEncHandle_t          *pEncCompData = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    Bitstream_t             bitstreamPtr;
    SequenceControlSet_t   *sequenceControlSetPtr = pEncCompData->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr;
    EncodeContext_t        *encodeContextPtr = sequenceControlSetPtr->encodeContextPtr;
    EbPPSConfig_t           ppsConfig;
    EB_BUFFERHEADERTYPE    *outputStreamBuffer;

    // Output buffer Allocation
    outputStreamBuffer = (EB_BUFFERHEADERTYPE*)malloc(sizeof(EB_BUFFERHEADERTYPE));
    if (!outputStreamBuffer)
        return EB_ErrorInsufficientResources;

    outputStreamBuffer->pBuffer = (EB_U8*)malloc(sizeof(EB_U8) * PACKETIZATION_PROCESS_BUFFER_SIZE);
    if (!outputStreamBuffer->pBuffer) {
        free(outputStreamBuffer);
        return EB_ErrorInsufficientResources;
    }

    outputStreamBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);
    outputStreamBuffer->nAllocLen = PACKETIZATION_PROCESS_BUFFER_SIZE;
    outputStreamBuffer->pAppPrivate = NULL;
    outputStreamBuffer->sliceType = EB_INVALID_PICTURE;
    outputStreamBuffer->nFilledLen = 0;

    // Intermediate buffers
    OutputBitstreamUnit_t* outBitstreamPtr = NULL;
    EB_NO_THROW_NEW(
        outBitstreamPtr,
        OutputBitstreamUnitCtor,
        PACKETIZATION_PROCESS_BUFFER_SIZE);
    if (!outBitstreamPtr) {
        free(outputStreamBuffer->pBuffer);
        free(outputStreamBuffer);
        return EB_ErrorInsufficientResources;
    }

    bitstreamPtr.outputBitstreamPtr = outBitstreamPtr;

    // Reset the bitstream before writing to it
    ResetBitstream(outBitstreamPtr);

    if (sequenceControlSetPtr->staticConfig.accessUnitDelimiter) {
        EncodeAUD(
            &bitstreamPtr,
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
        &bitstreamPtr,
        sequenceControlSetPtr);

    // Code the SPS
    EncodeSPS(
        &bitstreamPtr,
        sequenceControlSetPtr);

    ppsConfig.ppsId = 0;
    ppsConfig.constrainedFlag = 0;
    EncodePPS(
        &bitstreamPtr,
        sequenceControlSetPtr,
        &ppsConfig);

    if (sequenceControlSetPtr->staticConfig.constrainedIntra == EB_TRUE) {
        // Configure second pps
        ppsConfig.ppsId = 1;
        ppsConfig.constrainedFlag = 1;

        EncodePPS(
            &bitstreamPtr,
            sequenceControlSetPtr,
            &ppsConfig);
    }

    // Flush the Bitstream
    FlushBitstream(outBitstreamPtr);

    // Copy SPS & PPS to the Output Bitstream
    CopyRbspBitstreamToPayload(
        &bitstreamPtr,
        &outputStreamBuffer->pBuffer,
        (EB_U32*) &(outputStreamBuffer->nFilledLen),
        (EB_U32*) &(outputStreamBuffer->nAllocLen),
        encodeContextPtr,
        NAL_UNIT_INVALID);

    *outputStreamPtr = outputStreamBuffer;
    EB_DELETE(outBitstreamPtr);
    return return_error;
}

#ifdef __GNUC__
    __attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265EncReleaseStreamHeader(
    EB_BUFFERHEADERTYPE       *StreamHeaderPtr)
{
    if (!StreamHeaderPtr || !(StreamHeaderPtr->pBuffer)) {
        return EB_ErrorBadParameter;
    }

    free(StreamHeaderPtr->pBuffer);
    free(StreamHeaderPtr);

    return EB_ErrorNone;
}


#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265EncEosNal(
    EB_COMPONENTTYPE           *h265EncComponent,
    EB_BUFFERHEADERTYPE       **outputStreamPtr)
{
    EB_ERRORTYPE           return_error = EB_ErrorNone;
    EbEncHandle_t          *pEncCompData = (EbEncHandle_t*)h265EncComponent->pComponentPrivate;
    Bitstream_t             bitstreamPtr;
    SequenceControlSet_t   *sequenceControlSetPtr = pEncCompData->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr;
    EncodeContext_t        *encodeContextPtr = sequenceControlSetPtr->encodeContextPtr;
    EB_BUFFERHEADERTYPE    *outputStreamBuffer;

    // Output buffer Allocation
    outputStreamBuffer = (EB_BUFFERHEADERTYPE*)malloc(sizeof(EB_BUFFERHEADERTYPE));
    if (!outputStreamBuffer)
        return EB_ErrorInsufficientResources;

    outputStreamBuffer->pBuffer = (EB_U8*)malloc(sizeof(EB_U8) * PACKETIZATION_PROCESS_BUFFER_SIZE);
    if (!outputStreamBuffer->pBuffer) {
        free(outputStreamBuffer);
        return EB_ErrorInsufficientResources;
    }

    outputStreamBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);
    outputStreamBuffer->nAllocLen = PACKETIZATION_PROCESS_BUFFER_SIZE;
    outputStreamBuffer->pAppPrivate = NULL;
    outputStreamBuffer->sliceType = EB_INVALID_PICTURE;
    outputStreamBuffer->nFilledLen = 0;

    // Intermediate buffers
    OutputBitstreamUnit_t* outBitstreamPtr;
    EB_NEW(
        outBitstreamPtr,
        OutputBitstreamUnitCtor,
        EOS_NAL_BUFFER_SIZE);
    bitstreamPtr.outputBitstreamPtr = outBitstreamPtr;

    // Reset the bitstream before writing to it
    ResetBitstream(outBitstreamPtr);

    CodeEndOfSequenceNalUnit(&bitstreamPtr);

    // Flush the Bitstream
    FlushBitstream(outBitstreamPtr);

    // Copy SPS & PPS to the Output Bitstream
    CopyRbspBitstreamToPayload(
        &bitstreamPtr,
        &outputStreamBuffer->pBuffer,
        (EB_U32*) &(outputStreamBuffer->nFilledLen),
        (EB_U32*) &(outputStreamBuffer->nAllocLen),
        encodeContextPtr,
        NAL_UNIT_INVALID);

    *outputStreamPtr = outputStreamBuffer;
    EB_DELETE(outBitstreamPtr);
    return return_error;
}

#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265EncReleaseEosNal(
    EB_BUFFERHEADERTYPE       *EosNalPtr)
{
    if (!EosNalPtr || !(EosNalPtr->pBuffer)) {
        return EB_ErrorBadParameter;
    }

    free(EosNalPtr->pBuffer);
    free(EosNalPtr);

    return EB_ErrorNone;
}

/* charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" */
static EB_ERRORTYPE BaseDecodeFunction(EB_U8* encodedString, EB_U32 base64EncodeLength, EB_U8* decodedString, EB_U32 base64DecodeLength)
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

            //
            while (countBits != 0) {
                countBits -= 8;
                if (k >= base64DecodeLength) {
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
    EB_U32 base64DecodeLength;

    if (src->naluFound == EB_FALSE) {
        return EB_ErrorBadParameter;
    }

    base64Encode = src->naluBase64Encode;
    base64EncodeLength = (uint32_t)strlen((char*)base64Encode);
    base64DecodeLength = (base64EncodeLength / 4) * 3;
    base64Decode = (EB_U8*)malloc(base64DecodeLength);
    if (base64Decode == EB_NULL)
        return EB_ErrorInsufficientResources;

    return_error = BaseDecodeFunction(base64Encode, base64EncodeLength, base64Decode, base64DecodeLength);

    if (return_error != EB_ErrorNone) {
        SVT_LOG("\nSVT [WARNING]: SEI encoded message cannot be decoded \n ");
        goto error;
    }

    if (src->naluNalType == NAL_UNIT_PREFIX_SEI && src->naluPrefix == 0) {
        EB_U64 currentPOC = src->pts;
        if (currentPOC == src->naluPOC) {
            headerPtr->userSeiMsg.payloadSize = (base64EncodeLength / 4) * 3;
            if (src->naluPayloadType == 4)
                headerPtr->userSeiMsg.payloadType = USER_DATA_REGISTERED_ITU_T_T35;
            else if (src->naluPayloadType == 5)
                headerPtr->userSeiMsg.payloadType = USER_DATA_UNREGISTERED;
            else {
                SVT_LOG("\nSVT [WARNING]: Unsupported SEI payload Type for frame %u\n ", src->naluPOC);
                goto error;
            }
            EB_MEMCPY(headerPtr->userSeiMsg.payload, base64Decode, headerPtr->userSeiMsg.payloadSize);
        }
        else {
            SVT_LOG("\nSVT [WARNING]: User SEI frame number %u doesn't match input frame number %" PRId64 "\n ", src->naluPOC, currentPOC);
            goto error;
        }
    }
    else {
        SVT_LOG("\nSVT [WARNING]: SEI message for frame %u is not inserted. Will support only PREFIX SEI message \n ", src->naluPOC);
        goto error;
    }

    free(base64Decode);
    return return_error;

error:
    src->naluFound = EB_FALSE;
    free(base64Decode);
    return EB_ErrorBadParameter;

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
    }
    return return_error;
}

#ifndef NON_AVX512_SUPPORT
    EB_U32  unpack_chunk_size = 64;
    EB_U32  unpack_offset = 32;
    EB_U32  unpack_complOffset = 32;
#else
    EB_U32  unpack_chunk_size = 32;
    EB_U32  unpack_offset = 16;
    EB_U32  unpack_complOffset = 16;
#endif

/***********************************************
**** Copy the input buffer from the
**** sample application to the library buffers
************************************************/
static EB_ERRORTYPE CopyFrameBuffer(
    SequenceControlSet_t        *sequenceControlSetPtr,
    EB_U8      			        *dst,
    EB_U8      			        *src,
    UnPackContext_t           *context_unpack)
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

    // verfify stride values are within range

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

        if (lumaWidth > lumaStride || lumaWidth > sourceLumaStride || chromaWidth > chromaStride) {
            return EB_ErrorBadParameter;
        }


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

            if (lumaWidth > lumaStride || lumaWidth > sourceLumaStride || chromaWidth > chromaStride) {
                return EB_ErrorBadParameter;
            }

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

        EB_U32 lumaBufferOffset = (inputPicturePtr->strideY*sequenceControlSetPtr->topPadding + sequenceControlSetPtr->leftPadding);
        EB_U32 chromaBufferOffset = (inputPicturePtr->strideCr*(sequenceControlSetPtr->topPadding >> subHeightCMinus1) + (sequenceControlSetPtr->leftPadding >> subWidthCMinus1));
        EB_U16 lumaWidth = (EB_U16)(inputPicturePtr->width - sequenceControlSetPtr->maxInputPadRight);
        EB_U16 chromaWidth = (lumaWidth >> subWidthCMinus1);
        EB_U16 lumaHeight = (EB_U16)(inputPicturePtr->height - sequenceControlSetPtr->maxInputPadBottom);
        EB_U16 chromaHeight = lumaHeight >> subHeightCMinus1;

        EB_U16 sourceLumaStride = (EB_U16)(inputPtr->yStride);
        //EB_U16 sourceCrStride = (EB_U16)(inputPtr->crStride);
        EB_U16 sourceCbStride = (EB_U16)(inputPtr->cbStride);

        if (lumaWidth > sourceLumaStride || chromaWidth > sourceCbStride) {
            return EB_ErrorBadParameter;
        }

        EB_U32 numChunks_luma = (lumaWidth - (unpack_offset + unpack_complOffset)) / unpack_chunk_size;
        EB_U32 numChunks_chroma = (chromaWidth - (unpack_offset + unpack_complOffset)) / unpack_chunk_size;
        EB_U32 in_luma = lumaHeight * (unpack_offset + numChunks_luma * unpack_chunk_size + unpack_complOffset);
        EB_U32 out_luma = lumaHeight * (unpack_offset + numChunks_luma * unpack_chunk_size + inputPicturePtr->strideY - lumaWidth + unpack_complOffset);
        EB_U32 in_chroma = chromaHeight * (unpack_offset + numChunks_chroma * unpack_chunk_size + unpack_complOffset);
        EB_U32 out_chroma = chromaHeight * (unpack_offset + numChunks_chroma * unpack_chunk_size + inputPicturePtr->strideCb - chromaWidth + unpack_complOffset);
        int nb_luma = context_unpack->nbLumaThreads;
        int nb_chroma = context_unpack->nbChromaThreads;
        int nb = context_unpack->nbLumaThreads + context_unpack->nbChromaThreads*2;
        EbObjectWrapper_t *copyFrameBufferWrapperPtr;
        EBUnPack2DType_t *unpackDataPtr;

         for (int i = 0; i< nb_luma; i++){
            EbGetEmptyObject(context_unpack->copyFrameInputFifoPtr,&copyFrameBufferWrapperPtr);
            unpackDataPtr                  = (EBUnPack2DType_t*)copyFrameBufferWrapperPtr->objectPtr;
            unpackDataPtr->in16BitBuffer   = (EB_U16*)(inputPtr->luma) + (EB_U32)(in_luma/nb_luma)*i;
            unpackDataPtr->inStride        = (EB_U16)(inputPtr->yStride);
            unpackDataPtr->out8BitBuffer   = inputPicturePtr->bufferY + lumaBufferOffset + (EB_U32)(out_luma/nb_luma)*i;
            unpackDataPtr->out8Stride      = inputPicturePtr->strideY;
            unpackDataPtr->outnBitBuffer   = inputPicturePtr->bufferBitIncY + lumaBufferOffset + (EB_U32)(out_luma/nb_luma)*i;
            unpackDataPtr->outnStride      = inputPicturePtr->strideBitIncY;
            unpackDataPtr->width           = lumaWidth;
            unpackDataPtr->height          = lumaHeight/nb_luma;
            EbPostFullObject(copyFrameBufferWrapperPtr);
        }

        for(int i = 0;i< nb_chroma; i++){
            EbGetEmptyObject(context_unpack->copyFrameInputFifoPtr,&copyFrameBufferWrapperPtr);
            unpackDataPtr                      = (EBUnPack2DType_t*)copyFrameBufferWrapperPtr->objectPtr;
            unpackDataPtr->in16BitBuffer       = (EB_U16*)(inputPtr->cb) + (EB_U32)(in_chroma/nb_chroma)*i;
            unpackDataPtr->inStride            = (EB_U16)(inputPtr->cbStride);
            unpackDataPtr->out8BitBuffer       = inputPicturePtr->bufferCb + chromaBufferOffset + (EB_U32)(out_chroma/nb_chroma)*i;
            unpackDataPtr->out8Stride          = inputPicturePtr->strideCb;
            unpackDataPtr->outnBitBuffer       = inputPicturePtr->bufferBitIncCb + chromaBufferOffset + (EB_U32)(out_chroma/nb_chroma)*i;
            unpackDataPtr->outnStride          = inputPicturePtr->strideBitIncCb;
            unpackDataPtr->width               = chromaWidth;
            unpackDataPtr->height              = chromaHeight/nb_chroma;
            EbPostFullObject(copyFrameBufferWrapperPtr);
        }

        for(int i = 0;i< nb_chroma;i++){
            EbGetEmptyObject(context_unpack->copyFrameInputFifoPtr,&copyFrameBufferWrapperPtr);
            unpackDataPtr                      = (EBUnPack2DType_t*)copyFrameBufferWrapperPtr->objectPtr;
            unpackDataPtr->in16BitBuffer       = (EB_U16*)(inputPtr->cr) + (EB_U32)(in_chroma/nb_chroma)*i;
            unpackDataPtr->inStride            = (EB_U16)(inputPtr->crStride);
            unpackDataPtr->out8BitBuffer       = inputPicturePtr->bufferCr + chromaBufferOffset  + (EB_U32)(out_chroma/nb_chroma)*i;
            unpackDataPtr->out8Stride          = inputPicturePtr->strideCr;
            unpackDataPtr->outnBitBuffer       = inputPicturePtr->bufferBitIncCr + chromaBufferOffset  + (EB_U32)(out_chroma/nb_chroma)*i;
            unpackDataPtr->outnStride          = inputPicturePtr->strideBitIncCr;
            unpackDataPtr->width               = chromaWidth;
            unpackDataPtr->height              = chromaHeight/nb_chroma;
            EbPostFullObject(copyFrameBufferWrapperPtr);
        }

        EbObjectWrapper_t   *unpackEndSyncWrapperPtr;
        for(int i = 0 ; i < nb; i++){
            EbGetFullObject(context_unpack->unPackOutPutFifoPtr,&unpackEndSyncWrapperPtr);
            EbReleaseObject(unpackEndSyncWrapperPtr);
        }
    }

    // Copy Dolby Vision RPU metadata from input
    if (inputPtr->dolbyVisionRpu.payloadSize) {
        inputPicturePtr->dolbyVisionRpu.payloadSize = inputPtr->dolbyVisionRpu.payloadSize;
        EB_MEMCPY(inputPicturePtr->dolbyVisionRpu.payload, inputPtr->dolbyVisionRpu.payload, inputPtr->dolbyVisionRpu.payloadSize);
    }
    else {
        inputPicturePtr->dolbyVisionRpu.payloadSize = 0;
    }

    return return_error;
}

static EB_ERRORTYPE  CopyInputBuffer(
    SequenceControlSet_t*    sequenceControlSet,
    EB_BUFFERHEADERTYPE*     dst,
    EB_BUFFERHEADERTYPE*     src,
    UnPackContext_t*         context_unpack
)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    // Copy the higher level structure
    dst->nAllocLen   = src->nAllocLen;
    dst->nFilledLen  = src->nFilledLen;
    dst->pAppPrivate = src->pAppPrivate;
    dst->nFlags      = src->nFlags;
    dst->pts         = src->pts;
    dst->nTickCount  = src->nTickCount;
    dst->nSize       = src->nSize;
    dst->qpValue     = src->qpValue;
    dst->sliceType   = src->sliceType;

    // Copy the picture buffer
    if(src->pBuffer != NULL)
        return_error = CopyFrameBuffer(sequenceControlSet, dst->pBuffer, src->pBuffer, context_unpack);

    if (return_error != EB_ErrorNone)
        return return_error;


    // Copy User SEI
    if (src->pBuffer != NULL)
        CopyUserSei(sequenceControlSet, dst, src);

    if (src->segmentOvPtr != NULL && dst->segmentOvPtr != NULL && sequenceControlSet->staticConfig.segmentOvEnabled) {
        EB_MEMCPY(dst->segmentOvPtr, src->segmentOvPtr, sequenceControlSet->lcuTotalCount * sizeof(SegmentOverride_t));
    }

    return return_error;

}

/**********************************
 * Empty This Buffer
 **********************************/
#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265EncSendPicture(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE   *pBuffer)
{
    EbEncHandle_t          *encHandlePtr = (EbEncHandle_t*) h265EncComponent->pComponentPrivate;
    EbObjectWrapper_t      *ebWrapperPtr;

    EB_ERRORTYPE return_error = EB_ErrorNone;

    // Take the buffer and put it into our internal queue structure
    EbGetEmptyObject(
        encHandlePtr->inputBufferProducerFifoPtrArray[0],
        &ebWrapperPtr);

    if (pBuffer != NULL) {

        return_error = CopyInputBuffer(
            encHandlePtr->sequenceControlSetInstanceArray[0]->sequenceControlSetPtr,
            (EB_BUFFERHEADERTYPE*)ebWrapperPtr->objectPtr,
            pBuffer,
            (UnPackContext_t*)encHandlePtr->unpackContextPtrArray[0]);

        if (return_error != EB_ErrorNone)
        {
            return return_error;
        }
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
#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
EB_API EB_ERRORTYPE EbH265GetPacket(
    EB_COMPONENTTYPE      *h265EncComponent,
    EB_BUFFERHEADERTYPE  **pBuffer,
    uint8_t                picSendDone)
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

#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
EB_API void EbH265ReleaseOutBuffer(
    EB_BUFFERHEADERTYPE  **pBuffer)
{
    if ((*pBuffer)->pBuffer) {
        free((*pBuffer)->pBuffer);
        (*pBuffer)->pBuffer = EB_NULL;
    }

    if ((*pBuffer)->wrapperPtr)
        // Release out put buffer back into the pool
        EbReleaseObject((EbObjectWrapper_t  *)(*pBuffer)->wrapperPtr);

    return;
}

/**********************************
* Fill This Buffer
**********************************/
#ifdef __GNUC__
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
    SVT_LOG("SVT [version]:\tSVT-HEVC Encoder Lib v%d.%d.%d\n", SVT_VERSION_MAJOR, SVT_VERSION_MINOR,SVT_VERSION_PATCHLEVEL);
#ifdef _MSC_VER
#if _MSC_VER < 1910
    SVT_LOG("SVT [build]  : Visual Studio 2013");
#elif (_MSC_VER >= 1910) && (_MSC_VER < 1920)
    SVT_LOG("SVT [build]  :\tVisual Studio 2017");
#elif (_MSC_VER >= 1920)
    SVT_LOG("SVT [build]  :\tVisual Studio 2019");
#else
    SVT_LOG("SVT [build]  :\tUnknown Visual Studio Version");
#endif
#elif defined(__GNUC__)
    SVT_LOG("SVT [build]  :\tGCC %s\t", __VERSION__);
#else
    SVT_LOG("SVT [build]  :\tunknown compiler");
#endif
    SVT_LOG(" %u bit\n", (unsigned) sizeof(void*) * 8);
    SVT_LOG("LIB Build date: %s %s\n",__DATE__,__TIME__);
    SVT_LOG("-------------------------------------------\n");

    EB_COMPONENTTYPE  *h265EncComponent = (EB_COMPONENTTYPE*)hComponent;
    // Set Component Size & Version
    h265EncComponent->nSize                     = sizeof(EB_COMPONENTTYPE);

    // Encoder Private Handle Ctor
    EbEncHandle_t *handle;
    EB_NEW(handle, EbEncHandleCtor, h265EncComponent);
    h265EncComponent->pComponentPrivate = handle;

    return EB_ErrorNone;
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

    EbPictureBufferDesc_t* buf;
    // Enhanced Picture Buffer
    EB_NEW(
        buf,
        EbPictureBufferDescCtor,
        (EB_PTR)&inputPictureBufferDescInitData);
    inputBuffer->pBuffer = (uint8_t*)buf;

    if (is16bit && config->compressedTenBitFormat == 1) {
        const EB_COLOR_FORMAT colorFormat = (EB_COLOR_FORMAT)sequenceControlSetPtr->chromaFormatIdc;
        //pack 4 2bit pixels into 1Byte
        EB_MALLOC_ALIGNED_ARRAY(buf->bufferBitIncY, (inputPictureBufferDescInitData.maxWidth * inputPictureBufferDescInitData.maxHeight / 4));
        EB_MALLOC_ALIGNED_ARRAY(buf->bufferBitIncCb, (inputPictureBufferDescInitData.maxWidth * inputPictureBufferDescInitData.maxHeight / 4) >> (3 - colorFormat));
        EB_MALLOC_ALIGNED_ARRAY(buf->bufferBitIncCr, (inputPictureBufferDescInitData.maxWidth * inputPictureBufferDescInitData.maxHeight / 4) >> (3 - colorFormat));
    }

    return return_error;
}


/**************************************
* EB_BUFFERHEADERTYPE Constructor
**************************************/
EB_ERRORTYPE EbInputBufferHeaderCreator(
    EB_PTR *objectDblPtr,
    EB_PTR  objectInitDataPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_BUFFERHEADERTYPE* inputBuffer;
    SequenceControlSet_t        *sequenceControlSetPtr = (SequenceControlSet_t*)objectInitDataPtr;

    EB_CALLOC(inputBuffer, 1, sizeof(EB_BUFFERHEADERTYPE));
    *objectDblPtr = (EB_PTR)inputBuffer;
    // Initialize Header
    inputBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);

    return_error = AllocateFrameBuffer(
        sequenceControlSetPtr,
        inputBuffer);
    if (return_error != EB_ErrorNone)
        return return_error;

    if (sequenceControlSetPtr->staticConfig.segmentOvEnabled) {
        EB_MALLOC_ARRAY(inputBuffer->segmentOvPtr, sequenceControlSetPtr->lcuTotalCount);
    }

    return EB_ErrorNone;
}

void EbInputBufferHeaderDestroyer(EB_PTR p)
{
    EB_BUFFERHEADERTYPE *obj = (EB_BUFFERHEADERTYPE*)p;
    EbPictureBufferDesc_t* buf = (EbPictureBufferDesc_t*)obj->pBuffer;
    if (buf) {
        EB_FREE_ALIGNED_ARRAY(buf->bufferBitIncY);
        EB_FREE_ALIGNED_ARRAY(buf->bufferBitIncCb);
        EB_FREE_ALIGNED_ARRAY(buf->bufferBitIncCr);
    }

    EB_FREE_ARRAY(obj->segmentOvPtr);
    EB_DELETE(buf);
    EB_FREE(obj);
}

/**************************************
 * EB_BUFFERHEADERTYPE Constructor
 **************************************/
EB_ERRORTYPE EbOutputBufferHeaderCreator(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr)
{
    EB_H265_ENC_CONFIGURATION   * config = (EB_H265_ENC_CONFIGURATION*)objectInitDataPtr;
    EB_U32 nStride = (EB_U32)(EB_OUTPUTSTREAMBUFFERSIZE_MACRO(config->sourceWidth * config->sourceHeight));  //TBC

    EB_BUFFERHEADERTYPE* outBufPtr;
    EB_CALLOC(outBufPtr, 1, sizeof(EB_BUFFERHEADERTYPE));
    *objectDblPtr = (EB_PTR)outBufPtr;

    //Jing:TODO
    //Simple work around here, for 8K case.
    //Will improve here if memory is limited
    //Can use fps/tbr/intra_period to compute a ideal maximum size
    if (config->rateControlMode == 1 && config->targetBitRate >= 50000000) {
        nStride = 10000000;
    }

	// Initialize Header
    // pBuffer is allocated in PK
	outBufPtr->nSize = sizeof(EB_BUFFERHEADERTYPE);
	outBufPtr->nAllocLen =  nStride;

    return EB_ErrorNone;
}

void EbOutputBufferHeaderDestroyer(EB_PTR p)
{
    EB_BUFFERHEADERTYPE* obj = (EB_BUFFERHEADERTYPE*)p;
    EB_FREE(obj);
}

/**************************************
* EB_BUFFERHEADERTYPE Constructor
**************************************/
EB_ERRORTYPE EbOutputReconBufferHeaderCreator(
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

    EB_CALLOC(reconBuffer, 1, sizeof(EB_BUFFERHEADERTYPE));
    *objectDblPtr = (EB_PTR)reconBuffer;
    // Initialize Header
    reconBuffer->nSize = sizeof(EB_BUFFERHEADERTYPE);

    // Assign the variables
    EB_MALLOC(reconBuffer->pBuffer, frameSize);

    reconBuffer->nAllocLen   = frameSize;

    return EB_ErrorNone;
}

void EbOutputReconBufferHeaderDestroyer(EB_PTR p)
{
    EB_BUFFERHEADERTYPE *obj = (EB_BUFFERHEADERTYPE*)p;
    EB_FREE(obj->pBuffer);
    EB_FREE(obj);
}
