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
	Session.hpp

 Abstract:
	Interface for the SessionTable class.
	
--****************************************************************************/

#ifndef HOLOSTOR_HOLOSTORLIB_SESSIONTABLE_HPP_
#define HOLOSTOR_HOLOSTORLIB_SESSIONTABLE_HPP_

#include "HoloStor.h"
#include "Config.h"
#include "Types.h"
//
#include "Session.hpp"

namespace HoloStor {

// SessionTable must be an aggregate since it is used as a global object
// initialized by an initializer-list rather than by a constructor.

class SessionTable {
public:
	Session	*m_table[MaxSessions];		// pointer to array of session pointers
	//
	HOLOSTOR_SESSION add(Session *pSession);
	Session *lookup(HOLOSTOR_SESSION hSession) const;	// low, uniform latency
	Session *remove(HOLOSTOR_SESSION hSession);
	//
	NEWOPERATORS
};

} // namespace HoloStor
#endif	// HOLOSTOR_HOLOSTORLIB_SESSIONTABLE_HPP_
