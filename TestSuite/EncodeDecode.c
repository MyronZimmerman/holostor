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
	EncodeDecode.c

 Abstract:
	Regression test of HoloStor setup and encode/decode library calls:
		HoloStor_SessionCreate, HoloStor_SessionClose, 
		HoloStor_Encode, HoloStor_Decode, HoloStor_Rebuild,
		HoloStor_WriteDelta, HoloStor_EncodeDelta
	
	All combinations of allowed parameters and block failures within the
	specified range are tested using the specified test data. Performance
	metrics (CPU clock cycles per data byte) are presented.
	
*****************************************************************************/

#ifdef	__KERNEL__
#include <linux/kernel.h>
#include <linux/string.h>	// Needed for strncmp()
#include <linux/sched.h>	// Needed for cond_resched()
#include <asm/div64.h>		// Needed for do_div()
#else //!__KERNEL__
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#endif // __KERNEL__

#include "HoloStor.h"
#include "PentiumCycles.h"

#ifdef _MSC_VER
#pragma intrinsic (memcpy, memset, memcmp)
#endif

//
// SSE2 instructions require Data and ECC buffers be aligned on 16-byte
// boundaries.
// 
// NOTE: In general, align buffers of Cache Line sizes for best performance.
//
#ifdef _MSC_VER
#if MSC_VER >= 1300
#define	ALIGN16	_declspec(align(16))
#else	// !_MSC_VER >= 1300	// VC 6 does not support alignment and SSE2
#define ALIGN16
#endif	// _MSC_VER >= 1300
#else	// !_MSC_VER (GCC)
#define	ALIGN16 __attribute__ ((aligned(16)))
#endif	// _MSC_VER


//
// Primitive Types (augmenting those defined in HoloStor.h).
//
typedef char CHAR;
typedef char* PCHAR;
typedef int INT;
typedef int* PINT;
typedef long LONG;
typedef long* PLONG;
typedef unsigned long ULONG;
typedef unsigned long* PULONG;
typedef int INT32;
typedef unsigned int UINT32;
#ifdef _MSC_VER
typedef __int64 INT64;
# define INLINE __inline	// Microsoft specific for C (and C++)
#else
typedef long long INT64;
# define INLINE inline
#endif
typedef void VOID;
typedef void* PVOID;
typedef int BOOL;
#define	TRUE	1
#define	FALSE	0

#ifdef	__KERNEL__
#define	printf		printk		// Use DEFAULT_MESSAGE_LOGLEVEL for printf
#define	strncasecmp	strncmp		// Command line args will be case sensitive
#endif
#ifdef	_MSC_VER
#define strncasecmp	_strnicmp	// Linux/Windows "libc" difference
#pragma warning(disable:4996)	// C4996: sscanf: This function may be unsafe.
#endif

// In non-preemptive kernels, release the CPU so other processes can run.
#ifdef	__KERNEL__
#define preempt()	cond_resched()
#else
#define preempt()
#endif

//
// Local defines.
//
#define	MAX_BLOCKS	17			// Limited by the HoloStor 1.0 library
#define	MAX_BSIZE	65536		// 64 KB - a reasonable upper test limit


//
// Local structures.
//
typedef struct _COUNTER {		// Performance counter
	INT64	Bytes;
	INT64	Cycles;
	LONG	Calls;
	LONG	MinRate;
	LONG	MaxRate;
} COUNTER, *PCOUNTER;


//
// Local data.
//
ULONG	MinBsize	= 1024;		// Default test ranges (Command Line set-able)
ULONG	MaxBsize	= 1024;
ULONG	MinData		= 14;
ULONG	MaxData		= 14;
ULONG	MinEcc		= 3;
ULONG	MaxEcc		= 3;
ULONG	TestData	= 0;
ULONG	Cache		= 0;
ULONG	Verbose		= 1;
//
ULONG	Method		= ~0ul;		// Use the best method supported in HW

HOLOSTOR_CFG	Cfg;
PVOID	Group[MAX_BLOCKS];
LONG	CacheLine;
LONG	Failures;
LONG	Indexes[MAX_BLOCKS];
COUNTER	EncodeCounter[MAX_BLOCKS];
COUNTER	DecodeCounter[MAX_BLOCKS];
COUNTER	WriteDeltaCounter;
COUNTER	EncodeDeltaCounter;

ALIGN16 CHAR Buffers[MAX_BLOCKS * MAX_BSIZE];
ALIGN16 CHAR Data[MAX_BLOCKS * MAX_BSIZE];
ALIGN16 CHAR Data2[MAX_BLOCKS * MAX_BSIZE];
ALIGN16 CHAR Delta[MAX_BSIZE], NewEcc[MAX_BSIZE];


//
// Local functions -- starting with assembly language assist routines.
//
// Get the CacheLine size for this processor -- processors without CLFLUSH
// support return 0.
//
#ifdef _MSC_VER
#pragma warning(disable:4035)
__inline int GetCacheLineSize(VOID)
{
	__asm	mov		eax, 1			// Use CPUID to get processor info
	__asm	cpuid
	__asm	mov		eax, ebx		// Isolate the CFLUSH line size bits
	__asm	shr		eax, 8
	__asm	and		eax, 0xFF
	__asm	imul	eax, 8			// Scale to bytes
}
#pragma warning(default:4035)
#else	// !_MSC_VER (GCC)
inline int GetCacheLineSize(VOID)
{
	unsigned _eax, _ebx, _ecx, _edx;
	__asm__ __volatile__(
	    "cpuid"
	    : "=a" (_eax), "=b" (_ebx), "=c" (_ecx), "=d" (_edx) /* output */
	    : "a" (1)                                            /* input */
	);
	return ((_ebx >> 8) & 0xFF) * 8;
}
#endif	// _MSC_VER

//
// Read processor clock cycles.
//
INLINE INT64 ReadProcessorClock(VOID) { return PentiumCycles(); }

//
// Divide a 32-bit divisor into a 64-bit dividend.
//
INT64 div64by32( INT64 dividend, INT32 divisor)
{
#ifdef	__KERNEL__
	INT64 ll = dividend;
	do_div(ll, divisor);
	return ll;
#else
	return dividend / divisor;
#endif
}

//
// Return a uniform deviate (i.e. random number) in the range 0 to 2**32-1.
// Based on ranqd1 of Numerical Recipes in C 2nd Ed.
//
UINT32 rand32(VOID)
{
	static UINT32 idum = 0;			// seed
	idum = 1664525U*idum+1013904223U;
	return idum;
}

//
// Get a command line parameter and validate limits.  If the argument is
// not a match return FALSE so other parameters can be tried.  If the
// argument matches but either the syntax is incorrect or the limits are
// exceeded, also return FALSE.  Since no other parameters will match, the
// error will be caught as an invalid argument. 
//
BOOL
GetParameter( PULONG lpValue, PCHAR lpArg, PCHAR lpParam, ULONG Min, ULONG Max)
{
	ULONG	l;

	l = strlen(lpParam) - 3;	// exclude "%lu" or "%lx" suffix
	if (strncasecmp(lpArg, lpParam, l)!=0)
		return FALSE;			// not a match

	if (sscanf( &lpArg[l], &lpParam[l], &l)!=1)
		return FALSE;			// bad syntax

	if (l<Min || l>Max)
	{
		printf("Parameter %s must be >= %lu and <= %lu\n", lpArg, Min, Max);
		return FALSE;			// limits exceeded
	}

	*lpValue = l;

	return TRUE;				// success
}

//
// Prepare the cache state as specified by the "Cache" flag.
//
VOID
PrepareCacheState(VOID)
{
	PCHAR	lp;
	volatile LONG l;

	switch (Cache)
	{
	case 0:				// No cache flushing (warm cache case)
		break;

	case 1:				// Fill the cache with other data (dirty cache case)
		for (lp = &Data[MAX_BLOCKS*MAX_BSIZE]; lp>=Data; lp -= CacheLine)
			l = *lp;
		break;

	case 2:				// Flush and invalidate the cache (cold cache case)
		for (lp = &Buffers[MAX_BLOCKS*MAX_BSIZE]; lp>=Buffers; lp -= CacheLine)
#ifdef _MSC_VER
			__asm
			{
				mov		eax, lp
				_emit	0x0F
				_emit	0xAE
				_emit	0x38	// clflush [eax] -- not an instruction on VC 6
			}
#else
			__asm__("clflush	%0" : : "m" (*(lp)));
#endif
		break;
	}
}

//
// Update the specified counter.
//
VOID
UpdateCounter( PCOUNTER lpCounter, UINT32 lBytes, INT64 i64StartTime)
{
	INT64	delta;
	INT32	rate;

	delta = ReadProcessorClock() - i64StartTime;
	lpCounter->Cycles += delta;
	lpCounter->Bytes += lBytes;
	lpCounter->Calls++;

	//
	// Save min and max rates.
	//
	rate = (INT32) div64by32(delta*1000, lBytes);

	if (lpCounter->MinRate==0)
		lpCounter->MinRate = rate;

	if (rate<lpCounter->MinRate)
		lpCounter->MinRate = rate;

	if (rate>lpCounter->MaxRate)
		lpCounter->MaxRate = rate;

	// Done with a measurement - let other processes run.
	preempt();
}

//
// Print the specified counter.
//
VOID
PrintCounter( PCOUNTER lpCounter)
{
	LONG	avg;

	if (lpCounter->Calls==0)
	{
		printf("** Counter was never used **\n");
		return;
	}

	// XXX - lpCounter->Bytes assumed < 4-GBytes
	avg = (LONG) div64by32(lpCounter->Cycles*1000, (LONG)lpCounter->Bytes);

	printf("Min=%ld.%03ld, Max=%ld.%03ld, Avg=%ld.%03ld, Calls=%ld\n",
		lpCounter->MinRate / 1000, lpCounter->MinRate % 1000,
		lpCounter->MaxRate / 1000, lpCounter->MaxRate % 1000,
		avg / 1000, avg % 1000, lpCounter->Calls);
}


//
// MAIN
//
INT
main( INT argc, PCHAR argv[])
{
	UINT32*	lp;
	ULONG	bsize, ndata, necc, count;
	LONG status;
	HOLOSTOR_SESSION hSession;
	unsigned mask;
	INT64	time;
	ULONG	i, j;

	//
	// Parse command line parameters (if any).
	//
	for (i = 1; i<(ULONG)argc; i++)
	{
		if (GetParameter( &MinBsize, argv[i], "MinBsize=%lu", 64, MAX_BSIZE))
			continue;

		if (GetParameter( &MaxBsize, argv[i], "MaxBsize=%lu", 64, MAX_BSIZE))
			continue;

		if (GetParameter( &MinData, argv[i], "MinData=%lu", 1, MAX_BLOCKS))
			continue;

		if (GetParameter( &MaxData, argv[i], "MaxData=%lu", 1, MAX_BLOCKS))
			continue;

		if (GetParameter( &MinEcc, argv[i], "MinEcc=%lu", 1, MAX_BLOCKS-1))
			continue;

		if (GetParameter( &MaxEcc, argv[i], "MaxEcc=%lu", 1, MAX_BLOCKS-1))
			continue;

		if (GetParameter( &TestData, argv[i], "TestData=%lx", 0, 0xFFFFFFFF))
			continue;

		if (GetParameter( &Cache, argv[i], "Cache=%lu", 0, 2))
			continue;

		if (GetParameter( &Verbose, argv[i], "Verbosity=%lu", 0, 2))
			continue;

		if (GetParameter( &Method, argv[i], "Method=%lu", 0, 2))
			continue;

		if (strcmp(argv[i], "/?")!=0)
			printf("Invalid argument: %s\n", argv[i]);

		printf("Usage: EncodeDecode [/?] [MinBsize=# MaxBsize=# MinData=# MaxData=#\n");
		printf("       MinEcc=# MaxEcc=# Verbosity=0-2 TestData=X,0(random)\n");
		printf("       Cache=0(warm),1(dirty),2(flush)\n");
		printf("       Method=0(std),1(mmx),2(sse2)]\n");
		return 1;
	}

	//
	// Strip low bits to force Bsize values to 2**N values.
	//
	i = MinBsize;
	while ((i >>= 1)!=0)
		MinBsize &= ~i;

	i = MaxBsize;
	while ((i >>= 1)!=0)
		MaxBsize &= ~i;

	//
	// Get CacheLine size for this processor and check if CLFLUSH is supported.
	//
	CacheLine = GetCacheLineSize();

	if (CacheLine==0)
	{
		//
		// Must be a Pentium III or lower -- assume 32 byte cache line size.
		//
		CacheLine = 32;

		if (Cache==2)
		{
			printf("Warning: Cache line flushing not supported on this processor.\n");
			printf("         Switching to cache dirtying.\n");
			Cache = 1;
		}
	}

	//
	// Display test parameters.
	//
	printf("HoloStor Encode/Decode/Rebuild Tests 1.1 \n");
	printf("Block Size = %ld to %ld; Data Blocks = %ld to %ld; ECC Blocks = %ld to %ld\n",
		MinBsize, MaxBsize, MinData, MaxData, MinEcc, MaxEcc);
	printf("CacheLine = %ld, Test Data = ", CacheLine);
	if (TestData==0)
		printf("0(random)");
	else
		printf("%08lX", TestData);
	printf(", Cache = %s\n", Cache==0 ? "0(warm)" : Cache==1 ? "1(dirty)" : "2(flush)" );

	{
		unsigned method = Method;
		HoloStor_SetMethod(&method);
		printf("Method = %u (%s constrained)\n", method,
			(Method==~0ul)?"HW":"user");
	}

	//
	// Initialize test data.
	//
	for (lp = (UINT32*)Data; lp < (UINT32*)&Data[MAX_BLOCKS * MAX_BSIZE]; )
		*lp++ = TestData==0 ? rand32() : TestData;

	for (lp = (UINT32*)Data2; lp < (UINT32*)&Data2[MAX_BLOCKS * MAX_BSIZE]; )
		*lp++ = TestData==0 ? rand32() : ~TestData;

	//
	// Loop through block sizes, data block and ECC block counts.
	//
	for (bsize=MinBsize; bsize<=MaxBsize; bsize *= 2)
	for (ndata=MinData; ndata<=MaxData; ndata++)
	for (necc=MinEcc; necc<=MaxEcc; necc++)
	{
		//
		// Limit tests to maximum number of blocks allowed by the library.
		//
		if (ndata+necc>MAX_BLOCKS)
			continue;

		//
		// Initialize session for this configuration.
		//
		Cfg.BlockSize = bsize;
		Cfg.DataBlocks = ndata;
		Cfg.EccBlocks = necc;

		hSession = HoloStor_CreateSession( &Cfg);
		if (hSession<0)
		{
			printf("Error: HoloStor_CreateSession=%d; Bsize=%ld, Data=%ld, Ecc=%ld\n",
				hSession, bsize, ndata, necc);
			return 10;
		}

		//
		// Copy data to buffers, setup the group list, and encode ECC blocks.
		//
		memcpy( Buffers, Data, ndata*bsize);

		for (i=0; i<ndata+necc; i++)
			Group[i] = &Buffers[i * bsize];

		PrepareCacheState();

		time = ReadProcessorClock();

		status = HoloStor_Encode( hSession, Group);
		if (status<0)
		{
			printf("Error: HoloStor_Encode=%ld; Bsize=%ld, Data=%ld, Ecc=%ld\n",
				status, bsize, ndata, necc);
			return 11;
		}

		UpdateCounter( &EncodeCounter[necc], ndata*bsize, time);

		//
		// Loop through and apply the deltas to all data blocks to Data2 values.
		//
		for (i=0; i<ndata; i++)
		{
			PrepareCacheState();

			time = ReadProcessorClock();
			HoloStor_WriteDelta( hSession, Group[i], &Data2[i*bsize], Delta);
			UpdateCounter( &WriteDeltaCounter, bsize, time);

			memcpy( Group[i], &Data2[i*bsize], bsize);

			for (j=0; j<necc; j++)
			{
				PrepareCacheState();

				time = ReadProcessorClock();
				HoloStor_EncodeDelta( hSession, i, Delta, j, Group[ndata+j], NewEcc);
				UpdateCounter( &EncodeDeltaCounter, bsize, time);

				memcpy( Group[ndata+j], NewEcc, bsize);
			}
		}

		//
		// Loop through failing all possible Data/ECC block combinations.
		//
		// NOTE:
		// Number of combinations = Sum of (N+K)!/(N+K-i)!/i! over i=1 to K
		//
		for (i=0; i<necc; i++)
			Indexes[i] = -1;	// Initialize all indexes to no block failures

		for (;;)
		{
			//
			// Increment indexes to the next failure combination.
			//
			for (i=0; i<necc; i++)
				if (++Indexes[i] < (LONG)(ndata+necc-i))
					break;

			if (i==necc)		// Stop if the highest index has overflowed
				break;

			for ( ; i>0; i--)	// Reset overflowed indexes overflowed (if any)
				Indexes[i-1] = Indexes[i] + 1;

			//
			// Construct mask and clear data for failed blocks in this
			// combination.
			//
			mask = 0;
			count = 0;
			for (i=0; i<necc; i++)
			{
				LONG k = Indexes[i];
				if (k>=0)
				{
					count++;
					memset( Group[k], 0, bsize);
					mask |= 1 << k;
				}
			}

			if (Verbose>1)
				printf("Failure block mask 0x%x;  Bsize=%ld, Data=%ld, Ecc=%ld\n",
					mask, bsize, ndata, necc);

			//
			// Use Decode if only data blocks have failed,
			// otherwise rebuild all.
			//
			PrepareCacheState();

			time = ReadProcessorClock();

			if (mask < (ULONG)(1 << ndata))
				status = HoloStor_Decode( hSession, Group, mask);
			else
				status = HoloStor_Rebuild( hSession, Group, mask, -1);

			if (status<0)
			{
				printf("Error: Recover/Rebuild on mask 0x%x;  Bsize=%ld, Data=%ld, Ecc=%ld\n",
					mask, bsize, ndata, necc);
				return 12;
			}

			UpdateCounter( &DecodeCounter[count], ndata*bsize, time);
		}

		//
		// Confirm that final recovered data matches original data.
		//
		if (memcmp( Buffers, Data2, ndata*bsize) != 0)
		{
			printf("Error: Recovered data mismatch; Bsize=%ld, Data=%ld, Ecc=%ld\n",
				bsize, ndata, necc);

			Failures++;
		} else
			if (Verbose>1)
				printf("Passed case: Bsize=%ld, Data=%ld, Ecc=%ld\n",
					bsize, ndata, necc);

		//
		// Close this session.
		//
		status = HoloStor_CloseSession( hSession);
		if (status<0)
		{
			printf("Error: HoloStor_CloseSession=%ld; Bsize=%ld, Data=%ld, Ecc=%ld\n",
				status, bsize, ndata, necc);
			return 13;
		}

		// Done with a pass - let other processes run.
		preempt();
	}

	printf("All tests have completed with %ld failure cases.\n", Failures);

	//
	// Print out final performance metrics.
	//
	printf("\nPerformance Metrics (CPU clock cycles per byte) are:\n");
	for (i=MinEcc; i<=MaxEcc; i++)
	{
		printf("   Encode[%ld]: ", i);
		PrintCounter( &EncodeCounter[i]);
	}
	for (i=1; i<=MaxEcc; i++)
	{
		printf("   Decode[%ld]: ", i);
		PrintCounter( &DecodeCounter[i]);
	}

	printf(" Write Delta: ");
	PrintCounter( &WriteDeltaCounter);

	printf("Encode Delta: ");
	PrintCounter( &EncodeDeltaCounter);

	return Failures==0 ? 0 : 2;
}
