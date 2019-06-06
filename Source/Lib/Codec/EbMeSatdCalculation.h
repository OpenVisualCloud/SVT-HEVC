/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#ifndef EbMeSatdCalculation_h
#define EbMeSatdCalculation_h

#include "EbMeSatdCalculation_C.h"
#include "EbMeSatdCalculation_AVX2.h"

#include "EbDefinitions.h"
#ifdef __cplusplus
extern "C" {
#endif

/***************************************
* Function Types
***************************************/
typedef EB_U32(*EB_SATDCALCULATION16X16_TYPE)(
	EB_U8   *src,
	EB_U32   srcStride,
	EB_U8   *ref,
	EB_U32   refStride);

/***************************************
* Function Tables
***************************************/

static EB_SATDCALCULATION16X16_TYPE SatdCalculation_16x16_funcPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
	SatdCalculation_16x16,
    // AVX2
	SatdCalculation_16x16_avx2
	/*SatdCalculation_16x16_SSE2_INTRIN,*/
};


#ifdef __cplusplus
}
#endif        
#endif // EbMeSatdCalculation_h