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
#	Build UnitTest program for Linux.
#
###############################################################################
#
# Compile options
#	NDEBUG - disables ANSI assert(3)
#	_DEBUG - enables debug #ifdef's (must match library)
#
SHELL = /bin/sh

# Common definitions
WARNINGS = -Wall
CPPFLAGS = $(CFG) -I.. -I../HoloStorLib -I../Extras 
CXXFLAGS = $(WARNINGS) $(GFLAG) $(OPT) -fno-exceptions -fno-rtti
LDFLAGS = $(GFLAG)
EXTRA_LIBS =
#
R_DIR = LinuxRelease
D_DIR = LinuxDebug
#
LIB = HoloStorLib.a
EXE = UnitTest.exe

.PHONY: all debug release clean clobber

# Public targets: all, debug, release, clean, clobber
#
all: release debug

# Target-specific variables
release: OPT=-O3
release: GFLAG=
release: CFG=-DNDEBUG
# The target
release: $(R_DIR)/$(EXE)

# Target-specific variables
debug  : OPT=
debug  : GFLAG=-g
debug  : CFG=-D_DEBUG
# The target
debug  : $(D_DIR)/$(EXE)

clean:
	-rm $(R_DIR)/*.o $(R_DIR)/$(EXE)
	-rm $(D_DIR)/*.o $(D_DIR)/$(EXE)

clobber:
	-rm -rf $(R_DIR) $(D_DIR)

# Private targets
#	Static pattern rules: $* matches either $(D_DIR) or $(R_DIR).
#
$(D_DIR)/$(EXE) $(R_DIR)/$(EXE): %/$(EXE): \
		%   %/main.o %/UnitTest.o ../HoloStorLib/%/$(LIB)
	$(CXX) $(LDFLAGS) $*/main.o $*/UnitTest.o \
		../HoloStorLib/$*/$(LIB) $(EXTRA_LIBS) -o $*/$(EXE)

$(D_DIR)/main.o $(R_DIR)/main.o: %/main.o: \
		main.cpp ../HoloStor.h UnitTest.hpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) main.cpp -o $*/main.o

$(D_DIR)/UnitTest.o $(R_DIR)/UnitTest.o: %/UnitTest.o: \
		UnitTest.cpp ../HoloStor.h UnitTest.hpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) UnitTest.cpp -o $*/UnitTest.o 

$(R_DIR) $(D_DIR):			# Make the directories.
	if [ ! -e $@ ]; then mkdir $@; fi
