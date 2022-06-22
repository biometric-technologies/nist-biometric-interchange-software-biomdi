#
# This software was developed at the National Institute of Standards and
# Technology (NIST) by employees of the Federal Government in the course
# of their official duties. Pursuant to title 17 Section 105 of the
# United States Code, this software is not subject to copyright protection
# and is in the public domain. NIST assumes no responsibility  whatsoever for
# its use by other parties, and makes no guarantees, expressed or implied,
# about its quality, reliability, or any other characteristic.
#

# This set of directories is where the header files, libraries, programs,
# and man pages are to be installed.
PREFIX := /usr/local
INCPATH := $(PREFIX)/include
LIBPATH := $(PREFIX)/lib
BINPATH := $(PREFIX)/bin
MANPATH := $(PREFIX)/man/man1

#
# If you have the Jasper and GD librares installed, change the '0' to a '1' in
# the following line to enable image features in irextool and libirex.
USE_IMGLIBS = 1

#
# Files and directories that are created during the build process, that
# are to be removed during 'make clean'.
DISPOSABLEFILES = *.o *.exe .gdb_history *.dll *.dylib *.so *.a
DISPOSABLEDIRS  = *.dSYM

#
# Each package that includes this common file can define these variables:
# COMMONINCOPT : Location of include files from other packages, specified
#                as a compiler option (e.g. -I/usr/local/an2k/include
#
# COMMONLIBOPT : Location of libraries from other packages, specified as a
#                compiler option (e.g. -L/usr/local/an2k/lib)
#
# Many ports systems install into /opt/local instead of /usr/local, so
# add those to the common area. If there are other areas to search, append
# the compiler directives to these existing variables.
#

COMMONINCOPT := -I/opt/local/include 
COMMONLIBOPT := -L/opt/local/lib 

# The next set of variables are set by files that include this file, and
# specify the location of package-only files:
#
# LOCALINC : Location where include files are stored.
# LOCALLIB : Location where the libaries are stored.
# LOCALBIN : Location where the programs are stored.
# LOCALMAN : Locatation where the man pages are stored.
#
CP := cp -f
RM := rm -f
PWD := $(shell pwd)
OS := $(shell uname -s)
ARCH := $(shell uname -m)

ifeq ($(findstring CYGWIN,$(OS)), CYGWIN)
	ROOT = Administrator
else
	ROOT  = root
endif

ifeq ($(findstring amd64, $(ARCH)), amd64)
	EXTRACFLAGS = -fPIC
else
	ifeq ($(findstring x86_64, $(ARCH)), x86_64)
		EXTRACFLAGS = -fPIC
	endif
endif

ifeq ($(USE_IMGLIBS), 1)
	EXTRACFLAGS += -DUSE_IMGLIBS
endif

#
# If there are any 'non-standard' include or lib directories that need to
# be searched prior to the 'standard' libraries, add the to the CFLAGS
# variable.

CFLAGS := -g -I$(LOCALINC) -I$(INCPATH) $(COMMONINCOPT) -L$(LOCALLIB) -L$(LIBPATH) $(COMMONLIBOPT) $(EXTRACFLAGS)
