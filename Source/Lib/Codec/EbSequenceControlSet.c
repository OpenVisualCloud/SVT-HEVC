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
    EB_U32  writeCount = 0;
    EB_U32  segmentIndex = 0;
    
    dst->staticConfig               = src->staticConfig;                            writeCount += sizeof(EB_H265_ENC_CONFIGURATION);           
    dst->encodeContextPtr           = src->encodeContextPtr;                        writeCount += sizeof(EncodeContext_t*);            
    dst->spsId                      = src->spsId;                                   writeCount += sizeof(EB_U32);
    dst->vpsId                      = src->vpsId;                                   writeCount += sizeof(EB_U32);
    dst->profileSpace               = src->profileSpace;                            writeCount += sizeof(EB_U32);
    dst->profileIdc                 = src->profileIdc;                              writeCount += sizeof(EB_U32);                     
    dst->levelIdc                   = src->levelIdc;                                writeCount += sizeof(EB_U32);                     
    dst->tierIdc                    = src->tierIdc;                                 writeCount += sizeof(EB_U32);                     
    dst->chromaFormatIdc            = src->chromaFormatIdc;                         writeCount += sizeof(EB_U32);                     
    dst->maxTemporalLayers          = src->maxTemporalLayers;                       writeCount += sizeof(EB_U32);                     
    dst->bitsForPictureOrderCount   = src->bitsForPictureOrderCount;                writeCount += sizeof(EB_U32);                     
    dst->maxInputLumaWidth          = src->maxInputLumaWidth;                       writeCount += sizeof(EB_U32);                     
    dst->maxInputLumaHeight         = src->maxInputLumaHeight;                      writeCount += sizeof(EB_U32);  
    dst->maxInputPadRight           = src->maxInputPadRight;                        writeCount += sizeof(EB_U32);
    dst->maxInputPadBottom          = src->maxInputPadBottom;                       writeCount += sizeof(EB_U32);
    dst->lumaWidth                  = src->lumaWidth;                               writeCount += sizeof(EB_U32);                     
    dst->lumaHeight                 = src->lumaHeight;                              writeCount += sizeof(EB_U32); 
    dst->chromaWidth                = src->chromaWidth;                             writeCount += sizeof(EB_U32);                     
    dst->chromaHeight               = src->chromaHeight;                            writeCount += sizeof(EB_U32);                     
    dst->padRight                   = src->padRight;                                writeCount += sizeof(EB_U32);
    dst->padBottom                  = src->padBottom;                               writeCount += sizeof(EB_U32);
	dst->croppingLeftOffset         = src->croppingLeftOffset;                      writeCount += sizeof(EB_S32);     
    dst->croppingRightOffset        = src->croppingRightOffset;                     writeCount += sizeof(EB_S32);     
    dst->croppingTopOffset          = src->croppingTopOffset;                       writeCount += sizeof(EB_S32);
    dst->croppingBottomOffset       = src->croppingBottomOffset;                    writeCount += sizeof(EB_S32);
    dst->conformanceWindowFlag      = src->conformanceWindowFlag;                   writeCount += sizeof(EB_U32);  
    dst->frameRate                  = src->frameRate;                               writeCount += sizeof(EB_U32);                     
    dst->inputBitdepth              = src->inputBitdepth;                           writeCount += sizeof(EB_BITDEPTH);                
    dst->outputBitdepth             = src->outputBitdepth;                          writeCount += sizeof(EB_BITDEPTH);        
	dst->predStructPtr              = src->predStructPtr;                           writeCount += sizeof(PredictionStructure_t*);
    dst->intraPeriodLength          = src->intraPeriodLength;                       writeCount += sizeof(EB_S32);                     
    dst->intraRefreshType           = src->intraRefreshType;                        writeCount += sizeof(EB_U32);  
    dst->maxRefCount                = src->maxRefCount;                             writeCount += sizeof(EB_U32);
    dst->lcuSize                    = src->lcuSize;                                 writeCount += sizeof(EB_U32);                     
    dst->maxLcuDepth                = src->maxLcuDepth;                             writeCount += sizeof(EB_U32);
                                                               
    dst->interlacedVideo            = src->interlacedVideo ;                        writeCount += sizeof(EB_BOOL);
                                                                              
    dst->generalProgressiveSourceFlag   = src->generalProgressiveSourceFlag;        writeCount += sizeof(EB_U32);
    dst->generalInterlacedSourceFlag    = src->generalInterlacedSourceFlag;         writeCount += sizeof(EB_U32);
    dst->generalFrameOnlyConstraintFlag = src->generalFrameOnlyConstraintFlag;      writeCount += sizeof(EB_U32);
    dst->targetBitrate              = src->targetBitrate;                           writeCount += sizeof(EB_U32);                     
    dst->availableBandwidth         = src->availableBandwidth;                      writeCount += sizeof(EB_U32);                     
    dst->qp                         = src->qp;                                      writeCount += sizeof(EB_U32);   
    dst->enableQpScalingFlag        = src->enableQpScalingFlag;                     writeCount += sizeof(EB_BOOL);                  
    dst->enableTmvpSps              = src->enableTmvpSps;                           writeCount += sizeof(EB_U32);   
    dst->mvMergeTotalCount          = src->mvMergeTotalCount;                       writeCount += sizeof(EB_U32);            
    dst->leftPadding                = src->leftPadding;                             writeCount += sizeof(EB_U16);
    dst->rightPadding               = src->rightPadding;                            writeCount += sizeof(EB_U16);
    dst->topPadding                 = src->topPadding;                              writeCount += sizeof(EB_U16);
    dst->botPadding                 = src->botPadding;                              writeCount += sizeof(EB_U16);         
    dst->enableDenoiseFlag          = src->enableDenoiseFlag;                       writeCount += sizeof(EB_BOOL);
    dst->maxEncMode                 = src->maxEncMode;                              writeCount += sizeof(EB_U8);

    // Segments
    for (segmentIndex = 0; segmentIndex < MAX_TEMPORAL_LAYERS; ++segmentIndex) {
        dst->meSegmentColumnCountArray[segmentIndex] = src->meSegmentColumnCountArray[segmentIndex]; writeCount += sizeof(EB_U32);
        dst->meSegmentRowCountArray[segmentIndex] = src->meSegmentRowCountArray[segmentIndex]; writeCount += sizeof(EB_U32);
        dst->encDecSegmentColCountArray[segmentIndex] = src->encDecSegmentColCountArray[segmentIndex]; writeCount += sizeof(EB_U32);
        dst->encDecSegmentRowCountArray[segmentIndex] = src->encDecSegmentRowCountArray[segmentIndex]; writeCount += sizeof(EB_U32);
    }

    EB_MEMCPY(
        &dst->bufferingPeriod,
        &src->bufferingPeriod,
        sizeof(AppBufferingPeriodSei_t));

    writeCount += sizeof(AppBufferingPeriodSei_t);

    EB_MEMCPY(
        &dst->recoveryPoint,
        &src->recoveryPoint,
        sizeof(AppRecoveryPoint_t));

    writeCount += sizeof(AppRecoveryPoint_t);

    EB_MEMCPY(
        &dst->contentLightLevel,
        &src->contentLightLevel,
        sizeof(AppContentLightLevelSei_t));

    writeCount += sizeof(AppContentLightLevelSei_t);

    EB_MEMCPY(
        &dst->masteringDisplayColorVolume,
        &src->masteringDisplayColorVolume,
        sizeof(AppMasteringDisplayColorVolumeSei_t));

    writeCount += sizeof(AppMasteringDisplayColorVolumeSei_t);

    EB_MEMCPY(
        &dst->picTimingSei,
        &src->picTimingSei,
        sizeof(AppPictureTimingSei_t));

    writeCount += sizeof(AppPictureTimingSei_t);

    EB_MEMCPY(
        &dst->regUserDataSeiPtr,
        &src->regUserDataSeiPtr,
        sizeof(RegistedUserData_t));

    writeCount += sizeof(RegistedUserData_t);

    EB_MEMCPY(
        &dst->unRegUserDataSeiPtr,
        &src->unRegUserDataSeiPtr,
        sizeof(UnregistedUserData_t));

    writeCount += sizeof(UnregistedUserData_t);

    EbVideoUsabilityInfoCopy(
        dst->videoUsabilityInfoPtr,
        src->videoUsabilityInfoPtr);
    
    writeCount += sizeof(AppVideoUsabilityInfo_t*); 

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

/************************************************
 * Configure ME Tiles
 ************************************************/
static void ConfigureTiles(
    SequenceControlSet_t *scsPtr)
{
    // Tiles Initialisation
    const unsigned tileColumns = scsPtr->tileColumnCount;
    const unsigned tileRows = scsPtr->tileRowCount;


    unsigned rowIndex, columnIndex;
    unsigned xLcuIndex, yLcuIndex, lcuIndex;
    unsigned xLcuStart, yLcuStart;

    // Tile-loops
    yLcuStart = 0;
    for (rowIndex = 0; rowIndex < tileRows; ++rowIndex) {
        xLcuStart = 0;
        for (columnIndex = 0; columnIndex < tileColumns; ++columnIndex) {

            // LCU-loops
            for (yLcuIndex = yLcuStart; yLcuIndex < yLcuStart + scsPtr->tileRowArray[rowIndex]; ++yLcuIndex) {
                for (xLcuIndex = xLcuStart; xLcuIndex < xLcuStart + scsPtr->tileColumnArray[columnIndex]; ++xLcuIndex) {
                    lcuIndex = xLcuIndex + yLcuIndex * scsPtr->pictureWidthInLcu;
                    scsPtr->lcuParamsArray[lcuIndex].tileLeftEdgeFlag = (xLcuIndex == xLcuStart) ? EB_TRUE : EB_FALSE;
                    scsPtr->lcuParamsArray[lcuIndex].tileTopEdgeFlag = (yLcuIndex == yLcuStart) ? EB_TRUE : EB_FALSE;
                    scsPtr->lcuParamsArray[lcuIndex].tileRightEdgeFlag =
                        (xLcuIndex == xLcuStart + scsPtr->tileColumnArray[columnIndex] - 1) ? EB_TRUE : EB_FALSE;
                    scsPtr->lcuParamsArray[lcuIndex].tileStartX = xLcuStart * scsPtr->lcuSize;
                    scsPtr->lcuParamsArray[lcuIndex].tileStartY = yLcuStart * scsPtr->lcuSize;
                    scsPtr->lcuParamsArray[lcuIndex].tileEndX = (columnIndex == (tileColumns - 1)) ? scsPtr->lumaWidth : (xLcuStart + scsPtr->tileColumnArray[columnIndex]) * scsPtr->lcuSize;
                    scsPtr->lcuParamsArray[lcuIndex].tileEndY = (rowIndex == (tileRows - 1)) ? scsPtr->lumaHeight : (yLcuStart + scsPtr->tileRowArray[rowIndex]) * scsPtr->lcuSize;
                    scsPtr->lcuParamsArray[lcuIndex].tileIndex = rowIndex * tileColumns + columnIndex;
                }
            }
            xLcuStart += scsPtr->tileColumnArray[columnIndex];
        }
        yLcuStart += scsPtr->tileRowArray[rowIndex];
    }

    return;
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

    ConfigureTiles(sequenceControlSetPtr);

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

