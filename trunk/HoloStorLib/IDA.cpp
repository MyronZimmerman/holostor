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
	IDA.cpp

 Abstract:
	Implementation of the IDA class.  Class IDA generates recovery matrices.
	
--****************************************************************************/

#include "IDA.hpp"

#ifdef _DEBUG
#include <iostream>
#endif
#include <assert.h>		// for ANSI assert()

namespace HoloStor {

bool 
IDA::IDAInit(unsigned n, unsigned k)
{
	if (n + k > gfQ::order + 1)
		return false;
	m_mEncode = EncodeMatrix(n+k, n);	// this can be nil if out of memory
	return true;
}

bool
IDA::GenerateCoding(Tuple faults, matrixGFQ_t& mCoding, UCHAR *rowsUsed)
{
	const unsigned n = m_mEncode.cols();
	const unsigned m = m_mEncode.rows();
	// Generate the row depricated matrix corresponding to the faults.
	// The rows are taken from the n lowest numbered valid rows.
	unsigned iDst, iSrc;
	matrixGFQ_t B(n,n);
	if ( B.isNil() || m_mEncode.isNil() )
		return false;					// out of memory
	unsigned uInvalidMask = faults.mask();
	for (iDst = 0, iSrc = 0; iSrc < m; iSrc++, uInvalidMask >>= 1) {
		if ( uInvalidMask&1 )
			continue;
		if ( iDst >= n )
			break;
		// copy row
		for (unsigned j = 0; j < n; j++)
			B(iDst,j) = m_mEncode(iSrc,j);
		rowsUsed[iDst] = iSrc;			// the identity of rows used
		iDst++;
	}
	if (iDst < n)
		return false;		// excessive faults to recover (shouldn't happen)
	
	// Compute the inverse, which will recover the data nodes.
	matrixGFQ_t C;
	if ( !B.inverse(C) )
		return false;		// logic error (shouldn't happen) or out of memory
	//
#ifdef	_DEBUG
	matrixGFQ_t I = C*B;
	if ( I.isNil() )
		return false;		// out of memory
	bool bMatrixOK = true;
	for (unsigned i = 0; i < I.rows(); i++)
		for (unsigned j = 0; j < I.cols(); j++)
			if ( I(i,j).regular() != (i==j ? 1 : 0) )
				bMatrixOK = false;
	if (!bMatrixOK) {
		std::cerr << "BAD Matrix inversion" << std::endl;
		B.print("B");
		C.print("C");
		I.print("I");
	}
#endif
	// Multiply by the m_mEncode to produce a matrix that will recover Data and ECC nodes.
	mCoding = m_mEncode * C;
	if ( mCoding.isNil() )
		return false;				// out of memory
#ifdef	_DEBUGx						// XXX - silenced until debug verbosity levels added
	m_mEncode.print("**mEncode**");
	faults.print("**faults**");
	C.print("**inverse**");
	mCoding.print("**coding**");
#endif
	return true;
}

// Return an MxN encoding matrix.  The matrix is systematic with parity and Cauchy
// elements.
matrixGFQ_t
IDA::EncodeMatrix(unsigned m, unsigned n)
{
	matrixGFQ_t A(m, n);
	if ( A.isNil() )
		return A;									// out of memory (return nil)
	const unsigned nCauchyStart = n + 1;			// starting row index of Cauchy rows
	const unsigned nCauchyRows = m - nCauchyStart;	// number of Cauchy rows
	assert(n + nCauchyRows <= gfQ::order);
	for (unsigned i = 0; i < m; i++) {
		for (unsigned j = 0; j < n; j++) {
			if (i < n) 
				A(i,j) = (i==j)?1:0;	// systematic
			else if (i == n)
				A(i,j) = 1;				// parity
			else if (i > n) {			// Cauchy rows
				gfQ x(i - nCauchyStart);// first nCauchyRows values of gfQ are for x
				gfQ y(j + nCauchyRows);	// next n values of gfQ are for y
				// XXX - GCC 3.2 generates bogus code for this next line when >= -O1
				// A(i,j) = gfQ(1) / (x + y);
				gfQ z = gfQ(1) / (x + y);
				A(i,j) = z;
			}
		}
	}
	return A;
}

} // namespace HoloStor
