/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbAvcStyleMcp.h"
#include "EbPictureOperators.h"

static const   EB_U8  FracMappedPosTabX[16] = { 0, 1, 2, 3,
	0, 1, 2, 3,
	0, 0, 2, 0,
	0, 1, 2, 3 };
	
static const   EB_U8  FracMappedPosTabY[16] = { 0, 0, 0, 0,
	1, 0, 0, 0,
	2, 2, 2, 2,
	3, 0, 0, 0 };
	
static const   EB_U8  IntegerPosoffsetTabX[16] = { 0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 1,
	0, 0, 0, 0 };
	
static const   EB_U8  IntegerPosoffsetTabY[16] = { 0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 1, 1, 1 };




void UnpackUniPredRef10Bit( 
    EbPictureBufferDesc_t *refFramePicList0, 
    EB_U32                 posX,
    EB_U32                 posY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *dst,
    EB_U32                 dstLumaIndex, 
    EB_U32                 dstChromaIndex,          //input parameter, please refer to the detailed explanation above.
    EB_U32                 componentMask,
    EB_BYTE                tempBuf)
{
    EB_U32   chromaPuWidth      = puWidth >> 1;
    EB_U32   chromaPuHeight     = puHeight >> 1;

    //doing the luma interpolation
    if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) 
    {
        (void) tempBuf;
        EB_U32 inPosx = (posX >> 2);
        EB_U32 inPosy = (posY >> 2);
        EB_U16 *ptr16    =  (EB_U16 *)refFramePicList0->bufferY  + inPosx + inPosy*refFramePicList0->strideY;

          Extract8BitdataSafeSub(
            ptr16,
            refFramePicList0->strideY,
            dst->bufferY + dstLumaIndex,
            dst->strideY,
            puWidth,
            puHeight
            );


    }


    //chroma
    if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {

        {
        EB_U32 inPosx = (posX >> 3);
        EB_U32 inPosy = (posY >> 3);
        EB_U16 *ptr16    =  (EB_U16 *)refFramePicList0->bufferCb  + inPosx + inPosy*refFramePicList0->strideCb;
  
          extract8Bitdata(
            ptr16,
            refFramePicList0->strideCb,
            dst->bufferCb + dstChromaIndex,
		    dst->strideCb,
            chromaPuWidth,
            chromaPuHeight
            );

        ptr16    =  (EB_U16 *)refFramePicList0->bufferCr  + inPosx + inPosy*refFramePicList0->strideCr;
           
        extract8Bitdata(
            ptr16,
            refFramePicList0->strideCr,
            dst->bufferCr + dstChromaIndex,
		    dst->strideCr,
            chromaPuWidth,
            chromaPuHeight
            );

        }
    }


}

void UnpackBiPredRef10Bit( 
    EbPictureBufferDesc_t *refFramePicList0,
    EbPictureBufferDesc_t *refFramePicList1,
    EB_U32                 refList0PosX,
    EB_U32                 refList0PosY,
    EB_U32                 refList1PosX,
    EB_U32                 refList1PosY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *biDst,
    EB_U32                 dstLumaIndex, 
	EB_U32                 dstChromaIndex,
	EB_U32                 componentMask,
    EB_BYTE                refList0TempDst,
    EB_BYTE                refList1TempDst,
    EB_BYTE                firstPassIFTempDst)
{
    EB_U32   chromaPuWidth      = puWidth >> 1;
    EB_U32   chromaPuHeight     = puHeight >> 1;

	//Luma
    if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {

        (void) firstPassIFTempDst;
        (void) refList0TempDst;
        (void) refList1TempDst;
       UnpackL0L1AvgSafeSub(
            (EB_U16 *)refFramePicList0->bufferY  + (refList0PosX >> 2) + (refList0PosY >> 2)*refFramePicList0->strideY,
            refFramePicList0->strideY,                                                                              
            (EB_U16 *)refFramePicList1->bufferY  + (refList1PosX >> 2) + (refList1PosY >> 2)*refFramePicList1->strideY,
            refFramePicList1->strideY,                                                                              
            biDst->bufferY + dstLumaIndex,                                                                             
            biDst->strideY,                                                                                            
            puWidth,                                                                                                   
            puHeight
            );



	}

	//uni-prediction List0 chroma
    if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {

         UnpackL0L1Avg(
            (EB_U16 *)refFramePicList0->bufferCb  + (refList0PosX >> 3) + (refList0PosY >> 3)*refFramePicList0->strideCb,
            refFramePicList0->strideCb,                                                                               
            (EB_U16 *)refFramePicList1->bufferCb  + (refList1PosX >> 3) + (refList1PosY >> 3)*refFramePicList1->strideCb,
            refFramePicList1->strideCb,                                                                                                                                                       
            biDst->bufferCb + dstChromaIndex,
			biDst->strideCb ,                                                                                          
            chromaPuWidth,
            chromaPuHeight
            );


        UnpackL0L1Avg(
            (EB_U16 *)refFramePicList0->bufferCr  + (refList0PosX >> 3) + (refList0PosY >> 3)*refFramePicList0->strideCr,
            refFramePicList0->strideCr,                                                                               
            (EB_U16 *)refFramePicList1->bufferCr  + (refList1PosX >> 3) + (refList1PosY >> 3)*refFramePicList1->strideCr,
            refFramePicList1->strideCr,                                                                                                                                                       
            biDst->bufferCr + dstChromaIndex,
			biDst->strideCr,                                                                                          
            chromaPuWidth,
            chromaPuHeight
            );

     }
}

void UniPredIFreeRef8Bit(
    EbPictureBufferDesc_t *refPic,
    EB_U32                 posX,
    EB_U32                 posY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *dst,
    EB_U32                 dstLumaIndex, 
    EB_U32                 dstChromaIndex,          //input parameter, please refer to the detailed explanation above.
    EB_U32                 componentMask,
    EB_BYTE                tempBuf)
{
    EB_U32   integPosx;
    EB_U32   integPosy;
    EB_U8    fracPosx,mappedFracPosx;
    EB_U8    fracPosy,mappedFracPosy;
    EB_U32   lumaStride;
    EB_U32   chromaPuWidth      = puWidth >> 1;
    EB_U32   chromaPuHeight     = puHeight >> 1;

	EB_U8    fracPos;

    lumaStride = dst->strideY;

    //luma
    //compute the luma fractional position
    integPosx = (posX >> 2);
    integPosy = (posY >> 2);
     
    fracPosx  = posX & 0x03;
    fracPosy  = posY & 0x03;
  
    //New Interpolation Mapping
    mappedFracPosx = fracPosx;
    mappedFracPosy = fracPosy;

	fracPos = (mappedFracPosx + (mappedFracPosy << 2));
	mappedFracPosx = FracMappedPosTabX[fracPos];
	mappedFracPosy = FracMappedPosTabY[fracPos];
	integPosx += IntegerPosoffsetTabX[fracPos];
	integPosy += IntegerPosoffsetTabY[fracPos];
   if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) 
   {
	   //doing the luma interpolation
       AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][0](
		   refPic->bufferY + integPosx + integPosy*refPic->strideY, refPic->strideY,
		   dst->bufferY + dstLumaIndex, lumaStride,
		   puWidth, puHeight,
		   tempBuf,
		   mappedFracPosx ? mappedFracPosx : mappedFracPosy);

   }
    //chroma
    if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {
        //compute the chroma fractional position
        integPosx = (posX >> 3);
        integPosy = (posY >> 3);
        fracPosx  = posX & 0x07;
        fracPosy  = posY & 0x07;


        mappedFracPosx = 0;
        if (fracPosx > 4){
            integPosx++;
        }
        mappedFracPosy = 0;
        if (fracPosy > 4){
            integPosy++;
        }
        
        // Note: chromaPuWidth equals 4 is only supported in Intrinsic 
	   //       for integer positions ( mappedFracPosx + (mappedFracPosy << 3) equals 0 )
	   //doing the chroma Cb interpolation
       AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][0](
		   refPic->bufferCb + integPosx + integPosy*refPic->strideCb,
		   refPic->strideCb,
		   dst->bufferCb + dstChromaIndex,
		   dst->strideCb,
		   chromaPuWidth,
		   chromaPuHeight,
		   tempBuf,
		   mappedFracPosx ? mappedFracPosx : mappedFracPosy);

	   //doing the chroma Cr interpolation
       AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][0](
		   refPic->bufferCr + integPosx + integPosy*refPic->strideCr,
		   refPic->strideCr,
		   dst->bufferCr + dstChromaIndex,
		   dst->strideCr,
		   chromaPuWidth,
		   chromaPuHeight,
		   tempBuf,
		   mappedFracPosx ? mappedFracPosx : mappedFracPosy);

    }
}

void BiPredAverageKernel_C(
	EB_BYTE                  src0,
	EB_U32                   src0Stride,
	EB_BYTE                  src1,
	EB_U32                   src1Stride,
	EB_BYTE                  dst,
	EB_U32                   dstStride,
	EB_U32                   areaWidth,
	EB_U32                   areaHeight
	)
{


	EB_U32 i, j;

	
	for (j = 0; j < areaHeight; j++)
	{
		for (i = 0; i < areaWidth; i++)
		{
			dst[i + j * dstStride] = (src0[i + j * src0Stride] + src1[i + j * src1Stride] + 1) / 2;
		}
	}
		

}

typedef void(*EB_BIAVG_FUNC)(
	EB_BYTE                  src0,
	EB_U32                   src0Stride,
	EB_BYTE                  src1,
	EB_U32                   src1Stride,
	EB_BYTE                  dst,
	EB_U32                   dstStride,
	EB_U32                   areaWidth,
	EB_U32                   areaHeight);


/*****************************
* Function Tables
*****************************/
static EB_BIAVG_FUNC FUNC_TABLE BiPredAverageKernel_funcPtrArray[EB_ASM_TYPE_TOTAL] =
{
	BiPredAverageKernel_C,
#ifndef NON_AVX512_SUPPORT
	BiPredAverageKernel_AVX512_INTRIN
#else
    BiPredAverageKernel_AVX2_INTRIN
#endif
};


void BiPredIFreeRef8Bit(
    EbPictureBufferDesc_t *refPicList0,
    EbPictureBufferDesc_t *refPicList1,
    EB_U32                 refList0PosX,
    EB_U32                 refList0PosY,
    EB_U32                 refList1PosX,
    EB_U32                 refList1PosY,
    EB_U32                 puWidth,
    EB_U32                 puHeight,
    EbPictureBufferDesc_t *biDst,
    EB_U32                 dstLumaIndex, 
	EB_U32                 dstChromaIndex,
	EB_U32                 componentMask,
    EB_BYTE                refList0TempDst,
    EB_BYTE                refList1TempDst,
    EB_BYTE                firstPassIFTempDst)
{
    EB_U32   integPosx;
    EB_U32   integPosy;
    EB_U8    fracPosx,mappedFracPosx;
    EB_U8    fracPosy,mappedFracPosy;
	EB_U8    fracPos;

    EB_U32   lumaStride, refLumaStride;

    EB_U32   chromaPuWidth      = puWidth >> 1;
    EB_U32   chromaPuHeight     = puHeight >> 1;
    lumaStride = biDst->strideY;
    refLumaStride = refPicList0->strideY;
	EB_U8 shift = 0;

	//Luma
    if (componentMask & PICTURE_BUFFER_DESC_LUMA_MASK) {

		EB_U32   integPosL0x, integPosL1x;
		EB_U32   integPosL0y, integPosL1y;
		EB_U8    fracPosL0, fracPosL1;
		//uni-prediction List0 luma
		//compute the luma fractional position
		integPosL0x = (refList0PosX >> 2);
		integPosL0y = (refList0PosY >> 2);

		fracPosx = refList0PosX & 0x03;
		fracPosy = refList0PosY & 0x03;

		mappedFracPosx = fracPosx;
		mappedFracPosy = fracPosy;

		fracPosL0 = (mappedFracPosx + (mappedFracPosy << 2));

		//uni-prediction List1 luma
		//compute the luma fractional position
		integPosL1x = (refList1PosX >> 2);
		integPosL1y = (refList1PosY >> 2);

		fracPosx = refList1PosX & 0x03;
		fracPosy = refList1PosY & 0x03;

		mappedFracPosx = fracPosx;
		mappedFracPosy = fracPosy;

		fracPosL1 = (mappedFracPosx + (mappedFracPosy << 2));

		if ( (fracPosL0 == 0) && (fracPosL1 == 0) )	 
		{
#ifndef NON_AVX512_SUPPORT
			BiPredAverageKernel_funcPtrArray[!!(ASM_TYPES & AVX512_MASK) ](
#else
            BiPredAverageKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
#endif
				refPicList0->bufferY + integPosL0x + integPosL0y * refLumaStride,
				refLumaStride,
				refPicList1->bufferY + integPosL1x + integPosL1y * refLumaStride,
				refLumaStride,
				biDst->bufferY + dstLumaIndex,
				lumaStride,
				puWidth,
				puHeight);
		}
		else
		{
			//uni-prediction List0 luma
			//compute the luma fractional position
			integPosx = (refList0PosX >> 2);
			integPosy = (refList0PosY >> 2);

			fracPosx = refList0PosX & 0x03;
			fracPosy = refList0PosY & 0x03;

			mappedFracPosx = fracPosx;
			mappedFracPosy = fracPosy;

			fracPos = (mappedFracPosx + (mappedFracPosy << 2));
			mappedFracPosx = FracMappedPosTabX[fracPos];
			mappedFracPosy = FracMappedPosTabY[fracPos];
			integPosx += IntegerPosoffsetTabX[fracPos];
			integPosy += IntegerPosoffsetTabY[fracPos];

            AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][0](
				refPicList0->bufferY + integPosx + integPosy*refLumaStride, refLumaStride,
				refList0TempDst, puWidth,
				puWidth, puHeight,
				firstPassIFTempDst,
				mappedFracPosx ? mappedFracPosx : mappedFracPosy);

			//uni-prediction List1 luma
			//compute the luma fractional position
			integPosx = (refList1PosX >> 2);
			integPosy = (refList1PosY >> 2);

			fracPosx = refList1PosX & 0x03;
			fracPosy = refList1PosY & 0x03;

			mappedFracPosx = fracPosx;
			mappedFracPosy = fracPosy;

			fracPos = (mappedFracPosx + (mappedFracPosy << 2));
			mappedFracPosx = FracMappedPosTabX[fracPos];
			mappedFracPosy = FracMappedPosTabY[fracPos];
			integPosx += IntegerPosoffsetTabX[fracPos];
			integPosy += IntegerPosoffsetTabY[fracPos];

			//doing the luma interpolation
            AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][0](
				refPicList1->bufferY + integPosx + integPosy*refLumaStride, refLumaStride,
				refList1TempDst, puWidth,
				puWidth, puHeight,
				firstPassIFTempDst,
				mappedFracPosx ? mappedFracPosx : mappedFracPosy);


			// bi-pred luma
			PictureAverageArray[!!(ASM_TYPES & PREAVX2_MASK)](refList0TempDst, puWidth , refList1TempDst, puWidth , biDst->bufferY + dstLumaIndex, lumaStride , puWidth, puHeight );
		}

	}

	//uni-prediction List0 chroma
    if (componentMask & PICTURE_BUFFER_DESC_CHROMA_MASK) {

		EB_U32   integPosL0x, integPosL1x;
		EB_U32   integPosL0y, integPosL1y;
		EB_U8    fracPosL0, fracPosL1;

		//uni-prediction List0 luma
		integPosL0x = (refList0PosX >> 3);
		integPosL0y = (refList0PosY >> 3);
		fracPosx = refList0PosX & 0x07;
		fracPosy = refList0PosY & 0x07;

		mappedFracPosx = 0;
		if (fracPosx > 4)
			integPosL0x++;
		mappedFracPosy = 0;
		if (fracPosy > 4)
			integPosL0y++;

		fracPosL0 = mappedFracPosx + (mappedFracPosy << 3);

		//uni-prediction List1 luma
		integPosL1x = (refList1PosX >> 3);
		integPosL1y = (refList1PosY >> 3);
		fracPosx = refList1PosX & 0x07;
		fracPosy = refList1PosY & 0x07;

		mappedFracPosx = 0;
		if (fracPosx > 4)
			integPosL1x++;
		mappedFracPosy = 0;
		if (fracPosy > 4)
			integPosL1y++;

		fracPosL1 = mappedFracPosx + (mappedFracPosy << 3);

		shift = 0;
		if ((fracPosL0 == 0) && (fracPosL1 == 0))
		{
#ifndef NON_AVX512_SUPPORT
            BiPredAverageKernel_funcPtrArray[!!(ASM_TYPES & AVX512_MASK)](
#else
            BiPredAverageKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
#endif
				refPicList0->bufferCb + integPosL0x + integPosL0y * refPicList0->strideCb,
				refPicList0->strideCb,
				refPicList1->bufferCb + integPosL1x + integPosL1y * refPicList1->strideCb,
				refPicList1->strideCb,
				biDst->bufferCb + dstChromaIndex,
				biDst->strideCb << shift,
				chromaPuWidth,
				chromaPuHeight);

#ifndef NON_AVX512_SUPPORT
            BiPredAverageKernel_funcPtrArray[!!(ASM_TYPES & AVX512_MASK)](
#else
            BiPredAverageKernel_funcPtrArray[!!(ASM_TYPES & AVX2_MASK)](
#endif
				refPicList0->bufferCr + integPosL0x + integPosL0y * refPicList0->strideCr,
				refPicList0->strideCr,
				refPicList1->bufferCr + integPosL1x + integPosL1y * refPicList1->strideCr,
				refPicList1->strideCr,
				biDst->bufferCr + dstChromaIndex,
				biDst->strideCr << shift,
				chromaPuWidth,
				chromaPuHeight);

		}
		else
		{

			// bi-pred chroma  Cb
			// Note: chromaPuWidth equals 4 is only supported in Intrinsic 
			//       for integer positions ( mappedFracPosx + (mappedFracPosy << 3) equals 0 )
			//doing the chroma Cb interpolation list 0
			//compute the chroma fractional position
			integPosx = (refList0PosX >> 3);
			integPosy = (refList0PosY >> 3);
			fracPosx = refList0PosX & 0x07;
			fracPosy = refList0PosY & 0x07;

			mappedFracPosx = 0;
			if (fracPosx > 4)
				integPosx++;
			mappedFracPosy = 0;
			if (fracPosy > 4)
				integPosy++;

            AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][0](
				refPicList0->bufferCb + integPosx + integPosy*refPicList0->strideCb,
				refPicList0->strideCb,
				refList0TempDst,
				chromaPuWidth,
				chromaPuWidth,
				chromaPuHeight,
				firstPassIFTempDst,
				mappedFracPosx ? mappedFracPosx : mappedFracPosy);


			//doing the chroma Cb interpolation list 1

			integPosx = (refList1PosX >> 3);
			integPosy = (refList1PosY >> 3);
			fracPosx = refList1PosX & 0x07;
			fracPosy = refList1PosY & 0x07;

			mappedFracPosx = 0;
			if (fracPosx > 4)
				integPosx++;
			mappedFracPosy = 0;
			if (fracPosy > 4)
				integPosy++;

            AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][0](
				refPicList1->bufferCb + integPosx + integPosy*refPicList1->strideCb,
				refPicList1->strideCb,
				refList1TempDst,
				chromaPuWidth,
				chromaPuWidth,
				chromaPuHeight,
				firstPassIFTempDst,
				mappedFracPosx ? mappedFracPosx : mappedFracPosy);

			// bi-pred Chroma Cb
			PictureAverageArray[!!(ASM_TYPES & PREAVX2_MASK)](
				refList0TempDst,
				chromaPuWidth << shift,
				refList1TempDst,
				chromaPuWidth << shift,
				biDst->bufferCb + dstChromaIndex,
				biDst->strideCb << shift,
				chromaPuWidth,
				chromaPuHeight >> shift
				);


			// bi-pred chroma  Cr
			// Note: chromaPuWidth equals 4 is only supported in Intrinsic 
			//       for integer positions ( mappedFracPosx + (mappedFracPosy << 3) equals 0 )
			//doing the chroma Cb interpolation list 0
			//compute the chroma fractional position
			integPosx = (refList0PosX >> 3);
			integPosy = (refList0PosY >> 3);
			fracPosx = refList0PosX & 0x07;
			fracPosy = refList0PosY & 0x07;

			mappedFracPosx = 0;
			if (fracPosx > 4)
				integPosx++;
			mappedFracPosy = 0;
			if (fracPosy > 4)
				integPosy++;

            AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][0](
				refPicList0->bufferCr + integPosx + integPosy*refPicList0->strideCr,
				refPicList0->strideCr,
				refList0TempDst,
				chromaPuWidth,
				chromaPuWidth,
				chromaPuHeight,
				firstPassIFTempDst,
				mappedFracPosx ? mappedFracPosx : mappedFracPosy);


			//doing the chroma Cb interpolation list 1

			integPosx = (refList1PosX >> 3);
			integPosy = (refList1PosY >> 3);
			fracPosx = refList1PosX & 0x07;
			fracPosy = refList1PosY & 0x07;

			mappedFracPosx = 0;
			if (fracPosx > 4)
				integPosx++;
			mappedFracPosy = 0;
			if (fracPosy > 4)
				integPosy++;

            AvcStyleUniPredLumaIFFunctionPtrArray[!!(ASM_TYPES & PREAVX2_MASK)][0](
				refPicList1->bufferCr + integPosx + integPosy*refPicList1->strideCr,
				refPicList1->strideCr,
				refList1TempDst,
				chromaPuWidth,
				chromaPuWidth,
				chromaPuHeight,
				firstPassIFTempDst,
				mappedFracPosx ? mappedFracPosx : mappedFracPosy);

			// bi-pred Chroma Cr
			PictureAverageArray[!!(ASM_TYPES & PREAVX2_MASK)](
				refList0TempDst,
				chromaPuWidth << shift,
				refList1TempDst,
				chromaPuWidth << shift, 
				biDst->bufferCr + dstChromaIndex,
				biDst->strideCr << shift, 
				chromaPuWidth,
				chromaPuHeight >> shift
				);
		}
    }
}
