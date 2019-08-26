/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbInitialRateControlReorderQueue_h
#define EbInitialRateControlReorderQueue_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbRateControlTables.h"
#include "EbPictureControlSet.h"
#ifdef __cplusplus
extern "C" {
#endif
/************************************************
 * Initial Rate Control Reorder Queue Entry
 ************************************************/
typedef struct InitialRateControlReorderEntry_s {
    EB_U64                          pictureNumber;    
    EbObjectWrapper_t              *parentPcsWrapperPtr;
} InitialRateControlReorderEntry_t;   

extern EB_ERRORTYPE InitialRateControlReorderEntryCtor(   
    InitialRateControlReorderEntry_t   **entryDblPtr,
    EB_U32                               pictureNumber);


/************************************************
 * High Level Rate Control Histogram Queue Entry
 ************************************************/
typedef struct HlRateControlHistogramEntry_s {
    EB_U64                          pictureNumber;    
    EB_S16                          lifeCount;    
    EB_BOOL                         passedToHlrc;
    EB_BOOL                         isCoded;
    EB_U64                          totalNumBitsCoded;
    EbObjectWrapper_t              *parentPcsWrapperPtr;
    EB_BOOL                         endOfSequenceFlag;  
    EB_U64                          predBitsRefQp[MAX_REF_QP_NUM];
    EB_PICTURE                        sliceType;                                                   
    EB_U32                          temporalLayerIndex;     

    
    // Motion Estimation Distortion and OIS Historgram 
    EB_U16                         *meDistortionHistogram;
    EB_U16                         *oisDistortionHistogram;
    EB_U32                          fullLcuCount;
} HlRateControlHistogramEntry_t;   

extern EB_ERRORTYPE HlRateControlHistogramEntryCtor(   
    HlRateControlHistogramEntry_t   **entryDblPtr,
    EB_U32                            pictureNumber);

#ifdef __cplusplus
}
#endif
#endif //EbInitialRateControlReorderQueue_h
