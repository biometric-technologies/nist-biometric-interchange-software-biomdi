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
all: irexSDKv

# IREX participants should edit the CC line below to link their library
# typically -lIREX
irexSDKv: irexSDKv.c dummy.c
ifeq ($(USE_IMGLIBS), 1)
	$(CC) -Wall $(CFLAGS) $? -lm -liid -lmanipirex -ljasper -lgd -o $@
else
	$(CC) -Wall $(CFLAGS) -L. $? -lmanipirex -liid -lm -o $@
endif
	$(CP) $@ $(LOCALBIN)
	$(CP) $@.1 $(LOCALMAN)
	mkdir -p output/t6/croponly output/t6/cropmask output/t6/unsegpolar
	mkdir -p output/templates/croponly output/templates/cropmask output/templates/unsegpolar
	mkdir -p output/matching/croponly output/matching/cropmask output/matching/unsegpolar

clean:
	$(RM) irexSDKv $(DISPOSABLEFILES)
	$(RM) -r $(DISPOSABLEDIRS)
