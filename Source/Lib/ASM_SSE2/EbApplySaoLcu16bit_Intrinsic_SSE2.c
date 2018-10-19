/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbDefinitions.h"
#include <emmintrin.h>
#include "EbSampleAdaptiveOffset_SSE2.h"

EB_ERRORTYPE SAOApplyBO16bit_SSE2_INTRIN(
    EB_U16 *reconSamplePtr,
    EB_U32  reconStride,
    EB_U32  saoBandPosition,
    EB_S8  *saoOffsetPtr,
    EB_U32  lcuHeight,
    EB_U32  lcuWidth)
{
    reconStride -=  lcuWidth ;
    
    EB_U32 width_cnt, height_cnt;
    EB_ERRORTYPE return_error = EB_ErrorNone;
    __m128i xmm_0 = _mm_setzero_si128();
    __m128i xmm_Max10bit = _mm_set1_epi16(0x03FF);
    __m128i xmm_1 = _mm_set1_epi8(0x01);
    __m128i xmm_2 = _mm_add_epi8(xmm_1, xmm_1);
    __m128i xmm_3 = _mm_add_epi8(xmm_1, xmm_2);
    __m128i xmm_saoBandPosition, saoOffset0, saoOffset1, saoOffset2, saoOffset3, xmm_Offset0_3, rec_0_7, rec_8_15, xmm_Offset0_3_lo, xmm_Offset0_3_hi;
    __m128i boIdx_sub_saoBandPosn, cmp_result, cmp_result1, cmp_result2, cmp_result3, or_result, result_0_7, result_8_15;

    xmm_saoBandPosition = _mm_cvtsi32_si128(saoBandPosition);
    xmm_saoBandPosition = _mm_unpacklo_epi8(xmm_saoBandPosition, xmm_saoBandPosition);
    xmm_saoBandPosition = _mm_unpacklo_epi16(xmm_saoBandPosition, xmm_saoBandPosition);
    xmm_saoBandPosition = _mm_unpacklo_epi32(xmm_saoBandPosition, xmm_saoBandPosition);
    xmm_saoBandPosition = _mm_unpacklo_epi64(xmm_saoBandPosition, xmm_saoBandPosition);

    xmm_Offset0_3 = _mm_cvtsi32_si128(*(EB_S32*)saoOffsetPtr);
    xmm_Offset0_3 = _mm_unpacklo_epi8(xmm_Offset0_3, xmm_Offset0_3); 
    xmm_Offset0_3 = _mm_unpacklo_epi16(xmm_Offset0_3, xmm_Offset0_3);
    xmm_Offset0_3_lo = _mm_unpacklo_epi32(xmm_Offset0_3, xmm_Offset0_3);
    xmm_Offset0_3_hi = _mm_unpackhi_epi32(xmm_Offset0_3, xmm_Offset0_3);

    saoOffset0 = _mm_unpacklo_epi64(xmm_Offset0_3_lo, xmm_Offset0_3_lo);
    saoOffset1 = _mm_unpackhi_epi64(xmm_Offset0_3_lo, xmm_Offset0_3_lo);
    saoOffset2 = _mm_unpacklo_epi64(xmm_Offset0_3_hi, xmm_Offset0_3_hi);
    saoOffset3 = _mm_unpackhi_epi64(xmm_Offset0_3_hi, xmm_Offset0_3_hi);

    for (height_cnt = 0; height_cnt < lcuHeight; ++height_cnt) {
        for (width_cnt = 0; width_cnt < lcuWidth; width_cnt += 16) {

            rec_0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
            rec_8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));
            
            //(boIndex < saoBandPosition || boIndex > saoBandPosition + SAO_BO_LEN - 1) ? 0 : saoOffsetPtr[boIndex - saoBandPosition]
            boIdx_sub_saoBandPosn = _mm_sub_epi8(_mm_packus_epi16(_mm_srli_epi16(rec_0_7, 5), _mm_srli_epi16(rec_8_15, 5)), xmm_saoBandPosition);
            cmp_result = _mm_and_si128(_mm_cmpeq_epi8(boIdx_sub_saoBandPosn, xmm_0), saoOffset0);
            cmp_result1 = _mm_and_si128(_mm_cmpeq_epi8(boIdx_sub_saoBandPosn, xmm_1), saoOffset1);
            cmp_result2 = _mm_and_si128(_mm_cmpeq_epi8(boIdx_sub_saoBandPosn, xmm_2), saoOffset2);
            cmp_result3 = _mm_and_si128(_mm_cmpeq_epi8(boIdx_sub_saoBandPosn, xmm_3), saoOffset3);
            or_result = _mm_or_si128(_mm_or_si128(cmp_result, cmp_result1), _mm_or_si128(cmp_result2, cmp_result3));

            //(EB_U16)CLIP3(0, MAX_SAMPLE_VALUE_10BIT, reconSamplePtr[lcuWidthCount] + saoOffsetPtr[boIndex - saoBandPosition]);
            result_0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(rec_0_7, _mm_unpacklo_epi8(or_result, _mm_cmpgt_epi8(xmm_0, or_result))), xmm_Max10bit), xmm_0);
            result_8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(rec_8_15, _mm_unpackhi_epi8(or_result, _mm_cmpgt_epi8(xmm_0, or_result))), xmm_Max10bit), xmm_0);
            _mm_storeu_si128((__m128i *)(reconSamplePtr), result_0_7);
            _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), result_8_15);
            reconSamplePtr += 16;
        }
        reconSamplePtr += reconStride;
    }
    return return_error;
}

EB_ERRORTYPE SAOApplyEO_90_16bit_SSE2_INTRIN(
    EB_U16 *reconSamplePtr,
    EB_U32  reconStride,
    EB_U16 *temporalBufferUpper,
    EB_S8  *saoOffsetPtr,
    EB_U32  lcuHeight,
    EB_U32  lcuWidth)
{
    __m128i xmm_0, xmm_N1, xmm_N2, xmm_1, xmm_Max10bit, tempBufUpper0_7, tempBufUpper8_15, recon0_7, recon8_15, signTop, nSignTop;
    __m128i saoOffset0_7, saoOffset0_4, saoOffset0, saoOffset1, saoOffset2, saoOffset3;
    EB_U32 width_cnt, height_cnt;
    EB_U16 * reconSamplePtrTemp;

    xmm_0 = _mm_setzero_si128();
    xmm_N1 = _mm_set1_epi8(-1);
    xmm_N2 = _mm_set1_epi8(-2);
    xmm_1 = _mm_set1_epi8(1);
    xmm_Max10bit = _mm_set1_epi16(0x3FF);

    saoOffset0_7 = _mm_loadl_epi64((__m128i *)(saoOffsetPtr)); //0123457xxxxxxxx
    saoOffset0_7 = _mm_unpacklo_epi8(saoOffset0_7, saoOffset0_7); //0011xx3344556677
    saoOffset0_4 = _mm_unpacklo_epi32(saoOffset0_7, _mm_srli_si128(saoOffset0_7, 6)); //00113344...
    saoOffset0_4 = _mm_unpacklo_epi16(saoOffset0_4, saoOffset0_4); // 0000111133334444
    saoOffset0 = _mm_shuffle_epi32(saoOffset0_4, 0x00); //0000000000000000
    saoOffset1 = _mm_shuffle_epi32(saoOffset0_4, 0x55); //1111111111111111
    saoOffset2 = _mm_shuffle_epi32(saoOffset0_4, 0xaa);
    saoOffset3 = _mm_shuffle_epi32(saoOffset0_4, 0xff);

    for (width_cnt = 0; width_cnt < lcuWidth; width_cnt += 16) {
        
        reconSamplePtrTemp = reconSamplePtr;
        tempBufUpper0_7 = _mm_loadu_si128((__m128i *)(temporalBufferUpper));
        tempBufUpper8_15 = _mm_loadu_si128((__m128i *)(temporalBufferUpper + 8));
        temporalBufferUpper += 16;
        recon0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtrTemp));
        recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtrTemp + 8));
        signTop = _mm_packs_epi16(_mm_sub_epi16(tempBufUpper0_7, recon0_7), _mm_sub_epi16(tempBufUpper8_15, recon8_15));
        
        // -signTop - 1
        nSignTop = _mm_sub_epi8(xmm_N2, _mm_add_epi8(_mm_cmpgt_epi8(signTop, xmm_0), _mm_cmpgt_epi8(signTop, xmm_N1)));
        
        for (height_cnt = 0; height_cnt < lcuHeight; ++height_cnt) {
            __m128i recon_s0_s7, recon_s8_s15, signBottom, nSignTotal, saoOffset_eo, sign, clipped_0_7, clipped_8_15;

            recon_s0_s7 = _mm_loadu_si128((__m128i *)(reconSamplePtrTemp + reconStride));
            recon_s8_s15 = _mm_loadu_si128((__m128i *)(reconSamplePtrTemp + reconStride + 8));
            signBottom = _mm_packs_epi16(_mm_sub_epi16(recon_s0_s7, recon0_7), _mm_sub_epi16(recon_s8_s15, recon8_15));
            signBottom = _mm_add_epi8(_mm_cmpgt_epi8(signBottom, xmm_0), _mm_cmpgt_epi8(signBottom, xmm_N1));

            //-(signTop + signBottom)
            nSignTotal = _mm_sub_epi8(nSignTop, signBottom);

            saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpgt_epi8(nSignTotal, xmm_1), saoOffset0),
                                                     _mm_and_si128(_mm_cmpeq_epi8(nSignTotal, xmm_1), saoOffset1)),
                                        _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(nSignTotal, xmm_N1), saoOffset2),
                                                     _mm_and_si128(_mm_cmpeq_epi8(nSignTotal, xmm_N2), saoOffset3)));

            sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);
            clipped_0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpacklo_epi8(saoOffset_eo, sign), recon0_7), xmm_Max10bit), xmm_0);
            clipped_8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpackhi_epi8(saoOffset_eo, sign), recon8_15), xmm_Max10bit), xmm_0);
            _mm_storeu_si128((__m128i *)(reconSamplePtrTemp), clipped_0_7);
            _mm_storeu_si128((__m128i *)(reconSamplePtrTemp + 8), clipped_8_15);

            // Next iteration
            recon0_7 = recon_s0_s7;
            recon8_15 = recon_s8_s15;
            nSignTop = signBottom;
            reconSamplePtrTemp += reconStride;
        }
        reconSamplePtr += 16;
    }
    return EB_ErrorNone;
}


EB_ERRORTYPE SAOApplyEO_0_16bit_SSE2_INTRIN(
    EB_U16 *reconSamplePtr,
    EB_U32  reconStride,
    EB_U16 *temporalBufferLeft,
    EB_S8  *saoOffsetPtr,
    EB_U32  lcuHeight,
    EB_U32  lcuWidth)
{
    __m128i xmm_0, xmm_N1, xmm_N3, xmm_N4, xmm_255, saoOffset0_7;
    __m128i saoOffset0, saoOffset1, saoOffset2, saoOffset3, saoOffset0_4;
    __m128i tempBufferLeft, xmm_Max10bit, recon0_7, recon8_15, signLeft, signRight, eoIndex, saoOffset_eo, sign, clip3_0_7, clip3_8_15, recon15_22;
    EB_U32 width_cnt, height_cnt, count;

    lcuWidth -= 16;
    reconStride -= lcuWidth;
    xmm_N1 = _mm_set1_epi8(-1);
    xmm_N3 = _mm_set1_epi8(-3);
    xmm_N4 = _mm_set1_epi8(-4);
    xmm_255 = _mm_srli_si128(xmm_N1, 14);
    xmm_Max10bit = _mm_set1_epi16(0x3FF);
    xmm_0 = _mm_setzero_si128();

    ///We use offset 0,1,3,4 in the calculation because saoOffset[2] is always 0
    saoOffset0_7 = _mm_loadl_epi64((__m128i *)saoOffsetPtr);
    saoOffset0_7 = _mm_unpacklo_epi8(saoOffset0_7, saoOffset0_7); //0011xx3344556677
    saoOffset0_4 = _mm_unpacklo_epi32(saoOffset0_7, _mm_srli_si128(saoOffset0_7, 6)); //00113344...
    saoOffset0_4 = _mm_unpacklo_epi16(saoOffset0_4, saoOffset0_4); // 0000111133334444
    saoOffset0 = _mm_shuffle_epi32(saoOffset0_4, 0x00); //0000000000000000
    saoOffset1 = _mm_shuffle_epi32(saoOffset0_4, 0x55); //1111111111111111
    saoOffset2 = _mm_shuffle_epi32(saoOffset0_4, 0xaa);
    saoOffset3 = _mm_shuffle_epi32(saoOffset0_4, 0xff);

    for (height_cnt = 0; height_cnt < lcuHeight; height_cnt += 8) {

        tempBufferLeft = _mm_loadu_si128((__m128i *)(temporalBufferLeft));
        temporalBufferLeft += 8;

        for (count = 0; count < 8; ++count) {

            recon0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
            recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));

            signLeft = _mm_packs_epi16(_mm_sub_epi16(_mm_or_si128(_mm_slli_si128(recon0_7, 2), _mm_and_si128(tempBufferLeft, xmm_255)), recon0_7),
                                       _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + 7)), recon8_15));
                                       
            signRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + 1)), recon0_7),
                                        _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + 9)), recon8_15));

            eoIndex = _mm_add_epi8(_mm_add_epi8(_mm_cmpgt_epi8(signLeft, xmm_0), _mm_cmpgt_epi8(signLeft, xmm_N1)),
                                   _mm_add_epi8(_mm_cmpgt_epi8(signRight, xmm_0), _mm_cmpgt_epi8(signRight, xmm_N1)));
            
            saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_N4), saoOffset0),
                                                     _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_N3), saoOffset1)),
                                        _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_N1), saoOffset2),
                                                     _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_0), saoOffset3)));

            sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);

            clip3_0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(recon0_7,  _mm_unpacklo_epi8(saoOffset_eo, sign)), xmm_Max10bit), xmm_0);
            clip3_8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(recon8_15, _mm_unpackhi_epi8(saoOffset_eo, sign)), xmm_Max10bit), xmm_0);
            
            recon15_22 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 15));

            _mm_storeu_si128((__m128i *)(reconSamplePtr), clip3_0_7);
            _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip3_8_15);

            tempBufferLeft = _mm_srli_si128(tempBufferLeft, 2);


            for (width_cnt = 0; width_cnt < lcuWidth; width_cnt += 16){

                __m128i recon16_23, recon23_30, recon24_31, clip3_16_23, clip3_24_31;

                reconSamplePtr += 16;

                recon16_23 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
                recon23_30 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 7));
                recon24_31 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));
                
                signLeft = _mm_packs_epi16(_mm_sub_epi16(recon15_22, recon16_23), _mm_sub_epi16(recon23_30, recon24_31));

                signRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + 1)), recon16_23),
                                            _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + 9)), recon24_31));

                eoIndex = _mm_add_epi8(_mm_add_epi8(_mm_cmpgt_epi8(signLeft, xmm_0), _mm_cmpgt_epi8(signLeft, xmm_N1)),
                                       _mm_add_epi8(_mm_cmpgt_epi8(signRight, xmm_0), _mm_cmpgt_epi8(signRight, xmm_N1)));

                // saoOffsetPtr[eoIndex]
                saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_N4), saoOffset0),
                                                         _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_N3), saoOffset1)),
                                            _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_N1), saoOffset2),
                                                         _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_0), saoOffset3)));

                sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);

                clip3_16_23 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(recon16_23, _mm_unpacklo_epi8(saoOffset_eo, sign)), xmm_Max10bit), xmm_0);
                clip3_24_31 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(recon24_31, _mm_unpackhi_epi8(saoOffset_eo, sign)), xmm_Max10bit), xmm_0);

                recon15_22 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 15));

                _mm_storeu_si128((__m128i *)(reconSamplePtr), clip3_16_23);
                _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip3_24_31);
            }
            reconSamplePtr += reconStride;
        }
    }
    return EB_ErrorNone;
}

extern EB_ERRORTYPE SAOApplyEO_135_16bit_SSE2_INTRIN(
    EB_U16 *reconSamplePtr,
    EB_U32  reconStride,
    EB_U16 *temporalBufferLeft,
    EB_U16 *temporalBufferUpper,
    EB_S8  *saoOffsetPtr,
    EB_U32  lcuHeight,
    EB_U32  lcuWidth)
{
    EB_ALIGN(16) EB_S8 signTopLeftTemp[MAX_LCU_SIZE + 1];
    EB_ALIGN(16) EB_S8 * signTLBuf = signTopLeftTemp;
    EB_U32 width_cnt;
    EB_S32 height_cnt; // needs to signed
    __m128i xmm_0, xmm_n1, xmm_n3, xmm_n4, up_n1_6, up_7_14;
    __m128i saoOffset0, saoOffset1, saoOffset2, saoOffset3, saoOffset0_7, saoOffset0_4;
    __m128i recon0_7, recon8_15, signTopLeft, signBotRight, eoIndex, saoOffset_eo, sign, Max10bit, clip0_7, clip8_15;

    Max10bit = _mm_set1_epi16(0x03FF);
    xmm_0 = _mm_setzero_si128();
    xmm_n1 = _mm_set1_epi8(-1);
    xmm_n3 = _mm_set1_epi8(-3);
    xmm_n4 = _mm_set1_epi8(-4);
    saoOffset0_7 = _mm_loadl_epi64((__m128i *)(saoOffsetPtr)); //01x34567..
    saoOffset0_7 = _mm_unpacklo_epi8(saoOffset0_7, saoOffset0_7); //0011xx3344556677
    saoOffset0_4 = _mm_unpacklo_epi32(saoOffset0_7, _mm_srli_si128(saoOffset0_7, 6)); //00113344...
    saoOffset0_4 = _mm_unpacklo_epi16(saoOffset0_4, saoOffset0_4); // 0000111133334444
    saoOffset0 = _mm_shuffle_epi32(saoOffset0_4, 0x00); //0000000000000000
    saoOffset1 = _mm_shuffle_epi32(saoOffset0_4, 0x55); //1111111111111111
    saoOffset2 = _mm_shuffle_epi32(saoOffset0_4, 0xaa);
    saoOffset3 = _mm_shuffle_epi32(saoOffset0_4, 0xff);
    
    // First row
    for (width_cnt = 0; width_cnt < lcuWidth; width_cnt += 16) {

        up_n1_6 = _mm_loadu_si128((__m128i *)(temporalBufferUpper - 1));
        up_7_14 = _mm_loadu_si128((__m128i *)(temporalBufferUpper + 7));
        temporalBufferUpper += 16;
        recon0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
        recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));
        signTopLeft = _mm_packs_epi16(_mm_sub_epi16(up_n1_6, recon0_7), _mm_sub_epi16(up_7_14, recon8_15));
        signTopLeft = _mm_add_epi8(_mm_cmpgt_epi8(signTopLeft, xmm_0), _mm_cmpgt_epi8(signTopLeft, xmm_n1));

        signBotRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + reconStride + 1)), recon0_7),
                                       _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + reconStride + 9)), recon8_15));
        signBotRight = _mm_add_epi8(_mm_cmpgt_epi8(signBotRight, xmm_0), _mm_cmpgt_epi8(signBotRight, xmm_n1));
        
        eoIndex = _mm_add_epi8(signTopLeft, signBotRight);
        _mm_storeu_si128((__m128i *)(signTLBuf + 1), signBotRight); // the next signTopLeft
        signTLBuf += 16;

        saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n4), saoOffset0),
                                                 _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n3), saoOffset1)),
                                    _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n1), saoOffset2),
                                                 _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_0), saoOffset3)));

        sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);
        clip0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpacklo_epi8(saoOffset_eo, sign), recon0_7) , Max10bit), xmm_0);
        clip8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpackhi_epi8(saoOffset_eo, sign), recon8_15), Max10bit), xmm_0);
        _mm_storeu_si128((__m128i *)(reconSamplePtr), clip0_7);
        _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip8_15);
        reconSamplePtr += 16;
    }

    EB_U32 reconStrideTemp = reconStride - lcuWidth;
    EB_S32 count;
    signTLBuf -= lcuWidth;
    reconSamplePtr += reconStrideTemp;
    
    __m128i xmm_n2 = _mm_set1_epi8(-2);
    __m128i xmm_1 = _mm_set1_epi8(1);
    __m128i left;

    lcuHeight--; // Already written first row, total number for rows decrease by 1
    lcuWidth -= 16;
    
    for (height_cnt = lcuHeight; height_cnt > 0 ; height_cnt -= 8) { //count backwards as it helps with the subsequence for loop's exit condition
   
            left = _mm_loadu_si128((__m128i *)(temporalBufferLeft));
            temporalBufferLeft += 8;
   
        for (count = 0; count < (height_cnt == 7 ? 7 : 8); ++count) {
             
            recon0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
            recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));
            signTopLeft = _mm_sub_epi16(recon0_7, left);
            left = _mm_srli_si128(left, 2);
            signTopLeft = _mm_add_epi8(_mm_cmpgt_epi16(signTopLeft, xmm_0), _mm_cmpgt_epi16(signTopLeft, xmm_n1));
            signTopLeft = _mm_or_si128(_mm_slli_si128(_mm_loadu_si128((__m128i *)(signTLBuf + 1)), 1), _mm_srli_si128(_mm_slli_si128(signTopLeft, 15), 15));

            signBotRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + reconStride + 1)), recon0_7),
                                           _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + reconStride + 9)), recon8_15));

            signBotRight = _mm_add_epi8(_mm_cmpgt_epi8(signBotRight, xmm_0), _mm_cmpgt_epi8(signBotRight, xmm_n1));
            eoIndex = _mm_sub_epi8(signTopLeft, signBotRight);
            signTopLeft = _mm_load_si128((__m128i *)(signTLBuf + 16)); // For for loop below
            _mm_storeu_si128((__m128i *)(signTLBuf + 1), signBotRight);

            saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpgt_epi8(eoIndex, xmm_1), saoOffset0),
                                                     _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_1), saoOffset1)),
                                        _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n1), saoOffset2),
                                                     _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n2), saoOffset3)));

            sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);
            clip0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpacklo_epi8(saoOffset_eo, sign), recon0_7) , Max10bit), xmm_0);
            clip8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpackhi_epi8(saoOffset_eo, sign), recon8_15), Max10bit), xmm_0);
            _mm_storeu_si128((__m128i *)(reconSamplePtr), clip0_7);
            _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip8_15);
            reconSamplePtr += 16;

            for (width_cnt = 0; width_cnt < lcuWidth; width_cnt += 16) {
                
                signTLBuf += 16;
                recon0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
                recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));
                signBotRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + reconStride + 1)), recon0_7),
                                               _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + reconStride + 9)), recon8_15));

                signBotRight = _mm_add_epi8(_mm_cmpgt_epi8(signBotRight, xmm_0), _mm_cmpgt_epi8(signBotRight, xmm_n1));
                eoIndex = _mm_sub_epi8(signTopLeft, signBotRight);
                signTopLeft = _mm_load_si128((__m128i *)(signTLBuf + 16));
                _mm_storeu_si128((__m128i *)(signTLBuf + 1), signBotRight); // Next signTopLeft

                saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpgt_epi8(eoIndex, xmm_1), saoOffset0),
                                                         _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_1), saoOffset1)),
                                            _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n1), saoOffset2),
                                                         _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n2), saoOffset3)));
                sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);
                clip0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpacklo_epi8(saoOffset_eo, sign), recon0_7) , Max10bit), xmm_0);
                clip8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpackhi_epi8(saoOffset_eo, sign), recon8_15), Max10bit), xmm_0);
                _mm_storeu_si128((__m128i *)(reconSamplePtr), clip0_7);
                _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip8_15);
                reconSamplePtr += 16;
            }
            signTLBuf -= lcuWidth;
            reconSamplePtr += reconStrideTemp;           
        }
    }
    return EB_ErrorNone;
}

EB_ERRORTYPE SAOApplyEO_45_16bit_SSE2_INTRIN(
    EB_U16                           *reconSamplePtr,
    EB_U32                           reconStride,
    EB_U16                           *temporalBufferLeft,
    EB_U16                           *temporalBufferUpper,
    EB_S8                           *saoOffsetPtr,
    EB_U32                           lcuHeight,
    EB_U32                           lcuWidth)
{
    EB_ALIGN(16) EB_S8 signTopRightTempBuffer[MAX_LCU_SIZE + 1];
    EB_ALIGN(16) EB_S8 * signTopRightBufp = signTopRightTempBuffer;
    
    __m128i xmm_1, xmm_0, xmm_n1, xmm_n2, xmm_n3, xmm_n4, Max10bit, saoOffset0_7, saoOffset0_4, saoOffset0, saoOffset1, saoOffset2, saoOffset3;
    __m128i recon0_7, recon8_15, signBotLeft, signTopRight, eoIndex, sign, saoOffset_eo, clip0_7, clip8_15, xmm_temporalBufLeft, signBotLeft0, keep_lsb_mask;
    EB_U32 width_cnt;
    EB_S32 reconStrideTemp, height_cnt, count;
    
    xmm_0 = _mm_setzero_si128();
    xmm_n1 = _mm_set1_epi8(-1);
    xmm_n3 = _mm_set1_epi8(-3);
    xmm_n4 = _mm_set1_epi8(-4);
    Max10bit = _mm_set1_epi16(0x03FF);
    keep_lsb_mask = _mm_srli_si128(xmm_n1, 15);

    // We use offset 0,1,3,4 in the calculation because saoOffset[2] is always 0
    saoOffset0_7 = _mm_loadl_epi64((__m128i *)(saoOffsetPtr)); //01x34567..
    saoOffset0_7 = _mm_unpacklo_epi8(saoOffset0_7, saoOffset0_7); //0011xx3344556677
    saoOffset0_4 = _mm_unpacklo_epi32(saoOffset0_7, _mm_srli_si128(saoOffset0_7, 6)); //00113344...
    saoOffset0_4 = _mm_unpacklo_epi16(saoOffset0_4, saoOffset0_4); // 0000111133334444
    saoOffset0 = _mm_shuffle_epi32(saoOffset0_4, 0x00); //0000000000000000
    saoOffset1 = _mm_shuffle_epi32(saoOffset0_4, 0x55); //1111111111111111
    saoOffset2 = _mm_shuffle_epi32(saoOffset0_4, 0xaa);
    saoOffset3 = _mm_shuffle_epi32(saoOffset0_4, 0xff);
    
    //Seek to beginning of last row
    
    temporalBufferLeft += (lcuHeight - 7);
    reconSamplePtr += (reconStride * (lcuHeight - 1));    
    lcuHeight -= 2; // First row and last row are done separately
    lcuWidth -= 16; // First col has different eoIndex calculation thus is not done in loop

    // Write last row

    xmm_temporalBufLeft = _mm_loadu_si128((__m128i *)(temporalBufferLeft));
    temporalBufferLeft--;
    recon0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
    recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));

    signBotLeft0 = _mm_sub_epi16(_mm_srli_si128(xmm_temporalBufLeft, 14), recon0_7);

    signBotLeft0 = _mm_and_si128(keep_lsb_mask, _mm_add_epi8(_mm_cmpgt_epi16(signBotLeft0, xmm_0), _mm_cmpgt_epi16(signBotLeft0, xmm_n1)));

    signBotLeft = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + reconStride - 1)), recon0_7),
                                  _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + reconStride + 7)), recon8_15));

    signBotLeft = _mm_add_epi8(_mm_cmpgt_epi8(signBotLeft, xmm_0), _mm_cmpgt_epi8(signBotLeft, xmm_n1));
    signBotLeft = _mm_or_si128(signBotLeft0, _mm_and_si128(signBotLeft, _mm_slli_si128(xmm_n1, 1)));

    signTopRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr - reconStride + 1)), recon0_7),
                                   _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr - reconStride + 9)), recon8_15));
    signTopRight = _mm_add_epi8(_mm_cmpgt_epi8(signTopRight, xmm_0), _mm_cmpgt_epi8(signTopRight, xmm_n1));

    eoIndex = _mm_add_epi8(signBotLeft, signTopRight);

    _mm_storeu_si128((__m128i *)(signTopRightBufp + 1), signTopRight);
        
    saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n4), saoOffset0), _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n3), saoOffset1)),
                                _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n1), saoOffset2), _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_0), saoOffset3)));

    sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);
    clip0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpacklo_epi8(saoOffset_eo, sign), recon0_7), Max10bit), xmm_0);
    clip8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpackhi_epi8(saoOffset_eo, sign), recon8_15), Max10bit), xmm_0);
    _mm_storeu_si128((__m128i *)(reconSamplePtr), clip0_7);
    _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip8_15);
    
    reconSamplePtr += 16;

    for (width_cnt = 0; width_cnt < lcuWidth; width_cnt += 16) {
    
        signTopRightBufp += 16;
        
        recon0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
        recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));

        signBotLeft = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + reconStride - 1)), recon0_7),
                                      _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr + reconStride + 7)), recon8_15));

        signBotLeft = _mm_add_epi8(_mm_cmpgt_epi8(signBotLeft, xmm_0), _mm_cmpgt_epi8(signBotLeft, xmm_n1));

        signTopRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr - reconStride + 1)), recon0_7),
                                       _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr - reconStride + 9)), recon8_15));
        signTopRight = _mm_add_epi8(_mm_cmpgt_epi8(signTopRight, xmm_0), _mm_cmpgt_epi8(signTopRight, xmm_n1));

        eoIndex = _mm_add_epi8(signBotLeft, signTopRight);

        _mm_storeu_si128((__m128i *)(signTopRightBufp + 1), signTopRight);
        
        saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n4), saoOffset0), _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n3), saoOffset1)),
                                    _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n1), saoOffset2), _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_0), saoOffset3)));

        sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);
        clip0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpacklo_epi8(saoOffset_eo, sign), recon0_7), Max10bit), xmm_0);
        clip8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpackhi_epi8(saoOffset_eo, sign), recon8_15), Max10bit), xmm_0);
        _mm_storeu_si128((__m128i *)(reconSamplePtr), clip0_7);
        _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip8_15);

        reconSamplePtr += 16;    
    }

    reconStrideTemp = -1 *(reconStride) - lcuWidth - 16;
    signTopRightBufp -= lcuWidth;
    reconSamplePtr += reconStrideTemp;
    
    xmm_1 = _mm_set1_epi8(1);
    xmm_n2 = _mm_set1_epi8(-2);

    for (height_cnt = lcuHeight; height_cnt > 0; height_cnt -= 8) {

        xmm_temporalBufLeft = _mm_loadu_si128((__m128i *)(temporalBufferLeft));
        temporalBufferLeft -= 8;

        for (count = 0; count < (height_cnt == 6 ? 6 : 8); ++count) {

            recon0_7 = _mm_loadu_si128((__m128i *)reconSamplePtr);
            recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));
            
            signBotLeft = _mm_sub_epi16(recon0_7, _mm_srli_si128(xmm_temporalBufLeft, 14));
            
            signBotLeft = _mm_and_si128(keep_lsb_mask, _mm_add_epi8(_mm_cmpgt_epi16(signBotLeft, xmm_0), _mm_cmpgt_epi16(signBotLeft, xmm_n1)));

            signBotLeft = _mm_or_si128(_mm_slli_si128(_mm_loadu_si128((__m128i *)(signTopRightBufp + 1)), 1), signBotLeft);
            
            signTopRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr - reconStride + 1)), recon0_7),
                                           _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr - reconStride + 9)), recon8_15));
            
            signTopRight = _mm_add_epi8(_mm_cmpgt_epi8(signTopRight, xmm_0), _mm_cmpgt_epi8(signTopRight, xmm_n1));
            
            eoIndex = _mm_sub_epi8(signBotLeft, signTopRight);
            
            signBotLeft = _mm_load_si128((__m128i *)(signTopRightBufp + 16)); // Variable is used in nested for loop below

            _mm_storeu_si128((__m128i *)(signTopRightBufp + 1), signTopRight);

            saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpgt_epi8(eoIndex, xmm_1), saoOffset0),
                                                     _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_1), saoOffset1)),
                                        _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n1), saoOffset2),
                                                     _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n2), saoOffset3)));

            sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);
            clip0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpacklo_epi8(saoOffset_eo, sign), recon0_7 ), Max10bit), xmm_0);
            clip8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpackhi_epi8(saoOffset_eo, sign), recon8_15), Max10bit), xmm_0);
            _mm_storeu_si128((__m128i *)(reconSamplePtr), clip0_7);
            _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip8_15);
            
            // Next iteration
            xmm_temporalBufLeft = _mm_slli_si128(xmm_temporalBufLeft, 2);
            reconSamplePtr += 16;

            for (width_cnt = 0; width_cnt < lcuWidth; width_cnt += 16) {

                signTopRightBufp += 16;

                recon0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
                recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));

                signTopRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr - reconStride + 1)), recon0_7),
                                               _mm_sub_epi16(_mm_loadu_si128((__m128i *)(reconSamplePtr - reconStride + 9)), recon8_15));
                signTopRight = _mm_add_epi8(_mm_cmpgt_epi8(signTopRight, xmm_0), _mm_cmpgt_epi8(signTopRight, xmm_n1));
                
                eoIndex = _mm_sub_epi8(signBotLeft, signTopRight);
                
                signBotLeft = _mm_load_si128((__m128i *)(signTopRightBufp + 16)); //Next iteration
                
                _mm_storeu_si128((__m128i*)(signTopRightBufp + 1), signTopRight);

                saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpgt_epi8(eoIndex, xmm_1), saoOffset0),
                                                         _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_1), saoOffset1)),
                                            _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n1), saoOffset2),
                                                         _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n2), saoOffset3)));
                sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);
                clip0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpacklo_epi8(saoOffset_eo, sign), recon0_7 ), Max10bit), xmm_0);
                clip8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpackhi_epi8(saoOffset_eo, sign), recon8_15), Max10bit), xmm_0);
                _mm_storeu_si128((__m128i *)(reconSamplePtr), clip0_7);
                _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip8_15);
                
                reconSamplePtr += 16;
            }
            signTopRightBufp -= lcuWidth;
            reconSamplePtr += reconStrideTemp;   
        }
    }

    // First row

    recon0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
    recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));

    signBotLeft = _mm_sub_epi16(recon0_7, _mm_srli_si128(xmm_temporalBufLeft, 14));    
    xmm_temporalBufLeft = _mm_slli_si128(xmm_temporalBufLeft, 2);

    signBotLeft = _mm_and_si128(keep_lsb_mask, _mm_add_epi8(_mm_cmpgt_epi16(signBotLeft, xmm_0), _mm_cmpgt_epi16(signBotLeft, xmm_n1)));

    signBotLeft = _mm_or_si128(_mm_slli_si128(_mm_loadu_si128((__m128i *)(signTopRightBufp + 1)), 1), signBotLeft);

    signTopRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(temporalBufferUpper + 1)), recon0_7),
                                   _mm_sub_epi16(_mm_loadu_si128((__m128i *)(temporalBufferUpper + 9)), recon8_15));
    temporalBufferUpper += 16;
    signTopRight = _mm_add_epi8(_mm_cmpgt_epi8(signTopRight, xmm_0), _mm_cmpgt_epi8(signTopRight, xmm_n1));
    eoIndex = _mm_sub_epi8(signBotLeft, signTopRight);

    signBotLeft = _mm_load_si128((__m128i *)(signTopRightBufp + 16)); //For for-loop below

    _mm_storeu_si128((__m128i *)(signTopRightBufp + 1), signTopRight);

    saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpgt_epi8(eoIndex, xmm_1), saoOffset0),
                                             _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_1), saoOffset1)),
                                _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n1), saoOffset2),
                                             _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n2), saoOffset3)));
    
    sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);
    
    clip0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpacklo_epi8(saoOffset_eo, sign), recon0_7 ), Max10bit), xmm_0);
    clip8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpackhi_epi8(saoOffset_eo, sign), recon8_15), Max10bit), xmm_0);
    
    _mm_storeu_si128((__m128i *)(reconSamplePtr), clip0_7);
    _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip8_15);

    reconSamplePtr += 16;
    
    for (width_cnt = 0; width_cnt < lcuWidth; width_cnt += 16) {
    
        signTopRightBufp += 16;

        recon0_7 = _mm_loadu_si128((__m128i *)(reconSamplePtr));
        recon8_15 = _mm_loadu_si128((__m128i *)(reconSamplePtr + 8));

        signTopRight = _mm_packs_epi16(_mm_sub_epi16(_mm_loadu_si128((__m128i *)(temporalBufferUpper + 1)), recon0_7),
                                       _mm_sub_epi16(_mm_loadu_si128((__m128i *)(temporalBufferUpper + 9)), recon8_15));
        temporalBufferUpper += 16;
        signTopRight = _mm_add_epi8(_mm_cmpgt_epi8(signTopRight, xmm_0), _mm_cmpgt_epi8(signTopRight, xmm_n1));
        eoIndex = _mm_sub_epi8(signBotLeft, signTopRight);
        signBotLeft = _mm_load_si128((__m128i *)(signTopRightBufp + 16)); //Used in the next iteration
        _mm_storeu_si128((__m128i*)(signTopRightBufp + 1), signTopRight);
        

        saoOffset_eo = _mm_or_si128(_mm_or_si128(_mm_and_si128(_mm_cmpgt_epi8(eoIndex, xmm_1), saoOffset0),
                                                 _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_1), saoOffset1)),
                                    _mm_or_si128(_mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n1), saoOffset2),
                                                 _mm_and_si128(_mm_cmpeq_epi8(eoIndex, xmm_n2), saoOffset3)));
        sign = _mm_cmpgt_epi8(xmm_0, saoOffset_eo);
        clip0_7 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpacklo_epi8(saoOffset_eo, sign), recon0_7 ), Max10bit), xmm_0);
        clip8_15 = _mm_max_epi16(_mm_min_epi16(_mm_add_epi16(_mm_unpackhi_epi8(saoOffset_eo, sign), recon8_15), Max10bit), xmm_0);
        _mm_storeu_si128((__m128i *)(reconSamplePtr), clip0_7);
        _mm_storeu_si128((__m128i *)(reconSamplePtr + 8), clip8_15);
        reconSamplePtr += 16;
    }
     return EB_ErrorNone;
}
