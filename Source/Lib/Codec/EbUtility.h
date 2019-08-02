/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbUtility_h
#define EbUtility_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
/****************************
 * UTILITY FUNCTIONS
 ****************************/

// CU Stats Helper Functions
typedef struct CodedUnitStats_s 
{
    EB_U8   depth;
    EB_U8   size;
    EB_U8   sizeLog2;
    EB_U16  originX;
    EB_U16  originY;
    EB_U8   cuNumInDepth;
    EB_U8   parent32x32Index;

} CodedUnitStats_t;

// PU Stats Helper Functions
typedef struct PredictionUnitStats_t
{
    EB_U8  width;
    EB_U8  height; 
    EB_U8  offsetX;
    EB_U8  offsetY;

} PredictionUnitStats_t;

// TU Stats Helper Functions
typedef struct TransformUnitStats_s 
{
    EB_U8  depth;
    EB_U8  offsetX;
    EB_U8  offsetY;

} TransformUnitStats_t;

extern EB_U64 Log2fHighPrecision(EB_U64 x, EB_U8 precision);

extern const CodedUnitStats_t* GetCodedUnitStats(const EB_U32 cuIdx);
extern const TransformUnitStats_t* GetTransformUnitStats(const EB_U32 tuIdx);

#define PU_ORIGIN_ADJUST(cuOrigin, cuSize, offset) ((((cuSize) * (offset)) >> 2) + (cuOrigin))
#define PU_SIZE_ADJUST(cuSize, puSize) (((cuSize) * (puSize)) >> 2)

#define TU_ORIGIN_ADJUST(cuOrigin, cuSize, offset) ((((cuSize) * (offset)) >> 2) + (cuOrigin))
#define TU_SIZE_ADJUST(cuSize, tuDepth) ((cuSize) >> (tuDepth))

extern EB_ERRORTYPE ZOrderIncrement(EB_U32 *xLoc, EB_U32 *yLoc);
extern void ZOrderIncrementWithLevel(
    EB_U32 *xLoc,   
    EB_U32 *yLoc,   
    EB_U32 *level,  
    EB_U32 *index);

extern EB_U32 Log2f(EB_U32 x);
extern EB_U64 Log2f64(EB_U64 x);
extern EB_U32 EndianSwap(EB_U32 ui);

/****************************
 * MACROS
 ****************************/

#ifdef _MSC_VER
#define MULTI_LINE_MACRO_BEGIN do {
#define MULTI_LINE_MACRO_END \
	__pragma(warning(push)) \
	__pragma(warning(disable:4127)) \
	} while(0) \
	__pragma(warning(pop))
#else
#define MULTI_LINE_MACRO_BEGIN do {
#define MULTI_LINE_MACRO_END } while(0)
#endif

//**************************************************
// MACROS
//**************************************************
#define MAX(x, y)                       ((x)>(y)?(x):(y))
#define MIN(x, y)                       ((x)<(y)?(x):(y))
#define MEDIAN(a,b,c)                   ((a)>(b)?(a)>(c)?(b)>(c)?(b):(c):(a):(b)>(c)?(a)>(c)?(a):(c):(b))
#define CLIP3(MinVal, MaxVal, a)        (((a)<(MinVal)) ? (MinVal) : (((a)>(MaxVal)) ? (MaxVal) :(a)))
#define CLIP3EQ(MinVal, MaxVal, a)        (((a)<=(MinVal)) ? (MinVal) : (((a)>=(MaxVal)) ? (MaxVal) :(a)))
#define BITDEPTH_MIDRANGE_VALUE(precision)  (1 << ((precision) - 1))
#define SWAP(a, b)                      MULTI_LINE_MACRO_BEGIN (a) ^= (b); (b) ^= (a); (a) ^= (b); MULTI_LINE_MACRO_END
#define ABS(a)                          (((a) < 0) ? (-(a)) : (a))
#define EB_ABS_DIFF(a,b)                ((a) > (b) ? ((a) - (b)) : ((b) - (a)))
#define EB_DIFF_SQR(a,b)                (((a) - (b)) * ((a) - (b)))
#define SQR(x)                          ((x)*(x))
#define POW2(x)                         (1 << (x))
#define SIGN(a,b)                       (((a - b) < 0) ? (-1) : ((a - b) > 0) ? (1) : 0)
#define ROUND(a)                        (a >= 0) ? ( a + 1/2) : (a - 1/2);
#define UNSIGNED_DEC(x)                 MULTI_LINE_MACRO_BEGIN (x) = (((x) > 0) ? ((x) - 1) : 0); MULTI_LINE_MACRO_END
#define CIRCULAR_ADD(x,max)             (((x) >= (max)) ? ((x) - (max)) : ((x) < 0) ? ((max) + (x)) : (x))
#define CIRCULAR_ADD_UNSIGNED(x,max)    (((x) >= (max)) ? ((x) - (max)) : (x))
#define CEILING(x,base)                 ((((x) + (base) - 1) / (base)) * (base))
#define POW2_CHECK(x)                   ((x) == ((x) & (-((EB_S32)(x)))))
#define ROUND_UP_MUL_8(x)               ((x) + ((8 - ((x) & 0x7)) & 0x7))
#define ROUND_UP_MULT(x,mult)           ((x) + (((mult) - ((x) & ((mult)-1))) & ((mult)-1)))

// rounds down to the next power of two
#define FLOOR_POW2(x)       \
    MULTI_LINE_MACRO_BEGIN  \
		(x) |= ((x) >> 1);  \
        (x) |= ((x) >> 2);  \
        (x) |= ((x) >> 4);  \
        (x) |= ((x) >> 8);  \
        (x) |= ((x) >>16);  \
        (x) -= ((x) >> 1);  \
    MULTI_LINE_MACRO_END

// rounds up to the next power of two
#define CEIL_POW2(x)        \
    MULTI_LINE_MACRO_BEGIN  \
		(x) -= 1;           \
        (x) |= ((x) >> 1);  \
        (x) |= ((x) >> 2);  \
        (x) |= ((x) >> 4);  \
        (x) |= ((x) >> 8);  \
        (x) |= ((x) >>16);  \
        (x) += 1;           \
    MULTI_LINE_MACRO_END

// Calculates the Log2 floor of the integer 'x'
//   Intended to only be used for macro definitions
#define LOG2F(x) (              \
    ((x) < 0x0002u) ? 0u :      \
    ((x) < 0x0004u) ? 1u :      \
    ((x) < 0x0008u) ? 2u :      \
    ((x) < 0x0010u) ? 3u :      \
    ((x) < 0x0020u) ? 4u :      \
    ((x) < 0x0040u) ? 5u :      \
    ((x) < 0x0100u) ? 6u :      \
    ((x) < 0x0200u) ? 7u :      \
    ((x) < 0x0400u) ? 8u :      \
    ((x) < 0x0800u) ? 9u :      \
    ((x) < 0x1000u) ? 10u :     \
    ((x) < 0x2000u) ? 11u :     \
    ((x) < 0x4000u) ? 12u : 13u)

#define TWO_D_INDEX(x, y, stride)   \
    (((y) * (stride)) + (x))
        
// MAX_CU_COUNT is used to find the total number of partitions for the max partition depth and for
// each parent partition up to the root partition level (i.e. LCU level).

// MAX_CU_COUNT is given by SUM from k=1 to n (4^(k-1)), reduces by using the following finite sum
// SUM from k=1 to n (q^(k-1)) = (q^n - 1)/(q-1) => (4^n - 1) / 3
#define MAX_CU_COUNT(max_depth_count) ((((1 << (max_depth_count)) * (1 << (max_depth_count))) - 1)/3)

//**************************************************
// CONSTANTS
//**************************************************
#define MIN_UNSIGNED_VALUE      0
#define MAX_UNSIGNED_VALUE     ~0u
#define MIN_SIGNED_VALUE       ~0 - ((signed) (~0u >> 1))
#define MAX_SIGNED_VALUE       ((signed) (~0u >> 1))
#define MINI_GOP_MAX_COUNT			15
#define MINI_GOP_WINDOW_MAX_COUNT	 8	// widow subdivision: 8 x 3L

#define MIN_HIERARCHICAL_LEVEL		 2
static const EB_U32 MiniGopOffset[4] = { 1, 3, 7, 31 };

typedef struct MiniGopStats_s
{
	EB_U32  hierarchicalLevels;
	EB_U32  startIndex;
	EB_U32  endIndex;
	EB_U32  lenght;

} MiniGopStats_t;
extern const MiniGopStats_t* GetMiniGopStats(const EB_U32 miniGopIndex);
typedef enum MINI_GOP_INDEX {
	L6_INDEX   = 0,
	L5_0_INDEX = 1,
	L4_0_INDEX = 2,
	L3_0_INDEX = 3,
	L3_1_INDEX = 4,
	L4_1_INDEX = 5,
	L3_2_INDEX = 6,
	L3_3_INDEX = 7,
	L5_1_INDEX = 8,
	L4_2_INDEX = 9,
	L3_4_INDEX = 10,
	L3_5_INDEX = 11,
	L4_3_INDEX = 12,
	L3_6_INDEX = 13,
	L3_7_INDEX = 14
} MINI_GOP_INDEX;

//extern void EbStartTime(unsigned long long *Startseconds, unsigned long long *Startuseconds);
//extern void EbFinishTime(unsigned long long *Finishseconds, unsigned long long *Finishuseconds);
//extern void EbComputeOverallElapsedTime(unsigned long long Startseconds, unsigned long long Startuseconds, unsigned long long Finishseconds, unsigned long long Finishuseconds, double *duration);
//extern void EbComputeOverallElapsedTimeMs(unsigned long long Startseconds, unsigned long long Startuseconds, unsigned long long Finishseconds, unsigned long long Finishuseconds, double *duration);
#ifdef __cplusplus
}
#endif

#endif // EbUtility_h
