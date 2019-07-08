/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbSequenceControlSet_h
#define EbSequenceControlSet_h

#include "EbDefinitions.h"
#include "EbThreads.h"
#include "EbSystemResourceManager.h"
#include "EbEncodeContext.h"
#include "EbPredictionStructure.h"
#include "EbSei.h"

#ifdef __cplusplus
extern "C" {
#endif


/************************************
 * Sequence Control Set
 ************************************/
typedef struct SequenceControlSet_s
{        
    EB_H265_ENC_CONFIGURATION   staticConfig;

    // Tiles
    // Better to put into PictureControlSet
    EB_U32              tileUniformSpacing;
    EB_U16              tileColumnCount;
    EB_U16              tileRowCount;
    EB_U8               tileSliceMode;
    EB_U16              tileColumnWidthArray[EB_TILE_COLUMN_MAX_COUNT];
    EB_U16              tileRowHeightArray[EB_TILE_ROW_MAX_COUNT];
    EB_U16              tileColumnArray[EB_TILE_COLUMN_MAX_COUNT];
    EB_U16              tileRowArray[EB_TILE_ROW_MAX_COUNT];
    
    // Encoding Context
    EncodeContext_t            *encodeContextPtr;
    
    // Profile & ID
    EB_U32                      spsId;
    EB_U32                      vpsId;
    EB_U32                      profileSpace;
    EB_U32                      profileIdc;
    EB_U32                      levelIdc;
    EB_U32                      tierIdc;
    EB_U32                      chromaFormatIdc;
    EB_U32                      maxTemporalLayers;                  
    EB_U32                      bitsForPictureOrderCount;
    
    // Picture deminsions
	EB_U16                      maxInputLumaWidth;
	EB_U16                      maxInputLumaHeight;
	//EB_U16                      maxInputChromaWidth;
	//EB_U16                      maxInputChromaHeight; 
	EB_U16                      maxInputPadRight;
	EB_U16                      maxInputPadBottom;
    EB_U16                      lumaWidth;
	EB_U16                      lumaHeight;
    EB_U32                      chromaWidth;
    EB_U32                      chromaHeight;
    EB_U32                      padRight;
    EB_U32                      padBottom;
    EB_U16                      leftPadding;
    EB_U16                      topPadding;
    EB_U16                      rightPadding;
    EB_U16                      botPadding;


    EB_U32                      frameRate;  
    EB_U32                      encoderBitDepth;

	// Cropping Definitions    
    EB_S32                      croppingLeftOffset;
    EB_S32                      croppingRightOffset;
    EB_S32                      croppingTopOffset;
    EB_S32                      croppingBottomOffset;    

    // Conformance Window flag
    EB_U32                      conformanceWindowFlag;
    
    // Bitdepth
    EB_BITDEPTH                 inputBitdepth;
    EB_BITDEPTH                 outputBitdepth;
    
    // Group of Pictures (GOP) Structure
    EB_U32                      maxRefCount;            // Maximum number of reference pictures, however each pred
                                                        //   entry can be less.
    PredictionStructure_t      *predStructPtr;          
    EB_S32                      intraPeriodLength;      // The frequency of intra pictures
    EB_U32                      intraRefreshType;       // 1: CRA, 2: IDR
    
    // LCU
    EB_U8                       lcuSize;
    EB_U8                       maxLcuDepth;

    // Interlaced Video 
    EB_BOOL                     interlacedVideo;

    EB_U32                      generalProgressiveSourceFlag;
    EB_U32                      generalInterlacedSourceFlag;
    EB_U32                      generalFrameOnlyConstraintFlag;
    
    // Rate Control
    EB_U32                      rateControlMode;
    EB_U32                      targetBitrate;
    EB_U32                      availableBandwidth;          
    
    // Quantization
    EB_U32                      qp;
    EB_BOOL                     enableQpScalingFlag;
    
    // tmvp enable 
    EB_U32                      enableTmvpSps;

    // Strong Intra Smoothing
    EB_BOOL                     enableStrongIntraSmoothing;

    // MV merge
    EB_U32                      mvMergeTotalCount;

    // MD5
    EB_U32                      reduceCodingLoopCandidates;

    // Video Usability Info
    AppVideoUsabilityInfo_t    *videoUsabilityInfoPtr;
    
    // Picture timing sei
    AppPictureTimingSei_t       picTimingSei;

    // Buffering period
    AppBufferingPeriodSei_t     bufferingPeriod;

    // Recovery point
    AppRecoveryPoint_t          recoveryPoint;

    // Content Light Level sei
    AppContentLightLevelSei_t   contentLightLevel;

    // Mastering Display Color Volume Sei
    AppMasteringDisplayColorVolumeSei_t   masteringDisplayColorVolume;

    // Registered User data Sei
    RegistedUserData_t          regUserDataSeiPtr;

    // Un Registered User data Sei
    UnregistedUserData_t        unRegUserDataSeiPtr;

    // Maximum Decoded Picture Buffer size.
    EB_U32                      maxDpbSize;
    
    // Picture Analysis
	EB_U32						pictureAnalysisNumberOfRegionsPerWidth;
	EB_U32						pictureAnalysisNumberOfRegionsPerHeight;

	// A picure is marked as active if the number of active regions is higher than pictureActivityRegionTh
	EB_U32						pictureActivityRegionTh;

    // Segments
    EB_U32                     meSegmentColumnCountArray[MAX_TEMPORAL_LAYERS];
    EB_U32                     meSegmentRowCountArray[MAX_TEMPORAL_LAYERS];
    EB_U32                     encDecSegmentColCountArray[MAX_TEMPORAL_LAYERS];
    EB_U32                     encDecSegmentRowCountArray[MAX_TEMPORAL_LAYERS];

    // Buffers
	EB_U32						pictureControlSetPoolInitCount;      
	EB_U32						pictureControlSetPoolInitCountChild; 
	EB_U32						paReferencePictureBufferInitCount;
	EB_U32						referencePictureBufferInitCount;
    EB_U32                      reconBufferFifoInitCount;
	EB_U32						inputOutputBufferFifoInitCount;
	EB_U32						resourceCoordinationFifoInitCount;     
	EB_U32						pictureAnalysisFifoInitCount;
	EB_U32						pictureDecisionFifoInitCount;
	EB_U32						motionEstimationFifoInitCount;
	EB_U32						initialRateControlFifoInitCount;
	EB_U32						pictureDemuxFifoInitCount;             
	EB_U32						rateControlTasksFifoInitCount;           
	EB_U32						rateControlFifoInitCount;                
	EB_U32						modeDecisionConfigurationFifoInitCount;
	//EB_U32						modeDecisionFifoInitCount;            
	EB_U32						encDecFifoInitCount;
	EB_U32						entropyCodingFifoInitCount;
	EB_U32						pictureAnalysisProcessInitCount;     
	EB_U32						motionEstimationProcessInitCount; 
	EB_U32						sourceBasedOperationsProcessInitCount;
	EB_U32						modeDecisionConfigurationProcessInitCount; 
    EB_U32						encDecProcessInitCount;
    EB_U32						entropyCodingProcessInitCount;

    EB_U32						totalProcessInitCount;

	LcuParams_t                *lcuParamsArray;
    EB_U8 						pictureWidthInLcu;
	EB_U8 						pictureHeightInLcu;
	EB_U16						lcuTotalCount;
	EB_INPUT_RESOLUTION			inputResolution;
	EB_SCD_MODE  				scdMode;

    EB_BOOL				        enableDenoiseFlag;

    EB_U8                       transCoeffShapeArray[2][8][4];    // [componantTypeIndex][resolutionIndex][levelIndex][tuSizeIndex]

    EB_U8                       maxEncMode;

} SequenceControlSet_t;

typedef struct EbSequenceControlSetInitData_s
{
    EncodeContext_t            *encodeContextPtr;
} EbSequenceControlSetInitData_t;

typedef struct EbSequenceControlSetInstance_s
{       
    EncodeContext_t            *encodeContextPtr;
    SequenceControlSet_t       *sequenceControlSetPtr;
    EB_HANDLE                   configMutex;
      
} EbSequenceControlSetInstance_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EbSequenceControlSetCtor(
    EB_PTR                          *objectDblPtr, 
    EB_PTR                           objectInitDataPtr);
    

    
extern EB_ERRORTYPE CopySequenceControlSet(
    SequenceControlSet_t            *dst,
    SequenceControlSet_t            *src);
        
extern EB_ERRORTYPE EbSequenceControlSetInstanceCtor(
    EbSequenceControlSetInstance_t **objectDblPtr);
    


extern EB_ERRORTYPE LcuParamsCtor(
	SequenceControlSet_t *sequenceControlSetPtr);

extern EB_ERRORTYPE LcuParamsInit(
	SequenceControlSet_t *sequenceControlSetPtr);
extern EB_ERRORTYPE DeriveInputResolution(
	SequenceControlSet_t *sequenceControlSetPtr,
	EB_U32				  inputSize);

#ifdef __cplusplus
}
#endif 
#endif // EbSequenceControlSet_h
