/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbDeblockingFilter_h
#define EbDeblockingFilter_h

#include "EbDeblockingFilter_C.h"
#include "EbDeblockingFilter_SSE2.h"
#include "EbDeblockingFilter_SSSE3.h"
#include "EbPredictionUnit.h"
#include "EbNeighborArrays.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)                   (((blk4x4Addr) & (MAX_LCU_SIZE_IN_4X4BLK - 1)) + (((blk4x4Addr) / MAX_LCU_SIZE_IN_4X4BLK) * MAX_LCU_SIZE_IN_4X4BLK))
#define BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)                 (((blk4x4Addr) & (MAX_LCU_SIZE_IN_4X4BLK - 1)) + (((blk4x4Addr) / MAX_LCU_SIZE_IN_4X4BLK) * MAX_LCU_SIZE_IN_4X4BLK))
#define GET_LUMA_4X4BLK_ADDR(lumaLcuWise4x4BlkPos_x, lumaLcuWise4x4BlkPos_y, logMaxLcuSizeIn4x4blk)           (((lumaLcuWise4x4BlkPos_x)>>2) + (((lumaLcuWise4x4BlkPos_y)>>2) << (logMaxLcuSizeIn4x4blk)))
#define GET_CHROMA_4X4BLK_ADDR(chromaLcuWise2x2BlkPos_x, chromaLcuWise2x2BlkPos_y, logMaxLcuSizeIn4x4blk)     (((chromaLcuWise2x2BlkPos_x)>>1) + (((chromaLcuWise2x2BlkPos_y)>>1) << (logMaxLcuSizeIn4x4blk)))
#define LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(pos_x, pos_y, qpArrayStride)                            (((pos_x) >> LOG_MIN_CU_SIZE) + ((pos_y) >> LOG_MIN_CU_SIZE) * (qpArrayStride))
#define CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(subw, subh, pos_x, pos_y, qpArrayStride)                          (((subw*(EB_S32)(pos_x)) >> LOG_MIN_CU_SIZE) + ((subh*(EB_S32)(pos_y)) >> LOG_MIN_CU_SIZE) * (qpArrayStride))
#define CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(pu1Ptr, pu2Ptr, pu1RefList, pu2RefList)                    (                         \
                            EB_ABS_DIFF((pu1Ptr)->mv[(pu1RefList)].x, (pu2Ptr)->mv[(pu2RefList)].x) >= 4 ||         \
                            EB_ABS_DIFF((pu1Ptr)->mv[(pu1RefList)].y, (pu2Ptr)->mv[(pu2RefList)].y) >= 4            \
                            )

// Precision macros used in the mode decision
#define BIT_ESTIMATE_PRECISION           15
#define LAMBDA_PRECISION                 16
#define COST_PRECISION                   8
#define MD_SHIFT                         (BIT_ESTIMATE_PRECISION + LAMBDA_PRECISION - COST_PRECISION)
#define MD_OFFSET                        (1 << (MD_SHIFT-1))
#define VAR_QP                           1



#define MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET 53
#define BETA_OFFSET_VALUE				  12 // range -12 to 12

#define TC_OFFSET_VALUE					12//12 // range -12 to 12

void SetQpArrayBasedOnCU(
	PictureControlSet_t *pictureControlSetPtr,          //input parameter
	EB_U32               cuPos_x,                       //input parameter, sample-based horizontal picture-wise locatin of the CU
	EB_U32               cuPos_y,                       //input parameter, sample-based vertical picture-wise locatin of the CU
	EB_U32               cuSizeInMinCuSize,             //input parameter
	EB_U32               cuQp) ;                         //input parameter, Qp of the CU

extern void entropySetQpArrayBasedOnCU(
    PictureControlSet_t *pictureControlSetPtr,
    EB_U32               cuPos_x,
    EB_U32               cuPos_y,
    EB_U32               cuWidth,
    EB_U32               cuHeight,
    EB_U32               cuQp);

extern EB_U8 CalculateBSForPUBoundary(
    EB_U32                 blk4x4Pos_x,
    EB_U32                 blk4x4Pos_y,
    PredictionUnit_t      *puPtr,
    PredictionUnit_t      *neighbourPuPtr,
    EB_MODETYPE            puCodingMode,
    EB_MODETYPE            neighbourPuCodingMode,
    EB_BOOL                isVerticalEdge, 
    PictureControlSet_t   *pictureControlSetPtr,
    SequenceControlSet_t  *sequenceControlSetPtr);

extern void SetBSArrayBasedOnPUBoundary(
    NeighborArrayUnit_t     *modeTypeNeighborArray,
    NeighborArrayUnit_t     *mvNeighborArray,
    PredictionUnit_t        *puPtr,
    CodingUnit_t            *cuPtr,
    const CodedUnitStats_t  *cuStatsPtr,
    EB_U32                   tbOriginX,
    EB_U32                   tbOriginY,
    EB_BOOL                  tileLeftBoundary,
    EB_BOOL                  tileTopBoundary,
    //EB_U32                       tileOriginX,
    //EB_U32                       tileOriginY,
    PictureControlSet_t     *pictureControlSetPtr,
    EB_U8                   *horizontalEdgeBSArray,
    EB_U8                   *verticalEdgeBSArray);

extern void SetBSArrayBasedOnTUBoundary(
    EB_U32                 tuPos_x,
    EB_U32                 tuPos_y,
    EB_U32                 tuWidth,
    EB_U32                 tuHeight,
    const CodedUnitStats_t *cuStatsPtr,
    EB_MODETYPE            cuCodingMode,
    EB_U32                 tbOriginX,
    EB_U32                 tbOriginY,
    PictureControlSet_t   *pictureControlSetPtr,
    EB_U8                 *horizontalEdgeBSArray,
    EB_U8                 *verticalEdgeBSArray);

extern EB_U8 Intra4x4CalculateBSForPUBoundary(
    EB_U32                 blk4x4Pos_x,
    EB_U32                 blk4x4Pos_y,
    PredictionUnit_t      *puPtr,
    PredictionUnit_t      *neighbourPuPtr,
    EB_MODETYPE            puCodingMode,
    EB_MODETYPE            neighbourPuCodingMode,
    EB_BOOL                isVerticalEdge, 
    EB_BOOL                isVerticalPuBoundaryAlsoTuBoundary,
    EB_BOOL                isHorizontalPuBoundaryAlsoTuBoundary,
    PictureControlSet_t   *pictureControlSetPtr,
    SequenceControlSet_t  *sequenceControlSetPtr);

extern void Intra4x4SetBSArrayBasedOnTUBoundary(
    EB_U32                 tuPos_x,
    EB_U32                 tuPos_y,
    EB_U32                 tuWidth,
    EB_U32                 tuHeight,
    EB_BOOL                isHorizontalTuBoundaryAlsoPuBoundary,
    EB_BOOL                isVerticalTuBoundaryAlsoPuBoundary,
    const CodedUnitStats_t *cuStatsPtr,
    EB_MODETYPE            cuCodingMode,
    EB_U32                 tbOriginX,
    EB_U32                 tbOriginY,
    PictureControlSet_t   *pictureControlSetPtr,
    EB_U8                 *horizontalEdgeBSArray,
    EB_U8                 *verticalEdgeBSArray);

extern void Intra4x4SetBSArrayBasedOnPUBoundary(
    NeighborArrayUnit_t     *modeTypeNeighborArray,
    NeighborArrayUnit_t     *mvNeighborArray,
    PredictionUnit_t        *puPtr,
    CodingUnit_t            *cuPtr,
    const CodedUnitStats_t  *cuStatsPtr,
	EB_U32                   puOriginX,
    EB_U32                   puOriginY,
    EB_U32                   puWidth,
    EB_U32                   puHeight,
    EB_U32                   tbOriginX,
    EB_U32                   tbOriginY,
    EB_BOOL                  isVerticalPUBoundaryAlsoPictureBoundary,
    EB_BOOL                  isHorizontalPUBoundaryAlsoPictureBoundary,
    PictureControlSet_t     *pictureControlSetPtr,
    EB_U8                   *horizontalEdgeBSArray,
    EB_U8                   *verticalEdgeBSArray);


extern EB_ERRORTYPE LCUInternalAreaDLFCore(
	EbPictureBufferDesc_t *reconpicture,
    EB_U32                 lcuPosx,
    EB_U32                 lcuPosy,
    EB_U32                 lcuWidth,
    EB_U32                 lcuHeight,
    EB_U8                 *verticalEdgeBSArray,
    EB_U8                 *horizontalEdgeBSArray,
    PictureControlSet_t   *reconPictureControlSet);

extern EB_ERRORTYPE LCUInternalAreaDLFCore16bit(
	EbPictureBufferDesc_t *reconpicture,
    EB_U32                 lcuPosx,
    EB_U32                 lcuPosy,
    EB_U32                 lcuWidth,
    EB_U32                 lcuHeight,
    EB_U8                 *verticalEdgeBSArray,
    EB_U8                 *horizontalEdgeBSArray,
    PictureControlSet_t   *reconPictureControlSet);

extern void LCUBoundaryDLFCore(
	EbPictureBufferDesc_t *reconpicture,
    EB_U32                 lcuPos_x,                       
    EB_U32                 lcuPos_y,                       
    EB_U32                 lcuWidth,                       
    EB_U32                 lcuHeight,                      
    EB_U8                 *lcuVerticalEdgeBSArray,         
    EB_U8                 *lcuHorizontalEdgeBSArray,       
    EB_U8                 *topLcuVerticalEdgeBSArray,      
    EB_U8                 *leftLcuHorizontalEdgeBSArray,   
    PictureControlSet_t   *pictureControlSetPtr);

extern void LCUBoundaryDLFCore16bit(
	EbPictureBufferDesc_t *reconpicture,
    EB_U32                 lcuPos_x,                       
    EB_U32                 lcuPos_y,                       
    EB_U32                 lcuWidth,                       
    EB_U32                 lcuHeight,                      
    EB_U8                 *lcuVerticalEdgeBSArray,         
    EB_U8                 *lcuHorizontalEdgeBSArray,       
    EB_U8                 *topLcuVerticalEdgeBSArray,      
    EB_U8                 *leftLcuHorizontalEdgeBSArray,   
    PictureControlSet_t   *pictureControlSetPtr);

extern void LCUPictureEdgeDLFCore(
    EbPictureBufferDesc_t *reconPic,                       
    EB_U32                 lcuIdx,
    EB_U32                 lcuPos_x,                       
    EB_U32                 lcuPos_y,                       
    EB_U32                 lcuWidth,                       
    EB_U32                 lcuHeight,                      
    PictureControlSet_t   *pictureControlSetPtr);          

extern void LCUPictureEdgeDLFCore16bit(
    EbPictureBufferDesc_t *reconPic,                       
    EB_U32                 lcuIdx,
    EB_U32                 lcuPos_x,                       
    EB_U32                 lcuPos_y,                       
    EB_U32                 lcuWidth,                       
    EB_U32                 lcuHeight,                      
    PictureControlSet_t   *pictureControlSetPtr);          

/***************************************
* Function Types
***************************************/
typedef void(*EB_LUMA4SAMPLEDGEDLF_FUNC)(
    EB_BYTE                edgeStartSample,
    EB_U32                 reconLumaPicStride,
    EB_BOOL                isVerticalEdge,
    EB_S32                 tc,
    EB_S32                 beta);

typedef void(*EB_CHROMA2SAMPLEDGEDLF_FUNC)(
    EB_BYTE                edgeStartSampleCb,
    EB_BYTE                edgeStartSampleCr,
    EB_U32                 reconChromaPicStride,
    EB_BOOL                isVerticalEdge,
    EB_U8                  cbTc,
    EB_U8                  crTc);

typedef void(*EB_CHROMADLF_TYPE_16BIT)(
    EB_U16				  *edgeStartSampleCb,
    EB_U16				  *edgeStartSampleCr,
    EB_U32                 reconChromaPicStride,
    EB_BOOL                isVerticalEdge,
    EB_U8                  cbTc,
    EB_U8                  crTc);

typedef void(*EB_LUMADLF_TYPE_16BIT)(
    EB_U16         *edgeStartFilteredSamplePtr,
    EB_U32          reconLumaPicStride,
    EB_BOOL         isVerticalEdge,
    EB_S32          tc,
    EB_S32          beta);

/***************************************
* Function Ptr Types
***************************************/
static EB_LUMA4SAMPLEDGEDLF_FUNC FUNC_TABLE Luma4SampleEdgeDLFCore_Table[EB_ASM_TYPE_TOTAL] =
{  
	// C_DEFAULT
    Luma4SampleEdgeDLFCore,
    // AVX2
    Luma4SampleEdgeDLFCore_SSSE3,
};

static EB_CHROMA2SAMPLEDGEDLF_FUNC FUNC_TABLE Chroma2SampleEdgeDLFCore_Table[EB_ASM_TYPE_TOTAL] =
{
    // C_DEFAULT
    Chroma2SampleEdgeDLFCore,
    // AVX2
    Chroma2SampleEdgeDLFCore_SSSE3,
};

static EB_CHROMADLF_TYPE_16BIT FUNC_TABLE chromaDlf_funcPtrArray16bit[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    Chroma2SampleEdgeDLFCore16bit,
    // AVX2
    Chroma2SampleEdgeDLFCore16bit_SSE2_INTRIN,
};

static EB_LUMADLF_TYPE_16BIT FUNC_TABLE lumaDlf_funcPtrArray16bit[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    Luma4SampleEdgeDLFCore16bit,
    // AVX2
    Luma4SampleEdgeDLFCore16bit_SSSE3_INTRIN,
};

#ifdef __cplusplus
}
#endif
#endif
