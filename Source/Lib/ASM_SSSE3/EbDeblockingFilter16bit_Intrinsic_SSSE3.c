/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "tmmintrin.h"
#include "EbDefinitions.h"

EB_EXTERN void Luma4SampleEdgeDLFCore16bit_SSSE3_INTRIN(
    EB_U16*                edgeStartFilteredSamplePtr,
    EB_U32                 reconLumaPicStride,
    EB_BOOL                isVerticalEdge,
    EB_S32                 tc,
    EB_S32                 beta)
{
    EB_S32  d;
    EB_BOOL strongFiltering;
    __m128i xmm_tc;
    __m128i xmm0  = _mm_setzero_si128();
    __m128i xmm_4 = _mm_setr_epi16(4, 4, 4, 4, 4, 4, 4, 4);

    xmm_tc = _mm_cvtsi32_si128(tc);
    xmm_tc = _mm_unpacklo_epi16(xmm_tc, xmm_tc);
    xmm_tc = _mm_unpacklo_epi32(xmm_tc, xmm_tc);
    xmm_tc = _mm_unpacklo_epi64(xmm_tc, xmm_tc);

    // dp = | p22 - 2*p12 + p02 |  +  | p25 - 2*p15 + p05 |
    // dp = | q22 - 2*q12 + q02 |  +  | q25 - 2*q15 + q05 |
    if (!isVerticalEdge) {
        __m128i xmm_beta;// = _mm_setzero_si128();
        __m128i xmm_Max10bit;// = _mm_setzero_si128();
        __m128i xmm_t1;// = _mm_setzero_si128();
        __m128i xmm_t2;// = _mm_setzero_si128();
        __m128i xmm_t3;// = _mm_setzero_si128();
        __m128i xmm_t4;// = _mm_setzero_si128();
        __m128i xmm_pq3 = _mm_loadl_epi64((__m128i *)(edgeStartFilteredSamplePtr-4*reconLumaPicStride));
        __m128i xmm_pq2 = _mm_loadl_epi64((__m128i *)(edgeStartFilteredSamplePtr-3*reconLumaPicStride));
        __m128i xmm_pq1 = _mm_loadl_epi64((__m128i *)(edgeStartFilteredSamplePtr-2*reconLumaPicStride));
        __m128i xmm_pq0 = _mm_loadl_epi64((__m128i *)(edgeStartFilteredSamplePtr-1*reconLumaPicStride));
        __m128i xmm_qp0 = _mm_loadl_epi64((__m128i *)edgeStartFilteredSamplePtr);
        __m128i xmm_qp1 = _mm_loadl_epi64((__m128i *)(edgeStartFilteredSamplePtr+1*reconLumaPicStride));
        __m128i xmm_qp2 = _mm_loadl_epi64((__m128i *)(edgeStartFilteredSamplePtr+2*reconLumaPicStride));
        __m128i xmm_qp3 = _mm_loadl_epi64((__m128i *)(edgeStartFilteredSamplePtr+3*reconLumaPicStride));

        xmm_pq0 = _mm_unpacklo_epi64(xmm_pq0, xmm_qp0);
        xmm_pq1 = _mm_unpacklo_epi64(xmm_pq1, xmm_qp1);
        xmm_pq2 = _mm_unpacklo_epi64(xmm_pq2, xmm_qp2);
        xmm_pq3 = _mm_unpacklo_epi64(xmm_pq3, xmm_qp3);
        xmm_qp0 = _mm_unpacklo_epi64(xmm_qp0, xmm_pq0);
        xmm_qp1 = _mm_unpacklo_epi64(xmm_qp1, xmm_pq1);
        xmm_qp2 = _mm_unpacklo_epi64(xmm_qp2, xmm_pq2);
        xmm_qp3 = _mm_unpacklo_epi64(xmm_qp3, xmm_pq3);

        xmm_t2 = _mm_add_epi16(xmm_pq1, xmm_pq1); // 2*edgeStartSample[-2*filterStride]
        xmm_t2 = _mm_sub_epi16(xmm_t2, xmm_pq0);  // 2*edgeStartSample[-2*filterStride] - edgeStartSample[-filterStride]
        xmm_t2 = _mm_sub_epi16(xmm_t2, xmm_pq2);  // 2*edgeStartSample[-2*filterStride] - edgeStartSample[-filterStride] - edgeStartSample[-3*filterStride]
        xmm_t2 = _mm_abs_epi16(xmm_t2);           // dp0 = ABS(edgeStartSample[-3*filterStride] - 2*edgeStartSample[-2*filterStride] + edgeStartSample[-filterStride]);
                                                  // xmm_t2 = 0x dq3 xx xx dq0 dp3 xx xx dp0
        // calculate d
        xmm_t1 = xmm_t2;
        xmm_t2 = _mm_srli_si128(xmm_t2, 8);
        xmm_t2 = _mm_add_epi16(xmm_t2, xmm_t1); // xmm_t2 = 0x xx xx xx xx (d3 = dp3 + dq3) xx xx (d0 = dp0 + dq0)
        xmm_t3 = xmm_t2;
        xmm_t2 = _mm_srli_si128(xmm_t2, 6);
        xmm_t2 = _mm_add_epi16(xmm_t2, xmm_t3);
        d = _mm_extract_epi16(xmm_t2, 0); // d  = d0  + d3;

        // decision if d > beta : no filtering
        if (d < beta)
        {
            xmm_beta = _mm_cvtsi32_si128(beta);
            xmm_beta = _mm_unpacklo_epi16(xmm_beta, xmm_beta);
            xmm_beta = _mm_unpacklo_epi32(xmm_beta, xmm_beta);
            xmm_beta = _mm_unpacklo_epi64(xmm_beta, xmm_beta);
            xmm_t3 = _mm_slli_epi16(xmm_t3, 1);       // d0<<1
            xmm_t2 = _mm_srai_epi16(xmm_beta, 2);     // beta>>2
            xmm_t2 = _mm_cmpgt_epi16(xmm_t2, xmm_t3); // (d0<<1) < (beta>>2)

            xmm_t3 = _mm_sub_epi16(xmm_pq3, xmm_pq0); // filteredP3-filteredP0
            xmm_t3 = _mm_abs_epi16(xmm_t3);           // ABS(filteredP3-filteredP0)
            xmm_t4 = xmm_t3;
            xmm_t3 = _mm_srli_si128(xmm_t3, 8);
            xmm_t3 = _mm_add_epi16(xmm_t3, xmm_t4);   // ABS(filteredP3-filteredP0)+ABS(filteredQ3-filteredQ0)
            xmm_t4 = _mm_srai_epi16(xmm_beta, 3);     // beta>>3
            xmm_t4 = _mm_cmpgt_epi16(xmm_t4, xmm_t3); // (beta>>3) > (ABS(filteredP3-filteredP0)+ABS(filteredQ3-filteredQ0))
            xmm_t2 = _mm_and_si128(xmm_t2, xmm_t4);   // (d0<<1) < (beta>>2)) && (beta>>3) > (ABS(filteredP3-filteredP0)+ABS(filteredQ3-filteredQ0))

            xmm_t3 = _mm_sub_epi16(xmm_pq0, xmm_qp0); // filteredP0-filteredQ0
            xmm_t3 = _mm_abs_epi16(xmm_t3);           // ABS(filteredP0-filteredQ0)

            xmm_t4 = _mm_slli_epi16(xmm_tc, 2);
            xmm_t4 = _mm_add_epi16(xmm_t4, xmm_tc);   // 5*tc
            xmm_t4 = _mm_avg_epu16(xmm_t4, xmm0);     // (5*tc+1)>>1
            xmm_t4 = _mm_cmpgt_epi16(xmm_t4, xmm_t3); // ((5*tc+1)>>1) > ABS(filteredP0-filteredQ0)
            xmm_t2 = _mm_and_si128(xmm_t2, xmm_t4);   // ((d0<<1) < (beta>>2)) && (beta>>3) > (ABS(filteredP3-filteredP0)+ABS(filteredQ3-filteredQ0)) && ((5*tc+1)>>1) > ABS(filteredP0-filteredQ0)

            xmm_t3 = _mm_srli_si128(xmm_t2, 6);
            xmm_t2 = _mm_and_si128(xmm_t2, xmm_t3);
            strongFiltering = (EB_BOOL)_mm_extract_epi16(xmm_t2, 0); // strongFiltering = strongFilteringLine0 && strongFilteringLine3;

            xmm_t2 = xmm_qp0;

            if (strongFiltering) {
                xmm_tc = _mm_slli_epi16(xmm_tc, 1);
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_pq0); // filteredP0 + filteredQ0
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp1); // filteredP0 + filteredQ0 + filteredQ1
                xmm_t2 = _mm_slli_epi16(xmm_t2, 1);      // (filteredP0 + filteredQ0 + filteredQ1) << 1
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_pq1); // filteredP1 + ((filteredP0 + filteredQ0 + filteredQ1) << 1)
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp2); // filteredP1 + ((filteredP0 + filteredQ0 + filteredQ1) << 1) + filteredQ2
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_4);   // filteredP1 + ((filteredP0 + filteredQ0 + filteredQ1) << 1) + filteredQ2 + 4
                xmm_t2 = _mm_srai_epi16(xmm_t2, 3);      // (filteredP1 + ((filteredP0 + filteredQ0 + filteredQ1) << 1) + filteredQ2 + 4) >> 3

                xmm_t3 = _mm_sub_epi16(xmm_qp0, xmm_tc); // filteredQ0-(tc<<1)
                xmm_t4 = _mm_add_epi16(xmm_qp0, xmm_tc); // filteredQ0+(tc<<1)
                xmm_t2 = _mm_max_epi16(xmm_t2, xmm_t3);
                xmm_t2 = _mm_min_epi16(xmm_t2, xmm_t4);  // CLIP3
                _mm_storel_epi64((__m128i *)edgeStartFilteredSamplePtr, xmm_t2); // edgeStartSample[i]
                xmm_t2 = _mm_srli_si128(xmm_t2, 8);
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr-reconLumaPicStride), xmm_t2); // edgeStartSample[-filterStride+i]

                xmm_t2 = _mm_add_epi16(xmm_qp2, xmm_qp3);
                xmm_t2 = _mm_slli_epi16(xmm_t2, 1);
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp2); // filteredQ2*3 + (filteredQ3<<1)
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_pq0); // filteredP0 + filteredQ2*3 + (filteredQ3<<1)
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp0); // filteredP0 + filteredQ0 + filteredQ2*3 + (filteredQ3<<1)
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp1); // filteredP0 + filteredQ0 + filteredQ1 + filteredQ2*3 + (filteredQ3<<1)
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_4);   // filteredP0 + filteredQ0 + filteredQ1 + filteredQ2*3 + (filteredQ3<<1) + 4
                xmm_4  = _mm_srli_epi16(xmm_4, 1);       // xmm_4 = 2
                xmm_t2 = _mm_srai_epi16(xmm_t2, 3);      // (filteredP0 + filteredQ0 + filteredQ1 + filteredQ2*3 + (filteredQ3<<1) + 4) >> 3

                xmm_t3 = _mm_sub_epi16(xmm_qp2, xmm_tc); // filteredQ2-(tc<<1)
                xmm_t4 = _mm_add_epi16(xmm_qp2, xmm_tc); // filteredQ2+(tc<<1)
                xmm_t2 = _mm_max_epi16(xmm_t2, xmm_t3);
                xmm_t2 = _mm_min_epi16(xmm_t2, xmm_t4);  // CLIP3
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr+2*reconLumaPicStride), xmm_t2); // edgeStartSample[2*filterStride+i]
                xmm_t2 = _mm_srli_si128(xmm_t2, 8);
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr-3*reconLumaPicStride), xmm_t2); // edgeStartSample[-3*filterStride+i]

                xmm_t2 = _mm_add_epi16(xmm_pq0, xmm_qp0); // filteredP0 + filteredQ0
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp1);  // filteredP0 + filteredQ0 + filteredQ1
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp2);  // filteredP0 + filteredQ0 + filteredQ1 + filteredQ2
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_4);    // filteredP0 + filteredQ0 + filteredQ1 + filteredQ2 + 2
                xmm_t2 = _mm_srai_epi16(xmm_t2, 2);       // (filteredP0 + filteredQ0 + filteredQ1 + filteredQ2 + 2) >> 2

                xmm_t3 = _mm_sub_epi16(xmm_qp1, xmm_tc);  // filteredQ1-(tc<<1)
                xmm_t4 = _mm_add_epi16(xmm_qp1, xmm_tc);  // filteredQ1+(tc<<1)
                xmm_t2 = _mm_max_epi16(xmm_t2, xmm_t3);
                xmm_t2 = _mm_min_epi16(xmm_t2, xmm_t4);   // CLIP3
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr+  reconLumaPicStride), xmm_t2); // edgeStartSample[filterStride+i]
                xmm_t2 = _mm_srli_si128(xmm_t2, 8);
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr-2*reconLumaPicStride), xmm_t2); // edgeStartSample[-2*filterStride+i]
            }
            else { // weak filtering
                // calculate delta, ithreshcut
                xmm_4 = _mm_slli_epi16(xmm_4, 1);
                xmm_t2 = _mm_sub_epi16(xmm_t2, xmm_pq0);  // filteredQ0 - filteredP0
                xmm_t3 = _mm_slli_epi16(xmm_t2, 3);
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_t3);   // (filteredQ0 - filteredP0) * 9
                xmm_t4 = _mm_sub_epi16(xmm_qp1, xmm_pq1); // filteredQ1 - filteredP1
                xmm_t2 = _mm_sub_epi16(xmm_t2, xmm_t4);
                xmm_t4 = _mm_slli_epi16(xmm_t4, 1);
                xmm_t2 = _mm_sub_epi16(xmm_t2, xmm_t4);   // (filteredQ0 - filteredP0) * 9 - (filteredQ1 - filteredP1) * 3
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_4);    // (filteredQ0 - filteredP0) * 9 - (filteredQ1 - filteredP1) * 3 + 8
                xmm_t2 = _mm_srai_epi16(xmm_t2, 4);       // delta = ((filteredQ0 - filteredP0) * 9 - (filteredQ1 - filteredP1) * 3 + 8) >> 4;

                xmm_t4 = _mm_abs_epi16(xmm_t2);           // ABS(delta)
                xmm_t3 = _mm_slli_epi16(xmm_tc, 2);
                xmm_t3 = _mm_add_epi16(xmm_t3, xmm_tc);
                xmm_t3 = _mm_slli_epi16(xmm_t3, 1);       // threshWeakFiltering = tc * 10;
                xmm_t3 = _mm_cmpgt_epi16(xmm_t3, xmm_t4); // if (ABS(delta) < threshWeakFiltering) {
                xmm_t3 = _mm_unpacklo_epi64(xmm_t3, xmm_t3);

                // // calculate delta
                xmm_t4 = _mm_sub_epi16(xmm0, xmm_tc);   // -tc
                xmm_t2 = _mm_max_epi16(xmm_t2, xmm_t4);
                xmm_t2 = _mm_min_epi16(xmm_t2, xmm_tc); // delta = CLIP3 (-tc, tc, delta);
                xmm_t2 = _mm_and_si128(xmm_t2, xmm_t3);

                // // filter first two samples
                xmm_t4 = _mm_sub_epi16(xmm0, xmm_t2);        // -delta
                xmm_t2 = _mm_unpacklo_epi64(xmm_t2, xmm_t4); // 0x -delta delta

                xmm_Max10bit = _mm_setr_epi16(0x03FF, 0x03FF, 0x03FF, 0x03FF, 0x03FF, 0x03FF, 0x03FF, 0x03FF);
                xmm_t4 = _mm_add_epi16(xmm_pq0, xmm_t2);   // filteredQ0 - delta
                xmm_t4 = _mm_min_epi16(xmm_t4, xmm_Max10bit);
                xmm_t4 = _mm_max_epi16(xmm_t4, xmm0);
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr-reconLumaPicStride), xmm_t4); // edgeStartSample[-filterStride+i] = CLIP3(0, 1023, (filteredP0 + delta)); // filtering p0
                xmm_t4 = _mm_srli_si128(xmm_t4, 8);
                _mm_storel_epi64((__m128i *)edgeStartFilteredSamplePtr, xmm_t4);                      // edgeStartSample[i]               = CLIP3(0, 1023, (filteredQ0 - delta)); // filtering q0

                // // calculate side thresholds and the new tc value tc2
                xmm_t4 = _mm_srai_epi16(xmm_beta, 1);
                xmm_beta = _mm_add_epi16(xmm_beta, xmm_t4);
                xmm_beta = _mm_srai_epi16(xmm_beta, 3);  // sideBlocksThresh = (beta + (beta >> 1)) >> 3;
                xmm_tc = _mm_srai_epi16(xmm_tc, 1);      // tc2 = tc >> 1;

                xmm_t4 = _mm_srli_si128(xmm_t1, 6);
                xmm_t1 = _mm_add_epi16(xmm_t1, xmm_t4);       // xmm_t1 = 0x xx xx xx (dq = dq0 + dq3) xx xx (dp = dp0 + dp3)
                xmm_beta = _mm_cmpgt_epi16(xmm_beta, xmm_t1); // if (sideBlocksThresh > dp) {
                xmm_beta = _mm_slli_epi64(xmm_beta, 63);
                xmm_beta = _mm_srli_epi64(xmm_beta, 63);
                xmm_t4 = _mm_sub_epi64(xmm0, xmm_beta);
                xmm_t4 = _mm_and_si128(xmm_t4, xmm_t3);

                xmm_pq0 = _mm_avg_epu16(xmm_pq0, xmm_pq2); // (filteredP0 + filteredP2 + 1) >> 1)
                xmm_pq0 = _mm_sub_epi16(xmm_pq0, xmm_pq1); // (filteredP0 + filteredP2 + 1) >> 1) - filteredP1
                xmm_pq0 = _mm_add_epi16(xmm_pq0, xmm_t2);  // ((filteredP0 + filteredP2 + 1) >> 1) - filteredP1 + delta
                xmm_pq0  = _mm_srai_epi16(xmm_pq0, 1);     // (((filteredP0 + filteredP2 + 1) >> 1) - filteredP1 + delta) >> 1
                xmm_t3  = _mm_sub_epi16(xmm0, xmm_tc);     // -tc2
                xmm_pq0 = _mm_max_epi16(xmm_pq0, xmm_t3);
                xmm_pq0 = _mm_min_epi16(xmm_pq0, xmm_tc);  // delta1 = CLIP3(-tc2, tc2, ((((filteredP0 + filteredP2 + 1) >> 1) - filteredP1 + delta) >> 1));
                xmm_pq0 = _mm_and_si128(xmm_pq0, xmm_t4);
                xmm_pq1 = _mm_add_epi16(xmm_pq1, xmm_pq0); // filteredP1+delta1
                xmm_pq1 = _mm_min_epi16(xmm_pq1, xmm_Max10bit); // CLIP3(0, 1023, filteredP1+delta1)
                xmm_pq1 = _mm_max_epi16(xmm_pq1, xmm0);
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr-2*reconLumaPicStride), xmm_pq1); // edgeStartSample[-2*filterStride+i]
                xmm_pq1 = _mm_srli_si128(xmm_pq1, 8);
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr+reconLumaPicStride), xmm_pq1);   // edgeStartSample[filterStride+i]
            }
        }
    }
    else {
        __m128i xmm_qp0, xmm_qp1, xmm_qp2, xmm_qp3, xmm_beta, xmm_Max10bit, xmm_t1, xmm_t2, xmm_t3, xmm_t4;
        __m128i xmm_pq3 = _mm_loadu_si128((__m128i *)(edgeStartFilteredSamplePtr                     -4)); // 07 06 05 04 03 02 01 00
        __m128i xmm_pq0 = _mm_loadu_si128((__m128i *)(edgeStartFilteredSamplePtr+  reconLumaPicStride-4)); // 17 16 15 14 13 12 11 10
        __m128i xmm_pq1 = _mm_loadu_si128((__m128i *)(edgeStartFilteredSamplePtr+2*reconLumaPicStride-4)); // 27 26 25 24 23 22 21 20
        __m128i xmm_pq2 = _mm_loadu_si128((__m128i *)(edgeStartFilteredSamplePtr+3*reconLumaPicStride-4)); // 37 36 35 34 33 32 31 30

        xmm_qp0 = xmm_pq3;
        xmm_qp1 = xmm_pq1;
        xmm_pq3 = _mm_unpacklo_epi16(xmm_pq3, xmm_pq0); // 13 03 12 02 11 01 10 00
        xmm_pq1 = _mm_unpacklo_epi16(xmm_pq1, xmm_pq2); // 33 23 32 22 31 21 30 20
        xmm_qp0 = _mm_unpackhi_epi16(xmm_qp0, xmm_pq0); // 17 07 16 06 15 05 14 04
        xmm_qp1 = _mm_unpackhi_epi16(xmm_qp1, xmm_pq2); // 37 27 36 26 35 25 34 24
        xmm_pq0 = xmm_pq3;
        xmm_qp2 = xmm_qp0;
        xmm_pq3 = _mm_unpacklo_epi32(xmm_pq3, xmm_pq1); // 31 21 11 01 30 20 10 00 (P2 P3)
        xmm_pq0 = _mm_unpackhi_epi32(xmm_pq0, xmm_pq1); // 33 23 13 03 32 22 12 02 (P0 P1)
        xmm_qp0 = _mm_unpacklo_epi32(xmm_qp0, xmm_qp1); // 35 25 15 05 34 24 14 04 (Q1 Q0)
        xmm_qp2 = _mm_unpackhi_epi32(xmm_qp2, xmm_qp1); // 37 27 17 07 36 26 16 06 (Q3 Q2)

        xmm_qp1 = xmm_qp0;                              // (Q1 Q0)
        xmm_qp0 = _mm_srli_si128(xmm_qp0, 8);           // (Q1)
        xmm_qp0 = _mm_unpacklo_epi64(xmm_qp0, xmm_qp1); // (Q0 Q1)

        xmm_qp1 = xmm_qp2;                              // (Q3 Q2)
        xmm_qp2 = _mm_srli_si128(xmm_qp2, 8);           // (Q3)
        xmm_qp2 = _mm_unpacklo_epi64(xmm_qp2, xmm_qp1); // (Q2 Q3)

        xmm_t1  = xmm_pq3;                              // (P2 P3)
        xmm_pq2 = _mm_unpackhi_epi64(xmm_pq3, xmm_qp2); // Q2 P2
        xmm_pq3 = _mm_unpacklo_epi64(xmm_pq3, xmm_qp2); // Q3 P3

        xmm_qp3 = _mm_unpacklo_epi64(xmm_qp2, xmm_pq3); // P3 Q3
        xmm_qp2 = _mm_unpackhi_epi64(xmm_qp2, xmm_t1);  // P2 Q2
        xmm_t1  = xmm_pq0;                              // (P0 P1)
        xmm_pq1 = _mm_unpacklo_epi64(xmm_pq0, xmm_qp0); // Q1 P1
        xmm_pq0 = _mm_unpackhi_epi64(xmm_pq0, xmm_qp0); // Q0 P0

        xmm_qp1 = _mm_unpacklo_epi64(xmm_qp0, xmm_t1);  // P1 Q1
        xmm_qp0 = _mm_unpackhi_epi64(xmm_qp0, xmm_t1);  // P0 Q0

        xmm_t2 = _mm_add_epi16(xmm_pq1, xmm_pq1); // 2*edgeStartSample[-2*filterStride]
        xmm_t2 = _mm_sub_epi16(xmm_t2, xmm_pq0);  // 2*edgeStartSample[-2*filterStride] - edgeStartSample[-filterStride]
        xmm_t2 = _mm_sub_epi16(xmm_t2, xmm_pq2);  // 2*edgeStartSample[-2*filterStride] - edgeStartSample[-filterStride] - edgeStartSample[-3*filterStride]
        xmm_t2 = _mm_abs_epi16(xmm_t2);           // dp0 = ABS(edgeStartSample[-3*filterStride] - 2*edgeStartSample[-2*filterStride] + edgeStartSample[-filterStride]);
                                                  // xmm_t2 = 0x dq3 xx xx dq0 dp3 xx xx dp0
        // calculate d
        xmm_t1 = xmm_t2;
        xmm_t2 = _mm_srli_si128(xmm_t2, 8);
        xmm_t2 = _mm_add_epi16(xmm_t2, xmm_t1); // xmm_t2 = 0x xx xx xx xx (d3 = dp3 + dq3) xx xx (d0 = dp0 + dq0)
        xmm_t3 = xmm_t2;
        xmm_t2 = _mm_srli_si128(xmm_t2, 6);
        xmm_t2 = _mm_add_epi16(xmm_t2, xmm_t3);
        d = _mm_extract_epi16(xmm_t2, 0); // d  = d0  + d3;

        // decision if d > beta : no filtering
        if (d < beta)
        {
            xmm_beta = _mm_cvtsi32_si128(beta);
            xmm_beta = _mm_unpacklo_epi16(xmm_beta, xmm_beta);
            xmm_beta = _mm_unpacklo_epi32(xmm_beta, xmm_beta);
            xmm_beta = _mm_unpacklo_epi64(xmm_beta, xmm_beta);
            xmm_t3 = _mm_slli_epi16(xmm_t3, 1);         // d0<<1
            xmm_t2 = _mm_srai_epi16(xmm_beta, 2);       // beta>>2
            xmm_t2 = _mm_cmpgt_epi16(xmm_t2, xmm_t3);   // (d0<<1) < (beta>>2)

            xmm_t3 = _mm_sub_epi16(xmm_pq3, xmm_pq0);   // filteredP3-filteredP0
            xmm_t3 = _mm_abs_epi16(xmm_t3);             // ABS(filteredP3-filteredP0)
            xmm_t4 = xmm_t3;
            xmm_t3 = _mm_srli_si128(xmm_t3, 8);
            xmm_t3 = _mm_add_epi16(xmm_t3, xmm_t4);     // ABS(filteredP3-filteredP0)+ABS(filteredQ3-filteredQ0)
            xmm_t4 = _mm_srai_epi16(xmm_beta, 3);       // beta>>3
            xmm_t4 = _mm_cmpgt_epi16(xmm_t4, xmm_t3);   // (beta>>3) > (ABS(filteredP3-filteredP0)+ABS(filteredQ3-filteredQ0))
            xmm_t2 = _mm_and_si128(xmm_t2, xmm_t4);     // (d0<<1) < (beta>>2)) && (beta>>3) > (ABS(filteredP3-filteredP0)+ABS(filteredQ3-filteredQ0))

            xmm_t3 = _mm_sub_epi16(xmm_pq0, xmm_qp0);   // filteredP0-filteredQ0
            xmm_t3 = _mm_abs_epi16(xmm_t3);             // ABS(filteredP0-filteredQ0)

            xmm_t4 = _mm_slli_epi16(xmm_tc, 2);
            xmm_t4 = _mm_add_epi16(xmm_t4, xmm_tc);     // 5*tc
            xmm_t4 = _mm_avg_epu16(xmm_t4, xmm0);       // (5*tc+1)>>1
            xmm_t4 = _mm_cmpgt_epi16(xmm_t4, xmm_t3);   // ((5*tc+1)>>1) > ABS(filteredP0-filteredQ0)
            xmm_t2 = _mm_and_si128(xmm_t2, xmm_t4);     // ((d0<<1) < (beta>>2)) && (beta>>3) > (ABS(filteredP3-filteredP0)+ABS(filteredQ3-filteredQ0)) && ((5*tc+1)>>1) > ABS(filteredP0-filteredQ0)

            xmm_t3 = _mm_srli_si128(xmm_t2, 6);
            xmm_t2 = _mm_and_si128(xmm_t2, xmm_t3);
            strongFiltering = (EB_BOOL)_mm_extract_epi16(xmm_t2, 0); // strongFiltering = strongFilteringLine0 && strongFilteringLine3;

            xmm_t2 = xmm_qp0;

            if (strongFiltering) {
                xmm_tc = _mm_slli_epi16(xmm_tc, 1);
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_pq0); // filteredP0 + filteredQ0
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp1); // filteredP0 + filteredQ0 + filteredQ1
                xmm_t2 = _mm_slli_epi16(xmm_t2, 1);      // (filteredP0 + filteredQ0 + filteredQ1) << 1
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_pq1); // filteredP1 + ((filteredP0 + filteredQ0 + filteredQ1) << 1)
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp2); // filteredP1 + ((filteredP0 + filteredQ0 + filteredQ1) << 1) + filteredQ2
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_4);   // filteredP1 + ((filteredP0 + filteredQ0 + filteredQ1) << 1) + filteredQ2 + 4
                xmm_t2 = _mm_srai_epi16(xmm_t2, 3);      // (filteredP1 + ((filteredP0 + filteredQ0 + filteredQ1) << 1) + filteredQ2 + 4) >> 3

                xmm_t3 = _mm_sub_epi16(xmm_qp0, xmm_tc); // filteredQ0-(tc<<1)
                xmm_t4 = _mm_add_epi16(xmm_qp0, xmm_tc); // filteredQ0+(tc<<1)
                xmm_t2 = _mm_max_epi16(xmm_t2, xmm_t3);
                xmm_t2 = _mm_min_epi16(xmm_t2, xmm_t4);  // CLIP3
                xmm_pq1 = xmm_t2;                        // P0 Q0

                xmm_t2 = _mm_add_epi16(xmm_qp2, xmm_qp3);
                xmm_t2 = _mm_slli_epi16(xmm_t2, 1);
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp2); // filteredQ2*3 + (filteredQ3<<1)
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_pq0); // filteredP0 + filteredQ2*3 + (filteredQ3<<1)
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp0); // filteredP0 + filteredQ0 + filteredQ2*3 + (filteredQ3<<1)
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp1); // filteredP0 + filteredQ0 + filteredQ1 + filteredQ2*3 + (filteredQ3<<1)
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_4);   // filteredP0 + filteredQ0 + filteredQ1 + filteredQ2*3 + (filteredQ3<<1) + 4
                xmm_4  = _mm_srli_epi16(xmm_4, 1);       // xmm_4 = 2
                xmm_t2 = _mm_srai_epi16(xmm_t2, 3);      // (filteredP0 + filteredQ0 + filteredQ1 + filteredQ2*3 + (filteredQ3<<1) + 4) >> 3

                xmm_t3 = _mm_sub_epi16(xmm_qp2, xmm_tc); // filteredQ2-(tc<<1)
                xmm_t4 = _mm_add_epi16(xmm_qp2, xmm_tc); // filteredQ2+(tc<<1)
                xmm_t2 = _mm_max_epi16(xmm_t2, xmm_t3);
                xmm_t2 = _mm_min_epi16(xmm_t2, xmm_t4);  // CLIP3
                xmm_pq2 = xmm_t2;                        // P2 Q2

                xmm_t2 = _mm_add_epi16(xmm_pq0, xmm_qp0); // filteredP0 + filteredQ0
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp1);  // filteredP0 + filteredQ0 + filteredQ1
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_qp2);  // filteredP0 + filteredQ0 + filteredQ1 + filteredQ2
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_4);    // filteredP0 + filteredQ0 + filteredQ1 + filteredQ2 + 2
                xmm_t2 = _mm_srai_epi16(xmm_t2, 2);       // (filteredP0 + filteredQ0 + filteredQ1 + filteredQ2 + 2) >> 2

                xmm_t3 = _mm_sub_epi16(xmm_qp1, xmm_tc); // filteredQ1-(tc<<1)
                xmm_t4 = _mm_add_epi16(xmm_qp1, xmm_tc); // filteredQ1+(tc<<1)
                xmm_t2 = _mm_max_epi16(xmm_t2, xmm_t3);  // pmaxsw          xmm_t2,         xmm_t3      ; CLIP3
                xmm_t2 = _mm_min_epi16(xmm_t2, xmm_t4);  // pminsw          xmm_t2,         xmm_t4      ; P1 Q1

                xmm_t1  = xmm_t2;
                xmm_t3  = xmm_qp3;
                xmm_qp3 = _mm_unpackhi_epi16(xmm_qp3, xmm_pq2); // 31 30 21 20 11 10 01 00
                xmm_t2  = _mm_unpackhi_epi16(xmm_t2, xmm_pq1);  // 33 32 23 22 13 12 03 02
                xmm_pq1 = _mm_unpacklo_epi16(xmm_pq1, xmm_t1);  // 35 34 25 24 15 14 05 04
                xmm_pq2 = _mm_unpacklo_epi16(xmm_pq2, xmm_t3);  // 37 36 27 26 17 16 07 06

                xmm_t1  = xmm_qp3;
                xmm_t3  = xmm_pq1;
                xmm_qp3 = _mm_unpacklo_epi32(xmm_qp3, xmm_t2);  // 13 12 11 10 03 02 01 00
                xmm_t1  = _mm_unpackhi_epi32(xmm_t1, xmm_t2);   // 33 32 31 30 23 22 21 20
                xmm_pq1 = _mm_unpacklo_epi32(xmm_pq1, xmm_pq2); // 17 16 15 14 07 06 05 04
                xmm_t3  = _mm_unpackhi_epi32(xmm_t3, xmm_pq2);  // 37 36 35 34 27 26 25 24

                xmm_t2  = xmm_qp3;
                xmm_pq2 = xmm_t1;
                xmm_qp3 = _mm_unpacklo_epi64(xmm_qp3, xmm_pq1); // 07 06 05 04 03 02 01 00
                xmm_t2  = _mm_unpackhi_epi64(xmm_t2, xmm_pq1);  // 17 16 15 14 13 12 11 10
                xmm_t1  = _mm_unpacklo_epi64(xmm_t1, xmm_t3);   // 27 26 25 24 23 22 21 20
                xmm_pq2 = _mm_unpackhi_epi64(xmm_pq2, xmm_t3);  // 37 36 35 34 33 32 31 30

                _mm_storeu_si128((__m128i *)(edgeStartFilteredSamplePtr                     -4), xmm_qp3);
                _mm_storeu_si128((__m128i *)(edgeStartFilteredSamplePtr+  reconLumaPicStride-4), xmm_t2);
                _mm_storeu_si128((__m128i *)(edgeStartFilteredSamplePtr+2*reconLumaPicStride-4), xmm_t1);
                _mm_storeu_si128((__m128i *)(edgeStartFilteredSamplePtr+3*reconLumaPicStride-4), xmm_pq2);
            }
            else { // weak filtering
                // calculate delta, ithreshcut
                xmm_4 = _mm_slli_epi16(xmm_4, 1);
                xmm_t2 = _mm_sub_epi16(xmm_t2, xmm_pq0);  // filteredQ0 - filteredP0
                xmm_t3 = _mm_slli_epi16(xmm_t2, 3);
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_t3);   // (filteredQ0 - filteredP0) * 9
                xmm_t4 = _mm_sub_epi16(xmm_qp1, xmm_pq1); // filteredQ1 - filteredP1
                xmm_t2 = _mm_sub_epi16(xmm_t2, xmm_t4);
                xmm_t4 = _mm_slli_epi16(xmm_t4, 1);
                xmm_t2 = _mm_sub_epi16(xmm_t2, xmm_t4);   // (filteredQ0 - filteredP0) * 9 - (filteredQ1 - filteredP1) * 3
                xmm_t2 = _mm_add_epi16(xmm_t2, xmm_4);    // (filteredQ0 - filteredP0) * 9 - (filteredQ1 - filteredP1) * 3 + 8
                xmm_t2 = _mm_srai_epi16(xmm_t2, 4);       // delta = ((filteredQ0 - filteredP0) * 9 - (filteredQ1 - filteredP1) * 3 + 8) >> 4;

                xmm_t4 = _mm_abs_epi16(xmm_t2);           // ABS(delta)
                xmm_t3 = _mm_slli_epi16(xmm_tc, 2);
                xmm_t3 = _mm_add_epi16(xmm_t3, xmm_tc);
                xmm_t3 = _mm_slli_epi16(xmm_t3, 1);       // threshWeakFiltering = tc * 10;
                xmm_t3 = _mm_cmpgt_epi16(xmm_t3, xmm_t4); // if (ABS(delta) < threshWeakFiltering) {
                xmm_t3 = _mm_unpacklo_epi64(xmm_t3, xmm_t3);

                // // calculate delta
                xmm_t4 = _mm_sub_epi16(xmm0, xmm_tc);     // -tc
                xmm_t2 = _mm_max_epi16(xmm_t2, xmm_t4);
                xmm_t2 = _mm_min_epi16(xmm_t2, xmm_tc);   // delta = CLIP3 (-tc, tc, delta);
                xmm_t2 = _mm_and_si128(xmm_t2, xmm_t3);

                // // filter first two samples
                xmm_t4 = _mm_sub_epi16(xmm0, xmm_t2);        // -delta
                xmm_t2 = _mm_unpacklo_epi64(xmm_t2, xmm_t4); // 0x -delta delta

                xmm_Max10bit = _mm_setr_epi16(0x03FF, 0x03FF, 0x03FF, 0x03FF, 0x03FF, 0x03FF, 0x03FF, 0x03FF);
                xmm_qp0 = _mm_add_epi16(xmm_pq0, xmm_t2);       // filteredQ0 - delta
                xmm_qp0 = _mm_min_epi16(xmm_qp0, xmm_Max10bit); // CLIP3(0, 1023, (filteredQ0 - delta) CLIP3(0, 1023, (filteredP0 + delta)
                xmm_qp0 = _mm_max_epi16(xmm_qp0, xmm0);

                // // calculate side thresholds and the new tc value tc2
                xmm_t4 = _mm_srai_epi16(xmm_beta, 1);
                xmm_beta = _mm_add_epi16(xmm_beta, xmm_t4);
                xmm_beta = _mm_srai_epi16(xmm_beta, 3);  // sideBlocksThresh = (beta + (beta >> 1)) >> 3;
                xmm_tc = _mm_srai_epi16(xmm_tc, 1);      // tc2 = tc >> 1;

                xmm_t4 = _mm_srli_si128(xmm_t1, 6);
                xmm_t1 = _mm_add_epi16(xmm_t1, xmm_t4);       // xmm_t1 = 0x xx xx xx (dq = dq0 + dq3) xx xx (dp = dp0 + dp3)
                xmm_beta = _mm_cmpgt_epi16(xmm_beta, xmm_t1); // if (sideBlocksThresh > dp) {
                xmm_beta = _mm_slli_epi64(xmm_beta, 63);
                xmm_beta = _mm_srli_epi64(xmm_beta, 63);
                xmm_t4 = _mm_sub_epi64(xmm0, xmm_beta);
                xmm_t4 = _mm_and_si128(xmm_t4, xmm_t3);

                xmm_pq0 = _mm_avg_epu16(xmm_pq0, xmm_pq2); // (filteredP0 + filteredP2 + 1) >> 1)
                xmm_pq0 = _mm_sub_epi16(xmm_pq0, xmm_pq1); // (filteredP0 + filteredP2 + 1) >> 1) - filteredP1
                xmm_pq0 = _mm_add_epi16(xmm_pq0, xmm_t2);  // ((filteredP0 + filteredP2 + 1) >> 1) - filteredP1 + delta
                xmm_pq0  = _mm_srai_epi16(xmm_pq0, 1);     // (((filteredP0 + filteredP2 + 1) >> 1) - filteredP1 + delta) >> 1
                xmm_t3  = _mm_sub_epi16(xmm0, xmm_tc);     // -tc2
                xmm_pq0 = _mm_max_epi16(xmm_pq0, xmm_t3);
                xmm_pq0 = _mm_min_epi16(xmm_pq0, xmm_tc);  // delta1 = CLIP3(-tc2, tc2, ((((filteredP0 + filteredP2 + 1) >> 1) - filteredP1 + delta) >> 1));
                xmm_pq0 = _mm_and_si128(xmm_pq0, xmm_t4);
                xmm_pq1 = _mm_add_epi16(xmm_pq1, xmm_pq0); // filteredP1+delta1
                xmm_pq1 = _mm_min_epi16(xmm_pq1, xmm_Max10bit); // CLIP3(0, 1023, filteredQ1+delta2) CLIP3(0, 1023, filteredP1+delta1)
                xmm_pq1 = _mm_max_epi16(xmm_pq1, xmm0);

                xmm_t1 = xmm_pq1;
                xmm_pq1 = _mm_unpacklo_epi16(xmm_pq1, xmm_qp0);  // 31 30 21 20 11 10 01 00
                xmm_qp0 = _mm_unpackhi_epi16(xmm_qp0, xmm_t1);   // 33 32 23 22 13 12 03 02
                xmm_t1  = _mm_unpackhi_epi32(xmm_pq1,  xmm_qp0); // 33 32 31 30 23 22 21 20
                xmm_pq1 = _mm_unpacklo_epi32(xmm_pq1, xmm_qp0);  // 13 12 11 10 03 02 01 00

                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr                     -2), xmm_pq1);
                xmm_pq1 = _mm_srli_si128(xmm_pq1, 8);
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr+  reconLumaPicStride-2), xmm_pq1);
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr+2*reconLumaPicStride-2), xmm_t1);
                xmm_t1 = _mm_srli_si128(xmm_t1, 8);
                _mm_storel_epi64((__m128i *)(edgeStartFilteredSamplePtr+3*reconLumaPicStride-2), xmm_t1);
            }
        }
    }
}
