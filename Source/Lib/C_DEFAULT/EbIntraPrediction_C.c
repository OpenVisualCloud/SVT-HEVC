/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

/** IntraModeVerticalLuma()
is used to compute the prediction for Intra luma vertical mode
*/

#include "EbIntraPrediction_C.h"
#include "EbUtility.h"
#include "EbDefinitions.h"


void IntraModeVerticalLuma(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 leftOffset = 0;
    EB_U32 topLeftOffset = size << 1;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size-1]   = Left[size-1]
    // ... (arbitrary value)
    // refSamples[2*size]   = TopLeft[0]
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size]   = Top[size-1]
    // -----------------------------------------------

    for (columnIndex = 0; columnIndex < size; ++columnIndex) {
        writeIndex = columnIndex;
        for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
            predictionPtr[writeIndex] = refSamples[topOffset + columnIndex];
            writeIndex += rowStride * predictionBufferStride;
        }
    }

    // Filter prediction samples of the first column for size 4, 8, 16
    if (size < 32) {
        writeIndex = 0;
        for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
            predictionPtr[writeIndex] = CLIP3(0, MAX_SAMPLE_VALUE, (predictionPtr[writeIndex] + ((refSamples[leftOffset + rowIndex] - refSamples[topLeftOffset]) >> 1)));
            writeIndex += rowStride * predictionBufferStride;
        }
    }
}
/** IntraModeVerticalLuma16bit()
is used to compute the prediction for Intra luma vertical mode
*/
void IntraModeVerticalLuma16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 leftOffset = 0;
    EB_U32 topLeftOffset = size << 1;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size-1]   = Left[size-1]
    // ... (arbitrary value)
    // refSamples[2*size]   = TopLeft[0]
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size]   = Top[size-1]
    // -----------------------------------------------

    for (columnIndex = 0; columnIndex < size; ++columnIndex) {
        writeIndex = columnIndex;
        for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
            predictionPtr[writeIndex] = refSamples[topOffset + columnIndex];
            writeIndex += rowStride * predictionBufferStride;
        }
    }

    // Filter prediction samples of the first column for size 4, 8, 16
    if (size < 32) {
        writeIndex = 0;
        for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
            predictionPtr[writeIndex] = CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (predictionPtr[writeIndex] + ((refSamples[leftOffset + rowIndex] - refSamples[topLeftOffset]) >> 1)));
            writeIndex += rowStride * predictionBufferStride;
        }
    }
}

/** IntraModeVerticalChroma()
is used to compute the prediction for Intra chroma vertical mode
*/

void IntraModeVerticalChroma(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // ... (arbitrary value)
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size]   = Top[size-1]
    // -----------------------------------------------

    for (columnIndex = 0; columnIndex < size; ++columnIndex) {
        writeIndex = columnIndex;
        for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
            predictionPtr[writeIndex] = refSamples[topOffset + columnIndex];
            writeIndex += rowStride * predictionBufferStride;
        }
    }
}
/** IntraModeVerticalChroma16bit()
is used to compute the prediction for Intra chroma vertical mode
*/

void IntraModeVerticalChroma16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // ... (arbitrary value)
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size]   = Top[size-1]
    // -----------------------------------------------

    for (columnIndex = 0; columnIndex < size; ++columnIndex) {
        writeIndex = columnIndex;
        for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
            predictionPtr[writeIndex] = refSamples[topOffset + columnIndex];
            writeIndex += rowStride * predictionBufferStride;
        }
    }
}
/** IntraModeHorizontalLuma()
is used to compute the prediction for Intra luma horizontal mode
*/
void IntraModeHorizontalLuma(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 leftOffset = 0;
    EB_U32 topLeftOffset = size << 1;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------    
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size-1]   = Left[size-1]
    // ... (arbitrary value)
    // refSamples[2*size]   = TopLeft[0]
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size]   = Top[size-1]
    // -----------------------------------------------

    for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
        writeIndex = rowIndex * predictionBufferStride;
        for (columnIndex = 0; columnIndex < size; ++columnIndex) {
            predictionPtr[writeIndex] = refSamples[leftOffset + rowIndex];
            ++writeIndex;
        }
    }

    // Filter prediction samples of the first row for size 4, 8, 16
    if (size < 32) {
        writeIndex = 0;
        for (columnIndex = 0; columnIndex < size; ++columnIndex) {
            predictionPtr[writeIndex] = CLIP3(0, MAX_SAMPLE_VALUE, (predictionPtr[writeIndex] + ((refSamples[topLeftOffset + columnIndex + 1] - refSamples[topLeftOffset]) >> 1)));
            ++writeIndex;
        }
    }

    return;
}
/** IntraModeHorizontalLuma16bit()
is used to compute the prediction for Intra luma horizontal mode
*/
void IntraModeHorizontalLuma16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 leftOffset = 0;
    EB_U32 topLeftOffset = size << 1;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------    
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size-1]   = Left[size-1]
    // ... (arbitrary value)
    // refSamples[2*size]   = TopLeft[0]
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size]   = Top[size-1]
    // -----------------------------------------------

    for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
        writeIndex = rowIndex * predictionBufferStride;
        for (columnIndex = 0; columnIndex < size; ++columnIndex) {
            predictionPtr[writeIndex] = refSamples[leftOffset + rowIndex];
            ++writeIndex;
        }
    }

    // Filter prediction samples of the first row for size 4, 8, 16
    if (size < 32) {
        writeIndex = 0;
        for (columnIndex = 0; columnIndex < size; ++columnIndex) {
            predictionPtr[writeIndex] = CLIP3(0, MAX_SAMPLE_VALUE_10BIT, (predictionPtr[writeIndex] + ((refSamples[topLeftOffset + columnIndex + 1] - refSamples[topLeftOffset]) >> 1)));
            ++writeIndex;
        }
    }

    return;
}
/** IntraModeHorizontalChroma()
is used to compute the prediction for Intra chroma horizontal mode
*/
void IntraModeHorizontalChroma(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 leftOffset = 0;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size-1]   = Left[size-1]
    // ... (arbitrary value)
    // -----------------------------------------------

    for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
        writeIndex = rowIndex * predictionBufferStride;
        for (columnIndex = 0; columnIndex < size; ++columnIndex) {
            predictionPtr[writeIndex] = refSamples[leftOffset + rowIndex];
            ++writeIndex;
        }
    }

    return;
}
/** IntraModeHorizontalChroma16bit()
is used to compute the prediction for Intra chroma horizontal mode
*/
void IntraModeHorizontalChroma16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 leftOffset = 0;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size-1]   = Left[size-1]
    // ... (arbitrary value)
    // -----------------------------------------------

    for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
        writeIndex = rowIndex * predictionBufferStride;
        for (columnIndex = 0; columnIndex < size; ++columnIndex) {
            predictionPtr[writeIndex] = refSamples[leftOffset + rowIndex];
            ++writeIndex;
        }
    }

    return;
}

/** IntraModeDCLuma()
is used to compute the prediction for Intra luma DC mode
*/
void IntraModeDCLuma(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{

    EB_U32 sum = 0;
    EB_U32 index;
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 leftOffset = 0;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U8  predictionDcValue = 128; // needs to be changed to a macro based on bit depth
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------   
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size-1]   = Left[size-1]
    // ... (arbitrary value)
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size]   = Top[size-1]
    // -----------------------------------------------

    // top reference samples
    for (index = 0; index< size; index++) {
        sum += refSamples[topOffset + index];
    }

    // left reference samples
    for (index = 0; index< size; index++) {
        sum += refSamples[leftOffset + index];
    }

    predictionDcValue = (EB_U8)((sum + size) >> Log2f(size << 1));

    // Generate the prediction
    for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
        writeIndex = rowIndex * predictionBufferStride;
        for (columnIndex = 0; columnIndex < size; ++columnIndex) {
            predictionPtr[writeIndex] = predictionDcValue;
            ++writeIndex;
        }
    }

    // Perform DC filtering on boundary pixels for all sizes 4, 8, 16
    if (size < 32) {
        predictionPtr[0] = (EB_U8)((refSamples[leftOffset] + refSamples[topOffset] + (predictionPtr[0] << 1) + 2) >> 2);

        // first row
        for (columnIndex = 1; columnIndex<size; columnIndex++) {
            predictionPtr[columnIndex] = (EB_U8)((refSamples[topOffset + columnIndex] + 3 * predictionPtr[columnIndex] + 2) >> 2);
        }

        //first col
        for (rowIndex = rowStride; rowIndex<size; rowIndex += rowStride) {
            writeIndex = rowIndex*predictionBufferStride;
            predictionPtr[writeIndex] = (EB_U8)((refSamples[leftOffset + rowIndex] + 3 * predictionPtr[writeIndex] + 2) >> 2);
        }
    }

    return;
}
/** IntraModeDCLuma16bit()
is used to compute the prediction for Intra luma DC mode
*/
void IntraModeDCLuma16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{

    EB_U32 sum = 0;
    EB_U32 index;
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 leftOffset = 0;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U16  predictionDcValue = 128; // needs to be changed to a macro based on bit depth
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------   
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size-1]   = Left[size-1]
    // ... (arbitrary value)
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size]   = Top[size-1]
    // -----------------------------------------------

    // top reference samples
    for (index = 0; index< size; index++) {
        sum += refSamples[topOffset + index];
    }

    // left reference samples
    for (index = 0; index< size; index++) {
        sum += refSamples[leftOffset + index];
    }

    predictionDcValue = (EB_U16)((sum + size) >> Log2f(size << 1));

    // Generate the prediction
    for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
        writeIndex = rowIndex * predictionBufferStride;
        for (columnIndex = 0; columnIndex < size; ++columnIndex) {
            predictionPtr[writeIndex] = predictionDcValue;
            ++writeIndex;
        }
    }

    // Perform DC filtering on boundary pixels for all sizes 4, 8, 16
    if (size < 32) {
        predictionPtr[0] = (EB_U16)((refSamples[leftOffset] + refSamples[topOffset] + (predictionPtr[0] << 1) + 2) >> 2);

        // first row
        for (columnIndex = 1; columnIndex<size; columnIndex++) {
            predictionPtr[columnIndex] = (EB_U16)((refSamples[topOffset + columnIndex] + 3 * predictionPtr[columnIndex] + 2) >> 2);
        }

        //first col
        for (rowIndex = rowStride; rowIndex<size; rowIndex += rowStride) {
            writeIndex = rowIndex*predictionBufferStride;
            predictionPtr[writeIndex] = (EB_U16)((refSamples[leftOffset + rowIndex] + 3 * predictionPtr[writeIndex] + 2) >> 2);
        }
    }

    return;
}

/** IntraModeDCChroma()
is used to compute the prediction for Intra chroma DC mode
*/
void IntraModeDCChroma(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{

    EB_U32 sum = 0;
    EB_U32 index;
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 leftOffset = 0;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U32 predictionDcValue = 128; // needs to be changed to a macro based on bit depth
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size-1]   = Left[size-1]
    // ... (arbitrary value)
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size]   = Top[size-1]
    // -----------------------------------------------

    // top reference samples
    for (index = 0; index< size; index++) {
        sum += refSamples[topOffset + index];
    }

    // left reference samples
    for (index = 0; index< size; index++) {
        sum += refSamples[leftOffset + index];
    }

    predictionDcValue = (EB_U8)((sum + size) >> Log2f(size << 1));

    // Generate the prediction
    for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
        writeIndex = rowIndex * predictionBufferStride;
        for (columnIndex = 0; columnIndex < size; ++columnIndex) {
            predictionPtr[writeIndex] = (EB_U8)predictionDcValue;
            ++writeIndex;
        }
    }

    return;
}
/** IntraModeDCChroma16bit()
is used to compute the prediction for Intra chroma DC mode
*/
void IntraModeDCChroma16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{

    EB_U32 sum = 0;
    EB_U32 index;
    EB_U32 columnIndex, rowIndex;
    EB_U32 writeIndex;
    EB_U32 leftOffset = 0;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U32 predictionDcValue = 128; // needs to be changed to a macro based on bit depth
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size-1]   = Left[size-1]
    // ... (arbitrary value)
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size]   = Top[size-1]
    // -----------------------------------------------

    // top reference samples
    for (index = 0; index< size; index++) {
        sum += refSamples[topOffset + index];
    }

    // left reference samples
    for (index = 0; index< size; index++) {
        sum += refSamples[leftOffset + index];
    }

    predictionDcValue = (EB_U16)((sum + size) >> Log2f(size << 1));

    // Generate the prediction
    for (rowIndex = 0; rowIndex < size; rowIndex += rowStride) {
        writeIndex = rowIndex * predictionBufferStride;
        for (columnIndex = 0; columnIndex < size; ++columnIndex) {
            predictionPtr[writeIndex] = (EB_U16)predictionDcValue;
            ++writeIndex;
        }
    }

    return;
}

/** IntraModePlanar()
is used to compute the prediction for Intra planar mode
*/
void IntraModePlanar(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 leftOffset = 0;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U8  topRightPel, bottomLeftPel, leftPel;
    EB_U32 x, y;
    EB_U32 shift = Log2f(size) + 1;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // (refSamples are similar as vertical mode)
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size]     = Left[size]
    // ... (arbitrary value)
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size+1] = Top[size]
    // -----------------------------------------------
    // Get above and left reference samples
    topRightPel = refSamples[topOffset + size];
    bottomLeftPel = refSamples[leftOffset + size];

    // Generate prediction
    for (y = 0; y < size; y += rowStride) {
        leftPel = refSamples[leftOffset + y];
        for (x = 0; x < size; x++) {

            predictionPtr[x] = (EB_U8)(((size - 1 - x)*leftPel + (x + 1)*topRightPel + (size - 1 - y)*refSamples[topOffset + x] + (y + 1)*bottomLeftPel + size) >> shift);
        }
        predictionPtr += rowStride * predictionBufferStride;
    }

    return;
}

/** IntraModePlanar16bit()
is used to compute the prediction for Intra planar mode
*/

void IntraModePlanar16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)                       //skip half rows
{
    EB_U32 leftOffset = 0;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U16  topRightPel, bottomLeftPel, leftPel;
    EB_U32 x, y;
    EB_U32 shift = Log2f(size) + 1;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // (refSamples are similar as vertical mode)
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[size]     = Left[size]
    // ... (arbitrary value)
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size+1] = Top[size]
    // -----------------------------------------------
    // Get above and left reference samples
    topRightPel = refSamples[topOffset + size];
    bottomLeftPel = refSamples[leftOffset + size];

    // Generate prediction
    for (y = 0; y < size; y += rowStride) {
        leftPel = refSamples[leftOffset + y];
        for (x = 0; x < size; x++) {

            predictionPtr[x] = (EB_U16)(((size - 1 - x)*leftPel + (x + 1)*topRightPel + (size - 1 - y)*refSamples[topOffset + x] + (y + 1)*bottomLeftPel + size) >> shift);
        }
        predictionPtr += rowStride * predictionBufferStride;
    }

    return;
}


/** IntraModeAngular_34()
is used to compute the prediction for Intra angular mode 34
*/
void IntraModeAngular_34(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)
{
    EB_U32 rowIndex, colIndex;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U32 rowStride = skip ? 2 : 1;
    (void)skip;

    // --------- Reference Samples Structure ---------
    // ... (arbitrary value)
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[4*size]   = Top[2*size-1]
    // -----------------------------------------------

    // Compute the prediction
    for (rowIndex = 0; rowIndex<size; rowIndex += rowStride) {
        for (colIndex = 0; colIndex<size; colIndex++) {
            predictionPtr[colIndex] = refSamples[topOffset + rowIndex + colIndex + 1];
        }
        predictionPtr += rowStride * predictionBufferStride;
    }

    return;
}
/** IntraModeAngular16bit_34()
is used to compute the prediction for Intra angular mode 34
*/
void IntraModeAngular16bit_34(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)
{
    EB_U32 rowIndex, colIndex;
    EB_U32 topOffset = (size << 1) + 1;
    EB_U32 rowStride = skip ? 2 : 1;
    (void)skip;

    // --------- Reference Samples Structure ---------
    // ... (arbitrary value)
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[4*size]   = Top[2*size-1]
    // -----------------------------------------------

    // Compute the prediction
    for (rowIndex = 0; rowIndex<size; rowIndex += rowStride) {
        for (colIndex = 0; colIndex<size; colIndex++) {
            predictionPtr[colIndex] = refSamples[topOffset + rowIndex + colIndex + 1];
        }
        predictionPtr += rowStride * predictionBufferStride;
    }

    return;
}

/** IntraModeAngular_18()
is used to compute the prediction for Intra angular mode 18
*/
void IntraModeAngular_18(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)
{
    EB_U32 rowIndex, colIndex;
    EB_U32 topLeftOffset = size << 1;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // ... (arbitrary value)
    // refSamples[size+1]   = Left[size-2]
    // refSamples[size+2]   = Left[size-3]
    // ...
    // refSamples[2*size-1] = Left[0]
    // refSamples[2*size]   = TopLeft[0]
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size-1] = Top[size-2]
    // ... (arbitrary value)
    // -----------------------------------------------

    // Compute the prediction
    for (rowIndex = 0; rowIndex<size; rowIndex += rowStride) {
        for (colIndex = 0; colIndex<size; colIndex++) {
            predictionPtr[colIndex] = refSamples[topLeftOffset - rowIndex + colIndex];
        }
        predictionPtr += rowStride * predictionBufferStride;
    }

    return;
}
/** IntraModeAngular16bit_18()
is used to compute the prediction for Intra angular mode 18
*/
void IntraModeAngular16bit_18(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)
{
    EB_U32 rowIndex, colIndex;
    EB_U32 topLeftOffset = size << 1;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // ... (arbitrary value)
    // refSamples[size+1]   = Left[size-2]
    // refSamples[size+2]   = Left[size-3]
    // ...
    // refSamples[2*size-1] = Left[0]
    // refSamples[2*size]   = TopLeft[0]
    // refSamples[2*size+1] = Top[0]
    // refSamples[2*size+2] = Top[1]
    // ...
    // refSamples[3*size-1] = Top[size-2]
    // ... (arbitrary value)
    // -----------------------------------------------

    // Compute the prediction
    for (rowIndex = 0; rowIndex<size; rowIndex += rowStride) {
        for (colIndex = 0; colIndex<size; colIndex++) {
            predictionPtr[colIndex] = refSamples[topLeftOffset - rowIndex + colIndex];
        }
        predictionPtr += rowStride * predictionBufferStride;
    }

    return;
}

/** IntraModeAngular_2()
is used to compute the prediction for Intra angular mode2
*/
void IntraModeAngular_2(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)
{
    EB_U32 rowIndex, colIndex;
    EB_U32 leftOffset = 0;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[2*size-1] = Left[2*size-1]
    // -----------------------------------------------
    // Compute the prediction
    for (rowIndex = 0; rowIndex<size; rowIndex += rowStride) {
        for (colIndex = 0; colIndex<size; colIndex++) {
            predictionPtr[colIndex] = refSamples[leftOffset + rowIndex + colIndex + 1];
        }
        predictionPtr += rowStride * predictionBufferStride;
    }

    return;
}
/** IntraModeAngular16bit_2()
is used to compute the prediction for Intra angular mode2
*/
void IntraModeAngular16bit_2(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip)
{
    EB_U32 rowIndex, colIndex;
    EB_U32 leftOffset = 0;
    EB_U32 rowStride = skip ? 2 : 1;

    // --------- Reference Samples Structure ---------
    // refSamples[0]        = Left[0]
    // refSamples[1]        = Left[1]
    // ...
    // refSamples[2*size-1] = Left[2*size-1]
    // -----------------------------------------------
    // Compute the prediction
    for (rowIndex = 0; rowIndex<size; rowIndex += rowStride) {
        for (colIndex = 0; colIndex<size; colIndex++) {
            predictionPtr[colIndex] = refSamples[leftOffset + rowIndex + colIndex + 1];
        }
        predictionPtr += rowStride * predictionBufferStride;
    }

    return;
}


/** IntraModeAngular_Vertical_Kernel()
is used to compute the prediction for Intra angular mode 33-27 and 19-25
*/
void IntraModeAngular_Vertical_Kernel(
    EB_U32         size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSampMain,                //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip,
    EB_S32   intraPredAngle)
{
    EB_U32 rowIndex, colIndex;
    EB_U32 rowStride = skip ? 2 : 1;
    EB_S32 deltaSum = intraPredAngle;
    EB_S32 deltaInt;
    EB_U32 deltaFract;

    // --------- Reference Samples Structure ---------
    // refSampMain[-size+1] to refSampMain[-1] must be prepared (from bottom to top) for mode 19 to 25 (not required for mode 27 to 33)
    // refSampMain[0]      = TopLeft[0]
    // refSampMain[1]      = Top[0]
    // refSampMain[2]      = Top[1]
    // ...
    // refSampMain[size]   = Top[size-1]
    // refSampMain[size+1] = Top[size]     for mode 27 to 33 (not required for mode 19 to 25)
    // ...
    // refSampMain[2*size] = Top[2*size-1] for mode 27 to 33 (not required for mode 19 to 25)
    // -----------------------------------------------

    // Compute the prediction
    refSampMain += 1; // top sample
    for (rowIndex = 0; rowIndex<size; rowIndex += rowStride) {
        deltaInt = deltaSum >> 5;
        deltaFract = deltaSum & 31;
        for (colIndex = 0; colIndex<size; colIndex++) {
            predictionPtr[colIndex] = (EB_U8)(((32 - deltaFract)*refSampMain[deltaInt + (EB_S32)colIndex] + deltaFract*refSampMain[deltaInt + (EB_S32)colIndex + 1] + 16) >> 5);
        }
        predictionPtr += rowStride * predictionBufferStride;
        deltaSum += rowStride * intraPredAngle;
    }
}
/** IntraModeAngular16bit_Vertical_Kernel()
is used to compute the prediction for Intra angular mode 33-27 and 19-25
*/
void IntraModeAngular16bit_Vertical_Kernel(
    EB_U32         size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSampMain,                //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip,
    EB_S32   intraPredAngle)
{
    EB_U32 rowIndex, colIndex;
    EB_U32 rowStride = skip ? 2 : 1;
    EB_S32 deltaSum = intraPredAngle;
    EB_S32 deltaInt;
    EB_U32 deltaFract;

    // --------- Reference Samples Structure ---------
    // refSampMain[-size+1] to refSampMain[-1] must be prepared (from bottom to top) for mode 19 to 25 (not required for mode 27 to 33)
    // refSampMain[0]      = TopLeft[0]
    // refSampMain[1]      = Top[0]
    // refSampMain[2]      = Top[1]
    // ...
    // refSampMain[size]   = Top[size-1]
    // refSampMain[size+1] = Top[size]     for mode 27 to 33 (not required for mode 19 to 25)
    // ...
    // refSampMain[2*size] = Top[2*size-1] for mode 27 to 33 (not required for mode 19 to 25)
    // -----------------------------------------------

    // Compute the prediction
    refSampMain += 1; // top sample
    for (rowIndex = 0; rowIndex<size; rowIndex += rowStride) {
        deltaInt = deltaSum >> 5;
        deltaFract = deltaSum & 31;
        for (colIndex = 0; colIndex<size; colIndex++) {
            predictionPtr[colIndex] = (EB_U16)(((32 - deltaFract)*refSampMain[deltaInt + (EB_S32)colIndex] + deltaFract*refSampMain[deltaInt + (EB_S32)colIndex + 1] + 16) >> 5);
        }
        predictionPtr += rowStride * predictionBufferStride;
        deltaSum += rowStride * intraPredAngle;
    }
}

/** IntraModeAngular_Horizontal_Kernel()
is used to compute the prediction for Intra angular mode 11-17 and 3-9
*/
void IntraModeAngular_Horizontal_Kernel(
    EB_U32         size,                       //input parameter, denotes the size of the current PU
    EB_U8         *refSampMain,                //input parameter, pointer to the reference samples
    EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
    EB_U32         predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip,
    EB_S32         intraPredAngle)
{
    EB_U32 rowIndex, colIndex;
    EB_U32 rowStride = skip ? 2 : 1;
    EB_S32 deltaSum = 0;
    EB_S32 deltaInt;
    EB_U32 deltaFract;

    // --------- Reference Samples Structure ---------
    // refSampMain[-size+1] to refSampMain[-1] must be prepared (from right to left) for mode 11 to 17 (not required for mode 3 to 9)
    // refSampMain[0]      = TopLeft[0]
    // refSampMain[1]      = Left[0]
    // refSampMain[2]      = Left[1]
    // ...
    // refSampMain[size]   = Left[size-1]
    // refSampMain[size+1] = Left[size]     for mode 3 to 9 (not required for mode 11 to 17)
    // ...
    // refSampMain[2*size] = Left[2*size-1] for mode 3 to 9 (not required for mode 11 to 17)
    // -----------------------------------------------

    // Compute the prediction
    refSampMain += 1; // left sample
    for (colIndex = 0; colIndex<size; colIndex++) {
        deltaSum += intraPredAngle;
        deltaInt = deltaSum >> 5;
        deltaFract = deltaSum & 31;
        for (rowIndex = 0; rowIndex<size; rowIndex += rowStride) {
            predictionPtr[rowIndex*predictionBufferStride] = (EB_U8)(((32 - deltaFract)*refSampMain[deltaInt + (EB_S32)rowIndex] + deltaFract*refSampMain[deltaInt + (EB_S32)rowIndex + 1] + 16) >> 5);
        }
        predictionPtr++;
    }
}
/** IntraModeAngular16bit_Horizontal_Kernel()
is used to compute the prediction for Intra angular mode 11-17 and 3-9
*/
void IntraModeAngular16bit_Horizontal_Kernel(
    EB_U32         size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSampMain,                //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    EB_U32         predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip,
    EB_S32         intraPredAngle)
{
    EB_U32 rowIndex, colIndex;
    EB_U32 rowStride = skip ? 2 : 1;
    EB_S32 deltaSum = 0;
    EB_S32 deltaInt;
    EB_U32 deltaFract;

    // --------- Reference Samples Structure ---------
    // refSampMain[-size+1] to refSampMain[-1] must be prepared (from right to left) for mode 11 to 17 (not required for mode 3 to 9)
    // refSampMain[0]      = TopLeft[0]
    // refSampMain[1]      = Left[0]
    // refSampMain[2]      = Left[1]
    // ...
    // refSampMain[size]   = Left[size-1]
    // refSampMain[size+1] = Left[size]     for mode 3 to 9 (not required for mode 11 to 17)
    // ...
    // refSampMain[2*size] = Left[2*size-1] for mode 3 to 9 (not required for mode 11 to 17)
    // -----------------------------------------------

    // Compute the prediction
    refSampMain += 1; // left sample
    for (colIndex = 0; colIndex<size; colIndex++) {
        deltaSum += intraPredAngle;
        deltaInt = deltaSum >> 5;
        deltaFract = deltaSum & 31;
        for (rowIndex = 0; rowIndex<size; rowIndex += rowStride) {
            predictionPtr[rowIndex*predictionBufferStride] = (EB_U16)(((32 - deltaFract)*refSampMain[deltaInt + (EB_S32)rowIndex] + deltaFract*refSampMain[deltaInt + (EB_S32)rowIndex + 1] + 16) >> 5);
        }
        predictionPtr++;
    }
}