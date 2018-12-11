/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbPackUnPack_C.h"
#include "EbDefinitions.h"

/************************************************
* pack 8 and 2 bit 2D data into 10 bit data
************************************************/
void EB_ENC_msbPack2D(
    EB_U8     *in8BitBuffer,
    EB_U32     in8Stride,
    EB_U8     *innBitBuffer,
    EB_U16    *out16BitBuffer,
    EB_U32     innStride,
    EB_U32     outStride,
    EB_U32     width,
    EB_U32     height)
{
    EB_U64   j, k;
    EB_U16   outPixel;
    EB_U8    nBitPixel;

    //SIMD hint: use _mm_unpacklo_epi8 +_mm_unpackhi_epi8 to do the concatenation

    for (j = 0; j<height; j++)
    {
        for (k = 0; k<width; k++)
        {
            outPixel = (in8BitBuffer[k + j*in8Stride]) << 2;
            nBitPixel = (innBitBuffer[k + j*innStride] >> 6) & 3;

            out16BitBuffer[k + j*outStride] = outPixel | nBitPixel;
        }
    }
}

/************************************************
* pack 8 and 2 bit 2D data into 10 bit data
2bit data storage : 4 2bit-pixels in one byte
************************************************/
void CompressedPackmsb(
	EB_U8     *in8BitBuffer,
	EB_U32     in8Stride,
	EB_U8     *innBitBuffer,
	EB_U16    *out16BitBuffer,
	EB_U32     innStride,
	EB_U32     outStride,
	EB_U32     width,
	EB_U32     height)
{
	EB_U64   row, kIdx;
	EB_U16   outPixel;
	EB_U8    nBitPixel;
	EB_U8   four2bitPels;

	for (row = 0; row<height; row++)
	{
		for (kIdx = 0; kIdx<width / 4; kIdx++)
		{

			four2bitPels = innBitBuffer[kIdx + row*innStride];

			nBitPixel = (four2bitPels >> 6) & 3;

			outPixel = in8BitBuffer[kIdx * 4 + 0 + row*in8Stride] << 2;
			out16BitBuffer[kIdx * 4 + 0 + row*outStride] = outPixel | nBitPixel;

			nBitPixel = (four2bitPels >> 4) & 3;
			outPixel = in8BitBuffer[kIdx * 4 + 1 + row*in8Stride] << 2;
			out16BitBuffer[kIdx * 4 + 1 + row*outStride] = outPixel | nBitPixel;

			nBitPixel = (four2bitPels >> 2) & 3;
			outPixel = in8BitBuffer[kIdx * 4 + 2 + row*in8Stride] << 2;
			out16BitBuffer[kIdx * 4 + 2 + row*outStride] = outPixel | nBitPixel;

			nBitPixel = (four2bitPels >> 0) & 3;
			outPixel = in8BitBuffer[kIdx * 4 + 3 + row*in8Stride] << 2;
			out16BitBuffer[kIdx * 4 + 3 + row*outStride] = outPixel | nBitPixel;


		}
	}
}

/************************************************
* convert unpacked nbit (n=2) data to compressedPAcked
2bit data storage : 4 2bit-pixels in one byte
************************************************/
void CPack_C(
	const EB_U8     *innBitBuffer,
	EB_U32     innStride,
	EB_U8     *inCompnBitBuffer,
	EB_U32     outStride,
	EB_U8    *localCache,
	EB_U32     width,
	EB_U32     height)
{
	EB_U32 rowIndex, colIndex;
	(void)localCache;

	for (rowIndex = 0; rowIndex < height; rowIndex++)
	{
		for (colIndex = 0; colIndex < width; colIndex += 4)
		{
			EB_U32 i = colIndex + rowIndex*innStride;

			EB_U8 compressedUnpackedPixel = 0;
			compressedUnpackedPixel = compressedUnpackedPixel | ((innBitBuffer[i + 0] >> 0) & 0xC0);//1100.0000
			compressedUnpackedPixel = compressedUnpackedPixel | ((innBitBuffer[i + 1] >> 2) & 0x30);//0011.0000
			compressedUnpackedPixel = compressedUnpackedPixel | ((innBitBuffer[i + 2] >> 4) & 0x0C);//0000.1100
			compressedUnpackedPixel = compressedUnpackedPixel | ((innBitBuffer[i + 3] >> 6) & 0x03);//0000.0011

			EB_U32 j = colIndex / 4 + rowIndex*outStride;
			inCompnBitBuffer[j] = compressedUnpackedPixel;
		}
	}

}


/************************************************
* unpack 10 bit data into  8 and 2 bit 2D data
************************************************/
void EB_ENC_msbUnPack2D(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,
    EB_U8       *outnBitBuffer,
    EB_U32       out8Stride,
    EB_U32       outnStride,
    EB_U32       width,
    EB_U32       height)
{
    EB_U64   j, k;
    EB_U16   inPixel;
    EB_U8    tmpPixel;
    for (j = 0; j<height; j++)
    {
        for (k = 0; k<width; k++)
        {
            inPixel = in16BitBuffer[k + j*inStride];
            out8BitBuffer[k + j*out8Stride] = (EB_U8)(inPixel >> 2);
            tmpPixel = (EB_U8)(inPixel << 6);
            outnBitBuffer[k + j*outnStride] = tmpPixel;
        }
    }

}
void UnPack8BitData(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,  
    EB_U32       out8Stride, 
    EB_U32       width,
    EB_U32       height)
{
    EB_U64   j, k;
    EB_U16   inPixel;
    //EB_U8    tmpPixel;
    for (j = 0; j<height; j++)
    {
        for (k = 0; k<width; k++)
        {
            inPixel = in16BitBuffer[k + j*inStride];
            out8BitBuffer[k + j*out8Stride] = (EB_U8)(inPixel >> 2);
            //tmpPixel = (EB_U8)(inPixel << 6);
            //outnBitBuffer[k + j*outnStride] = tmpPixel;
        }
    }

}
void UnpackAvg(
        EB_U16 *ref16L0,
        EB_U32  refL0Stride,
        EB_U16 *ref16L1,
        EB_U32  refL1Stride,
        EB_U8  *dstPtr,
        EB_U32  dstStride,      
        EB_U32  width,
        EB_U32  height )
 {
 
    EB_U64   j, k;
    EB_U8   inPixelL0, inPixelL1;

    for (j = 0; j<height; j++)
    {
        for (k = 0; k<width; k++)
        {
            inPixelL0 = (EB_U8)(ref16L0[k + j*refL0Stride]>>2);
            inPixelL1 = (EB_U8)(ref16L1[k + j*refL1Stride]>>2);
            dstPtr[k + j*dstStride] = (inPixelL0  + inPixelL1 + 1)>>1;
          
        }
    }

 
 }

void UnPack8BitDataSafeSub(
    EB_U16      *in16BitBuffer,
    EB_U32       inStride,
    EB_U8       *out8BitBuffer,  
    EB_U32       out8Stride, 
    EB_U32       width,
    EB_U32       height
    )
{
    EB_U64   j, k;
    EB_U16   inPixel;
    //EB_U8    tmpPixel;
    for (j = 0; j<height; j++)
    {
        for (k = 0; k<width; k++)
        {
            inPixel = in16BitBuffer[k + j*inStride];
            out8BitBuffer[k + j*out8Stride] = (EB_U8)(inPixel >> 2);
            //tmpPixel = (EB_U8)(inPixel << 6);
            //outnBitBuffer[k + j*outnStride] = tmpPixel;
        }
    }
}

void UnpackAvgSafeSub(
        EB_U16 *ref16L0,
        EB_U32  refL0Stride,
        EB_U16 *ref16L1,
        EB_U32  refL1Stride,
        EB_U8  *dstPtr,
        EB_U32  dstStride,    
        EB_U32  width,
        EB_U32  height )
 {
 
    EB_U64   j, k;
    EB_U8   inPixelL0, inPixelL1;

    for (j = 0; j<height; j++)
    {
        for (k = 0; k<width; k++)
        {
            inPixelL0 = (EB_U8)(ref16L0[k + j*refL0Stride]>>2);
            inPixelL1 = (EB_U8)(ref16L1[k + j*refL1Stride]>>2);
            dstPtr[k + j*dstStride] = (inPixelL0  + inPixelL1 + 1)>>1;
          
        }
    } 
 }
