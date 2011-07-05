/*  Copyright (C) 2002-2011 Thomas P. Scott and Myron Zimmerman

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
	gfprime.hpp

 Abstract:
	Class template for arithmetic over a prime field.  The elements of the field
	are manipulated by its +, -, * binary operators and - unary operator.

--****************************************************************************/
#ifndef HOLOSTOR_HOLOSTORLIB_GFPRIME_HPP_
#define HOLOSTOR_HOLOSTORLIB_GFPRIME_HPP_

#ifdef _DEBUG
#include <iostream>
#endif
#include <assert.h>		// for ANSI assert()

#include "Config.h"

namespace HoloStor {

/**************** gfp(p) ****************/
// Galois Field consisting of integers modulo a prime, i.e. { 0, 1, ... prime-1 }.
// This class is a thin wrapper around unsigned; as a thin layer, objects are most
// efficiently passed by value.  
//
// The multiplicative inverse is hard, requiring the extended Euclidean algorithm
// (averages log2(prime) steps).
//
// Alternatively, if a primitive root (i.e. generator) can be found, its powers 
// will cycle through all the non-zero values of field.  This can be used to 
// generate log and anti-log tables.

template <unsigned P>
class gfp {
private:
	unsigned value;
public:
	// Constructor
	gfp() : value(0) {}
	// Copy constructors
	gfp(const int v) : value(v) { assert(value < gfp<P>::prime); }
	gfp(const gfp<P>& rhs) : value(rhs.value) {}
	// Copy assignment
	gfp<P> operator=(const gfp<P> rhs) {
		value = rhs.value;
		return *this;
	}
	// Unary negation
	gfp<P> operator-() const { return gfp<P>(0) - *this; }
	// Addition, subtraction and multiplication
	gfp<P> operator+=(const gfp<P>& rhs);
	gfp<P> operator-=(const gfp<P>& rhs);
	gfp<P> operator*=(const gfp<P>& rhs);
	gfp<P> operator/=(const gfp<P>& rhs) { assert(0); return *this; }	// unsupported
	// Comparisons
	friend int operator==(const gfp<P>& a, const gfp<P>& b) { return a.value == b.value; }
	friend int operator!=(const gfp<P>& a, const gfp<P>& b) { return a.value != b.value; }
	//
	static const unsigned prime;
	unsigned regular() const { return value; }
	//
	static void test();
	//
	NEWOPERATORS
};

template <unsigned P>
const unsigned 
gfp<P>::prime = P;

#ifdef _DEBUG
template <unsigned P>
std::ostream& operator<<(std::ostream&s, const gfp<P>& x) 
{
	std::cout << x.regular();
	return s;
}
#endif

template <unsigned P>
gfp<P>
operator +(const gfp<P>& a, const gfp<P>& b)
{
	gfp<P> c = (a.regular() + b.regular()) % gfp<P>::prime; 	return c;
}

template <unsigned P>
gfp<P>
gfp<P>::operator +=(const gfp<P>& rhs)
{
	value = (value + rhs.value) % gfp<P>::prime;	return *this;
}

template <unsigned P>
gfp<P>
operator -(const gfp<P>& a, const gfp<P>& b)
{
	unsigned v = a.regular();
	if (v >= b.regular())
		v -= b.regular();
	else
		v += gfp<P>::prime - b.regular();
	return v;				// invokes gfp<P>(int) constructor on return
}

template <unsigned P>
gfp<P>
gfp<P>::operator -=(const gfp<P>& rhs)
{
	if (value >= rhs.value)
		value -= rhs.value;
	else
		value += gfp<P>::prime - rhs.value;
	return *this;
}

template <unsigned P>
gfp<P>
operator *(const gfp<P>& a, const gfp<P>& b)
{
	gfp<P> c = (a.regular() * b.regular()) % gfp<P>::prime; 	return c;
}

template <unsigned P>
gfp<P>
gfp<P>::operator *=(const gfp<P>& rhs)
{
	gfp<P> c = (value * rhs.value) % gfp<P>::prime;		return *this;
}

template <unsigned P>
void
gfp<P>::test()
{
#ifdef _DEBUG
		using namespace std;
		// gfp<P>::prime assumed to be 7.
		gfp<P> a = 1, b = 6;
		a = b;
		cout << "Expect 6: " << a << endl;
		a = b = 5;
		cout << "Expect 5: " << a << endl;
		a = b + gfp<P>(3);
		cout << "Expect 1: " << a << endl;
		a += gfp<P>(6);
		cout << "Expect 0: " << a << endl;
		cout << "Expect 4: " << -gfp<P>(3) << endl;
#endif
}

} // namespace HoloStor
#endif // HOLOSTOR_HOLOSTORLIB_GFPRIME_HPP_
