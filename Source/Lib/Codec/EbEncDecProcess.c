/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbTransforms.h"
#include "EbEncDecTasks.h"
#include "EbEncDecResults.h"
#include "EbPictureDemuxResults.h"
#include "EbCodingLoop.h"
#include "EbSampleAdaptiveOffset.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"


void PrecomputeCabacCost(CabacCost_t            *CabacCostPtr,
    CabacEncodeContext_t   *cabacEncodeCtxPtr);

const EB_S16 encMinDeltaQpWeightTab[MAX_TEMPORAL_LAYERS] = { 100, 100, 100, 100, 100, 100 };
const EB_S16 encMaxDeltaQpWeightTab[MAX_TEMPORAL_LAYERS] = { 100, 100, 100, 100, 100, 100 };

const EB_S8  encMinDeltaQpISliceTab[4] = { -5, -5, -3, -2 };

const EB_S8  encMinDeltaQpTab[4][MAX_TEMPORAL_LAYERS] = {
    { -4, -2, -2, -1, -1, -1 },
    { -4, -2, -2, -1, -1, -1 },
    { -3, -1, -1, -1, -1, -1 },
    { -1, -0, -0, -0, -0, -0 },
};

const EB_S8  encMaxDeltaQpTab[4][MAX_TEMPORAL_LAYERS] = {
    { 4, 5, 5, 5, 5, 5 },
    { 4, 5, 5, 5, 5, 5 },
    { 4, 5, 5, 5, 5, 5 },
    { 4, 5, 5, 5, 5, 5 }
};

/******************************************************
 * Enc Dec Context Constructor
 ******************************************************/
EB_ERRORTYPE EncDecContextCtor(
    EncDecContext_t        **contextDblPtr,
    EbFifo_t                *modeDecisionConfigurationInputFifoPtr,
    EbFifo_t                *packetizationOutputFifoPtr,
    EbFifo_t                *feedbackFifoPtr,
    EbFifo_t                *pictureDemuxFifoPtr,
    EB_BOOL                  is16bit,
    EB_COLOR_FORMAT          colorFormat)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EncDecContext_t *contextPtr;
    EB_MALLOC(EncDecContext_t*, contextPtr, sizeof(EncDecContext_t), EB_N_PTR);
    *contextDblPtr = contextPtr;

    contextPtr->is16bit = is16bit;
    contextPtr->colorFormat = colorFormat;
    contextPtr->tileRowIndex = 0;

    // Input/Output System Resource Manager FIFOs
    contextPtr->modeDecisionInputFifoPtr = modeDecisionConfigurationInputFifoPtr;
    contextPtr->encDecOutputFifoPtr = packetizationOutputFifoPtr;
    contextPtr->encDecFeedbackFifoPtr = feedbackFifoPtr;
    contextPtr->pictureDemuxOutputFifoPtr = pictureDemuxFifoPtr;

    // Trasform Scratch Memory
    EB_MALLOC(EB_S16*, contextPtr->transformInnerArrayPtr, 3152, EB_N_PTR); //refer to EbInvTransform_SSE2.as. case 32x32
    // MD rate Estimation tables
    EB_MALLOC(MdRateEstimationContext_t*, contextPtr->mdRateEstimationPtr, sizeof(MdRateEstimationContext_t), EB_N_PTR);

    // Sao Stats
    return_error = SaoStatsCtor(&contextPtr->saoStats);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // Prediction Buffer
    {
        EbPictureBufferDescInitData_t initData;

        initData.bufferEnableMask = PICTURE_BUFFER_DESC_FULL_MASK;
        initData.maxWidth = MAX_LCU_SIZE;
        initData.maxHeight = MAX_LCU_SIZE;
        initData.bitDepth = EB_8BIT;
        initData.leftPadding = 0;
        initData.rightPadding = 0;
        initData.topPadding = 0;
        initData.botPadding = 0;
        initData.splitMode = EB_FALSE;
        initData.colorFormat = colorFormat;

        contextPtr->inputSample16bitBuffer = (EbPictureBufferDesc_t *)EB_NULL;
        if (is16bit) {
            initData.bitDepth = EB_16BIT;

            return_error = EbPictureBufferDescCtor(
                (EB_PTR*)&contextPtr->inputSample16bitBuffer,
                (EB_PTR)&initData);
            if (return_error == EB_ErrorInsufficientResources){
                return EB_ErrorInsufficientResources;
            }
        }

    }

    // Scratch Coeff Buffer
    {
        EbPictureBufferDescInitData_t initData;

        initData.bufferEnableMask = PICTURE_BUFFER_DESC_FULL_MASK;
        initData.maxWidth = MAX_LCU_SIZE;
        initData.maxHeight = MAX_LCU_SIZE;
        initData.bitDepth = EB_16BIT;
        initData.colorFormat = colorFormat;
        initData.leftPadding = 0;
        initData.rightPadding = 0;
        initData.topPadding = 0;
        initData.botPadding = 0;
        initData.splitMode = EB_FALSE;

        return_error = EbPictureBufferDescCtor(
            (EB_PTR*)&contextPtr->residualBuffer,
            (EB_PTR)&initData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

        return_error = EbPictureBufferDescCtor(
            (EB_PTR*)&contextPtr->transformBuffer,
            (EB_PTR)&initData);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }

    }

    // Intra Reference Samples
    return_error = IntraReferenceSamplesCtor(&contextPtr->intraRefPtr, colorFormat);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }
    contextPtr->intraRefPtr16 = (IntraReference16bitSamples_t *)EB_NULL;
    if (is16bit) {
        return_error = IntraReference16bitSamplesCtor(&contextPtr->intraRefPtr16, colorFormat);
        if (return_error == EB_ErrorInsufficientResources){
            return EB_ErrorInsufficientResources;
        }
    }

    // MCP Context
    return_error = MotionCompensationPredictionContextCtor(
        &contextPtr->mcpContext,
        MAX_LCU_SIZE,
        MAX_LCU_SIZE,
        is16bit);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    // Mode Decision Context
    return_error = ModeDecisionContextCtor(
        &contextPtr->mdContext, 
        0, 
        0,
        is16bit);
    if (return_error == EB_ErrorInsufficientResources){
        return EB_ErrorInsufficientResources;
    }

    contextPtr->mdContext->encDecContextPtr = contextPtr;

    // SAO Application
    //TODO: we need to allocate Up buffer using current frame Width (not MAX_PICTURE_WIDTH_SIZE)
    //Need one pixel at position(x=-1) and another at position(x=width) to accomodate SIMD optimization of SAO
    if (!is16bit) {
        EB_MALLOC(EB_U8 *, contextPtr->saoUpBuffer[0], sizeof(EB_U8) * (MAX_PICTURE_WIDTH_SIZE + 2) * 2, EB_N_PTR);

        EB_MEMSET(contextPtr->saoUpBuffer[0], 0, (MAX_PICTURE_WIDTH_SIZE + 2) * 2);
        contextPtr->saoUpBuffer[0] ++;
        contextPtr->saoUpBuffer[1] = contextPtr->saoUpBuffer[0] + (MAX_PICTURE_WIDTH_SIZE + 2);

        EB_MALLOC(EB_U8 *, contextPtr->saoLeftBuffer[0], sizeof(EB_U8) *(MAX_LCU_SIZE + 2) * 2 + 14, EB_N_PTR);

        EB_MEMSET(contextPtr->saoLeftBuffer[0], 0, (MAX_LCU_SIZE + 2) * 2 + 14);
        contextPtr->saoLeftBuffer[1] = contextPtr->saoLeftBuffer[0] + (MAX_LCU_SIZE + 2);
    }
    else{

        //CHKN only allocate in 16 bit mode
        EB_MALLOC(EB_U16 *, contextPtr->saoUpBuffer16[0], sizeof(EB_U16) * (MAX_PICTURE_WIDTH_SIZE + 2) * 2, EB_N_PTR);

        EB_MEMSET(contextPtr->saoUpBuffer16[0], 0, sizeof(EB_U16) * (MAX_PICTURE_WIDTH_SIZE + 2) * 2);
        contextPtr->saoUpBuffer16[0] ++;
        contextPtr->saoUpBuffer16[1] = contextPtr->saoUpBuffer16[0] + (MAX_PICTURE_WIDTH_SIZE + 2);

        //CHKN the add of 14 should be justified, also the left ping pong buffers are not symetric which is not ok
        EB_MALLOC(EB_U16 *, contextPtr->saoLeftBuffer16[0], sizeof(EB_U16) *(MAX_LCU_SIZE + 2) * 2 + 14, EB_N_PTR);

        EB_MEMSET(contextPtr->saoLeftBuffer16[0], 0, sizeof(EB_U16) *(MAX_LCU_SIZE + 2) * 2 + 14);
        contextPtr->saoLeftBuffer16[1] = contextPtr->saoLeftBuffer16[0] + (MAX_LCU_SIZE + 2);
    }

    return EB_ErrorNone;
}

/********************************************
 * ApplySaoOffsetsLcu
 *   applies SAO for the whole LCU
 ********************************************/
static EB_ERRORTYPE ApplySaoOffsetsLcu(
    PictureControlSet_t              *pictureControlSetPtr,
    EB_U32                            lcuIndex,
    EncDecContext_t                  *contextPtr,        // input parameter, DLF context Ptr, used to store the intermediate source samples
    EB_U32                            videoComponent,    // input parameter, video component, Y:0 - U:1 - V:2
    SaoParameters_t                  *saoPtr,            // input parameter, LCU Ptr
    EB_U32                            tbOriginX,
    EB_U32                            tbOriginY,
    EB_U8                            *reconSamplePtr,    // input/output parameter, picture Control Set Ptr, used to get/update recontructed samples
    EB_U32                            reconStride,       // input parameter, reconstructed stride
    EB_U32                            bitDepth,          // input parameter, sample bit depth
    EB_U32                            lcuWidth,          // input parameter, LCU width
    EB_U32                            lcuHeight,         // input parameter, LCU height
    EB_U32                            pictureWidth,      // input parameter, Picture width
    EB_U32                            pictureHeight,
    EB_U8                             pingpongIdxUp,
    EB_U8                             pingpongIdxLeft)
{
    EB_ERRORTYPE    return_error = EB_ErrorNone;
    EB_U32          lcuHeightCount;
    EB_U32          isChroma = (videoComponent == SAO_COMPONENT_LUMA) ? 0 : 1;
    EB_U8           *temporalBufferUpper;
    EB_U8           *temporalBufferLeft;
    EB_U8           tmp[64], tmpY[64];
    EB_U32          i, j;
    EB_S8           ReorderedsaoOffset[4 + 1 + 3];
    EB_S8           OrderedsaoOffsetBo[5];
    EB_BOOL         FirstColLcu, LastColLcu, FirstRowLcu, LastRowLcu;
    EncodeContext_t *encodeContextPtr;
    const EB_COLOR_FORMAT colorFormat = pictureControlSetPtr->colorFormat;
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    LargestCodingUnit_t *lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];

    FirstColLcu = lcuPtr->lcuEdgeInfoPtr->tileLeftEdgeFlag;
    LastColLcu = lcuPtr->lcuEdgeInfoPtr->tileRightEdgeFlag;
    FirstRowLcu = lcuPtr->lcuEdgeInfoPtr->tileTopEdgeFlag;
    LastRowLcu = (EB_BOOL)((tbOriginY >> (isChroma ? subHeightCMinus1:0)) + lcuHeight == pictureHeight);
    LastRowLcu = (lcuPtr->tileInfoPtr->tileLcuEndY * MAX_LCU_SIZE <= lcuPtr->originY + MAX_LCU_SIZE);

    (void)bitDepth;
    (void)pictureWidth;
    lcuHeightCount = 0;

    ReorderedsaoOffset[0] = (EB_S8)saoPtr->saoOffset[videoComponent][0];
    ReorderedsaoOffset[1] = (EB_S8)saoPtr->saoOffset[videoComponent][1];
    ReorderedsaoOffset[2] = 0;
    ReorderedsaoOffset[3] = (EB_S8)saoPtr->saoOffset[videoComponent][2];
    ReorderedsaoOffset[4] = (EB_S8)saoPtr->saoOffset[videoComponent][3];
    ReorderedsaoOffset[5] = 0;
    ReorderedsaoOffset[6] = 0;
    ReorderedsaoOffset[7] = 0;

    OrderedsaoOffsetBo[0] = (EB_S8)saoPtr->saoOffset[videoComponent][0];
    OrderedsaoOffsetBo[1] = (EB_S8)saoPtr->saoOffset[videoComponent][1];
    OrderedsaoOffsetBo[2] = (EB_S8)saoPtr->saoOffset[videoComponent][2];
    OrderedsaoOffsetBo[3] = (EB_S8)saoPtr->saoOffset[videoComponent][3];
    OrderedsaoOffsetBo[4] = 0;

    temporalBufferLeft = contextPtr->saoLeftBuffer[pingpongIdxLeft];
    temporalBufferUpper = contextPtr->saoUpBuffer[pingpongIdxUp] + (tbOriginX >> (isChroma ? subWidthCMinus1:0));

    //TODO:   get to this function only when lcuPtr->saoTypeIndex[isChroma] is not OFF
    switch (saoPtr->saoTypeIndex[isChroma]) {

    case 1: // EO - 0 degrees

        //FirstColLcu = (EB_BOOL)(tbOriginX == 0);
        //LastColLcu = (EB_BOOL)((tbOriginX >> (isChroma ? subWidthCMinus1 : 0)) + lcuWidth == pictureWidth);

        //save the non filtered first colomn if it is the first LCU in the row.
        if (FirstColLcu) {
            for (j = 0; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride];
            }
        }
        //save the non filtered last colomn if it is the last LCU in the row.
        if (LastColLcu) {
            for (j = 0; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride + lcuWidth - 1];
            }
        }


        SaoFunctionTableEO_0_90[!!(ASM_TYPES & PREAVX2_MASK)][(saoPtr->saoTypeIndex[isChroma]) - 1][((lcuHeight & 15) == 0) && ((lcuWidth & 15) == 0) && (lcuWidth >= 32)](
            reconSamplePtr,
            reconStride,
            temporalBufferLeft,
            ReorderedsaoOffset,
            lcuHeight,
            lcuWidth);

        //Restore
        if (FirstColLcu) {
            for (j = 0; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride] = tmp[j];
            }
        }
        if (LastColLcu) {
            for (j = 0; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride + lcuWidth - 1] = tmp[j];
            }
        }


        break;

    case 2: // EO - 90 degrees

        //FirstRowLcu = (EB_BOOL)(tbOriginY == 0);
        //LastRowLcu = (EB_BOOL)((tbOriginY >> (isChroma ? subHeightCMinus1:0)) + lcuHeight == pictureHeight);

        //save the non filtered first row if this LCU is on the first LCU Row
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmp[i] = reconSamplePtr[i];
            }
        }
        //save the non filtered last row if this LCU is on the last LCU Row
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmp[i] = reconSamplePtr[i + reconStride*(lcuHeight - 1)];
            }
        }
        SaoFunctionTableEO_0_90[!!(ASM_TYPES & PREAVX2_MASK)][(saoPtr->saoTypeIndex[isChroma]) - 1][((lcuHeight & 15) == 0) && ((lcuWidth & 15) == 0) && (lcuWidth >= 32)](
            reconSamplePtr,
            reconStride,
            temporalBufferUpper,
            ReorderedsaoOffset,
            lcuHeight,
            lcuWidth);

        //restore
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i] = tmp[i];
            }
        }
        //restore
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i + reconStride*(lcuHeight - 1)] = tmp[i];
            }
        }

        break;

    case 3: // EO - 135 degrees

        //FirstColLcu = (EB_BOOL)(tbOriginX == 0);
        //LastColLcu = (EB_BOOL)((tbOriginX >> (isChroma ? subWidthCMinus1 : 0)) + lcuWidth == pictureWidth);
        //FirstRowLcu = (EB_BOOL)(tbOriginY == 0);
        //LastRowLcu = (EB_BOOL)((tbOriginY >> (isChroma ? subHeightCMinus1:0)) + lcuHeight == pictureHeight);

        //save the non filtered first colomn if it is the first LCU in the row.
        if (FirstColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride];
            }
        }
        //save the non filtered last colomn if it is the last LCU in the row.
        if (LastColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride + lcuWidth - 1];
            }
        }

        //save the non filtered first row if this LCU is on the first LCU Row
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmpY[i] = reconSamplePtr[i];
            }
        }
        //save the non filtered last row if this LCU is on the last LCU Row
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmpY[i] = reconSamplePtr[i + reconStride*(lcuHeight - 1)];
            }
        }

        SaoFunctionTableEO_135_45[!!(ASM_TYPES & PREAVX2_MASK)][(saoPtr->saoTypeIndex[isChroma]) - 3][(lcuHeight == MAX_LCU_SIZE_REMAINING) && ((lcuWidth & 15) == 0) && (lcuWidth >= 32)][((lcuHeight & 15) == 0) && ((lcuWidth & 15) == 0) && (lcuWidth >= 32)](
            reconSamplePtr,
            reconStride,
            temporalBufferLeft,
            temporalBufferUpper,
            ReorderedsaoOffset,
            lcuHeight,
            lcuWidth);

        //Restore
        if (FirstColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride] = tmp[j];
            }
        }
        if (LastColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride + lcuWidth - 1] = tmp[j];
            }
        }
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i] = tmpY[i];
            }
        }
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i + reconStride*(lcuHeight - 1)] = tmpY[i];
            }
        }

        break;

    case 4: // EO - 45 degrees

        //FirstColLcu = (EB_BOOL)(tbOriginX == 0);
        //LastColLcu = (EB_BOOL)((tbOriginX >> (isChroma ? subWidthCMinus1 : 0)) + lcuWidth == pictureWidth);
        //FirstRowLcu = (EB_BOOL)(tbOriginY == 0);
        //LastRowLcu = (EB_BOOL)((tbOriginY >> (isChroma ? subHeightCMinus1:0)) + lcuHeight == pictureHeight);

        //save the non filtered first colomn if it is the first LCU in the row.
        if (FirstColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride];
            }
        }
        //save the non filtered last colomn if it is the last LCU in the row.
        if (LastColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride + lcuWidth - 1];
            }
        }

        //save the non filtered first row if this LCU is on the first LCU Row
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmpY[i] = reconSamplePtr[i];
            }
        }
        //save the non filtered last row if this LCU is on the last LCU Row
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmpY[i] = reconSamplePtr[i + reconStride*(lcuHeight - 1)];
            }
        }

        SaoFunctionTableEO_135_45[!!(ASM_TYPES & PREAVX2_MASK)][(saoPtr->saoTypeIndex[isChroma]) - 3][(lcuHeight == MAX_LCU_SIZE_REMAINING) && ((lcuWidth & 15) == 0) && (lcuWidth >= 32)][((lcuHeight & 15) == 0) && ((lcuWidth & 15) == 0) && (lcuWidth >= 32)](
            reconSamplePtr,
            reconStride,
            temporalBufferLeft,
            temporalBufferUpper,
            ReorderedsaoOffset,
            lcuHeight,
            lcuWidth);

        //Restore
        if (FirstColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride] = tmp[j];
            }
        }
        if (LastColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride + lcuWidth - 1] = tmp[j];
            }
        }
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i] = tmpY[i];
            }
        }
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i + reconStride*(lcuHeight - 1)] = tmpY[i];
            }
        }

        break;

    case 5: // BO

        SaoFunctionTableBo[!!(ASM_TYPES & PREAVX2_MASK)][((lcuWidth & 15) == 0 && (lcuHeight != MAX_LCU_SIZE_REMAINING))](
            reconSamplePtr,
            reconStride,
            saoPtr->saoBandPosition[videoComponent],
            OrderedsaoOffsetBo,
            lcuHeight,
            lcuWidth);
        break;
    default:
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr,
            EB_ENC_SAO_ERROR1);

        break;
    }
    
    return return_error;
}

/********************************************
 * ApplySaoOffsetsPicture
 *   applies SAO for the whole Picture
 ********************************************/
static EB_ERRORTYPE ApplySaoOffsetsPicture(
    EncDecContext_t         *contextPtr,             // input parameter, DLF context Ptr, used to store the intermediate source samples
    SequenceControlSet_t    *sequenceControlSetPtr,  // input parameter, Sequence control set Ptr
    PictureControlSet_t     *pictureControlSetPtr)   // input/output parameter, picture Control Set Ptr, used to get/update recontructed samples
{
    EB_ERRORTYPE    return_error = EB_ErrorNone;

    EB_U32 lcuNumberInWidth;
    EB_U32 lcuNumberInHeight;

    EB_U32 pictureWidthInLcu = (sequenceControlSetPtr->lumaWidth + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;
    EB_U32 pictureHeightInLcu = (sequenceControlSetPtr->lumaHeight + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;

    EB_U32 lcuIndex;
    EB_U32 lcuRow;
    EB_U32 lcuWidth;
    EB_U32 lcuHeight;
    EB_U32 lcuHeightPlusOne;
    EB_U32 tbOriginX;
    EB_U32 tbOriginY;

    EB_U32 reconSampleLumaIndex;
    EB_U32 reconSampleChromaIndex;
    EB_U8 *reconSampleYPtr;
    EB_U8 *reconSampleCbPtr;
    EB_U8 *reconSampleCrPtr;

    EB_U8 pingpongIdxUp = 0;
    EB_U8 pingpongIdxLeft = 0;

    SaoParameters_t *saoParams;

    EbPictureBufferDesc_t  * reconPicturePtr;
    if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
        reconPicturePtr = ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->referencePicture;
    else
        reconPicturePtr = pictureControlSetPtr->reconPicturePtr;
    const EB_COLOR_FORMAT colorFormat = reconPicturePtr->colorFormat;    // Chroma format
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
    // Apply SAO

    // Y
    if (pictureControlSetPtr->saoFlag[0]) {

        lcuIndex = 0;
        for (lcuNumberInHeight = 0; lcuNumberInHeight < pictureHeightInLcu; ++lcuNumberInHeight) { 

            for (lcuNumberInWidth = 0; lcuNumberInWidth < pictureWidthInLcu; ++lcuNumberInWidth, ++lcuIndex) {

                LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

                tbOriginX = lcuParams->originX;
                tbOriginY = lcuParams->originY;

                lcuWidth = lcuParams->width;
                lcuHeight = lcuParams->height;

                saoParams = &pictureControlSetPtr->lcuPtrArray[lcuIndex]->saoParams;
                reconSampleLumaIndex = (reconPicturePtr->originY + tbOriginY) * reconPicturePtr->strideY + reconPicturePtr->originX + tbOriginX;

                if (tbOriginX == 0) {

                    reconSampleYPtr = &(reconPicturePtr->bufferY[reconSampleLumaIndex]) + (lcuHeight - 1)*reconPicturePtr->strideY;

                    //Save last pixel row of this LCU row for next LCU row
                    EB_MEMCPY(contextPtr->saoUpBuffer[pingpongIdxUp], reconSampleYPtr, sizeof(EB_U8) * sequenceControlSetPtr->lumaWidth);

                }

                lcuHeightPlusOne = (sequenceControlSetPtr->lumaHeight == tbOriginY + lcuHeight) ? lcuHeight : lcuHeight + 1;

                //Save last pixel column of this LCU  for next LCU
                for (lcuRow = 0; lcuRow < lcuHeightPlusOne; ++lcuRow) {
                    contextPtr->saoLeftBuffer[pingpongIdxLeft][lcuRow] = reconPicturePtr->bufferY[reconSampleLumaIndex + lcuWidth - 1 + lcuRow*reconPicturePtr->strideY];
                }

                if (saoParams->saoTypeIndex[0])
                    ApplySaoOffsetsLcu(
                        pictureControlSetPtr,
                        lcuIndex,
                        contextPtr,
                        0,
                        saoParams,
                        tbOriginX,
                        tbOriginY,
                        &(reconPicturePtr->bufferY[reconSampleLumaIndex]),
                        reconPicturePtr->strideY,
                        pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->bitDepth,
                        lcuWidth,
                        lcuHeight,
                        sequenceControlSetPtr->lumaWidth,
                        sequenceControlSetPtr->lumaHeight,
                        1 - pingpongIdxUp,
                        1 - pingpongIdxLeft);

                // Toggle pingpong buffer
                pingpongIdxLeft = 1 - pingpongIdxLeft;
            }

            // Toggle pingpong buffer
            pingpongIdxUp = 1 - pingpongIdxUp;
        }
    }

    // U
    if (pictureControlSetPtr->saoFlag[1]) {

        lcuIndex = 0;
        for (lcuNumberInHeight = 0; lcuNumberInHeight < pictureHeightInLcu; ++lcuNumberInHeight) {

            for (lcuNumberInWidth = 0; lcuNumberInWidth < pictureWidthInLcu; ++lcuNumberInWidth, ++lcuIndex) {

                LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

                tbOriginX = lcuParams->originX;
                tbOriginY = lcuParams->originY;

                lcuWidth = lcuParams->width >> subWidthCMinus1;
                lcuHeight = lcuParams->height >> subHeightCMinus1;

                saoParams = &pictureControlSetPtr->lcuPtrArray[lcuIndex]->saoParams;

                reconSampleChromaIndex = ((reconPicturePtr->originX + tbOriginX) >> subWidthCMinus1) +
                    (((reconPicturePtr->originY + tbOriginY) * reconPicturePtr->strideCb) >> subHeightCMinus1);

                if (tbOriginX == 0) {

                    reconSampleCbPtr = &(reconPicturePtr->bufferCb[reconSampleChromaIndex]) + (lcuHeight - 1)*reconPicturePtr->strideCb;
                    //Save last pixel row of this LCU row for next LCU row
                    EB_MEMCPY(contextPtr->saoUpBuffer[pingpongIdxUp], reconSampleCbPtr, sizeof(EB_U8) * sequenceControlSetPtr->chromaWidth);
                }

                lcuHeightPlusOne = (sequenceControlSetPtr->chromaHeight == (tbOriginY >> subHeightCMinus1) + lcuHeight) ? lcuHeight : lcuHeight + 1;
                //Save last pixel colunm of this LCU  for next LCU
                for (lcuRow = 0; lcuRow < lcuHeightPlusOne; ++lcuRow) {
                    contextPtr->saoLeftBuffer[pingpongIdxLeft][lcuRow] = reconPicturePtr->bufferCb[reconSampleChromaIndex + lcuWidth - 1 + lcuRow*reconPicturePtr->strideCb];
                }

                if (saoParams->saoTypeIndex[1])
                    ApplySaoOffsetsLcu(
                        pictureControlSetPtr,
                        lcuIndex,
                        contextPtr,
                        1,
                        saoParams,
                        tbOriginX,
                        tbOriginY,
                        &(reconPicturePtr->bufferCb[reconSampleChromaIndex]),
                        reconPicturePtr->strideCb,
                        pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->bitDepth,
                        lcuWidth,
                        lcuHeight,
                        sequenceControlSetPtr->chromaWidth,
                        sequenceControlSetPtr->chromaHeight,
                        1 - pingpongIdxUp,
                        1 - pingpongIdxLeft);

                // Toggle pingpong buffer
                pingpongIdxLeft = 1 - pingpongIdxLeft;

            }

            // Toggle pingpong buffer
            pingpongIdxUp = 1 - pingpongIdxUp;
        }

        // V

        lcuIndex = 0;
        for (lcuNumberInHeight = 0; lcuNumberInHeight < pictureHeightInLcu; ++lcuNumberInHeight) {

            for (lcuNumberInWidth = 0; lcuNumberInWidth < pictureWidthInLcu; ++lcuNumberInWidth, ++lcuIndex) {

                LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

                tbOriginX = lcuParams->originX;
                tbOriginY = lcuParams->originY;

                lcuWidth = lcuParams->width >> subWidthCMinus1;
                lcuHeight = lcuParams->height >> subHeightCMinus1;

                saoParams = &pictureControlSetPtr->lcuPtrArray[lcuIndex]->saoParams;

                reconSampleChromaIndex = ((reconPicturePtr->originX + tbOriginX) >> subWidthCMinus1) +
                    (((reconPicturePtr->originY + tbOriginY) * reconPicturePtr->strideCr) >> subHeightCMinus1);

                if (tbOriginX == 0) {

                    reconSampleCrPtr = &(reconPicturePtr->bufferCr[reconSampleChromaIndex]) + (lcuHeight - 1)*reconPicturePtr->strideCr;

                    //Save last pixel row of this LCU row for next LCU row
                    EB_MEMCPY(contextPtr->saoUpBuffer[pingpongIdxUp], reconSampleCrPtr, sizeof(EB_U8) * sequenceControlSetPtr->chromaWidth);
                }


                lcuHeightPlusOne = (sequenceControlSetPtr->chromaHeight == (tbOriginY >> subHeightCMinus1) + lcuHeight) ? lcuHeight : lcuHeight + 1;

                //Save last pixel colunm of this LCU  for next LCU
                for (lcuRow = 0; lcuRow < lcuHeightPlusOne; ++lcuRow) {
                    contextPtr->saoLeftBuffer[pingpongIdxLeft][lcuRow] = reconPicturePtr->bufferCr[reconSampleChromaIndex + lcuWidth - 1 + lcuRow*reconPicturePtr->strideCr];
                }

                if (saoParams->saoTypeIndex[1])
                    ApplySaoOffsetsLcu(
                        pictureControlSetPtr,
                        lcuIndex,
                        contextPtr,
                        2,
                        saoParams,
                        tbOriginX,
                        tbOriginY,
                        &(reconPicturePtr->bufferCr[reconSampleChromaIndex]),
                        reconPicturePtr->strideCr,
                        pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->bitDepth,
                        lcuWidth,
                        lcuHeight,
                        sequenceControlSetPtr->chromaWidth,
                        sequenceControlSetPtr->chromaHeight,
                        1 - pingpongIdxUp,
                        1 - pingpongIdxLeft);

                //Toggle pingpong buffer
                pingpongIdxLeft = 1 - pingpongIdxLeft;

            }

            //Toggle pingpong buffer
            pingpongIdxUp = 1 - pingpongIdxUp;

        }

    }

    return return_error;
}
/********************************************
 * ApplySaoOffsetsLcu16bit
 *   applies SAO for the whole LCU
 ********************************************/
static EB_ERRORTYPE ApplySaoOffsetsLcu16bit(
    PictureControlSet_t              *pictureControlSetPtr,
    EB_U32                            lcuIndex,
    EncDecContext_t                  *contextPtr,        // input parameter, DLF context Ptr, used to store the intermediate source samples
    EB_U32                            videoComponent,    // input parameter, video component, Y:0 - U:1 - V:2
    SaoParameters_t                  *saoPtr,            // input parameter, LCU Ptr
    EB_U32                            tbOriginX,
    EB_U32                            tbOriginY,
    EB_U16                            *reconSamplePtr,    // input/output parameter, picture Control Set Ptr, used to get/update recontructed samples
    EB_U32                            reconStride,       // input parameter, reconstructed stride
    EB_U32                            bitDepth,          // input parameter, sample bit depth
    EB_U32                            lcuWidth,          // input parameter, LCU width
    EB_U32                            lcuHeight,         // input parameter, LCU height
    EB_U32                            pictureWidth,      // input parameter, Picture width
    EB_U32                            pictureHeight,
    EB_U8                             pingpongIdxUp,
    EB_U8                             pingpongIdxLeft)
{
    EB_ERRORTYPE    return_error = EB_ErrorNone;
    EB_U32          lcuHeightCount;
    EB_U32          isChroma = (videoComponent == SAO_COMPONENT_LUMA) ? 0 : 1;
    EB_U16           *temporalBufferUpper;
    EB_U16           *temporalBufferLeft;
    EB_U16           tmp[64], tmpY[64];
    EB_U32          i, j;
    EB_S8           ReorderedsaoOffset[4 + 1];
    EB_S8           OrderedsaoOffsetBo[5];
    EB_BOOL         FirstColLcu, LastColLcu, FirstRowLcu, LastRowLcu;

    const EB_COLOR_FORMAT colorFormat = pictureControlSetPtr->colorFormat;
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
    LargestCodingUnit_t *lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];

    FirstColLcu = lcuPtr->lcuEdgeInfoPtr->tileLeftEdgeFlag;
    LastColLcu = lcuPtr->lcuEdgeInfoPtr->tileRightEdgeFlag;
    FirstRowLcu = lcuPtr->lcuEdgeInfoPtr->tileTopEdgeFlag;
    LastRowLcu = (EB_BOOL)((tbOriginY >> (isChroma ? subHeightCMinus1:0)) + lcuHeight == pictureHeight);
    LastRowLcu = (lcuPtr->tileInfoPtr->tileLcuEndY * MAX_LCU_SIZE <= lcuPtr->originY + MAX_LCU_SIZE);

    (void)bitDepth;
    (void)pictureWidth;
    lcuHeightCount = 0;

    ReorderedsaoOffset[0] = (EB_S8)saoPtr->saoOffset[videoComponent][0];
    ReorderedsaoOffset[1] = (EB_S8)saoPtr->saoOffset[videoComponent][1];
    ReorderedsaoOffset[2] = 0;
    ReorderedsaoOffset[3] = (EB_S8)saoPtr->saoOffset[videoComponent][2];
    ReorderedsaoOffset[4] = (EB_S8)saoPtr->saoOffset[videoComponent][3];

    OrderedsaoOffsetBo[0] = (EB_S8)saoPtr->saoOffset[videoComponent][0];
    OrderedsaoOffsetBo[1] = (EB_S8)saoPtr->saoOffset[videoComponent][1];
    OrderedsaoOffsetBo[2] = (EB_S8)saoPtr->saoOffset[videoComponent][2];
    OrderedsaoOffsetBo[3] = (EB_S8)saoPtr->saoOffset[videoComponent][3];
    OrderedsaoOffsetBo[4] = 0;

    temporalBufferLeft = contextPtr->saoLeftBuffer16[pingpongIdxLeft];
    temporalBufferUpper = contextPtr->saoUpBuffer16[pingpongIdxUp] + (tbOriginX >> (isChroma ? subWidthCMinus1: 0));
    //TODO:   get to this function only when lcuPtr->saoTypeIndex[isChroma] is not OFF
    switch (saoPtr->saoTypeIndex[isChroma]) {
    case 1: // EO - 0 degrees


        //FirstColLcu = (EB_BOOL)(tbOriginX == 0);
        //LastColLcu = (EB_BOOL)((tbOriginX >> (isChroma ? subWidthCMinus1 : 0)) + lcuWidth == pictureWidth);

        //save the non filtered first colomn if it is the first LCU in the row.
        if (FirstColLcu) {
            for (j = 0; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride];
            }
        }
        //save the non filtered last colomn if it is the last LCU in the row.
        if (LastColLcu) {
            for (j = 0; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride + lcuWidth - 1];
            }
        }


        SaoFunctionTableEO_0_90_16bit[!!(ASM_TYPES & PREAVX2_MASK)][(saoPtr->saoTypeIndex[isChroma]) - 1][((lcuHeight & 15) == 0) && ((lcuWidth & 15) == 0) && (lcuWidth >= 32)](
            reconSamplePtr,
            reconStride,
            temporalBufferLeft,
            ReorderedsaoOffset,
            lcuHeight,
            lcuWidth);

        //Restore
        if (FirstColLcu) {
            for (j = 0; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride] = tmp[j];
            }
        }
        if (LastColLcu) {
            for (j = 0; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride + lcuWidth - 1] = tmp[j];
            }
        }


        break;

    case 2: // EO - 90 degrees

        //FirstRowLcu = (EB_BOOL)((tbOriginY == 0));
        //LastRowLcu = (EB_BOOL)(((tbOriginY >> (isChroma ? subHeightCMinus1 : 0)) + lcuHeight == pictureHeight));

        //save the non filtered first row if this LCU is on the first LCU Row
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmp[i] = reconSamplePtr[i];
            }
        }
        //save the non filtered last row if this LCU is on the last LCU Row
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmp[i] = reconSamplePtr[i + reconStride*(lcuHeight - 1)];
            }
        }

        SaoFunctionTableEO_0_90_16bit[!!(ASM_TYPES & PREAVX2_MASK)][(saoPtr->saoTypeIndex[isChroma]) - 1][((lcuHeight & 15) == 0) && ((lcuWidth & 15) == 0) && (lcuWidth >= 32)](
            reconSamplePtr,
            reconStride,
            temporalBufferUpper,
            ReorderedsaoOffset,
            lcuHeight,
            lcuWidth);

        //restore
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i] = tmp[i];
            }
        }
        //restore
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i + reconStride*(lcuHeight - 1)] = tmp[i];
            }
        }

        break;

    case 3: // EO - 135 degrees

        //FirstColLcu = (EB_BOOL)((tbOriginX == 0));
        //LastColLcu = (EB_BOOL)((tbOriginX >> (isChroma ? subWidthCMinus1 : 0)) + lcuWidth == pictureWidth);
        //FirstRowLcu = (EB_BOOL)((tbOriginY == 0));
        //LastRowLcu = (EB_BOOL)(((tbOriginY >> (isChroma ? subHeightCMinus1 : 0)) + lcuHeight == pictureHeight));

        //save the non filtered first colomn if it is the first LCU in the row.
        if (FirstColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride];
            }
        }
        //save the non filtered last colomn if it is the last LCU in the row.
        if (LastColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride + lcuWidth - 1];
            }
        }

        //save the non filtered first row if this LCU is on the first LCU Row
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmpY[i] = reconSamplePtr[i];
            }
        }
        //save the non filtered last row if this LCU is on the last LCU Row
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmpY[i] = reconSamplePtr[i + reconStride*(lcuHeight - 1)];
            }
        }

        SaoFunctionTableEO_135_45_16bit[!!(ASM_TYPES & PREAVX2_MASK)][(saoPtr->saoTypeIndex[isChroma]) - 3][((lcuWidth & 15) == 0) && (lcuWidth >= 32) && ((lcuHeight & 7) == 0) && (lcuHeight >= 8)](
            reconSamplePtr,
            reconStride,
            temporalBufferLeft,
            temporalBufferUpper,
            ReorderedsaoOffset,
            lcuHeight,
            lcuWidth);

        //Restore
        if (FirstColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride] = tmp[j];
            }
        }
        if (LastColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride + lcuWidth - 1] = tmp[j];
            }
        }
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i] = tmpY[i];
            }
        }
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i + reconStride*(lcuHeight - 1)] = tmpY[i];
            }
        }

        break;

    case 4: // EO - 45 degrees

        //FirstColLcu = (EB_BOOL)((tbOriginX == 0));
        //LastColLcu = (EB_BOOL)((tbOriginX >> (isChroma ? subWidthCMinus1 : 0)) + lcuWidth == pictureWidth);
        //FirstRowLcu = (EB_BOOL)((tbOriginY == 0));
        //LastRowLcu = (EB_BOOL)(((tbOriginY >> (isChroma ? subHeightCMinus1 : 0)) + lcuHeight == pictureHeight));

        //save the non filtered first colomn if it is the first LCU in the row.
        if (FirstColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride];
            }
        }
        //save the non filtered last colomn if it is the last LCU in the row.
        if (LastColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                tmp[j] = reconSamplePtr[j*reconStride + lcuWidth - 1];
            }
        }

        //save the non filtered first row if this LCU is on the first LCU Row
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmpY[i] = reconSamplePtr[i];
            }
        }
        //save the non filtered last row if this LCU is on the last LCU Row
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                tmpY[i] = reconSamplePtr[i + reconStride*(lcuHeight - 1)];
            }
        }

        SaoFunctionTableEO_135_45_16bit[!!(ASM_TYPES & PREAVX2_MASK)][(saoPtr->saoTypeIndex[isChroma]) - 3][((lcuWidth & 15) == 0) && (lcuWidth >= 32) && ((lcuHeight & 7) == 0) && (lcuHeight >= 8)](
            reconSamplePtr,
            reconStride,
            temporalBufferLeft,
            temporalBufferUpper,
            ReorderedsaoOffset,
            lcuHeight,
            lcuWidth);

        //Restore
        if (FirstColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride] = tmp[j];
            }
        }
        if (LastColLcu) {
            for (j = lcuHeightCount; j < lcuHeight; j++) {
                reconSamplePtr[j*reconStride + lcuWidth - 1] = tmp[j];
            }
        }
        if (FirstRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i] = tmpY[i];
            }
        }
        if (LastRowLcu) {
            for (i = 0; i < lcuWidth; i++) {
                reconSamplePtr[i + reconStride*(lcuHeight - 1)] = tmpY[i];
            }
        }

        break;

    case 5: // BO

        SaoFunctionTableBo_16bit[!!(ASM_TYPES & PREAVX2_MASK)][((lcuWidth & 15) == 0)](
            reconSamplePtr,
            reconStride,
            saoPtr->saoBandPosition[videoComponent],
            OrderedsaoOffsetBo,
            lcuHeight,
            lcuWidth);

        break;

    default:
        CHECK_REPORT_ERROR_NC(
            ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr->appCallbackPtr,
            EB_ENC_SAO_ERROR1);
        break;
    }
    
    return return_error;
}
/********************************************
 * ApplySaoOffsetsPicture16bit
 *   applies SAO for the whole Picture in 16bit mode
 ********************************************/
static EB_ERRORTYPE ApplySaoOffsetsPicture16bit(
    EncDecContext_t                *contextPtr,             // input parameter, DLF context Ptr, used to store the intermediate source samples
    SequenceControlSet_t           *sequenceControlSetPtr,  // input parameter, Sequence control set Ptr
    PictureControlSet_t            *pictureControlSetPtr)   // input/output parameter, picture Control Set Ptr, used to get/update recontructed samples
{
    EB_ERRORTYPE    return_error = EB_ErrorNone;

    EB_U32 lcuNumberInWidth;
    EB_U32 lcuNumberInHeight;

    EB_U32 pictureWidthInLcu = (sequenceControlSetPtr->lumaWidth + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;
    EB_U32 pictureHeightInLcu = (sequenceControlSetPtr->lumaHeight + sequenceControlSetPtr->lcuSize - 1) / sequenceControlSetPtr->lcuSize;

    EB_U32 lcuIndex;
    EB_U32 lcuRow;
    EB_U32 lcuWidth;
    EB_U32 lcuHeight;
    EB_U32 lcuHeightPlusOne;
    EB_U32 tbOriginX;
    EB_U32 tbOriginY;

    EB_U32 reconSampleLumaIndex;
    EB_U32 reconSampleChromaIndex;
    EB_U16 *reconSampleYPtr;
    EB_U16 *reconSampleCbPtr;
    EB_U16 *reconSampleCrPtr;

    EB_U8 pingpongIdxUp = 0;
    EB_U8 pingpongIdxLeft = 0;

    SaoParameters_t *saoParams;
    EbPictureBufferDesc_t  * recBuf16bit;
    if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
        recBuf16bit = ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->referencePicture16bit;
    else
        recBuf16bit = pictureControlSetPtr->reconPicture16bitPtr;
    const EB_COLOR_FORMAT colorFormat = recBuf16bit->colorFormat;
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
    // Apply SAO
    // Y
    if (pictureControlSetPtr->saoFlag[0]) {

        lcuIndex = 0;
        for (lcuNumberInHeight = 0; lcuNumberInHeight < pictureHeightInLcu; ++lcuNumberInHeight) {

            for (lcuNumberInWidth = 0; lcuNumberInWidth < pictureWidthInLcu; ++lcuNumberInWidth, ++lcuIndex) {

                LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

                tbOriginX = lcuParams->originX;
                tbOriginY = lcuParams->originY;

                lcuWidth = lcuParams->width;
                lcuHeight = lcuParams->height;

                saoParams = &pictureControlSetPtr->lcuPtrArray[lcuIndex]->saoParams;
                reconSampleLumaIndex = (recBuf16bit->originY + tbOriginY) * recBuf16bit->strideY + recBuf16bit->originX + tbOriginX;

                if (tbOriginX == 0) {

                    reconSampleYPtr = (EB_U16*)(recBuf16bit->bufferY) + reconSampleLumaIndex + (lcuHeight - 1)*recBuf16bit->strideY;
                    //Save last pixel row of this LCU row for next LCU row
                    memcpy16bit(contextPtr->saoUpBuffer16[pingpongIdxUp], reconSampleYPtr, sequenceControlSetPtr->lumaWidth);

                }

                //Save last pixel colunm of this LCU  for next LCU
                lcuHeightPlusOne = (sequenceControlSetPtr->lumaHeight == tbOriginY + lcuHeight) ? lcuHeight : lcuHeight + 1;
                reconSampleYPtr = (EB_U16*)(recBuf16bit->bufferY) + reconSampleLumaIndex + lcuWidth - 1;

                for (lcuRow = 0; lcuRow < lcuHeightPlusOne; ++lcuRow) {
                    contextPtr->saoLeftBuffer16[pingpongIdxLeft][lcuRow] = reconSampleYPtr[lcuRow*recBuf16bit->strideY];
                }


                if (saoParams->saoTypeIndex[0])
                    ApplySaoOffsetsLcu16bit(
                        pictureControlSetPtr,
                        lcuIndex,
                        contextPtr,
                        0,
                        saoParams,
                        tbOriginX,
                        tbOriginY,
                        (EB_U16*)recBuf16bit->bufferY + reconSampleLumaIndex,
                        recBuf16bit->strideY,
                        pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->bitDepth,
                        lcuWidth,
                        lcuHeight,
                        sequenceControlSetPtr->lumaWidth,
                        sequenceControlSetPtr->lumaHeight,
                        1 - pingpongIdxUp,
                        1 - pingpongIdxLeft);

                //Toggle pingpong buffer
                pingpongIdxLeft = 1 - pingpongIdxLeft;

            }

            //Toggle pingpong buffer
            pingpongIdxUp = 1 - pingpongIdxUp;

        }
    }

    //TODO: U+V loop could be merged in one loop
    // U
    if (pictureControlSetPtr->saoFlag[1]) {

        lcuIndex = 0;
        for (lcuNumberInHeight = 0; lcuNumberInHeight < pictureHeightInLcu; ++lcuNumberInHeight) {

            for (lcuNumberInWidth = 0; lcuNumberInWidth < pictureWidthInLcu; ++lcuNumberInWidth, ++lcuIndex) {

                LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

                tbOriginX = lcuParams->originX;
                tbOriginY = lcuParams->originY;

                lcuWidth = lcuParams->width >> subWidthCMinus1;
                lcuHeight = lcuParams->height >> subHeightCMinus1;

                saoParams = &pictureControlSetPtr->lcuPtrArray[lcuIndex]->saoParams;
                reconSampleChromaIndex = ((recBuf16bit->originX + tbOriginX) >> subWidthCMinus1) +
                    (((recBuf16bit->originY + tbOriginY) * recBuf16bit->strideCb) >> subHeightCMinus1);

                if (tbOriginX == 0) {

                    reconSampleCbPtr = (EB_U16*)(recBuf16bit->bufferCb) + reconSampleChromaIndex + (lcuHeight - 1)*recBuf16bit->strideCb;

                    //Save last pixel row of this LCU row for next LCU row
                    memcpy16bit(contextPtr->saoUpBuffer16[pingpongIdxUp], reconSampleCbPtr, sequenceControlSetPtr->chromaWidth);
                }

                //Save last pixel colunm of this LCU  for next LCU
                lcuHeightPlusOne = (sequenceControlSetPtr->chromaHeight == (tbOriginY >> subHeightCMinus1) + lcuHeight) ? lcuHeight : lcuHeight + 1;
                reconSampleCbPtr = (EB_U16*)(recBuf16bit->bufferCb) + reconSampleChromaIndex + lcuWidth - 1;

                for (lcuRow = 0; lcuRow < lcuHeightPlusOne; ++lcuRow) {
                    contextPtr->saoLeftBuffer16[pingpongIdxLeft][lcuRow] = reconSampleCbPtr[lcuRow*recBuf16bit->strideCb];
                }

                if (saoParams->saoTypeIndex[1])
                    ApplySaoOffsetsLcu16bit(
                        pictureControlSetPtr,
                        lcuIndex,
                        contextPtr,
                        1,
                        saoParams,
                        tbOriginX,
                        tbOriginY,
                        (EB_U16*)recBuf16bit->bufferCb + reconSampleChromaIndex,
                        recBuf16bit->strideCb,
                        pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->bitDepth,
                        lcuWidth,
                        lcuHeight,
                        sequenceControlSetPtr->chromaWidth,
                        sequenceControlSetPtr->chromaHeight,
                        1 - pingpongIdxUp,
                        1 - pingpongIdxLeft);


                // Toggle pingpong buffer
                pingpongIdxLeft = 1 - pingpongIdxLeft;

            }

            // Toggle pingpong buffer
            pingpongIdxUp = 1 - pingpongIdxUp;

        }

        // V
        lcuIndex = 0;
        for (lcuNumberInHeight = 0; lcuNumberInHeight < pictureHeightInLcu; ++lcuNumberInHeight) {

            for (lcuNumberInWidth = 0; lcuNumberInWidth < pictureWidthInLcu; ++lcuNumberInWidth, ++lcuIndex) {

                LcuParams_t *lcuParams = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

                tbOriginX = lcuParams->originX;
                tbOriginY = lcuParams->originY;

                lcuWidth = lcuParams->width >> subWidthCMinus1;
                lcuHeight = lcuParams->height >> subHeightCMinus1;

                saoParams = &pictureControlSetPtr->lcuPtrArray[lcuIndex]->saoParams;

                reconSampleChromaIndex = ((recBuf16bit->originX + tbOriginX) >> subWidthCMinus1) +
                    (((recBuf16bit->originY + tbOriginY) * recBuf16bit->strideCr) >> subHeightCMinus1);

                if (tbOriginX == 0) {

                    reconSampleCrPtr = (EB_U16*)(recBuf16bit->bufferCr) + reconSampleChromaIndex + (lcuHeight - 1)*recBuf16bit->strideCr;
                    //Save last pixel row of this LCU row for next LCU row
                    memcpy16bit(contextPtr->saoUpBuffer16[pingpongIdxUp], reconSampleCrPtr, sequenceControlSetPtr->chromaWidth);
                }

                //Save last pixel colunm of this LCU  for next LCU
                lcuHeightPlusOne = (sequenceControlSetPtr->chromaHeight == (tbOriginY >> subHeightCMinus1) + lcuHeight) ? lcuHeight : lcuHeight + 1;
                reconSampleCrPtr = (EB_U16*)(recBuf16bit->bufferCr) + reconSampleChromaIndex + lcuWidth - 1;

                for (lcuRow = 0; lcuRow < lcuHeightPlusOne; ++lcuRow) {
                    contextPtr->saoLeftBuffer16[pingpongIdxLeft][lcuRow] = reconSampleCrPtr[lcuRow*recBuf16bit->strideCr];
                }

                if (saoParams->saoTypeIndex[1])
                    ApplySaoOffsetsLcu16bit(
                        pictureControlSetPtr,
                        lcuIndex,
                        contextPtr,
                        2,
                        saoParams,
                        tbOriginX,
                        tbOriginY,
                        (EB_U16*)recBuf16bit->bufferCr + reconSampleChromaIndex,
                        recBuf16bit->strideCr,
                        pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr->bitDepth,
                        lcuWidth,
                        lcuHeight,
                        sequenceControlSetPtr->chromaWidth,
                        sequenceControlSetPtr->chromaHeight,
                        1 - pingpongIdxUp,
                        1 - pingpongIdxLeft);

                // Toggle pingpong buffer
                pingpongIdxLeft = 1 - pingpongIdxLeft;

            }

            // Toggle pingpong buffer
            pingpongIdxUp = 1 - pingpongIdxUp;

        }
    }

    return return_error;
}


/**************************************************
 * Reset Mode Decision Neighbor Arrays
 *************************************************/
static void ResetEncodePassNeighborArrays(PictureControlSet_t *pictureControlSetPtr, unsigned tileIdx)
{
    NeighborArrayUnitReset(pictureControlSetPtr->epIntraLumaModeNeighborArray[tileIdx]);
    NeighborArrayUnitReset(pictureControlSetPtr->epMvNeighborArray[tileIdx]);
    NeighborArrayUnitReset(pictureControlSetPtr->epSkipFlagNeighborArray[tileIdx]);
    NeighborArrayUnitReset(pictureControlSetPtr->epModeTypeNeighborArray[tileIdx]);
    NeighborArrayUnitReset(pictureControlSetPtr->epLeafDepthNeighborArray[tileIdx]);
    NeighborArrayUnitReset(pictureControlSetPtr->epLumaReconNeighborArray[tileIdx]);
    NeighborArrayUnitReset(pictureControlSetPtr->epCbReconNeighborArray[tileIdx]);
    NeighborArrayUnitReset(pictureControlSetPtr->epCrReconNeighborArray[tileIdx]);
    NeighborArrayUnitReset(pictureControlSetPtr->epSaoNeighborArray[tileIdx]);
    return;
}

/**************************************************
 * Reset Coding Loop
 **************************************************/
static void ResetEncDec(
    EncDecContext_t         *contextPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    SequenceControlSet_t    *sequenceControlSetPtr,
    EB_U32                   segmentIndex)
{
    EB_PICTURE                     sliceType;
    MdRateEstimationContext_t   *mdRateEstimationArray;
    EB_U32                       entropyCodingQp;

    contextPtr->is16bit = (EB_BOOL)(sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);

    // SAO
    pictureControlSetPtr->saoFlag[0] = EB_TRUE;
    pictureControlSetPtr->saoFlag[1] = EB_TRUE;

    // QP
    contextPtr->qp = pictureControlSetPtr->pictureQp;
    // Asuming cb and cr offset to be the same for chroma QP in both slice and pps for lambda computation 

    EB_U8	qpScaled = CLIP3(MIN_QP_VALUE, MAX_CHROMA_MAP_QP_VALUE, (EB_S32)(contextPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
    contextPtr->chromaQp = (EB_U8)MapChromaQp(qpScaled);

    // Lambda Assignement
    if (pictureControlSetPtr->sliceType == EB_I_PICTURE && pictureControlSetPtr->temporalId == 0){

        (*lambdaAssignmentFunctionTable[3])(
            pictureControlSetPtr->ParentPcsPtr,
            &contextPtr->fastLambda,
            &contextPtr->fullLambda,
            &contextPtr->fastChromaLambda,
            &contextPtr->fullChromaLambda,
            &contextPtr->fullChromaLambdaSao,
            contextPtr->qp,
            contextPtr->chromaQp);
    }
    else{
        (*lambdaAssignmentFunctionTable[pictureControlSetPtr->ParentPcsPtr->predStructure])(
            pictureControlSetPtr->ParentPcsPtr,
            &contextPtr->fastLambda,
            &contextPtr->fullLambda,
            &contextPtr->fastChromaLambda,
            &contextPtr->fullChromaLambda,
            &contextPtr->fullChromaLambdaSao,
            contextPtr->qp,
            contextPtr->chromaQp);
    }

    // Slice Type
    sliceType =
        (pictureControlSetPtr->ParentPcsPtr->idrFlag == EB_TRUE) ? EB_I_PICTURE :
        pictureControlSetPtr->sliceType;

    // Increment the MD Rate Estimation array pointer to point to the right address based on the QP and slice type
    mdRateEstimationArray = (MdRateEstimationContext_t*)sequenceControlSetPtr->encodeContextPtr->mdRateEstimationArray;
    mdRateEstimationArray += sliceType * TOTAL_NUMBER_OF_QP_VALUES + contextPtr->qp;

    // Reset MD rate Estimation table to initial values by copying from mdRateEstimationArray

    contextPtr->mdRateEstimationPtr = mdRateEstimationArray;

    // TMVP Map Writer Pointer
    if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
        contextPtr->referenceObjectWritePtr = (EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr;
    else
        contextPtr->referenceObjectWritePtr = (EbReferenceObject_t*)EB_NULL;

    if (pictureControlSetPtr->useDeltaQp) {
        entropyCodingQp = pictureControlSetPtr->pictureQp;
    }
    else {
        entropyCodingQp = pictureControlSetPtr->pictureQp;
    }

    if (segmentIndex == 0) {
        //Jing: Double check the entropy context here
        //      Not good here, better to use a mutex, but should be OK since input from MDC is FIFO
        if (contextPtr->tileRowIndex == 0) {
            // Reset CABAC Contexts
            ResetEntropyCoder(
                    sequenceControlSetPtr->encodeContextPtr,
                    pictureControlSetPtr->coeffEstEntropyCoderPtr,
                    entropyCodingQp,
                    pictureControlSetPtr->sliceType);

            //this function could be optimized by removed chroma, and unessary TU sizes.
            PrecomputeCabacCost(&(*pictureControlSetPtr->cabacCost),
                    (CabacEncodeContext_t*)pictureControlSetPtr->coeffEstEntropyCoderPtr->cabacEncodeContextPtr);
        }


        for (int tileIdx = contextPtr->tileRowIndex * pictureControlSetPtr->ParentPcsPtr->tileColumnCount;
                tileIdx < (contextPtr->tileRowIndex + 1) * pictureControlSetPtr->ParentPcsPtr->tileColumnCount;
                tileIdx++) {
            ResetEncodePassNeighborArrays(pictureControlSetPtr, tileIdx);
        }
    }

    if (contextPtr->mdContext->coeffCabacUpdate)
    {
        ContextModelEncContext_t  *cabacCtxModelArray = (ContextModelEncContext_t*)sequenceControlSetPtr->encodeContextPtr->cabacContextModelArray;
        // Increment the context model array pointer to point to the right address based on the QP and slice type 
        cabacCtxModelArray += pictureControlSetPtr->sliceType * TOTAL_NUMBER_OF_QP_VALUES + entropyCodingQp;

        //LatestValid  <--  init
        EB_MEMCPY(&(contextPtr->mdContext->latestValidCoeffCtxModel), &(cabacCtxModelArray->lastSigXContextModel[0]), sizeof(CoeffCtxtMdl_t));
    }

    return;
}

/******************************************************
 * EncDec Configure LCU
 ******************************************************/
static void EncDecConfigureLcu(
    EncDecContext_t         *contextPtr,
    LargestCodingUnit_t     *lcuPtr,
    PictureControlSet_t     *pictureControlSetPtr,
    SequenceControlSet_t    *sequenceControlSetPtr,
    EB_U8                    pictureQp,
    EB_U8                    lcuQp)
{

    //RC is off
    if (sequenceControlSetPtr->staticConfig.rateControlMode == 0 && sequenceControlSetPtr->staticConfig.improveSharpness == 0 && sequenceControlSetPtr->staticConfig.bitRateReduction == 0) {
        contextPtr->qp = pictureQp;
    }
    //RC is on
    else {
        contextPtr->qp = lcuQp;
    }

    // Asuming cb and cr offset to be the same for chroma QP in both slice and pps for lambda computation
    EB_U8	qpScaled = CLIP3((EB_S8)MIN_QP_VALUE, (EB_S8)MAX_CHROMA_MAP_QP_VALUE, (EB_S8)(contextPtr->qp + pictureControlSetPtr->cbQpOffset + pictureControlSetPtr->sliceCbQpOffset));
    contextPtr->chromaQp = MapChromaQp(qpScaled);

    /* Note(CHKN) : when Qp modulation varies QP on a sub-LCU(CU) basis,  Lamda has to change based on Cu->QP , and then this code has to move inside the CU loop in MD */
    (void)lcuPtr;

    // Lambda Assignement
    if (pictureControlSetPtr->sliceType == EB_I_PICTURE && pictureControlSetPtr->temporalId == 0){

        (*lambdaAssignmentFunctionTable[3])(
            pictureControlSetPtr->ParentPcsPtr,
            &contextPtr->fastLambda,
            &contextPtr->fullLambda,
            &contextPtr->fastChromaLambda,
            &contextPtr->fullChromaLambda,
            &contextPtr->fullChromaLambdaSao,
            contextPtr->qp,
            contextPtr->chromaQp);
    }
    else{
        (*lambdaAssignmentFunctionTable[pictureControlSetPtr->ParentPcsPtr->predStructure])(
            pictureControlSetPtr->ParentPcsPtr,
            &contextPtr->fastLambda,
            &contextPtr->fullLambda,
            &contextPtr->fastChromaLambda,
            &contextPtr->fullChromaLambda,
            &contextPtr->fullChromaLambdaSao,
            contextPtr->qp,
            contextPtr->chromaQp);
    }

    return;
}

/******************************************************
 * Update MD Segments
 *
 * This function is responsible for synchronizing the
 *   processing of MD Segment-rows.
 *   In short, the function starts processing
 *   of MD segment-rows as soon as their inputs are available
 *   and the previous segment-row has completed.  At
 *   any given time, only one segment row per picture
 *   is being processed.
 *
 * The function has two functions:
 *
 * (1) Update the Segment Completion Mask which tracks
 *   which MD Segment inputs are available.
 *
 * (2) Increment the segment-row counter (currentRowIdx)
 *   as the segment-rows are completed.
 *
 * Since there is the potentential for thread collusion,
 *   a MUTEX a used to protect the sensitive data and
 *   the execution flow is separated into two paths
 *
 * (A) Initial update.
 *  -Update the Completion Mask [see (1) above]
 *  -If the picture is not currently being processed,
 *     check to see if the next segment-row is available
 *     and start processing.
 * (B) Continued processing
 *  -Upon the completion of a segment-row, check
 *     to see if the next segment-row's inputs have
 *     become available and begin processing if so.
 *
 * On last important point is that the thread-safe
 *   code section is kept minimally short. The MUTEX
 *   should NOT be locked for the entire processing
 *   of the segment-row (B) as this would block other
 *   threads from performing an update (A).
 ******************************************************/
EB_BOOL AssignEncDecSegments(
    EncDecSegments_t   *segmentPtr,
    EB_U16             *segmentInOutIndex,
    EncDecTasks_t      *taskPtr,
    EbFifo_t           *srmFifoPtr)
{
    EB_BOOL continueProcessingFlag = EB_FALSE;
    EbObjectWrapper_t *wrapperPtr;
    EncDecTasks_t *feedbackTaskPtr;

    EB_U32 rowSegmentIndex = 0;
    EB_U32 segmentIndex = 0;
    EB_U32 rightSegmentIndex;
    EB_U32 bottomLeftSegmentIndex;

    EB_S16 feedbackRowIndex = -1;

    EB_U32 selfAssigned = EB_FALSE;

    //static FILE *trace = 0;
    //
    //if(trace == 0) {
    //    trace = fopen("seg-trace.txt","w");
    //}


    switch (taskPtr->inputType) {

    case ENCDEC_TASKS_MDC_INPUT:

        // The entire picture is provided by the MDC process, so
        //   no logic is necessary to clear input dependencies.

        // Start on Segment 0 immediately 
        *segmentInOutIndex = segmentPtr->rowArray[0].currentSegIndex;
        taskPtr->inputType = ENCDEC_TASKS_CONTINUE;
        ++segmentPtr->rowArray[0].currentSegIndex;
        continueProcessingFlag = EB_TRUE;

        //fprintf(trace, "Start  Pic: %u Seg: %u\n",
        //    (unsigned) ((PictureControlSet_t*) taskPtr->pictureControlSetWrapperPtr->objectPtr)->pictureNumber,
        //    *segmentInOutIndex);

        break;

    case ENCDEC_TASKS_ENCDEC_INPUT:

        // Setup rowSegmentIndex to release the inProgress token
        //rowSegmentIndex = taskPtr->encDecSegmentRowArray[0];

        // Start on the assigned row immediately
        *segmentInOutIndex = segmentPtr->rowArray[taskPtr->encDecSegmentRow].currentSegIndex;
        taskPtr->inputType = ENCDEC_TASKS_CONTINUE;
        ++segmentPtr->rowArray[taskPtr->encDecSegmentRow].currentSegIndex;
        continueProcessingFlag = EB_TRUE;

        //fprintf(trace, "Start  Pic: %u Seg: %u\n",
        //    (unsigned) ((PictureControlSet_t*) taskPtr->pictureControlSetWrapperPtr->objectPtr)->pictureNumber,
        //    *segmentInOutIndex);

        break;

    case ENCDEC_TASKS_CONTINUE:

        // Update the Dependency List for Right and Bottom Neighbors
        segmentIndex = *segmentInOutIndex;
        rowSegmentIndex = segmentIndex / segmentPtr->segmentBandCount;

        rightSegmentIndex = segmentIndex + 1;
        bottomLeftSegmentIndex = segmentIndex + segmentPtr->segmentBandCount;

        // Right Neighbor
        if (segmentIndex < segmentPtr->rowArray[rowSegmentIndex].endingSegIndex)
        {
            EbBlockOnMutex(segmentPtr->rowArray[rowSegmentIndex].assignmentMutex);

            --segmentPtr->depMap.dependencyMap[rightSegmentIndex];

            if (segmentPtr->depMap.dependencyMap[rightSegmentIndex] == 0) {
                *segmentInOutIndex = segmentPtr->rowArray[rowSegmentIndex].currentSegIndex;
                ++segmentPtr->rowArray[rowSegmentIndex].currentSegIndex;
                selfAssigned = EB_TRUE;
                continueProcessingFlag = EB_TRUE;

                //fprintf(trace, "Start  Pic: %u Seg: %u\n",
                //    (unsigned) ((PictureControlSet_t*) taskPtr->pictureControlSetWrapperPtr->objectPtr)->pictureNumber,
                //    *segmentInOutIndex);
            }

            EbReleaseMutex(segmentPtr->rowArray[rowSegmentIndex].assignmentMutex);
        }

        // Bottom-left Neighbor
        if (rowSegmentIndex < segmentPtr->segmentRowCount - 1 && bottomLeftSegmentIndex >= segmentPtr->rowArray[rowSegmentIndex + 1].startingSegIndex)
        {
            EbBlockOnMutex(segmentPtr->rowArray[rowSegmentIndex + 1].assignmentMutex);

            --segmentPtr->depMap.dependencyMap[bottomLeftSegmentIndex];

            if (segmentPtr->depMap.dependencyMap[bottomLeftSegmentIndex] == 0) {
                if (selfAssigned == EB_TRUE) {
                    feedbackRowIndex = (EB_S16)rowSegmentIndex + 1;
                }
                else {
                    *segmentInOutIndex = segmentPtr->rowArray[rowSegmentIndex + 1].currentSegIndex;
                    ++segmentPtr->rowArray[rowSegmentIndex + 1].currentSegIndex;
                    selfAssigned = EB_TRUE;
                    continueProcessingFlag = EB_TRUE;

                    //fprintf(trace, "Start  Pic: %u Seg: %u\n",
                    //    (unsigned) ((PictureControlSet_t*) taskPtr->pictureControlSetWrapperPtr->objectPtr)->pictureNumber,
                    //    *segmentInOutIndex);
                }
            }
            EbReleaseMutex(segmentPtr->rowArray[rowSegmentIndex + 1].assignmentMutex);
        }

        if (feedbackRowIndex > 0) {
            EbGetEmptyObject(
                srmFifoPtr,
                &wrapperPtr);
            feedbackTaskPtr = (EncDecTasks_t*)wrapperPtr->objectPtr;
            feedbackTaskPtr->inputType = ENCDEC_TASKS_ENCDEC_INPUT;
            feedbackTaskPtr->encDecSegmentRow = feedbackRowIndex;
            feedbackTaskPtr->pictureControlSetWrapperPtr = taskPtr->pictureControlSetWrapperPtr;
            feedbackTaskPtr->tileRowIndex = taskPtr->tileRowIndex;

            EbPostFullObject(wrapperPtr);
        }

        break;

    default:
        break;
    }

    return continueProcessingFlag;
}
static void ReconOutput(
    PictureControlSet_t    *pictureControlSetPtr,
    SequenceControlSet_t   *sequenceControlSetPtr)
{
    EbObjectWrapper_t             *outputReconWrapperPtr;
    EB_BUFFERHEADERTYPE           *outputReconPtr;
    EncodeContext_t               *encodeContextPtr = sequenceControlSetPtr->encodeContextPtr;
    EB_BOOL is16bit = (sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);
    const EB_COLOR_FORMAT colorFormat = (EB_COLOR_FORMAT)sequenceControlSetPtr->chromaFormatIdc;
    const EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    // Get Recon Buffer
    EbGetEmptyObject( 
        sequenceControlSetPtr->encodeContextPtr->reconOutputFifoPtr,
        &outputReconWrapperPtr);
    outputReconPtr = (EB_BUFFERHEADERTYPE*)outputReconWrapperPtr->objectPtr;
    outputReconPtr->nFlags = 0;

    // STOP READ/WRITE PROTECTED SECTION
    outputReconPtr->nFilledLen = 0;

    // Copy the Reconstructed Picture to the Output Recon Buffer
    {
        EB_U32 sampleTotalCount;
        EB_U8 *reconReadPtr;
        EB_U8 *reconWritePtr;

        EbPictureBufferDesc_t *reconPtr;
        {
            if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
                reconPtr = is16bit ?
                ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->referencePicture16bit :
                ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->referencePicture;
            else {
                if (is16bit)
                    reconPtr = pictureControlSetPtr->reconPicture16bitPtr;
                else
                    reconPtr = pictureControlSetPtr->reconPicturePtr;
            }
        }

        // Y Recon Samples
        sampleTotalCount = ((reconPtr->maxWidth - sequenceControlSetPtr->maxInputPadRight) * (reconPtr->maxHeight - sequenceControlSetPtr->maxInputPadBottom)) << is16bit;
        reconReadPtr = reconPtr->bufferY + (reconPtr->originY << is16bit) * reconPtr->strideY + (reconPtr->originX << is16bit);
        reconWritePtr = &(outputReconPtr->pBuffer[outputReconPtr->nFilledLen]);

        CHECK_REPORT_ERROR(
            (outputReconPtr->nFilledLen + sampleTotalCount <= outputReconPtr->nAllocLen),
            encodeContextPtr->appCallbackPtr,
            EB_ENC_ROB_OF_ERROR);

        // Initialize Y recon buffer
        PictureCopyKernel(
            reconReadPtr,
            reconPtr->strideY,
            reconWritePtr,
            reconPtr->maxWidth - sequenceControlSetPtr->maxInputPadRight,
            reconPtr->width - sequenceControlSetPtr->padRight,
            reconPtr->height - sequenceControlSetPtr->padBottom,
            1 << is16bit);

        outputReconPtr->nFilledLen += sampleTotalCount;

        // U Recon Samples
        sampleTotalCount = ((reconPtr->maxWidth - sequenceControlSetPtr->maxInputPadRight) * (reconPtr->maxHeight - sequenceControlSetPtr->maxInputPadBottom) >> (3 - colorFormat)) << is16bit;
        reconReadPtr = reconPtr->bufferCb + ((reconPtr->originX << is16bit) >> subWidthCMinus1) +
            ((reconPtr->originY << is16bit) >> subHeightCMinus1) * reconPtr->strideCb;
        reconWritePtr = &(outputReconPtr->pBuffer[outputReconPtr->nFilledLen]);

        CHECK_REPORT_ERROR(
            (outputReconPtr->nFilledLen + sampleTotalCount <= outputReconPtr->nAllocLen),
            encodeContextPtr->appCallbackPtr,
            EB_ENC_ROB_OF_ERROR);

        // Initialize U recon buffer
        PictureCopyKernel(
            reconReadPtr,
            reconPtr->strideCb,
            reconWritePtr,
            (reconPtr->maxWidth - sequenceControlSetPtr->maxInputPadRight) >> subWidthCMinus1,
            (reconPtr->width - sequenceControlSetPtr->padRight) >> subWidthCMinus1,
            (reconPtr->height - sequenceControlSetPtr->padBottom) >> subHeightCMinus1,
            1 << is16bit);
        outputReconPtr->nFilledLen += sampleTotalCount;

        // V Recon Samples
        sampleTotalCount = ((reconPtr->maxWidth - sequenceControlSetPtr->maxInputPadRight) * (reconPtr->maxHeight - sequenceControlSetPtr->maxInputPadBottom) >> (3 - colorFormat)) << is16bit;
        reconReadPtr = reconPtr->bufferCr + ((reconPtr->originX << is16bit) >> subWidthCMinus1) +
            ((reconPtr->originY << is16bit) >> subHeightCMinus1) * reconPtr->strideCr;
        reconWritePtr = &(outputReconPtr->pBuffer[outputReconPtr->nFilledLen]);

        CHECK_REPORT_ERROR(
            (outputReconPtr->nFilledLen + sampleTotalCount <= outputReconPtr->nAllocLen),
            encodeContextPtr->appCallbackPtr,
            EB_ENC_ROB_OF_ERROR);

        // Initialize V recon buffer

        PictureCopyKernel(
            reconReadPtr,
            reconPtr->strideCr,
            reconWritePtr,
            (reconPtr->maxWidth - sequenceControlSetPtr->maxInputPadRight) >> subWidthCMinus1,
            (reconPtr->width - sequenceControlSetPtr->padRight) >> subWidthCMinus1,
            (reconPtr->height - sequenceControlSetPtr->padBottom) >> subHeightCMinus1,
            1 << is16bit);
        outputReconPtr->nFilledLen += sampleTotalCount;
        outputReconPtr->pts = pictureControlSetPtr->pictureNumber;
    }

    // The totalNumberOfReconFrames counter has to be write/read protected as
    //   it is used to determine the end of the stream.  If it is not protected
    //   the encoder might not properly terminate.
    EbBlockOnMutex(encodeContextPtr->terminatingConditionsMutex);

    // START READ/WRITE PROTECTED SECTION
    if (encodeContextPtr->totalNumberOfReconFrames == encodeContextPtr->terminatingPictureNumber) {
        outputReconPtr->nFlags = EB_BUFFERFLAG_EOS;
    }
    encodeContextPtr->totalNumberOfReconFrames++;

    // Post the Recon object
    EbPostFullObject(outputReconWrapperPtr);

    EbReleaseMutex(encodeContextPtr->terminatingConditionsMutex);
}

void PadRefAndSetFlags(
    PictureControlSet_t    *pictureControlSetPtr,
    SequenceControlSet_t   *sequenceControlSetPtr
    )
{

    EbReferenceObject_t   *referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr;
    EbPictureBufferDesc_t *refPicPtr = (EbPictureBufferDesc_t*)referenceObject->referencePicture;
    EbPictureBufferDesc_t *refPic16BitPtr = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;
    EB_BOOL                is16bit = (sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);
    EB_COLOR_FORMAT colorFormat = pictureControlSetPtr->colorFormat;
    EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    if (!is16bit) {
        // Y samples
        GeneratePadding(
            refPicPtr->bufferY,
            refPicPtr->strideY,
            refPicPtr->width,
            refPicPtr->height,
            refPicPtr->originX,
            refPicPtr->originY);

        // Cb samples
        GeneratePadding(
            refPicPtr->bufferCb,
            refPicPtr->strideCb,
            refPicPtr->width >> subWidthCMinus1,
            refPicPtr->height >> subHeightCMinus1,
            refPicPtr->originX >> subWidthCMinus1,
            refPicPtr->originY >> subHeightCMinus1);

        // Cr samples
        GeneratePadding(
            refPicPtr->bufferCr,
            refPicPtr->strideCr,
            refPicPtr->width >> subWidthCMinus1,
            refPicPtr->height >> subHeightCMinus1,
            refPicPtr->originX >> subWidthCMinus1,
            refPicPtr->originY >> subHeightCMinus1);
    }

    //We need this for MCP
    if (is16bit) {
        // Y samples
        GeneratePadding16Bit(
            refPic16BitPtr->bufferY,
            refPic16BitPtr->strideY << 1,
            refPic16BitPtr->width << 1,
            refPic16BitPtr->height,
            refPic16BitPtr->originX << 1,
            refPic16BitPtr->originY);

        // Cb samples
        GeneratePadding16Bit(
            refPic16BitPtr->bufferCb,
            refPic16BitPtr->strideCb << 1,
            refPic16BitPtr->width << (1 - subWidthCMinus1),
            refPic16BitPtr->height >> subHeightCMinus1,
            refPic16BitPtr->originX << (1 - subWidthCMinus1),
            refPic16BitPtr->originY >> subHeightCMinus1);

        // Cr samples
        GeneratePadding16Bit(
            refPic16BitPtr->bufferCr,
            refPic16BitPtr->strideCr << 1,
            refPic16BitPtr->width << (1 - subWidthCMinus1),
            refPic16BitPtr->height >> subHeightCMinus1,
            refPic16BitPtr->originX << (1 - subWidthCMinus1),
            refPic16BitPtr->originY >> subHeightCMinus1);   
    }

    // set up TMVP flag for the reference picture
    referenceObject->tmvpEnableFlag = (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) ? EB_TRUE : EB_FALSE;

    // set up the ref POC
    referenceObject->refPOC = pictureControlSetPtr->ParentPcsPtr->pictureNumber;

    // set up the QP
    referenceObject->qp = (EB_U8)pictureControlSetPtr->ParentPcsPtr->pictureQp;

    // set up the Slice Type
    referenceObject->sliceType = pictureControlSetPtr->ParentPcsPtr->sliceType;


}


void CopyStatisticsToRefObject(
    PictureControlSet_t    *pictureControlSetPtr,
    SequenceControlSet_t   *sequenceControlSetPtr
    )
{
    pictureControlSetPtr->intraCodedArea = (100 * pictureControlSetPtr->intraCodedArea) / (sequenceControlSetPtr->lumaWidth * sequenceControlSetPtr->lumaHeight);
    if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
        pictureControlSetPtr->intraCodedArea = 0;

    ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->intraCodedArea = (EB_U8)(pictureControlSetPtr->intraCodedArea);

    EB_U32 lcuIndex;
    for (lcuIndex = 0; lcuIndex < pictureControlSetPtr->lcuTotalCount; ++lcuIndex){
        ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->nonMovingIndexArray[lcuIndex] = pictureControlSetPtr->ParentPcsPtr->nonMovingIndexArray[lcuIndex];
    }

    EbReferenceObject_t  * refObjL0, *refObjL1;
    ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->penalizeSkipflag = EB_FALSE;
    if (pictureControlSetPtr->sliceType == EB_B_PICTURE){ 
        refObjL0 = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
        refObjL1 = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;

        if (pictureControlSetPtr->temporalLayerIndex == 0){
            if (pictureControlSetPtr->ParentPcsPtr->intraCodedBlockProbability > 30){
                ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->penalizeSkipflag = EB_TRUE;
            }
        }
        else{
            ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->penalizeSkipflag = (refObjL0->penalizeSkipflag || refObjL1->penalizeSkipflag) ? EB_TRUE : EB_FALSE;
        }
    }
    ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->tmpLayerIdx = (EB_U8)pictureControlSetPtr->temporalLayerIndex;
    ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->isSceneChange = pictureControlSetPtr->ParentPcsPtr->sceneChangeFlag;


}

EB_ERRORTYPE QpmDeriveWeightsMinAndMax(
    PictureControlSet_t                    *pictureControlSetPtr,
    EncDecContext_t                        *contextPtr)
{
    EB_ERRORTYPE                    return_error = EB_ErrorNone;
    EB_U32 cuDepth;
    contextPtr->minDeltaQpWeight = encMinDeltaQpWeightTab[pictureControlSetPtr->temporalLayerIndex];
    contextPtr->maxDeltaQpWeight = encMaxDeltaQpWeightTab[pictureControlSetPtr->temporalLayerIndex];
    //QpmDeriveDeltaQpMapWeights


    EB_BOOL adjustMinQPFlag = EB_FALSE;

    adjustMinQPFlag = pictureControlSetPtr->adjustMinQPFlag;
    contextPtr->minDeltaQpWeight = 100;
    contextPtr->maxDeltaQpWeight = 100;

    {
        if (pictureControlSetPtr->sliceType == EB_I_PICTURE){
            if (pictureControlSetPtr->ParentPcsPtr->percentageOfEdgeinLightBackground > 0 && pictureControlSetPtr->ParentPcsPtr->percentageOfEdgeinLightBackground <= 3
                && !adjustMinQPFlag && pictureControlSetPtr->ParentPcsPtr->darkBackGroundlightForeGround) {
                contextPtr->minDeltaQpWeight = 100;
            }
            else{
                if (adjustMinQPFlag){


                    contextPtr->minDeltaQpWeight = 250;

                }
                else if (pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage > 30){

                    contextPtr->minDeltaQpWeight = 150;
                    contextPtr->maxDeltaQpWeight = 50;
                }
            }
        }
        else{
            if (adjustMinQPFlag){
                contextPtr->minDeltaQpWeight = 170;
            }
        }
        if (pictureControlSetPtr->ParentPcsPtr->highDarkAreaDensityFlag){
            contextPtr->minDeltaQpWeight = 25;
            contextPtr->maxDeltaQpWeight = 25;
        }
    }

    // Refine maxDeltaQpWeight; apply conservative max_degrade_weight when most of the picture is homogenous over time.  
    if (pictureControlSetPtr->ParentPcsPtr->picHomogenousOverTimeLcuPercentage > 90) {
        contextPtr->maxDeltaQpWeight = contextPtr->maxDeltaQpWeight >> 1;
    }


    for (cuDepth = 0; cuDepth < 4; cuDepth++){
        contextPtr->minDeltaQp[cuDepth] = pictureControlSetPtr->sliceType == EB_I_PICTURE ? encMinDeltaQpISliceTab[cuDepth] : encMinDeltaQpTab[cuDepth][pictureControlSetPtr->temporalLayerIndex];
        contextPtr->maxDeltaQp[cuDepth] = encMaxDeltaQpTab[cuDepth][pictureControlSetPtr->temporalLayerIndex];
    }

    return return_error;
}

/******************************************************
* Derive EncDec Settings for SQ
Input   : encoder mode and tune
Output  : EncDec Kernel signal(s)
******************************************************/
static EB_ERRORTYPE SignalDerivationEncDecKernelSq(
    SequenceControlSet_t *sequenceControlSetPtr,
    PictureControlSet_t  *pictureControlSetPtr,
    EncDecContext_t      *contextPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;

    // Set MD Open Loop Flag
    if (pictureControlSetPtr->encMode == ENC_MODE_0) {
        contextPtr->mdContext->intraMdOpenLoopFlag = EB_FALSE;
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
        if (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE) {
            contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->temporalLayerIndex == 0 ? EB_FALSE : EB_TRUE;
        }
        else {
            contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->sliceType == EB_I_PICTURE ? EB_FALSE : EB_TRUE;
        }
    }
    else {
        contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->sliceType == EB_I_PICTURE ? EB_FALSE : EB_TRUE;
    }

    // Derive INTRA Injection Method
    // 0 : Default (OIS)
    // 1 : Enhanced I_PICTURE, Default (OIS) otherwise 
    // 2 : 35 modes 
    if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_4) {
        contextPtr->mdContext->intraInjectionMethod = 1;
    }
    else {
        contextPtr->mdContext->intraInjectionMethod = 0;
    }

    // Derive Spatial SSE Flag
    if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_4) {          
        if (pictureControlSetPtr->sliceType == EB_I_PICTURE && contextPtr->mdContext->intraMdOpenLoopFlag == EB_FALSE) {
            contextPtr->mdContext->spatialSseFullLoop = EB_TRUE;
        }
        else {
            contextPtr->mdContext->spatialSseFullLoop = EB_FALSE;
        }
    }
    else {
        contextPtr->mdContext->spatialSseFullLoop = EB_FALSE;
    }


    // Set CHROMA Level
    // Level    Settings
    // 0        Full Search Chroma for All 
    // 1        Best Search Chroma for All LCUs; Chroma OFF if I_PICTURE, Chroma for only MV_Merge if P/B_PICTURE 
    // 2        Full vs. Best Swicth Method 0: chromaCond0 || chromaCond1 || chromaCond2
    // 3        Full vs. Best Swicth Method 1: chromaCond0 || chromaCond1
    // 4        Full vs. Best Swicth Method 2: chromaCond2 || chromaCond3
    // 5        Full vs. Best Swicth Method 3: chromaCond0
    // If INTRA Close Loop, then the switch modes (2,3,4,5) are not supported as reference samples for Chroma compensation will be a mix of source samples and reconstructed samples

    if (sequenceControlSetPtr->inputResolution >= INPUT_SIZE_4K_RANGE) {
        if (pictureControlSetPtr->ParentPcsPtr->encMode == ENC_MODE_0) {
            contextPtr->mdContext->chromaLevel = 0;
        }
        else if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_9) {
            if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
                contextPtr->mdContext->chromaLevel = 1;
            else if (pictureControlSetPtr->temporalLayerIndex == 0) {
                contextPtr->mdContext->chromaLevel = 0;
            }
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                if (contextPtr->mdContext->intraMdOpenLoopFlag) {
                    contextPtr->mdContext->chromaLevel = 2;
                }
                else {
                    contextPtr->mdContext->chromaLevel = 0;
                }
            }
            else {
                if (contextPtr->mdContext->intraMdOpenLoopFlag) {
                    contextPtr->mdContext->chromaLevel = 5;
                }
                else {
                    contextPtr->mdContext->chromaLevel = 0;
                }
            }
        }
        else if (pictureControlSetPtr->ParentPcsPtr->encMode == ENC_MODE_10) {
            if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
                contextPtr->mdContext->chromaLevel = 1;
            else if (pictureControlSetPtr->temporalLayerIndex == 0) {
                contextPtr->mdContext->chromaLevel = 0;
            }
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                if (contextPtr->mdContext->intraMdOpenLoopFlag) {
                    contextPtr->mdContext->chromaLevel = 3;
                }
                else {
                    contextPtr->mdContext->chromaLevel = 0;
                }
            }
            else {
                if (contextPtr->mdContext->intraMdOpenLoopFlag) {
                    contextPtr->mdContext->chromaLevel = 5;
                }
                else {
                    contextPtr->mdContext->chromaLevel = 0;
                }
            }
        }
        else {
            contextPtr->mdContext->chromaLevel = 1;
        }

    }
    else {
        if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_8) {
            contextPtr->mdContext->chromaLevel = 0;
        }
        else {


            if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
                contextPtr->mdContext->chromaLevel = 1;
            else if (pictureControlSetPtr->temporalLayerIndex == 0)
                contextPtr->mdContext->chromaLevel = 0;
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag)
                if (contextPtr->mdContext->intraMdOpenLoopFlag) {
                    contextPtr->mdContext->chromaLevel = 4;
                }
                else {
                    contextPtr->mdContext->chromaLevel = 0;
                }
            else
                contextPtr->mdContext->chromaLevel = 1;
        }
    }

    // Set Coeff Cabac Update Flag
    if (pictureControlSetPtr->ParentPcsPtr->encMode == ENC_MODE_0) {
        contextPtr->allowEncDecMismatch = EB_FALSE;
    }
    else {
        contextPtr->allowEncDecMismatch = (pictureControlSetPtr->temporalLayerIndex > 0) ?
            EB_TRUE :
            EB_FALSE;
    }

    // Set Allow EncDec Mismatch Flag
    if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
        contextPtr->mdContext->coeffCabacUpdate = ((pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL85_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL84_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_OPEN_LOOP_DEPTH_MODE) && contextPtr->mdContext->chromaLevel == 0) ?
            EB_TRUE :
            EB_FALSE;
    }
    else {
        contextPtr->mdContext->coeffCabacUpdate = EB_FALSE;
    }

    // Set INTRA8x8 Restriction @ P/B Slices
    contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_TRUE;
 
    // Set AMVP Generation @ MD Flag
    contextPtr->mdContext->generateAmvpTableMd = (pictureControlSetPtr->encMode <= ENC_MODE_3) ? EB_TRUE : EB_FALSE;

    // Set Cbf based Full-Loop Escape Flag
    contextPtr->mdContext->fullLoopEscape = EB_TRUE;

    // Set Fast-Loop Method
    contextPtr->mdContext->singleFastLoopFlag = (pictureControlSetPtr->encMode == ENC_MODE_0);

    // Set AMVP Injection Flag
    if (pictureControlSetPtr->encMode == ENC_MODE_0) {
        contextPtr->mdContext->amvpInjection = EB_TRUE;
    }
    else {
        contextPtr->mdContext->amvpInjection = EB_FALSE;
    }

    // Set Unipred 3x3 Injection Flag
    if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
        contextPtr->mdContext->unipred3x3Injection = EB_TRUE;
    }
    else {
        contextPtr->mdContext->unipred3x3Injection = EB_FALSE;
    }

    // Set Bipred 3x3 Injection Flag
    if (pictureControlSetPtr->encMode <= ENC_MODE_1)
        contextPtr->mdContext->bipred3x3Injection = EB_TRUE;
    else
        contextPtr->mdContext->bipred3x3Injection = EB_FALSE;

    // Set RDOQ/PM_CORE Flag
    if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
        contextPtr->mdContext->rdoqPmCoreMethod = (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_1) ? EB_RDOQ : EB_NO_RDOQ;
    }
    else {
        contextPtr->mdContext->rdoqPmCoreMethod = (pictureControlSetPtr->ParentPcsPtr->encMode == ENC_MODE_0) ? EB_RDOQ : EB_NO_RDOQ;
    }


    // Set PM Method (active only when brr is ON)
    if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
        contextPtr->pmMethod = 0;
    }
    else {
        if (sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE) {
            contextPtr->pmMethod = 1;
        }
        else {
            contextPtr->pmMethod = 0;
        }
    }

    // Set Fast EL Flag
    if (pictureControlSetPtr->encMode <= ENC_MODE_9) {
        contextPtr->fastEl = EB_FALSE;          
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_10) {
        if (sequenceControlSetPtr->inputResolution >= INPUT_SIZE_4K_RANGE) {
            contextPtr->fastEl = EB_TRUE;
        }
        else {
            contextPtr->fastEl = EB_FALSE;
        }
    }
    else {
        contextPtr->fastEl = EB_TRUE;
    }

	contextPtr->yBitsThsld = YBITS_THSHLD_1(pictureControlSetPtr->encMode);
    // Set SAO Mode
    contextPtr->saoMode = 0;
   
    // Set Exit Partitioning Flag
    if (pictureControlSetPtr->encMode >= ENC_MODE_9) {
        if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
            contextPtr->mdContext->enableExitPartitioning = EB_TRUE;
        }
        else {
            contextPtr->mdContext->enableExitPartitioning = EB_FALSE;
        }
    }
    else {
        contextPtr->mdContext->enableExitPartitioning = EB_FALSE;
    }

    // Set Limit INTRA Flag 
    if (pictureControlSetPtr->encMode < ENC_MODE_10) {
        contextPtr->mdContext->limitIntra = EB_FALSE;
    }
    else {
        if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_FALSE) {
            contextPtr->mdContext->limitIntra = EB_TRUE;
        }
        else {
            contextPtr->mdContext->limitIntra = EB_FALSE;
        }
    }

    // Set MPM Level
    // Level    Settings 
    // 0        Full MPM: 3
    // 1        ON but 1
    // 2        OFF
    if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
        contextPtr->mdContext->mpmLevel = 0;
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
        contextPtr->mdContext->mpmLevel = 1;
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            contextPtr->mdContext->mpmLevel = 1;
        }
        else {
            contextPtr->mdContext->mpmLevel = 2;
        }
    }
    else {
        if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            contextPtr->mdContext->mpmLevel = 1;
        }
        else {
            contextPtr->mdContext->mpmLevel = 2;
        }
    }
    
    // Set PF @ MD Level
    // Level    Settings 
    // 0        OFF
    // 1        N2    
    // 2        M2 if 8x8 or 16x16 or Detector, N4 otherwise
    // 3        M2 if 8x8, N4 otherwise
    if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {

        if (pictureControlSetPtr->ParentPcsPtr->encMode == ENC_MODE_0) {
            contextPtr->mdContext->pfMdLevel = 0;
        }
        else if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_6) {
            if (pictureControlSetPtr->temporalLayerIndex == 0) {
                contextPtr->mdContext->pfMdLevel = 0;
            }
            else {
                contextPtr->mdContext->pfMdLevel = 1;
            }
        }
        else if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_10) {
            if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                contextPtr->mdContext->pfMdLevel = 1;
            }
            else {
                contextPtr->mdContext->pfMdLevel = 3;
            }
        }
        else {
            if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
                contextPtr->mdContext->pfMdLevel = 1;
            }
            else if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0) {
                contextPtr->mdContext->pfMdLevel = 2;
            }
            else {
                contextPtr->mdContext->pfMdLevel = 3;
            }
        }
    }
    else
    {
        if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_4) {
            contextPtr->mdContext->pfMdLevel = 0;
        }
        else {
            if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                contextPtr->mdContext->pfMdLevel = 0;
            }
            else {
                contextPtr->mdContext->pfMdLevel = 1;
            }
        }
    }


    // Set INTRA4x4 Search Level
    // Level    Settings 
    // 0        INLINE if not BDP, refinment otherwise 
    // 1        REFINMENT   
    // 2        OFF
    if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
        if (pictureControlSetPtr->encMode == ENC_MODE_0) {
            contextPtr->mdContext->intra4x4Level = 0;
        }
        else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
            if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                contextPtr->mdContext->intra4x4Level = 1;
            }
            else {
                contextPtr->mdContext->intra4x4Level = 2;
            }
        }
        else {
            contextPtr->mdContext->intra4x4Level = 2;
        }
    }
    else {
        if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
            contextPtr->mdContext->intra4x4Level = 0;
        }
        else {
            contextPtr->mdContext->intra4x4Level = 1;
        }
    }

    // Set INTRA4x4 NFL
    if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
        contextPtr->mdContext->intra4x4Nfl = 4;
    }
    else {
        contextPtr->mdContext->intra4x4Nfl = 2;
    }

    // Set INTRA4x4 Injection
    // 0: 35 mdoes
    // 1: up to 4: DC, Best INTR8x8, +3, -0
    if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
        contextPtr->mdContext->intra4x4IntraInjection = 0;
    }
    else {
        contextPtr->mdContext->intra4x4IntraInjection = 1;
    }

    // NMM Level MD           Settings
    // 0                      5
    // 1                      3 if 32x32, 2 otherwise
    // 2                      2

    if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
        contextPtr->mdContext->nmmLevelMd = 0;
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
        if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE || sequenceControlSetPtr->inputResolution == INPUT_SIZE_1080p_RANGE || sequenceControlSetPtr->inputResolution == INPUT_SIZE_576p_RANGE_OR_LOWER) {
            contextPtr->mdContext->nmmLevelMd = 0;
        }
        else {
            contextPtr->mdContext->nmmLevelMd = 1;
        }
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
        if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
            contextPtr->mdContext->nmmLevelMd = 0;
        }
        else {
            contextPtr->mdContext->nmmLevelMd = 1;
        }
    }
    else {
        contextPtr->mdContext->nmmLevelMd = 1;
    }

    // NMM Level BDP          Settings
    // 0                      5
    // 1                      3
    // 2                      3 if 32x32 or depth refinment true, 2 otherwise
    // 3                      3 if 32x32, 2 otherwise
    // 4                      2
    if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
        contextPtr->mdContext->nmmLevelBdp = 0;
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
        if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE || sequenceControlSetPtr->inputResolution == INPUT_SIZE_1080p_RANGE || sequenceControlSetPtr->inputResolution == INPUT_SIZE_576p_RANGE_OR_LOWER) {
            contextPtr->mdContext->nmmLevelBdp = 0;
        }
        else {
            if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0) {
                contextPtr->mdContext->nmmLevelBdp = 1;
            }
            else {
                contextPtr->mdContext->nmmLevelBdp = 2;
            }
        }
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
        if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
            contextPtr->mdContext->nmmLevelBdp = 0;
        }
        else {
            if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0) {
                contextPtr->mdContext->nmmLevelBdp = 1;
            }
            else {
                contextPtr->mdContext->nmmLevelBdp = 2;
            }
        }
    }
    else {
        if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0) {
            contextPtr->mdContext->nmmLevelBdp = 1;
        }
        else {
            contextPtr->mdContext->nmmLevelBdp = 2;
        }
    }


    // NFL Level MD         Settings
    // 0                    4
    // 1                    3 if 32x32, 2 otherwise
    // 2                    2
    // 3                    2 if Detectors, 1 otherwise
    // 4                    2 if 64x64 or 32x32 or 16x16, 1 otherwise
    // 5                    2 if 64x64 or 332x32, 1 otherwise
    // 6                    1

    if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
        contextPtr->mdContext->nflLevelMd = 0;

    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
        if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
            contextPtr->mdContext->nflLevelMd = 0;
        }
        else {
            if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
                contextPtr->mdContext->nflLevelMd = 0;
            }
            else {
                if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                    contextPtr->mdContext->nflLevelMd = 1;
                }
                else {
                    contextPtr->mdContext->nflLevelMd = 2;
                }
            }
        }

    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
        if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            contextPtr->mdContext->nflLevelMd = 0;
        }
        else {
            if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                contextPtr->mdContext->nflLevelMd = 1;
            }
            else {
                contextPtr->mdContext->nflLevelMd = 2;
            }
        }
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_10) {
        if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_1080p_RANGE) {
            if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
                contextPtr->mdContext->nflLevelMd = 1;
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag)
                contextPtr->mdContext->nflLevelMd = 4;
            else
                contextPtr->mdContext->nflLevelMd = 5;

        }
        else
        {
            if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
                contextPtr->mdContext->nflLevelMd = 0;
            }
            else {
                contextPtr->mdContext->nflLevelMd = 2;
            }
        }
    }
    else if (pictureControlSetPtr->encMode == ENC_MODE_11) {

        if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_1080p_RANGE) {
                contextPtr->mdContext->nflLevelMd = 1;
            }
            else {
                contextPtr->mdContext->nflLevelMd = 0;
            }
        }
        else {
            contextPtr->mdContext->nflLevelMd = 3;
        }
    }
    else {
        contextPtr->mdContext->nflLevelMd = 6;
    }

    // NFL Level Pillar/8x8 Refinement         Settings
    // 0                                       4
    // 1                                       4 if depthRefinment, 3 if 32x32, 2 otherwise
    // 2                                       3 
    // 3                                       3 if depthRefinment or 32x32, 2 otherwise
    // 4                                       3 if 32x32, 2 otherwise
    // 5                                       2    
    // 6                                       2 if Detectors, 1 otherwise
    // 7                                       2 if 64x64 or 32x32 or 16x16, 1 otherwise
    // 8                                       2 if 64x64 or 332x32, 1 otherwise
    // 9                                       1      
    if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
        contextPtr->mdContext->nflLevelPillar8x8ref = 0;
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
        if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
            contextPtr->mdContext->nflLevelPillar8x8ref = 0;
        }
        else {
            if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
                contextPtr->mdContext->nflLevelPillar8x8ref = 1;
            }
            else {
                if (pictureControlSetPtr->temporalLayerIndex == 0) {
                    contextPtr->mdContext->nflLevelPillar8x8ref = 2;
                }
                else {
                    contextPtr->mdContext->nflLevelPillar8x8ref = 3;
                }
            }
        }
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
        if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            contextPtr->mdContext->nflLevelPillar8x8ref = 1;
        }
        else {
            if (pictureControlSetPtr->temporalLayerIndex == 0) {
                contextPtr->mdContext->nflLevelPillar8x8ref = 2;
            }
            else {
                contextPtr->mdContext->nflLevelPillar8x8ref = 3;
            }
        }
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_10) {
        if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_1080p_RANGE)
        {
            if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
                contextPtr->mdContext->nflLevelPillar8x8ref = 4;
            else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag)
                contextPtr->mdContext->nflLevelPillar8x8ref = 7;
            else
                contextPtr->mdContext->nflLevelPillar8x8ref = 8;
        }
        else
        {
            if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
                contextPtr->mdContext->nflLevelPillar8x8ref = 4;
            else
                contextPtr->mdContext->nflLevelPillar8x8ref = 5;
        }
    }
    else if (pictureControlSetPtr->encMode == ENC_MODE_11) {

        if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            contextPtr->mdContext->nflLevelPillar8x8ref = 4;
        }
        else {
            contextPtr->mdContext->nflLevelPillar8x8ref = 6;
        }
    }
    else {
        contextPtr->mdContext->nflLevelPillar8x8ref = 9;
    }

    // NFL Level MvMerge/64x64 Refinement      Settings
    // 0                                       4
    // 1                                       3 
    // 2                                       3 if depthRefinment or 32x32, 2 otherwise
    // 3                                       3 if 32x32, 2 otherwise
    // 4                                       2    
    // 5                                       2 if Detectors, 1 otherwise
    // 6                                       2 if 64x64 or 32x32 or 16x16, 1 otherwise
    // 7                                       2 if 64x64 or 332x32, 1 otherwise
    // 8                                       1      

    if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
        contextPtr->mdContext->nflLevelMvMerge64x64ref = 0;
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
        if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
            contextPtr->mdContext->nflLevelMvMerge64x64ref = 0;
        }
        else {
            if (pictureControlSetPtr->temporalLayerIndex == 0) {
                contextPtr->mdContext->nflLevelMvMerge64x64ref = 1;
            }
            else {
                contextPtr->mdContext->nflLevelMvMerge64x64ref = 2;
            }
        }
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            contextPtr->mdContext->nflLevelMvMerge64x64ref = 1;
        }
        else {
            contextPtr->mdContext->nflLevelMvMerge64x64ref = 2;
        }
    }
    else if (pictureControlSetPtr->encMode <= ENC_MODE_10) {
        if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_1080p_RANGE)
        {
            if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag)
                contextPtr->mdContext->nflLevelMvMerge64x64ref = 6;
            else
                contextPtr->mdContext->nflLevelMvMerge64x64ref = 7;
        }
        else
        {
            contextPtr->mdContext->nflLevelMvMerge64x64ref = 4;
        }
    }
    else if (pictureControlSetPtr->encMode == ENC_MODE_11)
    {
        contextPtr->mdContext->nflLevelMvMerge64x64ref = 5;
    }
    else {
        contextPtr->mdContext->nflLevelMvMerge64x64ref = 8;
    }

    return return_error;
}

/******************************************************
* Derive EncDec Settings for OQ
Input   : encoder mode and tune
Output  : EncDec Kernel signal(s)
******************************************************/
static EB_ERRORTYPE SignalDerivationEncDecKernelOq(
    SequenceControlSet_t *sequenceControlSetPtr,
    PictureControlSet_t  *pictureControlSetPtr,
    EncDecContext_t      *contextPtr)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    
    // Set MD Open Loop Flag
	if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->intraMdOpenLoopFlag = EB_FALSE;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE ? EB_FALSE : EB_TRUE;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->temporalLayerIndex == 0 ? EB_FALSE : EB_TRUE;
		}
		else {
			contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE ? EB_FALSE : EB_TRUE;
		}
	}
    else if (pictureControlSetPtr->encMode <= ENC_MODE_10) {
        contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->temporalLayerIndex == 0 ? EB_FALSE : EB_TRUE;
    }
    else {
        contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->sliceType == EB_I_PICTURE ? EB_FALSE : EB_TRUE;
    }
    // Derive INTRA Injection Method
    // 0 : Default (OIS)
    // 1 : Enhanced I_PICTURE, Default (OIS) otherwise 
    // 2 : 35 modes 
    if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->intraInjectionMethod = 2;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_9) {
		contextPtr->mdContext->intraInjectionMethod = 1;
	}
    else {
        contextPtr->mdContext->intraInjectionMethod = 0;
    }
    
    // Derive Spatial SSE Flag
	if (pictureControlSetPtr->sliceType == EB_I_PICTURE && contextPtr->mdContext->intraMdOpenLoopFlag == EB_FALSE && pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->spatialSseFullLoop = EB_TRUE;
	}
	else {
		contextPtr->mdContext->spatialSseFullLoop = EB_FALSE;
	}
    
    // Set Allow EncDec Mismatch Flag
	if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_6) {
		contextPtr->allowEncDecMismatch = EB_FALSE;
	}
    else if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->allowEncDecMismatch = (pictureControlSetPtr->temporalLayerIndex > 0) ?
				EB_TRUE :
				EB_FALSE;
		}
		else {
			contextPtr->allowEncDecMismatch = EB_FALSE;
		}
    }
    else {
        contextPtr->allowEncDecMismatch = (pictureControlSetPtr->temporalLayerIndex > 0) ?
            EB_TRUE :
            EB_FALSE;
    }

    // Set CHROMA Level
    // 0: Full Search Chroma for All LCUs
    // 1: Best Search Chroma for All LCUs; Chroma OFF if I_PICTURE, Chroma for only MV_Merge if P/B_PICTURE 
    // 2: Full vs. Best Swicth Method 0: chromaCond0 || chromaCond1 || chromaCond2
    // 3: Full vs. Best Swicth Method 1: chromaCond0 || chromaCond1
    // 4: Full vs. Best Swicth Method 2: chromaCond2 || chromaCond3
    // 5: Full vs. Best Swicth Method 3: chromaCond0
    // If INTRA Close Loop, then the switch modes (2,3,4,5) are not supported as reference samples for Chroma compensation will be a mix of source samples and reconstructed samplesoop

	if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		contextPtr->mdContext->chromaLevel = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution >= INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->chromaLevel = 1;
			}
			else if (pictureControlSetPtr->temporalLayerIndex == 0) {
				contextPtr->mdContext->chromaLevel = 0;
			}
			else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				if (contextPtr->mdContext->intraMdOpenLoopFlag) {
					contextPtr->mdContext->chromaLevel = 4;
				}
				else {
					contextPtr->mdContext->chromaLevel = 0;
				}
			}
			else {
				contextPtr->mdContext->chromaLevel = 1;
			}
		}
		else {
			contextPtr->mdContext->chromaLevel = 0;
		}
	}
    else if (pictureControlSetPtr->encMode <= ENC_MODE_10) {
        if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            contextPtr->mdContext->chromaLevel = 1;
        }
        else if (pictureControlSetPtr->temporalLayerIndex == 0) {
            contextPtr->mdContext->chromaLevel = 0;
        }
        else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
            if (contextPtr->mdContext->intraMdOpenLoopFlag) {
                contextPtr->mdContext->chromaLevel = 4;
            }
            else {
                contextPtr->mdContext->chromaLevel = 0;
            }
        }
        else {
            contextPtr->mdContext->chromaLevel = 1;
        }
    }
    else {
        contextPtr->mdContext->chromaLevel = 1;
    }

    // Set Coeff Cabac Update Flag
    if (pictureControlSetPtr->encMode <= ENC_MODE_9) {
		contextPtr->mdContext->coeffCabacUpdate = ((pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL85_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL84_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_OPEN_LOOP_DEPTH_MODE) && contextPtr->mdContext->chromaLevel == 0) ?
			EB_TRUE :
			EB_FALSE;
	}
    else {
        contextPtr->mdContext->coeffCabacUpdate = EB_FALSE;
    }
  
    // Set INTRA8x8 Restriction @ P/B Slices
	if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
		contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_FALSE;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
		if (sequenceControlSetPtr->inputResolution >= INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_TRUE;
		}
		else {
			contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_FALSE;
		}
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
		contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_FALSE;
	}
    else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		if (sequenceControlSetPtr->inputResolution >= INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_TRUE;
		}
		else {
			contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_FALSE;
		}
	}
    else {
        contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_TRUE;
    }

    // Set AMVP Generation @ MD Flag
    contextPtr->mdContext->generateAmvpTableMd = (pictureControlSetPtr->encMode <= ENC_MODE_10) ? EB_TRUE : EB_FALSE;

    // Set Cbf based Full-Loop Escape Flag
    if (pictureControlSetPtr->encMode <= ENC_MODE_1) {
        contextPtr->mdContext->fullLoopEscape = EB_FALSE;
    }
    else {
        if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_576p_RANGE_OR_LOWER) {
            contextPtr->mdContext->fullLoopEscape = EB_FALSE;
        }
        else {
            contextPtr->mdContext->fullLoopEscape = EB_TRUE;
        }
    }

    // Set Fast-Loop Method
	if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->singleFastLoopFlag = EB_TRUE;
	}
	else {
		contextPtr->mdContext->singleFastLoopFlag = EB_FALSE;
	}

    // Set AMVP Injection Flag
    if (pictureControlSetPtr->encMode <= ENC_MODE_1) {
        contextPtr->mdContext->amvpInjection = EB_TRUE;
    }
    else {
        contextPtr->mdContext->amvpInjection = EB_FALSE;
    }
    
    // Set Unipred 3x3 Injection Flag
    if (pictureControlSetPtr->encMode <= ENC_MODE_1) {
        contextPtr->mdContext->unipred3x3Injection = EB_TRUE;
    }
    else {
        contextPtr->mdContext->unipred3x3Injection = EB_FALSE;
    }
    
    // Set Bipred 3x3 Injection Flag
	if (pictureControlSetPtr->encMode <= ENC_MODE_1) {
		contextPtr->mdContext->bipred3x3Injection = EB_TRUE;
	}
	else {
		contextPtr->mdContext->bipred3x3Injection = EB_FALSE;
	}

    // Set RDOQ/PM_CORE Flag
	contextPtr->mdContext->rdoqPmCoreMethod = (pictureControlSetPtr->ParentPcsPtr->encMode == ENC_MODE_0) ?
		EB_RDOQ :
		(pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_4) ?
		EB_PMCORE :
		EB_NO_RDOQ;

    // Set PM Method (active only when brr is ON)
    contextPtr->pmMethod = 0;

    // Set Fast EL Flag
    contextPtr->fastEl      = (pictureControlSetPtr->encMode <= ENC_MODE_10) ? EB_FALSE : EB_TRUE;
	contextPtr->yBitsThsld  = (pictureControlSetPtr->encMode <= ENC_MODE_10) ? YBITS_THSHLD_1(0) : YBITS_THSHLD_1(12);
    
    // Set SAO Mode
    contextPtr->saoMode = (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_10) ? 1 : 0;
    
    // Set Exit Partitioning Flag 
    if (pictureControlSetPtr->encMode >= ENC_MODE_10) {
        if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
            contextPtr->mdContext->enableExitPartitioning = EB_TRUE;
        }
        else {
            contextPtr->mdContext->enableExitPartitioning = EB_FALSE;
        }
    }
    else {
        contextPtr->mdContext->enableExitPartitioning = EB_FALSE;
    }

    // Set Limit INTRA Flag 
	if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
		contextPtr->mdContext->limitIntra = EB_FALSE;
	}
    else {
        if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_FALSE) {
            contextPtr->mdContext->limitIntra = EB_TRUE;
        }
        else {
            contextPtr->mdContext->limitIntra = EB_FALSE;
        }
    }

    // Set MPM Level
    // Level    Settings 
    // 0        Full MPM: 3
    // 1        ON but 1
    // 2        OFF    
    if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
        contextPtr->mdContext->mpmLevel = 0;
    }
	else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		contextPtr->mdContext->mpmLevel = 2;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->mpmLevel = 1;
			}
			else {
				contextPtr->mdContext->mpmLevel = 2;
			}
		}
		else {
			contextPtr->mdContext->mpmLevel = 2;
		}
	}
    else {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			contextPtr->mdContext->mpmLevel = 1;
		}
		else {
			contextPtr->mdContext->mpmLevel = 2;
		}
    }

    // Set PF @ MD Level
    // Level    Settings 
    // 0        OFF
    // 1        N2    
    // 2        M2 if 8x8 or 16x16 or Detector, N4 otherwise
    // 3        M2 if 8x8, N4 otherwise
	if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		contextPtr->mdContext->pfMdLevel = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				contextPtr->mdContext->pfMdLevel = 0;
			}
			else {
				contextPtr->mdContext->pfMdLevel = 1;
			}
		}
		else {
			contextPtr->mdContext->pfMdLevel = 0;
		}
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_8) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->temporalLayerIndex == 0) {
				contextPtr->mdContext->pfMdLevel = 0;
			}
			else {
				contextPtr->mdContext->pfMdLevel = 1;
			}
		}
		else {
			if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				contextPtr->mdContext->pfMdLevel = 0;
			}
			else {
				contextPtr->mdContext->pfMdLevel = 1;
			}
		}
	}
    else if (pictureControlSetPtr->encMode <= ENC_MODE_10) {
        if (pictureControlSetPtr->temporalLayerIndex == 0) {
            contextPtr->mdContext->pfMdLevel = 0;
        }
        else {
            contextPtr->mdContext->pfMdLevel = 1;
        }
    }
    else
    {

        if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
            contextPtr->mdContext->pfMdLevel = 1;
        }
        else {
            contextPtr->mdContext->pfMdLevel = 3;
        }
    }

    // Set INTRA4x4 Search Level
    // Level    Settings 
    // 0        INLINE if not BDP, refinment otherwise 
    // 1        REFINMENT   
    // 2        OFF
	if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->intra4x4Level = 0;
	}
    else {
        contextPtr->mdContext->intra4x4Level = 2;
    }

    // Set INTRA4x4 NFL
	if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->intra4x4Nfl = 4;
	}
	else {
		contextPtr->mdContext->intra4x4Nfl = 2;
	}
    
    // Set INTRA4x4 Injection
    // 0: 35 mdoes
    // 1: up to 4: DC, Best INTR8x8, +3, -0
	if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->intra4x4IntraInjection = 0;
	}
	else {
		contextPtr->mdContext->intra4x4IntraInjection = 1;
	}

    // NMM Level MD           Settings
    // 0                      5
    // 1                      3 if 32x32, 2 otherwise
    // 2                      2
    if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		contextPtr->mdContext->nmmLevelMd = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->nmmLevelMd = 1;
		}
		else {
			contextPtr->mdContext->nmmLevelMd = 0;
		}
	}
    else {
        contextPtr->mdContext->nmmLevelMd = 1;
    }

    // NMM Level BDP          Settings
    // 0                      5
    // 1                      3
    // 2                      3 if 32x32 or depth refinment true, 2 otherwise
    // 3                      3 if 32x32, 2 otherwise
    // 4                      2
    if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		contextPtr->mdContext->nmmLevelBdp = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->nmmLevelBdp = 3;
		}
		else {
			contextPtr->mdContext->nmmLevelBdp = 0;
		}
	}
    else if (pictureControlSetPtr->encMode <= ENC_MODE_8) {
		if (sequenceControlSetPtr->inputResolution <= INPUT_SIZE_576p_RANGE_OR_LOWER) {
			contextPtr->mdContext->nmmLevelBdp = 0;
		}
		else if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0) {
				contextPtr->mdContext->nmmLevelBdp = 1;
			}
			else {
				contextPtr->mdContext->nmmLevelBdp = 2;
			}
		}
		else {
			contextPtr->mdContext->nmmLevelBdp = 3;
		}
    }
    else {
		if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0) {
			contextPtr->mdContext->nmmLevelBdp = 1;
		}
		else {
			contextPtr->mdContext->nmmLevelBdp = 2;
		}
    }

    // NFL Level MD         Settings
    // 0                    4
    // 1                    3 if 32x32, 2 otherwise
    // 2                    2
    // 3                    2 if Detectors, 1 otherwise
    // 4                    2 if 64x64 or 32x32 or 16x16, 1 otherwise
    // 5                    2 if 64x64 or 332x32, 1 otherwise
    // 6                    1

    if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->nflLevelMd = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			contextPtr->mdContext->nflLevelMd = 0;
		}
		else {
			if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				contextPtr->mdContext->nflLevelMd = 1;
			}
			else {
				contextPtr->mdContext->nflLevelMd = 2;
			}
		}
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelMd = 1;
			}
			else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				contextPtr->mdContext->nflLevelMd = 4;
			}
			else {
				contextPtr->mdContext->nflLevelMd = 5;
			}
		}
		else {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelMd = 0;
			}
			else {
				if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
					contextPtr->mdContext->nflLevelMd = 1;
				}
				else {
					contextPtr->mdContext->nflLevelMd = 2;
				}
			}
		}
    }
	else if (pictureControlSetPtr->encMode <= ENC_MODE_9) {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			contextPtr->mdContext->nflLevelMd = 1;
		}
		else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
			contextPtr->mdContext->nflLevelMd = 4;
		}
		else {
			contextPtr->mdContext->nflLevelMd = 5;
		}
    }
    else {
		contextPtr->mdContext->nflLevelMd = 6;
    }

    // NFL Level Pillar/8x8 Refinement         Settings
    // 0                                       4
    // 1                                       4 if depthRefinment, 3 if 32x32, 2 otherwise
    // 2                                       3 
    // 3                                       3 if depthRefinment or 32x32, 2 otherwise
    // 4                                       3 if 32x32, 2 otherwise
    // 5                                       2    
    // 6                                       2 if Detectors, 1 otherwise
    // 7                                       2 if 64x64 or 32x32 or 16x16, 1 otherwise
    // 8                                       2 if 64x64 or 332x32, 1 otherwise
    // 9                                       1      
    if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->nflLevelPillar8x8ref = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			contextPtr->mdContext->nflLevelPillar8x8ref = 0;
		}
		else {
			if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				contextPtr->mdContext->nflLevelPillar8x8ref = 4;
			}
			else {
				contextPtr->mdContext->nflLevelPillar8x8ref = 5;
			}
		}
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelPillar8x8ref = 4;
			}
			else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				contextPtr->mdContext->nflLevelPillar8x8ref = 7;
			}
			else {
				contextPtr->mdContext->nflLevelPillar8x8ref = 8;
			}
		}
		else {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelPillar8x8ref = 0;
			}
			else {
				if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
					contextPtr->mdContext->nflLevelPillar8x8ref = 4;
				}
				else {
					contextPtr->mdContext->nflLevelPillar8x8ref = 5;
				}
			}
		}
	}
    else if (pictureControlSetPtr->encMode <= ENC_MODE_9) {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			contextPtr->mdContext->nflLevelPillar8x8ref = 4;
		}
		else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
			contextPtr->mdContext->nflLevelPillar8x8ref = 7;
		}
		else {
			contextPtr->mdContext->nflLevelPillar8x8ref = 8;
		}
    }
    else {
		contextPtr->mdContext->nflLevelPillar8x8ref = 9;

    }

    // NFL Level MvMerge/64x64 Refinement      Settings
    // 0                                       4
    // 1                                       3 
    // 2                                       3 if depthRefinment or 32x32, 2 otherwise
    // 3                                       3 if 32x32, 2 otherwise
    // 4                                       2    
    // 5                                       2 if Detectors, 1 otherwise
    // 6                                       2 if 64x64 or 32x32 or 16x16, 1 otherwise
    // 7                                       2 if 64x64 or 332x32, 1 otherwise
    // 8                                       1     
    if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->nflLevelMvMerge64x64ref = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			contextPtr->mdContext->nflLevelMvMerge64x64ref = 0;
		}
		else {
			if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				contextPtr->mdContext->nflLevelMvMerge64x64ref = 3;
			}
			else {
				contextPtr->mdContext->nflLevelMvMerge64x64ref = 4;
			}
		}
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_7) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				contextPtr->mdContext->nflLevelMvMerge64x64ref = 6;
			}
			else {
				contextPtr->mdContext->nflLevelMvMerge64x64ref = 7;
			}
		}
		else {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelMvMerge64x64ref = 0;
			}
			else {
				if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
					contextPtr->mdContext->nflLevelMvMerge64x64ref = 3;
				}
				else {
					contextPtr->mdContext->nflLevelMvMerge64x64ref = 4;
				}
			}
		}
	}
    else if (pictureControlSetPtr->encMode <= ENC_MODE_9) {
		if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
			contextPtr->mdContext->nflLevelMvMerge64x64ref = 6;
		}
		else {
			contextPtr->mdContext->nflLevelMvMerge64x64ref = 7;
		}
    }
    else {
		contextPtr->mdContext->nflLevelMvMerge64x64ref = 8;

    }

    return return_error;
}

/******************************************************
* Derive EncDec Settings for VMAF
Input   : encoder mode and tune
Output  : EncDec Kernel signal(s)
******************************************************/
EB_ERRORTYPE SignalDerivationEncDecKernelVmaf(
	SequenceControlSet_t *sequenceControlSetPtr,
	PictureControlSet_t  *pictureControlSetPtr,
	EncDecContext_t      *contextPtr) {

	EB_ERRORTYPE return_error = EB_ErrorNone;

	// Set MD Open Loop Flag
	if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->intraMdOpenLoopFlag = EB_FALSE;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_9) {
		contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE ? EB_FALSE : EB_TRUE;
	}
	else {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE ? EB_FALSE : EB_TRUE;
		}
		else {
			contextPtr->mdContext->intraMdOpenLoopFlag = pictureControlSetPtr->sliceType == EB_I_PICTURE ? EB_FALSE : EB_TRUE;
		}
	}

	// Derive INTRA Injection Method
	// 0 : Default (OIS)
	// 1 : Enhanced I_PICTURE, Default (OIS) otherwise 
	// 2 : 35 modes 
	if (pictureControlSetPtr->encMode == ENC_MODE_0) {
		contextPtr->mdContext->intraInjectionMethod = 2;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_9) {
		contextPtr->mdContext->intraInjectionMethod = 1;
	}
	else {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->intraInjectionMethod = 1;
		}
		else {
			contextPtr->mdContext->intraInjectionMethod = 0;
		}
	}

	// Derive Spatial SSE Flag
	if (pictureControlSetPtr->sliceType == EB_I_PICTURE && contextPtr->mdContext->intraMdOpenLoopFlag == EB_FALSE && pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_1) {
		contextPtr->mdContext->spatialSseFullLoop = EB_TRUE;
	}
	else {
		contextPtr->mdContext->spatialSseFullLoop = EB_FALSE;
	}

	// Set Allow EncDec Mismatch Flag
	contextPtr->allowEncDecMismatch = EB_FALSE;
	
	// Set CHROMA Level
	// 0: Full Search Chroma for All LCUs
	// 1: Best Search Chroma for All LCUs; Chroma OFF if I_PICTURE, Chroma for only MV_Merge if P/B_PICTURE 
	// 2: Full vs. Best Swicth Method 0: chromaCond0 || chromaCond1 || chromaCond2
	// 3: Full vs. Best Swicth Method 1: chromaCond0 || chromaCond1
	// 4: Full vs. Best Swicth Method 2: chromaCond2 || chromaCond3
	// 5: Full vs. Best Swicth Method 3: chromaCond0
	// If INTRA Close Loop, then the switch modes (2,3,4,5) are not supported as reference samples for Chroma compensation will be a mix of source samples and reconstructed samplesoop
	if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_3) {
		contextPtr->mdContext->chromaLevel = 0;
	}
	else if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_4) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->chromaLevel = 0;
		}
		else {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
				contextPtr->mdContext->chromaLevel = 1;
			else if (pictureControlSetPtr->temporalLayerIndex == 0)
				contextPtr->mdContext->chromaLevel = 0;
			else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				if (contextPtr->mdContext->intraMdOpenLoopFlag) {
					contextPtr->mdContext->chromaLevel = 4;
				}
				else {
					contextPtr->mdContext->chromaLevel = 0;
				}
			}
			else {
				contextPtr->mdContext->chromaLevel = 1;
			}
		}
	}
	else if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_8) {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
			contextPtr->mdContext->chromaLevel = 1;
		else if (pictureControlSetPtr->temporalLayerIndex == 0)
			contextPtr->mdContext->chromaLevel = 0;
		else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
			if (contextPtr->mdContext->intraMdOpenLoopFlag) {
				contextPtr->mdContext->chromaLevel = 4;
			}
			else {
				contextPtr->mdContext->chromaLevel = 0;
			}
		}
		else {
			contextPtr->mdContext->chromaLevel = 1;
		}
	}
	else {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->chromaLevel = 1;
		}
		else {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE)
				contextPtr->mdContext->chromaLevel = 1;
			else if (pictureControlSetPtr->temporalLayerIndex == 0)
				contextPtr->mdContext->chromaLevel = 0;
			else if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				if (contextPtr->mdContext->intraMdOpenLoopFlag) {
					contextPtr->mdContext->chromaLevel = 4;
				}
				else {
					contextPtr->mdContext->chromaLevel = 0;
				}
			}
			else {
				contextPtr->mdContext->chromaLevel = 1;
			}
		}
	}

	// Set Coeff Cabac Update Flag
	if (pictureControlSetPtr->encMode <= ENC_MODE_1) {
		contextPtr->mdContext->coeffCabacUpdate = ((pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL85_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_FULL84_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_OPEN_LOOP_DEPTH_MODE) && contextPtr->mdContext->chromaLevel == 0) ?
			EB_TRUE :
			EB_FALSE;
	}
	else {
		contextPtr->mdContext->coeffCabacUpdate = EB_FALSE;
	}
	
	// Set INTRA8x8 Restriction @ P/B Slices
	if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
		contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_FALSE;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_TRUE;
		}
		else {
			contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_FALSE;
		}
	}
	else {
		contextPtr->mdContext->intra8x8RestrictionInterSlice = EB_TRUE;
	}

	// Set AMVP Generation @ MD Flag
	contextPtr->mdContext->generateAmvpTableMd = EB_TRUE;

	// Set Cbf based Full-Loop Escape Flag
	contextPtr->mdContext->fullLoopEscape = (pictureControlSetPtr->encMode <= ENC_MODE_0)? EB_FALSE: EB_TRUE;

	// Set Fast-Loop Method
	contextPtr->mdContext->singleFastLoopFlag = (pictureControlSetPtr->encMode == ENC_MODE_0);

	// Set AMVP Injection Flag
	contextPtr->mdContext->amvpInjection = (pictureControlSetPtr->encMode <= ENC_MODE_1)?EB_TRUE: EB_FALSE;

	// Set Unipred 3x3 Injection Flag
	contextPtr->mdContext->unipred3x3Injection = (pictureControlSetPtr->encMode <= ENC_MODE_1)?EB_TRUE: EB_FALSE;

	// Set Bipred 3x3 Injection Flag
	contextPtr->mdContext->bipred3x3Injection = (pictureControlSetPtr->encMode <= ENC_MODE_1)? EB_TRUE : EB_FALSE;

	// Set RDOQ/PM_CORE Flag
	if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_4) {
		contextPtr->mdContext->rdoqPmCoreMethod = EB_RDOQ;
	}
	else if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_5) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->rdoqPmCoreMethod = EB_RDOQ;
		}
		else {
			contextPtr->mdContext->rdoqPmCoreMethod = EB_LIGHT;
		}
	}
	else if (pictureControlSetPtr->ParentPcsPtr->encMode <= ENC_MODE_9) {
		contextPtr->mdContext->rdoqPmCoreMethod = EB_LIGHT;
	}
	else {
		contextPtr->mdContext->rdoqPmCoreMethod = EB_NO_RDOQ;
	}
	
	// Set PM Method (active only when brr is ON)
	contextPtr->pmMethod = 0;

	// Set Fast EL Flag
	contextPtr->fastEl = EB_FALSE;
	contextPtr->yBitsThsld = YBITS_THSHLD_1(0);

	// Set SAO Mode
	contextPtr->saoMode = 0;

	// Set Exit Partitioning Flag 
	if (pictureControlSetPtr->encMode <= ENC_MODE_8) {
		contextPtr->mdContext->enableExitPartitioning = EB_FALSE;
	}
	else {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->enableExitPartitioning = EB_TRUE;
		}
		else {
			contextPtr->mdContext->enableExitPartitioning = EB_FALSE;
		}
	}

	// Set Limit INTRA Flag 
	if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
		contextPtr->mdContext->limitIntra = EB_FALSE;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_5) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_FALSE) {
				contextPtr->mdContext->limitIntra = EB_TRUE;
			}
			else {
				contextPtr->mdContext->limitIntra = EB_FALSE;
			}
		}
		else {
			contextPtr->mdContext->limitIntra = EB_FALSE;
		}
	}
	else {
		if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_FALSE) {
			contextPtr->mdContext->limitIntra = EB_TRUE;
		}
		else {
			contextPtr->mdContext->limitIntra = EB_FALSE;
		}
	}

	// Set MPM Level
	// Level    Settings 
	// 0        Full MPM: 3
	// 1        ON but 1
	// 2        OFF    
	if (pictureControlSetPtr->encMode <= ENC_MODE_0) {
		contextPtr->mdContext->mpmLevel = 0;
	}
	else if(pictureControlSetPtr->encMode <= ENC_MODE_9) {
		contextPtr->mdContext->mpmLevel = 2;
	}
	else {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->mpmLevel = 2;
		}
		else {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->mpmLevel = 1;
			}
			else {
				contextPtr->mdContext->mpmLevel = 2;
			}
		}
	}

	// Set PF @ MD Level
	// Level    Settings 
	// 0        OFF
	// 1        N2    
	// 2        M2 if 8x8 or 16x16 or Detector, N4 otherwise
	// 3        M2 if 8x8, N4 otherwise
	if (pictureControlSetPtr->encMode < ENC_MODE_8) {
		contextPtr->mdContext->pfMdLevel = 0;
	}
	else {
		if (pictureControlSetPtr->temporalLayerIndex == 0) {
			contextPtr->mdContext->pfMdLevel = 0;
		}
		else {
			contextPtr->mdContext->pfMdLevel = 1;
		}
	}

	// Set INTRA4x4 Search Level
	// Level    Settings 
	// 0        INLINE if not BDP, refinment otherwise 
	// 1        REFINMENT   
	// 2        OFF
	contextPtr->mdContext->intra4x4Level = (pictureControlSetPtr->encMode <= ENC_MODE_1) ? 0 : 2;

	// Set INTRA4x4 NFL
	contextPtr->mdContext->intra4x4Nfl = (pictureControlSetPtr->encMode <= ENC_MODE_1) ? 4 : 2;

	// Set INTRA4x4 Injection
	// 0: 35 mdoes
	// 1: up to 4: DC, Best INTR8x8, +3, -0
	contextPtr->mdContext->intra4x4IntraInjection = (pictureControlSetPtr->encMode <= ENC_MODE_1) ? 0 : 1;

	// NMM Level MD           Settings
	// 0                      5
	// 1                      3 if 32x32, 2 otherwise
	// 2                      2
	if (pictureControlSetPtr->encMode <= ENC_MODE_6) {
		contextPtr->mdContext->nmmLevelMd = 0;
	}
	else {
		contextPtr->mdContext->nmmLevelMd = 1;
	}

	// NMM Level BDP          Settings
	// 0                      5
	// 1                      3
	// 2                      3 if 32x32 or depth refinment true, 2 otherwise
	// 3                      3 if 32x32, 2 otherwise
	// 4                      2
	if (pictureControlSetPtr->encMode < ENC_MODE_7) {
		contextPtr->mdContext->nmmLevelBdp = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_9) {
		contextPtr->mdContext->nmmLevelBdp = 3;
	}
	else {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			contextPtr->mdContext->nmmLevelBdp = 3;
		}
		else {
			if (pictureControlSetPtr->ParentPcsPtr->temporalLayerIndex == 0) {
				contextPtr->mdContext->nmmLevelBdp = 1;
			}
			else {
				contextPtr->mdContext->nmmLevelBdp = 2;
			}
		}
	}

	// NFL Level MD         Settings
	// 0                    4
	// 1                    3 if 32x32, 2 otherwise
	// 2                    2
	// 3                    2 if Detectors, 1 otherwise
	// 4                    2 if 64x64 or 32x32 or 16x16, 1 otherwise
	// 5                    2 if 64x64 or 332x32, 1 otherwise
	// 6                    1
	if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->nflLevelMd = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			contextPtr->mdContext->nflLevelMd = 0;
		}
		else {
			if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				contextPtr->mdContext->nflLevelMd = 1;
			}
			else {
				contextPtr->mdContext->nflLevelMd = 2;
			}
		}
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelMd = 0;
			}
			else {
				if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
					contextPtr->mdContext->nflLevelMd = 1;
				}
				else {
					contextPtr->mdContext->nflLevelMd = 2;
				}
			}
		}
		else {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {

				contextPtr->mdContext->nflLevelMd = 1;
			}
			else {
				contextPtr->mdContext->nflLevelMd = 3;
			}
		}
	}
	else {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelMd = 1;			
		}
		else {
			contextPtr->mdContext->nflLevelMd = 3;
		}
	}

	// NFL Level Pillar/8x8 Refinement         Settings
	// 0                                       4
	// 1                                       4 if depthRefinment, 3 if 32x32, 2 otherwise
	// 2                                       3 
	// 3                                       3 if depthRefinment or 32x32, 2 otherwise
	// 4                                       3 if 32x32, 2 otherwise
	// 5                                       2    
	// 6                                       2 if Detectors, 1 otherwise
	// 7                                       2 if 64x64 or 32x32 or 16x16, 1 otherwise
	// 8                                       2 if 64x64 or 332x32, 1 otherwise
	// 9                                       1  
	if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->nflLevelPillar8x8ref = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				    contextPtr->mdContext->nflLevelPillar8x8ref = 0;
			}
			else {
				if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
					contextPtr->mdContext->nflLevelPillar8x8ref = 4;
				}
				else {
					contextPtr->mdContext->nflLevelPillar8x8ref = 5;
				}
			}
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelPillar8x8ref = 0;
			}
			else {
				if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
					contextPtr->mdContext->nflLevelPillar8x8ref = 4;
				}
				else {
					contextPtr->mdContext->nflLevelPillar8x8ref = 5;
				}
			}
		}
		else {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelPillar8x8ref = 4;
			}
			else {
				contextPtr->mdContext->nflLevelPillar8x8ref = 6;
			}
		}
	}
	else {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			contextPtr->mdContext->nflLevelPillar8x8ref = 4;
		}
		else {
			contextPtr->mdContext->nflLevelPillar8x8ref = 6;
		}
	}

	// NFL Level MvMerge/64x64 Refinement      Settings
	// 0                                       4
	// 1                                       3 
	// 2                                       3 if depthRefinment or 32x32, 2 otherwise
	// 3                                       3 if 32x32, 2 otherwise
	// 4                                       2    
	// 5                                       2 if Detectors, 1 otherwise
	// 6                                       2 if 64x64 or 32x32 or 16x16, 1 otherwise
	// 7                                       2 if 64x64 or 332x32, 1 otherwise
	// 8                                       1   
	if (pictureControlSetPtr->encMode <= ENC_MODE_2) {
		contextPtr->mdContext->nflLevelMvMerge64x64ref = 0;
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_3) {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			contextPtr->mdContext->nflLevelMvMerge64x64ref = 0;
		}
		else {
			if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
				contextPtr->mdContext->nflLevelMvMerge64x64ref = 3;
			}
			else {
				contextPtr->mdContext->nflLevelMvMerge64x64ref = 4;
			}
		}
	}
	else if (pictureControlSetPtr->encMode <= ENC_MODE_4) {
		if (sequenceControlSetPtr->inputResolution == INPUT_SIZE_4K_RANGE) {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelMvMerge64x64ref = 0;
			}
			else {
				if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
					contextPtr->mdContext->nflLevelMvMerge64x64ref = 3;
				}
				else {
					contextPtr->mdContext->nflLevelMvMerge64x64ref = 4;
				}
			}
		}
		else {
			if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
				contextPtr->mdContext->nflLevelMvMerge64x64ref = 3;
			}
			else {
				contextPtr->mdContext->nflLevelMvMerge64x64ref = 5;
			}
		}
	}
	else {
		if (pictureControlSetPtr->sliceType == EB_I_PICTURE) {
			contextPtr->mdContext->nflLevelMvMerge64x64ref = 3;
		}
		else {
			contextPtr->mdContext->nflLevelMvMerge64x64ref = 5;
		}
	}

	return return_error;
}

/******************************************************
 * EncDec Kernel
 ******************************************************/
void* EncDecKernel(void *inputPtr)
{
    // Context & SCS & PCS
    EncDecContext_t        *contextPtr = (EncDecContext_t*)inputPtr;
    PictureControlSet_t    *pictureControlSetPtr;
    SequenceControlSet_t   *sequenceControlSetPtr;
                           
    // Input               
    EbObjectWrapper_t      *encDecTasksWrapperPtr;
    EncDecTasks_t          *encDecTasksPtr;
                           
    // Output              
    EbObjectWrapper_t      *encDecResultsWrapperPtr;
    EncDecResults_t        *encDecResultsPtr;
    EbObjectWrapper_t      *pictureDemuxResultsWrapperPtr;
    PictureDemuxResults_t  *pictureDemuxResultsPtr;
                           
    // LCU Loop variables  
    LargestCodingUnit_t    *lcuPtr;
    EB_U16                  lcuIndex;
    EB_U8                   lcuSize;
    EB_U8                   lcuSizeLog2;
    EB_U32                  xLcuIndex;
    EB_U32                  yLcuIndex;
    EB_U32                  lcuOriginX;
    EB_U32                  lcuOriginY;
    EB_BOOL                 lastLcuFlag;
    EB_BOOL                 endOfRowFlag;
    EB_U32                  lcuRowIndexStart;
    EB_U32                  lcuRowIndexCount;
    EB_U32                  pictureWidthInLcu;
    EB_U32                  tileRowWidthInLcu;
    //EB_U32                  currentTileWidthInLcu;
    MdcLcuData_t           *mdcPtr;
    // Variables           
    EB_BOOL                 enableSaoFlag = EB_TRUE;
    EB_BOOL                 is16bit;
                           
    // Segments            
    //EB_BOOL                 initialProcessCall;
    EB_U16                  segmentIndex = 0;
    EB_U32                  xLcuStartIndex;
    EB_U32                  yLcuStartIndex;
    EB_U32                  lcuStartIndex;
    EB_U32                  lcuSegmentCount;
    EB_U32                  lcuSegmentIndex;
    EB_U32                  segmentRowIndex;
    EB_U32                  segmentBandIndex;
    EB_U32                  segmentBandSize;
    EncDecSegments_t       *segmentsPtr;
    EB_U32                  tileX, tileY, tileRowIndex;
    EB_U32                  tileGroupLcuStartX, tileGroupLcuStartY;


    for (;;) {

        // Get Mode Decision Results
        EbGetFullObject(
            contextPtr->modeDecisionInputFifoPtr,
            &encDecTasksWrapperPtr);
        EB_CHECK_END_OBJ(encDecTasksWrapperPtr);

        encDecTasksPtr = (EncDecTasks_t*)encDecTasksWrapperPtr->objectPtr;
        pictureControlSetPtr = (PictureControlSet_t*)encDecTasksPtr->pictureControlSetWrapperPtr->objectPtr;
        sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
        enableSaoFlag = (sequenceControlSetPtr->staticConfig.enableSaoFlag) ? EB_TRUE : EB_FALSE;
        tileRowIndex = encDecTasksPtr->tileRowIndex;

        segmentsPtr = pictureControlSetPtr->encDecSegmentCtrl[tileRowIndex];
        (void)tileX;
        (void)tileY;

        contextPtr->tileRowIndex = tileRowIndex;
        contextPtr->tileIndex = 0;

        tileGroupLcuStartX = tileGroupLcuStartY = 0;
        tileGroupLcuStartY = pictureControlSetPtr->ParentPcsPtr->tileRowStartLcu[tileRowIndex];
        lastLcuFlag = EB_FALSE;
        is16bit = (EB_BOOL)(sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);
#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld ENCDEC IN \n", pictureControlSetPtr->pictureNumber);
#endif

        // LCU Constants
        lcuSize = (EB_U8)sequenceControlSetPtr->lcuSize;
        lcuSizeLog2 = (EB_U8)Log2f(lcuSize);
        contextPtr->lcuSize = lcuSize;
        pictureWidthInLcu = (sequenceControlSetPtr->lumaWidth + lcuSize - 1) >> lcuSizeLog2;
        tileRowWidthInLcu = pictureWidthInLcu;
        endOfRowFlag = EB_FALSE;
        lcuRowIndexStart = lcuRowIndexCount = 0;
        contextPtr->totIntraCodedArea = 0;
        contextPtr->codedLcuCount = 0;

        // EncDec Kernel Signal(s) derivation
        if (sequenceControlSetPtr->staticConfig.tune == TUNE_SQ) {
            SignalDerivationEncDecKernelSq(
                    sequenceControlSetPtr,
                    pictureControlSetPtr,
                    contextPtr);
        }
        else if (sequenceControlSetPtr->staticConfig.tune == TUNE_VMAF) {
            SignalDerivationEncDecKernelVmaf(
                    sequenceControlSetPtr,
                    pictureControlSetPtr,
                    contextPtr);
        }
        else {
            SignalDerivationEncDecKernelOq(
                    sequenceControlSetPtr,
                    pictureControlSetPtr,
                    contextPtr);
        }

        // Derive Interpoldation Method @ Fast-Loop 
        contextPtr->mdContext->interpolationMethod = (pictureControlSetPtr->ParentPcsPtr->useSubpelFlag == EB_FALSE) ?
            INTERPOLATION_FREE_PATH  :
            INTERPOLATION_METHOD_HEVC;

        // Sep PM mode (active only when brr is ON)
        contextPtr->pmMode = sequenceControlSetPtr->inputResolution < INPUT_SIZE_4K_RANGE ?
            PM_MODE_1:
            PM_MODE_0;

        // Set Constrained INTRA Flag 
        pictureControlSetPtr->constrainedIntraFlag = (sequenceControlSetPtr->staticConfig.constrainedIntra == EB_TRUE && pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_FALSE) ?
            EB_TRUE :
            EB_FALSE;

        // Segment-loop
        while (AssignEncDecSegments(segmentsPtr, &segmentIndex, encDecTasksPtr, contextPtr->encDecFeedbackFifoPtr) == EB_TRUE)
        {
            // Per tile group(tile row)
            xLcuStartIndex = segmentsPtr->xStartArray[segmentIndex];
            yLcuStartIndex = segmentsPtr->yStartArray[segmentIndex];

            lcuStartIndex = yLcuStartIndex * tileRowWidthInLcu + xLcuStartIndex;
            lcuSegmentCount = segmentsPtr->validLcuCountArray[segmentIndex];

            segmentRowIndex = segmentIndex / segmentsPtr->segmentBandCount;
            segmentBandIndex = segmentIndex - segmentRowIndex * segmentsPtr->segmentBandCount;
            segmentBandSize = (segmentsPtr->lcuBandCount * (segmentBandIndex + 1) + segmentsPtr->segmentBandCount - 1) / segmentsPtr->segmentBandCount;


            // Reset Coding Loop State
            ProductResetModeDecision( // HT done 
                    contextPtr->mdContext,
                    pictureControlSetPtr,
                    sequenceControlSetPtr,
                    contextPtr->tileRowIndex,
                    segmentIndex);

            // Reset EncDec Coding State
            ResetEncDec(    // HT done
                    contextPtr,
                    pictureControlSetPtr,
                    sequenceControlSetPtr,
                    segmentIndex);

            contextPtr->mdContext->CabacCost = pictureControlSetPtr->cabacCost;

            if (pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr != NULL) {
                ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->picAvgVariance = pictureControlSetPtr->ParentPcsPtr->picAvgVariance;
                ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->averageIntensity = pictureControlSetPtr->ParentPcsPtr->averageIntensity[0];
            }

            if (sequenceControlSetPtr->staticConfig.improveSharpness || sequenceControlSetPtr->staticConfig.bitRateReduction) {

                QpmDeriveWeightsMinAndMax(
                        pictureControlSetPtr,
                        contextPtr);
            }

            for (yLcuIndex = yLcuStartIndex, lcuSegmentIndex = lcuStartIndex; lcuSegmentIndex < lcuStartIndex + lcuSegmentCount; ++yLcuIndex) {
                for (xLcuIndex = xLcuStartIndex; xLcuIndex < tileRowWidthInLcu && (xLcuIndex + yLcuIndex < segmentBandSize) && lcuSegmentIndex < lcuStartIndex + lcuSegmentCount; ++xLcuIndex, ++lcuSegmentIndex) {

                    // LCU per picture-wise
                    lcuIndex = (EB_U16)((tileGroupLcuStartY + yLcuIndex) * pictureWidthInLcu + (tileGroupLcuStartX + xLcuIndex));
                    lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIndex];
                    lcuOriginX = (xLcuIndex+tileGroupLcuStartX) << lcuSizeLog2;
                    lcuOriginY = (yLcuIndex+tileGroupLcuStartY) << lcuSizeLog2;
                    //printf("Process lcu (%d, %d), lcuIndex %d, segmentIndex %d\n", lcuOriginX, lcuOriginY, lcuIndex, segmentIndex);
                    
                    // Set current LCU tile Index
                    contextPtr->mdContext->tileIndex = lcuPtr->lcuEdgeInfoPtr->tileIndexInRaster;
                    contextPtr->tileIndex = lcuPtr->lcuEdgeInfoPtr->tileIndexInRaster;


                    endOfRowFlag = (xLcuIndex == tileRowWidthInLcu - 1) ? EB_TRUE : EB_FALSE;
                    lcuRowIndexStart = (xLcuIndex == tileRowWidthInLcu - 1 && lcuRowIndexCount == 0) ? yLcuIndex : lcuRowIndexStart;

                    // Jing: Send to entropy at tile group ends, not each tile for simplicity
                    lcuRowIndexCount = (xLcuIndex == tileRowWidthInLcu - 1) ? lcuRowIndexCount + 1 : lcuRowIndexCount;
                    mdcPtr = &pictureControlSetPtr->mdcLcuArray[lcuIndex];
                    contextPtr->lcuIndex = lcuIndex;

                    // Derive cuUseRefSrcFlag Flag
                    contextPtr->mdContext->cuUseRefSrcFlag = (pictureControlSetPtr->ParentPcsPtr->useSrcRef) && (pictureControlSetPtr->ParentPcsPtr->edgeResultsPtr[lcuIndex].edgeBlockNum == EB_FALSE || pictureControlSetPtr->ParentPcsPtr->lcuFlatNoiseArray[lcuIndex]) ? EB_TRUE : EB_FALSE;
                    // Derive restrictIntraGlobalMotion Flag
                    contextPtr->mdContext->restrictIntraGlobalMotion = ((pictureControlSetPtr->ParentPcsPtr->isPan || pictureControlSetPtr->ParentPcsPtr->isTilt) && pictureControlSetPtr->ParentPcsPtr->nonMovingIndexArray[lcuIndex] < INTRA_GLOBAL_MOTION_NON_MOVING_INDEX_TH && pictureControlSetPtr->ParentPcsPtr->yMean[lcuIndex][RASTER_SCAN_CU_INDEX_64x64] < INTRA_GLOBAL_MOTION_DARK_LCU_TH);

                    // Configure the LCU
                    ModeDecisionConfigureLcu(  // HT done
                            contextPtr->mdContext,
                            lcuPtr,
                            pictureControlSetPtr,
                            sequenceControlSetPtr,
                            contextPtr->qp,
                            lcuPtr->qp);

                    LcuParams_t * lcuParamPtr = &sequenceControlSetPtr->lcuParamsArray[lcuIndex];

                    if ( 
                            pictureControlSetPtr->ParentPcsPtr->depthMode  == PICT_FULL85_DEPTH_MODE || 
                            pictureControlSetPtr->ParentPcsPtr->depthMode  == PICT_FULL84_DEPTH_MODE || 
                            pictureControlSetPtr->ParentPcsPtr->depthMode  == PICT_OPEN_LOOP_DEPTH_MODE ||
                            (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && (pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_FULL85_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_FULL84_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_OPEN_LOOP_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_LIGHT_OPEN_LOOP_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_AVC_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_LIGHT_AVC_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_PRED_OPEN_LOOP_DEPTH_MODE || pictureControlSetPtr->ParentPcsPtr->lcuMdModeArray[lcuIndex] == LCU_PRED_OPEN_LOOP_1_NFL_DEPTH_MODE))) {

                        // Define Inputs / Outputs
                        ModeDecisionLcu( // HT done
                                sequenceControlSetPtr,
                                pictureControlSetPtr,
                                mdcPtr,
                                lcuPtr,
                                (EB_U16)lcuOriginX,
                                (EB_U16)lcuOriginY,
                                (EB_U32)lcuIndex,
                                contextPtr->mdContext);

                        // Muli-stage MD: INTRA_4x4 Refinment
                        ModeDecisionRefinementLcu(
                                pictureControlSetPtr,
                                lcuPtr,
                                lcuOriginX,
                                lcuOriginY,
                                contextPtr->mdContext);

                        // Link MD to BDP (could be done after INTRA4x4 refinment)
                        if (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && pictureControlSetPtr->bdpPresentFlag) {
                            LinkMdtoBdp(
                                    pictureControlSetPtr,
                                    lcuPtr,
                                    contextPtr->mdContext);
                        }
                    }
                    else {

                        // Pillar: 32x32 vs 16x16
                        BdpPillar(
                                sequenceControlSetPtr,
                                pictureControlSetPtr,
                                lcuParamPtr,
                                lcuPtr,
                                lcuIndex,
                                contextPtr->mdContext);

                        // If all 4 quadrants are CU32x32, THEN compare the 4 CU32x32 to CU64x64
                        EB_BOOL isFourCu32x32 = (lcuParamPtr->isCompleteLcu && pictureControlSetPtr->temporalLayerIndex > 0 &&
                                lcuPtr->codedLeafArrayPtr[1]->splitFlag == EB_FALSE  &&
                                lcuPtr->codedLeafArrayPtr[22]->splitFlag == EB_FALSE &&
                                lcuPtr->codedLeafArrayPtr[43]->splitFlag == EB_FALSE &&
                                lcuPtr->codedLeafArrayPtr[64]->splitFlag == EB_FALSE);

                        if (pictureControlSetPtr->sliceType != EB_I_PICTURE && isFourCu32x32) {

                            // 64x64 refinement stage
                            Bdp64x64vs32x32RefinementProcess(
                                    pictureControlSetPtr,
                                    lcuParamPtr,
                                    lcuPtr,
                                    lcuIndex,
                                    contextPtr->mdContext);
                        }

                        // 8x8 refinement stage
                        Bdp16x16vs8x8RefinementProcess(
                                sequenceControlSetPtr,
                                pictureControlSetPtr,
                                lcuParamPtr,
                                lcuPtr,
                                lcuIndex,
                                contextPtr->mdContext);

                        // MV Merge Pass
                        if (pictureControlSetPtr->sliceType != EB_I_PICTURE) {
                            BdpMvMergePass(
                                    pictureControlSetPtr,
                                    lcuParamPtr,
                                    lcuPtr,
                                    lcuIndex,
                                    contextPtr->mdContext);
                        }

                        // Muli-stage MD: INTRA_4x4 Refinment
                        ModeDecisionRefinementLcu(
                                pictureControlSetPtr,
                                lcuPtr,
                                lcuOriginX,
                                lcuOriginY,
                                contextPtr->mdContext);

                        // Link BDP to MD (could be done after INTRA4x4 refinment)
                        if (pictureControlSetPtr->ParentPcsPtr->depthMode == PICT_LCU_SWITCH_DEPTH_MODE && pictureControlSetPtr->mdPresentFlag) {

                            LinkBdptoMd(
                                    pictureControlSetPtr,
                                    lcuPtr,
                                    contextPtr->mdContext);
                        }

                    }

                    // Configure the LCU
                    EncDecConfigureLcu(         // HT done
                            contextPtr,
                            lcuPtr,
                            pictureControlSetPtr,
                            sequenceControlSetPtr,
                            contextPtr->qp,
                            lcuPtr->qp);


                    // Encode Pass
                    EncodePass(                 // HT done 
                            sequenceControlSetPtr,
                            pictureControlSetPtr,
                            lcuPtr,
                            lcuIndex,
                            lcuOriginX,
                            lcuOriginY,
                            lcuPtr->qp,
                            enableSaoFlag,
                            contextPtr);

                    if (pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr != NULL){
                        ((EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr)->intraCodedAreaLCU[lcuIndex] = (EB_U8)((100 * contextPtr->intraCodedAreaLCU[lcuIndex]) / (64 * 64));
                    }

                }
                xLcuStartIndex = (xLcuStartIndex > 0) ? xLcuStartIndex - 1 : 0;
            }
        }

        EbBlockOnMutex(pictureControlSetPtr->intraMutex);
        pictureControlSetPtr->intraCodedArea += (EB_U32)contextPtr->totIntraCodedArea;
        pictureControlSetPtr->encDecCodedLcuCount += (EB_U32)contextPtr->codedLcuCount;
        lastLcuFlag = (pictureControlSetPtr->lcuTotalCount == pictureControlSetPtr->encDecCodedLcuCount);
        //printf("[%p]: Tile %d, coded lcu count %d, total coded lcu count %d, lastLcuFlag is %d\n",
        //        contextPtr, encDecTasksPtr->tileIndex,
        //        contextPtr->codedLcuCount,
        //        pictureControlSetPtr->encDecCodedLcuCount, lastLcuFlag);
        EbReleaseMutex(pictureControlSetPtr->intraMutex);

        if (lastLcuFlag) {
            if (pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr != NULL){
                // copy stat to ref object (intraCodedArea, Luminance, Scene change detection flags)
                CopyStatisticsToRefObject(
                        pictureControlSetPtr,
                        sequenceControlSetPtr);
            }

            EB_BOOL applySAOAtEncoderFlag = sequenceControlSetPtr->staticConfig.enableSaoFlag &&
                (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag ||
                 sequenceControlSetPtr->staticConfig.reconEnabled);

            applySAOAtEncoderFlag = contextPtr->allowEncDecMismatch ? EB_FALSE : applySAOAtEncoderFlag;

            if (applySAOAtEncoderFlag)
            {
                if (is16bit) {
                    ApplySaoOffsetsPicture16bit(
                            contextPtr,
                            sequenceControlSetPtr,
                            pictureControlSetPtr);
                }
                else {
                    ApplySaoOffsetsPicture(
                            contextPtr,
                            sequenceControlSetPtr,
                            pictureControlSetPtr);
                }

            }

            // Pad the reference picture and set up TMVP flag and ref POC
            if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE)
                PadRefAndSetFlags(
                        pictureControlSetPtr,
                        sequenceControlSetPtr);

            if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag == EB_TRUE &&
                    pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr) {
                EbPictureBufferDesc_t *inputPicturePtr = (EbPictureBufferDesc_t*)pictureControlSetPtr->ParentPcsPtr->enhancedPicturePtr;
                EB_COLOR_FORMAT colorFormat = inputPicturePtr->colorFormat;
                EB_U16 subWidthCMinus1 = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
                EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;
                const EB_U32  SrclumaOffSet = inputPicturePtr->originX + inputPicturePtr->originY    *inputPicturePtr->strideY;
                const EB_U32 SrccbOffset = (inputPicturePtr->originX >> subWidthCMinus1) + (inputPicturePtr->originY >> subHeightCMinus1) * inputPicturePtr->strideCb;
                const EB_U32 SrccrOffset = (inputPicturePtr->originX >> subWidthCMinus1) + (inputPicturePtr->originY >> subHeightCMinus1) * inputPicturePtr->strideCr;

                EbReferenceObject_t   *referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr->objectPtr;
                EbPictureBufferDesc_t *refDenPic = referenceObject->refDenSrcPicture;
                const EB_U32           ReflumaOffSet = refDenPic->originX + refDenPic->originY    *refDenPic->strideY;
                const EB_U32 RefcbOffset = (refDenPic->originX >> subWidthCMinus1) + (refDenPic->originY >> subHeightCMinus1) * refDenPic->strideCb;
                const EB_U32 RefcrOffset = (refDenPic->originX >> subWidthCMinus1) + (refDenPic->originY >> subHeightCMinus1) * refDenPic->strideCr;

                EB_U16  verticalIdx;

                for (verticalIdx = 0; verticalIdx < refDenPic->height; ++verticalIdx)
                {
                    EB_MEMCPY(refDenPic->bufferY + ReflumaOffSet + verticalIdx*refDenPic->strideY,
                            inputPicturePtr->bufferY + SrclumaOffSet + verticalIdx* inputPicturePtr->strideY,
                            inputPicturePtr->width);
                }

                for (verticalIdx = 0; verticalIdx < inputPicturePtr->height >> subHeightCMinus1; ++verticalIdx)
                {
                    EB_MEMCPY(refDenPic->bufferCb + RefcbOffset + verticalIdx*refDenPic->strideCb,
                            inputPicturePtr->bufferCb + SrccbOffset + verticalIdx* inputPicturePtr->strideCb,
                            inputPicturePtr->width >> subWidthCMinus1);

                    EB_MEMCPY(refDenPic->bufferCr + RefcrOffset + verticalIdx*refDenPic->strideCr,
                            inputPicturePtr->bufferCr + SrccrOffset + verticalIdx* inputPicturePtr->strideCr,
                            inputPicturePtr->width >> subWidthCMinus1 );
                }

                GeneratePadding(
                        refDenPic->bufferY,
                        refDenPic->strideY,
                        refDenPic->width,
                        refDenPic->height,
                        refDenPic->originX,
                        refDenPic->originY);

                GeneratePadding(
                        refDenPic->bufferCb,
                        refDenPic->strideCb,
                        refDenPic->width >> subWidthCMinus1,
                        refDenPic->height >> subHeightCMinus1,
                        refDenPic->originX >> subWidthCMinus1,
                        refDenPic->originY >> subHeightCMinus1);

                GeneratePadding(
                        refDenPic->bufferCr,
                        refDenPic->strideCr,
                        refDenPic->width >> subWidthCMinus1,
                        refDenPic->height >> subHeightCMinus1,
                        refDenPic->originX >> subWidthCMinus1,
                        refDenPic->originY >> subHeightCMinus1);
            }

            if (sequenceControlSetPtr->staticConfig.reconEnabled) {
                ReconOutput(
                        pictureControlSetPtr,
                        sequenceControlSetPtr);
            }

            if (pictureControlSetPtr->ParentPcsPtr->isUsedAsReferenceFlag) {
                //Jing: TODO: double check here

                // Get Empty EntropyCoding Results
                EbGetEmptyObject(
                        contextPtr->pictureDemuxOutputFifoPtr,
                        &pictureDemuxResultsWrapperPtr);

                pictureDemuxResultsPtr = (PictureDemuxResults_t*)pictureDemuxResultsWrapperPtr->objectPtr;
                pictureDemuxResultsPtr->referencePictureWrapperPtr = pictureControlSetPtr->ParentPcsPtr->referencePictureWrapperPtr;
                pictureDemuxResultsPtr->sequenceControlSetWrapperPtr = pictureControlSetPtr->sequenceControlSetWrapperPtr;
                pictureDemuxResultsPtr->pictureNumber = pictureControlSetPtr->pictureNumber;
                pictureDemuxResultsPtr->pictureType = EB_PIC_REFERENCE;

                // Post Reference Picture
                EbPostFullObject(pictureDemuxResultsWrapperPtr);
#if LATENCY_PROFILE
                    double latency = 0.0;
                    EB_U64 finishTimeSeconds = 0;
                    EB_U64 finishTimeuSeconds = 0;
                    EbFinishTime((uint64_t*)&finishTimeSeconds, (uint64_t*)&finishTimeuSeconds);

                    EbComputeOverallElapsedTimeMs(
                            pictureControlSetPtr->ParentPcsPtr->startTimeSeconds,
                            pictureControlSetPtr->ParentPcsPtr->startTimeuSeconds,
                            finishTimeSeconds,
                            finishTimeuSeconds,
                            &latency);

                    SVT_LOG("POC %lld ENCDEC REF DONE, decoder order %d, latency %3.3f \n",
                            pictureControlSetPtr->pictureNumber,
                            pictureControlSetPtr->ParentPcsPtr->decodeOrder,
                            latency);
#endif
            }

            // When de interlacing is performed in the lib, each two consecutive pictures (fields: top & bottom) are going to use the same input buffer     
            // only when both fields are encoded we can free the input buffer
            // using the current prediction structure, bottom fields are usually encoded after top fields
            // so that when picture scan type is interlaced we free the input buffer after encoding the bottom field
            // we are trying to avoid making a such change in the APP (ideally an input buffer live count should be set in the APP (under EB_BUFFERHEADERTYPE data structure))

        }

#if DEADLOCK_DEBUG
        SVT_LOG("POC %lld ENCDEC OUT \n", pictureControlSetPtr->pictureNumber);
#endif

        // Send the Entropy Coder incremental updates as each LCU row becomes available
        {
            if (endOfRowFlag == EB_TRUE) {
                for (unsigned int tileIdx = tileRowIndex * pictureControlSetPtr->ParentPcsPtr->tileColumnCount;
                        tileIdx < (tileRowIndex + 1) * pictureControlSetPtr->ParentPcsPtr->tileColumnCount;
                        tileIdx++) {
                    // Get Empty EncDec Results
                    EbGetEmptyObject(
                            contextPtr->encDecOutputFifoPtr,
                            &encDecResultsWrapperPtr);
                    encDecResultsPtr = (EncDecResults_t*)encDecResultsWrapperPtr->objectPtr;
                    encDecResultsPtr->pictureControlSetWrapperPtr = encDecTasksPtr->pictureControlSetWrapperPtr;
                    encDecResultsPtr->completedLcuRowIndexStart = lcuRowIndexStart;
                    encDecResultsPtr->completedLcuRowCount = lcuRowIndexCount;
                    encDecResultsPtr->tileIndex = tileIdx;

                    // Post EncDec Results
                    EbPostFullObject(encDecResultsWrapperPtr);
                }
            }
        }

        // Release Mode Decision Results
        EbReleaseObject(encDecTasksWrapperPtr);

    }
    return EB_NULL;
}
