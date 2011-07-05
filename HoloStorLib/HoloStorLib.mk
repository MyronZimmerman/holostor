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
#	Build HoloStor library for Linux.
#
###############################################################################
#
# Compile options
#	NDEBUG - disables ANSI assert(3)
#	_DEBUG - enables HoloStor debug #ifdef's (using std::cout)
#
SHELL = /bin/sh

# Common definitions
WARNINGS = -Wall
CPPFLAGS = $(CFG) -I.. 				# For ../HoloStor.h 
CXXFLAGS = $(WARNINGS) $(GFLAG) $(OPT) -fcheck-new -fno-exceptions -fno-rtti
CFLAGS   = $(WARNINGS) $(GFLAG) $(OPT)
LIB = HoloStorLib.a
MOD = HoloStorMod.o

# Core functioinality.
CORE = \
	Tuple.o \
	GF16.o \
	CombinIter.o \
	MathUtils.o \
	Session.o \
	main.o \
	IDA.o \
	GF2Mul.o \
	CodingTable.o \
	SessionTable.o \
	CodingMatrix.o

# Core plus porting layer.
OBJECTS = $(CORE) \
	Platform.o

OSTYPE := $(shell uname -o)
ARCH := $(shell uname -m)


.PHONY: all debug release kernel clean clobber kcheck

# Public targets: all, debug, release, kernel, clean, clobber, kcheck
#
all: debug
all: release
ifeq ($(OSTYPE),GNU/Linux)
all: kernel
endif

# Target-specific variables
kernel: OPT= -Os
kernel: GFLAG= -g
kernel: CFG += -DNDEBUG
# Common -f and -m compile options.
kernel: CFG += -fconserve-stack -fno-asynchronous-unwind-tables -fno-common \
	-fno-delete-null-pointer-checks -fomit-frame-pointer \
	-fno-optimize-sibling-calls -fno-strict-aliasing \
	-fno-strict-overflow -fstack-protector
kernel: CFG += -maccumulate-outgoing-args \
	-mno-3dnow -mno-mmx -mno-sse -mno-sse2 -mtune=generic

ifeq ($(ARCH),i686)
kernel: CFG += -ffreestanding -freg-struct-return
kernel: CFG += -m32 -march=i686 -mpreferred-stack-boundary=2 -mregparm=3 \
	-msoft-float 
endif
ifeq ($(ARCH),x86_64)
kernel: CFG += -funit-at-a-time
kernel: CFG += -m64 -mcmodel=kernel -mno-red-zone
endif

# The target
kernel: LinuxKernel/$(MOD)

LinuxKernel/$(MOD): LinuxKernel/$(LIB)
	ld -r -o $@ -u HoloStor_CreateSession $<

# Target-specific variables
release: OPT=-O3
release: GFLAG=
release: CFG=-DNDEBUG
# The target
release: LinuxRelease/$(LIB)

# Target-specific variables
debug  : OPT=
debug  : GFLAG=-g
#debug  : GFLAG=-g -fprofile-arcs -ftest-coverage
debug  : CFG=-D_DEBUG
# The target
debug  : LinuxDebug/$(LIB)

clean:
	-rm *.gcov *.da *.bb *.bbg
	-rm LinuxKernel/$(LIB) LinuxKernel/$(MOD)
	-rm LinuxRelease/$(LIB)
	-rm LinuxDebug/$(LIB)

clobber: clean
	-rm -rf LinuxKernel LinuxRelease LinuxDebug

# Check the kernel module to be sure that it is linked properly.
kcheck: LinuxKernel/$(MOD)
	nm -u LinuxKernel/$(MOD)

# Private targets
#
LinuxKernel/$(LIB): LinuxKernel \
	LinuxKernel/$(LIB)($(CORE))

LinuxRelease/$(LIB): LinuxRelease \
	LinuxRelease/$(LIB)($(OBJECTS))

LinuxDebug/$(LIB): LinuxDebug \
	LinuxDebug/$(LIB)($(OBJECTS))

# Overload the implicit rules so that library components are updated
# individually.  Otherwise one set of objects are built and put into both
# libraries.
.cpp.a:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $*.o
	$(AR) r $@ $*.o
	$(RM) $*.o

.c.a:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $*.o
	$(AR) r $@ $*.o
	$(RM) $*.o

LinuxKernel LinuxRelease LinuxDebug:		# Make the directories.
	if [ ! -e $@ ]; then mkdir $@; fi
