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
	HoloStor.h

 Abstract:
	Interface to the HoloStor library.
	
--****************************************************************************/

#ifndef _HOLOSTOR_H_
#define _HOLOSTOR_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define HOLOSTOR_VERSION		"1.0.4"
#define HOLOSTOR_VERSION_NUM	0x010004

#define HOLOSTORAPI

#define IN		// attribute indicating a parameter is passed into the API
#define OUT		// attribute indicating a parameter is returned from the API

typedef struct _HOLOSTOR_CFG {
  unsigned int	BlockSize;			// Block size in bytes
  unsigned int	DataBlocks;			// Data blocks per reliability group
  unsigned int	EccBlocks;			// Redundancy blocks per reliability group
} HOLOSTOR_CFG;

typedef int HOLOSTOR_SESSION;

// Function return values
#define HOLOSTOR_STATUS_SUCCESS				(0)		// or positive
#define HOLOSTOR_STATUS_INVALID_PARAMETER	(-1)
#define HOLOSTOR_STATUS_BAD_CONFIGURATION	(-2)
#define HOLOSTOR_STATUS_NO_MEMORY			(-3)
#define HOLOSTOR_STATUS_TOO_MANY_BAD_BLOCKS	(-4)
#define HOLOSTOR_STATUS_BAD_SESSION			(-5)
#define HOLOSTOR_STATUS_MISALIGNED_BUFFER	(-6)
#define HOLOSTOR_STATUS_TOO_MANY_SESSIONS	(-7)

HOLOSTORAPI HOLOSTOR_SESSION
HoloStor_CreateSession(
  const HOLOSTOR_CFG*	lpConfiguration
  );

HOLOSTORAPI int
HoloStor_CloseSession(
  IN HOLOSTOR_SESSION	hSession
  );

HOLOSTORAPI int
HoloStor_Encode(
  IN HOLOSTOR_SESSION	hSession,
  IN OUT void**		lpBlockGroup		// IN Data; OUT all ECC
  );

HOLOSTORAPI int
HoloStor_Decode(
  IN HOLOSTOR_SESSION	hSession,
  IN OUT void**		lpBlockGroup,		// IN Data & ECC; OUT missing data
  IN unsigned int	uInvalidBlockMask	// Mask of buffers with invalid data
  );

HOLOSTORAPI int
HoloStor_Rebuild(
  IN HOLOSTOR_SESSION	hSession,
  IN OUT void**		lpBlockGroup, 		// IN Data & ECC; OUT as specified
  IN unsigned int	uInvalidBlockMask,	// Mask of buffers with invalid data
  IN int			lWhichBlock			// Block index to rebuild (-1 all)
  );

HOLOSTORAPI int
HoloStor_WriteDelta(
  IN HOLOSTOR_SESSION	hSession,
  IN const void*	lpDataBlockOld,		// Data block before updating
  IN const void*	lpDataBlockNew,		// New contents of the data block
  OUT void*			lpDeltaBlock		// Delta for forwarding to ECC's
  );

HOLOSTORAPI int
HoloStor_EncodeDelta(
  IN HOLOSTOR_SESSION	hSession,
  IN unsigned int	lDataIndex,			// Data block index of delta
  IN const void*	lpDeltaBlock,		// Forwarded data delta
  IN unsigned int	lEccIndex,			// ECC block index being updated
  IN const void*	lpEccBlockOld,		// Old ECC block
  OUT void*			lpEccBlockNew		// Returned new ECC block
  );

// Force the library to use a sub-optimal method (for testing ONLY).
// Method 0 is always supported; higher values provide higher performance.
// Input a numerical method limit and the largest limited value supported
// in HW is returned.
HOLOSTORAPI int
HoloStor_SetMethod(
  IN OUT unsigned int*	pMethod			// opaque method id (see source code)
  );

#ifdef  __cplusplus
}
#endif

#endif	// _HOLOSTOR_H_
