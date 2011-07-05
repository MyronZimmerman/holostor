/*  Copyright (C) 2011 Thomas P. Scott and Myron Zimmerman

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
	hello.c

 Abstract:
	Trivial Linux Loadable Kernel Module.

--****************************************************************************/

#include <linux/init.h>		// Needed for the macros
#include <linux/module.h>	// Needed by all modules

static int
hello_init(void)
{
	printk(KERN_INFO "hello: init\n");
	return 0;
}

static void
hello_exit(void)
{
	printk(KERN_INFO "hello: exit\n");
}

module_init(hello_init);
module_exit(hello_exit);
