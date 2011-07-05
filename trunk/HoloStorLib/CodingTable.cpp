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
	CodingTable.cpp

 Abstract:
	Implementation of the CodingTable class.  CodingTable is a container for
	the CodingMatrix class.  The primary requirement is fast lookup given a
	uInvalidMask.
	
--****************************************************************************/

#include "CodingTable.hpp"
//
#include "MathUtils.hpp"
#include "CombinIter.hpp"
#include "IDA.hpp"
//
#include <assert.h>		// for ANSI assert()

namespace HoloStor {

//
// Calculate a table index based on a bit-mask of invalid blocks.  Bits within Mask
// are numbered 1 (LSB) thru M (MSB).  M is the same as the Blocks argument. Suppose 
// bits A0, A1, A2 are on where A0 > A1 > A2.  Then the index returned is 
//					A2*(Blocks)**2 + A1*(Blocks)**1 + A0.
// The problem is similar to converting BCD (base M) to binary.
//
// Properties:
//	+ 0 returned implies 0 bits on.
//	+ returned value > _MaxHash(n,k) implies more than k bits on.
//
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4035)		// no return value
UINT32
Mask2Index(
            UINT32 Mask,		// Bit mask of invalid blocks
            UINT32 Blocks		// Number of blocks
            )
{
	__asm {
			mov		ecx, Mask
			xor		eax, eax
A:
			cmp		ecx, 0				// Stop when no more bits are on
			jz		B
			imul	eax, Blocks			// Scale index by # of blocks
			bsf		edx, ecx			// Find the lowest on bit
			btr		ecx, edx			// Turn off the bit
			lea		eax, [edx+eax+1]	// Add the bit # + 1 to index
			jmp		A
B:
	}
}
#pragma warning(pop)
#elif defined(__GNUC__)			// XXX - works for -O2 but not for -O3
UINT32
Mask2Index(
            UINT32 Mask,		// Bit mask of invalid blocks
            UINT32 Blocks		// Number of blocks
            )
{
	UINT32 sum;

	__asm__ __volatile__(
		"movl	%1,%%ecx\n\t"
		"xorl	%%eax,%%eax\n"
		"0:\n\t"
		"cmpl	$0,%%ecx\n\t"
		"jz		0f\n\t"
		"imull	%2,%%eax\n\t"
		"bsfl	%%ecx,%%edx\n\t"
		"btrl	%%edx,%%ecx\n\t"
		"leal	1(%%edx,%%eax),%%eax\n\t"
		"jmp	0b\n"
		"0:"
		:"=a" (sum) : "m" (Mask), "m" (Blocks) : "ecx","edx"
	);
	return sum;
}
#else // !defined(_MSC_VER) && !defined(__GNUC__)
UINT32
Mask2Index(
            UINT32 Mask,		// Bit mask of invalid blocks
            UINT32 Blocks		// Number of blocks
            )
{
	UINT32 sum = 0;
	for (int index = 0; index < Blocks; index++)
		if (Mask & (1<<index)) {
			sum *= Blocks;
			sum += index+1;
		}
	return sum;
}
#endif // defined(_MSC_VER)

// When there are k bits on, Mask2Index() returns sum from i=0 to k-1 a[i]*M**i
// where M=n+k and a[i] is the bit position number.  Bit position numbers are 1..M
// and ordered such that a[0] > a[1] > a[2] ...  The returned value is maximal
// for a[0]=M, a[1]=M-1, ...
unsigned
CodingTable::_MaxHash(unsigned n, unsigned k)
{
	const unsigned M = n + k;
	unsigned factor = 1;
	unsigned sum = 0;
	for (unsigned i = 0; i < k; i++) {
		sum += (M - i)*factor;
		factor *= M;
	}
	return sum;
}

unsigned
CodingTable::_MatrixCount(unsigned n, unsigned k)
{
	unsigned sum = 0;
	for (unsigned i = 1; i <= k; i++)
		sum += HoloStor::combinations(n+k, i);
	return sum;
}

void
CodingTable::_cleanup()
{
	if (pHashTable != NULL)
		HoloStor_TableFree(pHashTable);	// instead of:  delete [] pHashTable;
	pHashTable = NULL;
	if (pCodeTable != NULL)
		delete [] pCodeTable;
	pCodeTable = NULL;
}

int
CodingTable::CodingTableInit(const HOLOSTOR_CFG *pCfg)
{
	if (pCfg->BlockSize < CodingMatrix::MinBlockSize())
		return HOLOSTOR_STATUS_BAD_CONFIGURATION;
	const unsigned n = pCfg->DataBlocks;
	const unsigned k = pCfg->EccBlocks;
	if (n < MinN || n > MaxN || k < MinK || k > MaxK)	// impose limits before too late
		return HOLOSTOR_STATUS_BAD_CONFIGURATION;
	IDA generator;
	if ( !generator.IDAInit(n, k) )
		return HOLOSTOR_STATUS_BAD_CONFIGURATION;			// unsupported combination of n and k
	//
	_cleanup();
	nTotalBlocks = n + k;
	//
	nMatrices = _MatrixCount(n, k);
	pCodeTable = new CodingMatrix[nMatrices];			// uses HoloStor_TableAlloc()
	WORKAROUND1(pCodeTable);							// XXX - GCC 3.3.1 bug workaround
	if (pCodeTable == NULL)
		return HOLOSTOR_STATUS_NO_MEMORY;
	nHashValues = _MaxHash(n, k) + 1;					// k needs to be limited here
	// OK to bypass operator new[] since CodingIndex is a primitive type.
	//		pHashTable = new CodingIndex[nHashValues];
	pHashTable =
		(CodingIndex*)HoloStor_TableAlloc(sizeof(CodingIndex)*nHashValues);
	if (pHashTable == NULL)
		return HOLOSTOR_STATUS_NO_MEMORY;
	for (unsigned i = 0; i < nHashValues; i++)
		pHashTable[i] = BadHash;
	//
	unsigned index = 0;
	for (unsigned nFaults = 1; nFaults <= k; nFaults++) {
		CombinIter iter;
		if ( !iter.CombinIterInit(nTotalBlocks, nFaults) )
			return HOLOSTOR_STATUS_BAD_CONFIGURATION;	// XXX - shouldn't happen
		Tuple ktuple;
		while ( iter.Draw(ktuple) ) {
			if ( !pCodeTable[index].CodingMatrixInit(ktuple, generator) )
				return HOLOSTOR_STATUS_NO_MEMORY;
			UINT32 hash = Mask2Index(ktuple.mask(), nTotalBlocks);
			assert(hash < nHashValues && index < nMatrices);
			pHashTable[hash] = index++;
		}
	}
	return HOLOSTOR_STATUS_SUCCESS;
}

CodingMatrix *
CodingTable::lookup(UINT32 uInvalidMask) const
{
	UINT32 hash = Mask2Index(uInvalidMask, nTotalBlocks);
	if (hash == 0 || hash >= nHashValues)
		return NULL;				// either none or too many faults to recover
	unsigned index = pHashTable[hash];
	assert(index != BadHash && index < nMatrices);
	return &pCodeTable[index];
}

} // namespace HoloStor
