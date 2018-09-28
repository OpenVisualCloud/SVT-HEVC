/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMotionVectorUnit_h
#define EbMotionVectorUnit_h

#include "EbTypes.h"
#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
#pragma pack(push, 1)

typedef union {
	struct 
	{
		signed short x;
		signed short y;
	}; 
	EB_U32 mvUnion;
} Mv_t;

#pragma pack(pop)

#pragma pack(push, 1)
typedef struct 
{
    signed   mvdX    : 16;
    signed   mvdY    : 16;
    unsigned refIdx  : 1;
    unsigned         : 7;
    unsigned predIdx : 1;
    unsigned         : 7;
   
} Mvd_t;
#pragma pack(pop)

typedef struct
{
	Mv_t            mv[MAX_NUM_OF_REF_PIC_LIST];
	EB_U8           predDirection;
} MvUnit_t;

#ifdef __cplusplus
}
#endif
#endif // EbMotionVectorUnit_h