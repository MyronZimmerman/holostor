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
	GF2Mul.hpp

 Abstract:
	Interface to class GF2Mul.
	
	This class implements multiply (& add) operations in a GF(2**ELEMENT_WIDTH) 
	extension field with operations in GF(2) (i.e. XOR and AND).  The advantage 
	of this approach is speed.  The bit operations can be simultaneously 
	performed on all the bits of hyperword_t.

--****************************************************************************/
#ifndef	HOLOSTOR_HOLOSTORLIB_GF2MUL_HPP_
#define HOLOSTOR_HOLOSTORLIB_GF2MUL_HPP_

#include "HoloStor.h"
#include "Config.h"
#include "Types.h"
//
#include "TypesGF.hpp"
#include "gfprime.hpp"
//
#include <string.h>		// for ANSI memset(), memcpy()
#include <assert.h>		// for ANSI assert()

namespace HoloStor {

class GF2Mul {
	typedef gfp<2> gf2;
private:
	unsigned char m_index;					// good for Width == 4 only
	//
	void _init(const gfQ& x);
	static matrix<gf2> multOp(gfQ x);
public:
	// constructors
	GF2Mul() : m_index(0) {}
	GF2Mul(const gfQ& x)		{ _init(x); }
	GF2Mul(const unsigned v)	{ _init(gfQ(v)); }
	// copy constructors
	GF2Mul(const GF2Mul& rhs) : m_index(rhs.m_index) {}
	// copy assignment
	GF2Mul operator=(const GF2Mul rhs) {
		m_index = rhs.m_index;
		return *this;
	}
	//
	void gf2multadd(hyperword_t *pDst, const hyperword_t *pSrc, unsigned nElements = 1) const;
	//
	static void dump();
	//
	NEWOPERATORS
};

} // namespace HoloStor
#endif	// HOLOSTOR_HOLOSTORLIB_GF2MUL_HPP_
