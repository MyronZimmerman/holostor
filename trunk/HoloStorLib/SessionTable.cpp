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
	SessionTable.cpp

 Abstract:
	Implementation of the SessionTable class.  SessionTable is a container for
	the Session class.  The primary requirement for this container is fast
	lookup. Lookup latency should also be uniform but this is not a requirement.
	
--****************************************************************************/

#include "SessionTable.hpp"

namespace HoloStor {

/*
 * Issues:
 *	 The container could either be fixed in size or dynamically expanded. 
 * Though there are dynamic expansion algorithms, these introduce non-uniform
 * latencies or require deallocation and locking to eliminate races.  Rather
 * than increase latency with locking, the table is kept fixed in size.
 */

//
// Wrapper for the CMPXCHG instruction. This performs the following as an
// atomic operation:
//   if (Comperand == *Destination) {
//       *Destination = Exchange;
//       return Comperand;
//   } else
//       return *Destination;
//
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4035)				// no return value
inline static PVOID 
InterlockedCompareExchangePointer(
  PVOID volatile* Destination,
  PVOID Exchange,
  PVOID Comperand)
{
	__asm {
        mov ecx,Destination
        mov eax,Comperand
        mov edx,Exchange
        lock cmpxchg [ecx],edx				// implicitly uses eax
    }
    // returns value in EAX
}
#pragma warning(pop)
#else	// !_MSC_VER (GCC)
inline static PVOID 
InterlockedCompareExchangePointer(
	PVOID volatile* Destination,
	PVOID Exchange,
	PVOID Comperand)
{
	PVOID	_x;

#ifdef __x86_64__
	__asm__ __volatile__(
		"lock cmpxchgq	%2,(%3)"
		: "=a" (_x) : "a" (Comperand), "r" (Exchange), "r" (Destination)
	);
#else
	__asm__ __volatile__(
		"lock cmpxchgl	%2,(%3)"
		: "=a" (_x) : "a" (Comperand), "r" (Exchange), "r" (Destination)
	);
#endif

	return _x;
}
#endif	// _MSC_VER

HOLOSTOR_SESSION
SessionTable::add(Session *pSession)
{
	for (unsigned i = 0; i < MaxSessions; i++) {
		if (m_table[i] != NULL)
			continue;
		// Warning: possible race among SessionTable::add()'s claiming it.
		PVOID prior = InterlockedCompareExchangePointer(
							(PVOID*)&m_table[i], pSession, NULL
					  );
		if (prior == NULL)
			return i;	// won the race
	}
	// No more room. 
	return HOLOSTOR_STATUS_TOO_MANY_SESSIONS;
}

Session * 
SessionTable::lookup(HOLOSTOR_SESSION hSession) const
{
	if (hSession < 0 || hSession >= (HOLOSTOR_SESSION)MaxSessions)
		return NULL;
	return m_table[hSession];
}

Session * 
SessionTable::remove(HOLOSTOR_SESSION hSession)
{
	if (hSession < 0 || hSession >= (HOLOSTOR_SESSION)MaxSessions)
		return NULL;
	Session *pSession = m_table[hSession];
	m_table[hSession] = NULL;
	return pSession;
}

} // namespace HoloStor
