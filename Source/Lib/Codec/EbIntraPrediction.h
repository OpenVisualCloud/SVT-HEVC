/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbIntraPrediction_h
#define EbIntraPrediction_h

#include "EbIntraPrediction_C.h"
#include "EbIntraPrediction_SSE2.h"
#include "EbIntraPrediction_SSSE3.h"
#include "EbIntraPrediction_SSE4_1.h"
#include "EbIntraPrediction_AVX2.h"

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureBufferDesc.h"
#include "EbPictureControlSet.h"
#include "EbCodingUnit.h"
#include "EbPictureControlSet.h"
#include "EbModeDecision.h"
#include "EbNeighborArrays.h"
#include "EbMotionEstimationProcess.h"
#include "EbObject.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MAX_PU_SIZE                            64 

struct ModeDecisionContext_s;

typedef struct IntraReferenceSamples_s {
    EbDctor                 dctor;
    EB_U8                  *yIntraReferenceArray;
    EB_U8                  *cbIntraReferenceArray;
    EB_U8                  *crIntraReferenceArray;
    EB_U8                  *yIntraFilteredReferenceArray;
    EB_U8                  *cbIntraFilteredReferenceArray;
    EB_U8                  *crIntraFilteredReferenceArray;

    EB_U8                  *yIntraReferenceArrayReverse;        
    EB_U8                  *yIntraFilteredReferenceArrayReverse;
    EB_U8                  *cbIntraReferenceArrayReverse;
    EB_U8                  *cbIntraFilteredReferenceArrayReverse;
    EB_U8                  *crIntraReferenceArrayReverse;
    EB_U8                  *crIntraFilteredReferenceArrayReverse;

    // Scratch buffers used in the interpolaiton process
    EB_U8                   ReferenceAboveLineY[(MAX_PU_SIZE<<2)+1];
    EB_U8                   ReferenceLeftLineY[(MAX_PU_SIZE<<2)+1];
    EB_BOOL                 AboveReadyFlagY;
    EB_BOOL                 LeftReadyFlagY;

    EB_U8                   ReferenceAboveLineCb[(MAX_PU_SIZE<<2)+2];
    EB_U8                   ReferenceLeftLineCb[(MAX_PU_SIZE<<2)+2];
    EB_BOOL                 AboveReadyFlagCb;
    EB_BOOL                 LeftReadyFlagCb;

    EB_U8                   ReferenceAboveLineCr[(MAX_PU_SIZE<<2)+2];
    EB_U8                   ReferenceLeftLineCr[(MAX_PU_SIZE<<2)+2];
    EB_BOOL                 AboveReadyFlagCr;
    EB_BOOL                 LeftReadyFlagCr;

} IntraReferenceSamples_t;

typedef struct IntraReference16bitSamples_s {
    EbDctor                  dctor;
    EB_U16                  *yIntraReferenceArray;
    EB_U16                  *cbIntraReferenceArray;
    EB_U16                  *crIntraReferenceArray;
    EB_U16                  *yIntraFilteredReferenceArray;
    EB_U16                  *cbIntraFilteredReferenceArray;
    EB_U16                  *crIntraFilteredReferenceArray;

    EB_U16                  *yIntraReferenceArrayReverse;        
    EB_U16                  *yIntraFilteredReferenceArrayReverse;
    EB_U16                  *cbIntraReferenceArrayReverse;
    EB_U16                  *cbIntraFilteredReferenceArrayReverse;
    EB_U16                  *crIntraReferenceArrayReverse;
    EB_U16                  *crIntraFilteredReferenceArrayReverse;

    // Scratch buffers used in the interpolaiton process
    EB_U16                   ReferenceAboveLineY[(MAX_PU_SIZE<<2)+1];
    EB_U16                   ReferenceLeftLineY[(MAX_PU_SIZE<<2)+1];
    EB_BOOL                  AboveReadyFlagY;
    EB_BOOL                  LeftReadyFlagY;

    EB_U16                   ReferenceAboveLineCb[(MAX_PU_SIZE<<2)+2];
    EB_U16                   ReferenceLeftLineCb[(MAX_PU_SIZE<<2)+2];
    EB_BOOL                  AboveReadyFlagCb;
    EB_BOOL                  LeftReadyFlagCb;

    EB_U16                   ReferenceAboveLineCr[(MAX_PU_SIZE<<2)+2];
    EB_U16                   ReferenceLeftLineCr[(MAX_PU_SIZE<<2)+2];
    EB_BOOL                  AboveReadyFlagCr;
    EB_BOOL                  LeftReadyFlagCr;

} IntraReference16bitSamples_t;

extern EB_ERRORTYPE IntraReferenceSamplesCtor(
        IntraReferenceSamples_t *contextPtr,
        EB_COLOR_FORMAT colorFormat);

extern EB_ERRORTYPE IntraReference16bitSamplesCtor(
    IntraReference16bitSamples_t *contextPtr,
    EB_COLOR_FORMAT colorFormat);



#define TOTAL_LUMA_MODES                   35
#define TOTAL_CHROMA_MODES                  5
#define TOTAL_INTRA_GROUPS                  5
#define INTRA_PLANAR_MODE                   0
#define INTRA_DC_MODE                       1
#define INTRA_HORIZONTAL_MODE              10
#define INTRA_VERTICAL_MODE                26
#define STRONG_INTRA_SMOOTHING_BLOCKSIZE   32
#define SMOOTHING_THRESHOLD                 8
#define SMOOTHING_THRESHOLD_10BIT          32

extern EB_ERRORTYPE GenerateIntraReferenceSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_COLOR_FORMAT             colorFormat,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary);

extern EB_ERRORTYPE GenerateLumaIntraReferenceSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary);


extern EB_ERRORTYPE GenerateChromaIntraReferenceSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_COLOR_FORMAT             colorFormat,
    EB_BOOL                     secondChroma,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary);


extern EB_ERRORTYPE GenerateIntraReference16bitSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_COLOR_FORMAT             colorFormat,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary);


extern EB_ERRORTYPE GenerateLumaIntraReference16bitSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary);


extern EB_ERRORTYPE GenerateChromaIntraReference16bitSamplesEncodePass(
    EB_BOOL                     constrainedIntraFlag,   //input parameter, indicates if constrained intra is switched on/off
    EB_BOOL                     strongIntraSmoothingFlag,
    EB_U32                      originX,
    EB_U32                      originY,
    EB_U32                      size,
    EB_U32                      lcuSize,
    EB_U32                      cuDepth,
    NeighborArrayUnit_t        *modeTypeNeighborArray,
    NeighborArrayUnit_t        *lumaReconNeighborArray,
    NeighborArrayUnit_t        *cbReconNeighborArray,
    NeighborArrayUnit_t        *crReconNeighborArray,
    void                       *refWrapperPtr,
    EB_COLOR_FORMAT             colorFormat,
    EB_BOOL                     secondChroma,
    EB_BOOL                     pictureLeftBoundary,
    EB_BOOL                     pictureTopBoundary,
    EB_BOOL                     pictureRightBoundary);

extern EB_ERRORTYPE IntraPredictionCl(
	struct ModeDecisionContext_s           *contextPtr,
	EB_U32                                  componentMask,
	PictureControlSet_t                    *pictureControlSetPtr,
	ModeDecisionCandidateBuffer_t		   *candidateBufferPtr);

extern EB_ERRORTYPE IntraPredictionOl(
	struct ModeDecisionContext_s		   *contextPtr,
	EB_U32                                  componentMask,
	PictureControlSet_t                    *pictureControlSetPtr,
	ModeDecisionCandidateBuffer_t		   *candidateBufferPtr);

extern EB_ERRORTYPE EncodePassIntraPrediction(
    void                                   *refSamples,
    EB_U32                                  originX,
    EB_U32                                  originY,
    EB_U32                                  puSize,
    EB_U32                                  puChromaSize,
    EbPictureBufferDesc_t                  *predictionPtr,
    EB_COLOR_FORMAT                         colorFormat,
    EB_BOOL                                 secondChroma,
    EB_U32                                  lumaMode,
    EB_U32                                  chromaMode,
    EB_U32                                  componentMask);
extern EB_ERRORTYPE EncodePassIntraPrediction16bit(
    void                                   *refSamples,
    EB_U32                                  originX,
    EB_U32                                  originY,
    EB_U32                                  puSize,
    EB_U32                                  puChromaSize,
    EbPictureBufferDesc_t                  *predictionPtr,
    EB_COLOR_FORMAT                         colorFormat,
    EB_BOOL                                 secondChroma,
    EB_U32                                  lumaMode,
    EB_U32                                  chromaMode,
    EB_U32                                  componentMask);

extern EB_ERRORTYPE EncodePassIntra4x4Prediction(
    void                                   *referenceSamples,
    EB_U32                                  originX,
    EB_U32                                  originY,
    EB_U32                                  puSize,
    EB_U32                                  chromaPuSize,
    EbPictureBufferDesc_t                  *predictionPtr,
    EB_U32                                  lumaMode,
    EB_U32                                  chromaMode,
    EB_COLOR_FORMAT                         colorFormat,
    EB_BOOL                                 secondChroma,
    EB_U32                                  componentMask);

extern EB_ERRORTYPE EncodePassIntra4x4Prediction16bit(
    void                                   *referenceSamples,
    EB_U32                                  originX,
    EB_U32                                  originY,
    EB_U32                                  puSize,
    EB_U32                                  chromaPuSize,
    EbPictureBufferDesc_t                  *predictionPtr,
    EB_U32                                  lumaMode,
    EB_U32                                  chromaMode,
    EB_COLOR_FORMAT                         colorFormat,
    EB_BOOL                                 secondChroma,
    EB_U32                                  componentMask);


static const EB_U32 intra422PredModeMap[] = {
    0, 1, 2, 2, 2, 2, 3, 5, 7, 8, 10, 12,13,15,17,18,19,20,21,22,23,23,24,24,25,25,26,27,27,28,28,29,29,30,31};

static const EB_U32 intraLumaModeNumber[] = {
    18,
    35,
    35,
    35,
    35 //4
};

static const EB_U32 intraModeOrderMap[] = {
    34,    //  Planar
    9,     //  Vertical
    25,    //  Horizontal
    0,     //  DC
    1,     //  Intra mode 4
    5,     //  Intra mode 5
    13,    //  Intra mode 6
    17,    //  Intra mode 7
    21,    //  Intra mode 8
    29,    //  Intra mode 9
    33,    //  Intra mode 10
    3,     //  Intra mode 11
    7,     //  Intra mode 12
    11,    //  Intra mode 13
    15,    //  Intra mode 14
    19,    //  Intra mode 15
    23,    //  Intra mode 16
    27,    //  Intra mode 17
    31,    //  Intra mode 18
    2,     //  Intra mode 19
    4,     //  Intra mode 20
    6,     //  Intra mode 21
    8,     //  Intra mode 22
    10,    //  Intra mode 23
    12,    //  Intra mode 24
    14,    //  Intra mode 25
    16,    //  Intra mode 26
    18,    //  Intra mode 27
    20,    //  Intra mode 28
    22,    //  Intra mode 29
    24,    //  Intra mode 30
    26,    //  Intra mode 31
    28,    //  Intra mode 32
    30,    //  Intra mode 33
    32,    //  Intra mode 34
};

static const EB_U32 chromaMappingTable[] = {
    EB_INTRA_CHROMA_PLANAR,
    EB_INTRA_VERTICAL,
    EB_INTRA_HORIZONTAL,
    EB_INTRA_DC,

    EB_INTRA_CHROMA_DM
};

extern EB_ERRORTYPE UpdateNeighborSamplesArrayOL(
    IntraReferenceSamples_t         *intraRefPtr,
    EbPictureBufferDesc_t           *inputPtr,          
    EB_U32                           stride,
    EB_U32                           srcOriginX,
    EB_U32                           srcOriginY,
    EB_U32                           blockSize,
    LargestCodingUnit_t             *lcuPtr);

extern EB_ERRORTYPE UpdateChromaNeighborSamplesArrayOL(
    IntraReferenceSamples_t         *intraRefPtr,
    EbPictureBufferDesc_t           *inputPtr,          
    EB_U32                           stride,
    EB_U32                           srcOriginX,
    EB_U32                           srcOriginY,
	EB_U32                           cbStride,
    EB_U32                           crStride,
    EB_U32                           cuChromaOriginX,
    EB_U32                           cuChromaOriginY,
    EB_U32                           blockSize,
    LargestCodingUnit_t             *lcuPtr);

extern EB_ERRORTYPE IntraOpenLoopReferenceSamplesCtor(
    IntraReferenceSamplesOpenLoop_t *contextPtr);

extern EB_ERRORTYPE UpdateNeighborSamplesArrayOpenLoop(
    IntraReferenceSamplesOpenLoop_t *intraRefPtr,
    EbPictureBufferDesc_t           *inputPtr,
    EB_U32                           stride,
    EB_U32                           srcOriginX,
    EB_U32                           srcOriginY,
    EB_U32                           blockSize);

extern EB_ERRORTYPE IntraPredictionOpenLoop(
    EB_U32                       cuSize, 
    MotionEstimationContext_t   *contextPtr,
    EB_U32           openLoopIntraCandidate);


extern EB_ERRORTYPE Intra4x4IntraPredictionCl(
    EB_U32                                  puIndex,
    EB_U32                                  puOriginX,
    EB_U32                                  puOriginY,
    EB_U32                                  puWidth,
    EB_U32                                  puHeight,
    EB_U32                                  lcuSize,
    EB_U32                                  componentMask,
    PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr,
    EB_PTR                                  predictionContextPtr);

extern EB_ERRORTYPE Intra4x4IntraPredictionOl(
	EB_U32                                  puIndex,
    EB_U32                                  puOriginX,
    EB_U32                                  puOriginY,
    EB_U32                                  puWidth,
    EB_U32                                  puHeight,
    EB_U32                                  lcuSize,
	EB_U32                                  componentMask,
	PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr,  
    EB_PTR                                  predictionContextPtr); 

extern void GenerateIntraLumaReferenceSamplesMd(
    struct ModeDecisionContext_s	*mdContextPtr,
	EbPictureBufferDesc_t		    *inputPicturePtr);

extern void GenerateIntraChromaReferenceSamplesMd(
    struct ModeDecisionContext_s	*mdContextPtr,
	EbPictureBufferDesc_t		    *inputPicturePtr);

/***************************************
* Function Ptr Types
***************************************/
typedef void(*EB_INTRA_NOANG_TYPE)(
    const EB_U32      size,
    EB_U8            *refSamples,
    EB_U8            *predictionPtr,
    const EB_U32      predictionBufferStride,
    const EB_BOOL     skip);

typedef EB_U32(*EB_NEIGHBOR_DC_INTRA_TYPE)(
	MotionEstimationContext_t       *contextPtr,
	EbPictureBufferDesc_t           *inputPtr,
	EB_U32                           srcOriginX,
	EB_U32                           srcOriginY,
	EB_U32                           blockSize);

typedef void(*EB_INTRA_NOANG_16bit_TYPE)(
    const EB_U32   size,
    EB_U16         *refSamples,
    EB_U16         *predictionPtr,
    const EB_U32   predictionBufferStride,
    const EB_BOOL  skip);

typedef void(*EB_INTRA_ANG_TYPE)(
    EB_U32            size,
    EB_U8            *refSampMain,
    EB_U8            *predictionPtr,
    EB_U32            predictionBufferStride,
    const EB_BOOL     skip,
    EB_S32            intraPredAngle);

typedef void(*EB_INTRA_ANG_16BIT_TYPE)(
    EB_U32          size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSampMain,                //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    EB_U32			predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL   skip,
    EB_S32   intraPredAngle);

/***************************************
* Function Ptrs
***************************************/
static EB_INTRA_NOANG_TYPE FUNC_TABLE IntraVerticalLuma_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeVerticalLuma,
    // AVX2
    IntraModeVerticalLuma_AVX2_INTRIN,

};

static EB_INTRA_NOANG_16bit_TYPE FUNC_TABLE IntraVerticalLuma_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeVerticalLuma16bit,
    // AVX2
    IntraModeVerticalLuma16bit_SSE2_INTRIN,
};

static EB_INTRA_NOANG_TYPE FUNC_TABLE IntraVerticalChroma_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeVerticalChroma,
    // AVX2
    IntraModeVerticalChroma_AVX2_INTRIN,
};

static EB_INTRA_NOANG_16bit_TYPE FUNC_TABLE IntraVerticalChroma_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeVerticalChroma16bit,
    // AVX2
    IntraModeVerticalChroma16bit_SSE2_INTRIN,
};

static EB_INTRA_NOANG_TYPE FUNC_TABLE IntraHorzLuma_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeHorizontalLuma,
    // AVX2
    IntraModeHorizontalLuma_SSE2_INTRIN,
};

static EB_INTRA_NOANG_16bit_TYPE FUNC_TABLE IntraHorzLuma_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeHorizontalLuma16bit,
    // AVX2
    IntraModeHorizontalLuma16bit_SSE2_INTRIN,
};

static EB_INTRA_NOANG_TYPE FUNC_TABLE IntraHorzChroma_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeHorizontalChroma,
    // AVX2
    IntraModeHorizontalChroma_SSE2_INTRIN,
};


static EB_INTRA_NOANG_16bit_TYPE FUNC_TABLE IntraHorzChroma_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeHorizontalChroma16bit,
    // AVX2
    IntraModeHorizontalChroma16bit_SSE2_INTRIN,
};

static EB_INTRA_NOANG_TYPE FUNC_TABLE IntraDCLuma_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeDCLuma,
    // AVX2
    IntraModeDCLuma_AVX2_INTRIN,
  
};

EB_U32 UpdateNeighborDcIntraPred(
	MotionEstimationContext_t       *contextPtr,
	EbPictureBufferDesc_t           *inputPtr,
	EB_U32                           srcOriginX,
	EB_U32                           srcOriginY,
	EB_U32                           blockSize);

static EB_INTRA_NOANG_16bit_TYPE FUNC_TABLE IntraDCLuma_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeDCLuma16bit,
    // AVX2
    IntraModeDCLuma16bit_SSE4_1_INTRIN,
};

static EB_INTRA_NOANG_TYPE FUNC_TABLE IntraDCChroma_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeDCChroma,
    // AVX2
    IntraModeDCChroma_AVX2_INTRIN,
};

static EB_INTRA_NOANG_16bit_TYPE FUNC_TABLE IntraDCChroma_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeDCChroma16bit,
    // AVX2
    IntraModeDCChroma16bit_SSSE3_INTRIN,
};

static EB_INTRA_NOANG_TYPE FUNC_TABLE IntraPlanar_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModePlanar,
    // AVX2
    IntraModePlanar_AVX2_INTRIN,
};

static EB_INTRA_NOANG_16bit_TYPE FUNC_TABLE IntraPlanar_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModePlanar16bit,
    // AVX2
    IntraModePlanar16bit_SSE2_INTRIN,
};

static EB_INTRA_NOANG_TYPE FUNC_TABLE IntraAng34_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeAngular_34,
    // AVX2
    IntraModeAngular_34_AVX2_INTRIN,
};

static EB_INTRA_NOANG_16bit_TYPE FUNC_TABLE IntraAng34_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeAngular16bit_34,
    // AVX2
    IntraModeAngular16bit_34_SSE2_INTRIN,
};

static EB_INTRA_NOANG_TYPE FUNC_TABLE IntraAng18_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeAngular_18,
    // AVX2
    IntraModeAngular_18_AVX2_INTRIN,

};

static EB_INTRA_NOANG_16bit_TYPE FUNC_TABLE IntraAng18_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeAngular16bit_18,
    // AVX2
    IntraModeAngular16bit_18_SSE2_INTRIN,
};

static EB_INTRA_NOANG_TYPE FUNC_TABLE IntraAng2_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeAngular_2,
    // AVX2
    IntraModeAngular_2_AVX2_INTRIN,
};

static EB_INTRA_NOANG_16bit_TYPE FUNC_TABLE IntraAng2_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeAngular16bit_2,
    // AVX2
    IntraModeAngular16bit_2_SSE2_INTRIN,
};

static EB_INTRA_ANG_TYPE FUNC_TABLE IntraAngVertical_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeAngular_Vertical_Kernel,
    // AVX2
    IntraModeAngular_Vertical_Kernel_AVX2_INTRIN,
};

static EB_INTRA_ANG_16BIT_TYPE FUNC_TABLE IntraAngVertical_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeAngular16bit_Vertical_Kernel,
    // AVX2
    IntraModeAngular16bit_Vertical_Kernel_SSE2_INTRIN,
};

static EB_INTRA_ANG_TYPE FUNC_TABLE IntraAngHorizontal_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeAngular_Horizontal_Kernel,
    // AVX2
    IntraModeAngular_Horizontal_Kernel_AVX2_INTRIN,
};

static EB_INTRA_ANG_16BIT_TYPE FUNC_TABLE IntraAngHorizontal_16bit_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    IntraModeAngular16bit_Horizontal_Kernel,
    // AVX2
    IntraModeAngular16bit_Horizontal_Kernel_SSE2_INTRIN,
};

#ifdef __cplusplus
}
#endif
#endif // EbIntraPrediction_h
