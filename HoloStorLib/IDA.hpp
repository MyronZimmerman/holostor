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
	IDA.hpp

 Abstract:
	Interface to the IDA class.

--****************************************************************************/

#ifndef HOLOSTOR_HOLOSTORLIB_IDA_HPP_
#define HOLOSTOR_HOLOSTORLIB_IDA_HPP_

#include "HoloStor.h"
#include "Config.h"
#include "Types.h"
//
#include "Tuple.hpp"
#include "TypesGF.hpp"

namespace HoloStor {

class IDA {
private:
	matrixGFQ_t m_mEncode;
public:
	// constructor
	IDA() { }
	bool IDAInit(unsigned n, unsigned k);
	bool GenerateCoding(Tuple faults, matrixGFQ_t& mCoding, UCHAR* rowsUsed);
	static matrixGFQ_t EncodeMatrix(unsigned m, unsigned n);	// XXX - public for access by UnitTest
	//
	NEWOPERATORS
};

} // namespace HoloStor
#endif // HOLOSTOR_HOLOSTORLIB_IDA_HPP_
