/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#include "EbUtility.h"
#include "EbDefinitions.h"

/*****************************************
 * Z-Order
 *****************************************/
EB_ERRORTYPE ZOrderIncrement(
    EB_U32 *xLoc,   // x location, level agnostic
    EB_U32 *yLoc)   // y location, level agnostic
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_U32 mask;

    // The basic idea of this function is to increment an x,y coordinate
    // that has had its size removed to the next z-coding order location.
    //
    // In a four quadrant partition, the z coding order is [0,0], [1,0], [0,1], [1,1]
    // Some observations (only looking at one bit position or the LSB) are:
    //  1. X is always toggled (achieved with X ^= 0x1)
    //  2. Y can be toggled with (Y = Y ^ X)
    //  3. Recall that a value XOR'ed with 1 toggles, and XOR'ed with 0 stays the same
    //
    //  Extending this logic is somewhat trickier. The two main observations to make are
    //  4. The LSB of X and Y are always progressed.
    //  5. Every other bit-position, N, other than the LSB are progressed in their state
    //     when the N-1 bit position resets back to [0,0].
    //
    //  From 5, we can infer the need of a "progression mask" of the form 0x1, 0x3, 0x7, 0xF, etc.
    //  The first step of contructing the mask is to find which bit positions are ready to
    //  reset (found by X & Y) and setting the LSB of the mask to 1 (the LSB always progresses).
    //  The second step is to eliminate all ones from the mask above the lowest-ordered zero bit.
    //  Note we can achieve more precision in the second mask step by more masking-out operations,
    //  but for a 64 -> 4 (5 steps), the precision below is sufficient.
    //
    //  Finally, X and Y are progressed only at the bit-positions in the mask.

    mask  = ((*xLoc & *yLoc) << 1) | 0x1;
    mask &= (mask << 1) | 0x01;
    mask &= (mask << 2) | 0x03;
    mask &= (mask << 4) | 0x0F;
    mask &= (mask << 8) | 0xFF;

    *yLoc ^= *xLoc & mask;
    *xLoc ^= mask;

    return return_error;
}

/*****************************************
 * Z-Order Increment with Level
 *   This is the main function for progressing
 *   through a treeblock's coding units. To get
 *   the true CU size, multiple the xLoc, yLoc
 *   by the smallest CU size.
 *****************************************/
void ZOrderIncrementWithLevel(
    EB_U32 *xLoc,   // x location, units of smallest block size
    EB_U32 *yLoc,   // y location, units of smallest block size
    EB_U32 *level,  // level, number of block size-steps from the smallest block size
    //   (e.g. if 8x8 = level 0, 16x16 = level 1, 32x32 == level 2, 64x64 == level 3)
    EB_U32 *index)  // The CU index, can be used to index a lookup table (see GetCodedUnitStats)
{
    EB_U32 mask;

    // The basic idea of this function is to increment an x,y coordinate
    // that has had its size removed to the next z-coding order location.
    //
    // In a four quadrant partition, the z coding order is [0,0], [1,0], [0,1], [1,1]
    // Some observations (only looking at one bit position or the LSB) are:
    //  1. X is always toggled (achieved with X ^= 0x1)
    //  2. Y can be toggled with (Y = Y ^ X)
    //  3. Recall that a value XOR'ed with 1 toggles, and XOR'ed with 0 stays the same
    //
    //  Extending this logic is somewhat trickier. The two main observations to make are
    //  4. The LSB of X and Y are always progressed.
    //  5. Every other bit-position, N, other than the LSB are progressed in their state
    //     when the N-1 bit position resets back to [0,0].
    //
    //  From 5, we can infer the need of a "progression mask" of the form 0x1, 0x3, 0x7, 0xF, etc.
    //  The first step of contructing the mask is to find which bit positions are ready to
    //  reset (found by X & Y) and setting the LSB of the mask to 1 (the LSB always progresses).
    //  The second step is to eliminate all ones from the mask above the lowest-ordered zero bit.
    //  Note we can achieve more precision in the second mask step by more masking-out operations,
    //  but for a 64 -> 4 (5 steps), the precision below is sufficient.
    //
    //  Finally, X and Y are progressed only at the bit-positions in the mask.

    // Seed the mask
    mask  = ((*xLoc & *yLoc) << 1) | 0x1;

    // This step zero-outs the mask if level is not zero.
    //   The purpose of this is step further down the tree
    //   if not already at the bottom of the tree
    //   Equivalent to: mask = (level > 0) ? mask : 0;
    mask &= (EB_U32)(-(*level == 0));

    // Construct the mask
    mask &= (mask << 1) | 0x01;
    mask &= (mask << 2) | 0x03;
    mask &= (mask << 4) | 0x0F;
    mask &= (mask << 8) | 0xFF;

    // Decrement the level if not already at the bottom of the tree
    //  Equivalent to level = (level > 0) ? level - 1 : 0;
    *level = (*level-1) & -(*level > 0);

    // If at one of the "corner" positions where the mask > 1, we
    //   need to increase the level since larger blocks are processed
    //   before smaller blocks.  Note that by using mask, we are protected
    //   against inadvertently incrementing the level if not already at
    //   the bottom of the tree.  The level increment should really be
    //   Log2f(mask >> 1), but since there are only 3 valid positions,
    //   we are using a cheesy Log2f approximation
    //   Equivalent to: level += (mask > 3) ? 2 : mask >> 1;

    *level += ((2 ^ (mask >> 1)) & -(mask > 3)) ^ (mask >> 1);

    // Increment the xLoc, yLoc.  Note that this only occurs when
    //   we are at the bottom of the tree.
    *yLoc ^= *xLoc & mask;
    *xLoc ^= mask;

    // Increment the index. Note that the natural progression of this
    //   block aligns with how leafs are stored in the accompanying
    //   CU data structures.
    ++(*index);

    return;
}

static CodedUnitStats_t CodedUnitStatsArray[] = {

//   Depth       Size      SizeLog2     OriginX    OriginY   cuNumInDepth   Index
    {0,           64,         6,           0,         0,        0     ,   0    },   // 0
    {1,           32,         5,           0,         0,        0     ,   1    },   // 1
    {2,           16,         4,           0,         0,        0     ,   1    },   // 2
    {3,            8,         3,           0,         0,        0     ,   1    },   // 3
    {3,            8,         3,           8,         0,        1     ,   1    },   // 4
    {3,            8,         3,           0,         8,        8     ,   1    },   // 5
    {3,            8,         3,           8,         8,        9     ,   1    },   // 6
    {2,           16,         4,          16,         0,        1     ,   1    },   // 7
    {3,            8,         3,          16,         0,        2     ,   1    },   // 8
    {3,            8,         3,          24,         0,        3     ,   1    },   // 9
    {3,            8,         3,          16,         8,        10    ,   1     },  // 10
    {3,            8,         3,          24,         8,        11    ,   1     },  // 11
    {2,           16,         4,           0,        16,        4     ,   1    },   // 12
    {3,            8,         3,           0,        16,        16    ,   1     },  // 13
    {3,            8,         3,           8,        16,        17    ,   1     },  // 14
    {3,            8,         3,           0,        24,        24    ,   1     },  // 15
    {3,            8,         3,           8,        24,        25    ,   1     },  // 16
    {2,           16,         4,          16,        16,        5     ,   1    },   // 17
    {3,            8,         3,          16,        16,        18    ,   1     },  // 18
    {3,            8,         3,          24,        16,        19    ,   1     },  // 19
    {3,            8,         3,          16,        24,        26    ,   1     },  // 20
    {3,            8,         3,          24,        24,        27    ,   1     },  // 21
    {1,           32,         5,          32,         0,        1     ,   2    },   // 22
    {2,           16,         4,          32,         0,        2     ,   2    },   // 23
    {3,            8,         3,          32,         0,        4     ,   2    },   // 24
    {3,            8,         3,          40,         0,        5     ,   2    },   // 25
    {3,            8,         3,          32,         8,        12    ,   2     },  // 26
    {3,            8,         3,          40,         8,        13    ,   2     },  // 27
    {2,           16,         4,          48,         0,        3     ,   2    },   // 28
    {3,            8,         3,          48,         0,        6     ,   2    },   // 29
    {3,            8,         3,          56,         0,        7     ,   2    },   // 30
    {3,            8,         3,          48,         8,        14    ,   2     },  // 31
    {3,            8,         3,          56,         8,        15    ,   2     },  // 32
    {2,           16,         4,          32,        16,        6     ,   2    },   // 33
    {3,            8,         3,          32,        16,        20    ,   2     },  // 34
    {3,            8,         3,          40,        16,        21    ,   2     },  // 35
    {3,            8,         3,          32,        24,        28    ,   2     },  // 36
    {3,            8,         3,          40,        24,        29    ,   2     },  // 37
    {2,           16,         4,          48,        16,        7     ,   2    },   // 38
    {3,            8,         3,          48,        16,        22    ,   2     },  // 39
    {3,            8,         3,          56,        16,        23    ,   2     },  // 40
    {3,            8,         3,          48,        24,        30    ,   2     },  // 41
    {3,            8,         3,          56,        24,        31    ,   2     },  // 42
    {1,           32,         5,           0,        32,        2     ,   3    },   // 43
    {2,           16,         4,           0,        32,        8     ,   3    },   // 44
    {3,            8,         3,           0,        32,        32    ,   3     },  // 45
    {3,            8,         3,           8,        32,        33    ,   3     },  // 46
    {3,            8,         3,           0,        40,        40    ,   3     },  // 47
    {3,            8,         3,           8,        40,        41    ,   3     },  // 48
    {2,           16,         4,          16,        32,        9     ,   3    },   // 49
    {3,            8,         3,          16,        32,        34    ,   3     },  // 50
    {3,            8,         3,          24,        32,        35    ,   3     },  // 51
    {3,            8,         3,          16,        40,        42    ,   3     },  // 52
    {3,            8,         3,          24,        40,        43    ,   3     },  // 53
    {2,           16,         4,           0,        48,        12    ,   3     },  // 54
    {3,            8,         3,           0,        48,        48    ,   3     },  // 55
    {3,            8,         3,           8,        48,        49    ,   3     },  // 56
    {3,            8,         3,           0,        56,        56    ,   3     },  // 57
    {3,            8,         3,           8,        56,        57    ,   3     },  // 58
    {2,           16,         4,          16,        48,        13    ,   3     },  // 59
    {3,            8,         3,          16,        48,        50    ,   3     },  // 60
    {3,            8,         3,          24,        48,        51    ,   3     },  // 61
    {3,            8,         3,          16,        56,        58    ,   3     },  // 62
    {3,            8,         3,          24,        56,        59    ,   3     },  // 63
    {1,           32,         5,          32,        32,        3     ,   4     },  // 64
    {2,           16,         4,          32,        32,        10    ,   4     },  // 65
    {3,            8,         3,          32,        32,        36    ,   4     },  // 66
    {3,            8,         3,          40,        32,        37    ,   4     },  // 67
    {3,            8,         3,          32,        40,        44    ,   4     },  // 68
    {3,            8,         3,          40,        40,        45    ,   4     },  // 69
    {2,           16,         4,          48,        32,        11    ,   4     },  // 70
    {3,            8,         3,          48,        32,        38    ,   4     },  // 71
    {3,            8,         3,          56,        32,        39    ,   4     },  // 72
    {3,            8,         3,          48,        40,        46    ,   4     },  // 73
    {3,            8,         3,          56,        40,        47    ,   4     },  // 74
    {2,           16,         4,          32,        48,        14    ,   4     },  // 75
    {3,            8,         3,          32,        48,        52    ,   4     },  // 76
    {3,            8,         3,          40,        48,        53    ,   4     },  // 77
    {3,            8,         3,          32,        56,        60    ,   4     },  // 78
    {3,            8,         3,          40,        56,        61    ,   4     },  // 79
    {2,           16,         4,          48,        48,        15    ,   4     },  // 80
    {3,            8,         3,          48,        48,        54    ,   4     },  // 81
    {3,            8,         3,          56,        48,        55    ,   4     },  // 82
    {3,            8,         3,          48,        56,        62    ,   4     },  // 83
    {3,            8,         3,          56,        56,        63    ,   4     }   // 84
};

/**************************************************************
 * Get Coded Unit Statistics
 **************************************************************/
const CodedUnitStats_t* GetCodedUnitStats(const EB_U32 cuIdx)
{
    return &CodedUnitStatsArray[cuIdx];
}

static const TransformUnitStats_t TransformUnitStatsArray[] = {
	//
	//        depth
	//       /
	//      /       offsetX (units of the current depth)
	//     /       /
	//    /       /       offsetY (units of the current depth)
	//   /       /       /
	{0,     0,      0},     // 0
	{1,     0,      0},     // 1
	{1,     2,      0},     // 2
	{1,     0,      2},     // 3
	{1,     2,      2},     // 4
	{2,     0,      0},     // 5
	{2,     1,      0},     // 6
	{2,     0,      1},     // 7
	{2,     1,      1},     // 8
	{2,     2,      0},     // 9
	{2,     3,      0},     // 10
	{2,     2,      1},     // 11
	{2,     3,      1},     // 12
	{ 2,	0,		2},     // 13
	{ 2,	1,		2},     // 14
	{ 2,	0,		3},     // 15
	{ 2,	1,		3},     // 16
	{ 2,	2,		2},     // 17
	{ 2,	3,		2},     // 18
	{ 2,	2,		3},     // 19
	{ 2,	3,		3},    // 20
	{0xFF,  0xFF,   0xFF}   // Invalid
};

/**************************************************************
 * Get Transform Unit Statistics
 **************************************************************/
const TransformUnitStats_t* GetTransformUnitStats(const EB_U32 tuIdx)
{
    return &TransformUnitStatsArray[tuIdx];
}

/*****************************************
 * Integer Log 2
 *  This is a quick adaptation of a Number
 *  Leading Zeros (NLZ) algorithm to get
 *  the log2f of an integer
 *****************************************/
/*EB_U32 Log2f(EB_U32 x)
{
    EB_U32 y;
    EB_S32 n = 32, c = 16;

    do {
        y = x >> c;
        if (y > 0) {
            n -= c;
            x = y;
        }
        c >>= 1;
    } while (c > 0);

    return 32 - n;
}*/

/*****************************************
 * Long Log 2
 *  This is a quick adaptation of a Number
 *  Leading Zeros (NLZ) algorithm to get
 *  the log2f of a 64-bit number
 *****************************************/
inline EB_U64 Log2f64(EB_U64 x)
{
    EB_U64 y;
    EB_S64 n = 64, c = 32;

    do {
        y = x >> c;
        if (y > 0) {
            n -= c;
            x = y;
        }
        c >>= 1;
    } while (c > 0);

    return 64 - n;
}

/*****************************************
 * Endian Swap
 *****************************************/
EB_U32 EndianSwap(EB_U32 ui)
{
    unsigned int ul2;

    ul2  = ui>>24;
    ul2 |= (ui>>8) & 0x0000ff00;
    ul2 |= (ui<<8) & 0x00ff0000;
    ul2 |= ui<<24;

    return ul2;

}

EB_U64 Log2fHighPrecision(EB_U64 x, EB_U8 precision)
{

    EB_U64 sigBitLocation = Log2f64(x);
    EB_U64 Remainder = x - ((EB_U64)1 << (EB_U8) sigBitLocation);
    EB_U64 result;

    result = (sigBitLocation<<precision) + ((Remainder<<precision) / ((EB_U64)1<< (EB_U8) sigBitLocation));

    return result;

}

static const MiniGopStats_t MiniGopStatsArray[] = {

	//	HierarchicalLevels	StartIndex	EndIndex	Lenght	miniGopIndex
	{ 5,  0, 31, 32 },	// 0
	{ 4,  0, 15, 16 },	// 1
	{ 3,  0,  7,  8 },	// 2
	{ 2,  0,  3,  4 },	// 3
	{ 2,  4,  7,  4 },	// 4
	{ 3,  8, 15,  8 },	// 5
	{ 2,  8, 11,  4 },	// 6
	{ 2, 12, 15,  4 },	// 7
	{ 4, 16, 31, 16 },	// 8
	{ 3, 16, 23,  8 },	// 9
	{ 2, 16, 19,  4 },	// 10
	{ 2, 20, 23,  4 },	// 11
	{ 3, 24, 31,  8 },	// 12
	{ 2, 24, 27,  4 },	// 13
	{ 2, 28, 31,  4 }	// 14
};

/**************************************************************
* Get Mini GOP Statistics
**************************************************************/
const MiniGopStats_t* GetMiniGopStats(const EB_U32 miniGopIndex) {
    return &MiniGopStatsArray[miniGopIndex];
}

void EbStartTime(unsigned long long *Startseconds, unsigned long long *Startuseconds) {
#ifdef _WIN32
    *Startseconds = (unsigned long long)clock();
    (void)(*Startuseconds);
#else
    struct timeval start;
    gettimeofday(&start, NULL);
    *Startseconds = start.tv_sec;
    *Startuseconds = start.tv_usec;
#endif
}

void EbFinishTime(unsigned long long *Finishseconds, unsigned long long *Finishuseconds) {
#ifdef _WIN32
    *Finishseconds = (unsigned long long)clock();
    (void)(*Finishuseconds);
#else
    struct timeval finish;
    gettimeofday(&finish, NULL);
    *Finishseconds = finish.tv_sec;
    *Finishuseconds = finish.tv_usec;
#endif
}

void EbComputeOverallElapsedTime(unsigned long long Startseconds, unsigned long long Startuseconds, unsigned long long Finishseconds, unsigned long long Finishuseconds, double *duration) {
#ifdef _WIN32
    //double  duration;
    *duration = (double)(Finishseconds - Startseconds) / CLOCKS_PER_SEC;
    //SVT_LOG("\nElapsed time: %3.3f seconds\n", *duration);
    (void)(Startuseconds);
    (void)(Finishuseconds);
#else
    long   mtime, seconds, useconds;
    seconds = Finishseconds - Startseconds;
    useconds = Finishuseconds - Startuseconds;
    mtime = ((seconds) * 1000 + useconds / 1000.0) + 0.5;
    *duration = (double)mtime / 1000;
    //SVT_LOG("\nElapsed time: %3.3ld seconds\n", mtime/1000);
#endif
}

void EbComputeOverallElapsedTimeMs(unsigned long long Startseconds, unsigned long long Startuseconds, unsigned long long Finishseconds, unsigned long long Finishuseconds, double *duration) {
#ifdef _WIN32
    //double  duration;
    *duration = (double)(Finishseconds - Startseconds);
    //SVT_LOG("\nElapsed time: %3.3f seconds\n", *duration);
    (void)(Startuseconds);
    (void)(Finishuseconds);
#else
    long   mtime, seconds, useconds;
    seconds = Finishseconds - Startseconds;
    useconds = Finishuseconds - Startuseconds;
    mtime = ((seconds) * 1000 + useconds / 1000.0) + 0.5;
    *duration = (double)mtime;
    //SVT_LOG("\nElapsed time: %3.3ld seconds\n", mtime/1000);
#endif
}
