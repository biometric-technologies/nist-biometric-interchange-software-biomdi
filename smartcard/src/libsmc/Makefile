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
SOURCES = cardaccess.c readers.c
TARGETS = libsmc
LOCALINC := ../include
LOCALLIB := ../../lib
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

libsmc: cardaccess.c readers.c
	test -d $(LOCALLIB) || mkdir $(LOCALLIB)
ifeq ($(OS), Darwin)
	$(CC) -c $(CFLAGS) $(INCLUDES) $^
	libtool -dynamic -o libsmc.dylib -macosx_version_min $(shell sw_vers -productVersion | cut -d. -f1).$(shell sw_vers -productVersion | cut -d. -f2) -lc -framework PCSC cardaccess.o readers.o
	$(CP) libsmc.dylib $(LOCALLIB)
else
ifeq ($(findstring CYGWIN,$(OS)), CYGWIN)
	$(CC) $(CFLAGS) -c $(SOURCES) $(INCLUDES)
	ar rs libsmc.a cardaccess.o readers.o
	ranlib libsmc.a
	$(CC) -shared -o libsmc.dll -Wl,--out-implib=libsmc.dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive libsmc.a -Wl,--no-whole-archive
	$(CP) libsmc.a $(LOCALLIB)
	$(CP) libsmc.dll.a $(LOCALLIB)
	$(CP) libsmc.dll $(LOCALLIB)
else
	$(CC) $(CFLAGS) -shared $^ $(INCLUDES) -o libsmc.so -lpcsclite
	$(CP) libsmc.so $(LOCALLIB)
endif
endif

clean:
	$(RM) $(DISPOSABLEFILES)
	$(RM) -r $(DISPOSABLEDIRS)

debug: CFLAGS+=-DDEBUG_OUTPUT
debug: all
