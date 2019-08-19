/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbSequenceControlSet.h"

/**************************************************************************************************
    General notes on how Sequence Control Sets (SCS) are used.

    SequenceControlSetInstance 
        is the master copy that interacts with the API in real-time.  When a 
        change happens, the changeFlag is signaled so that appropriate action can
        be taken.  There is one scsInstance per stream/encode instance.  The scsInstance
        owns the encodeContext

    encodeContext
        has context type variables (i.e. non-config) that keep track of global parameters.

    SequenceControlSets
        general SCSs are controled by a system resource manager.  They are kept completely
        separate from the instances.  In general there is one active SCS at a time.  When the
        changeFlag is signaled, the old active SCS is no longer used for new input pictures.  
        A fresh copy of the scsInstance is made to a new SCS, which becomes the active SCS.  The
        old SCS will eventually be released back into the SCS pool when its current pictures are
        finished encoding.
        
    Motivations
        The whole reason for this structure is due to the nature of the pipeline.  We have to
        take great care not to have pipeline mismanagement.  Once an object enters use in the 
        pipeline, it cannot be changed on the fly or you will have pipeline coherency problems.
 ***************************************************************************************************/
EB_ERRORTYPE EbSequenceControlSetCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr)
{
    EbSequenceControlSetInitData_t *scsInitData = (EbSequenceControlSetInitData_t*) objectInitDataPtr;
    EB_U32 segmentIndex; 
    EB_ERRORTYPE return_error = EB_ErrorNone;
    SequenceControlSet_t *sequenceControlSetPtr;
    EB_MALLOC(SequenceControlSet_t*, sequenceControlSetPtr, sizeof(SequenceControlSet_t), EB_N_PTR);

    *objectDblPtr = (EB_PTR) sequenceControlSetPtr;
    
    sequenceControlSetPtr->staticConfig.qp                                  = 32;

    // Segments
    for(segmentIndex=0; segmentIndex < MAX_TEMPORAL_LAYERS; ++segmentIndex) {
        sequenceControlSetPtr->meSegmentColumnCountArray[segmentIndex] = 1;
        sequenceControlSetPtr->meSegmentRowCountArray[segmentIndex]    = 1;
        sequenceControlSetPtr->encDecSegmentColCountArray[segmentIndex] = 1;
        sequenceControlSetPtr->encDecSegmentRowCountArray[segmentIndex]  = 1;
    }
    
    // Encode Context
    if(scsInitData != EB_NULL) {
        sequenceControlSetPtr->encodeContextPtr                             = scsInitData->encodeContextPtr;
    }
    else {
        sequenceControlSetPtr->encodeContextPtr                             = (EncodeContext_t *)EB_NULL;
    }

	sequenceControlSetPtr->conformanceWindowFlag                            = 0;
    
    // Profile & ID
    sequenceControlSetPtr->spsId                                            = 0;
    sequenceControlSetPtr->vpsId                                            = 0;
    sequenceControlSetPtr->profileSpace                                     = 0;
    sequenceControlSetPtr->profileIdc                                       = 0;
    sequenceControlSetPtr->levelIdc                                         = 0;
    sequenceControlSetPtr->tierIdc                                          = 0;
    sequenceControlSetPtr->chromaFormatIdc                                  = EB_YUV420;
    sequenceControlSetPtr->maxTemporalLayers                                = 1;
    
    sequenceControlSetPtr->bitsForPictureOrderCount                         = 16;
    
    // Picture Dimensions
    sequenceControlSetPtr->lumaWidth                                        = 0;
    sequenceControlSetPtr->lumaHeight                                       = 0;

    sequenceControlSetPtr->chromaWidth                                      = 0;
    sequenceControlSetPtr->chromaHeight                                     = 0;
    sequenceControlSetPtr->frameRate                                        = 0;
    sequenceControlSetPtr->encoderBitDepth                                  = 8;
    
    // Bitdepth
    sequenceControlSetPtr->inputBitdepth                                    = EB_8BIT;
    sequenceControlSetPtr->outputBitdepth                                   = EB_8BIT;
    
    // GOP Structure
	sequenceControlSetPtr->maxRefCount                                      = 1;
    sequenceControlSetPtr->intraPeriodLength                                = 0;
    sequenceControlSetPtr->intraRefreshType                                 = 0;
    
    // LCU
    sequenceControlSetPtr->lcuSize                                          = 64;
    sequenceControlSetPtr->maxLcuDepth                                      = 3;
      
    // Interlaced Video
    sequenceControlSetPtr->interlacedVideo                                  = EB_FALSE;

    sequenceControlSetPtr->generalProgressiveSourceFlag                     = 1;
    sequenceControlSetPtr->generalInterlacedSourceFlag                      = 0;
    sequenceControlSetPtr->generalFrameOnlyConstraintFlag                   = 0;

    // temporal mvp enable flag
    sequenceControlSetPtr->enableTmvpSps                                    = 1;

    // Strong Intra Smoothing
    sequenceControlSetPtr->enableStrongIntraSmoothing                       = EB_TRUE;
    
    // Rate Control
    sequenceControlSetPtr->rateControlMode                                  = 0; 
    sequenceControlSetPtr->targetBitrate                                    = 0x1000; 
    sequenceControlSetPtr->availableBandwidth                               = 0x1000;
    
    // Quantization
    sequenceControlSetPtr->qp                                               = 20;

    // Mv merge
    sequenceControlSetPtr->mvMergeTotalCount                                = 5;

    // Video Usability Info
    EB_MALLOC(AppVideoUsabilityInfo_t*, sequenceControlSetPtr->videoUsabilityInfoPtr, sizeof(AppVideoUsabilityInfo_t), EB_N_PTR);
    
    // Initialize vui parameters
    return_error = EbVideoUsabilityInfoCtor(
        sequenceControlSetPtr->videoUsabilityInfoPtr);

    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    // Initialize picture timing SEI
    EbPictureTimeingSeiCtor(
       &sequenceControlSetPtr->picTimingSei);

    // Initialize picture timing SEI
    EbBufferingPeriodSeiCtor(
        &sequenceControlSetPtr->bufferingPeriod);

    //Initilaize Active Parametr Set SEI
    EbActiveParameterSetSeiCtor(
        &sequenceControlSetPtr->activeParameterSet);

        // Initialize picture timing SEI
    EbRecoveryPointSeiCtor(
        &sequenceControlSetPtr->recoveryPoint);

    // Initialize Content Light Level SEI
    EbContentLightLevelCtor(
        &sequenceControlSetPtr->contentLightLevel);

    // Initialize Mastering Color Volume SEI
    EbMasteringDisplayColorVolumeCtor(
        &sequenceControlSetPtr->masteringDisplayColorVolume);

    // Initialize Registered User Data SEI
    EbRegUserDataSEICtor(
        &sequenceControlSetPtr->regUserDataSeiPtr);

    // Initialize Un-Registered User Data SEI
    EbUnRegUserDataSEICtor(
        &sequenceControlSetPtr->unRegUserDataSeiPtr);

    // Initialize LCU params
    LcuParamsCtor(
        sequenceControlSetPtr);

    sequenceControlSetPtr->maxDpbSize	= 0;
    
    return EB_ErrorNone;
}
     

/************************************************
 * Sequence Control Set Copy
 ************************************************/
EB_ERRORTYPE CopySequenceControlSet(
    SequenceControlSet_t *dst,
    SequenceControlSet_t *src)
{
    AppVideoUsabilityInfo_t *videoUsabilityInfoPtr = dst->videoUsabilityInfoPtr;
    size_t sizeCopy = (size_t)((EB_U64)(&dst->pictureControlSetPoolInitCount) -
                               (EB_U64)(&dst->staticConfig));

    EB_MEMCPY(&dst->staticConfig, &src->staticConfig, sizeCopy);

    dst->videoUsabilityInfoPtr = videoUsabilityInfoPtr;
    EbVideoUsabilityInfoCopy(
        dst->videoUsabilityInfoPtr,
        src->videoUsabilityInfoPtr);

    dst->enableDenoiseFlag = src->enableDenoiseFlag;
    dst->maxEncMode        = src->maxEncMode;

    EB_MEMCPY(&dst->activeParameterSet, &src->activeParameterSet, sizeof(AppActiveparameterSetSei_t));

    return EB_ErrorNone;
}
    
EB_ERRORTYPE EbSequenceControlSetInstanceCtor(
    EbSequenceControlSetInstance_t **objectDblPtr)
{
    EbSequenceControlSetInitData_t scsInitData;
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_MALLOC(EbSequenceControlSetInstance_t*, *objectDblPtr, sizeof(EbSequenceControlSetInstance_t), EB_N_PTR);

    return_error = EncodeContextCtor(
        (void **) &(*objectDblPtr)->encodeContextPtr,
        EB_NULL);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    scsInitData.encodeContextPtr = (*objectDblPtr)->encodeContextPtr;
    
    return_error = EbSequenceControlSetCtor(
        (void **) &(*objectDblPtr)->sequenceControlSetPtr,
        (void *) &scsInitData);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    
    EB_CREATEMUTEX(EB_HANDLE*, (*objectDblPtr)->configMutex, sizeof(EB_HANDLE), EB_MUTEX);

        
    return EB_ErrorNone;
}    
       

extern EB_ERRORTYPE LcuParamsCtor(
	SequenceControlSet_t *sequenceControlSetPtr) {

	EB_ERRORTYPE return_error = EB_ErrorNone;

	EB_MALLOC(LcuParams_t*, sequenceControlSetPtr->lcuParamsArray, sizeof(LcuParams_t) * ((MAX_PICTURE_WIDTH_SIZE + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize) * ((MAX_PICTURE_HEIGHT_SIZE + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize), EB_N_PTR);
	return return_error;
}


extern EB_ERRORTYPE LcuParamsInit(
	SequenceControlSet_t *sequenceControlSetPtr) {

	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U16	lcuIndex;
	EB_U16	rasterScanCuIndex;

	EB_U8   pictureLcuWidth  = sequenceControlSetPtr->pictureWidthInLcu;
	EB_U8	pictureLcuHeight = sequenceControlSetPtr->pictureHeightInLcu;
	EB_MALLOC(LcuParams_t*, sequenceControlSetPtr->lcuParamsArray, sizeof(LcuParams_t) * pictureLcuWidth * pictureLcuHeight, EB_N_PTR);

	for (lcuIndex = 0; lcuIndex < pictureLcuWidth * pictureLcuHeight; ++lcuIndex) {
		sequenceControlSetPtr->lcuParamsArray[lcuIndex].horizontalIndex = (EB_U8)(lcuIndex % pictureLcuWidth);
		sequenceControlSetPtr->lcuParamsArray[lcuIndex].verticalIndex   = (EB_U8)(lcuIndex / pictureLcuWidth);
        sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX         = sequenceControlSetPtr->lcuParamsArray[lcuIndex].horizontalIndex * sequenceControlSetPtr->lcuSize;
		sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY         = sequenceControlSetPtr->lcuParamsArray[lcuIndex].verticalIndex * sequenceControlSetPtr->lcuSize;

		sequenceControlSetPtr->lcuParamsArray[lcuIndex].width = (EB_U8)(((sequenceControlSetPtr->lumaWidth - sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX)  < sequenceControlSetPtr->lcuSize) ?
			sequenceControlSetPtr->lumaWidth - sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX :
			sequenceControlSetPtr->lcuSize);

		sequenceControlSetPtr->lcuParamsArray[lcuIndex].height = (EB_U8)(((sequenceControlSetPtr->lumaHeight - sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY) < sequenceControlSetPtr->lcuSize) ?
			sequenceControlSetPtr->lumaHeight - sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY :
			sequenceControlSetPtr->lcuSize);

		sequenceControlSetPtr->lcuParamsArray[lcuIndex].isCompleteLcu = (EB_U8)(((sequenceControlSetPtr->lcuParamsArray[lcuIndex].width == sequenceControlSetPtr->lcuSize) && (sequenceControlSetPtr->lcuParamsArray[lcuIndex].height == sequenceControlSetPtr->lcuSize)) ?
			1 :
			0);

		sequenceControlSetPtr->lcuParamsArray[lcuIndex].isEdgeLcu = (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX < sequenceControlSetPtr->lcuSize) ||
			(sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY < sequenceControlSetPtr->lcuSize) ||
			(sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX > sequenceControlSetPtr->lumaWidth - sequenceControlSetPtr->lcuSize) ||
			(sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY > sequenceControlSetPtr->lumaHeight - sequenceControlSetPtr->lcuSize) ? 1 : 0;


        EB_U8 potentialLogoLcu = 0;

        // 4K
        /*__ 14 __         __ 14__*/
        ///////////////////////////
        //		 |		  |		 //
        //		 8		  8		 //
        //___14_ |		  |_14___//
        //						 //
        //						 //
        //-----------------------// |
        //						 // 8
        ///////////////////////////	|		

        // 1080p/720P
        /*__ 7 __          __ 7__*/
        ///////////////////////////
        //		 |		  |		 //
        //		 4		  4		 //
        //___7__ |		  |__7___//
        //						 //
        //						 //
        //-----------------------// |
        //						 // 4
        ///////////////////////////	|	

        // 480P
        /*__ 3 __          __ 3__*/
        ///////////////////////////
        //		 |		  |		 //
        //		 2		  2		 //
        //___3__ |		  |__3___//
        //						 //
        //						 //
        //-----------------------// |
        //						 // 2
        ///////////////////////////	|	
        if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_576p_RANGE_OR_LOWER){
            potentialLogoLcu = (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX >= (sequenceControlSetPtr->lumaWidth - (3 * sequenceControlSetPtr->lcuSize))) && (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY < 2 * sequenceControlSetPtr->lcuSize) ? 1 : potentialLogoLcu;
            potentialLogoLcu = (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX <  ((3 * sequenceControlSetPtr->lcuSize))) && (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY < 2 * sequenceControlSetPtr->lcuSize) ? 1 : potentialLogoLcu;
            potentialLogoLcu = (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY >= (sequenceControlSetPtr->lumaHeight - (2 * sequenceControlSetPtr->lcuSize))) ? 1 : potentialLogoLcu;

        }
        else
        if (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE){
            potentialLogoLcu = (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX >= (sequenceControlSetPtr->lumaWidth - (7 * sequenceControlSetPtr->lcuSize))) && (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY < 4 * sequenceControlSetPtr->lcuSize) ? 1 : potentialLogoLcu;
            potentialLogoLcu = (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX <  ((7 * sequenceControlSetPtr->lcuSize))) && (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY < 4 * sequenceControlSetPtr->lcuSize) ? 1 : potentialLogoLcu;
            potentialLogoLcu = (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY >= (sequenceControlSetPtr->lumaHeight - (4 * sequenceControlSetPtr->lcuSize))) ? 1 : potentialLogoLcu;

        }
        else{

            potentialLogoLcu = (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX >= (sequenceControlSetPtr->lumaWidth - (14 * sequenceControlSetPtr->lcuSize))) && (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY < 8 * sequenceControlSetPtr->lcuSize) ? 1 : potentialLogoLcu;
            potentialLogoLcu = (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX < ((14 * sequenceControlSetPtr->lcuSize))) && (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY < 8 * sequenceControlSetPtr->lcuSize) ? 1 : potentialLogoLcu;
            potentialLogoLcu = (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY >= (sequenceControlSetPtr->lumaHeight - (8 * sequenceControlSetPtr->lcuSize))) ? 1 : potentialLogoLcu;

        }
        sequenceControlSetPtr->lcuParamsArray[lcuIndex].potentialLogoLcu = potentialLogoLcu;

		for (rasterScanCuIndex = RASTER_SCAN_CU_INDEX_64x64; rasterScanCuIndex <= RASTER_SCAN_CU_INDEX_8x8_63; rasterScanCuIndex++) {

			sequenceControlSetPtr->lcuParamsArray[lcuIndex].rasterScanCuValidity[rasterScanCuIndex] = ((sequenceControlSetPtr->lcuParamsArray[lcuIndex].originX + RASTER_SCAN_CU_X[rasterScanCuIndex] + RASTER_SCAN_CU_SIZE[rasterScanCuIndex] > sequenceControlSetPtr->lumaWidth ) || (sequenceControlSetPtr->lcuParamsArray[lcuIndex].originY + RASTER_SCAN_CU_Y[rasterScanCuIndex] + RASTER_SCAN_CU_SIZE[rasterScanCuIndex] > sequenceControlSetPtr->lumaHeight)) ?
				EB_FALSE :
				EB_TRUE;
		}
	}

    //ConfigureTiles(sequenceControlSetPtr);

	return return_error;
}

extern EB_ERRORTYPE DeriveInputResolution(
	SequenceControlSet_t *sequenceControlSetPtr,
	EB_U32				  inputSize) {
	EB_ERRORTYPE return_error = EB_ErrorNone;

	sequenceControlSetPtr->inputResolution = (inputSize < INPUT_SIZE_1080i_TH) ?
		INPUT_SIZE_576p_RANGE_OR_LOWER :
		(inputSize < INPUT_SIZE_1080p_TH) ?
			INPUT_SIZE_1080i_RANGE :
			(inputSize < INPUT_SIZE_4K_TH) ?
				INPUT_SIZE_1080p_RANGE :
				INPUT_SIZE_4K_RANGE;

	return return_error;
}

