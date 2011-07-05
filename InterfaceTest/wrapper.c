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
	wrapper.c

 Abstract:
	Implementation of Loadable Kernel Module wrapper for Linux.
	
--****************************************************************************/

#include <linux/module.h>		// Needed by all modules
#include <linux/moduleparam.h>	// Needed for MODULE_PARM
#include <linux/kernel.h>		// Needed for KERN_ALERT
#include <linux/init.h>			// Needed for the macros
#include <linux/string.h>		// Needed for strncpy()
#include <linux/errno.h>		// Needed for EAGAIN
//
#include <linux/slab.h>			// for kmalloc/kfree()
#include <linux/vmalloc.h>		// for vmalloc/vfree()

//#define HOLOSTOR_ALLOC_METRICS	// define to enable allocation statistics

#define MAX_ARGV        10

static char *argv[MAX_ARGV] = { NULL, };
static int argc = 0;
module_param_array(argv, charp, &argc, 0);
MODULE_PARM_DESC(argv, "Command line arguments");

//////////////////////////////////////////////////////////////////////
//
//	Kernel runtime support used by HoloStorLib.
//
//////////////////////////////////////////////////////////////////////

#ifdef HOLOSTOR_ALLOC_METRICS
//
// Replace the default HoloStor_Quick{Alloc,Free}/HoloStor_Table{Alloc,Free}
// user space definitions provided in the library with kernel definitions
// that gather statistics, report alloc failure and check alignment.
// 
static int MaxQuickAlloc = 0;
static int MaxTableAlloc = 0;

void* HoloStor_QuickAlloc(unsigned int size)
{
	static int warned  = 0;
	void *ptr = kmalloc(size, GFP_KERNEL);
	//
	if (((unsigned)ptr % 16)!= 0 && !warned) {
		++warned;
		printk(KERN_ALERT "wrapper: kmalloc() returned 0x%x\n", ptr);
	}
	if (size > MaxQuickAlloc)
		MaxQuickAlloc = size;
	if (ptr)
		return ptr;
	printk(KERN_ALERT "wrapper: QuickAlloc of %d bytes failed\n", size);
	return NULL;
}

void HoloStor_QuickFree(void *p) { kfree(p); }

void* HoloStor_TableAlloc(unsigned int size)
{
	static int warned  = 0;
	void *ptr = vmalloc(size);
	//
	if (((unsigned)ptr % 16)!= 0 && !warned) {
		++warned;
		printk(KERN_ALERT "wrapper: kmalloc() returned 0x%x\n", ptr);
	}
	if (size > MaxTableAlloc)
		MaxTableAlloc = size;
	if (ptr)
		return ptr;
	printk(KERN_ALERT "wrapper: TableAlloc of %d bytes failed\n", size);
	return NULL;
}

void HoloStor_TableFree(void *p) { vfree(p); }

#else	//!HOLOSTOR_ALLOC_METRICS
//
// Replace the default HoloStor_Quick{Alloc,Free}/HoloStor_Table{Alloc,Free}
// user space definitions provided in the library with basic definitions
// that are appropriate for kernel space.
// 
void* HoloStor_QuickAlloc(unsigned int size) {
	return kmalloc(size, GFP_KERNEL);
}
void  HoloStor_QuickFree(void *p) { kfree(p); }
void* HoloStor_TableAlloc(unsigned int size) { return vmalloc(size); }
void  HoloStor_TableFree(void *p) { vfree(p); }

#endif	// HOLOSTOR_ALLOC_METRICS

//////////////////////////////////////////////////////////////////////
//
//	Kernel runtime support used by main().
//
//////////////////////////////////////////////////////////////////////

void * malloc(size_t size) { return kmalloc(size,GFP_KERNEL); }
void free(void *p) { kfree(p); }

extern int main(int argc, char *argv[]);

//////////////////////////////////////////////////////////////////////
//
//	Setup.
//
//////////////////////////////////////////////////////////////////////

// Adjust argv[] to appear as in user space and call main().
static int start(void)
{
	// The use of static assumes that only one insmod can run at a time.
	static char szAppName[] = "main";
	static char *nargv[MAX_ARGV+2];
	int i, nargc;

	printk(KERN_ALERT "argc=%d\n", argc);
	for (i = 0; i < argc; i++) 
		printk(KERN_ALERT "argv[%d]=%s\n", i, argv[i]);

	nargc = 0;
	nargv[nargc++] = szAppName;
	for (i = 0; i < argc; i++) 
		nargv[nargc++] = argv[i];
	nargv[nargc] = NULL;
	return main(nargc, nargv);
}

//////////////////////////////////////////////////////////////////////
//
//	LKM init and exit.
//
//////////////////////////////////////////////////////////////////////

static int wrapper_init(void)
{
	int iret;
	printk(KERN_ALERT "wrapper: init\n");
	iret = start();
	printk(KERN_ALERT "wrapper: main returned %d\n", iret);
#ifdef HOLOSTOR_ALLOC_METRICS
	printk(KERN_ALERT "wrapper: MaxQuickAlloc=%d, MaxTableAlloc=%d\n",
		MaxQuickAlloc, MaxTableAlloc);
#endif
	//
	return 0;
}

static void wrapper_exit(void)
{
	printk(KERN_ALERT "wrapper: exit\n");
}

module_init(wrapper_init);
module_exit(wrapper_exit);
