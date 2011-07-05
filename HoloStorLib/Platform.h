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
	Platform.h

 Abstract:
	Interface to platform support routines in C.

	Two types of memory allocators are assumed:
	1) HoloStor_Quick{Alloc,Free} for small frequent allocations, and
	2) HoloStor_Table{Alloc,Free} for large (multi-page) infrequent
	allocations. 

	The Quick allocator is used by default.  The Table allocator is
	used for infrequent allocations of 1-page or more of memory.  Both
	allocators should return memory aligned on a cache line boundary
	to reduce run-to-run jitter.
	
--****************************************************************************/

#ifndef HOLOSTOR_HOLOSTORLIB_PLATFORM_H_
#define HOLOSTOR_HOLOSTORLIB_PLATFORM_H_

#ifdef  __cplusplus
extern "C" {
#endif

extern void* HoloStor_QuickAlloc(unsigned int size);
extern void  HoloStor_QuickFree(void *p);
extern void* HoloStor_TableAlloc(unsigned int size);
extern void  HoloStor_TableFree(void *p);

#ifdef  __cplusplus
}
#endif
#endif // HOLOSTOR_HOLOSTORLIB_PLATFORM_H_
