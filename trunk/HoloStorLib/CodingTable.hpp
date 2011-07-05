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
	CodingTable.hpp

 Abstract:
	Interface for the MatrixTable class.

--****************************************************************************/

#ifndef HOLOSTOR_HOLOSTORLIB_CODINGTABLE_HPP_
#define HOLOSTOR_HOLOSTORLIB_CODINGTABLE_HPP_

#include "HoloStor.h"
#include "Config.h"
#include "Types.h"
//
#include "CodingMatrix.hpp"

namespace HoloStor {

class CodingTable {
private:
	unsigned nTotalBlocks;
	unsigned nMatrices, nHashValues;
	CodingIndex *pHashTable;		// find a matrix by uInvalidMask
	CodingMatrix *pCodeTable;		// array of actual coding matrices
	//
	static const CodingIndex BadHash = ~0;	// unused hash value
	//
	void _cleanup();				// deallocate memory
public:
	// constructor
	CodingTable() : pHashTable(NULL), pCodeTable(NULL) {}
	// destructor
	~CodingTable() { _cleanup(); }
	//
	int CodingTableInit(const HOLOSTOR_CFG *pCfg);
	CodingMatrix *lookup(UINT32 uInvalidMask) const;
	//
	static unsigned _MatrixCount(unsigned n, unsigned k);	// count recovery matrices
	static unsigned _MaxHash(unsigned n, unsigned k);		// maximum hash value
};

} // namespace HoloStor
#endif	// HOLOSTOR_HOLOSTORLIB_CODINGTABLE_HPP_
