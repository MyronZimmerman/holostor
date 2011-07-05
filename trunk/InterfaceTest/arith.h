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
	arith.h

 Abstract:
	Interfaces to arithmetic support routines.

--****************************************************************************/

#ifdef	__KERNEL__
#include <linux/kernel.h>
#else
#include <stdio.h>
#define	USE_FLOAT
#endif

typedef unsigned long long cnt_t;

typedef struct {
	char *	text;
	int		nSamples;
#ifdef	USE_FLOAT
	double	sum;
	double	sumSq;
#endif
} metric_t;

void MetricsInit (metric_t *metricTable);
void MetricsPrint(metric_t *metricTable);
void MetricSample(metric_t *thisMetric, cnt_t x, cnt_t y);

char *PercentE(cnt_t x, cnt_t y, unsigned precision);

void arith_self_test(void);
