/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTypes_h
#define EbTypes_h

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "EbBuild.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef _WIN32 
#ifndef __cplusplus
#define inline __inline 
#endif
#elif __GNUC__ 
#define inline __inline__
#define fseeko64 fseek
#define ftello64 ftell
#else
#error OS not supported 
#endif

#define CHKN_OMX        1
#define DEADLOCK_DEBUG  0
#define CHKN_EOS        1
#define ONE_MEMCPY      1
#define EB_MIN(a,b)             (((a) < (b)) ? (a) : (b))

#ifdef	_MSC_VER

#define NOINLINE                __declspec ( noinline ) 
#define FORCE_INLINE            __forceinline
#pragma warning( disable : 4068 ) // unknown pragma

#else // _MSC_VER

#define NOINLINE                __attribute__(( noinline ))

#ifdef __cplusplus
#define FORCE_INLINE            inline   __attribute__((always_inline))
#else
#define FORCE_INLINE            __inline __attribute__((always_inline))
#endif // __cplusplus

#define __popcnt                __builtin_popcount

#endif // _MSC_VER

// Define Cross-Platform 64-bit fseek() and ftell()
#ifdef _MSC_VER
	typedef __int64 off64_t;
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#elif _WIN32 // MinGW
#else 
	// *Note -- fseeko and ftello are already defined for linux
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

	/** The EB_GOP type is used to describe the hierarchical coding structure of
	Groups of Pictures (GOP) units.
	*/
#define EB_PRED                 EB_U8
#define EB_PRED_LOW_DELAY_P     0
#define EB_PRED_LOW_DELAY_B     1
#define EB_PRED_RANDOM_ACCESS   2
#define EB_PRED_TOTAL_COUNT     3
#define EB_PRED_INVALID         0xFF
	
	
#define EB_INPUT_RESOLUTION             EB_U8
#define INPUT_SIZE_576p_RANGE_OR_LOWER	 0
#define INPUT_SIZE_1080i_RANGE			 1
#define INPUT_SIZE_1080p_RANGE	         2
#define INPUT_SIZE_4K_RANGE			     3

	
#define INPUT_SIZE_576p_TH				0x90000		// 0.58 Million   
#define INPUT_SIZE_1080i_TH				0xB71B0		// 0.75 Million
#define INPUT_SIZE_1080p_TH				0x1AB3F0	// 1.75 Million
#define INPUT_SIZE_4K_TH				0x29F630	// 2.75 Million   


#define EB_NORMAL_LATENCY        0
#define EB_LOW_LATENCY           1
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

	/** The EB_PTR type is intended to be used to pass pointers to and from the svt
	API.  This is a 32 bit pointer and is aligned on a 32 bit word boundary.
	*/
	typedef void * EB_PTR;

	/** The EB_BYTE type is intended to be used to pass arrays of bytes such as
	buffers to and from the svt API.  The EB_BYTE type is a 32 bit pointer.
	The pointer is word aligned and the buffer is byte aligned.
	*/
	typedef EB_U8 * EB_BYTE;

	/** The EB_SAMPLE type is intended to be used to pass arrays of bytes such as
	buffers to and from the svt API.  The EB_BYTE type is a 32 bit pointer.
	The pointer is word aligned and the buffer is byte aligned.
	*/

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
	} EB_RDOQ_PMCORE_TYPE;

	/** Assembly Types
	*/
	typedef enum EB_ASM {
		ASM_NON_AVX2,
		ASM_AVX2,
		ASM_TYPE_TOTAL,
		ASM_AVX512,
		ASM_TYPE_INVALID = ~0
	} EB_ASM;

	/** The EB_NULL type is used to define the C style NULL pointer.
	*/
    #define EB_NULL ((void*) 0)
    
    typedef enum EB_ERRORTYPE
    {
        EB_ErrorNone = 0,
        EB_ErrorInsufficientResources               = (EB_S32) 0x80001000,
        EB_ErrorUndefined                           = (EB_S32) 0x80001001,
        EB_ErrorComponentNotFound                   = (EB_S32) 0x80001003,
        EB_ErrorInvalidComponent                    = (EB_S32) 0x80001004,
        EB_ErrorBadParameter                        = (EB_S32) 0x80001005,
        EB_ErrorNotImplemented                      = (EB_S32) 0x80001006,
        EB_ErrorCreateThreadFailed                  = (EB_S32) 0x80002010,
        EB_ErrorThreadUnresponsive                  = (EB_S32) 0x80002011,
        EB_ErrorDestroyThreadFailed                 = (EB_S32) 0x80002012,
        EB_ErrorNullThread                          = (EB_S32) 0x80002013,
        EB_ErrorCreateSemaphoreFailed               = (EB_S32) 0x80002020,
        EB_ErrorSemaphoreUnresponsive               = (EB_S32) 0x80002021,
        EB_ErrorDestroySemaphoreFailed              = (EB_S32) 0x80002022,
        EB_ErrorCreateMutexFailed                   = (EB_S32) 0x80002030,
        EB_ErrorMutexUnresponsive                   = (EB_S32) 0x80002031,
        EB_ErrorDestroyMutexFailed                  = (EB_S32) 0x80002032,
        EB_NoErrorEmptyQueue                        = (EB_S32) 0x80002033,
        EB_ErrorMax                                 = 0x7FFFFFFF
    } EB_ERRORTYPE;

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
		EB_U32                    size;                      // size of (data)
		EB_BOOL                   passthrough;               // whether this is passthrough data from application
		void(*releaseCbFncPtr)(struct EbLinkedListNode*); // callback to be executed by encoder when picture reaches end of pipeline, or
		// when aborting. However, at end of pipeline encoder shall
		// NOT invoke this callback if passthrough is TRUE (but
		// still needs to do so when aborting)
		void                     *data;                      // pointer to application's data
		struct EbLinkedListNode  *next;                      // pointer to next node (null when last)

	} EbLinkedListNode;

	typedef enum EbPtrType{
		EB_N_PTR = 0,                                   // malloc'd pointer
		EB_A_PTR = 1,                                   // malloc'd pointer aligned
		EB_MUTEX = 2,                                   // mutex
		EB_SEMAPHORE = 3,                                   // semaphore
		EB_THREAD = 4                                    // thread handle
	}EbPtrType;

	typedef struct EbMemoryMapEntry
	{
		EB_PTR                    ptr;                       // points to a memory pointer
		EbPtrType                 ptrType;                   // pointer type
	} EbMemoryMapEntry;

    typedef struct EB_BUFFERHEADERTYPE
    {
        EB_U32 nSize;
        EB_U8* pBuffer;
        EB_U32 nAllocLen;
        EB_U32 nFilledLen;
        EB_U32 nOffset;
        EB_PTR pAppPrivate;
        EB_U32 nTickCount;
        EB_S64 nTimeStamp;
        EB_U32 nFlags;
    } EB_BUFFERHEADERTYPE;

    typedef struct EB_PARAM_PORTDEFINITIONTYPE {
        EB_U32 nFrameWidth;
        EB_U32 nFrameHeight;
        EB_S32 nStride;
        EB_U32 nSize;
    } EB_PARAM_PORTDEFINITIONTYPE;

    typedef struct EB_COMPONENTTYPE
    {
        EB_U32 nSize;
        EB_PTR pComponentPrivate;
        EB_PTR pApplicationPrivate;
    } EB_COMPONENTTYPE;

#define EB_BUFFERFLAG_EOS 0x00000001 

#define MAX_APP_NUM_PTR                             (0x186A0 << 2)             // Maximum number of pointers to be allocated for the app 

	// Display Total Memory at the end of the memory allocations
#define DISPLAY_MEMORY                                  0

	extern    EbMemoryMapEntry        *appMemoryMap;            // App Memory table
	extern    EB_U32                  *appMemoryMapIndex;       // App Memory index
	extern    EB_U64                  *totalAppMemory;          // App Memory malloc'd
	extern    EB_U32                   appMallocCount;

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

#define EB_APP_MEMORY() \
    printf("Total Number of Mallocs in App: %d\n", appMallocCount); \
    printf("Total App Memory: %.2lf KB\n\n",*totalAppMemory/(double)1024);

	/* string copy */
	extern errno_t
		strcpy_ss(char *dest, rsize_t dmax, const char *src);

	/* fitted string copy */
	extern errno_t
		strncpy_ss(char *dest, rsize_t dmax, const char *src, rsize_t slen);

/* string length */
extern rsize_t
strnlen_ss(const char *s, rsize_t smax);



// TODO EbUtility.h is probably a better place for it, would require including EbUtility.h

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

#define EB_STRNCPY(dst, src, count) \
	strncpy_ss(dst, sizeof(dst), src, count)

#define EB_STRCPY(dst, size, src) \
	strcpy_ss(dst, size, src)

#define EB_STRCMP(target,token) \
	strcmp(target,token)

#define EB_STRLEN(target, max_size) \
	strnlen_ss(target, max_size)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // EbTypes_h
/* File EOF */
