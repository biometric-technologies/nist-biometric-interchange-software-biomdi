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
COMMONINCOPT = -I../../../smartcard/src/include
COMMONLIBOPT = -L../../../smartcard/lib
include ../../common.mk
PROGRAMS = testcardaccess testmtdo testmocprune

#
# OS-X includes PCSC development headers after installing "Command Line Tools,"
# or by building with xcrun. Refer to Apple Technical Note TN2339 for details:
# https://developer.apple.com/library/ios/technotes/tn2339/_index.html
#
ifneq ($(OS), Darwin)
INCLUDES=-I/usr/include/PCSC -I/usr/local/include/PCSC
endif

all:	$(PROGRAMS)

testtlv: testtlv.c
	$(CC) $(CFLAGS) $^ -o $@ -lmoc -ltlv -lfmr

testmtdo: testmtdo.c
	$(CC) $(CFLAGS) $^ -o $@ -lmoc -ltlv -lfmr

testmocprune: testmocprune.c
	$(CC) $(CFLAGS) $^ -o $@ ../moctest/genutils.o -lfmr -I../moctest

testcardaccess:	testcardaccess.c
ifeq ($(OS), Darwin)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lmoc -lsmc -lfmr -framework PCSC
else
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ -lpcsclite -lmoc -lsmc -lfmr
endif

clean:
	$(RM) $(PROGRAMS) $(DISPOSABLEFILES)
	$(RM) -r $(DISPOSABLEDIRS)
