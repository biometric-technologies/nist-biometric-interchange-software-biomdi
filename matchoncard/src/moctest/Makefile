#
# This software was developed at the National Institute of Standards and
# Technology (NIST) by employees of the Federal Government in the course
# of their official duties. Pursuant to title 17 Section 105 of the
# United States Code, this software is not subject to copyright protection
# and is in the public domain. NIST assumes no responsibility  whatsoever for
# its use by other parties, and makes no guarantees, expressed or implied,
# about its quality, reliability, or any other characteristic.
#
LOCALINC := ../include
LOCALLIB := ../../lib
LOCALBIN := ../../bin
COMMONINCOPT = -I../../../smartcard/src/include
COMMONLIBOPT = -L../../../smartcard/lib
include ../../common.mk
PROGRAMS = cardinfo cardtest sdktest
UTILS = cardutils.o genutils.o

#
# OS-X includes PCSC development headers after installing "Command Line Tools,"
# or by building with xcrun. Refer to Apple Technical Note TN2339 for details:
# https://developer.apple.com/library/ios/technotes/tn2339/_index.html
#
ifneq ($(OS), Darwin)
INCLUDES=-I/usr/include/PCSC -I/usr/local/include/PCSC
endif

all:	$(UTILS) $(PROGRAMS)

cardutils.o:	cardutils.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -c -o $@

genutils.o:	genutils.c
	$(CC) $(CFLAGS) $(INCLUDES) $^ -c -o $@

cardtest: cardtest.c $(UTILS)
ifeq ($(OS), Darwin)
	$(CC) $(CFLAGS) $(INCLUDES) cardtest.c -o $@ -lmoc -lsmc -lfmr -ltlv -framework PCSC cardutils.o genutils.o
else
	$(CC) $(CFLAGS) $(INCLUDES) cardtest.c -o $@ -lpcsclite -lmoc -lsmc -ltlv -lfmr cardutils.o genutils.o
endif
	$(CP) cardtest $(LOCALBIN)

cardinfo: cardinfo.c cardutils.o
ifeq ($(OS), Darwin)
	$(CC) $(CFLAGS) $(INCLUDES) cardinfo.c -o $@ -lmoc -lsmc -lfmr -ltlv -framework PCSC cardutils.o
else
	$(CC) $(CFLAGS) $(INCLUDES) cardinfo.c -o $@ -lpcsclite -lmoc -lsmc -lfmr -ltlv -lfmr cardutils.o
endif
	$(CP) cardinfo $(LOCALBIN)

sdktest: sdktest.c genutils.o
	$(CC) $(CFLAGS) sdktest.c -o $@ -lmoc -lfmr -ltlv -lfmr genutils.o

clean:
	$(RM) $(PROGRAMS) $(DISPOSABLEFILES)
	$(RM) -r $(DISPOSABLEDIRS)
