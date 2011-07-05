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
	main.cpp

 Abstract:
	Implementation of the HoloStor library interface.
	
--****************************************************************************/

#include "HoloStor.h"
#include "Config.h"
#include "Types.h"
//
#include "Session.hpp"
#include "SessionTable.hpp"
//
static const char Copyright[] = " HoloStor " HOLOSTOR_VERSION
	" Copyright (C) 2003-2011 Thomas P. Scott and Myron Zimmerman ";
// Thwart removal of the above copyright by the optimizer.
static const void *CopyrightUse = ((void)CopyrightUse, Copyright);

namespace HoloStor {

unsigned int CpuType = CPU_UNKNOWN;

// Get the CPU Type (using assembly language assist).
unsigned int
GetCpuType(){
	unsigned int i;			// EDX extended feature flags
#ifdef	_MSC_VER
	__asm {
		xor		ebx,ebx		// Touch other registers used by CPUID so
		xor		ecx,ecx		// that the compiler knows side effects
		mov		eax,1
		cpuid
		mov		i,edx
	}
#else	// !_MSC_VER	(GCC)
	__asm__ __volatile__(
		"movl	$1,%%eax\n\t"
		"cpuid"
		: "=d" (i) : : "eax","ebx","ecx"
	);
#endif // _MSC_VER
	// First check for SSE2 feature bit (bit 26 in EDX)
	if ((i&(1<<26)) != 0)
		return CPU_SSE2;
	// If not, then check for MMX feature bit (bit 23)
	if ((i&(1<<23)) != 0)
		return CPU_MMX;
	return CPU_STD;
}

/*
 * To avoid reliance on the runtime system, global objects must not have
 * a constructor/destructor.  The SessionTable goes futher by being an 
 * aggregate [see the C++ ARM] so that it can be initialized with an
 * initializer-list.
 */
SessionTable sessions = { { 0 } };

} // namespace HoloStor

using namespace HoloStor;

HOLOSTORAPI HOLOSTOR_SESSION
HoloStor_CreateSession(
  const HOLOSTOR_CFG	*lpConfiguration
  )
{
	if (CpuType == CPU_UNKNOWN)
		CpuType = GetCpuType();

	Session *pSession = new Session;
	if (pSession == NULL)
		return HOLOSTOR_STATUS_NO_MEMORY;
	int eStatus = pSession->SessionInit(lpConfiguration);
	if (eStatus != HOLOSTOR_STATUS_SUCCESS) {
		delete pSession;
		return eStatus;
	}
	HOLOSTOR_SESSION hSession = sessions.add(pSession);
	if (hSession < 0)
		delete pSession;
	return hSession;
}

HOLOSTORAPI INT
HoloStor_CloseSession(
  IN HOLOSTOR_SESSION	hSession
  )
{
	Session *pSession = sessions.remove(hSession);
	if (pSession == NULL)
		return HOLOSTOR_STATUS_BAD_SESSION;
	delete pSession;
	return HOLOSTOR_STATUS_SUCCESS;
}

HOLOSTORAPI INT
HoloStor_Encode(
  IN HOLOSTOR_SESSION	hSession,
  IN OUT PVOID *	lpBlockGroup	// IN Data; OUT all ECC
  )
{
	Session *pSession = sessions.lookup(hSession);
	if (pSession == NULL)
		return HOLOSTOR_STATUS_BAD_SESSION;
	return pSession->
		Rebuild(pSession->uEccBlockMask(), (UCHAR**)lpBlockGroup, -1);
}

HOLOSTORAPI INT
HoloStor_Decode(
  IN HOLOSTOR_SESSION	hSession,
  IN OUT PVOID *	lpBlockGroup,	// IN Data & ECC; OUT missing data
  IN UINT		uInvalidBlockMask	// Mask of buffers with invalid data
  )
{
	Session *pSession = sessions.lookup(hSession);
	if (pSession == NULL)
		return HOLOSTOR_STATUS_BAD_SESSION;
	return pSession->
		Rebuild(uInvalidBlockMask, (UCHAR**)lpBlockGroup, -1);
}

HOLOSTORAPI INT
HoloStor_Rebuild(
  IN HOLOSTOR_SESSION	hSession,
  IN OUT PVOID *	lpBlockGroup, 	// IN Data & ECC; OUT as specified
  IN UINT		uInvalidBlockMask,	// Mask of buffers with invalid data
  IN INT		lWhichBlock			// Block index to rebuild (-1 all)
  )
{
	Session *pSession = sessions.lookup(hSession);
	if (pSession == NULL)
		return HOLOSTOR_STATUS_BAD_SESSION;
	return pSession->
		Rebuild(uInvalidBlockMask, (UCHAR**)lpBlockGroup, lWhichBlock);
}

HOLOSTORAPI INT
HoloStor_WriteDelta(
  IN HOLOSTOR_SESSION	hSession,
  IN const void *	lpDataBlockOld,		// Data block before updating
  IN const void *	lpDataBlockNew,		// New contents of the data block
  OUT void *		lpDeltaBlock		// Delta for forwarding to ECC's
  )
{
	Session *pSession = sessions.lookup(hSession);
	if (pSession == NULL)
		return HOLOSTOR_STATUS_BAD_SESSION;
	return pSession->WriteDelta((const UCHAR*)lpDataBlockOld,
								(const UCHAR*)lpDataBlockNew,
								      (UCHAR*)lpDeltaBlock);
}

HOLOSTORAPI INT
HoloStor_EncodeDelta(
  IN HOLOSTOR_SESSION	hSession,
  IN UINT			lDataIndex,			// Data block index of delta
  IN const void *	lpDeltaBlock,		// Forwarded data delta
  IN UINT			lEccIndex,			// ECC block index being updated
  IN const void *	lpEccBlockOld,		// Old ECC block
  OUT void *		lpEccBlockNew		// Returned new ECC block
  )
{
	Session *pSession = sessions.lookup(hSession);
	if (pSession == NULL)
		return HOLOSTOR_STATUS_BAD_SESSION;
	return pSession->EncodeDelta(lDataIndex, (const UCHAR*)lpDeltaBlock,
								 lEccIndex,  (const UCHAR*)lpEccBlockOld,
								                   (UCHAR*)lpEccBlockNew);
}

HOLOSTORAPI INT
HoloStor_SetMethod(
  IN OUT UINT* pMethod
  )
{
	if (pMethod == NULL)
		return HOLOSTOR_STATUS_INVALID_PARAMETER;
	if (CpuType == CPU_UNKNOWN)
		CpuType = GetCpuType();
	if (*pMethod < CpuType)
		CpuType = *pMethod;			// limit support
	*pMethod = CpuType;
	return HOLOSTOR_STATUS_SUCCESS;
}
