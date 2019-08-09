/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMvMerge_h
#define EbMvMerge_h

#include "EbDefinitions.h"
#include "EbPictureControlSet.h"
#include "EbCodingUnit.h"
#include "EbPredictionUnit.h"
#include "EbNeighborArrays.h"
#ifdef __cplusplus
extern "C" {
#endif
/** This macro is used to compare if two PUs have the same MVs (same refPicIndex, same MV_x and same MV_y)
    in a particular reference picture list.
*/
#define CHECK_MV_EQUIVALENT(pu1PredDir, pu1RefIdx, pu1Mv_x, pu1Mv_y, pu2PredDir, pu2RefIdx, pu2Mv_x, pu2Mv_y, refPicList)    (                                                \
                (!(((pu1PredDir) + 1) & ( 1 << (refPicList))) &&                           \
                !(((pu2PredDir) + 1) & ( 1 << (refPicList)))) ||                           \
                ((((pu1PredDir) + 1) & ( 1 << (refPicList))) &&                            \
                (((pu2PredDir) + 1) & ( 1 << (refPicList))) &&                             \
                ((pu1RefIdx) == (pu2RefIdx)) &&     \
                ((pu1Mv_x) == (pu2Mv_x)) &&       \
                ((pu1Mv_y) == (pu2Mv_y)))         \
                )

typedef struct MvMergeCandidate_s {
    Mv_t    mv[MAX_NUM_OF_REF_PIC_LIST];
    EB_U8   predictionDirection;
} MvMergeCandidate_t;


#ifdef __cplusplus
}
#endif
#endif // EbMvMerge_h
