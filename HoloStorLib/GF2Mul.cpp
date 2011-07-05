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
	GF2Mul.cpp

 Abstract:
	Implementation of class GF2Mul.
	
	This class implements multiply (& add) operations in a GF(2**Width) 
	extension field with operations in GF(2) (i.e. XOR and AND).  The advantage 
	of this approach is speed.  The bit operations can be simultaneously 
	performed on all the bits of hyperword_t.

--****************************************************************************/

#include "GF2Mul.hpp"

#ifdef _DEBUG
#include <iostream>
#endif
#include <assert.h>		// for ANSI assert()

#if ELEMENT_WIDTH != 4
#error Invalid ELEMENT_WIDTH constant (must be 4)
#endif

namespace HoloStor {

// Create the matrix T over GF(2) that corresponds to the matrix M over GF(2**Width).
void
GF2Mul::_init(const gfQ& x)
{
	m_index = x.regular();
}

// Return the Width x Width matrix over GF(2) that represents GF(2**Width)
// multiplication by x.
matrix<GF2Mul::gf2>
GF2Mul::multOp(gfQ x)
{
	matrix<gf2> a(ELEMENT_WIDTH,ELEMENT_WIDTH);
	if ( a.isNil() )
		return a;								// out of memory - return nil
	for (int j = 0; j < ELEMENT_WIDTH; j++) {
		unsigned u = x.regular();
		for (int i = 0; i < ELEMENT_WIDTH; i++, u>>=1)
			a(i,j) = u&1;
		x *= gfQ(2);
	}
	return a;
}

// Multiply the Element(s) located at pSrc by the scalar associated with
// this object and add the value to the Element(s) located at pDst. Each
// Element consists of hyperword_t[ELEMENT_WIDTH].
//
void
SSE2_multadd(hyperword_t *pDst, const hyperword_t *pSrc, unsigned nElements, unsigned nIndex);
void
 MMX_multadd(hyperword_t *pDst, const hyperword_t *pSrc, unsigned nElements, unsigned nIndex);
void
 STD_multadd(hyperword_t *pDst, const hyperword_t *pSrc, unsigned nElements, unsigned nIndex);

void
GF2Mul::gf2multadd(hyperword_t *pDst, const hyperword_t *pSrc, unsigned nElements) const
{
	switch (CpuType) {
#if HYPERWORD_SIZE == 4
	case CPU_SSE2:
		SSE2_multadd( pDst, pSrc, nElements, m_index);
		break;
	case CPU_MMX:
		 MMX_multadd( pDst, pSrc, nElements, m_index);
		break;
#endif
	case CPU_STD:
	default:
		 STD_multadd( pDst, pSrc, nElements, m_index);
		break;
	}
}

void
SSE2_multadd(hyperword_t *pDst, const hyperword_t *pSrc, unsigned nElements, unsigned nIndex)
{
#if _MSC_VER
#if _MSC_VER < 1300		// MSFT VC before .NET do not support SSE2 instructions.
	MMX_multadd( pDst, pSrc, nElements, nIndex);	// so pass this case on to MMX
#else
#undef ASM_PROLOGUE
#undef ASM_EPILOGUE
#define	ASM_PROLOGUE(label)			\
	__asm	mov		eax,pDst		\
	__asm	mov		edx,pSrc		\
	__asm	mov		ecx,nElements	\
	__asm label:					\
	__asm	movdqa	xmm0,[eax+0] 	\
	__asm	movdqa	xmm1,[eax+16]	\
	__asm	movdqa	xmm2,[eax+32]	\
	__asm	movdqa	xmm3,[eax+48]	\
	__asm	movdqa	xmm4,[edx+0] 	\
	__asm	movdqa	xmm5,[edx+16]	\
	__asm	movdqa	xmm6,[edx+32]	\
	__asm	movdqa	xmm7,[edx+48]

#define	ASM_EPILOGUE(label)			\
	__asm	movdqa	[eax+0],xmm0	\
	__asm	movdqa	[eax+16],xmm1	\
	__asm	movdqa	[eax+32],xmm2	\
	__asm	movdqa	[eax+48],xmm3	\
	__asm	add		eax,64			\
	__asm	add		edx,64			\
	__asm	loop	label

	switch (nIndex) {
	case 0:
		return;
	case 1:
		ASM_PROLOGUE(L1)
		__asm	pxor	xmm0,xmm4	// 0 ^ 0
		__asm	pxor	xmm1,xmm5	// 1 ^ 1
		__asm	pxor	xmm2,xmm6	// 2 ^ 2
		__asm	pxor	xmm3,xmm7	// 3 ^ 3
		ASM_EPILOGUE(L1)
		break;
	case 2:
		ASM_PROLOGUE(L2)
		__asm	pxor	xmm0,xmm7	// 0 ^ 3
		__asm	pxor	xmm1,xmm4	// 1 ^ 0
		__asm	pxor	xmm1,xmm7	// 1 ^ 3
		__asm	pxor	xmm2,xmm5	// 2 ^ 1
		__asm	pxor	xmm3,xmm6	// 3 ^ 2
		ASM_EPILOGUE(L2)
		break;
	case 3:
		ASM_PROLOGUE(L3)
		__asm	pxor	xmm0,xmm4	// 0 ^ 0
		__asm	pxor	xmm0,xmm7	// 0 ^ 3
		__asm	pxor	xmm1,xmm4	// 1 ^ 0
		__asm	pxor	xmm1,xmm5	// 1 ^ 1
		__asm	pxor	xmm1,xmm7	// 1 ^ 3
		__asm	pxor	xmm2,xmm5	// 2 ^ 1
		__asm	pxor	xmm2,xmm6	// 2 ^ 2
		__asm	pxor	xmm3,xmm6	// 3 ^ 2
		__asm	pxor	xmm3,xmm7	// 3 ^ 3
		ASM_EPILOGUE(L3)
		break;
	case 4:
		ASM_PROLOGUE(L4)
		__asm	pxor	xmm0,xmm6	// 0 ^ 2
		__asm	pxor	xmm1,xmm6	// 1 ^ 2
		__asm	pxor	xmm1,xmm7	// 1 ^ 3
		__asm	pxor	xmm2,xmm4	// 2 ^ 0
		__asm	pxor	xmm2,xmm7	// 2 ^ 3
		__asm	pxor	xmm3,xmm5	// 3 ^ 1
		ASM_EPILOGUE(L4)
		break;
	case 5:
		ASM_PROLOGUE(L5)
		__asm	pxor	xmm0,xmm4	// 0 ^ 0
		__asm	pxor	xmm0,xmm6	// 0 ^ 2
		__asm	pxor	xmm1,xmm5	// 1 ^ 1
		__asm	pxor	xmm1,xmm6	// 1 ^ 2
		__asm	pxor	xmm1,xmm7	// 1 ^ 3
		__asm	pxor	xmm2,xmm4	// 2 ^ 0
		__asm	pxor	xmm2,xmm6	// 2 ^ 2
		__asm	pxor	xmm2,xmm7	// 2 ^ 3
		__asm	pxor	xmm3,xmm5	// 3 ^ 1
		__asm	pxor	xmm3,xmm7	// 3 ^ 3
		ASM_EPILOGUE(L5)
		break;
	case 6:
		ASM_PROLOGUE(L6)
		__asm	pxor	xmm0,xmm6	// 0 ^ 2
		__asm	pxor	xmm0,xmm7	// 0 ^ 3
		__asm	pxor	xmm1,xmm4	// 1 ^ 0
		__asm	pxor	xmm1,xmm6	// 1 ^ 2
		__asm	pxor	xmm2,xmm4	// 2 ^ 0
		__asm	pxor	xmm2,xmm5	// 2 ^ 1
		__asm	pxor	xmm2,xmm7	// 2 ^ 3
		__asm	pxor	xmm3,xmm5	// 3 ^ 1
		__asm	pxor	xmm3,xmm6	// 3 ^ 2
		ASM_EPILOGUE(L6)
		break;
	case 7:
		ASM_PROLOGUE(L7)
		__asm	pxor	xmm0,xmm4	// 0 ^ 0
		__asm	pxor	xmm0,xmm6	// 0 ^ 2
		__asm	pxor	xmm0,xmm7	// 0 ^ 3
		__asm	pxor	xmm1,xmm4	// 1 ^ 0
		__asm	pxor	xmm1,xmm5	// 1 ^ 1
		__asm	pxor	xmm1,xmm6	// 1 ^ 2
		__asm	pxor	xmm2,xmm4	// 2 ^ 0
		__asm	pxor	xmm2,xmm5	// 2 ^ 1
		__asm	pxor	xmm2,xmm6	// 2 ^ 2
		__asm	pxor	xmm2,xmm7	// 2 ^ 3
		__asm	pxor	xmm3,xmm5	// 3 ^ 1
		__asm	pxor	xmm3,xmm6	// 3 ^ 2
		__asm	pxor	xmm3,xmm7	// 3 ^ 3
		ASM_EPILOGUE(L7)
		break;
	case 8:
		ASM_PROLOGUE(L8)
		__asm	pxor	xmm0,xmm5	// 0 ^ 1
		__asm	pxor	xmm1,xmm5	// 1 ^ 1
		__asm	pxor	xmm1,xmm6	// 1 ^ 2
		__asm	pxor	xmm2,xmm6	// 2 ^ 2
		__asm	pxor	xmm2,xmm7	// 2 ^ 3
		__asm	pxor	xmm3,xmm4	// 3 ^ 0
		__asm	pxor	xmm3,xmm7	// 3 ^ 3
		ASM_EPILOGUE(L8)
		break;
	case 9:
		ASM_PROLOGUE(L9)
		__asm	pxor	xmm0,xmm4	// 0 ^ 0
		__asm	pxor	xmm0,xmm5	// 0 ^ 1
		__asm	pxor	xmm1,xmm6	// 1 ^ 2
		__asm	pxor	xmm2,xmm7	// 2 ^ 3
		__asm	pxor	xmm3,xmm4	// 3 ^ 0
		ASM_EPILOGUE(L9)
		break;
	case 10:
		ASM_PROLOGUE(L10)
		__asm	pxor	xmm0,xmm5	// 0 ^ 1
		__asm	pxor	xmm0,xmm7	// 0 ^ 3
		__asm	pxor	xmm1,xmm4	// 1 ^ 0
		__asm	pxor	xmm1,xmm5	// 1 ^ 1
		__asm	pxor	xmm1,xmm6	// 1 ^ 2
		__asm	pxor	xmm1,xmm7	// 1 ^ 3
		__asm	pxor	xmm2,xmm5	// 2 ^ 1
		__asm	pxor	xmm2,xmm6	// 2 ^ 2
		__asm	pxor	xmm2,xmm7	// 2 ^ 3
		__asm	pxor	xmm3,xmm4	// 3 ^ 0
		__asm	pxor	xmm3,xmm6	// 3 ^ 2
		__asm	pxor	xmm3,xmm7	// 3 ^ 3
		ASM_EPILOGUE(L10)
		break;
	case 11:
		ASM_PROLOGUE(L11)
		__asm	pxor	xmm0,xmm4	// 0 ^ 0
		__asm	pxor	xmm0,xmm5	// 0 ^ 1
		__asm	pxor	xmm0,xmm7	// 0 ^ 3
		__asm	pxor	xmm1,xmm4	// 1 ^ 0
		__asm	pxor	xmm1,xmm6	// 1 ^ 2
		__asm	pxor	xmm1,xmm7	// 1 ^ 3
		__asm	pxor	xmm2,xmm5	// 2 ^ 1
		__asm	pxor	xmm2,xmm7	// 2 ^ 3
		__asm	pxor	xmm3,xmm4	// 3 ^ 0
		__asm	pxor	xmm3,xmm6	// 3 ^ 2
		ASM_EPILOGUE(L11)
		break;
	case 12:
		ASM_PROLOGUE(L12)
		__asm	pxor	xmm0,xmm5	// 0 ^ 1
		__asm	pxor	xmm0,xmm6	// 0 ^ 2
		__asm	pxor	xmm1,xmm5	// 1 ^ 1
		__asm	pxor	xmm1,xmm7	// 1 ^ 3
		__asm	pxor	xmm2,xmm4	// 2 ^ 0
		__asm	pxor	xmm2,xmm6	// 2 ^ 2
		__asm	pxor	xmm3,xmm4	// 3 ^ 0
		__asm	pxor	xmm3,xmm5	// 3 ^ 1
		__asm	pxor	xmm3,xmm7	// 3 ^ 3
		ASM_EPILOGUE(L12)
		break;
	case 13:
		ASM_PROLOGUE(L13)
		__asm	pxor	xmm0,xmm4	// 0 ^ 0
		__asm	pxor	xmm0,xmm5	// 0 ^ 1
		__asm	pxor	xmm0,xmm6	// 0 ^ 2
		__asm	pxor	xmm1,xmm7	// 1 ^ 3
		__asm	pxor	xmm2,xmm4	// 2 ^ 0
		__asm	pxor	xmm3,xmm4	// 3 ^ 0
		__asm	pxor	xmm3,xmm5	// 3 ^ 1
		ASM_EPILOGUE(L13)
		break;
	case 14:
		ASM_PROLOGUE(L14)
		__asm	pxor	xmm0,xmm5	// 0 ^ 1
		__asm	pxor	xmm0,xmm6	// 0 ^ 2
		__asm	pxor	xmm0,xmm7	// 0 ^ 3
		__asm	pxor	xmm1,xmm4	// 1 ^ 0
		__asm	pxor	xmm1,xmm5	// 1 ^ 1
		__asm	pxor	xmm2,xmm4	// 2 ^ 0
		__asm	pxor	xmm2,xmm5	// 2 ^ 1
		__asm	pxor	xmm2,xmm6	// 2 ^ 2
		__asm	pxor	xmm3,xmm4	// 3 ^ 0
		__asm	pxor	xmm3,xmm5	// 3 ^ 1
		__asm	pxor	xmm3,xmm6	// 3 ^ 2
		__asm	pxor	xmm3,xmm7	// 3 ^ 3
		ASM_EPILOGUE(L14)
		break;
	case 15:
		ASM_PROLOGUE(L15)
		__asm	pxor	xmm0,xmm4	// 0 ^ 0
		__asm	pxor	xmm0,xmm5	// 0 ^ 1
		__asm	pxor	xmm0,xmm6	// 0 ^ 2
		__asm	pxor	xmm0,xmm7	// 0 ^ 3
		__asm	pxor	xmm1,xmm4	// 1 ^ 0
		__asm	pxor	xmm2,xmm4	// 2 ^ 0
		__asm	pxor	xmm2,xmm5	// 2 ^ 1
		__asm	pxor	xmm3,xmm4	// 3 ^ 0
		__asm	pxor	xmm3,xmm5	// 3 ^ 1
		__asm	pxor	xmm3,xmm6	// 3 ^ 2
		ASM_EPILOGUE(L15)
		break;
	}
#endif	// _MSC_VER < 1300
#else	// !_MSC_VER (GCC)
#undef ASM_PROLOGUE
#undef ASM_EPILOGUE
#ifdef __x86_64__
# define EAX		"%%rax"
# define EDX		"%%rdx"
# define LOAD_REGS			\
	"movq	%0,%%rax\n\t"		\
	"movq	%1,%%rdx\n\t"		\
	"movl	%2,%%ecx\n\t"
# define BUMP_REGS(value)		\
	"addq	" value ",%%rax\n\t"	\
	"addq	" value ",%%rdx\n\t"
#else
# define EAX		"%%eax"
# define EDX		"%%edx"
# define LOAD_REGS			\
	"movl	%0,%%eax\n\t"		\
	"movl	%1,%%edx\n\t"		\
	"movl	%2,%%ecx\n\t"
# define BUMP_REGS(value)		\
	"addl	" value ",%%eax\n\t"	\
	"addl	" value ",%%edx\n\t"
#endif
#define	ASM_PROLOGUE(label)	__asm__(	\
	LOAD_REGS				\
	"0:\n\t"				\
	"movdqa	  (" EAX "),%%xmm0\n\t"		\
	"movdqa	16(" EAX "),%%xmm1\n\t"		\
	"movdqa	32(" EAX "),%%xmm2\n\t"		\
	"movdqa	48(" EAX "),%%xmm3\n\t"		\
	"movdqa	  (" EDX "),%%xmm4\n\t"		\
	"movdqa	16(" EDX "),%%xmm5\n\t"		\
	"movdqa	32(" EDX "),%%xmm6\n\t"		\
	"movdqa	48(" EDX "),%%xmm7\n\t"

#define	ASM_EPILOGUE(label)			\
	"movdqa	%%xmm0,  (" EAX ")\n\t"		\
	"movdqa	%%xmm1,16(" EAX ")\n\t"		\
	"movdqa	%%xmm2,32(" EAX ")\n\t"		\
	"movdqa	%%xmm3,48(" EAX ")\n\t"		\
	BUMP_REGS("$64")			\
	"loop	0b"				\
	: : "m" (pDst), "m" (pSrc), "m" (nElements) );

	switch (nIndex) {
	case 0:
		return;
	case 1:
		ASM_PROLOGUE(L1)
		"pxor	%%xmm4,%%xmm0\n\t"
		"pxor	%%xmm5,%%xmm1\n\t"
		"pxor	%%xmm6,%%xmm2\n\t"
		"pxor	%%xmm7,%%xmm3\n\t"
		ASM_EPILOGUE(L1)
		break;
	case 2:
		ASM_PROLOGUE(L2)
		"pxor	%%xmm7,%%xmm0\n\t"
		"pxor	%%xmm4,%%xmm1\n\t"
		"pxor	%%xmm7,%%xmm1\n\t"
		"pxor	%%xmm5,%%xmm2\n\t"
		"pxor	%%xmm6,%%xmm3\n\t"
		ASM_EPILOGUE(L2)
		break;
	case 3:
		ASM_PROLOGUE(L3)
		"pxor	%%xmm4,%%xmm0\n\t"
		"pxor	%%xmm7,%%xmm0\n\t"
		"pxor	%%xmm4,%%xmm1\n\t"
		"pxor	%%xmm5,%%xmm1\n\t"
		"pxor	%%xmm7,%%xmm1\n\t"
		"pxor	%%xmm5,%%xmm2\n\t"
		"pxor	%%xmm6,%%xmm2\n\t"
		"pxor	%%xmm6,%%xmm3\n\t"
		"pxor	%%xmm7,%%xmm3\n\t"
		ASM_EPILOGUE(L3)
		break;
	case 4:
		ASM_PROLOGUE(L4)
		"pxor	%%xmm6,%%xmm0\n\t"
		"pxor	%%xmm6,%%xmm1\n\t"
		"pxor	%%xmm7,%%xmm1\n\t"
		"pxor	%%xmm4,%%xmm2\n\t"
		"pxor	%%xmm7,%%xmm2\n\t"
		"pxor	%%xmm5,%%xmm3\n\t"
		ASM_EPILOGUE(L4)
		break;
	case 5:
		ASM_PROLOGUE(L5)
		"pxor	%%xmm4,%%xmm0\n\t"
		"pxor	%%xmm6,%%xmm0\n\t"
		"pxor	%%xmm5,%%xmm1\n\t"
		"pxor	%%xmm6,%%xmm1\n\t"
		"pxor	%%xmm7,%%xmm1\n\t"
		"pxor	%%xmm4,%%xmm2\n\t"
		"pxor	%%xmm6,%%xmm2\n\t"
		"pxor	%%xmm7,%%xmm2\n\t"
		"pxor	%%xmm5,%%xmm3\n\t"
		"pxor	%%xmm7,%%xmm3\n\t"
		ASM_EPILOGUE(L5)
		break;
	case 6:
		ASM_PROLOGUE(L6)
		"pxor	%%xmm6,%%xmm0\n\t"
		"pxor	%%xmm7,%%xmm0\n\t"
		"pxor	%%xmm4,%%xmm1\n\t"
		"pxor	%%xmm6,%%xmm1\n\t"
		"pxor	%%xmm4,%%xmm2\n\t"
		"pxor	%%xmm5,%%xmm2\n\t"
		"pxor	%%xmm7,%%xmm2\n\t"
		"pxor	%%xmm5,%%xmm3\n\t"
		"pxor	%%xmm6,%%xmm3\n\t"
		ASM_EPILOGUE(L6)
		break;
	case 7:
		ASM_PROLOGUE(L7)
		"pxor	%%xmm4,%%xmm0\n\t"
		"pxor	%%xmm6,%%xmm0\n\t"
		"pxor	%%xmm7,%%xmm0\n\t"
		"pxor	%%xmm4,%%xmm1\n\t"
		"pxor	%%xmm5,%%xmm1\n\t"
		"pxor	%%xmm6,%%xmm1\n\t"
		"pxor	%%xmm4,%%xmm2\n\t"
		"pxor	%%xmm5,%%xmm2\n\t"
		"pxor	%%xmm6,%%xmm2\n\t"
		"pxor	%%xmm7,%%xmm2\n\t"
		"pxor	%%xmm5,%%xmm3\n\t"
		"pxor	%%xmm6,%%xmm3\n\t"
		"pxor	%%xmm7,%%xmm3\n\t"
		ASM_EPILOGUE(L7)
		break;
	case 8:
		ASM_PROLOGUE(L8)
		"pxor	%%xmm5,%%xmm0\n\t"
		"pxor	%%xmm5,%%xmm1\n\t"
		"pxor	%%xmm6,%%xmm1\n\t"
		"pxor	%%xmm6,%%xmm2\n\t"
		"pxor	%%xmm7,%%xmm2\n\t"
		"pxor	%%xmm4,%%xmm3\n\t"
		"pxor	%%xmm7,%%xmm3\n\t"
		ASM_EPILOGUE(L8)
		break;
	case 9:
		ASM_PROLOGUE(L9)
		"pxor	%%xmm4,%%xmm0\n\t"
		"pxor	%%xmm5,%%xmm0\n\t"
		"pxor	%%xmm6,%%xmm1\n\t"
		"pxor	%%xmm7,%%xmm2\n\t"
		"pxor	%%xmm4,%%xmm3\n\t"
		ASM_EPILOGUE(L9)
		break;
	case 10:
		ASM_PROLOGUE(L10)
		"pxor	%%xmm5,%%xmm0\n\t"
		"pxor	%%xmm7,%%xmm0\n\t"
		"pxor	%%xmm4,%%xmm1\n\t"
		"pxor	%%xmm5,%%xmm1\n\t"
		"pxor	%%xmm6,%%xmm1\n\t"
		"pxor	%%xmm7,%%xmm1\n\t"
		"pxor	%%xmm5,%%xmm2\n\t"
		"pxor	%%xmm6,%%xmm2\n\t"
		"pxor	%%xmm7,%%xmm2\n\t"
		"pxor	%%xmm4,%%xmm3\n\t"
		"pxor	%%xmm6,%%xmm3\n\t"
		"pxor	%%xmm7,%%xmm3\n\t"
		ASM_EPILOGUE(L10)
		break;
	case 11:
		ASM_PROLOGUE(L11)
		"pxor	%%xmm4,%%xmm0\n\t"
		"pxor	%%xmm5,%%xmm0\n\t"
		"pxor	%%xmm7,%%xmm0\n\t"
		"pxor	%%xmm4,%%xmm1\n\t"
		"pxor	%%xmm6,%%xmm1\n\t"
		"pxor	%%xmm7,%%xmm1\n\t"
		"pxor	%%xmm5,%%xmm2\n\t"
		"pxor	%%xmm7,%%xmm2\n\t"
		"pxor	%%xmm4,%%xmm3\n\t"
		"pxor	%%xmm6,%%xmm3\n\t"
		ASM_EPILOGUE(L11)
		break;
	case 12:
		ASM_PROLOGUE(L12)
		"pxor	%%xmm5,%%xmm0\n\t"
		"pxor	%%xmm6,%%xmm0\n\t"
		"pxor	%%xmm5,%%xmm1\n\t"
		"pxor	%%xmm7,%%xmm1\n\t"
		"pxor	%%xmm4,%%xmm2\n\t"
		"pxor	%%xmm6,%%xmm2\n\t"
		"pxor	%%xmm4,%%xmm3\n\t"
		"pxor	%%xmm5,%%xmm3\n\t"
		"pxor	%%xmm7,%%xmm3\n\t"
		ASM_EPILOGUE(L12)
		break;
	case 13:
		ASM_PROLOGUE(L13)
		"pxor	%%xmm4,%%xmm0\n\t"
		"pxor	%%xmm5,%%xmm0\n\t"
		"pxor	%%xmm6,%%xmm0\n\t"
		"pxor	%%xmm7,%%xmm1\n\t"
		"pxor	%%xmm4,%%xmm2\n\t"
		"pxor	%%xmm4,%%xmm3\n\t"
		"pxor	%%xmm5,%%xmm3\n\t"
		ASM_EPILOGUE(L13)
		break;
	case 14:
		ASM_PROLOGUE(L14)
		"pxor	%%xmm5,%%xmm0\n\t"
		"pxor	%%xmm6,%%xmm0\n\t"
		"pxor	%%xmm7,%%xmm0\n\t"
		"pxor	%%xmm4,%%xmm1\n\t"
		"pxor	%%xmm5,%%xmm1\n\t"
		"pxor	%%xmm4,%%xmm2\n\t"
		"pxor	%%xmm5,%%xmm2\n\t"
		"pxor	%%xmm6,%%xmm2\n\t"
		"pxor	%%xmm4,%%xmm3\n\t"
		"pxor	%%xmm5,%%xmm3\n\t"
		"pxor	%%xmm6,%%xmm3\n\t"
		"pxor	%%xmm7,%%xmm3\n\t"
		ASM_EPILOGUE(L14)
		break;
	case 15:
		ASM_PROLOGUE(L15)
		"pxor	%%xmm4,%%xmm0\n\t"
		"pxor	%%xmm5,%%xmm0\n\t"
		"pxor	%%xmm6,%%xmm0\n\t"
		"pxor	%%xmm7,%%xmm0\n\t"
		"pxor	%%xmm4,%%xmm1\n\t"
		"pxor	%%xmm4,%%xmm2\n\t"
		"pxor	%%xmm5,%%xmm2\n\t"
		"pxor	%%xmm4,%%xmm3\n\t"
		"pxor	%%xmm5,%%xmm3\n\t"
		"pxor	%%xmm6,%%xmm3\n\t"
		ASM_EPILOGUE(L15)
		break;
	}
#endif	// _MSC_VER
}

void
MMX_multadd(hyperword_t *pDst, const hyperword_t *pSrc, unsigned nElements, unsigned nIndex)
{
#if _MSC_VER
#undef ASM_PROLOGUE
#undef ASM_EPILOGUE
#define	ASM_PROLOGUE(label)			\
	__asm	mov		eax,pDst		\
	__asm	mov		edx,pSrc		\
	__asm	mov		ecx,nElements	\
	__asm label:					\
	__asm	movq	mm0,[eax+0]		\
	__asm	movq	mm1,[eax+16]	\
	__asm	movq	mm2,[eax+32]	\
	__asm	movq	mm3,[eax+48]	\
	__asm	movq	mm4,[edx+0] 	\
	__asm	movq	mm5,[edx+16]	\
	__asm	movq	mm6,[edx+32]	\
	__asm	movq	mm7,[edx+48]

#define	ASM_EPILOGUE(label)			\
	__asm	movq	[eax+0],mm0		\
	__asm	movq	[eax+16],mm1	\
	__asm	movq	[eax+32],mm2	\
	__asm	movq	[eax+48],mm3	\
	__asm	add		eax,8			\
	__asm	add		edx,8			\
	__asm	bt		eax,3			\
	__asm	jc		label			\
	__asm	add		eax,64-16		\
	__asm	add		edx,64-16		\
	__asm	loop	label

	switch (nIndex) {
	case 0:
		return;
	case 1:
		ASM_PROLOGUE(L1)
		__asm	pxor	mm0,mm4	// 0 ^ 0
		__asm	pxor	mm1,mm5	// 1 ^ 1
		__asm	pxor	mm2,mm6	// 2 ^ 2
		__asm	pxor	mm3,mm7	// 3 ^ 3
		ASM_EPILOGUE(L1)
		break;
	case 2:
		ASM_PROLOGUE(L2)
		__asm	pxor	mm0,mm7	// 0 ^ 3
		__asm	pxor	mm1,mm4	// 1 ^ 0
		__asm	pxor	mm1,mm7	// 1 ^ 3
		__asm	pxor	mm2,mm5	// 2 ^ 1
		__asm	pxor	mm3,mm6	// 3 ^ 2
		ASM_EPILOGUE(L2)
		break;
	case 3:
		ASM_PROLOGUE(L3)
		__asm	pxor	mm0,mm4	// 0 ^ 0
		__asm	pxor	mm0,mm7	// 0 ^ 3
		__asm	pxor	mm1,mm4	// 1 ^ 0
		__asm	pxor	mm1,mm5	// 1 ^ 1
		__asm	pxor	mm1,mm7	// 1 ^ 3
		__asm	pxor	mm2,mm5	// 2 ^ 1
		__asm	pxor	mm2,mm6	// 2 ^ 2
		__asm	pxor	mm3,mm6	// 3 ^ 2
		__asm	pxor	mm3,mm7	// 3 ^ 3
		ASM_EPILOGUE(L3)
		break;
	case 4:
		ASM_PROLOGUE(L4)
		__asm	pxor	mm0,mm6	// 0 ^ 2
		__asm	pxor	mm1,mm6	// 1 ^ 2
		__asm	pxor	mm1,mm7	// 1 ^ 3
		__asm	pxor	mm2,mm4	// 2 ^ 0
		__asm	pxor	mm2,mm7	// 2 ^ 3
		__asm	pxor	mm3,mm5	// 3 ^ 1
		ASM_EPILOGUE(L4)
		break;
	case 5:
		ASM_PROLOGUE(L5)
		__asm	pxor	mm0,mm4	// 0 ^ 0
		__asm	pxor	mm0,mm6	// 0 ^ 2
		__asm	pxor	mm1,mm5	// 1 ^ 1
		__asm	pxor	mm1,mm6	// 1 ^ 2
		__asm	pxor	mm1,mm7	// 1 ^ 3
		__asm	pxor	mm2,mm4	// 2 ^ 0
		__asm	pxor	mm2,mm6	// 2 ^ 2
		__asm	pxor	mm2,mm7	// 2 ^ 3
		__asm	pxor	mm3,mm5	// 3 ^ 1
		__asm	pxor	mm3,mm7	// 3 ^ 3
		ASM_EPILOGUE(L5)
		break;
	case 6:
		ASM_PROLOGUE(L6)
		__asm	pxor	mm0,mm6	// 0 ^ 2
		__asm	pxor	mm0,mm7	// 0 ^ 3
		__asm	pxor	mm1,mm4	// 1 ^ 0
		__asm	pxor	mm1,mm6	// 1 ^ 2
		__asm	pxor	mm2,mm4	// 2 ^ 0
		__asm	pxor	mm2,mm5	// 2 ^ 1
		__asm	pxor	mm2,mm7	// 2 ^ 3
		__asm	pxor	mm3,mm5	// 3 ^ 1
		__asm	pxor	mm3,mm6	// 3 ^ 2
		ASM_EPILOGUE(L6)
		break;
	case 7:
		ASM_PROLOGUE(L7)
		__asm	pxor	mm0,mm4	// 0 ^ 0
		__asm	pxor	mm0,mm6	// 0 ^ 2
		__asm	pxor	mm0,mm7	// 0 ^ 3
		__asm	pxor	mm1,mm4	// 1 ^ 0
		__asm	pxor	mm1,mm5	// 1 ^ 1
		__asm	pxor	mm1,mm6	// 1 ^ 2
		__asm	pxor	mm2,mm4	// 2 ^ 0
		__asm	pxor	mm2,mm5	// 2 ^ 1
		__asm	pxor	mm2,mm6	// 2 ^ 2
		__asm	pxor	mm2,mm7	// 2 ^ 3
		__asm	pxor	mm3,mm5	// 3 ^ 1
		__asm	pxor	mm3,mm6	// 3 ^ 2
		__asm	pxor	mm3,mm7	// 3 ^ 3
		ASM_EPILOGUE(L7)
		break;
	case 8:
		ASM_PROLOGUE(L8)
		__asm	pxor	mm0,mm5	// 0 ^ 1
		__asm	pxor	mm1,mm5	// 1 ^ 1
		__asm	pxor	mm1,mm6	// 1 ^ 2
		__asm	pxor	mm2,mm6	// 2 ^ 2
		__asm	pxor	mm2,mm7	// 2 ^ 3
		__asm	pxor	mm3,mm4	// 3 ^ 0
		__asm	pxor	mm3,mm7	// 3 ^ 3
		ASM_EPILOGUE(L8)
		break;
	case 9:
		ASM_PROLOGUE(L9)
		__asm	pxor	mm0,mm4	// 0 ^ 0
		__asm	pxor	mm0,mm5	// 0 ^ 1
		__asm	pxor	mm1,mm6	// 1 ^ 2
		__asm	pxor	mm2,mm7	// 2 ^ 3
		__asm	pxor	mm3,mm4	// 3 ^ 0
		ASM_EPILOGUE(L9)
		break;
	case 10:
		ASM_PROLOGUE(L10)
		__asm	pxor	mm0,mm5	// 0 ^ 1
		__asm	pxor	mm0,mm7	// 0 ^ 3
		__asm	pxor	mm1,mm4	// 1 ^ 0
		__asm	pxor	mm1,mm5	// 1 ^ 1
		__asm	pxor	mm1,mm6	// 1 ^ 2
		__asm	pxor	mm1,mm7	// 1 ^ 3
		__asm	pxor	mm2,mm5	// 2 ^ 1
		__asm	pxor	mm2,mm6	// 2 ^ 2
		__asm	pxor	mm2,mm7	// 2 ^ 3
		__asm	pxor	mm3,mm4	// 3 ^ 0
		__asm	pxor	mm3,mm6	// 3 ^ 2
		__asm	pxor	mm3,mm7	// 3 ^ 3
		ASM_EPILOGUE(L10)
		break;
	case 11:
		ASM_PROLOGUE(L11)
		__asm	pxor	mm0,mm4	// 0 ^ 0
		__asm	pxor	mm0,mm5	// 0 ^ 1
		__asm	pxor	mm0,mm7	// 0 ^ 3
		__asm	pxor	mm1,mm4	// 1 ^ 0
		__asm	pxor	mm1,mm6	// 1 ^ 2
		__asm	pxor	mm1,mm7	// 1 ^ 3
		__asm	pxor	mm2,mm5	// 2 ^ 1
		__asm	pxor	mm2,mm7	// 2 ^ 3
		__asm	pxor	mm3,mm4	// 3 ^ 0
		__asm	pxor	mm3,mm6	// 3 ^ 2
		ASM_EPILOGUE(L11)
		break;
	case 12:
		ASM_PROLOGUE(L12)
		__asm	pxor	mm0,mm5	// 0 ^ 1
		__asm	pxor	mm0,mm6	// 0 ^ 2
		__asm	pxor	mm1,mm5	// 1 ^ 1
		__asm	pxor	mm1,mm7	// 1 ^ 3
		__asm	pxor	mm2,mm4	// 2 ^ 0
		__asm	pxor	mm2,mm6	// 2 ^ 2
		__asm	pxor	mm3,mm4	// 3 ^ 0
		__asm	pxor	mm3,mm5	// 3 ^ 1
		__asm	pxor	mm3,mm7	// 3 ^ 3
		ASM_EPILOGUE(L12)
		break;
	case 13:
		ASM_PROLOGUE(L13)
		__asm	pxor	mm0,mm4	// 0 ^ 0
		__asm	pxor	mm0,mm5	// 0 ^ 1
		__asm	pxor	mm0,mm6	// 0 ^ 2
		__asm	pxor	mm1,mm7	// 1 ^ 3
		__asm	pxor	mm2,mm4	// 2 ^ 0
		__asm	pxor	mm3,mm4	// 3 ^ 0
		__asm	pxor	mm3,mm5	// 3 ^ 1
		ASM_EPILOGUE(L13)
		break;
	case 14:
		ASM_PROLOGUE(L14)
		__asm	pxor	mm0,mm5	// 0 ^ 1
		__asm	pxor	mm0,mm6	// 0 ^ 2
		__asm	pxor	mm0,mm7	// 0 ^ 3
		__asm	pxor	mm1,mm4	// 1 ^ 0
		__asm	pxor	mm1,mm5	// 1 ^ 1
		__asm	pxor	mm2,mm4	// 2 ^ 0
		__asm	pxor	mm2,mm5	// 2 ^ 1
		__asm	pxor	mm2,mm6	// 2 ^ 2
		__asm	pxor	mm3,mm4	// 3 ^ 0
		__asm	pxor	mm3,mm5	// 3 ^ 1
		__asm	pxor	mm3,mm6	// 3 ^ 2
		__asm	pxor	mm3,mm7	// 3 ^ 3
		ASM_EPILOGUE(L14)
		break;
	case 15:
		ASM_PROLOGUE(L15)
		__asm	pxor	mm0,mm4	// 0 ^ 0
		__asm	pxor	mm0,mm5	// 0 ^ 1
		__asm	pxor	mm0,mm6	// 0 ^ 2
		__asm	pxor	mm0,mm7	// 0 ^ 3
		__asm	pxor	mm1,mm4	// 1 ^ 0
		__asm	pxor	mm2,mm4	// 2 ^ 0
		__asm	pxor	mm2,mm5	// 2 ^ 1
		__asm	pxor	mm3,mm4	// 3 ^ 0
		__asm	pxor	mm3,mm5	// 3 ^ 1
		__asm	pxor	mm3,mm6	// 3 ^ 2
		ASM_EPILOGUE(L15)
		break;
	}	
	__asm	emms

#else	// !_MSC_VER (GCC)
#undef ASM_PROLOGUE
#undef ASM_EPILOGUE
#ifdef __x86_64__
# define TEST_BIT3				\
	"btq	$3,%%rax\n\t"
#else
# define TEST_BIT3				\
	"btl	$3,%%eax\n\t"
#endif
#define	ASM_PROLOGUE(label)	__asm__(	\
	LOAD_REGS				\
	"0:\n\t"				\
	"movq	  (" EAX "),%%mm0\n\t"		\
	"movq	16(" EAX "),%%mm1\n\t"		\
	"movq	32(" EAX "),%%mm2\n\t"		\
	"movq	48(" EAX "),%%mm3\n\t"		\
	"movq	  (" EDX "),%%mm4\n\t"		\
	"movq	16(" EDX "),%%mm5\n\t"		\
	"movq	32(" EDX "),%%mm6\n\t"		\
	"movq	48(" EDX "),%%mm7\n\t"

#define	ASM_EPILOGUE(label)			\
	"movq	%%mm0,  (" EAX ")\n\t"		\
	"movq	%%mm1,16(" EAX ")\n\t"		\
	"movq	%%mm2,32(" EAX ")\n\t"		\
	"movq	%%mm3,48(" EAX ")\n\t"		\
	BUMP_REGS("$8")				\
	TEST_BIT3				\
	"jc		0b\n\t"			\
	BUMP_REGS("$64-16")			\
	"loop	0b"				\
	: : "m" (pDst), "m" (pSrc), "m" (nElements) );

	switch (nIndex) {
	case 0:
		return;
	case 1:
		ASM_PROLOGUE(L1)
		"pxor	%%mm4,%%mm0\n\t"
		"pxor	%%mm5,%%mm1\n\t"
		"pxor	%%mm6,%%mm2\n\t"
		"pxor	%%mm7,%%mm3\n\t"
		ASM_EPILOGUE(L1)
		break;
	case 2:
		ASM_PROLOGUE(L2)
		"pxor	%%mm7,%%mm0\n\t"
		"pxor	%%mm4,%%mm1\n\t"
		"pxor	%%mm7,%%mm1\n\t"
		"pxor	%%mm5,%%mm2\n\t"
		"pxor	%%mm6,%%mm3\n\t"
		ASM_EPILOGUE(L2)
		break;
	case 3:
		ASM_PROLOGUE(L3)
		"pxor	%%mm4,%%mm0\n\t"
		"pxor	%%mm7,%%mm0\n\t"
		"pxor	%%mm4,%%mm1\n\t"
		"pxor	%%mm5,%%mm1\n\t"
		"pxor	%%mm7,%%mm1\n\t"
		"pxor	%%mm5,%%mm2\n\t"
		"pxor	%%mm6,%%mm2\n\t"
		"pxor	%%mm6,%%mm3\n\t"
		"pxor	%%mm7,%%mm3\n\t"
		ASM_EPILOGUE(L3)
		break;
	case 4:
		ASM_PROLOGUE(L4)
		"pxor	%%mm6,%%mm0\n\t"
		"pxor	%%mm6,%%mm1\n\t"
		"pxor	%%mm7,%%mm1\n\t"
		"pxor	%%mm4,%%mm2\n\t"
		"pxor	%%mm7,%%mm2\n\t"
		"pxor	%%mm5,%%mm3\n\t"
		ASM_EPILOGUE(L4)
		break;
	case 5:
		ASM_PROLOGUE(L5)
		"pxor	%%mm4,%%mm0\n\t"
		"pxor	%%mm6,%%mm0\n\t"
		"pxor	%%mm5,%%mm1\n\t"
		"pxor	%%mm6,%%mm1\n\t"
		"pxor	%%mm7,%%mm1\n\t"
		"pxor	%%mm4,%%mm2\n\t"
		"pxor	%%mm6,%%mm2\n\t"
		"pxor	%%mm7,%%mm2\n\t"
		"pxor	%%mm5,%%mm3\n\t"
		"pxor	%%mm7,%%mm3\n\t"
		ASM_EPILOGUE(L5)
		break;
	case 6:
		ASM_PROLOGUE(L6)
		"pxor	%%mm6,%%mm0\n\t"
		"pxor	%%mm7,%%mm0\n\t"
		"pxor	%%mm4,%%mm1\n\t"
		"pxor	%%mm6,%%mm1\n\t"
		"pxor	%%mm4,%%mm2\n\t"
		"pxor	%%mm5,%%mm2\n\t"
		"pxor	%%mm7,%%mm2\n\t"
		"pxor	%%mm5,%%mm3\n\t"
		"pxor	%%mm6,%%mm3\n\t"
		ASM_EPILOGUE(L6)
		break;
	case 7:
		ASM_PROLOGUE(L7)
		"pxor	%%mm4,%%mm0\n\t"
		"pxor	%%mm6,%%mm0\n\t"
		"pxor	%%mm7,%%mm0\n\t"
		"pxor	%%mm4,%%mm1\n\t"
		"pxor	%%mm5,%%mm1\n\t"
		"pxor	%%mm6,%%mm1\n\t"
		"pxor	%%mm4,%%mm2\n\t"
		"pxor	%%mm5,%%mm2\n\t"
		"pxor	%%mm6,%%mm2\n\t"
		"pxor	%%mm7,%%mm2\n\t"
		"pxor	%%mm5,%%mm3\n\t"
		"pxor	%%mm6,%%mm3\n\t"
		"pxor	%%mm7,%%mm3\n\t"
		ASM_EPILOGUE(L7)
		break;
	case 8:
		ASM_PROLOGUE(L8)
		"pxor	%%mm5,%%mm0\n\t"
		"pxor	%%mm5,%%mm1\n\t"
		"pxor	%%mm6,%%mm1\n\t"
		"pxor	%%mm6,%%mm2\n\t"
		"pxor	%%mm7,%%mm2\n\t"
		"pxor	%%mm4,%%mm3\n\t"
		"pxor	%%mm7,%%mm3\n\t"
		ASM_EPILOGUE(L8)
		break;
	case 9:
		ASM_PROLOGUE(L9)
		"pxor	%%mm4,%%mm0\n\t"
		"pxor	%%mm5,%%mm0\n\t"
		"pxor	%%mm6,%%mm1\n\t"
		"pxor	%%mm7,%%mm2\n\t"
		"pxor	%%mm4,%%mm3\n\t"
		ASM_EPILOGUE(L9)
		break;
	case 10:
		ASM_PROLOGUE(L10)
		"pxor	%%mm5,%%mm0\n\t"
		"pxor	%%mm7,%%mm0\n\t"
		"pxor	%%mm4,%%mm1\n\t"
		"pxor	%%mm5,%%mm1\n\t"
		"pxor	%%mm6,%%mm1\n\t"
		"pxor	%%mm7,%%mm1\n\t"
		"pxor	%%mm5,%%mm2\n\t"
		"pxor	%%mm6,%%mm2\n\t"
		"pxor	%%mm7,%%mm2\n\t"
		"pxor	%%mm4,%%mm3\n\t"
		"pxor	%%mm6,%%mm3\n\t"
		"pxor	%%mm7,%%mm3\n\t"
		ASM_EPILOGUE(L10)
		break;
	case 11:
		ASM_PROLOGUE(L11)
		"pxor	%%mm4,%%mm0\n\t"
		"pxor	%%mm5,%%mm0\n\t"
		"pxor	%%mm7,%%mm0\n\t"
		"pxor	%%mm4,%%mm1\n\t"
		"pxor	%%mm6,%%mm1\n\t"
		"pxor	%%mm7,%%mm1\n\t"
		"pxor	%%mm5,%%mm2\n\t"
		"pxor	%%mm7,%%mm2\n\t"
		"pxor	%%mm4,%%mm3\n\t"
		"pxor	%%mm6,%%mm3\n\t"
		ASM_EPILOGUE(L11)
		break;
	case 12:
		ASM_PROLOGUE(L12)
		"pxor	%%mm5,%%mm0\n\t"
		"pxor	%%mm6,%%mm0\n\t"
		"pxor	%%mm5,%%mm1\n\t"
		"pxor	%%mm7,%%mm1\n\t"
		"pxor	%%mm4,%%mm2\n\t"
		"pxor	%%mm6,%%mm2\n\t"
		"pxor	%%mm4,%%mm3\n\t"
		"pxor	%%mm5,%%mm3\n\t"
		"pxor	%%mm7,%%mm3\n\t"
		ASM_EPILOGUE(L12)
		break;
	case 13:
		ASM_PROLOGUE(L13)
		"pxor	%%mm4,%%mm0\n\t"
		"pxor	%%mm5,%%mm0\n\t"
		"pxor	%%mm6,%%mm0\n\t"
		"pxor	%%mm7,%%mm1\n\t"
		"pxor	%%mm4,%%mm2\n\t"
		"pxor	%%mm4,%%mm3\n\t"
		"pxor	%%mm5,%%mm3\n\t"
		ASM_EPILOGUE(L13)
		break;
	case 14:
		ASM_PROLOGUE(L14)
		"pxor	%%mm5,%%mm0\n\t"
		"pxor	%%mm6,%%mm0\n\t"
		"pxor	%%mm7,%%mm0\n\t"
		"pxor	%%mm4,%%mm1\n\t"
		"pxor	%%mm5,%%mm1\n\t"
		"pxor	%%mm4,%%mm2\n\t"
		"pxor	%%mm5,%%mm2\n\t"
		"pxor	%%mm6,%%mm2\n\t"
		"pxor	%%mm4,%%mm3\n\t"
		"pxor	%%mm5,%%mm3\n\t"
		"pxor	%%mm6,%%mm3\n\t"
		"pxor	%%mm7,%%mm3\n\t"
		ASM_EPILOGUE(L14)
		break;
	case 15:
		ASM_PROLOGUE(L15)
		"pxor	%%mm4,%%mm0\n\t"
		"pxor	%%mm5,%%mm0\n\t"
		"pxor	%%mm6,%%mm0\n\t"
		"pxor	%%mm7,%%mm0\n\t"
		"pxor	%%mm4,%%mm1\n\t"
		"pxor	%%mm4,%%mm2\n\t"
		"pxor	%%mm5,%%mm2\n\t"
		"pxor	%%mm4,%%mm3\n\t"
		"pxor	%%mm5,%%mm3\n\t"
		"pxor	%%mm6,%%mm3\n\t"
		ASM_EPILOGUE(L15)
		break;
	}
	__asm__ __volatile__("emms");
#endif	// _MSC_VER
}

void
STD_multadd(hyperword_t *pDst, const hyperword_t *pSrc, unsigned  nElements, unsigned nIndex)
{
#define XOR_BODY(i,pLHS,pRHS)	(pLHS)->basicword[i] ^= (pRHS)->basicword[i]
#if		HYPERWORD_SIZE == 1
#define XOR(pLHS, pRHS)				\
			XOR_BODY(0,pLHS,pRHS)
#elif	HYPERWORD_SIZE == 2
#define XOR(pLHS, pRHS)				\
			XOR_BODY(0,pLHS,pRHS);	\
			XOR_BODY(1,pLHS,pRHS)
#elif	HYPERWORD_SIZE == 4
#define XOR(pLHS, pRHS)				\
			XOR_BODY(0,pLHS,pRHS);	\
			XOR_BODY(1,pLHS,pRHS);	\
			XOR_BODY(2,pLHS,pRHS);	\
			XOR_BODY(3,pLHS,pRHS)
#else
#error	Invalid HYPERWORD_SIZE constant (must be 1, 2 or 4)
#endif

	do {
		switch (nIndex) {
		case 0:
			break;
		case 1:
			XOR(&pDst[0], &pSrc[0]);
			XOR(&pDst[1], &pSrc[1]);
			XOR(&pDst[2], &pSrc[2]);
			XOR(&pDst[3], &pSrc[3]);
			break;
		case 2:
			XOR(&pDst[0], &pSrc[3]);
			XOR(&pDst[1], &pSrc[0]);
			XOR(&pDst[1], &pSrc[3]);
			XOR(&pDst[2], &pSrc[1]);
			XOR(&pDst[3], &pSrc[2]);
			break;
		case 3:
			XOR(&pDst[0], &pSrc[0]);
			XOR(&pDst[0], &pSrc[3]);
			XOR(&pDst[1], &pSrc[0]);
			XOR(&pDst[1], &pSrc[1]);
			XOR(&pDst[1], &pSrc[3]);
			XOR(&pDst[2], &pSrc[1]);
			XOR(&pDst[2], &pSrc[2]);
			XOR(&pDst[3], &pSrc[2]);
			XOR(&pDst[3], &pSrc[3]);
			break;
		case 4:
			XOR(&pDst[0], &pSrc[2]);
			XOR(&pDst[1], &pSrc[2]);
			XOR(&pDst[1], &pSrc[3]);
			XOR(&pDst[2], &pSrc[0]);
			XOR(&pDst[2], &pSrc[3]);
			XOR(&pDst[3], &pSrc[1]);
			break;
		case 5:
			XOR(&pDst[0], &pSrc[0]);
			XOR(&pDst[0], &pSrc[2]);
			XOR(&pDst[1], &pSrc[1]);
			XOR(&pDst[1], &pSrc[2]);
			XOR(&pDst[1], &pSrc[3]);
			XOR(&pDst[2], &pSrc[0]);
			XOR(&pDst[2], &pSrc[2]);
			XOR(&pDst[2], &pSrc[3]);
			XOR(&pDst[3], &pSrc[1]);
			XOR(&pDst[3], &pSrc[3]);
			break;
		case 6:
			XOR(&pDst[0], &pSrc[2]);
			XOR(&pDst[0], &pSrc[3]);
			XOR(&pDst[1], &pSrc[0]);
			XOR(&pDst[1], &pSrc[2]);
			XOR(&pDst[2], &pSrc[0]);
			XOR(&pDst[2], &pSrc[1]);
			XOR(&pDst[2], &pSrc[3]);
			XOR(&pDst[3], &pSrc[1]);
			XOR(&pDst[3], &pSrc[2]);
			break;
		case 7:
			XOR(&pDst[0], &pSrc[0]);
			XOR(&pDst[0], &pSrc[2]);
			XOR(&pDst[0], &pSrc[3]);
			XOR(&pDst[1], &pSrc[0]);
			XOR(&pDst[1], &pSrc[1]);
			XOR(&pDst[1], &pSrc[2]);
			XOR(&pDst[2], &pSrc[0]);
			XOR(&pDst[2], &pSrc[1]);
			XOR(&pDst[2], &pSrc[2]);
			XOR(&pDst[2], &pSrc[3]);
			XOR(&pDst[3], &pSrc[1]);
			XOR(&pDst[3], &pSrc[2]);
			XOR(&pDst[3], &pSrc[3]);
			break;
		case 8:
			XOR(&pDst[0], &pSrc[1]);
			XOR(&pDst[1], &pSrc[1]);
			XOR(&pDst[1], &pSrc[2]);
			XOR(&pDst[2], &pSrc[2]);
			XOR(&pDst[2], &pSrc[3]);
			XOR(&pDst[3], &pSrc[0]);
			XOR(&pDst[3], &pSrc[3]);
			break;
		case 9:
			XOR(&pDst[0], &pSrc[0]);
			XOR(&pDst[0], &pSrc[1]);
			XOR(&pDst[1], &pSrc[2]);
			XOR(&pDst[2], &pSrc[3]);
			XOR(&pDst[3], &pSrc[0]);
			break;
		case 10:
			XOR(&pDst[0], &pSrc[1]);
			XOR(&pDst[0], &pSrc[3]);
			XOR(&pDst[1], &pSrc[0]);
			XOR(&pDst[1], &pSrc[1]);
			XOR(&pDst[1], &pSrc[2]);
			XOR(&pDst[1], &pSrc[3]);
			XOR(&pDst[2], &pSrc[1]);
			XOR(&pDst[2], &pSrc[2]);
			XOR(&pDst[2], &pSrc[3]);
			XOR(&pDst[3], &pSrc[0]);
			XOR(&pDst[3], &pSrc[2]);
			XOR(&pDst[3], &pSrc[3]);
			break;
		case 11:
			XOR(&pDst[0], &pSrc[0]);
			XOR(&pDst[0], &pSrc[1]);
			XOR(&pDst[0], &pSrc[3]);
			XOR(&pDst[1], &pSrc[0]);
			XOR(&pDst[1], &pSrc[2]);
			XOR(&pDst[1], &pSrc[3]);
			XOR(&pDst[2], &pSrc[1]);
			XOR(&pDst[2], &pSrc[3]);
			XOR(&pDst[3], &pSrc[0]);
			XOR(&pDst[3], &pSrc[2]);
			break;
		case 12:
			XOR(&pDst[0], &pSrc[1]);
			XOR(&pDst[0], &pSrc[2]);
			XOR(&pDst[1], &pSrc[1]);
			XOR(&pDst[1], &pSrc[3]);
			XOR(&pDst[2], &pSrc[0]);
			XOR(&pDst[2], &pSrc[2]);
			XOR(&pDst[3], &pSrc[0]);
			XOR(&pDst[3], &pSrc[1]);
			XOR(&pDst[3], &pSrc[3]);
			break;
		case 13:
			XOR(&pDst[0], &pSrc[0]);
			XOR(&pDst[0], &pSrc[1]);
			XOR(&pDst[0], &pSrc[2]);
			XOR(&pDst[1], &pSrc[3]);
			XOR(&pDst[2], &pSrc[0]);
			XOR(&pDst[3], &pSrc[0]);
			XOR(&pDst[3], &pSrc[1]);
			break;
		case 14:
			XOR(&pDst[0], &pSrc[1]);
			XOR(&pDst[0], &pSrc[2]);
			XOR(&pDst[0], &pSrc[3]);
			XOR(&pDst[1], &pSrc[0]);
			XOR(&pDst[1], &pSrc[1]);
			XOR(&pDst[2], &pSrc[0]);
			XOR(&pDst[2], &pSrc[1]);
			XOR(&pDst[2], &pSrc[2]);
			XOR(&pDst[3], &pSrc[0]);
			XOR(&pDst[3], &pSrc[1]);
			XOR(&pDst[3], &pSrc[2]);
			XOR(&pDst[3], &pSrc[3]);
			break;
		case 15:
			XOR(&pDst[0], &pSrc[0]);
			XOR(&pDst[0], &pSrc[1]);
			XOR(&pDst[0], &pSrc[2]);
			XOR(&pDst[0], &pSrc[3]);
			XOR(&pDst[1], &pSrc[0]);
			XOR(&pDst[2], &pSrc[0]);
			XOR(&pDst[2], &pSrc[1]);
			XOR(&pDst[3], &pSrc[0]);
			XOR(&pDst[3], &pSrc[1]);
			XOR(&pDst[3], &pSrc[2]);
			break;
		}
		pDst += ELEMENT_WIDTH; pSrc += ELEMENT_WIDTH;
	} while (--nElements > 0);
}

// Dump out the operations described by the 4x4 GF(2) multiplication matrices as code.
void
GF2Mul::dump()
{
#ifdef	_DEBUG

// Define one of the following to generate source code for above
//#define C_STD
//#define MSC_MMX
#define GCC_MMX
//#define MSC_SSE2
//#define GCC_SSE2

	using namespace std;
	matrix<gf2> mop;
	cout << "** Begin GF2Mul::dump() **" << endl;
	for (unsigned int v = 0; v < gfQ::order; v++) {
		mop = multOp(gfQ(v));
		cout << "	case " << v << ":" << endl;
#ifndef C_STD
		cout << "		ASM_PROLOGUE(L" << v << ")" << endl;
#endif
		//mop.print();
		for (unsigned i = 0; i < mop.rows(); i++)
		for (unsigned j = 0; j < mop.cols(); j++)
		if (mop(i,j).regular() == 1)
			{
#ifdef C_STD
		cout << "		XOR(&pDst[" << i << "], &pSrc[" << j << "]);" << endl;
#endif
#ifdef MSC_MMX
		cout << "		__asm	pxor	mm" << i << ",mm" << j+4 << "	// " << i << " ^ " << j << endl;
#endif
#ifdef GCC_MMX
		cout << "		\"pxor	%%mm" << j+4 << ",%%mm" << i << "\\n\\t\"" << endl;
#endif
#ifdef MSC_SSE2
		cout << "		__asm	pxor	xmm" << i << ",xmm" << j+4 << "	// " << i << " ^ " << j << endl;
#endif
#ifdef GCC_SSE2
		cout << "		\"pxor	%%xmm" << j+4 << ",%%xmm" << i << "\\n\\t\"" << endl;
#endif
			}
#ifndef	C_STD
		cout << "		ASM_EPILOGUE(L" << v << ")" << endl;
#endif
		cout << "		break;" << endl;
	}
	cout << "** End GF2Mul::dump() **" << endl;
#endif
}

} // namespace HoloStor
