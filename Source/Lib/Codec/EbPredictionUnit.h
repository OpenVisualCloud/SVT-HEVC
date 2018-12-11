/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPredictionUnit_h
#define EbPredictionUnit_h

#include "EbSyntaxElements.h"
#include "EbDefinitions.h"
#include "EbMotionVectorUnit.h"
#ifdef __cplusplus
extern "C" {
#endif
#pragma pack(push, 1)
typedef struct PredictionUnit_s 
{
	Mv_t                          mv [MAX_NUM_OF_REF_PIC_LIST];  // 16-bytes
    Mvd_t                         mvd[MAX_NUM_OF_REF_PIC_LIST]; // 16-bytes
    unsigned                      mergeIndex                : 5;
	unsigned                      mergeFlag                 : 1;
    
    unsigned                      interPredDirectionIndex   : 2;
	unsigned                      intraLumaMode             : 6;	
	unsigned                      intraLumaLeftMode         : 6;
	unsigned                      intraLumaTopMode          : 6;

} PredictionUnit_t;
#pragma pack(pop)


#ifdef __cplusplus
}
#endif
#endif //EbPredictionUnit_h