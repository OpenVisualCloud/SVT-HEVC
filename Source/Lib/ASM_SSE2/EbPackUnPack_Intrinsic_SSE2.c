/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbPackUnPack_SSE2.h"


#include <emmintrin.h>

#include "EbDefinitions.h"

void EB_ENC_UnPack8BitDataSafeSub_SSE2_INTRIN(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,
    EB_U32       width,
    EB_U32       height
    )
{

    EB_U32 x, y;

    __m128i xmm_00FF, inPixel0, inPixel1, inPixel1_shftR_2_U8, inPixel0_shftR_2_U8, inPixel0_shftR_2, inPixel1_shftR_2;


    xmm_00FF = _mm_set1_epi16(0x00FF);

    if (width == 8)
    {
        for (y = 0; y < height; y += 2)
        {
            inPixel0 = _mm_loadu_si128((__m128i*) in16BitBuffer);
            inPixel1 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride));


            inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF);
            inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF);

            inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
            inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);


            _mm_storel_epi64((__m128i*)out8BitBuffer, inPixel0_shftR_2_U8);
            _mm_storel_epi64((__m128i*)(out8BitBuffer + out8Stride), inPixel1_shftR_2_U8);

            out8BitBuffer += 2 * out8Stride;
            in16BitBuffer += 2 * inStride;
        }

    }
    else if (width == 16)
    {
        __m128i inPixel2, inPixel3;

        for (y = 0; y < height; y += 2)
        {
            inPixel0 = _mm_loadu_si128((__m128i*) in16BitBuffer);
            inPixel1 = _mm_loadu_si128((__m128i*) (in16BitBuffer + 8));
            inPixel2 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride));
            inPixel3 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 8));


            inPixel0_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
            inPixel1_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));


            _mm_storeu_si128((__m128i*)out8BitBuffer, inPixel0_shftR_2_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride), inPixel1_shftR_2_U8);


            out8BitBuffer += 2 * out8Stride;
            in16BitBuffer += 2 * inStride;
        }

    }
    else if (width == 32)
    {
         __m128i inPixel2, inPixel3, inPixel4, inPixel5, inPixel6, inPixel7;
         __m128i  out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

        for (y = 0; y < height; y += 2)
        {
            inPixel0 = _mm_loadu_si128((__m128i*)in16BitBuffer);
            inPixel1 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 8));
            inPixel2 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 24));
            inPixel4 = _mm_loadu_si128((__m128i*)(in16BitBuffer + inStride));
            inPixel5 = _mm_loadu_si128((__m128i*)(in16BitBuffer + inStride + 8));
            inPixel6 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 16));
            inPixel7 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 24));


            out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
            out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
            out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel5, 2), xmm_00FF));
            out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel7, 2), xmm_00FF));


            _mm_storeu_si128((__m128i*)out8BitBuffer, out8_0_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + 16), out8_1_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride), out8_2_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride + 16), out8_3_U8);


            out8BitBuffer += 2 * out8Stride;
            in16BitBuffer += 2 * inStride;
        }

    }
    else if (width == 64)
    {
        __m128i inPixel2, inPixel3, inPixel4, inPixel5, inPixel6, inPixel7;
        __m128i  out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

        for (y = 0; y < height; ++y)
        {
            inPixel0 = _mm_loadu_si128((__m128i*)in16BitBuffer);
            inPixel1 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 8));
            inPixel2 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 24));
            inPixel4 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 32));
            inPixel5 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 40));
            inPixel6 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 48));
            inPixel7 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 56));


            out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
            out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
            out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel5, 2), xmm_00FF));
            out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel7, 2), xmm_00FF));


            _mm_storeu_si128((__m128i*)out8BitBuffer, out8_0_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + 16), out8_1_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + 32), out8_2_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + 48), out8_3_U8);


            out8BitBuffer += out8Stride;
            in16BitBuffer += inStride;
        }

    } else
    {
        EB_U32 inStrideDiff = (2 * inStride) - width;
        EB_U32 out8StrideDiff = (2 * out8Stride) - width;

        EB_U32 inStrideDiff64 = inStride - width;
        EB_U32 out8StrideDiff64 = out8Stride - width;

        if (!(width & 63))
        {
            __m128i inPixel2, inPixel3, inPixel4, inPixel5, inPixel6, inPixel7;
            __m128i out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

            for (x = 0; x < height; x += 1){
                for (y = 0; y < width; y += 64){

                    inPixel0 = _mm_loadu_si128((__m128i*)in16BitBuffer);
                    inPixel1 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 8));
                    inPixel2 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 16));
                    inPixel3 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 24));
                    inPixel4 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 32));
                    inPixel5 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 40));
                    inPixel6 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 48));
                    inPixel7 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 56));


                    out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
                    out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
                    out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel5, 2), xmm_00FF));
                    out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel7, 2), xmm_00FF));


                    _mm_storeu_si128((__m128i*)out8BitBuffer, out8_0_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + 16), out8_1_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + 32), out8_2_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + 48), out8_3_U8);

                    out8BitBuffer += 64;
                    in16BitBuffer += 64;
                }
                in16BitBuffer += inStrideDiff64;
                out8BitBuffer += out8StrideDiff64;
            }
        }
        else if (!(width & 31))
        {
            __m128i inPixel2, inPixel3, inPixel4, inPixel5, inPixel6, inPixel7;
            __m128i out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 32)
                {
                    inPixel0 = _mm_loadu_si128((__m128i*)in16BitBuffer);
                    inPixel1 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 8));
                    inPixel2 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 16));
                    inPixel3 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 24));
                    inPixel4 = _mm_loadu_si128((__m128i*)(in16BitBuffer + inStride));
                    inPixel5 = _mm_loadu_si128((__m128i*)(in16BitBuffer + inStride + 8));
                    inPixel6 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 16));
                    inPixel7 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 24));

                    out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
                    out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
                    out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel5, 2), xmm_00FF));
                    out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel7, 2), xmm_00FF));

                    _mm_storeu_si128((__m128i*)out8BitBuffer, out8_0_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + 16), out8_1_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride), out8_2_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride + 16), out8_3_U8);

                    out8BitBuffer += 32;
                    in16BitBuffer += 32;
                }
                in16BitBuffer += inStrideDiff;
                out8BitBuffer += out8StrideDiff;
            }
        }
        else if (!(width & 15))
        {
            __m128i inPixel2, inPixel3;

            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 16)
                {
                    inPixel0 = _mm_loadu_si128((__m128i*) in16BitBuffer);
                    inPixel1 = _mm_loadu_si128((__m128i*) (in16BitBuffer + 8));
                    inPixel2 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride));
                    inPixel3 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 8));

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));


                    _mm_storeu_si128((__m128i*)out8BitBuffer, inPixel0_shftR_2_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride), inPixel1_shftR_2_U8);

                    out8BitBuffer += 16;
                    in16BitBuffer += 16;
                }
                in16BitBuffer += inStrideDiff;
                out8BitBuffer += out8StrideDiff;
            }
        }
        else if (!(width & 7))
        {
            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 8)
                {
                    inPixel0 = _mm_loadu_si128((__m128i*) in16BitBuffer);
                    inPixel1 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride));

                    inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF);
                    inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF);

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

                    _mm_storel_epi64((__m128i*)out8BitBuffer, inPixel0_shftR_2_U8);
                    _mm_storel_epi64((__m128i*)(out8BitBuffer + out8Stride), inPixel1_shftR_2_U8);

                    out8BitBuffer += 8;
                    in16BitBuffer += 8;
                }
                in16BitBuffer += inStrideDiff;
                out8BitBuffer += out8StrideDiff;
            }
        }
        else
        {
            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 4)
                {
                    inPixel0 = _mm_loadl_epi64((__m128i*)in16BitBuffer);
                    inPixel1 = _mm_loadl_epi64((__m128i*)(in16BitBuffer + inStride));


                    inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF);
                    inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF);

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

                    *(EB_U32*)out8BitBuffer = _mm_cvtsi128_si32(inPixel0_shftR_2_U8);
                    *(EB_U32*)(out8BitBuffer + out8Stride) = _mm_cvtsi128_si32(inPixel1_shftR_2_U8);

                    out8BitBuffer += 4;
                    in16BitBuffer += 4;
                }
                in16BitBuffer += inStrideDiff;
                out8BitBuffer += out8StrideDiff;
            }
        }

    }


    return;
}

void EB_ENC_UnPack8BitData_SSE2_INTRIN(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U32       out8Stride,
    EB_U32       width,
    EB_U32       height)
{
    EB_U32 x, y;

    __m128i xmm_00FF, inPixel0, inPixel1, inPixel1_shftR_2_U8, inPixel0_shftR_2_U8, inPixel0_shftR_2, inPixel1_shftR_2;
//    __m128i tempPixel0_U8, tempPixel1_U8;

    xmm_00FF = _mm_set1_epi16(0x00FF);

    if (width == 4)
    {
        for (y = 0; y < height; y += 2)
        {
            inPixel0 = _mm_loadl_epi64((__m128i*)in16BitBuffer);
            inPixel1 = _mm_loadl_epi64((__m128i*)(in16BitBuffer + inStride));

            //tempPixel0 = _mm_slli_epi16(_mm_and_si128(inPixel0, xmm_3), 6);
            //tempPixel1 = _mm_slli_epi16(_mm_and_si128(inPixel1, xmm_3), 6);
            //
            //tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
            //tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

            inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF);
            inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF);

            inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
            inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

            //*(EB_U32*)outnBitBuffer = _mm_cvtsi128_si32(tempPixel0_U8);
            //*(EB_U32*)(outnBitBuffer + outnStride) = _mm_cvtsi128_si32(tempPixel1_U8);
            *(EB_U32*)out8BitBuffer = _mm_cvtsi128_si32(inPixel0_shftR_2_U8);
            *(EB_U32*)(out8BitBuffer + out8Stride) = _mm_cvtsi128_si32(inPixel1_shftR_2_U8);

            //outnBitBuffer += 2 * outnStride;
            out8BitBuffer += 2 * out8Stride;
            in16BitBuffer += 2 * inStride;
        }
    }
    else if (width == 8)
    {
        for (y = 0; y < height; y += 2)
        {
            inPixel0 = _mm_loadu_si128((__m128i*) in16BitBuffer);
            inPixel1 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride));

            //tempPixel0 = _mm_slli_epi16(_mm_and_si128(inPixel0, xmm_3), 6);
            //tempPixel1 = _mm_slli_epi16(_mm_and_si128(inPixel1, xmm_3), 6);
            //
            //tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
            //tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

            inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF);
            inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF);

            inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
            inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

            //_mm_storel_epi64((__m128i*)outnBitBuffer, tempPixel0_U8);
            //_mm_storel_epi64((__m128i*)(outnBitBuffer + outnStride), tempPixel1_U8);
            _mm_storel_epi64((__m128i*)out8BitBuffer, inPixel0_shftR_2_U8);
            _mm_storel_epi64((__m128i*)(out8BitBuffer + out8Stride), inPixel1_shftR_2_U8);

            //outnBitBuffer += 2 * outnStride;
            out8BitBuffer += 2 * out8Stride;
            in16BitBuffer += 2 * inStride;
        }
    }
    else if (width == 16)
    {
        __m128i inPixel2, inPixel3;

        for (y = 0; y < height; y += 2)
        {
            inPixel0 = _mm_loadu_si128((__m128i*) in16BitBuffer);
            inPixel1 = _mm_loadu_si128((__m128i*) (in16BitBuffer + 8));
            inPixel2 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride));
            inPixel3 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 8));

            //tempPixel0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel1, xmm_3), 6));
            //tempPixel1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));

            inPixel0_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
            inPixel1_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));

            //_mm_storeu_si128((__m128i*)outnBitBuffer, tempPixel0_U8);
            //_mm_storeu_si128((__m128i*)(outnBitBuffer + outnStride), tempPixel1_U8);
            _mm_storeu_si128((__m128i*)out8BitBuffer, inPixel0_shftR_2_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride), inPixel1_shftR_2_U8);

            //outnBitBuffer += 2 * outnStride;
            out8BitBuffer += 2 * out8Stride;
            in16BitBuffer += 2 * inStride;
        }
    }
    else if (width == 32)
    {
         __m128i inPixel2, inPixel3, inPixel4, inPixel5, inPixel6, inPixel7;
         __m128i /*outn0_U8, outn1_U8, outn2_U8, outn3_U8,*/ out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

        for (y = 0; y < height; y += 2)
        {
            inPixel0 = _mm_loadu_si128((__m128i*)in16BitBuffer);
            inPixel1 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 8));
            inPixel2 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 24));
            inPixel4 = _mm_loadu_si128((__m128i*)(in16BitBuffer + inStride));
            inPixel5 = _mm_loadu_si128((__m128i*)(in16BitBuffer + inStride + 8));
            inPixel6 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 16));
            inPixel7 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 24));

            //outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel1, xmm_3), 6));
            //outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
            //outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel5, xmm_3), 6));
            //outn3_U8 = _mm_packus_epi16(_mm_and_si128(inPixel6, xmm_3), _mm_and_si128(inPixel7, xmm_3));

            out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
            out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
            out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel5, 2), xmm_00FF));
            out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel7, 2), xmm_00FF));

            //_mm_storeu_si128((__m128i*)outnBitBuffer, outn0_U8);
            //_mm_storeu_si128((__m128i*)(outnBitBuffer + 16), outn1_U8);
            //_mm_storeu_si128((__m128i*)(outnBitBuffer + outnStride), outn2_U8);
            //_mm_storeu_si128((__m128i*)(outnBitBuffer + outnStride + 16), outn3_U8);

            _mm_storeu_si128((__m128i*)out8BitBuffer, out8_0_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + 16), out8_1_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride), out8_2_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride + 16), out8_3_U8);

            //outnBitBuffer += 2 * outnStride;
            out8BitBuffer += 2 * out8Stride;
            in16BitBuffer += 2 * inStride;
        }
    }
    else if (width == 64)
    {
        __m128i inPixel2, inPixel3, inPixel4, inPixel5, inPixel6, inPixel7;
        __m128i /*outn0_U8, outn1_U8, outn2_U8, outn3_U8,*/ out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

        for (y = 0; y < height; ++y)
        {
            inPixel0 = _mm_loadu_si128((__m128i*)in16BitBuffer);
            inPixel1 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 8));
            inPixel2 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 24));
            inPixel4 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 32));
            inPixel5 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 40));
            inPixel6 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 48));
            inPixel7 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 56));

            //outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel1, xmm_3), 6));
            //outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
            //outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel5, xmm_3), 6));
            //outn3_U8 = _mm_packus_epi16(_mm_and_si128(inPixel6, xmm_3), _mm_and_si128(inPixel7, xmm_3));

            out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
            out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
            out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel5, 2), xmm_00FF));
            out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel7, 2), xmm_00FF));

            //_mm_storeu_si128((__m128i*)outnBitBuffer, outn0_U8);
            //_mm_storeu_si128((__m128i*)(outnBitBuffer + 16), outn1_U8);
            //_mm_storeu_si128((__m128i*)(outnBitBuffer + 32), outn2_U8);
            //_mm_storeu_si128((__m128i*)(outnBitBuffer + 48), outn3_U8);

            _mm_storeu_si128((__m128i*)out8BitBuffer, out8_0_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + 16), out8_1_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + 32), out8_2_U8);
            _mm_storeu_si128((__m128i*)(out8BitBuffer + 48), out8_3_U8);

            //outnBitBuffer += outnStride;
            out8BitBuffer += out8Stride;
            in16BitBuffer += inStride;
        }
    }
    else
    {
        EB_U32 inStrideDiff = (2 * inStride) - width;
        EB_U32 out8StrideDiff = (2 * out8Stride) - width;
        //EB_U32 outnStrideDiff = (2 * outnStride) - width;

        EB_U32 inStrideDiff64 = inStride - width;
        EB_U32 out8StrideDiff64 = out8Stride - width;
        //EB_U32 outnStrideDiff64 = outnStride - width;

        if (!(width & 63))
        {
            __m128i inPixel2, inPixel3, inPixel4, inPixel5, inPixel6, inPixel7;
            __m128i /*outn0_U8, outn1_U8, outn2_U8, outn3_U8, */out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

            for (x = 0; x < height; x += 1){
                for (y = 0; y < width; y += 64){

                    inPixel0 = _mm_loadu_si128((__m128i*)in16BitBuffer);
                    inPixel1 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 8));
                    inPixel2 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 16));
                    inPixel3 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 24));
                    inPixel4 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 32));
                    inPixel5 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 40));
                    inPixel6 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 48));
                    inPixel7 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 56));

                    //outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel1, xmm_3), 6));
                    //outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
                    //outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel5, xmm_3), 6));
                    //outn3_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel6, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel7, xmm_3), 6));

                    out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
                    out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
                    out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel5, 2), xmm_00FF));
                    out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel7, 2), xmm_00FF));

                    //_mm_storeu_si128((__m128i*)outnBitBuffer, outn0_U8);
                    //_mm_storeu_si128((__m128i*)(outnBitBuffer + 16), outn1_U8);
                    //_mm_storeu_si128((__m128i*)(outnBitBuffer + 32), outn2_U8);
                    //_mm_storeu_si128((__m128i*)(outnBitBuffer + 48), outn3_U8);

                    _mm_storeu_si128((__m128i*)out8BitBuffer, out8_0_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + 16), out8_1_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + 32), out8_2_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + 48), out8_3_U8);

                    //outnBitBuffer += 64;
                    out8BitBuffer += 64;
                    in16BitBuffer += 64;
                }
                in16BitBuffer += inStrideDiff64;
                //outnBitBuffer += outnStrideDiff64;
                out8BitBuffer += out8StrideDiff64;
            }
        }
        else if (!(width & 31))
        {
            __m128i inPixel2, inPixel3, inPixel4, inPixel5, inPixel6, inPixel7;
            __m128i /*outn0_U8, outn1_U8, outn2_U8, outn3_U8,*/ out8_0_U8, out8_1_U8, out8_2_U8, out8_3_U8;

            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 32)
                {
                    inPixel0 = _mm_loadu_si128((__m128i*)in16BitBuffer);
                    inPixel1 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 8));
                    inPixel2 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 16));
                    inPixel3 = _mm_loadu_si128((__m128i*)(in16BitBuffer + 24));
                    inPixel4 = _mm_loadu_si128((__m128i*)(in16BitBuffer + inStride));
                    inPixel5 = _mm_loadu_si128((__m128i*)(in16BitBuffer + inStride + 8));
                    inPixel6 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 16));
                    inPixel7 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 24));

                    //outn0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel1, xmm_3), 6));
                    //outn1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
                    //outn2_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel4, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel5, xmm_3), 6));
                    //outn3_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel6, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel7, xmm_3), 6));

                    out8_0_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
                    out8_1_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));
                    out8_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel4, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel5, 2), xmm_00FF));
                    out8_3_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel6, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel7, 2), xmm_00FF));

                    //_mm_storeu_si128((__m128i*)outnBitBuffer, outn0_U8);
                    //_mm_storeu_si128((__m128i*)(outnBitBuffer + 16), outn1_U8);
                    //_mm_storeu_si128((__m128i*)(outnBitBuffer + outnStride), outn2_U8);
                    //_mm_storeu_si128((__m128i*)(outnBitBuffer + outnStride + 16), outn3_U8);

                    _mm_storeu_si128((__m128i*)out8BitBuffer, out8_0_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + 16), out8_1_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride), out8_2_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride + 16), out8_3_U8);

                    //outnBitBuffer += 32;
                    out8BitBuffer += 32;
                    in16BitBuffer += 32;
                }
                in16BitBuffer += inStrideDiff;
                //outnBitBuffer += outnStrideDiff;
                out8BitBuffer += out8StrideDiff;
            }
        }
        else if (!(width & 15))
        {
            __m128i inPixel2, inPixel3;

            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 16)
                {
                    inPixel0 = _mm_loadu_si128((__m128i*) in16BitBuffer);
                    inPixel1 = _mm_loadu_si128((__m128i*) (in16BitBuffer + 8));
                    inPixel2 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride));
                    inPixel3 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride + 8));

                    //tempPixel0_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel0, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel1, xmm_3), 6));
                    //tempPixel1_U8 = _mm_packus_epi16(_mm_slli_epi16(_mm_and_si128(inPixel2, xmm_3), 6), _mm_slli_epi16(_mm_and_si128(inPixel3, xmm_3), 6));
                    //
                    inPixel0_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF));
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(_mm_and_si128(_mm_srli_epi16(inPixel2, 2), xmm_00FF), _mm_and_si128(_mm_srli_epi16(inPixel3, 2), xmm_00FF));

                    //_mm_storeu_si128((__m128i*)outnBitBuffer, tempPixel0_U8);
                    //_mm_storeu_si128((__m128i*)(outnBitBuffer + outnStride), tempPixel1_U8);
                    _mm_storeu_si128((__m128i*)out8BitBuffer, inPixel0_shftR_2_U8);
                    _mm_storeu_si128((__m128i*)(out8BitBuffer + out8Stride), inPixel1_shftR_2_U8);

                    //outnBitBuffer += 16;
                    out8BitBuffer += 16;
                    in16BitBuffer += 16;
                }
                in16BitBuffer += inStrideDiff;
                //outnBitBuffer += outnStrideDiff;
                out8BitBuffer += out8StrideDiff;
            }
        }
        else if (!(width & 7))
        {
            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 8)
                {
                    inPixel0 = _mm_loadu_si128((__m128i*) in16BitBuffer);
                    inPixel1 = _mm_loadu_si128((__m128i*) (in16BitBuffer + inStride));

                    //tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
                    //tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

                    inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF);
                    inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF);

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

                    //_mm_storel_epi64((__m128i*)outnBitBuffer, tempPixel0_U8);
                    //_mm_storel_epi64((__m128i*)(outnBitBuffer + outnStride), tempPixel1_U8);
                    _mm_storel_epi64((__m128i*)out8BitBuffer, inPixel0_shftR_2_U8);
                    _mm_storel_epi64((__m128i*)(out8BitBuffer + out8Stride), inPixel1_shftR_2_U8);

                    //outnBitBuffer += 8;
                    out8BitBuffer += 8;
                    in16BitBuffer += 8;
                }
                in16BitBuffer += inStrideDiff;
                //outnBitBuffer += outnStrideDiff;
                out8BitBuffer += out8StrideDiff;
            }
        }
        else
        {
            for (x = 0; x < height; x += 2)
            {
                for (y = 0; y < width; y += 4)
                {
                    inPixel0 = _mm_loadl_epi64((__m128i*)in16BitBuffer);
                    inPixel1 = _mm_loadl_epi64((__m128i*)(in16BitBuffer + inStride));

                    //tempPixel0_U8 = _mm_packus_epi16(tempPixel0, tempPixel0);
                    //tempPixel1_U8 = _mm_packus_epi16(tempPixel1, tempPixel1);

                    inPixel0_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel0, 2), xmm_00FF);
                    inPixel1_shftR_2 = _mm_and_si128(_mm_srli_epi16(inPixel1, 2), xmm_00FF);

                    inPixel0_shftR_2_U8 = _mm_packus_epi16(inPixel0_shftR_2, inPixel0_shftR_2);
                    inPixel1_shftR_2_U8 = _mm_packus_epi16(inPixel1_shftR_2, inPixel1_shftR_2);

                    //*(EB_U32*)outnBitBuffer = _mm_cvtsi128_si32(tempPixel0_U8);
                    //*(EB_U32*)(outnBitBuffer + outnStride) = _mm_cvtsi128_si32(tempPixel1_U8);
                    *(EB_U32*)out8BitBuffer = _mm_cvtsi128_si32(inPixel0_shftR_2_U8);
                    *(EB_U32*)(out8BitBuffer + out8Stride) = _mm_cvtsi128_si32(inPixel1_shftR_2_U8);

                    //outnBitBuffer += 4;
                    out8BitBuffer += 4;
                    in16BitBuffer += 4;
                }
                in16BitBuffer += inStrideDiff;
                //outnBitBuffer += outnStrideDiff;
                out8BitBuffer += out8StrideDiff;
            }
        }
    }

    return;
}
void UnpackAvg_SSE2_INTRIN(
        EB_U16 *ref16L0,
        EB_U32  refL0Stride,
        EB_U16 *ref16L1,
        EB_U32  refL1Stride,
        EB_U8  *dstPtr,
        EB_U32  dstStride,
        EB_U32  width,
        EB_U32  height)
{

    EB_U32  y;
    __m128i inPixel0, inPixel1;



    if (width == 4)
    {
         __m128i out8_0_U8_L0, out8_0_U8_L1;
        __m128i avg8_0_U8;

        for (y = 0; y < height; y += 2)
        {
            //--------
            //Line One
            //--------

            //List0
            inPixel0 = _mm_loadl_epi64((__m128i*)ref16L0);
            inPixel1 = _mm_srli_epi16(inPixel0, 2) ;
            out8_0_U8_L0 = _mm_packus_epi16( inPixel1 , inPixel1 );

            //List1
            inPixel0 = _mm_loadl_epi64((__m128i*)ref16L1);
            inPixel1 = _mm_srli_epi16(inPixel0, 2) ;
            out8_0_U8_L1 = _mm_packus_epi16( inPixel1 , inPixel1 );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);

            *(EB_U32*)dstPtr = _mm_cvtsi128_si32(avg8_0_U8);

            //--------
            //Line Two
            //--------

            //List0
            inPixel0 = _mm_loadl_epi64((__m128i*)(ref16L0+ refL0Stride) );
            inPixel1 = _mm_srli_epi16(inPixel0, 2) ;
            out8_0_U8_L0 = _mm_packus_epi16( inPixel1 , inPixel1 );

            //List1

            inPixel0 = _mm_loadl_epi64((__m128i*)(ref16L1+ refL1Stride) );
            inPixel1 = _mm_srli_epi16(inPixel0, 2) ;
            out8_0_U8_L1 = _mm_packus_epi16( inPixel1 , inPixel1 );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);

            *(EB_U32*)(dstPtr+dstStride) = _mm_cvtsi128_si32(avg8_0_U8);

            dstPtr  += 2*dstStride;
            ref16L0 += 2*refL0Stride;
            ref16L1 += 2*refL1Stride;

        }

    }
    else if (width == 8)
    {

        __m128i out8_0_U8_L0, out8_0_U8_L1, out8_2_U8_L0,out8_2_U8_L1;
        __m128i avg8_0_U8,avg8_2_U8;

        for (y = 0; y < height; y += 2)
        {
            //--------
            //Line One
            //--------

            //List0

            inPixel0 = _mm_loadu_si128((__m128i*) ref16L0);

            inPixel1 = _mm_srli_epi16(inPixel0, 2) ;
            out8_0_U8_L0 = _mm_packus_epi16( inPixel1 , inPixel1 );

             //List1

            inPixel0 = _mm_loadu_si128((__m128i*) ref16L1);

            inPixel1 = _mm_srli_epi16(inPixel0, 2) ;
            out8_0_U8_L1 = _mm_packus_epi16( inPixel1 , inPixel1 );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);

            _mm_storel_epi64((__m128i*) dstPtr      , avg8_0_U8);


            //--------
            //Line Two
            //--------

            //List0

            inPixel0 = _mm_loadu_si128((__m128i*)(ref16L0 + refL0Stride) );

            inPixel1 = _mm_srli_epi16(inPixel0, 2) ;
            out8_2_U8_L0 = _mm_packus_epi16( inPixel1 , inPixel1 );

            //List1

            inPixel0 = _mm_loadu_si128((__m128i*)(ref16L1 + refL1Stride) );

            inPixel1 = _mm_srli_epi16(inPixel0, 2) ;
            out8_2_U8_L1 = _mm_packus_epi16( inPixel1 , inPixel1 );

            //AVG
            avg8_2_U8 =  _mm_avg_epu8 (out8_2_U8_L0 , out8_2_U8_L1);

            _mm_storel_epi64((__m128i*)(dstPtr +dstStride)    , avg8_2_U8);


            dstPtr  += 2*dstStride;
            ref16L0 += 2*refL0Stride;
            ref16L1 += 2*refL1Stride;
        }

    }
    else if (width == 16)
    {
        __m128i inPixel4, inPixel5;
        __m128i out8_0_U8_L0, out8_0_U8_L1, out8_2_U8_L0,out8_2_U8_L1;
        __m128i avg8_0_U8,avg8_2_U8;

        for (y = 0; y < height; y += 2)
        {
            //--------
            //Line One
            //--------

            //List0

            inPixel0 = _mm_loadu_si128((__m128i*)  ref16L0);
            inPixel1 = _mm_loadu_si128((__m128i*) (ref16L0 + 8));

            out8_0_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(inPixel0, 2)  ,  _mm_srli_epi16(inPixel1, 2)  );

             //List1

            inPixel0 = _mm_loadu_si128((__m128i*) ref16L1);
            inPixel1 = _mm_loadu_si128((__m128i*)(ref16L1 + 8));

            out8_0_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(inPixel0, 2)  ,  _mm_srli_epi16(inPixel1, 2)  );


            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);

            _mm_storeu_si128((__m128i*) dstPtr      , avg8_0_U8);


            //--------
            //Line Two
            //--------

            //List0

            inPixel4 = _mm_loadu_si128((__m128i*) (ref16L0 + refL0Stride));
            inPixel5 = _mm_loadu_si128((__m128i*) (ref16L0 + refL0Stride + 8));

               out8_2_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(inPixel4, 2)  ,  _mm_srli_epi16(inPixel5, 2)  );

            //List1

            inPixel4 = _mm_loadu_si128((__m128i*) (ref16L1 + refL1Stride));
            inPixel5 = _mm_loadu_si128((__m128i*) (ref16L1 + refL1Stride + 8));

               out8_2_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(inPixel4, 2)  ,  _mm_srli_epi16(inPixel5, 2)  );


            //AVG
            avg8_2_U8 =  _mm_avg_epu8 (out8_2_U8_L0 , out8_2_U8_L1);

            _mm_storeu_si128((__m128i*)(dstPtr  + dstStride      ) , avg8_2_U8);

            dstPtr  += 2*dstStride;
            ref16L0 += 2*refL0Stride;
            ref16L1 += 2*refL1Stride;

        }

    }
    else if (width == 32)
    {
        __m128i inPixel2, inPixel3, inPixel4, inPixel5, inPixel6, inPixel7;
        __m128i out8_0_U8_L0, out8_1_U8_L0, out8_2_U8_L0, out8_3_U8_L0;
        __m128i out8_0_U8_L1, out8_1_U8_L1, out8_2_U8_L1, out8_3_U8_L1;
        __m128i avg8_0_U8, avg8_1_U8, avg8_2_U8, avg8_3_U8;


       for (y = 0; y < height; y += 2)
        {
            //--------
            //Line One
            //--------

            //List0

            inPixel0 =  _mm_loadu_si128((__m128i*)  ref16L0);
            inPixel1 =  _mm_loadu_si128((__m128i*) (ref16L0 + 8));
            inPixel2 =  _mm_loadu_si128((__m128i*) (ref16L0 + 16));
            inPixel3 =  _mm_loadu_si128((__m128i*) (ref16L0 + 24));

            out8_0_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(inPixel0, 2)  ,  _mm_srli_epi16(inPixel1, 2)  );
            out8_1_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(inPixel2, 2)  ,  _mm_srli_epi16(inPixel3, 2)  );

             //List1

            inPixel0 = _mm_loadu_si128((__m128i*) ref16L1);
            inPixel1 = _mm_loadu_si128((__m128i*)(ref16L1 + 8));
            inPixel2 = _mm_loadu_si128((__m128i*)(ref16L1 + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(ref16L1 + 24));

            out8_0_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(inPixel0, 2)  ,  _mm_srli_epi16(inPixel1, 2)  );
            out8_1_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(inPixel2, 2)  ,  _mm_srli_epi16(inPixel3, 2)  );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);
            avg8_1_U8 =  _mm_avg_epu8 (out8_1_U8_L0 , out8_1_U8_L1);

            _mm_storeu_si128((__m128i*) dstPtr      , avg8_0_U8);
            _mm_storeu_si128((__m128i*)(dstPtr + 16), avg8_1_U8);


            //--------
            //Line Two
            //--------

            //List0

            inPixel4 = _mm_loadu_si128((__m128i*) (ref16L0 + refL0Stride));
            inPixel5 = _mm_loadu_si128((__m128i*) (ref16L0 + refL0Stride + 8));
            inPixel6 = _mm_loadu_si128((__m128i*) (ref16L0 + refL0Stride + 16));
            inPixel7 = _mm_loadu_si128((__m128i*) (ref16L0 + refL0Stride + 24));

               out8_2_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(inPixel4, 2)  ,  _mm_srli_epi16(inPixel5, 2)  );
            out8_3_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(inPixel6, 2)  ,  _mm_srli_epi16(inPixel7, 2)  );

            //List1

            inPixel4 = _mm_loadu_si128((__m128i*) (ref16L1 + refL1Stride));
            inPixel5 = _mm_loadu_si128((__m128i*) (ref16L1 + refL1Stride + 8));
            inPixel6 = _mm_loadu_si128((__m128i*) (ref16L1 + refL1Stride + 16));
            inPixel7 = _mm_loadu_si128((__m128i*) (ref16L1 + refL1Stride + 24));

               out8_2_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(inPixel4, 2)  ,  _mm_srli_epi16(inPixel5, 2)  );
            out8_3_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(inPixel6, 2)  ,  _mm_srli_epi16(inPixel7, 2)  );

            //AVG
            avg8_2_U8 =  _mm_avg_epu8 (out8_2_U8_L0 , out8_2_U8_L1);
            avg8_3_U8 =  _mm_avg_epu8 (out8_3_U8_L0 , out8_3_U8_L1);

            _mm_storeu_si128((__m128i*)(dstPtr  + dstStride      ) , avg8_2_U8);
            _mm_storeu_si128((__m128i*)(dstPtr  + dstStride + 16 ) , avg8_3_U8);

            dstPtr  += 2*dstStride;
            ref16L0 += 2*refL0Stride;
            ref16L1 += 2*refL1Stride;

        }

    }
    else if (width == 64)
    {
        __m128i inPixel2, inPixel3, inPixel4, inPixel5, inPixel6, inPixel7;
        __m128i out8_0_U8_L0, out8_1_U8_L0, out8_2_U8_L0, out8_3_U8_L0;
        __m128i out8_0_U8_L1, out8_1_U8_L1, out8_2_U8_L1, out8_3_U8_L1;
        __m128i avg8_0_U8, avg8_1_U8, avg8_2_U8, avg8_3_U8;

        for (y = 0; y < height; ++y)
        {
            //List0

            inPixel0 = _mm_loadu_si128((__m128i*) ref16L0);
            inPixel1 = _mm_loadu_si128((__m128i*)(ref16L0 + 8));
            inPixel2 = _mm_loadu_si128((__m128i*)(ref16L0 + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(ref16L0 + 24));
            inPixel4 = _mm_loadu_si128((__m128i*)(ref16L0 + 32));
            inPixel5 = _mm_loadu_si128((__m128i*)(ref16L0 + 40));
            inPixel6 = _mm_loadu_si128((__m128i*)(ref16L0 + 48));
            inPixel7 = _mm_loadu_si128((__m128i*)(ref16L0 + 56));



            out8_0_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(inPixel0, 2)  ,  _mm_srli_epi16(inPixel1, 2)  );
            out8_1_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(inPixel2, 2)  ,  _mm_srli_epi16(inPixel3, 2)  );
            out8_2_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(inPixel4, 2)  ,  _mm_srli_epi16(inPixel5, 2)  );
            out8_3_U8_L0 = _mm_packus_epi16(  _mm_srli_epi16(inPixel6, 2)  ,  _mm_srli_epi16(inPixel7, 2)  );


            //List1

            inPixel0 = _mm_loadu_si128((__m128i*) ref16L1);
            inPixel1 = _mm_loadu_si128((__m128i*)(ref16L1 + 8));
            inPixel2 = _mm_loadu_si128((__m128i*)(ref16L1 + 16));
            inPixel3 = _mm_loadu_si128((__m128i*)(ref16L1 + 24));
            inPixel4 = _mm_loadu_si128((__m128i*)(ref16L1 + 32));
            inPixel5 = _mm_loadu_si128((__m128i*)(ref16L1 + 40));
            inPixel6 = _mm_loadu_si128((__m128i*)(ref16L1 + 48));
            inPixel7 = _mm_loadu_si128((__m128i*)(ref16L1 + 56));


            //Note: old Version used to use _mm_and_si128 to mask the MSB bits of the pixels
            out8_0_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(inPixel0, 2)  ,  _mm_srli_epi16(inPixel1, 2)  );
            out8_1_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(inPixel2, 2)  ,  _mm_srli_epi16(inPixel3, 2)  );
            out8_2_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(inPixel4, 2)  ,  _mm_srli_epi16(inPixel5, 2)  );
            out8_3_U8_L1 = _mm_packus_epi16(  _mm_srli_epi16(inPixel6, 2)  ,  _mm_srli_epi16(inPixel7, 2)  );

            //AVG
            avg8_0_U8 =  _mm_avg_epu8 (out8_0_U8_L0 , out8_0_U8_L1);
            avg8_1_U8 =  _mm_avg_epu8 (out8_1_U8_L0 , out8_1_U8_L1);
            avg8_2_U8 =  _mm_avg_epu8 (out8_2_U8_L0 , out8_2_U8_L1);
            avg8_3_U8 =  _mm_avg_epu8 (out8_3_U8_L0 , out8_3_U8_L1);

            _mm_storeu_si128((__m128i*) dstPtr      , avg8_0_U8);
            _mm_storeu_si128((__m128i*)(dstPtr + 16), avg8_1_U8);
            _mm_storeu_si128((__m128i*)(dstPtr + 32), avg8_2_U8);
            _mm_storeu_si128((__m128i*)(dstPtr + 48), avg8_3_U8);


            dstPtr  += dstStride;
            ref16L0 += refL0Stride;
            ref16L1 += refL1Stride;
        }
    }


    return;
}
