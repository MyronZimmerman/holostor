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
	Tuple.hpp

 Abstract:
	Interface for the Tuple class.  A Tuple stores up to MaxK values of type 
	tuple_t.
	
--****************************************************************************/

#ifndef HOLOSTOR_HOLOSTORLIB_TUPLE_HPP_
#define HOLOSTOR_HOLOSTORLIB_TUPLE_HPP_

#include <string.h>		// for ANSI memcpy()
//
#include "Config.h"
#include "Types.h"

namespace HoloStor {

class Tuple {
private:
	typedef unsigned char tuple_t;
	typedef unsigned int uint_t;
	//
	tuple_t	xValue[MaxK+1];	// xValue[0] is dimension, xValue[1..MaxK] are coordinates
public:
	// constructor
	Tuple() { setDim(0); }
	// copy constructors
	Tuple(const Tuple& rhs) { ::memcpy(xValue, rhs.xValue, sizeof(xValue)); }
	// copy assignment
	Tuple operator=(const Tuple rhs) {
		if (this != &rhs)		// not self-assignment
			::memcpy(xValue, rhs.xValue, sizeof(xValue));
		return *this;
	}
	// operator() - access the coordinate
	      tuple_t& operator()(uint_t i)       { return xValue[i+1]; }
	const tuple_t& operator()(uint_t i) const { return xValue[i+1]; }
	//
	bool setDim(uint_t i);
	uint_t getDim() const		{ return xValue[0]; }
	bool isEnd(uint_t i) const	{ return i >= getDim(); }
	bool isMember(tuple_t x) const	{
		for (uint_t i = 0; i < getDim(); i++)
			if (x == (*this)(i))
				return true;
		return false;
	}
	//
	unsigned mask() const;					// bit mask representation
	void print(const char *s = NULL) const;	// for debug
	//
	NEWOPERATORS
};

} // namespace HoloStor
#endif	// HOLOSTOR_HOLOSTORLIB_TUPLE_HPP_
