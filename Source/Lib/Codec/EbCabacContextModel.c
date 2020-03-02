/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbUtility.h"
#include "EbCabacContextModel.h"
#include "EbDefinitions.h"

/*****************************
 * Context Model Tables for initial probabilities (slope)
 *****************************/
static const EB_S16 cabacInitialProbabilityTableI[] = {
    //splitFlag
    139,  141,  157,
    //skipFlag
    CNU,  CNU,  CNU,
    //mergeFlag
    CNU,
    //mergeIdx
    CNU,
    //mvpIdx
    CNU,  CNU,
    //partSize
    184,  CNU,  CNU,  CNU,
    //predMode
    CNU,
    //intraLumaMode
    184,
    //intraChromaMode
    63,  139,
    //deltaQp
    154,  154,  154,
    //interDir
    CNU,  CNU,  CNU,  CNU,  CNU,
    //refPic
    CNU,  CNU,
    //mvdInit
    CNU,  CNU,
    //cbf
    111,  141,  CNU,  CNU,  CNU,   94,  138,  182,  CNU,  CNU,
    //rootCbf
    CNU,
    //transSubDiv
    153,  138,  138,
    //lastSigX
    110,  110,  124,  125,  140,  153,  125,  127,  140,  109,  111,  143,  127,  111,   79,
    108,  123,   63,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,
    //lastSigY
    110,  110,  124,  125,  140,  153,  125,  127,  140,  109,  111,  143,  127,  111,   79,
    108,  123,   63,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,
    //sigFlag
    111,  111,  125,  110,  110,   94,  124,  108,  124,  107,  125,  141,  179,  153,  125,
    107,  125,  141,  179,  153,  125,  107,  125,  141,  179,  153,  125,  140,  139,  182,
    182,  152,  136,  152,  136,  153,  136,  139,  111,  136,  139,  111,
    //coeffGroupSigFlag
    91,  171,  134,  141,
    //greaterThanOne
    140,   92,  137,  138,  140,  152,  138,  139,  153,   74,  149,   92,
    139,  107,  122,  152,  140,  179,  166,  182,  140,  227,  122,  197,
    //greaterThanTwo
    138,  153,  136,  167,  152,  152,
    //alfCtrlFlag
    200,
    //alfFlag
    153,
    //alfUvlc
    140,  154,
    //alfSvlc
    187,  154,  159,
    // sao merge flag
    153,
    //saoTypeIndex
    200,
    //cuAmp
    CNU
};

static const EB_S16 cabacInitialProbabilityTableP[] = {
    //splitFlag
    107,  139,  126,
    //skipFlag
    197,  185,  201,
    //mergeFlag
    110,
    //mergeIdx
    122,
    //mvpIdx
    168,  CNU,
    //partSize
    154,  139,  CNU,  CNU,
    //predMode
    149,
    //intraLumaMode
    154,
    //intraChromaMode
    152,  139,
    //deltaQp
    154,  154,  154,
    //interDir
    95,   79,   63,   31,  31,
    //refPic
    153,  153,
    //mvdInit
    140,  198,
    //cbf
    153,  111,  CNU,  CNU,  CNU,  149,  107,  167,  CNU,  CNU,
    //rootCbf
    79,
    //transSubDiv
    124,  138,   94,
    //lastSigX
    125,  110,   94,  110,   95,   79,  125,  111,  110,   78,  110,  111,  111,   95,   94,
    108,  123,  108,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,
    //lastSigY
    125,  110,   94,  110,   95,   79,  125,  111,  110,   78,  110,  111,  111,   95,   94,
    108,  123,  108,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,
    //sigFlag
    155,  154,  139,  153,  139,  123,  123,   63,  153,  166,  183,  140,  136,  153,  154,
    166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  170,  153,  123,
    123,  107,  121,  107,  121,  167,  151,  183,  140,  151,  183,  140,
    //coeffGroupSigFlag
    121,  140,   61,  154,
    //greaterThanOne
    154,  196,  196,  167,  154,  152,  167,  182,  182,  134,  149,  136,
    153,  121,  136,  137,  169,  194,  166,  167,  154,  167,  137,  182,
    //greaterThanTwo
    107,  167,   91,  122,  107,  167,
    //alfCtrlFlag
    139,
    //alfFlag
    153,
    //alfUvlc
    154,  154,
    //alfSvlc
    141,  154,  189,
    // sao merge flag
    153,
    //saoTypeIndex
    185,
    //cuAmp
    154
};

static const EB_S16 cabacInitialProbabilityTableB[] = {
    //splitFlag
    107,  139,  126,
    //skipFlag
    197,  185,  201,
    //mergeFlag
    154,
    //mergeIdx
    137,
    //mvpIdx
    168,  CNU,
    //partSize
    154,  139,  CNU,  CNU,
    //predMode
    134,
    //intraLumaMode
    183,
    //intraChromaMode
    152,  139,
    //deltaQp
    154,  154,  154,
    //interDir
    95,   79,   63,   31,  31,
    //refPic
    153,  153,
    //mvdInit
    169,  198,
    //cbf
    153,  111,  CNU,  CNU,  CNU,  149,   92,  167,  CNU,  CNU,
    //rootCbf
    79,
    //transSubDiv
    224,  167,  122,
    //lastSigX
    125,  110,  124,  110,   95,   94,  125,  111,  111,   79,  125,  126,  111,  111,   79,
    108,  123,   93,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,
    //lastSigY
    125,  110,  124,  110,   95,   94,  125,  111,  111,   79,  125,  126,  111,  111,   79,
    108,  123,   93,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,  CNU,
    //sigFlag
    170,  154,  139,  153,  139,  123,  123,   63,  124,  166,  183,  140,  136,  153,  154,
    166,  183,  140,  136,  153,  154,  166,  183,  140,  136,  153,  154,  170,  153,  138,
    138,  122,  121,  122,  121,  167,  151,  183,  140,  151,  183,  140,
    //coeffGroupSigFlag
    121,  140,   61,  154,
    //greaterThanOne
    154,  196,  167,  167,  154,  152,  167,  182,  182,  134,  149,  136,
    153,  121,  136,  122,  169,  208,  166,  167,  154,  152,  167,  182,
    //greaterThanTwo
    107,  167,   91,  107,  107,  167,
    //alfCtrlFlag
    169,
    //alfFlag
    153,
    //alfUvlc
    154,  154,
    //alfSvlc
    141,  154,  159,
    // sao merge flag
    153,
    //saoTypeIndex
    160,
    //cuAmp
    154
};

EB_ERRORTYPE EncodeCabacContextModelInit(
    ContextModelEncContext_t *cabacContextModelArray)
{
    EB_U32           modelIndex;
    EB_U32           sliceIdx;
    EB_S32           qpIndex;
    EB_U32           bufferOffset1;
    EB_U32           bufferOffset2;
    EB_S32           initialProbState;
    EB_S32           initialProbSlope;
    EB_S32           initialProbOffset;
    EB_U32           mostProbSymbol;
    EB_ContextModel *contextModelPtr;
    EB_ContextModel  tempContextModel;
    const EB_S16    *cabacInitialProbabilityTable;

    contextModelPtr = (EB_ContextModel*)cabacContextModelArray;

    // Loop over all slice types
    for(sliceIdx = 0; sliceIdx < TOTAL_NUMBER_OF_SLICE_TYPES; sliceIdx++) {

        cabacInitialProbabilityTable =
            (sliceIdx == EB_I_PICTURE) ? cabacInitialProbabilityTableI :
            (sliceIdx == EB_P_PICTURE) ? cabacInitialProbabilityTableP :
            cabacInitialProbabilityTableB;

        bufferOffset1 = sliceIdx * TOTAL_NUMBER_OF_QP_VALUES * MAX_SIZE_OF_CABAC_CONTEXT_MODELS;

        // Loop over all Qps
        for(qpIndex = 0; qpIndex < TOTAL_NUMBER_OF_QP_VALUES; qpIndex++) {

            bufferOffset2 = qpIndex * MAX_SIZE_OF_CABAC_CONTEXT_MODELS;

            // Loop over all cabac context models
            for(modelIndex = 0; modelIndex < TOTAL_NUMBER_OF_CABAC_CONTEXT_MODELS; modelIndex++) {

                initialProbSlope  = (EB_S32)((cabacInitialProbabilityTable[modelIndex] >> 4)*5 -45);
                initialProbOffset = (EB_S32)(((cabacInitialProbabilityTable[modelIndex] & 15)<<3) -16);
                initialProbState  = CLIP3(1, 126, ( ( initialProbSlope * qpIndex ) >> 4 ) + initialProbOffset );
                mostProbSymbol    = (initialProbState >=64)? 1: 0;
                tempContextModel  =  mostProbSymbol ? initialProbState - 64 : 63 - initialProbState;

                contextModelPtr[modelIndex + bufferOffset1 + bufferOffset2] =  (tempContextModel<<1) + mostProbSymbol;
            }
            // Initialise junkpad values to zero
            // *Note - this code goes out of the array boundary
            //EB_MEMSET(&contextModelPtr[TOTAL_NUMBER_OF_CABAC_CONTEXT_MODELS+bufferOffset1 + bufferOffset2], 0, NUMBER_OF_PAD_VALUES_IN_CONTEXT_MODELS*sizeof(EB_ContextModel));
        }
    }

    return EB_ErrorNone;
}
