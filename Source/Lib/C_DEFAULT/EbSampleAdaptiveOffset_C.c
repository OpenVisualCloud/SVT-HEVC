/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/


#include "EbSampleAdaptiveOffset_C.h"
#include "EbUtility.h"

static int clipToSigned8Bit(int x)
{
#if 1
    if (x < -128) return -128;
    if (x > 127) return 127;
#endif
    return x;
}

/********************************************
* GatherSaoStatisticsLcu_62x62_16bit
* collects Sao Statistics
********************************************/
EB_ERRORTYPE GatherSaoStatisticsLcu_62x62_16bit(
    EB_U16                   *inputSamplePtr,        // input parameter, source Picture Ptr
    EB_U32                   inputStride,           // input parameter, source stride
    EB_U16                   *reconSamplePtr,        // input parameter, deblocked Picture Ptr
    EB_U32                   reconStride,           // input parameter, deblocked stride
    EB_U32                   lcuWidth,              // input parameter, LCU width
    EB_U32                   lcuHeight,             // input parameter, LCU height
    EB_S32                  *boDiff,                // output parameter, used to store Band Offset diff, boDiff[SAO_BO_INTERVALS]
    EB_U16                  *boCount,                                        // output parameter, used to store Band Offset count, boCount[SAO_BO_INTERVALS]
    EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],     // output parameter, used to store Edge Offset diff, eoDiff[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
    EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1])    // output parameter, used to store Edge Offset count, eoCount[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
{
    EB_ERRORTYPE    return_error = EB_ErrorNone;

    EB_S32          diff;
    EB_U32          boIndex, eoType, eoIndex;
    EB_S16          signLeft, signRight, signTop, signBottom, signTopLeft, signBottomRight, signTopRight, signBottomLeft;

    EB_U16          *temporalInputSamplePtr;
    EB_U16          *temporalReconSamplePtr;

    EB_U32          i = 0;
    EB_U32          j = 0;

    EB_U32 boShift = 5;

    // Intialize SAO Arrays

    // BO
    for (boIndex = 0; boIndex < SAO_BO_INTERVALS; ++boIndex) {

        boDiff[boIndex] = 0;
        boCount[boIndex] = 0;
    }

    // EO
    for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {
        for (eoIndex = 0; eoIndex < SAO_EO_CATEGORIES + 1; ++eoIndex) {

            eoDiff[eoType][eoIndex] = 0;
            eoCount[eoType][eoIndex] = 0;

        }
    }


    temporalInputSamplePtr = inputSamplePtr + 1 + inputStride;
    temporalReconSamplePtr = reconSamplePtr + 1 + reconStride;

    for (j = 0; j < lcuHeight - 2; j++) {
        for (i = 0; i < lcuWidth - 2; i++) {

            diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

            //BO
            boIndex = temporalReconSamplePtr[i] >> boShift;
            boDiff[boIndex] += diff;
            boCount[boIndex] ++;

            //EO-0
            signLeft = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - 1)]);
            signRight = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + 1]);
            eoIndex = signRight + signLeft + 2;
            eoDiff[0][eoIndex] += diff;
            eoCount[0][eoIndex] ++;

            //EO-90
            signTop = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride)]);
            signBottom = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride]);
            eoIndex = signTop + signBottom + 2;
            eoDiff[1][eoIndex] += diff;
            eoCount[1][eoIndex] ++;

            //EO-45
            signTopRight = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride + 1)]);
            signBottomLeft = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride - 1]);
            eoIndex = signTopRight + signBottomLeft + 2;
            eoDiff[3][eoIndex] += diff;
            eoCount[3][eoIndex] ++;

            //EO-135
            signTopLeft = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride - 1)]);
            signBottomRight = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride + 1]);
            eoIndex = signTopLeft + signBottomRight + 2;
            eoDiff[2][eoIndex] += diff;
            eoCount[2][eoIndex] ++;
        }

        temporalInputSamplePtr += inputStride;
        temporalReconSamplePtr += reconStride;

    }
    for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {
        eoDiff[eoType][2] = eoDiff[eoType][3];
        eoDiff[eoType][3] = eoDiff[eoType][4];
        eoCount[eoType][2] = eoCount[eoType][3];
        eoCount[eoType][3] = eoCount[eoType][4];
    }

    return return_error;
}

EB_ERRORTYPE GatherSaoStatisticsLcuLossy_62x62(
    EB_U8                   *inputSamplePtr,        // input parameter, source Picture Ptr
    EB_U32                   inputStride,           // input parameter, source stride
    EB_U8                   *reconSamplePtr,        // input parameter, deblocked Picture Ptr
    EB_U32                   reconStride,           // input parameter, deblocked stride
    EB_U32                   lcuWidth,              // input parameter, LCU width
    EB_U32                   lcuHeight,             // input parameter, LCU height
    EB_S32                  *boDiff,                // output parameter, used to store Band Offset diff, boDiff[SAO_BO_INTERVALS]
    EB_U16                  *boCount,                                        // output parameter, used to store Band Offset count, boCount[SAO_BO_INTERVALS]
    EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],     // output parameter, used to store Edge Offset diff, eoDiff[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
    EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1])    // output parameter, used to store Edge Offset count, eoCount[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
{
    EB_ERRORTYPE    return_error = EB_ErrorNone;

    EB_S32          diff;
    EB_U32          boIndex, eoType, eoIndex;
    EB_S16          signLeft, signRight, signTop, signBottom, signTopLeft, signBottomRight, signTopRight, signBottomLeft;

    EB_U8          *temporalInputSamplePtr;
    EB_U8          *temporalReconSamplePtr;

    EB_U32          i = 0;
    EB_U32          j = 0;

    EB_U32 boShift = 3;

    // Intialize SAO Arrays

    // BO
    for (boIndex = 0; boIndex < SAO_BO_INTERVALS; ++boIndex) {

        boDiff[boIndex] = 0;
        boCount[boIndex] = 0;
    }

    // EO
    for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {
        for (eoIndex = 0; eoIndex < SAO_EO_CATEGORIES + 1; ++eoIndex) {

            eoDiff[eoType][eoIndex] = 0;
            eoCount[eoType][eoIndex] = 0;

        }
    }


    temporalInputSamplePtr = inputSamplePtr + 1 + inputStride;
    temporalReconSamplePtr = reconSamplePtr + 1 + reconStride;

    for (j = 0; j < lcuHeight - 2; j++) {
        for (i = 0; i < lcuWidth - 2; i++) {

            diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

            //BO
            boIndex = temporalReconSamplePtr[i] >> boShift;
            boDiff[boIndex] += clipToSigned8Bit(diff);
            boCount[boIndex] ++;

            //EO-0
            signLeft = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - 1)]);
            signRight = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + 1]);
            eoIndex = signRight + signLeft + 2;
            eoDiff[0][eoIndex] += clipToSigned8Bit(diff);
            eoCount[0][eoIndex] ++;

            //EO-90
            signTop = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride)]);
            signBottom = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride]);
            eoIndex = signTop + signBottom + 2;
            eoDiff[1][eoIndex] += clipToSigned8Bit(diff);
            eoCount[1][eoIndex] ++;

            //EO-45
            signTopRight = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride + 1)]);
            signBottomLeft = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride - 1]);
            eoIndex = signTopRight + signBottomLeft + 2;
            eoDiff[3][eoIndex] += clipToSigned8Bit(diff);
            eoCount[3][eoIndex] ++;

            //EO-135
            signTopLeft = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride - 1)]);
            signBottomRight = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride + 1]);
            eoIndex = signTopLeft + signBottomRight + 2;
            eoDiff[2][eoIndex] += clipToSigned8Bit(diff);
            eoCount[2][eoIndex] ++;
        }

        temporalInputSamplePtr += inputStride;
        temporalReconSamplePtr += reconStride;

    }
    for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {
        eoDiff[eoType][2] = eoDiff[eoType][3];
        eoDiff[eoType][3] = eoDiff[eoType][4];
        eoCount[eoType][2] = eoCount[eoType][3];
        eoCount[eoType][3] = eoCount[eoType][4];
    }

    return return_error;
}

EB_ERRORTYPE GatherSaoStatisticsLcu_OnlyEo_90_45_135_Lossy(
    EB_U8                   *inputSamplePtr,        // input parameter, source Picture Ptr
    EB_U32                   inputStride,           // input parameter, source stride
    EB_U8                   *reconSamplePtr,        // input parameter, deblocked Picture Ptr
    EB_U32                   reconStride,           // input parameter, deblocked stride
    EB_U32                   lcuWidth,              // input parameter, LCU width
    EB_U32                   lcuHeight,             // input parameter, LCU height
    EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],     // output parameter, used to store Edge Offset diff, eoDiff[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
    EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1])    // output parameter, used to store Edge Offset count, eoCount[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
{
    EB_ERRORTYPE    return_error = EB_ErrorNone;

    EB_S32          diff;
    EB_U32          eoType, eoIndex;
    EB_S16          signTop, signBottom, signTopLeft, signBottomRight, signTopRight, signBottomLeft;

    EB_U8          *temporalInputSamplePtr;
    EB_U8          *temporalReconSamplePtr;

    EB_U32          i = 0;
    EB_U32          j = 0;


    // Intialize SAO Arrays

    // EO
    for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {
        for (eoIndex = 0; eoIndex < SAO_EO_CATEGORIES + 1; ++eoIndex) {

            eoDiff[eoType][eoIndex] = 0;
            eoCount[eoType][eoIndex] = 0;

        }
    }


    temporalInputSamplePtr = inputSamplePtr + 1 + inputStride;
    temporalReconSamplePtr = reconSamplePtr + 1 + reconStride;

    for (j = 0; j < lcuHeight - 2; j++) {
        for (i = 0; i < lcuWidth - 2; i++) {

            diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

            //EO-90
            signTop = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride)]);
            signBottom = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride]);
            eoIndex = signTop + signBottom + 2;
            eoDiff[1][eoIndex] += clipToSigned8Bit(diff);
            eoCount[1][eoIndex] ++;

            //EO-45
            signTopRight = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride + 1)]);
            signBottomLeft = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride - 1]);
            eoIndex = signTopRight + signBottomLeft + 2;
            eoDiff[3][eoIndex] += clipToSigned8Bit(diff);
            eoCount[3][eoIndex] ++;

            //EO-135
            signTopLeft = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride - 1)]);
            signBottomRight = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride + 1]);
            eoIndex = signTopLeft + signBottomRight + 2;
            eoDiff[2][eoIndex] += clipToSigned8Bit(diff);
            eoCount[2][eoIndex] ++;
        }

        temporalInputSamplePtr += inputStride;
        temporalReconSamplePtr += reconStride;

    }
    for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {
        eoDiff[eoType][2] = eoDiff[eoType][3];
        eoDiff[eoType][3] = eoDiff[eoType][4];
        eoCount[eoType][2] = eoCount[eoType][3];
        eoCount[eoType][3] = eoCount[eoType][4];
    }

    return return_error;
}

/********************************************
*
* collects Sao Statistics
********************************************/
EB_ERRORTYPE GatherSaoStatisticsLcu_62x62_OnlyEo_90_45_135_16bit(
    EB_U16                   *inputSamplePtr,        // input parameter, source Picture Ptr
    EB_U32                   inputStride,           // input parameter, source stride
    EB_U16                   *reconSamplePtr,        // input parameter, deblocked Picture Ptr
    EB_U32                   reconStride,           // input parameter, deblocked stride
    EB_U32                   lcuWidth,              // input parameter, LCU width
    EB_U32                   lcuHeight,             // input parameter, LCU height
    EB_S32                   eoDiff[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1],    // output parameter, used to store Edge Offset diff, eoDiff[SAO_EO_TYPES] [SAO_EO_CATEGORIES]
    EB_U16                   eoCount[SAO_EO_TYPES][SAO_EO_CATEGORIES + 1])   // output parameter, used to store Edge Offset count, eoCount[SAO_EO_TYPES] [SAO_EO_CATEGORIES]

{
    EB_ERRORTYPE    return_error = EB_ErrorNone;

    EB_S32          diff;
    EB_U32  eoType, eoIndex;

    EB_S16 signTop, signBottom, signTopLeft, signBottomRight, signTopRight, signBottomLeft;

    EB_U16          *temporalInputSamplePtr;
    EB_U16          *temporalReconSamplePtr;

    EB_U32          i = 0;
    EB_U32          j = 0;

    // Intialize SAO Arrays
    // EO
    for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {
        for (eoIndex = 0; eoIndex < SAO_EO_CATEGORIES + 1; ++eoIndex) {
            eoDiff[eoType][eoIndex] = 0;
            eoCount[eoType][eoIndex] = 0;

        }
    }

    temporalInputSamplePtr = inputSamplePtr + 1 + inputStride;
    temporalReconSamplePtr = reconSamplePtr + 1 + reconStride;

    for (j = 0; j < lcuHeight - 2; j++)
    {
        for (i = 0; i < lcuWidth - 2; i++)
        {
            diff = temporalInputSamplePtr[i] - temporalReconSamplePtr[i];

            //EO-90
            signTop = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride)]);
            signBottom = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride]);
            eoIndex = signTop + signBottom + 2;
            eoDiff[1][eoIndex] += diff;
            eoCount[1][eoIndex] ++;

            //EO-45
            signTopRight = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride + 1)]);
            signBottomLeft = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride - 1]);
            eoIndex = signTopRight + signBottomLeft + 2;
            eoDiff[3][eoIndex] += diff;
            eoCount[3][eoIndex] ++;

            //EO-135
            signTopLeft = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[(EB_S32)(i - reconStride - 1)]);
            signBottomRight = SIGN(temporalReconSamplePtr[i], temporalReconSamplePtr[i + reconStride + 1]);
            eoIndex = signTopLeft + signBottomRight + 2;
            eoDiff[2][eoIndex] += diff;
            eoCount[2][eoIndex] ++;
        }

        temporalInputSamplePtr += inputStride;
        temporalReconSamplePtr += reconStride;
    }
    for (eoType = 0; eoType < SAO_EO_TYPES; ++eoType) {
        eoDiff[eoType][2] = eoDiff[eoType][3];
        eoDiff[eoType][3] = eoDiff[eoType][4];
        eoCount[eoType][2] = eoCount[eoType][3];
        eoCount[eoType][3] = eoCount[eoType][4];
    }


    return return_error;
}

/********************************************
* SAOApplyBO
*   applies BO SAO for the whole LCU
********************************************/
EB_ERRORTYPE SAOApplyBO(
    EB_U8                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U32                           saoBandPosition,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_U32  lcuWidthCount = 0;
    EB_U32  lcuHeightCount = 0;
    EB_U32  boIndex;


    while (lcuHeightCount < lcuHeight) {
        while (lcuWidthCount < lcuWidth) {

            boIndex = reconSamplePtr[lcuWidthCount] >> 3;
            reconSamplePtr[lcuWidthCount] = (boIndex < saoBandPosition || boIndex > saoBandPosition + SAO_BO_LEN - 1) ?
                reconSamplePtr[lcuWidthCount] :
                (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, reconSamplePtr[lcuWidthCount] + saoOffsetPtr[boIndex - saoBandPosition]);

            ++lcuWidthCount;
        }

        lcuWidthCount = 0;

        ++lcuHeightCount;

        reconSamplePtr += reconStride;
    }
    return return_error;
}
/********************************************
* SAOApplyBO16bit
*   applies BO SAO for the whole LCU
********************************************/
EB_ERRORTYPE SAOApplyBO16bit(
    EB_U16                          *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U32                           saoBandPosition,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_U32  lcuWidthCount = 0;
    EB_U32  lcuHeightCount = 0;
    EB_U32  boIndex;


    while (lcuHeightCount < lcuHeight) {
        while (lcuWidthCount < lcuWidth) {

            boIndex = reconSamplePtr[lcuWidthCount] >> 5;
            reconSamplePtr[lcuWidthCount] = (boIndex < saoBandPosition || boIndex > saoBandPosition + SAO_BO_LEN - 1) ?
                reconSamplePtr[lcuWidthCount] :
                (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, reconSamplePtr[lcuWidthCount] + saoOffsetPtr[boIndex - saoBandPosition]);

            ++lcuWidthCount;
        }

        lcuWidthCount = 0;

        ++lcuHeightCount;

        reconSamplePtr += reconStride;
    }
    return return_error;
}


/********************************************
* SAOApplyEO_0
*   applies EO 0 SAO for the whole LCU
********************************************/
EB_ERRORTYPE SAOApplyEO_0(
    EB_U8                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U8                           *temporalBufferLeft,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_S16  signLeft, signRight;
    EB_U32  eoIndex;

    EB_U32  i = 0, j = 0;

    for (j = 0; j<lcuHeight; j++) {

        signLeft = SIGN(reconSamplePtr[0], temporalBufferLeft[j]);

        for (i = 0; i<lcuWidth; i++) {

            signRight = SIGN(reconSamplePtr[i], reconSamplePtr[i + 1]);
            eoIndex = signLeft + signRight + 2;
            signLeft = -signRight;

            reconSamplePtr[i] = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, reconSamplePtr[i] + saoOffsetPtr[eoIndex]);

        }
        reconSamplePtr += reconStride;
    }

    return return_error;
}
/********************************************
* SAOApplyEO_0_16bit
*   applies EO 0 SAO for the whole LCU
********************************************/
EB_ERRORTYPE SAOApplyEO_0_16bit(
    EB_U16                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                           *temporalBufferLeft,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_S16  signLeft, signRight;
    EB_U32  eoIndex;

    EB_U32  i = 0, j = 0;

    for (j = 0; j<lcuHeight; j++) {

        signLeft = SIGN(reconSamplePtr[0], temporalBufferLeft[j]);

        for (i = 0; i<lcuWidth; i++) {

            signRight = SIGN(reconSamplePtr[i], reconSamplePtr[i + 1]);
            eoIndex = signLeft + signRight + 2;
            signLeft = -signRight;

            reconSamplePtr[i] = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, reconSamplePtr[i] + saoOffsetPtr[eoIndex]);

        }
        reconSamplePtr += reconStride;
    }

    return return_error;
}


/********************************************
* SAOApplyEO_90
*   applies EO 90 SAO for the whole LCU
********************************************/
EB_ERRORTYPE SAOApplyEO_90(
    EB_U8                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U8                           *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_S16  signBottom;
    EB_U32  eoIndex;

    EB_U32  i = 0, j = 0;
    EB_U32  lcuColumn;
    EB_S16  signTop[MAX_LCU_SIZE];


    for (lcuColumn = 0; lcuColumn < lcuWidth; ++lcuColumn) {
        signTop[lcuColumn] = SIGN(reconSamplePtr[lcuColumn], temporalBufferUpper[lcuColumn]);
    }

    for (j = 0; j<lcuHeight; j++) {

        for (i = 0; i<lcuWidth; i++) {

            signBottom = SIGN(reconSamplePtr[i], reconSamplePtr[i + reconStride]);
            eoIndex = signTop[i] + signBottom + 2;
            signTop[i] = -signBottom;

            reconSamplePtr[i] = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, reconSamplePtr[i] + saoOffsetPtr[eoIndex]);
        }


        reconSamplePtr += reconStride;
    }

    return return_error;

}
/********************************************
* SAOApplyEO_90_16bit
*   applies EO 90 SAO for the whole LCU
********************************************/
EB_ERRORTYPE SAOApplyEO_90_16bit(
    EB_U16                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                           *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_S16  signBottom;
    EB_U32  eoIndex;

    EB_U32  i = 0, j = 0;
    EB_U32  lcuColumn;
    EB_S16  signTop[MAX_LCU_SIZE];


    for (lcuColumn = 0; lcuColumn < lcuWidth; ++lcuColumn) {
        signTop[lcuColumn] = SIGN(reconSamplePtr[lcuColumn], temporalBufferUpper[lcuColumn]);
    }

    for (j = 0; j<lcuHeight; j++) {

        for (i = 0; i<lcuWidth; i++) {

            signBottom = SIGN(reconSamplePtr[i], reconSamplePtr[i + reconStride]);
            eoIndex = signTop[i] + signBottom + 2;
            signTop[i] = -signBottom;

            reconSamplePtr[i] = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, reconSamplePtr[i] + saoOffsetPtr[eoIndex]);
        }


        reconSamplePtr += reconStride;
    }

    return return_error;

}
/********************************************
* SAOApplyEO_135
*   applies EO 135 SAO for the whole LCU
********************************************/
EB_ERRORTYPE SAOApplyEO_135(
    EB_U8                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U8                           *temporalBufferLeft,
    EB_U8                           *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_S16  signBottomRight;
    EB_U32  eoIndex;
    EB_U32  lcuColumn;
    EB_S16  signTopLeft[MAX_LCU_SIZE] = { 0 }, signTopLeftTemp[MAX_LCU_SIZE + 1] = { 0 }, signTopLeftSwap[MAX_LCU_SIZE] = { 0 };

    EB_U32  lcuWidthCount = 0;
    EB_U32  lcuHeightCount = 0;


    for (lcuColumn = lcuWidthCount; lcuColumn < lcuWidth; ++lcuColumn) {
        signTopLeft[lcuColumn] = SIGN(reconSamplePtr[lcuColumn], temporalBufferUpper[(EB_S32)(lcuColumn - 1)]);
    }

    while (lcuHeightCount < lcuHeight) {

        while (lcuWidthCount < lcuWidth) {

            signBottomRight = SIGN(reconSamplePtr[lcuWidthCount], reconSamplePtr[lcuWidthCount + reconStride + 1]);
            eoIndex = signTopLeft[lcuWidthCount] + signBottomRight + 2;
            signTopLeftTemp[lcuWidthCount + 1] = -signBottomRight;

            reconSamplePtr[lcuWidthCount] = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, reconSamplePtr[lcuWidthCount] + saoOffsetPtr[eoIndex]);

            ++lcuWidthCount;
        }

        lcuWidthCount = 0;

        signTopLeftTemp[lcuWidthCount] = SIGN(reconSamplePtr[reconStride + lcuWidthCount], temporalBufferLeft[lcuHeightCount]);

        EB_MEMCPY(signTopLeftSwap, signTopLeft, sizeof(EB_S16) * MAX_LCU_SIZE);
        EB_MEMCPY(signTopLeft, signTopLeftTemp, sizeof(EB_S16) * MAX_LCU_SIZE);
        EB_MEMCPY(signTopLeftTemp, signTopLeftSwap, sizeof(EB_S16) * MAX_LCU_SIZE);

        ++lcuHeightCount;

        reconSamplePtr += reconStride;
    }

    return return_error;

}
/********************************************
* SAOApplyEO_135
*   applies EO 135 SAO for the whole LCU
********************************************/
EB_ERRORTYPE SAOApplyEO_135_16bit(
    EB_U16                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                          *temporalBufferLeft,
    EB_U16                           *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth)
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_S16  signBottomRight;
    EB_U32  eoIndex;
    EB_U32  lcuColumn;
    EB_S16  signTopLeft[MAX_LCU_SIZE] = { 0 }, signTopLeftTemp[MAX_LCU_SIZE + 1] = { 0 }, signTopLeftSwap[MAX_LCU_SIZE] = { 0 };

    EB_U32  lcuWidthCount = 0;
    EB_U32  lcuHeightCount = 0;


    for (lcuColumn = lcuWidthCount; lcuColumn < lcuWidth; ++lcuColumn) {
        signTopLeft[lcuColumn] = SIGN(reconSamplePtr[lcuColumn], temporalBufferUpper[(EB_S32)(lcuColumn - 1)]);
    }

    while (lcuHeightCount < lcuHeight) {

        while (lcuWidthCount < lcuWidth) {

            signBottomRight = SIGN(reconSamplePtr[lcuWidthCount], reconSamplePtr[lcuWidthCount + reconStride + 1]);
            eoIndex = signTopLeft[lcuWidthCount] + signBottomRight + 2;
            signTopLeftTemp[lcuWidthCount + 1] = -signBottomRight;

            reconSamplePtr[lcuWidthCount] = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, reconSamplePtr[lcuWidthCount] + saoOffsetPtr[eoIndex]);

            ++lcuWidthCount;
        }

        lcuWidthCount = 0;

        signTopLeftTemp[lcuWidthCount] = SIGN(reconSamplePtr[reconStride + lcuWidthCount], temporalBufferLeft[lcuHeightCount]);

        EB_MEMCPY(signTopLeftSwap, signTopLeft, sizeof(EB_S16) * MAX_LCU_SIZE);
        EB_MEMCPY(signTopLeft, signTopLeftTemp, sizeof(EB_S16) * MAX_LCU_SIZE);
        EB_MEMCPY(signTopLeftTemp, signTopLeftSwap, sizeof(EB_S16) * MAX_LCU_SIZE);

        ++lcuHeightCount;

        reconSamplePtr += reconStride;
    }

    return return_error;

}
/********************************************
* SAOApplyEO_45
*   applies EO 45 SAO for the whole LCU
********************************************/
EB_ERRORTYPE SAOApplyEO_45(
    EB_U8                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U8                           *temporalBufferLeft,
    EB_U8                           *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth
    )
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_S16  signBottomLeft;
    EB_U32  eoIndex;
    EB_U32  lcuColumn;

    EB_S16 signTopRight[MAX_LCU_SIZE + 1];

    EB_U32  lcuWidthCount = 0;
    EB_U32  lcuHeightCount = 0;


    for (lcuColumn = lcuWidthCount; lcuColumn < lcuWidth + 1; ++lcuColumn) {
        signTopRight[lcuColumn] = SIGN(reconSamplePtr[(EB_S32)(lcuColumn - 1)], temporalBufferUpper[lcuColumn]);
    }

    while (lcuHeightCount < lcuHeight) {

        signBottomLeft = SIGN(reconSamplePtr[lcuWidthCount], temporalBufferLeft[lcuHeightCount + 1]);
        eoIndex = signTopRight[lcuWidthCount + 1] + signBottomLeft + 2;
        signTopRight[lcuWidthCount] = -signBottomLeft;

        reconSamplePtr[lcuWidthCount] = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, reconSamplePtr[lcuWidthCount] + saoOffsetPtr[eoIndex]);

        ++lcuWidthCount;

        while (lcuWidthCount < lcuWidth) {

            signBottomLeft = SIGN(reconSamplePtr[lcuWidthCount], reconSamplePtr[lcuWidthCount + reconStride - 1]);
            eoIndex = signTopRight[lcuWidthCount + 1] + signBottomLeft + 2;
            signTopRight[lcuWidthCount] = -signBottomLeft;

            reconSamplePtr[lcuWidthCount] = (EB_U8)CLIP3(0, MAX_SAMPLE_VALUE, reconSamplePtr[lcuWidthCount] + saoOffsetPtr[eoIndex]);

            ++lcuWidthCount;
        }

        signTopRight[lcuWidth] = SIGN(reconSamplePtr[lcuWidth - 1 + reconStride], reconSamplePtr[lcuWidth]);

        lcuWidthCount = 0;

        ++lcuHeightCount;

        reconSamplePtr += reconStride;
    }

    return return_error;

}
/********************************************
* SAOApplyEO_45_16bit
*   applies EO 45 SAO for the whole LCU
********************************************/
EB_ERRORTYPE SAOApplyEO_45_16bit(
    EB_U16                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                           *temporalBufferLeft,
    EB_U16                           *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth
    )
{
    EB_ERRORTYPE return_error = EB_ErrorNone;
    EB_S16  signBottomLeft;
    EB_U32  eoIndex;
    EB_U32  lcuColumn;

    EB_S16 signTopRight[MAX_LCU_SIZE + 1];

    EB_U32  lcuWidthCount = 0;
    EB_U32  lcuHeightCount = 0;


    for (lcuColumn = lcuWidthCount; lcuColumn < lcuWidth + 1; ++lcuColumn) {
        signTopRight[lcuColumn] = SIGN(reconSamplePtr[(EB_S32)(lcuColumn - 1)], temporalBufferUpper[lcuColumn]);
    }

    while (lcuHeightCount < lcuHeight) {

        signBottomLeft = SIGN(reconSamplePtr[lcuWidthCount], temporalBufferLeft[lcuHeightCount + 1]);
        eoIndex = signTopRight[lcuWidthCount + 1] + signBottomLeft + 2;
        signTopRight[lcuWidthCount] = -signBottomLeft;

        reconSamplePtr[lcuWidthCount] = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, reconSamplePtr[lcuWidthCount] + saoOffsetPtr[eoIndex]);

        ++lcuWidthCount;

        while (lcuWidthCount < lcuWidth) {

            signBottomLeft = SIGN(reconSamplePtr[lcuWidthCount], reconSamplePtr[lcuWidthCount + reconStride - 1]);
            eoIndex = signTopRight[lcuWidthCount + 1] + signBottomLeft + 2;
            signTopRight[lcuWidthCount] = -signBottomLeft;

            reconSamplePtr[lcuWidthCount] = (EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, reconSamplePtr[lcuWidthCount] + saoOffsetPtr[eoIndex]);

            ++lcuWidthCount;
        }

        signTopRight[lcuWidth] = SIGN(reconSamplePtr[lcuWidth - 1 + reconStride], reconSamplePtr[lcuWidth]);

        lcuWidthCount = 0;

        ++lcuHeightCount;

        reconSamplePtr += reconStride;
    }

    return return_error;

}
