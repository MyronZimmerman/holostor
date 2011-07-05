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
	GF16.cpp

 Abstract:
	Class for arithmetic over the 2**4 extension field.  The elements of the field
	are manipulated by its +, -, *, / binary operators and - unary 	operator.

--****************************************************************************/

#include "GF16.hpp"

namespace HoloStor {

const unsigned
GF16::degree = 4;

const unsigned 
GF16::order = 16;

// Based on irreduciable polynomial x^4+x+1.
const GF16::storage_t
GF16::dLog[16] = { 0, 15, 1, 4, 2, 8, 5, 10, 3, 14, 9, 7, 6, 13, 11, 12 };
//
const GF16::storage_t
GF16::dExp[16] = { 1, 2, 4, 8, 3, 6, 12, 11, 5, 10, 7, 14, 15, 13, 9, 1 };

} // namespace HoloStor
