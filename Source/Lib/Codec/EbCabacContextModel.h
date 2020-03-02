/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbCabacContextModel_h
#define EbCabacContextModel_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

    /**************************************
    * Cabac Context Model Defines
    **************************************/

#define NUMBER_OF_SPLIT_FLAG_CONTEXT_MODELS                            3       // number of context models for split flag

#define NUMBER_OF_SKIP_FLAG_CONTEXT_MODELS                             3       // number of context models for skip flag

#define NUMBER_OF_MERGE_FLAG_CONTEXT_MODELS                            1       // number of context models for merge flag 

#define NUMBER_OF_MERGE_INDEX_CONTEXT_MODELS                           1       // number of context models for merge index 

#define NUMBER_OF_ALF_CTRL_FLAG_CONTEXT_MODELS                         1       // number of context models for ALF control flag

#define NUMBER_OF_PART_SIZE_CONTEXT_MODELS                             4       // number of context models for partition size

#define NUMBER_OF_CU_AMP_CONTEXT_MODELS                                1       // number of cu AMP

#define NUMBER_OF_PRED_MODE_CONTEXT_MODELS                             1       // number of context models for prediction mode

#define NUMBER_OF_INTRA_LUMA_CONTEXT_MODELS                            1       // number of context models for intra luma mode

#define NUMBER_OF_INTRA_CHROMA_CONTEXT_MODELS                          2       // number of context models for intra chroma mode

#define NUMBER_OF_INTER_DIR_CONTEXT_MODELS                             5       // number of context models for inter prediction direction

#define NUMBER_OF_MVD_CONTEXT_MODELS                                   2       // number of context models for motion vector difference

#define NUMBER_OF_REF_PIC_CONTEXT_MODELS                               2       // number of context models for reference picture index

#define NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CONTEXT_MODELS                 3       // number of context models for transform sub-division flags

#define NUMBER_OF_CBF_CONTEXT_MODELS                                   5       // number of context models for CBF

#define TOTAL_NUMBER_OF_CBF_CONTEXT_MODELS                            10       // total number of context models for Y and  (Chroma) Cbf

#define NUMBER_OF_ROOT_CBF_CONTEXT_MODELS                              1       // number of context models for ROOT CBF

#define NUMBER_OF_DELTA_QP_CONTEXT_MODELS                              3       // number of context models for delta QP

#define NUMBER_OF_SIG_FLAG_LUMA_CONTEXT_MODELS                        27       // number of context models for significance flag for Luma

#define NUMBER_OF_SIG_FLAG_CHROMA_CONTEXT_MODELS                      15       // number of context models for significance flag for Chroma

#define TOTAL_NUMBER_OF_SIG_FLAG_CONTEXT_MODELS                       42       // total number of context models for luma and chroma significance flag

#define NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS                  2       // number of context models for luma and chroma Coefficient group significance flag

#define TOTAL_NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS            4       // Total number of context models for luma and chroma Coefficient group significance flag

#define NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS                          15       // number of context models for last significant XY flag

#define TOTAL_NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS                    30       // total number of context models for luma and chroma 

#define NUMBER_OF_GREATER_ONE_COEFF_LUMA_CONTEXT_MODELS               16       // number of context models for coefficients greater than one for Luma

#define NUMBER_OF_GREATER_ONE_COEFF_CHROMA_CONTEXT_MODELS              8       // number of context models for coefficients greater than one for Chroma

#define TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CONTEXT_MODELS              24       // total number of context models for luma and chroma coefficients greater than one

#define NUMBER_OF_GREATER_TWO_COEFF_LUMA_CONTEXT_MODELS                4       // number of context models for coefficients greater than two for Luma

#define NUMBER_OF_GREATER_TWO_COEFF_CHROMA_CONTEXT_MODELS              2       // number of context models for coefficients greater than two for Chroma

#define TOTAL_NUMBER_OF_GREATER_TWO_COEFF_CONTEXT_MODELS               6       // total number of context models for luma and chroma coefficients greater than two

#define NUMBER_OF_MVP_INDEX_CONTEXT_MODELS                             2       // number of context models for MVP index

#define NUMBER_OF_ALF_FLAG_CONTEXT_MODELS                              1       // number of context models for ALF flag

#define NUMBER_OF_ALF_UVLC_CONTEXT_MODELS                              2       // number of context models for ALF UVLC (filter length)

#define NUMBER_OF_ALF_SVLC_CONTEXT_MODELS                              3       // number of context models for ALF SVLC (filter coefficients)

#define NUMBER_OF_SAO_MERGE_FLAG_CONTEXT_MODELS                        1       // number of context models for SAO merge flags  

#define NUMBER_OF_SAO_TYPE_INDEX_CONTEXT_MODELS                        1       // number of context models for AO SVLC (filter coefficients)

#define MAX_SIZE_OF_CABAC_CONTEXT_MODELS                             256       // This should be a power of 2

#define TOTAL_NUMBER_OF_QP_VALUES                                     52       // qp range is 0 to 51

#define TOTAL_NUMBER_OF_SLICE_TYPES                                    3       // I, P and B

#define NUMBER_OF_CABAC_CONTEXT_MODELS1                 (NUMBER_OF_SPLIT_FLAG_CONTEXT_MODELS +  NUMBER_OF_SKIP_FLAG_CONTEXT_MODELS + NUMBER_OF_MERGE_FLAG_CONTEXT_MODELS  + NUMBER_OF_MERGE_INDEX_CONTEXT_MODELS +          \
                                                         NUMBER_OF_ALF_CTRL_FLAG_CONTEXT_MODELS + NUMBER_OF_PART_SIZE_CONTEXT_MODELS + NUMBER_OF_MVP_INDEX_CONTEXT_MODELS + NUMBER_OF_CU_AMP_CONTEXT_MODELS )

#define NUMBER_OF_CABAC_CONTEXT_MODELS2                 (NUMBER_OF_PRED_MODE_CONTEXT_MODELS +  NUMBER_OF_INTRA_LUMA_CONTEXT_MODELS + NUMBER_OF_INTRA_CHROMA_CONTEXT_MODELS  + NUMBER_OF_INTER_DIR_CONTEXT_MODELS +          \
                                                         NUMBER_OF_MVD_CONTEXT_MODELS + NUMBER_OF_REF_PIC_CONTEXT_MODELS +  NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CONTEXT_MODELS + TOTAL_NUMBER_OF_CBF_CONTEXT_MODELS)

#define NUMBER_OF_CABAC_CONTEXT_MODELS3                 (NUMBER_OF_ROOT_CBF_CONTEXT_MODELS +  NUMBER_OF_DELTA_QP_CONTEXT_MODELS + TOTAL_NUMBER_OF_SIG_FLAG_CONTEXT_MODELS  + TOTAL_NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS +          \
                                                         TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CONTEXT_MODELS + TOTAL_NUMBER_OF_GREATER_TWO_COEFF_CONTEXT_MODELS +  TOTAL_NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS + NUMBER_OF_ALF_FLAG_CONTEXT_MODELS)

#define NUMBER_OF_CABAC_CONTEXT_MODELS4                 (NUMBER_OF_ALF_UVLC_CONTEXT_MODELS +  NUMBER_OF_ALF_SVLC_CONTEXT_MODELS +  \
                                                         NUMBER_OF_SAO_MERGE_FLAG_CONTEXT_MODELS + NUMBER_OF_SAO_TYPE_INDEX_CONTEXT_MODELS + TOTAL_NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS)

#define TOTAL_NUMBER_OF_CABAC_CONTEXT_MODELS            (NUMBER_OF_CABAC_CONTEXT_MODELS1 + \
                                                         NUMBER_OF_CABAC_CONTEXT_MODELS2 + \
                                                         NUMBER_OF_CABAC_CONTEXT_MODELS3 + \
                                                         NUMBER_OF_CABAC_CONTEXT_MODELS4)

#define NUMBER_OF_PAD_VALUES_IN_CONTEXT_MODELS          (MAX_SIZE_OF_CABAC_CONTEXT_MODELS - TOTAL_NUMBER_OF_CABAC_CONTEXT_MODELS)

#define TOTAL_NUMBER_OF_CABAC_CONTEXT_MODEL_BUFFERS     (TOTAL_NUMBER_OF_QP_VALUES * TOTAL_NUMBER_OF_SLICE_TYPES)

#define CNU                                            154      ///< dummy data used in initialization of unused context models 'Context model Not Used'

    /**************************************
    * The EB_ContextModel is used to define the CABAC context models.
    **************************************/
    typedef EB_U32 EB_ContextModel;

    /**************************************
    * Cabac Context Model Structure
    **************************************/
    typedef struct ContextModelEncContext_s {
        EB_ContextModel  splitFlagContextModel[NUMBER_OF_SPLIT_FLAG_CONTEXT_MODELS];
        EB_ContextModel  skipFlagContextModel[NUMBER_OF_SKIP_FLAG_CONTEXT_MODELS];
        EB_ContextModel  mergeFlagContextModel[NUMBER_OF_MERGE_FLAG_CONTEXT_MODELS];
        EB_ContextModel  mergeIndexContextModel[NUMBER_OF_MERGE_INDEX_CONTEXT_MODELS];
        EB_ContextModel  mvpIndexContextModel[NUMBER_OF_MVP_INDEX_CONTEXT_MODELS];
        EB_ContextModel  partSizeContextModel[NUMBER_OF_PART_SIZE_CONTEXT_MODELS];
        EB_ContextModel  predModeContextModel[NUMBER_OF_PRED_MODE_CONTEXT_MODELS];
        EB_ContextModel  intraLumaContextModel[NUMBER_OF_INTRA_LUMA_CONTEXT_MODELS];
        EB_ContextModel  intraChromaContextModel[NUMBER_OF_INTRA_CHROMA_CONTEXT_MODELS];
        EB_ContextModel  deltaQpContextModel[NUMBER_OF_DELTA_QP_CONTEXT_MODELS];
        EB_ContextModel  interDirContextModel[NUMBER_OF_INTER_DIR_CONTEXT_MODELS];
        EB_ContextModel  refPicContextModel[NUMBER_OF_REF_PIC_CONTEXT_MODELS];
        EB_ContextModel  mvdContextModel[NUMBER_OF_MVD_CONTEXT_MODELS];
        EB_ContextModel  cbfContextModel[TOTAL_NUMBER_OF_CBF_CONTEXT_MODELS];
        EB_ContextModel  rootCbfContextModel[NUMBER_OF_ROOT_CBF_CONTEXT_MODELS];
        EB_ContextModel  transSubDivFlagContextModel[NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CONTEXT_MODELS];
        EB_ContextModel  lastSigXContextModel[TOTAL_NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS];
        EB_ContextModel  lastSigYContextModel[TOTAL_NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS];
        EB_ContextModel  significanceFlagContextModel[TOTAL_NUMBER_OF_SIG_FLAG_CONTEXT_MODELS];
        EB_ContextModel  coeffGroupSigFlagContextModel[TOTAL_NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS];
        EB_ContextModel  greaterThanOneContextModel[TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CONTEXT_MODELS];
        EB_ContextModel  greaterThanTwoContextModel[TOTAL_NUMBER_OF_GREATER_TWO_COEFF_CONTEXT_MODELS];

        EB_ContextModel  alfCtrlFlagContextModel[NUMBER_OF_ALF_CTRL_FLAG_CONTEXT_MODELS];
        EB_ContextModel  alfFlagContextModel[NUMBER_OF_ALF_FLAG_CONTEXT_MODELS];
        EB_ContextModel  alfUvlcContextModel[NUMBER_OF_ALF_UVLC_CONTEXT_MODELS];
        EB_ContextModel  alfSvlcContextModel[NUMBER_OF_ALF_SVLC_CONTEXT_MODELS];

        EB_ContextModel  saoMergeFlagContextModel[NUMBER_OF_SAO_MERGE_FLAG_CONTEXT_MODELS];
        EB_ContextModel  saoTypeIndexContextModel[NUMBER_OF_SAO_TYPE_INDEX_CONTEXT_MODELS];

        // This is AMP related, rename later
        EB_ContextModel  cuAmpContextModel[NUMBER_OF_CU_AMP_CONTEXT_MODELS];

        // The padJunk buffer is present to make this structure a power of 2. This should keep the structure on cache lines and very fast to access
        EB_ContextModel  padJunkBuffer[NUMBER_OF_PAD_VALUES_IN_CONTEXT_MODELS];

    } ContextModelEncContext_t;


/**************************************
 * Cabac Context Model Structure
 **************************************/
typedef struct  EpContext_s {

    EB_ContextModel  splitFlagContextModel          [NUMBER_OF_SPLIT_FLAG_CONTEXT_MODELS];     //Updated only in EncPass
    EB_ContextModel  skipFlagContextModel           [NUMBER_OF_SKIP_FLAG_CONTEXT_MODELS];      //Updated in both EncPass + MD
    EB_ContextModel  mergeFlagContextModel          [NUMBER_OF_MERGE_FLAG_CONTEXT_MODELS];     //Updated in both EncPass + MD
    EB_ContextModel  mergeIndexContextModel         [NUMBER_OF_MERGE_INDEX_CONTEXT_MODELS];    //Updated in both EncPass + MD
   
} EpContextModel_t;

typedef struct  SyntaxContextModelEncContext_s {
    EB_ContextModel  splitFlagContextModel          [NUMBER_OF_SPLIT_FLAG_CONTEXT_MODELS];
    EB_ContextModel  skipFlagContextModel           [NUMBER_OF_SKIP_FLAG_CONTEXT_MODELS];
    EB_ContextModel  mergeFlagContextModel          [NUMBER_OF_MERGE_FLAG_CONTEXT_MODELS];
    EB_ContextModel  mergeIndexContextModel         [NUMBER_OF_MERGE_INDEX_CONTEXT_MODELS];
    EB_ContextModel  mvpIndexContextModel           [NUMBER_OF_MVP_INDEX_CONTEXT_MODELS];
    EB_ContextModel  partSizeContextModel           [NUMBER_OF_PART_SIZE_CONTEXT_MODELS];
    EB_ContextModel  predModeContextModel           [NUMBER_OF_PRED_MODE_CONTEXT_MODELS];
    EB_ContextModel  intraLumaContextModel          [NUMBER_OF_INTRA_LUMA_CONTEXT_MODELS];
    EB_ContextModel  intraChromaContextModel        [NUMBER_OF_INTRA_CHROMA_CONTEXT_MODELS];
    EB_ContextModel  deltaQpContextModel            [NUMBER_OF_DELTA_QP_CONTEXT_MODELS];
    EB_ContextModel  interDirContextModel           [NUMBER_OF_INTER_DIR_CONTEXT_MODELS];
    EB_ContextModel  refPicContextModel             [NUMBER_OF_REF_PIC_CONTEXT_MODELS];
    EB_ContextModel  mvdContextModel                [NUMBER_OF_MVD_CONTEXT_MODELS];
    EB_ContextModel  cbfContextModel                [TOTAL_NUMBER_OF_CBF_CONTEXT_MODELS];
    EB_ContextModel  rootCbfContextModel            [NUMBER_OF_ROOT_CBF_CONTEXT_MODELS];
    EB_ContextModel  transSubDivFlagContextModel    [NUMBER_OF_TRANSFORM_SUBDIV_FLAG_CONTEXT_MODELS];
    // This is AMP related, rename later
    //EB_ContextModel  cuAmpContextModel              [NUMBER_OF_CU_AMP_CONTEXT_MODELS];

    } SyntaxContextModelEncContext_t;

    typedef struct CoeffContextModelEncContext_s {

        EB_ContextModel  lastSigXContextModel[TOTAL_NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS];
        EB_ContextModel  lastSigYContextModel[TOTAL_NUMBER_OF_LAST_SIG_XY_CONTEXT_MODELS];
        EB_ContextModel  significanceFlagContextModel[TOTAL_NUMBER_OF_SIG_FLAG_CONTEXT_MODELS];
        EB_ContextModel  coeffGroupSigFlagContextModel[TOTAL_NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS];
        EB_ContextModel  greaterThanOneContextModel[TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CONTEXT_MODELS];
        EB_ContextModel  greaterThanTwoContextModel[TOTAL_NUMBER_OF_GREATER_TWO_COEFF_CONTEXT_MODELS];


    } CoeffCtxtMdl_t;

    typedef struct CabacCost_s
    {

        EB_U32 CabacBitsLast[60 * 2 + 28 * 2];
        EB_U8 CabacBitsSig[2 * TOTAL_NUMBER_OF_SIG_FLAG_CONTEXT_MODELS];
        EB_U8 CabacBitsG1[2 * TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CONTEXT_MODELS];
        EB_U8 CabacBitsG2[2 * TOTAL_NUMBER_OF_GREATER_TWO_COEFF_CONTEXT_MODELS];
        EB_U8 CabacBitsSigMl[2 * TOTAL_NUMBER_OF_COEFF_GROUP_SIG_FLAG_CONTEXT_MODELS];
        EB_U16 CabacBitsG1x[TOTAL_NUMBER_OF_GREATER_ONE_COEFF_CONTEXT_MODELS / 4 * 16];
        EB_U8 CabacBitsSigV[32][16]; // 512 bytes
    } CabacCost_t;

    /**************************************
    * Extern Function Declarations
    **************************************/
extern EB_ERRORTYPE EncodeCabacContextModelInit(
        ContextModelEncContext_t *cabacContextModelArray);


#ifdef __cplusplus
}
#endif
#endif //EbCabacContextModel_h