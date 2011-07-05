###############################################################################
#
# Copyright (C) 2003-2011 Thomas P. Scott and Myron Zimmerman
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
#	Build EncodeDecode sample program for Linux.
#
###############################################################################
#
SHELL = /bin/sh

# Common definitions
LIB = HoloStorLib.a
MOD = HoloStorMod.o
EXE = EncodeDecode.exe

# Files needed to build the LKM
CORE_LIST = HoloStor.h PentiumCycles.h Makefile EncodeDecode.c wrapper.c $(MOD)
CORE_TARGETS = $(addprefix LinuxKernel/,$(CORE_LIST))

# Default configurables
GFLAG =					# optional -g 

# Public targets: all, release, kernel, clean, clobber
#
all: release kernel

# Target-specific variables for release
release: CFLAGS = $(GFLAG) -O3
release: CPPFLAGS = -DNDEBUG -I.. -I.
# Note: -static is optional and is used here to reduce the dependency of the
# prebult executable on the runtime environment.
release: LDFLAGS = $(GFLAG) -static
#
release: LinuxRelease/EncodeDecode.exe
	cp LinuxRelease/EncodeDecode.exe .

# Target-specific variables for kernel
# Note: Due to inline APIs, kernel modules *must* be optimized to at least -O.
#
kernel: LinuxKernel $(CORE_TARGETS)
	make -C LinuxKernel all
	cp LinuxKernel/TestSuite.ko EncodeDecode.ko

clean:
	-rm -rf LinuxRelease LinuxKernel 

clobber: clean
	-rm EncodeDecode.exe EncodeDecode.ko

# Private targets
#
LinuxRelease LinuxKernel:			# Make the sub-directories.
	if [ ! -e $@ ]; then mkdir $@; fi

LinuxRelease/EncodeDecode.exe: LinuxRelease LinuxRelease/EncodeDecode.o
	$(CC) $(LDFLAGS) LinuxRelease/EncodeDecode.o ../$(LIB) -o $@ 

LinuxRelease/EncodeDecode.o: EncodeDecode.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) EncodeDecode.c \
		-o LinuxRelease/EncodeDecode.o

# Create the environment necessary for building a Linux LKM
LinuxKernel/HoloStor.h: ../HoloStor.h
	cp $< $@

LinuxKernel/PentiumCycles.h: PentiumCycles.h
	cp $< $@

LinuxKernel/Makefile: Makefile-LKM
	cp $< $@

LinuxKernel/EncodeDecode.c: EncodeDecode.c
	cp $< $@

LinuxKernel/wrapper.c: wrapper.c
	cp $< $@

LinuxKernel/$(MOD): ../$(MOD)
	cp $< $@
