/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>

#include "EbPictureControlSet.h"
#include "EbReferenceObject.h"

#include "EbInterPrediction.h"
#include "EbMcp.h"
#include "EbAvcStyleMcp.h"
#include "EbAdaptiveMotionVectorPrediction.h"

#include "EbModeDecisionProcess.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

static void InterPredictionContextDtor(EB_PTR p)
{
    InterPredictionContext_t *obj = (InterPredictionContext_t*)p;
    EB_DELETE(obj->mcpContext);
}

EB_ERRORTYPE InterPredictionContextCtor(
    InterPredictionContext_t  *contextPtr,
	EB_U16                     maxCUWidth,
    EB_U16                     maxCUHeight,
    EB_BOOL                    is16bit)

{
    contextPtr->dctor = InterPredictionContextDtor;

    EB_NEW(
        contextPtr->mcpContext,
        MotionCompensationPredictionContextCtor,
        maxCUWidth,
        maxCUHeight,
        is16bit);

    return EB_ErrorNone;
}


void RoundMvOnTheFly(
	EB_S16 *motionVector_x,
	EB_S16 *motionVector_y)
{
	*motionVector_x = (*motionVector_x + 2)&~0x03;
	*motionVector_y = (*motionVector_y + 2)&~0x03;

	return;
}


EB_ERRORTYPE Inter2Nx2NPuPredictionInterpolationFree(
    ModeDecisionContext_t                  *mdContextPtr,
    EB_U32                                  componentMask,
    PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr)
{
    EB_ERRORTYPE            return_error = EB_ErrorNone;
    EbPictureBufferDesc_t  *refPicList0;
    EbPictureBufferDesc_t  *refPicList1;
    EbReferenceObject_t    *referenceObject;

    EB_U32                  refList0PosX;
    EB_U32                  refList0PosY;
    EB_U32                  refList1PosX;
    EB_U32                  refList1PosY;
    const EB_U32 puOriginX = mdContextPtr->cuOriginX;
    const EB_U32 puOriginY = mdContextPtr->cuOriginY;
    const EB_U32 puWidth = mdContextPtr->cuStats->size;
    const EB_U32 puHeight = mdContextPtr->cuStats->size;
    const EB_U32 puIndex = mdContextPtr->puItr;

    InterPredictionContext_t *contextPtr = (InterPredictionContext_t*)(mdContextPtr->interPredictionContext);
    EB_U32                  puOriginIndex;
    EB_U32                  puChromaOriginIndex;
    EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

    SequenceControlSet_t* sequenceControlSetPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr));
    EB_BOOL  is16bit = (EB_BOOL)(sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);

    EB_S16 motionVector_x;
    EB_S16 motionVector_y;

    if (mdContextPtr->cuUseRefSrcFlag)
        is16bit = EB_FALSE;
    switch (candidateBufferPtr->candidatePtr->predictionDirection[puIndex]) {
    case UNI_PRED_LIST_0:

        if (is16bit) {

            puOriginIndex = ((puOriginY  & (63)) * 64) + (puOriginX & (63));
            puChromaOriginIndex = (((puOriginY  & (63)) * 32) + (puOriginX & (63))) >> 1;
            referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
            refPicList0 = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;

            motionVector_x = candidateBufferPtr->candidatePtr->motionVector_x_L0;
            motionVector_y = candidateBufferPtr->candidatePtr->motionVector_y_L0;
            if (mdContextPtr->roundMvToInteger) {
                RoundMvOnTheFly(
                    &motionVector_x,
                    &motionVector_y);
            }

            refList0PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList0->width) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
            );

            refList0PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList0->height) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
            );

            UnpackUniPredRef10Bit(
                refPicList0,
                refList0PosX,
                refList0PosY,
                puWidth,
                puHeight,
                candidateBufferPtr->predictionPtr,
                puOriginIndex,
                puChromaOriginIndex,
                componentMask,
                contextPtr->mcpContext->avcStyleMcpIntermediateResultBuf0);


        }
        else {
            puOriginIndex = ((puOriginY  & (63)) * 64) + (puOriginX & (63));
            puChromaOriginIndex = (((puOriginY  & (63)) * 32) + (puOriginX & (63))) >> 1;
            referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
            refPicList0 = (EbPictureBufferDesc_t*)referenceObject->referencePicture;

            if (mdContextPtr->cuUseRefSrcFlag)
                refPicList0 = referenceObject->refDenSrcPicture;
            else
                refPicList0 = referenceObject->referencePicture;

            motionVector_x = candidateBufferPtr->candidatePtr->motionVector_x_L0;
            motionVector_y = candidateBufferPtr->candidatePtr->motionVector_y_L0;
            if (mdContextPtr->roundMvToInteger) {
                RoundMvOnTheFly(
                    &motionVector_x,
                    &motionVector_y);
            }

            refList0PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList0->width) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
            );

            refList0PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList0->height) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
            );

            UniPredIFreeRef8Bit(
                refPicList0,
                refList0PosX,
                refList0PosY,
                puWidth,
                puHeight,
                candidateBufferPtr->predictionPtr,
                puOriginIndex,
                puChromaOriginIndex,
                componentMask,
                contextPtr->mcpContext->avcStyleMcpIntermediateResultBuf0); // temp until mismatch fixed
        }

        break;

    case UNI_PRED_LIST_1:
        puOriginIndex = ((puOriginY  & (63)) * 64) + (puOriginX & (63));
        puChromaOriginIndex = (((puOriginY  & (63)) * 32) + (puOriginX & (63))) >> 1;
        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;

        if (is16bit) {

            refPicList1 = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;

            motionVector_x = candidateBufferPtr->candidatePtr->motionVector_x_L1;
            motionVector_y = candidateBufferPtr->candidatePtr->motionVector_y_L1;
            if (mdContextPtr->roundMvToInteger) {
                RoundMvOnTheFly(
                    &motionVector_x,
                    &motionVector_y);
            }
            refList1PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList1->width) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
            );
            refList1PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList1->height) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
            );

            UnpackUniPredRef10Bit(
                refPicList1,
                refList1PosX,
                refList1PosY,
                puWidth,
                puHeight,
                candidateBufferPtr->predictionPtr,
                puOriginIndex,
                puChromaOriginIndex,
                componentMask,
                contextPtr->mcpContext->avcStyleMcpIntermediateResultBuf0);



        }
        else {
            refPicList1 = (EbPictureBufferDesc_t*)referenceObject->referencePicture;

            if (mdContextPtr->cuUseRefSrcFlag)
                refPicList1 = referenceObject->refDenSrcPicture;
            else
                refPicList1 = referenceObject->referencePicture;

            motionVector_x = candidateBufferPtr->candidatePtr->motionVector_x_L1;
            motionVector_y = candidateBufferPtr->candidatePtr->motionVector_y_L1;
            if (mdContextPtr->roundMvToInteger) {
                RoundMvOnTheFly(
                    &motionVector_x,
                    &motionVector_y);
            }

            refList1PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList1->width) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
            );
            refList1PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList1->height) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
            );

            UniPredIFreeRef8Bit(
                refPicList1,
                refList1PosX,
                refList1PosY,
                puWidth,
                puHeight,
                candidateBufferPtr->predictionPtr,
                puOriginIndex,
                puChromaOriginIndex,
                componentMask,
                contextPtr->mcpContext->avcStyleMcpIntermediateResultBuf0);    

        }

        break;

    case BI_PRED:
        puOriginIndex = ((puOriginY  & (63)) * 64) + (puOriginX & (63));
        puChromaOriginIndex = (((puOriginY  & (63)) * 32) + (puOriginX & (63))) >> 1;

        // List0
        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;

        if (is16bit) {

            refPicList0 = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;

            motionVector_x = candidateBufferPtr->candidatePtr->motionVector_x_L0;
            motionVector_y = candidateBufferPtr->candidatePtr->motionVector_y_L0;
            if (mdContextPtr->roundMvToInteger) {
                RoundMvOnTheFly(
                    &motionVector_x,
                    &motionVector_y);
            }

            refList0PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList0->width) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
            );
            refList0PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList0->height) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
            );

            // List1
            referenceObject = (EbReferenceObject_t *)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;
            refPicList1 = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;

            motionVector_x = candidateBufferPtr->candidatePtr->motionVector_x_L1;
            motionVector_y = candidateBufferPtr->candidatePtr->motionVector_y_L1;
            if (mdContextPtr->roundMvToInteger) {
                RoundMvOnTheFly(
                    &motionVector_x,
                    &motionVector_y);
            }

            refList1PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList1->width) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
            );
            refList1PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList1->height) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
            );

            UnpackBiPredRef10Bit(
                refPicList0,
                refPicList1,
                refList0PosX,
                refList0PosY,
                refList1PosX,
                refList1PosY,
                puWidth,
                puHeight,
                candidateBufferPtr->predictionPtr,
                puOriginIndex,
                puChromaOriginIndex,
                componentMask,
                contextPtr->mcpContext->avcStyleMcpIntermediateResultBuf0,
                contextPtr->mcpContext->avcStyleMcpIntermediateResultBuf1,
                contextPtr->mcpContext->avcStyleMcpTwoDInterpolationFirstPassFilterResultBuf);



        }
        else {
            refPicList0 = (EbPictureBufferDesc_t*)referenceObject->referencePicture;

            if (mdContextPtr->cuUseRefSrcFlag)
                refPicList0 = referenceObject->refDenSrcPicture;
            else
                refPicList0 = referenceObject->referencePicture;


            motionVector_x = candidateBufferPtr->candidatePtr->motionVector_x_L0;
            motionVector_y = candidateBufferPtr->candidatePtr->motionVector_y_L0;
            if (mdContextPtr->roundMvToInteger) {
                RoundMvOnTheFly(
                    &motionVector_x,
                    &motionVector_y);
            }

            refList0PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList0->width) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
            );
            refList0PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList0->height) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
            );

            // List1
            referenceObject = (EbReferenceObject_t *)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;
            refPicList1 = (EbPictureBufferDesc_t*)referenceObject->referencePicture;

            if (mdContextPtr->cuUseRefSrcFlag)
                refPicList1 = referenceObject->refDenSrcPicture;
            else
                refPicList1 = referenceObject->referencePicture;

            motionVector_x = candidateBufferPtr->candidatePtr->motionVector_x_L1;
            motionVector_y = candidateBufferPtr->candidatePtr->motionVector_y_L1;
            if (mdContextPtr->roundMvToInteger) {
                RoundMvOnTheFly(
                    &motionVector_x,
                    &motionVector_y);
            }

            refList1PosX = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList1->width) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginX << 2) + REFPADD_QPEL + motionVector_x))
            );
            refList1PosY = (EB_U32)CLIP3((EB_S32)MVBOUNDLOW,
                (EB_S32)(((refPicList1->height) << 2) + MVBOUNDHIGH),
                (EB_S32)(((puOriginY << 2) + REFPADD_QPEL + motionVector_y))
            );

            BiPredIFreeRef8Bit(
                refPicList0,
                refPicList1,
                refList0PosX,
                refList0PosY,
                refList1PosX,
                refList1PosY,
                puWidth,
                puHeight,
                candidateBufferPtr->predictionPtr,
                puOriginIndex,
                puChromaOriginIndex,
                componentMask,
                contextPtr->mcpContext->avcStyleMcpIntermediateResultBuf0,
                contextPtr->mcpContext->avcStyleMcpIntermediateResultBuf1,
                contextPtr->mcpContext->avcStyleMcpTwoDInterpolationFirstPassFilterResultBuf); 
        }

        break;

    default:
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr,
            EB_ENC_INTER_PRED_ERROR0);
        break;
    }

    return return_error;
}



/***************************************************
*  PreLoad Reference Block  for 16bit mode
***************************************************/
void UnPackReferenceBlock(
	EbPictureBufferDesc_t *refFramePic,
	EB_U32                 posX,
	EB_U32                 posY,
	EB_U32                 puWidth,
	EB_U32                 puHeight,
	EbPictureBufferDesc_t *dst,
	EB_U32				   componentMask)
{

	puWidth  += 8;						// 4 + PU_WIDTH + 4 to account for 8-tap interpolation
	puHeight += 8;						// 4 + PU_WIDTH + 4 to account for 8-tap interpolation
	EB_U32 inPosx = (posX >> 2) - 4;	// -4 to account for 8-tap interpolation 
	EB_U32 inPosy = (posY >> 2) - 4;	// -4 to account for 8-tap interpolation 
	EB_U16 *ptr16;

	if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {
		ptr16 = (EB_U16 *)refFramePic->bufferY + inPosx + inPosy*refFramePic->strideY;

		Extract8BitdataSafeSub(
			ptr16,
			refFramePic->strideY,
			dst->bufferY,
			dst->strideY,
			puWidth,
			puHeight
		);
	}

	if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {
		ptr16 = (EB_U16 *)refFramePic->bufferCb + (inPosx >> 1) + (inPosy >> 1) * refFramePic->strideCb;

		Extract8BitdataSafeSub(
			ptr16,
			refFramePic->strideCb,
			dst->bufferCb,
			dst->strideCb,
			puWidth >> 1,
			puHeight >> 1
		);

		ptr16 = (EB_U16 *)refFramePic->bufferCr + (inPosx >> 1) + (inPosy >> 1)  *refFramePic->strideCr;

		Extract8BitdataSafeSub(
			ptr16,
			refFramePic->strideCr,
			dst->bufferCr,
			dst->strideCr,
			puWidth >> 1,
			puHeight >> 1
		);
	}

}

EB_ERRORTYPE Inter2Nx2NPuPredictionHevc(
    ModeDecisionContext_t                  *mdContextPtr,
    EB_U32                                  componentMask,
    PictureControlSet_t                    *pictureControlSetPtr,
    ModeDecisionCandidateBuffer_t          *candidateBufferPtr)
{

    EB_ERRORTYPE              return_error    = EB_ErrorNone;
    EbPictureBufferDesc_t    *refPicList0     = 0;
    EbPictureBufferDesc_t    *refPicList1     = 0;
    EbReferenceObject_t      *referenceObject;
    EB_U16                    refList0PosX    = 0;
    EB_U16                    refList0PosY    = 0;
    EB_U16                    refList1PosX    = 0;
    EB_U16                    refList1PosY    = 0;


    const EB_U32 puOriginX  = mdContextPtr->cuOriginX;
    const EB_U32 puOriginY  = mdContextPtr->cuOriginY;
    const EB_U32 puWidth    = mdContextPtr->cuStats->size;
    const EB_U32 puHeight   = mdContextPtr->cuStats->size;
    const EB_U32 puIndex    = mdContextPtr->puItr;

    EB_U32                    puOriginIndex       = ((puOriginY  & (63)) * 64) + (puOriginX & (63));
    EB_U32                    puChromaOriginIndex = ((((puOriginY  & (63)) * 32) + (puOriginX & (63))) >> 1);
    

    SequenceControlSet_t     *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
    EncodeContext_t          *encodeContextPtr      = sequenceControlSetPtr->encodeContextPtr;
    InterPredictionContext_t *contextPtr            = (InterPredictionContext_t*)(mdContextPtr->interPredictionContext);

    EB_BOOL  is16bit = (EB_BOOL)(sequenceControlSetPtr->staticConfig.encoderBitDepth > EB_8BIT);

    EB_S16 motionVector_x;
    EB_S16 motionVector_y;

    if (mdContextPtr->cuUseRefSrcFlag)
        is16bit = EB_FALSE;


    // Setup List 0
    if (candidateBufferPtr->candidatePtr->predictionDirection[puIndex] == UNI_PRED_LIST_0 || candidateBufferPtr->candidatePtr->predictionDirection[puIndex] == BI_PRED) {
    
        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;

        if (mdContextPtr->cuUseRefSrcFlag)
            refPicList0 = referenceObject->refDenSrcPicture;
        else
			refPicList0 = is16bit ? referenceObject->referencePicture16bit : referenceObject->referencePicture ;


        motionVector_x = candidateBufferPtr->candidatePtr->motionVector_x_L0;
        motionVector_y = candidateBufferPtr->candidatePtr->motionVector_y_L0;

        // minus 71 is derived from the expression -64 + 1 - 8, and plus 7 is derived from expression -1 + 8
        refList0PosX = (EB_U16)CLIP3(
            (EB_S32)((refPicList0->originX - 71) << 2),
            (EB_S32)((refPicList0->width + refPicList0->originX + 7) << 2),
            (EB_S32)((puOriginX + refPicList0->originX) << 2) + motionVector_x);

        refList0PosY = (EB_U16)CLIP3(
            (EB_S32)((refPicList0->originY - 71) << 2),
            (EB_S32)((refPicList0->height + refPicList0->originY + 7) << 2),
            (EB_S32)((puOriginY + refPicList0->originY) << 2) + motionVector_y);

        CHECK_REPORT_ERROR(
            (refList0PosX < ((refPicList0->width + (refPicList0->originX << 1)) << 2)),
            encodeContextPtr->appCallbackPtr,
            EB_ENC_INTER_INVLD_MCP_ERROR);

        CHECK_REPORT_ERROR(
            (refList0PosY < ((refPicList0->height + (refPicList0->originY << 1)) << 2)),
            encodeContextPtr->appCallbackPtr,
            EB_ENC_INTER_INVLD_MCP_ERROR);

    }

    // Setup List 1
    if (candidateBufferPtr->candidatePtr->predictionDirection[puIndex] == UNI_PRED_LIST_1 || candidateBufferPtr->candidatePtr->predictionDirection[puIndex] == BI_PRED) {

        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;

        if (mdContextPtr->cuUseRefSrcFlag)
            refPicList1 = referenceObject->refDenSrcPicture;
        else
			refPicList1 = is16bit ? referenceObject->referencePicture16bit : referenceObject->referencePicture;


        motionVector_x = candidateBufferPtr->candidatePtr->motionVector_x_L1;
        motionVector_y = candidateBufferPtr->candidatePtr->motionVector_y_L1;

        // minus 71 is derived from the expression -64 + 1 - 8, and plus 7 is derived from expression -1 + 8
        refList1PosX = (EB_U16)CLIP3(
            (EB_S32)((refPicList1->originX - 71) << 2),
            (EB_S32)((refPicList1->width + refPicList1->originX + 7) << 2),
            (EB_S32)((puOriginX + refPicList1->originX) << 2) + motionVector_x);

        refList1PosY = (EB_U16)CLIP3(
            (EB_S32)((refPicList1->originY - 71) << 2),
            (EB_S32)((refPicList1->height + refPicList1->originY + 7) << 2),
            (EB_S32)((puOriginY + refPicList1->originY) << 2) + motionVector_y);

        CHECK_REPORT_ERROR(
            (refList1PosX < ((refPicList1->width + (refPicList1->originX << 1)) << 2)),
            encodeContextPtr->appCallbackPtr,
            EB_ENC_INTER_INVLD_MCP_ERROR);

        CHECK_REPORT_ERROR(
            (refList1PosY  < ((refPicList1->height + (refPicList1->originY << 1)) << 2)),
            encodeContextPtr->appCallbackPtr,
            EB_ENC_INTER_INVLD_MCP_ERROR);

    }

    switch (candidateBufferPtr->candidatePtr->predictionDirection[puIndex]) {

    case UNI_PRED_LIST_0:

		if (is16bit && mdContextPtr->cuUseRefSrcFlag == EB_FALSE) {
            if (refPicList0)
			    UnPackReferenceBlock(
				    refPicList0,
				    refList0PosX,
				    refList0PosY,
				    puWidth,
				    puHeight,
				    contextPtr->mcpContext->localReferenceBlock8BITL0,
				    componentMask);

			UniPredHevcInterpolationMd(
				contextPtr->mcpContext->localReferenceBlock8BITL0,
				refList0PosX,
				refList0PosY,
				puWidth,
				puHeight,
				candidateBufferPtr->predictionPtr,
				puOriginIndex,
				puChromaOriginIndex,
				contextPtr->mcpContext->motionCompensationIntermediateResultBuf0,
				contextPtr->mcpContext->TwoDInterpolationFirstPassFilterResultBuf,
				EB_TRUE,
				componentMask);

        }
        else {
            if (refPicList0) {
                UniPredHevcInterpolationMd(
                    refPicList0,
                    refList0PosX,
                    refList0PosY,
                    puWidth,
                    puHeight,
                    candidateBufferPtr->predictionPtr,
                    puOriginIndex,
                    puChromaOriginIndex,
                    contextPtr->mcpContext->motionCompensationIntermediateResultBuf0,
                    contextPtr->mcpContext->TwoDInterpolationFirstPassFilterResultBuf,
                    EB_FALSE,
                    componentMask);
            }
        }

        break;

    case UNI_PRED_LIST_1:

		if (is16bit && mdContextPtr->cuUseRefSrcFlag == EB_FALSE) {
            if (refPicList1)
			    UnPackReferenceBlock(
				    refPicList1,
				    refList1PosX,
				    refList1PosY,
				    puWidth,
				    puHeight,
				    contextPtr->mcpContext->localReferenceBlock8BITL1,
				    componentMask);

			UniPredHevcInterpolationMd(
				contextPtr->mcpContext->localReferenceBlock8BITL1,
				refList1PosX,
				refList1PosY,
				puWidth,
				puHeight,
				candidateBufferPtr->predictionPtr,
				puOriginIndex,
				puChromaOriginIndex,
				contextPtr->mcpContext->motionCompensationIntermediateResultBuf0,
				contextPtr->mcpContext->TwoDInterpolationFirstPassFilterResultBuf,
				EB_TRUE,
				componentMask);

        }
        else {
            if (refPicList1) {
                UniPredHevcInterpolationMd(
                    refPicList1,
                    refList1PosX,
                    refList1PosY,
                    puWidth,
                    puHeight,
                    candidateBufferPtr->predictionPtr,
                    puOriginIndex,
                    puChromaOriginIndex,
                    contextPtr->mcpContext->motionCompensationIntermediateResultBuf0,
                    contextPtr->mcpContext->TwoDInterpolationFirstPassFilterResultBuf,
                    EB_FALSE,
                    componentMask);
            }
        }

        break;

    case BI_PRED:

		if (is16bit && mdContextPtr->cuUseRefSrcFlag == EB_FALSE) {
            if (refPicList0)
			    UnPackReferenceBlock(
				    refPicList0,
				    refList0PosX,
				    refList0PosY,
				    puWidth,
				    puHeight,
				    contextPtr->mcpContext->localReferenceBlock8BITL0,
				    componentMask);

            if (refPicList1)
			    UnPackReferenceBlock(
				    refPicList1,
				    refList1PosX,
				    refList1PosY,
				    puWidth,
				    puHeight,
				    contextPtr->mcpContext->localReferenceBlock8BITL1,
				    componentMask);

			BiPredHevcInterpolationMd(
				contextPtr->mcpContext->localReferenceBlock8BITL0,
				contextPtr->mcpContext->localReferenceBlock8BITL1,
				refList0PosX,
				refList0PosY,
				refList1PosX,
				refList1PosY,
				puWidth,
				puHeight,
				candidateBufferPtr->predictionPtr,
				puOriginIndex,
				puChromaOriginIndex,
				contextPtr->mcpContext->motionCompensationIntermediateResultBuf0,
				contextPtr->mcpContext->motionCompensationIntermediateResultBuf1,
				contextPtr->mcpContext->TwoDInterpolationFirstPassFilterResultBuf,
				EB_TRUE,
				componentMask);


        }
        else {
            if (refPicList0 && refPicList1) {
                BiPredHevcInterpolationMd(
                    refPicList0,
                    refPicList1,
                    refList0PosX,
                    refList0PosY,
                    refList1PosX,
                    refList1PosY,
                    puWidth,
                    puHeight,
                    candidateBufferPtr->predictionPtr,
                    puOriginIndex,
                    puChromaOriginIndex,
                    contextPtr->mcpContext->motionCompensationIntermediateResultBuf0,
                    contextPtr->mcpContext->motionCompensationIntermediateResultBuf1,
                    contextPtr->mcpContext->TwoDInterpolationFirstPassFilterResultBuf,
                    EB_FALSE,
                    componentMask);
            }
        }

        break;

    default:
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr,
            EB_ENC_INTER_PRED_ERROR0);
        break;
    }

    return return_error;
}

/***************************************************
 * Inter Prediction used in the Encode Pass
 ***************************************************/
EB_ERRORTYPE EncodePassInterPrediction(
	MvUnit_t                               *mvUnit,
	EB_U16                                  puOriginX,
	EB_U16                                  puOriginY,
	EB_U8                                   puWidth,
	EB_U8                                   puHeight,

    PictureControlSet_t                    *pictureControlSetPtr,
    EbPictureBufferDesc_t                  *predictionPtr,
    MotionCompensationPredictionContext_t  *mcpContext)
{
    EB_ERRORTYPE            return_error    =   EB_ErrorNone;
    EbPictureBufferDesc_t  *refPicList0     =   0;
    EbPictureBufferDesc_t  *refPicList1     =   0;
    EbReferenceObject_t    *referenceObject;
	EB_U16                  refList0PosX = 0;
	EB_U16                  refList0PosY = 0;
	EB_U16                  refList1PosX = 0;
	EB_U16                  refList1PosY = 0;

    EB_COLOR_FORMAT colorFormat=predictionPtr->colorFormat;
    EB_U16 subWidthCMinus1  = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;

    EB_U32                  puOriginIndex           = ((predictionPtr->originY +puOriginY) * predictionPtr->strideY)  + (predictionPtr->originX+puOriginX);
    EB_U32                  puChromaOriginIndex     = (((predictionPtr->originY+puOriginY) * predictionPtr->strideCb) >> subHeightCMinus1) + ((predictionPtr->originX+puOriginX) >> subWidthCMinus1);
    SequenceControlSet_t   *sequenceControlSetPtr  = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	EncodeContext_t        *encodeContextPtr       = sequenceControlSetPtr->encodeContextPtr;

    // Setup List 0
    if(mvUnit->predDirection == UNI_PRED_LIST_0 || mvUnit->predDirection == BI_PRED) {

        //refPicList0Idx  = candidateBufferPtr->candidatePtr->motionVectorIdx[REF_LIST_0][puIndex];
        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
        refPicList0     = (EbPictureBufferDesc_t*) referenceObject->referencePicture;

        // minus 71 is derived from the expression -64 + 1 - 8, and plus 7 is derived from expression -1 + 8
        //refList0PosX = (EB_U32) CLIP3((EB_S32)((refPicList0->originX-71) << 2), (EB_S32)((refPicList0->width + refPicList0->originX + 7)<<2) , (EB_S32) ((puOriginX + tbOriginX + refPicList0->originX)<<2) + candidateBufferPtr->candidatePtr->motionVector_x[REF_LIST_0][puIndex]);
        refList0PosX = (EB_U32) CLIP3(
                           (EB_S32)((refPicList0->originX-71) << 2),
                           (EB_S32)((refPicList0->width + refPicList0->originX + 7)<<2) ,
                           (EB_S32) ((puOriginX+ refPicList0->originX)<<2) + mvUnit->mv[REF_LIST_0].x);
        //refList0PosY = (EB_U32) CLIP3((EB_S32)((refPicList0->originY-71) << 2),(EB_S32)((refPicList0->height + refPicList0->originY + 7)<<2) , (EB_S32) ((puOriginY + tbOriginY + refPicList0->originY)<<2) + candidateBufferPtr->candidatePtr->motionVector_y[REF_LIST_0][puIndex]);
        refList0PosY = (EB_U32) CLIP3(
                           (EB_S32)((refPicList0->originY-71) << 2),
                           (EB_S32)((refPicList0->height + refPicList0->originY + 7)<<2) ,
                           (EB_S32) ((puOriginY + refPicList0->originY)<<2) + mvUnit->mv[REF_LIST_0].y);

        CHECK_REPORT_ERROR(
            (refList0PosX < ((refPicList0->width + (refPicList0->originX << 1)) << 2)),
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_INTER_INVLD_MCP_ERROR);

        CHECK_REPORT_ERROR(
            (refList0PosY < ((refPicList0->height + (refPicList0->originY << 1)) << 2)),
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_INTER_INVLD_MCP_ERROR);

    }

    // Setup List 1
    if(mvUnit->predDirection == UNI_PRED_LIST_1 || mvUnit->predDirection == BI_PRED) {

        //refPicList1Idx  = candidateBufferPtr->candidatePtr->motionVectorIdx[REF_LIST_1][puIndex];
        referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;
        refPicList1     = (EbPictureBufferDesc_t*) referenceObject->referencePicture;

        //refList1PosX = ((puOriginX + tbOriginX + refPicList1->originX)<<2) + candidateBufferPtr->candidatePtr->motionVector_x[REF_LIST_1][puIndex];
        //refList1PosX = ((puOriginX + refPicList1->originX)<<2) + mvUnit->xMv[REF_LIST_1];
        //refList1PosY = ((puOriginY + tbOriginY + refPicList1->originY)<<2) + candidateBufferPtr->candidatePtr->motionVector_y[REF_LIST_1][puIndex];
        //refList1PosY = ((puOriginY + refPicList1->originY)<<2) + mvUnit->yMv[REF_LIST_1];

        // minus 71 is derived from the expression -64 + 1 - 8, and plus 7 is derived from expression -1 + 8
        //refList1PosX = (EB_U32) CLIP3((EB_S32)((refPicList1->originX-71) << 2), (EB_S32)((refPicList1->width + refPicList1->originX  + 7)<<2) , (EB_S32) ((puOriginX + tbOriginX + refPicList1->originX)<<2) + candidateBufferPtr->candidatePtr->motionVector_x[REF_LIST_1][puIndex]);
        refList1PosX = (EB_U32) CLIP3(
                           (EB_S32)((refPicList1->originX-71) << 2),
                           (EB_S32)((refPicList1->width + refPicList1->originX  + 7)<<2) ,
                           (EB_S32) ((puOriginX + refPicList1->originX)<<2) + mvUnit->mv[REF_LIST_1].x);
        //refList1PosY = (EB_U32) CLIP3((EB_S32)((refPicList1->originY-71) << 2), (EB_S32)((refPicList1->height + refPicList1->originY + 7)<<2) , (EB_S32) ((puOriginY + tbOriginY + refPicList1->originY)<<2) + candidateBufferPtr->candidatePtr->motionVector_y[REF_LIST_1][puIndex]);
        refList1PosY = (EB_U32) CLIP3(
                           (EB_S32)((refPicList1->originY-71) << 2),
                           (EB_S32)((refPicList1->height + refPicList1->originY + 7)<<2) ,
                           (EB_S32) ((puOriginY + refPicList1->originY)<<2) + mvUnit->mv[REF_LIST_1].y);

        CHECK_REPORT_ERROR(
            (refList1PosX < ((refPicList1->width + (refPicList1->originX << 1)) << 2)),
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_INTER_INVLD_MCP_ERROR);

        CHECK_REPORT_ERROR(
            (refList1PosY  < ((refPicList1->height + (refPicList1->originY << 1)) << 2)),
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_INTER_INVLD_MCP_ERROR);

    }


    //switch(candidateBufferPtr->candidatePtr->predictionDirection[puIndex]) {
    switch(mvUnit->predDirection)
    {

    case UNI_PRED_LIST_0:

        EncodeUniPredInterpolation(
            refPicList0,
            refList0PosX,
            refList0PosY,
            puWidth,
            puHeight,
            predictionPtr,
            puOriginIndex,
            puChromaOriginIndex,
            mcpContext->motionCompensationIntermediateResultBuf0,
            mcpContext->TwoDInterpolationFirstPassFilterResultBuf);

        break;

    case UNI_PRED_LIST_1:

        EncodeUniPredInterpolation(
            refPicList1,
            refList1PosX,
            refList1PosY,
            puWidth,
            puHeight,
            predictionPtr,
            puOriginIndex,
            puChromaOriginIndex,
            mcpContext->motionCompensationIntermediateResultBuf0,
            mcpContext->TwoDInterpolationFirstPassFilterResultBuf);

        break;

    case BI_PRED:

        EncodeBiPredInterpolation(
            refPicList0,
            refPicList1,
            refList0PosX,
            refList0PosY,
            refList1PosX,
            refList1PosY,
            puWidth,
            puHeight,
            predictionPtr,
            puOriginIndex,
            puChromaOriginIndex,
            mcpContext->motionCompensationIntermediateResultBuf0,
            mcpContext->motionCompensationIntermediateResultBuf1,
            mcpContext->TwoDInterpolationFirstPassFilterResultBuf);

        break;

    default:
        CHECK_REPORT_ERROR_NC(
            encodeContextPtr->appCallbackPtr, 
            EB_ENC_INTER_PRED_ERROR0);
        break;
    }

    return return_error;
}


/***************************************************
 * Inter Prediction used in the Encode Pass  16bit
 ***************************************************/
EB_ERRORTYPE EncodePassInterPrediction16bit(
    MvUnit_t                               *mvUnit,
	EB_U16                                  puOriginX,
	EB_U16                                  puOriginY,
	EB_U8                                   puWidth,
	EB_U8                                   puHeight,
    PictureControlSet_t                    *pictureControlSetPtr,
    EbPictureBufferDesc_t                  *predictionPtr,
    MotionCompensationPredictionContext_t  *mcpContext)
{

    EB_ERRORTYPE            return_error    = EB_ErrorNone;
    EbPictureBufferDesc_t  *refPicList0     = 0;
    EbPictureBufferDesc_t  *refPicList1     = 0;
    EbReferenceObject_t    *referenceObject;
	EB_U16                  refList0PosX = 0;
	EB_U16                  refList0PosY = 0;
	EB_U16                  refList1PosX = 0;
	EB_U16                  refList1PosY = 0;
    const EB_COLOR_FORMAT colorFormat = predictionPtr->colorFormat;
    const EB_U16 subWidthCMinus1  = (colorFormat == EB_YUV444 ? 1 : 2) - 1;
    const EB_U16 subHeightCMinus1 = (colorFormat >= EB_YUV422 ? 1 : 2) - 1;


    EB_U32 puOriginIndex = (predictionPtr->originX + puOriginX) +
        ((predictionPtr->originY+puOriginY) * predictionPtr->strideY);
    EB_U32 puChromaOriginIndex = ((predictionPtr->originX + puOriginX) >> subWidthCMinus1) +
        (((predictionPtr->originY+puOriginY) * predictionPtr->strideCb) >> subHeightCMinus1);
    SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	EncodeContext_t *encodeContextPtr = sequenceControlSetPtr->encodeContextPtr;

	// Setup List 0
	if (mvUnit->predDirection == UNI_PRED_LIST_0 || mvUnit->predDirection == BI_PRED) {
		referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr;
		refPicList0 = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;

		// minus 71 is derived from the expression -64 + 1 - 8, and plus 7 is derived from expression -1 + 8
		//refList0PosX = (EB_U32) CLIP3((EB_S32)((refPicList0->originX-71) << 2), (EB_S32)((refPicList0->width + refPicList0->originX + 7)<<2) , (EB_S32) ((puOriginX + tbOriginX + refPicList0->originX)<<2) + candidateBufferPtr->candidatePtr->motionVector_x[REF_LIST_0][puIndex]);
		refList0PosX = (EB_U32)CLIP3(
			(EB_S32)((refPicList0->originX - 71) << 2),
			(EB_S32)((refPicList0->width + refPicList0->originX + 7) << 2),
			(EB_S32)((puOriginX + refPicList0->originX) << 2) + mvUnit->mv[REF_LIST_0].x);
		//refList0PosY = (EB_U32) CLIP3((EB_S32)((refPicList0->originY-71) << 2),(EB_S32)((refPicList0->height + refPicList0->originY + 7)<<2) , (EB_S32) ((puOriginY + tbOriginY + refPicList0->originY)<<2) + candidateBufferPtr->candidatePtr->motionVector_y[REF_LIST_0][puIndex]);
		refList0PosY = (EB_U32)CLIP3(
			(EB_S32)((refPicList0->originY - 71) << 2),
			(EB_S32)((refPicList0->height + refPicList0->originY + 7) << 2),
			(EB_S32)((puOriginY + refPicList0->originY) << 2) + mvUnit->mv[REF_LIST_0].y);


		CHECK_REPORT_ERROR(
			(refList0PosX < ((refPicList0->width + (refPicList0->originX << 1)) << 2)),
			encodeContextPtr->appCallbackPtr,
			EB_ENC_INTER_INVLD_MCP_ERROR);

		CHECK_REPORT_ERROR(
			(refList0PosY < ((refPicList0->height + (refPicList0->originY << 1)) << 2)),
			encodeContextPtr->appCallbackPtr,
			EB_ENC_INTER_INVLD_MCP_ERROR);

		EB_U32  lumaOffSet = ((refList0PosX >> 2) - 4) * 2 + ((refList0PosY >> 2) - 4) * 2 * refPicList0->strideY; //refPicList0->originX + refPicList0->originY*refPicList0->strideY; //
		EB_U32  cbOffset = ((refList0PosX >> (2 + subWidthCMinus1)) - 2) * 2 + ((refList0PosY >> (2 + subHeightCMinus1)) - 2) * 2 * refPicList0->strideCb; //Jing:double check for 444
		EB_U32  crOffset = ((refList0PosX >> (2 + subWidthCMinus1)) - 2) * 2 + ((refList0PosY >> (2 + subHeightCMinus1)) - 2) * 2 * refPicList0->strideCr;
		//EB_U8  verticalIdx;

		mcpContext->localReferenceBlockL0->bufferY = refPicList0->bufferY + lumaOffSet;
		mcpContext->localReferenceBlockL0->bufferCb = refPicList0->bufferCb + cbOffset;
		mcpContext->localReferenceBlockL0->bufferCr = refPicList0->bufferCr + crOffset;
		mcpContext->localReferenceBlockL0->strideY = refPicList0->strideY;
		mcpContext->localReferenceBlockL0->strideCb = refPicList0->strideCb;
		mcpContext->localReferenceBlockL0->strideCr = refPicList0->strideCr;
	}

	// Setup List 1
	if (mvUnit->predDirection == UNI_PRED_LIST_1 || mvUnit->predDirection == BI_PRED) {

		referenceObject = (EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr;
		refPicList1 = (EbPictureBufferDesc_t*)referenceObject->referencePicture16bit;

		// minus 71 is derived from the expression -64 + 1 - 8, and plus 7 is derived from expression -1 + 8
		//refList1PosX = (EB_U32) CLIP3((EB_S32)((refPicList1->originX-71) << 2), (EB_S32)((refPicList1->width + refPicList1->originX  + 7)<<2) , (EB_S32) ((puOriginX + tbOriginX + refPicList1->originX)<<2) + candidateBufferPtr->candidatePtr->motionVector_x[REF_LIST_1][puIndex]);
		refList1PosX = (EB_U32)CLIP3(
			(EB_S32)((refPicList1->originX - 71) << 2),
			(EB_S32)((refPicList1->width + refPicList1->originX + 7) << 2),
			(EB_S32)((puOriginX + refPicList1->originX) << 2) + mvUnit->mv[REF_LIST_1].x);
		//refList1PosY = (EB_U32) CLIP3((EB_S32)((refPicList1->originY-71) << 2), (EB_S32)((refPicList1->height + refPicList1->originY + 7)<<2) , (EB_S32) ((puOriginY + tbOriginY + refPicList1->originY)<<2) + candidateBufferPtr->candidatePtr->motionVector_y[REF_LIST_1][puIndex]);
		refList1PosY = (EB_U32)CLIP3(
			(EB_S32)((refPicList1->originY - 71) << 2),
			(EB_S32)((refPicList1->height + refPicList1->originY + 7) << 2),
			(EB_S32)((puOriginY + refPicList1->originY) << 2) + mvUnit->mv[REF_LIST_1].y);

		CHECK_REPORT_ERROR(
			(refList1PosX < ((refPicList1->width + (refPicList1->originX << 1)) << 2)),
			encodeContextPtr->appCallbackPtr,
			EB_ENC_INTER_INVLD_MCP_ERROR);

		CHECK_REPORT_ERROR(
			(refList1PosY  < ((refPicList1->height + (refPicList1->originY << 1)) << 2)),
			encodeContextPtr->appCallbackPtr,
			EB_ENC_INTER_INVLD_MCP_ERROR);

		//mcpContext->localReferenceBlockL1->bufferY  = refPicList1->bufferY  + ((refList1PosX >> 2) - 4) * 2 + ((refList1PosY >> 2) - 4) * 2 * refPicList1->strideY;
		//mcpContext->localReferenceBlockL1->bufferCb = refPicList1->bufferCb + ((refList1PosX >> 3) - 2) * 2 + ((refList1PosY >> 3) - 2) * 2 * refPicList1->strideCb;
		//mcpContext->localReferenceBlockL1->bufferCr = refPicList1->bufferCr + ((refList1PosX >> 3) - 2) * 2 + ((refList1PosY >> 3) - 2) * 2 * refPicList1->strideCr;
		EB_U32  lumaOffSet = ((refList1PosX >> 2) - 4) * 2 + ((refList1PosY >> 2) - 4) * 2 * refPicList1->strideY; //refPicList0->originX + refPicList0->originY*refPicList0->strideY; //
		EB_U32  cbOffset = ((refList1PosX >> (2 + subWidthCMinus1)) - 2) * 2 + ((refList1PosY >> (2 + subHeightCMinus1)) - 2) * 2 * refPicList1->strideCb;
		EB_U32  crOffset = ((refList1PosX >> (2 + subWidthCMinus1)) - 2) * 2 + ((refList1PosY >> (2 + subHeightCMinus1)) - 2) * 2 * refPicList1->strideCr;
		//EB_U8  verticalIdx;

		mcpContext->localReferenceBlockL1->bufferY = refPicList1->bufferY + lumaOffSet;
		mcpContext->localReferenceBlockL1->bufferCb = refPicList1->bufferCb + cbOffset;
		mcpContext->localReferenceBlockL1->bufferCr = refPicList1->bufferCr + crOffset;

		mcpContext->localReferenceBlockL1->strideY = refPicList1->strideY;
		mcpContext->localReferenceBlockL1->strideCb = refPicList1->strideCb;
		mcpContext->localReferenceBlockL1->strideCr = refPicList1->strideCr;
	}

    switch(mvUnit->predDirection)
    {

    case UNI_PRED_LIST_0:
        //CHKN load the needed FP block from reference frame in a local buffer, we load 4 pixel more in each direction to be used for interpolation.
        //TODO: the max area needed for interpolation is 4, we could optimize this later on a case by case basis
        //size of the buffer would be (64+4+4)x(64+4+4)

        UniPredInterpolation16bit(
            mcpContext->localReferenceBlockL0,
            refPicList0,
            refList0PosX,
            refList0PosY,
            puWidth,
            puHeight,
            predictionPtr,
            puOriginIndex,
            puChromaOriginIndex,
            mcpContext->motionCompensationIntermediateResultBuf0);

        break;

    case UNI_PRED_LIST_1:

        UniPredInterpolation16bit(
            mcpContext->localReferenceBlockL1,
            refPicList1,
            refList1PosX,
            refList1PosY,
            puWidth,
            puHeight,
            predictionPtr,
            puOriginIndex,
            puChromaOriginIndex,
            mcpContext->motionCompensationIntermediateResultBuf0);


        break;

    case BI_PRED:

        BiPredInterpolation16bit(
            mcpContext->localReferenceBlockL0,
            mcpContext->localReferenceBlockL1,
            refList0PosX,
            refList0PosY,
            refList1PosX,
            refList1PosY,
            puWidth,
            puHeight,
            predictionPtr,
            puOriginIndex,
            puChromaOriginIndex,
            mcpContext->motionCompensationIntermediateResultBuf0,
            mcpContext->motionCompensationIntermediateResultBuf1,
            mcpContext->TwoDInterpolationFirstPassFilterResultBuf);

        break;

    default:
	    CHECK_REPORT_ERROR_NC(
		    encodeContextPtr->appCallbackPtr, 
		    EB_ENC_INTER_PRED_ERROR0);
        break;
    }

    return return_error;
}

/** ChooseMVPIdx_V2 function is used to choose the best AMVP candidate.
    @param *candidatePtr(output)
        candidatePtr points to the prediction result.
    @param cuPtr(input)
        pointer to the CU where the target PU belongs to.
    @param *puIndex(input)
        the index of the PU inside a CU
    @param ref0AMVPCandArray(input)
    @param ref0NumAvailableAMVPCand(input)
    @param ref1AMVPCandArray(input)
    @param ref1NumAvailableAMVPCand(input)
 */
EB_ERRORTYPE ChooseMVPIdx_V2(
    ModeDecisionCandidate_t  *candidatePtr,
    EB_U32                    cuOriginX,
    EB_U32                    cuOriginY,
    EB_U32                    puIndex,
    EB_U32                    tbSize,
    EB_S16                   *ref0AMVPCandArray_x,
    EB_S16                   *ref0AMVPCandArray_y,
    EB_U32                    ref0NumAvailableAMVPCand,
    EB_S16                   *ref1AMVPCandArray_x,
    EB_S16                   *ref1AMVPCandArray_y,
    EB_U32                    ref1NumAvailableAMVPCand,
    PictureControlSet_t      *pictureControlSetPtr)
{
    EB_ERRORTYPE  return_error = EB_ErrorNone;
    EB_U8         mvpRef0Idx;
    EB_U8         mvpRef1Idx;


    EB_U32        pictureWidth = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaWidth;
    EB_U32        pictureHeight = ((SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr)->lumaHeight;

    EB_U32   mvd0, mvd1;

    switch (candidatePtr->predictionDirection[puIndex]) {
    case UNI_PRED_LIST_0:
        // Clip the input MV
        ClipMV(
            cuOriginX,
            cuOriginY,
            &candidatePtr->motionVector_x_L0,
            &candidatePtr->motionVector_y_L0,
            pictureWidth,
            pictureHeight,
            tbSize);

        // Choose the AMVP candidate
        switch (ref0NumAvailableAMVPCand) {
        case 0:
        case 1:
            //mvpRef0Idx = 0;
            candidatePtr->motionVectorPredIdx[REF_LIST_0] = 0;
            candidatePtr->motionVectorPred_x[REF_LIST_0] = ref0AMVPCandArray_x[0];
            candidatePtr->motionVectorPred_y[REF_LIST_0] = ref0AMVPCandArray_y[0];
            break;
        case 2:

            mvd0 = EB_ABS_DIFF(ref0AMVPCandArray_x[0], candidatePtr->motionVector_x_L0) +
                EB_ABS_DIFF(ref0AMVPCandArray_y[0], candidatePtr->motionVector_y_L0);

            mvd1 = EB_ABS_DIFF(ref0AMVPCandArray_x[1], candidatePtr->motionVector_x_L0) +
                EB_ABS_DIFF(ref0AMVPCandArray_y[1], candidatePtr->motionVector_y_L0);

            mvpRef0Idx = ((mvd0) <= (mvd1)) ? 0 : 1;

            candidatePtr->motionVectorPredIdx[REF_LIST_0] = mvpRef0Idx;
            candidatePtr->motionVectorPred_x[REF_LIST_0] = ref0AMVPCandArray_x[mvpRef0Idx];
            candidatePtr->motionVectorPred_y[REF_LIST_0] = ref0AMVPCandArray_y[mvpRef0Idx];
            break;
        default:
            break;
        }

        break;

    case UNI_PRED_LIST_1:

        // Clip the input MV
        ClipMV(
            cuOriginX,
            cuOriginY,
            &candidatePtr->motionVector_x_L1,
            &candidatePtr->motionVector_y_L1,
            pictureWidth,
            pictureHeight,
            tbSize);

        // Choose the AMVP candidate
        switch (ref1NumAvailableAMVPCand) {
        case 0:
        case 1:
            //mvpRef1Idx = 0;
            candidatePtr->motionVectorPredIdx[REF_LIST_1] = 0;
            candidatePtr->motionVectorPred_x[REF_LIST_1] = ref1AMVPCandArray_x[0];
            candidatePtr->motionVectorPred_y[REF_LIST_1] = ref1AMVPCandArray_y[0];
            break;
        case 2:

            mvd0 = EB_ABS_DIFF(ref1AMVPCandArray_x[0], candidatePtr->motionVector_x_L1) +
                EB_ABS_DIFF(ref1AMVPCandArray_y[0], candidatePtr->motionVector_y_L1);

            mvd1 = EB_ABS_DIFF(ref1AMVPCandArray_x[1], candidatePtr->motionVector_x_L1) +
                EB_ABS_DIFF(ref1AMVPCandArray_y[1], candidatePtr->motionVector_y_L1);

            mvpRef1Idx = ((mvd0) <= (mvd1)) ? 0 : 1;



            candidatePtr->motionVectorPredIdx[REF_LIST_1] = mvpRef1Idx;
            candidatePtr->motionVectorPred_x[REF_LIST_1] = ref1AMVPCandArray_x[mvpRef1Idx];
            candidatePtr->motionVectorPred_y[REF_LIST_1] = ref1AMVPCandArray_y[mvpRef1Idx];
            break;
        default:
            break;
        }

        // MVP in refPicList0
        //mvpRef0Idx = 0;
        //candidatePtr->motionVectorPredIdx[REF_LIST_0][puIndex] = mvpRef0Idx;
        //candidatePtr->motionVectorPred_x[REF_LIST_0][puIndex]  = 0;
        //candidatePtr->motionVectorPred_y[REF_LIST_0][puIndex]  = 0;

        break;

    case BI_PRED:

        // Choose the MVP in list0
        // Clip the input MV
        ClipMV(
            cuOriginX,
            cuOriginY,
            &candidatePtr->motionVector_x_L0,
            &candidatePtr->motionVector_y_L0,
            pictureWidth,
            pictureHeight,
            tbSize);

        // Choose the AMVP candidate
        switch (ref0NumAvailableAMVPCand) {
        case 0:
        case 1:
            //mvpRef0Idx = 0;
            candidatePtr->motionVectorPredIdx[REF_LIST_0] = 0;
            candidatePtr->motionVectorPred_x[REF_LIST_0] = ref0AMVPCandArray_x[0];
            candidatePtr->motionVectorPred_y[REF_LIST_0] = ref0AMVPCandArray_y[0];
            break;
        case 2:

            mvd0 = EB_ABS_DIFF(ref0AMVPCandArray_x[0], candidatePtr->motionVector_x_L0) +
                EB_ABS_DIFF(ref0AMVPCandArray_y[0], candidatePtr->motionVector_y_L0);

            mvd1 = EB_ABS_DIFF(ref0AMVPCandArray_x[1], candidatePtr->motionVector_x_L0) +
                EB_ABS_DIFF(ref0AMVPCandArray_y[1], candidatePtr->motionVector_y_L0);

            mvpRef0Idx = ((mvd0) <= (mvd1)) ? 0 : 1;



            candidatePtr->motionVectorPredIdx[REF_LIST_0] = mvpRef0Idx;
            candidatePtr->motionVectorPred_x[REF_LIST_0] = ref0AMVPCandArray_x[mvpRef0Idx];
            candidatePtr->motionVectorPred_y[REF_LIST_0] = ref0AMVPCandArray_y[mvpRef0Idx];
            break;
        default:
            break;
        }

        // Choose the MVP in list1
        // Clip the input MV
        ClipMV(
            cuOriginX,
            cuOriginY,
            &candidatePtr->motionVector_x_L1,
            &candidatePtr->motionVector_y_L1,
            pictureWidth,
            pictureHeight,
            tbSize);

        // Choose the AMVP candidate
        switch (ref1NumAvailableAMVPCand) {
        case 0:
        case 1:
            //mvpRef1Idx = 0;
            candidatePtr->motionVectorPredIdx[REF_LIST_1] = 0;
            candidatePtr->motionVectorPred_x[REF_LIST_1] = ref1AMVPCandArray_x[0];
            candidatePtr->motionVectorPred_y[REF_LIST_1] = ref1AMVPCandArray_y[0];
            break;
        case 2:

            mvd0 = EB_ABS_DIFF(ref1AMVPCandArray_x[0], candidatePtr->motionVector_x_L1) +
                EB_ABS_DIFF(ref1AMVPCandArray_y[0], candidatePtr->motionVector_y_L1);

            mvd1 = EB_ABS_DIFF(ref1AMVPCandArray_x[1], candidatePtr->motionVector_x_L1) +
                EB_ABS_DIFF(ref1AMVPCandArray_y[1], candidatePtr->motionVector_y_L1);

            mvpRef1Idx = ((mvd0) <= (mvd1)) ? 0 : 1;

            candidatePtr->motionVectorPredIdx[REF_LIST_1] = mvpRef1Idx;
            candidatePtr->motionVectorPred_x[REF_LIST_1] = ref1AMVPCandArray_x[mvpRef1Idx];
            candidatePtr->motionVectorPred_y[REF_LIST_1] = ref1AMVPCandArray_y[mvpRef1Idx];
            break;
        default:
            break;
        }

        break;

    default:
        break;
    }

    return return_error;
}
