/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include <stdlib.h>
#include <string.h>

#include "EbNeighborArrays.h"
#include "EbUtility.h"

static void NeighborArrayUnitDctor(EB_PTR p)
{
    NeighborArrayUnit_t *obj = (NeighborArrayUnit_t*)p;
    EB_FREE(obj->leftArray);
    EB_FREE(obj->topArray);
    EB_FREE(obj->topLeftArray);
}

/*************************************************
 * Neighbor Array Unit Ctor
 *************************************************/
EB_ERRORTYPE NeighborArrayUnitCtor(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U32   maxPictureWidth,
    EB_U32   maxPictureHeight,
    EB_U32   unitSize,
    EB_U32   granularityNormal,
    EB_U32   granularityTopLeft,
    EB_U32   typeMask)
{
    naUnitPtr->dctor = NeighborArrayUnitDctor;
    naUnitPtr->unitSize                 = (EB_U8)(unitSize);
    naUnitPtr->granularityNormal        = (EB_U8)(granularityNormal);
    naUnitPtr->granularityNormalLog2    = (EB_U8)(Log2f(naUnitPtr->granularityNormal));
    naUnitPtr->granularityTopLeft       = (EB_U8)(granularityTopLeft);
    naUnitPtr->granularityTopLeftLog2   = (EB_U8)(Log2f(naUnitPtr->granularityTopLeft));
    naUnitPtr->leftArraySize            = (EB_U16)((typeMask & NEIGHBOR_ARRAY_UNIT_LEFT_MASK) ? maxPictureHeight >> naUnitPtr->granularityNormalLog2 : 0);
    naUnitPtr->topArraySize             = (EB_U16)((typeMask & NEIGHBOR_ARRAY_UNIT_TOP_MASK) ? maxPictureWidth >> naUnitPtr->granularityNormalLog2 : 0);
    naUnitPtr->topLeftArraySize         = (EB_U16)((typeMask & NEIGHBOR_ARRAY_UNIT_TOPLEFT_MASK) ? (maxPictureWidth + maxPictureHeight) >> naUnitPtr->granularityTopLeftLog2 : 0);

    if(naUnitPtr->leftArraySize) {
        EB_MALLOC(naUnitPtr->leftArray, naUnitPtr->unitSize * naUnitPtr->leftArraySize);
    }

    if(naUnitPtr->topArraySize) {
        EB_MALLOC(naUnitPtr->topArray, naUnitPtr->unitSize * naUnitPtr->topArraySize);
    }

    if(naUnitPtr->topLeftArraySize) {
        EB_MALLOC(naUnitPtr->topLeftArray, naUnitPtr->unitSize * naUnitPtr->topLeftArraySize);
    }

    return EB_ErrorNone;
}


/*************************************************
 * Neighbor Array Unit Reset
 *************************************************/
void NeighborArrayUnitReset(NeighborArrayUnit_t *naUnitPtr)
{
    if(naUnitPtr->leftArray) {
        EB_MEMSET(naUnitPtr->leftArray, ~0, naUnitPtr->unitSize * naUnitPtr->leftArraySize);
    }

    if(naUnitPtr->topArray) {
        EB_MEMSET(naUnitPtr->topArray, ~0, naUnitPtr->unitSize * naUnitPtr->topArraySize);
    }

    if(naUnitPtr->topLeftArray) {
        EB_MEMSET(naUnitPtr->topLeftArray, ~0, naUnitPtr->unitSize * naUnitPtr->topLeftArraySize);
    }

    return;
}


/*************************************************
 * Neighbor Array Unit Get Left Index
 *************************************************/
EB_U32 GetNeighborArrayUnitLeftIndex(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U32               locY)
{
    return (locY >> naUnitPtr->granularityNormalLog2);
}

/*************************************************
 * Neighbor Array Unit Get Top Index
 *************************************************/
EB_U32 GetNeighborArrayUnitTopIndex(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U32               locX)
{
    return (locX >> naUnitPtr->granularityNormalLog2);
}

/*************************************************
 * Neighbor Array Unit Get Top Index
 *************************************************/
EB_U32 GetNeighborArrayUnitTopLeftIndex(
    NeighborArrayUnit_t *naUnitPtr,
    EB_S32               locX,
    EB_S32               locY)
{
    return naUnitPtr->leftArraySize + (locX >> naUnitPtr->granularityTopLeftLog2) - (locY >> naUnitPtr->granularityTopLeftLog2);
}

/*************************************************
 * Neighbor Array Sample Update
 *************************************************/
void NeighborArrayUnitSampleWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *srcPtr,
    EB_U32               stride,
    EB_U32               srcOriginX,
    EB_U32               srcOriginY,
    EB_U32               picOriginX,
    EB_U32               picOriginY,
    EB_U32               blockWidth,
    EB_U32               blockHeight,
    EB_U32               neighborArrayTypeMask)
{
    EB_U32 idx;
    EB_U8  *dstPtr;
    EB_U8  *readPtr;

    EB_S32 dstStep;
    EB_S32 readStep;
    EB_U32 count;

    // Adjust the Source ptr to start at the origin of the block being updated.
    srcPtr += ((srcOriginY * stride) + srcOriginX) * naUnitPtr->unitSize;

    if(neighborArrayTypeMask & NEIGHBOR_ARRAY_UNIT_TOP_MASK) {

        //
        //     ----------12345678---------------------  Top Neighbor Array
        //                ^    ^
        //                |    |
        //                |    |
        //               xxxxxxxx
        //               x      x
        //               x      x
        //               12345678
        //
        //  The top neighbor array is updated with the samples from the
        //    bottom row of the source block
        //
        //  Index = originX

        // Adjust readPtr to the bottom-row
        readPtr = srcPtr + ((blockHeight - 1) * stride);

        dstPtr = naUnitPtr->topArray +
                 GetNeighborArrayUnitTopIndex(
                     naUnitPtr,
                     picOriginX) * naUnitPtr->unitSize;

        dstStep = naUnitPtr->unitSize;
        readStep = naUnitPtr->unitSize;
        count   = blockWidth;

        for(idx = 0; idx < count; ++idx) {

            *dstPtr = *readPtr;

            dstPtr  += dstStep;
            readPtr += readStep;
        }

    }

    if(neighborArrayTypeMask & NEIGHBOR_ARRAY_UNIT_LEFT_MASK) {

        //   Left Neighbor Array
        //
        //    |
        //    |
        //    1         xxxxxxx1
        //    2  <----  x      2
        //    3  <----  x      3
        //    4         xxxxxxx4
        //    |
        //    |
        //
        //  The left neighbor array is updated with the samples from the
        //    right column of the source block
        //
        //  Index = originY

        // Adjust readPtr to the right-column
        readPtr = srcPtr + (blockWidth - 1);

        dstPtr = naUnitPtr->leftArray +
                 GetNeighborArrayUnitLeftIndex(
                     naUnitPtr,
                     picOriginY) * naUnitPtr->unitSize;

        dstStep = 1;
        readStep = stride;
        count   = blockHeight;

        for(idx = 0; idx < count; ++idx) {

            *dstPtr = *readPtr;

            dstPtr += dstStep;
            readPtr += readStep;
        }

    }

    if(neighborArrayTypeMask & NEIGHBOR_ARRAY_UNIT_TOPLEFT_MASK) {

        /*
        //   Top-left Neighbor Array
        //
        //    4-5--6--7--------------
        //    3 \      \
        //    2  \      \
        //    1   \      \
        //    |\   xxxxxx7
        //    | \  x     6
        //    |  \ x     5
        //    |   \1x2x3x4
        //    |
        //
        //  The top-left neighbor array is updated with the reversed samples
        //    from the right column and bottom row of the source block
        //
        // Index = originX - originY
        */

        // Adjust readPtr to the bottom-row
        readPtr = srcPtr + ((blockHeight - 1) * stride);

        // Copy bottom row
        dstPtr =
            naUnitPtr->topLeftArray +
            GetNeighborArrayUnitTopLeftIndex(
                naUnitPtr,
                picOriginX,
                picOriginY + (blockWidth - 1)) * naUnitPtr->unitSize;

		EB_MEMCPY(dstPtr,readPtr,blockWidth);

        // Reset readPtr to the right-column
        readPtr = srcPtr + (blockWidth - 1);

        // Copy right column
        dstPtr =
            naUnitPtr->topLeftArray +
            GetNeighborArrayUnitTopLeftIndex(
                naUnitPtr,
                picOriginX + (blockWidth - 1),
                picOriginY) * naUnitPtr->unitSize;

        dstStep = -1;
        readStep = stride;
        count   = blockHeight;

        for(idx = 0; idx < count; ++idx) {

            *dstPtr = *readPtr;

            dstPtr += dstStep;
            readPtr += readStep;
        }

    }

    return;
}

/*************************************************
 * Neighbor Array Sample Update for 16 bit case
 *************************************************/
void NeighborArrayUnit16bitSampleWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U16               *srcPtr,
    EB_U32               stride,
    EB_U32               srcOriginX,
    EB_U32               srcOriginY,
    EB_U32               picOriginX,
    EB_U32               picOriginY,
    EB_U32               blockWidth,
    EB_U32               blockHeight,
    EB_U32               neighborArrayTypeMask)
{
    EB_U32 idx;
    EB_U16  *dstPtr;
    EB_U16  *readPtr;

    EB_S32 dstStep;
    EB_S32 readStep;
    EB_U32 count;

    // Adjust the Source ptr to start at the origin of the block being updated.
    srcPtr += ((srcOriginY * stride) + srcOriginX)/*CHKN  * naUnitPtr->unitSize*/;

    if(neighborArrayTypeMask & NEIGHBOR_ARRAY_UNIT_TOP_MASK) {

        //
        //     ----------12345678---------------------  Top Neighbor Array
        //                ^    ^
        //                |    |
        //                |    |
        //               xxxxxxxx
        //               x      x
        //               x      x
        //               12345678
        //
        //  The top neighbor array is updated with the samples from the
        //    bottom row of the source block
        //
        //  Index = originX

        // Adjust readPtr to the bottom-row
        readPtr = srcPtr + ((blockHeight - 1) * stride);

        dstPtr = (EB_U16*)(naUnitPtr->topArray) +
                 GetNeighborArrayUnitTopIndex(
                     naUnitPtr,
                     picOriginX);//CHKN * naUnitPtr->unitSize;

        dstStep = naUnitPtr->unitSize;
        readStep = naUnitPtr->unitSize;
        count   = blockWidth;

        for(idx = 0; idx < count; ++idx) {

            *dstPtr = *readPtr;

            dstPtr  += 1;
            readPtr += 1;
        }

    }

    if(neighborArrayTypeMask & NEIGHBOR_ARRAY_UNIT_LEFT_MASK) {

        //   Left Neighbor Array
        //
        //    |
        //    |
        //    1         xxxxxxx1
        //    2  <----  x      2
        //    3  <----  x      3
        //    4         xxxxxxx4
        //    |
        //    |
        //
        //  The left neighbor array is updated with the samples from the
        //    right column of the source block
        //
        //  Index = originY

        // Adjust readPtr to the right-column
        readPtr = srcPtr + (blockWidth - 1);

        dstPtr = (EB_U16*)(naUnitPtr->leftArray) +
                 GetNeighborArrayUnitLeftIndex(
                     naUnitPtr,
                     picOriginY) ;//CHKN * naUnitPtr->unitSize;

        dstStep = 1;
        readStep = stride;
        count   = blockHeight;

        for(idx = 0; idx < count; ++idx) {

            *dstPtr = *readPtr;

            dstPtr += dstStep;
            readPtr += readStep;
        }

    }

    if(neighborArrayTypeMask & NEIGHBOR_ARRAY_UNIT_TOPLEFT_MASK) {

        /*
        //   Top-left Neighbor Array
        //
        //    4-5--6--7--------------
        //    3 \      \
        //    2  \      \
        //    1   \      \
        //    |\   xxxxxx7
        //    | \  x     6
        //    |  \ x     5
        //    |   \1x2x3x4
        //    |
        //
        //  The top-left neighbor array is updated with the reversed samples
        //    from the right column and bottom row of the source block
        //
        // Index = originX - originY
        */

        // Adjust readPtr to the bottom-row
        readPtr = srcPtr + ((blockHeight - 1) * stride);

        // Copy bottom row
        dstPtr =
            (EB_U16*)(naUnitPtr->topLeftArray) +
            GetNeighborArrayUnitTopLeftIndex(
                naUnitPtr,
                picOriginX,
                picOriginY + (blockWidth - 1)) ;//CHKN * naUnitPtr->unitSize;

        dstStep = 1;
        readStep = 1;
        count   = blockWidth;

        for(idx = 0; idx < count; ++idx) {

            *dstPtr = *readPtr;

            dstPtr += dstStep;
            readPtr += readStep;
        }

        // Reset readPtr to the right-column
        readPtr = srcPtr + (blockWidth - 1);

        // Copy right column
        dstPtr =
            (EB_U16*)(naUnitPtr->topLeftArray) +
            GetNeighborArrayUnitTopLeftIndex(
                naUnitPtr,
                picOriginX + (blockWidth - 1),
                picOriginY);//CHKN  * naUnitPtr->unitSize;

        dstStep = -1;
        readStep = stride;
        count   = blockHeight;

        for(idx = 0; idx < count; ++idx) {

            *dstPtr = *readPtr;

            dstPtr += dstStep;
            readPtr += readStep;
        }

    }

    return;
}
/*************************************************
 * Neighbor Array Unit Mode Write
 *************************************************/
void NeighborArrayUnitModeWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *value,
    EB_U32               originX,
    EB_U32               originY,
    EB_U32               blockWidth,
    EB_U32               blockHeight,
    EB_U32               neighborArrayTypeMask)
{
    EB_U32 idx;
    EB_U8  *dstPtr;

    EB_U32 count;
    EB_U32 naOffset;
	EB_U32 naUnitSize;

	naUnitSize = naUnitPtr->unitSize;

    if(neighborArrayTypeMask & NEIGHBOR_ARRAY_UNIT_TOP_MASK) {

        //
        //     ----------12345678---------------------  Top Neighbor Array
        //                ^    ^
        //                |    |
        //                |    |
        //               xxxxxxxx
        //               x      x
        //               x      x
        //               12345678
        //
        //  The top neighbor array is updated with the samples from the
        //    bottom row of the source block
        //
        //  Index = originX

        naOffset = GetNeighborArrayUnitTopIndex(
            naUnitPtr,
            originX);

        dstPtr = naUnitPtr->topArray +
             naOffset * naUnitSize;

        count   = blockWidth >> naUnitPtr->granularityNormalLog2;

        for(idx = 0; idx < count; ++idx) {

            EB_MEMCPY(dstPtr, value, naUnitSize);

            dstPtr += naUnitSize;
        }
    }

    if(neighborArrayTypeMask & NEIGHBOR_ARRAY_UNIT_LEFT_MASK) {

        //   Left Neighbor Array
        //
        //    |
        //    |
        //    1         xxxxxxx1
        //    2  <----  x      2
        //    3  <----  x      3
        //    4         xxxxxxx4
        //    |
        //    |
        //
        //  The left neighbor array is updated with the samples from the
        //    right column of the source block
        //
        //  Index = originY

        naOffset = GetNeighborArrayUnitLeftIndex(
                naUnitPtr,
                originY);

        dstPtr = naUnitPtr->leftArray +
            naOffset * naUnitSize;

        count   = blockHeight >> naUnitPtr->granularityNormalLog2;

        for(idx = 0; idx < count; ++idx) {

            EB_MEMCPY(dstPtr, value, naUnitSize);

            dstPtr += naUnitSize;
        }
    }

    if(neighborArrayTypeMask & NEIGHBOR_ARRAY_UNIT_TOPLEFT_MASK) {

        /*
        //   Top-left Neighbor Array
        //
        //    4-5--6--7------------
        //    3 \      \
        //    2  \      \
        //    1   \      \
        //    |\   xxxxxx7
        //    | \  x     6
        //    |  \ x     5
        //    |   \1x2x3x4
        //    |
        //
        //  The top-left neighbor array is updated with the reversed samples
        //    from the right column and bottom row of the source block
        //
        // Index = originX - originY
        */

        naOffset = GetNeighborArrayUnitTopLeftIndex(
            naUnitPtr,
            originX,
            originY + (blockHeight - 1));

        // Copy bottom-row + right-column
        // *Note - start from the bottom-left corner
        dstPtr = naUnitPtr->topLeftArray +
            naOffset * naUnitSize;

        count   = ((blockWidth + blockHeight) >> naUnitPtr->granularityTopLeftLog2) - 1;

        for(idx = 0; idx < count; ++idx) {

            EB_MEMCPY(dstPtr, value, naUnitSize);

            dstPtr += naUnitSize;
        }
    }

    return;
}
/*************************************************
 * Neighbor Array Unit Depth Skip Write
 *************************************************/
void NeighborArrayUnitDepthSkipWrite(//NeighborArrayUnitDepthWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *value,
    EB_U32               originX,
    EB_U32               originY,
    EB_U32               blockSize)
{

    EB_U8  *dstPtr;
    EB_U8 *naUnittopArray;
	EB_U8 *naUnitleftArray;

    EB_U32 count;

	naUnittopArray = naUnitPtr->topArray;
	naUnitleftArray = naUnitPtr->leftArray;

    //
    //     ----------12345678---------------------  Top Neighbor Array
    //                ^    ^
    //                |    |
    //                |    |
    //               xxxxxxxx
    //               x      x
    //               x      x
    //               12345678
    //
    //  The top neighbor array is updated with the samples from the
    //    bottom row of the source block
    //
    //  Index = originX

    dstPtr = naUnittopArray + (originX >> 3);
    count   = blockSize >> 3;
	EB_MEMSET(dstPtr, *value, count);


    //   Left Neighbor Array
    //
    //    |
    //    |
    //    1         xxxxxxx1
    //    2  <----  x      2
    //    3  <----  x      3
    //    4         xxxxxxx4
    //    |
    //    |
    //
    //  The left neighbor array is updated with the samples from the
    //    right column of the source block
    //
    //  Index = originY


    dstPtr = naUnitleftArray + (originY >> 3);
	EB_MEMSET(dstPtr, *value, count);

    return;
}

/*************************************************
 * Neighbor Array Unit Intra Write
 *************************************************/
void NeighborArrayUnitIntraWrite(//NeighborArrayUnitDepthWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *value,
    EB_U32               originX,
    EB_U32               originY,
    EB_U32               blockSize)
{

    EB_U8  *dstPtr;
    EB_U8 *naUnittopArray;
	EB_U8 *naUnitleftArray;

    EB_U32 count;

	naUnittopArray = naUnitPtr->topArray;
	naUnitleftArray = naUnitPtr->leftArray;


    //
    //     ----------12345678---------------------  Top Neighbor Array
    //                ^    ^
    //                |    |
    //                |    |
    //               xxxxxxxx
    //               x      x
    //               x      x
    //               12345678
    //
    //  The top neighbor array is updated with the samples from the
    //    bottom row of the source block
    //
    //  Index = originX

    dstPtr = naUnittopArray + (originX >> 2);
    count   = blockSize >> 2;
	EB_MEMSET(dstPtr, *value, count);


    //   Left Neighbor Array
    //
    //    |
    //    |
    //    1         xxxxxxx1
    //    2  <----  x      2
    //    3  <----  x      3
    //    4         xxxxxxx4
    //    |
    //    |
    //
    //  The left neighbor array is updated with the samples from the
    //    right column of the source block
    //
    //  Index = originY


    dstPtr = naUnitleftArray + (originY >> 2);
	EB_MEMSET(dstPtr, *value, count);

    return;
}


/*************************************************
 * Neighbor Array Unit Mode Type Write
 *************************************************/
void NeighborArrayUnitModeTypeWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *value,
    EB_U32               originX,
    EB_U32               originY,
    EB_U32               blockSize)
{
    EB_U8  *dstPtr;
    EB_U8  *naUnitTopArray;
	EB_U8  *naUnitLeftArray;
	EB_U8  *naUnitTopLeftArray;

    EB_U32 count;
    EB_U32 naOffset;
	naUnitTopArray = naUnitPtr->topArray;
	naUnitLeftArray = naUnitPtr->leftArray;
	naUnitTopLeftArray = naUnitPtr->topLeftArray;


    //
    //     ----------12345678---------------------  Top Neighbor Array
    //                ^    ^
    //                |    |
    //                |    |
    //               xxxxxxxx
    //               x      x
    //               x      x
    //               12345678
    //
    //  The top neighbor array is updated with the samples from the
    //    bottom row of the source block
    //
    //  Index = originX

    naOffset = originX >> 2;
    dstPtr = naUnitTopArray + naOffset;
    count   = blockSize >> 2;

	EB_MEMSET(dstPtr, *value, count);


    //   Left Neighbor Array
    //
    //    |
    //    |
    //    1         xxxxxxx1
    //    2  <----  x      2
    //    3  <----  x      3
    //    4         xxxxxxx4
    //    |
    //    |
    //
    //  The left neighbor array is updated with the samples from the
    //    right column of the source block
    //
    //  Index = originY

    naOffset = originY >> 2;
    dstPtr = naUnitLeftArray + naOffset;

	EB_MEMSET(dstPtr, *value, count);


    /*
    //   Top-left Neighbor Array
    //
    //    4-5--6--7------------
    //    3 \      \
    //    2  \      \
    //    1   \      \
    //    |\   xxxxxx7
    //    | \  x     6
    //    |  \ x     5
    //    |   \1x2x3x4
    //    |
    //
    //  The top-left neighbor array is updated with the reversed samples
    //    from the right column and bottom row of the source block
    //
    // Index = originX - originY
    */

    naOffset = GetNeighborArrayUnitTopLeftIndex(
        naUnitPtr,
        originX,
        originY + (blockSize - 1));

    // Copy bottom-row + right-column
    // *Note - start from the bottom-left corner
    dstPtr = naUnitTopLeftArray + naOffset;
    count  = ((blockSize + blockSize) >> 2) - 1;

	EB_MEMSET(dstPtr, *value, count);


    return;
}


/*************************************************
 * Neighbor Array Unit Mode Write
 *************************************************/
void NeighborArrayUnitMvWrite(
    NeighborArrayUnit_t *naUnitPtr,
    EB_U8               *value,
    EB_U32               originX,
    EB_U32               originY,
	EB_U32               blockSize)
{
    EB_U32 idx;
    EB_U8  *dstPtr;
    EB_U8   *naUnittopArray;
    EB_U8   *naUnitleftArray;
    EB_U8   *naUnittopLeftArray;

    EB_U32 count;
    EB_U32 naOffset;
	EB_U32 naUnitSize;

	naUnitSize = naUnitPtr->unitSize;
	naUnittopArray = naUnitPtr->topArray;
	naUnitleftArray = naUnitPtr->leftArray;
	naUnittopLeftArray = naUnitPtr->topLeftArray;


    //
    //     ----------12345678---------------------  Top Neighbor Array
    //                ^    ^
    //                |    |
    //                |    |
    //               xxxxxxxx
    //               x      x
    //               x      x
    //               12345678
    //
    //  The top neighbor array is updated with the samples from the
    //    bottom row of the source block
    //
    //  Index = originX

    naOffset = originX >> 2 ;

    dstPtr = naUnittopArray +
         naOffset * naUnitSize;

    //dstStep = naUnitSize;
    count   = blockSize >> 2;

    for(idx = 0; idx < count; ++idx) {

        EB_MEMCPY(dstPtr, value, naUnitSize);

        dstPtr += naUnitSize;
    }


    //   Left Neighbor Array
    //
    //    |
    //    |
    //    1         xxxxxxx1
    //    2  <----  x      2
    //    3  <----  x      3
    //    4         xxxxxxx4
    //    |
    //    |
    //
    //  The left neighbor array is updated with the samples from the
    //    right column of the source block
    //
    //  Index = originY

    naOffset = originY >> 2 ;

    dstPtr = naUnitleftArray +
        naOffset * naUnitSize;


    for(idx = 0; idx < count; ++idx) {

        EB_MEMCPY(dstPtr, value, naUnitSize);

        dstPtr += naUnitSize;
    }


    /*
    //   Top-left Neighbor Array
    //
    //    4-5--6--7------------
    //    3 \      \
    //    2  \      \
    //    1   \      \
    //    |\   xxxxxx7
    //    | \  x     6
    //    |  \ x     5
    //    |   \1x2x3x4
    //    |
    //
    //  The top-left neighbor array is updated with the reversed samples
    //    from the right column and bottom row of the source block
    //
    // Index = originX - originY
    */

    naOffset = GetNeighborArrayUnitTopLeftIndex(
        naUnitPtr,
        originX,
        originY + (blockSize - 1));

    // Copy bottom-row + right-column
    // *Note - start from the bottom-left corner
    dstPtr = naUnittopLeftArray +
        naOffset * naUnitSize;

    count   = ((blockSize + blockSize) >> 2) - 1;

    for(idx = 0; idx < count; ++idx) {

        EB_MEMCPY(dstPtr, value, naUnitSize);

        dstPtr += naUnitSize;
    }

    return;
}
