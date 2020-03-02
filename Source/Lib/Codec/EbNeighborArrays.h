/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbNeighborArrays_h
#define EbNeighborArrays_h

#include "EbDefinitions.h"
#include "EbSyntaxElements.h"
#include "EbMotionVectorUnit.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
// Neighbor Array Granulairity
#define LCU_NEIGHBOR_ARRAY_GRANULARITY                  64
#define CU_NEIGHBOR_ARRAY_GRANULARITY                   8
#define PU_NEIGHBOR_ARRAY_GRANULARITY                   4
#define TU_NEIGHBOR_ARRAY_GRANULARITY                   4
#define SAMPLE_NEIGHBOR_ARRAY_GRANULARITY               1

typedef enum NEIGHBOR_ARRAY_TYPE
{
    NEIGHBOR_ARRAY_LEFT     = 0,
    NEIGHBOR_ARRAY_TOP      = 1,
    NEIGHBOR_ARRAY_TOPLEFT  = 2,
    NEIGHBOR_ARRAY_INVALID  = ~0
} NEIGHBOR_ARRAY_TYPE;

#define NEIGHBOR_ARRAY_UNIT_LEFT_MASK                   (1 << NEIGHBOR_ARRAY_LEFT)
#define NEIGHBOR_ARRAY_UNIT_TOP_MASK                    (1 << NEIGHBOR_ARRAY_TOP)
#define NEIGHBOR_ARRAY_UNIT_TOPLEFT_MASK                (1 << NEIGHBOR_ARRAY_TOPLEFT)

#define NEIGHBOR_ARRAY_UNIT_FULL_MASK                   (NEIGHBOR_ARRAY_UNIT_LEFT_MASK | NEIGHBOR_ARRAY_UNIT_TOP_MASK | NEIGHBOR_ARRAY_UNIT_TOPLEFT_MASK)
#define NEIGHBOR_ARRAY_UNIT_TOP_AND_LEFT_ONLY_MASK      (NEIGHBOR_ARRAY_UNIT_LEFT_MASK | NEIGHBOR_ARRAY_UNIT_TOP_MASK)

typedef struct NeighborArrayUnit_s
{
    EbDctor  dctor;
    EB_U8   *leftArray;
    EB_U8   *topArray;
    EB_U8   *topLeftArray;
    EB_U16   leftArraySize;
    EB_U16   topArraySize;
    EB_U16   topLeftArraySize;
    EB_U8    unitSize;
    EB_U8    granularityNormal;
    EB_U8    granularityNormalLog2;
    EB_U8    granularityTopLeft;
    EB_U8    granularityTopLeftLog2;

} NeighborArrayUnit_t;

extern EB_ERRORTYPE NeighborArrayUnitCtor(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U32   maxPictureWidth,
    EB_U32   maxPictureHeight,
    EB_U32   unitSize,
    EB_U32   granularityNormal,
    EB_U32   granularityTopLeft,
    EB_U32   typeMask);

extern void NeighborArrayUnitReset(NeighborArrayUnit_t *naUnitPtr);

extern EB_U32 GetNeighborArrayUnitLeftIndex(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U32               locY);

extern EB_U32 GetNeighborArrayUnitTopIndex(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U32               locX);

extern EB_U32 GetNeighborArrayUnitTopLeftIndex(
    NeighborArrayUnit_t *naUnitPtr,
    EB_S32               locX,
    EB_S32               locY);

extern void NeighborArrayUnitSampleWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *srcPtr,
    EB_U32               stride,
    EB_U32               srcOriginX,
    EB_U32               srcOriginY,
    EB_U32               picOriginX,
    EB_U32               picOriginY,
    EB_U32               blockWidth,
    EB_U32               blockHeight,
    EB_U32               neighborArrayTypeMask);

extern void NeighborArrayUnit16bitSampleWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U16               *srcPtr,
    EB_U32               stride,
    EB_U32               srcOriginX,
    EB_U32               srcOriginY,
    EB_U32               picOriginX,
    EB_U32               picOriginY,
    EB_U32               blockWidth,
    EB_U32               blockHeight,
    EB_U32               neighborArrayTypeMask);

extern void NeighborArrayUnitModeWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *value,
    EB_U32               originX,
    EB_U32               originY,
    EB_U32               blockWidth,
    EB_U32               blockHeight,
    EB_U32               neighborArrayTypeMask);

extern void NeighborArrayUnitDepthSkipWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *value,
    EB_U32               originX,
    EB_U32               originY,
    EB_U32               blockSize);

extern void NeighborArrayUnitIntraWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *value,
    EB_U32               originX,
    EB_U32               originY,
    EB_U32               blockSize);

extern void NeighborArrayUnitModeTypeWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *value,
    EB_U32               originX,
    EB_U32               originY,
    EB_U32               blockSize);

extern void NeighborArrayUnitMvWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *value,
    EB_U32               originX,
    EB_U32               originY,
    EB_U32               blockSize);
#ifdef __cplusplus
}
#endif
#endif //EbNeighborArrays_h
