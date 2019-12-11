/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
* SPDX-License-Identifier: GPL-2.0
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
extern void EbHevcZOrderIncrementWithLevel(
    EB_U32 *xLoc,
    EB_U32 *yLoc,
    EB_U32 *level,
    EB_U32 *index);

extern EB_U32 Log2f(EB_U32 x);
extern EB_U64 EbHevcLog2f64(EB_U64 x);
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

void EbHevcStartTime(EB_U64 *Startseconds, EB_U64 *Startuseconds);
void EbHevcFinishTime(EB_U64 *Finishseconds, EB_U64 *Finishuseconds);
void EbHevcComputeOverallElapsedTime(EB_U64 Startseconds, EB_U64 Startuseconds, EB_U64 Finishseconds, EB_U64 Finishuseconds, double *duration);
void EbHevcComputeOverallElapsedTimeMs(EB_U64 Startseconds, EB_U64 Startuseconds, EB_U64 Finishseconds, EB_U64 Finishuseconds, double *duration);

/*
 * Simple doubly linked list implementation copied from Linux kernel.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

#define LIST_POISON1 ((void *)0x00100100)
#define LIST_POISON2 ((void *)0x00200200)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

static inline void __list_add(struct list_head *new,
        struct list_head *prev, struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = LIST_POISON1;
    entry->prev = LIST_POISON2;
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}
#ifdef __cplusplus
}
#endif

#endif // EbUtility_h
