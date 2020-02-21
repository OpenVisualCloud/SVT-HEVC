/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbDefinitions.h"
#include "EbSequenceControlSet.h"

static void EbSequenceControlSetDctor(EB_PTR p)
{
    SequenceControlSet_t *obj = (SequenceControlSet_t*)p;
    EB_FREE_ARRAY(obj->lcuParamsArray);
    EB_DELETE(obj->videoUsabilityInfoPtr);
}

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
    SequenceControlSet_t *sequenceControlSetPtr,
    EB_PTR objectInitDataPtr)
{
    sequenceControlSetPtr->dctor = EbSequenceControlSetDctor;
    EbSequenceControlSetInitData_t *scsInitData = (EbSequenceControlSetInitData_t*) objectInitDataPtr;
    sequenceControlSetPtr->staticConfig.qp = 32;

    // Segments
    for(EB_U32 layerIndex=0; layerIndex < MAX_TEMPORAL_LAYERS; ++layerIndex) {
        sequenceControlSetPtr->meSegmentColumnCountArray[layerIndex] = 1;
        sequenceControlSetPtr->meSegmentRowCountArray[layerIndex]    = 1;
        sequenceControlSetPtr->encDecSegmentColCountArray[layerIndex] = 1;
        sequenceControlSetPtr->encDecSegmentRowCountArray[layerIndex]  = 1;
        sequenceControlSetPtr->tileGroupColCountArray[layerIndex] = 1;
        sequenceControlSetPtr->tileGroupRowCountArray[layerIndex] = 1;
    }
    
    // Encode Context
    if(scsInitData != EB_NULL) {
        sequenceControlSetPtr->encodeContextPtr                             = scsInitData->encodeContextPtr;
    }

    // Profile & ID
    sequenceControlSetPtr->chromaFormatIdc                                  = EB_YUV420;
    sequenceControlSetPtr->maxTemporalLayers                                = 1;

    sequenceControlSetPtr->bitsForPictureOrderCount                         = 16;

    sequenceControlSetPtr->encoderBitDepth                                  = 8;

    // Bitdepth
    sequenceControlSetPtr->inputBitdepth                                    = EB_8BIT;
    sequenceControlSetPtr->outputBitdepth                                   = EB_8BIT;
    
    // GOP Structure
    sequenceControlSetPtr->maxRefCount                                      = 1;

    // LCU
    sequenceControlSetPtr->lcuSize                                          = 64;
    sequenceControlSetPtr->maxLcuDepth                                      = 3;

    sequenceControlSetPtr->generalProgressiveSourceFlag                     = 1;

    // temporal mvp enable flag
    sequenceControlSetPtr->enableTmvpSps                                    = 1;

    // Strong Intra Smoothing
    sequenceControlSetPtr->enableStrongIntraSmoothing                       = EB_TRUE;
    
    // Rate Control
    sequenceControlSetPtr->targetBitrate                                    = 0x1000; 
    sequenceControlSetPtr->availableBandwidth                               = 0x1000;

    // Quantization
    sequenceControlSetPtr->qp                                               = 20;

    // Mv merge
    sequenceControlSetPtr->mvMergeTotalCount                                = 5;

    // Initialize vui parameters
    EB_NEW(
        sequenceControlSetPtr->videoUsabilityInfoPtr,
        EbVideoUsabilityInfoCtor);

    return EB_ErrorNone;
}

EB_ERRORTYPE EbSequenceControlSetCreator(
    EB_PTR  *objectDblPtr,
    EB_PTR   objectInitDataPtr)
{
    SequenceControlSet_t* obj;

    *objectDblPtr = NULL;
    EB_NEW(obj, EbSequenceControlSetCtor, objectInitDataPtr);
    *objectDblPtr = obj;

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

static void EbSequenceControlSetInstanceDctor(EB_PTR p)
{
    EbSequenceControlSetInstance_t* obj = (EbSequenceControlSetInstance_t*)p;
    EB_DELETE(obj->encodeContextPtr);
    EB_DELETE(obj->sequenceControlSetPtr);
    EB_DESTROY_MUTEX(obj->configMutex);
}

EB_ERRORTYPE EbSequenceControlSetInstanceCtor(
    EbSequenceControlSetInstance_t *objectPtr)
{
    EbSequenceControlSetInitData_t scsInitData;
    objectPtr->dctor = EbSequenceControlSetInstanceDctor;

    EB_NEW(
        objectPtr->encodeContextPtr,
        EncodeContextCtor,
        EB_NULL);
    scsInitData.encodeContextPtr = objectPtr->encodeContextPtr;

    EB_NEW(
        objectPtr->sequenceControlSetPtr,
        EbSequenceControlSetCtor,
        (void *) &scsInitData);

    EB_CREATE_MUTEX(objectPtr->configMutex);

    return EB_ErrorNone;
}

EB_ERRORTYPE LcuParamsInit(
	SequenceControlSet_t *sequenceControlSetPtr) {

	EB_ERRORTYPE return_error = EB_ErrorNone;
	EB_U16	lcuIndex;
	EB_U16	rasterScanCuIndex;

	EB_U8   pictureLcuWidth  = sequenceControlSetPtr->pictureWidthInLcu;
	EB_U8	pictureLcuHeight = sequenceControlSetPtr->pictureHeightInLcu;

    EB_MALLOC_ARRAY(sequenceControlSetPtr->lcuParamsArray, pictureLcuWidth * pictureLcuHeight);

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

