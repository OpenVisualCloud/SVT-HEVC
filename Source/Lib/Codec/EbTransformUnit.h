/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbTransformUnit_h
#define EbTransformUnit_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif
#define TRANSFORM_UNIT_MAX_COUNT    5
#define TRANSFORM_UNIT_2Nx2N_AREA   16
#define TRANSFORM_UNIT_NxN_AREA     4

#pragma pack(push, 1)
typedef struct TransformUnit_s {
    unsigned                tuIndex               : 5;
    unsigned                lumaCbfContext        : 1;
    unsigned                splitFlag             : 1;
    unsigned                chromaCbfContext      : 2;
    unsigned                cbCbf                 : 1;
    unsigned                crCbf                 : 1;
    unsigned                lumaCbf               : 1;
	unsigned                transCoeffShapeLuma   : 2;
	unsigned                transCoeffShapeChroma : 2;
	unsigned                transCoeffShapeChroma2 : 2;

    unsigned                cbCbf2                : 1;
    unsigned                crCbf2                : 1;
    EB_U16                  nzCoefCount[3];
	EB_BOOL					isOnlyDc[3];

    EB_U16                  nzCoefCount2[2];
    EB_BOOL                 isOnlyDc2[3];
} TransformUnit_t; 
#pragma pack(pop)
#ifdef __cplusplus
}
#endif
#endif // EbTransformUnit_h
