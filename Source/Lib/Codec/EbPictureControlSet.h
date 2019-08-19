/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbPictureControlSet_h
#define EbPictureControlSet_h

#include <time.h>

#include "EbApi.h"

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbPictureBufferDesc.h"
#include "EbCodingUnit.h"
#include "EbEntropyCodingObject.h"
#include "EbDefinitions.h"
#include "EbPredictionStructure.h"
#include "EbNeighborArrays.h"
#include "EbModeDecisionSegments.h"
#include "EbEncDecSegments.h"
#include "EbRateControlTables.h"
#ifdef __cplusplus
extern "C" {
#endif

#define SEGMENT_ENTROPY_BUFFER_SIZE         0x989680// Entropy Bitstream Buffer Size
#define PACKETIZATION_PROCESS_BUFFER_SIZE   0x001000 // Bitstream used to code SPS, PPS, etc.
#define EOS_NAL_BUFFER_SIZE                 0x0010 // Bitstream used to code EOS NAL
#define HISTOGRAM_NUMBER_OF_BINS            256  
#define MAX_NUMBER_OF_REGIONS_IN_WIDTH		4
#define MAX_NUMBER_OF_REGIONS_IN_HEIGHT		4

#define MAX_REF_QP_NUM                      52

// Segment Macros
#define SEGMENT_MAX_COUNT   64
#define SEGMENT_COMPLETION_MASK_SET(mask, index)        MULTI_LINE_MACRO_BEGIN (mask) |= (((EB_U64) 1) << (index)); MULTI_LINE_MACRO_END
#define SEGMENT_COMPLETION_MASK_TEST(mask, totalCount)  ((mask) == ((((EB_U64) 1) << (totalCount)) - 1))
#define SEGMENT_ROW_COMPLETION_TEST(mask, rowIndex, width) ((((mask) >> ((rowIndex) * (width))) & ((1ull << (width))-1)) == ((1ull << (width))-1)) 
#define SEGMENT_CONVERT_IDX_TO_XY(index, x, y, picWidthInLcu) \
                                                        MULTI_LINE_MACRO_BEGIN \
                                                            (y) = (index) / (picWidthInLcu); \
                                                            (x) = (index) - (y) * (picWidthInLcu); \
                                                        MULTI_LINE_MACRO_END
#define SEGMENT_START_IDX(index, picSizeInLcu, numOfSeg) (((index) * (picSizeInLcu)) / (numOfSeg))
#define SEGMENT_END_IDX(index, picSizeInLcu, numOfSeg)   ((((index)+1) * (picSizeInLcu)) / (numOfSeg))


// BDP OFF
#define MD_NEIGHBOR_ARRAY_INDEX                0
// BDP ON
#define PILLAR_NEIGHBOR_ARRAY_INDEX            1
#define REFINEMENT_NEIGHBOR_ARRAY_INDEX        2
#define MV_MERGE_PASS_NEIGHBOR_ARRAY_INDEX     3

#define NEIGHBOR_ARRAY_TOTAL_COUNT             4



struct PredictionUnit_s;

#define MAX_NUMBER_OF_SLICES_PER_PICTURE        16
typedef struct InLoopFilterSliceData_s 
{
    EB_U8           betaOffset;
    EB_U8           tcOffset;
    EB_BOOL         pcmLoopFilterEnable;
    EB_BOOL         sliceLoopFilterEnable;
    EB_S8           cbQpOffset;
    EB_S8           crQpOffset;
    EB_BOOL         saoLumaEnable;
    EB_BOOL         saoChromaEnable;

} InLoopFilterSliceData_t;

/**************************************
 * Segment-based Control Sets
 **************************************/

typedef struct EbMdcLeafData_s 
{
    EB_U8           leafIndex;
    EB_BOOL         splitFlag;
} EbMdcLeafData_t; 


typedef struct MdcLcuData_s
{
    // Rate Control
    EB_U8           qp;
    
    // ME Results
    EB_U64          treeblockVariance;

    // MDCP Results
    EB_U8           firstValidLeaf;
    EB_U8           leafCount;
    EbMdcLeafData_t leafDataArray[CU_MAX_COUNT];

} MdcLcuData_t;

typedef struct BdpCuData_s
{
    EB_U8   leafCount;
    EB_U8   leafDataArray[CU_MAX_COUNT];

} BdpCuData_t;

/**************************************
 * MD Segment Control
 **************************************/
typedef struct MdSegmentCtrl_s 
{
    EB_U64                                completionMask;
    EB_HANDLE                             writeLockMutex;
    
    EB_U32                                totalCount;
    EB_U32                                columnCount;
    EB_U32                                rowCount;
    
    EB_BOOL                               inProgress;
    EB_U32                                currentRowIdx;

} MdSegmentCtrl_t;

/**************************************
 * Picture Control Set
 **************************************/
struct CodedTreeblock_s;
struct LargestCodingUnit_s;

typedef struct EntropyTileInfo_s
{
    EB_S8                                 entropyCodingCurrentAvailableRow;
    EB_BOOL                               entropyCodingRowArray[MAX_LCU_ROWS];
    EB_S8                                 entropyCodingCurrentRow;
    EB_S8                                 entropyCodingRowCount;
    EB_HANDLE                             entropyCodingMutex;
    EB_BOOL                               entropyCodingInProgress;
	EB_BOOL                               entropyCodingPicDone;
    EntropyCoder_t                       *entropyCoderPtr;
} EntropyTileInfo;

typedef struct PictureControlSet_s 
{
    EbObjectWrapper_t                    *sequenceControlSetWrapperPtr;
    EbPictureBufferDesc_t                *reconPicturePtr;
  
    EbPictureBufferDesc_t                *reconPicture16bitPtr;
    
    // Packetization (used to encode SPS, PPS, etc)
    Bitstream_t                          *bitstreamPtr;
    
    // Reference Lists
    EbObjectWrapper_t                    *refPicPtrArray[MAX_NUM_OF_REF_PIC_LIST];

    EB_U8                                 refPicQpArray[MAX_NUM_OF_REF_PIC_LIST];
    EB_PICTURE                            refSliceTypeArray[MAX_NUM_OF_REF_PIC_LIST];    
    
    // GOP
    EB_U64                                pictureNumber;        
    EB_U8                                 temporalLayerIndex;
    EB_U8                                 temporalId;

    EB_COLOR_FORMAT                       colorFormat;

    EncDecSegments_t                     **encDecSegmentCtrl;


    EntropyTileInfo                       **entropyCodingInfo;
    EB_HANDLE                             entropyCodingPicMutex;
    EB_BOOL                               entropyCodingPicResetFlag;

    EB_HANDLE                             intraMutex;
    EB_U32                                intraCodedArea;
    EB_U32                                encDecCodedLcuCount;

    // Mode Decision Config
    MdcLcuData_t                         *mdcLcuArray;

    // Error Resilience
    EB_BOOL                               constrainedIntraFlag;

    // Slice Type
    EB_PICTURE                            sliceType;
    
    // Rate Control
    EB_U8                                 pictureQp;
    EB_U8                                 difCuDeltaQpDepth;
    EB_U8                                 useDeltaQp;
    
    // LCU Array
    EB_U8                                 lcuMaxDepth;
    EB_U16                                lcuTotalCount;
    LargestCodingUnit_t                 **lcuPtrArray;

    // DLF
    EB_U8                               **verticalEdgeBSArray;
    EB_U8                               **horizontalEdgeBSArray;
    EB_U8                                *qpArray;
    EB_U8                                *entropyQpArray;
    EB_U16                                qpArrayStride;
    EB_U32                                qpArraySize;
    EB_U8                                *cbfMapArray;

    // QP Assignment
    EB_U8                                 prevCodedQp[EB_TILE_MAX_COUNT];
    EB_U8                                 prevQuantGroupCodedQp[EB_TILE_MAX_COUNT];

    // Enc/DecQP Assignment
    EB_U8                                 encPrevCodedQp[EB_TILE_MAX_COUNT][MAX_PICTURE_HEIGHT_SIZE / MAX_LCU_SIZE];
    EB_U8                                 encPrevQuantGroupCodedQp[EB_TILE_MAX_COUNT][MAX_PICTURE_HEIGHT_SIZE / MAX_LCU_SIZE];
                                                                              
    // EncDec Entropy Coder (for rate estimation)           
    EntropyCoder_t                       *coeffEstEntropyCoderPtr;
	CabacCost_t                          *cabacCost;

    // Mode Decision Neighbor Arrays
    NeighborArrayUnit_t                  **mdIntraLumaModeNeighborArray[NEIGHBOR_ARRAY_TOTAL_COUNT];
    NeighborArrayUnit_t                  **mdMvNeighborArray[NEIGHBOR_ARRAY_TOTAL_COUNT];
    NeighborArrayUnit_t                  **mdSkipFlagNeighborArray[NEIGHBOR_ARRAY_TOTAL_COUNT];
    NeighborArrayUnit_t                  **mdModeTypeNeighborArray[NEIGHBOR_ARRAY_TOTAL_COUNT];
    NeighborArrayUnit_t                  **mdLeafDepthNeighborArray[NEIGHBOR_ARRAY_TOTAL_COUNT];
	NeighborArrayUnit_t                  **mdLumaReconNeighborArray[NEIGHBOR_ARRAY_TOTAL_COUNT];
	NeighborArrayUnit_t                  **mdCbReconNeighborArray[NEIGHBOR_ARRAY_TOTAL_COUNT];
	NeighborArrayUnit_t                  **mdCrReconNeighborArray[NEIGHBOR_ARRAY_TOTAL_COUNT];

	// Mode Decision Refinement Neighbor Arrays
	NeighborArrayUnit_t                  **mdRefinementIntraLumaModeNeighborArray;
	NeighborArrayUnit_t                  **mdRefinementModeTypeNeighborArray;
	NeighborArrayUnit_t                  **mdRefinementLumaReconNeighborArray;

    // Encode Pass Neighbor Arrays      
    NeighborArrayUnit_t                  **epIntraLumaModeNeighborArray;
    NeighborArrayUnit_t                  **epMvNeighborArray;
    NeighborArrayUnit_t                  **epSkipFlagNeighborArray;
    NeighborArrayUnit_t                  **epModeTypeNeighborArray;
    NeighborArrayUnit_t                  **epLeafDepthNeighborArray;
    NeighborArrayUnit_t                  **epLumaReconNeighborArray;
    NeighborArrayUnit_t                  **epCbReconNeighborArray;
    NeighborArrayUnit_t                  **epCrReconNeighborArray;
    NeighborArrayUnit_t                  **epSaoNeighborArray;
    NeighborArrayUnit_t                  **epLumaReconNeighborArray16bit;
    NeighborArrayUnit_t                  **epCbReconNeighborArray16bit;
    NeighborArrayUnit_t                  **epCrReconNeighborArray16bit;

    // AMVP & MV Merge Neighbor Arrays 
    //NeighborArrayUnit_t                  *amvpMvMergeMvNeighborArray;
    //NeighborArrayUnit_t                  *amvpMvMergeModeTypeNeighborArray;

    // Entropy Coding Neighbor Arrays
    NeighborArrayUnit_t                  **modeTypeNeighborArray;
    NeighborArrayUnit_t                  **leafDepthNeighborArray;
    NeighborArrayUnit_t                  **intraLumaModeNeighborArray;
    NeighborArrayUnit_t                  **skipFlagNeighborArray;

    EB_REFLIST                            colocatedPuRefList;
    EB_BOOL                               isLowDelay;

    // SAO
    EB_BOOL                               saoFlag[2];

    EB_BOOL                               sliceLevelChromaQpFlag;

	// slice level chroma QP offsets
	EB_S8 								  sliceCbQpOffset;
	EB_S8 								  sliceCrQpOffset;

	EB_S8                                 cbQpOffset;
	EB_S8                                 crQpOffset;

	struct PictureParentControlSet_s     *ParentPcsPtr;//The parent of this PCS.
	EbObjectWrapper_t                    *PictureParentControlSetWrapperPtr;
	EB_S8                                 betaOffset;
	EB_S8                                 tcOffset;

	EB_U8                                 highIntraSlection;

	EB_FRAME_CARACTERICTICS               sceneCaracteristicId;
	EB_BOOL                               adjustMinQPFlag;
    EB_ENC_MODE                           encMode;

    EB_BOOL                               bdpPresentFlag;
    EB_BOOL                               mdPresentFlag;

} PictureControlSet_t;




// To optimize based on the max input size
// To study speed-memory trade-offs
typedef struct LcuParameters_s {

	EB_U8   horizontalIndex;
	EB_U8   verticalIndex;
	EB_U16  originX;
	EB_U16  originY;
	EB_U8   width;
	EB_U8   height;
	EB_U8   isCompleteLcu;
	EB_BOOL rasterScanCuValidity[CU_MAX_COUNT];
    EB_U8   potentialLogoLcu;
	EB_U8   isEdgeLcu;
} LcuParams_t;

typedef struct CuStat_s {
	EB_BOOL			grassArea;
	EB_BOOL			skinArea;

	EB_BOOL			highChroma;
	EB_BOOL			highLuma;

    EB_U16          edgeCu; 
    EB_U16          similarEdgeCount;
    EB_U16          pmSimilarEdgeCount;
} CuStat_t;

typedef struct LcuStat_s {

	CuStat_t		cuStatArray[CU_MAX_COUNT];
    EB_U8           stationaryEdgeOverTimeFlag;

    EB_U8           pmStationaryEdgeOverTimeFlag;
    EB_U8           pmCheck1ForLogoStationaryEdgeOverTimeFlag;

    EB_U8           check1ForLogoStationaryEdgeOverTimeFlag;
    EB_U8           check2ForLogoStationaryEdgeOverTimeFlag;
    EB_U8           lowDistLogo;

} LcuStat_t;
                    
//CHKN
// Add the concept of PictureParentControlSet which is a subset of the old PictureControlSet.
// It actually holds only high level Pciture based control data:(GOP management,when to start a picture, when to release the PCS, ....).
// The regular PictureControlSet(Child) will be dedicated to store LCU based encoding results and information.
// Parent is created before the Child, and continue to live more. Child PCS only lives the exact time needed to encode the picture: from ME to EC/ALF.
typedef struct PictureParentControlSet_s 
{
    EbObjectWrapper_t                    *sequenceControlSetWrapperPtr;   
    EbObjectWrapper_t                    *inputPictureWrapperPtr;
    EbObjectWrapper_t                    *referencePictureWrapperPtr;
    EbObjectWrapper_t                    *paReferencePictureWrapperPtr;

    EB_BOOL                               idrFlag;
    EB_BOOL                               craFlag;
    EB_BOOL                               openGopCraFlag;
    EB_BOOL                               sceneChangeFlag;   
    EB_BOOL                               endOfSequenceFlag;   

    EB_U8                                 pictureQp;                                 
    EB_U64                                pictureNumber;                                

    EbPictureBufferDesc_t                *enhancedPicturePtr; 
    EbPictureBufferDesc_t                *chromaDownSamplePicturePtr;

    EB_PICNOISE_CLASS                     picNoiseClass;

    EB_BUFFERHEADERTYPE                  *ebInputPtr;

    EbObjectWrapper_t                    *ebInputWrapperPtr;

    EB_PICTURE                              sliceType;                                 
    NalUnitType                           nalUnit;
    EB_U8                                 predStructIndex;  
    EB_BOOL                               useRpsInSps;   
    EB_U8                                 referenceStructIndex;                      
    EB_U8                                 temporalLayerIndex;                       
    EB_U64                                decodeOrder;        
    EB_BOOL                               isUsedAsReferenceFlag;
    EB_U8                                 refList0Count;                             
    EB_U8                                 refList1Count;   
    PredictionStructure_t                *predStructPtr;          // need to check
    struct PictureParentControlSet_s     *refPaPcsArray[MAX_NUM_OF_REF_PIC_LIST];
    EbObjectWrapper_t                    *pPcsWrapperPtr;

    // Tiles
    EB_U32                                tileUniformSpacing;
    EB_U16                                tileColumnCount;
    EB_U16                                tileRowCount;
    EB_U16                                tileRowStartLcu[EB_TILE_ROW_MAX_COUNT + 1]; //plus one to calculate the width/height of last 
    EB_U16                                tileColStartLcu[EB_TILE_COLUMN_MAX_COUNT + 1];
    EB_U16                                pictureWidthInLcu;
    EB_U16                                pictureHeightInLcu;

    TileInfo_t                           *tileInfoArray; //Tile info in raster scan order
    LcuEdgeInfo_t                        *lcuEdgeInfoArray; //LCU tile/picture edge info


    // Rate Control
    EB_U64                                predBitsRefQp[MAX_REF_QP_NUM];
    EB_U64                                targetBitsBestPredQp;
    EB_U64                                targetBitsRc;
    EB_U8                                 bestPredQp;
    EB_U64                                totalNumBits;
    EB_U8                                 firstFrameInTemporalLayer;
    EB_U8                                 firstNonIntraFrameInTemporalLayer;
    EB_U64                                framesInInterval  [EB_MAX_TEMPORAL_LAYERS];
    EB_U64                                bitsPerSwPerLayer[EB_MAX_TEMPORAL_LAYERS];
    EB_U64                                totalBitsPerGop;
    EB_BOOL                               tablesUpdated;
    EB_BOOL                               percentageUpdated;
    EB_U32                                targetBitRate;
    EB_BOOL                               minTargetRateAssigned;
    EB_U32                                frameRate;
    EB_BOOL                               frameRateIsUpdated;
    EB_U32                                droppedFramesNumber;
    EB_U16                                lcuTotalCount;
    EB_BOOL                               endOfSequenceRegion;
    EB_BOOL                               sceneChangeInGop;
    // used for Look ahead
    EB_U8                                 framesInSw;
    EB_S16                                historgramLifeCount;

    EB_BOOL                               qpOnTheFly;

	EB_U8                                 calculatedQp;
    EB_U8                                 intraSelectedOrgQp;
    EB_U64                                sadMe;


    EB_U64                                quantizedCoeffNumBits;
    EB_U64                                targetBits;
    EB_U64                                averageQp;
 
    EB_U64                                lastIdrPicture;

    EB_U64                                startTimeSeconds;
    EB_U64                                startTimeuSeconds;
    EB_U32                                lumaSse;   
    EB_U32                                crSse;   
    EB_U32                                cbSse;   
        

    // PA
    EB_U32                                preAssignmentBufferCount;

	EbObjectWrapper_t                    *refPaPicPtrArray[MAX_NUM_OF_REF_PIC_LIST];
    EB_U64                                refPicPocArray[MAX_NUM_OF_REF_PIC_LIST];
    EB_U16                              **variance;
                                        
	EB_U8                               **yMean;
	EB_U8                               **cbMean;
	EB_U8                               **crMean;

	EB_U16                                picAvgVariance;

    // Histograms
	EB_U32                            ****pictureHistogram;

	EB_U64								  averageIntensityPerRegion[MAX_NUMBER_OF_REGIONS_IN_WIDTH][MAX_NUMBER_OF_REGIONS_IN_HEIGHT][3];
    
    // Segments
    EB_U16                                meSegmentsTotalCount;
    EB_U8                                 meSegmentsColumnCount;
    EB_U8                                 meSegmentsRowCount;
    EB_U64                                meSegmentsCompletionMask;

    // Motion Estimation Results
    EB_U8                                 maxNumberOfPusPerLcu;
    EB_U8                                 maxNumberOfMeCandidatesPerPU;


	MeCuResults_t						**meResults;
	EB_U32								 *rcMEdistortion;

    // Motion Estimation Distortion and OIS Historgram 
    EB_U16                               *meDistortionHistogram;
    EB_U16                               *oisDistortionHistogram;
                                       
    EB_U32                               *intraSadIntervalIndex;
    EB_U32                               *interSadIntervalIndex;

    EB_HANDLE                             rcDistortionHistogramMutex;

    EB_U16                                fullLcuCount;

    // Open loop Intra candidate Search Results

	OisCu32Cu16Results_t                **oisCu32Cu16Results;
	OisCu8Results_t                     **oisCu8Results;

    // Dynamic GOP
    EB_PRED                               predStructure;
    EB_U8                                 hierarchicalLevels;
    EB_BOOL                               initPredStructPositionFlag;
    EB_S8                                 hierarchicalLayersDiff;


    // Interlaced Video
    EB_PICT_STRUCT                        pictStruct;

	// Average intensity
    EB_U8                                 averageIntensity[3];

    EbObjectWrapper_t                    *outputStreamWrapperPtr;
	EB_BOOL							      disableTmvpFlag;
	// zz cost array
	EB_U8                                *zzCostArray; 
	// Non moving index array
	EB_U8                                *nonMovingIndexArray; 

	EB_BOOL								  isPan;
	EB_BOOL								  isTilt;

    EB_BOOL                              *similarColocatedLcuArray;
    EB_BOOL                              *similarColocatedLcuArrayAllLayers;
	EB_U8                                *lcuFlatNoiseArray;
	EB_U64                               *lcuVarianceOfVarianceOverTime;
	EB_BOOL							     *isLcuHomogeneousOverTime;
    EB_U32							      regionActivityCost[MAX_NUMBER_OF_REGIONS_IN_WIDTH][MAX_NUMBER_OF_REGIONS_IN_HEIGHT];
    // 5L or 6L prediction error compared to 4L prediction structure 
    // 5L: computed for base layer frames (16 -  8 on top of 16 - 0)
    // 6L: computed for base layer frames (32 - 24 on top of 32 - 0 & 16 - 8 on top of 16 - 0)
    EB_U8                                 picHomogenousOverTimeLcuPercentage;
    
    // To further optimize
    EdgeLcuResults_t                     *edgeResultsPtr;				                // used by EncDecProcess()

    EB_U8							     *sharpEdgeLcuFlag;
    EB_U8							     *failingMotionLcuFlag;		                // used by EncDecProcess()  and ModeDecisionConfigurationProcess // USED for L2 to replace the uncovered detectors for L6 and L7
    EB_BOOL							     *uncoveredAreaLcuFlag;		                // used by EncDecProcess() 
    EB_BOOL                              *lcuHomogeneousAreaArray;		                // used by EncDecProcess()  
    EB_BOOL                               logoPicFlag;				                    // used by EncDecProcess()  
    EB_U64                              **varOfVar32x32BasedLcuArray;	                // used by ModeDecisionConfigurationProcess()- the variance of 8x8 block variances for each 32x32 block	 	 
    EB_BOOL                              *lcuCmplxContrastArray;			            // used by EncDecProcess()
                                       
                                       
    EB_U64                              **lcuYSrcEnergyCuArray;			            // used by ModeDecisionConfigurationProcess()	 0- 64x64, 1-4 32x32
    EB_U64							    **lcuYSrcMeanCuArray;			                // used by ModeDecisionConfigurationProcess()	 0- 64x64, 1-4 32x32
    EB_U8                                 intraCodedBlockProbability;	                // used by EncDecProcess()	
    EB_BOOL                               lowMotionContentFlag;			            // used by EncDecProcess()
    EB_U32                                zzCostAverage;					            // used by ModeDecisionConfigurationProcess()	
    EB_U16                                nonMovingIndexAverage;			            // used by ModeDecisionConfigurationProcess()	
    EB_BOOL                              *lcuIsolatedNonHomogeneousAreaArray;			// used by ModeDecisionConfigurationProcess()	
    EB_U8                                 grassPercentageInPicture;
    EB_U8 			                      percentageOfEdgeinLightBackground;
    EB_BOOL			                      darkBackGroundlightForeGround;
    EbObjectWrapper_t                    *previousPictureControlSetWrapperPtr;
    LcuStat_t                            *lcuStatArray;
    EB_U8                                 veryLowVarPicFlag;
	EB_BOOL		                          highDarkAreaDensityFlag;		                // computed @ PictureAnalysisProcess() and used @ SourceBasedOperationsProcess()
    EB_BOOL		                          highDarkLowLightAreaDensityFlag;		        // computed @ PictureAnalysisProcess() and used @ SourceBasedOperationsProcess()
    EB_U8                                 blackAreaPercentage;
	EB_U32							      intraComplexityMin[4];
	EB_U32                                intraComplexityMax[4];
	EB_U32                                intraComplexityAccum[4];
	EB_U32                                intraComplexityAvg[4];
	EB_U32							      interComplexityMin[4];
	EB_U32                                interComplexityMax[4];
	EB_U32                                interComplexityAccum[4];
	EB_U32                                interComplexityAvg[4];
	EB_U32                                processedleafCount[4];
                                       
	EB_U32							      intraComplexityMinPre;
	EB_U32                                intraComplexityMaxPre;
	EB_U32                                interComplexityMinPre;
	EB_U32                                interComplexityMaxPre;
                                       
	EB_S32							      intraMinDistance[4];
	EB_S32							      intraMaxDistance[4];
	EB_S32							      interMinDistance[4];
	EB_S32							      interMaxDistance[4];
	EB_U8                                 lcuBlockPercentage;
                                       
    EB_LCU_DEPTH_MODE                    *lcuMdModeArray;
    EB_LCU_COMPLEXITY_STATUS             *complexLcuArray;
    EB_BOOL                               useSrcRef;
    EB_U8                                *cmplxStatusLcu;			// used by EncDecProcess()  
                                       
    // Encoder Mode                    
    EB_ENC_MODE                           encMode;
                                       
    // Multi-modes signal(s)           
    EB_PICTURE_DEPTH_MODE                 depthMode;
    EB_BOOL                               limitOisToDcModeFlag;
    EB_BOOL                               useSubpelFlag;
    EB_CU_8x8_MODE                        cu8x8Mode;
	EB_CU_16x16_MODE                      cu16x16Mode;
    EB_BOOL                               skipOis8x8;
    EB_NOISE_DETECT_MODE                  noiseDetectionMethod;
    EB_U8                                 noiseDetectionTh;
	EB_BOOL                               enableDenoiseSrcFlag;
    EB_BOOL                               enableHmeFlag;
    EB_BOOL                               enableHmeLevel0Flag;
    EB_BOOL                               enableHmeLevel1Flag;
    EB_BOOL                               enableHmeLevel2Flag;
	EB_BOOL                               disableVarianceFlag;
} PictureParentControlSet_t;


typedef struct PictureControlSetInitData_s
{
	EB_U16                           pictureWidth;
	EB_U16                           pictureHeight;
	EB_U16                           leftPadding;
	EB_U16                           rightPadding;
	EB_U16                           topPadding;
	EB_U16                           botPadding;
    EB_BITDEPTH                      bitDepth;
    EB_COLOR_FORMAT                  colorFormat;
    EB_U32                           lcuSize;
    EB_U32                           maxDepth;
    EB_BOOL                          is16bit;
	EB_U32                           compressedTenBitFormat;
    EB_U16                           encDecSegmentCol;
    EB_U16                           encDecSegmentRow;

	EB_ENC_MODE                      encMode;

	EB_U8                            speedControl;

//    EB_U8                            tune;

    EB_U16                           tileRowCount;
    EB_U16                           tileColumnCount;
} PictureControlSetInitData_t;

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE PictureControlSetCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);

extern EB_ERRORTYPE PictureParentControlSetCtor(
    EB_PTR *objectDblPtr, 
    EB_PTR objectInitDataPtr);

#ifdef __cplusplus
}
#endif
#endif // EbPictureControlSet_h
