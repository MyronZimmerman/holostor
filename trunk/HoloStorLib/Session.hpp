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
	Interface for the Session class.
	
--****************************************************************************/

#ifndef HOLOSTOR_HOLOSTORLIB_SESSION_HPP_
#define HOLOSTOR_HOLOSTORLIB_SESSION_HPP_

#include "HoloStor.h"
#include "Config.h"
#include "Types.h"
#include "CodingTable.hpp"

namespace HoloStor {

class Session {
private:
	HOLOSTOR_CFG m_config;
	CodingTable m_codes;
	UINT32 m_uAllMask;	// mask of all blocks
	UINT32 m_uDataMask;	// mask of Data blocks
	UINT32 m_uEccMask;	// mask of ECC blocks
public:
	// constructor
	Session();
	//
	int SessionInit(const HOLOSTOR_CFG* lpConfiguration);
	int Rebuild(UINT32 uInvalidBlockMask, UCHAR** lpBlockGroup, INT lWhichBlock) const;
	int EncodeDelta(unsigned lDeltaIndex, const UCHAR* lpDeltaBlock,
					unsigned lEccIndex,   const UCHAR* lpEccBlockOld, UCHAR* lpEccBlockNew) const;
	int WriteDelta(const UCHAR* lpDataBlockOld, const UCHAR* lpDataBlockNew, UCHAR* lpDeltaBlock) const;
	//
	UINT32 uEccBlockMask() const { return m_uEccMask; }
	//
	NEWOPERATORS
};

} // namespace HoloStor
#endif	// HOLOSTOR_HOLOSTORLIB_SESSION_HPP_
