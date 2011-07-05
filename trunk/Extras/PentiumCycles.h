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
	PentiumCycles.h

 Abstract:
	Interface and implementation for the PentiumCycles().
	
--****************************************************************************/

#ifndef _PENTIUMCYCLES_H_INCLUDED_
#define _PENTIUMCYCLES_H_INCLUDED_

#ifdef _MSC_VER
// pcycles_t is signed because unsigned __int64 to double is not implemented
typedef __int64 pcycles_t;	// __int64 (and __inline) are Microsoft Specific
//
#pragma warning(push)
#pragma warning(disable:4035)	// no return value
static __inline pcycles_t PentiumCycles(void) { __asm rdtsc }
#pragma warning(pop)
#endif // _MSC_VER

#ifdef __GNUC__
//
typedef unsigned long long pcycles_t;			// ISO C99
//
#ifdef __x86_64__
// Need to work around a bug in 64-bit "=A".
static inline pcycles_t PentiumCycles(void) {
	unsigned int _a, _d;
	__asm__ __volatile__ ("rdtsc": "=a" (_a), "=d" (_d));
	return ((unsigned long)_a) | (((unsigned long)_d)<<32);
}
#else
static inline pcycles_t PentiumCycles(void) {
	pcycles_t _A;
	__asm__ __volatile__ ("rdtsc": "=A" (_A));
	return _A;
}
#endif
#endif // __GNUC__

#endif // _PENTIUMCYCLES_H_INCLUDED_
