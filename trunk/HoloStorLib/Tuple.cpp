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
	Tuple.cpp

 Abstract:
	Implementation of the Tuple class.  
	
--****************************************************************************/

#include "Tuple.hpp"

#ifdef	_DEBUG
#include <iostream>			// for cout
#endif

namespace HoloStor {

bool
Tuple::setDim(uint_t i)
{
	if (i > (uint_t)MaxK)
		return false;
	xValue[0] = i;
	return true;
}

unsigned
Tuple::mask() const
{
	unsigned bits = 0;
	for (UINT i = 0; i < getDim(); i++)
		bits |= (1<<(*this)(i));
	return bits;
}

void
Tuple::print(const char *s) const
{
#ifdef	_DEBUG
	if (s)
		std::cout << s << std::endl;
	std::cout << "(";
	for (UINT i = 0; i < getDim(); i++)
		std::cout << (int)(*this)(i) << " ";
	std::cout << ")" << std::endl;
#endif
}

} // namespace HoloStor
