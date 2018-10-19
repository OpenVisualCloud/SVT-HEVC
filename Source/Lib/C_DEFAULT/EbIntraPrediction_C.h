/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbIntraPrediction_C_h
#define EbIntraPrediction_C_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

void IntraModeVerticalLuma(
    const EB_U32      size,                   //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,             //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,          //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride, //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip                    //skip one row 
    );

void IntraModeVerticalLuma16bit(
    const EB_U32   size,
    EB_U16         *refSamples,
    EB_U16         *predictionPtr,
    const EB_U32   predictionBufferStride,
    const EB_BOOL  skip);

void IntraModeVerticalChroma(
    const EB_U32      size,                   //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,             //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,          //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride, //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip                    //skip one row 
    );

void IntraModeVerticalChroma16bit(
    const EB_U32   size,
    EB_U16         *refSamples,
    EB_U16         *predictionPtr,
    const EB_U32   predictionBufferStride,
    const EB_BOOL  skip);

void IntraModeHorizontalLuma(
    const EB_U32      size,                   //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,             //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,          //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride, //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip                    //skip one row 
    );

void IntraModeHorizontalLuma16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);                       //skip half rows

void IntraModeHorizontalChroma(
    const EB_U32      size,                   //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,             //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,          //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride, //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip                    //skip one row 
    );

void IntraModeHorizontalChroma16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);                       //skip half rows

void IntraModeDCLuma(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip);                   //skip one row 

void IntraModeDCLuma16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);                       //skip half rows

void IntraModeDCChroma(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip);                    //skip one row 

void IntraModeDCChroma16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);                       //skip half rows

void IntraModePlanar(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip);                     //skip one row 

void IntraModePlanar16bit(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);                       //skip half rows

void IntraModeAngular_34(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip);                     //skip one row 

void IntraModeAngular16bit_34(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);

void IntraModeAngular_18(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip);                    //skip one row 

void IntraModeAngular16bit_18(
    const EB_U32   size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip);

void IntraModeAngular_2(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip);                      //skip one row 

void IntraModeAngular16bit_2(
    const EB_U32    size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSamples,                 //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32    predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL   skip);

void IntraModeAngular_Vertical_Kernel(
    EB_U32            size,
    EB_U8            *refSampMain,
    EB_U8            *predictionPtr,
    EB_U32            predictionBufferStride,
    const EB_BOOL     skip,
    EB_S32            intraPredAngle);

void IntraModeAngular16bit_Vertical_Kernel(
    EB_U32          size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSampMain,                //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    EB_U32			predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL	skip,
    EB_S32			intraPredAngle);

void IntraModeAngular_Horizontal_Kernel(
    EB_U32            size,
    EB_U8            *refSampMain,
    EB_U8            *predictionPtr,
    EB_U32            predictionBufferStride,
    const EB_BOOL     skip,
    EB_S32            intraPredAngle);

void IntraModeAngular16bit_Horizontal_Kernel(
    EB_U32         size,                       //input parameter, denotes the size of the current PU
    EB_U16         *refSampMain,                //input parameter, pointer to the reference samples
    EB_U16         *predictionPtr,              //output parameter, pointer to the prediction
    EB_U32         predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL  skip,
    EB_S32         intraPredAngle);


#ifdef __cplusplus
}
#endif        
#endif // EbCompute8x8SAD_asm_h