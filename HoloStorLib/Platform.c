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
	Platform.c

 Abstract:
	Implementation of platform support routines in C.
	
--****************************************************************************/

#include "Platform.h"

#ifdef	__KERNEL__
#include <linux/slab.h>		// for kmalloc/kfree() - good for up to 128kb
#include <linux/vmalloc.h>	// for vmalloc/vfree()
//
void* HoloStor_QuickAlloc(unsigned int size) {
	return kmalloc(size, GFP_KERNEL);
}
void  HoloStor_QuickFree(void *p) { kfree(p); }
void* HoloStor_TableAlloc(unsigned int size) { return vmalloc(size); }
void  HoloStor_TableFree(void *p) { vfree(p); }

#else // !__KERNEL__
#include <stdlib.h>
//
void* HoloStor_QuickAlloc(unsigned int size) { return malloc(size); }
void  HoloStor_QuickFree(void *p) { free(p); }
void* HoloStor_TableAlloc(unsigned int size) { return malloc(size); }
void  HoloStor_TableFree(void *p) { free(p); }

#endif // __KERNEL__
