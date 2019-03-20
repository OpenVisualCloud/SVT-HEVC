/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <string.h>

#include "EbDefinitions.h"
#include "EbUtility.h"
#include "EbPictureControlSet.h"
#include "EbCodingUnit.h"
#include "EbSequenceControlSet.h"
#include "EbReferenceObject.h"

#include "EbDeblockingFilter.h"
#include "EbTransforms.h"
#include "EbErrorCodes.h"
#include "EbErrorHandling.h"

#if 0
#define   convertToChromaQp(iQpY)  ( ((iQpY) < 0) ? (iQpY) : (((iQpY) > 57) ? ((iQpY)-6) : (EB_S32)(MapChromaQp((EB_U32)iQpY))) )
#else
#define   convertToChromaQp(iQpY, is422or444)  \
    (!(is422or444) ? ( ((iQpY) < 0) ? (iQpY) : (((iQpY) > 57) ? ((iQpY)-6) : (EB_S32)(MapChromaQp((EB_U32)iQpY))) ) \
                   : ( ((iQpY) < 0) ? (iQpY) : (((iQpY) > 51) ? 51 : iQpY)) )
#endif



const EB_U8 TcTable_8x8[54] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10, 11, 13, 14, 16, 18, 20, 22, 24
};

const EB_S32 BetaTable_8x8[52] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64
};


void EncodePassUpdateSaoNeighborArrays(
	NeighborArrayUnit_t     *saoParamNeighborArray,
	SaoParameters_t         *saoParams,
	EB_U32                   originX,
	EB_U32                   originY,
	EB_U32                   size);
/** setQpArrayBasedOnCU()
is used to set qp in the qpArray on a CU basis.
*/
void SetQpArrayBasedOnCU(
	PictureControlSet_t *pictureControlSetPtr,          //input parameter
	EB_U32               cuPos_x,                       //input parameter, sample-based horizontal picture-wise locatin of the CU
	EB_U32               cuPos_y,                       //input parameter, sample-based vertical picture-wise locatin of the CU
	EB_U32               cuSizeInMinCuSize,             //input parameter
	EB_U32               cuQp)                          //input parameter, Qp of the CU
{
	EB_U32 verticalIdx;
	EB_U32 qpArrayIdx = (cuPos_y / MIN_CU_SIZE) * pictureControlSetPtr->qpArrayStride + (cuPos_x / MIN_CU_SIZE);

	for (verticalIdx = 0; verticalIdx < cuSizeInMinCuSize; ++verticalIdx) {
		EB_MEMSET(pictureControlSetPtr->qpArray + qpArrayIdx + verticalIdx * pictureControlSetPtr->qpArrayStride,
			cuQp, sizeof(EB_U8)*cuSizeInMinCuSize);
	}

	return;
}


/** setQpArrayBasedOnCU()
is used to set qp in the qpArray on a CU basis.
*/
void entropySetQpArrayBasedOnCU(
	PictureControlSet_t *pictureControlSetPtr,          //input parameter
	EB_U32               cuPos_x,                       //input parameter, sample-based horizontal picture-wise locatin of the CU
	EB_U32               cuPos_y,                       //input parameter, sample-based vertical picture-wise locatin of the CU
	EB_U32               cuWidth,                       //input parameter
	EB_U32               cuHeight,                      //input parameter
	EB_U32               cuQp)                          //input parameter, Qp of the CU
{
	EB_U32 verticalIdx;
	EB_U32 cuPos_xInMinCuSize = cuPos_x / MIN_CU_SIZE;
	EB_U32 cuPos_yInMinCuSize = cuPos_y / MIN_CU_SIZE;
	EB_U32 cuWidthInMinCuSize = cuWidth / MIN_CU_SIZE;
	EB_U32 cuHeightInMinCuSize = cuHeight / MIN_CU_SIZE;
	EB_U32 qpArrayIdx = cuPos_yInMinCuSize * pictureControlSetPtr->qpArrayStride + cuPos_xInMinCuSize;

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

	CHECK_REPORT_ERROR(
		(cuPos_x % MIN_CU_SIZE == 0 && cuPos_y % MIN_CU_SIZE == 0),
		encodeContextPtr->appCallbackPtr,
		EB_ENC_DLF_ERROR4);

	CHECK_REPORT_ERROR(
		(cuWidth % MIN_CU_SIZE == 0 && cuHeight % MIN_CU_SIZE == 0),
		encodeContextPtr->appCallbackPtr,
		EB_ENC_DLF_ERROR4);
	for (verticalIdx = 0; verticalIdx < cuHeightInMinCuSize; ++verticalIdx) {
		EB_MEMSET(pictureControlSetPtr->entropyQpArray + qpArrayIdx + verticalIdx * pictureControlSetPtr->qpArrayStride,
			cuQp, sizeof(EB_U8)*cuWidthInMinCuSize);
	}

	return;
}
/** CalculateBSForPUBoundary()
is used to calulate the bS for a particular(horizontal/vertical) edge of a 4x4 block on the
PU boundary.
*/
EB_U8 CalculateBSForPUBoundary(
	EB_U32                 blk4x4Pos_x,                           //input parameter, picture-wise horizontal location of the 4x4 block.
	EB_U32                 blk4x4Pos_y,                           //input parameter, picture-wise vertical location of the 4x4 block.
	PredictionUnit_t      *puPtr,                                 //input parameter, the pointer to the PU where the 4x4 block belongs to.
	PredictionUnit_t      *neighbourPuPtr,                        //input parameter, neighbourPuPtr is the pointer to the left/top neighbour PU of the 4x4 block.
	EB_MODETYPE            puCodingMode,
	EB_MODETYPE            neighbourPuCodingMode,
	EB_BOOL                isVerticalEdge,                        //input parameter
	PictureControlSet_t   *pictureControlSetPtr,
	SequenceControlSet_t  *sequenceControlSetPtr)                 //input parameter
{
	EB_U8 bS = 0;
	EB_U32  blk4x4PosNeighbourX;
	EB_U32  blk4x4PosNeighbourY;

	EB_BOOL              bSDeterminationCondition1;
	EB_BOOL              bSDeterminationCondition2;
	EB_BOOL              bSDeterminationCondition3;
	EB_PICTURE             sliceType;
	EB_U64               puRefList0POC;
	EB_U64               puRefList1POC;
	EB_U64               neighborPuRefList0POC;
	EB_U64               neighborPuRefList1POC;

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

	sliceType = pictureControlSetPtr->sliceType;

	blk4x4PosNeighbourX = isVerticalEdge ? blk4x4Pos_x - 4 : blk4x4Pos_x;
	blk4x4PosNeighbourY = isVerticalEdge ? blk4x4Pos_y : blk4x4Pos_y - 4;

	if (puCodingMode == INTRA_MODE || neighbourPuCodingMode == INTRA_MODE) {
		bS = 2;
	}
	else {
		switch (sliceType) {
		case EB_P_PICTURE:
			puRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
			neighborPuRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;

			// different number of motion vectors or difference reference pictures
			bSDeterminationCondition1 = (EB_BOOL)(puRefList0POC != neighborPuRefList0POC);

			// if the difference of the horizontal or vertical components of MV >= 4 quater luma pixel, bS = 1
			bSDeterminationCondition2 = (EB_BOOL)(CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
				puPtr,
				neighbourPuPtr,
				REF_LIST_0,
				REF_LIST_0));

			// in non-zero luma TU, bS = 1
			bSDeterminationCondition3 = (EB_BOOL)((
				pictureControlSetPtr->cbfMapArray[(blk4x4Pos_x >> 2) + (blk4x4Pos_y >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)] ||
				pictureControlSetPtr->cbfMapArray[(blk4x4PosNeighbourX >> 2) + (blk4x4PosNeighbourY >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)]));

			bS = (EB_U8)(bSDeterminationCondition1 | bSDeterminationCondition2 | bSDeterminationCondition3);

			break;

		case EB_B_PICTURE:
			switch (puPtr->interPredDirectionIndex + ((neighbourPuPtr->interPredDirectionIndex) * 3)) {
			case 0:         // UNI_PRED_LIST_0 + UNI_PRED_LIST_0
				puRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
				neighborPuRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;

				bSDeterminationCondition1 = (EB_BOOL)(
					(puRefList0POC != neighborPuRefList0POC) ||
					CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
					puPtr,
					neighbourPuPtr,
					REF_LIST_0,
					REF_LIST_0));

				break;
			case 1:         // UNI_PRED_LIST_1 + UNI_PRED_LIST_0
				puRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;
				neighborPuRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;

				bSDeterminationCondition1 = (EB_BOOL)(
					(puRefList1POC != neighborPuRefList0POC) ||
					CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
					puPtr,
					neighbourPuPtr,
					REF_LIST_1,
					REF_LIST_0));

				break;
			case 2:         // BI_PRED + UNI_PRED_LIST_0
				bSDeterminationCondition1 = EB_TRUE;

				break;
			case 3:         // UNI_PRED_LIST_0 + UNI_PRED_LIST_1
				puRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
				neighborPuRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;

				bSDeterminationCondition1 = (EB_BOOL)(
					(puRefList0POC != neighborPuRefList1POC) ||
					CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
					puPtr,
					neighbourPuPtr,
					REF_LIST_0,
					REF_LIST_1));

				break;
			case 4:         // UNI_PRED_LIST_1 + UNI_PRED_LIST_1
				puRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;
				neighborPuRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;

				bSDeterminationCondition1 = (EB_BOOL)(
					(puRefList1POC != neighborPuRefList1POC) ||
					CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
					puPtr,
					neighbourPuPtr,
					REF_LIST_1,
					REF_LIST_1));

				break;
			case 5:         // BI_PRED + UNI_PRED_LIST_1
				bSDeterminationCondition1 = EB_TRUE;

				break;
			case 6:         // UNI_PRED_LIST_0 + BI_PRED
				bSDeterminationCondition1 = EB_TRUE;

				break;
			case 7:         // UNI_PRED_LIST_1 + BI_PRED
				bSDeterminationCondition1 = EB_TRUE;

				break;
			case 8:         // BI_PRED + BI_PRED
				puRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
				neighborPuRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
				puRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;
				neighborPuRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;

				if ((puRefList0POC == neighborPuRefList0POC) && (puRefList1POC == neighborPuRefList1POC) &&
					(puRefList0POC == neighborPuRefList1POC) && (puRefList1POC == neighborPuRefList0POC)) {

					bSDeterminationCondition1 = (EB_BOOL)(
						(CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
						puPtr,
						neighbourPuPtr,
						REF_LIST_0,
						REF_LIST_0) ||
						CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
						puPtr,
						neighbourPuPtr,
						REF_LIST_1,
						REF_LIST_1)) &&
						(CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
						puPtr,
						neighbourPuPtr,
						REF_LIST_0,
						REF_LIST_1) ||
						CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
						puPtr,
						neighbourPuPtr,
						REF_LIST_1,
						REF_LIST_0)));
				}
				else {
					if ((puRefList0POC == neighborPuRefList0POC) && (puRefList1POC == neighborPuRefList1POC)) {
						bSDeterminationCondition1 = (EB_BOOL)(
							CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
							puPtr,
							neighbourPuPtr,
							REF_LIST_0,
							REF_LIST_0) ||
							CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
							puPtr,
							neighbourPuPtr,
							REF_LIST_1,
							REF_LIST_1));
					}
					else {
						if ((puRefList0POC == neighborPuRefList1POC) && (puRefList1POC == neighborPuRefList0POC)) {
							bSDeterminationCondition1 = (EB_BOOL)(
								CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
								puPtr,
								neighbourPuPtr,
								REF_LIST_0,
								REF_LIST_1) ||
								CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
								puPtr,
								neighbourPuPtr,
								REF_LIST_1,
								REF_LIST_0));
						}
						else {
							bSDeterminationCondition1 = EB_TRUE;
						}
					}
				}

				break;
			default:
				CHECK_REPORT_ERROR_NC(
					encodeContextPtr->appCallbackPtr,
					EB_ENC_DLF_ERROR2);
				bSDeterminationCondition1 = EB_FALSE;    // junk code to avoid compiling warning!
				break;
			}

			// in non-zero luma TU, bS = 1
			bSDeterminationCondition2 = (EB_BOOL)((
				pictureControlSetPtr->cbfMapArray[(blk4x4Pos_x >> 2) + (blk4x4Pos_y >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)] ||
				pictureControlSetPtr->cbfMapArray[(blk4x4PosNeighbourX >> 2) + (blk4x4PosNeighbourY >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)]));

			bS = (EB_U8)(bSDeterminationCondition1 | bSDeterminationCondition2);

			break;

		case EB_I_PICTURE:
			CHECK_REPORT_ERROR_NC(
				encodeContextPtr->appCallbackPtr,
				EB_ENC_DLF_ERROR3);
			break;

		default:
			CHECK_REPORT_ERROR_NC(
				encodeContextPtr->appCallbackPtr,
				EB_ENC_DLF_ERROR5);
			break;
		}
	}

	return bS;
}


void SetBSArrayBasedOnPUBoundary(

	NeighborArrayUnit_t         *modeTypeNeighborArray,
	NeighborArrayUnit_t         *mvNeighborArray,
	PredictionUnit_t            *puPtr,
	CodingUnit_t                *cuPtr,
	const CodedUnitStats_t      *cuStatsPtr,
	EB_U32                       tbOriginX,
	EB_U32                       tbOriginY,
	PictureControlSet_t         *pictureControlSetPtr,
	EB_U8                       *horizontalEdgeBSArray,
	EB_U8                       *verticalEdgeBSArray)
{
	const EB_U32 logMinPuSize = Log2f(MIN_PU_SIZE);
	const EB_U32 MaxLcuSizeIn4x4blk = MAX_LCU_SIZE >> 2;
	const EB_U32 logMaxLcuSizeIn4x4blk = Log2f(MaxLcuSizeIn4x4blk);
	SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;

	// MV Uint
	MvUnit_t   *mvUnitPtr;

	// 4x4 block
	EB_U32  puTopLeft4x4blkAddr = (cuStatsPtr->originX >> 2) + ((cuStatsPtr->originY >> 2) << logMaxLcuSizeIn4x4blk);
	EB_U32  puTopRight4x4blkAddr = puTopLeft4x4blkAddr + (cuStatsPtr->size >> 2) - 1;
	EB_U32  puBottomLeft4x4blkAddr = puTopLeft4x4blkAddr + (((cuStatsPtr->size >> 2) - 1) << logMaxLcuSizeIn4x4blk);
	EB_U32  blk4x4Addr;
	EB_U32  blk4x4Pos_x;    // sample-based LCU-wise horizontal location of the 4x4 block
	EB_U32  blk4x4Pos_y;    // sample-based LCU-wise vertical location of the 4x4 block
	// PU
	//PredictionUnit_t *neighbourPuPtrOLD;
	PredictionUnit_t  neighbourPu;
	EB_MODETYPE          neighbourPuMode;
	EB_U32               neighborPuIdx;

	// set bS for the horizontal PU boundary which lies on the 8 sample edge
	if ((cuStatsPtr->originY & 7) == 0 && cuStatsPtr->originY + tbOriginY != 0) {

		for (blk4x4Addr = puTopLeft4x4blkAddr; blk4x4Addr <= puTopRight4x4blkAddr; ++blk4x4Addr) {

			blk4x4Pos_x = (blk4x4Addr & (MaxLcuSizeIn4x4blk - 1)) << 2;
			blk4x4Pos_y = (blk4x4Addr >> logMaxLcuSizeIn4x4blk) << 2;
			neighborPuIdx = (tbOriginX + blk4x4Pos_x) >> logMinPuSize;

			neighbourPuMode     = *(modeTypeNeighborArray->topArray + neighborPuIdx);
			mvUnitPtr           = (MvUnit_t*)(mvNeighborArray->topArray) + neighborPuIdx;
			neighbourPu.interPredDirectionIndex = mvUnitPtr->predDirection;
			neighbourPu.mv[REF_LIST_0].mvUnion = mvUnitPtr->mv[REF_LIST_0].mvUnion;
			neighbourPu.mv[REF_LIST_1].mvUnion = mvUnitPtr->mv[REF_LIST_1].mvUnion;

			horizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)]
				= CalculateBSForPUBoundary(
				tbOriginX + blk4x4Pos_x,       // to be modified
				tbOriginY + blk4x4Pos_y,       // to be modified
				puPtr,
				&neighbourPu,
				(EB_MODETYPE)cuPtr->predictionModeFlag,
				neighbourPuMode,
				EB_FALSE,
				pictureControlSetPtr,
				sequenceControlSetPtr);
		}
	}

	// set bS for the vertical PU boundary which lies on the 8 sample edge
	if ((cuStatsPtr->originX & 7) == 0 && cuStatsPtr->originX + tbOriginX != 0) {
		for (blk4x4Addr = puTopLeft4x4blkAddr; blk4x4Addr <= puBottomLeft4x4blkAddr; blk4x4Addr += MaxLcuSizeIn4x4blk) {
			blk4x4Pos_x = (blk4x4Addr & (MaxLcuSizeIn4x4blk - 1)) << 2;
			blk4x4Pos_y = (blk4x4Addr >> logMaxLcuSizeIn4x4blk) << 2;
			neighborPuIdx = (tbOriginY + blk4x4Pos_y) >> logMinPuSize;


			neighbourPuMode = *(modeTypeNeighborArray->leftArray + neighborPuIdx);
			mvUnitPtr = (MvUnit_t*)(mvNeighborArray->leftArray) + neighborPuIdx /** neighborArrays->mv->unitSize*/;
			neighbourPu.interPredDirectionIndex = mvUnitPtr->predDirection;
			neighbourPu.mv[REF_LIST_0].mvUnion = mvUnitPtr->mv[REF_LIST_0].mvUnion;
			neighbourPu.mv[REF_LIST_1].mvUnion = mvUnitPtr->mv[REF_LIST_1].mvUnion;

			verticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)]
				= CalculateBSForPUBoundary(
				tbOriginX + blk4x4Pos_x,       // to be modified
				tbOriginY + blk4x4Pos_y,       // to be modified
				puPtr,
				&neighbourPu,
				(EB_MODETYPE)cuPtr->predictionModeFlag,
				neighbourPuMode,
				EB_TRUE,
				pictureControlSetPtr,
				sequenceControlSetPtr);
		}
	}

	return;
}

/** CalculateBSForTUBoundaryInsidePU()
is used to calulate the bS for a particular(horizontal/vertical) edge of a 4x4 block on a
TU boundary which lays inside a PU.
*/
static inline EB_U8 CalculateBSForTUBoundaryInsidePU(
	EB_U32                 blk4x4Pos_x,             //input paramter, picture-wise horizontal location of the 4x4 block.
	EB_U32                 blk4x4Pos_y,             //input paramter, picture-wise vertical location of the 4x4 block.
	EB_MODETYPE            cuCodingMode,            //input paramter, the coding mode of the CU where the 4x4 block belongs to.
	EB_BOOL                isVerticalEdge,          //input paramter
	PictureControlSet_t   *pictureControlSetPtr,
	SequenceControlSet_t  *sequenceControlSetPtr)
{
	EB_U8   bS = 0;
	EB_U32  blk4x4PosNeighbourX;
	EB_U32  blk4x4PosNeighbourY;

	blk4x4PosNeighbourX = isVerticalEdge ? blk4x4Pos_x - 4 : blk4x4Pos_x;
	blk4x4PosNeighbourY = isVerticalEdge ? blk4x4Pos_y : blk4x4Pos_y - 4;

	if (cuCodingMode == INTRA_MODE) {
		bS = 2;
	}
	else {
		bS = pictureControlSetPtr->cbfMapArray[(blk4x4Pos_x >> 2) + (blk4x4Pos_y >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)] ||
			pictureControlSetPtr->cbfMapArray[(blk4x4PosNeighbourX >> 2) + (blk4x4PosNeighbourY >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)];
	}

	return bS;
}

/** SetBSArrayBasedOnTUBoundary()
is used to calulate the bS on TU boundary.

Note - This implementation is based on the assumption that the TU can never go across the PU.
*/
void SetBSArrayBasedOnTUBoundary(
	EB_U32                 tuPos_x,                                  //input parameter, the picture-wise horizontal location of a TU.
	EB_U32                 tuPos_y,                                  //input parameter, the picture-wise vertical location of a TU.
	EB_U32                 tuWidth,                                  //input parameter
	EB_U32                 tuHeight,                                 //input parameter
	//CodingUnit_t          *cuPtr,                                    //input parameter, pointer to the CU where the TU belongs to.
	const CodedUnitStats_t      *cuStatsPtr,
	EB_MODETYPE            cuCodingMode,                             //input paramter, the coding mode of the CU where the TU belongs to.
	EB_U32                 tbOriginX,
	EB_U32                 tbOriginY,
	PictureControlSet_t   *pictureControlSetPtr,
	EB_U8                 *horizontalEdgeBSArray,
	EB_U8                 *verticalEdgeBSArray)
{
	const EB_U32 MaxLcuSizeIn4x4blk = MAX_LCU_SIZE >> 2;
	const EB_U32 logMaxLcuSizeIn4x4blk = Log2f(MaxLcuSizeIn4x4blk);
	SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	EB_U32                lcuSize = sequenceControlSetPtr->lcuSize;

	EB_U32  tuTopLeft4x4blkAddr = ((tuPos_x&(lcuSize - 1)) >> 2) + (((tuPos_y&(lcuSize - 1)) >> 2) << logMaxLcuSizeIn4x4blk);
	EB_U32  tuTopRight4x4blkAddr = tuTopLeft4x4blkAddr + (tuWidth >> 2) - 1;
	EB_U32  tuBottomLeft4x4blkAddr = tuTopLeft4x4blkAddr + (((tuHeight >> 2) - 1) << logMaxLcuSizeIn4x4blk);
	EB_U32  blk4x4Addr;

	//set bS for the horizontal TU boundary which lies on the 8 sample edge inside a PU
    if ((tuPos_y & 7) == 0 && tuPos_y != 0 && tuPos_y != tbOriginY + cuStatsPtr->originY) {
		for (blk4x4Addr = tuTopLeft4x4blkAddr; blk4x4Addr <= tuTopRight4x4blkAddr; ++blk4x4Addr) {
			horizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)]
				= CalculateBSForTUBoundaryInsidePU(
				tbOriginX + ((blk4x4Addr & (MaxLcuSizeIn4x4blk - 1)) << 2),
				tbOriginY + ((blk4x4Addr >> logMaxLcuSizeIn4x4blk) << 2),
				cuCodingMode,
				EB_FALSE,
				pictureControlSetPtr,
				sequenceControlSetPtr);
		}
	}

	//set bS for the vertical TU boundary which lies on the 8 sample edge inside a PU
    if ((tuPos_x & 7) == 0 && tuPos_x != 0 && tuPos_x != tbOriginX + cuStatsPtr->originX) {
		for (blk4x4Addr = tuTopLeft4x4blkAddr; blk4x4Addr <= tuBottomLeft4x4blkAddr; blk4x4Addr += MaxLcuSizeIn4x4blk) {
			verticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)]
				= CalculateBSForTUBoundaryInsidePU(
				tbOriginX + ((blk4x4Addr & (MaxLcuSizeIn4x4blk - 1)) << 2),
				tbOriginY + ((blk4x4Addr >> logMaxLcuSizeIn4x4blk) << 2),
				cuCodingMode,
				EB_TRUE,
				pictureControlSetPtr,
				sequenceControlSetPtr);
		}
	}

	return;
}

EB_U8 Intra4x4CalculateBSForPUBoundary(
	EB_U32                 blk4x4Pos_x,                           //input parameter, picture-wise horizontal location of the 4x4 block.
	EB_U32                 blk4x4Pos_y,                           //input parameter, picture-wise vertical location of the 4x4 block.
	PredictionUnit_t      *puPtr,                                 //input parameter, the pointer to the PU where the 4x4 block belongs to.
	PredictionUnit_t      *neighbourPuPtr,                        //input parameter, neighbourPuPtr is the pointer to the left/top neighbour PU of the 4x4 block.
	EB_MODETYPE            puCodingMode,
	EB_MODETYPE            neighbourPuCodingMode,
	EB_BOOL                isVerticalEdge,                        //input parameter
	EB_BOOL                isVerticalPuBoundaryAlsoTuBoundary,    //input parameter
	EB_BOOL                isHorizontalPuBoundaryAlsoTuBoundary,
	PictureControlSet_t   *pictureControlSetPtr,
	SequenceControlSet_t  *sequenceControlSetPtr)                 //input parameter
{
	EB_U8 bS = 0;
	EB_U32  blk4x4PosNeighbourX;
	EB_U32  blk4x4PosNeighbourY;

	EB_BOOL              bSDeterminationCondition1;
	EB_BOOL              bSDeterminationCondition2;
	EB_BOOL              bSDeterminationCondition3;
	EB_BOOL              isTuBoundary = isVerticalEdge ? isVerticalPuBoundaryAlsoTuBoundary : isHorizontalPuBoundaryAlsoTuBoundary;
	EB_PICTURE             sliceType;
	EB_U64               puRefList0POC;
	EB_U64               puRefList1POC;
	EB_U64               neighborPuRefList0POC;
	EB_U64               neighborPuRefList1POC;

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

	sliceType = pictureControlSetPtr->sliceType;

	blk4x4PosNeighbourX = isVerticalEdge ? blk4x4Pos_x - 4 : blk4x4Pos_x;
	blk4x4PosNeighbourY = isVerticalEdge ? blk4x4Pos_y : blk4x4Pos_y - 4;

	if (puCodingMode == INTRA_MODE || neighbourPuCodingMode == INTRA_MODE) {
		bS = 2;
	}
	else {
		switch (sliceType) {
		case EB_P_PICTURE:
			puRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
			neighborPuRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;

			// different number of motion vectors or difference reference pictures
			bSDeterminationCondition1 = (EB_BOOL)(puRefList0POC != neighborPuRefList0POC);

			// if the difference of the horizontal or vertical components of MV >= 4 quater luma pixel, bS = 1
			bSDeterminationCondition2 = (EB_BOOL)(CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
				puPtr,
				neighbourPuPtr,
				REF_LIST_0,
				REF_LIST_0));

			// in non-zero luma TU, bS = 1
			bSDeterminationCondition3 = (EB_BOOL)(isTuBoundary && (
				pictureControlSetPtr->cbfMapArray[(blk4x4Pos_x >> 2) + (blk4x4Pos_y >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)] ||
				pictureControlSetPtr->cbfMapArray[(blk4x4PosNeighbourX >> 2) + (blk4x4PosNeighbourY >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)]));

			bS = (EB_U8)(bSDeterminationCondition1 | bSDeterminationCondition2 | bSDeterminationCondition3);

			break;

		case EB_B_PICTURE:
			switch (puPtr->interPredDirectionIndex + ((neighbourPuPtr->interPredDirectionIndex) * 3)) {
			case 0:         // UNI_PRED_LIST_0 + UNI_PRED_LIST_0
				puRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
				neighborPuRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;

				bSDeterminationCondition1 = (EB_BOOL)(
					(puRefList0POC != neighborPuRefList0POC) ||
					CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
					puPtr,
					neighbourPuPtr,
					REF_LIST_0,
					REF_LIST_0));

				break;
			case 1:         // UNI_PRED_LIST_1 + UNI_PRED_LIST_0
				puRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;
				neighborPuRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;

				bSDeterminationCondition1 = (EB_BOOL)(
					(puRefList1POC != neighborPuRefList0POC) ||
					CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
					puPtr,
					neighbourPuPtr,
					REF_LIST_1,
					REF_LIST_0));

				break;
			case 2:         // BI_PRED + UNI_PRED_LIST_0
				bSDeterminationCondition1 = EB_TRUE;

				break;
			case 3:         // UNI_PRED_LIST_0 + UNI_PRED_LIST_1
				puRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
				neighborPuRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;

				bSDeterminationCondition1 = (EB_BOOL)(
					(puRefList0POC != neighborPuRefList1POC) ||
					CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
					puPtr,
					neighbourPuPtr,
					REF_LIST_0,
					REF_LIST_1));

				break;
			case 4:         // UNI_PRED_LIST_1 + UNI_PRED_LIST_1
				puRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;
				neighborPuRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;

				bSDeterminationCondition1 = (EB_BOOL)(
					(puRefList1POC != neighborPuRefList1POC) ||
					CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
					puPtr,
					neighbourPuPtr,
					REF_LIST_1,
					REF_LIST_1));

				break;
			case 5:         // BI_PRED + UNI_PRED_LIST_1
				bSDeterminationCondition1 = EB_TRUE;

				break;
			case 6:         // UNI_PRED_LIST_0 + BI_PRED
				bSDeterminationCondition1 = EB_TRUE;

				break;
			case 7:         // UNI_PRED_LIST_1 + BI_PRED
				bSDeterminationCondition1 = EB_TRUE;

				break;
			case 8:         // BI_PRED + BI_PRED
				puRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
				neighborPuRefList0POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_0]->objectPtr)->refPOC;
				puRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;
				neighborPuRefList1POC = ((EbReferenceObject_t*)pictureControlSetPtr->refPicPtrArray[REF_LIST_1]->objectPtr)->refPOC;

				if ((puRefList0POC == neighborPuRefList0POC) && (puRefList1POC == neighborPuRefList1POC) &&
					(puRefList0POC == neighborPuRefList1POC) && (puRefList1POC == neighborPuRefList0POC)) {

					bSDeterminationCondition1 = (EB_BOOL)(
						(CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
						puPtr,
						neighbourPuPtr,
						REF_LIST_0,
						REF_LIST_0) ||
						CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
						puPtr,
						neighbourPuPtr,
						REF_LIST_1,
						REF_LIST_1)) &&
						(CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
						puPtr,
						neighbourPuPtr,
						REF_LIST_0,
						REF_LIST_1) ||
						CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
						puPtr,
						neighbourPuPtr,
						REF_LIST_1,
						REF_LIST_0)));
				}
				else {
					if ((puRefList0POC == neighborPuRefList0POC) && (puRefList1POC == neighborPuRefList1POC)) {
						bSDeterminationCondition1 = (EB_BOOL)(
							CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
							puPtr,
							neighbourPuPtr,
							REF_LIST_0,
							REF_LIST_0) ||
							CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
							puPtr,
							neighbourPuPtr,
							REF_LIST_1,
							REF_LIST_1));
					}
					else {
						if ((puRefList0POC == neighborPuRefList1POC) && (puRefList1POC == neighborPuRefList0POC)) {
							bSDeterminationCondition1 = (EB_BOOL)(
								CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
								puPtr,
								neighbourPuPtr,
								REF_LIST_0,
								REF_LIST_1) ||
								CHECK_MV_COMPONENT_EQUAL_OR_GREATER_THAN_4(
								puPtr,
								neighbourPuPtr,
								REF_LIST_1,
								REF_LIST_0));
						}
						else {
							bSDeterminationCondition1 = EB_TRUE;
						}
					}
				}

				break;
			default:
				CHECK_REPORT_ERROR_NC(
					encodeContextPtr->appCallbackPtr,
					EB_ENC_DLF_ERROR2);
				bSDeterminationCondition1 = EB_FALSE;    // junk code to avoid compiling warning!
				break;
			}

			// in non-zero luma TU, bS = 1
			bSDeterminationCondition2 = (EB_BOOL)(isTuBoundary && (
				pictureControlSetPtr->cbfMapArray[(blk4x4Pos_x >> 2) + (blk4x4Pos_y >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)] ||
				pictureControlSetPtr->cbfMapArray[(blk4x4PosNeighbourX >> 2) + (blk4x4PosNeighbourY >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)]));

			bS = (EB_U8)(bSDeterminationCondition1 | bSDeterminationCondition2);

			break;

		case EB_I_PICTURE:
			CHECK_REPORT_ERROR_NC(
				encodeContextPtr->appCallbackPtr,
				EB_ENC_DLF_ERROR3);
			break;

		default:
			CHECK_REPORT_ERROR_NC(
				encodeContextPtr->appCallbackPtr,
				EB_ENC_DLF_ERROR5);
			break;
		}
	}

	return bS;
}

static inline EB_U8 Intra4x4CalculateBSForTUBoundaryInsidePU(
	EB_U32                 blk4x4Pos_x,             //input paramter, picture-wise horizontal location of the 4x4 block.
	EB_U32                 blk4x4Pos_y,             //input paramter, picture-wise vertical location of the 4x4 block.
	EB_MODETYPE            cuCodingMode,            //input paramter, the coding mode of the CU where the 4x4 block belongs to.
	EB_BOOL                isVerticalEdge,          //input paramter
	PictureControlSet_t   *pictureControlSetPtr,
	SequenceControlSet_t  *sequenceControlSetPtr)
{
	EB_U8   bS = 0;
	EB_U32  blk4x4PosNeighbourX;
	EB_U32  blk4x4PosNeighbourY;

	blk4x4PosNeighbourX = isVerticalEdge ? blk4x4Pos_x - 4 : blk4x4Pos_x;
	blk4x4PosNeighbourY = isVerticalEdge ? blk4x4Pos_y : blk4x4Pos_y - 4;

	if (cuCodingMode == INTRA_MODE) {
		bS = 2;
	}
	else {
		bS = pictureControlSetPtr->cbfMapArray[(blk4x4Pos_x >> 2) + (blk4x4Pos_y >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)] ||
			pictureControlSetPtr->cbfMapArray[(blk4x4PosNeighbourX >> 2) + (blk4x4PosNeighbourY >> 2) *(sequenceControlSetPtr->lumaWidth >> 2)];
	}

	return bS;
}
void Intra4x4SetBSArrayBasedOnTUBoundary(
	EB_U32                 tuPos_x,                                  //input parameter, the picture-wise horizontal location of a TU.
	EB_U32                 tuPos_y,                                  //input parameter, the picture-wise vertical location of a TU.
	EB_U32                 tuWidth,                                  //input parameter
	EB_U32                 tuHeight,                                 //input parameter
	EB_BOOL                isHorizontalTuBoundaryAlsoPuBoundary,     //input parameter
	EB_BOOL                isVerticalTuBoundaryAlsoPuBoundary,       //input parameter
	//CodingUnit_t          *cuPtr,                                    //input parameter, pointer to the CU where the TU belongs to.
	const CodedUnitStats_t      *cuStatsPtr,
	EB_MODETYPE            cuCodingMode,                             //input paramter, the coding mode of the CU where the TU belongs to.
	EB_U32                 tbOriginX,
	EB_U32                 tbOriginY,
	PictureControlSet_t   *pictureControlSetPtr,
	EB_U8                 *horizontalEdgeBSArray,
	EB_U8                 *verticalEdgeBSArray)
{
	const EB_U32 MaxLcuSizeIn4x4blk = MAX_LCU_SIZE >> 2;
	const EB_U32 logMaxLcuSizeIn4x4blk = Log2f(MaxLcuSizeIn4x4blk);
	SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	EB_U32                lcuSize = sequenceControlSetPtr->lcuSize;

	EB_U32  tuTopLeft4x4blkAddr = ((tuPos_x&(lcuSize - 1)) >> 2) + (((tuPos_y&(lcuSize - 1)) >> 2) << logMaxLcuSizeIn4x4blk);
	EB_U32  tuTopRight4x4blkAddr = tuTopLeft4x4blkAddr + (tuWidth >> 2) - 1;
	EB_U32  tuBottomLeft4x4blkAddr = tuTopLeft4x4blkAddr + (((tuHeight >> 2) - 1) << logMaxLcuSizeIn4x4blk);
	EB_U32  blk4x4Addr;

	//set bS for the horizontal TU boundary which lies on the 8 sample edge inside a PU
	if ((tuPos_y & 7) == 0 && tuPos_y != 0 && tuPos_y != tbOriginY + cuStatsPtr->originY && !isHorizontalTuBoundaryAlsoPuBoundary) {
		for (blk4x4Addr = tuTopLeft4x4blkAddr; blk4x4Addr <= tuTopRight4x4blkAddr; ++blk4x4Addr) {
			horizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)]
				= Intra4x4CalculateBSForTUBoundaryInsidePU(
				tbOriginX + ((blk4x4Addr & (MaxLcuSizeIn4x4blk - 1)) << 2),
				tbOriginY + ((blk4x4Addr >> logMaxLcuSizeIn4x4blk) << 2),
				cuCodingMode,
				EB_FALSE,
				pictureControlSetPtr,
				sequenceControlSetPtr);
		}
	}

	//set bS for the vertical TU boundary which lies on the 8 sample edge inside a PU
	if ((tuPos_x & 7) == 0 && tuPos_x != 0 && tuPos_x != tbOriginX + cuStatsPtr->originX && !isVerticalTuBoundaryAlsoPuBoundary) {
		for (blk4x4Addr = tuTopLeft4x4blkAddr; blk4x4Addr <= tuBottomLeft4x4blkAddr; blk4x4Addr += MaxLcuSizeIn4x4blk) {
			verticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)]
				= Intra4x4CalculateBSForTUBoundaryInsidePU(
				tbOriginX + ((blk4x4Addr & (MaxLcuSizeIn4x4blk - 1)) << 2),
				tbOriginY + ((blk4x4Addr >> logMaxLcuSizeIn4x4blk) << 2),
				cuCodingMode,
				EB_TRUE,
				pictureControlSetPtr,
				sequenceControlSetPtr);
		}
	}

	return;
}

void Intra4x4SetBSArrayBasedOnPUBoundary(
	NeighborArrayUnit_t         *modeTypeNeighborArray,
	NeighborArrayUnit_t         *mvNeighborArray,
	PredictionUnit_t            *puPtr,
	CodingUnit_t                *cuPtr,
	const CodedUnitStats_t      *cuStatsPtr,
	EB_U32                       puOriginX,
    EB_U32                       puOriginY,
    EB_U32                       puWidth,
    EB_U32                       puHeight,
    EB_U32                       tbOriginX,
	EB_U32                       tbOriginY,
	EB_BOOL                      isVerticalPUBoundaryAlsoPictureBoundary,      //input parameter, please refer to the detailed explanation above.
	EB_BOOL                      isHorizontalPUBoundaryAlsoPictureBoundary,    //input parameter, please refer to the detailed explanation above.
	PictureControlSet_t         *pictureControlSetPtr,
	EB_U8                       *horizontalEdgeBSArray,
	EB_U8                       *verticalEdgeBSArray)
{
	const EB_U32 logMinPuSize = Log2f(MIN_PU_SIZE);
	const EB_U32 MaxLcuSizeIn4x4blk = MAX_LCU_SIZE >> 2;
	const EB_U32 logMaxLcuSizeIn4x4blk = Log2f(MaxLcuSizeIn4x4blk);
	SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;


	// MV Uint
	MvUnit_t   *mvUnitPtr;

	// 4x4 block
	EB_U32  puTopLeft4x4blkAddr = (puOriginX >> 2) + ((puOriginY >> 2) << logMaxLcuSizeIn4x4blk);
	EB_U32  puTopRight4x4blkAddr = puTopLeft4x4blkAddr + (puWidth >> 2) - 1;
	EB_U32  puBottomLeft4x4blkAddr = puTopLeft4x4blkAddr + (((puHeight >> 2) - 1) << logMaxLcuSizeIn4x4blk);
	EB_U32  blk4x4Addr;
	EB_U32  blk4x4Pos_x;    // sample-based LCU-wise horizontal location of the 4x4 block
	EB_U32  blk4x4Pos_y;    // sample-based LCU-wise vertical location of the 4x4 block
	EB_BOOL isVerticalPuBoundaryAlsoTuBoundary;
	EB_BOOL isHorizontalPuBoundaryAlsoTuBoundary;

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

	// PU
	//PredictionUnit_t *neighbourPuPtrOLD;
	PredictionUnit_t  neighbourPu;
	EB_MODETYPE          neighbourPuMode;
	EB_U32               neighborPuIdx;

	/*CHECK_REPORT_ERROR(
		(cuPtr->partitionMode == SIZE_2Nx2N),
		encodeContextPtr->appCallbackPtr,
		EB_ENC_DLF_ERROR6);*/

	// set bS for the horizontal PU boundary which lies on the 8 sample edge
	if ((puOriginY & 7) == 0 && puOriginY + tbOriginY != 0 && !isHorizontalPUBoundaryAlsoPictureBoundary) {
		for (blk4x4Addr = puTopLeft4x4blkAddr; blk4x4Addr <= puTopRight4x4blkAddr; ++blk4x4Addr) {

			blk4x4Pos_x = (blk4x4Addr & (MaxLcuSizeIn4x4blk - 1)) << 2;
			blk4x4Pos_y = (blk4x4Addr >> logMaxLcuSizeIn4x4blk) << 2;

			if (puOriginY == cuStatsPtr->originY){
				isHorizontalPuBoundaryAlsoTuBoundary = EB_TRUE;
			}
			else{
				isHorizontalPuBoundaryAlsoTuBoundary = cuPtr->transformUnitArray[0].splitFlag ? EB_TRUE : EB_FALSE;
			}
			if (puOriginX == cuStatsPtr->originX){
				isVerticalPuBoundaryAlsoTuBoundary = EB_TRUE;
			}
			else{
				isVerticalPuBoundaryAlsoTuBoundary = cuPtr->transformUnitArray[0].splitFlag ? EB_TRUE : EB_FALSE;
			}

			CHECK_REPORT_ERROR(
				((blk4x4Pos_x == puOriginX) || (blk4x4Pos_y == puOriginY)),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_DLF_ERROR6);

			neighborPuIdx = (tbOriginX + blk4x4Pos_x) >> logMinPuSize;


			CHECK_REPORT_ERROR(
				(*(modeTypeNeighborArray->topArray + neighborPuIdx) != (EB_U8)INVALID_MODE),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_DLF_ERROR9);

			neighbourPuMode = *(modeTypeNeighborArray->topArray + neighborPuIdx);
			mvUnitPtr = (MvUnit_t*)(mvNeighborArray->topArray) + neighborPuIdx;
			neighbourPu.interPredDirectionIndex = mvUnitPtr->predDirection;
			neighbourPu.mv[REF_LIST_0].mvUnion = mvUnitPtr->mv[REF_LIST_0].mvUnion;
			neighbourPu.mv[REF_LIST_1].mvUnion = mvUnitPtr->mv[REF_LIST_1].mvUnion;

            horizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)]
                = Intra4x4CalculateBSForPUBoundary(
                tbOriginX + blk4x4Pos_x,       // to be modified
                tbOriginY + blk4x4Pos_y,       // to be modified
                puPtr,
                &neighbourPu,
                INTRA_MODE,
                INTRA_MODE,
                EB_FALSE,
                EB_FALSE,
                EB_FALSE,
                pictureControlSetPtr,
                sequenceControlSetPtr);
		}
	}

	// set bS for the vertical PU boundary which lies on the 8 sample edge
	if ((puOriginX & 7) == 0 && puOriginX + tbOriginX != 0 && !isVerticalPUBoundaryAlsoPictureBoundary) {
		for (blk4x4Addr = puTopLeft4x4blkAddr; blk4x4Addr <= puBottomLeft4x4blkAddr; blk4x4Addr += MaxLcuSizeIn4x4blk) {
			blk4x4Pos_x = (blk4x4Addr & (MaxLcuSizeIn4x4blk - 1)) << 2;
			blk4x4Pos_y = (blk4x4Addr >> logMaxLcuSizeIn4x4blk) << 2;

			if (puOriginY == cuStatsPtr->originY){
				isHorizontalPuBoundaryAlsoTuBoundary = EB_TRUE;
			}
			else{
				isHorizontalPuBoundaryAlsoTuBoundary = cuPtr->transformUnitArray[0].splitFlag ? EB_TRUE : EB_FALSE;
			}
			if (puOriginX == cuStatsPtr->originX){
				isVerticalPuBoundaryAlsoTuBoundary = EB_TRUE;
			}
			else{
				isVerticalPuBoundaryAlsoTuBoundary =  cuPtr->transformUnitArray[0].splitFlag  ? EB_TRUE : EB_FALSE;
			}

			CHECK_REPORT_ERROR(
				((blk4x4Pos_x == puOriginX) || (blk4x4Pos_y == puOriginY)),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_DLF_ERROR6);

			neighborPuIdx = (tbOriginY + blk4x4Pos_y) >> logMinPuSize;

			CHECK_REPORT_ERROR(
				(*(modeTypeNeighborArray->leftArray + neighborPuIdx) != (EB_U8)INVALID_MODE),
				encodeContextPtr->appCallbackPtr,
				EB_ENC_DLF_ERROR9);

			neighbourPuMode = *(modeTypeNeighborArray->leftArray + neighborPuIdx);
			mvUnitPtr = (MvUnit_t*)(mvNeighborArray->leftArray) + neighborPuIdx /** neighborArrays->mv->unitSize*/;
			neighbourPu.interPredDirectionIndex = mvUnitPtr->predDirection;
			neighbourPu.mv[REF_LIST_0].mvUnion = mvUnitPtr->mv[REF_LIST_0].mvUnion;
			neighbourPu.mv[REF_LIST_1].mvUnion = mvUnitPtr->mv[REF_LIST_1].mvUnion;

			verticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)]
				= Intra4x4CalculateBSForPUBoundary(
				tbOriginX + blk4x4Pos_x,       // to be modified
				tbOriginY + blk4x4Pos_y,       // to be modified
				puPtr,
				&neighbourPu,
				(EB_MODETYPE)cuPtr->predictionModeFlag,
				neighbourPuMode,
				EB_TRUE,
				isVerticalPuBoundaryAlsoTuBoundary,
				isHorizontalPuBoundaryAlsoTuBoundary,
				pictureControlSetPtr,
				sequenceControlSetPtr);
		}
	}

	return;
}


/** Luma8x8blkDLFCore()
is used to conduct the deblocking fitler for each 8x8 independent
luma block. There are two 8 sample luma edges crossing each other inside
the block.
EdgeA --- upper vertical 4 sample chroma edge
EdgeB --- left horizontal 4 sample chroma edge
EdgeC --- lower vertical 4 sample chroma edge
EdgeD --- right horizontal 4 sample chroma edge

@param reconChromaPic (input)
reconChromaPic is the pointer to the (0, 0) position of the luma reconstructed picture to
be filtered.
@param reconLumaPicStride (input)
@param centerSamplePos_x (input)
centerSamplePos_x is the picutre-wise horizontal location of the start sample of edge C & D
@param centerSamplePos_y (input)
centerSamplePos_y is the picutre-wise vertical location of the start sample of edge C & D
@param *bSEdgeA (input)
@param *bSEdgeB (input)
@param *bSEdgeC (input)
@param *bSEdgeD (input)
@param *reconPictureControlSet (input)
reconPictureControlSet is the pointer to the PictureControlSet of the reconstructed picture
*/
static void Luma8x8blkDLFCore(
	EbPictureBufferDesc_t *reconPic,                //please refer to the detailed explantion above.
	EB_U32                 reconLumaPicStride,      //please refer to the detailed explantion above.
	EB_U32                 centerSamplePos_x,       //please refer to the detailed explantion above.
	EB_U32                 centerSamplePos_y,       //please refer to the detailed explantion above.
	EB_U8                  bSEdgeA,                 //please refer to the detailed explantion above.
	EB_U8                  bSEdgeB,                 //please refer to the detailed explantion above.
	EB_U8                  bSEdgeC,                 //please refer to the detailed explantion above.
	EB_U8                  bSEdgeD,                 //please refer to the detailed explantion above.
	PictureControlSet_t   *reconPictureControlSet)  //please refer to the detailed explantion above.
{
	EB_U32   curCuQp;
	EB_BYTE  edgeStartFilteredSamplePtr;
	EB_U32  neighbourCuQp;
	EB_S32   CUqpIndex;
	EB_U32   Qp;
	EB_U32   Tc;
	EB_S32   Beta;
	//EB_BOOL  PCMFlagArray[2];

	//vertical edge A filtering
	if (bSEdgeA > 0) {
		/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
		*/

		// Qp for the current CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x,
			centerSamplePos_y - 4,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x - 1,
			centerSamplePos_y - 4,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		Qp = (curCuQp + neighbourCuQp + 1) >> 1;

		Tc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET,  Qp + ((bSEdgeA > 1) << 1) + reconPictureControlSet->tcOffset )];
		Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];
		edgeStartFilteredSamplePtr = reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + (centerSamplePos_y - 4) * reconLumaPicStride + centerSamplePos_x;

		// luma 4 sample edge DLF core
		Luma4SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartFilteredSamplePtr,
			reconLumaPicStride,
			EB_TRUE,
			Tc,
			Beta);
	}

	//vertical edge C filtering
	if (bSEdgeC > 0) {
		/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
		*/

		// Qp for the current CU

		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];
		// Qp for the neighboring CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x - 1,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		Qp = (curCuQp + neighbourCuQp + 1) >> 1;
		Tc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bSEdgeC > 1) << 1) + reconPictureControlSet->tcOffset)];
		Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];
		edgeStartFilteredSamplePtr = reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + centerSamplePos_y * reconLumaPicStride + centerSamplePos_x;


		// luma 4 sample edge DLF core
		Luma4SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartFilteredSamplePtr,
			reconLumaPicStride,
			EB_TRUE,
			Tc,
			Beta);
	}

	//horizontal edge B filtering
	if (bSEdgeB > 0) {
		/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
		*/

		// Qp for the current CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x - 4,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x - 4,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		Qp = (curCuQp + neighbourCuQp + 1) >> 1;
		Tc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bSEdgeB > 1) << 1) + reconPictureControlSet->tcOffset)];
		Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];
		edgeStartFilteredSamplePtr = reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + centerSamplePos_y * reconLumaPicStride + (centerSamplePos_x - 4);

		// luma 4 sample edge DLF core
		Luma4SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartFilteredSamplePtr,
			reconLumaPicStride,
			EB_FALSE,
			Tc,
			Beta);
	}

	//horizontal edge D filtering
	if (bSEdgeD > 0) {
		/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
		*/

		// Qp for the current CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		Qp = (curCuQp + neighbourCuQp + 1) >> 1;
		Tc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bSEdgeD > 1) << 1) + reconPictureControlSet->tcOffset)];
		Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];
		edgeStartFilteredSamplePtr = reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + centerSamplePos_y * reconLumaPicStride + centerSamplePos_x;

		// luma 4 sample edge DLF core
		Luma4SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartFilteredSamplePtr,
			reconLumaPicStride,
			EB_FALSE,
			Tc,
			Beta);
	}

	return;
}

void Luma8x8blkDLFCore16bit(
	EbPictureBufferDesc_t *reconPic,                //please refer to the detailed explantion above.
	EB_U32                 reconLumaPicStride,      //please refer to the detailed explantion above.
	EB_U32                 centerSamplePos_x,       //please refer to the detailed explantion above.
	EB_U32                 centerSamplePos_y,       //please refer to the detailed explantion above.
	EB_U8                  bSEdgeA,                 //please refer to the detailed explantion above.
	EB_U8                  bSEdgeB,                 //please refer to the detailed explantion above.
	EB_U8                  bSEdgeC,                 //please refer to the detailed explantion above.
	EB_U8                  bSEdgeD,                 //please refer to the detailed explantion above.
	PictureControlSet_t   *reconPictureControlSet)  //please refer to the detailed explantion above.

{
	EB_U32   curCuQp;
	EB_U32  neighbourCuQp;
	EB_S32   CUqpIndex;
	EB_U32   Qp;
	EB_U32   Tc;
	EB_S32   Beta;
	EB_U16  *edgeStartFilteredSamplePtr;
	//EB_BOOL  PCMFlagArray[2];

	//vertical edge A filtering
	if (bSEdgeA > 0) {
		/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
		*/

		// Qp for the current CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x,
			centerSamplePos_y - 4,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x - 1,
			centerSamplePos_y - 4,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		Qp = (curCuQp + neighbourCuQp + 1) >> 1;
		Tc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bSEdgeA > 1) << 1) + reconPictureControlSet->tcOffset)];
		Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];

		//CHKN
		Tc = Tc << 2;
		Beta = Beta << 2;

		edgeStartFilteredSamplePtr = (EB_U16*)reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + (centerSamplePos_y - 4) * reconLumaPicStride + centerSamplePos_x;

		// luma 4 sample edge DLF core
		lumaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartFilteredSamplePtr,
			reconLumaPicStride,
			EB_TRUE,
			Tc,
			Beta);
	}

	//vertical edge C filtering
	if (bSEdgeC > 0) {
		/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
		*/

		// Qp for the current CU

		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x - 1,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];
		Qp = (curCuQp + neighbourCuQp + 1) >> 1;
		Tc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bSEdgeC > 1) << 1) + reconPictureControlSet->tcOffset)];
		Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];

		//CHKN
		Tc = Tc << 2;
		Beta = Beta << 2;

		edgeStartFilteredSamplePtr = (EB_U16*)reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + centerSamplePos_y  * reconLumaPicStride + centerSamplePos_x;

		// luma 4 sample edge DLF core
		lumaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartFilteredSamplePtr,
			reconLumaPicStride,
			EB_TRUE,
			Tc,
			Beta);
		//PCMFlagArray);
	}

	//horizontal edge B filtering
	if (bSEdgeB > 0) {
		/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
		*/

		// Qp for the current CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x - 4,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x - 4,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];
		Qp = (curCuQp + neighbourCuQp + 1) >> 1;
		Tc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bSEdgeB > 1) << 1) + reconPictureControlSet->tcOffset)];
		Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];

		//CHKN
		Tc = Tc << 2;
		Beta = Beta << 2;
		edgeStartFilteredSamplePtr = (EB_U16*)reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + centerSamplePos_y * reconLumaPicStride + (centerSamplePos_x - 4);

		// luma 4 sample edge DLF core
		lumaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartFilteredSamplePtr,
			reconLumaPicStride,
			EB_FALSE,
			Tc,
			Beta);
	}

	//horizontal edge D filtering
	if (bSEdgeD > 0) {
		/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
		*/

		// Qp for the current CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
			centerSamplePos_x,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];
		Qp = (curCuQp + neighbourCuQp + 1) >> 1;
		Tc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bSEdgeD > 1) << 1) + reconPictureControlSet->tcOffset)];
		Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];

		//CHKN
		Tc = Tc << 2;
		Beta = Beta << 2;
		edgeStartFilteredSamplePtr = (EB_U16*)reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + centerSamplePos_y * reconLumaPicStride + centerSamplePos_x;

		// luma 4 sample edge DLF core
		lumaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartFilteredSamplePtr,
			reconLumaPicStride,
			EB_FALSE,
			Tc,
			Beta);
	}

	return;
}

/** chroma8x8blkDLFCore()
is used to conduct the deblocking fitler for each 8x8 independent
chroma block. There are two 8 sample chroma edges crossing each other inside
the block.
EdgeA --- upper vertical 4 sample chroma edge
EdgeB --- left horizontal 4 sample chroma edge
EdgeC --- lower vertical 4 sample chroma edge
EdgeD --- right horizontal 4 sample chroma edge

@param reconChromaPic (input)
reconChromaPic is the pointer to the (0, 0) position of the chroma (Cb or Cr)
reconstructed picture to be filtered.
@param reconChromaPicStride (input)
@param centerSamplePos_x (input)
centerSamplePos_x is the picutre-wise horizontal location of the start sample of edge C & D
@param centerSamplePos_y (input)
centerSamplePos_y is the picutre-wise vertical location of the start sample of edge C & D
@param *bSEdgeAArray (input)
bSEdgeAArray[0] - bS of the upper 2 sample of edge A. bSEdgeAArray[1] - bS of the lower 2 sample of edge A.
@param *bSEdgeBArray (input)
bSEdgeBArray[0] - bS of the left 2 sample of edge B. bSEdgeAArray[1] - bS of the right 2 sample of edge B.
@param *bSEdgeCArray (input)
bSEdgeCArray[0] - bS of the upper 2 sample of edge C. bSEdgeAArray[1] - bS of the lower 2 sample of edge C.
@param *bSEdgeDArray (input)
bSEdgeDArray[0] - bS of the left 2 sample of edge D. bSEdgeAArray[1] - bS of the right 2 sample of edge D.
@param *reconPictureControlSet (input)
reconPictureControlSet is the pointer to the PictureControlSet of the reconstructed picture
*/
static void chroma8x8blkDLFCore(
	EbPictureBufferDesc_t *reconPic,                    //please refer to the detailed explanation above.
	EB_U32                 reconChromaPicStride,        //please refer to the detailed explanation above.
	EB_U32                 centerSamplePos_x,           //please refer to the detailed explanation above.
	EB_U32                 centerSamplePos_y,           //please refer to the detailed explanation above.
	EB_U8                 *bSEdgeAArray,                //please refer to the detailed explanation above.
	EB_U8                 *bSEdgeBArray,                //please refer to the detailed explanation above.
	EB_U8                 *bSEdgeCArray,                //please refer to the detailed explanation above.
	EB_U8                 *bSEdgeDArray,                //please refer to the detailed explanation above.
	PictureControlSet_t   *reconPictureControlSet)      //please refer to the detailed explanation above.
{
	EB_U8   curCuQp;
	EB_BYTE  edgeStartSampleCb;
	EB_BYTE  edgeStartSampleCr;
	EB_U8  neighbourCuQp;
	EB_U8   cbQp;
	EB_U8   crQp;
	EB_U8   cbTc;
	EB_U8   crTc;
	//EB_BOOL  chromaPCMFlagArray[2];
	EB_S32   CUqpIndex;
    EB_COLOR_FORMAT colorFormat   = reconPic->colorFormat;
    const EB_S32 subWidthC        = colorFormat==EB_YUV444?1:2;
    const EB_S32 subHeightC       = colorFormat==EB_YUV420?2:1;
    const EB_S32 subWidthCMinus1  = colorFormat==EB_YUV444?0:1;
    const EB_S32 subHeightCMinus1 = colorFormat==EB_YUV420?1:0;

	//vertical edge A filtering
	if (bSEdgeAArray[0] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y - 4,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 1,
			centerSamplePos_y - 4,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];
		edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + (centerSamplePos_y - 4) * reconChromaPicStride + centerSamplePos_x;
		edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + (centerSamplePos_y - 4) * reconChromaPicStride + centerSamplePos_x;

		Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_TRUE,
			cbTc,
			crTc);

	}
	if (bSEdgeAArray[1] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y - 2,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 1,
			centerSamplePos_y - 2,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];
		edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + (centerSamplePos_y - 2) * reconChromaPicStride + centerSamplePos_x;
		edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + (centerSamplePos_y - 2) * reconChromaPicStride + centerSamplePos_x;


		Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_TRUE,
			cbTc,
			crTc);

	}

	//vertical edge C filtering
	if (bSEdgeCArray[0] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 1,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);

		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];
		edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + centerSamplePos_y * reconChromaPicStride + centerSamplePos_x;
		edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + centerSamplePos_y * reconChromaPicStride + centerSamplePos_x;


		Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_TRUE,
			cbTc,
			crTc);
	}

	if (bSEdgeCArray[1] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y + 2,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];


		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 1,
			centerSamplePos_y + 2,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];


		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);
		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];
		edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + (centerSamplePos_y + 2) * reconChromaPicStride + centerSamplePos_x;
		edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + (centerSamplePos_y + 2) * reconChromaPicStride + centerSamplePos_x;

		Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_TRUE,
			cbTc,
			crTc);
	}

	//horizontal edge B filtering
	if (bSEdgeBArray[0] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 4,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 4,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];
		edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x - 4);
		edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x - 4);

		Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_FALSE,
			cbTc,
			crTc);

	}

	if (bSEdgeBArray[1] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 2,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 2,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];
		edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x - 2);
		edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x - 2);

		Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_FALSE,
			cbTc,
			crTc);

	}

	//horizontal edge D filtering
	if (bSEdgeDArray[0] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);

		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - (EB_S32) reconPictureControlSet->qpArrayStride;

		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];
		edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + centerSamplePos_y * reconChromaPicStride + centerSamplePos_x;
		edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + centerSamplePos_y * reconChromaPicStride + centerSamplePos_x;

		Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_FALSE,
			cbTc,
			crTc);

	}

	if (bSEdgeDArray[1] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x + 2,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x + 2,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];
		edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x + 2);
		edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x + 2);

		Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_FALSE,
			cbTc,
			crTc);

	}

	return;
}
static void chroma8x8blkDLFCore16bit(
	EbPictureBufferDesc_t *reconPic,                    //please refer to the detailed explanation above.
	EB_U32                 reconChromaPicStride,        //please refer to the detailed explanation above.
	EB_U32                 centerSamplePos_x,           //please refer to the detailed explanation above.
	EB_U32                 centerSamplePos_y,           //please refer to the detailed explanation above.
	EB_U8                 *bSEdgeAArray,                //please refer to the detailed explanation above.
	EB_U8                 *bSEdgeBArray,                //please refer to the detailed explanation above.
	EB_U8                 *bSEdgeCArray,                //please refer to the detailed explanation above.
	EB_U8                 *bSEdgeDArray,                //please refer to the detailed explanation above.
	PictureControlSet_t   *reconPictureControlSet)      //please refer to the detailed explanation above.
{
	EB_U8   curCuQp;
	EB_U16  *edgeStartSampleCb;
	EB_U16  *edgeStartSampleCr;
	EB_U8  neighbourCuQp;
	EB_U8   cbQp;
	EB_U8   crQp;
	EB_U8    cbTc;
	EB_U8    crTc;
	//EB_BOOL  chromaPCMFlagArray[2];
	EB_S32   CUqpIndex;
    EB_COLOR_FORMAT colorFormat = reconPic->colorFormat;
    const EB_S32 subWidthC      = colorFormat==EB_YUV444?1:2;
    const EB_S32 subHeightC     = colorFormat==EB_YUV420?2:1;
    const EB_S32 subWidthCMinus1  = colorFormat==EB_YUV444?0:1;
    const EB_S32 subHeightCMinus1 = colorFormat==EB_YUV420?1:0;

	//vertical edge A filtering
	if (bSEdgeAArray[0] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y - 4,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 1,
			centerSamplePos_y - 4,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];

		//CHKN
		cbTc = cbTc << 2;
		crTc = crTc << 2;

        edgeStartSampleCb = (EB_U16*)reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + (centerSamplePos_y - 4) * reconChromaPicStride + centerSamplePos_x;
        edgeStartSampleCr = (EB_U16*)reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + (centerSamplePos_y - 4) * reconChromaPicStride + centerSamplePos_x;

		chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_TRUE,
			cbTc,
			crTc);
		//chromaPCMFlagArray);
	}
	if (bSEdgeAArray[1] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y - 2,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 1,
			centerSamplePos_y - 2,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];

		//CHKN
		cbTc = cbTc << 2;
		crTc = crTc << 2;

		edgeStartSampleCb = (EB_U16*)reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + (centerSamplePos_y - 2) * reconChromaPicStride + centerSamplePos_x;
		edgeStartSampleCr = (EB_U16*)reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + (centerSamplePos_y - 2) * reconChromaPicStride + centerSamplePos_x;

		chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_TRUE,
			cbTc,
			crTc);
		//chromaPCMFlagArray);
	}

	//vertical edge C filtering
	if (bSEdgeCArray[0] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 1,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);

		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];

		//CHKN
		cbTc = cbTc << 2;
		crTc = crTc << 2;

		edgeStartSampleCb = (EB_U16*)reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + centerSamplePos_y * reconChromaPicStride + centerSamplePos_x;
		edgeStartSampleCr = (EB_U16*)reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + centerSamplePos_y * reconChromaPicStride + centerSamplePos_x;

		chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_TRUE,
			cbTc,
			crTc);
		//chromaPCMFlagArray);
	}

	if (bSEdgeCArray[1] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y + 2,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 1,
			centerSamplePos_y + 2,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);
		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];

		//CHKN
		cbTc = cbTc << 2;
		crTc = crTc << 2;
		edgeStartSampleCb = (EB_U16*)reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + (centerSamplePos_y + 2) * reconChromaPicStride + centerSamplePos_x;
		edgeStartSampleCr = (EB_U16*)reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + (centerSamplePos_y + 2) * reconChromaPicStride + centerSamplePos_x;


		chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_TRUE,
			cbTc,
			crTc);
		//chromaPCMFlagArray);
	}

	//horizontal edge B filtering
	if (bSEdgeBArray[0] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 4,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 4,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];

		//CHKN
		cbTc = cbTc << 2;
		crTc = crTc << 2;

		edgeStartSampleCb = (EB_U16*)reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x - 4);
		edgeStartSampleCr = (EB_U16*)reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x - 4);

		chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_FALSE,
			cbTc,
			crTc);
		//chromaPCMFlagArray);
	}

	if (bSEdgeBArray[1] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 2,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x - 2,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];

		//CHKN
		cbTc = cbTc << 2;
		crTc = crTc << 2;
		edgeStartSampleCb = (EB_U16*)reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x - 2);
		edgeStartSampleCr = (EB_U16*)reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x - 2);


		chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_FALSE,
			cbTc,
			crTc);
		//chromaPCMFlagArray);
	}

	//horizontal edge D filtering
	if (bSEdgeDArray[0] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);

		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - (EB_S32) reconPictureControlSet->qpArrayStride;

		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];

		//CHKN
		cbTc = cbTc << 2;
		crTc = crTc << 2;
		edgeStartSampleCb = (EB_U16*)reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + centerSamplePos_y * reconChromaPicStride + centerSamplePos_x;
		edgeStartSampleCr = (EB_U16*)reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + centerSamplePos_y * reconChromaPicStride + centerSamplePos_x;


		chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_FALSE,
			cbTc,
			crTc);
		//chromaPCMFlagArray);
	}

	if (bSEdgeDArray[1] > 1) {
		// Qp for the current CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x + 2,
			centerSamplePos_y,
			reconPictureControlSet->qpArrayStride);


		curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		// Qp for the neighboring CU
		CUqpIndex =
			CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
            subWidthC,
            subHeightC,
			centerSamplePos_x + 2,
			centerSamplePos_y - 1,
			reconPictureControlSet->qpArrayStride);
		//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
		neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

		cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
		crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

		//chromaQp = MapChromaQp(Qp);
		cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
		crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];

		//CHKN
		cbTc = cbTc << 2;
		crTc = crTc << 2;
		edgeStartSampleCb = (EB_U16*)reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x + 2);
		edgeStartSampleCr = (EB_U16*)reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + centerSamplePos_y * reconChromaPicStride + (centerSamplePos_x + 2);

		chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
			edgeStartSampleCb,
			edgeStartSampleCr,
			reconChromaPicStride,
			EB_FALSE,
			cbTc,
			crTc);
		//chromaPCMFlagArray);
	}

	return;
}
/** LCUInternalAreaDLFCore()
is used to apply the deblocking filter in the LCU internal area (excluding the LCU boundary)
as the first pass DLF.
*/
EB_ERRORTYPE LCUInternalAreaDLFCore(
	EbPictureBufferDesc_t *reconpicture,                   //input parameter, the pointer to the reconstructed picture.
	EB_U32                 lcuPos_x,                    //input parameter, the picture-wise horizontal location of the LCU.
	EB_U32                 lcuPos_y,                    //input parameter, the picture-wise vertical location of the LCU.
	EB_U32                 lcuWidth,                    //input parameter, the LCU width.
	EB_U32                 lcuHeight,                   //input parameter, the LCU height.
	EB_U8                 *verticalEdgeBSArray,         //input parameter, the pointer to the vertical edge BS array.
	EB_U8                 *horizontalEdgeBSArray,       //input parameter, the pointer to the hotizontal edge BS array.
	PictureControlSet_t   *reconPictureControlSet)      //input parameter, picture control set.
{
	EB_ERRORTYPE      return_error = EB_ErrorNone;

	const EB_U32 MaxLcuSizeIn4x4blk = MAX_LCU_SIZE >> 2;
	const EB_U32 logMaxLcuSizeIn4x4blk = Log2f(MaxLcuSizeIn4x4blk);

	EB_U32  verticalIdx;
	EB_U32  horizontalIdx;
	EB_U8   bS;

	// variables for luma componet
	EB_U32  blk4x4Addr;
	EB_U32  fourSampleEdgeStartSamplePos_x;
	EB_U32  fourSampleEdgeStartSamplePos_y;
	EB_U32  numVerticalLumaSampleEdges = (lcuWidth >> 3) - 1;
	EB_U32  numHorizontalLumaSampleEdges = (lcuHeight >> 3) - 1;
	EB_U32  num4SampleEdgesPerVerticalLumaSampleEdge = (lcuHeight >> 2) - 2;
	EB_U32  num4SampleEdgesPerHorizontalLumaSampleEdge = (lcuWidth >> 2) - 2;
	EB_S32  Beta;

	// variables for chroma component
	EB_U32  blk2x2Addr;
	EB_U32  twoSampleEdgeStartSamplePos_x;
	EB_U32  twoSampleEdgeStartSamplePos_y;
    EB_COLOR_FORMAT colorFormat            = reconpicture->colorFormat;
    const EB_S32 subWidthC                 = colorFormat==EB_YUV444?1:2;
    const EB_S32 subHeightC                = colorFormat==EB_YUV420?2:1;
    const EB_U32 subWidthShfitMinus1       = colorFormat==EB_YUV444?1:0;
    const EB_U32 subHeightShfitMinus1      = colorFormat==EB_YUV420?0:1;
    const EB_S32 subWidthCMinus1           = colorFormat==EB_YUV444?0:1;
    const EB_S32 subHeightCMinus1          = colorFormat==EB_YUV420?1:0;
    EB_U32  chromaLcuPos_x                 = lcuPos_x >> (colorFormat==EB_YUV444?0:1);
    EB_U32  chromaLcuPos_y                 = lcuPos_y >> (colorFormat==EB_YUV420?1:0);
    EB_U32  numVerticalChromaSampleEdges   = (lcuWidth  >> (colorFormat==EB_YUV444?3:4)) - (colorFormat==EB_YUV444?1:((lcuWidth  & 15) == 0));
    EB_U32  numHorizontalChromaSampleEdges = (lcuHeight >> (colorFormat==EB_YUV420?4:3)) - (colorFormat==EB_YUV420?((lcuHeight & 15) == 0):1);
    EB_U32  num2SampleEdgesPerVerticalChromaSampleEdge   = (lcuHeight >> (colorFormat==EB_YUV420?2:1)) - 2 - (((lcuHeight & (colorFormat==EB_YUV420?15:7)) == 0) << 1);
	EB_U32  num2SampleEdgesPerHorizontalChromaSampleEdge = (lcuWidth  >> (colorFormat==EB_YUV444?1:2)) - 2 - (((lcuWidth & (colorFormat==EB_YUV444?7:15)) == 0) << 1);
	EB_U8  curCuQp;
	EB_BYTE edgeStartFilteredSamplePtr;
	EB_BYTE edgeStartSampleCb;
	EB_BYTE edgeStartSampleCr;
	EB_U8  neighbourCuQp;
	EB_U8  Qp;
	EB_S32  lumaTc;
	//EB_BOOL lumaPCMFlagArray[2];
	//EB_BOOL chromaPCMFlagArray[2];
	EB_S32  CUqpIndex;
	EB_U8  cbQp;
	EB_U8  crQp;
	EB_U8   cbTc;
	EB_U8   crTc;

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(reconPictureControlSet->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

	CHECK_REPORT_ERROR(
		((lcuWidth & 7) == 0 && (lcuHeight & 7) == 0),
		encodeContextPtr->appCallbackPtr,
		EB_ENC_DLF_ERROR8);

	/***** luma component filtering *****/
	// filter all vertical edges
	for (horizontalIdx = 1; horizontalIdx <= numVerticalLumaSampleEdges; ++horizontalIdx) {
		for (verticalIdx = 1; verticalIdx <= num4SampleEdgesPerVerticalLumaSampleEdge; ++verticalIdx) {
			// edge A
			fourSampleEdgeStartSamplePos_x = horizontalIdx << 3;   // LCU-wise position
			fourSampleEdgeStartSamplePos_y = verticalIdx << 2;     // LCU-wise position
			blk4x4Addr = GET_LUMA_4X4BLK_ADDR(
				fourSampleEdgeStartSamplePos_x,
				fourSampleEdgeStartSamplePos_y,
				logMaxLcuSizeIn4x4blk);
			//neighbourBlk4x4Addr = blk4x4Addr - MaxLcuSizeIn4x4blk;

			bS = verticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)];
			if (bS > 0) {
				/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
				*/

				// Qp for the current CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x + lcuPos_x,
					fourSampleEdgeStartSamplePos_y + lcuPos_y,
					reconPictureControlSet->qpArrayStride);


				curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

				// Qp for the neighboring CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x + lcuPos_x - 1,
					fourSampleEdgeStartSamplePos_y + lcuPos_y,
					reconPictureControlSet->qpArrayStride);
				//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
				neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

				Qp = (curCuQp + neighbourCuQp + 1) >> 1;
				lumaTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bS > 1) << 1) + reconPictureControlSet->tcOffset)];
				Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];
				edgeStartFilteredSamplePtr = reconpicture->bufferY + reconpicture->originX + reconpicture->originY * reconpicture->strideY + (fourSampleEdgeStartSamplePos_y + lcuPos_y) * reconpicture->strideY + fourSampleEdgeStartSamplePos_x + lcuPos_x;

				// 4 sample edge DLF core
				Luma4SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartFilteredSamplePtr,
					reconpicture->strideY,
					EB_TRUE,
					lumaTc,
					Beta);
			}
		}
	}

	// filter all horizontal edges
	for (verticalIdx = 1; verticalIdx <= numHorizontalLumaSampleEdges; ++verticalIdx) {
		for (horizontalIdx = 1; horizontalIdx <= num4SampleEdgesPerHorizontalLumaSampleEdge; ++horizontalIdx) {
			// edge B
			fourSampleEdgeStartSamplePos_x = horizontalIdx << 2;    // LCU-wise position
			fourSampleEdgeStartSamplePos_y = verticalIdx << 3;      // LCU-wise position
			blk4x4Addr = GET_LUMA_4X4BLK_ADDR(
				fourSampleEdgeStartSamplePos_x,
				fourSampleEdgeStartSamplePos_y,
				logMaxLcuSizeIn4x4blk);
			//neighbourBlk4x4Addr = blk4x4Addr - 1;

			bS = horizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)];
			if (bS > 0) {
				/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
				*/

				// Qp for the current CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x + lcuPos_x,
					fourSampleEdgeStartSamplePos_y + lcuPos_y,
					reconPictureControlSet->qpArrayStride);


				curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

				// Qp for the neighboring CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x + lcuPos_x,
					fourSampleEdgeStartSamplePos_y + lcuPos_y - 1,
					reconPictureControlSet->qpArrayStride);
				// CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
				neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];
				Qp = (curCuQp + neighbourCuQp + 1) >> 1;
				lumaTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bS > 1) << 1) + reconPictureControlSet->tcOffset)];
				Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];
				edgeStartFilteredSamplePtr = reconpicture->bufferY + reconpicture->originX + reconpicture->originY * reconpicture->strideY + (fourSampleEdgeStartSamplePos_y + lcuPos_y) * reconpicture->strideY + fourSampleEdgeStartSamplePos_x + lcuPos_x;

				// 4 sample edge DLF core
				Luma4SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartFilteredSamplePtr,
					reconpicture->strideY,
					EB_FALSE,
					lumaTc,
					Beta);
			}
		}
	}


	/***** chroma component filtering ****/
	// filter all vertical edges
	for (horizontalIdx = 1; horizontalIdx <= numVerticalChromaSampleEdges; ++horizontalIdx) {
		for (verticalIdx = 1; verticalIdx <= num2SampleEdgesPerVerticalChromaSampleEdge; ++verticalIdx) {
			twoSampleEdgeStartSamplePos_x = horizontalIdx << 3;          // LCU-wise position
			twoSampleEdgeStartSamplePos_y = (verticalIdx << 1) + 2;      // LCU-wise position
			blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
				twoSampleEdgeStartSamplePos_x >> subWidthShfitMinus1,
				twoSampleEdgeStartSamplePos_y >> subHeightShfitMinus1,
				logMaxLcuSizeIn4x4blk);
			bS = verticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

			if (bS > 1) {
				// Qp for the current CU
				CUqpIndex =
					CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                    subWidthC,
                    subHeightC,
					twoSampleEdgeStartSamplePos_x + chromaLcuPos_x,
					twoSampleEdgeStartSamplePos_y + chromaLcuPos_y,
					reconPictureControlSet->qpArrayStride);


				curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

				// Qp for the neighboring CU
				CUqpIndex =
					CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                    subWidthC,
                    subHeightC,
					twoSampleEdgeStartSamplePos_x + chromaLcuPos_x - 1,
					twoSampleEdgeStartSamplePos_y + chromaLcuPos_y,
					reconPictureControlSet->qpArrayStride);
				//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
				neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

				cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
				crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);
				//chromaQp = MapChromaQp(Qp);
				cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, ((EB_S32)cbQp + 2) + reconPictureControlSet->tcOffset)];
				crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, ((EB_S32)crQp + 2) + reconPictureControlSet->tcOffset)];
				edgeStartSampleCb = reconpicture->bufferCb + (reconpicture->originX >> subWidthCMinus1) + (reconpicture->originY >> subHeightCMinus1) * reconpicture->strideCb + (twoSampleEdgeStartSamplePos_y + chromaLcuPos_y) * reconpicture->strideCb + (twoSampleEdgeStartSamplePos_x + chromaLcuPos_x);
				edgeStartSampleCr = reconpicture->bufferCr + (reconpicture->originX >> subWidthCMinus1) + (reconpicture->originY >> subHeightCMinus1) * reconpicture->strideCr + (twoSampleEdgeStartSamplePos_y + chromaLcuPos_y) * reconpicture->strideCr + (twoSampleEdgeStartSamplePos_x + chromaLcuPos_x);

				Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartSampleCb,
					edgeStartSampleCr,
					reconpicture->strideCb,
					EB_TRUE,
					cbTc,
					crTc);

			}
		}
	}

	// filter all horizontal edges
	for (verticalIdx = 1; verticalIdx <= numHorizontalChromaSampleEdges; ++verticalIdx) {
		for (horizontalIdx = 1; horizontalIdx <= num2SampleEdgesPerHorizontalChromaSampleEdge; ++horizontalIdx) {
			twoSampleEdgeStartSamplePos_x = (horizontalIdx << 1) + 2;    // LCU-wise position
			twoSampleEdgeStartSamplePos_y = verticalIdx << 3;            // LCU-wise position
			blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
				twoSampleEdgeStartSamplePos_x >> subWidthShfitMinus1,
				twoSampleEdgeStartSamplePos_y >> subHeightShfitMinus1,
				logMaxLcuSizeIn4x4blk);
			bS = horizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

			if (bS > 1) {
				// Qp for the current CU
				CUqpIndex =
					CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                    subWidthC,
                    subHeightC,
					twoSampleEdgeStartSamplePos_x + chromaLcuPos_x,
					twoSampleEdgeStartSamplePos_y + chromaLcuPos_y,
					reconPictureControlSet->qpArrayStride);


				curCuQp = reconPictureControlSet->qpArray[CUqpIndex];


				// Qp for the neighboring CU
				CUqpIndex =
					CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                    subWidthC,
                    subHeightC,
					twoSampleEdgeStartSamplePos_x + chromaLcuPos_x,
					twoSampleEdgeStartSamplePos_y + chromaLcuPos_y - 1,
					reconPictureControlSet->qpArrayStride);
				//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
				neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

				cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
				crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);
				//chromaQp = MapChromaQp(Qp);
				cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
				crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];
				edgeStartSampleCb = reconpicture->bufferCb + (reconpicture->originX >> subWidthCMinus1) + (reconpicture->originY >> subHeightCMinus1) * reconpicture->strideCb + (twoSampleEdgeStartSamplePos_y + chromaLcuPos_y) * reconpicture->strideCb + (twoSampleEdgeStartSamplePos_x + chromaLcuPos_x);
				edgeStartSampleCr = reconpicture->bufferCr + (reconpicture->originX >> subWidthCMinus1) + (reconpicture->originY >> subHeightCMinus1) * reconpicture->strideCr + (twoSampleEdgeStartSamplePos_y + chromaLcuPos_y) * reconpicture->strideCr + (twoSampleEdgeStartSamplePos_x + chromaLcuPos_x);

				Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartSampleCb,
					edgeStartSampleCr,
					reconpicture->strideCb,
					EB_FALSE,
					cbTc,
					crTc);

			}
		}
	}

	return return_error;
}

/** LCUInternalAreaDLFCore16bit()
is used to apply the deblocking filter in the LCU internal area (excluding the LCU boundary)
as the first pass DLF.
*/
EB_ERRORTYPE LCUInternalAreaDLFCore16bit(
	EbPictureBufferDesc_t *reconpicture,                   //input parameter, the pointer to the reconstructed picture.
	EB_U32                 lcuPos_x,                    //input parameter, the picture-wise horizontal location of the LCU.
	EB_U32                 lcuPos_y,                    //input parameter, the picture-wise vertical location of the LCU.
	EB_U32                 lcuWidth,                    //input parameter, the LCU width.
	EB_U32                 lcuHeight,                   //input parameter, the LCU height.
	EB_U8                 *verticalEdgeBSArray,         //input parameter, the pointer to the vertical edge BS array.
	EB_U8                 *horizontalEdgeBSArray,       //input parameter, the pointer to the hotizontal edge BS array.
	PictureControlSet_t   *reconPictureControlSet)      //input parameter, picture control set.
{
	EB_ERRORTYPE      return_error = EB_ErrorNone;

	const EB_U32 MaxLcuSizeIn4x4blk = MAX_LCU_SIZE >> 2;
	const EB_U32 logMaxLcuSizeIn4x4blk = Log2f(MaxLcuSizeIn4x4blk);

	EB_U32  verticalIdx;
	EB_U32  horizontalIdx;
	EB_U8   bS;

	// variables for luma componet
	EB_U32  blk4x4Addr;
	EB_U32  fourSampleEdgeStartSamplePos_x;
	EB_U32  fourSampleEdgeStartSamplePos_y;
	EB_U32  numVerticalLumaSampleEdges = (lcuWidth >> 3) - 1;
	EB_U32  numHorizontalLumaSampleEdges = (lcuHeight >> 3) - 1;
	EB_U32  num4SampleEdgesPerVerticalLumaSampleEdge = (lcuHeight >> 2) - 2;
	EB_U32  num4SampleEdgesPerHorizontalLumaSampleEdge = (lcuWidth >> 2) - 2;
	EB_S32  Beta;

	// variables for chroma component
	EB_U32  blk2x2Addr;
	EB_U32  twoSampleEdgeStartSamplePos_x;
	EB_U32  twoSampleEdgeStartSamplePos_y;
    EB_COLOR_FORMAT colorFormat = reconpicture->colorFormat;
    const EB_S32 subWidthC = (colorFormat == EB_YUV444) ? 1 : 2;
    const EB_S32 subHeightC = (colorFormat == EB_YUV420) ? 2 : 1;
    const EB_U32 subWidthShfitMinus1 = (colorFormat == EB_YUV444) ? 1 : 0;
    const EB_U32 subHeightShfitMinus1 = (colorFormat == EB_YUV420) ? 0 : 1;
    const EB_S32 subWidthCMinus1 = (colorFormat == EB_YUV444) ? 0 : 1;
    const EB_S32 subHeightCMinus1 = (colorFormat == EB_YUV420) ? 1 : 0;

    EB_U32  chromaLcuPos_x = lcuPos_x >> subWidthCMinus1;
	EB_U32  chromaLcuPos_y = lcuPos_y >> subHeightCMinus1;

    EB_U32  numVerticalChromaSampleEdges   = (lcuWidth >> (3 + subWidthCMinus1)) - (colorFormat==EB_YUV444?1:((lcuWidth & 15) == 0));
    EB_U32  numHorizontalChromaSampleEdges = (lcuHeight >> (3 + subHeightCMinus1)) - (colorFormat==EB_YUV420?((lcuHeight & 15) == 0):1);
    EB_U32  num2SampleEdgesPerVerticalChromaSampleEdge   = (lcuHeight >> (colorFormat==EB_YUV420?2:1)) - 2 - (((lcuHeight & (colorFormat==EB_YUV420?15:7)) == 0) << 1);
    EB_U32  num2SampleEdgesPerHorizontalChromaSampleEdge = (lcuWidth >> (colorFormat==EB_YUV444?1:2)) - 2 - (((lcuWidth & (colorFormat==EB_YUV444?7:15)) == 0) << 1);



	EB_U8  curCuQp;
	EB_U16 *edgeStartFilteredSamplePtr;
	EB_U16 *edgeStartSampleCb;
	EB_U16 *edgeStartSampleCr;
	EB_U8  neighbourCuQp;
	EB_U8  Qp;
	EB_S32  lumaTc;
	//EB_BOOL lumaPCMFlagArray[2];
	//EB_BOOL chromaPCMFlagArray[2];
	EB_S32  CUqpIndex;
	EB_U8  cbQp;
	EB_U8  crQp;
	EB_U8   cbTc;
	EB_U8   crTc;

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(reconPictureControlSet->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

	CHECK_REPORT_ERROR(
		((lcuWidth & 7) == 0 && (lcuHeight & 7) == 0),
		encodeContextPtr->appCallbackPtr,
		EB_ENC_DLF_ERROR10);


	/***** luma component filtering *****/
	// filter all vertical edges
	for (horizontalIdx = 1; horizontalIdx <= numVerticalLumaSampleEdges; ++horizontalIdx) {
		for (verticalIdx = 1; verticalIdx <= num4SampleEdgesPerVerticalLumaSampleEdge; ++verticalIdx) {
			// edge A
			fourSampleEdgeStartSamplePos_x = horizontalIdx << 3;   // LCU-wise position
			fourSampleEdgeStartSamplePos_y = verticalIdx << 2;     // LCU-wise position
			blk4x4Addr = GET_LUMA_4X4BLK_ADDR(
				fourSampleEdgeStartSamplePos_x,
				fourSampleEdgeStartSamplePos_y,
				logMaxLcuSizeIn4x4blk);
			//neighbourBlk4x4Addr = blk4x4Addr - MaxLcuSizeIn4x4blk;

			bS = verticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)];
			if (bS > 0) {
				/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
				*/

				// Qp for the current CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x + lcuPos_x,
					fourSampleEdgeStartSamplePos_y + lcuPos_y,
					reconPictureControlSet->qpArrayStride);


				curCuQp = reconPictureControlSet->qpArray[CUqpIndex];


				// Qp for the neighboring CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x + lcuPos_x - 1,
					fourSampleEdgeStartSamplePos_y + lcuPos_y,
					reconPictureControlSet->qpArrayStride);
				//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
				neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];
				Qp = (curCuQp + neighbourCuQp + 1) >> 1;
				lumaTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bS > 1) << 1) + reconPictureControlSet->tcOffset)];
				Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];

				//CHKN
				lumaTc = lumaTc << 2;
				Beta = Beta << 2;

				edgeStartFilteredSamplePtr = (EB_U16*)reconpicture->bufferY + reconpicture->originX + reconpicture->originY * reconpicture->strideY + (fourSampleEdgeStartSamplePos_y + lcuPos_y) * reconpicture->strideY + fourSampleEdgeStartSamplePos_x + lcuPos_x;

				// 4 sample edge DLF core
				lumaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartFilteredSamplePtr,
					reconpicture->strideY,
					EB_TRUE,
					lumaTc,
					Beta);
			}
		}
	}

	// filter all horizontal edges
	for (verticalIdx = 1; verticalIdx <= numHorizontalLumaSampleEdges; ++verticalIdx) {
		for (horizontalIdx = 1; horizontalIdx <= num4SampleEdgesPerHorizontalLumaSampleEdge; ++horizontalIdx) {
			// edge B
			fourSampleEdgeStartSamplePos_x = horizontalIdx << 2;    // LCU-wise position
			fourSampleEdgeStartSamplePos_y = verticalIdx << 3;      // LCU-wise position
			blk4x4Addr = GET_LUMA_4X4BLK_ADDR(
				fourSampleEdgeStartSamplePos_x,
				fourSampleEdgeStartSamplePos_y,
				logMaxLcuSizeIn4x4blk);
			//neighbourBlk4x4Addr = blk4x4Addr - 1;

			bS = horizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)];
			if (bS > 0) {
				/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
				*/

				// Qp for the current CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x + lcuPos_x,
					fourSampleEdgeStartSamplePos_y + lcuPos_y,
					reconPictureControlSet->qpArrayStride);


				curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

				// Qp for the neighboring CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x + lcuPos_x,
					fourSampleEdgeStartSamplePos_y + lcuPos_y - 1,
					reconPictureControlSet->qpArrayStride);
				// CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
				neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];
				Qp = (curCuQp + neighbourCuQp + 1) >> 1;
				lumaTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bS > 1) << 1) + reconPictureControlSet->tcOffset)];
				Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + reconPictureControlSet->betaOffset)];

				//CHKN
				lumaTc = lumaTc << 2;
				Beta = Beta << 2;
				edgeStartFilteredSamplePtr = (EB_U16*)reconpicture->bufferY + reconpicture->originX + reconpicture->originY * reconpicture->strideY + (fourSampleEdgeStartSamplePos_y + lcuPos_y) * reconpicture->strideY + fourSampleEdgeStartSamplePos_x + lcuPos_x;

				// 4 sample edge DLF core
				lumaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartFilteredSamplePtr,
					reconpicture->strideY,
					EB_FALSE,
					lumaTc,
					Beta);
			}
		}
	}

	/***** chroma component filtering ****/
	// filter all vertical edges
	for (horizontalIdx = 1; horizontalIdx <= numVerticalChromaSampleEdges; ++horizontalIdx) {
		for (verticalIdx = 1; verticalIdx <= num2SampleEdgesPerVerticalChromaSampleEdge; ++verticalIdx) {
			twoSampleEdgeStartSamplePos_x = horizontalIdx << 3;          // LCU-wise position
			twoSampleEdgeStartSamplePos_y = (verticalIdx << 1) + 2;      // LCU-wise position
			blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
				twoSampleEdgeStartSamplePos_x >> subWidthShfitMinus1,
				twoSampleEdgeStartSamplePos_y >> subHeightShfitMinus1,
				logMaxLcuSizeIn4x4blk);
			bS = verticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

			if (bS > 1) {
				// Qp for the current CU
				CUqpIndex =
					CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                    subWidthC,
                    subHeightC,
					twoSampleEdgeStartSamplePos_x + chromaLcuPos_x,
					twoSampleEdgeStartSamplePos_y + chromaLcuPos_y,
					reconPictureControlSet->qpArrayStride);


				curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

				// Qp for the neighboring CU
				CUqpIndex =
					CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                    subWidthC,
                    subHeightC,
					twoSampleEdgeStartSamplePos_x + chromaLcuPos_x - 1,
					twoSampleEdgeStartSamplePos_y + chromaLcuPos_y,
					reconPictureControlSet->qpArrayStride);
				//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
				neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];
				cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
				crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);

				//chromaQp = MapChromaQp(Qp);
				cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, ((EB_S32)cbQp + 2) + reconPictureControlSet->tcOffset)];
				crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, ((EB_S32)crQp + 2) + reconPictureControlSet->tcOffset)];

				//CHKN
				cbTc = cbTc << 2;
				crTc = crTc << 2;
				edgeStartSampleCb = (EB_U16*)reconpicture->bufferCb + (reconpicture->originX >> subWidthCMinus1) + (reconpicture->originY >> subHeightCMinus1) * reconpicture->strideCb + (twoSampleEdgeStartSamplePos_y + chromaLcuPos_y) * reconpicture->strideCb + (twoSampleEdgeStartSamplePos_x + chromaLcuPos_x);
				edgeStartSampleCr = (EB_U16*)reconpicture->bufferCr + (reconpicture->originX >> subWidthCMinus1) + (reconpicture->originY >> subHeightCMinus1) * reconpicture->strideCr + (twoSampleEdgeStartSamplePos_y + chromaLcuPos_y) * reconpicture->strideCr + (twoSampleEdgeStartSamplePos_x + chromaLcuPos_x);


				chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartSampleCb,
					edgeStartSampleCr,
					reconpicture->strideCb,
					EB_TRUE,
					cbTc,
					crTc);
				//chromaPCMFlagArray);
			}
		}
	}

	// filter all horizontal edges
	for (verticalIdx = 1; verticalIdx <= numHorizontalChromaSampleEdges; ++verticalIdx) {
		for (horizontalIdx = 1; horizontalIdx <= num2SampleEdgesPerHorizontalChromaSampleEdge; ++horizontalIdx) {
			twoSampleEdgeStartSamplePos_x = (horizontalIdx << 1) + 2;    // LCU-wise position
			twoSampleEdgeStartSamplePos_y = verticalIdx << 3;            // LCU-wise position
			blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
				twoSampleEdgeStartSamplePos_x >> subWidthShfitMinus1,
				twoSampleEdgeStartSamplePos_y >> subHeightShfitMinus1,
				logMaxLcuSizeIn4x4blk);
			bS = horizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

			if (bS > 1) {
				// Qp for the current CU
				CUqpIndex =
					CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                    subWidthC,
                    subHeightC,
					twoSampleEdgeStartSamplePos_x + chromaLcuPos_x,
					twoSampleEdgeStartSamplePos_y + chromaLcuPos_y,
					reconPictureControlSet->qpArrayStride);


				curCuQp = reconPictureControlSet->qpArray[CUqpIndex];

				// Qp for the neighboring CU
				CUqpIndex =
					CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                    subWidthC,
                    subHeightC,
					twoSampleEdgeStartSamplePos_x + chromaLcuPos_x,
					twoSampleEdgeStartSamplePos_y + chromaLcuPos_y - 1,
					reconPictureControlSet->qpArrayStride);
				//CUqpIndex     = ((CUqpIndex - (EB_S32)reconPictureControlSet->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - reconPictureControlSet->qpArrayStride;
				neighbourCuQp = reconPictureControlSet->qpArray[CUqpIndex];

				cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->cbQpOffset), colorFormat>=EB_YUV422);
				crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + reconPictureControlSet->crQpOffset), colorFormat>=EB_YUV422);
				//chromaQp = MapChromaQp(Qp);
				cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + reconPictureControlSet->tcOffset)];
				crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + reconPictureControlSet->tcOffset)];

				//CHKN
				cbTc = cbTc << 2;
				crTc = crTc << 2;
				edgeStartSampleCb = (EB_U16*)reconpicture->bufferCb + (reconpicture->originX >> subWidthCMinus1) + (reconpicture->originY >> subHeightCMinus1) * reconpicture->strideCb + (twoSampleEdgeStartSamplePos_y + chromaLcuPos_y) * reconpicture->strideCb + (twoSampleEdgeStartSamplePos_x + chromaLcuPos_x);
				edgeStartSampleCr = (EB_U16*)reconpicture->bufferCr + (reconpicture->originX >> subWidthCMinus1) + (reconpicture->originY >> subHeightCMinus1) * reconpicture->strideCr + (twoSampleEdgeStartSamplePos_y + chromaLcuPos_y) * reconpicture->strideCr + (twoSampleEdgeStartSamplePos_x + chromaLcuPos_x);

				chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartSampleCb,
					edgeStartSampleCr,
					reconpicture->strideCb,
					EB_FALSE,
					cbTc,
					crTc);
				//chromaPCMFlagArray);
			}
		}
	}

	return return_error;
}


/** LCUBoundaryDLFCore()
is used to apply the deblocking filter on the LCU boundary
*/
void LCUBoundaryDLFCore(
	EbPictureBufferDesc_t *reconpicture,                       //input parameter, the pointer to the reconstructed picture.
	EB_U32                 lcuPos_x,                        //input parameter, the picture-wise horizontal location of the LCU.
	EB_U32                 lcuPos_y,                        //input parameter, the picture-wise vertical location of the LCU.
	EB_U32                 lcuWidth,                        //input parameter, the LCU width.
	EB_U32                 lcuHeight,                       //input parameter, the LCU height.
	EB_U8                 *lcuVerticalEdgeBSArray,          //input parameter, the pointer to the vertical edge BS array.
	EB_U8                 *lcuHorizontalEdgeBSArray,        //input parameter, the pointer to the horizontal edge BS array.
	EB_U8                 *topLcuVerticalEdgeBSArray,       //input parameter, the pointer to the vertical edge BS array of the top neighbour LCU.
	EB_U8                 *leftLcuHorizontalEdgeBSArray,    //input parameter, the pointer to the horizontal edge BS array of the left neighbour LCU.
	PictureControlSet_t   *pictureControlSetPtr)            //input parameter, picture control set.
{
	// luma variable
	EB_U32  edgeABlk4x4Addr;
	EB_U32  edgeBBlk4x4Addr;
	EB_U32  edgeCBlk4x4Addr;
	EB_U32  edgeDBlk4x4Addr;
	EB_U8   bSLumaEdgeA;
	EB_U8   bSLumaEdgeB;
	EB_U8   bSLumaEdgeC;
	EB_U8   bSLumaEdgeD;
	EB_U32  num8x8LumaBlkInTop8x8LumablkRowMinus1 = (lcuWidth >> 3) - 1;
	EB_U32  num8x8LumaBlkInLeft8x8LumablkColumnMinus1 = (lcuHeight >> 3) - 1;

	// chroma variable
	EB_U32  edgeAUpperBlk2x2Addr;
	EB_U32  edgeALowerBlk2x2Addr;
	EB_U32  edgeBLeftBlk2x2Addr;
	EB_U32  edgeBRightBlk2x2Addr;
	EB_U32  edgeCUpperBlk2x2Addr;
	EB_U32  edgeCLowerBlk2x2Addr;
	EB_U32  edgeDLeftBlk2x2Addr;
	EB_U32  edgeDRightBlk2x2Addr;
	EB_U8   bSChromaEdgeA[2];
	EB_U8   bSChromaEdgeB[2];
	EB_U8   bSChromaEdgeC[2];
	EB_U8   bSChromaEdgeD[2];
    EB_COLOR_FORMAT colorFormat = reconpicture->colorFormat;    
    EB_U32  chromaLcuPos_x      = lcuPos_x >> (colorFormat==EB_YUV444?0:1);
    EB_U32  chromaLcuPos_y      = lcuPos_y >> (colorFormat==EB_YUV420?1:0);
	EB_U32  num8x8ChromaBlkInTop8x8ChromablkRowMinus1     = (lcuWidth  >> (colorFormat==EB_YUV444?3:4)) - (colorFormat==EB_YUV444?1:((lcuWidth  & 15) == 0));
	EB_U32  num8x8ChromaBlkInLeft8x8ChromablkColumnMinus1 = (lcuHeight >> (colorFormat==EB_YUV420?4:3)) - (colorFormat==EB_YUV420?((lcuHeight & 15) == 0):1);
	EB_U32  horizontalIdx;
	EB_U32  verticalIdx;

	const EB_U32 logMaxLcuSizeIn4x4blk = Log2f(MAX_LCU_SIZE >> 2);

	SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	EB_U32                lcuSize = sequenceControlSetPtr->lcuSize;
	EB_U32       chromaLcuSizeX = lcuSize >> (colorFormat==EB_YUV444?0:1);
	EB_U32       chromaLcuSizeY = lcuSize >> (colorFormat==EB_YUV420?1:0);
    const EB_U32 subWidthShfitMinus1  = colorFormat==EB_YUV444?1:0;
    const EB_U32 subHeightShfitMinus1 = colorFormat==EB_YUV420?0:1;

	EncodeContext_t        *encodeContextPtr = ((SequenceControlSet_t*)(pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr))->encodeContextPtr;

	CHECK_REPORT_ERROR(
		((lcuWidth & 7) == 0 && (lcuHeight & 7) == 0),
		encodeContextPtr->appCallbackPtr,
		EB_ENC_DLF_ERROR8);

	/***** luma component filtering *****/
	// filter the top-left corner 8x8 luma block
	edgeABlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
		0,
		lcuSize - 4,
		logMaxLcuSizeIn4x4blk);

	edgeBBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
		lcuSize - 4,
		0,
		logMaxLcuSizeIn4x4blk);

	edgeCBlk4x4Addr = edgeDBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
		0,
		0,
		logMaxLcuSizeIn4x4blk);

	bSLumaEdgeA = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
		topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeABlk4x4Addr)];
	bSLumaEdgeB = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
		leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBBlk4x4Addr)];
	bSLumaEdgeC = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCBlk4x4Addr)];
	bSLumaEdgeD = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDBlk4x4Addr)];

	// luma 8x8 block DLF core
	Luma8x8blkDLFCore(
		reconpicture,
		reconpicture->strideY,
		lcuPos_x,
		lcuPos_y,
		bSLumaEdgeA,
		bSLumaEdgeB,
		bSLumaEdgeC,
		bSLumaEdgeD,
		pictureControlSetPtr);

	// filter the top 8x8 luma block row
	for (horizontalIdx = 1; horizontalIdx <= num8x8LumaBlkInTop8x8LumablkRowMinus1; ++horizontalIdx) {
		edgeABlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			horizontalIdx << 3,
			lcuSize - 4,
			logMaxLcuSizeIn4x4blk);

		edgeBBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			(horizontalIdx << 3) - 4,
			0,
			logMaxLcuSizeIn4x4blk);

		edgeCBlk4x4Addr = edgeDBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			horizontalIdx << 3,
			0,
			logMaxLcuSizeIn4x4blk);

		bSLumaEdgeA = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
			topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeABlk4x4Addr)];
		bSLumaEdgeB = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBBlk4x4Addr)];
		bSLumaEdgeC = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCBlk4x4Addr)];
		bSLumaEdgeD = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDBlk4x4Addr)];

		// luma 8x8 block DLF core
		Luma8x8blkDLFCore(
			reconpicture,
			reconpicture->strideY,
			lcuPos_x + (horizontalIdx << 3),
			lcuPos_y,
			bSLumaEdgeA,
			bSLumaEdgeB,
			bSLumaEdgeC,
			bSLumaEdgeD,
			pictureControlSetPtr);
	}

	// filter the left 8x8 luma block column
	for (verticalIdx = 1; verticalIdx <= num8x8LumaBlkInLeft8x8LumablkColumnMinus1; ++verticalIdx) {
		edgeABlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			0,
			(verticalIdx << 3) - 4,
			logMaxLcuSizeIn4x4blk);

		edgeBBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			lcuSize - 4,
			verticalIdx << 3,
			logMaxLcuSizeIn4x4blk);

		edgeCBlk4x4Addr = edgeDBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			0,
			verticalIdx << 3,
			logMaxLcuSizeIn4x4blk);

		bSLumaEdgeA = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeABlk4x4Addr)];
		bSLumaEdgeB = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
			leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBBlk4x4Addr)];
		bSLumaEdgeC = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCBlk4x4Addr)];
		bSLumaEdgeD = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDBlk4x4Addr)];

		// luma 8x8 block DLF core
		Luma8x8blkDLFCore(
			reconpicture,
			reconpicture->strideY,
			lcuPos_x,
			lcuPos_y + (verticalIdx << 3),
			bSLumaEdgeA,
			bSLumaEdgeB,
			bSLumaEdgeC,
			bSLumaEdgeD,
			pictureControlSetPtr);
	}

	/***** chroma components filtering *****/
	// filter the top-left corner 8x8 chroma block
	edgeAUpperBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		0,
		(chromaLcuSizeY - 4) >> subHeightShfitMinus1,
		logMaxLcuSizeIn4x4blk);
	edgeALowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		0,
		(chromaLcuSizeY - 2) >> subHeightShfitMinus1,
		logMaxLcuSizeIn4x4blk);

	edgeBLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		(chromaLcuSizeX - 4) >> subWidthShfitMinus1,
		0,
		logMaxLcuSizeIn4x4blk);
	edgeBRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		(chromaLcuSizeX - 2) >> subWidthShfitMinus1,
		0,
		logMaxLcuSizeIn4x4blk);

	edgeCUpperBlk2x2Addr = edgeDLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		0,
		0,
		logMaxLcuSizeIn4x4blk);

	edgeCLowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		0,
		2 >> subHeightShfitMinus1,
		logMaxLcuSizeIn4x4blk);

	edgeDRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		2 >> subWidthShfitMinus1,
		0,
		logMaxLcuSizeIn4x4blk);

	bSChromaEdgeA[0] = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
		topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeAUpperBlk2x2Addr)];
	bSChromaEdgeA[1] = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
		topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeALowerBlk2x2Addr)];
	bSChromaEdgeB[0] = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
		leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBLeftBlk2x2Addr)];
	bSChromaEdgeB[1] = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
		leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBRightBlk2x2Addr)];
	bSChromaEdgeC[0] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCUpperBlk2x2Addr)];
	bSChromaEdgeC[1] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCLowerBlk2x2Addr)];
	bSChromaEdgeD[0] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDLeftBlk2x2Addr)];
	bSChromaEdgeD[1] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDRightBlk2x2Addr)];

	chroma8x8blkDLFCore(
		reconpicture,
		reconpicture->strideCb,
		chromaLcuPos_x,
		chromaLcuPos_y,
		bSChromaEdgeA,
		bSChromaEdgeB,
		bSChromaEdgeC,
		bSChromaEdgeD,
		pictureControlSetPtr);

	// filter the top 8x8 chroma block row
	for (horizontalIdx = 1; horizontalIdx <= num8x8ChromaBlkInTop8x8ChromablkRowMinus1; ++horizontalIdx) {
		edgeAUpperBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			horizontalIdx << (3 - subWidthShfitMinus1),
			(chromaLcuSizeY - 4) >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);
		edgeALowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			horizontalIdx << (3 - subWidthShfitMinus1),
			(chromaLcuSizeY - 2) >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);

		edgeBLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			((horizontalIdx << 3) - 4) >> subWidthShfitMinus1,
			0,
			logMaxLcuSizeIn4x4blk);
		edgeBRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			((horizontalIdx << 3) - 2) >> subWidthShfitMinus1,
			0,
			logMaxLcuSizeIn4x4blk);

		edgeCUpperBlk2x2Addr = edgeDLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			horizontalIdx << (3 - subWidthShfitMinus1),
			0,
			logMaxLcuSizeIn4x4blk);

		edgeCLowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			horizontalIdx << (3 - subWidthShfitMinus1),
			2 >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);

		edgeDRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			((horizontalIdx << 3) + 2) >> subWidthShfitMinus1,
			0,
			logMaxLcuSizeIn4x4blk);

		bSChromaEdgeA[0] = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
			topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeAUpperBlk2x2Addr)];
		bSChromaEdgeA[1] = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
			topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeALowerBlk2x2Addr)];
		bSChromaEdgeB[0] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBLeftBlk2x2Addr)];
		bSChromaEdgeB[1] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBRightBlk2x2Addr)];
		bSChromaEdgeC[0] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCUpperBlk2x2Addr)];
		bSChromaEdgeC[1] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCLowerBlk2x2Addr)];
		bSChromaEdgeD[0] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDLeftBlk2x2Addr)];
		bSChromaEdgeD[1] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDRightBlk2x2Addr)];

		chroma8x8blkDLFCore(
			reconpicture,
			reconpicture->strideCb,
			chromaLcuPos_x + (horizontalIdx << 3),
			chromaLcuPos_y,
			bSChromaEdgeA,
			bSChromaEdgeB,
			bSChromaEdgeC,
			bSChromaEdgeD,
			pictureControlSetPtr);
	}

	// filter the left 8x8 chroma block column
	for (verticalIdx = 1; verticalIdx <= num8x8ChromaBlkInLeft8x8ChromablkColumnMinus1; ++verticalIdx) {
		edgeAUpperBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			0,
			((verticalIdx << 3) - 4) >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);
		edgeALowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			0,
			((verticalIdx << 3) - 2) >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);

		edgeBLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			(chromaLcuSizeX - 4) >> subWidthShfitMinus1,
			verticalIdx << (3 - subHeightShfitMinus1),
			logMaxLcuSizeIn4x4blk);
		edgeBRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			(chromaLcuSizeX - 2) >> subWidthShfitMinus1,
			verticalIdx << (3 - subHeightShfitMinus1),
			logMaxLcuSizeIn4x4blk);

		edgeCUpperBlk2x2Addr = edgeDLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			0,
			verticalIdx << (3 - subHeightShfitMinus1),
			logMaxLcuSizeIn4x4blk);

		edgeCLowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			0,
			((verticalIdx << 3) + 2) >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);

		edgeDRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			2 >> subWidthShfitMinus1,
			verticalIdx << (3 - subHeightShfitMinus1),
			logMaxLcuSizeIn4x4blk);

		bSChromaEdgeA[0] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeAUpperBlk2x2Addr)];
		bSChromaEdgeA[1] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeALowerBlk2x2Addr)];
		bSChromaEdgeB[0] = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
			leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBLeftBlk2x2Addr)];
		bSChromaEdgeB[1] = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
			leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBRightBlk2x2Addr)];
		bSChromaEdgeC[0] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCUpperBlk2x2Addr)];
		bSChromaEdgeC[1] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCLowerBlk2x2Addr)];
		bSChromaEdgeD[0] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDLeftBlk2x2Addr)];
		bSChromaEdgeD[1] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDRightBlk2x2Addr)];

		chroma8x8blkDLFCore(
			reconpicture,
			reconpicture->strideCb,
			chromaLcuPos_x,
			chromaLcuPos_y + (verticalIdx << 3),
			bSChromaEdgeA,
			bSChromaEdgeB,
			bSChromaEdgeC,
			bSChromaEdgeD,
			pictureControlSetPtr);
	}

	return;
}
void LCUBoundaryDLFCore16bit(
	EbPictureBufferDesc_t *reconpicture,                       //input parameter, the pointer to the reconstructed picture.
	EB_U32                 lcuPos_x,                        //input parameter, the picture-wise horizontal location of the LCU.
	EB_U32                 lcuPos_y,                        //input parameter, the picture-wise vertical location of the LCU.
	EB_U32                 lcuWidth,                        //input parameter, the LCU width.
	EB_U32                 lcuHeight,                       //input parameter, the LCU height.
	EB_U8                 *lcuVerticalEdgeBSArray,          //input parameter, the pointer to the vertical edge BS array.
	EB_U8                 *lcuHorizontalEdgeBSArray,        //input parameter, the pointer to the horizontal edge BS array.
	EB_U8                 *topLcuVerticalEdgeBSArray,       //input parameter, the pointer to the vertical edge BS array of the top neighbour LCU.
	EB_U8                 *leftLcuHorizontalEdgeBSArray,    //input parameter, the pointer to the horizontal edge BS array of the left neighbour LCU.
	PictureControlSet_t   *pictureControlSetPtr)            //input parameter, picture control set.
{
	// luma variable
	EB_U32  edgeABlk4x4Addr;
	EB_U32  edgeBBlk4x4Addr;
	EB_U32  edgeCBlk4x4Addr;
	EB_U32  edgeDBlk4x4Addr;
	EB_U8   bSLumaEdgeA;
	EB_U8   bSLumaEdgeB;
	EB_U8   bSLumaEdgeC;
	EB_U8   bSLumaEdgeD;
	EB_U32  num8x8LumaBlkInTop8x8LumablkRowMinus1 = (lcuWidth >> 3) - 1;
	EB_U32  num8x8LumaBlkInLeft8x8LumablkColumnMinus1 = (lcuHeight >> 3) - 1;

	// chroma variable
	EB_U32  edgeAUpperBlk2x2Addr;
	EB_U32  edgeALowerBlk2x2Addr;
	EB_U32  edgeBLeftBlk2x2Addr;
	EB_U32  edgeBRightBlk2x2Addr;
	EB_U32  edgeCUpperBlk2x2Addr;
	EB_U32  edgeCLowerBlk2x2Addr;
	EB_U32  edgeDLeftBlk2x2Addr;
	EB_U32  edgeDRightBlk2x2Addr;
	EB_U8   bSChromaEdgeA[2];
	EB_U8   bSChromaEdgeB[2];
	EB_U8   bSChromaEdgeC[2];
	EB_U8   bSChromaEdgeD[2];

    EB_COLOR_FORMAT colorFormat = reconpicture->colorFormat;    
    EB_U32  chromaLcuPos_x      = lcuPos_x >> (colorFormat==EB_YUV444?0:1);
    EB_U32  chromaLcuPos_y      = lcuPos_y >> (colorFormat==EB_YUV420?1:0);
	EB_U32  num8x8ChromaBlkInTop8x8ChromablkRowMinus1     = (lcuWidth  >> (colorFormat==EB_YUV444?3:4)) - (colorFormat==EB_YUV444?1:((lcuWidth  & 15) == 0));
	EB_U32  num8x8ChromaBlkInLeft8x8ChromablkColumnMinus1 = (lcuHeight >> (colorFormat==EB_YUV420?4:3)) - (colorFormat==EB_YUV420?((lcuHeight & 15) == 0):1);

	EB_U32  horizontalIdx;
	EB_U32  verticalIdx;
	const EB_U32 logMaxLcuSizeIn4x4blk = Log2f(MAX_LCU_SIZE >> 2);

	SequenceControlSet_t *sequenceControlSetPtr = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	EB_U32 lcuSize = sequenceControlSetPtr->lcuSize;
    EB_U32 chromaLcuSizeX = lcuSize >> (colorFormat==EB_YUV444?0:1);
    EB_U32 chromaLcuSizeY = lcuSize >> (colorFormat==EB_YUV420?1:0);
    const EB_U32 subWidthShfitMinus1 = colorFormat==EB_YUV444?1:0;
    const EB_U32 subHeightShfitMinus1 = colorFormat==EB_YUV420?0:1;

	/***** luma component filtering *****/
	// filter the top-left corner 8x8 luma block
	edgeABlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
		0,
		lcuSize - 4,
		logMaxLcuSizeIn4x4blk);

	edgeBBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
		lcuSize - 4,
		0,
		logMaxLcuSizeIn4x4blk);

	edgeCBlk4x4Addr = edgeDBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
		0,
		0,
		logMaxLcuSizeIn4x4blk);

	bSLumaEdgeA = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
		topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeABlk4x4Addr)];
	bSLumaEdgeB = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
		leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBBlk4x4Addr)];
	bSLumaEdgeC = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCBlk4x4Addr)];
	bSLumaEdgeD = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDBlk4x4Addr)];

	// luma 8x8 block DLF core
	Luma8x8blkDLFCore16bit(
		reconpicture,
		reconpicture->strideY,
		lcuPos_x,
		lcuPos_y,
		bSLumaEdgeA,
		bSLumaEdgeB,
		bSLumaEdgeC,
		bSLumaEdgeD,
		pictureControlSetPtr);

	// filter the top 8x8 luma block row
	for (horizontalIdx = 1; horizontalIdx <= num8x8LumaBlkInTop8x8LumablkRowMinus1; ++horizontalIdx) {
		edgeABlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			horizontalIdx << 3,
			lcuSize - 4,
			logMaxLcuSizeIn4x4blk);

		edgeBBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			(horizontalIdx << 3) - 4,
			0,
			logMaxLcuSizeIn4x4blk);

		edgeCBlk4x4Addr = edgeDBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			horizontalIdx << 3,
			0,
			logMaxLcuSizeIn4x4blk);

		bSLumaEdgeA = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
			topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeABlk4x4Addr)];
		bSLumaEdgeB = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBBlk4x4Addr)];
		bSLumaEdgeC = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCBlk4x4Addr)];
		bSLumaEdgeD = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDBlk4x4Addr)];

		// luma 8x8 block DLF core
		Luma8x8blkDLFCore16bit(
			reconpicture,
			reconpicture->strideY,
			lcuPos_x + (horizontalIdx << 3),
			lcuPos_y,
			bSLumaEdgeA,
			bSLumaEdgeB,
			bSLumaEdgeC,
			bSLumaEdgeD,
			pictureControlSetPtr);
	}

	// filter the left 8x8 luma block column
	for (verticalIdx = 1; verticalIdx <= num8x8LumaBlkInLeft8x8LumablkColumnMinus1; ++verticalIdx) {
		edgeABlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			0,
			(verticalIdx << 3) - 4,
			logMaxLcuSizeIn4x4blk);

		edgeBBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			lcuSize - 4,
			verticalIdx << 3,
			logMaxLcuSizeIn4x4blk);

		edgeCBlk4x4Addr = edgeDBlk4x4Addr = GET_LUMA_4X4BLK_ADDR(
			0,
			verticalIdx << 3,
			logMaxLcuSizeIn4x4blk);

		bSLumaEdgeA = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeABlk4x4Addr)];
		bSLumaEdgeB = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
			leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBBlk4x4Addr)];
		bSLumaEdgeC = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCBlk4x4Addr)];
		bSLumaEdgeD = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDBlk4x4Addr)];

		// luma 8x8 block DLF core
		Luma8x8blkDLFCore16bit(
			reconpicture,
			reconpicture->strideY,
			lcuPos_x,
			lcuPos_y + (verticalIdx << 3),
			bSLumaEdgeA,
			bSLumaEdgeB,
			bSLumaEdgeC,
			bSLumaEdgeD,
			pictureControlSetPtr);
	}

	/***** chroma components filtering *****/
	// filter the top-left corner 8x8 chroma block
	edgeAUpperBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		0,
        (chromaLcuSizeY - 4) >> subHeightShfitMinus1,
		logMaxLcuSizeIn4x4blk);
	edgeALowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		0,
        (chromaLcuSizeY - 2) >> subHeightShfitMinus1,
		logMaxLcuSizeIn4x4blk);

	edgeBLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
        (chromaLcuSizeX - 4) >> subWidthShfitMinus1,
		0,
		logMaxLcuSizeIn4x4blk);
	edgeBRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
        (chromaLcuSizeX - 2) >> subWidthShfitMinus1,
		0,
		logMaxLcuSizeIn4x4blk);

	edgeCUpperBlk2x2Addr = edgeDLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		0,
		0,
		logMaxLcuSizeIn4x4blk);

	edgeCLowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
		0,
        2 >> subHeightShfitMinus1,
		logMaxLcuSizeIn4x4blk);

	edgeDRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
        2 >> subWidthShfitMinus1,
		0,
		logMaxLcuSizeIn4x4blk);

	bSChromaEdgeA[0] = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
		topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeAUpperBlk2x2Addr)];
	bSChromaEdgeA[1] = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
		topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeALowerBlk2x2Addr)];
	bSChromaEdgeB[0] = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
		leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBLeftBlk2x2Addr)];
	bSChromaEdgeB[1] = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
		leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBRightBlk2x2Addr)];
	bSChromaEdgeC[0] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCUpperBlk2x2Addr)];
	bSChromaEdgeC[1] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCLowerBlk2x2Addr)];
	bSChromaEdgeD[0] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDLeftBlk2x2Addr)];
	bSChromaEdgeD[1] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDRightBlk2x2Addr)];

	chroma8x8blkDLFCore16bit(
		reconpicture,
		reconpicture->strideCb,
		chromaLcuPos_x,
		chromaLcuPos_y,
		bSChromaEdgeA,
		bSChromaEdgeB,
		bSChromaEdgeC,
		bSChromaEdgeD,
		pictureControlSetPtr);

	// filter the top 8x8 chroma block row
	for (horizontalIdx = 1; horizontalIdx <= num8x8ChromaBlkInTop8x8ChromablkRowMinus1; ++horizontalIdx) {
		edgeAUpperBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
            horizontalIdx << (3 - subWidthShfitMinus1),
            (chromaLcuSizeY - 4) >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);
		edgeALowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
            horizontalIdx << (3 - subWidthShfitMinus1),
            (chromaLcuSizeY - 2) >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);

		edgeBLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
            ((horizontalIdx << 3) - 4) >> subWidthShfitMinus1,
			0,
			logMaxLcuSizeIn4x4blk);
		edgeBRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
            ((horizontalIdx << 3) - 2) >> subWidthShfitMinus1,
			0,
			logMaxLcuSizeIn4x4blk);

		edgeCUpperBlk2x2Addr = edgeDLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
            horizontalIdx << (3 - subWidthShfitMinus1),
			0,
			logMaxLcuSizeIn4x4blk);

		edgeCLowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
            horizontalIdx << (3 - subWidthShfitMinus1),
            2 >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);

		edgeDRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
            ((horizontalIdx << 3) + 2) >> subWidthShfitMinus1,
			0,
			logMaxLcuSizeIn4x4blk);

		bSChromaEdgeA[0] = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
			topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeAUpperBlk2x2Addr)];
		bSChromaEdgeA[1] = topLcuVerticalEdgeBSArray == EB_NULL ? 0 :
			topLcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeALowerBlk2x2Addr)];
		bSChromaEdgeB[0] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBLeftBlk2x2Addr)];
		bSChromaEdgeB[1] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBRightBlk2x2Addr)];
		bSChromaEdgeC[0] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCUpperBlk2x2Addr)];
		bSChromaEdgeC[1] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCLowerBlk2x2Addr)];
		bSChromaEdgeD[0] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDLeftBlk2x2Addr)];
		bSChromaEdgeD[1] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDRightBlk2x2Addr)];

		chroma8x8blkDLFCore16bit(
			reconpicture,
			reconpicture->strideCb,
			chromaLcuPos_x + (horizontalIdx << 3),
			chromaLcuPos_y,
			bSChromaEdgeA,
			bSChromaEdgeB,
			bSChromaEdgeC,
			bSChromaEdgeD,
			pictureControlSetPtr);
	}

	// filter the left 8x8 chroma block column
	for (verticalIdx = 1; verticalIdx <= num8x8ChromaBlkInLeft8x8ChromablkColumnMinus1; ++verticalIdx) {
		edgeAUpperBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			0,
            ((verticalIdx << 3) - 4) >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);
		edgeALowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			0,
            ((verticalIdx << 3) - 2) >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);

		edgeBLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
            (chromaLcuSizeX - 4) >> subWidthShfitMinus1,
            verticalIdx << (3 - subHeightShfitMinus1),
			logMaxLcuSizeIn4x4blk);
		edgeBRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
            (chromaLcuSizeX - 2) >> subWidthShfitMinus1,
            verticalIdx << (3 - subHeightShfitMinus1),
			logMaxLcuSizeIn4x4blk);

		edgeCUpperBlk2x2Addr = edgeDLeftBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			0,
            verticalIdx << (3 - subHeightShfitMinus1),
			logMaxLcuSizeIn4x4blk);

		edgeCLowerBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
			0,
            ((verticalIdx << 3) + 2) >> subHeightShfitMinus1,
			logMaxLcuSizeIn4x4blk);

		edgeDRightBlk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
            2 >> subWidthShfitMinus1,
            verticalIdx << (3 - subHeightShfitMinus1),
			logMaxLcuSizeIn4x4blk);

		bSChromaEdgeA[0] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeAUpperBlk2x2Addr)];
		bSChromaEdgeA[1] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeALowerBlk2x2Addr)];
		bSChromaEdgeB[0] = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
			leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBLeftBlk2x2Addr)];
		bSChromaEdgeB[1] = leftLcuHorizontalEdgeBSArray == EB_NULL ? 0 :
			leftLcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeBRightBlk2x2Addr)];
		bSChromaEdgeC[0] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCUpperBlk2x2Addr)];
		bSChromaEdgeC[1] = lcuVerticalEdgeBSArray[BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(edgeCLowerBlk2x2Addr)];
		bSChromaEdgeD[0] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDLeftBlk2x2Addr)];
		bSChromaEdgeD[1] = lcuHorizontalEdgeBSArray[BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(edgeDRightBlk2x2Addr)];

		chroma8x8blkDLFCore16bit(
			reconpicture,
			reconpicture->strideCb,
			chromaLcuPos_x,
			chromaLcuPos_y + (verticalIdx << 3),
			bSChromaEdgeA,
			bSChromaEdgeB,
			bSChromaEdgeC,
			bSChromaEdgeD,
			pictureControlSetPtr);
	}

	return;
}

/** LCUPictureEdgeDLFCore()
is used to apply the deblocking filter on the LCU boundary
*/
void LCUPictureEdgeDLFCore(
	EbPictureBufferDesc_t *reconPic,                       //input parameter, the pointer to the reconstructed picture.
	EB_U32                 lcuIdx,
	EB_U32                 lcuPos_x,                        //input parameter, the picture-wise horizontal location of the LCU.
	EB_U32                 lcuPos_y,                        //input parameter, the picture-wise vertical location of the LCU.
	EB_U32                 lcuWidth,                        //input parameter, the LCU width.
	EB_U32                 lcuHeight,                       //input parameter, the LCU height.
	//    EB_U8                 *lcuVerticalEdgeBSArray,          //input parameter, the pointer to the vertical edge BS array.
	//    EB_U8                 *lcuHorizontalEdgeBSArray,        //input parameter, the pointer to the horizontal edge BS array.
	//    EB_U8                 *topLcuVerticalEdgeBSArray,       //input parameter, the pointer to the vertical edge BS array of the top neighbour LCU.
	//    EB_U8                 *leftLcuHorizontalEdgeBSArray,    //input parameter, the pointer to the horizontal edge BS array of the left neighbour LCU.
	PictureControlSet_t   *pictureControlSetPtr)            //input parameter, picture control set.
{
	const EB_U32 MaxLcuSizeIn4x4blk = MAX_LCU_SIZE >> 2;
	const EB_U32 logMaxLcuSizeIn4x4blk = Log2f(MaxLcuSizeIn4x4blk);
	const EB_U32 logMaxLcuSize = Log2f(MAX_LCU_SIZE);


	EB_U32  verticalIdx;
	EB_U32  horizontalIdx;
	EB_U32  num4SampleHorizontalEdges;
	EB_U32  num4SampleVerticalEdges;
	EB_U32  blk4x4Addr;
	EB_U32  blk2x2Addr;
	EB_U32  pictureWidthInLcu;
	EB_U32  pictureHeightInLcu;
	EB_U32  fourSampleEdgeStartSamplePos_x;
	EB_U32  fourSampleEdgeStartSamplePos_y;
	EB_U8   bS;
	EB_U32  lcuSize;
	EB_U32  chromaLcuSizeX;
	EB_U32  chromaLcuSizeY;
	EB_U8  curCuQp;
	EB_BYTE edgeStartFilteredSamplePtr;
	EB_BYTE edgeStartSampleCb;
	EB_BYTE edgeStartSampleCr;
	EB_U8  neighbourCuQp;
	EB_U8  Qp;
	EB_S32  Beta;
	EB_S32  lumaTc;
	//EB_BOOL lumaPCMFlagArray[2];
	//EB_BOOL chromaPCMFlagArray[2];
	EB_S32  CUqpIndex;
	EB_U8  cbQp;
	EB_U8  crQp;
	EB_U8   cbTc;
	EB_U8   crTc;
    EB_COLOR_FORMAT colorFormat   = reconPic->colorFormat;
    const EB_S32 subWidthC        = colorFormat==EB_YUV444?1:2;
    const EB_S32 subHeightC       = colorFormat==EB_YUV420?2:1;
    const EB_S32 subWidthCMinus1  = colorFormat==EB_YUV444?0:1;
    const EB_S32 subHeightCMinus1 = colorFormat==EB_YUV420?1:0;

	SequenceControlSet_t  *sequenceControlSet = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	LargestCodingUnit_t   *lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIdx];

	pictureWidthInLcu = (sequenceControlSet->lumaWidth + sequenceControlSet->lcuSize - 1) / sequenceControlSet->lcuSize;
	pictureHeightInLcu = (sequenceControlSet->lumaHeight + sequenceControlSet->lcuSize - 1) / sequenceControlSet->lcuSize;
	lcuSize = sequenceControlSet->lcuSize;
	chromaLcuSizeX = lcuSize >> (colorFormat==EB_YUV444?0:1);
	chromaLcuSizeY = lcuSize >> (colorFormat==EB_YUV420?1:0);
    const EB_U32 subWidthShfitMinus1  = colorFormat==EB_YUV444?1:0;
    const EB_U32 subHeightShfitMinus1 = colorFormat==EB_YUV420?0:1;
	if (lcuPos_x >> lcuPtr->sizeLog2 == pictureWidthInLcu - 1) {
		/***** picture right-most 4 sample horizontal edges filtering *****/
		// luma component filtering
		//num4SampleHorizontalEdges     = (sequenceControlSet->lumaHeight >> 3) - 1;
		num4SampleHorizontalEdges = (lcuHeight >> 3);
		fourSampleEdgeStartSamplePos_x = sequenceControlSet->lumaWidth - 4;          // picutre-wise position
		//for(verticalIdx = 1; verticalIdx <= num4SampleHorizontalEdges; ++verticalIdx) {
		for (verticalIdx = (lcuPos_y == 0); verticalIdx < num4SampleHorizontalEdges; ++verticalIdx) {
			// edge B
			//fourSampleEdgeStartSamplePos_y = verticalIdx << 3;                       // picture-wise position
			fourSampleEdgeStartSamplePos_y = lcuPos_y + (verticalIdx << 3);                       // picture-wise position
			//lcuIdx     = (fourSampleEdgeStartSamplePos_y >> logMaxLcuSize) * pictureWidthInLcu + (fourSampleEdgeStartSamplePos_x >> logMaxLcuSize);
			//lcuPtr     = pictureControlSetPtr->lcuPtrArray[lcuIdx];
			blk4x4Addr = GET_LUMA_4X4BLK_ADDR(
				fourSampleEdgeStartSamplePos_x & (lcuSize - 1),
				fourSampleEdgeStartSamplePos_y & (lcuSize - 1),
				logMaxLcuSizeIn4x4blk);
			//neighbourBlk4x4Addr = blk4x4Addr - 1;

			bS = pictureControlSetPtr->horizontalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)];
			if (bS > 0) {
				/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
				*/

				// Qp for the current CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x,
					fourSampleEdgeStartSamplePos_y,
					pictureControlSetPtr->qpArrayStride);


				curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

				// Qp for the neighboring CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x,
					fourSampleEdgeStartSamplePos_y - 1,
					pictureControlSetPtr->qpArrayStride);
				//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
				neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];


				Qp = (curCuQp + neighbourCuQp + 1) >> 1;
				lumaTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bS > 1) << 1) + pictureControlSetPtr->tcOffset)];
				Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + pictureControlSetPtr->betaOffset)];
				edgeStartFilteredSamplePtr = reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + fourSampleEdgeStartSamplePos_y * reconPic->strideY + fourSampleEdgeStartSamplePos_x;

				// 4 sample luma edge filter core
				Luma4SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartFilteredSamplePtr,
					reconPic->strideY,
					EB_FALSE,
					lumaTc,
					Beta);
			}
		}

		// chroma component filtering
		if ((sequenceControlSet->chromaWidth & 7) == 0) {
			//num4SampleHorizontalEdges     = (sequenceControlSet->chromaHeight >> 3) + ((sequenceControlSet->chromaHeight & 7) != 0) - 1;
			num4SampleHorizontalEdges = (lcuHeight >> (colorFormat==EB_YUV420?4:3)) + (((lcuHeight >> (colorFormat==EB_YUV420?1:0)) & (colorFormat==EB_YUV420?7:15)) != 0);
			fourSampleEdgeStartSamplePos_x = sequenceControlSet->chromaWidth - 4;       // Picture wise location
			//for(verticalIdx = 1; verticalIdx <= num4SampleHorizontalEdges; ++verticalIdx) {
			for (verticalIdx = (lcuPos_y == 0); verticalIdx < num4SampleHorizontalEdges; ++verticalIdx) {
				//fourSampleEdgeStartSamplePos_y = verticalIdx << 3;                      // Picture wise location
				fourSampleEdgeStartSamplePos_y = (lcuPos_y >> (colorFormat==EB_YUV420?1:0)) + (verticalIdx << 3);                      // Picture wise location
				//lcuIdx     = (fourSampleEdgeStartSamplePos_y >> (logMaxLcuSize-1)) * pictureWidthInLcu + (fourSampleEdgeStartSamplePos_x >> (logMaxLcuSize-1));
				//lcuPtr     = pictureControlSetPtr->lcuPtrArray[lcuIdx];

				// left 2 sample edge
				blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
					(fourSampleEdgeStartSamplePos_x & (chromaLcuSizeX - 1)) >> subWidthShfitMinus1,
					(fourSampleEdgeStartSamplePos_y & (chromaLcuSizeY - 1)) >> subHeightShfitMinus1,
					logMaxLcuSizeIn4x4blk);
				bS = pictureControlSetPtr->horizontalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

				if (bS > 1) {
					// Qp for the current CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x,
						fourSampleEdgeStartSamplePos_y,
						pictureControlSetPtr->qpArrayStride);


					curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

					// Qp for the neighboring CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x,
						fourSampleEdgeStartSamplePos_y - 1,
						pictureControlSetPtr->qpArrayStride);
					//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
					neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

					cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->cbQpOffset), colorFormat>=EB_YUV422);
					crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->crQpOffset), colorFormat>=EB_YUV422);
					//chromaQp = MapChromaQp(Qp);
					cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + pictureControlSetPtr->tcOffset)];
					crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + pictureControlSetPtr->tcOffset)];
					edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + fourSampleEdgeStartSamplePos_y * reconPic->strideCb + fourSampleEdgeStartSamplePos_x;
					edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + fourSampleEdgeStartSamplePos_y * reconPic->strideCr + fourSampleEdgeStartSamplePos_x;

					Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
						edgeStartSampleCb,
						edgeStartSampleCr,
						reconPic->strideCb,
						EB_FALSE,
						cbTc,
						crTc);

				}

				// right 2 sample edge
				blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
					((fourSampleEdgeStartSamplePos_x & (chromaLcuSizeX - 1)) + 2) >> subWidthShfitMinus1,
					(fourSampleEdgeStartSamplePos_y & (chromaLcuSizeY - 1)) >> subHeightShfitMinus1,
					logMaxLcuSizeIn4x4blk);
				bS = pictureControlSetPtr->horizontalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

				if (bS > 1) {
					// Qp for the current CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x + 2,
						fourSampleEdgeStartSamplePos_y,
						pictureControlSetPtr->qpArrayStride);


					curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

					// Qp for the neighboring CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x + 2,
						fourSampleEdgeStartSamplePos_y - 1,
						pictureControlSetPtr->qpArrayStride);
					//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
					neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

					cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->cbQpOffset), colorFormat>=EB_YUV422);
					crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->crQpOffset), colorFormat>=EB_YUV422);
					//chromaQp = MapChromaQp(Qp);
					cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + pictureControlSetPtr->tcOffset)];
					crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + pictureControlSetPtr->tcOffset)];
					edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + fourSampleEdgeStartSamplePos_y * reconPic->strideCb + (fourSampleEdgeStartSamplePos_x + 2);
					edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + fourSampleEdgeStartSamplePos_y * reconPic->strideCr + (fourSampleEdgeStartSamplePos_x + 2);

					Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
						edgeStartSampleCb,
						edgeStartSampleCr,
						reconPic->strideCb,
						EB_FALSE,
						cbTc,
						crTc);
				}
			}
		}
	}

	if (lcuPos_y >> lcuPtr->sizeLog2 == pictureHeightInLcu - 1) {
		/***** picture bottom 4 sample vertical edges filtering *****/
		// luma component filtering
		//num4SampleVerticalEdges       = (sequenceControlSet->lumaWidth >> 3) - 1;
		num4SampleVerticalEdges = (lcuWidth >> 3);
		fourSampleEdgeStartSamplePos_y = sequenceControlSet->lumaHeight - 4;      // picture-wise position
		//for(horizontalIdx = 1; horizontalIdx <= num4SampleVerticalEdges; ++horizontalIdx) {
		for (horizontalIdx = (lcuPos_x == 0); horizontalIdx < num4SampleVerticalEdges; ++horizontalIdx) {
			// edge A
			fourSampleEdgeStartSamplePos_x = lcuPos_x + (horizontalIdx << 3);                  // picuture-wise position
			lcuIdx = (fourSampleEdgeStartSamplePos_y >> logMaxLcuSize) * pictureWidthInLcu + (fourSampleEdgeStartSamplePos_x >> logMaxLcuSize);
			lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIdx];
			blk4x4Addr = GET_LUMA_4X4BLK_ADDR(
				fourSampleEdgeStartSamplePos_x & (lcuSize - 1),
				fourSampleEdgeStartSamplePos_y & (lcuSize - 1),
				logMaxLcuSizeIn4x4blk);
			//neighbourBlk4x4Addr = blk4x4Addr - MaxLcuSizeIn4x4blk;

			bS = pictureControlSetPtr->verticalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)];
			if (bS > 0) {
				/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
				*/

				// Qp for the current CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x,
					fourSampleEdgeStartSamplePos_y,
					pictureControlSetPtr->qpArrayStride);


				curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

				// Qp for the neighboring CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x - 1,
					fourSampleEdgeStartSamplePos_y,
					pictureControlSetPtr->qpArrayStride);
				//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
				neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

				Qp = (curCuQp + neighbourCuQp + 1) >> 1;
				lumaTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bS > 1) << 1) + pictureControlSetPtr->tcOffset)];
				Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + pictureControlSetPtr->betaOffset)];
				edgeStartFilteredSamplePtr = reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + fourSampleEdgeStartSamplePos_y * reconPic->strideY + fourSampleEdgeStartSamplePos_x;

				// 4 sample edge luma filter core
				Luma4SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartFilteredSamplePtr,
					reconPic->strideY,
					EB_TRUE,
					lumaTc,
					Beta);
			}
		}

		// chroma component filtering
		if ((sequenceControlSet->chromaHeight & 7) == 0) {
			//num4SampleVerticalEdges       = (sequenceControlSet->chromaWidth >> 3) + ((sequenceControlSet->chromaWidth & 7) != 0) - 1;
			num4SampleVerticalEdges = (lcuWidth >> (3+(colorFormat==EB_YUV444?0:1))) + (((lcuWidth >> (colorFormat==EB_YUV444?0:1)) & (colorFormat==EB_YUV444?15:7)) != 0);

			fourSampleEdgeStartSamplePos_y = sequenceControlSet->chromaHeight - 4;        // Picture wise location
			//for(horizontalIdx = 1; horizontalIdx <= num4SampleVerticalEdges; ++horizontalIdx) {
			for (horizontalIdx = (lcuPos_x == 0); horizontalIdx < num4SampleVerticalEdges; ++horizontalIdx) {
				fourSampleEdgeStartSamplePos_x = (lcuPos_x >> (colorFormat==EB_YUV444?0:1)) + (horizontalIdx << 3);   // Picture wise location
				lcuIdx = (fourSampleEdgeStartSamplePos_y >> (logMaxLcuSize - (colorFormat==EB_YUV420?1:0))) * pictureWidthInLcu + (fourSampleEdgeStartSamplePos_x >> (logMaxLcuSize - (colorFormat==EB_YUV444?0:1)));
				lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIdx];

				// Upper 2 sample edge
				blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
					(fourSampleEdgeStartSamplePos_x & (chromaLcuSizeX - 1)) >> subWidthShfitMinus1,
					(fourSampleEdgeStartSamplePos_y & (chromaLcuSizeY - 1)) >> subHeightShfitMinus1,
					logMaxLcuSizeIn4x4blk);
				bS = pictureControlSetPtr->verticalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

				if (bS > 1) {
					// Qp for the current CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x,
						fourSampleEdgeStartSamplePos_y,
						pictureControlSetPtr->qpArrayStride);


					curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

					// Qp for the neighboring CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x - 1,
						fourSampleEdgeStartSamplePos_y,
						pictureControlSetPtr->qpArrayStride);
					//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
					neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];


					cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->cbQpOffset), colorFormat>=EB_YUV422);
					crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->crQpOffset), colorFormat>=EB_YUV422);
					//chromaQp = MapChromaQp(Qp);
					cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + pictureControlSetPtr->tcOffset)];
					crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + pictureControlSetPtr->tcOffset)];
					edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + fourSampleEdgeStartSamplePos_y * reconPic->strideCb + fourSampleEdgeStartSamplePos_x;
					edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + fourSampleEdgeStartSamplePos_y * reconPic->strideCr + fourSampleEdgeStartSamplePos_x;


					Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
						edgeStartSampleCb,
						edgeStartSampleCr,
						reconPic->strideCb,
						EB_TRUE,
						cbTc,
						crTc);
				}

				// Lower 2 sample edge
				blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
					(fourSampleEdgeStartSamplePos_x & (chromaLcuSizeX - 1)) >> subWidthShfitMinus1,
					((fourSampleEdgeStartSamplePos_y & (chromaLcuSizeY - 1)) + 2) >> subHeightShfitMinus1,
					logMaxLcuSizeIn4x4blk);
				bS = pictureControlSetPtr->verticalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

				if (bS > 1) {
					// Qp for the current CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x,
						fourSampleEdgeStartSamplePos_y + 2,
						pictureControlSetPtr->qpArrayStride);


					curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

					// Qp for the neighboring CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x - 1,
						fourSampleEdgeStartSamplePos_y + 2,
						pictureControlSetPtr->qpArrayStride);
					//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
					neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];
					cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->cbQpOffset), colorFormat>=EB_YUV422);
					crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->crQpOffset), colorFormat>=EB_YUV422);
					//chromaQp = MapChromaQp(Qp);
					cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + pictureControlSetPtr->tcOffset)];
					crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + pictureControlSetPtr->tcOffset)];
					edgeStartSampleCb = reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + (fourSampleEdgeStartSamplePos_y + 2) * reconPic->strideCb + fourSampleEdgeStartSamplePos_x;
					edgeStartSampleCr = reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + (fourSampleEdgeStartSamplePos_y + 2) * reconPic->strideCr + fourSampleEdgeStartSamplePos_x;


					Chroma2SampleEdgeDLFCore_Table[!!(ASM_TYPES & PREAVX2_MASK)](
						edgeStartSampleCb,
						edgeStartSampleCr,
						reconPic->strideCb,
						EB_TRUE,
						cbTc,
						crTc);
				}
			}
		}
	}
	return;
}

/** LCUPictureEdgeDLFCore()
is used to apply the deblocking filter on the LCU boundary
*/
void LCUPictureEdgeDLFCore16bit(
	EbPictureBufferDesc_t *reconPic,                       //input parameter, the pointer to the reconstructed picture.
	EB_U32                 lcuIdx,
	EB_U32                 lcuPos_x,                        //input parameter, the picture-wise horizontal location of the LCU.
	EB_U32                 lcuPos_y,                        //input parameter, the picture-wise vertical location of the LCU.
	EB_U32                 lcuWidth,                        //input parameter, the LCU width.
	EB_U32                 lcuHeight,                       //input parameter, the LCU height.
	PictureControlSet_t   *pictureControlSetPtr)            //input parameter, picture control set.
{
	const EB_U32 MaxLcuSizeIn4x4blk = MAX_LCU_SIZE >> 2;
	const EB_U32 logMaxLcuSizeIn4x4blk = Log2f(MaxLcuSizeIn4x4blk);
	const EB_U32 logMaxLcuSize = Log2f(MAX_LCU_SIZE);


	EB_U32  verticalIdx;
	EB_U32  horizontalIdx;
	EB_U32  num4SampleHorizontalEdges;
	EB_U32  num4SampleVerticalEdges;
	EB_U32  blk4x4Addr;
	EB_U32  blk2x2Addr;
	EB_U32  pictureWidthInLcu;
	EB_U32  pictureHeightInLcu;
	EB_U32  fourSampleEdgeStartSamplePos_x;
	EB_U32  fourSampleEdgeStartSamplePos_y;
	EB_U8   bS;
	EB_U32  lcuSize;
    EB_U32  chromaLcuSizeX;
    EB_U32  chromaLcuSizeY;
	EB_U8  curCuQp;
	EB_U16 *edgeStartFilteredSamplePtr;
	EB_U16 *edgeStartSampleCb;
	EB_U16 *edgeStartSampleCr;
	EB_U8  neighbourCuQp;
	EB_U8  Qp;
	EB_S32  Beta;
	EB_S32  lumaTc;
	//EB_BOOL lumaPCMFlagArray[2];
	//EB_BOOL chromaPCMFlagArray[2];
	EB_S32  CUqpIndex;
	EB_U8  cbQp;
	EB_U8  crQp;
	EB_U8   cbTc;
	EB_U8   crTc;
    EB_COLOR_FORMAT colorFormat = reconPic->colorFormat;
    const EB_S32 subWidthC      = colorFormat==EB_YUV444?1:2;
    const EB_S32 subHeightC     = colorFormat==EB_YUV420?2:1;
    const EB_S32 subWidthCMinus1  = colorFormat==EB_YUV444?0:1;
    const EB_S32 subHeightCMinus1 = colorFormat==EB_YUV420?1:0;

	SequenceControlSet_t  *sequenceControlSet = (SequenceControlSet_t*)pictureControlSetPtr->sequenceControlSetWrapperPtr->objectPtr;
	LargestCodingUnit_t   *lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIdx];

	pictureWidthInLcu = (sequenceControlSet->lumaWidth + sequenceControlSet->lcuSize - 1) / sequenceControlSet->lcuSize;
	pictureHeightInLcu = (sequenceControlSet->lumaHeight + sequenceControlSet->lcuSize - 1) / sequenceControlSet->lcuSize;
    lcuSize = sequenceControlSet->lcuSize;
    chromaLcuSizeX = lcuSize >> (colorFormat==EB_YUV444?0:1);
    chromaLcuSizeY = lcuSize >> (colorFormat==EB_YUV420?1:0);
    const EB_U32 subWidthShfitMinus1  = colorFormat==EB_YUV444?1:0;
    const EB_U32 subHeightShfitMinus1 = colorFormat==EB_YUV420?0:1;

	if (lcuPos_x >> lcuPtr->sizeLog2 == pictureWidthInLcu - 1) {
		/***** picture right-most 4 sample horizontal edges filtering *****/
		// luma component filtering
		//num4SampleHorizontalEdges     = (sequenceControlSet->lumaHeight >> 3) - 1;
		num4SampleHorizontalEdges = (lcuHeight >> 3);
		fourSampleEdgeStartSamplePos_x = sequenceControlSet->lumaWidth - 4;          // picutre-wise position
		//for(verticalIdx = 1; verticalIdx <= num4SampleHorizontalEdges; ++verticalIdx) {
		for (verticalIdx = (lcuPos_y == 0); verticalIdx < num4SampleHorizontalEdges; ++verticalIdx) {
			// edge B
			//fourSampleEdgeStartSamplePos_y = verticalIdx << 3;                       // picture-wise position
			fourSampleEdgeStartSamplePos_y = lcuPos_y + (verticalIdx << 3);                       // picture-wise position
			//lcuIdx     = (fourSampleEdgeStartSamplePos_y >> logMaxLcuSize) * pictureWidthInLcu + (fourSampleEdgeStartSamplePos_x >> logMaxLcuSize);
			//lcuPtr     = pictureControlSetPtr->lcuPtrArray[lcuIdx];
			blk4x4Addr = GET_LUMA_4X4BLK_ADDR(
				fourSampleEdgeStartSamplePos_x & (lcuSize - 1),
				fourSampleEdgeStartSamplePos_y & (lcuSize - 1),
				logMaxLcuSizeIn4x4blk);
			//neighbourBlk4x4Addr = blk4x4Addr - 1;

			bS = pictureControlSetPtr->horizontalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)];
			if (bS > 0) {
				/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
				*/

				// Qp for the current CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x,
					fourSampleEdgeStartSamplePos_y,
					pictureControlSetPtr->qpArrayStride);


				curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

				// Qp for the neighboring CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x,
					fourSampleEdgeStartSamplePos_y - 1,
					pictureControlSetPtr->qpArrayStride);
				//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
				neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];


				Qp = (curCuQp + neighbourCuQp + 1) >> 1;
				lumaTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bS > 1) << 1) + pictureControlSetPtr->tcOffset)];
				Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + pictureControlSetPtr->betaOffset)];

				lumaTc = lumaTc << 2;
				Beta = Beta << 2;
				edgeStartFilteredSamplePtr = (EB_U16*)reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + fourSampleEdgeStartSamplePos_y * reconPic->strideY + fourSampleEdgeStartSamplePos_x;

				// 4 sample luma edge filter core
				lumaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartFilteredSamplePtr,
					reconPic->strideY,
					EB_FALSE,
					lumaTc,
					Beta);
			}
		}

		// chroma component filtering
		if ((sequenceControlSet->chromaWidth & 7) == 0) {
			//num4SampleHorizontalEdges     = (sequenceControlSet->chromaHeight >> 3) + ((sequenceControlSet->chromaHeight & 7) != 0) - 1;
            num4SampleHorizontalEdges = (lcuHeight >> (colorFormat==EB_YUV420?4:3)) + (((lcuHeight >> (colorFormat==EB_YUV420?1:0)) & (colorFormat==EB_YUV420?7:15)) != 0);
			fourSampleEdgeStartSamplePos_x = sequenceControlSet->chromaWidth - 4;       // Picture wise location
			//for(verticalIdx = 1; verticalIdx <= num4SampleHorizontalEdges; ++verticalIdx) {
			for (verticalIdx = (lcuPos_y == 0); verticalIdx < num4SampleHorizontalEdges; ++verticalIdx) {
				//fourSampleEdgeStartSamplePos_y = verticalIdx << 3;                      // Picture wise location
                fourSampleEdgeStartSamplePos_y = (lcuPos_y >> (colorFormat==EB_YUV420?1:0)) + (verticalIdx << 3);                      // Picture wise location
				//lcuIdx     = (fourSampleEdgeStartSamplePos_y >> (logMaxLcuSize-1)) * pictureWidthInLcu + (fourSampleEdgeStartSamplePos_x >> (logMaxLcuSize-1));
				//lcuPtr     = pictureControlSetPtr->lcuPtrArray[lcuIdx];

				// left 2 sample edge
				blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
                    (fourSampleEdgeStartSamplePos_x & (chromaLcuSizeX - 1)) >> subWidthShfitMinus1,
                    (fourSampleEdgeStartSamplePos_y & (chromaLcuSizeY - 1)) >> subHeightShfitMinus1,
					logMaxLcuSizeIn4x4blk);
				bS = pictureControlSetPtr->horizontalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

				if (bS > 1) {
					// Qp for the current CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x,
						fourSampleEdgeStartSamplePos_y,
						pictureControlSetPtr->qpArrayStride);


					curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

					// Qp for the neighboring CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x,
						fourSampleEdgeStartSamplePos_y - 1,
						pictureControlSetPtr->qpArrayStride);
					//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
					neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

					cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->cbQpOffset), colorFormat>=EB_YUV422);
					crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->crQpOffset), colorFormat>=EB_YUV422);
					//chromaQp = MapChromaQp(Qp);
					cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + pictureControlSetPtr->tcOffset)];
					crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + pictureControlSetPtr->tcOffset)];

					cbTc = cbTc << 2;
					crTc = crTc << 2;
					edgeStartSampleCb = ((EB_U16*)reconPic->bufferCb) + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + fourSampleEdgeStartSamplePos_y * reconPic->strideCb + fourSampleEdgeStartSamplePos_x;
					edgeStartSampleCr = ((EB_U16*)reconPic->bufferCr) + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + fourSampleEdgeStartSamplePos_y * reconPic->strideCr + fourSampleEdgeStartSamplePos_x;

					chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
						edgeStartSampleCb,
						edgeStartSampleCr,
						reconPic->strideCb,
						EB_FALSE,
						cbTc,
						crTc);

				}

				// right 2 sample edge
				blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
                    ((fourSampleEdgeStartSamplePos_x & (chromaLcuSizeX - 1)) + 2) >> subWidthShfitMinus1,
                    (fourSampleEdgeStartSamplePos_y & (chromaLcuSizeY - 1)) >> subHeightShfitMinus1,
					logMaxLcuSizeIn4x4blk);
				bS = pictureControlSetPtr->horizontalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_HORIZONTAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

				if (bS > 1) {
					// Qp for the current CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x + 2,
						fourSampleEdgeStartSamplePos_y,
						pictureControlSetPtr->qpArrayStride);


					curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

					// Qp for the neighboring CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x + 2,
						fourSampleEdgeStartSamplePos_y - 1,
						pictureControlSetPtr->qpArrayStride);
					//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
					neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];


					cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->cbQpOffset), colorFormat>=EB_YUV422);
					crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->crQpOffset), colorFormat>=EB_YUV422);
					//chromaQp = MapChromaQp(Qp);
					cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + pictureControlSetPtr->tcOffset)];
					crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + pictureControlSetPtr->tcOffset)];

					cbTc = cbTc << 2;
					crTc = crTc << 2;
					edgeStartSampleCb = ((EB_U16*)reconPic->bufferCb) + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + fourSampleEdgeStartSamplePos_y * reconPic->strideCb + (fourSampleEdgeStartSamplePos_x + 2);
					edgeStartSampleCr = ((EB_U16*)reconPic->bufferCr) + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + fourSampleEdgeStartSamplePos_y * reconPic->strideCr + (fourSampleEdgeStartSamplePos_x + 2);

					chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
						edgeStartSampleCb,
						edgeStartSampleCr,
						reconPic->strideCb,
						EB_FALSE,
						cbTc,
						crTc);
				}
			}
		}
	}

	if (lcuPos_y >> lcuPtr->sizeLog2 == pictureHeightInLcu - 1) {
		/***** picture bottom 4 sample vertical edges filtering *****/
		// luma component filtering
		//num4SampleVerticalEdges       = (sequenceControlSet->lumaWidth >> 3) - 1;
		num4SampleVerticalEdges = (lcuWidth >> 3);
		fourSampleEdgeStartSamplePos_y = sequenceControlSet->lumaHeight - 4;      // picture-wise position
		//for(horizontalIdx = 1; horizontalIdx <= num4SampleVerticalEdges; ++horizontalIdx) {
		for (horizontalIdx = (lcuPos_x == 0); horizontalIdx < num4SampleVerticalEdges; ++horizontalIdx) {
			// edge A
			fourSampleEdgeStartSamplePos_x = lcuPos_x + (horizontalIdx << 3);                  // picuture-wise position
			lcuIdx = (fourSampleEdgeStartSamplePos_y >> logMaxLcuSize) * pictureWidthInLcu + (fourSampleEdgeStartSamplePos_x >> logMaxLcuSize);
			lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIdx];
			blk4x4Addr = GET_LUMA_4X4BLK_ADDR(
				fourSampleEdgeStartSamplePos_x & (lcuSize - 1),
				fourSampleEdgeStartSamplePos_y & (lcuSize - 1),
				logMaxLcuSizeIn4x4blk);
			//neighbourBlk4x4Addr = blk4x4Addr - MaxLcuSizeIn4x4blk;

			bS = pictureControlSetPtr->verticalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk4x4Addr)];
			if (bS > 0) {
				/** *Note - The PCMFlagArray initiallisation should be completely changed when we have PCM coding.
				*/

				// Qp for the current CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x,
					fourSampleEdgeStartSamplePos_y,
					pictureControlSetPtr->qpArrayStride);


				curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];

				// Qp for the neighboring CU
				CUqpIndex =
					LUMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
					fourSampleEdgeStartSamplePos_x - 1,
					fourSampleEdgeStartSamplePos_y,
					pictureControlSetPtr->qpArrayStride);
				//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
				neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];
				Qp = (curCuQp + neighbourCuQp + 1) >> 1;
				lumaTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, Qp + ((bS > 1) << 1) + pictureControlSetPtr->tcOffset)];
				Beta = BetaTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE, Qp + pictureControlSetPtr->betaOffset)];

				lumaTc = lumaTc << 2;
				Beta = Beta << 2;
				edgeStartFilteredSamplePtr = (EB_U16*)reconPic->bufferY + reconPic->originX + reconPic->originY * reconPic->strideY + fourSampleEdgeStartSamplePos_y * reconPic->strideY + fourSampleEdgeStartSamplePos_x;

				// 4 sample edge luma filter core
				lumaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
					edgeStartFilteredSamplePtr,
					reconPic->strideY,
					EB_TRUE,
					lumaTc,
					Beta);
			}
		}

		// chroma component filtering
		if ((sequenceControlSet->chromaHeight & 7) == 0) {
			//num4SampleVerticalEdges       = (sequenceControlSet->chromaWidth >> 3) + ((sequenceControlSet->chromaWidth & 7) != 0) - 1;
            num4SampleVerticalEdges = (lcuWidth >> (3+(colorFormat==EB_YUV444?0:1))) + (((lcuWidth >> (colorFormat==EB_YUV444?0:1)) & (colorFormat==EB_YUV444?15:7)) != 0);
			fourSampleEdgeStartSamplePos_y = sequenceControlSet->chromaHeight - 4;        // Picture wise location
			//for(horizontalIdx = 1; horizontalIdx <= num4SampleVerticalEdges; ++horizontalIdx) {
			for (horizontalIdx = (lcuPos_x == 0); horizontalIdx < num4SampleVerticalEdges; ++horizontalIdx) {
                fourSampleEdgeStartSamplePos_x = (lcuPos_x >> (colorFormat==EB_YUV444?0:1)) + (horizontalIdx << 3);   // Picture wise location
                lcuIdx = (fourSampleEdgeStartSamplePos_y >> (logMaxLcuSize - (colorFormat==EB_YUV420?1:0))) * pictureWidthInLcu + (fourSampleEdgeStartSamplePos_x >> (logMaxLcuSize - (colorFormat==EB_YUV444?0:1)));
				lcuPtr = pictureControlSetPtr->lcuPtrArray[lcuIdx];

				// Upper 2 sample edge
				blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
                    (fourSampleEdgeStartSamplePos_x & (chromaLcuSizeX - 1)) >> subWidthShfitMinus1,
                    (fourSampleEdgeStartSamplePos_y & (chromaLcuSizeY - 1)) >> subHeightShfitMinus1,
					logMaxLcuSizeIn4x4blk);
				bS = pictureControlSetPtr->verticalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

				if (bS > 1) {
					// Qp for the current CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x,
						fourSampleEdgeStartSamplePos_y,
						pictureControlSetPtr->qpArrayStride);


					curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];


					// Qp for the neighboring CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x - 1,
						fourSampleEdgeStartSamplePos_y,
						pictureControlSetPtr->qpArrayStride);
					//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
					neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];
					cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->cbQpOffset), colorFormat>=EB_YUV422);
					crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->crQpOffset), colorFormat>=EB_YUV422);
					//chromaQp = MapChromaQp(Qp);
					cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + pictureControlSetPtr->tcOffset)];
					crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + pictureControlSetPtr->tcOffset)];

					cbTc = cbTc << 2;
					crTc = crTc << 2;
					edgeStartSampleCb = (EB_U16*)reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + fourSampleEdgeStartSamplePos_y * reconPic->strideCb + fourSampleEdgeStartSamplePos_x;
					edgeStartSampleCr = (EB_U16*)reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + fourSampleEdgeStartSamplePos_y * reconPic->strideCr + fourSampleEdgeStartSamplePos_x;


					chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
						edgeStartSampleCb,
						edgeStartSampleCr,
						reconPic->strideCb,
						EB_TRUE,
						cbTc,
						crTc);
				}

				// Lower 2 sample edge
                blk2x2Addr = GET_CHROMA_4X4BLK_ADDR(
                    (fourSampleEdgeStartSamplePos_x & (chromaLcuSizeX - 1)) >> subWidthShfitMinus1,
                    ((fourSampleEdgeStartSamplePos_y & (chromaLcuSizeY - 1)) + 2) >> subHeightShfitMinus1,
					logMaxLcuSizeIn4x4blk);
				bS = pictureControlSetPtr->verticalEdgeBSArray[lcuIdx][BLK4X4_ADDR_TO_VERTICAL_EDGE_BS_ARRAY_IDX(blk2x2Addr)];

				if (bS > 1) {
					// Qp for the current CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x,
						fourSampleEdgeStartSamplePos_y + 2,
						pictureControlSetPtr->qpArrayStride);


					curCuQp = pictureControlSetPtr->qpArray[CUqpIndex];


					// Qp for the neighboring CU
					CUqpIndex =
						CHROMA_SAMPLE_PIC_WISE_LOCATION_TO_QP_ARRAY_IDX(
                        subWidthC,
                        subHeightC,
						fourSampleEdgeStartSamplePos_x - 1,
						fourSampleEdgeStartSamplePos_y + 2,
						pictureControlSetPtr->qpArrayStride);
					//CUqpIndex     = ((CUqpIndex - (EB_S32)pictureControlSetPtr->qpArrayStride) < 0) ? CUqpIndex : CUqpIndex - pictureControlSetPtr->qpArrayStride;
					neighbourCuQp = pictureControlSetPtr->qpArray[CUqpIndex];
					cbQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->cbQpOffset), colorFormat>=EB_YUV422);
					crQp = convertToChromaQp((EB_S32)(((curCuQp + neighbourCuQp + 1) >> 1) + pictureControlSetPtr->crQpOffset), colorFormat>=EB_YUV422);
					//chromaQp = MapChromaQp(Qp);
					cbTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (cbQp + 2) + pictureControlSetPtr->tcOffset)];
					crTc = TcTable_8x8[CLIP3EQ(MIN_QP_VALUE, MAX_QP_VALUE_PLUS_INTRA_TC_OFFSET, (crQp + 2) + pictureControlSetPtr->tcOffset)];

					cbTc = cbTc << 2;
					crTc = crTc << 2;
					edgeStartSampleCb = (EB_U16*)reconPic->bufferCb + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCb + (fourSampleEdgeStartSamplePos_y + 2) * reconPic->strideCb + fourSampleEdgeStartSamplePos_x;
					edgeStartSampleCr = (EB_U16*)reconPic->bufferCr + (reconPic->originX >> subWidthCMinus1) + (reconPic->originY >> subHeightCMinus1) * reconPic->strideCr + (fourSampleEdgeStartSamplePos_y + 2) * reconPic->strideCr + fourSampleEdgeStartSamplePos_x;

					chromaDlf_funcPtrArray16bit[!!(ASM_TYPES & PREAVX2_MASK)](
						edgeStartSampleCb,
						edgeStartSampleCr,
						reconPic->strideCb,
						EB_TRUE,
						cbTc,
						crTc);
				}
			}
		}
	}
    
	return;
}

