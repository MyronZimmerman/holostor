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
#	Build InterfaceTest program for Linux.
#
###############################################################################
#
# Compile options
#	NDEBUG - disables ANSI assert(3)
#	_DEBUG - enables debug #ifdef's
#
SHELL = /bin/sh

# Common definitions
WARNINGS = -Wall
CPPFLAGS = $(CFG) -I.. -I../Extras
CFLAGS = $(WARNINGS) $(GFLAG) $(OPT)
LDFLAGS = $(GFLAG)
#
R_DIR = LinuxRelease
K_DIR = LinuxKernel
D_DIR = LinuxDebug
#
LIB = HoloStorLib.a
MOD = HoloStorMod.o
EXE = InterfaceTest.exe

OSTYPE := $(shell uname -o)

.PHONY: all debug release kernel clean clobber

# Public targets: all, debug, release, kernel (native only), clean, clobber
#
all: debug
all: release
ifeq ($(OSTYPE),GNU/Linux)
all: kernel
endif

# Target-specific variables
release: OPT=-O3
release: GFLAG=
release: CFG=-DNDEBUG
release: EXTRA_LIBS=
# The target
release: $(R_DIR)/$(EXE)

# Target-specific variables
debug  : OPT=
debug  : GFLAG=-g
debug  : CFG=-D_DEBUG
debug  : EXTRA_LIBS=-lstdc++
# The target
debug  : $(D_DIR)/$(EXE)

clean:
	-rm $(R_DIR)/*.o $(R_DIR)/$(EXE)
	-rm $(D_DIR)/*.o $(D_DIR)/$(EXE)
	-make -C $(K_DIR) clean

clobber:
	-rm -rf $(R_DIR) $(K_DIR) $(D_DIR)

# Private targets
#	Static pattern rules: $* matches either $(D_DIR) or $(R_DIR).
#
$(D_DIR)/$(EXE) $(R_DIR)/$(EXE): %/$(EXE): \
		%   %/main.o %/arith.o ../HoloStorLib/%/$(LIB)
	$(CC) $(LDFLAGS) $*/main.o $*/arith.o \
		../HoloStorLib/$*/$(LIB) $(EXTRA_LIBS) -o $*/$(EXE)

$(D_DIR)/main.o $(R_DIR)/main.o: %/main.o: \
		main.c ../HoloStor.h ../Extras/PentiumCycles.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) main.c -o $*/main.o

$(D_DIR)/arith.o $(R_DIR)/arith.o: %/arith.o: \
		arith.c arith.h 
	$(CC) -c $(CFLAGS) $(CPPFLAGS) arith.c -o $*/arith.o

$(R_DIR) $(K_DIR) $(D_DIR):		# Make the directories.
	if [ ! -e $@ ]; then mkdir $@; fi

# Create the environment necessary for building a Linux LKM
$(K_DIR)/PentiumCycles.h: ../Extras/PentiumCycles.h
	cp $< $@

$(K_DIR)/HoloStor.h: ../HoloStor.h
	cp $< $@

$(K_DIR)/Makefile: Makefile-LKM
	cp $< $@

$(K_DIR)/main.c: main.c
	cp $< $@

$(K_DIR)/arith.c: arith.c
	cp $< $@

$(K_DIR)/arith.h: arith.h
	cp $< $@

$(K_DIR)/wrapper.c: wrapper.c
	cp $< $@

$(K_DIR)/$(MOD): ../HoloStorLib/LinuxKernel/$(MOD)
	cp $< $@

CORE_LIST = PentiumCycles.h HoloStor.h arith.h Makefile \
	main.c wrapper.c arith.c
CORE_TARGETS = $(addprefix $(K_DIR)/,$(CORE_LIST))

kernel: $(K_DIR) $(CORE_TARGETS) $(K_DIR)/HoloStorMod.o
	make -C $(K_DIR) all
