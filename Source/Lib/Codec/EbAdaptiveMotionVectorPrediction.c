/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <string.h>

#include "EbDefinitions.h"
#include "EbUtility.h"

#include "EbAdaptiveMotionVectorPrediction.h"
#include "EbAvailability.h"
#include "EbSequenceControlSet.h"
#include "EbReferenceObject.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"
#include "EbModeDecisionProcess.h"

static const EB_U32 mvMergeCandIndexArrayForFillingUp[2][12] = 
{
    {0 , 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3},
    {1 , 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2}
};

/** ScaleMV
        is used to scale the motion vector in AMVP process.
 */
static inline void ScaleMV(
    EB_U64    currentPicPOC,                // Iuput parameter, the POC of the current picture to be encoded.
    EB_U64    targetRefPicPOC,              // Iuput parameter, the POC of the reference picture where the inter coding is searching for.
    EB_U64    colPuPicPOC,                  // Iuput parameter, the POC of picture where the co-located PU is.
    EB_U64    colPuRefPicPOC,               // Iuput parameter, the POC of the reference picture where the MV of the co-located PU points to.
    EB_S16   *MVx,                          // Output parameter,
    EB_S16   *MVy)                          // Output parameter,
{
    EB_S16 td = (EB_S16)(colPuPicPOC - colPuRefPicPOC);
    EB_S16 tb = (EB_S16)(currentPicPOC - targetRefPicPOC);
    EB_S16 scaleFactor;
    EB_S16 temp;

    if(td != tb) {
        tb = CLIP3(-128, 127, tb);
        td = CLIP3(-128, 127, td);
        temp = (EB_S16) ((0x4000 + ABS(td >> 1)) / td);
        scaleFactor = CLIP3( -4096, 4095, (tb * temp + 32) >> 6 );

        *MVx = CLIP3(-32768, 32767, (scaleFactor * (*MVx) + 127 + (scaleFactor * (*MVx) < 0)) >> 8);
        *MVy = CLIP3(-32768, 32767, (scaleFactor * (*MVy) + 127 + (scaleFactor * (*MVy) < 0)) >> 8);
    }

    return;
}

EB_ERRORTYPE ClipMV(
    EB_U32                   cuOriginX,
    EB_U32                   cuOriginY,
    EB_S16                  *MVx,
    EB_S16                  *MVy,
    EB_U32                   pictureWidth,
    EB_U32                   pictureHeight,
    EB_U32                   tbSize)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    // horizontal clipping
    (*MVx) = CLIP3( ((EB_S16)((1 - cuOriginX - 8 - tbSize)<<2)), ((EB_S16)((pictureWidth + 8 - cuOriginX -1)<<2)), (*MVx));
    // vertical clipping
    (*MVy) = CLIP3( ((EB_S16)((1 - cuOriginY - 8 - tbSize)<<2)), ((EB_S16)((pictureHeight + 8 - cuOriginY -1)<<2)), (*MVy));

    return return_error;
}

/** GetNonScalingSpatialAMVP()
        is used to generate the spatial AMVP candidate by using the non-scaling method.

    It returns the availability of the spatial AMVP candidate.
 */
static inline EB_BOOL GetNonScalingSpatialAMVP(
    PredictionUnit_t  *neighbourPuPtr,           // input parameter, please refer to the detailed explanation above.
    EB_REFLIST            targetRefPicList,         // input parameter, the reference picture list where the AMVP is searching for.
    EB_U64                targetRefPicPOC,          // input parameter, the POC of the reference picture where the AMVP is searcing for.
    EB_S16               *MVPCandx,                 // output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16               *MVPCandy,                 // output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t  *pictureControlSetPtr)
{
    EB_BOOL                 availability = EB_FALSE;
    EbReferenceObject_t    *referenceObject;
    EB_U64                  refPicPOC;
    EB_REFLIST              refPicList;
	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    CHECK_REPORT_ERROR(
        (neighbourPuPtr != EB_NULL),
        encodeContextPtr->appCallbackPtr, 
        EB_ENC_AMVP_ERROR2);

    if (neighbourPuPtr != EB_NULL) {
    switch(neighbourPuPtr->interPredDirectionIndex) {
    case UNI_PRED_LIST_0:

        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
        refPicPOC       = referenceObject->refPOC;
        availability    = (EB_BOOL)(targetRefPicPOC == refPicPOC);
        *MVPCandx       = neighbourPuPtr->mv[REF_LIST_0].x;
        *MVPCandy       = neighbourPuPtr->mv[REF_LIST_0].y;

        break;

    case UNI_PRED_LIST_1:

        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;
        refPicPOC       = referenceObject->refPOC;
        availability    = (EB_BOOL)(targetRefPicPOC == refPicPOC);
        *MVPCandx       = neighbourPuPtr->mv[REF_LIST_1].x;
        *MVPCandy       = neighbourPuPtr->mv[REF_LIST_1].y;

        break;

    case BI_PRED:

        // Check the AMVP in targetRefPicList
        refPicList      = targetRefPicList;
        availability    = (EB_BOOL) ((neighbourPuPtr->interPredDirectionIndex == BI_PRED) || (neighbourPuPtr->interPredDirectionIndex == (EB_U8) refPicList));
        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[refPicList]->objectPtr;

        if (neighbourPuPtr) {

        refPicPOC       = referenceObject->refPOC;
        availability    = (EB_BOOL)(availability &  (targetRefPicPOC == refPicPOC));
        if(availability) {
            *MVPCandx = neighbourPuPtr->mv[refPicList].x;
            *MVPCandy = neighbourPuPtr->mv[refPicList].y;
        }
        else {
            // Check the AMVP in 1 - targetRefPicList
            refPicList      = (EB_REFLIST) (1 - targetRefPicList);
            availability    = (EB_BOOL) ((neighbourPuPtr->interPredDirectionIndex == BI_PRED) || (neighbourPuPtr->interPredDirectionIndex == (EB_U8) refPicList));
            referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[refPicList]->objectPtr;


            refPicPOC       = referenceObject->refPOC;
            availability    = (EB_BOOL)(availability &(targetRefPicPOC == refPicPOC));
            *MVPCandx       = neighbourPuPtr->mv[refPicList].x;
            *MVPCandy       = neighbourPuPtr->mv[refPicList].y;
        }
        }
        else
            CHECK_REPORT_ERROR_NC(
                encodeContextPtr->appCallbackPtr,
                EB_ENC_AMVP_ERROR2);
        break;

    default:
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_AMVP_ERROR3);
        }
    }
    else
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr,
            EB_ENC_AMVP_ERROR2);

    return availability;
}

/** GetNonScalingSpatialAMVP_V2() 
        is used to generate the spatial AMVP candidate by using the non-scaling method.
    
    It returns the availability of the spatial AMVP candidate.
 */
static inline EB_BOOL GetNonScalingSpatialAMVP_V2(
    MvUnit_t               *mvUnit,                 // input parameter, please refer to the detailed explanation above.
    EB_REFLIST            targetRefPicList,         // input parameter, the reference picture list where the AMVP is searching for.
    EB_U64                targetRefPicPOC,          // input parameter, the POC of the reference picture where the AMVP is searcing for.
    EB_S16               *MVPCandx,                 // output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16               *MVPCandy,                 // output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t  *pictureControlSetPtr)
{
    EB_BOOL                 availability = EB_FALSE;
    EbReferenceObject_t    *referenceObject;
    EB_U64                  refPicPOC;
    EB_REFLIST              refPicList;
    EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;


     
    switch(mvUnit->predDirection){
    case UNI_PRED_LIST_0:
              
        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
        refPicPOC       = referenceObject->refPOC;
        availability    = (EB_BOOL)(targetRefPicPOC == refPicPOC);
        if(availability){
            *MVPCandx       =  mvUnit->mv[REF_LIST_0].x; 
            *MVPCandy       =  mvUnit->mv[REF_LIST_0].y; 
        }
              
        break;

    case UNI_PRED_LIST_1:

        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;
        refPicPOC       = referenceObject->refPOC;
        availability    = (EB_BOOL)(targetRefPicPOC == refPicPOC); 
        if(availability){
            *MVPCandx       = mvUnit->mv[REF_LIST_1].x;  
            *MVPCandy       = mvUnit->mv[REF_LIST_1].y;  
        }

        break;

    case BI_PRED:

        // Check the AMVP in targetRefPicList  
        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[targetRefPicList]->objectPtr;   
        refPicPOC       = referenceObject->refPOC;
        availability    = (EB_BOOL)(targetRefPicPOC == refPicPOC);
        if(availability){         
             *MVPCandx  = mvUnit->mv[targetRefPicList].x;   
             *MVPCandy  = mvUnit->mv[targetRefPicList].y;   
        }else{
            // Check the AMVP in 1 - targetRefPicList
            refPicList      = (EB_REFLIST) (1 - targetRefPicList);   
            referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[refPicList]->objectPtr;  
            refPicPOC       = referenceObject->refPOC;
            availability    = (EB_BOOL)(targetRefPicPOC == refPicPOC); 
            *MVPCandx       =  mvUnit->mv[refPicList].x;
            *MVPCandy       =  mvUnit->mv[refPicList].y;
              
        }

        break;

    default:
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr,
            EB_ENC_AMVP_ERROR1);
    }

    return availability;
}

    /** GetScalingSpatialAMVP() 
        is used to generate the spatial AMVP candidate by using the scaling method.
 */
static inline EB_BOOL GetScalingSpatialAMVP_V2(
    MvUnit_t               *mvUnit,                     // input parameter, please refer to the detailed explanation above.
    EB_REFLIST            targetRefPicList,             // input parameter, the reference picture list where the AMVP is searching for.
    EB_U64                targetRefPicPOC,              // input parameter, the POC of the reference picture where the AMVP is searcing for.
    EB_S16               *MVPCandx,                     // output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16               *MVPCandy,                     // output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t  *pictureControlSetPtr)
{
    //EB_BOOL                 availability;
    EB_U64                  curPicPOC;
    EbReferenceObject_t    *referenceObject;
    EB_U64                  refPicPOC;
    EB_REFLIST              refPicList ;

   

    curPicPOC = pictureControlSetPtr->pictureNumber;
    
    
    if(mvUnit->predDirection == BI_PRED)
       refPicList       =  targetRefPicList;
    else
       refPicList      = (EB_REFLIST)mvUnit->predDirection;

    
    referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[refPicList]->objectPtr;
    refPicPOC       = referenceObject->refPOC;
   
    *MVPCandx       = mvUnit->mv[refPicList].x;  
    *MVPCandy       = mvUnit->mv[refPicList].y;  

    ScaleMV(
        curPicPOC,
        targetRefPicPOC,
        curPicPOC,
        refPicPOC,
        MVPCandx,
        MVPCandy);

    return EB_TRUE;
}

/** GetScalingSpatialAMVP()
        is used to generate the spatial AMVP candidate by using the scaling method.
 */
static inline EB_BOOL GetScalingSpatialAMVP(
    PredictionUnit_t  *neighbourPuPtr,               // input parameter, please refer to the detailed explanation above.
    EB_REFLIST            targetRefPicList,             // input parameter, the reference picture list where the AMVP is searching for.
    EB_U64                targetRefPicPOC,              // input parameter, the POC of the reference picture where the AMVP is searcing for.
    EB_S16               *MVPCandx,                     // output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16               *MVPCandy,                     // output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t  *pictureControlSetPtr)
{
    EB_BOOL                 availability;
    EB_U64                  curPicPOC;
    EbReferenceObject_t    *referenceObject;
    EB_U64                  refPicPOC;
    EB_REFLIST              refPicList = REF_LIST_0;

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    /*CHECK_REPORT_ERROR_NC(
        encodeContextPtr->appCallbackPtr, 
        EB_ENC_AMVP_ERROR7);*/

    curPicPOC = pictureControlSetPtr->pictureNumber;

    switch(neighbourPuPtr->interPredDirectionIndex) {
    case UNI_PRED_LIST_0:

        refPicList      = REF_LIST_0;
        break;

    case UNI_PRED_LIST_1:

        refPicList      = REF_LIST_1;
        break;

    case BI_PRED:

        availability    = (EB_BOOL) ((neighbourPuPtr->interPredDirectionIndex == BI_PRED) || (neighbourPuPtr->interPredDirectionIndex == (EB_U8) targetRefPicList));
        refPicList      = availability ? targetRefPicList : (EB_REFLIST)(1 - targetRefPicList);
        break;

    default:
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_AMVP_ERROR1);
    }

    referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[refPicList]->objectPtr;
    refPicPOC       = referenceObject->refPOC;
    *MVPCandx       = neighbourPuPtr->mv[refPicList].x;
    *MVPCandy       = neighbourPuPtr->mv[refPicList].y;
    ScaleMV(
        curPicPOC,
        targetRefPicPOC,
        curPicPOC,
        refPicPOC,
        MVPCandx,
        MVPCandy);

    return EB_TRUE;
}

/** GetSpatialMVPPosAx()
        is used to generate the spatial MVP candidate in position A0 or A1 (if needed).

    It returns the availability of the candidate.
 */
static EB_BOOL GetSpatialMVPPosAx(
    PredictionUnit_t  *puA0Ptr,                  // Input parameter, PU pointer of position A0.
    PredictionUnit_t  *puA1Ptr,                  // Input parameter, PU pointer of position A1.
    EB_BOOL               puA0Available,
    EB_BOOL               puA1Available,
    EB_U8                 puA0CodingMode,
    EB_U8                 puA1CodingMode,
    EB_REFLIST            targetRefPicList,         // Input parameter, the reference picture list where the AMVP is searching for.
    EB_U64                targetRefPicPOC,          // Input parameter, the POC of the reference picture where the AMVP is searcing for.
    EB_S16               *MVPCandx,                 // Output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16               *MVPCandy,                 // Output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t  *pictureControlSetPtr)
{
    EB_BOOL  puA0Availability  = (EB_BOOL) (puA0Available == EB_TRUE && puA0CodingMode != INTRA_MODE);
    EB_BOOL  puA1Availability  = (EB_BOOL) (puA1Available == EB_TRUE && puA1CodingMode != INTRA_MODE);
    EB_U8    puAvailability    = (EB_U8) puA0Availability + (((EB_U8) puA1Availability) << 1);
    EB_BOOL  MVPAxAvailability = EB_FALSE;

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    switch(puAvailability) {

    case 1:  // only A0 is available
        MVPAxAvailability = GetNonScalingSpatialAMVP(
                                puA0Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPAxAvailability) {
            MVPAxAvailability = GetScalingSpatialAMVP(
                                    puA0Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);
        }

        break;

    case 2:  // only A1 is available
        MVPAxAvailability = GetNonScalingSpatialAMVP(
                                puA1Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPAxAvailability) {
            MVPAxAvailability = GetScalingSpatialAMVP(
                                    puA1Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);
        }

        break;

    case 3:  // A0 & A1 are both available
        MVPAxAvailability = GetNonScalingSpatialAMVP(
                                puA0Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPAxAvailability) {
            MVPAxAvailability = GetNonScalingSpatialAMVP(
                                    puA1Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);

            if(!MVPAxAvailability) {
                MVPAxAvailability = GetScalingSpatialAMVP(
                                        puA0Ptr,
                                        targetRefPicList,
                                        targetRefPicPOC,
                                        MVPCandx,
                                        MVPCandy,
                                        pictureControlSetPtr);
            }
        }

        break;

    case 0:  // none of A0 and A1 is available
        break;

    default:
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_AMVP_ERROR4);
        break;
    }

    return MVPAxAvailability;
}
   
/** GetSpatialMVPPosAx_V3() 
       same as GetSpatialMVPPosAx       
   is used to generate the spatial MVP candidate in position A0 or A1 (if needed).    
   It returns the availability of the candidate. 
 */
static EB_BOOL GetSpatialMVPPosAx_V3(
    MvUnit_t               *mvUnitA0,                // Input parameter, PU pointer of position A0.            
    MvUnit_t               *mvUnitA1,                // Input parameter, PU pointer of position A1.              
    EB_U8                 puAvailability,   
    EB_REFLIST            targetRefPicList,         // Input parameter, the reference picture list where the AMVP is searching for.
    EB_U64                targetRefPicPOC,          // Input parameter, the POC of the reference picture where the AMVP is searcing for.
    EB_S16               *MVPCandx,                 // Output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16               *MVPCandy,                 // Output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t  *pictureControlSetPtr)
{
       
    EB_BOOL  MVPAxAvailability = EB_FALSE;
    EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    switch(puAvailability){

    case 1:  // only A0 is available
        MVPAxAvailability = GetNonScalingSpatialAMVP_V2(
                                 mvUnitA0,
                                 targetRefPicList,
                                 targetRefPicPOC,
                                 MVPCandx,
                                 MVPCandy,
                                 pictureControlSetPtr);

        if(!MVPAxAvailability){
            MVPAxAvailability = GetScalingSpatialAMVP_V2(
                                     mvUnitA0,
                                     targetRefPicList,
                                     targetRefPicPOC,
                                     MVPCandx,
                                     MVPCandy,
                                     pictureControlSetPtr);
        }

        break;

    case 2:  // only A1 is available
        MVPAxAvailability = GetNonScalingSpatialAMVP_V2(
                                  mvUnitA1,
                                  targetRefPicList,
                                  targetRefPicPOC,
                                  MVPCandx,
                                  MVPCandy,
                                  pictureControlSetPtr);

        if(!MVPAxAvailability){
            MVPAxAvailability = GetScalingSpatialAMVP_V2(
                                     mvUnitA1,
                                     targetRefPicList,
                                     targetRefPicPOC,
                                     MVPCandx,
                                     MVPCandy,
                                     pictureControlSetPtr);
        }

        break;

    case 3:  // A0 & A1 are both available
        MVPAxAvailability = GetNonScalingSpatialAMVP_V2(
                                 mvUnitA0,
                                 targetRefPicList,
                                 targetRefPicPOC,
                                 MVPCandx,
                                 MVPCandy,
                                 pictureControlSetPtr);

        if(!MVPAxAvailability){
            MVPAxAvailability = GetNonScalingSpatialAMVP_V2(
                                     mvUnitA1,
                                     targetRefPicList,
                                     targetRefPicPOC,
                                     MVPCandx,
                                     MVPCandy,
                                     pictureControlSetPtr);

            if(!MVPAxAvailability){
                MVPAxAvailability = GetScalingSpatialAMVP_V2(
                                         mvUnitA0,
                                         targetRefPicList,
                                         targetRefPicPOC,
                                         MVPCandx,
                                         MVPCandy,
                                         pictureControlSetPtr);
            }
        }

        break;

    case 0:  // none of A0 and A1 is available
        break;

    default:
        CHECK_REPORT_ERROR_NC(
	        encodeContextPtr->appCallbackPtr, 
	        EB_ENC_AMVP_ERROR4);
        break;
    }
        
    return MVPAxAvailability;
}

/** GetNonScalingSpatialMVPPosBx()
        is used to generate the spatial MVP candidate in position B0, B1 (if needed) or B2 (if needed)
        and no MV scaling is used.
 */
static EB_BOOL GetNonScalingSpatialMVPPosBx(
    PredictionUnit_t     *puB0Ptr,                  // Input parameter, PU pointer of position B0.
    PredictionUnit_t     *puB1Ptr,                  // Input parameter, PU pointer of position B1.
    PredictionUnit_t     *puB2Ptr,                  // Input parameter, PU pointer of position B2.
    EB_BOOL               puB0Available,
    EB_BOOL               puB1Available,
    EB_BOOL               puB2Available,
    EB_U8                 puB0CodingMode,
    EB_U8                 puB1CodingMode,
    EB_U8                 puB2CodingMode,
    EB_REFLIST            targetRefPicList,         // Input parameter, the reference picture list where the AMVP is searching for.
    EB_U64                targetRefPicPOC,          // Input parameter, the POC of the reference picture where the AMVP is searcing for.
    EB_S16               *MVPCandx,                 // Output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16               *MVPCandy,                 // Output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t  *pictureControlSetPtr)
{
    EB_BOOL  MVPBxAvailability = EB_FALSE;
    EB_BOOL  puB0Availability  = (EB_BOOL) (puB0Available == EB_TRUE && puB0CodingMode != INTRA_MODE);
    EB_BOOL  puB1Availability  = (EB_BOOL) (puB1Available == EB_TRUE && puB1CodingMode != INTRA_MODE);
    EB_BOOL  puB2Availability  = (EB_BOOL) (puB2Available == EB_TRUE && puB2CodingMode != INTRA_MODE);
    EB_U8    puAvailability    = (EB_U8) puB0Availability + (((EB_U8) puB1Availability) << 1) + (((EB_U8) puB2Availability) << 2);

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    switch(puAvailability) {

    case 1:  // only B0 is available
        MVPBxAvailability = GetNonScalingSpatialAMVP(
                                puB0Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        break;

    case 2:  // only B1 is available
        MVPBxAvailability = GetNonScalingSpatialAMVP(
                                puB1Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        break;

    case 3:  // only B0 & B1 are available
        MVPBxAvailability = GetNonScalingSpatialAMVP(
                                puB0Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPBxAvailability) {
            MVPBxAvailability = GetNonScalingSpatialAMVP(
                                    puB1Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);
        }

        break;

    case 4:  // only B2 is available
        MVPBxAvailability = GetNonScalingSpatialAMVP(
                                puB2Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        break;

    case 5:  // only B0 & B2 are available
        MVPBxAvailability = GetNonScalingSpatialAMVP(
                                puB0Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPBxAvailability) {
            MVPBxAvailability = GetNonScalingSpatialAMVP(
                                    puB2Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);
        }

        break;

    case 6:  // only B1 & B2 are available
        MVPBxAvailability = GetNonScalingSpatialAMVP(
                                puB1Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPBxAvailability) {
            MVPBxAvailability = GetNonScalingSpatialAMVP(
                                    puB2Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);
        }

        break;

    case 7:  // B0, B1 & B2 are all available
        MVPBxAvailability = GetNonScalingSpatialAMVP(
                                puB0Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPBxAvailability) {
            MVPBxAvailability = GetNonScalingSpatialAMVP(
                                    puB1Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);

            if(!MVPBxAvailability) {
                MVPBxAvailability = GetNonScalingSpatialAMVP(
                                        puB2Ptr,
                                        targetRefPicList,
                                        targetRefPicPOC,
                                        MVPCandx,
                                        MVPCandy,
                                        pictureControlSetPtr);

            }
        }

        break;

    case 0:  // none of B0, B1 or B2 is available
        break;

    default:
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_AMVP_ERROR5);
		break;
    }

    return MVPBxAvailability;
}


/** GetScalingSpatialMVPPosBx()
        is used to generate the spatial MVP candidate in position B0, B1 (if needed) or B2 (if needed)
        and the MV scaling is used.
 */
static EB_BOOL GetScalingSpatialMVPPosBx(
    PredictionUnit_t     *puB0Ptr,                  // Input parameter, PU pointer of position B0.
    PredictionUnit_t     *puB1Ptr,                  // Input parameter, PU pointer of position B1.
    PredictionUnit_t     *puB2Ptr,                  // Input parameter, PU pointer of position B2.
    EB_BOOL               puB0Available,
    EB_BOOL               puB1Available,
    EB_BOOL               puB2Available,
    EB_U8                 puB0CodingMode,
    EB_U8                 puB1CodingMode,
    EB_U8                 puB2CodingMode,
    EB_REFLIST            targetRefPicList,         // Input parameter, the reference picture list where the AMVP is searching for.
    EB_U64                targetRefPicPOC,          // Input parameter, the POC of the reference picture where the AMVP is searcing for.
    EB_S16               *MVPCandx,                 // Output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16               *MVPCandy,                 // Output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t  *pictureControlSetPtr)
{
    EB_BOOL  MVPBxAvailability = EB_FALSE;
    EB_BOOL  puB0Availability  = (EB_BOOL)(puB0Available == EB_TRUE && puB0CodingMode != INTRA_MODE);
    EB_BOOL  puB1Availability  = (EB_BOOL)(puB1Available == EB_TRUE && puB1CodingMode != INTRA_MODE);
    EB_BOOL  puB2Availability  = (EB_BOOL)(puB2Available == EB_TRUE && puB2CodingMode != INTRA_MODE);
    EB_U8    puAvailability    = (EB_U8)puB0Availability + (((EB_U8)puB1Availability)<<1) + (((EB_U8)puB2Availability<<2));

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    switch(puAvailability) {

    case 1:  // only B0 is available

        MVPBxAvailability = GetScalingSpatialAMVP(
                                puB0Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        break;

    case 2:  // only B1 is available

        MVPBxAvailability = GetScalingSpatialAMVP(
                                puB1Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        break;

    case 3:  // only B0 & B1 are available

        MVPBxAvailability = GetScalingSpatialAMVP(
                                puB0Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPBxAvailability) {
            MVPBxAvailability = GetScalingSpatialAMVP(
                                    puB1Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);
        }

        break;

    case 4:  // only B2 is available

        MVPBxAvailability = GetScalingSpatialAMVP(
                                puB2Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        break;

    case 5:  // only B0 & B2 are available

        MVPBxAvailability = GetScalingSpatialAMVP(
                                puB0Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPBxAvailability) {
            MVPBxAvailability = GetScalingSpatialAMVP(
                                    puB2Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);
        }

        break;

    case 6:  // only B1 & B2 are available

        MVPBxAvailability = GetScalingSpatialAMVP(
                                puB1Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPBxAvailability) {
            MVPBxAvailability = GetScalingSpatialAMVP(
                                    puB2Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);
        }

        break;

    case 7:  // B0, B1 & B2 are all available

        MVPBxAvailability = GetScalingSpatialAMVP(
                                puB0Ptr,
                                targetRefPicList,
                                targetRefPicPOC,
                                MVPCandx,
                                MVPCandy,
                                pictureControlSetPtr);

        if(!MVPBxAvailability) {
            MVPBxAvailability = GetScalingSpatialAMVP(
                                    puB1Ptr,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);

            if(!MVPBxAvailability) {
                MVPBxAvailability = GetScalingSpatialAMVP(
                                        puB2Ptr,
                                        targetRefPicList,
                                        targetRefPicPOC,
                                        MVPCandx,
                                        MVPCandy,
                                        pictureControlSetPtr);
            }
        }

        break;

    case 0:  // none of B0, B1 or B2 is available
        break;

    default:
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_AMVP_ERROR4);
        break;
    }

    return MVPBxAvailability;
}


/** GetNonScalingSpatialMVPPosBx_V3()
is used to generate the spatial MVP candidate in position B0, B1 (if needed) or B2 (if needed)
and no MV scaling is used.
*/
static EB_BOOL GetNonScalingSpatialMVPPosBx_V3(
	//PredictionUnit_t  *puB0Ptr, PredictionUnit_t  *puB1Ptr, PredictionUnit_t  *puB2Ptr,                        
	MvUnit_t          *mvUnitB0,                  // Input parameter, PU pointer of position B0.                 
	MvUnit_t          *mvUnitB1,                  // Input parameter, PU pointer of position B1.                   
	MvUnit_t          *mvUnitB2,                  // Input parameter, PU pointer of position B2.
	EB_U8             puAvailability,
	//EB_BOOL               puB0Available,
	//EB_BOOL               puB1Available,
	//EB_BOOL               puB2Available,
	//EB_MODETYPE           puB0CodingMode,
	//EB_MODETYPE           puB1CodingMode,
	//EB_MODETYPE           puB2CodingMode,
	EB_REFLIST            targetRefPicList,         // Input parameter, the reference picture list where the AMVP is searching for.
	EB_U64                targetRefPicPOC,          // Input parameter, the POC of the reference picture where the AMVP is searcing for.
	EB_S16               *MVPCandx,                 // Output parameter, the horizontal componenet of the output AMVP candidate.
	EB_S16               *MVPCandy,                 // Output parameter, the vertical componenet of the output AMVP candidate.
	PictureControlSet_t  *pictureControlSetPtr)
{

	//CHKN move these avail outside
	EB_BOOL  MVPBxAvailability = EB_FALSE;
	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;
	//EB_BOOL  puB0Availability  = (EB_BOOL) (puB0Available == EB_TRUE && puB0CodingMode != INTRA_MODE);
	//EB_BOOL  puB1Availability  = (EB_BOOL) (puB1Available == EB_TRUE && puB1CodingMode != INTRA_MODE);
	//EB_BOOL  puB2Availability  = (EB_BOOL) (puB2Available == EB_TRUE && puB2CodingMode != INTRA_MODE);
	//EB_U8    puAvailability    = (EB_U8) puB0Availability + (((EB_U8) puB1Availability) << 1) + (((EB_U8) puB2Availability) << 2);

	switch (puAvailability){

	case 1:  // only B0 is available
		MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
			mvUnitB0,
			targetRefPicList,
			targetRefPicPOC,
			MVPCandx,
			MVPCandy,
			pictureControlSetPtr);

		break;

	case 2:  // only B1 is available
		MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
			mvUnitB1,
			targetRefPicList,
			targetRefPicPOC,
			MVPCandx,
			MVPCandy,
			pictureControlSetPtr);

		break;

	case 3:  // only B0 & B1 are available
		MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
			mvUnitB0,
			targetRefPicList,
			targetRefPicPOC,
			MVPCandx,
			MVPCandy,
			pictureControlSetPtr);

		if (!MVPBxAvailability){
			MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
				mvUnitB1,
				targetRefPicList,
				targetRefPicPOC,
				MVPCandx,
				MVPCandy,
				pictureControlSetPtr);
		}

		break;

	case 4:  // only B2 is available
		MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
			mvUnitB2,
			targetRefPicList,
			targetRefPicPOC,
			MVPCandx,
			MVPCandy,
			pictureControlSetPtr);

		break;

	case 5:  // only B0 & B2 are available
		MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
			mvUnitB0,
			targetRefPicList,
			targetRefPicPOC,
			MVPCandx,
			MVPCandy,
			pictureControlSetPtr);

		if (!MVPBxAvailability){
			MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
				mvUnitB2,
				targetRefPicList,
				targetRefPicPOC,
				MVPCandx,
				MVPCandy,
				pictureControlSetPtr);
		}

		break;

	case 6:  // only B1 & B2 are available
		MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
			mvUnitB1,
			targetRefPicList,
			targetRefPicPOC,
			MVPCandx,
			MVPCandy,
			pictureControlSetPtr);

		if (!MVPBxAvailability){
			MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
				mvUnitB2,
				targetRefPicList,
				targetRefPicPOC,
				MVPCandx,
				MVPCandy,
				pictureControlSetPtr);
		}

		break;

	case 7:  // B0, B1 & B2 are all available
		MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
			mvUnitB0,
			targetRefPicList,
			targetRefPicPOC,
			MVPCandx,
			MVPCandy,
			pictureControlSetPtr);

		if (!MVPBxAvailability){
			MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
				mvUnitB1,
				targetRefPicList,
				targetRefPicPOC,
				MVPCandx,
				MVPCandy,
				pictureControlSetPtr);

			if (!MVPBxAvailability){
				MVPBxAvailability = GetNonScalingSpatialAMVP_V2(
					mvUnitB2,
					targetRefPicList,
					targetRefPicPOC,
					MVPCandx,
					MVPCandy,
					pictureControlSetPtr);

			}
		}

		break;

	case 0:  // none of B0, B1 or B2 is available
		break;

	default:
		CHECK_REPORT_ERROR_NC(
			encodeContextPtr->appCallbackPtr,
			EB_ENC_AMVP_ERROR5);
		break;
	}

	return MVPBxAvailability;
}
/** GetScalingSpatialMVPPosBx_V3()
        is used to generate the spatial MVP candidate in position B0, B1 (if needed) or B2 (if needed)
        and the MV scaling is used.
 */
static EB_BOOL GetScalingSpatialMVPPosBx_V3(    
    MvUnit_t          *mvUnitB0,                  // Input parameter, PU pointer of position B0.                 
    MvUnit_t          *mvUnitB1,                  // Input parameter, PU pointer of position B1.                   
    MvUnit_t          *mvUnitB2,                  // Input parameter, PU pointer of position B2.
    EB_U8              puAvailability,
    //EB_BOOL               puB0Available,
    //EB_BOOL               puB1Available,
    //EB_BOOL               puB2Available,
    //EB_MODETYPE           puB0CodingMode,
    //EB_MODETYPE           puB1CodingMode,
    //EB_MODETYPE           puB2CodingMode,
    EB_REFLIST            targetRefPicList,         // Input parameter, the reference picture list where the AMVP is searching for.
    EB_U64                targetRefPicPOC,          // Input parameter, the POC of the reference picture where the AMVP is searcing for.
    EB_S16               *MVPCandx,                 // Output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16               *MVPCandy,                 // Output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t  *pictureControlSetPtr)
{
    EB_BOOL  MVPBxAvailability = EB_FALSE;
    EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;
    //EB_BOOL  puB0Availability  = (EB_BOOL)(puB0Available == EB_TRUE && puB0CodingMode != INTRA_MODE);
    //EB_BOOL  puB1Availability  = (EB_BOOL)(puB1Available == EB_TRUE && puB1CodingMode != INTRA_MODE);
    //EB_BOOL  puB2Availability  = (EB_BOOL)(puB2Available == EB_TRUE && puB2CodingMode != INTRA_MODE);
    //EB_U8    puAvailability    = (EB_U8)puB0Availability + (((EB_U8)puB1Availability)<<1) + (((EB_U8)puB2Availability<<2));

    switch(puAvailability){

    case 1:  // only B0 is available
        
        MVPBxAvailability = GetScalingSpatialAMVP_V2(
                             mvUnitB0,
                             targetRefPicList,
                             targetRefPicPOC,
                             MVPCandx,
                             MVPCandy,
                             pictureControlSetPtr);

        break;

    case 2:  // only B1 is available

        MVPBxAvailability = GetScalingSpatialAMVP_V2(
                             mvUnitB1,
                             targetRefPicList,
                             targetRefPicPOC,
                             MVPCandx,
                             MVPCandy,
                             pictureControlSetPtr);

        break;

    case 3:  // only B0 & B1 are available

        MVPBxAvailability = GetScalingSpatialAMVP_V2(
                             mvUnitB0,
                             targetRefPicList,
                             targetRefPicPOC,
                             MVPCandx,
                             MVPCandy,
                             pictureControlSetPtr);

            if(!MVPBxAvailability){
                MVPBxAvailability = GetScalingSpatialAMVP_V2(
                                     mvUnitB1,
                                     targetRefPicList,
                                     targetRefPicPOC,
                                     MVPCandx,
                                     MVPCandy,
                                     pictureControlSetPtr);
            }

        break;

    case 4:  // only B2 is available

        MVPBxAvailability = GetScalingSpatialAMVP_V2(
                              mvUnitB2,
                              targetRefPicList,
                              targetRefPicPOC,
                              MVPCandx,
                              MVPCandy,
                              pictureControlSetPtr);

        break;

    case 5:  // only B0 & B2 are available
        
            MVPBxAvailability = GetScalingSpatialAMVP_V2(
                                 mvUnitB0,
                                 targetRefPicList,
                                 targetRefPicPOC,
                                 MVPCandx,
                                 MVPCandy,
                                 pictureControlSetPtr);

            if(!MVPBxAvailability){
                MVPBxAvailability = GetScalingSpatialAMVP_V2(
                                     mvUnitB2,
                                     targetRefPicList,
                                     targetRefPicPOC,
                                     MVPCandx,
                                     MVPCandy,
                                     pictureControlSetPtr);
            }

        break;

    case 6:  // only B1 & B2 are available

        MVPBxAvailability = GetScalingSpatialAMVP_V2(
                              mvUnitB1,
                              targetRefPicList,
                              targetRefPicPOC,
                              MVPCandx,
                              MVPCandy,
                              pictureControlSetPtr);

        if(!MVPBxAvailability){
            MVPBxAvailability = GetScalingSpatialAMVP_V2(
                                    mvUnitB2,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);
        }

        break;

    case 7:  // B0, B1 & B2 are all available

        MVPBxAvailability = GetScalingSpatialAMVP_V2(
                             mvUnitB0,
                             targetRefPicList,
                             targetRefPicPOC,
                             MVPCandx,
                             MVPCandy,
                             pictureControlSetPtr);

        if(!MVPBxAvailability){
            MVPBxAvailability = GetScalingSpatialAMVP_V2(
                                    mvUnitB1,
                                    targetRefPicList,
                                    targetRefPicPOC,
                                    MVPCandx,
                                    MVPCandy,
                                    pictureControlSetPtr);

            if(!MVPBxAvailability){
                MVPBxAvailability = GetScalingSpatialAMVP_V2(
                                        mvUnitB2,
                                        targetRefPicList,
                                        targetRefPicPOC,
                                        MVPCandx,
                                        MVPCandy,
                                        pictureControlSetPtr);
            }
        }

        break;

    case 0:  // none of B0, B1 or B2 is available
        break;

    default:
        CHECK_REPORT_ERROR_NC(
	        encodeContextPtr->appCallbackPtr, 
	        EB_ENC_AMVP_ERROR5);
        break;
    }

    return MVPBxAvailability;
}

/** GetTemporalMVP()
        is used to generate the temporal MVP candidate.
 */
EB_BOOL GetTemporalMVP(    
    EB_U32                 puPicWiseLocX,
    EB_U32                 puPicWiseLocY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EB_REFLIST             targetRefPicList,                 // Input parameter, the reference picture list where the TMVP is searching for.
    //EB_U32            targetRefPicIdx,                  // Input parameter, the index of the reference picture where the TMVP is searcing for.
    EB_U64                 targetRefPicPOC,                  // Input parameter, the POC of the reference picture where the TMVP is searcing for.
    TmvpUnit_t            *tmvpMapPtr,                       // Input parameter, the pointer to the TMVP map.
    EB_U64                 colocatedPuPOC,                   // Input parameter, the POC of the co-located PU.
    EB_REFLIST             preDefinedColocatedPuRefList,     // Input parameter, the reference picture list of the co-located PU, which is defined in the slice header.
    EB_BOOL                isLowDelay,                       // Input parameter, indicates if the slice where the input PU belongs to is in lowdelay mode.
    EB_U32                 tbSize,
    EB_S16                *MVPCandx,                         // Output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16                *MVPCandy,                         // Output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t   *pictureControlSetPtr)
{
    EB_U32       bottomRightPositionX;
    EB_U32       bottomRightBlockPositionX;
    EB_U32       bottomRightBlockPositionY;

    TmvpPos      tmvpPosition;

    EB_U32       tmvpMapLcuIndexOffset = 0;
    EB_U32       tmvpMapPuIndex = 0;
    EB_U32       lcuSizeLog2   = LOG2F(tbSize);//CHKN this known

    EB_U32       pictureWidth  = ((SequenceControlSet_t*) pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaWidth;
    EB_U32       pictureHeight = ((SequenceControlSet_t*) pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaHeight;

    EB_BOOL      temporalMVPAvailability = EB_FALSE;
    EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

   
    //CHKN compute this outside (remove isLowdelay/targetRefPicList/preDefinedColocatedPuRefList
    //CHKN bottom right avail outside, common between L0+L1, tmvpPosition is common between L0+L1

    EB_REFLIST   colocatedPuRefList = isLowDelay ? targetRefPicList : (EB_REFLIST) (1-preDefinedColocatedPuRefList);



   
    //CHKN this if/else is common between L0+L1
    if(
        (puPicWiseLocX + puWidth) >= pictureWidth ||                    // Right Picture Edge Boundary Check
        (puPicWiseLocY + puHeight) >= pictureHeight ||                  // Bottom Picture Edge Boundary Check
        ((puPicWiseLocY & (tbSize - 1)) + puHeight) >= tbSize         // Bottom LCU Edge Check
    )
    {
        // Center co-located
        tmvpPosition = TmvpColocatedCenter;
    }
    else {
        // Bottom-right co-located
        bottomRightPositionX      = (puPicWiseLocX & (tbSize-1)) + puWidth;
        tmvpMapLcuIndexOffset     = bottomRightPositionX >> lcuSizeLog2;
        bottomRightBlockPositionX = (bottomRightPositionX & (tbSize-1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
        bottomRightBlockPositionY = ((puPicWiseLocY & (tbSize-1)) + puHeight) >> LOG_MV_COMPRESS_UNIT_SIZE;    // won't rollover due to prior boundary checks

        CHECK_REPORT_ERROR(
	        ((tmvpMapLcuIndexOffset < 2)),
	        encodeContextPtr->appCallbackPtr, 
	        EB_ENC_AMVP_ERROR8);

        tmvpMapPuIndex = bottomRightBlockPositionY * (tbSize >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;

        // Determine whether the Bottom-Right is Available
        tmvpPosition = (tmvpMapPtr[tmvpMapLcuIndexOffset].availabilityFlag[tmvpMapPuIndex] == EB_TRUE) ? TmvpColocatedBottomRight : TmvpColocatedCenter;
    }



    if(tmvpPosition == TmvpColocatedBottomRight) {
        colocatedPuRefList = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                             colocatedPuRefList :
                             (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

        *MVPCandx  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList][tmvpMapPuIndex].x;
        *MVPCandy  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList][tmvpMapPuIndex].y;
        temporalMVPAvailability = EB_TRUE;

        ScaleMV(
            pictureControlSetPtr->pictureNumber,
            targetRefPicPOC,
            colocatedPuPOC,
            tmvpMapPtr[tmvpMapLcuIndexOffset].refPicPOC[colocatedPuRefList][tmvpMapPuIndex],
            MVPCandx,
            MVPCandy);
    }
    else { // (tmvpType == TmvpColocatedCenter)
        tmvpMapLcuIndexOffset     = 0;
        bottomRightBlockPositionX = ((puPicWiseLocX & (tbSize-1)) + (puWidth  >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
        bottomRightBlockPositionY = ((puPicWiseLocY & (tbSize-1)) + (puHeight >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
        tmvpMapPuIndex            = bottomRightBlockPositionY * (tbSize >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;

        temporalMVPAvailability = tmvpMapPtr->availabilityFlag[tmvpMapPuIndex];
        if(temporalMVPAvailability) {
            colocatedPuRefList = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                                 colocatedPuRefList :
                                 (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

            *MVPCandx  = tmvpMapPtr->mv[colocatedPuRefList][tmvpMapPuIndex].x;
            *MVPCandy  = tmvpMapPtr->mv[colocatedPuRefList][tmvpMapPuIndex].y;
            ScaleMV(
                pictureControlSetPtr->pictureNumber,
                targetRefPicPOC,
                colocatedPuPOC,
                tmvpMapPtr->refPicPOC[colocatedPuRefList][tmvpMapPuIndex],
                MVPCandx,
                MVPCandy);
        }
    }

    return temporalMVPAvailability;
}
 
 /** GetTemporalMVP_V2()
        is used to generate the temporal MVP candidate.
 */
EB_BOOL GetTemporalMVP_V2(   
    TmvpPos                tmvpPosition,
    EB_U32                 tmvpMapLcuIndexOffset,
    EB_U32                 tmvpMapPuIndex,
    EB_REFLIST             targetRefPicList,                 // Input parameter, the reference picture list where the TMVP is searching for.  
    EB_U64                 targetRefPicPOC,                  // Input parameter, the POC of the reference picture where the TMVP is searcing for.
    TmvpUnit_t            *tmvpMapPtr,                       // Input parameter, the pointer to the TMVP map.
    EB_U64                 colocatedPuPOC,                   // Input parameter, the POC of the co-located PU.
    EB_REFLIST             preDefinedColocatedPuRefList,     // Input parameter, the reference picture list of the co-located PU, which is defined in the slice header. 
    EB_S16                *MVPCandx,                         // Output parameter, the horizontal componenet of the output AMVP candidate.
    EB_S16                *MVPCandy,                         // Output parameter, the vertical componenet of the output AMVP candidate.
    PictureControlSet_t   *pictureControlSetPtr)
{
   
    EB_BOOL      temporalMVPAvailability = EB_FALSE;
    EB_REFLIST   colocatedPuRefList =  pictureControlSetPtr->isLowDelay ? targetRefPicList : (EB_REFLIST) (1-preDefinedColocatedPuRefList);

    
    if(tmvpPosition == TmvpColocatedBottomRight) {
        colocatedPuRefList = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                             colocatedPuRefList :
                             (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

        *MVPCandx  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList][tmvpMapPuIndex].x;
        *MVPCandy  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList][tmvpMapPuIndex].y;
        temporalMVPAvailability = EB_TRUE;

        ScaleMV(
            pictureControlSetPtr->pictureNumber,
            targetRefPicPOC,
            colocatedPuPOC,
            tmvpMapPtr[tmvpMapLcuIndexOffset].refPicPOC[colocatedPuRefList][tmvpMapPuIndex],
            MVPCandx,
            MVPCandy);
    }
    else { // (tmvpType == TmvpColocatedCenter)      

        temporalMVPAvailability = tmvpMapPtr->availabilityFlag[tmvpMapPuIndex];
        if(temporalMVPAvailability) {
            colocatedPuRefList = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                                 colocatedPuRefList :
                                 (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

            *MVPCandx  = tmvpMapPtr->mv[colocatedPuRefList][tmvpMapPuIndex].x;
            *MVPCandy  = tmvpMapPtr->mv[colocatedPuRefList][tmvpMapPuIndex].y;
            ScaleMV(
                pictureControlSetPtr->pictureNumber,
                targetRefPicPOC,
                colocatedPuPOC,
                tmvpMapPtr->refPicPOC[colocatedPuRefList][tmvpMapPuIndex],
                MVPCandx,
                MVPCandy);
        }
    }

    return temporalMVPAvailability;
}

EB_BOOL GetTemporalMVPBPicture(
    EB_U32                 puPicWiseLocX,
    EB_U32                 puPicWiseLocY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    PictureControlSet_t  *pictureControlSetPtr,
    TmvpUnit_t           *tmvpMapPtr,                       // Input parameter, the pointer to the TMVP map.
    EB_U64                colocatedPuPOC,                   // Input parameter, the POC of the co-located PU.
    EB_REFLIST            preDefinedColocatedPuRefList,     // Input parameter, the reference picture list of the co-located PU, which is defined in the slice header.
    EB_BOOL               isLowDelay,                       // Input parameter, indicates if the slice where the input PU belongs to is in lowdelay mode.
    EB_U32                tbSize,
    EB_S16               *MVPCand)                          // Output parameter, the pointer to the componenets of the output AMVP candidate.

{
    EB_U32       bottomRightPositionX;
    EB_U32       bottomRightBlockPositionX;
    EB_U32       bottomRightBlockPositionY;

    TmvpPos      tmvpPosition;

    EB_U32       tmvpMapLcuIndexOffset = 0;
    EB_U32       tmvpMapPuIndex = 0;
    EB_U32       lcuSizeLog2   = LOG2F(tbSize);

    EB_U32       pictureWidth  = ((SequenceControlSet_t*) pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaWidth;
    EB_U32       pictureHeight = ((SequenceControlSet_t*) pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaHeight;

    EB_BOOL      temporalMVPAvailability = EB_FALSE;
    EB_U64       targetRefPicPOC0 = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
    EB_U64       targetRefPicPOC1 = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;
    EB_REFLIST   colocatedPuRefList0 = isLowDelay ? REF_LIST_0 : (EB_REFLIST) (1-preDefinedColocatedPuRefList);
    EB_REFLIST   colocatedPuRefList1 = isLowDelay ? REF_LIST_1 : (EB_REFLIST) (1-preDefinedColocatedPuRefList);

    EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    if(
        (puPicWiseLocX + puWidth) >= pictureWidth ||                    // Right Picture Edge Boundary Check
        (puPicWiseLocY + puHeight) >= pictureHeight ||                  // Bottom Picture Edge Boundary Check
        ((puPicWiseLocY & (tbSize - 1)) + puHeight) >= tbSize         // Bottom LCU Edge Check
    )
    {
        // Center co-located
        tmvpPosition = TmvpColocatedCenter;
    }
    else {
        // Bottom-right co-located
        bottomRightPositionX      = (puPicWiseLocX & (tbSize-1)) + puWidth;
        tmvpMapLcuIndexOffset     = bottomRightPositionX >> lcuSizeLog2;
        bottomRightBlockPositionX = (bottomRightPositionX & (tbSize-1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
        bottomRightBlockPositionY = ((puPicWiseLocY & (tbSize-1)) + puHeight) >> LOG_MV_COMPRESS_UNIT_SIZE;    // won't rollover due to prior boundary checks

        CHECK_REPORT_ERROR(
            (tmvpMapLcuIndexOffset < 2),
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_AMVP_ERROR8);

        tmvpMapPuIndex = bottomRightBlockPositionY * (tbSize >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;

        // Determine whether the Bottom-Right is Available
        tmvpPosition = (tmvpMapPtr[tmvpMapLcuIndexOffset].availabilityFlag[tmvpMapPuIndex] == EB_TRUE) ? TmvpColocatedBottomRight : TmvpColocatedCenter;
    }

    if(tmvpPosition == TmvpColocatedBottomRight) {
        colocatedPuRefList0 = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                              colocatedPuRefList0 :
                              (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

        MVPCand[0]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList0][tmvpMapPuIndex].x;
        MVPCand[1]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList0][tmvpMapPuIndex].y;
        temporalMVPAvailability = EB_TRUE;


        ScaleMV(
            pictureControlSetPtr->pictureNumber,
            targetRefPicPOC0,
            colocatedPuPOC,
            tmvpMapPtr[tmvpMapLcuIndexOffset].refPicPOC[colocatedPuRefList0][tmvpMapPuIndex],
            &MVPCand[0],
            &MVPCand[1]);

        colocatedPuRefList1 = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                              colocatedPuRefList1 :
                              (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

        MVPCand[2]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList1][tmvpMapPuIndex].x;
        MVPCand[3]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList1][tmvpMapPuIndex].y;
        temporalMVPAvailability = EB_TRUE;

        ScaleMV(
            pictureControlSetPtr->pictureNumber,
            targetRefPicPOC1,
            colocatedPuPOC,
            tmvpMapPtr[tmvpMapLcuIndexOffset].refPicPOC[colocatedPuRefList1][tmvpMapPuIndex],
            &MVPCand[2],
            &MVPCand[3]/*MVPCandxL1,
            MVPCandyL1*/);
    }
    else { // (tmvpType == TmvpColocatedCenter)
        tmvpMapLcuIndexOffset     = 0;
        bottomRightBlockPositionX = ((puPicWiseLocX & (tbSize-1)) + (puWidth  >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
        bottomRightBlockPositionY = ((puPicWiseLocY & (tbSize-1)) + (puHeight >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
        tmvpMapPuIndex            = bottomRightBlockPositionY * (tbSize >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;

        temporalMVPAvailability = tmvpMapPtr->availabilityFlag[tmvpMapPuIndex];
        if(temporalMVPAvailability) {
            colocatedPuRefList0 = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                                  colocatedPuRefList0 :
                                  (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

            MVPCand[0]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList0][tmvpMapPuIndex].x;
            MVPCand[1]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList0][tmvpMapPuIndex].y;

            ScaleMV(
                pictureControlSetPtr->pictureNumber,
                targetRefPicPOC0,
                colocatedPuPOC,
                tmvpMapPtr->refPicPOC[colocatedPuRefList0][tmvpMapPuIndex],
                &MVPCand[0],
                &MVPCand[1]);

            colocatedPuRefList1 = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                                  colocatedPuRefList1 :
                                  (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

            MVPCand[2]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList1][tmvpMapPuIndex].x;
            MVPCand[3]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList1][tmvpMapPuIndex].y;

            ScaleMV(
                pictureControlSetPtr->pictureNumber,
                targetRefPicPOC1,
                colocatedPuPOC,
                tmvpMapPtr->refPicPOC[colocatedPuRefList1][tmvpMapPuIndex],
                &MVPCand[2],
                &MVPCand[3]);
        }
    }

    return temporalMVPAvailability;
}

EB_BOOL GetTemporalMVPBPicture_V2(
    TmvpPos                tmvpPosition,
    EB_U32                 tmvpMapLcuIndexOffset,
    EB_U32                 tmvpMapPuIndex,

    //EB_U32                 puPicWiseLocX,
    //EB_U32                 puPicWiseLocY,
    //EB_U32                 puWidth,
    //EB_U32                 puHeight,
    PictureControlSet_t  *pictureControlSetPtr,
    TmvpUnit_t           *tmvpMapPtr,                       // Input parameter, the pointer to the TMVP map.
    EB_U64                colocatedPuPOC,                   // Input parameter, the POC of the co-located PU.
    EB_REFLIST            preDefinedColocatedPuRefList,     // Input parameter, the reference picture list of the co-located PU, which is defined in the slice header.
    EB_BOOL               isLowDelay,                       // Input parameter, indicates if the slice where the input PU belongs to is in lowdelay mode.
//    EB_U32                tbSize,
    EB_S16               *MVPCand)                          // Output parameter, the pointer to the componenets of the output AMVP candidate.

{
    //EB_U32       bottomRightPositionX;
    //EB_U32       bottomRightBlockPositionX;
    //EB_U32       bottomRightBlockPositionY;

    //TmvpPos      tmvpPosition;

    //EB_U32       tmvpMapLcuIndexOffset = 0;
    //EB_U32       tmvpMapPuIndex = 0;
    //EB_U32       lcuSizeLog2   = LOG2F(tbSize);

    //EB_U32       pictureWidth  = ((SequenceControlSet_t*) pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaWidth;
   // EB_U32       pictureHeight = ((SequenceControlSet_t*) pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaHeight;

    EB_BOOL      temporalMVPAvailability = EB_FALSE;
    EB_U64       targetRefPicPOC0 = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
    EB_U64       targetRefPicPOC1 = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;
    EB_REFLIST   colocatedPuRefList0 = isLowDelay ? REF_LIST_0 : (EB_REFLIST) (1-preDefinedColocatedPuRefList);
    EB_REFLIST   colocatedPuRefList1 = isLowDelay ? REF_LIST_1 : (EB_REFLIST) (1-preDefinedColocatedPuRefList);

    //EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(puPtr->cuPtr->lcuPtr->pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

   /* if(
        (puPicWiseLocX + puWidth) >= pictureWidth ||                    // Right Picture Edge Boundary Check
        (puPicWiseLocY + puHeight) >= pictureHeight ||                  // Bottom Picture Edge Boundary Check
        ((puPicWiseLocY & (tbSize - 1)) + puHeight) >= tbSize         // Bottom LCU Edge Check
    )
    {
        // Center co-located
        tmvpPosition = TmvpColocatedCenter;
    }
    else {
        // Bottom-right co-located
        bottomRightPositionX      = (puPicWiseLocX & (tbSize-1)) + puWidth;
        tmvpMapLcuIndexOffset     = bottomRightPositionX >> lcuSizeLog2;
        bottomRightBlockPositionX = (bottomRightPositionX & (tbSize-1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
        bottomRightBlockPositionY = ((puPicWiseLocY & (tbSize-1)) + puHeight) >> LOG_MV_COMPRESS_UNIT_SIZE;    // won't rollover due to prior boundary checks

        tmvpMapPuIndex = bottomRightBlockPositionY * (tbSize >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;

        // Determine whether the Bottom-Right is Available
        tmvpPosition = (tmvpMapPtr[tmvpMapLcuIndexOffset].availabilityFlag[tmvpMapPuIndex] == EB_TRUE) ? TmvpColocatedBottomRight : TmvpColocatedCenter;
    }*/

    if(tmvpPosition == TmvpColocatedBottomRight) {
        colocatedPuRefList0 = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                              colocatedPuRefList0 :
                              (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

        MVPCand[0]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList0][tmvpMapPuIndex].x;
        MVPCand[1]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList0][tmvpMapPuIndex].y;
        temporalMVPAvailability = EB_TRUE;


        ScaleMV(
            pictureControlSetPtr->pictureNumber,
            targetRefPicPOC0,
            colocatedPuPOC,
            tmvpMapPtr[tmvpMapLcuIndexOffset].refPicPOC[colocatedPuRefList0][tmvpMapPuIndex],
            &MVPCand[0],
            &MVPCand[1]);

        colocatedPuRefList1 = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                              colocatedPuRefList1 :
                              (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

        MVPCand[2]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList1][tmvpMapPuIndex].x;
        MVPCand[3]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList1][tmvpMapPuIndex].y;
        temporalMVPAvailability = EB_TRUE;

        ScaleMV(
            pictureControlSetPtr->pictureNumber,
            targetRefPicPOC1,
            colocatedPuPOC,
            tmvpMapPtr[tmvpMapLcuIndexOffset].refPicPOC[colocatedPuRefList1][tmvpMapPuIndex],
            &MVPCand[2],
            &MVPCand[3]/*MVPCandxL1,
            MVPCandyL1*/);
    }
    else { // (tmvpType == TmvpColocatedCenter)
       /* tmvpMapLcuIndexOffset     = 0;
        bottomRightBlockPositionX = ((puPicWiseLocX & (tbSize-1)) + (puWidth  >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
        bottomRightBlockPositionY = ((puPicWiseLocY & (tbSize-1)) + (puHeight >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
        tmvpMapPuIndex            = bottomRightBlockPositionY * (tbSize >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;*/

        temporalMVPAvailability = tmvpMapPtr->availabilityFlag[tmvpMapPuIndex];
        if(temporalMVPAvailability) {
            colocatedPuRefList0 = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                                  colocatedPuRefList0 :
                                  (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

            MVPCand[0]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList0][tmvpMapPuIndex].x;
            MVPCand[1]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList0][tmvpMapPuIndex].y;

            ScaleMV(
                pictureControlSetPtr->pictureNumber,
                targetRefPicPOC0,
                colocatedPuPOC,
                tmvpMapPtr->refPicPOC[colocatedPuRefList0][tmvpMapPuIndex],
                &MVPCand[0],
                &MVPCand[1]);

            colocatedPuRefList1 = tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex] == BI_PRED ?
                                  colocatedPuRefList1 :
                                  (EB_REFLIST) tmvpMapPtr[tmvpMapLcuIndexOffset].predictionDirection[tmvpMapPuIndex];

            MVPCand[2]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList1][tmvpMapPuIndex].x;
            MVPCand[3]  = tmvpMapPtr[tmvpMapLcuIndexOffset].mv[colocatedPuRefList1][tmvpMapPuIndex].y;

            ScaleMV(
                pictureControlSetPtr->pictureNumber,
                targetRefPicPOC1,
                colocatedPuPOC,
                tmvpMapPtr->refPicPOC[colocatedPuRefList1][tmvpMapPuIndex],
                &MVPCand[2],
                &MVPCand[3]);
        }
    }

    return temporalMVPAvailability;
}

/**************************************************************************
 * FillAMVPCandidates()
 *       Generates the AMVP candidates for a PU.
 *
 *    B2 |                B1 | B0
 *     -----------------------
 *       |                   |
 *       |                   |
 *       |                   |
 *       |                   |
 *       |                   |
 *       |                   |
 *       |                   |
 *    A1 |                   |
 *     --|-------------------|
 *    A0
 *
 ***************************************************************************/
EB_ERRORTYPE FillAMVPCandidates(
    NeighborArrayUnit_t    *mvNeighborArray,
    NeighborArrayUnit_t    *modeNeighborArray,
    EB_U32                  originX,
    EB_U32                  originY,
    EB_U32                  width,
    EB_U32                  height,
    EB_U32                  cuSize,
    EB_U32                  cuDepth,
    EB_U32                  lcuSize,
    PictureControlSet_t    *pictureControlSetPtr,
    EB_BOOL                 tmvpEnableFlag,
    EB_U32                  tbAddr,
    EB_REFLIST              targetRefPicList,
	EB_S16                 *xMvAmvpArray,
	EB_S16                 *yMvAmvpArray,
	EB_U8                  *amvpCandidateCount)
{
    EB_ERRORTYPE            return_error = EB_ErrorNone;

    const EB_U32            cuSizeLog2 = Log2f(cuSize);
    const EB_U32            cuIndex = ((originY & (lcuSize - 1)) >> cuSizeLog2) * (1 << cuDepth) + ((originX & (lcuSize - 1)) >> cuSizeLog2);

    PredictionUnit_t     puA0;
    PredictionUnit_t     puA1;
    PredictionUnit_t     puB0;
    PredictionUnit_t     puB1;
    PredictionUnit_t     puB2;
    EB_U8                puA0CodingMode;
    EB_U8                puA1CodingMode;
    EB_U8                puB0CodingMode;
    EB_U8                puB1CodingMode;
    EB_U8                puB2CodingMode;

    // Neighbor Array Fixed-indicies
    const EB_U32 a0_ModeNaIndex = GetNeighborArrayUnitLeftIndex(
                modeNeighborArray,
                originY + height);
    const EB_U32 a1_ModeNaIndex = GetNeighborArrayUnitLeftIndex(
                modeNeighborArray,
                originY + height - 1);
    const EB_U32 b0_ModeNaIndex = GetNeighborArrayUnitTopIndex(
                modeNeighborArray,
                originX + width);
    const EB_U32 b1_ModeNaIndex = GetNeighborArrayUnitTopIndex(
                modeNeighborArray,
                originX + width - 1);
    const EB_U32 b2_ModeNaIndex = GetNeighborArrayUnitTopLeftIndex(
                modeNeighborArray,
                originX,
                originY);

    const EB_U32 a0_MvNaIndex = GetNeighborArrayUnitLeftIndex(
            mvNeighborArray,
            originY + height);
    const EB_U32 a1_MvNaIndex = GetNeighborArrayUnitLeftIndex(
            mvNeighborArray,
            originY + height - 1);
    const EB_U32 b0_MvNaIndex = GetNeighborArrayUnitTopIndex(
            mvNeighborArray,
            originX + width);
    const EB_U32 b1_MvNaIndex = GetNeighborArrayUnitTopIndex(
            mvNeighborArray,
            originX + width - 1);
    const EB_U32 b2_MvNaIndex = GetNeighborArrayUnitTopLeftIndex(
            mvNeighborArray,
            originX,
            originY);

    const LargestCodingUnit_t *lcuPtr = pictureControlSetPtr->lcuPtrArray[tbAddr];
    const EB_BOOL pictureLeftBoundary = (lcuPtr->lcuEdgeInfoPtr->tileLeftEdgeFlag == EB_TRUE && (originX & (lcuSize - 1)) == 0) ? EB_TRUE : EB_FALSE;
    const EB_BOOL pictureTopBoundary = (lcuPtr->lcuEdgeInfoPtr->tileTopEdgeFlag == EB_TRUE && (originY & (lcuSize - 1)) == 0) ? EB_TRUE : EB_FALSE;
    const EB_BOOL pictureRightBoundary =
        (lcuPtr->lcuEdgeInfoPtr->tileRightEdgeFlag == EB_TRUE && ((originX + width) & (lcuSize - 1)) == 0) ? EB_TRUE : EB_FALSE;

    EB_BOOL                 a0_availability;
    EB_BOOL                 a1_availability;
    EB_BOOL                 b0_availability;
    EB_BOOL                 b1_availability;
    EB_BOOL                 b2_availability;
    EB_BOOL                 tmvp_availability;

    EB_BOOL                 ax_availability;
    EB_BOOL                 bx_availability;
    EB_BOOL                 scale_availability;
    EB_BOOL                 AMVPScaleLimit;

    // TMVP
    EB_REFLIST              preDefinedColocatedPuRefList = pictureControlSetPtr->colocatedPuRefList;
    EB_BOOL                 isLowDelay = pictureControlSetPtr->isLowDelay;
    EbReferenceObject_t    *referenceObjectReadPtr;
    TmvpUnit_t             *tmvpMapPtr;
    EB_REFLIST              colocatedPuRefList;

    const EB_U64            targetRefPicPOC =((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[targetRefPicList]->objectPtr)->refPOC;
	EB_S16                  xMVPCandArray[4];
	EB_S16                  yMVPCandArray[4];
    EB_S16                  xMVPCand;
    EB_S16                  yMVPCand;
    EB_U32                  numAvailableMVPCand = 0;

    //----------------------------------------------
    // Availability Checks for A0, A1, B0, B1, B2
    //----------------------------------------------

    // *********** A0 ***********

    // CU scan-order availability check
    a0_availability = isBottomLeftAvailable(cuDepth, cuIndex);

    // Adjust availability for PU scan-order

    // Picture Boundary Check
    a0_availability = (a0_ModeNaIndex < modeNeighborArray->leftArraySize) ? a0_availability : EB_FALSE;

    // Slice, picture & Intra Check
    a0_availability = (modeNeighborArray->leftArray[a0_ModeNaIndex] != INTER_MODE) ? EB_FALSE : a0_availability;
	a0_availability = (pictureLeftBoundary == EB_TRUE) ? EB_FALSE : a0_availability;


    if(a0_availability) {
        puA0CodingMode                   = modeNeighborArray->leftArray[a0_ModeNaIndex];
        puA0.interPredDirectionIndex     = ((MvUnit_t*) mvNeighborArray->leftArray)[a0_MvNaIndex].predDirection;
        puA0.mv[REF_LIST_0].mvUnion = ((MvUnit_t*) mvNeighborArray->leftArray)[a0_MvNaIndex].mv[REF_LIST_0].mvUnion;
        puA0.mv[REF_LIST_1].mvUnion = ((MvUnit_t*) mvNeighborArray->leftArray)[a0_MvNaIndex].mv[REF_LIST_1].mvUnion;
    }
    else {
        puA0CodingMode = (EB_U8) INVALID_MODE;
        EB_MEMSET(&puA0, 0, sizeof(PredictionUnit_t));
    }

    // *********** A1 ***********

    // Slice, picture, & Intra Check
    a1_availability = (modeNeighborArray->leftArray[a1_ModeNaIndex] != INTER_MODE) ? EB_FALSE : EB_TRUE;
	a1_availability = (pictureLeftBoundary == EB_TRUE) ? EB_FALSE : a1_availability;

    // To check: Intra-CU check (imposed by the standard to facilitate concurrent PU searches) 
    // a1_availability = (partIndex == 1 && isVerticalPartition == EB_TRUE) ? EB_FALSE : a1_availability;

    if (a1_availability) {
        puA1CodingMode                   = modeNeighborArray->leftArray[a1_ModeNaIndex];
        puA1.interPredDirectionIndex     = ((MvUnit_t*) mvNeighborArray->leftArray)[a1_MvNaIndex].predDirection;
        puA1.mv[REF_LIST_0].mvUnion = ((MvUnit_t*) mvNeighborArray->leftArray)[a1_MvNaIndex].mv[REF_LIST_0].mvUnion;
        puA1.mv[REF_LIST_1].mvUnion = ((MvUnit_t*) mvNeighborArray->leftArray)[a1_MvNaIndex].mv[REF_LIST_1].mvUnion;
    }
    else {
        puA1CodingMode = (EB_U8) INVALID_MODE;
        EB_MEMSET(&puA1, 0, sizeof(PredictionUnit_t));
    }

    // *********** B2 ***********

    // Slice, picture, & Intra Check
    b2_availability = (modeNeighborArray->topLeftArray[b2_ModeNaIndex] != INTER_MODE) ? EB_FALSE : EB_TRUE;
	b2_availability = (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE) ? EB_FALSE : b2_availability;

    if (b2_availability) {
        puB2CodingMode                   = modeNeighborArray->topLeftArray[b2_ModeNaIndex];
        puB2.interPredDirectionIndex     = ((MvUnit_t*) mvNeighborArray->topLeftArray)[b2_MvNaIndex].predDirection;
        puB2.mv[REF_LIST_0].mvUnion = ((MvUnit_t*) mvNeighborArray->topLeftArray)[b2_MvNaIndex].mv[REF_LIST_0].mvUnion;
        puB2.mv[REF_LIST_1].mvUnion = ((MvUnit_t*) mvNeighborArray->topLeftArray)[b2_MvNaIndex].mv[REF_LIST_1].mvUnion;
    }
    else {
        puB2CodingMode = (EB_U8) INVALID_MODE;
        EB_MEMSET(&puB2, 0, sizeof(PredictionUnit_t));
    }

    // *********** B1 ***********

    // Slice, picture, & Intra Check
    b1_availability = (modeNeighborArray->topArray[b1_ModeNaIndex] != INTER_MODE) ? EB_FALSE : EB_TRUE;
    b1_availability = (pictureTopBoundary == EB_TRUE) ? EB_FALSE : b1_availability;

    // To check: Intra-CU check (imposed by the standard to facilitate concurrent PU searches)
    // b1_availability = (partIndex == 1 && isHorizontalPartition == EB_TRUE) ? EB_FALSE : b1_availability;

    if (b1_availability) {
        puB1CodingMode                   = modeNeighborArray->topArray[b1_ModeNaIndex];
        puB1.interPredDirectionIndex     = ((MvUnit_t*) mvNeighborArray->topArray)[b1_MvNaIndex].predDirection;
        puB1.mv[REF_LIST_0].mvUnion = ((MvUnit_t*) mvNeighborArray->topArray)[b1_MvNaIndex].mv[REF_LIST_0].mvUnion;
        puB1.mv[REF_LIST_1].mvUnion = ((MvUnit_t*) mvNeighborArray->topArray)[b1_MvNaIndex].mv[REF_LIST_1].mvUnion;
    }
    else {
        puB1CodingMode = (EB_U8) INVALID_MODE;
        EB_MEMSET(&puB1, 0, sizeof(PredictionUnit_t));
    }

    // *********** B0 ***********

    // CU scan-order availability check
    b0_availability = isUpperRightAvailable(cuDepth, cuIndex);
    // Picture Boundary Check
    b0_availability = (b0_ModeNaIndex < modeNeighborArray->topArraySize) ? b0_availability : EB_FALSE;

    // Slice, picture & Intra Check
    b0_availability = (modeNeighborArray->topArray[b0_ModeNaIndex] != INTER_MODE) ? EB_FALSE : b0_availability;
	b0_availability = (pictureTopBoundary == EB_TRUE) ? EB_FALSE : b0_availability;
    b0_availability = (pictureRightBoundary == EB_TRUE) ? EB_FALSE : b0_availability;

    if (b0_availability) {
        puB0CodingMode                   = modeNeighborArray->topArray[b0_ModeNaIndex];
        puB0.interPredDirectionIndex     = ((MvUnit_t*) mvNeighborArray->topArray)[b0_MvNaIndex].predDirection;
        puB0.mv[REF_LIST_0].mvUnion = ((MvUnit_t*) mvNeighborArray->topArray)[b0_MvNaIndex].mv[REF_LIST_0].mvUnion;
        puB0.mv[REF_LIST_1].mvUnion = ((MvUnit_t*) mvNeighborArray->topArray)[b0_MvNaIndex].mv[REF_LIST_1].mvUnion;
    }
    else {
        puB0CodingMode = (EB_U8) INVALID_MODE;
        EB_MEMSET(&puB0, 0, sizeof(PredictionUnit_t));
    }

    // Initialise TMVP map pointer
    colocatedPuRefList     = pictureControlSetPtr->sliceType == EB_B_PICTURE ? preDefinedColocatedPuRefList : REF_LIST_0;
    referenceObjectReadPtr = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[colocatedPuRefList]->objectPtr;
    if(referenceObjectReadPtr->tmvpEnableFlag == EB_FALSE)
    {
        referenceObjectReadPtr = (EbReferenceObject_t*) EB_NULL;
    }
    if(referenceObjectReadPtr != EB_NULL) {
        tmvpMapPtr = &(referenceObjectReadPtr->tmvpMap[tbAddr]);
    }
    else {
        tmvpMapPtr = NULL;
    }

    // ***AMVP***
    // Get spatial MVP candidate in position Ax and fill it into MVPCandArray
    ax_availability = GetSpatialMVPPosAx(
                          &puA0,
                          &puA1,
                          a0_availability,
                          a1_availability,
                          puA0CodingMode,
                          puA1CodingMode,
                          targetRefPicList,
                          targetRefPicPOC,
                          &xMVPCand,
                          &yMVPCand,
                          pictureControlSetPtr);

    if (ax_availability == EB_TRUE) {
        xMVPCandArray[numAvailableMVPCand] = xMVPCand;
        yMVPCandArray[numAvailableMVPCand] = yMVPCand;
        ++numAvailableMVPCand;
    }

    AMVPScaleLimit = ax_availability;

    // get non-scaling spatial MVP candidate in position Bx and then fill it into MVPCandArray
    bx_availability = GetNonScalingSpatialMVPPosBx(
                          &puB0,
                          &puB1,
                          &puB2,
                          b0_availability,
                          b1_availability,
                          b2_availability,
                          puB0CodingMode,
                          puB1CodingMode,
                          puB2CodingMode,
                          targetRefPicList,
                          targetRefPicPOC,
                          &xMVPCand,
                          &yMVPCand,
                          pictureControlSetPtr);

    if (bx_availability == EB_TRUE) {
        xMVPCandArray[numAvailableMVPCand] = xMVPCand;
        yMVPCandArray[numAvailableMVPCand] = yMVPCand;
        ++numAvailableMVPCand;
    }

    if (AMVPScaleLimit == EB_FALSE) {
        scale_availability = GetScalingSpatialMVPPosBx(
                                 &puB0,
                                 &puB1,
                                 &puB2,
                                 b0_availability,
                                 b1_availability,
                                 b2_availability,
                                 puB0CodingMode,
                                 puB1CodingMode,
                                 puB2CodingMode,
                                 targetRefPicList,
                                 targetRefPicPOC,
                                 &xMVPCand,
                                 &yMVPCand,
                                 pictureControlSetPtr);

        if (scale_availability == EB_TRUE) {
            xMVPCandArray[numAvailableMVPCand] = xMVPCand;
            yMVPCandArray[numAvailableMVPCand] = yMVPCand;
            ++numAvailableMVPCand;
        }
    }

    // Remove bX if its a duplicate of aX
    if(numAvailableMVPCand == 2 && ((xMVPCandArray[0] == xMVPCandArray[1]) && (yMVPCandArray[0] == yMVPCandArray[1]))) {
        numAvailableMVPCand = 1;
    }

    // TMVP
    if (tmvpEnableFlag && tmvpMapPtr != EB_NULL) {

        tmvp_availability = GetTemporalMVP(
                                originX,
                                originY,
                                width,
                                height,
                                targetRefPicList,
                                targetRefPicPOC,
                                tmvpMapPtr,
                                referenceObjectReadPtr->refPOC,
                                preDefinedColocatedPuRefList,
                                isLowDelay,
                                lcuSize,
                                &xMVPCand,
                                &yMVPCand,
                                pictureControlSetPtr);

        if (tmvp_availability == EB_TRUE) {
            xMVPCandArray[numAvailableMVPCand] = xMVPCand;
            yMVPCandArray[numAvailableMVPCand] = yMVPCand;
            ++numAvailableMVPCand;
        }
    }

    // if numAvailableMVPCand < 2, fill (0,0) MVP into MVPCandArray
    if((numAvailableMVPCand < 1) || (numAvailableMVPCand == 1 && xMVPCandArray[0] != 0 && yMVPCandArray[0] != 0)) {
        xMVPCandArray[numAvailableMVPCand] = 0;
        yMVPCandArray[numAvailableMVPCand] = 0;
        ++numAvailableMVPCand;
    }

    // output the AMVP candidates
    if(numAvailableMVPCand >= MAX_NUM_OF_AMVP_CANDIDATES) {
        *amvpCandidateCount = 2;
        xMvAmvpArray[0] = xMVPCandArray[0];
        yMvAmvpArray[0] = yMVPCandArray[0];
        xMvAmvpArray[1] = xMVPCandArray[1];
        yMvAmvpArray[1] = yMVPCandArray[1];
    }
    else {
        *amvpCandidateCount = 1;
        xMvAmvpArray[0] = xMVPCandArray[0];
        yMvAmvpArray[0] = yMVPCandArray[0];
    }

    return return_error;
}

/**************************************************************************
 * GenerateL0L1AmvpMergeLists()
 *       Generates the AMVP(L0+L1) + MV merge candidates for a PU.
 * 
 *    B2 |                B1 | B0  
 *     -----------------------
 *       |                   |                   
 *       |                   |                   
 *       |                   |                  
 *       |                   |                  
 *       |                   |                               
 *       |                   |                   
 *       |                   |                   
 *    A1 |                   |             
 *     --|-------------------|
 *    A0
 * 
 ***************************************************************************/
EB_ERRORTYPE GenerateL0L1AmvpMergeLists(
	ModeDecisionContext_t		*contextPtr,
	InterPredictionContext_t	*interPredictionPtr,
	PictureControlSet_t			*pictureControlSetPtr,
	EB_BOOL                      tmvpEnableFlag,
	EB_U32						 tbAddr,
	EB_S16						 xMvAmvpArray[2][2],
	EB_S16						 yMvAmvpArray[2][2],
	EB_U32						 amvpCandidateCount[2],
    EB_S16                           firstPuAMVPCandArray_x[MAX_NUM_OF_REF_PIC_LIST][2],
	EB_S16                           firstPuAMVPCandArray_y[MAX_NUM_OF_REF_PIC_LIST][2]
    ) {
    EB_ERRORTYPE            return_error = EB_ErrorNone;

    NeighborArrayUnit_t    *mvNeighborArray		= contextPtr->mvNeighborArray;
	NeighborArrayUnit_t    *modeNeighborArray	= contextPtr->modeTypeNeighborArray;
	MvMergeCandidate_t     *mergeCandidateArray = interPredictionPtr->mvMergeCandidateArray;

	const EB_U32            cuIndex	            = contextPtr->cuStats->cuNumInDepth;
	const EB_U32            originX	            = contextPtr->cuOriginX;
	const EB_U32            originY	            = contextPtr->cuOriginY;
    const EB_U32            width	            = contextPtr->cuStats->size;
	const EB_U32            height	            = contextPtr->cuStats->size;
	const EB_U32            cuDepth	            = contextPtr->cuStats->depth;
    EB_U32                 *mergeCandidateCount = &interPredictionPtr->mvMergeCandidateCount;
    EB_REFLIST              targetRefPicList          = REF_LIST_0;
	
    // Neighbor Array Fixed-indicies
    const EB_U32 a0_ModeNaIndex = GetNeighborArrayUnitLeftIndex(
        modeNeighborArray,
        originY + height);
    const EB_U32 a1_ModeNaIndex = GetNeighborArrayUnitLeftIndex(
        modeNeighborArray,
        originY + height - 1);
    const EB_U32 b0_ModeNaIndex = GetNeighborArrayUnitTopIndex(
        modeNeighborArray,
        originX + width);
    const EB_U32 b1_ModeNaIndex = GetNeighborArrayUnitTopIndex(
        modeNeighborArray,
        originX + width - 1);
    const EB_U32 b2_ModeNaIndex = GetNeighborArrayUnitTopLeftIndex(
        modeNeighborArray,
        originX,
        originY);

    const EB_U32 a0_MvNaIndex = GetNeighborArrayUnitLeftIndex(
        mvNeighborArray,
        originY + height);
    const EB_U32 a1_MvNaIndex = GetNeighborArrayUnitLeftIndex(
        mvNeighborArray,
        originY + height - 1);
    const EB_U32 b0_MvNaIndex = GetNeighborArrayUnitTopIndex(
        mvNeighborArray,
        originX + width);
    const EB_U32 b1_MvNaIndex = GetNeighborArrayUnitTopIndex(
        mvNeighborArray,
        originX + width - 1);
    const EB_U32 b2_MvNaIndex = GetNeighborArrayUnitTopLeftIndex(
        mvNeighborArray,
        originX,
        originY);
    const LargestCodingUnit_t *lcuPtr = pictureControlSetPtr->lcuPtrArray[tbAddr];

    const EB_BOOL pictureLeftBoundary = (lcuPtr->lcuEdgeInfoPtr->tileLeftEdgeFlag == EB_TRUE && (originX & (64 - 1)) == 0) ? EB_TRUE : EB_FALSE;
    const EB_BOOL pictureTopBoundary = (lcuPtr->lcuEdgeInfoPtr->tileTopEdgeFlag == EB_TRUE && (originY & (64 - 1)) == 0) ? EB_TRUE : EB_FALSE;
    const EB_BOOL pictureRightBoundary =
        (lcuPtr->lcuEdgeInfoPtr->tileRightEdgeFlag == EB_TRUE && ((originX + width) & (64 - 1)) == 0) ? EB_TRUE : EB_FALSE;

    EB_BOOL                 a0_availability;
    EB_BOOL                 a1_availability;
    EB_BOOL                 b0_availability;
    EB_BOOL                 b1_availability;
    EB_BOOL                 b2_availability;
    EB_BOOL                 tmvp_availability;

    EB_BOOL                 ax_availability;
    EB_BOOL                 bx_availability;
    EB_BOOL                 scale_availability;
    EB_BOOL                 AMVPScaleLimit;

    // TMVP
    EB_REFLIST              preDefinedColocatedPuRefList = pictureControlSetPtr->colocatedPuRefList;
    EB_BOOL                 isLowDelay = pictureControlSetPtr->isLowDelay;
    EbReferenceObject_t    *referenceObjectReadPtr;
    TmvpUnit_t             *tmvpMapPtr;
    EB_REFLIST              colocatedPuRefList;
   
    EB_U64                  targetRefPicPOC;


    EB_U32                  mvMergeCandidateIndex = 0;
    EB_S16                  MVPCandx;
    EB_S16                  MVPCandy;
	EB_S16                  MVPCand[4];

     // MV merge candidate array filling up
    EB_U32                  fillingIndex;
    EB_U32                  loopEnd;
    EB_U32                  list0FillingMvMergeCandidateIndex;
    EB_U32                  list1FillingMvMergeCandidateIndex;
    EB_S16                  list0FillingMvMergeCandidateMv_x;
    EB_S16                  list1FillingMvMergeCandidateMv_x;
    EB_S16                  list0FillingMvMergeCandidateMv_y;
    EB_S16                  list1FillingMvMergeCandidateMv_y;
    EB_U32                  refPicIndex;
    MvUnit_t               *mvUnitA0;
    MvUnit_t               *mvUnitA1;
    MvUnit_t               *mvUnitB2;
    MvUnit_t               *mvUnitB1;
    MvUnit_t               *mvUnitB0;

    EB_U8                  a0a1Availability,b0b1b2Availability;

    EB_S16                 *xMvAmvpArrayL0     = & (xMvAmvpArray[REF_LIST_0][0]);
    EB_S16                 *xMvAmvpArrayL1     = & (xMvAmvpArray[REF_LIST_1][0]);
    EB_S16                 *yMvAmvpArrayL0     = & (yMvAmvpArray[REF_LIST_0][0]);
    EB_S16                 *yMvAmvpArrayL1     = & (yMvAmvpArray[REF_LIST_1][0]);

    EB_U32                  numAvailableMVPCandL0 = 0;
    EB_U32                  numAvailableMVPCandL1 = 0;
   
    EB_U32                 totalMergeCandidates = *mergeCandidateCount;

    EB_BOOL      tmvpInfoReady = EB_FALSE;
    EB_U32       pictureWidth  ;
    EB_U32       pictureHeight ;
    EB_U32       bottomRightPositionX;
    EB_U32       bottomRightBlockPositionX;
    EB_U32       bottomRightBlockPositionY;
    TmvpPos      tmvpPosition = TmvpColocatedBottomRight;
    EB_U32       tmvpMapLcuIndexOffset = 0;
    EB_U32       tmvpMapPuIndex = 0;

    //----------------------------------------------
    // Availability Checks for A0, A1, B0, B1, B2
    //----------------------------------------------
    // *********** A0 ***********
    
    // CU scan-order availability check
    a0_availability = isBottomLeftAvailable(cuDepth, cuIndex);

    // Picture Boundary Check
    a0_availability = (a0_ModeNaIndex < modeNeighborArray->leftArraySize) ? a0_availability : EB_FALSE;

    // Slice & Intra Check 
    a0_availability = (((EB_MODETYPE*) modeNeighborArray->leftArray)[a0_ModeNaIndex] != INTER_MODE) ? EB_FALSE : a0_availability;

    // picture Check
	a0_availability = (pictureLeftBoundary == EB_TRUE) ? EB_FALSE : a0_availability;

    if(a0_availability){       
        mvUnitA0                         = &( (MvUnit_t*)mvNeighborArray->leftArray )[a0_MvNaIndex];
    }
    else{
        mvUnitA0 = (MvUnit_t*)EB_NULL;        
    }

    // *********** A1 ***********
    
    // Slice & Intra Check
    a1_availability = (((EB_MODETYPE*) modeNeighborArray->leftArray)[a1_ModeNaIndex] != INTER_MODE) ? EB_FALSE : EB_TRUE;

    // picture Check
	a1_availability = (pictureLeftBoundary == EB_TRUE) ? EB_FALSE : a1_availability;
    
    if (a1_availability) {      
        mvUnitA1                         = &( (MvUnit_t*)mvNeighborArray->leftArray )[a1_MvNaIndex];
    }
    else {
        mvUnitA1 = (MvUnit_t*)EB_NULL;   
    }

    // *********** B2 ***********
    
    // Slice & Intra Check
    b2_availability = (((EB_MODETYPE*) modeNeighborArray->topLeftArray)[b2_ModeNaIndex] != INTER_MODE) ? EB_FALSE : EB_TRUE;

    // picture Check
	b2_availability = (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE) ? EB_FALSE : b2_availability;
    
    if (b2_availability) {        
        mvUnitB2                         = &( (MvUnit_t*)mvNeighborArray->topLeftArray )[b2_MvNaIndex];
    }
    else {
        mvUnitB2 = (MvUnit_t*)EB_NULL;        
    }
    
    // *********** B1 ***********
    
    // Slice & Intra Check
    b1_availability = (((EB_MODETYPE*) modeNeighborArray->topArray)[b1_ModeNaIndex] != INTER_MODE) ? EB_FALSE : EB_TRUE;

    // picture Check
    b1_availability = (pictureTopBoundary == EB_TRUE) ? EB_FALSE : b1_availability;

    if (b1_availability) {       
        mvUnitB1                         = &( (MvUnit_t*)mvNeighborArray->topArray )[b1_MvNaIndex];
    }
    else {
        mvUnitB1 = (MvUnit_t*)EB_NULL;        
    }

    // *********** B0 ***********

    // CU scan-order availability check
    b0_availability = isUpperRightAvailable(cuDepth, cuIndex);

    // Picture Boundary Check
    b0_availability = (b0_ModeNaIndex < modeNeighborArray->topArraySize) ? b0_availability : EB_FALSE;

    // Slice & Intra Check 
    b0_availability = (((EB_MODETYPE*) modeNeighborArray->topArray)[b0_ModeNaIndex] != INTER_MODE) ? EB_FALSE : b0_availability;

    // picture Check
    b0_availability = (pictureTopBoundary == EB_TRUE) ? EB_FALSE : b0_availability;
    b0_availability = (pictureRightBoundary == EB_TRUE) ? EB_FALSE : b0_availability;

    if (b0_availability) {       
        mvUnitB0                         = &( (MvUnit_t*)mvNeighborArray->topArray )[b0_MvNaIndex];
    }
    else {
        mvUnitB0 = (MvUnit_t*)EB_NULL;       
    }

    //-----------------AMVP L0------------- 
    //-----------------AMVP L0-------------
    //-----------------AMVP L0-------------

    targetRefPicList = REF_LIST_0;
    //numAvailableMVPCand = 0;

    // Initialise TMVP map pointer
    colocatedPuRefList     = pictureControlSetPtr->sliceType == EB_B_PICTURE ? preDefinedColocatedPuRefList : REF_LIST_0;
    referenceObjectReadPtr = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[colocatedPuRefList]->objectPtr;
    if(referenceObjectReadPtr->tmvpEnableFlag == EB_FALSE)
    {
        referenceObjectReadPtr = (EbReferenceObject_t*) EB_NULL;
    }
    if(referenceObjectReadPtr != EB_NULL) {
        tmvpMapPtr = &(referenceObjectReadPtr->tmvpMap[tbAddr]);
    }
    else{
        tmvpMapPtr = NULL;
    }

    if (contextPtr->generateAmvpTableMd == EB_TRUE)
    {
        //get the TargetrefPoc here
        targetRefPicPOC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[targetRefPicList]->objectPtr)->refPOC;

        //compute A0A1 Availability for both L0+L1
        a0a1Availability = (EB_U8)a0_availability + (((EB_U8)a1_availability) << 1);

        //compute B0B1B2 Availability for both L0+L1
        b0b1b2Availability = (EB_U8)b0_availability + (((EB_U8)b1_availability) << 1) + (((EB_U8)b2_availability) << 2);

        // ***AMVP***
        // Get spatial MVP candidate in position Ax and fill it into MVPCandArray
        ax_availability = GetSpatialMVPPosAx_V3(
            mvUnitA0,
            mvUnitA1,
            a0a1Availability,
            targetRefPicList,
            targetRefPicPOC,
            &xMvAmvpArrayL0[numAvailableMVPCandL0],
            &yMvAmvpArrayL0[numAvailableMVPCandL0],
            pictureControlSetPtr);

        if (ax_availability == EB_TRUE) {
            ++numAvailableMVPCandL0;
        }


        //remove this
        AMVPScaleLimit = ax_availability;

        // get non-scaling spatial MVP candidate in position Bx and then fill it into MVPCandArray
        bx_availability = GetNonScalingSpatialMVPPosBx_V3(
            mvUnitB0,
            mvUnitB1,
            mvUnitB2,
            b0b1b2Availability,
            targetRefPicList,
            targetRefPicPOC,
            &xMvAmvpArrayL0[numAvailableMVPCandL0],
            &yMvAmvpArrayL0[numAvailableMVPCandL0],
            pictureControlSetPtr);

        if (bx_availability == EB_TRUE) {
            ++numAvailableMVPCandL0;
        }

        if (AMVPScaleLimit == EB_FALSE) {
            scale_availability = GetScalingSpatialMVPPosBx_V3(
                mvUnitB0,
                mvUnitB1,
                mvUnitB2,
                b0b1b2Availability,
                targetRefPicList,
                targetRefPicPOC,
                &xMvAmvpArrayL0[numAvailableMVPCandL0],
                &yMvAmvpArrayL0[numAvailableMVPCandL0],
                pictureControlSetPtr);

            if (scale_availability == EB_TRUE) {
                ++numAvailableMVPCandL0;
            }
        }

        // Remove duplicate if any
        if (numAvailableMVPCandL0 == 2 && ((xMvAmvpArrayL0[0] == xMvAmvpArrayL0[1]) && (yMvAmvpArrayL0[0] == yMvAmvpArrayL0[1]))) {
            numAvailableMVPCandL0 = 1;
        }

        // TMVP
        if (tmvpEnableFlag && tmvpMapPtr != EB_NULL  && numAvailableMVPCandL0 < 2)
        {
            pictureWidth = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaWidth;
            pictureHeight = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaHeight;
            tmvpMapLcuIndexOffset = 0;
            tmvpMapPuIndex = 0;

            if ((originX + width) >= pictureWidth ||                  // Right Picture Edge Boundary Check
                (originY + height) >= pictureHeight ||                  // Bottom Picture Edge Boundary Check
                ((originY & 63) + height) >= MAX_LCU_SIZE         // Bottom LCU Edge Check
                )
            {
                // Center co-located
                tmvpPosition = TmvpColocatedCenter;

            }
            else {
            // Bottom-right co-located
                bottomRightPositionX = (originX & (63)) + width;
                tmvpMapLcuIndexOffset = bottomRightPositionX >> 6;
                bottomRightBlockPositionX = (bottomRightPositionX & (63)) >> LOG_MV_COMPRESS_UNIT_SIZE;
                bottomRightBlockPositionY = ((originY & (63)) + height) >> LOG_MV_COMPRESS_UNIT_SIZE;    // won't rollover due to prior boundary checks

                tmvpMapPuIndex = bottomRightBlockPositionY * (MAX_LCU_SIZE >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;

                // Determine whether the Bottom-Right is Available
                tmvpPosition = (tmvpMapPtr[tmvpMapLcuIndexOffset].availabilityFlag[tmvpMapPuIndex] == EB_TRUE) ? TmvpColocatedBottomRight : TmvpColocatedCenter;
            }

            if (tmvpPosition == TmvpColocatedCenter)
            {
                tmvpMapLcuIndexOffset = 0;
                bottomRightBlockPositionX = ((originX & (63)) + (width >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
                bottomRightBlockPositionY = ((originY & (63)) + (height >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
                tmvpMapPuIndex = bottomRightBlockPositionY * (MAX_LCU_SIZE >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;
            }

            tmvpInfoReady = EB_TRUE;

            tmvp_availability = GetTemporalMVP_V2(
                tmvpPosition,
                tmvpMapLcuIndexOffset,
                tmvpMapPuIndex,
                targetRefPicList,
                targetRefPicPOC,
                tmvpMapPtr,
                referenceObjectReadPtr->refPOC,
                preDefinedColocatedPuRefList,
                &xMvAmvpArrayL0[numAvailableMVPCandL0],
                &yMvAmvpArrayL0[numAvailableMVPCandL0],
                pictureControlSetPtr);

            if (tmvp_availability == EB_TRUE) {
                ++numAvailableMVPCandL0;
            }
        }

        // fill (0,0) MVP if there is no candidates
        if ((numAvailableMVPCandL0 < 1) || (numAvailableMVPCandL0 == 1 && xMvAmvpArrayL0[0] != 0 && yMvAmvpArrayL0[0] != 0)) {
            xMvAmvpArrayL0[numAvailableMVPCandL0] = 0;
            yMvAmvpArrayL0[numAvailableMVPCandL0] = 0;
            ++numAvailableMVPCandL0;
        }

        amvpCandidateCount[REF_LIST_0] = numAvailableMVPCandL0;

        //------------------AMVP L1-------------- 
        //------------------AMVP L1-------------- 
        //------------------AMVP L1-------------- 
        if (pictureControlSetPtr->sliceType == EB_B_PICTURE)
        {
            targetRefPicList = REF_LIST_1;
            numAvailableMVPCandL1 = 0;

            //get the TargetrefPoc here
            targetRefPicPOC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[targetRefPicList]->objectPtr)->refPOC;

            // ***AMVP***
            // Get spatial MVP candidate in position Ax and fill it into MVPCandArray
            ax_availability = GetSpatialMVPPosAx_V3(
                mvUnitA0,
                mvUnitA1,
                a0a1Availability,
                targetRefPicList,
                targetRefPicPOC,
                &xMvAmvpArrayL1[numAvailableMVPCandL1],
                &yMvAmvpArrayL1[numAvailableMVPCandL1],
                pictureControlSetPtr);

            if (ax_availability == EB_TRUE) {
                ++numAvailableMVPCandL1;
            }

            AMVPScaleLimit = ax_availability;

            // get non-scaling spatial MVP candidate in position Bx and then fill it into MVPCandArray
            bx_availability = GetNonScalingSpatialMVPPosBx_V3(
                mvUnitB0,
                mvUnitB1,
                mvUnitB2,
                b0b1b2Availability,
                targetRefPicList,
                targetRefPicPOC,
                &xMvAmvpArrayL1[numAvailableMVPCandL1],
                &yMvAmvpArrayL1[numAvailableMVPCandL1],
                pictureControlSetPtr);

            if (bx_availability == EB_TRUE) {
                ++numAvailableMVPCandL1;
            }

            if (AMVPScaleLimit == EB_FALSE) {
                scale_availability = GetScalingSpatialMVPPosBx_V3(
                    mvUnitB0,
                    mvUnitB1,
                    mvUnitB2,
                    b0b1b2Availability,
                    targetRefPicList,
                    targetRefPicPOC,
                    &xMvAmvpArrayL1[numAvailableMVPCandL1],
                    &yMvAmvpArrayL1[numAvailableMVPCandL1],
                    pictureControlSetPtr);

                if (scale_availability == EB_TRUE) {
                    ++numAvailableMVPCandL1;
                }
            }

            // Remove duplicate 
            if (numAvailableMVPCandL1 == 2 && ((xMvAmvpArrayL1[0] == xMvAmvpArrayL1[1]) && (yMvAmvpArrayL1[0] == yMvAmvpArrayL1[1]))) {
                numAvailableMVPCandL1 = 1;
            }

            // TMVP
            if (tmvpEnableFlag && tmvpMapPtr != EB_NULL && numAvailableMVPCandL1 < 2) {


                if (tmvpInfoReady == EB_FALSE){

                    pictureWidth = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaWidth;
                    pictureHeight = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaHeight;
                    tmvpMapLcuIndexOffset = 0;
                    tmvpMapPuIndex = 0;

                    if ((originX + width) >= pictureWidth ||                  // Right Picture Edge Boundary Check
                        (originY + height) >= pictureHeight ||                  // Bottom Picture Edge Boundary Check
                        ((originY & (63)) + height) >= MAX_LCU_SIZE         // Bottom LCU Edge Check
                        )
                    {
                        // Center co-located
                        tmvpPosition = TmvpColocatedCenter;

                    }
                    else {
                        // Bottom-right co-located
                        bottomRightPositionX = (originX & (63)) + width;
                        tmvpMapLcuIndexOffset = bottomRightPositionX >> 6;
                        bottomRightBlockPositionX = (bottomRightPositionX & (63)) >> LOG_MV_COMPRESS_UNIT_SIZE;
                        bottomRightBlockPositionY = ((originY & (63)) + height) >> LOG_MV_COMPRESS_UNIT_SIZE;    // won't rollover due to prior boundary checks


                        tmvpMapPuIndex = bottomRightBlockPositionY * (MAX_LCU_SIZE >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;

                        // Determine whether the Bottom-Right is Available
                        tmvpPosition = (tmvpMapPtr[tmvpMapLcuIndexOffset].availabilityFlag[tmvpMapPuIndex] == EB_TRUE) ? TmvpColocatedBottomRight : TmvpColocatedCenter;
                    }

                    if (tmvpPosition == TmvpColocatedCenter) {

                        tmvpMapLcuIndexOffset = 0;
                        bottomRightBlockPositionX = ((originX & (63)) + (width >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
                        bottomRightBlockPositionY = ((originY & (63)) + (height >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
                        tmvpMapPuIndex = bottomRightBlockPositionY * (MAX_LCU_SIZE >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;

                    }


                }

                tmvp_availability = GetTemporalMVP_V2(
                    tmvpPosition,
                    tmvpMapLcuIndexOffset,
                    tmvpMapPuIndex,
                    targetRefPicList,
                    targetRefPicPOC,
                    tmvpMapPtr,
                    referenceObjectReadPtr->refPOC,
                    preDefinedColocatedPuRefList,
                    &xMvAmvpArrayL1[numAvailableMVPCandL1],
                    &yMvAmvpArrayL1[numAvailableMVPCandL1],
                    pictureControlSetPtr);



                if (tmvp_availability == EB_TRUE) {
                    ++numAvailableMVPCandL1;
                }
            }

            //fill (0,0) MVP in case there is no Candidates     
            if ((numAvailableMVPCandL1 < 1) || (numAvailableMVPCandL1 == 1 && xMvAmvpArrayL1[0] != 0 && yMvAmvpArrayL1[0] != 0)) {
                xMvAmvpArrayL1[numAvailableMVPCandL1] = 0;
                yMvAmvpArrayL1[numAvailableMVPCandL1] = 0;
                ++numAvailableMVPCandL1;
            }

            amvpCandidateCount[REF_LIST_1] = numAvailableMVPCandL1;


        }
    }

    //------------------Merge-------------- 
    //------------------Merge-------------- 
    //------------------Merge-------------- 
   

    //----------------------------------------
    // Candidate Selection
    //----------------------------------------

    switch(pictureControlSetPtr->sliceType) 
    {
    
    case EB_P_PICTURE:

        // A1
        if (mvUnitA1 != (MvUnit_t*)EB_NULL) {
        if (a1_availability == EB_TRUE)
        {
            mergeCandidateArray[mvMergeCandidateIndex].predictionDirection          = UNI_PRED_LIST_0;
            mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion       = mvUnitA1->mv[REF_LIST_0].mvUnion;
            ++mvMergeCandidateIndex;
        }
        }

        if(mvMergeCandidateIndex == totalMergeCandidates )
            break;

        // B1 
        if (mvUnitB1 != (MvUnit_t*)EB_NULL) {
            EB_BOOL mvUnion = EB_FALSE;
            if (mvUnitB1 != (MvUnit_t*)EB_NULL && mvUnitA1 != (MvUnit_t*)EB_NULL)
                mvUnion = (mvUnitB1->mv[REF_LIST_0].mvUnion != mvUnitA1->mv[REF_LIST_0].mvUnion);
            if(  (  b1_availability == EB_TRUE) &&
                 ( (a1_availability == EB_FALSE) || 
                   mvUnion))
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection          = UNI_PRED_LIST_0;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion       = mvUnitB1->mv[REF_LIST_0].mvUnion;
                ++mvMergeCandidateIndex;
            }
        }

        if(mvMergeCandidateIndex == totalMergeCandidates )
            break;
        
        // B0
        if (mvUnitB0 != (MvUnit_t*)EB_NULL) {
            EB_BOOL mvUnion = EB_FALSE;
            if (mvUnitB0 != (MvUnit_t*)EB_NULL && mvUnitB1 != (MvUnit_t*)EB_NULL)
                mvUnion = (mvUnitB0->mv[REF_LIST_0].mvUnion != mvUnitB1->mv[REF_LIST_0].mvUnion);
            if ( (b0_availability == EB_TRUE) &&
                 ((b1_availability == EB_FALSE) ||                
                 mvUnion))
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection           = UNI_PRED_LIST_0;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion        = mvUnitB0->mv[REF_LIST_0].mvUnion;
                ++mvMergeCandidateIndex;
            }
        }

        if(mvMergeCandidateIndex == totalMergeCandidates )
            break;

        // A0
        if (mvUnitA0 != (MvUnit_t*)EB_NULL) {
            EB_BOOL mvUnion = EB_FALSE;
            if (mvUnitA0 != (MvUnit_t*)EB_NULL && mvUnitA1 != (MvUnit_t*)EB_NULL)
                mvUnion = (mvUnitA0->mv[REF_LIST_0].mvUnion != mvUnitA1->mv[REF_LIST_0].mvUnion);
            if ((a0_availability == EB_TRUE) &&
               ((a1_availability == EB_FALSE) ||                
               mvUnion))
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection          = UNI_PRED_LIST_0;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion       = mvUnitA0->mv[REF_LIST_0].mvUnion;
                ++mvMergeCandidateIndex;
            }
        }

        if(mvMergeCandidateIndex == totalMergeCandidates )
            break;

        // B2
        if (mvUnitB2 != (MvUnit_t*)EB_NULL) {
            EB_BOOL mvUnion1 = EB_FALSE;
            EB_BOOL mvUnion2 = EB_FALSE;
            if (mvUnitB2 != (MvUnit_t*)EB_NULL && mvUnitA1 != (MvUnit_t*)EB_NULL)
                mvUnion1 = (mvUnitB2->mv[REF_LIST_0].mvUnion != mvUnitA1->mv[REF_LIST_0].mvUnion);
            if (mvUnitB2 != (MvUnit_t*)EB_NULL && mvUnitB1 != (MvUnit_t*)EB_NULL)
                mvUnion2 = (mvUnitB2->mv[REF_LIST_0].mvUnion != mvUnitB1->mv[REF_LIST_0].mvUnion);
        if (    mvMergeCandidateIndex < 4      &&
                (b2_availability == EB_TRUE)   &&
                ((a1_availability == EB_FALSE) || mvUnion1) &&
                ((b1_availability == EB_FALSE) || mvUnion2)     )
        {
            mergeCandidateArray[mvMergeCandidateIndex].predictionDirection          = UNI_PRED_LIST_0;
            mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion       = mvUnitB2->mv[REF_LIST_0].mvUnion;
            ++mvMergeCandidateIndex;
        }
        }
        
        if(mvMergeCandidateIndex == totalMergeCandidates )
            break;

        //Temporal candidate
        if (tmvpEnableFlag && tmvpMapPtr != EB_NULL){
            tmvp_availability = GetTemporalMVP(
                originX,
                originY,
                width,
                height,
                REF_LIST_0,
                ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC,
                tmvpMapPtr,
                referenceObjectReadPtr->refPOC,
                preDefinedColocatedPuRefList,
                isLowDelay,
                MAX_LCU_SIZE,    
                &MVPCandx,
                &MVPCandy,
                pictureControlSetPtr);

             if(tmvp_availability && mvMergeCandidateIndex < MAX_NUM_OF_MV_MERGE_CANDIDATE) {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection      = UNI_PRED_LIST_0;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].x         = MVPCandx;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].y         = MVPCandy;
                ++mvMergeCandidateIndex;
            }


        }

        if(mvMergeCandidateIndex == totalMergeCandidates )
            break;

        // Add zero MV
        for (refPicIndex = 0; refPicIndex < MAX_NUM_OF_MV_MERGE_CANDIDATE && mvMergeCandidateIndex < totalMergeCandidates; ++refPicIndex)
       // for (refPicIndex = 0; refPicIndex < MAX_NUM_OF_MV_MERGE_CANDIDATE && mvMergeCandidateIndex < MAX_NUM_OF_MV_MERGE_CANDIDATE; ++refPicIndex)
        {
            mergeCandidateArray[mvMergeCandidateIndex].predictionDirection      = UNI_PRED_LIST_0;
            mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].x         = 0;
            mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].y         = 0;
            ++mvMergeCandidateIndex;
        }


        break;

    case EB_B_PICTURE:

            // A1
            if (a1_availability == EB_TRUE)
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection    = (EB_U8) mvUnitA1->predDirection;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion = mvUnitA1->mv[REF_LIST_0].mvUnion;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].mvUnion = mvUnitA1->mv[REF_LIST_1].mvUnion;
                ++mvMergeCandidateIndex;
            }
     
            if(mvMergeCandidateIndex == totalMergeCandidates )
                break;
              
            
            // B1
            if (  (b1_availability == EB_TRUE) &&
                  ( (a1_availability == EB_FALSE) ||
                    (mvUnitB1->predDirection != mvUnitA1->predDirection) || 
                    (mvUnitB1->mv[REF_LIST_0].mvUnion != mvUnitA1->mv[REF_LIST_0].mvUnion) || 
                    (mvUnitB1->mv[REF_LIST_1].mvUnion != mvUnitA1->mv[REF_LIST_1].mvUnion))   )
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection     = mvUnitB1->predDirection;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion  = mvUnitB1->mv[REF_LIST_0].mvUnion;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].mvUnion  = mvUnitB1->mv[REF_LIST_1].mvUnion;
                ++mvMergeCandidateIndex;
            }

            if(mvMergeCandidateIndex == totalMergeCandidates )
                break;

            // B0
            if (    (b0_availability == EB_TRUE) &&
                    ((b1_availability == EB_FALSE) ||                
                     (mvUnitB0->predDirection      != mvUnitB1->predDirection)      ||
                     (mvUnitB0->mv[REF_LIST_0].mvUnion != mvUnitB1->mv[REF_LIST_0].mvUnion)  ||
                     (mvUnitB0->mv[REF_LIST_1].mvUnion != mvUnitB1->mv[REF_LIST_1].mvUnion)))
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection     = mvUnitB0->predDirection;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion  = mvUnitB0->mv[REF_LIST_0].mvUnion;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].mvUnion  = mvUnitB0->mv[REF_LIST_1].mvUnion;
                ++mvMergeCandidateIndex;
            }

            if(mvMergeCandidateIndex == totalMergeCandidates )
                break;

            // A0
            if (    (a0_availability == EB_TRUE) &&
                    ((a1_availability == EB_FALSE) ||               
                     (mvUnitA0->predDirection            != mvUnitA1->predDirection)          ||
                     (mvUnitA0->mv[REF_LIST_0].mvUnion  != mvUnitA1->mv[REF_LIST_0].mvUnion)  ||
                     (mvUnitA0->mv[REF_LIST_1].mvUnion  != mvUnitA1->mv[REF_LIST_1].mvUnion)))
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection    = mvUnitA0->predDirection;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion = mvUnitA0->mv[REF_LIST_0].mvUnion;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].mvUnion = mvUnitA0->mv[REF_LIST_1].mvUnion;
                ++mvMergeCandidateIndex;
            }

            if(mvMergeCandidateIndex == totalMergeCandidates )
                break;
            // B2
            if (   mvMergeCandidateIndex < 4    &&
                   (b2_availability == EB_TRUE) &&
                   ((a1_availability == EB_FALSE) ||              
                    (mvUnitB2->predDirection           != mvUnitA1->predDirection)           ||
                    (mvUnitB2->mv[REF_LIST_0].mvUnion  != mvUnitA1->mv[REF_LIST_0].mvUnion)  ||
                    (mvUnitB2->mv[REF_LIST_1].mvUnion  != mvUnitA1->mv[REF_LIST_1].mvUnion)) &&
                    ((b1_availability == EB_FALSE) ||               
                    (mvUnitB2->predDirection           != mvUnitB1->predDirection)           ||
                    (mvUnitB2->mv[REF_LIST_0].mvUnion  != mvUnitB1->mv[REF_LIST_0].mvUnion)  ||
                    (mvUnitB2->mv[REF_LIST_1].mvUnion  != mvUnitB1->mv[REF_LIST_1].mvUnion)))
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection    = mvUnitB2->predDirection;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion = mvUnitB2->mv[REF_LIST_0].mvUnion;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].mvUnion = mvUnitB2->mv[REF_LIST_1].mvUnion;
                ++mvMergeCandidateIndex;
            }

            if(mvMergeCandidateIndex == totalMergeCandidates )
                break;

            // Temporal candidate
            if (tmvpEnableFlag && tmvpMapPtr != EB_NULL) {

                if(tmvpInfoReady == EB_FALSE){

                    pictureWidth  = ((SequenceControlSet_t*) pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaWidth;
                    pictureHeight = ((SequenceControlSet_t*) pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaHeight;
                    tmvpMapLcuIndexOffset = 0;
                    tmvpMapPuIndex = 0;

                    if( (originX + width ) >= pictureWidth  ||              // Right Picture Edge Boundary Check
                        (originY + height) >= pictureHeight ||              // Bottom Picture Edge Boundary Check
                        ((originY & (63)) + height) >= MAX_LCU_SIZE     // Bottom LCU Edge Check
                        )
                    {
                        // Center co-located
                        tmvpPosition = TmvpColocatedCenter;
               
                    } else {
                        // Bottom-right co-located
                        bottomRightPositionX      = (originX & (63)) + width;
                        tmvpMapLcuIndexOffset     = bottomRightPositionX >> 6;
                        bottomRightBlockPositionX = (bottomRightPositionX & (63)) >> LOG_MV_COMPRESS_UNIT_SIZE;
                        bottomRightBlockPositionY = ((originY & (63)) + height) >> LOG_MV_COMPRESS_UNIT_SIZE;    // won't rollover due to prior boundary checks

                    
                        tmvpMapPuIndex = bottomRightBlockPositionY * (MAX_LCU_SIZE >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;

                        // Determine whether the Bottom-Right is Available
                        tmvpPosition = (tmvpMapPtr[tmvpMapLcuIndexOffset].availabilityFlag[tmvpMapPuIndex] == EB_TRUE) ? TmvpColocatedBottomRight : TmvpColocatedCenter;
                    }
                        
                    if(tmvpPosition == TmvpColocatedCenter) {
                        tmvpMapLcuIndexOffset     = 0;
                        bottomRightBlockPositionX = ((originX & (63)) + (width  >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
                        bottomRightBlockPositionY = ((originY & (63)) + (height >> 1)) >> LOG_MV_COMPRESS_UNIT_SIZE;
                        tmvpMapPuIndex            = bottomRightBlockPositionY * (MAX_LCU_SIZE >> LOG_MV_COMPRESS_UNIT_SIZE) + bottomRightBlockPositionX;


                    }
               }  

                tmvp_availability = GetTemporalMVPBPicture_V2(
                                        tmvpPosition,
                                        tmvpMapLcuIndexOffset,
                                        tmvpMapPuIndex,  
                                        pictureControlSetPtr,
                                        tmvpMapPtr,
                                        referenceObjectReadPtr->refPOC,
                                        pictureControlSetPtr->colocatedPuRefList,
                                        pictureControlSetPtr->isLowDelay,                                       
                                        MVPCand);
                                 

                if(tmvp_availability) {
                    mergeCandidateArray[mvMergeCandidateIndex].predictionDirection   = BI_PRED;
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].x      = MVPCand[0];
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].y      = MVPCand[1];
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].x      = MVPCand[2];
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].y      = MVPCand[3];

                    ++ mvMergeCandidateIndex;
                }
             }

             if(mvMergeCandidateIndex == totalMergeCandidates )
                break;

            // Bi-pred Combinations
            //  The MVs from the lists from the the {A0,A1,B0,B1,B2} are combined to create new merge candidates
            loopEnd = mvMergeCandidateIndex * (mvMergeCandidateIndex - 1);
            for(fillingIndex = 0; mvMergeCandidateIndex < totalMergeCandidates && fillingIndex < loopEnd; ++fillingIndex) {            
                list0FillingMvMergeCandidateIndex       = mvMergeCandIndexArrayForFillingUp[REF_LIST_0][fillingIndex];
                list1FillingMvMergeCandidateIndex       = mvMergeCandIndexArrayForFillingUp[REF_LIST_1][fillingIndex];

                list0FillingMvMergeCandidateMv_x        = mergeCandidateArray[list0FillingMvMergeCandidateIndex].mv[REF_LIST_0].x;
                list0FillingMvMergeCandidateMv_y        = mergeCandidateArray[list0FillingMvMergeCandidateIndex].mv[REF_LIST_0].y;
                list1FillingMvMergeCandidateMv_x        = mergeCandidateArray[list1FillingMvMergeCandidateIndex].mv[REF_LIST_1].x;
                list1FillingMvMergeCandidateMv_y        = mergeCandidateArray[list1FillingMvMergeCandidateIndex].mv[REF_LIST_1].y;

                if(((mergeCandidateArray[list0FillingMvMergeCandidateIndex].predictionDirection + 1) & 0x1) &&
                        ((mergeCandidateArray[list1FillingMvMergeCandidateIndex].predictionDirection + 1) & 0x2) &&
                        (((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC !=
                         ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC ||
                         (list0FillingMvMergeCandidateMv_x != list1FillingMvMergeCandidateMv_x) ||
                         (list0FillingMvMergeCandidateMv_y != list1FillingMvMergeCandidateMv_y)))
                {

                    mergeCandidateArray[mvMergeCandidateIndex].predictionDirection   = BI_PRED;
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].x      = list0FillingMvMergeCandidateMv_x;
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].y      = list0FillingMvMergeCandidateMv_y;
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].x      = list1FillingMvMergeCandidateMv_x;
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].y      = list1FillingMvMergeCandidateMv_y;
                    ++mvMergeCandidateIndex;
                }
            }

            if(mvMergeCandidateIndex == totalMergeCandidates )
                break;

            // Add zero MV
            for (refPicIndex = 0; refPicIndex < MAX_NUM_OF_MV_MERGE_CANDIDATE && mvMergeCandidateIndex < totalMergeCandidates; ++refPicIndex)           
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection   = BI_PRED;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].x      = 0;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].y      = 0;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].x      = 0;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].y      = 0;
                ++mvMergeCandidateIndex;
            }
     

        break;

    default:
        break;
    }

    // Update the number of MV merge candidate      
    *mergeCandidateCount = mvMergeCandidateIndex;
    //TODO: remove this tmp buffering
    firstPuAMVPCandArray_x[REF_LIST_0][0] = xMvAmvpArray[REF_LIST_0][0];
    firstPuAMVPCandArray_x[REF_LIST_0][1] = xMvAmvpArray[REF_LIST_0][1];
    firstPuAMVPCandArray_y[REF_LIST_0][0] = yMvAmvpArray[REF_LIST_0][0];
    firstPuAMVPCandArray_y[REF_LIST_0][1] = yMvAmvpArray[REF_LIST_0][1];

    firstPuAMVPCandArray_x[REF_LIST_1][0] = xMvAmvpArray[REF_LIST_1][0];
    firstPuAMVPCandArray_x[REF_LIST_1][1] = xMvAmvpArray[REF_LIST_1][1];
    firstPuAMVPCandArray_y[REF_LIST_1][0] = yMvAmvpArray[REF_LIST_1][0];
    firstPuAMVPCandArray_y[REF_LIST_1][1] = yMvAmvpArray[REF_LIST_1][1];

    return return_error;
}

EB_ERRORTYPE NonConformantGenerateL0L1AmvpMergeLists(
	ModeDecisionContext_t		*contextPtr,
	InterPredictionContext_t	*interPredictionPtr,
	PictureControlSet_t			*pictureControlSetPtr,
    EB_U32						 tbAddr) {

    EB_ERRORTYPE            return_error = EB_ErrorNone;
	NeighborArrayUnit_t    *mvNeighborArray		= contextPtr->mvNeighborArray;
	NeighborArrayUnit_t    *modeNeighborArray	= contextPtr->modeTypeNeighborArray;
	MvMergeCandidate_t     *mergeCandidateArray = interPredictionPtr->mvMergeCandidateArray;

	const EB_U32            originX	            = contextPtr->cuOriginX;
	const EB_U32            originY	            = contextPtr->cuOriginY;
    const EB_U32            width	            = contextPtr->cuStats->size;
	const EB_U32            height	            = contextPtr->cuStats->size;

    EB_U32                 *mergeCandidateCount = &interPredictionPtr->mvMergeCandidateCount;
	
    // Neighbor Array Fixed-indicies
    const EB_U32 a1_ModeNaIndex = GetNeighborArrayUnitLeftIndex(
        modeNeighborArray,
        originY + height - 1);
    const EB_U32 b1_ModeNaIndex = GetNeighborArrayUnitTopIndex(
        modeNeighborArray,
        originX + width - 1);
    const EB_U32 b2_ModeNaIndex = GetNeighborArrayUnitTopLeftIndex(
        modeNeighborArray,
        originX,
        originY);

    const EB_U32 a1_MvNaIndex = GetNeighborArrayUnitLeftIndex(
        mvNeighborArray,
        originY + height - 1);
    const EB_U32 b1_MvNaIndex = GetNeighborArrayUnitTopIndex(
        mvNeighborArray,
        originX + width - 1);
    const EB_U32 b2_MvNaIndex = GetNeighborArrayUnitTopLeftIndex(
        mvNeighborArray,
        originX,
        originY);
    const LargestCodingUnit_t *lcuPtr = pictureControlSetPtr->lcuPtrArray[tbAddr];
    const EB_BOOL pictureLeftBoundary = (lcuPtr->lcuEdgeInfoPtr->pictureLeftEdgeFlag == EB_TRUE && (originX & (63)) == 0) ? EB_TRUE : EB_FALSE;
    const EB_BOOL pictureTopBoundary  = (lcuPtr->lcuEdgeInfoPtr->pictureTopEdgeFlag  == EB_TRUE && (originY & (63)) == 0) ? EB_TRUE : EB_FALSE;

    EB_BOOL                 a1_availability;
    EB_BOOL                 b1_availability;
    EB_BOOL                 b2_availability;


    EB_U32                  mvMergeCandidateIndex = 0;

     // MV merge candidate array filling up
    EB_U32                  fillingIndex;
    EB_U32                  loopEnd;
    EB_U32                  list0FillingMvMergeCandidateIndex;
    EB_U32                  list1FillingMvMergeCandidateIndex;
    EB_S16                  list0FillingMvMergeCandidateMv_x;
    EB_S16                  list1FillingMvMergeCandidateMv_x;
    EB_S16                  list0FillingMvMergeCandidateMv_y;
    EB_S16                  list1FillingMvMergeCandidateMv_y;
    EB_U32                  refPicIndex;
    MvUnit_t               *mvUnitA1;
    MvUnit_t               *mvUnitB2;
    MvUnit_t               *mvUnitB1;
   
    EB_U32                 totalMergeCandidates = *mergeCandidateCount;

    //----------------------------------------------
    // Availability Checks for A1, B1, B2
    //----------------------------------------------

    // *********** A1 ***********
    
    // Slice & Intra Check
    a1_availability = (((EB_MODETYPE*) modeNeighborArray->leftArray)[a1_ModeNaIndex] != INTER_MODE) ? EB_FALSE : EB_TRUE;

    // picture Check
	a1_availability = (pictureLeftBoundary == EB_TRUE) ? EB_FALSE : a1_availability;
    
    if (a1_availability) {      
        mvUnitA1                         = &( (MvUnit_t*)mvNeighborArray->leftArray )[a1_MvNaIndex];
    }
    else {
        mvUnitA1 = (MvUnit_t*)EB_NULL;   
    }

    // *********** B2 ***********
    
    // Slice & Intra Check
    b2_availability = (((EB_MODETYPE*) modeNeighborArray->topLeftArray)[b2_ModeNaIndex] != INTER_MODE) ? EB_FALSE : EB_TRUE;

    // picture Check
	b2_availability = (pictureLeftBoundary == EB_TRUE || pictureTopBoundary == EB_TRUE) ? EB_FALSE : b2_availability;
    
    if (b2_availability) {        
        mvUnitB2                         = &( (MvUnit_t*)mvNeighborArray->topLeftArray )[b2_MvNaIndex];
    }
    else {
        mvUnitB2 = (MvUnit_t*)EB_NULL;        
    }
    
    // *********** B1 ***********
    
    // Slice & Intra Check
    b1_availability = (((EB_MODETYPE*) modeNeighborArray->topArray)[b1_ModeNaIndex] != INTER_MODE) ? EB_FALSE : EB_TRUE;

    // picture Check
	b1_availability = (pictureTopBoundary == EB_TRUE) ? EB_FALSE : b1_availability;

    if (b1_availability) {       
        mvUnitB1                         = &( (MvUnit_t*)mvNeighborArray->topArray )[b1_MvNaIndex];
    }
    else {
        mvUnitB1 = (MvUnit_t*)EB_NULL;        
    }

    //----------------------------------------
    // Candidate Selection
    //----------------------------------------

    switch(pictureControlSetPtr->sliceType) 
    {
    
    case EB_P_PICTURE:

        // A1
        if (a1_availability == EB_TRUE)
        {
            mergeCandidateArray[mvMergeCandidateIndex].predictionDirection          = UNI_PRED_LIST_0;
            mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion       = mvUnitA1->mv[REF_LIST_0].mvUnion;
            ++mvMergeCandidateIndex;
        }

        if(mvMergeCandidateIndex == totalMergeCandidates )
            break;

        // B1 
        if(  (  b1_availability == EB_TRUE) &&
             ( (a1_availability == EB_FALSE) || 
               (mvUnitB1->mv[REF_LIST_0].mvUnion != mvUnitA1->mv[REF_LIST_0].mvUnion)))
        {
            mergeCandidateArray[mvMergeCandidateIndex].predictionDirection          = UNI_PRED_LIST_0;
            mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion       = mvUnitB1->mv[REF_LIST_0].mvUnion;
            ++mvMergeCandidateIndex;
        }

        if(mvMergeCandidateIndex == totalMergeCandidates )
            break;

        // B2
        if (    mvMergeCandidateIndex < 4      &&
                (b2_availability == EB_TRUE)   &&
                ((a1_availability == EB_FALSE) || (mvUnitB2->mv[REF_LIST_0].mvUnion != mvUnitA1->mv[REF_LIST_0].mvUnion)) &&
                ((b1_availability == EB_FALSE) || (mvUnitB2->mv[REF_LIST_0].mvUnion != mvUnitB1->mv[REF_LIST_0].mvUnion))     )
        {
            mergeCandidateArray[mvMergeCandidateIndex].predictionDirection          = UNI_PRED_LIST_0;
            mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion       = mvUnitB2->mv[REF_LIST_0].mvUnion;
            ++mvMergeCandidateIndex;
        }
        
        if(mvMergeCandidateIndex == totalMergeCandidates )
            break;

        // Add zero MV
        for (refPicIndex = 0; refPicIndex < MAX_NUM_OF_MV_MERGE_CANDIDATE && mvMergeCandidateIndex < totalMergeCandidates; ++refPicIndex)
       // for (refPicIndex = 0; refPicIndex < MAX_NUM_OF_MV_MERGE_CANDIDATE && mvMergeCandidateIndex < MAX_NUM_OF_MV_MERGE_CANDIDATE; ++refPicIndex)
        {
            mergeCandidateArray[mvMergeCandidateIndex].predictionDirection      = UNI_PRED_LIST_0;
            mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].x         = 0;
            mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].y         = 0;
            ++mvMergeCandidateIndex;
        }


        break;

    case EB_B_PICTURE:

            // A1
            if (a1_availability == EB_TRUE)
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection    = (EB_U8) mvUnitA1->predDirection;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion = mvUnitA1->mv[REF_LIST_0].mvUnion;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].mvUnion = mvUnitA1->mv[REF_LIST_1].mvUnion;
                ++mvMergeCandidateIndex;
            }
     
            if(mvMergeCandidateIndex == totalMergeCandidates )
                break;
              
            
            // B1
            if (  (b1_availability == EB_TRUE) &&
                  ( (a1_availability == EB_FALSE) ||
                    (mvUnitB1->predDirection != mvUnitA1->predDirection) || 
                    (mvUnitB1->mv[REF_LIST_0].mvUnion != mvUnitA1->mv[REF_LIST_0].mvUnion) || 
                    (mvUnitB1->mv[REF_LIST_1].mvUnion != mvUnitA1->mv[REF_LIST_1].mvUnion))   )
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection     = mvUnitB1->predDirection;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion  = mvUnitB1->mv[REF_LIST_0].mvUnion;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].mvUnion  = mvUnitB1->mv[REF_LIST_1].mvUnion;
                ++mvMergeCandidateIndex;
            }

            if(mvMergeCandidateIndex == totalMergeCandidates )
                break;

            // B2
            if (   mvMergeCandidateIndex < 4    &&
                   (b2_availability == EB_TRUE) &&
                   ((a1_availability == EB_FALSE) ||              
                    (mvUnitB2->predDirection           != mvUnitA1->predDirection)           ||
                    (mvUnitB2->mv[REF_LIST_0].mvUnion  != mvUnitA1->mv[REF_LIST_0].mvUnion)  ||
                    (mvUnitB2->mv[REF_LIST_1].mvUnion  != mvUnitA1->mv[REF_LIST_1].mvUnion)) &&
                    ((b1_availability == EB_FALSE) ||               
                    (mvUnitB2->predDirection           != mvUnitB1->predDirection)           ||
                    (mvUnitB2->mv[REF_LIST_0].mvUnion  != mvUnitB1->mv[REF_LIST_0].mvUnion)  ||
                    (mvUnitB2->mv[REF_LIST_1].mvUnion  != mvUnitB1->mv[REF_LIST_1].mvUnion)))
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection    = mvUnitB2->predDirection;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].mvUnion = mvUnitB2->mv[REF_LIST_0].mvUnion;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].mvUnion = mvUnitB2->mv[REF_LIST_1].mvUnion;
                ++mvMergeCandidateIndex;
            }

            if(mvMergeCandidateIndex == totalMergeCandidates )
                break;

            // Bi-pred Combinations
            //  The MVs from the lists from the the {A1,B1,B2} are combined to create new merge candidates
            loopEnd = mvMergeCandidateIndex * (mvMergeCandidateIndex - 1);
            for(fillingIndex = 0; mvMergeCandidateIndex < totalMergeCandidates && fillingIndex < loopEnd; ++fillingIndex) {            
                list0FillingMvMergeCandidateIndex       = mvMergeCandIndexArrayForFillingUp[REF_LIST_0][fillingIndex];
                list1FillingMvMergeCandidateIndex       = mvMergeCandIndexArrayForFillingUp[REF_LIST_1][fillingIndex];

                list0FillingMvMergeCandidateMv_x        = mergeCandidateArray[list0FillingMvMergeCandidateIndex].mv[REF_LIST_0].x;
                list0FillingMvMergeCandidateMv_y        = mergeCandidateArray[list0FillingMvMergeCandidateIndex].mv[REF_LIST_0].y;
                list1FillingMvMergeCandidateMv_x        = mergeCandidateArray[list1FillingMvMergeCandidateIndex].mv[REF_LIST_1].x;
                list1FillingMvMergeCandidateMv_y        = mergeCandidateArray[list1FillingMvMergeCandidateIndex].mv[REF_LIST_1].y;

                if(((mergeCandidateArray[list0FillingMvMergeCandidateIndex].predictionDirection + 1) & 0x1) &&
                        ((mergeCandidateArray[list1FillingMvMergeCandidateIndex].predictionDirection + 1) & 0x2) &&
                        (((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC !=
                         ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC ||
                         (list0FillingMvMergeCandidateMv_x != list1FillingMvMergeCandidateMv_x) ||
                         (list0FillingMvMergeCandidateMv_y != list1FillingMvMergeCandidateMv_y)))
                {

                    mergeCandidateArray[mvMergeCandidateIndex].predictionDirection   = BI_PRED;
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].x      = list0FillingMvMergeCandidateMv_x;
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].y      = list0FillingMvMergeCandidateMv_y;
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].x      = list1FillingMvMergeCandidateMv_x;
                    mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].y      = list1FillingMvMergeCandidateMv_y;
                    ++mvMergeCandidateIndex;
                }
            }

            if(mvMergeCandidateIndex == totalMergeCandidates )
                break;

            // Add zero MV
            for (refPicIndex = 0; refPicIndex < MAX_NUM_OF_MV_MERGE_CANDIDATE && mvMergeCandidateIndex < totalMergeCandidates; ++refPicIndex)           
            {
                mergeCandidateArray[mvMergeCandidateIndex].predictionDirection   = BI_PRED;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].x      = 0;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_0].y      = 0;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].x      = 0;
                mergeCandidateArray[mvMergeCandidateIndex].mv[REF_LIST_1].y      = 0;
                ++mvMergeCandidateIndex;
            }
     

        break;

    default:
        break;
    }

    // Update the number of MV merge candidate      
    *mergeCandidateCount = mvMergeCandidateIndex;

    return return_error;
}




