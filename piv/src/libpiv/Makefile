#
# This software was developed at the National Institute of Standards and
# Technology (NIST) by employees of the Federal Government in the course
# of their official duties. Pursuant to title 17 Section 105 of the
# United States Code, this software is not subject to copyright protection
# and is in the public domain. NIST assumes no responsibility  whatsoever for
# its use by other parties, and makes no guarantees, expressed or implied,
# about its quality, reliability, or any other characteristic.
#
# Set a variable so we can check the OS name; Mac OS-X (Darwin) uses a different
# form of linking libraries.
#
LOCALINC := ../include
LOCALLIB := ../../lib
LOCALBIN := ../../bin
LOCALMAN := ../../man
COMMONINCOPT = -I/usr/include/PCSC -I/usr/local/include/PCSC
include ../../common.mk

OS := $(shell uname -s)
SOURCES = piv.c pivcard.c pivdata.c
OBJECTS = piv.o pivcard.o pivdata.o

all: $(SOURCES)
ifeq ($(OS), Darwin)
	$(CC) -c $(CFLAGS) $(SOURCES)
	libtool -dynamic -o libpiv.dylib -lc -lfrf -lfmr -ltlv -lsmc -framework PCSC -macosx_version_min $(shell sw_vers -productVersion | cut -d. -f1).$(shell sw_vers -productVersion | cut -d. -f2) $(OBJECTS) 
	$(CP) libpiv.dylib $(LOCALLIB)
else
ifeq ($(findstring CYGWIN,$(OS)), CYGWIN)
	$(CC) $(CFLAGS) -c $(SOURCES)
	ar rs libpiv.a $(OBJECTS)
	ranlib libpiv.a
	$(CC) -shared -o libpiv.dll -Wl,--out-implib=libpiv.dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive libpiv.a -Wl,--no-whole-archive
	$(CP) libpiv.a $(LOCALLIB)
	$(CP) libpiv.dll.a $(LOCALLIB)
	$(CP) libpiv.dll $(LOCALLIB)
else
	$(CC) $(CFLAGS) -shared $(SOURCES) -lfrf -lfmr -ltlv -lsmc -o libpiv.so
	$(CP) libpiv.so $(LOCALLIB)
endif
endif

clean:
	$(RM) $(DISPOSABLEFILES)
	$(RM) -r $(DISPOSABLEDIRS)
