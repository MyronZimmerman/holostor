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
	MathUtils.cpp

 Abstract:
	Implementation of the Math functions.
	
--****************************************************************************/

#include "MathUtils.hpp"

namespace HoloStor {

// The number of permutations (without repetitions) of
// n objects taken r at a time.
unsigned permutations(unsigned n, unsigned r)	// n >= 0, r >= 0, n >= r
{
	unsigned product = 1;
	while (r-- > 0)
		product *= n--;
	return product;								// n!/(n-r)!
}

// The number of distinct combinations (without repetitions) of
// n objects taken r at a time.
unsigned combinations(unsigned n, unsigned r)	// n >= 0, r >=0, n >= r
{ 
	// To maximize the range before overflow, arrange the factors so that the 
	// smaller of r and n-r is used for the factorial in the denominator.
	unsigned nmr = n - r;
	if (r < nmr)
		return permutations(n, r)   / permutations(r, r);	  // [n!/(n-r)!]/r!
	else
		return permutations(n, nmr) / permutations(nmr, nmr); // [n!/r!]/(n-r)!
}

} // namespace HoloStor
