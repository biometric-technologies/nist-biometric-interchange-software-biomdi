/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/
/******************************************************************************/
/* This program will validate the fingerprint minutiae records, fingerprint   */
/* image records, and the the CBEFF header. The FMR and FIR libraries are     */
/* used to validate the fingerprint records per the ANSI INCITS 378-2004 and  */
/* 381-2004 specifications. The additional constraints of the NIST            */
/* SP-800-76 PIV specification are verified within this program.              */
/*                                                                            */
/******************************************************************************/

/* Needed by the GNU C libraries for Posix and other extensions */
#define _XOPEN_SOURCE	1

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/queue.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <biomdi.h>
#include <biomdimacro.h>
#include <fmr.h>
#include <piv.h>
#include <pivdata.h>

int
main(int argc, char *argv[])
{
	char *usage = "usage: testsubset <datafile>";
	FILE *infp, *outfp;
	struct stat sb;
	int ret;
	uint8_t *inbuf, *outbuf;
	int inbuflen, outbuflen;
	BDB inbdb, outbdb;
	FMR *infmr;
	FVMR **fvmrs;
	int count, i;
	char *infn, outfn[256];

	if (argc != 2) {
		fprintf(stderr, "%s\n", usage);
		exit (EXIT_FAILURE);
	}

	if (argv[1] == NULL) {
		printf("%s\n", usage);
		exit (EXIT_FAILURE);
	}
	infn = argv[1];

	if (stat(infn, &sb) != 0) {
		ERRP("Stat of %s failed: %s.", infn, strerror(errno));
		exit (EXIT_FAILURE);
	}

	infp = fopen(infn, "rb");
	if (infp == NULL) {
		ERRP("Open of %s failed: %s.", infn, strerror(errno));
		exit (EXIT_FAILURE);
	}
	inbuf = malloc(sb.st_size);
	inbuflen = sb.st_size;
	if (fread(inbuf, 1, sb.st_size, infp) != sb.st_size) {
		ERRP("Read of %s failed: %s.", infn, strerror(errno));
		exit (EXIT_FAILURE);
	}
	INIT_BDB(&inbdb, inbuf, inbuflen);
	new_fmr(FMR_STD_ANSI, &infmr);
	ret = scan_fmr(&inbdb, infmr);
	if (ret != READ_OK) {
		ERRP("FMR scan of %s failed.", infn);
		exit (EXIT_FAILURE);
	}
        count = get_fvmr_count(infmr);
	outbuf = malloc(PIV_MAX_OBJECT_SIZE);
	outbuflen = PIV_MAX_OBJECT_SIZE;
	INIT_BDB(&outbdb, outbuf, outbuflen);
	fvmrs = malloc(count * sizeof(FVMR *));
	get_fvmrs(infmr, fvmrs);
	for (i = 1; i <= count; i++) {
		outbuflen = PIV_MAX_OBJECT_SIZE;
		ret = pivSubsetFingerMinutiaeRec(i, inbuf, inbuflen,
		    outbuf, &outbuflen);
		sprintf(outfn, "%s-FP%u-%u", basename(infn),
		    fvmrs[i-1]->finger_number, fvmrs[i-1]->view_number);
		outfp = fopen(outfn, "wb");
		if (fwrite(outbuf, 1, outbuflen, outfp) != outbuflen) {
			ERRP("Write of %s failed: %s.", infn,
			    strerror(errno));
			exit (EXIT_FAILURE);
		}
		fclose(outfp);
	}

	fclose(infp);
	free(inbuf);
	free(outbuf);
	free_fmr(infmr);
	free(fvmrs);
	exit(EXIT_SUCCESS);
}
