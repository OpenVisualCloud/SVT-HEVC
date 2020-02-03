/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#include "EbAvailability.h"

// Bottom left availability function
EB_BOOL  isBottomLeftAvailable(EB_U32 depth,
                                      EB_U32 partIndex)
{
    EB_BOOL  availability;

    EB_U32   numberOfPartitionsInWidth  = 1 << depth;
    EB_U32   horizontalPartitionIndex   = partIndex & (numberOfPartitionsInWidth - 1);
    EB_U32   verticalPartitionIndex     = (partIndex - horizontalPartitionIndex) >> depth;

    if (depth < 4) {

        availability    =   (EB_BOOL)  ((horizontalPartitionIndex    &   (numberOfPartitionsInWidth - 1)) == 0);                                                                    // left column border check
        availability    =   (EB_BOOL)  (availability |  ((((horizontalPartitionIndex)  &   0x01) == 0) && (((verticalPartitionIndex)   &   0x01) == 0)));                           // x is even and y is even
        availability    =   (EB_BOOL)  (availability |  ((((horizontalPartitionIndex)  &   0x03) == 0) && (((verticalPartitionIndex)   &   0x03) == 0x01)));                        // x is a multiple of 4  and y mod 4 is equal to 1
        availability    =   (EB_BOOL)  (availability &  (((verticalPartitionIndex != (numberOfPartitionsInWidth - 1)))));                                                           // bottom row border check

    } else {

        availability    =   (EB_BOOL)  ((horizontalPartitionIndex    &   (numberOfPartitionsInWidth - 1)) == 0);                                                                    // left column border check
        availability    =   (EB_BOOL)  (availability |  ((((horizontalPartitionIndex)  &   0x01) == 0) && (((verticalPartitionIndex) & 0x01) == 0)));                               // x is even and y is even
        availability    =   (EB_BOOL)  (availability |  ((((horizontalPartitionIndex)  &   0x03) == 0) && (((verticalPartitionIndex + 1) & 0x03) != 0x00)));                        // x is a multiple of 4  and (y + 1) % 4 is different from 1
        availability    =   (EB_BOOL)  (availability |  ((((horizontalPartitionIndex)  &   0x07) == 0) && (verticalPartitionIndex != ((numberOfPartitionsInWidth >> 1) - 1))));     // x is a multiple of 8  and y is different from ((numberOfPartitionsInWidth) / 2 - 1)
        availability    =   (EB_BOOL)  (availability &  (((verticalPartitionIndex != (numberOfPartitionsInWidth - 1)))));                                                           // bottom row border check
    }

    return availability;
}

// Upper right availability function
EB_BOOL isUpperRightAvailable(
    EB_U32 depth,
    EB_U32 partIndex)
{
    EB_BOOL  availability;

    EB_U32   numberOfPartitionsInWidth  = 1 << depth;
    EB_U32   horizontalPartitionIndex   = partIndex & (numberOfPartitionsInWidth - 1);
    EB_U32   verticalPartitionIndex     = (partIndex - horizontalPartitionIndex) >> depth;

    if (depth < 4) {

        availability    =   (EB_BOOL)  (horizontalPartitionIndex == (numberOfPartitionsInWidth - 1));                                                                               // right column border check
        availability    =   (EB_BOOL)  (availability |  ((((horizontalPartitionIndex)  &   0x01) == 0x01) && (((verticalPartitionIndex)    &   0x01) == 0x01)));                    // x is odd and y is odd
        availability    =   (EB_BOOL)  (availability |  ((((horizontalPartitionIndex)  &   0x03) == 0x03) && (((verticalPartitionIndex)    &   0x03) == 0x02)));                    // x mod 4 is equal to 3 and y mod 4 is equal to 2
        availability    =   (EB_BOOL)  (availability &  (verticalPartitionIndex != 0));                                                                                             // top row border check

    } else {

        availability    =   (EB_BOOL) (horizontalPartitionIndex == (numberOfPartitionsInWidth - 1));                                                                                // right column border check
        availability    =   (EB_BOOL)  (availability |  ((((horizontalPartitionIndex)  &   0x01) == 0x01) && (((verticalPartitionIndex)    &   0x01) ==0x01)));                     // x is odd and y is odd
        availability    =   (EB_BOOL)  (availability |  ((((horizontalPartitionIndex)  &   0x03) == 0x03) && (verticalPartitionIndex % (numberOfPartitionsInWidth >> 2) != 0)));    // x mod 4 is equal to 3 and y is a multiple of (numberOfPartitionsInWidth / 4)
        availability    =   (EB_BOOL)  (availability |  ((((horizontalPartitionIndex)  &   0x07) == 0x07) && (verticalPartitionIndex != (numberOfPartitionsInWidth >> 1))));        // x mod 8 is equal to 7 and y is different from (numberOfPartitionsInWidth / 2)
        availability    =   (EB_BOOL)  (availability &  (verticalPartitionIndex != 0));                                                                                             // top row border check
    }

    return (EB_BOOL)(!availability);
}