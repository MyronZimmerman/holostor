/*  Copyright (C) 2003-2011 Thomas P. Scott and Myron Zimmerman

    Thomas P. Scott <tpscott@alum.mit.edu>
    Myron Zimmerman <MyronZimmerman@alum.mit.edu>

    This file is part of HoloStor.

    HoloStor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    HoloStor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with HoloStor.  If not, see <http://www.gnu.org/licenses/>.

    Parts of HoloStor are protected by US Patent 7,472,334, the use of
    which is granted in accordance to the terms of GPLv3.
*/
/*****************************************************************************

 Module Name:
	Session.cpp

 Abstract:
	Implementation of the Session class.
	
--****************************************************************************/

#include "Session.hpp"

#include <string.h>		// for ANSI memset()
#include <assert.h>		// for ANSI assert()

namespace HoloStor {

Session::Session()
{
	::memset(&m_config, 0, sizeof(m_config));
	m_uAllMask = 0;
}

int
Session::SessionInit(const HOLOSTOR_CFG *lpConfiguration)
{
	m_config = *lpConfiguration;
	//
	// Initialize the masks.
	unsigned i, count;
	m_uDataMask = 0;
	count = m_config.DataBlocks;
	for (i = 0; i < count; i++)
		m_uDataMask |= (1<<i);
	m_uEccMask = 0;
	count += m_config.EccBlocks;
	for (     ; i < count; i++)
		m_uEccMask |= (1<<i);
	m_uAllMask = m_uDataMask|m_uEccMask;
	//
	return m_codes.CodingTableInit(&m_config);
}

int
Session::Rebuild(
	UINT32 uInvalidBlockMask, UCHAR** lpBlockGroup, INT lWhichBlock) const
{
	const unsigned M = m_config.DataBlocks + m_config.EccBlocks;
	if (lWhichBlock >= (INT)M || uInvalidBlockMask > m_uAllMask)
		return HOLOSTOR_STATUS_INVALID_PARAMETER;
	UINT_PTR uMash = 0;
	for (unsigned i = 0; i < M; ++i)
		uMash |= (UINT_PTR)lpBlockGroup[i];
	if (uMash&0xF)
		return HOLOSTOR_STATUS_MISALIGNED_BUFFER;
	//
	if (uInvalidBlockMask == 0)			// XXX - shouldn't need to special case
		return HOLOSTOR_STATUS_SUCCESS;
	const CodingMatrix *cmPtr = m_codes.lookup(uInvalidBlockMask);
	if (cmPtr == NULL)
		return HOLOSTOR_STATUS_TOO_MANY_BAD_BLOCKS;
	cmPtr->Rebuild(lpBlockGroup, lWhichBlock, m_config.BlockSize);
	return HOLOSTOR_STATUS_SUCCESS;
}

int
Session::EncodeDelta(
	UINT lDeltaIndex, const UCHAR* lpDeltaBlock,
	UINT   lEccIndex, const UCHAR* lpEccBlockOld, UCHAR* lpEccBlockNew) const
{
	if (lDeltaIndex >= m_config.DataBlocks)
		return HOLOSTOR_STATUS_INVALID_PARAMETER;
	if ((UINT_PTR(lpDeltaBlock)|UINT_PTR(lpEccBlockOld)|UINT_PTR(lpEccBlockNew))&0xF)
		return HOLOSTOR_STATUS_MISALIGNED_BUFFER;
	const CodingMatrix *cmPtr = m_codes.lookup(1<<lEccIndex);
	if (cmPtr == NULL)
		return HOLOSTOR_STATUS_INVALID_PARAMETER;
	//
	cmPtr->EncodeDelta(lDeltaIndex,
					   lpDeltaBlock,
					   lpEccBlockOld,
					   lpEccBlockNew,
					   m_config.BlockSize);
	return HOLOSTOR_STATUS_SUCCESS;
}

int
Session::WriteDelta(const UCHAR* lpDataBlockOld,
					const UCHAR* lpDataBlockNew, UCHAR* lpDeltaBlock) const
{
	if ((UINT_PTR(lpDataBlockOld)|UINT_PTR(lpDataBlockNew)|UINT_PTR(lpDeltaBlock))&0xF)
		return HOLOSTOR_STATUS_MISALIGNED_BUFFER;
	//
	int count = m_config.BlockSize;

	switch (CpuType)
	{
	case CPU_SSE2:
#define USE_SSE2	// Do not use here because SSE2 is not faster then MMX
#ifdef USE_SSE2
#ifdef	_MSC_VER
		__asm {
			mov		ecx,count
			mov		ebx,lpDataBlockNew
			mov		edx,lpDataBlockOld

//#define CACHE_MANAGEMENT	// Hurts warm cache case, little effect on dirty cache case
#ifdef	CACHE_MANAGEMENT
			mov		eax,[ebx]			// Load TLB's by touching first and
			mov		eax,[ebx+ecx-4]		// last words of the source buffers
			mov		eax,[edx]
			mov		eax,[edx+ecx-4]
			xor		eax,eax
		L00:
			prefetchnta	[ebx+eax]		// Prefetch source data into cache
			prefetchnta	[edx+eax]
			add		eax,64				// Cache Line size is 64 on SSE2 systems
			cmp		eax,ecx
			jl		L00
#endif
			shr		ecx,6				// Divide count by 64 to get loops
			mov		eax,lpDeltaBlock
		L01:							// Loop XOR-ing in 64-byte chunks
			movdqa	xmm0,[ebx]
			movdqa	xmm1,[ebx+16]
			movdqa	xmm2,[ebx+32]
			movdqa	xmm3,[ebx+48]
			movdqa	xmm4,[edx]
			movdqa	xmm5,[edx+16]
			movdqa	xmm6,[edx+32]
			movdqa	xmm7,[edx+48]
			pxor	xmm0,xmm4
			pxor	xmm1,xmm5
			pxor	xmm2,xmm6
			pxor	xmm3,xmm7
#ifdef CACHE_MANAGEMENT
			movntdq	[eax],xmm0
			movntdq	[eax+16],xmm1
			movntdq	[eax+32],xmm2
			movntdq	[eax+48],xmm3
#else
			movdqa	[eax],xmm0
			movdqa	[eax+16],xmm1
			movdqa	[eax+32],xmm2
			movdqa	[eax+48],xmm3
#endif
			add		eax,64
			add		ebx,64
			add		edx,64
			loop	L01
#ifdef CACHE_MANAGEMENT
			sfence						// Flush WC buffer
#endif
		}
		break;
#else	// !_MSC_VER	(GCC)
		count >>= 6;	// Divide count by 64 to get loops
		do {			// Loop XOR-ing in 64-byte chunks
			asm volatile("movdqa	%0,%%xmm0" : : "m" (lpDataBlockNew[ 0]));
			asm volatile("movdqa	%0,%%xmm1" : : "m" (lpDataBlockNew[16]));
			asm volatile("movdqa	%0,%%xmm2" : : "m" (lpDataBlockNew[32]));
			asm volatile("movdqa	%0,%%xmm3" : : "m" (lpDataBlockNew[48]));
			asm volatile("movdqa	%0,%%xmm4" : : "m" (lpDataBlockOld[ 0]));
			asm volatile("movdqa	%0,%%xmm5" : : "m" (lpDataBlockOld[16]));
			asm volatile("movdqa	%0,%%xmm6" : : "m" (lpDataBlockOld[32]));
			asm volatile("movdqa	%0,%%xmm7" : : "m" (lpDataBlockOld[48]));
			asm volatile("pxor	%xmm4,%xmm0");
			asm volatile("pxor	%xmm5,%xmm1");
			asm volatile("pxor	%xmm6,%xmm2");
			asm volatile("pxor	%xmm7,%xmm3");
			asm volatile("movdqa	%%xmm0,%0" : : "m" (lpDeltaBlock[ 0]));
			asm volatile("movdqa	%%xmm1,%0" : : "m" (lpDeltaBlock[16]));
			asm volatile("movdqa	%%xmm2,%0" : : "m" (lpDeltaBlock[32]));
			asm volatile("movdqa	%%xmm3,%0" : : "m" (lpDeltaBlock[48]));
			lpDataBlockNew += 64;
			lpDataBlockOld += 64;
			lpDeltaBlock += 64;
		} while (--count);
#endif	// _MSC_VER
		break;
#endif	// USE_SSE2

	case CPU_MMX:
#ifdef	_MSC_VER
		__asm {
			mov		ecx,count
			mov		ebx,lpDataBlockNew
			mov		edx,lpDataBlockOld
			shr		ecx,5				// Divide count by 32 to get loops
			mov		eax,lpDeltaBlock
		L10:							// Loop XOR-ing in 32-byte chunks
			movq	mm0,[ebx]
			movq	mm1,[edx]
			pxor	mm0,mm1
			movq	mm1,[ebx+8]
			movq	mm2,[edx+8]
			pxor	mm1,mm2
			movq	mm2,[ebx+16]
			movq	mm3,[edx+16]
			pxor	mm2,mm3
			movq	mm3,[ebx+24]
			movq	mm4,[edx+24]
			pxor	mm3,mm4
			movq	[eax],mm0
			movq	[eax+8],mm1
			movq	[eax+16],mm2
			movq	[eax+24],mm3
			add		eax,32
			add		ebx,32
			add		edx,32
			loop	L10
			emms						// Reset MMX
		}
#else	// !_MSC_VER (GCC)
		count >>= 5;	// Divide count by 32 to get loops
		do {		// Loop XOR-ing in 32-byte chunks
			asm volatile("movq	%0,%%mm0" : : "m" (lpDataBlockNew[ 0]));
			asm volatile("movq	%0,%%mm1" : : "m" (lpDataBlockNew[ 8]));
			asm volatile("movq	%0,%%mm2" : : "m" (lpDataBlockNew[16]));
			asm volatile("movq	%0,%%mm3" : : "m" (lpDataBlockNew[24]));
			asm volatile("movq	%0,%%mm4" : : "m" (lpDataBlockOld[ 0]));
			asm volatile("movq	%0,%%mm5" : : "m" (lpDataBlockOld[ 8]));
			asm volatile("movq	%0,%%mm6" : : "m" (lpDataBlockOld[16]));
			asm volatile("movq	%0,%%mm7" : : "m" (lpDataBlockOld[24]));
			asm volatile("pxor	%mm4,%mm0");
			asm volatile("pxor	%mm5,%mm1");
			asm volatile("pxor	%mm6,%mm2");
			asm volatile("pxor	%mm7,%mm3");
			asm volatile("movq	%%mm0,%0" : : "m" (lpDeltaBlock[ 0]));
			asm volatile("movq	%%mm1,%0" : : "m" (lpDeltaBlock[ 8]));
			asm volatile("movq	%%mm2,%0" : : "m" (lpDeltaBlock[16]));
			asm volatile("movq	%%mm3,%0" : : "m" (lpDeltaBlock[24]));
			lpDataBlockNew += 32;
			lpDataBlockOld += 32;
			lpDeltaBlock += 32;
		} while (--count);
		asm volatile("emms");	// Reset MMX
#endif	// _MSC_VER
		break;

	case CPU_STD:
	default:
		ULONG*	dst = (ULONG*) lpDeltaBlock;
		ULONG* src1 = (ULONG*) lpDataBlockNew;
		ULONG* src2 = (ULONG*) lpDataBlockOld;

		for ( ; count>0; count -= sizeof(ULONG))
			*dst++ = *src1++ ^ *src2++;
	}
	return HOLOSTOR_STATUS_SUCCESS;
}

} // namespace HoloStor
