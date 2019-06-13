/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbIntraPrediction_AVX2_h
#define EbIntraPrediction_AVX2_h

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

extern void IntraModePlanar_AVX2_INTRIN(
	const EB_U32   size,                       //input parameter, denotes the size of the current PU
	EB_U8         *refSamples,                 //input parameter, pointer to the reference samples
	EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
	const EB_U32   predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
	const EB_BOOL  skip);

extern void IntraModeAngular_Vertical_Kernel_AVX2_INTRIN(
	EB_U32         size,                       //input parameter, denotes the size of the current PU
	EB_U8         *refSampMain,                //input parameter, pointer to the reference samples
	EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
	EB_U32         predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
	const EB_BOOL  skip,
	EB_S32         intraPredAngle);

extern void IntraModeAngular_Horizontal_Kernel_AVX2_INTRIN(
	EB_U32         size,                       //input parameter, denotes the size of the current PU
	EB_U8         *refSampMain,                //input parameter, pointer to the reference samples
	EB_U8         *predictionPtr,              //output parameter, pointer to the prediction
	EB_U32         predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
	const EB_BOOL  skip,
	EB_S32         intraPredAngle);

extern void IntraModeVerticalLuma_AVX2_INTRIN(
    const EB_U32      size,                   //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,             //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,          //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride, //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip                    //skip one row
    );

extern void IntraModeVerticalChroma_AVX2_INTRIN(
    const EB_U32      size,                   //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,             //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,          //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride, //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip                    //skip one row
    );

extern void IntraModeDCLuma_AVX2_INTRIN(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip)   ;                    //skip one row

extern void IntraModeDCChroma_AVX2_INTRIN(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip);               //skip one row



extern void IntraModeAngular_2_AVX2_INTRIN(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip);

extern void IntraModeAngular_18_AVX2_INTRIN(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip);

extern void IntraModeAngular_34_AVX2_INTRIN(
    const EB_U32      size,                       //input parameter, denotes the size of the current PU
    EB_U8            *refSamples,                 //input parameter, pointer to the reference samples
    EB_U8            *predictionPtr,              //output parameter, pointer to the prediction
    const EB_U32      predictionBufferStride,     //input parameter, denotes the stride for the prediction ptr
    const EB_BOOL     skip);

EB_U32 UpdateNeighborDcIntraPred_AVX2_INTRIN(
    EB_U8                           *yIntraReferenceArrayReverse,
    EB_U16                           inputHeight,
    EB_U16                           strideY,
    EB_BYTE                          bufferY,
    EB_U16                           originY,
    EB_U16                           originX,
    EB_U32                           srcOriginX,
    EB_U32                           srcOriginY,
	EB_U32                           blockSize);

#ifdef __cplusplus
}
#endif
#endif // EbIntraPrediction_AVX2_h
