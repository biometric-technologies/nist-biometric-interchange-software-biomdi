#
# This software was developed at the National Institute of Standards and
# Technology (NIST) by employees of the Federal Government in the course
# of their official duties. Pursuant to title 17 Section 105 of the
# United States Code, this software is not subject to copyright protection
# and is in the public domain. NIST assumes no responsibility  whatsoever for
# its use by other parties, and makes no guarantees, expressed or implied,
# about its quality, reliability, or any other characteristic.
#
SOURCES = irex.c bilinear_interpolation.c bicubic_interpolation.c fcc.c
OBJECTS = irex.o bilinear_interpolation.o bicubic_interpolation.o fcc.o
LIBRARY = libmanipirex

LOCALINC = ../include
LOCALLIB = ../../lib
LOCALBIN = ../../bin
LOCALMAN = ../../man

include ../../common.mk

all: $(SOURCES)
ifeq ($(OS), Darwin)
	$(CC) -c $(CFLAGS) $(SOURCES)
ifeq ($(USE_IMGLIBS), 1)
	libtool -dynamic -o $(LIBRARY).dylib -macosx_version_min $(shell sw_vers -productVersion | cut -d. -f1).$(shell sw_vers -productVersion | cut -d. -f2) -lc $(OBJECTS) $(COMMONLIBOPT) -ljasper
else
	libtool -dynamic -o $(LIBRARY).dylib -macosx_version_min $(shell sw_vers -productVersion | cut -d. -f1).$(shell sw_vers -productVersion | cut -d. -f2) -lc $(OBJECTS) $(COMMONLIBOPT) 
endif
	$(CP) $(LIBRARY).dylib $(LOCALLIB)
else
ifeq ($(findstring CYGWIN,$(OS)), CYGWIN)
	$(CC) $(CFLAGS) -c $(SOURCES)
	ar rs $(LIBRARY).a $(OBJECTS)
	ranlib $(LIBRARY).a
	$(CC) -shared -o $(LIBRARY).dll -Wl,--out-implib=$(LIBRARY).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive $(LIBRARY).a -Wl,--no-whole-archive
	$(CP) $(LIBRARY).a $(LOCALLIB)
	$(CP) $(LIBRARY).dll.a $(LOCALLIB)
	$(CP) $(LIBRARY).dll $(LOCALLIB)
else
	$(CC) $(CFLAGS) -shared $(SOURCES) -o $(LIBRARY).so
	$(CC) -m32 $(CFLAGS) -shared $(SOURCES) -o $(LIBRARY)32.so
	$(CP) *.so $(LOCALLIB)
endif
endif

clean:
	$(RM) $(DISPOSABLEFILES)
	$(RM) -r $(DISPOSABLEDIRS)
