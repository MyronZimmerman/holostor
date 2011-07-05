###############################################################################
#
# Copyright (C) 2011 Thomas P. Scott and Myron Zimmerman
#
# Thomas P. Scott <tpscott@alum.mit.edu>
# Myron Zimmerman <MyronZimmerman@alum.mit.edu>
#
# This file is part of HoloStor.
#
# HoloStor is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# HoloStor is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with HoloStor.  If not, see <http://www.gnu.org/licenses/>. 
#
# Parts of HoloStor are protected by US Patent 7,472,334, the use of
# which is granted in accordance to the terms of GPLv3. 
#
###############################################################################
#
# Abstract:
#	Makefile for a trivial Linux Loadable Kernel Module (LKM) used to 
# determine the appropriate compile flags needed by the currently running
# kernel for LKMs.
#
###############################################################################

OPTIONS = KBUILD_VERBOSE=1

obj-m += hello.o

.PHONY: all clean run

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules $(OPTIONS)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean

# Verify that the module loads.
run:
	-/sbin/insmod hello.ko
	-/sbin/rmmod hello
