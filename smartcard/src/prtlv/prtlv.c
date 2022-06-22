/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

/* Needed by the GNU C libraries for Posix and other extensions */
#define _XOPEN_SOURCE	1

#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <biomdimacro.h>
#include <fmr.h>
#include <tlv.h>

/*
 * This program reads a file containg a Tag-Length-Value record as defined
 * with the constraints described in ISO/IEC 7816-4.
 */ 
static void
usage()
{
	fprintf(stderr, "Usage: prtlv <filename>\n");
	exit (EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	FILE *fp;
	TLV *intlv;
	struct stat sb;

	if (argc != 2)
		usage();

	fp = fopen(argv[optind], "rb");
	if (fp == NULL)
		ERR_EXIT("Open of %s failed: %s", argv[optind],
		    strerror(errno));
	if (fstat(fileno(fp), &sb) < 0)
		ERR_EXIT("Could not get stats on input file");

	if (new_tlv(&intlv, 0, 0) < 0)
		ALLOC_ERR_EXIT("TLV structure");

	if (read_tlv(fp, intlv) != READ_OK)
		ERR_EXIT("Could not read input file");

	print_tlv(stdout, intlv);

	free_tlv(intlv);
	exit (0);
}
