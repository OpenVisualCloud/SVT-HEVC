/*
* Copyright(c) 2018 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/

#include "EbMcp.h"
#include "EbMcp_C.h"
#include "EbMcp_AVX2.h"

/**************************************************
* Function Pointer Tables
**************************************************/

const sampleBiPredClipping biPredClippingFuncPtrArray[EB_ASM_TYPE_TOTAL] = {
    // C_DEFAULT
    BiPredClipping,
    // AVX2
    BiPredClipping_SSSE3,
};

const sampleBiPredClipping16bit biPredClipping16bitFuncPtrArray[EB_ASM_TYPE_TOTAL] = {
	// C_DEFAULT
    BiPredClipping16bit,
	BiPredClipping16bit_SSE2_INTRIN
};

// Luma
const InterpolationFilterNew uniPredLumaIFFunctionPtrArrayNew[EB_ASM_TYPE_TOTAL][16] = {     //[ASM type][Interpolation position]
    // C_DEFAULT
    {
        LumaInterpolationCopy,                             //A
        LumaInterpolationFilterPosaNew,                    //a
        LumaInterpolationFilterPosbNew,                    //b
        LumaInterpolationFilterPoscNew,                    //c
        LumaInterpolationFilterPosdNew,                    //d
        LumaInterpolationFilterPoseNew,                    //e
        LumaInterpolationFilterPosfNew,                    //f
        LumaInterpolationFilterPosgNew,                    //g
        LumaInterpolationFilterPoshNew,                    //h
        LumaInterpolationFilterPosiNew,                    //i
        LumaInterpolationFilterPosjNew,                    //j
        LumaInterpolationFilterPoskNew,                    //k
        LumaInterpolationFilterPosnNew,                    //n
        LumaInterpolationFilterPospNew,                    //p
        LumaInterpolationFilterPosqNew,                    //q
        LumaInterpolationFilterPosrNew,                    //r
    },
    // AVX2
    {
        LumaInterpolationCopy_SSSE3,                        //A
        LumaInterpolationFilterPosa_SSSE3,					//a
        LumaInterpolationFilterPosb_SSSE3,                  //b
        LumaInterpolationFilterPosc_SSSE3,                  //c
        LumaInterpolationFilterPosd_SSSE3,                  //d
        LumaInterpolationFilterPose_SSSE3,                  //e
        LumaInterpolationFilterPosf_SSSE3,                  //f
        LumaInterpolationFilterPosg_SSSE3,                  //g
        LumaInterpolationFilterPosh_SSSE3,                  //h
        LumaInterpolationFilterPosi_SSSE3,                  //i
        LumaInterpolationFilterPosj_SSSE3,                  //j
        LumaInterpolationFilterPosk_SSSE3,                  //k
        LumaInterpolationFilterPosn_SSSE3,                  //n
        LumaInterpolationFilterPosp_SSSE3,                  //p
        LumaInterpolationFilterPosq_SSSE3,                  //q
        LumaInterpolationFilterPosr_SSSE3,                  //r
    },
};

// Luma
const InterpolationFilterNew16bit uniPredLuma16bitIFFunctionPtrArray[EB_ASM_TYPE_TOTAL][16] = {     //[ASM type][Interpolation position]
		// C_DEFAULT
		{
            LumaInterpolationCopy16bit,		                //A
            LumaInterpolationFilterPosaNew16bit,            //a
            LumaInterpolationFilterPosbNew16bit,            //b
            LumaInterpolationFilterPoscNew16bit,            //c
            LumaInterpolationFilterPosdNew16bit,            //d
            LumaInterpolationFilterPoseNew16bit,            //e
            LumaInterpolationFilterPosfNew16bit,            //f
            LumaInterpolationFilterPosgNew16bit,            //g
            LumaInterpolationFilterPoshNew16bit,            //h
            LumaInterpolationFilterPosiNew16bit,            //i
            LumaInterpolationFilterPosjNew16bit,            //j
            LumaInterpolationFilterPoskNew16bit,            //k
            LumaInterpolationFilterPosnNew16bit,            //n
            LumaInterpolationFilterPospNew16bit,            //p
            LumaInterpolationFilterPosqNew16bit,            //q
            LumaInterpolationFilterPosrNew16bit,            //r
		},
		// AVX2
		{
			LumaInterpolationCopy16bit_SSE2,                //A
			LumaInterpolationFilterPosa16bit_SSE2_INTRIN,	//a
			LumaInterpolationFilterPosb16bit_SSE2_INTRIN,	//b
			LumaInterpolationFilterPosc16bit_SSE2_INTRIN,	//c
			LumaInterpolationFilterPosd16bit_SSE2_INTRIN,	//d
			LumaInterpolationFilterPose16bit_SSE2_INTRIN,	//e
			LumaInterpolationFilterPosf16bit_SSE2_INTRIN,	//f
			LumaInterpolationFilterPosg16bit_SSE2_INTRIN,	//g
			LumaInterpolationFilterPosh16bit_SSE2_INTRIN,	//h
			LumaInterpolationFilterPosi16bit_SSE2_INTRIN,	//i
			LumaInterpolationFilterPosj16bit_SSE2_INTRIN,	//j
			LumaInterpolationFilterPosk16bit_SSE2_INTRIN,	//k
			LumaInterpolationFilterPosn16bit_SSE2_INTRIN,	//n
			LumaInterpolationFilterPosp16bit_SSE2_INTRIN,	//p
			LumaInterpolationFilterPosq16bit_SSE2_INTRIN,	//q
			LumaInterpolationFilterPosr16bit_SSE2_INTRIN,	//r
		},
};


const InterpolationFilterOutRaw biPredLumaIFFunctionPtrArrayNew[EB_ASM_TYPE_TOTAL][16] = {     //[ASM type][Interpolation position]
        // C_DEFAULT
        {
            LumaInterpolationCopyOutRaw,                        //A
            LumaInterpolationFilterPosaOutRaw,                  //a
            LumaInterpolationFilterPosbOutRaw,                  //b
            LumaInterpolationFilterPoscOutRaw,                  //c
            LumaInterpolationFilterPosdOutRaw,                  //d
            LumaInterpolationFilterPoseOutRaw,                  //e
            LumaInterpolationFilterPosfOutRaw,                  //f
            LumaInterpolationFilterPosgOutRaw,                  //g
            LumaInterpolationFilterPoshOutRaw,                  //h
            LumaInterpolationFilterPosiOutRaw,                  //i
            LumaInterpolationFilterPosjOutRaw,                  //j
            LumaInterpolationFilterPoskOutRaw,                  //k
            LumaInterpolationFilterPosnOutRaw,                  //n
            LumaInterpolationFilterPospOutRaw,                  //p
            LumaInterpolationFilterPosqOutRaw,                  //q
            LumaInterpolationFilterPosrOutRaw,                  //r
        },
        // AVX2
        {
            LumaInterpolationCopyOutRaw_SSSE3,                   //A
            LumaInterpolationFilterPosaOutRaw_SSSE3,             //a
            LumaInterpolationFilterPosbOutRaw_SSSE3,             //b
            LumaInterpolationFilterPoscOutRaw_SSSE3,             //c
            LumaInterpolationFilterPosdOutRaw_SSSE3,             //d
            LumaInterpolationFilterPoseOutRaw_SSSE3,             //e
            LumaInterpolationFilterPosfOutRaw_SSSE3,             //f
            LumaInterpolationFilterPosgOutRaw_SSSE3,             //g
            LumaInterpolationFilterPoshOutRaw_SSSE3,             //h
            LumaInterpolationFilterPosiOutRaw_SSSE3,             //i
            LumaInterpolationFilterPosjOutRaw_SSSE3,             //j
            LumaInterpolationFilterPoskOutRaw_SSSE3,             //k
            LumaInterpolationFilterPosnOutRaw_SSSE3,             //n
            LumaInterpolationFilterPospOutRaw_SSSE3,             //p
            LumaInterpolationFilterPosqOutRaw_SSSE3,             //q
            LumaInterpolationFilterPosrOutRaw_SSSE3,             //r
        },
};

const InterpolationFilterOutRaw16bit biPredLumaIFFunctionPtrArrayNew16bit[EB_ASM_TYPE_TOTAL][16] = {
		// C_DEFAULT
		{
            LumaInterpolationCopyOutRaw16bit,                             //A
            LumaInterpolationFilterPosaOutRaw16bit,                       //a
            LumaInterpolationFilterPosbOutRaw16bit,                       //b
            LumaInterpolationFilterPoscOutRaw16bit,                       //c
            LumaInterpolationFilterPosdOutRaw16bit,                       //d
            LumaInterpolationFilterPoseOutRaw16bit,                       //e
            LumaInterpolationFilterPosfOutRaw16bit,                       //f
            LumaInterpolationFilterPosgOutRaw16bit,                       //g
            LumaInterpolationFilterPoshOutRaw16bit,                       //h
            LumaInterpolationFilterPosiOutRaw16bit,                       //i
            LumaInterpolationFilterPosjOutRaw16bit,                       //j
            LumaInterpolationFilterPoskOutRaw16bit,                       //k
            LumaInterpolationFilterPosnOutRaw16bit,                       //n
            LumaInterpolationFilterPospOutRaw16bit,                       //p
            LumaInterpolationFilterPosqOutRaw16bit,                       //q
            LumaInterpolationFilterPosrOutRaw16bit,                       //r
		},
		// AVX2
		{
			LumaInterpolationCopyOutRaw16bit_SSE2_INTRIN,                  //A
			LumaInterpolationFilterPosaOutRaw16bit_SSE2_INTRIN,            //a
			LumaInterpolationFilterPosbOutRaw16bit_SSE2_INTRIN,            //b
			LumaInterpolationFilterPoscOutRaw16bit_SSE2_INTRIN,            //c
			LumaInterpolationFilterPosdOutRaw16bit_SSE2_INTRIN,            //d
			LumaInterpolationFilterPoseOutRaw16bit_SSE2_INTRIN,            //e
			LumaInterpolationFilterPosfOutRaw16bit_SSE2_INTRIN,            //f
			LumaInterpolationFilterPosgOutRaw16bit_SSE2_INTRIN,            //g
			LumaInterpolationFilterPoshOutRaw16bit_SSE2_INTRIN,            //h
			LumaInterpolationFilterPosiOutRaw16bit_SSE2_INTRIN,            //i
			LumaInterpolationFilterPosjOutRaw16bit_SSE2_INTRIN,            //j
			LumaInterpolationFilterPoskOutRaw16bit_SSE2_INTRIN,            //k
			LumaInterpolationFilterPosnOutRaw16bit_SSE2_INTRIN,            //n
			LumaInterpolationFilterPospOutRaw16bit_SSE2_INTRIN,            //p
			LumaInterpolationFilterPosqOutRaw16bit_SSE2_INTRIN,            //q
			LumaInterpolationFilterPosrOutRaw16bit_SSE2_INTRIN,            //r
		},
};

// Chroma
const ChromaFilterNew uniPredChromaIFFunctionPtrArrayNew[EB_ASM_TYPE_TOTAL][64] = {
        // C_DEFAULT
        {
            ChromaInterpolationCopy,                             //B
            ChromaInterpolationFilterOneD,                       //ab
            ChromaInterpolationFilterOneD,                       //ac
            ChromaInterpolationFilterOneD,                       //ad
            ChromaInterpolationFilterOneD,                       //ae
            ChromaInterpolationFilterOneD,                       //af
            ChromaInterpolationFilterOneD,                       //ag
            ChromaInterpolationFilterOneD,                       //ah
            ChromaInterpolationFilterOneD,                       //ba
            ChromaInterpolationFilterTwoD,                       //bb
            ChromaInterpolationFilterTwoD,                       //bc
            ChromaInterpolationFilterTwoD,                       //bd
            ChromaInterpolationFilterTwoD,                       //be
            ChromaInterpolationFilterTwoD,                       //bf
            ChromaInterpolationFilterTwoD,                       //bg
            ChromaInterpolationFilterTwoD,                       //bh
            ChromaInterpolationFilterOneD,                       //ca
            ChromaInterpolationFilterTwoD,                       //cb
            ChromaInterpolationFilterTwoD,                       //cc
            ChromaInterpolationFilterTwoD,                       //cd
            ChromaInterpolationFilterTwoD,                       //ce
            ChromaInterpolationFilterTwoD,                       //cf
            ChromaInterpolationFilterTwoD,                       //cg
            ChromaInterpolationFilterTwoD,                       //ch
            ChromaInterpolationFilterOneD,                       //da
            ChromaInterpolationFilterTwoD,                       //db
            ChromaInterpolationFilterTwoD,                       //dc
            ChromaInterpolationFilterTwoD,                       //dd
            ChromaInterpolationFilterTwoD,                       //de
            ChromaInterpolationFilterTwoD,                       //df
            ChromaInterpolationFilterTwoD,                       //dg
            ChromaInterpolationFilterTwoD,                       //dh
            ChromaInterpolationFilterOneD,                       //ea
            ChromaInterpolationFilterTwoD,                       //eb
            ChromaInterpolationFilterTwoD,                       //ec
            ChromaInterpolationFilterTwoD,                       //ed
            ChromaInterpolationFilterTwoD,                       //ee
            ChromaInterpolationFilterTwoD,                       //ef
            ChromaInterpolationFilterTwoD,                       //eg
            ChromaInterpolationFilterTwoD,                       //eh
            ChromaInterpolationFilterOneD,                       //fa
            ChromaInterpolationFilterTwoD,                       //fb
            ChromaInterpolationFilterTwoD,                       //fc
            ChromaInterpolationFilterTwoD,                       //fd
            ChromaInterpolationFilterTwoD,                       //fe
            ChromaInterpolationFilterTwoD,                       //ff
            ChromaInterpolationFilterTwoD,                       //fg
            ChromaInterpolationFilterTwoD,                       //fh
            ChromaInterpolationFilterOneD,                       //ga
            ChromaInterpolationFilterTwoD,                       //gb
            ChromaInterpolationFilterTwoD,                       //gc
            ChromaInterpolationFilterTwoD,                       //gd
            ChromaInterpolationFilterTwoD,                       //ge
            ChromaInterpolationFilterTwoD,                       //gf
            ChromaInterpolationFilterTwoD,                       //gg
            ChromaInterpolationFilterTwoD,                       //gh
            ChromaInterpolationFilterOneD,                       //ha
            ChromaInterpolationFilterTwoD,                       //hb
            ChromaInterpolationFilterTwoD,                       //hc
            ChromaInterpolationFilterTwoD,                       //hd
            ChromaInterpolationFilterTwoD,                       //he
            ChromaInterpolationFilterTwoD,                       //hf
            ChromaInterpolationFilterTwoD,                       //hg
            ChromaInterpolationFilterTwoD,                       //hh
        },
        // AVX2
        {

            ChromaInterpolationCopy_SSSE3,                       //B
            ChromaInterpolationFilterOneDHorizontal_SSSE3,		 //ab
            ChromaInterpolationFilterOneDHorizontal_SSSE3,       //ac
            ChromaInterpolationFilterOneDHorizontal_SSSE3,       //ad
            ChromaInterpolationFilterOneDHorizontal_SSSE3,       //ae
            ChromaInterpolationFilterOneDHorizontal_SSSE3,       //af
            ChromaInterpolationFilterOneDHorizontal_SSSE3,       //ag
            ChromaInterpolationFilterOneDHorizontal_SSSE3,       //ah
            ChromaInterpolationFilterOneDVertical_SSSE3,         //ba
            ChromaInterpolationFilterTwoD_SSSE3,                 //bb
            ChromaInterpolationFilterTwoD_SSSE3,                 //bc
            ChromaInterpolationFilterTwoD_SSSE3,                 //bd
            ChromaInterpolationFilterTwoD_SSSE3,                 //be
            ChromaInterpolationFilterTwoD_SSSE3,                 //bf
            ChromaInterpolationFilterTwoD_SSSE3,                 //bg
            ChromaInterpolationFilterTwoD_SSSE3,                 //bh
            ChromaInterpolationFilterOneDVertical_SSSE3,         //ca
            ChromaInterpolationFilterTwoD_SSSE3,                 //cb
            ChromaInterpolationFilterTwoD_SSSE3,                 //cc
            ChromaInterpolationFilterTwoD_SSSE3,                 //cd
            ChromaInterpolationFilterTwoD_SSSE3,                 //ce
            ChromaInterpolationFilterTwoD_SSSE3,                 //cf
            ChromaInterpolationFilterTwoD_SSSE3,                 //cg
            ChromaInterpolationFilterTwoD_SSSE3,                 //ch
            ChromaInterpolationFilterOneDVertical_SSSE3,         //da
            ChromaInterpolationFilterTwoD_SSSE3,                 //db
            ChromaInterpolationFilterTwoD_SSSE3,                 //dc
            ChromaInterpolationFilterTwoD_SSSE3,                 //dd
            ChromaInterpolationFilterTwoD_SSSE3,                 //de
            ChromaInterpolationFilterTwoD_SSSE3,                 //df
            ChromaInterpolationFilterTwoD_SSSE3,                 //dg
            ChromaInterpolationFilterTwoD_SSSE3,                 //dh
            ChromaInterpolationFilterOneDVertical_SSSE3,         //ea
            ChromaInterpolationFilterTwoD_SSSE3,                 //eb
            ChromaInterpolationFilterTwoD_SSSE3,                 //ec
            ChromaInterpolationFilterTwoD_SSSE3,                 //ed
            ChromaInterpolationFilterTwoD_SSSE3,                 //ee
            ChromaInterpolationFilterTwoD_SSSE3,                 //ef
            ChromaInterpolationFilterTwoD_SSSE3,                 //eg
            ChromaInterpolationFilterTwoD_SSSE3,                 //eh
            ChromaInterpolationFilterOneDVertical_SSSE3,         //fa
            ChromaInterpolationFilterTwoD_SSSE3,                 //fb
            ChromaInterpolationFilterTwoD_SSSE3,                 //fc
            ChromaInterpolationFilterTwoD_SSSE3,                 //fd
            ChromaInterpolationFilterTwoD_SSSE3,                 //fe
            ChromaInterpolationFilterTwoD_SSSE3,                 //ff
            ChromaInterpolationFilterTwoD_SSSE3,                 //fg
            ChromaInterpolationFilterTwoD_SSSE3,                 //fh
            ChromaInterpolationFilterOneDVertical_SSSE3,         //ga
            ChromaInterpolationFilterTwoD_SSSE3,                 //gb
            ChromaInterpolationFilterTwoD_SSSE3,                 //gc
            ChromaInterpolationFilterTwoD_SSSE3,                 //gd
            ChromaInterpolationFilterTwoD_SSSE3,                 //ge
            ChromaInterpolationFilterTwoD_SSSE3,                 //gf
            ChromaInterpolationFilterTwoD_SSSE3,                 //gg
            ChromaInterpolationFilterTwoD_SSSE3,                 //gh
            ChromaInterpolationFilterOneDVertical_SSSE3,         //ha
            ChromaInterpolationFilterTwoD_SSSE3,                 //hb
            ChromaInterpolationFilterTwoD_SSSE3,                 //hc
            ChromaInterpolationFilterTwoD_SSSE3,                 //hd
            ChromaInterpolationFilterTwoD_SSSE3,                 //he
            ChromaInterpolationFilterTwoD_SSSE3,                 //hf
            ChromaInterpolationFilterTwoD_SSSE3,                 //hg
            ChromaInterpolationFilterTwoD_SSSE3,                 //hh
        },
};


const InterpolationFilterChromaNew16bit uniPredChromaIFFunctionPtrArrayNew16bit[EB_ASM_TYPE_TOTAL][64] = {
        // C_DEFAULT
        {
            ChromaInterpolationCopy16bit,                           //B
            ChromaInterpolationFilterOneD16bit,                     //ab
            ChromaInterpolationFilterOneD16bit,                     //ac
            ChromaInterpolationFilterOneD16bit,                     //ad
            ChromaInterpolationFilterOneD16bit,                     //ae
            ChromaInterpolationFilterOneD16bit,                     //af
            ChromaInterpolationFilterOneD16bit,                     //ag
            ChromaInterpolationFilterOneD16bit,                     //ah
            ChromaInterpolationFilterOneD16bit,                     //ba
            ChromaInterpolationFilterTwoD16bit,                     //bb
            ChromaInterpolationFilterTwoD16bit,                     //bc
            ChromaInterpolationFilterTwoD16bit,                     //bd
            ChromaInterpolationFilterTwoD16bit,                     //be
            ChromaInterpolationFilterTwoD16bit,                     //bf
            ChromaInterpolationFilterTwoD16bit,                     //bg
            ChromaInterpolationFilterTwoD16bit,                     //bh
            ChromaInterpolationFilterOneD16bit,                     //ca
            ChromaInterpolationFilterTwoD16bit,                     //cb
            ChromaInterpolationFilterTwoD16bit,                     //cc
            ChromaInterpolationFilterTwoD16bit,                     //cd
            ChromaInterpolationFilterTwoD16bit,                     //ce
            ChromaInterpolationFilterTwoD16bit,                     //cf
            ChromaInterpolationFilterTwoD16bit,                     //cg
            ChromaInterpolationFilterTwoD16bit,                     //ch
            ChromaInterpolationFilterOneD16bit,                     //da
            ChromaInterpolationFilterTwoD16bit,                     //db
            ChromaInterpolationFilterTwoD16bit,                     //dc
            ChromaInterpolationFilterTwoD16bit,                     //dd
            ChromaInterpolationFilterTwoD16bit,                     //de
            ChromaInterpolationFilterTwoD16bit,                     //df
            ChromaInterpolationFilterTwoD16bit,                     //dg
            ChromaInterpolationFilterTwoD16bit,                     //dh
            ChromaInterpolationFilterOneD16bit,                     //ea
            ChromaInterpolationFilterTwoD16bit,                     //eb
            ChromaInterpolationFilterTwoD16bit,                     //ec
            ChromaInterpolationFilterTwoD16bit,                     //ed
            ChromaInterpolationFilterTwoD16bit,                     //ee
            ChromaInterpolationFilterTwoD16bit,                     //ef
            ChromaInterpolationFilterTwoD16bit,                     //eg
            ChromaInterpolationFilterTwoD16bit,                     //eh
            ChromaInterpolationFilterOneD16bit,                     //fa
            ChromaInterpolationFilterTwoD16bit,                     //fb
            ChromaInterpolationFilterTwoD16bit,                     //fc
            ChromaInterpolationFilterTwoD16bit,                     //fd
            ChromaInterpolationFilterTwoD16bit,                     //fe
            ChromaInterpolationFilterTwoD16bit,                     //ff
            ChromaInterpolationFilterTwoD16bit,                     //fg
            ChromaInterpolationFilterTwoD16bit,                     //fh
            ChromaInterpolationFilterOneD16bit,                     //ga
            ChromaInterpolationFilterTwoD16bit,                     //gb
            ChromaInterpolationFilterTwoD16bit,                     //gc
            ChromaInterpolationFilterTwoD16bit,                     //gd
            ChromaInterpolationFilterTwoD16bit,                     //ge
            ChromaInterpolationFilterTwoD16bit,                     //gf
            ChromaInterpolationFilterTwoD16bit,                     //gg
            ChromaInterpolationFilterTwoD16bit,                     //gh
            ChromaInterpolationFilterOneD16bit,                     //ha
            ChromaInterpolationFilterTwoD16bit,                     //hb
            ChromaInterpolationFilterTwoD16bit,                     //hc
            ChromaInterpolationFilterTwoD16bit,                     //hd
            ChromaInterpolationFilterTwoD16bit,                     //he
            ChromaInterpolationFilterTwoD16bit,                     //hf
            ChromaInterpolationFilterTwoD16bit,                     //hg
            ChromaInterpolationFilterTwoD16bit,                     //hh
        },
        // AVX2
        {
            ChromaInterpolationCopy16bit_SSE2,                          //B
            ChromaInterpolationFilterOneD16bitHorizontal_SSE2_INTRIN,   //ab
            ChromaInterpolationFilterOneD16bitHorizontal_SSE2_INTRIN,   //ac
            ChromaInterpolationFilterOneD16bitHorizontal_SSE2_INTRIN,   //ad
            ChromaInterpolationFilterOneD16bitHorizontal_SSE2_INTRIN,   //ae
            ChromaInterpolationFilterOneD16bitHorizontal_SSE2_INTRIN,   //af
            ChromaInterpolationFilterOneD16bitHorizontal_SSE2_INTRIN,   //ag
            ChromaInterpolationFilterOneD16bitHorizontal_SSE2_INTRIN,   //ah
            ChromaInterpolationFilterOneD16bitVertical_SSE2_INTRIN,     //ba
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //bb
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //bc
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //bd
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //be
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //bf
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //bg
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //bh
            ChromaInterpolationFilterOneD16bitVertical_SSE2_INTRIN,     //ca
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //cb
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //cc
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //cd
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //ce
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //cf
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //cg
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //ch
            ChromaInterpolationFilterOneD16bitVertical_SSE2_INTRIN,     //da
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //db
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //dc
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //dd
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //de
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //df
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //dg
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //dh
            ChromaInterpolationFilterOneD16bitVertical_SSE2_INTRIN,     //ea
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //eb
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //ec
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //ed
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //ee
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //ef
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //eg
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //eh
            ChromaInterpolationFilterOneD16bitVertical_SSE2_INTRIN,     //fa
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //fb
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //fc
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //fd
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //fe
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //ff
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //fg
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //fh
            ChromaInterpolationFilterOneD16bitVertical_SSE2_INTRIN,     //ga
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //gb
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //gc
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //gd
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //ge
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //gf
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //gg
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //gh
            ChromaInterpolationFilterOneD16bitVertical_SSE2_INTRIN,     //ha
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //hb
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //hc
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //hd
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //he
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //hf
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //hg
            ChromaInterpolationFilterTwoD16bit_AVX2_INTRIN,             //hh
        },
};

const ChromaFilterOutRaw biPredChromaIFFunctionPtrArrayNew[EB_ASM_TYPE_TOTAL][64] = {
        // C_DEFAULT
        {
            ChromaInterpolationCopyOutRaw,                      //B
            ChromaInterpolationFilterOneDOutRaw,                //ab
            ChromaInterpolationFilterOneDOutRaw,                //ac
            ChromaInterpolationFilterOneDOutRaw,                //ad
            ChromaInterpolationFilterOneDOutRaw,                //ae
            ChromaInterpolationFilterOneDOutRaw,                //af
            ChromaInterpolationFilterOneDOutRaw,                //ag
            ChromaInterpolationFilterOneDOutRaw,                //ah
            ChromaInterpolationFilterOneDOutRaw,                //ba
            ChromaInterpolationFilterTwoDOutRaw,                //bb
            ChromaInterpolationFilterTwoDOutRaw,                //bc
            ChromaInterpolationFilterTwoDOutRaw,                //bd
            ChromaInterpolationFilterTwoDOutRaw,                //be
            ChromaInterpolationFilterTwoDOutRaw,                //bf
            ChromaInterpolationFilterTwoDOutRaw,                //bg
            ChromaInterpolationFilterTwoDOutRaw,                //bh
            ChromaInterpolationFilterOneDOutRaw,                //ca
            ChromaInterpolationFilterTwoDOutRaw,                //cb
            ChromaInterpolationFilterTwoDOutRaw,                //cc
            ChromaInterpolationFilterTwoDOutRaw,                //cd
            ChromaInterpolationFilterTwoDOutRaw,                //ce
            ChromaInterpolationFilterTwoDOutRaw,                //cf
            ChromaInterpolationFilterTwoDOutRaw,                //cg
            ChromaInterpolationFilterTwoDOutRaw,                //ch
            ChromaInterpolationFilterOneDOutRaw,                //da
            ChromaInterpolationFilterTwoDOutRaw,                //db
            ChromaInterpolationFilterTwoDOutRaw,                //dc
            ChromaInterpolationFilterTwoDOutRaw,                //dd
            ChromaInterpolationFilterTwoDOutRaw,                //de
            ChromaInterpolationFilterTwoDOutRaw,                //df
            ChromaInterpolationFilterTwoDOutRaw,                //dg
            ChromaInterpolationFilterTwoDOutRaw,                //dh
            ChromaInterpolationFilterOneDOutRaw,                //ea
            ChromaInterpolationFilterTwoDOutRaw,                //eb
            ChromaInterpolationFilterTwoDOutRaw,                //ec
            ChromaInterpolationFilterTwoDOutRaw,                //ed
            ChromaInterpolationFilterTwoDOutRaw,                //ee
            ChromaInterpolationFilterTwoDOutRaw,                //ef
            ChromaInterpolationFilterTwoDOutRaw,                //eg
            ChromaInterpolationFilterTwoDOutRaw,                //eh
            ChromaInterpolationFilterOneDOutRaw,                //fa
            ChromaInterpolationFilterTwoDOutRaw,                //fb
            ChromaInterpolationFilterTwoDOutRaw,                //fc
            ChromaInterpolationFilterTwoDOutRaw,                //fd
            ChromaInterpolationFilterTwoDOutRaw,                //fe
            ChromaInterpolationFilterTwoDOutRaw,                //ff
            ChromaInterpolationFilterTwoDOutRaw,                //fg
            ChromaInterpolationFilterTwoDOutRaw,                //fh
            ChromaInterpolationFilterOneDOutRaw,                //ga
            ChromaInterpolationFilterTwoDOutRaw,                //gb
            ChromaInterpolationFilterTwoDOutRaw,                //gc
            ChromaInterpolationFilterTwoDOutRaw,                //gd
            ChromaInterpolationFilterTwoDOutRaw,                //ge
            ChromaInterpolationFilterTwoDOutRaw,                //gf
            ChromaInterpolationFilterTwoDOutRaw,                //gg
            ChromaInterpolationFilterTwoDOutRaw,                //gh
            ChromaInterpolationFilterOneDOutRaw,                //ha
            ChromaInterpolationFilterTwoDOutRaw,                //hb
            ChromaInterpolationFilterTwoDOutRaw,                //hc
            ChromaInterpolationFilterTwoDOutRaw,                //hd
            ChromaInterpolationFilterTwoDOutRaw,                //he
            ChromaInterpolationFilterTwoDOutRaw,                //hf
            ChromaInterpolationFilterTwoDOutRaw,                //hg
            ChromaInterpolationFilterTwoDOutRaw,                //hh
        },
        // AVX2
        {
            ChromaInterpolationCopyOutRaw_SSSE3,                 //B
            ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3, //ab
            ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3, //ac
            ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3, //ad
            ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3, //ae
            ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3, //af
            ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3, //ag
            ChromaInterpolationFilterOneDOutRawHorizontal_SSSE3, //ah
            ChromaInterpolationFilterOneDOutRawVertical_SSSE3,   //ba
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //bb
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,	          //bc
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,	          //bd
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,	          //be
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,	          //bf
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,	          //bg
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,	          //bh
            ChromaInterpolationFilterOneDOutRawVertical_SSSE3,	  //ca
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,			  //cb
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,			  //cc
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,			  //cd
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,			  //ce
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,			  //cf
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,			  //cg
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,			  //ch
            ChromaInterpolationFilterOneDOutRawVertical_SSSE3,    //da
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //db
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //dc
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //dd
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //de
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //df
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //dg
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //dh
            ChromaInterpolationFilterOneDOutRawVertical_SSSE3,    //ea
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //eb
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //ec
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //ed
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //ee
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //ef
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //eg
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //eh
            ChromaInterpolationFilterOneDOutRawVertical_SSSE3,    //fa
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //fb
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //fc
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //fd
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //fe
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //ff
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //fg
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //fh
            ChromaInterpolationFilterOneDOutRawVertical_SSSE3,    //ga
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //gb
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //gc
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //gd
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //ge
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //gf
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //gg
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //gh
            ChromaInterpolationFilterOneDOutRawVertical_SSSE3,    //ha
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //hb
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //hc
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //hd
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //he
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //hf
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //hg
            ChromaInterpolationFilterTwoDOutRaw_SSSE3,            //hh
        },
};

const ChromaFilterOutRaw16bit biPredChromaIFFunctionPtrArrayNew16bit[EB_ASM_TYPE_TOTAL][64] = {
        // C_DEFAULT
        {
            ChromaInterpolationCopyOutRaw16bit,                       //B
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ab
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ac
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ad
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ae
            ChromaInterpolationFilterOneDOutRaw16bit,                 //af
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ag
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ah
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ba
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //bb
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //bc
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //bd
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //be
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //bf
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //bg
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //bh
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ca
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //cb
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //cc
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //cd
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //ce
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //cf
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //cg
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //ch
            ChromaInterpolationFilterOneDOutRaw16bit,                 //da
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //db
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //dc
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //dd
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //de
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //df
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //dg
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //dh
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ea
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //eb
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //ec
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //ed
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //ee
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //ef
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //eg
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //eh
            ChromaInterpolationFilterOneDOutRaw16bit,                 //fa
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //fb
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //fc
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //fd
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //fe
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //ff
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //fg
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //fh
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ga
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //gb
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //gc
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //gd
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //ge
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //gf
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //gg
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //gh
            ChromaInterpolationFilterOneDOutRaw16bit,                 //ha
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //hb
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //hc
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //hd
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //he
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //hf
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //hg
            ChromaInterpolationFilterTwoDOutRaw16bit,                 //hh
        },
        // AVX2
        {
            ChromaInterpolationCopyOutRaw16bit_SSE2_INTRIN,                 //B
            ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN, //ab
            ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN, //ac
            ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN, //ad
            ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN, //ae
            ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN, //af
            ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN, //ag
            ChromaInterpolationFilterOneDOutRaw16bitHorizontal_AVX2_INTRIN, //ah
            ChromaInterpolationFilterOneDOutRaw16bitVertical_SSE2_INTRIN,   //ba
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //bb
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //bc
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //bd
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //be
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //bf
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //bg
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //bh
            ChromaInterpolationFilterOneDOutRaw16bitVertical_SSE2_INTRIN,   //ca
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //cb
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //cc
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //cd
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //ce
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //cf
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //cg
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //ch
            ChromaInterpolationFilterOneDOutRaw16bitVertical_SSE2_INTRIN,   //da
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //db
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //dc
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //dd
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //de
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //df
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //dg
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //dh
            ChromaInterpolationFilterOneDOutRaw16bitVertical_SSE2_INTRIN,   //ea
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //eb
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //ec
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //ed
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //ee
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //ef
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //eg
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //eh
            ChromaInterpolationFilterOneDOutRaw16bitVertical_SSE2_INTRIN,   //fa
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //fb
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //fc
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //fd
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //fe
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //ff
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //fg
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //fh
            ChromaInterpolationFilterOneDOutRaw16bitVertical_SSE2_INTRIN,   //ga
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //gb
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //gc
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //gd
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //ge
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //gf
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //gg
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //gh
            ChromaInterpolationFilterOneDOutRaw16bitVertical_SSE2_INTRIN,   //ha
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //hb
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //hc
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //hd
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //he
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //hf
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //hg
            ChromaInterpolationFilterTwoDOutRaw16bit_AVX2_INTRIN,           //hh

        },
};

