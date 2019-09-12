/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbRateControl_h
#define EbRateControl_h

#include "EbDefinitions.h"
#include "EbSystemResourceManager.h"
#include "EbApi.h"
#include "EbPictureControlSet.h"
#ifdef __cplusplus
extern "C" {
#endif

#define CCOEFF_INIT_FACT              2
#define SAD_CLIP_COEFF                5
// 88 + 3*16*8
#define SLICE_HEADER_BITS_NUM       104   
#define PPS_BITS_NUM                 80
#define SPS_BITS_NUM                296
#define RC_PRECISION                16
#define RC_PRECISION_OFFSET         (1 << (RC_PRECISION - 1))


#if OVERSHOOT_STAT_PRINT
#define CODED_FRAMES_STAT_QUEUE_MAX_DEPTH   10000
#endif

#define ADAPTIVE_PERCENTAGE 1
#define     RC_INTRA_QP_OFFSET              (-1)

#define     RC_DISABLE_FLRC                 0
#define     RC_DISABLE_FLRC_RATE_UPDATE     0
#define     RC_DISABLE_HLRC_RATE_INPUT      1
#define     RC_DISABLE_PRED_QP_LIMIT        1
#define     RC_DISABLE_EXTRA_BITS           0
#define     RC_NEW_EXTRA_BITS               1
#define     RC_UPDATE_TARGET_RATE           1

#define		RC_QPMOD_MAXQP					42


static const EB_U8 MOD_QP_OFFSET_LAYER_ARRAY[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
    { 1 },
    { 5, 6 },
    { 3, 4, 5 },
    { 1, 3, 5, 5 },
    { 1, 3, 5, 5, 6 },
    { 1, 3, 5, 5, 6, 7 }
};
static const EB_U8 QP_OFFSET_LAYER_ARRAY[MAX_HIERARCHICAL_LEVEL][MAX_TEMPORAL_LAYERS] = { // [Highest Temporal Layer] [Temporal Layer Index]
    { 1 },
    { 5, 6 },
    { 3, 4, 5 },
    { 1, 2, 4, 5 },
    { 1, 2, 4, 5, 6 },
    { 1, 2, 4, 5, 6, 7 }
};

static const EB_U32  RATE_PERCENTAGE_LAYER_ARRAY[EB_MAX_TEMPORAL_LAYERS][EB_MAX_TEMPORAL_LAYERS] = {
    {100,  0,  0,  0,  0,  0 },
    { 70, 30,  0,  0,  0,  0 },
    { 70, 15, 15,  0,  0,  0 },
    { 55, 15, 15, 15,  0,  0 },
    { 40, 15, 15, 15, 15,  0 },
    { 30, 10, 15, 15, 15, 15 }
};

// range from 0 to 51
// precision is 16 bits
static const EB_U64 TWO_TO_POWER_QP_OVER_THREE[] = {
         0x10000,      0x1428A,     0x19660,     0x20000,
         0x28514,      0x32CC0,     0x40000,     0x50A29,
         0x65980,      0x80000,     0xA1451,     0xCB2FF,
        0x100000,     0x1428A3,    0x1965FF,    0x200000,
        0x285146,     0x32CBFD,    0x400000,    0x50A28C,
        0x6597FB,     0x800000,    0xA14518,    0xCB2FF5,
       0x1000000,    0x1428A30,   0x1965FEA,   0x2000000,
       0x285145F,    0x32CBFD5,   0x4000000,   0x50A28BE,
       0x6597FA9,    0x8000000,   0xA14517D,   0xCB2FF53,
      0x10000000,   0x1428A2FA,  0x1965FEA5,  0x20000000,
      0x285145F3,   0x32CBFD4A,  0x40000000,  0x50A28BE6,
      0x6597FA95,   0x80000000,  0xA14517CC,  0xCB2FF52A,
     0x100000000,  0x1428A2F99, 0x1965FEA54, 0x200000000
};
// range is from -51 to 51 (0 to 102)
static const EB_U64 TWO_TO_POWER_X_OVER_SIX[] = {
         0xB5,       0xCB,        0xE4,        0xFF,	    0x11F,	    0x142,	
        0x16A,      0x196,       0x1C8,       0x1FF,	    0x23E,	    0x285,
        0x2D4,      0x32C,       0x390,       0x3FF,	    0x47D,	    0x50A,
        0x5A8,      0x659,       0x720,       0x7FF,	    0x8FA,	    0xA14,
        0xB50,      0xCB2,       0xE41,       0xFFF,	   0x11F5,	   0x1428,	
       0x16A0,     0x1965,      0x1C82,      0x1FFF,	   0x23EB,	   0x2851,
       0x2D41,     0x32CB,      0x3904,      0x3FFF,	   0x47D6,	   0x50A2,
       0x5A82,     0x6597,      0x7208,      0x7FFF,	   0x8FAC,	   0xA144,
       0xB504,     0xCB2F,      0xE411,      0xFFFF,	  0x11F58,	  0x14288,
      0x16A08,    0x1965E,     0x1C822,     0x1FFFE,      0x23EB1,    0x28511,
      0x2D410,    0x32CBC,     0x39044,     0x3FFFC,      0x47D62,    0x50A23,
      0x5A821,    0x65979,     0x72088,     0x7FFF8,      0x8FAC4,    0xA1447,
      0xB5043,    0xCB2F2,     0xE4110,     0xFFFF0,     0x11F588,   0x14288E,
     0x16A087,   0x1965E5,    0x1C8221,    0x1FFFE0,     0x23EB11,   0x28511D,
     0x2D410F,   0x32CBCA,    0x390443,    0x3FFFC0,     0x47D623,   0x50A23B,
     0x5A821F,   0x659794,    0x720886,    0x7FFF80,     0x8FAC46,   0xA14476,
     0xB5043E,   0xCB2F29,    0xE4110C,    0xFFFF00,    0x11F588C,  0x14288ED,
     0x16A087C	
};
/**************************************
 * Input Port Types
 **************************************/
typedef enum RATE_CONTROL_INPUT_PORT_TYPES {
    RATE_CONTROL_INPUT_PORT_PICTURE_MANAGER = 0,
    RATE_CONTROL_INPUT_PORT_PACKETIZATION   = 1,
    RATE_CONTROL_INPUT_PORT_ENTROPY_CODING  = 2,
    RATE_CONTROL_INPUT_PORT_TOTAL_COUNT     = 3,
    RATE_CONTROL_INPUT_PORT_INVALID         = ~0,
} RATE_CONTROL_INPUT_PORT_TYPES;

/**************************************
 * Input Port Config
 **************************************/
typedef struct RateControlPorts_s {
    RATE_CONTROL_INPUT_PORT_TYPES    type;
    EB_U32                           count;
} RateControlPorts_t;

/**************************************
 * Coded Frames Stats
 **************************************/
typedef struct CodedFramesStatsEntry_s {
    EB_U64               pictureNumber;    
    EB_S64               frameTotalBitActual;
    EB_BOOL              endOfSequenceFlag;
} CodedFramesStatsEntry_t;  
/**************************************
 * Context
 **************************************/
typedef struct RateControlLayerContext_s 
{
    EB_U64                  previousFrameSadMe; 
    EB_U64                  previousFrameBitActual;
    EB_U64                  previousFrameQuantizedCoeffBitActual;
    EB_BOOL                 feedbackArrived;

    EB_U64                  targetBitRate;
    EB_U64                  frameRate;   
    EB_U64                  channelBitRate; 

    EB_U64                  previousBitConstraint;
    EB_U64                  bitConstraint;
    EB_U64                  ecBitConstraint;
    EB_U64                  previousEcBits;
    EB_S64                  difTotalAndEcBits;
    EB_S64                  prevDifTotalAndEcBits;

    EB_S64                  bitDiff;
    EB_U32                  coeffAveragingWeight1;           
    EB_U32                  coeffAveragingWeight2; // coeffAveragingWeight2 = 16- coeffAveragingWeight1
    //Ccoeffs have 2*RC_PRECISION precision
    EB_S64                  cCoeff;
    EB_S64                  previousCCoeff;   
    //Kcoeffs have RC_PRECISION precision
    EB_U64                  kCoeff;
    EB_U64                  previousKCoeff;
    EB_U64                  coeffWeight;

    //deltaQpFraction has RC_PRECISION precision
    EB_S64                  deltaQpFraction;
    EB_U32                  previousFrameQp;
    EB_U32                  calculatedFrameQp;
    EB_U32                  previousCalculatedFrameQp;
    EB_U32                  areaInPixel;
    EB_U32                  previousFrameAverageQp;

    //totalMad has RC_PRECISION precision
    EB_U64                  totalMad;
    
    EB_U32                  firstFrame;
    EB_U32                  firstNonIntraFrame;
    EB_U32                  sameSADCount;
    EB_U32                  frameSameSADMinQpCount;
    EB_U32                  criticalStates;

    EB_U32                  maxQp;
    EB_U32                  temporalIndex;
    
    EB_U64                  alpha;
    
} RateControlLayerContext_t;

typedef struct RateControlIntervalParamContext_s
{
    EB_U64                       firstPoc;   
    EB_U64                       lastPoc;   
    EB_BOOL                      inUse;   
    EB_BOOL                      wasUsed;  
    EB_U64                       processedFramesNumber; 
    EB_BOOL                      lastGop;
    RateControlLayerContext_t  **rateControlLayerArray;

    EB_S64                       virtualBufferLevel;
    EB_S64                       previousVirtualBufferLevel;
    EB_U32                       intraFramesQp;

    EB_U32                       nextGopIntraFrameQp;
    EB_S64                       totalExtraBits;
    EB_U64                       firstPicPredBits;
    EB_U64                       firstPicActualBits;
    EB_U16                       firstPicPredQp;
    EB_U16                       firstPicActualQp;
    EB_BOOL                       firstPicActualQpAssigned;
    EB_BOOL                      sceneChangeInGop;
    EB_BOOL                      minTargetRateAssigned;
    EB_S64                       extraApBitRatioI;
  
} RateControlIntervalParamContext_t;

typedef struct HighLevelRateControlContext_s
{     

    EB_U64                       targetBitsPerSlidingWindow;
    EB_U64                       targetBitRate;
    EB_U64                       frameRate;   
    EB_U64                       channelBitRatePerFrame; 
    EB_U64                       channelBitRatePerSw; 
    EB_U64                       bitConstraintPerSw;
    EB_U64                       predBitsRefQpPerSw[MAX_REF_QP_NUM];
#if RC_UPDATE_TARGET_RATE
    EB_U32                       prevIntraSelectedRefQp;
    EB_U32                       prevIntraOrgSelectedRefQp;
    EB_U64                       previousUpdatedBitConstraintPerSw;
#endif
    

} HighLevelRateControlContext_t;

typedef struct RateControlContext_s
{
    EbFifo_t                    *rateControlInputTasksFifoPtr;    
    EbFifo_t                    *rateControlOutputResultsFifoPtr;

    HighLevelRateControlContext_t *highLevelRateControlPtr;

    RateControlIntervalParamContext_t **rateControlParamQueue;
    EB_U64                       rateControlParamQueueHeadIndex;   

    EB_U64                       frameRate;   

    EB_U64                       virtualBufferSize; 

    EB_S64                       virtualBufferLevelInitialValue;
    EB_S64                       previousVirtualBufferLevel;

    EB_S64                       virtualBufferLevel;

   //Virtual Buffer Thresholds
    EB_S64                       vbFillThreshold1;
    EB_S64                       vbFillThreshold2;

    // Rate Control Previous Bits Queue
#if OVERSHOOT_STAT_PRINT
    CodedFramesStatsEntry_t    **codedFramesStatQueue;
    EB_U32                       codedFramesStatQueueHeadIndex;
    EB_U32                       codedFramesStatQueueTailIndex;

    EB_U64                       totalBitActualPerSw;
    EB_U64                       maxBitActualPerSw;
    EB_U64                       maxBitActualPerGop;

#endif


    EB_U64                       rateAveragePeriodinFrames;
    EB_U32                       baseLayerFramesAvgQp;
    EB_U32                       baseLayerIntraFramesAvgQp;

    EB_U32                       baseLayerIntraFramesAvgQpFloat;
    EB_BOOL                      endOfSequenceRegion;

    EB_U32                       intraCoefRate;
    EB_U32                       nonPeriodicIntraCoefRate;   

    EB_U64                       framesInInterval [EB_MAX_TEMPORAL_LAYERS];
    EB_S64                       extraBits;  
    EB_S64                       extraBitsGen;
    EB_S16                      maxRateAdjustDeltaQP;
   
   
} RateControlContext_t;



/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE RateControlLayerContextCtor(
    RateControlLayerContext_t   **entryDblPtr);

  

extern EB_ERRORTYPE RateControlIntervalParamContextCtor(
    RateControlIntervalParamContext_t   **entryDblPtr);



extern EB_ERRORTYPE RateControlCodedFramesStatsContextCtor(
    CodedFramesStatsEntry_t   **entryDblPtr,
    EB_U64                      pictureNumber);



extern EB_ERRORTYPE RateControlContextCtor(
    RateControlContext_t   **contextDblPtr,
    EbFifo_t                *rateControlInputTasksFifoPtr,
    EbFifo_t                *rateControlOutputResultsFifoPtr,
    EB_S32                   intraPeriodLength);
    
   
    
extern void* RateControlKernel(void *inputPtr);

#ifdef __cplusplus
}
#endif
#endif // EbRateControl_h