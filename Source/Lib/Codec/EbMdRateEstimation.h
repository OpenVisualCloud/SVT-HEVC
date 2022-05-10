/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMdRateEstimation_h
#define EbMdRateEstimation_h

#include "EbDefinitions.h"
#include "EbCabacContextModel.h"
#ifdef __cplusplus
extern "C" {
#endif
/**************************************
 * MD Rate Estimation Defines
 **************************************/

#define NUMBER_OF_SPLIT_FLAG_CASES                            6       // number of cases for bit estimation for split flag

#define NUMBER_OF_SKIP_FLAG_CASES                             6       // number of cases for bit estimation for skip flag

#define NUMBER_OF_MERGE_FLAG_CASES                            2       // number of cases for bit estimation for merge flag

#define NUMBER_OF_MERGE_INDEX_CASES                           5       // number of cases for bit estimation for merge index

#define NUMBER_OF_ALF_CTRL_FLAG_CASES                         0       // number of cases for bit estimation for ALF control flag

#define NUMBER_OF_INTRA_PART_SIZE_CASES                       2       // number of cases for bit estimation for Intra partition size

//Note ** to be modified after adding all AMP modes
#define NUMBER_OF_INTER_PART_SIZE_CASES                       8       // number of cases for bit estimation for Inter partition size

#define NUMBER_OF_AMP_XPOS_CASES                              0       // number of cases for bit estimation for asymmetric motion partition size

#define NUMBER_OF_AMP_YPOS_CASES                              0       // number of cases for bit estimation for asymmetric motion partition size

#define NUMBER_OF_PRED_MODE_CASES                             2       // number of cases for bit estimation for prediction mode

#define NUMBER_OF_INTRA_LUMA_CASES                            4       // number of cases for bit estimation for intra luma mode

#define NUMBER_OF_INTRA_CHROMA_CASES                          5       // number of cases for bit estimation for intra chroma mode

#define NUMBER_OF_INTER_BI_DIR_CASES                          8       // number of cases for bit estimation for inter bi-prediction direction : unipred - bipred per depth

#define NUMBER_OF_INTER_UNI_DIR_CASES                         2       // number of cases for bit estimation for inter uni-prediction direction : unipred List 0 - unipred List 1

#define NUMBER_OF_MVD_CASES                                  12       // number of cases for bit estimation for motion vector difference

#define NUMBER_OF_REF_PIC_CASES                               3       // number of cases for bit estimation for reference picture index

#define NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES                 6       // number of cases for bit estimation for transform sub-division flags

#define NUMBER_OF_CBF_CASES                                  10       // number of cases for bit estimation for CBF

#define TOTAL_NUMBER_OF_CBF_CASES                            20       // total number of cases for bit estimation for Y, CB and Cr Cbf

#define NUMBER_OF_ROOT_CBF_CASES                              2       // number of cases for bit estimation for ROOT CBF

#define NUMBER_OF_DELTA_QP_CASES                              0       // number of cases for bit estimation for delta QP

#define NUMBER_OF_SIG_FLAG_CASES                              0      // number of cases for bit estimation for significance flag

#define TOTAL_NUMBER_OF_SIG_FLAG_CASES                        0      // total number of cases for bit estimation for luma and chroma significance flag

#define NUMBER_OF_LAST_SIG_XY_CASES                           0      // number of cases for bit estimation for last significant XY flag

#define TOTAL_NUMBER_OF_LAST_SIG_XY_CASES                     0      // total number of cases for bit estimation for luma and chroma

#define NUMBER_OF_GREATER_ONE_COEFF_CASES                     0      // number of cases for bit estimation for coefficients greater than one

#define TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CASES               0      // total number of cases for bit estimation for luma and chroma coefficients greater than one

#define NUMBER_OF_GREATER_TWO_COEFF_CASES                     0      // number of cases for bit estimation for coefficients greater than two

#define TOTAL_NUMBER_OF_GREATER_TWO_COEFF_CASES               0      // total number of cases for bit estimation for luma and chroma coefficients greater than two

#define NUMBER_OF_MVP_INDEX_CASES                             2      // number of cases for bit estimation for MVP index

#define NUMBER_OF_ALF_FLAG_CASES                              0      // number of cases for bit estimation for ALF flag

#define NUMBER_OF_ALF_UVLC_CASES                              0      // number of cases for bit estimation for ALF UVLC (filter length)

#define NUMBER_OF_ALF_SVLC_CASES                              0      // number of cases for bit estimation for ALF SVLC (filter length)

#define NUMBER_OF_SAO_MERGE_FLAG_CASES                        2      // number of cases for bit estimation for SAO merge flags

#define NUMBER_OF_SAO_TYPE_INDEX_FLAG_CASES                   6      // number of cases for bit estimation for SAO Type

#define NUMBER_OF_SAO_OFFSET_TRUNUNARY_CASES                  8      // number of cases for bit estimation for SAO Offset trun unary case

#define MAX_SIZE_OF_MD_RATE_ESTIMATION_CASES                128      // This should be a power of 2

#define NUMBER_OF_MD_RATE_ESTIMATION_CASES1             (NUMBER_OF_SPLIT_FLAG_CASES +  NUMBER_OF_SKIP_FLAG_CASES + NUMBER_OF_MERGE_FLAG_CASES  + NUMBER_OF_MERGE_INDEX_CASES +          \
                                                         NUMBER_OF_ALF_CTRL_FLAG_CASES + NUMBER_OF_INTRA_PART_SIZE_CASES + NUMBER_OF_INTER_PART_SIZE_CASES + NUMBER_OF_AMP_XPOS_CASES + NUMBER_OF_AMP_YPOS_CASES)

#define NUMBER_OF_MD_RATE_ESTIMATION_CASES2             (NUMBER_OF_PRED_MODE_CASES +  NUMBER_OF_INTRA_LUMA_CASES + NUMBER_OF_INTRA_CHROMA_CASES  + NUMBER_OF_INTER_UNI_DIR_CASES + NUMBER_OF_INTER_BI_DIR_CASES +          \
                                                         NUMBER_OF_MVD_CASES + NUMBER_OF_REF_PIC_CASES +  NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES + TOTAL_NUMBER_OF_CBF_CASES)

#define NUMBER_OF_MD_RATE_ESTIMATION_CASES3             (NUMBER_OF_ROOT_CBF_CASES +  NUMBER_OF_DELTA_QP_CASES + TOTAL_NUMBER_OF_SIG_FLAG_CASES  + TOTAL_NUMBER_OF_LAST_SIG_XY_CASES +          \
                                                         TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CASES + TOTAL_NUMBER_OF_GREATER_TWO_COEFF_CASES +  TOTAL_NUMBER_OF_LAST_SIG_XY_CASES + NUMBER_OF_ALF_FLAG_CASES)

#define NUMBER_OF_MD_RATE_ESTIMATION_CASES4             (NUMBER_OF_ALF_UVLC_CASES +  NUMBER_OF_ALF_SVLC_CASES + NUMBER_OF_SAO_MERGE_FLAG_CASES + \
                                                         NUMBER_OF_SAO_TYPE_INDEX_FLAG_CASES + NUMBER_OF_SAO_OFFSET_TRUNUNARY_CASES+  NUMBER_OF_MVP_INDEX_CASES )

#define TOTAL_NUMBER_OF_MD_RATE_ESTIMATION_CASES        (NUMBER_OF_MD_RATE_ESTIMATION_CASES1 + \
                                                         NUMBER_OF_MD_RATE_ESTIMATION_CASES2 + \
                                                         NUMBER_OF_MD_RATE_ESTIMATION_CASES3 + \
                                                         NUMBER_OF_MD_RATE_ESTIMATION_CASES4)

#define NUMBER_OF_PAD_VALUES_IN_MD_RATE_ESTIMATION      (MAX_SIZE_OF_MD_RATE_ESTIMATION_CASES - TOTAL_NUMBER_OF_MD_RATE_ESTIMATION_CASES)

#define TOTAL_NUMBER_OF_MD_RATE_ESTIMATION_CASE_BUFFERS (TOTAL_NUMBER_OF_QP_VALUES * TOTAL_NUMBER_OF_SLICE_TYPES)

extern const EB_U32 NextStateMpsLps[];
extern const EB_U32 CabacEstimatedBits[];

// Update Context Model based on the current one and the bin
#define UPDATE_CONTEXT_MODEL(bin, contextModelPtr)  \
    NextStateMpsLps[ ((bin ^ (*contextModelPtr & 1))<<7) | (*contextModelPtr)]


/**************************************
 * The EB_BitFraction is used to define the bit fraction numbers
 **************************************/
typedef EB_U32 EB_BitFraction;

/**************************************
 * MD Rate Estimation Structure
 **************************************/
typedef struct MdRateEstimationContext_s {
    EB_BitFraction  splitFlagBits           [NUMBER_OF_SPLIT_FLAG_CASES];
    EB_BitFraction  skipFlagBits            [NUMBER_OF_SKIP_FLAG_CASES];
    EB_BitFraction  mvpIndexBits            [NUMBER_OF_MVP_INDEX_CASES];
    EB_BitFraction  intraPartSizeBits       [NUMBER_OF_INTRA_PART_SIZE_CASES];
    EB_BitFraction  interPartSizeBits       [NUMBER_OF_INTER_PART_SIZE_CASES];
    EB_BitFraction  predModeBits            [NUMBER_OF_PRED_MODE_CASES];
    EB_BitFraction  intraLumaBits           [NUMBER_OF_INTRA_LUMA_CASES];
    EB_BitFraction  intraChromaBits         [NUMBER_OF_INTRA_CHROMA_CASES];
    EB_BitFraction  refPicBits              [NUMBER_OF_REF_PIC_CASES];
    EB_BitFraction  mvdBits                 [NUMBER_OF_MVD_CASES];
    EB_BitFraction  lumaCbfBits             [NUMBER_OF_CBF_CASES];
    EB_BitFraction  chromaCbfBits           [NUMBER_OF_CBF_CASES];
    EB_BitFraction  rootCbfBits             [NUMBER_OF_ROOT_CBF_CASES];
    EB_BitFraction  transSubDivFlagBits     [NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CASES];
    EB_BitFraction  mergeFlagBits           [NUMBER_OF_MERGE_FLAG_CASES];
    EB_BitFraction  mergeIndexBits          [NUMBER_OF_MERGE_INDEX_CASES];

    EB_BitFraction  saoMergeFlagBits        [NUMBER_OF_SAO_MERGE_FLAG_CASES];
    EB_BitFraction  saoTypeIndexBits        [NUMBER_OF_SAO_TYPE_INDEX_FLAG_CASES];
    EB_BitFraction  saoOffsetTrunUnaryBits  [NUMBER_OF_SAO_OFFSET_TRUNUNARY_CASES];

    EB_BitFraction  interBiDirBits          [NUMBER_OF_INTER_BI_DIR_CASES];
    EB_BitFraction  interUniDirBits         [NUMBER_OF_INTER_UNI_DIR_CASES];

    /*  these might be added later
        EB_BitFraction  deltaQpBits            [NUMBER_OF_DELTA_QP_CASES];
        EB_BitFraction  lastSigXBits           [TOTAL_NUMBER_OF_LAST_SIG_XY_CASES];
        EB_BitFraction  lastSigYBits           [TOTAL_NUMBER_OF_LAST_SIG_XY_CASES];

        EB_BitFraction  significanceFlagBits   [TOTAL_NUMBER_OF_SIG_FLAG_CASES];
        EB_BitFraction  greaterThanOneBits     [TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CASES];
        EB_BitFraction  greaterThanTwoBits     [TOTAL_NUMBER_OF_GREATER_TWO_COEFF_CASES];
        EB_BitFraction  alfCtrlFlagBits        [NUMBER_OF_ALF_CTRL_FLAG_CASES];
        EB_BitFraction  alfFlagBits            [NUMBER_OF_ALF_FLAG_CASES];
        EB_BitFraction  alfUvlcBits            [NUMBER_OF_ALF_UVLC_CASES];
        EB_BitFraction  alfSvlcBits            [NUMBER_OF_ALF_SVLC_CASES];

        // This is AMP related, rename later
        EB_BitFraction  xPosiBits              [NUMBER_OF_AMP_XPOS_CASES];
        EB_BitFraction  yPosiBits              [NUMBER_OF_AMP_YPOS_CASES];
    */
    // The padJunk buffer is present to make this structure a power of 2. This should keep the structure on cache lines and very fast to access
    EB_BitFraction  padJunkBuffer          [NUMBER_OF_PAD_VALUES_IN_MD_RATE_ESTIMATION];

} MdRateEstimationContext_t;


/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE MdRateEstimationContextInit(MdRateEstimationContext_t *mdRateEstimationArray,
                                        ContextModelEncContext_t  *cabacContextModelArray);

extern EB_ERRORTYPE EbHevcGetMvdFractionBits(
    EB_S32                      mvdX,
    EB_S32                      mvdY,
    MdRateEstimationContext_t  *mdRateEstimationArray,
    EB_U64                     *fractionBitNum);

EB_ERRORTYPE GetRefIndexFractionBits(
    MdRateEstimationContext_t  *mdRateEstimationArray,
    EB_U64                     *fractionBitNum);

extern EB_ERRORTYPE MeEbHevcGetMvdFractionBits(
    EB_S32                      mvdX,
    EB_S32                      mvdY,
    EB_BitFraction             *mvdBitsPtr,
    EB_U32                     *fractionBitNum);

extern EB_ERRORTYPE GetSaoOffsetsFractionBits(
    EB_U32                     saoType,
    EB_S32                     *offset,
    MdRateEstimationContext_t  *mdRateEstimationArray,
    EB_U64                     *fractionBitNum);

extern EB_ERRORTYPE UpdateRateEstimation(
    MdRateEstimationContext_t       *mdRateEstimationTemp,
    SyntaxContextModelEncContext_t  *cabacContextModelPtr);
#ifdef __cplusplus
}
#endif

#endif //EbMdRateEstimationTables_h