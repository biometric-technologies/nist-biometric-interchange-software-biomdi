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
SOURCES = tlv.c
LOCALINC := ../include
LOCALLIB := ../../lib
include ../../common.mk

all: $(SOURCES)
	test -d $(LOCALLIB) || mkdir $(LOCALLIB)
ifeq ($(OS), Darwin)
	$(CC) -c $(CFLAGS) $(SOURCES)
	libtool -dynamic -o libtlv.dylib -macosx_version_min $(shell sw_vers -productVersion | cut -d. -f1).$(shell sw_vers -productVersion | cut -d. -f2) -lc *.o 
	$(CP) libtlv.dylib $(LOCALLIB)
else
ifeq ($(findstring CYGWIN,$(OS)), CYGWIN)
	$(CC) $(CFLAGS) -c tlv.c
	ar rs libtlv.a tlv.o
	ranlib libtlv.a
	$(CC) -shared -o libtlv.dll -Wl,--out-implib=libtlv.dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive libtlv.a -Wl,--no-whole-archive
	$(CP) libtlv.a $(LOCALLIB)
	$(CP) libtlv.dll.a $(LOCALLIB)
	$(CP) libtlv.dll $(LOCALLIB)
else
	$(CC) $(CFLAGS) -shared $(SOURCES) -o libtlv.so
	$(CP) libtlv.so $(LOCALLIB)
endif
endif

clean:
	$(RM) $(DISPOSABLEFILES)
	$(RM) -r $(DISPOSABLEDIRS)
