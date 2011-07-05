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
	UnitTest.cpp

 Abstract:
	Unit test program for classes within the HoloStor library. These tests
	focus on validating assumptions of the compilation environment and on the 
	behavior of the objects comprising the library. Since the release build 
	of the library does not do any I/O, class instrumentation is of limited
	value (without a logging facility) and these tests are critical in tracking
	down optimization related problems.  Candidate tests for inclusion here are:
		* invocation of diagnostic methods defined within the classes (_DEBUG only)
		* performance measurements that support a choice of one design over
		another
		* validation of critical class methods (often based on inspection)
	
--****************************************************************************/

// XXX - Redefinition of new/delete in Config.h causes problems if 
// Config.h follows <iostream>.  Workaround: include "Config.h" first.
#include "Config.h"		// Needs to be before <iostream>
#include <iostream>
//
#include <string.h>		// for ANSI memset()
//
#include "HoloStor.h"
//
#include "UnitTest.hpp"
#include "Session.hpp"
//
#include "PentiumCycles.h"

// primitive types
typedef unsigned long ULONG;
typedef void* PVOID;
#ifdef __x86_64__
typedef unsigned long long UINT_PTR;
#else
typedef unsigned int UINT_PTR;
#endif

using namespace HoloStor;

#if	defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4035)	// no return value
//
// Perform a forward bit scan of the mask and return the bit index of
// the first bit on.
//
inline int
BitScan(unsigned int Mask) {
    __asm	bsf eax,Mask
}

inline unsigned int
BitReset(unsigned int Mask, int Bit)
{
	__asm {
		mov eax,Mask
		mov ecx,Bit
		btr eax,ecx		// return in EAX
	}
}

inline unsigned int
BitReset(unsigned int Mask)
{
	__asm {
		mov eax,Mask
		bsf	ecx,eax		// return in ECX
		btr eax,ecx		// return in EAX
	}
}
#pragma warning(pop)
#elif defined(__GNUC__)
inline int BitScan(unsigned int Mask) {
	int _x;
	__asm__("bsfl	%1,%0" : "=r" (_x) : "rm" (Mask));
	return _x;
}

inline unsigned int
BitReset(unsigned int Mask, int Bit) {
	unsigned int _x;
	__asm__("movl	%1,%0\n\tbtrl	%2,%0"
		: "+r" (_x) : "g" (Mask), "ri" (Bit));
	return _x;
}

inline unsigned int
BitReset(unsigned int Mask) {
	unsigned int _x;
	__asm__("movl	%1,%0\n\tbsfl	%0,%%ecx\n\tbtrl	%%ecx,%0"
		: "=r" (_x) : "g" (Mask) : "ecx" );
	return _x;
}
#else // !_MSC_VER
inline int
BitScan(unsigned int Mask)
{
	int i;
	for (i = 0; i < 32; i++)
		if (Mask & (1<<i))
			break;
	return i;
}

inline unsigned int
BitReset(unsigned int Mask, int Bit)
{
	return Mask & ~(1<<Bit);
}

inline unsigned int
BitReset(unsigned int Mask)
{
	return Mask & ~(1<<BitScan(Mask));
}
#endif // _MSC_VER

void pattern(char *buf, char val, int size)
{
	::memset(buf, val, size);
}

bool check(char *buf, char val, int size)
{
	for (int i = 0; i < size; i++)
		if (buf[i] != val)
			return false;
	return true;
}

// Class to display test start and end message and manage test output.
class Moniker {
private:
	const char* m_lpszName;		// test name
public:
	// constructor
	Moniker(const char* s) {
		m_lpszName = s;
		tag() << "*BEGIN*" << std::endl;
	}
	// destructor
	~Moniker() {
		tag() << "* END *" << std::endl << std::endl;
	}
	// return the output stream to use
	std::ostream&  out() const { return std::cout; }
	// write the test name and return the output stream
	std::ostream& tag() const { return out() << m_lpszName << " - "; }
};

// Aligned memory allocator
char *_AlignedAlloc(unsigned nBlkSize, unsigned nBlkAlignment)
{
	char *base = new char[nBlkSize+nBlkAlignment];
	//
	if (nBlkAlignment == 0)
		return base;									// no special alignment
	assert(nBlkAlignment >= sizeof(char*));
	// Round up to an aligned boundary, always leaving space below.
	char *pBlk = base + nBlkAlignment - ((UINT_PTR)base%nBlkAlignment);
	// Store the value returned by new.
	((char **)pBlk)[-1] = base;
	::memset(pBlk, 0, nBlkSize);						// zero memory
	return pBlk;
}

// Free memory allocated via _AlignedAlloc().
void _AlignedFree(char *pBlk, int nBlkAlignment)
{
	if (nBlkAlignment == 0) {
		delete [] pBlk;								// no special alignment
		return;
	}
	char *base = ((char **)pBlk)[-1];
	delete [] base;
}

//////////////////////////////////////////////////////////////////////
//
//	TestPredefines - Print predefined macros.
//
//////////////////////////////////////////////////////////////////////

void
TestPredefines()
{
	using namespace std;
	Moniker moniker("TestPredefines");
	moniker.tag() << "Important macros defined:" << endl;
#ifdef	_MFC_VER
	moniker.tag() << "_MFC_VER=" << _MFC_VER << endl;
#endif
#ifdef	_MSC_VER
	moniker.tag() << "_MSC_VER=" << _MSC_VER << endl;
#endif
#ifdef	__GNUC__
	moniker.tag() << "__GNUC__=" << __GNUC__ << endl;
#endif
#ifdef	_DEBUG
	moniker.tag() << "_DEBUG defined" << endl;
#endif
#ifdef	NDEBUG
	moniker.tag() << "NDEBUG defined" << endl;
#endif
}

//////////////////////////////////////////////////////////////////////
//
//	TestSizes - Print class sizes.
//
//////////////////////////////////////////////////////////////////////

#include "Tuple.hpp"
#include "SessionTable.hpp"
#include "Session.hpp"
#include "IDA.hpp"
#include "CombinIter.hpp"
#include "CodingTable.hpp"
#include "CodingMatrix.hpp"
#include "TypesGF.hpp"

#define	PS(x)	moniker.tag() << "sizeof(" #x ")=" << sizeof(x) << endl;

void
TestSizes()
{
	using namespace std;
	Moniker moniker("TestSizes");
	// Print sizes
	moniker.tag() << "Important class sizes:" << endl;
	PS(Tuple);
	PS(Session);
	PS(SessionTable);
	PS(IDA);
	PS(CombinIter);
	PS(CodingTable);
	PS(CodingMatrix);
	PS(gfQ);
	PS(matrixGFQ_t);
	PS(GF2Mul);
	PS(hyperword_t);
	PS(Element);
}

//////////////////////////////////////////////////////////////////////
//
//	Test4Leaks - Exercise some class creation/destruction.
//
//////////////////////////////////////////////////////////////////////

#include "Session.hpp"

void
Test4Leaks()
{
	using namespace std;
	Moniker moniker("Test4Leaks");
	//
	Session *pSession = new Session;
	moniker.tag() << "new Session" << endl;
	delete pSession;
}

//////////////////////////////////////////////////////////////////////
//
//	TestInterface - Perform simple Encode/Decode using HoloStor interfaces.
//
//////////////////////////////////////////////////////////////////////

void
TestInterface()
{
	using namespace std;
	Moniker moniker("TestInterface");
	pcycles_t time;
	unsigned i;
	int ret;
	HOLOSTOR_CFG cfg;
	cfg.BlockSize = sizeof(Element);	// minimum size
	cfg.DataBlocks = 2;
	cfg.EccBlocks = 1;
	//
	const unsigned M = cfg.DataBlocks + cfg.EccBlocks;
	char ** BlockGroup = new char* [M];
	for (i = 0; i < M; i++)
		BlockGroup[i] = _AlignedAlloc(cfg.BlockSize, 16);
	
	for (i = 0; i < M; i++)
		pattern(BlockGroup[i], '0'+i, cfg.BlockSize);		// initialize

	//
	HOLOSTOR_SESSION hSession = HoloStor_CreateSession(&cfg);
	moniker.tag() << "HoloStor_CreateSession returned " << hSession << endl;
	//
	time = PentiumCycles();
	ret = HoloStor_Encode(hSession, (PVOID*)BlockGroup);
	time = PentiumCycles() - time;
	moniker.tag() << "HoloStor_Encode " << (ULONG)time << " cycles" << endl;
	moniker.tag() << "HoloStor_Encode returned " << ret << endl;

	pattern(BlockGroup[0], 0, cfg.BlockSize);				// generate a fault
	//
	ret = HoloStor_Decode(hSession, (PVOID*)BlockGroup, (1<<0));
	moniker.tag() << "HoloStor_Decode returned " << ret << endl;
	//
	moniker.tag() << "HoloStor_Decode " << 
		(check(BlockGroup[0], '0'+0, cfg.BlockSize) ? "succeeded" : "*failed*") << endl;

	ret = HoloStor_CloseSession(hSession);
	moniker.tag() << "HoloStor_CloseSession returned " << ret << endl;
	//
	for (i = 0; i < M; i++)
		_AlignedFree(BlockGroup[i], 16);
	delete [] BlockGroup;
}

//////////////////////////////////////////////////////////////////////
//
//	TestCombinIter - Invoke the CombinIter self-test method.
//
//////////////////////////////////////////////////////////////////////

#include "CombinIter.hpp"

void
TestCombinIter()
{
	Moniker moniker("TestCombinIter");
	CombinIter iter;
	iter.Test();
}

//////////////////////////////////////////////////////////////////////
//
//	TestCodingHash - Exercise coding table hash.
//
//////////////////////////////////////////////////////////////////////

namespace HoloStor {
extern UINT32 Mask2Index(
            UINT32    Mask,               // Bit mask of invalid blocks
            UINT32    Blocks              // Number of blocks
            );
}

#include "Config.h"
#include "Tuple.hpp"
#include "CombinIter.hpp"
#include "CodingTable.hpp"

// Print the mask in binary.
char *
bin(unsigned u)
{
	const int MaxM = 17;			// maximum number of bits
	static char buffer[MaxM+1];		// +1 for '\0'
	int i = MaxM-1;
#if 0
	for (; i >= 0; i--)				// skip leading 0s
		if (u & (1<<i))
			break;
#endif
	char *cp = buffer;
	for (; i >= 0; i--)
		*cp++ = (u & (1<<i)) ? '1' : '0';
	*cp++ = '\0';
	return buffer;
}

void
TestCodingHash()
{
	using namespace std;
	const unsigned nBlocks = 5;					// test case (must be > MaxK)
	Moniker moniker("TestCodingHash");
	moniker.tag() << "nBlocks=" << nBlocks << endl;

	int nCases = 0;
	CombinIter iter;
	for (unsigned r = 0; r <= MaxK; r++) {
		unsigned max = CodingTable::_MaxHash(nBlocks-r,r);
		moniker.tag() << r << " faults out of " << nBlocks << " has max hash of " << max << endl;
		iter.CombinIterInit(nBlocks, r);
		Tuple tup;
		while (iter.Draw(tup)) {
			long mask = tup.mask();
			moniker.tag() << bin(mask) << " -> " << dec << Mask2Index(mask, nBlocks) << endl;
			nCases++;
		}
	}
	moniker.tag() << nCases-1 << " non-zero cases counted, " 
			  << CodingTable::_MatrixCount(nBlocks-MaxK,MaxK) << " cases expected" << endl;
}

//////////////////////////////////////////////////////////////////////
//
//	TestBench - Measure the performance of some key routines.
//
//////////////////////////////////////////////////////////////////////

void
TestBench()
{
	using namespace std;
	const unsigned nBlocks = 17;	// number of blocks for Mask2Index()
	unsigned mask = 0x11101;	// test case (4 bits out of 17)
	Moniker moniker("TestBench");
	moniker.tag() << "nBlocks=" << nBlocks << ", mask=" << bin(mask) << endl;
	pcycles_t time;
	while (mask) {
		unsigned n;
		moniker.tag() << "mask=" << bin(mask) << endl;
		//
		time = PentiumCycles();
		time = PentiumCycles() - time;
		moniker.tag() << "back-to-back timing: " << (int)time << " cycles" << endl;
		//
		time = PentiumCycles();
		n = Mask2Index(mask, nBlocks);
		time = PentiumCycles() - time;
		moniker.tag() << "Mask2Index:" << (int)time << " cycles" << endl;
		//
		time = PentiumCycles();
		n = BitScan(mask);
//		mask = BitReset(mask, n);
		mask = BitReset(mask);
		time = PentiumCycles() - time;
		moniker.tag() << "BitScan/Reset return " << n << ": " << (int)time << " cycles" << endl;
	}
}

//////////////////////////////////////////////////////////////////////
//
//	TestGF2Mul - Invoke GF2Mul::dump().
//
//////////////////////////////////////////////////////////////////////

void
TestGF2Mul()
{
	Moniker moniker("TestGF2Mul");
#if 0
	// The following code dumps the GF(2) multiplication operations for
	// diagnostic purposes.  GF2Mul::gf2 will need to be made public.
	for (int i = 0; i < 16; i++) {
		matrix<GF2Mul::gf2> mop = GF2Mul::multOp(gfQ(i));
		int nXORs = 0;
		for (int j = 0; j < 4; j++)
			for (int k =0; k < 4; k++)
				if (mop(j,k).regular() == 1)
					nXORs++;
		char label[16];
		sprintf(label, "%d has %d XORs", i, nXORs);
		mop.print(label);
	}
#endif
	GF2Mul::dump();
}

//////////////////////////////////////////////////////////////////////
//
//	TestGF - Compare results of operations over GF16 with gf2pow<4>.
//
//////////////////////////////////////////////////////////////////////

#include "GF16.hpp"

void
TestGF()
{
	using namespace std;
	Moniker moniker("TestGF");
	
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			// The following is a key value used in Cauchy matrices.
			if (i != j) {
				typedef GF16 gf;
				gf x(i), y(j);
				gf z = gf(1)/(x + y);
				// Now see if its valid
				gf r = gf(1);
				r /= z;						// should be x + y
				r -= x;						// should be y
				if (r != y)
					moniker.tag() << "bad calculation for i=" << i << ",j=" << j << endl;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
//	TestGFBench - Benchmark GF16.
//
//////////////////////////////////////////////////////////////////////


void
TestGFBench()
{
	using namespace std;
	Moniker moniker("TestGFBench");
	const int nCount = 1000;
	GF16 a = 1; GF16 c = 2;
	pcycles_t time = PentiumCycles();
	for (int i = 0; i < nCount; i++)
		a *= c;
	time = PentiumCycles() - time;
	moniker.tag() << (float)time/nCount << " cycles per multiply" << endl;
}

//////////////////////////////////////////////////////////////////////
//
//	TestMatrix - Exercise matrix inversion and multiplcation.
//
//////////////////////////////////////////////////////////////////////
#include "IDA.hpp"
#include "MathUtils.hpp"

void TestMatrix()
{
	using namespace std;
	Moniker moniker("TestMatrix");
	const int n = 13;				// test case
	const int k = 4;
	moniker.tag() << "n=" << n << ", k=" << k << endl;
	IDA ida;
	ida.IDAInit(n, k);
	moniker.tag() << "Attention! Visually inspect the following encoding matrix: " << endl;
	matrix<GF16> mx = IDA::EncodeMatrix(n+k, n);
	mx.print("IDA::Encode");		// silent unless Debug library
	//
	moniker.tag() << "Standby... Checking all possible combinations of submatrices. " << endl;
	pcycles_t time = PentiumCycles();
	CombinIter iter;
	iter.CombinIterInit(n+k, k);
	int nCases = 0;
	Tuple tup;						// Tuple limited to MaxK components.
	while (iter.Draw(tup)) {		// Iterate over rows to remove.
		nCases++;
		matrix<GF16> aa(n,n);
		int iDst = 0;
		for (UINT iSrc = 0; iSrc < n+k; iSrc++) {
			if (!tup.isMember(iSrc)) {				// row copy if not skipping
				for (int j = 0; j < n; j++)
					aa(iDst, j) = mx(iSrc, j);
				iDst++;
			}
		}
		//
		matrix<GF16> bb(n,n);
		if ( !aa.inverse(bb) ) {
			moniker.tag() << "inverse failed for the following matrix" << endl;
			aa.print();
			continue;
		}
		matrix<GF16> cc(n,n);
		cc = aa * bb;
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++)
				if ( cc(i,j).regular() != ((i == j) ? 1 : 0) ) {
					moniker.tag() << "the following should be an identity matrix" << endl;
					cc.print();
				}
	}
	time = PentiumCycles() - time;
	moniker.tag() << nCases << " tested, " << 
		             combinations(n+k, n) << " cases expected (time = " << 
					 (float)time << " cycles)" << endl;
}

//////////////////////////////////////////////////////////////////////
//
//	TestNilMatrix - Exercise Nil propagation in matrix 
//					inversion/multiplication.
//
//////////////////////////////////////////////////////////////////////

void TestNilMatrix()
{
	using namespace std;
	Moniker moniker("TestNilMatrix");
	matrix<int> a(1,1), b(1,1), c(1,1);
	a(0,0) = 2;
	b(0,0) = 3;
	c = a * b;
	if (c(0,0) != 6)
        moniker.tag() << "2*3 returned " << c(0,0) << " expected 6" << endl;
	// mark b as Nil and test for propagation through multiply and inverse
	b.setNil();
	c = a * b;
	if ( !c.isNil() )
		moniker.tag() << "bad multiply - expected Nil to be returned" << endl;
	if ( b.inverse(a) || !a.isNil() )
		moniker.tag() << "bad inverse - expected false && Nil to be returned" << endl;
}

void RunTests()
{
	TestPredefines();
	TestSizes();
	Test4Leaks();
	// Invoke self-test methods (silent unless Debug library).
	TestCombinIter();
	TestGF2Mul();
	//
	TestCodingHash();
	TestBench();
	TestGF();
	TestGFBench();
	TestNilMatrix();
	TestMatrix();
	TestInterface();
}
