This directory contains the source kit for HoloStor 1.0.4.

The kit consists of the following files:
  ReadMe.txt     This file.
  HoloStorInterfaceSpec-1.0.pdf   HoloStor Interface specification.
  HoloStorLibReleaseNotes.pdf     HoloStor Release Notes (binary distribution).
  HoloStor.sln   Solution file for Visual Studio 2008.
  HoloStor.h     Header describing Library interfaces.
  HoloStor.mk    Linux makefile.
  HoloStorLib/   Library sources (all platforms).
  Extras/        Common sources used by tests but not by HoloStorLib.
  InterfaceTest/ A test program exercising HoloStor public interfaces.
  TestSuite/     A test program exercising HoloStor public interfaces.
  UnitTest/      A test program testing internal interfaces.
  Samples/       Build files for a sample included in the binary distribution.
  Package/       Build file for creating a binary distribution.
  LKM-BuildTest/ A generic Linux Loadable Kernel Module (LKM) that can be used
                 to determine the compile options needed for the HoloStor LKM.

The HoloStor user mode library has been tested on Windows XP/Vista and
Ubuntu 10.10.  The HoloStor kernel mode module has been tested on
Ubuntu 10.10.

To build Linux binaries from sources:
1) make -f HoloStor.mk
The test programs are built with an ".exe" extension, but are in fact
Linux binaries.

To run the various tests in user mode:
1) ./TestSuite/LinuxRelease/EncodeDecode.exe
2) ./InterfaceTest/LinuxRelease/InterfaceTest.exe
3) ./UnitTest/LinuxRelease/UnitTest.exe
More information about EncodeDecode and running the HoloStor library in
kernel mode can be found in the Release Notes.

To package a build into a binary distribution:
1) cd Package; make -f Package.mk
