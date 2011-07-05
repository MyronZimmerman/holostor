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
	CombinIter.cpp

 Abstract:
	Implementation of the CombinIter class.  CombinIter iterates over
	combinations.

--****************************************************************************/

#include "CombinIter.hpp"
#include "MathUtils.hpp"

#ifdef	_DEBUG
#include <iostream>
#endif
#include <assert.h>		// for ANSI assert()

namespace HoloStor {

bool
CombinIter::CombinIterInit(unsigned n, unsigned k)
{
	nItems = n;
	nDrawn = k;
	//
	assert(nItems >= nDrawn);
	if (!tuple.setDim(nDrawn))
		return false;
	//
	for (unsigned i = 0; i < nDrawn; i++)
		tuple(i) = nDrawn-1-i;			// item labels are 0 ... nItems-1
	bMore = true;
	return true;
}

// Increment the tuple starting at tuple(nStart), returning true if successful.
bool
CombinIter::Next(unsigned nStart)
{
	if (nStart == nDrawn)
		return false;							// Next(nDrawn-1) rolled over.
	if (tuple(nStart) < nItems-1-nStart)
		tuple(nStart)++;
	else {
		// Value is already at its maximum. Increment tuple(nStart) and restart this value.
		if (Next(nStart+1) == false)
			return false;						// Out of combinations.
		tuple(nStart) = tuple(nStart+1)+1;
	}
	return true;
}

// Return the next tuple, returning true is successful.
bool
CombinIter::Draw(Tuple& tup)
{
	if (!bMore) 
		return false;
	tup = tuple;			// copy the current value
	// compute next
	bMore = Next();
	return true;
}

void
CombinIter::Test()
{
#ifdef	_DEBUG
	const unsigned n = 5;
	const unsigned r = 3;
	Tuple ntuple;
	CombinIter foo;
	foo.CombinIterInit(n,r);
	unsigned nCount = 0;
	while (foo.Draw(ntuple)) {
		nCount++;
		std::cout << "(";
		for (unsigned j = 0; ; ) {
			std::cout << (int)ntuple(j++);
			if (j == r)
			 break;
			std::cout << ",";
		}
		std::cout << ")" << std::endl;
	}
	std::cout << nCount << " combinations (" << HoloStor::combinations(n,r) << " expected)" << std::endl;
#endif
}

} // namespace HoloStor
