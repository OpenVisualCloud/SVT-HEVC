/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbReferenceObject_h
#define EbReferenceObject_h

#include "EbDefinitions.h"
#include "EbAdaptiveMotionVectorPrediction.h" 
#ifdef __cplusplus
extern "C" {
#endif

typedef struct EbReferenceObject_s {
    EbPictureBufferDesc_t          *referencePicture;
	EbPictureBufferDesc_t		   *referencePicture16bit;
    EbPictureBufferDesc_t          *refDenSrcPicture;	 

    TmvpUnit_t                     *tmvpMap; 
    EB_BOOL                         tmvpEnableFlag;
    EB_U64                          refPOC;

    EB_U8                           qp;
    EB_PICTURE                        sliceType;

	EB_U8                          intraCodedArea;//percentage of intra coded area 0-100%

	EB_U8                          intraCodedAreaLCU[MAX_NUMBER_OF_TREEBLOCKS_PER_PICTURE];//percentage of intra coded area 0-100%
	EB_U32                         nonMovingIndexArray[MAX_NUMBER_OF_TREEBLOCKS_PER_PICTURE];//array to hold non-moving blocks in reference frames
	EB_U32                         picSampleValue[MAX_NUMBER_OF_REGIONS_IN_WIDTH][MAX_NUMBER_OF_REGIONS_IN_HEIGHT][3];// [Y U V];

	EB_BOOL                        penalizeSkipflag;

	EB_U8                          tmpLayerIdx;
	EB_BOOL                        isSceneChange;
	EB_U16                         picAvgVariance;
    EB_U8                          averageIntensity;

} EbReferenceObject_t;

typedef struct EbReferenceObjectDescInitData_s {
    EbPictureBufferDescInitData_t   referencePictureDescInitData;
} EbReferenceObjectDescInitData_t;

typedef struct EbPaReferenceObject_s {
    EbPictureBufferDesc_t          *inputPaddedPicturePtr;
    EbPictureBufferDesc_t          *quarterDecimatedPicturePtr; 
    EbPictureBufferDesc_t          *sixteenthDecimatedPicturePtr;
	EB_U16                         variance[MAX_NUMBER_OF_TREEBLOCKS_PER_PICTURE];
	EB_U8                          yMean[MAX_NUMBER_OF_TREEBLOCKS_PER_PICTURE];
	EB_PICTURE                       sliceType;

	EB_U32 dependentPicturesCount; //number of pic using this reference frame  
    PictureParentControlSet_t       *pPcsPtr;
} EbPaReferenceObject_t;

typedef struct EbPaReferenceObjectDescInitData_s {
    EbPictureBufferDescInitData_t   referencePictureDescInitData;
    EbPictureBufferDescInitData_t   quarterPictureDescInitData;
    EbPictureBufferDescInitData_t   sixteenthPictureDescInitData;
} EbPaReferenceObjectDescInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EbReferenceObjectCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);

extern EB_ERRORTYPE EbPaReferenceObjectCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);

#ifdef __cplusplus
}
#endif
#endif //EbReferenceObject_h