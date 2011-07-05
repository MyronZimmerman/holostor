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
	Config.h

 Abstract:
	Configuration parameters for HoloStor library.

--****************************************************************************/
#ifndef HOLOSTOR_HOLOSTORLIB_CONFIG_H_
#define HOLOSTOR_HOLOSTORLIB_CONFIG_H_

enum CpuTypes {
	CPU_STD =  0u,		// CPU supports only standard 32-bit instructions
	CPU_MMX =  1u,		// CPU supports MMX (64-bit) instructions
	CPU_SSE2 = 2u,		// CPU supports SSE2 (128-bit) instructions
	CPU_UNKNOWN = ~0u	// CPU support is unknown
};

namespace HoloStor {
extern unsigned CpuType;
}

#define	HYPERWORD_SIZE	4		// Longs (32-bit) per hyperword (1,2 or 4)
#define	ELEMENT_WIDTH	4		// Hyperwords per element - Do not change!

const unsigned MaxSessions = 20;	// maximum simultaneous open sessions
//
const unsigned MinK = 1;		// minimum  ECC nodes supported by the library
const unsigned MaxK = 4;		// maximum  ECC nodes supported by the library
const unsigned MinN = 1;		// minimum Data nodes supported by the library
const unsigned MaxN = 16;		// maximum Data nodes supported by the library

// Workaround for GCC 3.3.1 (i686-pc-cygwin) / 3.3.2 (i686-pc-linux-gnu) bug -
// if CLASS::operator new[](size_t) returns 0, then ptr = new CLASS[n]
// returns 4 (and not 0 as it should).
#if __GNUC__==3 && __GNUC_MINOR__==3 && __GNUC_PATCHLEVEL__<=2
#define	WORKAROUND1(ptr)	if ((void*)(ptr) == (void*)4) ptr = 0;
#else
#define	WORKAROUND1(ptr)
#endif
// Workaround for a GCC 3.2.2 (i386-redhat-linux) bug -
// if CLASS::operator new[](size_t) returns 0, the ctor is called because
// the decision to call the ctor is erroneously based on return value + 4.
#if __GNUC__==3 && __GNUC_MINOR__==2
#define	WORKAROUND2(ptr)	if (ptr == 0) ptr = (void*)-4;
#else
#define	WORKAROUND2(ptr)
#endif

// Redefine the implementation of the class new/delete operators to:
//	- remove dependence on libstdc++.a (Linux application only)
//	- use kernel allocator (Linux kernel only)
//	- eliminate exceptions
#include <stdlib.h>
#include "Platform.h"
#define	NEWOPERATORS \
void * operator new(size_t size) { return HoloStor_QuickAlloc(size); }		\
void   operator delete(void * p, size_t size) { HoloStor_QuickFree(p); }	\
void * operator new[](size_t size) {					\
	void *ptr = HoloStor_TableAlloc(size);				\
	WORKAROUND2(ptr);	/* XXX - GCC 3.2.2 bug workaround */	\
	return ptr;							\
}									\
void   operator delete[](void* p, size_t size) { HoloStor_TableFree(p); }

#endif	// HOLOSTOR_HOLOSTORLIB_CONFIG_H_
