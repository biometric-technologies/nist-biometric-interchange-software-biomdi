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
SOURCES = moc.c mocapdu.c
TARGETS = libmoc
LOCALINC := ../include
LOCALLIB := ../../lib
COMMONINCOPT = -I../../../smartcard/src/include
include ../../common.mk

#
# OS-X includes PCSC development headers after installing "Command Line Tools,"
# or by building with xcrun. Refer to Apple Technical Note TN2339 for details:
# https://developer.apple.com/library/ios/technotes/tn2339/_index.html
#
ifneq ($(OS), Darwin)
INCLUDES=-I/usr/include/PCSC -I/usr/local/include/PCSC
endif

all: $(TARGETS)
libmoc: moc.c mocapdu.c
	test -d $(LOCALLIB) || mkdir $(LOCALLIB)
ifeq ($(OS), Darwin)
	$(CC) -c $(CFLAGS) $(INCLUDES) $^
	libtool -dynamic -o libmoc.dylib -macosx_version_min $(shell sw_vers -productVersion | cut -d. -f1).$(shell sw_vers -productVersion | cut -d. -f2) -lc -lfmr moc.o mocapdu.o
	$(CP) libmoc.dylib $(LOCALLIB)
else
ifeq ($(findstring CYGWIN,$(OS)), CYGWIN)
	$(CC) $(CFLAGS) -c $^ $(INCLUDES)
	ar rs libmoc.a moc.o mocapdu.o
	ranlib libmoc.a
	$(CC) -shared -o libmoc.dll -Wl,--out-implib=libmoc.dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive libmoc.a -Wl,--no-whole-archive
	$(CP) libmoc.a $(LOCALLIB)
	$(CP) libmoc.dll.a $(LOCALLIB)
	$(CP) libmoc.dll $(LOCALLIB)
else
	$(CC) $(CFLAGS) -shared $^ $(INCLUDES) -o libmoc.so -lfmr
	$(CP) libmoc.so $(LOCALLIB)
endif
endif

clean:
	$(RM) $(DISPOSABLEFILES)
	$(RM) -r $(DISPOSABLEDIRS)
