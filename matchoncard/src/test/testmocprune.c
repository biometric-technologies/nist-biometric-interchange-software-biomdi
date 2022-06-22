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
 * This is the test driver program for pruning/sorting/conversion
 * functionality implemented in the match-on-card test harness.
 * The minimum/maximum minutia, and sorting criteria are given in
 * the input file and is encoded as documented in the MINEX-II spec.
 *
 * The input file format:
 * template-filename min max order, e.g.
 * B2Example.raw 12 60 0x05
 *
 * and the program will produce a file call B2Example.raw.cc, overwriting
 * any existing file of that name.
 * 
 * The steps for testing are:
 * 1)  Loop:
 *     1a) read a template file name from the input file
 *     1b) read the template file
 *     1c) prune/convert/sort the input template
 *     1d) write the pruned/sorted/converted template to a file named
 *         after the template file name.
 */ 
static void
usage()
{
	fprintf(stderr, "Usage: testmocprune <filename>\n");
	exit (EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	FILE *infp = NULL;
	FILE *infmrfp = NULL;
	FILE *outccfp = NULL;
	FMR *infmr;
	FMR *ccfmr;
	char infmrfn[MAXPATHLEN];
	char outccfn[MAXPATHLEN];
	struct stat sb;
	int exitcode;
	uint8_t minutia_min, minutia_max, minutia_order;

	if (argc != 2)
		usage();

	exitcode = EXIT_FAILURE;
	infp = fopen(argv[optind], "r");
	if (infp == NULL)
		OPEN_ERR_EXIT(argv[optind]);

	while (1) {
		if (fscanf(infp, "%s %hhu %hhu %hhx",
		    &infmrfn, &minutia_min, &minutia_max, &minutia_order) != 4)
			if (feof(infp))
				break;
			else
				ERR_OUT("Reading input file");
		snprintf(outccfn, MAXPATHLEN, "%s.cc", infmrfn);
		new_fmr(FMR_STD_ANSI, &infmr);

		/* Read minutiae file */
		infmrfp = fopen(infmrfn, "rb");
		if (infmrfp == NULL)
			ERR_OUT("Could not open FMR file %s", infmrfn);
		if (read_fmr(infmrfp, infmr) != READ_OK)
			ERR_OUT("Could not read FMR file %s", infmrfn);
		fclose(infmrfp);
		infmrfp = NULL;

		new_fmr(FMR_STD_ISO_COMPACT_CARD, &ccfmr);
		if (prune_convert_sort_fmr(infmr, ccfmr,
		    minutia_min, minutia_max, minutia_order, 0, 0, TRUE) != 0)
			ERR_OUT("Pruning/sorting FMR failed.");

		outccfp = fopen(outccfn, "wb+");
		if (outccfp == NULL)
			ERR_OUT("Could not open CC file %s", outccfp);
		write_fmr(outccfp, ccfmr);
		fclose(outccfp);
		outccfp = NULL;
		free_fmr(ccfmr);
		free_fmr(infmr);

	} /* while not EOF */
	exitcode = EXIT_SUCCESS;

err_out:
	if (infp != NULL)
		fclose(infp);
	if (infmrfp != NULL)
		fclose(infmrfp);
	if (outccfp != NULL)
		fclose(outccfp);
	exit (exitcode);
}
