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
LOCALMAN := ../../man
COMMONINCOPT = -I/usr/include/PCSC -I/usr/local/include/PCSC
include ../../common.mk
PROGRAMS = testpivcard testpivdata testsubset

all: $(PROGRAMS)
testpivcard: testpivcard.c 
	$(CC) $(CFLAGS) $< -lpiv -lsmc -o $@
testpivdata: testpivdata.c 
	$(CC) $(CFLAGS) $< -lpiv -lfmr -o $@
testsubset: testsubset.c 
	$(CC) $(CFLAGS) $< -lpiv -lfmr -o $@

clean:
	$(RM) *.o $(PROGRAMS)
	$(RM) -rf *.dSYM
	$(RM) .gdb_history
