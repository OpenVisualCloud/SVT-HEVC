/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMotionEstimationLcuResults_h
#define EbMotionEstimationLcuResults_h

#include "EbDefinitions.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_ME_PU_COUNT         85  // Sum of all the possible partitions which have both deminsions greater than 4.
	// i.e. no 4x4, 8x4, or 4x8 partitions
#define SQUARE_PU_COUNT          85
#define MAX_ME_CANDIDATE_PER_PU   3

typedef struct MeCandidate_s {

	union {
		struct {
			signed short     xMvL0 ;  //Note: Do not change the order of these fields
			signed short     yMvL0 ;
			signed short     xMvL1 ;
			signed short     yMvL1 ;
        }mv;
		EB_U64 MVs;
	};

	unsigned    distortion : 32;     // 20-bits holds maximum SAD of 64x64 PU

	unsigned    direction : 8;      // 0: uni-pred L0, 1: uni-pred L1, 2: bi-pred

} MeCandidate_t;

// move this to a new file with ctor & dtor
typedef struct MeLcuResults_s {
    EbDctor         dctor;
    EB_U32          lcuDistortion;
    EB_U8          *totalMeCandidateIndex;
    EB_S16          xMvHmeSearchCenter[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    EB_S16          yMvHmeSearchCenter[MAX_NUM_OF_REF_PIC_LIST][MAX_REF_IDX];
    MeCandidate_t **meCandidate;
    MeCandidate_t  *meCandidateArray;

} MeLcuResults_t;




typedef struct  DistDir_s{
	unsigned    distortion : 32; //20bits are enough
	unsigned    direction : 2;
} DistDir_t;


typedef struct MeCuResults_s {
	union {
		struct {
			signed short     xMvL0;
			signed short     yMvL0;
			signed short     xMvL1;
			signed short     yMvL1;
		};
		EB_U64 MVs;
	};

	DistDir_t    distortionDirection[3];

	EB_U8        totalMeCandidateIndex;

} MeCuResults_t;

#ifdef __cplusplus
}
#endif
#endif // EbMotionEstimationLcuResults_h
