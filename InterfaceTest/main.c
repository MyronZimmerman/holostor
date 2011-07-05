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
	main.c

 Abstract:
	Interface test program for the HoloStor library.
	
--****************************************************************************/

#ifdef	__KERNEL__
#include <linux/kernel.h>
#include <linux/sched.h>	// Needed for cond_resched()
#else // !__KERNEL__
#include <stdio.h>
#include <malloc.h>
#include <string.h>			// for memset()
#include <stdlib.h>			// for abort() & rand()
#include <assert.h>			// for assert()
#endif // __KERNEL__
//
#include "HoloStor.h"
//
#include "PentiumCycles.h"
//
#include "arith.h"

// primitive types
typedef unsigned long ULONG;
typedef void* PVOID;
#ifdef __x86_64__
typedef unsigned long long UINT_PTR;
#else
typedef unsigned int UINT_PTR;
#endif

#ifdef	_MSC_VER
#pragma warning(disable:4996)	// C4996: sscanf: This function may be unsafe
#endif

//////////////////////////////////////////////////////////////////////
//
//	Kernel support.
//
//////////////////////////////////////////////////////////////////////

#ifdef	__KERNEL__
#define printf		printk		// Use DEFAULT_MESSAGE_LOGLEVEL for printf
#define assert(x)
void * malloc(size_t);
void free(void *);
#endif

// In non-preemptive kernels, release the CPU so other processes can run.
#ifdef	__KERNEL__
#define preempt()	cond_resched()
#else
#define preempt()
#endif

//////////////////////////////////////////////////////////////////////
//
//	Configurables.
//
//////////////////////////////////////////////////////////////////////

int nMinBlockSize = 512;	// minimum block size supported by the library
int nTypBlockSize = 1024;	// typical block size to benchmark
int nCacheLineSize = 32;	// PIII L1 & L2
int nBenchRepeats = 5;		// number of iterations for benchmarks

int nL2KBytes = 256;		// size of L2 cache (for CacheFlush)
int nFlushMultiple = 2;		// iterations through the L2 cache for a flush
int bFlush = 1;				// 1 to flush cache before benchmarks, 0 otherwise

int bReportSuccess = 0;		// report successes as well as failures
int bAbortOnFailure = 0;	// call abort on each failing test

#define DO_TESTX			// define to test w/ seeded allocation failures
//#define DUMMY_DECODE		// define to benchmark a dummy HoloStor_Decode() 

#if defined(DO_TESTX) && defined(__KERNEL__)
#undef DO_TESTX
#endif

#ifdef DO_TESTX
static int nAllocFaults = 0;				// statistics (outputs)
static int nAllocations = 0;
static int nAllocBytes = 0;
#define	REPORTMEMORY	\
	printf("Outstanding memory debt = %d bytes\n", nAllocBytes)
#else
#define	REPORTMEMORY
#endif

//////////////////////////////////////////////////////////////////////
//
//	Utility routines used by tests.
//
//////////////////////////////////////////////////////////////////////

int nPass = 0;
int nFail = 0;

// General interface reporting function.  
//	XXX - Could use __FILE__ and __LINE__ to identify location.
void
report(char *prefix, char *description, int ret, int expect)
{
	int bFailure = 0;
	char *outcome;
	if (expect < 0) {			// expect an error
		if (ret != expect)
			bFailure++;
	} else {					// expect success
		if (ret < 0)
			bFailure++;
	}
	if (bFailure) {
		outcome = "FAIL";
		nFail++;
	} else {
		outcome = "pass";
		nPass++;
	}
	if (bFailure || bReportSuccess)
		printf("%s (%2d returned/ %2d expected) %s - %s\n", 
			outcome, ret, expect, prefix, description);
#ifndef	__KERNEL__
	if (bFailure && bAbortOnFailure)
		abort();
#endif
}

// Aligned memory allocator
char *_AlignedAlloc(int nBlkSize, const HOLOSTOR_CFG *pCfg)
{
	const int nBlkAlignment = nCacheLineSize;
	char *base = (char *)malloc(nBlkSize+nBlkAlignment);
	char *pBlk;
	//
	if (nBlkAlignment == 0)
		return base;								// no special alignment
	assert(nBlkAlignment >= sizeof(char*));
	// Round up to an aligned boundary, always leaving space below.
	pBlk = base + nBlkAlignment - ((UINT_PTR)base%nBlkAlignment);
	// Store the value returned by new.
	((char **)pBlk)[-1] = base;
	memset(pBlk, 0, nBlkSize);						// zero memory
	return pBlk;
}

// Free memory allocated via _AlignedAlloc().
void _AlignedFree(char *pBlk, const HOLOSTOR_CFG *pCfg)
{
	const int nBlkAlignment = nCacheLineSize;
	char *base;
	//
	if (nBlkAlignment == 0) {
		free(pBlk);									// no special alignment
		return;
	}
	base = ((char **)pBlk)[-1];
	free(base);
}

// Allocate Reliability Group buffers.
char **ppAlloc(const HOLOSTOR_CFG *pCfg)
{
	int i;
	int M = pCfg->DataBlocks+pCfg->EccBlocks;
	char **ppBuffer = (char **)_AlignedAlloc(M*sizeof(char*), pCfg);

	for (i = 0; i < M; i++)
		ppBuffer[i] = (char *)_AlignedAlloc(pCfg->BlockSize, pCfg);
	return ppBuffer;
}

// Free Reliability Group buffers.
void ppFree(char **ppBuffer, const HOLOSTOR_CFG *pCfg)
{
	int i;
	int M = pCfg->DataBlocks+pCfg->EccBlocks;
	for (i = 0; i < M; i++)
		_AlignedFree((char*)ppBuffer[i], pCfg);
	_AlignedFree((char*)ppBuffer, pCfg);
}

// Fill a buffer.
void FillOne(char *pBuffer, int nValue, const HOLOSTOR_CFG *pCfg)
{
	unsigned i;
	for (i = 0; i < pCfg->BlockSize; i++)
		pBuffer[i] = nValue;
}

// Fill all the buffers.
void FillAll(char **ppBuffer, const HOLOSTOR_CFG *pCfg)
{
	int i;
	int M = pCfg->DataBlocks+pCfg->EccBlocks;
	for (i = 0; i < M; i++)
		FillOne(ppBuffer[i], '0'+i, pCfg);
}

// Check a buffer.
int CheckOne(char *pBuffer, int nValue, const HOLOSTOR_CFG *pCfg)
{
	unsigned i;
	for (i = 0; i < pCfg->BlockSize; i++)
		if (pBuffer[i] != nValue)
			return -1;
	return 0;							// same return values as interface
}

// Compare a pair of buffers.
int CompareOne(char *pBuffer1, char *pBuffer2, const HOLOSTOR_CFG *pCfg)
{
	unsigned i;
	for (i = 0; i < pCfg->BlockSize; i++)
		if (pBuffer1[i] != pBuffer2[i])
			return -1;
	return 0;							// same return values as interface
}

// Check all the buffers.
int CheckData(char **ppBuffer, const HOLOSTOR_CFG *pCfg)
{
	int i;
	int N = pCfg->DataBlocks;
	for (i = 0; i < N; i++)
		if (CheckOne(ppBuffer[i], '0'+i, pCfg) < 0)
			return -1;
	return 0;							// same return values as interface
}

int JunkFill = 0xFF;					// trash to fill a buffer

// Flush the cache
void CacheFlush(void)
{
	if (bFlush) {
		int i, sum = 0;
		int nCount = nFlushMultiple * nL2KBytes*1024/sizeof(long);
		long *pBuffer = (long*)malloc(nCount*sizeof(long));
		for (i = 0; i < nCount; i++)
			sum += pBuffer[i];
		free(pBuffer);
	}
}

#define UNROLL			8
#define LOOP_BODY(i)	pDst[i] ^= pSrc[i]

void copy32bit(ULONG *pDst, ULONG *pSrc, int nBytes)
{
	int count = nBytes/(UNROLL*sizeof(ULONG));
	while (count--) {
		switch (UNROLL) {
		case 8:
			LOOP_BODY(7);
			LOOP_BODY(6);
			LOOP_BODY(5);
			LOOP_BODY(4);
		case 4:
			LOOP_BODY(3);
			LOOP_BODY(2);
		case 2:
			LOOP_BODY(1);
		case 1:
			LOOP_BODY(0);
		}
		pDst += UNROLL;
		pSrc += UNROLL;
	}
}

// Dummy HoloStor_Decode that uses memcpy() for a performance comparison.
void DummyDecode(const HOLOSTOR_CFG *pCfg, char** BlockGroup, unsigned uInvalidMask)
{
	unsigned i, j;
	unsigned M = pCfg->DataBlocks+pCfg->EccBlocks;
	//
	char ValidBlock[17];							// XXX - hardcoded constant
	char InvalidBlock[4];							// XXX - hardcoded constant
	unsigned nValid = 0, nInvalid = 0;
	unsigned mask = uInvalidMask;
	for (i = 0; i < M; i++, mask >>= 1) {
		if (mask&1)
			InvalidBlock[nInvalid++] = i;
		else
			ValidBlock[nValid++] = i;
	}
	if (nValid > pCfg->DataBlocks)
		nValid = pCfg->DataBlocks;
	// Mimic HoloStor_Decode in the amount, source and destination of data copied
	for (i = 0; i < nInvalid; i++)
		for (j = 0; j < nValid; j++)
#if 1
			copy32bit(
				(ULONG*)BlockGroup[(int)InvalidBlock[i]],
				(ULONG*)BlockGroup[(int)ValidBlock[j]],
				pCfg->BlockSize
			);
#else
			memcpy(
				BlockGroup[InvalidBlock[i]],
				BlockGroup[ValidBlock[j]],
				pCfg->BlockSize
			);
#endif
}

//////////////////////////////////////////////////////////////////////
//
//	Self_Test - Check operation of the test infrastructure.
//
//////////////////////////////////////////////////////////////////////

void
self_test(void)
{
//	char moniker[] = "self_test";
	printf("*** self test begin ***\n");
	arith_self_test();
	printf("1054 displays with increasing precision as: %s %s %s %s\n",
		PercentE(1054,1,0),
		PercentE(1054,1,1),
		PercentE(1054,1,2),
		PercentE(1054,1,3));
	printf("*** self test complete ***\n");
}

//////////////////////////////////////////////////////////////////////
//
//	Test0 - Exercise session creation boundaries.
//
//////////////////////////////////////////////////////////////////////

void
test0(void)
{
	char moniker[] = "test0";
	int ret;
	HOLOSTOR_CFG cfg;
	HOLOSTOR_SESSION hSession;
	//
	cfg.BlockSize = nMinBlockSize;	// valid small configuration values
	cfg.DataBlocks = 1;
	cfg.EccBlocks = 1;
	//
	hSession = HoloStor_CreateSession(&cfg);
	report(moniker, "1 HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_SUCCESS);
	ret = HoloStor_CloseSession(hSession);
	report(moniker, "1 HoloStor_CloseSession", ret, HOLOSTOR_STATUS_SUCCESS);

	cfg.BlockSize = 1;				// too small
	cfg.DataBlocks = 1;				// OK
	cfg.EccBlocks = 1;				// OK
	//
	hSession = HoloStor_CreateSession(&cfg);
	report(moniker, "2 HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_BAD_CONFIGURATION);
	ret = HoloStor_CloseSession(hSession);
	report(moniker, "2 HoloStor_CloseSession", ret, HOLOSTOR_STATUS_BAD_SESSION);

	cfg.BlockSize = nMinBlockSize;	// OK
	cfg.DataBlocks = 1;				// OK
	cfg.EccBlocks = 0;				// too small
	//
	hSession = HoloStor_CreateSession(&cfg);
	report(moniker, "3 HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_BAD_CONFIGURATION);
	ret = HoloStor_CloseSession(hSession);
	report(moniker, "3 HoloStor_CloseSession", ret, HOLOSTOR_STATUS_BAD_SESSION);

	cfg.BlockSize = nMinBlockSize;	// OK
	cfg.DataBlocks = 1;				// OK
	cfg.EccBlocks = 5;				// too large
	//
	hSession = HoloStor_CreateSession(&cfg);
	report(moniker, "4 HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_BAD_CONFIGURATION);
	ret = HoloStor_CloseSession(hSession);
	report(moniker, "4 HoloStor_CloseSession", ret, HOLOSTOR_STATUS_BAD_SESSION);

	cfg.BlockSize = nMinBlockSize;	// OK
	cfg.DataBlocks = 0;				// too small
	cfg.EccBlocks = 1;				// OK
	//
	hSession = HoloStor_CreateSession(&cfg);
	report(moniker, "5 HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_BAD_CONFIGURATION);
	ret = HoloStor_CloseSession(hSession);
	report(moniker, "5 HoloStor_CloseSession", ret, HOLOSTOR_STATUS_BAD_SESSION);

	cfg.BlockSize = nMinBlockSize;	// OK
	cfg.DataBlocks = 1;				// OK
	cfg.EccBlocks = 5;				// too big: K>4
	//
	hSession = HoloStor_CreateSession(&cfg);
	report(moniker, "6 HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_BAD_CONFIGURATION);
	ret = HoloStor_CloseSession(hSession);
	report(moniker, "6 HoloStor_CloseSession", ret, HOLOSTOR_STATUS_BAD_SESSION);

	cfg.BlockSize = nMinBlockSize;	// OK
	cfg.DataBlocks = 16;			// too big: N+K>17
	cfg.EccBlocks = 4;
	//
	hSession = HoloStor_CreateSession(&cfg);
	report(moniker, "7 HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_BAD_CONFIGURATION);
	ret = HoloStor_CloseSession(hSession);
	report(moniker, "7 HoloStor_CloseSession", ret, HOLOSTOR_STATUS_BAD_SESSION);
}

//////////////////////////////////////////////////////////////////////
//
//	Test1 - Exercise Encode/Decode/Rebuild/WriteDelta/EncodeDelta boundaries.
//
//////////////////////////////////////////////////////////////////////

void
test1(void){
	char moniker[] = "test1";
	int ret;
	HOLOSTOR_CFG cfg;
	HOLOSTOR_SESSION hSession;
	char** BlockGroup;
	char *BadBuffers[2] = { (char*)1, (char*)2 };
	//
	cfg.BlockSize = nMinBlockSize;	// smallest supported
	cfg.DataBlocks = 1;
	cfg.EccBlocks = 1;
	BlockGroup = ppAlloc(&cfg);
	//
	hSession = HoloStor_CreateSession(&cfg);
	report(moniker, "1 HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_SUCCESS);
	// No invalid blocks
	ret = HoloStor_Decode(hSession, (PVOID*)BlockGroup, 0);
	report(moniker, "1 HoloStor_Decode", ret, HOLOSTOR_STATUS_SUCCESS);
	// Bad uInvalidBlockMask
	ret = HoloStor_Decode(hSession, (PVOID*)BlockGroup, 1<<2);
	report(moniker, "2 HoloStor_Decode", ret, HOLOSTOR_STATUS_INVALID_PARAMETER);
	// Execessive bad blocks
	ret = HoloStor_Decode(hSession, (PVOID*)BlockGroup, 3);
	report(moniker, "3 HoloStor_Decode", ret, HOLOSTOR_STATUS_TOO_MANY_BAD_BLOCKS);
	// No invalid blocks
	ret = HoloStor_Rebuild(hSession, (PVOID*)BlockGroup, 0, -2);
	report(moniker, "1 HoloStor_Rebuild", ret, HOLOSTOR_STATUS_SUCCESS);
	// Bad lWhichBlock
	ret = HoloStor_Rebuild(hSession, (PVOID*)BlockGroup, 0,  2);
	report(moniker, "2 HoloStor_Rebuild", ret, HOLOSTOR_STATUS_INVALID_PARAMETER);
	// Misaligned buffers
	ret = HoloStor_Encode(hSession, (PVOID*)BadBuffers);
	report(moniker, "1 HoloStor_Encode", ret, HOLOSTOR_STATUS_MISALIGNED_BUFFER);
	// Misaligned buffers
	ret = HoloStor_WriteDelta(hSession, (PVOID)1, (PVOID)2, (PVOID)3);
	report(moniker, "1 HoloStor_WriteDelta", ret, HOLOSTOR_STATUS_MISALIGNED_BUFFER);
	// Misaligned buffers
	ret = HoloStor_EncodeDelta(hSession,
						 0, (PVOID)1,
						 1, (PVOID)2,
						    (PVOID)3);
	report(moniker, "1 HoloStor_EncodeDelta", ret, HOLOSTOR_STATUS_MISALIGNED_BUFFER);
	// Invalid lDataIndex
	ret = HoloStor_EncodeDelta(hSession,
						 2, (PVOID)BlockGroup[0],
						 1, (PVOID)BlockGroup[0],
						    (PVOID)BlockGroup[1]);
	report(moniker, "2 HoloStor_EncodeDelta", ret, HOLOSTOR_STATUS_INVALID_PARAMETER);
	// Invalid lEccIndex
	ret = HoloStor_EncodeDelta(hSession,
						 0, (PVOID)BlockGroup[0],
						 2, (PVOID)BlockGroup[0],
						    (PVOID)BlockGroup[1]);
	report(moniker, "3 HoloStor_EncodeDelta", ret, HOLOSTOR_STATUS_INVALID_PARAMETER);
	//
	ret = HoloStor_CloseSession(hSession);
	report(moniker, "1 HoloStor_CloseSession", ret, HOLOSTOR_STATUS_SUCCESS);
	//
	ppFree(BlockGroup, &cfg);
}

//////////////////////////////////////////////////////////////////////
//
//	Test2 - Exercise Encode/Decode/Rebuild operation.
//
//////////////////////////////////////////////////////////////////////

void
test2(void){
	char moniker[] = "test2";
	unsigned i;
	int ret;
	unsigned uInvalidMask;
	HOLOSTOR_CFG cfg;
	HOLOSTOR_SESSION hSession;
	char** BlockGroup;
	//
	cfg.BlockSize = nMinBlockSize;				// smallest supported
	cfg.DataBlocks = 3;
	cfg.EccBlocks = 2;
	BlockGroup = ppAlloc(&cfg);
	//
	hSession = HoloStor_CreateSession(&cfg);
	report(moniker, "HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_SUCCESS);
	//
	FillAll(BlockGroup, &cfg);
	// Zap first ECC block.
	FillOne(BlockGroup[cfg.DataBlocks], JunkFill, &cfg);
	ret = HoloStor_Encode(hSession, (PVOID*)BlockGroup);
	report(moniker, "1 HoloStor_Encode", ret, HOLOSTOR_STATUS_SUCCESS);
	ret = CheckOne(BlockGroup[cfg.DataBlocks], JunkFill, &cfg);
	report(moniker, "1 CheckOne", ret, -1);		// pass if ECC changed
	// Zap first Data block.
	FillOne(BlockGroup[0], JunkFill, &cfg);
	ret = HoloStor_Decode(hSession, (PVOID*)BlockGroup, (1<<0));
	report(moniker, "2 HoloStor_Decode", ret, HOLOSTOR_STATUS_SUCCESS);
	ret = CheckData(BlockGroup, &cfg);
	report(moniker, "2 CheckData", ret, 0);		// pass if Data restored
	// Zap the maximum Data blocks.
	uInvalidMask = 0;
	for (i = 0; i < cfg.EccBlocks; i++) {
		FillOne(BlockGroup[i], JunkFill, &cfg);
		uInvalidMask |= (1<<i);
	}
	ret = HoloStor_Decode(hSession, (PVOID*)BlockGroup, uInvalidMask);
	report(moniker, "3 HoloStor_Decode", ret, HOLOSTOR_STATUS_SUCCESS);
	ret = CheckData(BlockGroup, &cfg);
	report(moniker, "3 CheckData", ret, 0);		// pass if Data restored
	// Zap one too many Data blocks.
	uInvalidMask = 0;
	for (i = 0; i < cfg.EccBlocks+1; i++)
		uInvalidMask |= (1<<i);
	ret = HoloStor_Decode(hSession, (PVOID*)BlockGroup, uInvalidMask);
	report(moniker, "4 HoloStor_Decode", ret, HOLOSTOR_STATUS_TOO_MANY_BAD_BLOCKS);
	//
	ret = HoloStor_CloseSession(hSession);
	report(moniker, "HoloStor_CloseSession", ret, HOLOSTOR_STATUS_SUCCESS);
	//
	ppFree(BlockGroup, &cfg);
}

//////////////////////////////////////////////////////////////////////
//
//	Test2b - Exercise WriteDelta/EncodeDelta operation.
//
//////////////////////////////////////////////////////////////////////

void
test2b(void){
	char moniker[] = "test2b";
	unsigned i, j;
	int ret;
	HOLOSTOR_CFG cfg;
	HOLOSTOR_SESSION hSession;
	char** BlockGroup1;
	char** BlockGroup2;
	char** BlockGroupX;
	//
	cfg.BlockSize = nMinBlockSize;	// smallest supported
	cfg.DataBlocks = 3;
	cfg.EccBlocks = 2;
	BlockGroup1 = ppAlloc(&cfg);
	BlockGroup2 = ppAlloc(&cfg);
	BlockGroupX = ppAlloc(&cfg);	// for scratch
	//
	hSession = HoloStor_CreateSession(&cfg);
	report(moniker, "HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_SUCCESS);
	//
	for (i = 0; i < cfg.DataBlocks; i++) {
		FillAll(BlockGroup1, &cfg);
		ret = HoloStor_Encode(hSession, (PVOID*)BlockGroup1);
		report(moniker, "1 HoloStor_Encode", ret, HOLOSTOR_STATUS_SUCCESS);
		FillAll(BlockGroup2, &cfg);
		// New value for BlockGroup2[i].
		FillOne(BlockGroup2[i], JunkFill, &cfg);
		// Calculate ECCs for BlockGroup2.
		ret = HoloStor_Encode(hSession, (PVOID*)BlockGroup2);
		report(moniker, "2 HoloStor_Encode", ret, HOLOSTOR_STATUS_SUCCESS);
		// Use HoloStor_WriteDelta/HoloStor_EncodeDelta to compute new ECCs for BlockGroup1.
		// XXX - Interface Spec should state that lpDeltaBlock can be
		//       the same as lpDataBlockOld/lpDataBlockNew
		ret = HoloStor_WriteDelta(hSession,
					BlockGroup1[i], BlockGroup2[i], BlockGroupX[i]
			  );
		report(moniker, "3 HoloStor_WriteDelta", ret, HOLOSTOR_STATUS_SUCCESS);
		for (j = cfg.DataBlocks; j < cfg.DataBlocks+cfg.EccBlocks; j++) {
			// XXX - Interface Spec should state that lpEccBlockNew can be
			//       the same as lpEccBlockOld.
			ret = HoloStor_EncodeDelta(hSession,
					i, BlockGroupX[i], j, BlockGroup1[j], BlockGroupX[j]
				  );
			report(moniker, "4 HoloStor_EncodeDelta", ret, HOLOSTOR_STATUS_SUCCESS);
			ret = CompareOne(BlockGroup2[j], BlockGroupX[j], &cfg);
			report(moniker, "5 CompareOne", ret, 0);
		}
	}
	//
	ret = HoloStor_CloseSession(hSession);
	report(moniker, "6 HoloStor_CloseSession", ret, HOLOSTOR_STATUS_SUCCESS);
	//
	ppFree(BlockGroup1, &cfg);
	ppFree(BlockGroup2, &cfg);
	ppFree(BlockGroupX, &cfg);
}

//////////////////////////////////////////////////////////////////////
//
//	Test3 - Measure Encode/Decode performance.
//
//////////////////////////////////////////////////////////////////////

void benchmark(const HOLOSTOR_CFG *pCfg, int nIterations);

void
test3(void){
	HOLOSTOR_CFG cfg;
	// Measure table lookup overhead with a small configuraton
	cfg.BlockSize = nMinBlockSize;	// smallest supported
	cfg.DataBlocks = 1;
	cfg.EccBlocks = 1;
	benchmark(&cfg, nBenchRepeats);
	// Measure typical data processing overhead with moderate configuration
	cfg.BlockSize = nTypBlockSize;	// typical value used
	cfg.DataBlocks = 14;
	cfg.EccBlocks = 3;
	benchmark(&cfg, nBenchRepeats);
	// Measure typical data processing overhead with maximal configuration
	cfg.BlockSize = nTypBlockSize;	// typical value used
	cfg.DataBlocks = 13;
	cfg.EccBlocks = 4;
	benchmark(&cfg, nBenchRepeats);
}

metric_t metric[] = {
	{ "HoloStor_Decode 0 (cycles/byte)" },
	{ "HoloStor_Decode 1 (cycles/byte)" },
	{ "HoloStor_Decode 2 (cycles/byte)" },
	{ "HoloStor_Decode 3 (cycles/byte)" },
	{ "HoloStor_Decode 4 (cycles/byte)" },
	{ "HoloStor_Encode   (cycles/byte)" },
	{ "HoloStor_CreateSession (cycles)" },
	{ NULL }	/* list terminator */
};
#define	METRIC_ENCODE	5
#define	METRIC_CREATE	6

void benchmark(const HOLOSTOR_CFG *pCfg, int nIterations)
{
	char moniker[] = "test3";
	MetricsInit(metric);
	do {
		int ret;
		unsigned i, faults;
		unsigned uInvalidMask;
		HOLOSTOR_SESSION hSession;
		char** BlockGroup;
		pcycles_t time;
		//
		time = PentiumCycles();
		hSession = HoloStor_CreateSession(pCfg);
		time = PentiumCycles() - time;
		//
		MetricSample(&metric[METRIC_CREATE], (cnt_t)time, (cnt_t)1);
		report(moniker, "HoloStor_CreateSession", hSession, HOLOSTOR_STATUS_SUCCESS);
		printf("[CreateSession for %u+%u : %s cycles]\n",
			pCfg->DataBlocks, pCfg->EccBlocks,
			PercentE(time,1,2));
		//
		BlockGroup = ppAlloc(pCfg);
		for (faults = 0; faults <= pCfg->EccBlocks; faults++) {
			cnt_t bytes;
			//
			FillAll(BlockGroup, pCfg);
			CacheFlush();
			//
			time = PentiumCycles();
			ret = HoloStor_Encode(hSession, (PVOID*)BlockGroup);
			time = PentiumCycles() - time;
			//
			bytes = pCfg->BlockSize*pCfg->DataBlocks;
			MetricSample(&metric[METRIC_ENCODE], (cnt_t)time, bytes);
			report(moniker, "HoloStor_Encode", ret, HOLOSTOR_STATUS_SUCCESS);
			printf(
"[Encode %u for %u+%u (%u byte blocks) : %d cycles (%s cycles/byte)]\n",
				pCfg->EccBlocks,
				pCfg->DataBlocks, pCfg->EccBlocks,
				pCfg->BlockSize,
				(unsigned)time,
				PercentE(time,bytes,2));
			//
			uInvalidMask = 0;
			for (i = 0; i < faults; i++) {
				FillOne(BlockGroup[i], JunkFill, pCfg);	// zap Data block
				uInvalidMask |= (1<<i);
			}
			CacheFlush();
			//
#ifdef	DUMMY_DECODE
			time = PentiumCycles();
			DummyDecode(pCfg, BlockGroup, uInvalidMask);
			time = PentiumCycles() - time;
			ret = HoloStor_Decode(hSession, (PVOID*)BlockGroup, uInvalidMask);
#else
			time = PentiumCycles();
			ret = HoloStor_Decode(hSession, (PVOID*)BlockGroup, uInvalidMask);
			time = PentiumCycles() - time;
#endif
			//
			bytes = pCfg->BlockSize*pCfg->DataBlocks;
			MetricSample(&metric[faults], (cnt_t)time, bytes);
			report(moniker, "HoloStor_Decode", ret, HOLOSTOR_STATUS_SUCCESS);
			printf(
"[Decode %d for %u+%u (%u byte blocks) : %d cycles (%s cycles/byte)]\n",
				faults,
				pCfg->DataBlocks, pCfg->EccBlocks,
				pCfg->BlockSize, 
				(unsigned)time,
				PercentE(time,bytes,2));
			ret = 0;
			for (i = 0; i < faults; i++)
				if ((ret = CheckData(BlockGroup, pCfg)) < 0)
					break;
			report(moniker, "CheckData", ret, 0);	// pass if Data restored
			// Done with a pass - let other processes run.
			preempt();
		}
		ppFree(BlockGroup, pCfg);

		//
		REPORTMEMORY;
		ret = HoloStor_CloseSession(hSession);
		report(moniker, "HoloStor_CloseSession", ret, HOLOSTOR_STATUS_SUCCESS);
	} while	(--nIterations);
#ifdef	DUMMY_DECODE
	printf("***Warning: dummy HoloStor_Decode being benchmarked***\n");
#endif
	printf("%s cache results\n", bFlush ? "Cold" : "Warm");
	MetricsPrint(metric);
}

//////////////////////////////////////////////////////////////////////
//
//							Main
//
//////////////////////////////////////////////////////////////////////

extern void testX(void);

int
main(int argc, char *argv[])
{
	unsigned method = ~0u;
#ifndef	__KERNEL__
	if (argc == 2) 
		sscanf(argv[1], "%u", &method);
#endif
	HoloStor_SetMethod(&method);
	printf("method = %u\n", method);
	self_test();
	printf("*** HoloStor library interface test ***\n");
#ifdef	DO_TESTX
	testX();	// perform first 
#endif
	test0();
	test1();
	test2();
	test2b();
	test3();
	printf("*** Summary: %d failures, %d successes ***\n", nFail, nPass);
	REPORTMEMORY;
	return nFail ? 1 : 0;
}

#ifdef	DO_TESTX
//////////////////////////////////////////////////////////////////////
//
//	TestX - Stress test with memory allocation error seeding (non-kernel only).
//
//////////////////////////////////////////////////////////////////////

static int pFailPercent = 0;	// probability of failure (an input)
//

// Randomly return TRUE for the given percent of the time.
int bFlip(unsigned percent)
{
	// rand() returns a uniform deviate in [0,RAND_MAX]
	return (unsigned long)rand() < percent*(((unsigned long)RAND_MAX+1)/100);
}

void* CommonAlloc(size_t size)
{
	int *p;
	if (bFlip(pFailPercent)) {
		nAllocFaults++;
		return NULL;
	}
	nAllocations++;
	p = (int *)malloc(size+sizeof(int));		// assumed always to succeed
	assert(p != 0);
	nAllocBytes += (*p++ = size);
	return p;
}

void CommonFree(int *p)
{
	assert(p != 0);
	nAllocBytes -= *--p;
	free(p);
}

//
// Replace HoloStor_{Quick,Table}{Alloc,Free} provided in the library with
// ones that optionally inject faults.
//
void* HoloStor_QuickAlloc(size_t size) { return CommonAlloc(size); }
void  HoloStor_QuickFree(void *p) { CommonFree(p); }
void* HoloStor_TableAlloc(size_t size) { return CommonAlloc(size); }
void  HoloStor_TableFree(void *p) { CommonFree(p); }

void
testX()
{
	char moniker[] = "test0";
	int nPasses = 1000;			// specify a value yielding good coverage
	int nCalls = 0;
	int nFaults = 0;
	int i, j;
	// Memory allocation is confined to session creation/close.  
	// Test all possible combinations.
	printf("*** allocation test begin (this may take some time) ***\n");
	pFailPercent = 1;						// enable error seeding
	while (nPasses--) {
		for (i = 1; i <= 16; i++)
		for (j = 1; j <= 4; j++) {
			int ret;
			HOLOSTOR_CFG cfg;
			HOLOSTOR_SESSION hSession;
			int SavedAllocFaults;
			//
			if (i + j > 17)
				continue;
			cfg.BlockSize = nMinBlockSize;	// valid configuration values
			cfg.DataBlocks = i;
			cfg.EccBlocks = j;
			//
			SavedAllocFaults = nAllocFaults;
			hSession = HoloStor_CreateSession(&cfg);
			if (nAllocFaults > SavedAllocFaults) {
				nFaults++;
				report(moniker, "X HoloStor_CreateSession", hSession,
					HOLOSTOR_STATUS_NO_MEMORY);
				ret = HoloStor_CloseSession(hSession);
				report(moniker, "X HoloStor_CloseSession", ret,
					HOLOSTOR_STATUS_BAD_SESSION);
			} else {
				report(moniker, "X HoloStor_CreateSession", hSession,
					HOLOSTOR_STATUS_SUCCESS);
				ret = HoloStor_CloseSession(hSession);
				report(moniker, "X HoloStor_CloseSession", ret,
					HOLOSTOR_STATUS_SUCCESS);
			}
			nCalls++;
		}
	}
	// Note: there is a small change that multiple faults can occur in the
	// same call to HoloStor_CreateSession, so the following fault rates can differ.
	printf("The memory allocator returned seeded failures in %d of %d calls\n",
		nAllocFaults, nAllocations);
	printf("Allocation faults reported in %d of %d HoloStor_CreateSession calls\n",
		nFaults, nCalls);
	printf("*** allocation test end ***\n");
	//
	pFailPercent = 0;					// disable error seeding
}
#endif // DO_TESTX
