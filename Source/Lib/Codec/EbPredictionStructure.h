/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPredictionStructure_h
#define EbPredictionStructure_h

#include "EbApi.h"
#include "EbDefinitions.h"
#include "EbObject.h"
#ifdef __cplusplus
extern "C" {
#endif
/************************************************
 * Defines
 ************************************************/
#define FLAT_PREDICTION_STRUCTURE_PERIOD                                    1
#define TWO_LEVEL_HIERARCHICAL_PREDICTION_STRUCTURE_PERIOD                  2
#define THREE_LEVEL_HIERARCHICAL_PREDICTION_STRUCTURE_PERIOD                4
#define FOUR_LEVEL_HIERARCHICAL_PREDICTION_STRUCTURE_PERIOD                 8
#define MAX_PREDICTION_STRUCTURE_PERIOD                                     64

/************************************************
 * RPS defines
 ************************************************/
#define RPS_UNUSED                                                          ~0
#define MAX_NUM_OF_NEGATIVE_REF_PICS                                        5
#define MAX_NUM_OF_POSITIVE_REF_PICS                                        5
#define MAX_NUM_OF_REF_PICS_TOTAL                                           (MAX_NUM_OF_POSITIVE_REF_PICS + MAX_NUM_OF_NEGATIVE_REF_PICS)

/************************************************
 * Reference List
 *
 *   referenceList - Contains the deltaPOCs of 
 *    the pictures referenced by the current
 *    picture.
 ************************************************/
typedef struct ReferenceList_s
{
    EB_S32                              referenceList;    
    EB_U32                              referenceListCount;

} ReferenceList_t;

/************************************************
 * Dependent List
 *
 *   listCount - Contains count of how
 *     deep into list should be used
 *     depending on how many references are 
 *     being used in the prediction structure.
 *
 *   list - Contains the deltaPOCs of
 *     pictures that reference the current picture.
 *     The dependent list pictures must be grouped
 *     by the referenceCount group in ascending 
 *     order.  The grouping is not display order!
 ************************************************/
typedef struct DependentList_s
{
    EB_S32                             *list;
    EB_U32                              listCount;
    
} DependentList_t;

/************************************************
 * Prediction Structure Config Entry
 *   Contains the basic reference lists and 
 *   configurations for each Prediction Structure
 *   Config Entry.
 ************************************************/
typedef struct PredictionStructureConfigEntry_s {
    EB_U32                              temporalLayerIndex;
    EB_U32                              decodeOrder;
    EB_S32                              refList0;
    EB_S32                              refList1;
} PredictionStructureConfigEntry_t;

/************************************************
 * Prediction Structure Config
 *   Contains a collection of basic control data
 *   for the basic prediction structure.
 ************************************************/
typedef struct PredictionStructureConfig_s {
    EB_U32                              entryCount;
    PredictionStructureConfigEntry_t   *entryArray;
} PredictionStructureConfig_t;

/************************************************
 * Prediction Structure Entry 
 *   Contains the reference and dependent lists
 *   for a particular picture in the Prediction
 *   Structure.
 ************************************************/
typedef struct PredictionStructureEntry_s {
    ReferenceList_t                     refList0;
    ReferenceList_t                     refList1;
    DependentList_t                     depList0;
    DependentList_t                     depList1;
    EB_U32                              temporalLayerIndex;
    EB_U32                              decodeOrder;
    EB_BOOL                             isReferenced;

    // High-level RPS
    EB_BOOL                             shortTermRpsInSpsFlag;
    EB_U32                              shortTermRpsInSpsIndex;
    EB_BOOL                             interRpsPredictionFlag;
    EB_BOOL                             longTermRpsPresentFlag;
    EB_U32                              gopPositionLeastSignificantBits;

    // Predicted Short-Term RPS
    EB_U32                              deltaRpsIndexMinus1;
    EB_U32                              absoluteDeltaRpsMinus1;
    EB_U32                              deltaRpsSign;
    EB_BOOL                             usedByCurrPicFlag[MAX_NUM_OF_REF_PICS_TOTAL];
    EB_BOOL                             usedByFuturePicFlag[MAX_NUM_OF_REF_PICS_TOTAL];

    // Non-Predicted Short-Term RPS
    EB_U32                              negativeRefPicsTotalCount;
    EB_U32                              positiveRefPicsTotalCount;
    EB_U32                              deltaNegativeGopPosMinus1[MAX_NUM_OF_NEGATIVE_REF_PICS];
    EB_U32                              deltaPositiveGopPosMinus1[MAX_NUM_OF_POSITIVE_REF_PICS];
    EB_BOOL                             usedByNegativeCurrPicFlag[MAX_NUM_OF_NEGATIVE_REF_PICS];
    EB_BOOL                             usedByPositiveCurrPicFlag[MAX_NUM_OF_POSITIVE_REF_PICS];

    // Long-Term RPS
    EB_U32                              longTermRefPicsTotalCount;
    EB_U32                              deltaGopPoslsb[MAX_NUM_OF_REF_PICS_TOTAL];
    EB_BOOL                             deltaGopPosMsbPresentFlag[MAX_NUM_OF_REF_PICS_TOTAL];
    EB_U32                              deltaGopPosMsbMinus1[MAX_NUM_OF_REF_PICS_TOTAL];
    EB_BOOL                             usedByLtCurrPicFlagArray[MAX_NUM_OF_REF_PICS_TOTAL];

    // List Construction
    EB_BOOL                             refPicsOverrideTotalCountFlag;
    EB_S32                              refPicsList0TotalCountMinus1;
    EB_S32                              refPicsList1TotalCountMinus1;
    EB_BOOL                             listsModificationPresentFlag;
    EB_BOOL                             restrictedRefPicListsFlag;      // Same list enable flag (if set, 
                                                                        //   it implies all slices of the 
                                                                        //   same type in the same picture 
                                                                        //   have identical lists)
    
    // List Modification
    // *Note - This should probably be moved to the slice header since its a dynamic control - JMJ Jan 2, 2013
    EB_BOOL                             list0ModificationFlag;
    EB_BOOL                             list1ModificationFlag;
    EB_U32                              list0ModIndex[MAX_NUM_OF_REF_PICS_TOTAL];
    EB_U32                              list1ModIndex[MAX_NUM_OF_REF_PICS_TOTAL];

    // Lists Combination (STUB)
    
} PredictionStructureEntry_t;

/************************************************
 * Prediction Structure
 *   Contains a collection of control and RPS
 *   data types for an entire Prediction Structure
 ************************************************/
typedef struct PredictionStructure_s {
    EbDctor                             dctor;
    EB_U32                              predStructEntryCount;
    PredictionStructureEntry_t        **predStructEntryPtrArray;
    EB_PRED                             predType;
    EB_U32                              temporalLayerCount;
    EB_U32                              predStructPeriod;
    EB_U32                              maximumExtent;

    // Section Indices
    EB_U32                              leadingPicIndex;
    EB_U32                              initPicIndex;
    EB_U32                              steadyStateIndex;
    
    // RPS Related Entries
    EB_BOOL                             restrictedRefPicListsEnableFlag;
    EB_BOOL                             listsModificationEnableFlag;
    EB_BOOL                             longTermEnableFlag;
    EB_U32                              defaultRefPicsList0TotalCountMinus1;
    EB_U32                              defaultRefPicsList1TotalCountMinus1;

} PredictionStructure_t;

/************************************************
 * Prediction Structure Group
 *   Contains the control structures for all 
 *   supported prediction structures. 
 ************************************************/
typedef struct PredictionStructureGroup_s {
    EbDctor                             dctor;
    PredictionStructure_t             **predictionStructurePtrArray;
    EB_U32                              predictionStructureCount;
} PredictionStructureGroup_t;

/************************************************
 * Declarations
 ************************************************/
extern EB_ERRORTYPE PredictionStructureGroupCtor(
    PredictionStructureGroup_t   *predictionStructureGroupPtr,
    EB_U32                        baseLayerSwitchMode);

extern PredictionStructure_t* GetPredictionStructure(
    PredictionStructureGroup_t    *predictionStructureGroupPtr,
    EB_PRED                        predStructure,
    EB_U32                         numberOfReferences,
    EB_U32                         levelsOfHierarchy);    
#ifdef __cplusplus
}
#endif
#endif // EbPredictionStructure_h