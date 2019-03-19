/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbIntraPrediction_SSE2_h
#define EbIntraPrediction_SSE2_h

#include "EbDefinitions.h"

/*******************************************
* Function Pointer Table
*******************************************/
#ifdef __cplusplus
extern "C" {
#endif

extern void IntraModeVerticalLuma16bit_SSE2_INTRIN(
    const EB_U32   size,
    EB_U16         *refSamples,
    EB_U16         *predictionPtr,
    const EB_U32   predictionBufferStride,
    const EB_BOOL  skip);



extern void IntraModeVerticalChroma16bit_SSE2_INTRIN(
    const EB_U32   size,
    EB_U16         *refSamples,
    EB_U16         *predictionPtr,
    const EB_U32   predictionBufferStride,
    const EB_BOOL  skip);


extern  void IntraModeHorizontalLuma_SSE2_INTRIN(
    const EB_U32      size,                   //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,             //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,          //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride, //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip                    //skip one row 
    );

void IntraModeHorizontalLuma16bit_SSE2_INTRIN(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);                       //skip half rows

extern void IntraModeHorizontalChroma_SSE2_INTRIN(
    const EB_U32      size,                   //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,             //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,          //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride, //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip                    //skip one row 
    );

extern void IntraModeHorizontalChroma16bit_SSE2_INTRIN(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);                       //skip half rows

void IntraModePlanar16bit_SSE2_INTRIN(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);                       //skip half rows

extern void IntraModeAngular16bit_34_SSE2_INTRIN(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);

extern void IntraModeAngular16bit_18_SSE2_INTRIN(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);

extern void IntraModeAngular16bit_2_SSE2_INTRIN(
    const EB_U32    size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32    predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL   skip);

extern void IntraModeAngular16bit_Vertical_Kernel_SSE2_INTRIN(
    EB_U32          size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSampMain,                //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    EB_U32			predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL	skip,
    EB_S32			intraPredAngle);

void IntraModeAngular16bit_Horizontal_Kernel_SSE2_INTRIN(
    EB_U32         size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSampMain,                //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    EB_U32         predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip,
    EB_S32         intraPredAngle);


#ifdef __cplusplus
}
#endif
#endif // EbIntraPrediction_SSE2_h
