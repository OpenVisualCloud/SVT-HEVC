/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbDefinitions_h
#define EbDefinitions_h

//#define BENCHMARK 0
#define LATENCY_PROFILE 0
#include "EbApi.h"
#ifdef __cplusplus
extern "C" {
#endif

// Internal Marcos
#define NON_AVX512_SUPPORT

#ifdef __cplusplus
#define EB_EXTERN extern "C"
#else
#define EB_EXTERN
#endif // __cplusplus

#ifdef _MSC_VER
// Windows Compiler
#elif __INTEL_COMPILER
#else
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#if   (__GNUC__ > 6)
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough="
#endif
#endif 

#ifdef __GNUC__
#define AVX512_FUNC_TARGET __attribute__(( target( "avx512f,avx512dq,avx512bw,avx512vl" ) ))
#define AVX2_FUNC_TARGET   __attribute__(( target( "avx2" ) ))
#else
#define AVX512_FUNC_TARGET
#define AVX2_FUNC_TARGET
#endif // __GNUC__

#ifndef _RSIZE_T_DEFINED
    typedef size_t rsize_t;
#define _RSIZE_T_DEFINED
#endif  /* _RSIZE_T_DEFINED */

#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
    typedef int errno_t;
#endif  /* _ERRNO_T_DEFINED */




//Maximum 8192x4320
#define EB_TILE_COLUMN_MAX_COUNT                    20u
#define EB_TILE_ROW_MAX_COUNT                       22u
#define EB_TILE_MAX_COUNT                           440u

#define EB_MIN(a,b)             (((a) < (b)) ? (a) : (b))

#ifdef _WIN32 
#ifndef __cplusplus
#define inline __inline 
#endif
#elif __GNUC__ 
#define inline __inline__
#else
#error OS not supported 
#endif

#ifdef	_MSC_VER
#define FORCE_INLINE            __forceinline
#pragma warning( disable : 4068 ) // unknown pragma
#else
#ifdef __cplusplus
#define FORCE_INLINE            inline   __attribute__((always_inline))
#else
#define FORCE_INLINE            __inline __attribute__((always_inline))
#endif
#endif

#define INPUT_SIZE_576p_TH				0x90000     // 0.58 Million   
#define INPUT_SIZE_1080i_TH				0xB71B0     // 0.75 Million
#define INPUT_SIZE_1080p_TH				0x1AB3F0    // 1.75 Million
#define INPUT_SIZE_4K_TH				0x29F630    // 2.75 Million   
#define INPUT_SIZE_8K_TH				0xB71B00    // 12 Million

#define EB_INPUT_RESOLUTION             EB_U8
#define INPUT_SIZE_576p_RANGE_OR_LOWER	 0
#define INPUT_SIZE_1080i_RANGE			 1
#define INPUT_SIZE_1080p_RANGE	         2
#define INPUT_SIZE_4K_RANGE			     3
    
/** The EB_GOP type is used to describe the hierarchical coding structure of
Groups of Pictures (GOP) units.
*/
#define EB_PRED                 EB_U8
#define EB_PRED_LOW_DELAY_P     0
#define EB_PRED_LOW_DELAY_B     1
#define EB_PRED_RANDOM_ACCESS   2
#define EB_PRED_TOTAL_COUNT     3
#define EB_PRED_INVALID         0xFF
    
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
typedef enum EB_ASM {
    EB_ASM_C,
    EB_ASM_AUTO,
    EB_ASM_TYPE_TOTAL,
    EB_ASM_TYPE_INVALID = ~0
} EB_ASM;

/** The EB_BOOL type is intended to be used to represent a true or a false
value when passing parameters to and from the svt API.  The
EB_BOOL is a 32 bit quantity and is aligned on a 32 bit word boundary.
*/

#define EB_BOOL   EB_U8
#define EB_FALSE  0
#define EB_TRUE   1

/** The EB_HANDLETYPE type is intended to be used to pass pointers to and from the svt
API.  This is a 32 bit pointer and is aligned on a 32 bit word boundary.
*/
typedef void* EB_HANDLETYPE;

/** The EB_BYTE type is intended to be used to pass arrays of bytes such as
buffers to and from the svt API.  The EB_BYTE type is a 32 bit pointer.
The pointer is word aligned and the buffer is byte aligned.
*/
typedef EB_U8 * EB_BYTE;

/** The EB_BITDEPTH type is used to describe the bitdepth of video data.
*/
typedef enum EB_BITDEPTH {
	EB_8BIT = 8,
	EB_10BIT = 10,
	EB_12BIT = 12,
	EB_14BIT = 14,
	EB_16BIT = 16
} EB_BITDEPTH;

typedef enum EB_RDOQ_PMCORE_TYPE {
    EB_NO_RDOQ = 0,
    EB_RDOQ,
    EB_PMCORE,
    EB_LIGHT,
} EB_RDOQ_PMCORE_TYPE;
    
typedef enum EbPtrType{
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

typedef struct EB_PARAM_PORTDEFINITIONTYPE {
    EB_U32 nFrameWidth;
    EB_U32 nFrameHeight;
    EB_S32 nStride;
    EB_U32 nSize;
} EB_PARAM_PORTDEFINITIONTYPE;

/* string copy */
extern errno_t strcpy_ss(char *dest, rsize_t dmax, const char *src);

/* fitted string copy */
extern errno_t strncpy_ss(char *dest, rsize_t dmax, const char *src, rsize_t slen);

/* string length */
extern rsize_t strnlen_ss(const char *s, rsize_t smax);

/********************************************************************************************
* faster memcopy for <= 64B blocks, great w/ inlining and size known at compile time (or w/ PGO)
* THIS NEEDS TO STAY IN A HEADER FOR BEST PERFORMANCE
********************************************************************************************/

#include <immintrin.h>

#ifdef __GNUC__
__attribute__((optimize("unroll-loops")))
#endif
FORCE_INLINE void eb_memcpy_small(void* dst_ptr, void const* src_ptr, size_t size)
{
    const char* src = (const char*)src_ptr;
    char*       dst = (char*)dst_ptr;
    size_t      i = 0;

#ifdef _INTEL_COMPILER
#pragma unroll
#endif
    while ((i + 16) <= size)
    {
        _mm_storeu_ps((float*)(dst + i), _mm_loadu_ps((const float*)(src + i)));
        i += 16;
    }

    if ((i + 8) <= size)
    {
        _mm_store_sd((double*)(dst + i), _mm_load_sd((const double*)(src + i)));
        i += 8;
    }

    for (; i < size; ++i)
        dst[i] = src[i];
}

FORCE_INLINE void eb_memcpy_SSE(void* dst_ptr, void const* src_ptr, size_t size)
{
    const char* src = (const char*)src_ptr;
    char*       dst = (char*)dst_ptr;
    size_t      i = 0;
    size_t align_cnt = EB_MIN((64 - ((size_t)dst & 63)), size);


    // align dest to a $line
    if (align_cnt != 64)
    {
        eb_memcpy_small(dst, src, align_cnt);
        dst += align_cnt;
        src += align_cnt;
        size -= align_cnt;
    }

    // copy a $line at a time
    // dst aligned to a $line
    size_t cline_cnt = (size & ~(size_t)63);
    for (i = 0; i < cline_cnt; i += 64)
    {

        __m128 c0 = _mm_loadu_ps((const float*)(src + i));
        __m128 c1 = _mm_loadu_ps((const float*)(src + i + sizeof(c0)));
        __m128 c2 = _mm_loadu_ps((const float*)(src + i + sizeof(c0) * 2));
        __m128 c3 = _mm_loadu_ps((const float*)(src + i + sizeof(c0) * 3));

        _mm_storeu_ps((float*)(dst + i), c0);
        _mm_storeu_ps((float*)(dst + i + sizeof(c0)), c1);
        _mm_storeu_ps((float*)(dst + i + sizeof(c0) * 2), c2);
        _mm_storeu_ps((float*)(dst + i + sizeof(c0) * 3), c3);

    }

    // copy the remainder
    if (i < size)
        eb_memcpy_small(dst + i, src + i, size - i);
}

FORCE_INLINE void eb_memcpy(void  *dstPtr, void  *srcPtr, size_t size)
{
    if (size > 64) {
        eb_memcpy_SSE(dstPtr, srcPtr, size);
    }
    else
    {
        eb_memcpy_small(dstPtr, srcPtr, size);
    }
}

#define EB_MEMCPY(dst, src, size) \
	eb_memcpy(dst, src, size)

#define EB_MEMSET(dst, val, count) \
	memset(dst, val, count)

// Used to hide GCC warnings for unused function tables
#ifdef __GNUC__
#define FUNC_TABLE __attribute__ ((unused))
#else
#define FUNC_TABLE
#endif
#define EB_MAX_TEMPORAL_LAYERS                  MAX_TEMPORAL_LAYERS


// Reserved types for lib's internal use. Must be less than EB_EXT_TYPE_BASE
#define       EB_TYPE_UNREG_USER_DATA_SEI    3
#define       EB_TYPE_REG_USER_DATA_SEI      4
#define       EB_TYPE_PIC_STRUCT             5             // It is a requirement (for the application) that if pictureStruct is present for 1 picture it shall be present for every picture


#define	Log2f					          Log2f_SSE2
extern EB_U32 Log2f(EB_U32 x);


/** The EB_PICT_STRUCT type is used to describe the picture structure.
*/
#define EB_PICT_STRUCT           EB_U8
#define PROGRESSIVE_PICT_STRUCT  0
#define TOP_FIELD_PICT_STRUCT    1
#define BOTTOM_FIELD_PICT_STRUCT 2


/** The EB_MODETYPE type is used to describe the PU type.
*/
typedef EB_U8 EB_MODETYPE;
#define INTER_MODE 1
#define INTRA_MODE 2

#define INVALID_MODE 0xFFu

#define PREAVX2_MASK    1
#define AVX2_MASK       2
#ifndef NON_AVX512_SUPPORT
#define AVX512_MASK     4
#endif
#define ASM_AVX2_BIT    3

/** INTRA_4x4 offsets
*/
static const EB_U8 INTRA_4x4_OFFSET_X[4] = { 0, 4, 0, 4 };
static const EB_U8 INTRA_4x4_OFFSET_Y[4] = { 0, 0, 4, 4 };

extern EB_U32 ASM_TYPES;

/** Depth offsets
*/
static const EB_U8 DepthOffset[4]       = { 85, 21, 5, 1 };

/** ME 2Nx2N offsets
*/
static const EB_U32 me2Nx2NOffset[4]    = { 0, 1, 5, 21 };

/** The EB_ENC_MODE type is used to describe the encoder mode .
*/

#define EB_ENC_MODE         EB_U8
#define ENC_MODE_0          0
#define ENC_MODE_1          1
#define ENC_MODE_2          2
#define ENC_MODE_3          3
#define ENC_MODE_4          4
#define ENC_MODE_5          5
#define ENC_MODE_6          6
#define ENC_MODE_7          7
#define ENC_MODE_8          8
#define ENC_MODE_9          9
#define ENC_MODE_10         10
#define ENC_MODE_11         11
#define ENC_MODE_12         12

#define MAX_SUPPORTED_MODES 13

#define MAX_SUPPORTED_MODES_SUB1080P    10
#define MAX_SUPPORTED_MODES_1080P       11
#define MAX_SUPPORTED_MODES_4K_OQ       11
#define MAX_SUPPORTED_MODES_4K_SQ       13

#define SPEED_CONTROL_INIT_MOD ENC_MODE_5;


/** The EB_TUID type is used to identify a TU within a CU.
*/

#define EB_REFLIST            EB_U8
#define REF_LIST_0             0
#define REF_LIST_1             1
#define TOTAL_NUM_OF_REF_LISTS 2
#define INVALID_LIST           0xFF

#define EB_PREDDIRECTION         EB_U8
#define UNI_PRED_LIST_0          0
#define UNI_PRED_LIST_1          1
#define BI_PRED                  2
#define EB_PREDDIRECTION_TOTAL   3
#define INVALID_PRED_DIRECTION   0xFF


#define UNI_PRED_LIST_0_MASK    (1 << UNI_PRED_LIST_0)
#define UNI_PRED_LIST_1_MASK    (1 << UNI_PRED_LIST_1)
#define BI_PRED_MASK            (1 << BI_PRED)


// The EB_QP_OFFSET_MODE type is used to describe the QP offset
#define EB_FRAME_CARACTERICTICS EB_U8
#define EB_FRAME_CARAC_0           0		
#define EB_FRAME_CARAC_1           1		
#define EB_FRAME_CARAC_2           2		
#define EB_FRAME_CARAC_3           3		
#define EB_FRAME_CARAC_4           4


/** The EB_PART_MODE type is used to describe the CU partition size.
*/
typedef EB_U8 EB_PART_MODE;
#define SIZE_2Nx2N 0  
#define SIZE_2NxN  1    
#define SIZE_Nx2N  2    
#define SIZE_NxN   3
#define SIZE_2NxnU 4
#define SIZE_2NxnD 5
#define SIZE_nLx2N 6
#define SIZE_nRx2N 7
#define SIZE_PART_MODE 8

// Rate Control
#define THRESHOLD1QPINCREASE     0
#define THRESHOLD2QPINCREASE     1

#define EB_IOS_POINT            EB_U8
#define OIS_VERY_FAST_MODE       0
#define OIS_FAST_MODE            1
#define OIS_MEDUIM_MODE          2
#define OIS_COMPLEX_MODE         3
#define OIS_VERY_COMPLEX_MODE    4

#define _MVXT(mv) ( (EB_S16)((mv) &  0xFFFF) ) 
#define _MVYT(mv) ( (EB_S16)((mv) >> 16    ) ) 

//WEIGHTED PREDICTION

#define  WP_SHIFT1D_10BIT      2   
#define  WP_OFFSET1D_10BIT     (-32768)

#define  WP_SHIFT2D_10BIT      6   
#define  WP_OFFSET2D_10BIT     0

#define  WP_IF_OFFSET_10BIT    8192//2^(14-1)

#define  WP_SHIFT_10BIT        4   // 14 - 10 
#define  WP_BI_SHIFT_10BIT     5

// Will contain the EbEncApi which will live in the EncHandle class
// Modifiable during encode time.
typedef struct EB_H265_DYN_ENC_CONFIGURATION
{
    // Define the settings

    // Rate Control
    EB_U32              availableTargetBitRate;


} EB_H265_DYN_ENC_CONFIGURATION;


/** The EB_INTRA_REFRESH_TYPE is used to describe the intra refresh type.
*/
typedef enum EB_INTRA_REFRESH_TYPE {
    NO_REFRESH = 0,
    CRA_REFRESH = 1,
    IDR_REFRESH = 2
}EB_INTRA_REFRESH_TYPE;

#if defined(_MSC_VER)
#define EB_ALIGN(n) __declspec(align(n))
#elif defined(__GNUC__)
#define EB_ALIGN(n) __attribute__((__aligned__(n)))
#else
#define EB_ALIGN(n)
#endif

/** The EB_HANDLE type is used to define OS object handles for threads,
semaphores, mutexs, etc.
*/
typedef void * EB_HANDLE;

#define MAX_NUM_PTR                (0x1312D00 << 2) //0x4C4B4000            // Maximum number of pointers to be allocated for the library

#define ALVALUE                     32

extern    EbMemoryMapEntry        *memoryMap;               // library Memory table
extern    EB_U32                  *memoryMapIndex;          // library memory index
extern    EB_U64                  *totalLibMemory;          // library Memory malloc'd

extern    EB_U32                   libMallocCount;
extern    EB_U32                   libThreadCount;
extern    EB_U32                   libSemaphoreCount;
extern    EB_U32                   libMutexCount;


#ifdef _WIN32
#define EB_ALLIGN_MALLOC(type, pointer, nElements, pointerClass) \
    pointer = (type) _aligned_malloc(nElements,ALVALUE); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
	    } \
	    else { \
        memoryMap[*(memoryMapIndex)].ptrType = pointerClass; \
        memoryMap[(*(memoryMapIndex))++].ptr = pointer; \
		if (nElements % 8 == 0) { \
			*totalLibMemory += (nElements); \
		} \
		else { \
			*totalLibMemory += ((nElements) + (8 - ((nElements) % 8))); \
		} \
    } \
    if (*(memoryMapIndex) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    libMallocCount++;

#else
#define EB_ALLIGN_MALLOC(type, pointer, nElements, pointerClass) \
    if (posix_memalign((void**)(&(pointer)), ALVALUE, nElements) != 0) { \
        return EB_ErrorInsufficientResources; \
    	    } \
        	    else { \
        pointer = (type) pointer;  \
        memoryMap[*(memoryMapIndex)].ptrType = pointerClass; \
        memoryMap[(*(memoryMapIndex))++].ptr = pointer; \
		if (nElements % 8 == 0) { \
			*totalLibMemory += (nElements); \
        		} \
        		else { \
			*totalLibMemory += ((nElements) + (8 - ((nElements) % 8))); \
		} \
    } \
    if (*(memoryMapIndex) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
        } \
    libMallocCount++;     
#endif

// Debug Macros
#define OVERSHOOT_STAT_PRINT             0  // Do not remove. 
                                            // For printing overshooting percentages for both RC and fixed QP. 
                                            // Target rate and and max buffer size should be set properly even for fixed QP.
                                            // Disabled by default. 
#define DEADLOCK_DEBUG                   0
#define DISPLAY_MEMORY                   0  // Display Total Memory at the end of the memory allocations
#define LIB_PRINTF_ENABLE                1
#if LIB_PRINTF_ENABLE
#define SVT_LOG printf
#else
#if _MSC_VER
#define SVT_LOG(s, ...) printf("")
#else
#define SVT_LOG(s, ...) printf("",##__VA_ARGS__)
#endif
#endif

#define EB_MEMORY() \
    SVT_LOG("Total Number of Mallocs in Library: %d\n", libMallocCount); \
    SVT_LOG("Total Number of Threads in Library: %d\n", libThreadCount); \
    SVT_LOG("Total Number of Semaphore in Library: %d\n", libSemaphoreCount); \
    SVT_LOG("Total Number of Mutex in Library: %d\n", libMutexCount); \
    SVT_LOG("Total Library Memory: %.2lf KB\n\n",*totalLibMemory/(double)1024);

#define EB_MALLOC(type, pointer, nElements, pointerClass) \
    pointer = (type) malloc(nElements); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
	    } \
	    else { \
        memoryMap[*(memoryMapIndex)].ptrType = pointerClass; \
        memoryMap[(*(memoryMapIndex))++].ptr = pointer; \
		if (nElements % 8 == 0) { \
			*totalLibMemory += (nElements); \
		} \
		else { \
			*totalLibMemory += ((nElements) + (8 - ((nElements) % 8))); \
		} \
    } \
    if (*(memoryMapIndex) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    libMallocCount++;

#define EB_CALLOC(type, pointer, count, size, pointerClass) \
    pointer = (type) calloc(count, size); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
    } \
    else { \
        memoryMap[*(memoryMapIndex)].ptrType = pointerClass; \
        memoryMap[(*(memoryMapIndex))++].ptr = pointer; \
		if (count % 8 == 0) { \
			*totalLibMemory += (count); \
		} \
		else { \
			*totalLibMemory += ((count) + (8 - ((count) % 8))); \
		} \
    } \
    if (*(memoryMapIndex) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    libMallocCount++;

#define EB_CREATESEMAPHORE(type, pointer, nElements, pointerClass, initialCount, maxCount) \
    pointer = EbCreateSemaphore(initialCount, maxCount); \
    if (pointer == (type)EB_NULL) { \
        return EB_ErrorInsufficientResources; \
    } \
    else { \
        memoryMap[*(memoryMapIndex)].ptrType = pointerClass; \
        memoryMap[(*(memoryMapIndex))++].ptr = pointer; \
		if (nElements % 8 == 0) { \
			*totalLibMemory += (nElements); \
		} \
		else { \
			*totalLibMemory += ((nElements) + (8 - ((nElements) % 8))); \
		} \
    } \
    if (*(memoryMapIndex) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    libSemaphoreCount++;

#define EB_CREATEMUTEX(type, pointer, nElements, pointerClass) \
    pointer = EbCreateMutex(); \
    if (pointer == (type)EB_NULL){ \
        return EB_ErrorInsufficientResources; \
    } \
    else { \
        memoryMap[*(memoryMapIndex)].ptrType = pointerClass; \
        memoryMap[(*(memoryMapIndex))++].ptr = pointer; \
		if (nElements % 8 == 0) { \
			*totalLibMemory += (nElements); \
		} \
		else { \
			*totalLibMemory += ((nElements) + (8 - ((nElements) % 8))); \
		} \
    } \
    if (*(memoryMapIndex) >= MAX_NUM_PTR) { \
        return EB_ErrorInsufficientResources; \
    } \
    libMutexCount++;

#define EB_STRDUP(dst, src) \
    EB_MALLOC_(char*, dst, strlen(src)+1, EB_N_PTR); \
    strcpy_ss((char*)dst, strlen(src)+1, src);

#ifdef _WIN32
#define EB_SCANF sscanf_s
#else
#define EB_SCANF sscanf
#endif

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

/** The EB_CTOR type is used to define the svt object constructors.
objectPtr is a EB_PTR to the object being constructed.
objectInitDataPtr is a EB_PTR to a data structure used to initialize the object.
*/
typedef EB_ERRORTYPE(*EB_CTOR)(
    EB_PTR *objectDblPtr,
    EB_PTR objectInitDataPtr);

/** The EB_DTOR type is used to define the svt object destructors.
objectPtr is a EB_PTR to the object being constructed.
*/
typedef void(*EB_DTOR)(
    EB_PTR objectPtr);

/**************************************
* Callback Functions
**************************************/
typedef struct EbCallback_s
{
    EB_PTR                                 appPrivateData;
    EB_PTR                                 handle;
    void(*ErrorHandler)(
        EB_PTR handle,
        EB_U32 errorCode);
} EbCallback_t;


/** NAL UNIT TYPES
*/
typedef enum NalUnitType
{

    NAL_UNIT_CODED_SLICE_TRAIL_N = 0, // 0
    NAL_UNIT_CODED_SLICE_TRAIL_R,     // 1

    NAL_UNIT_CODED_SLICE_TSA_N,       // 2
    NAL_UNIT_CODED_SLICE_TLA_R,       // 3

    NAL_UNIT_CODED_SLICE_STSA_N,      // 4
    NAL_UNIT_CODED_SLICE_STSA_R,      // 5

    NAL_UNIT_CODED_SLICE_RADL_N,      // 6
    NAL_UNIT_CODED_SLICE_RADL_R,      // 7

    NAL_UNIT_CODED_SLICE_RASL_N,      // 8
    NAL_UNIT_CODED_SLICE_RASL_R,      // 9

    NAL_UNIT_RESERVED_VCL_N10,
    NAL_UNIT_RESERVED_VCL_R11,
    NAL_UNIT_RESERVED_VCL_N12,
    NAL_UNIT_RESERVED_VCL_R13,
    NAL_UNIT_RESERVED_VCL_N14,
    NAL_UNIT_RESERVED_VCL_R15,

    NAL_UNIT_CODED_SLICE_BLA_W_LP,    // 16
    NAL_UNIT_CODED_SLICE_BLA_W_RADL,  // 17
    NAL_UNIT_CODED_SLICE_BLA_N_LP,    // 18
    NAL_UNIT_CODED_SLICE_IDR_W_RADL,  // 19
    NAL_UNIT_CODED_SLICE_IDR_N_LP,    // 20
    NAL_UNIT_CODED_SLICE_CRA,         // 21
    NAL_UNIT_RESERVED_IRAP_VCL22,
    NAL_UNIT_RESERVED_IRAP_VCL23,

    NAL_UNIT_RESERVED_VCL24,
    NAL_UNIT_RESERVED_VCL25,
    NAL_UNIT_RESERVED_VCL26,
    NAL_UNIT_RESERVED_VCL27,
    NAL_UNIT_RESERVED_VCL28,
    NAL_UNIT_RESERVED_VCL29,
    NAL_UNIT_RESERVED_VCL30,
    NAL_UNIT_RESERVED_VCL31,

    NAL_UNIT_VPS,                     // 32
    NAL_UNIT_SPS,                     // 33
    NAL_UNIT_PPS,                     // 34
    NAL_UNIT_ACCESS_UNIT_DELIMITER,   // 35
    NAL_UNIT_EOS,                     // 36
    NAL_UNIT_EOB,                     // 37
    NAL_UNIT_FILLER_DATA,             // 38
    NAL_UNIT_PREFIX_SEI,              // 39
    NAL_UNIT_SUFFIX_SEI,              // 40
    NAL_UNIT_RESERVED_NVCL41,
    NAL_UNIT_RESERVED_NVCL42,
    NAL_UNIT_RESERVED_NVCL43,
    NAL_UNIT_RESERVED_NVCL44,
    NAL_UNIT_RESERVED_NVCL45,
    NAL_UNIT_RESERVED_NVCL46,
    NAL_UNIT_RESERVED_NVCL47,
    NAL_UNIT_UNSPECIFIED_48,
    NAL_UNIT_UNSPECIFIED_49,
    NAL_UNIT_UNSPECIFIED_50,
    NAL_UNIT_UNSPECIFIED_51,
    NAL_UNIT_UNSPECIFIED_52,
    NAL_UNIT_UNSPECIFIED_53,
    NAL_UNIT_UNSPECIFIED_54,
    NAL_UNIT_UNSPECIFIED_55,
    NAL_UNIT_UNSPECIFIED_56,
    NAL_UNIT_UNSPECIFIED_57,
    NAL_UNIT_UNSPECIFIED_58,
    NAL_UNIT_UNSPECIFIED_59,
    NAL_UNIT_UNSPECIFIED_60,
    NAL_UNIT_UNSPECIFIED_61,
    NAL_UNIT_UNSPECIFIED_62,
    NAL_UNIT_UNSPECIFIED_63,
    NAL_UNIT_INVALID,

} NalUnitType;

typedef enum DIST_CALC_TYPE {
    DIST_CALC_RESIDUAL = 0,    // SSE(Coefficients - ReconCoefficients)
    DIST_CALC_PREDICTION = 1,    // SSE(Coefficients) *Note - useful in modes that don't send residual coeff bits
    DIST_CALC_TOTAL = 2
} DIST_CALC_TYPE;
#define BLKSIZE 64

typedef enum EB_SEI {

    BUFFERING_PERIOD = 0,
    PICTURE_TIMING = 1,
    REG_USER_DATA = 4,
    UNREG_USER_DATA = 5,
    RECOVERY_POINT = 6,
    DECODED_PICTURE_HASH = 132,
    PAN_SCAN_RECT = 2,
    FILLER_PAYLOAD = 3,
    USER_DATA_REGISTERED_ITU_T_T35 = 4,
    USER_DATA_UNREGISTERED = 5,
    SCENE_INFO = 9,
    FULL_FRAME_SNAPSHOT = 15,
    PROGRESSIVE_REFINEMENT_SEGMENT_START = 16,
    PROGRESSIVE_REFINEMENT_SEGMENT_END = 17,
    FILM_GRAIN_CHARACTERISTICS = 19,
    POST_FILTER_HINT = 22,
    TONE_MAPPING_INFO = 23,
    FRAME_PACKING = 45,
    DISPLAY_ORIENTATION = 47,
    SOP_DESCRIPTION = 128,
    ACTIVE_PARAMETER_SETS = 129,
    DECODING_UNIT_INFO = 130,
    TEMPORAL_LEVEL0_INDEX = 131,
    SCALABLE_NESTING = 133,
    REGION_REFRESH_INFO = 134,
    MASTERING_DISPLAY_INFO = 137,
    CONTENT_LIGHT_LEVEL_INFO = 144

} EB_SEI;

#define TUNE_SQ                 0
#define TUNE_OQ                 1
#define TUNE_VMAF               2


#define INTERPOLATION_FREE_PATH     0
#define INTERPOLATION_METHOD_HEVC   1


#define ME_FILTER_TAP                    4


#define    SUB_SAD_SEARCH   0
#define    FULL_SAD_SEARCH  1
#define    SSD_SEARCH       2


#define EB_INPUT_CLASS          EB_U8
#define INPUT_CLASS_C			 0
#define INPUT_CLASS_B			 1
#define INPUT_CLASS_TO_REMOVE	 2
#define INPUT_CLASS_A			 3


//***Profile, tier, level***
#define TOTAL_LEVEL_COUNT                           13

#define C1_TRSHLF_N       1
#define C1_TRSHLF_D       1
#define C2_TRSHLF_N       16
#define C2_TRSHLF_D       10


#define CHANGE_LAMBDA_FOR_AURA   0x01

#define ANTI_CONTOURING_TH_0     16 * 16
#define ANTI_CONTOURING_TH_1     32 * 32
#define ANTI_CONTOURING_TH_2 2 * 32 * 32

#define ANTI_CONTOURING_DELTA_QP_0  -3 
#define ANTI_CONTOURING_DELTA_QP_1  -9 
#define ANTI_CONTOURING_DELTA_QP_2  -11

#define ANTI_CONTOURING_LUMA_T1                40
#define ANTI_CONTOURING_LUMA_T2                180


#define VAR_BASED_DETAIL_PRESERVATION_SELECTOR_THRSLHD         (64*64)

#define PM_DC_TRSHLD1                       10 // The threshold for DC to disable masking for DC

#define MAX_BITS_PER_FRAME            8000000	

#define MEAN_DIFF_THRSHOLD              10
#define VAR_DIFF_THRSHOLD               10

#define SC_FRAMES_TO_IGNORE             100 // The speed control algorith starts after SC_FRAMES_TO_IGNORE number frames.
#define SC_FRAMES_INTERVAL_SPEED        60 // The speed control Interval To Check the speed
#define SC_FRAMES_INTERVAL_T1           60 // The speed control Interval Threshold1
#define SC_FRAMES_INTERVAL_T2          180 // The speed control Interval Threshold2
#define SC_FRAMES_INTERVAL_T3          120 // The speed control Interval Threshold3

#define EB_CMPLX_CLASS           EB_U8
#define CMPLX_LOW                0
#define CMPLX_MEDIUM             1
#define CMPLX_HIGH               2
#define CMPLX_VHIGH              3
#define CMPLX_NOISE              4

#define YBITS_THSHLD_1(x)                  ((x < ENC_MODE_12) ? 80 : 120)
#define YDC_THSHLD_1                        10
#define MASK_THSHLD_1                        1

//***Encoding Parameters***

#define MAX_PICTURE_WIDTH_SIZE                      9344u
#define MAX_PICTURE_HEIGHT_SIZE                     5120u

#define INTERNAL_BIT_DEPTH                          8 // to be modified
#define MAX_SAMPLE_VALUE                            ((1 << INTERNAL_BIT_DEPTH) - 1)
#define MAX_SAMPLE_VALUE_10BIT                      0x3FF
#define MAX_LCU_SIZE                                64u
#define MAX_LCU_SIZE_MINUS_1                        63u
#define LOG2F_MAX_LCU_SIZE                          6u
#define MAX_LCU_SIZE_REMAINING                      56u
#define MAX_LCU_SIZE_CHROMA                         32u
#define MAX_LOG2_LCU_SIZE                           6 // log2(MAX_LCU_SIZE)
#define MAX_LEVEL_COUNT                             5 // log2(MAX_LCU_SIZE) - log2(MIN_CU_SIZE)
#define MAX_TU_SIZE                                 32
#define MIN_TU_SIZE                                 4
#define MAX_TU_LEVEL_COUNT                          4 // log2(MAX_TU_SIZE) - log2(MIN_TU_SIZE)
#define MAX_TU_DEPTH                                2
#define LOG_MIN_CU_SIZE                             3
#define MIN_CU_SIZE                                 (1 << LOG_MIN_CU_SIZE)
#define MAX_CU_SIZE                                 64u
#define MAX_INTRA_SIZE                              32
#define MIN_INTRA_SIZE                              8
#define LOG_MIN_PU_SIZE                             2
#define MIN_PU_SIZE                                 (1 << LOG_MIN_PU_SIZE)
#define MAX_NUM_OF_PU_PER_CU                        1
#define MAX_NUM_OF_REF_PIC_LIST                     2
#define MAX_NUM_OF_PART_SIZE                        8
#define EB_MAX_LCU_DEPTH                            (((MAX_LCU_SIZE / MIN_CU_SIZE) == 1) ? 1 : \
                                                     ((MAX_LCU_SIZE / MIN_CU_SIZE) == 2) ? 2 : \
                                                     ((MAX_LCU_SIZE / MIN_CU_SIZE) == 4) ? 3 : \
                                                     ((MAX_LCU_SIZE / MIN_CU_SIZE) == 8) ? 4 : \
                                                     ((MAX_LCU_SIZE / MIN_CU_SIZE) == 16) ? 5 : \
                                                     ((MAX_LCU_SIZE / MIN_CU_SIZE) == 32) ? 6 : 7)
#define MIN_CU_BLK_COUNT                            ((MAX_LCU_SIZE / MIN_CU_SIZE) * (MAX_LCU_SIZE / MIN_CU_SIZE))
#define MAX_NUM_OF_TU_PER_CU                        21 
#define MIN_NUM_OF_TU_PER_CU                        5 
#define MAX_LCU_ROWS                                ((MAX_PICTURE_HEIGHT_SIZE) / (MAX_LCU_SIZE))

#define MAX_NUMBER_OF_TREEBLOCKS_PER_PICTURE       ((MAX_PICTURE_WIDTH_SIZE + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE) * \
                                                   ((MAX_PICTURE_HEIGHT_SIZE + MAX_LCU_SIZE - 1) / MAX_LCU_SIZE) 

#define MAX_AMVP_CANDIDATES_PER_REF_LIST            3
#define ZERO                                        0
#define AMVP0                                       1
#define AMVP1                                       2


//***Prediction Structure***
#define MAX_TEMPORAL_LAYERS                         6
#define MAX_HIERARCHICAL_LEVEL                      6
#define MAX_REF_IDX                                 1        // Set MAX_REF_IDX as 1 to avoid sending extra refPicIdc for each PU in IPPP flat GOP structure.
#define INVALID_POC                                 (((EB_U32) (~0)) - (((EB_U32) (~0)) >> 1))
#define MAX_ELAPSED_IDR_COUNT                       1024                       

//***Segments***
#define EB_SEGMENT_MIN_COUNT                        1
#define EB_SEGMENT_MAX_COUNT                        64

//***HME***
#define EB_HME_SEARCH_AREA_COLUMN_MAX_COUNT         2
#define EB_HME_SEARCH_AREA_ROW_MAX_COUNT            2

//***TMVP***
#define LOG_MV_COMPRESS_UNIT_SIZE                   4
#define MAX_TMVP_CAND_PER_LCU                       (MAX_LCU_SIZE >> LOG_MV_COMPRESS_UNIT_SIZE)*(MAX_LCU_SIZE >> LOG_MV_COMPRESS_UNIT_SIZE)

//***MV Merge***
#define MAX_NUM_OF_MV_MERGE_CANDIDATE               5 

//***AMVP***
#define MAX_NUM_OF_AMVP_CANDIDATES                  2

//***Mode Decision Candidate List***
#define MAX_MODE_DECISION_CATEGORY_NUM              6
#define LOG_MAX_AMVP_MODE_DECISION_CANDIDATE_NUM    2
#define MAX_AMVP_MODE_DECISION_CANDIDATE_NUM        (1 << LOG_MAX_AMVP_MODE_DECISION_CANDIDATE_NUM)

#define CU_MAX_COUNT                                85

#define EB_EVENT_MAX_COUNT                          20

#define MAX_INTRA_REFERENCE_SAMPLES                 (MAX_LCU_SIZE << 2) + 1

#define MAX_INTRA_MODES                             35

#define _MVXT(mv) ( (EB_S16)((mv) &  0xFFFF) ) 
#define _MVYT(mv) ( (EB_S16)((mv) >> 16    ) ) 


//***MCP***
#define MaxChromaFilterTag          4
#define MaxVerticalLumaFliterTag    8
#define MaxHorizontalLumaFliterTag  8

#define MCPXPaddingOffset           16                                    // to be modified 
#define MCPYPaddingOffset           16                                    // to be modified 

#define InternalBitDepth            8                                     // to be modified
#define MAX_Sample_Value            ((1 << InternalBitDepth) - 1)
#define IF_Shift                    6                                     // to be modified
#define IF_Prec                     14                                    // to be modified
#define IF_Negative_Offset          (IF_Prec - 1)                         // to be modified
#define InternalBitDepthIncrement   (InternalBitDepth - 8)


//***SAO***
#define SAO_BO_INTERVALS                                32
#define SAO_BO_LEN                                      4
#define SAO_EO_TYPES                                    4
#define SAO_EO_CATEGORIES                               4
#define MIN_SAO_OFFSET_VALUE                          -31// -7
#define MAX_SAO_OFFSET_VALUE                           31// 7
#define NUMBER_SAO_OFFSETS                              4
#define SAO_COMPONENT_LUMA                              0
#define SAO_COMPONENT_CHROMA                            1
#define SAO_COMPONENT_CHROMA_CB                         2
#define SAO_COMPONENT_CHROMA_CR                         3


#define MIN_QP_VALUE                     0
#define MAX_QP_VALUE                    51
#define MAX_CHROMA_MAP_QP_VALUE         57


//***Transforms***
#define TRANSFORMS_LUMA_FLAG        0
#define TRANSFORMS_CHROMA_FLAG      1
#define TRANSFORMS_COLOR_LEN        2
#define TRANSFORMS_LUMA_MASK        (1 << TRANSFORMS_LUMA_FLAG)
#define TRANSFORMS_CHROMA_MASK      (1 << TRANSFORMS_CHROMA_FLAG)
#define TRANSFORMS_FULL_MASK        ((1 << TRANSFORMS_LUMA_FLAG) | (1 << TRANSFORMS_CHROMA_FLAG))

#define TRANSFORMS_SIZE_32_FLAG     0
#define TRANSFORMS_SIZE_16_FLAG     1
#define TRANSFORMS_SIZE_8_FLAG      2
#define TRANSFORMS_SIZE_4_FLAG      3
#define TRANSFORMS_SIZE_LEN         4

#define TRANSFORM_MAX_SIZE          32
#define TRANSFORM_MIN_SIZE          4

#define QP_BD_OFFSET           12 //2x(bitDepth-8) 12 for 10 bit case
#define BIT_INCREMENT_10BIT    2
#define BIT_INCREMENT_8BIT     0

#define TRANS_BIT_INCREMENT    0
#define QUANT_IQUANT_SHIFT     20 // Q(QP%6) * IQ(QP%6) = 2^20
#define QUANT_SHIFT            14 // Q(4) = 2^14
#define SCALE_BITS             15 // Inherited from TMuC, pressumably for fractional bit estimates in RDOQ
#define MAX_TR_DYNAMIC_RANGE   15 // Maximum transform dynamic range (excluding sign bit)
#define MAX_POS_16BIT_NUM      32767
#define MIN_NEG_16BIT_NUM      -32768
#define QUANT_OFFSET_I         171
#define QUANT_OFFSET_P         85
#define LOW_LCU_VARIANCE		10
#define MEDIUM_LCU_VARIANCE		50


// INTRA restriction for global motion
#define INTRA_GLOBAL_MOTION_NON_MOVING_INDEX_TH  2
#define INTRA_GLOBAL_MOTION_DARK_LCU_TH         50

/*********************************************************
* used for the first time, but not the last time interpolation filter
*********************************************************/
#define Shift1       InternalBitDepthIncrement
#define MinusOffset1 (1 << (IF_Negative_Offset + InternalBitDepthIncrement))
#if (InternalBitDepthIncrement == 0)
#define ChromaMinusOffset1 0
#else
#define ChromaMinusOffset1 MinusOffset1
#endif

/*********************************************************
* used for neither the first time nor the last time interpolation filter
*********************************************************/
#define Shift2       IF_Shift

/*********************************************************
* used for the first time, and also the last time interpolation filter
*********************************************************/
#define Shift3       IF_Shift
#define Offset3      (1<<(Shift3-1))

/*********************************************************
* used for not the first time, but the last time interpolation filter
*********************************************************/
#define Shift4       (IF_Shift + IF_Shift - InternalBitDepthIncrement)
#define Offset4      ((1 << (IF_Shift + IF_Negative_Offset)) + (1 << (Shift4 - 1)))
#if (InternalBitDepthIncrement == 0)
#define ChromaOffset4 (1 << (Shift4 - 1))
#else
#define ChromaOffset4 Offset4
#endif

/*********************************************************
* used for weighted sample prediction
*********************************************************/
#define Shift5       (IF_Shift - InternalBitDepthIncrement + 1)
#define Offset5      ((1 << (Shift5 - 1)) + (1 << (IF_Negative_Offset + 1)))
#if (InternalBitDepthIncrement == 0)
#define ChromaOffset5 (1 << (Shift5 - 1))
#else
#define ChromaOffset5 Offset5
#endif

/*********************************************************
* used for biPredCopy()
*********************************************************/
#define Shift6       (IF_Shift - InternalBitDepthIncrement)
#define MinusOffset6 (1 << IF_Negative_Offset)
#if (InternalBitDepthIncrement == 0)
#define ChromaMinusOffset6 0
#else
#define ChromaMinusOffset6 MinusOffset6
#endif

/*********************************************************
* 10bit case
*********************************************************/

#define  SHIFT1D_10BIT      6  
#define  OFFSET1D_10BIT     32

#define  SHIFT2D1_10BIT     2  
#define  OFFSET2D1_10BIT    (-32768)

#define  SHIFT2D2_10BIT     10  
#define  OFFSET2D2_10BIT    524800

    //BIPRED
#define  BI_SHIFT_10BIT         4
#define  BI_OFFSET_10BIT        8192//2^(14-1)

#define  BI_AVG_SHIFT_10BIT     5
#define  BI_AVG_OFFSET_10BIT    16400

#define  BI_SHIFT2D2_10BIT      6
#define  BI_OFFSET2D2_10BIT     0

// Noise detection
#define  NOISE_VARIANCE_TH				390



#define  EB_PICNOISE_CLASS    EB_U8
#define  PIC_NOISE_CLASS_INV  0 //not computed
#define  PIC_NOISE_CLASS_1    1 //No Noise
#define  PIC_NOISE_CLASS_2    2 
#define  PIC_NOISE_CLASS_3    3 
#define  PIC_NOISE_CLASS_3_1  4 
#define  PIC_NOISE_CLASS_4    5 
#define  PIC_NOISE_CLASS_5    6 
#define  PIC_NOISE_CLASS_6    7 
#define  PIC_NOISE_CLASS_7    8 
#define  PIC_NOISE_CLASS_8    9 
#define  PIC_NOISE_CLASS_9    10
#define  PIC_NOISE_CLASS_10   11 //Extreme Noise

// Intrinisc
#define INTRINSIC_SSE2								1

// Enhance background macros for decimated 64x64
#define BEA_CLASS_0_0_DEC_TH 16 * 16	// 16x16 block size * 1
#define BEA_CLASS_0_DEC_TH	 16 * 16 * 2	// 16x16 block size * 2
#define BEA_CLASS_1_DEC_TH	 16 * 16 * 4	// 16x16 block size * 4
#define BEA_CLASS_2_DEC_TH	 16 * 16 * 8	// 16x16 block size * 8

// Enhance background macros
#define BEA_CLASS_0_0_TH 8 * 8    	// 8x8 block size * 1

#define BEA_CLASS_0_TH	8 * 8 * 2	// 8x8 block size * 2
#define BEA_CLASS_1_TH	8 * 8 * 4	// 8x8 block size * 4
#define BEA_CLASS_2_TH	8 * 8 * 8	// 8x8 block size * 8

#define UNCOVERED_AREA_ZZ_TH 4 * 4 * 14

#define BEA_CLASS_0_ZZ_COST	 0
#define BEA_CLASS_0_1_ZZ_COST	 3

#define BEA_CLASS_1_ZZ_COST	10
#define BEA_CLASS_2_ZZ_COST	20
#define BEA_CLASS_3_ZZ_COST	30
#define INVALID_ZZ_COST	(EB_U8) ~0
#define PM_NON_MOVING_INDEX_TH 23
#define QP_OFFSET_LCU_SCORE_0	0
#define QP_OFFSET_LCU_SCORE_1	50
#define QP_OFFSET_LCU_SCORE_2	100
#define UNCOVERED_AREA_ZZ_COST_TH 8
#define BEA_MIN_DELTA_QP_T00 1
#define BEA_MIN_DELTA_QP_T0  3
#define BEA_MIN_DELTA_QP_T1  5
#define BEA_MIN_DELTA_QP_T2  5
#define BEA_DISTANSE_RATIO_T0 900
#define BEA_DISTANSE_RATIO_T1 600
#define ACTIVE_PICTURE_ZZ_COST_TH 29


#define BEA_MAX_DELTA_QP 1

#define FAILING_MOTION_DELTA_QP			-5 
#define FAILING_MOTION_VAR_THRSLHD		50 
static const EB_U8 INTRA_AREA_TH_CLASS_1[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
    { 20 },
    { 30, 20 },
    { 40, 30, 20 },
	{ 50, 40, 30, 20 },
	{ 50, 40, 30, 20, 10 },
	{ 50, 40, 30, 20, 10, 10 }
};

// Picture split into regions for analysis (SCD, Dynamic GOP)
#define CLASS_SUB_0_REGION_SPLIT_PER_WIDTH	1
#define CLASS_SUB_0_REGION_SPLIT_PER_HEIGHT	1

#define CLASS_1_REGION_SPLIT_PER_WIDTH		2	
#define CLASS_1_REGION_SPLIT_PER_HEIGHT		2

#define HIGHER_THAN_CLASS_1_REGION_SPLIT_PER_WIDTH		4	
#define HIGHER_THAN_CLASS_1_REGION_SPLIT_PER_HEIGHT		4

// Dynamic GOP activity TH - to tune

#define DYNAMIC_GOP_SUB_1080P_L6_VS_L5_COST_TH		11
#define DYNAMIC_GOP_SUB_1080P_L5_VS_L4_COST_TH		19
#define DYNAMIC_GOP_SUB_1080P_L4_VS_L3_COST_TH		30	// No L4_VS_L3 - 25 is the TH after 1st round of tuning

#define DYNAMIC_GOP_ABOVE_1080P_L6_VS_L5_COST_TH	15//25//5//
#define DYNAMIC_GOP_ABOVE_1080P_L5_VS_L4_COST_TH	25//28//9//
#define DYNAMIC_GOP_ABOVE_1080P_L4_VS_L3_COST_TH	30	// No L4_VS_L3 - 28 is the TH after 1st round of tuning


#define DYNAMIC_GOP_SUB_480P_L6_VS_L5_COST_TH        9

#define GRADUAL_LUMINOSITY_CHANGE_TH		 3
#define FADED_LCU_PERCENTAGE_TH				10		
#define FADED_PICTURES_TH					15
#define CLASS_SUB_0_PICTURE_ACTIVITY_REGIONS_TH			1
#define CLASS_1_SIZE_PICTURE_ACTIVITY_REGIONS_TH		2
#define HIGHER_THAN_CLASS_1_PICTURE_ACTIVITY_REGIONS_TH	8

#define IS_COMPLEX_LCU_VARIANCE_TH						100
#define IS_COMPLEX_LCU_FLAT_VARIANCE_TH					 10
#define IS_COMPLEX_LCU_VARIANCE_DEVIATION_TH			 13
#define IS_COMPLEX_LCU_ZZ_SAD_FACTOR_TH					 25

// The EB_AURA_STATUS type is used to describe the aura status
#define EB_AURA_STATUS       EB_U8
#define AURA_STATUS_0        0		
#define AURA_STATUS_1        1		
#define AURA_STATUS_2        2		
#define AURA_STATUS_3        3		
#define INVALID_AURA_STATUS  128

// Aura detection definitions
#define	AURA_4K_DISTORTION_TH	25
#define	AURA_4K_DISTORTION_TH_6L 20



static const EB_S32 GLOBAL_MOTION_THRESHOLD[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
	{ 2 },
	{ 4, 2 },
	{ 8, 4, 2 },
	{ 16, 8, 4, 2 },
	{ 32, 16, 8, 4, 2 },	// Derived by analogy from 4-layer settings
	{ 64, 32, 16, 8, 4, 2 }
};


// The EB_4L_PRED_ERROR_CLASS type is used to inform about the prediction error compared to 4L
#define EB_4L_PRED_ERROR_CLASS    EB_U8
#define PRED_ERROR_CLASS_0          0
#define PRED_ERROR_CLASS_1          1
#define INVALID_PRED_ERROR_CLASS    128

#define EB_SCD_MODE EB_U8
#define SCD_MODE_0   0	 // SCD OFF
#define SCD_MODE_1   1	 // Light SCD (histograms generation on the 1/16 decimated input)
#define SCD_MODE_2   2	 // Full SCD


#define EB_NOISE_DETECT_MODE EB_U8
#define NOISE_DETECT_HALF_PRECISION      0	 // Use Half-pel decimated input to detect noise
#define NOISE_DETECT_QUARTER_PRECISION   1	 // Use Quarter-pel decimated input to detect noise
#define NOISE_DETECT_FULL_PRECISION      2	 // Use Full-pel decimated input to detect noise

#define EB_PM_MODE EB_U8
#define PM_MODE_0  1	 // 2-stage PM 4K
#define PM_MODE_1  2	 // 2-stage PM Sub 4K



#define EB_ZZ_SAD_MODE EB_U8
#define ZZ_SAD_MODE_0  0		// ZZ SAD on Decimated resolution
#define ZZ_SAD_MODE_1  1		// ZZ SAD on Full resolution

#define EB_PF_MODE EB_U8
#define PF_OFF  0	 
#define PF_N2   1	 
#define PF_N4   2	

#define NUM_QPS   52

#define TH_BIAS_DARK_LCU 37

typedef enum EB_CHROMA_MODE {
    CHROMA_MODE_FULL = 1,   // 0: Full Search Chroma for All 
    CHROMA_MODE_BEST = 2    // 1: Chroma OFF if I_SLICE, Chroma for only MV_Merge if P/B_SLICE 
} EB_CHROMA_MODE;

#define EB_LCU_COMPLEXITY_STATUS EB_U8
#define LCU_COMPLEXITY_STATUS_0                  0  
#define LCU_COMPLEXITY_STATUS_1                  1  
#define LCU_COMPLEXITY_STATUS_2                  2  
#define LCU_COMPLEXITY_STATUS_INVALID   (EB_U8) ~0

#define LCU_COMPLEXITY_NON_MOVING_INDEX_TH_0 30 
#define LCU_COMPLEXITY_NON_MOVING_INDEX_TH_1 29
#define LCU_COMPLEXITY_NON_MOVING_INDEX_TH_2 23

typedef enum EB_SAO_MODE {
    SAO_MODE_0 = 0,
    SAO_MODE_1 = 1
} EB_SAO_MODE;

typedef enum EB_CU_16x16_MODE {
	CU_16x16_MODE_0 = 0,  // Perform OIS, Full_Search, Fractional_Search & Bipred for CU_16x16
	CU_16x16_MODE_1 = 1   // Perform OIS and only Full_Search for CU_16x16
} EB_CU_16x16_MODE;

typedef enum EB_CU_8x8_MODE {
	CU_8x8_MODE_0 = 0,  // Perform OIS, Full_Search, Fractional_Search & Bipred for CU_8x8
	CU_8x8_MODE_1 = 1   // Do not perform OIS @ P/B Slices and only Full_Search for CU_8x8
} EB_CU_8x8_MODE;

typedef enum EB_PICTURE_DEPTH_MODE {

    PICT_LCU_SWITCH_DEPTH_MODE          = 0,
    PICT_FULL85_DEPTH_MODE              = 1,
    PICT_FULL84_DEPTH_MODE              = 2,
    PICT_BDP_DEPTH_MODE                 = 3,
    PICT_LIGHT_BDP_DEPTH_MODE           = 4,
    PICT_OPEN_LOOP_DEPTH_MODE           = 5
} EB_PICTURE_DEPTH_MODE;

#define EB_LCU_DEPTH_MODE EB_U8
#define LCU_FULL85_DEPTH_MODE                1
#define LCU_FULL84_DEPTH_MODE                2
#define LCU_BDP_DEPTH_MODE                   3
#define LCU_LIGHT_BDP_DEPTH_MODE             4
#define LCU_OPEN_LOOP_DEPTH_MODE             5
#define LCU_LIGHT_OPEN_LOOP_DEPTH_MODE       6
#define LCU_AVC_DEPTH_MODE                   7                     
#define LCU_LIGHT_AVC_DEPTH_MODE             8
#define LCU_PRED_OPEN_LOOP_DEPTH_MODE        9
#define LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE 10


typedef enum EB_INTRA4x4_SEARCH_METHOD {
    INTRA4x4_OFF               = 0,
    INTRA4x4_INLINE_SEARCH     = 1,
    INTRA4x4_REFINEMENT_SEARCH = 2,
} EB_INTRA4x4_SEARCH_METHOD;



typedef enum EB_ADP_DEPTH_SENSITIVE_PIC_CLASS {
    DEPTH_SENSITIVE_PIC_CLASS_0 = 0,    // Normal picture
    DEPTH_SENSITIVE_PIC_CLASS_1 = 1,    // High complex picture
    DEPTH_SENSITIVE_PIC_CLASS_2 = 2     // Moderate complex picture
} EB_ADP_DEPTH_SENSITIVE_PIC_CLASS;

   
typedef enum EB_ADP_REFINEMENT_MODE {
    ADP_REFINMENT_OFF = 0,  // Off
    ADP_MODE_0        = 1,  // Light AVC (only 16x16)
    ADP_MODE_1        = 2   // AVC (only 8x8 & 16x16 @ the Open Loop Search)
} EB_ADP_REFINEMENT_MODE;

typedef enum EB_MD_STAGE {

	MDC_STAGE = 0,
	BDP_PILLAR_STAGE = 1,
	BDP_64X64_32X32_REF_STAGE = 2,
	BDP_16X16_8X8_REF_STAGE = 3,
	BDP_MVMERGE_STAGE = 4

} EB_MD_STAGE;

#define EB_TRANS_COEFF_SHAPE EB_U8

#define DEFAULT_SHAPE 0         
#define N2_SHAPE      1  
#define N4_SHAPE      2  
#define ONLY_DC_SHAPE 3   


static const EB_S32 HME_LEVEL_0_SEARCH_AREA_MULTIPLIER_X[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
	{ 100 },
	{ 100, 100 },
	{ 100, 100, 100 },
	{ 200, 140, 100,  70 },
	{ 350, 200, 100, 100, 100 },
	{ 525, 350, 200, 100, 100, 100 }	 
};

static const EB_S32 HME_LEVEL_0_SEARCH_AREA_MULTIPLIER_Y[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
	{ 100 },
	{ 100, 100 },
	{ 100, 100, 100 },
	{ 200, 140, 100, 70 },
	{ 350, 200, 100, 100, 100 },
	{ 525, 350, 200, 100, 100, 100 }
};

typedef enum RASTER_SCAN_CU_INDEX {

	 // 2Nx2N [85 partitions]
	 RASTER_SCAN_CU_INDEX_64x64 = 0,
	 RASTER_SCAN_CU_INDEX_32x32_0 = 1,
	 RASTER_SCAN_CU_INDEX_32x32_1 = 2,
	 RASTER_SCAN_CU_INDEX_32x32_2 = 3,
	 RASTER_SCAN_CU_INDEX_32x32_3 = 4,
	 RASTER_SCAN_CU_INDEX_16x16_0 = 5,
	 RASTER_SCAN_CU_INDEX_16x16_1 = 6,
	 RASTER_SCAN_CU_INDEX_16x16_2 = 7,
	 RASTER_SCAN_CU_INDEX_16x16_3 = 8,
	 RASTER_SCAN_CU_INDEX_16x16_4 = 9,
	 RASTER_SCAN_CU_INDEX_16x16_5 = 10,
	 RASTER_SCAN_CU_INDEX_16x16_6 = 11,
	 RASTER_SCAN_CU_INDEX_16x16_7 = 12,
	 RASTER_SCAN_CU_INDEX_16x16_8 = 13,
	 RASTER_SCAN_CU_INDEX_16x16_9 = 14,
	 RASTER_SCAN_CU_INDEX_16x16_10 = 15,
	 RASTER_SCAN_CU_INDEX_16x16_11 = 16,
	 RASTER_SCAN_CU_INDEX_16x16_12 = 17,
	 RASTER_SCAN_CU_INDEX_16x16_13 = 18,
	 RASTER_SCAN_CU_INDEX_16x16_14 = 19,
	 RASTER_SCAN_CU_INDEX_16x16_15 = 20,
	 RASTER_SCAN_CU_INDEX_8x8_0 = 21,
	 RASTER_SCAN_CU_INDEX_8x8_1 = 22,
	 RASTER_SCAN_CU_INDEX_8x8_2 = 23,
	 RASTER_SCAN_CU_INDEX_8x8_3 = 24,
	 RASTER_SCAN_CU_INDEX_8x8_4 = 25,
	 RASTER_SCAN_CU_INDEX_8x8_5 = 26,
	 RASTER_SCAN_CU_INDEX_8x8_6 = 27,
	 RASTER_SCAN_CU_INDEX_8x8_7 = 28,
	 RASTER_SCAN_CU_INDEX_8x8_8 = 29,
	 RASTER_SCAN_CU_INDEX_8x8_9 = 30,
	 RASTER_SCAN_CU_INDEX_8x8_10 = 31,
	 RASTER_SCAN_CU_INDEX_8x8_11 = 32,
	 RASTER_SCAN_CU_INDEX_8x8_12 = 33,
	 RASTER_SCAN_CU_INDEX_8x8_13 = 34,
	 RASTER_SCAN_CU_INDEX_8x8_14 = 35,
	 RASTER_SCAN_CU_INDEX_8x8_15 = 36,
	 RASTER_SCAN_CU_INDEX_8x8_16 = 37,
	 RASTER_SCAN_CU_INDEX_8x8_17 = 38,
	 RASTER_SCAN_CU_INDEX_8x8_18 = 39,
	 RASTER_SCAN_CU_INDEX_8x8_19 = 40,
	 RASTER_SCAN_CU_INDEX_8x8_20 = 41,
	 RASTER_SCAN_CU_INDEX_8x8_21 = 42,
	 RASTER_SCAN_CU_INDEX_8x8_22 = 43,
	 RASTER_SCAN_CU_INDEX_8x8_23 = 44,
	 RASTER_SCAN_CU_INDEX_8x8_24 = 45,
	 RASTER_SCAN_CU_INDEX_8x8_25 = 46,
	 RASTER_SCAN_CU_INDEX_8x8_26 = 47,
	 RASTER_SCAN_CU_INDEX_8x8_27 = 48,
	 RASTER_SCAN_CU_INDEX_8x8_28 = 49,
	 RASTER_SCAN_CU_INDEX_8x8_29 = 50,
	 RASTER_SCAN_CU_INDEX_8x8_30 = 51,
	 RASTER_SCAN_CU_INDEX_8x8_31 = 52,
	 RASTER_SCAN_CU_INDEX_8x8_32 = 53,
	 RASTER_SCAN_CU_INDEX_8x8_33 = 54,
	 RASTER_SCAN_CU_INDEX_8x8_34 = 55,
	 RASTER_SCAN_CU_INDEX_8x8_35 = 56,
	 RASTER_SCAN_CU_INDEX_8x8_36 = 57,
	 RASTER_SCAN_CU_INDEX_8x8_37 = 58,
	 RASTER_SCAN_CU_INDEX_8x8_38 = 59,
	 RASTER_SCAN_CU_INDEX_8x8_39 = 60,
	 RASTER_SCAN_CU_INDEX_8x8_40 = 61,
	 RASTER_SCAN_CU_INDEX_8x8_41 = 62,
	 RASTER_SCAN_CU_INDEX_8x8_42 = 63,
	 RASTER_SCAN_CU_INDEX_8x8_43 = 64,
	 RASTER_SCAN_CU_INDEX_8x8_44 = 65,
	 RASTER_SCAN_CU_INDEX_8x8_45 = 66,
	 RASTER_SCAN_CU_INDEX_8x8_46 = 67,
	 RASTER_SCAN_CU_INDEX_8x8_47 = 68,
	 RASTER_SCAN_CU_INDEX_8x8_48 = 69,
	 RASTER_SCAN_CU_INDEX_8x8_49 = 70,
	 RASTER_SCAN_CU_INDEX_8x8_50 = 71,
	 RASTER_SCAN_CU_INDEX_8x8_51 = 72,
	 RASTER_SCAN_CU_INDEX_8x8_52 = 73,
	 RASTER_SCAN_CU_INDEX_8x8_53 = 74,
	 RASTER_SCAN_CU_INDEX_8x8_54 = 75,
	 RASTER_SCAN_CU_INDEX_8x8_55 = 76,
	 RASTER_SCAN_CU_INDEX_8x8_56 = 77,
	 RASTER_SCAN_CU_INDEX_8x8_57 = 78,
	 RASTER_SCAN_CU_INDEX_8x8_58 = 79,
	 RASTER_SCAN_CU_INDEX_8x8_59 = 80,
	 RASTER_SCAN_CU_INDEX_8x8_60 = 81,
	 RASTER_SCAN_CU_INDEX_8x8_61 = 82,
	 RASTER_SCAN_CU_INDEX_8x8_62 = 83,
	 RASTER_SCAN_CU_INDEX_8x8_63 = 84
} RASTER_SCAN_CU_INDEX;

static const EB_U32 RASTER_SCAN_CU_X[CU_MAX_COUNT] =
{
	0,
	0, 32,
	0, 32,
	0, 16, 32, 48,
	0, 16, 32, 48,
	0, 16, 32, 48,
	0, 16, 32, 48,
	0, 8, 16, 24, 32, 40, 48, 56,
	0, 8, 16, 24, 32, 40, 48, 56,
	0, 8, 16, 24, 32, 40, 48, 56,
	0, 8, 16, 24, 32, 40, 48, 56,
	0, 8, 16, 24, 32, 40, 48, 56,
	0, 8, 16, 24, 32, 40, 48, 56,
	0, 8, 16, 24, 32, 40, 48, 56,
	0, 8, 16, 24, 32, 40, 48, 56
};

static const EB_U32 RASTER_SCAN_CU_Y[CU_MAX_COUNT] =
{
	0,
	0, 0,
	32, 32,
	0, 0, 0, 0,
	16, 16, 16, 16,
	32, 32, 32, 32,
	48, 48, 48, 48,
	0, 0, 0, 0, 0, 0, 0, 0,
	8, 8, 8, 8, 8, 8, 8, 8,
	16, 16, 16, 16, 16, 16, 16, 16,
	24, 24, 24, 24, 24, 24, 24, 24,
	32, 32, 32, 32, 32, 32, 32, 32,
	40, 40, 40, 40, 40, 40, 40, 40,
	48, 48, 48, 48, 48, 48, 48, 48,
	56, 56, 56, 56, 56, 56, 56, 56
};

static const EB_U32 RASTER_SCAN_CU_SIZE[CU_MAX_COUNT] =
{	64,
	32, 32,
	32, 32,
	16, 16, 16, 16,
	16, 16, 16, 16,
	16, 16, 16, 16,
	16, 16, 16, 16,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8, 8, 8
};

static const EB_U32 RASTER_SCAN_CU_DEPTH[CU_MAX_COUNT] =
{	0,
	1, 1,
	1, 1,
	2, 2, 2, 2,
	2, 2, 2, 2,
	2, 2, 2, 2,
	2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3
};

static const EB_U32 RASTER_SCAN_TO_MD_SCAN[CU_MAX_COUNT] =
{
	0,
	1, 22,
	43, 64,
	2, 7, 23, 28,
	12, 17, 33, 38,
	44, 49, 65, 70,
	54, 59, 75, 80,
	3, 4, 8, 9, 24, 25, 29, 30,
	5, 6, 10, 11, 26, 27, 31, 32,
	13, 14, 18, 19, 34, 35, 39, 40,
	15, 16, 20, 21, 36, 37, 41, 42,
	45, 46, 50, 51, 66, 67, 71, 72,
	47, 48, 52, 53, 68, 69, 73, 74,
	55, 56, 60, 61, 76, 77, 81, 82,
	57, 58, 62, 63, 78, 79, 83, 84
};

static const EB_U32 ParentBlockIndex[85] = { 0, 0, 0, 2, 2, 2, 2, 0, 7, 7, 7, 7, 0, 12, 12, 12, 12, 0, 17, 17, 17, 17, 0, 0,
23, 23, 23, 23, 0, 28, 28, 28, 28, 0, 33, 33, 33, 33, 0, 38, 38, 38, 38, 0, 0,
44, 44, 44, 44, 0, 49, 49, 49, 49, 0, 54, 54, 54, 54, 0, 59, 59, 59, 59, 0, 0,
65, 65, 65, 65, 0, 70, 70, 70, 70, 0, 75, 75, 75, 75, 0, 80, 80, 80, 80 };

static const EB_U32 MD_SCAN_TO_RASTER_SCAN[CU_MAX_COUNT] =
{
	0,
	1,
	5, 21, 22, 29, 30,
	6, 23, 24, 31, 32,
	9, 37, 38, 45, 46,
	10, 39, 40, 47, 48,
	2,
	7, 25, 26, 33, 34,
	8, 27, 28, 35, 36,
	11, 41, 42, 49, 50,
	12, 43, 44, 51, 52,
	3,
	13, 53, 54, 61, 62,
	14, 55, 56, 63, 64,
	17, 69, 70, 77, 78,
	18, 71, 72, 79, 80,
	4,
	15, 57, 58, 65, 66,
	16, 59, 60, 67, 68,
	19, 73, 74, 81, 82,
	20, 75, 76, 83, 84
};

static const EB_U32 RASTER_SCAN_CU_PARENT_INDEX[CU_MAX_COUNT] =
{ 0,
0, 0,
0, 0,
1, 1, 2, 2,
1, 1, 2, 2,
3, 3, 4, 4,
3, 3, 4, 4,
5, 5, 6, 6, 7, 7, 8, 8,
5, 5, 6, 6, 7, 7, 8, 8,
9, 9, 10, 10, 11, 11, 12, 12,
9, 9, 10, 10, 11, 11, 12, 12,
13, 13, 14, 14, 15, 15, 16, 16,
13, 13, 14, 14, 15, 15, 16, 16,
17, 17, 18, 18, 19, 19, 20, 20,
17, 17, 18, 18, 19, 19, 20, 20
};


#define UNCOMPRESS_SAD(x) ( ((x) & 0x1FFF)<<(((x)>>13) & 7) )

static const EB_U32 MD_SCAN_TO_OIS_32x32_SCAN[CU_MAX_COUNT] =
{
/*0  */0,
/*1  */0,  
/*2  */0,
/*3  */0,
/*4  */0,
/*5  */0,
/*6  */0,
/*7  */0,
/*8  */0,
/*9  */0,
/*10 */0,
/*11 */0,
/*12 */0,
/*13 */0,
/*14 */0,
/*15 */0,
/*16 */0,
/*17 */0,
/*18 */0,
/*19 */0,
/*20 */0,
/*21 */0,
/*22 */1,
/*23 */1,
/*24 */1,
/*25 */1,
/*26 */1,
/*27 */1,
/*28 */1,
/*29 */1,
/*30 */1,
/*31 */1,
/*32 */1,
/*33 */1,
/*34 */1,
/*35 */1,
/*36 */1,
/*37 */1,
/*38 */1,
/*39 */1,
/*40 */1,
/*41 */1,
/*42 */1,
/*43 */2,
/*44 */2, 
/*45 */2,
/*46 */2,
/*47 */2,
/*48 */2,
/*49 */2,
/*50 */2,
/*51 */2,
/*52 */2,
/*53 */2, 
/*54 */2,
/*55 */2,
/*56 */2,
/*57 */2,
/*58 */2,
/*59 */2,
/*60 */2,
/*61 */2,
/*62 */2,
/*63 */2,
/*64 */3,
/*65 */3, 
/*66 */3,
/*67 */3,
/*68 */3,
/*69 */3,
/*70 */3,
/*71 */3,
/*72 */3,
/*73 */3,
/*74 */3,
/*75 */3, 
/*76 */3,
/*77 */3,
/*78 */3,  
/*79 */3,
/*80 */3,
/*81 */3,
/*82 */3,
/*83 */3,
/*84 */3,
};
/******************************************************************************
							ME/HME settings VMAF
*******************************************************************************/
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11   
static const EB_U8 EnableHmeLevel0FlagVmaf[5][MAX_SUPPORTED_MODES] = {
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_4K_RANGE
};
static const EB_U16 HmeLevel0TotalSearchAreaWidthVmaf[5][MAX_SUPPORTED_MODES] = {
	{  48,   48,    48,   48,   48,   48,   48,   48,   32,   32,   32,   32,   32 },
	{  64,   64,    64,   64,   64,   64,   64,   48,   48,   48,   48,   48,   48 },
	{  96,   96,    96,   96,   96,   96,   96,   64,   48,   48,   48,   48,   48 },
	{  96,   96,    96,   96,   96,   96,   96,   64,   48,   48,   48,   48,   48 },
	{ 128,  128,   128,   64,   64,   64,   64,   64,   64,   64,   64,   64,   64 }
};

static const EB_U16 HmeLevel0SearchAreaInWidthArrayLeftVmaf[5][MAX_SUPPORTED_MODES] = {
	{  24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16 },
	{  32,   32,   32,   32,   32,   32,   32,   24,   24,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24,   24 },
	{  64,   64,   64,   32,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const EB_U16 HmeLevel0SearchAreaInWidthArrayRightVmaf[5][MAX_SUPPORTED_MODES] = {
	{  24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16 },
	{  32,   32,   32,   32,   32,   32,   32,   24,   24,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24,   24 },
	{  64,   64,   64,   32,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const EB_U16 HmeLevel0TotalSearchAreaHeightVmaf[5][MAX_SUPPORTED_MODES] = {
	{  40,   40,   40,   40,   40,   40,   40,   32,   24,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   48,   40,   32,   32,   32,   32,   32 },
	{  48,   48,   48,   48,   48,   48,   48,   32,   32,   32,   32,   32,   32 },
	{  48,   48,   48,   48,   48,   48,   48,   48,   40,   40,   40,   40,   40 },
	{  80,   80,   80,   32,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const EB_U16 HmeLevel0SearchAreaInHeightArrayTopVmaf[5][MAX_SUPPORTED_MODES] = {
	{  20,   20,   20,   20,   20,   20,   20,   16,   12,   12,   12,   12,   12 },
	{  24,   24,   24,   24,   24,   24,   24,   20,   16,   16,   16,   16,   16 },
	{  24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   16 },
	{  24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20,   20 },
	{  40,   40,   40,   16,   16,   16,   16,   16,   16,   16,   16,   16,   16 }
};
static const EB_U16 HmeLevel0SearchAreaInHeightArrayBottomVmaf[5][MAX_SUPPORTED_MODES] = {
	{  20,   20,   20,   20,   20,   20,   20,   16,   12,   12,   12,   12,   12 },
	{  24,   24,   24,   24,   24,   24,   24,   20,   16,   16,   16,   16,   16 },
	{  24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   16 },
	{  24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20,   20 },
	{  40,   40,   40,   16,   16,   16,   16,   16,   16,   16,   16,   16,   16 }
};

// HME LEVEL 1
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11   
static const EB_U8 EnableHmeLevel1FlagVmaf[5][MAX_SUPPORTED_MODES] = {
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const EB_U16 HmeLevel1SearchAreaInWidthArrayLeftVmaf[5][MAX_SUPPORTED_MODES] = {
	{  16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel1SearchAreaInWidthArrayRightVmaf[5][MAX_SUPPORTED_MODES] = {
	{  16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel1SearchAreaInHeightArrayTopVmaf[5][MAX_SUPPORTED_MODES] = {
	{  16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    2,    2,    2 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    2,    2,    2,    2,    2 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel1SearchAreaInHeightArrayBottomVmaf[5][MAX_SUPPORTED_MODES] = {
	{  16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    2,    2,    2 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    2,    2,    2,    2,    2 },
	{  16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
// HME LEVEL 2
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11   
static const EB_U8 EnableHmeLevel2FlagVmaf[5][MAX_SUPPORTED_MODES] = {
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
	{   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 },      // INPUT_SIZE_720P_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 },      // INPUT_SIZE_1080i_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 },      // INPUT_SIZE_1080p_RANGE
	{   1,    1,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const EB_U16 HmeLevel2SearchAreaInWidthArrayLeftVmaf[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4 },
	{   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel2SearchAreaInWidthArrayRightVmaf[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4 },
	{   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel2SearchAreaInHeightArrayTopVmaf[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    4,    4,    4,    4,    4,    2,    2,    2,    2,    2 },
	{   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel2SearchAreaInHeightArrayBottomVmaf[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    4,    4,    4,    4,    4,    2,    2,    2,    2,    2 },
	{   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0,    0 },
	{   8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};

static const EB_U8 SearchAreaWidthVmaf[5][MAX_SUPPORTED_MODES] = {
	{  64,   64,   64,   16,   16,   16,   16,   16,   16,   16,   16,   16,   16 },
	{  64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8,    8 },
	{  64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8,    8 },
	{  64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8,    8 },
	{  64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8,    8 }
};

static const EB_U8 SearchAreaHeightVmaf[5][MAX_SUPPORTED_MODES] = {
	{  64,   64,   64,   16,   16,   16,   16,    9,    7,    7,    7,    7,    7 },
	{  64,   64,   64,   16,   16,   16,   16,    9,    7,    7,    7,    7,    7 },
	{  64,   64,   64,   16,   16,   16,   16,    7,    7,    7,    7,    7,    7 },
	{  64,   64,   64,   16,   16,   16,   16,    9,    7,    7,    7,    7,    7 },
	{  64,   64,   64,    9,    9,    9,    9,    9,    7,    7,    7,    7,    7 }
};
/******************************************************************************
                            ME/HME settings OQ
*******************************************************************************/
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11   
static const EB_U8 EnableHmeLevel0FlagOq[5][MAX_SUPPORTED_MODES] = {
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_4K_RANGE
};
static const EB_U16 HmeLevel0TotalSearchAreaWidthOq[5][MAX_SUPPORTED_MODES] = {
	{  48,   48,   48,   48,   48,   48,   48,   48,   48,   32,   32,   32,   32 },
	{  64,   64,   64,   64,   64,   64,   64,   48,   48,   48,   48,   48,   48 },
	{  96,   96,   96,   96,   96,   96,   96,   64,   64,   48,   48,   48,   48 },
	{  96,   96,   96,   96,   96,   96,   96,   64,   64,   48,   48,   48,   48 },
	{ 128,  128,  128,  128,   64,   64,   64,   64,   64,   64,   64,   64,   64 }
};

static const EB_U16 HmeLevel0SearchAreaInWidthArrayLeftOq[5][MAX_SUPPORTED_MODES] = {
	{  24,   24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16 },
	{  32,   32,   32,   32,   32,   32,   32,   24,   24,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   48,   32,   32,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   48,   32,   32,   24,   24,   24,   24 },
	{  64,   64,   64,   64,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const EB_U16 HmeLevel0SearchAreaInWidthArrayRightOq[5][MAX_SUPPORTED_MODES] = {
	{  24,   24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16 },
	{  32,   32,   32,   32,   32,   32,   32,   24,   24,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   48,   32,   32,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   48,   32,   32,   24,   24,   24,   24 },
	{  64,   64,   64,   64,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const EB_U16 HmeLevel0TotalSearchAreaHeightOq[5][MAX_SUPPORTED_MODES] = {
	{  40,   40,   40,   40,   40,   32,   32,   32,   32,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   40,   40,   40,   40,   32,   32,   32,   32 },
	{  48,   48,   48,   48,   48,   32,   32,   32,   32,   32,   32,   32,   32 },
	{  48,   48,   48,   48,   48,   48,   48,   48,   48,   40,   40,   40,   40 },
	{  80,   80,   80,   80,   32,   32,   32,   32,   32,   32,   32,   32,   32 }
};
static const EB_U16 HmeLevel0SearchAreaInHeightArrayTopOq[5][MAX_SUPPORTED_MODES] = {
	{  20,   20,   20,   20,   20,   16,   16,   16,   16,   12,   12,   12,   12 },
	{  24,   24,   24,   24,   24,   20,   20,   20,   20,   16,   16,   16,   16 },
	{  24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   16,   16,   16 },
	{  24,   24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20 },
	{  40,   40,   40,   40,   16,   16,   16,   16,   16,   16,   16,   16,   16 }
};
static const EB_U16 HmeLevel0SearchAreaInHeightArrayBottomOq[5][MAX_SUPPORTED_MODES] = {
	{  20,   20,   20,   20,   20,   16,   16,   16,   16,   12,   12,   12,   12 },
	{  24,   24,   24,   24,   24,   20,   20,   20,   20,   16,   16,   16,   16 },
	{  24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   16,   16,   16 },
	{  24,   24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20 },
	{  40,   40,   40,   40,   16,   16,   16,   16,   16,   16,   16,   16,   16 }
};

// HME LEVEL 1
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11   
static const EB_U8 EnableHmeLevel1FlagOq[5][MAX_SUPPORTED_MODES] = {
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const EB_U16 HmeLevel1SearchAreaInWidthArrayLeftOq[5][MAX_SUPPORTED_MODES] = {
	{  16,   16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel1SearchAreaInWidthArrayRightOq[5][MAX_SUPPORTED_MODES] = {
	{  16,   16,   16,   16,    8,    8,    8,    8,    8,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel1SearchAreaInHeightArrayTopOq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    8,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    4,    4,    4,    4,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel1SearchAreaInHeightArrayBottomOq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    8,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    8,    4,    4,    4,    4,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
// HME LEVEL 2
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11   
static const EB_U8 EnableHmeLevel2FlagOq[5][MAX_SUPPORTED_MODES] = {
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_720P_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_1080i_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_1080p_RANGE
	{   1,    1,    1,    1,    0,    0,    0,    0,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const EB_U16 HmeLevel2SearchAreaInWidthArrayLeftOq[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    4,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel2SearchAreaInWidthArrayRightOq[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    4,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel2SearchAreaInHeightArrayTopOq[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    2,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    2,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    2,    2,    2,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    2,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel2SearchAreaInHeightArrayBottomOq[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    2,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    2,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    2,    2,    2,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    2,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    0,    0,    0,    0,    0,    0,    0,    0,    0 }
};

static const EB_U8 SearchAreaWidthOq[5][MAX_SUPPORTED_MODES] = {
	{  64,   64,   64,   64,   16,   16,   16,   16,   16,   16,    8,    8,    8 },
	{  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8 },
	{  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8 },
	{  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8,    8 },
	{  64,   64,   64,   64,   16,   16,   16,   16,    8,    8,    8,    8,    8 }
};

static const EB_U8 SearchAreaHeightOq[5][MAX_SUPPORTED_MODES] = {
	{  64,   64,   64,   64,   16,    9,    9,    9,    9,    7,    7,    7,    7 },
	{  64,   64,   64,   64,   16,   13,   13,    9,    9,    7,    7,    7,    7 },
	{  64,   64,   64,   64,   16,    9,    9,    7,    7,    7,    7,    7,    7 },
	{  64,   64,   64,   64,   16,   13,   13,    9,    9,    7,    7,    7,    7 },
	{  64,   64,   64,   64,    9,    9,    9,    9,    7,    7,    7,    7,    7 }
};
/******************************************************************************
                            ME/HME settings SQ
*******************************************************************************/
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11    
static const EB_U8 EnableHmeLevel0FlagSq[5][MAX_SUPPORTED_MODES] = {
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_4K_RANGE
};
static const EB_U16 HmeLevel0TotalSearchAreaWidthSq[5][MAX_SUPPORTED_MODES] = {
	{  48,   48,   48,   48,   48,   48,   48,   48,   32,   32,   32,   32,   32 },
	{  64,   64,   64,   64,   64,   64,   64,   64,   48,   48,   32,   32,   32 },
	{  96,   96,   96,   96,   96,   96,   96,   96,   64,   48,   48,   48,   48 },
	{  96,   96,   96,   96,   96,   96,   64,   64,   64,   48,   48,   48,   48 },
	{ 128,  128,  128,  128,  128,   96,   96,   64,   64,   64,   64,   64,   64 }
};

static const EB_U16 HmeLevel0SearchAreaInWidthArrayLeftSq[5][MAX_SUPPORTED_MODES] = {
	{  24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16 },
	{  32,   32,   32,   32,   32,   32,   32,   32,   24,   24,   16,   16,   16 },
	{  48,   48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   32,   32,   32,   24,   24,   24,   24 },
	{  64,   64,   64,   64,   64,   48,   48,   32,   32,   32,   32,   32,   32 }
};
static const EB_U16 HmeLevel0SearchAreaInWidthArrayRightSq[5][MAX_SUPPORTED_MODES] = {
	{  24,   24,   24,   24,   24,   24,   24,   24,   16,   16,   16,   16,   16 },
	{  32,   32,   32,   32,   32,   32,   32,   32,   24,   24,   16,   16,   16 },
	{  48,   48,   48,   48,   48,   48,   48,   48,   32,   24,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   32,   32,   32,   24,   24,   24,   24 },
	{  64,   64,   64,   64,   64,   48,   48,   32,   32,   32,   32,   32,   32 }
};
static const EB_U16 HmeLevel0TotalSearchAreaHeightSq[5][MAX_SUPPORTED_MODES] = {
	{  40,   40,   40,   40,   40,   32,   32,   32,   24,   24,   16,   16,   16 },
	{  48,   48,   48,   48,   48,   40,   40,   40,   32,   32,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   32,   32,   32,   32,   32,   24,   24,   24 },
	{  48,   48,   48,   48,   48,   48,   48,   48,   48,   40,   40,   40,   40 },
	{  80,   80,   80,   80,   80,   64,   64,   32,   32,   32,   32,   32,   32 }
};
static const EB_U16 HmeLevel0SearchAreaInHeightArrayTopSq[5][MAX_SUPPORTED_MODES] = {
	{  20,   20,   20,   20,   20,   16,   16,   16,   12,   12,    8,    8,    8 },
	{  24,   24,   24,   24,   24,   20,   20,   20,   16,   16,   12,   12,   12 },
	{  24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   12,   12,   12 },
	{  24,   24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20 },
	{  40,   40,   40,   40,   40,   32,   32,   16,   16,   16,   16,   16,   16 }
};
static const EB_U16 HmeLevel0SearchAreaInHeightArrayBottomSq[5][MAX_SUPPORTED_MODES] = {
	{  20,   20,   20,   20,   20,   16,   16,   16,   12,   12,    8,    8,    8 },
	{  24,   24,   24,   24,   24,   20,   20,   20,   16,   16,   12,   12,   12 },
	{  24,   24,   24,   24,   24,   16,   16,   16,   16,   16,   12,   12,   12 },
	{  24,   24,   24,   24,   24,   24,   24,   24,   24,   20,   20,   20,   20 },
	{  40,   40,   40,   40,   40,   32,   32,   16,   16,   16,   16,   16,   16 }
};

// HME LEVEL 1
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11   
static const EB_U8 EnableHmeLevel1FlagSq[5][MAX_SUPPORTED_MODES] = {
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_720P_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080i_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 },      // INPUT_SIZE_1080p_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const EB_U16 HmeLevel1SearchAreaInWidthArrayLeftSq[5][MAX_SUPPORTED_MODES] = {
	{  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    4,    4,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel1SearchAreaInWidthArrayRightSq[5][MAX_SUPPORTED_MODES] = {
	{  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    4,    4,    4 },
	{  16,   16,   16,   16,    8,    8,    4,    4,    4,    4,    4,    4,    4 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel1SearchAreaInHeightArrayTopSq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    4,    4,    4,    2,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    4,    4,    4,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel1SearchAreaInHeightArrayBottomSq[5][MAX_SUPPORTED_MODES] = {
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    8,    8,    4,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    4,    4,    4,    2,    2,    2,    2,    2 },
    {  16,   16,   16,   16,    8,    8,    4,    4,    4,    4,    2,    2,    2 },
    {  16,   16,   16,   16,    4,    4,    4,    4,    0,    0,    0,    0,    0 }
};
// HME LEVEL 2
//     M0    M1    M2    M3    M4    M5    M6    M7    M8    M9    M10   M11   
static const EB_U8 EnableHmeLevel2FlagSq[5][MAX_SUPPORTED_MODES] = {
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_576p_RANGE_OR_LOWER
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_720P_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_1080i_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0 },      // INPUT_SIZE_1080p_RANGE
	{   1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,    0,    0 }       // INPUT_SIZE_4K_RANGE
};
static const EB_U16 HmeLevel2SearchAreaInWidthArrayLeftSq[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    0,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel2SearchAreaInWidthArrayRightSq[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    4,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    4,    4,    4,    0,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel2SearchAreaInHeightArrayTopSq[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    2,    2,    2,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    2,    2,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    2,    2,    2,    0,    0,    0,    0,    0,    0 }
};
static const EB_U16 HmeLevel2SearchAreaInHeightArrayBottomSq[5][MAX_SUPPORTED_MODES] = {
	{   8,    8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    4,    4,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    2,    2,    2,    2,    0,    0,    0,    0 },
	{   8,    8,    8,    8,    4,    4,    2,    2,    2,    0,    0,    0,    0 },
    {   8,    8,    8,    8,    2,    2,    2,    0,    0,    0,    0,    0,    0 }
};

static const EB_U8 SearchAreaWidthSq[5][MAX_SUPPORTED_MODES] = {
	{  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8 ,    8 },
	{  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8 ,    8 },
	{  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8 ,    8 },
	{  64,   64,   64,   64,   16,   16,   16,   16,   16,    8,    8,    8 ,    8 },
	{  64,   64,   64,   64,   24,   16,   16,   16,   16,   16,    8,    8 ,    8 }
};

static const EB_U8 SearchAreaHeightSq[5][MAX_SUPPORTED_MODES] = {
	{  64,   64,   64,   64,   16,    9,    9,    9,    7,    7,    5,    5,    5 },
	{  64,   64,   64,   64,   16,   13,   13,   13,    9,    7,    7,    7,    7 },
	{  64,   64,   64,   64,   16,    9,    9,    9,    7,    7,    5,    5,    5 },
	{  64,   64,   64,   64,   16,   13,    9,    9,    9,    7,    7,    7,    7 },
	{  64,   64,   64,   64,   24,   16,   13,    9,    9,    7,    7,    7,    5 }
};

#define MAX_SUPPORTED_SEGMENTS       7

#ifdef __cplusplus
}
#endif
#endif // EbDefinitions_h
/* File EOF */
