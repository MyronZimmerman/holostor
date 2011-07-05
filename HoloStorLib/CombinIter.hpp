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
	CombinIter.hpp

 Abstract:
	Template for dynamic tables.

--****************************************************************************/
#ifndef HOLOSTOR_HOLOSTORLIB_COMBINITER_HPP_
#define HOLOSTOR_HOLOSTORLIB_COMBINITER_HPP_

#include "Config.h"
#include "Types.h"
//
#include "Tuple.hpp"

namespace HoloStor {

class CombinIter {
private:
	bool bMore;
	unsigned nItems, nDrawn;
	Tuple tuple;						// ordering: tuple(0) > tuple(1) > tuple(2) ...
	//
	bool Next(unsigned nStart = 0);
public:
	// constructor
	CombinIter() : bMore(false), nItems(0), nDrawn(0) {}
	//
	bool CombinIterInit(unsigned n, unsigned k);
	bool Draw(Tuple& tup);
	//
	static void Test();
};

} // namespace HoloStor
#endif	// HOLOSTOR_HOLOSTORLIB_COMBINITER_HPP_
