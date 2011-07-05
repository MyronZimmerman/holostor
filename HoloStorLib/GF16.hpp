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
	GF16.hpp

 Abstract:
	Class for arithmetic over the 2**4 extension field.  The elements of the field
	are manipulated by the +, -, *, / binary operators and the - unary operator.

--****************************************************************************/
#ifndef HOLOSTOR_HOLOSTORLIB_GF16_HPP_
#define HOLOSTOR_HOLOSTORLIB_GF16_HPP_

#ifdef _DEBUG
#include <iostream>
#endif
#include <assert.h>		// for ANSI assert()

#include "Config.h"

namespace HoloStor {

/**************** GF16 ****************/
// Galois Field of order 2**4.  
//
// The polynomial coefficients are stored as bits in a word.  Addition and subtraction
// are by bitwise XOR.  Multiplication and division are by discrete logarithms. 
// The discrete log and anti-log tables are constructed from polynomials over GF(2) 
// that are reduced by an irreducible polynomial of degree 4.
//

class GF16 {
private:
	typedef unsigned char storage_t;
	//
	storage_t value;					// bit vector representation
	//
	static const storage_t dLog[16];	// discrete log and anti-log tables
	static const storage_t dExp[16];
	//
	static unsigned int mul(unsigned int a, unsigned int b) {
		if (a == 0 || b == 0)
			return 0;
		int i = (int)dLog[a] + (int)dLog[b];
		if (i >= 15)
			i -= 15;
		return dExp[i];
	}
	static unsigned int div(unsigned int a, unsigned int b) {
		assert(b != 0);
		if (a == 0)
			return 0;
		int i = (int)dLog[a] - (int)dLog[b];
		if (i < 0)
			i += 15;
		return dExp[i];
	}
public:
	// Constructor
	GF16() : value(0) {}
	// Copy constructors
	GF16(unsigned int v) : value(v)	{ assert(value < 16); }
	GF16(const GF16& rhs) : value(rhs.value) {}
	// Copy assignment
	GF16 operator=(const GF16 rhs) {
		value = rhs.value;
		return *this;
	}
	// Unary negation (a NOP)
	GF16 operator-() const { return *this; }
	// Addition, subtraction, multiplication and division
	friend GF16 operator+(const GF16& a, const GF16& b) { return GF16(a.value ^ b.value); }
	friend GF16 operator-(const GF16& a, const GF16& b) { return GF16(a.value ^ b.value); }
	friend GF16 operator*(const GF16& a, const GF16& b) { return GF16(GF16::mul(a.value, b.value)); }
	friend GF16 operator/(const GF16& a, const GF16& b) { return GF16(GF16::div(a.value, b.value)); }
	GF16 operator+=(const GF16& rhs) { value ^= rhs.value; return *this; }
	GF16 operator-=(const GF16& rhs) { value ^= rhs.value; return *this; }
	GF16 operator*=(const GF16& rhs) { value = mul(value, rhs.value); return *this; }
	GF16 operator/=(const GF16& rhs) { value = div(value, rhs.value); return *this; }
	// Comparisons
	friend int operator==(const GF16& a, const GF16& b) { return a.value == b.value; }
	friend int operator!=(const GF16& a, const GF16& b) { return a.value != b.value; }
	//
#ifdef _DEBUG
	friend std::ostream& operator<<(std::ostream& s, const GF16& x) {
		std::cout << (unsigned int)x.value;		// avoid the display format of char types
		return s;
	}
#endif
	//
	static const unsigned order;				// number of elements in the field
	static const unsigned degree;				// log2(order)
	unsigned int regular() const { return value; }
	//
	NEWOPERATORS
};

} // namespace HoloStor
#endif	// HOLOSTOR_HOLOSTORLIB_GF16_HPP_
