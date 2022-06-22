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
PROGRAMS = m1rec2piv
include ../../common.mk
all: $(PROGRAMS)
m1rec2piv: m1rec2piv.c
	$(CC) $(CFLAGS) $< -lfir -lfmr -lpiv -o $@
	$(CP) $@ $(LOCALBIN)
	$(CP) $@.1 $(LOCALMAN)

clean:
	$(RM) $(PROGRAMS) $(DISPOSABLEFILES)
	$(RM) -r $(DISPOSABLEDIRS)
