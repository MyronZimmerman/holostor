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
	Types.h

 Abstract:
	Generally useful types.
	
--****************************************************************************/
#ifndef HOLOSTOR_HOLOSTORLIB_TYPES_H_
#define HOLOSTOR_HOLOSTORLIB_TYPES_H_

typedef char CHAR;					// primitive types
typedef unsigned char UCHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef int INT;
typedef unsigned int UINT;
typedef int INT32;
typedef unsigned int UINT32;
typedef long LONG;
typedef unsigned long ULONG;
typedef void* PVOID;
#ifdef __x86_64__
typedef unsigned long long UINT_PTR;
typedef long long INT_PTR;
#else
typedef unsigned int UINT_PTR;
typedef int INT_PTR;
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

typedef unsigned short	CodingIndex;	// identifies a CodingMatrix in the CodingTable
typedef	unsigned char	MatrixIndex;	// index into matrices

typedef struct { UINT32 basicword[HYPERWORD_SIZE]; } hyperword_t;
typedef struct { hyperword_t hyperword[ELEMENT_WIDTH]; } Element;

#endif	// HOLOSTOR_HOLOSTORLIB_TYPES_H_
