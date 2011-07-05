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
	CodingMatrix.hpp

 Abstract:
	Interface for the CodingMatrix class.

--****************************************************************************/

#ifndef HOLOSTOR_HOLOSTORLIB_CODINGMATRIX_HPP_
#define HOLOSTOR_HOLOSTORLIB_CODINGMATRIX_HPP_

#include "HoloStor.h"
#include "Config.h"
#include "Types.h"
//
#include "Tuple.hpp"
#include "IDA.hpp"
//
#include "TypesGF.hpp"
#include "GF2Mul.hpp"

namespace HoloStor {

class CodingMatrix {
private:
	UCHAR nRows;				// number of rows to recover
	UCHAR RowID[MaxK];			// row numbers to recover
	UCHAR ColID[MaxN];			// col numbers used for recovery
	// coding with multiplication operations in GF(2) representation
	matrix<GF2Mul> mGF2ops;
public:
	// constructor
	CodingMatrix() : nRows(0) {}
	//
	bool CodingMatrixInit(Tuple faults, IDA& mCoding);
	void Rebuild(UCHAR **lpBlockGroup, INT lWhichBlock, UINT BlockSize) const;
	void EncodeDelta(UINT lDeltaIndex, const UCHAR* lpDeltaBlock,
		const UCHAR* lpEccBlockOld, UCHAR* lpEccBlockNew, UINT BlockSize) const;
	//
	static unsigned MinBlockSize() { return sizeof(Element); }
	//
	NEWOPERATORS
};

} // namespace HoloStor
#endif	// HOLOSTOR_HOLOSTORLIB_CODINGMATRIX_HPP_
