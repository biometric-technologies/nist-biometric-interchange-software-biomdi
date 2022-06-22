/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <biomdimacro.h>
#include <isobit.h>
#include <fmr.h>
#include <nistapdu.h>
#include <tlv.h>
#include <moc.h>

#define BUFSIZE 1024		/* To hold the minutiae template data object */

/*
 * This program reads a file containg an ISO compact card finger minutiae
 * record (a set a 3-byte minutia data records), and calls the libmoc and
 * libtlv libraries to convert and verify the conversion of the FMR to
 * the minutiae template data object. This object is defined in the
 * MINEX-II test specification.
 */ 
static void
usage()
{
	fprintf(stderr, "Usage: testmtdo <filename>\n");
	exit (EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	FILE *fp;
	TLV *mtdotlv;
	BDB *mtdobdb;
	FMR *infmr;
	FVMR **infvmrs;
	void *buf;
	struct stat sb;
	int vcount;

	if (argc != 2)
		usage();

	fp = fopen(argv[optind], "rb");
	if (fp == NULL)
		ERR_EXIT("Open of %s failed: %s", argv[optind],
		    strerror(errno));
	if (fstat(fileno(fp), &sb) < 0)
		ERR_EXIT("Could not get stats on input file");

	if (new_fmr(FMR_STD_ISO_COMPACT_CARD, &infmr) < 0)
		ALLOC_ERR_EXIT("FMR structure");

	if (new_tlv(&mtdotlv, 0, 0) != 0)
		ALLOC_ERR_EXIT("TLV structure");

	mtdobdb = (BDB *)malloc(sizeof(BDB));
	if (mtdobdb == NULL)
		ALLOC_ERR_EXIT("BDB structure");
	if (read_fmr(fp, infmr) != READ_OK)
		ERR_EXIT("Could not read input file");

	vcount = get_fvmr_count(infmr);
	if (vcount == 0)
		ERR_EXIT("FMR contains no views");
	infvmrs = (FVMR **) malloc(vcount * sizeof(FVMR *));
	if (get_fvmrs(infmr, infvmrs) != vcount)
		ERR_EXIT("Could not get views from FMR");

	buf = malloc(BUFSIZE);	/* Should be sufficient for 128 minutiae */
	if (buf == NULL)
		ALLOC_ERR_EXIT("BDB buffer");
	INIT_BDB(mtdobdb, buf, BUFSIZE);

	/* Convert only the first FVMR; this is just a test. */
	if (fvmr_to_mtdo(infvmrs[0], mtdobdb) != WRITE_OK)
		ERR_EXIT("Could not convert FVMR to MTDO");

	REWIND_BDB(mtdobdb);
	if (scan_tlv(mtdobdb, mtdotlv) != READ_OK) {
		ERR_EXIT("Could not scan TLV");
	}
	print_tlv(stdout, mtdotlv);

	free_tlv(mtdotlv);
	free_fmr(infmr);
	free(buf);
	exit (0);
}
