#include "EbTransforms_AVX2.h"
#include "EbDefinitions.h"

#include <emmintrin.h>
#include <immintrin.h>

#ifdef VNNI_SUPPORT

#ifdef __GNUC__
__attribute__((aligned(16)))
#endif
static EB_ALIGN(32) const EB_S16 EbHevcCoeff_tbl_AVX2[48 * 16] =
{
    64, 64, 89, 75, 83, 36, 75, -18, 64, 64, 89, 75, 83, 36, 75, -18, 64, -64, 50, -89, 36, -83, 18, -50, 64, -64, 50, -89, 36, -83, 18, -50,
    64, 64, 50, 18, -36, -83, -89, -50, 64, 64, 50, 18, -36, -83, -89, -50, -64, 64, 18, 75, 83, -36, 75, -89, -64, 64, 18, 75, 83, -36, 75, -89,
    64, 64, -18, -50, -83, -36, 50, 89, 64, 64, -18, -50, -83, -36, 50, 89, 64, -64, -75, -18, -36, 83, 89, -75, 64, -64, -75, -18, -36, 83, 89, -75,
    64, 64, -75, -89, 36, 83, 18, -75, 64, 64, -75, -89, 36, 83, 18, -75, -64, 64, 89, -50, -83, 36, 50, -18, -64, 64, 89, -50, -83, 36, 50, -18,
    90, 87, 87, 57, 80, 9, 70, -43, 90, 87, 87, 57, 80, 9, 70, -43, 57, -80, 43, -90, 25, -70, 9, -25, 57, -80, 43, -90, 25, -70, 9, -25,
    80, 70, 9, -43, -70, -87, -87, 9, 80, 70, 9, -43, -70, -87, -87, 9, -25, 90, 57, 25, 90, -80, 43, -57, -25, 90, 57, 25, 90, -80, 43, -57,
    57, 43, -80, -90, -25, 57, 90, 25, 57, 43, -80, -90, -25, 57, 90, 25, -9, -87, -87, 70, 43, 9, 70, -80, -9, -87, -87, 70, 43, 9, 70, -80,
    25, 9, -70, -25, 90, 43, -80, -57, 25, 9, -70, -25, 90, 43, -80, -57, 43, 70, 9, -80, -57, 87, 87, -90, 43, 70, 9, -80, -57, 87, 87, -90,
    90, 90, 90, 82, 88, 67, 85, 46, 90, 90, 90, 82, 88, 67, 85, 46, 82, 22, 78, -4, 73, -31, 67, -54, 82, 22, 78, -4, 73, -31, 67, -54,
    61, -73, 54, -85, 46, -90, 38, -88, 61, -73, 54, -85, 46, -90, 38, -88, 31, -78, 22, -61, 13, -38, 4, -13, 31, -78, 22, -61, 13, -38, 4, -13,
    88, 85, 67, 46, 31, -13, -13, -67, 88, 85, 67, 46, 31, -13, -13, -67, -54, -90, -82, -73, -90, -22, -78, 38, -54, -90, -82, -73, -90, -22, -78, 38,
    -46, 82, -4, 88, 38, 54, 73, -4, -46, 82, -4, 88, 38, 54, 73, -4, 90, -61, 85, -90, 61, -78, 22, -31, 90, -61, 85, -90, 61, -78, 22, -31,
    82, 78, 22, -4, -54, -82, -90, -73, 82, 78, 22, -4, -54, -82, -90, -73, -61, 13, 13, 85, 78, 67, 85, -22, -61, 13, 13, 85, 78, 67, 85, -22,
    31, -88, -46, -61, -90, 31, -67, 90, 31, -88, -46, -61, -90, 31, -67, 90, 4, 54, 73, -38, 88, -90, 38, -46, 4, 54, 73, -38, 88, -90, 38, -46,
    73, 67, -31, -54, -90, -78, -22, 38, 73, 67, -31, -54, -90, -78, -22, 38, 78, 85, 67, -22, -38, -90, -90, 4, 78, 85, 67, -22, -38, -90, -90, 4,
    -13, 90, 82, 13, 61, -88, -46, -31, -13, 90, 82, 13, 61, -88, -46, -31, -88, 82, -4, 46, 85, -73, 54, -61, -88, 82, -4, 46, 85, -73, 54, -61,
    61, 54, -73, -85, -46, -4, 82, 88, 61, 54, -73, -85, -46, -4, 82, 88, 31, -46, -88, -61, -13, 82, 90, 13, 31, -46, -88, -61, -13, 82, 90, 13,
    -4, -90, -90, 38, 22, 67, 85, -78, -4, -90, -90, 38, 22, 67, 85, -78, -38, -22, -78, 90, 54, -31, 67, -73, -38, -22, -78, 90, 54, -31, 67, -73,
    46, 38, -90, -88, 38, 73, 54, -4, 46, 38, -90, -88, 38, 73, 54, -4, -90, -67, 31, 90, 61, -46, -88, -31, -90, -67, 31, 90, 61, -46, -88, -31,
    22, 85, 67, -78, -85, 13, 13, 61, 22, 85, 67, -78, -85, 13, 13, 61, 73, -90, -82, 54, 4, 22, 78, -82, 73, -90, -82, 54, 4, 22, 78, -82,
    31, 22, -78, -61, 90, 85, -61, -90, 31, 22, -78, -61, 90, 85, -61, -90, 4, 73, 54, -38, -88, -4, 82, 46, 4, 73, 54, -38, -88, -4, 82, 46,
    -38, -78, -22, 90, 73, -82, -90, 54, -38, -78, -22, 90, 73, -82, -90, 54, 67, -13, -13, -31, -46, 67, 85, -88, 67, -13, -13, -31, -46, 67, 85, -88,
    13, 4, -38, -13, 61, 22, -78, -31, 13, 4, -38, -13, 61, 22, -78, -31, 88, 38, -90, -46, 85, 54, -73, -61, 88, 38, -90, -46, 85, 54, -73, -61,
    54, 67, -31, -73, 4, 78, 22, -82, 54, 67, -31, -73, 4, 78, 22, -82, -46, 85, 67, -88, -82, 90, 90, -90, -46, 85, 67, -88, -82, 90, 90, -90
};

extern void EbHevcTransform32_VNNI_INTRIN(EB_S16 *src, EB_U32 src_stride, EB_S16 *dst, EB_U32 dst_stride, EB_U32 shift)
{
    EB_U32 i;
    __m128i s0;
    __m256i o0;
    const __m256i *coeff32 = (const __m256i *)EbHevcCoeff_tbl_AVX2;
    shift &= 0x0000FFFF; // Redundant code to fix Visual Studio 2012 AVX2 compiler error
    s0 = _mm_cvtsi32_si128(shift);
    o0 = _mm256_set1_epi32(1 << (shift - 1));

    for (i = 0; i < 16; i++)
    {
        __m256i x0, x1, x2, x3,sox0,sox5,soxa,soxf,s1x0,s1x5,s1xa,s1xf;
        __m256i y0, y1, y2, y3;
        __m256i aa4, aa5, aa6, aa7;
        __m256i a0, a1, a2, a3, a4, a5, a6, a7;
        __m256i b0, b1, b2, b3, b4, b5, b6, b7;

        x0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((const __m128i *)(src + 0x00))), _mm_loadu_si128((const __m128i *)(src + src_stride + 0x00)), 0x1);
        x1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((const __m128i *)(src + 0x08))), _mm_loadu_si128((const __m128i *)(src + src_stride + 0x08)), 0x1);
        x2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((const __m128i *)(src + 0x10))), _mm_loadu_si128((const __m128i *)(src + src_stride + 0x10)), 0x1);
        x3 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm_loadu_si128((const __m128i *)(src + 0x18))), _mm_loadu_si128((const __m128i *)(src + src_stride + 0x18)), 0x1);

        // 32-point butterfly
        x2 = _mm256_shuffle_epi8(x2, _mm256_setr_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1));
        x3 = _mm256_shuffle_epi8(x3, _mm256_setr_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1));

        y0 = _mm256_add_epi16(x0, x3);
        y1 = _mm256_add_epi16(x1, x2);

        y2 = _mm256_sub_epi16(x0, x3);
        y3 = _mm256_sub_epi16(x1, x2);

        // 16-point butterfly
        y1 = _mm256_shuffle_epi8(y1, _mm256_setr_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1));

        x0 = _mm256_add_epi16(y0, y1);
        x1 = _mm256_sub_epi16(y0, y1);

        x2 = y2;
        x3 = y3;


		sox0 = _mm256_shuffle_epi32(x0, 0x00);
		sox5 = _mm256_shuffle_epi32(x0, 0x55);
		soxa = _mm256_shuffle_epi32(x0, 0xaa);
		soxf = _mm256_shuffle_epi32(x0, 0xff);
		s1x0 = _mm256_shuffle_epi32(x1, 0x00);
		s1x5 = _mm256_shuffle_epi32(x1, 0x55);
		s1xa = _mm256_shuffle_epi32(x1, 0xaa);
		s1xf = _mm256_shuffle_epi32(x1, 0xff);	

		a0 = _mm256_madd_epi16(sox0, coeff32[0]);

		a0 = _mm256_dpwssd_epi32(a0, sox5, coeff32[2]);
        a0 = _mm256_dpwssd_epi32(a0, soxa, coeff32[4]);
        a0 = _mm256_dpwssd_epi32(a0, soxf, coeff32[6]);
		
        a1 = _mm256_madd_epi16(sox0, coeff32[1]);
        a1 = _mm256_dpwssd_epi32(a1, sox5, coeff32[3]);
        a1 = _mm256_dpwssd_epi32(a1, soxa, coeff32[5]);
        a1 = _mm256_dpwssd_epi32(a1, soxf, coeff32[7]);

        a2 = _mm256_madd_epi16(s1x0, coeff32[8]);
        a2 = _mm256_dpwssd_epi32(a2, s1x5, coeff32[10]);
        a2 = _mm256_dpwssd_epi32(a2, s1xa, coeff32[12]);
        a2 = _mm256_dpwssd_epi32(a2, s1xf, coeff32[14]);

        a3 = _mm256_madd_epi16(s1x0, coeff32[9]);
        a3 = _mm256_dpwssd_epi32(a3, s1x5, coeff32[11]);
        a3 = _mm256_dpwssd_epi32(a3, s1xa, coeff32[13]);
        a3 = _mm256_dpwssd_epi32(a3, s1xf, coeff32[15]);		

		sox0 = _mm256_shuffle_epi32(x2, 0x00);
		sox5 = _mm256_shuffle_epi32(x2, 0x55);
		soxa = _mm256_shuffle_epi32(x2, 0xaa);
		soxf = _mm256_shuffle_epi32(x2, 0xff);
		s1x0 = _mm256_shuffle_epi32(x3, 0x00);
		s1x5 = _mm256_shuffle_epi32(x3, 0x55);
		s1xa = _mm256_shuffle_epi32(x3, 0xaa);
		s1xf = _mm256_shuffle_epi32(x3, 0xff);
 
        a4 = _mm256_madd_epi16(sox0, coeff32[16]);
        a4 = _mm256_dpwssd_epi32(a4, sox5, coeff32[20]);
        a4 = _mm256_dpwssd_epi32(a4, soxa, coeff32[24]);
        a4 = _mm256_dpwssd_epi32(a4, soxf, coeff32[28]);
        a4 = _mm256_dpwssd_epi32(a4, s1x0, coeff32[32]);
        a4 = _mm256_dpwssd_epi32(a4, s1x5, coeff32[36]);
        a4 = _mm256_dpwssd_epi32(a4, s1xa, coeff32[40]);
        a4 = _mm256_dpwssd_epi32(a4, s1xf, coeff32[44]);

        a5 = _mm256_madd_epi16(sox0, coeff32[17]);
        a5 = _mm256_dpwssd_epi32(a5, sox5, coeff32[21]);
        a5 = _mm256_dpwssd_epi32(a5, soxa, coeff32[25]);
        a5 = _mm256_dpwssd_epi32(a5, soxf, coeff32[29]);
        a5 = _mm256_dpwssd_epi32(a5, s1x0, coeff32[33]);
        a5 = _mm256_dpwssd_epi32(a5, s1x5, coeff32[37]);
        a5 = _mm256_dpwssd_epi32(a5, s1xa, coeff32[41]);
        a5 = _mm256_dpwssd_epi32(a5, s1xf, coeff32[45]);

        a6 = _mm256_madd_epi16(sox0, coeff32[18]);
        a6 = _mm256_dpwssd_epi32(a6, sox5, coeff32[22]);
        a6 = _mm256_dpwssd_epi32(a6, soxa, coeff32[26]);
        a6 = _mm256_dpwssd_epi32(a6, soxf, coeff32[30]);
        a6 = _mm256_dpwssd_epi32(a6, s1x0, coeff32[34]);
        a6 = _mm256_dpwssd_epi32(a6, s1x5, coeff32[38]);
        a6 = _mm256_dpwssd_epi32(a6, s1xa, coeff32[42]);
        a6 = _mm256_dpwssd_epi32(a6, s1xf, coeff32[46]);

        a7 = _mm256_madd_epi16(sox0, coeff32[19]);
        a7 = _mm256_dpwssd_epi32(a7, sox5, coeff32[23]);
        a7 = _mm256_dpwssd_epi32(a7, soxa, coeff32[27]);
        a7 = _mm256_dpwssd_epi32(a7, soxf, coeff32[31]);
        a7 = _mm256_dpwssd_epi32(a7, s1x0, coeff32[35]);
        a7 = _mm256_dpwssd_epi32(a7, s1x5, coeff32[39]);
        a7 = _mm256_dpwssd_epi32(a7, s1xa, coeff32[43]);
        a7 = _mm256_dpwssd_epi32(a7, s1xf, coeff32[47]);

        b0 = _mm256_sra_epi32(_mm256_add_epi32(a0, o0), s0);
        b1 = _mm256_sra_epi32(_mm256_add_epi32(a1, o0), s0);
        b2 = _mm256_sra_epi32(_mm256_add_epi32(a2, o0), s0);
        b3 = _mm256_sra_epi32(_mm256_add_epi32(a3, o0), s0);
        b4 = _mm256_sra_epi32(_mm256_add_epi32(a4, o0), s0);
        b5 = _mm256_sra_epi32(_mm256_add_epi32(a5, o0), s0);
        b6 = _mm256_sra_epi32(_mm256_add_epi32(a6, o0), s0);
        b7 = _mm256_sra_epi32(_mm256_add_epi32(a7, o0), s0);

        x0 = _mm256_packs_epi32(b0, b1);
        x1 = _mm256_packs_epi32(b2, b3);
        x2 = _mm256_packs_epi32(b4, b5);
        x3 = _mm256_packs_epi32(b6, b7);

        y0 = _mm256_unpacklo_epi16(x0, x1);
        y1 = _mm256_unpackhi_epi16(x0, x1);
        y2 = x2;
        y3 = x3;
        x0 = _mm256_unpacklo_epi16(y0, y2);
        x1 = _mm256_unpackhi_epi16(y0, y2);
        x2 = _mm256_unpacklo_epi16(y1, y3);
        x3 = _mm256_unpackhi_epi16(y1, y3);

        y0 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm256_extracti128_si256(x0, 0)), _mm256_extracti128_si256(x1, 0), 0x1);
        y1 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm256_extracti128_si256(x2, 0)), _mm256_extracti128_si256(x3, 0), 0x1);
        y2 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm256_extracti128_si256(x0, 1)), _mm256_extracti128_si256(x1, 1), 0x1);
        y3 = _mm256_insertf128_si256(_mm256_castsi128_si256(_mm256_extracti128_si256(x2, 1)), _mm256_extracti128_si256(x3, 1), 0x1);
        _mm256_storeu_si256((__m256i *)(dst + 0x00), y0);
        _mm256_storeu_si256((__m256i *)(dst + 0x10), y1);
        _mm256_storeu_si256((__m256i *)(dst + dst_stride + 0x00), y2);
        _mm256_storeu_si256((__m256i *)(dst + dst_stride + 0x10), y3);

        src += 2 * src_stride;
        dst += 2 * dst_stride;
    }
}
#endif