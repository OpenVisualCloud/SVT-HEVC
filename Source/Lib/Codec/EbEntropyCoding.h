/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbEntropyCoding_h
#define EbEntropyCoding_h

#include "EbDefinitions.h"
#include "EbEntropyCodingObject.h"
#include "EbEntropyCodingUtil.h"


#include "EbCodingUnit.h"
#include "EbPredictionUnit.h"
#include "EbPictureBufferDesc.h"
#include "EbSequenceControlSet.h"
#include "EbPictureControlSet.h"
#include "EbCabacContextModel.h"
#include "EbModeDecision.h"
#include "EbIntraPrediction.h"
#include "EbBitstreamUnit.h"
#include "EbPacketizationProcess.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************
 * Extern Function Declarations
 **************************************/
extern EB_ERRORTYPE EncodeLcu(
    LargestCodingUnit_t     *tbPtr,
    EB_U32                   lcuOriginX,
    EB_U32                   lcuOriginY,
    PictureControlSet_t     *pictureControlSetPtr,
    EB_U32                   lcuSize,
    EntropyCoder_t          *entropyCoderPtr,
    EbPictureBufferDesc_t   *coeffPtr,
    NeighborArrayUnit_t     *modeTypeNeighborArray,
    NeighborArrayUnit_t     *leafDepthNeighborArray,
    NeighborArrayUnit_t     *intraLumaModeNeighborArray,
    NeighborArrayUnit_t     *skipFlagNeighborArray,
    EB_U32                   pictureOriginX,
	EB_U32                   pictureOriginY);

#if TILES
extern EB_ERRORTYPE EncodeTileFinish(
    EntropyCoder_t        *entropyCoderPtr);
#endif

extern EB_ERRORTYPE EncodeLcuSaoParameters(
    LargestCodingUnit_t     *tbPtr,
    EntropyCoder_t          *entropyCoderPtr,
    EB_BOOL                  saoLumaSliceEnable,
    EB_BOOL                  saoChromaSliceEnable,
    EB_U8                    bitdepth);

extern EB_ERRORTYPE EncodeSliceHeader(
    EB_U32                   firstLcuAddr,
    EB_U32                   pictureQp,
    PictureControlSet_t     *pcsPtr,
    OutputBitstreamUnit_t   *bitstreamPtr);

extern EB_ERRORTYPE EncodeTerminateLcu(
    EntropyCoder_t        *entropyCoderPtr,
    EB_U32                 lastLcuInSlice);

extern EB_ERRORTYPE EncodeSliceFinish(
    EntropyCoder_t        *entropyCoderPtr);

extern EB_ERRORTYPE ResetBitstream(
    EB_PTR                 bitstreamPtr);

extern EB_ERRORTYPE ResetEntropyCoder(
    EncodeContext_t       *encodeContextPtr,
    EntropyCoder_t        *entropyCoderPtr,
    EB_U32                 qp,
    EB_PICTURE               sliceType);

extern EB_ERRORTYPE FlushBitstream(
    EB_PTR outputBitstreamPtr);

extern EB_ERRORTYPE EncodeAUD(
    Bitstream_t *bitstreamPtr,
    EB_PICTURE     sliceType,
    EB_U32       temporalId); 

extern EB_ERRORTYPE ComputeProfileTierLevelInfo(
    SequenceControlSet_t    *scsPtr);

extern EB_ERRORTYPE ComputeMaxDpbBuffer(
	SequenceControlSet_t    *scsPtr);

extern EB_ERRORTYPE EncodeVPS(
    Bitstream_t *bitstreamPtr,
    SequenceControlSet_t *scsPtr); 

extern EB_ERRORTYPE EncodeSPS(
    Bitstream_t *bitstreamPtr,
    SequenceControlSet_t *scsPtr);

extern EB_ERRORTYPE EncodePPS(
    Bitstream_t *bitstreamPtr,
    SequenceControlSet_t *scsPtr,
    EbPPSConfig_t       *ppsConfig);

EB_ERRORTYPE TuEstimateCoeffBits_R(
	EB_U32                     tuOriginIndex,
	EB_U32                     tuChromaOriginIndex,
	EB_U32                     componentMask,
	EntropyCoder_t            *entropyCoderPtr,
	EbPictureBufferDesc_t     *coeffPtr,
	EB_U32                     yCountNonZeroCoeffs,
	EB_U32                     cbCountNonZeroCoeffs,
	EB_U32                     crCountNonZeroCoeffs,
	EB_U64                    *yTuCoeffBits,
	EB_U64                    *cbTuCoeffBits,
	EB_U64                    *crTuCoeffBits,
	EB_U32                     transformSize,
	EB_U32                     transformChromaSize,
	EB_MODETYPE                type,
	EB_U32					   intraLumaMode,
	EB_U32                     intraChromaMode,
	EB_U32                     partialFrequencyN2Flag,
    EB_BOOL                    coeffCabacUpdate,
    CoeffCtxtMdl_t            *updatedCoeffCtxModel,
	CabacCost_t               *CabacCost);

extern EB_ERRORTYPE TuEstimateCoeffBitsEncDec(

	EB_U32                     tuOriginIndex,
	EB_U32                     tuChromaOriginIndex,
	EntropyCoder_t            *entropyCoderPtr,
	EbPictureBufferDesc_t     *coeffBufferTB,
    EB_U32                     countNonZeroCoeffs[3],
	EB_U64                    *yTuCoeffBits,
	EB_U64                    *cbTuCoeffBits,
	EB_U64                    *crTuCoeffBits,
	EB_U32                     transformSize,
	EB_U32                     transformChromaSize,
	EB_MODETYPE                type,
	CabacCost_t               *CabacCost);

extern EB_ERRORTYPE TuEstimateCoeffBitsLuma(

    EB_U32                     tuOriginIndex,
    EntropyCoder_t            *entropyCoderPtr,
    EbPictureBufferDesc_t     *coeffPtr,
    EB_U32                     yCountNonZeroCoeffs,
    EB_U64                    *yTuCoeffBits,
    EB_U32                     transformSize,
    EB_MODETYPE                type,
    EB_U32                     intraLumaMode,   
    EB_U32                     partialFrequencyN2Flag,
    EB_BOOL                    coeffCabacUpdate,
    CoeffCtxtMdl_t            *updatedCoeffCtxModel,

	CabacCost_t               *CabacCost);

extern EB_ERRORTYPE EncodeBufferingPeriodSEI(
    Bitstream_t             *bitstreamPtr,
    AppBufferingPeriodSei_t *bufferingPeriodPtr,
    AppVideoUsabilityInfo_t *vuiPtr,
    EncodeContext_t         *encodeContextPtr); 


extern EB_ERRORTYPE EncodePictureTimingSEI(
    Bitstream_t             *bitstreamPtr,
    AppPictureTimingSei_t   *picTimingSeiPtr,
    AppVideoUsabilityInfo_t *vuiPtr,
    EncodeContext_t         *encodeContextPtr,
    EB_U8                    pictStruct);


extern EB_ERRORTYPE EncodeRegUserDataSEI(
    Bitstream_t             *bitstreamPtr,
    RegistedUserData_t      *regUserDataSeiPtr);

extern EB_ERRORTYPE EncodeUnregUserDataSEI(
    Bitstream_t             *bitstreamPtr,
    UnregistedUserData_t    *unregUserDataSeiPtr,
    EncodeContext_t         *encodeContextPtr);

extern EB_ERRORTYPE EncodeRecoveryPointSEI(
    Bitstream_t             *bitstreamPtr,
    AppRecoveryPoint_t      *recoveryPointSeiPtr);

extern EB_ERRORTYPE EncodeContentLightLevelSEI(
    Bitstream_t             *bitstreamPtr,
    AppContentLightLevelSei_t   *contentLightLevelPtr);

EB_ERRORTYPE EncodeMasteringDisplayColorVolumeSEI(
    Bitstream_t             *bitstreamPtr,
    AppMasteringDisplayColorVolumeSei_t   *masterDisplayPtr);

EB_ERRORTYPE CodeDolbyVisionRpuMetadata(
    Bitstream_t  *bitstreamPtr,
    PictureControlSet_t *pictureControlSetPtr);

extern EB_ERRORTYPE CodeEndOfSequenceNalUnit(
    Bitstream_t             *bitstreamPtr);

extern EB_ERRORTYPE CopyRbspBitstreamToPayload(
    Bitstream_t *bitstreamPtr,
    EB_BYTE      outputBuffer,
    EB_U32      *outputBufferIndex,
    EB_U32      *outputBufferSize,
    EncodeContext_t         *encodeContextPtr,
    NalUnitType nalType);

void EncodeQuantizedCoefficients_SSE2(
    CabacEncodeContext_t         *cabacEncodeCtxPtr,
    EB_U32                        size,                 // Input: TU size
    EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
    EB_U32                        intraLumaMode,
    EB_U32                        intraChromaMode,
    EB_S16                       *coeffBufferPtr,
    const EB_U32                  coeffStride,
    EB_U32                        componentType,
    TransformUnit_t        *tuPtr);

EB_ERRORTYPE EstimateQuantizedCoefficients_SSE2(
	CabacCost_t                  *CabacCost,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
	EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong);


EB_ERRORTYPE EstimateQuantizedCoefficients_Lossy_SSE2(
	CabacCost_t                  *CabacCost,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
	EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong);

EB_ERRORTYPE EstimateQuantizedCoefficients_generic_Update(
    CoeffCtxtMdl_t               *UpdatedCoeffCtxModel,
    CabacCost_t                  *CabacCost,
    CabacEncodeContext_t         *cabacEncodeCtxPtr,
    EB_U32                        size,                 // Input: TU size
    EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
    EB_U32                        intraLumaMode,
    EB_U32                        intraChromaMode,
    EB_S16                       *coeffBufferPtr,
    const EB_U32                  coeffStride,
    EB_U32                        componentType,
    EB_U32                        numNonZeroCoeffs,
    EB_U64                       *coeffBitsLong);

EB_ERRORTYPE EstimateQuantizedCoefficients_generic(
    CabacCost_t                  *CabacCost,
    CabacEncodeContext_t         *cabacEncodeCtxPtr,
    EB_U32                        size,                 // Input: TU size
    EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
    EB_U32                        intraLumaMode,
    EB_U32                        intraChromaMode,
    EB_S16                       *coeffBufferPtr,
    const EB_U32                  coeffStride,
    EB_U32                        componentType,
    EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong);

EB_ERRORTYPE EstimateQuantizedCoefficients_Lossy(
    CabacCost_t                  *CabacCost,
    CabacEncodeContext_t         *cabacEncodeCtxPtr,
    EB_U32                        size,                 // Input: TU size
    EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
    EB_U32                        intraLumaMode,
    EB_U32                        intraChromaMode,
    EB_S16                       *coeffBufferPtr,
    const EB_U32                  coeffStride,
    EB_U32                        componentType,
    EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong);

void EncodeQuantizedCoefficients_generic(
    CabacEncodeContext_t         *cabacEncodeCtxPtr,
    EB_U32                        size,                 // Input: TU size
    EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
    EB_U32                        intraLumaMode,
    EB_U32                        intraChromaMode,
    EB_S16                       *coeffBufferPtr,
    const EB_U32                  coeffStride,
    EB_U32                        componentType,
    TransformUnit_t        *tuPtr);

/**************************************
* Function Types
**************************************/
typedef EB_ERRORTYPE(*ESTIMATE_QUANTIZED_COEFF_TYPE)(
	CabacCost_t                  *CabacCost,
	CabacEncodeContext_t         *cabacEncodeCtxPtr,
	EB_U32                        size,                 // Input: TU size
	EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
	EB_U32                        intraLumaMode,
	EB_U32                        intraChromaMode,
	EB_S16                       *coeffBufferPtr,
	const EB_U32                  coeffStride,
	EB_U32                        componentType,
	EB_U32                        numNonZeroCoeffs,
	EB_U64                       *coeffBitsLong);
typedef void(*ENCODE_QUANTIZED_COEFF_TYPE) (
    CabacEncodeContext_t         *cabacEncodeCtxPtr,
    EB_U32                        size,                 // Input: TU size
    EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
    EB_U32                        intraLumaMode,
    EB_U32                        intraChromaMode,
    EB_S16                       *coeffBufferPtr,
    const EB_U32                  coeffStride,
    EB_U32                        componentType,
    TransformUnit_t        *tuPtr);

static ESTIMATE_QUANTIZED_COEFF_TYPE FUNC_TABLE EstimateQuantizedCoefficients[2][EB_ASM_TYPE_TOTAL] = { // [lossless/lossy][c/assembly]
{
    // C_DEFAULT
    EstimateQuantizedCoefficients_generic,
    // AVX2
    EstimateQuantizedCoefficients_SSE2,
},
{
    // C_DEFAULT
    EstimateQuantizedCoefficients_Lossy,
    // AVX2
    EstimateQuantizedCoefficients_Lossy_SSE2,
}
};


static ENCODE_QUANTIZED_COEFF_TYPE FUNC_TABLE EncodeQuantizedCoefficientsFuncArray[EB_ASM_TYPE_TOTAL] =
{
	// C_DEFAULT
    EncodeQuantizedCoefficients_generic,
	// AVX2
	EncodeQuantizedCoefficients_SSE2,
};


EB_ERRORTYPE EstimateQuantizedCoefficients_Update_SSE2(
    CoeffCtxtMdl_t               *updatedCoeffCtxModel,
    CabacCost_t                  *CabacCost,
    CabacEncodeContext_t         *cabacEncodeCtxPtr,
    EB_U32                        size,                 // Input: TU size
    EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
    EB_U32                        intraLumaMode,
    EB_U32                        intraChromaMode,
    EB_S16                       *coeffBufferPtr,
    const EB_U32                  coeffStride,
    EB_U32                        componentType,
    EB_U32                        numNonZeroCoeffs,
    EB_U64                       *coeffBitsLong);

typedef EB_ERRORTYPE(*ESTIMATE_QUANTIZED_COEFF_TYPE_UPDATE)(
    CoeffCtxtMdl_t               *updatedCoeffCtxModel,
    CabacCost_t                  *CabacCost,
    CabacEncodeContext_t         *cabacEncodeCtxPtr,
    EB_U32                        size,                 // Input: TU size
    EB_MODETYPE                   type,                 // Input: CU type (INTRA, INTER)
    EB_U32                        intraLumaMode,
    EB_U32                        intraChromaMode,
    EB_S16                       *coeffBufferPtr,
    const EB_U32                  coeffStride,
    EB_U32                        componentType,
    EB_U32                        numNonZeroCoeffs,
    EB_U64                       *coeffBitsLong);

static ESTIMATE_QUANTIZED_COEFF_TYPE_UPDATE FUNC_TABLE EstimateQuantizedCoefficientsUpdate[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    EstimateQuantizedCoefficients_generic_Update,
    // AVX2
    EstimateQuantizedCoefficients_Update_SSE2
};



#ifdef __cplusplus
}
#endif
#endif //EbEntropyCoding_h
