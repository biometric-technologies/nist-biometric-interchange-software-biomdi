/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <biomdimacro.h>
#include <fmr.h>
#include <fmrsort.h>
#include <isobit.h>
#include <tlv.h>
#include <moc.h>

#include "genutils.h"

/*
 * This is an example test driver program for testing a finger minutia matching
 * algorithm, implemented within a library or object file, according to the
 * MINEX-II match-on-card specification.
 * NOTE: This program does not produce the same output as the card driver
 * because, well, this is an example program, showing how to convert I378
 * templates to ISO-CC, pruning/sorting on the way, and sending the CC bytes
 * to the SDK. This program does make use of many of the same libraries as
 * the card driver.
 * 
 * The input file contains a 1..n lines of FMR template pairs in this form:
 * verify.file enroll.file
 * 
 * The steps for testing are:
 * 1a) Call the SDK's get_pids() interface to get CBEFF IDs
 * 1b) Read BIT group file, containing 1 or 2 BITs, from a file.
 * 2)  Create an output score file; name of the file is based on CBEFF IDs
 * 3)  Open the input file
 * 4)  Loop:
 *     4a) read one pair from the template-pairs input file
 *     4b) prune/convert/sort the input templates
 *     4c) call the SDK's match_templates() interface
 *     4d) record similarity score in output file
 *
 */ 
static void
usage()
{
	fprintf(stderr, "Usage: sdktest <filename>\n");
	exit (EXIT_FAILURE);
}

/* Indices of the verify and enroll templates */
#define V	0
#define E	1

#define BDBBUFSIZE	1024	/* Buffer for the CC FMRs that go to the SDK */
int
main(int argc, char *argv[])
{
	FILE *infp = NULL;
	FILE *bitfp = NULL;
	FILE *outfp = NULL;
	FILE *fmrfp = NULL;

	void * bitbuf;
	char bitfn[MAXPATHLEN];
	TLV *bit_group;
	BIT *bit[2];
	BDB bitbdb;
	int bit_count;

	char fmrfn[2][MAXPATHLEN];	/* input FMR file names */
	FMR *infmr[2];			/* input FMR data structures */
	FMR *ccfmr[2];			/* compact card form of input FMRs */
	BDB ccbdb[2];			/* buffer wrapper for CC templates */
	void * ccbdbbuf[2] = {NULL, NULL}; /* memory for above wrapper */
	uint16_t cx[2], cy[2];	/* Center of interest coordinate for each FMR */
	int usecm[2];			/* Flag, use center of mass? */

	uint32_t genID, matcherID;
	uint16_t score;
	char outfn[MAXPATHLEN];
	struct stat sb;
	int32_t rv;
	int exitcode;

	time_t thetime;
	struct timeval starttm, finishtm;

	if (argc != 2)
		usage();

	exitcode = EXIT_FAILURE;
	infp = fopen(argv[optind], "r");
	if (infp == NULL)
		OPEN_ERR_EXIT(argv[optind]);

	/*
	 * Get the card and matcher IDs so we can use them to read the
	 * BIT file, and to name the results output file.
	 */
	//rv = get_pids(&genID, &matcherID);
	if (rv != 0)
		ERR_OUT("Could not get IDs: Return value is %d", rv);

	GENBITFN(bitfn, genID, matcherID);
	bitfp = fopen(bitfn, "rb");
	if (bitfp == NULL)
		OPEN_ERR_EXIT(bitfn);
	if (fstat(fileno(bitfp), &sb) < 0)
		ERR_EXIT("Could not get stats on BIT file %s", bitfn);

	/*
	 * Create a TLV object for the BIT group, and get the group
	 * from the file.
	 */
	if (new_tlv(&bit_group, 0, 0) != 0)
		ALLOC_ERR_EXIT("TLV structure");
	bitbuf = malloc(sb.st_size);
	if (bitbuf == NULL)
		ALLOC_ERR_EXIT("BDB structure");
	if (fread(bitbuf, 1, sb.st_size, bitfp) != sb.st_size)
		ERR_EXIT("Could not read BIT file %s", bitfn);
	INIT_BDB(&bitbdb, bitbuf, sb.st_size);
	if (scan_tlv(&bitbdb, bit_group) != READ_OK)
		ERR_EXIT("Could not scan BIT TLV");
	if (get_bits_from_tlv(bit, bit_group, &bit_count) != READ_OK)
		ERR_EXIT("Getting BITs from TLV group");
	free(bitbuf);
	
	/* If there is only one BIT, we use it for both templates */
	if (bit_count == 1)
		bit[1] = bit[0];
	free_tlv(bit_group);

	/* The output file name is a combination of SDK and matcher IDs. */
	GENTESTFN(outfn, genID, matcherID);
	if (stat(outfn, &sb) == 0)
		ERR_EXIT("File %s exists", outfn);
	if ((outfp = fopen(outfn, "w")) == NULL)
		OPEN_ERR_EXIT(outfn);
	fprintf(outfp, "Generator ID: 0x%08X, Matcher ID: 0x%08X\n",
	    genID, matcherID);
	fprintf(outfp, "There are %u BITs in the BIT group:\n", bit_count);
	fprintf(outfp, "BIT 1 Info: CBEFF: 0x%04X:%04X :: Minutiae min/max/"
	    "order: %u/%u/0x%02X\n", bit[0]->bit_format_owner,
	    bit[0]->bit_format_type, bit[0]->bit_minutia_min,
	    bit[0]->bit_minutia_max, bit[0]->bit_minutia_order);
	if (bit_count != 1)
		fprintf(outfp, "BIT 2 Info: CBEFF: 0x%04X:%04X :: "
		    "Minutiae min/max/order: %u/%u/0x%02X\n",
		    bit[1]->bit_format_owner, bit[1]->bit_format_type,
		    bit[1]->bit_minutia_min, bit[1]->bit_minutia_max,
		    bit[1]->bit_minutia_order);
	thetime = time(NULL);
	fprintf(outfp, "Local Time: %s", ctime(&thetime));

	/* Allocated the data block buffers for stroing the FMR
	 * in memory so it can be passed into the SDK.
	 */
	ccbdbbuf[V] = malloc(BDBBUFSIZE);
	if (ccbdbbuf[V] == NULL)
		ALLOC_ERR_OUT("BDB buffer");
	INIT_BDB(&ccbdb[V], ccbdbbuf[V], BDBBUFSIZE);
	ccbdbbuf[E] = malloc(BDBBUFSIZE);
	if (ccbdbbuf[E] == NULL)
		ALLOC_ERR_OUT("BDB buffer");
	INIT_BDB(&ccbdb[E], ccbdbbuf[E], BDBBUFSIZE);

	while (1) {
		if (fscanf(infp, "%s %s", &fmrfn[V], &fmrfn[E]) != 2)
			if (feof(infp))
				break;
			else
				ERR_OUT("Reading input file");
		fprintf(outfp, "%s %s", fmrfn[V], fmrfn[E]);

		new_fmr(FMR_STD_ANSI, &infmr[V]);
		new_fmr(FMR_STD_ANSI, &infmr[E]);

		/* Read verification minutiae file */
		fmrfp = fopen(fmrfn[V], "rb");
		if (fmrfp == NULL)
			ERR_OUT("Could not open FMR file %s", fmrfn[V]);
		if (read_fmr(fmrfp, infmr[V]) != READ_OK)
			ERR_OUT("Could not read FMR file %s", fmrfn[V]);
		fclose(fmrfp);
		CHOOSEPRUNECENTER(fmrfn[V], infmr[V], cx[V], cy[V], usecm[V]);

		/* Read enrollment minutiae file */
		fmrfp = fopen(fmrfn[E], "rb");
		if (fmrfp == NULL)
			ERR_OUT("Could not open FMR file %s", fmrfn[E]);
		if (read_fmr(fmrfp, infmr[E]) != READ_OK)
			ERR_OUT("Could not read FMR file %s", fmrfn[E]);
		fclose(fmrfp);
		CHOOSEPRUNECENTER(fmrfn[E], infmr[E], cx[E], cy[E], usecm[E]);

		/*
		 * The first BIT is applied to the enrollment template, the
		 * second to the verify template, as per the MINEX-II test spec.
		 */
		new_fmr(FMR_STD_ISO_COMPACT_CARD, &ccfmr[V]);
		if (prune_convert_sort_fmr(infmr[V], ccfmr[V],
		    bit[1]->bit_minutia_min, bit[1]->bit_minutia_max,
		    bit[1]->bit_minutia_order, cx[V], cy[V], usecm[V]) != 0)
			ERR_OUT("Pruning/sorting first FMR failed.");
		new_fmr(FMR_STD_ISO_COMPACT_CARD, &ccfmr[E]);
		if (prune_convert_sort_fmr(infmr[E], ccfmr[E],
		    bit[0]->bit_minutia_min, bit[0]->bit_minutia_max,
		    bit[0]->bit_minutia_order, cx[E], cy[E], usecm[E]) != 0)
			ERR_OUT("Pruning/sorting second FMR failed.");
	
		/*
		 * The CC FMR needs to be passed into the SDK as a simple
		 * byte array, so convert the FMR into a buffer allocated
		 * above.
		 */
		REWIND_BDB(&ccbdb[V]);
		if (push_fmr(&ccbdb[V], ccfmr[V]) != WRITE_OK)
			ERR_OUT("Could not push CC FMR to buffer");
		REWIND_BDB(&ccbdb[E]);
		if (push_fmr(&ccbdb[E], ccfmr[E]) != WRITE_OK)
			ERR_OUT("Could not push CC FMR to buffer");

		gettimeofday(&starttm, 0);
		//rv = match_templates(
		//    (uint8_t *)ccbdb[V].bdb_start,
		//    ccbdb[V].bdb_current - ccbdb[V].bdb_start,
		//    (uint8_t *)ccbdb[E].bdb_start,
                //    ccbdb[E].bdb_current - ccbdb[E].bdb_start,
		//    &score);
		if (rv != 0)
			ERR_OUT("Could not get score: Return value is %d", rv);
		gettimeofday(&finishtm, 0);
		fprintf(outfp, " %u", TIMEINTERVAL(starttm, finishtm));
		fprintf(outfp, " %d\n", score);

		free_fmr(ccfmr[V]);
		free_fmr(ccfmr[E]);
		free_fmr(infmr[V]);
		free_fmr(infmr[E]);

	} /* while not EOF */
	exitcode = EXIT_SUCCESS;

err_out:
	if (infp != NULL)
		fclose(infp);
	if (outfp != NULL)
		fclose(outfp);
	if (bitfp != NULL)
		fclose(bitfp);
	if (ccbdbbuf[V] != NULL)
		free(ccbdbbuf[V]);
	if (ccbdbbuf[E] != NULL)
		free(ccbdbbuf[E]);
	if (bit[0] != NULL)
		free(bit[0]);
	if (bit[1] != NULL)
		free(bit[1]);

	exit (exitcode);
}
