/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureManagerQueue_h
#define EbPictureManagerQueue_h

#include "EbDefinitions.h"
#include "EbSei.h"
#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPredictionStructure.h"

#ifdef __cplusplus
extern "C" {
#endif
/************************************************
 * Input Queue Entry
 ************************************************/
struct ReferenceQueueEntry_s;   // empty struct definition
 
typedef struct InputQueueEntry_s {
    EbObjectWrapper_t              *inputObjectPtr;
    EB_U32                          dependentCount;
    EB_U32                          referenceEntryIndex; 
    ReferenceList_t                *list0Ptr;
    ReferenceList_t                *list1Ptr;    
    EB_U32                          useCount;
    EB_BOOL                         memoryMgmtLoopDone;
    EB_BOOL                         rateControlLoopDone;
    EB_BOOL                         encodingHasBegun;

} InputQueueEntry_t;   

/************************************************
 * Reference Queue Entry
 ************************************************/
typedef struct ReferenceQueueEntry_s {

    EB_U64                          pictureNumber;
    EB_U64                          decodeOrder;
    EbObjectWrapper_t              *referenceObjectPtr;
    EB_U32                          dependentCount;
    EB_BOOL                         releaseEnable;
    EB_BOOL                         referenceAvailable;
    EB_U32                          depList0Count;
    EB_U32                          depList1Count;
    DependentList_t                 list0;
    DependentList_t                 list1;
    EB_BOOL                         isUsedAsReferenceFlag;

    EB_U64                          rcGroupIndex;
    EB_BOOL                         feedbackArrived;
    
} ReferenceQueueEntry_t;   

/************************************************
 * Rate Control Input Queue Entry
 ************************************************/
 
typedef struct RcInputQueueEntry_s {
    EB_U64                          pictureNumber;
    EbObjectWrapper_t              *inputObjectPtr;
    
    EB_BOOL                         isPassed;
    EB_BOOL                         releaseEnabled;
    EB_U64                          groupId;
    EB_U64                          gopFirstPoc;
    EB_U32                          gopIndex;
 
    
} RcInputQueueEntry_t;   

/************************************************
 * Rate Control FeedBack  Queue Entry
 ************************************************/
typedef struct RcFeedbackQueueEntry_s {

    EB_U64                          pictureNumber;
    EbObjectWrapper_t              *feedbackObjectPtr;

    EB_BOOL                         isAvailable;
    EB_BOOL                         isUpdated;
    EB_BOOL                         releaseEnabled;
    EB_U64                          groupId;
    EB_U64                          gopFirstPoc;
    EB_U32                          gopIndex;
    
} RcFeedbackQueueEntry_t;   

extern EB_ERRORTYPE InputQueueEntryCtor(   
    InputQueueEntry_t      **entryDblPtr);

   

extern EB_ERRORTYPE ReferenceQueueEntryCtor(   
    ReferenceQueueEntry_t  **entryDblPtr);


#ifdef __cplusplus
}
#endif  
#endif // EbPictureManagerQueue_h