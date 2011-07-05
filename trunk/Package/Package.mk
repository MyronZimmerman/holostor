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
#	Construct a binary release of HoloStor 1.0 Library for Linux.
#
###############################################################################
#
SHELL = /bin/sh

# XXX - GNU Make 3.79 Bug.  MAKEFILE_LIST is not defined.
#THIS := $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))

# Definitions
#
# Reference to self
THIS=Package.mk
# Ownership of file in tarball
TARFLAGS=--owner=root --group=root --numeric-owner
# Major.Minor.Build
VERSION=1.0.4
# -Alpha, -Beta, -RC1, "", etc.
RELEASE=
#
ARCH:=$(shell uname -m)
ROOT=HoloStorLib-linux-$(ARCH)-$(VERSION)$(RELEASE)


.PHONY: all clean check harvest convert permissions build package
# Public targets: all clean check harvest convert permissions build package
#

all:
	@echo "Creating $(ROOT) and $(ROOT).tar.gz"
	$(MAKE) -f $(THIS) harvest
	$(MAKE) -f $(THIS) build
	$(MAKE) -f $(THIS) package

help:
	@echo "Targets: harvest build package clean check help"
	@echo "Edit this makefile and change VERSION and RELEASE"
	@echo "To build a package:"
	@echo "	make harvest"
	@echo "	make build"
	@echo "	make package"
	@echo "Or just type make"

# The removal of $(ROOT) is best left to manual means.
clean:
	-rm $(ROOT).tar.gz
	@echo "You must remove $(ROOT) manually (and carefully)"

check:
	tar tvzf $(ROOT).tar.gz

# Setup the package tree following a library build.
harvest: $(ROOT) $(ROOT)/Samples
	cp ../HoloStor.h "$(ROOT)"
	cp ../HoloStorLibReleaseNotes.pdf "$(ROOT)"
	cp ../HoloStorInterfaceSpec-1.0.pdf "$(ROOT)"
	chmod 644 $(ROOT)/*.h $(ROOT)/*.pdf
	cp ../HoloStorLib/LinuxRelease/HoloStorLib.a "$(ROOT)"
	cp ../HoloStorLib/LinuxKernel/HoloStorMod.o "$(ROOT)"
	chmod 644 $(ROOT)/*.[ao]
	cp ../TestSuite/Makefile-LKM "$(ROOT)/Samples"
	cp ../TestSuite/EncodeDecode.c "$(ROOT)/Samples"
	cp ../TestSuite/wrapper.c "$(ROOT)/Samples"
	cp ../Extras/PentiumCycles.h "$(ROOT)/Samples"
	cp ../Samples/Samples.mk "$(ROOT)/Samples/Makefile"
	chmod 644 $(ROOT)/Samples/*.c $(ROOT)/Samples/Makefile*
	touch $(ROOT)

# Build the samples in the tree (run native).
build:
	cd "$(ROOT)/Samples"; make release clean
	cd "$(ROOT)/Samples"; make kernel clean
	touch $(ROOT)

# Package the tree into a tarball.
package: $(ROOT).tar.gz

# Private targets
#

# Make a tarball form the packaging tree.
$(ROOT).tar.gz: $(ROOT)
	tar -c $(TARFLAGS) -z -f $(ROOT).tar.gz $(ROOT)

# Make the directories of the packaging tree.
$(ROOT) $(ROOT)/Samples:
	if [ ! -e $@ ]; then mkdir $@; fi
