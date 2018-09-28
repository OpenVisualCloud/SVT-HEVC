/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2010-2014, ITU/ISO/IEC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "EbHmCode.h"
#include "EbUtility.h"

/*******************************************
* Compute4x4Satd
*   returns 4x4 Sum of Absolute Transformed Differences
*******************************************/
EB_U64 Compute4x4Satd(
	EB_S16 *diff)       // input parameter, diff samples Ptr
{
	EB_U64 satdBlock4x4 = 0;
	EB_S16 m[16], d[16];
	EB_U32 k;

	/*===== hadamard transform =====*/
	m[0] = diff[0] + diff[12];
	m[1] = diff[1] + diff[13];
	m[2] = diff[2] + diff[14];
	m[3] = diff[3] + diff[15];
	m[4] = diff[4] + diff[8];
	m[5] = diff[5] + diff[9];
	m[6] = diff[6] + diff[10];
	m[7] = diff[7] + diff[11];
	m[8] = diff[4] - diff[8];
	m[9] = diff[5] - diff[9];
	m[10] = diff[6] - diff[10];
	m[11] = diff[7] - diff[11];
	m[12] = diff[0] - diff[12];
	m[13] = diff[1] - diff[13];
	m[14] = diff[2] - diff[14];
	m[15] = diff[3] - diff[15];

	d[0] = m[0] + m[4];
	d[1] = m[1] + m[5];
	d[2] = m[2] + m[6];
	d[3] = m[3] + m[7];
	d[4] = m[8] + m[12];
	d[5] = m[9] + m[13];
	d[6] = m[10] + m[14];
	d[7] = m[11] + m[15];
	d[8] = m[0] - m[4];
	d[9] = m[1] - m[5];
	d[10] = m[2] - m[6];
	d[11] = m[3] - m[7];
	d[12] = m[12] - m[8];
	d[13] = m[13] - m[9];
	d[14] = m[14] - m[10];
	d[15] = m[15] - m[11];

	m[0] = d[0] + d[3];
	m[1] = d[1] + d[2];
	m[2] = d[1] - d[2];
	m[3] = d[0] - d[3];
	m[4] = d[4] + d[7];
	m[5] = d[5] + d[6];
	m[6] = d[5] - d[6];
	m[7] = d[4] - d[7];
	m[8] = d[8] + d[11];
	m[9] = d[9] + d[10];
	m[10] = d[9] - d[10];
	m[11] = d[8] - d[11];
	m[12] = d[12] + d[15];
	m[13] = d[13] + d[14];
	m[14] = d[13] - d[14];
	m[15] = d[12] - d[15];

	d[0] = m[0] + m[1];
	d[1] = m[0] - m[1];
	d[2] = m[2] + m[3];
	d[3] = m[3] - m[2];
	d[4] = m[4] + m[5];
	d[5] = m[4] - m[5];
	d[6] = m[6] + m[7];
	d[7] = m[7] - m[6];
	d[8] = m[8] + m[9];
	d[9] = m[8] - m[9];
	d[10] = m[10] + m[11];
	d[11] = m[11] - m[10];
	d[12] = m[12] + m[13];
	d[13] = m[12] - m[13];
	d[14] = m[14] + m[15];
	d[15] = m[15] - m[14];

	for (k = 0; k<16; ++k) {
		satdBlock4x4 += ABS(d[k]);
	}

	satdBlock4x4 = ((satdBlock4x4 + 1) >> 1);

	return satdBlock4x4;
}


EB_U64 Compute4x4Satd_U8(
	EB_U8 *src,       // input parameter, diff samples Ptr
	EB_U64 *dcValue,
	EB_U32  srcStride)
{
	EB_U64 satdBlock4x4 = 0;
	EB_S16 m[16], d[16];
	EB_U32 k;

	/*===== hadamard transform =====*/
	m[0] = src[0] + src[3 * srcStride];
	m[1] = src[1] + src[1 + (3 * srcStride)];
	m[2] = src[2] + src[2 + (3 * srcStride)];
	m[3] = src[3] + src[3 + (3 * srcStride)];
	m[4] = src[srcStride] + src[2 * srcStride];
	m[5] = src[1 + srcStride] + src[1 + (2 * srcStride)];
	m[6] = src[2 + srcStride] + src[2 + (2 * srcStride)];
	m[7] = src[3 + srcStride] + src[3 + (2 * srcStride)];
	m[8] = src[srcStride] - src[2 * srcStride];
	m[9] = src[1 + srcStride] - src[1 + (2 * srcStride)];
	m[10] = src[2 + srcStride] - src[2 + (2 * srcStride)];
	m[11] = src[3 + srcStride] - src[3 + (2 * srcStride)];
	m[12] = src[0] - src[3 * srcStride];
	m[13] = src[1] - src[1 + (3 * srcStride)];
	m[14] = src[2] - src[2 + (3 * srcStride)];
	m[15] = src[3] - src[3 + (3 * srcStride)];

	d[0] = m[0] + m[4];
	d[1] = m[1] + m[5];
	d[2] = m[2] + m[6];
	d[3] = m[3] + m[7];
	d[4] = m[8] + m[12];
	d[5] = m[9] + m[13];
	d[6] = m[10] + m[14];
	d[7] = m[11] + m[15];
	d[8] = m[0] - m[4];
	d[9] = m[1] - m[5];
	d[10] = m[2] - m[6];
	d[11] = m[3] - m[7];
	d[12] = m[12] - m[8];
	d[13] = m[13] - m[9];
	d[14] = m[14] - m[10];
	d[15] = m[15] - m[11];

	m[0] = d[0] + d[3];
	m[1] = d[1] + d[2];
	m[2] = d[1] - d[2];
	m[3] = d[0] - d[3];
	m[4] = d[4] + d[7];
	m[5] = d[5] + d[6];
	m[6] = d[5] - d[6];
	m[7] = d[4] - d[7];
	m[8] = d[8] + d[11];
	m[9] = d[9] + d[10];
	m[10] = d[9] - d[10];
	m[11] = d[8] - d[11];
	m[12] = d[12] + d[15];
	m[13] = d[13] + d[14];
	m[14] = d[13] - d[14];
	m[15] = d[12] - d[15];

	d[0] = m[0] + m[1];
	d[1] = m[0] - m[1];
	d[2] = m[2] + m[3];
	d[3] = m[3] - m[2];
	d[4] = m[4] + m[5];
	d[5] = m[4] - m[5];
	d[6] = m[6] + m[7];
	d[7] = m[7] - m[6];
	d[8] = m[8] + m[9];
	d[9] = m[8] - m[9];
	d[10] = m[10] + m[11];
	d[11] = m[11] - m[10];
	d[12] = m[12] + m[13];
	d[13] = m[12] - m[13];
	d[14] = m[14] + m[15];
	d[15] = m[15] - m[14];

	for (k = 0; k<16; ++k) {
		satdBlock4x4 += ABS(d[k]);
	}

	satdBlock4x4 = ((satdBlock4x4 + 1) >> 1);
	*dcValue += d[0];
	return satdBlock4x4;
}

//Table for updating the next state. MPS and LSP tables are combined. The higher bit would decide for MPS or LPS. 128 MPS and then 128 LPS
//8 bits unsigned would be enough
const EB_U32 NextStateMpsLps[] =
{
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
	34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
	50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
	66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
	82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97,
	98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
	114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 124, 125, 126, 127,
	1, 0, 0, 1, 2, 3, 4, 5, 4, 5, 8, 9, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 18, 19, 18, 19, 22, 23, 22, 23, 24, 25,
	26, 27, 26, 27, 30, 31, 30, 31, 32, 33, 32, 33, 36, 37, 36, 37,
	38, 39, 38, 39, 42, 43, 42, 43, 44, 45, 44, 45, 46, 47, 48, 49,
	48, 49, 50, 51, 52, 53, 52, 53, 54, 55, 54, 55, 56, 57, 58, 59,
	58, 59, 60, 61, 60, 61, 60, 61, 62, 63, 64, 65, 64, 65, 66, 67,
	66, 67, 66, 67, 68, 69, 68, 69, 70, 71, 70, 71, 70, 71, 72, 73,
	72, 73, 72, 73, 74, 75, 74, 75, 74, 75, 76, 77, 76, 77, 126, 127
};

const EB_U32 CabacEstimatedBits[] = 
{
	 0x07b23, 0x085f9, 0x074a0, 0x08cbc, 0x06ee4, 0x09354, 0x067f4, 0x09c1b, 0x060b0, 0x0a62a, 0x05a9c, 0x0af5b, 0x0548d, 0x0b955, 0x04f56, 0x0c2a9,
	 0x04a87, 0x0cbf7, 0x045d6, 0x0d5c3, 0x04144, 0x0e01b, 0x03d88, 0x0e937, 0x039e0, 0x0f2cd, 0x03663, 0x0fc9e, 0x03347, 0x10600, 0x03050, 0x10f95,
	 0x02d4d, 0x11a02, 0x02ad3, 0x12333, 0x0286e, 0x12cad, 0x02604, 0x136df, 0x02425, 0x13f48, 0x021f4, 0x149c4, 0x0203e, 0x1527b, 0x01e4d, 0x15d00,
	 0x01c99, 0x166de, 0x01b18, 0x17017, 0x019a5, 0x17988, 0x01841, 0x18327, 0x016df, 0x18d50, 0x015d9, 0x19547, 0x0147c, 0x1a083, 0x0138e, 0x1a8a3,
	 0x01251, 0x1b418, 0x01166, 0x1bd27, 0x01068, 0x1c77b, 0x00f7f, 0x1d18e, 0x00eda, 0x1d91a, 0x00e19, 0x1e254, 0x00d4f, 0x1ec9a, 0x00c90, 0x1f6e0,
	 0x00c01, 0x1fef8, 0x00b5f, 0x208b1, 0x00ab6, 0x21362, 0x00a15, 0x21e46, 0x00988, 0x2285d, 0x00934, 0x22ea8, 0x008a8, 0x239b2, 0x0081d, 0x24577,
	 0x007c9, 0x24ce6, 0x00763, 0x25663, 0x00710, 0x25e8f, 0x006a0, 0x26a26, 0x00672, 0x26f23, 0x005e8, 0x27ef8, 0x005ba, 0x284b5, 0x0055e, 0x29057,
	 0x0050c, 0x29bab, 0x004c1, 0x2a674, 0x004a7, 0x2aa5e, 0x0046f, 0x2b32f, 0x0041f, 0x2c0ad, 0x003e7, 0x2ca8d, 0x003ba, 0x2d323, 0x0010c, 0x3bfbb
 };
