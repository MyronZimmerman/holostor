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
	CodingMatrix.cpp

 Abstract:
	Implementation of the CodingMatrix class.
	
 --****************************************************************************/

#include <string.h>			// for ANSI memset(), memcpy()

#include "CodingMatrix.hpp"

namespace HoloStor {

bool 
CodingMatrix::CodingMatrixInit(Tuple faults, IDA& generator)
{
	nRows = faults.getDim();
	for (int k = 0; k < nRows; k++)
		RowID[k] = faults(k);
	matrixGFQ_t mCoding;
	if ( !generator.GenerateCoding(faults, mCoding, ColID) )
		return false;								// out of memory
	mGF2ops.setDim(nRows, mCoding.cols());
	if ( mGF2ops.isNil() )
		return false;								// out of memory
	for (unsigned i = 0; i < mGF2ops.rows(); i++)
		for (unsigned j = 0; j < mGF2ops.cols(); j++)
			mGF2ops(i, j) = GF2Mul( mCoding(RowID[i],j) );
	return true;
}

void
CodingMatrix::Rebuild(UCHAR **lpBlockGroup, INT lWhichBlock, UINT BlockSize) const
{
	// zero the destination blocks
	if (lWhichBlock >= 0)
		::memset(lpBlockGroup[lWhichBlock], 0, BlockSize);
	else for (int i = 0; i < nRows; i++) 
		::memset(lpBlockGroup[RowID[i]], 0, BlockSize);
	//
	for (int i = 0; i < nRows; i++) {
		const unsigned row = RowID[i];
		if (lWhichBlock >= 0 && row != (unsigned)lWhichBlock)
			continue;
		for (unsigned j = 0; j < mGF2ops.cols(); j++) {
			const int col = ColID[j];
#if 1
			// 20-30% faster
			mGF2ops(i, j).gf2multadd(
							(hyperword_t*)(lpBlockGroup[row]),
							(hyperword_t*)(lpBlockGroup[col]),
							BlockSize/sizeof(Element)
							);
#else
			for (int offset = 0; offset < BlockSize; offset += sizeof(Element)) {
				mGF2ops(i, j).gf2multadd(
								(hyperword_t*)(lpBlockGroup[row]+offset),
								(hyperword_t*)(lpBlockGroup[col]+offset)
								);
			}
#endif
		}
	}
}

void 
CodingMatrix::EncodeDelta(UINT lDeltaIndex, const UCHAR* lpDeltaBlock,
						  const UCHAR* lpEccBlockOld, UCHAR* lpEccBlockNew,
						  UINT BlockSize) const
{
	::memcpy(lpEccBlockNew, lpEccBlockOld, BlockSize);
	mGF2ops(0, lDeltaIndex).gf2multadd(
							(hyperword_t*)lpEccBlockNew,
							(hyperword_t*)lpDeltaBlock,
							BlockSize/sizeof(Element)
							);
}

} // namespace HoloStor
