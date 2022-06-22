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
include ../../common.mk
all: irextool
irextool: irextool.c
ifeq ($(USE_IMGLIBS), 1)
	$(CC) $(CFLAGS) $? -lm -liid -lmanipirex -ljasper -lgd -o $@
else
	$(CC) $(CFLAGS) $? -lm -liid -lmanipirex -o $@
endif
	$(CP) $@ $(LOCALBIN)
	$(CP) $@.1 $(LOCALMAN)

clean:
	$(RM) irextool $(DISPOSABLEFILES)
	$(RM) -r $(DISPOSABLEDIRS)
