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
#include <sys/times.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>

#include <biomdimacro.h>
#include <fmr.h>
#include <fmrsort.h>
#include <isobit.h>
#include <tlv.h>
#include <nistapdu.h>
#include <cardaccess.h>
#include <mocapdu.h>
#include <moc.h>

#include "cardutils.h"
#include "genutils.h"

/*
 * This is the test driver program for testing the Match-on-Card functionality
 * of a smartcard. The input file contains a list of FMR template pairs.
 * 
 * The input file is of the form:
 * verify.file enroll.file
 *
 * The steps for testing are:
 * 1)  Read BIT group containing 1 or 2 BITs
 * 2)  Read the card and matcher IDs from the card
 * 2a) Create an output score file; name of the file is based on CBEFF ID
 * 3)  Open the input file
 * 4)  Loop:
 *     4a) read one pair from the template-pairs input file
 *     4b) prune/convert/sort the input templates, using the first finger view
 *     4e) conditionally VERIFY with identical templates to reset card
 *         retry counter
 *     4d) execute MOC VERIFY
 *     4e) record similarity score in output file
 *     4f) record timing values in output file
 * 
 * The time to execute each card APDU will be recorded.
 * The output file will contain information retrieved from the card,
 * prefixed with the '#' comment delimiter. The testing information
 * is written as a series of lines in this format:
 *
 * Column:	Value:
 *    1		Verification template filename (full path)
 *    2		Verification template # minutiae pre-pruning
 *    3		Verification template # minutiae post-pruning
 *    4		Enrollment template filename (full path)
 *    5		Enrollment template # minutiae pre-pruning
 *    6		Enrollment template # minutiae post-pruning
 *    7		Load Enrollment template time (seconds)
 *    8		Matching time (secondss)
 *    9		Exit Status from match_templates() (0 for card test)
 *    10	Matcher decision T/F
 *    11	Get match score time (seconds)
 *    12	Match Score
 */ 
static void
usage()
{
	fprintf(stderr, "Usage: cardtest <filename> [-c] [-d]\n"
	    "\t<filename> is the input file containing minutiae file names\n"
	    "\t-c dump the compact card minutiae records to files\n"
	    "\t-d indicates a dry run, where enroll and verify are not done\n"
	    "\t   and the ENROLL and VERIFY APDUs are dumped to stdout.\n"
	);
	exit (EXIT_FAILURE);
}

/*
 * MOC_RETRY_MIN is the minimum value we will accept in the retry counter
 * from the card. MOC_RESET_RETRY_MAX is the maximum allowed VERIFY attempts,
 * assuming that all fail. Keep this max value at least two below the real
 * max to avoid potentially locking the card the next time this program is run.
 */
#define MOC_RETRY_MIN		3
#define MOC_RESET_RETRY_MAX	(RETRY_COUNTER_MAX - MOC_RETRY_MIN)

static void
create_mtdo(FMR *fmr, BDB *mtdo, void *buf, int *len)
{
	int vcount;
	FVMR **fvmrs;

	vcount = get_fvmr_count(fmr);
	if (vcount == 0)
		ERR_EXIT("FMR contains no views");
	fvmrs = (FVMR **) malloc(vcount * sizeof(FVMR *));
	if (get_fvmrs(fmr, fvmrs) != vcount)
		ERR_EXIT("Could not get views from FMR");

	INIT_BDB(mtdo, buf, RESPONSEBUFSIZE);
        
	/* Convert only the first (and probably only) FVMR. */
	if (fvmr_to_mtdo(fvmrs[0], mtdo) != WRITE_OK)
		ERR_EXIT("Could not convert FVMR to MTDO");

	free(fvmrs);
	*len = mtdo->bdb_current - mtdo->bdb_start;
	/* Reset the pointers in the BDB for the caller */
	REWIND_BDB(mtdo);
}

/* Indices of the verify and enroll templates */
#define V	0
#define E	1

int
main(int argc, char *argv[])
{
	FILE *infp = NULL;
	FILE *outfp = NULL;
	FILE *fmrfp = NULL;
	TLV *bit_group;
	BIT *bit[2] = {NULL, NULL};
	int bit_count;
	BDB cardresponse;
	APDU enrollapdu, verifyapdu;

	char fmrfn[2][MAXPATHLEN];	/* input FMR file names */
	FMR *infmr[2];			/* input FMR data structures */
	FMR *ccfmr[2];			/* compact card form of input FMRs */
	uint16_t cx[2], cy[2];	/* Center of interest coordinate for each FMR */
	int usecm[2];			/* Flag, use center of mass? */
	BDB mtdo[2];			/* minutiae template data objects */
	void *mtdobuf[2] = { NULL, NULL }; /* buffer for the above */
	int mtdolen[2];			/* length of the above MTDOs */

	uint8_t sw1, sw2;
	char cardID[MAXIDSTRINGSIZE + 1], matcherID[MAXIDSTRINGSIZE + 1];
	uint16_t score;
	void *respbuf = NULL;
	int minutiae_count;
	char outfn[MAXPATHLEN];
	struct stat sb;
	int exitcode;
	int resetctr;
	unsigned int iteration;
	int dryrun = 0;
	int dumpcc = 0;
	int ch;
	char rawccfn[32];
	FILE *rawccfp;

	SCARDCONTEXT context;
	SCARDHANDLE hCard;

	char **readers;
	int rdr, rdrcount;
	DWORD rdrprot;

	time_t thetime;
	struct timeval starttm, finishtm;
	double delta_t;

	if ((argc < 2) || (argc > 4))
		usage();

	while ((ch = getopt(argc, argv, "cd")) != -1) {
		switch (ch) {
		case 'c':
			dumpcc = 1;
			break;
		case 'd':
			dryrun = 1;
			break;
		default :
			usage();
			break;
		}
	}
	exitcode = EXIT_FAILURE;
	infp = fopen(argv[optind], "r");
	if (infp == NULL)
		ERR_EXIT("open of %s failed: %s", argv[optind],
		    strerror(errno));

	respbuf = malloc(RESPONSEBUFSIZE);
	if (respbuf == NULL)
		ALLOC_ERR_EXIT("Response BDB buffer");
	INIT_BDB(&cardresponse, respbuf, RESPONSEBUFSIZE);

	/*
	 * Connect to the reader and card. Check readers in order, and use
	 * the first one that contains a MOC card. In the future, we may
	 * want to accept the reader ID as an input parameter.
	 */
	if (SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context) !=
	     SCARD_S_SUCCESS)
		ERR_EXIT("Could not establish contact with reader");
	if (getReaders(context, &readers, &rdrcount) != 0)
		ERR_EXIT("Could not get list of readers.");
	if (rdrcount < 1)
		ERR_EXIT("No readers found");

	for (rdr = 0; rdr < rdrcount; rdr++) {
		printf("\nTrying reader %s\n", readers[rdr]);
		if (SCardConnect(context, readers[rdr], SCARD_SHARE_EXCLUSIVE,
		    SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &rdrprot)
		    != 0) {
			INFOP("Could not connect to card or no card in reader");
			continue;
		}
		/* Select the MOC application. */
		if (sendAPDU(hCard, &MOCSELECTAPP, 0, &cardresponse, &sw1, &sw2)
		    != 0)
			ERR_OUT("Could not send MOC SELECT APDU");
		if (sw1 != APDU_NORMAL_COMPLETE) {

			/* Try using the alternate select APDU. */
			if(sendAPDU(hCard, &ALTMOCSELECTAPP, 0, &cardresponse,
			    &sw1, &sw2) != 0)
				ERR_OUT("Could not send ALT SELECT APDU");
			if (sw1 == APDU_NORMAL_COMPLETE)
				break;
		} else {
			break;
		}

		INFOP("Card in reader is not MOC");
		SCardDisconnect(hCard, SCARD_RESET_CARD);
	}
	if (sw1 != APDU_NORMAL_COMPLETE)
		ERR_EXIT("Could not connect to card");
	printf("\nMOC card found in %s\n", readers[rdr]);
	
	/*
	 * Create a TLV object for the BIT group, and get the group
	 * from the card.
	 */
	if (new_tlv(&bit_group, 0, 0) != 0)
		ALLOC_ERR_EXIT("TLV structure");
	if (get_bitgroup_from_card(hCard, bit_group) != READ_OK)
		ERR_EXIT("Getting BIT group from card");
	if (get_bits_from_tlv(bit, bit_group, &bit_count) != READ_OK)
		ERR_EXIT("Getting BITs from TLV group");
	
	/* If there is only one BIT, we use it for both templates */
	if (bit_count == 1)
		bit[1] = bit[0];
	free_tlv(bit_group);

	/*
	 * Get the card and matcher IDs from the card so we can use them
	 * in the output file.
	 */
	REWIND_BDB(&cardresponse);
	if (sendAPDU(hCard, &MOCGETCARDID, dryrun, &cardresponse, &sw1, &sw2)
	    != 0)
		ERR_OUT("Could not get card ID");
	if (dryrun == 0) {
		CHECKSTATUS("GET CARD ID", sw1, sw2);
		REWIND_BDB(&cardresponse);
		if (getIDinresponse(cardID, &cardresponse) != 0)
			ERR_OUT("Could not get card ID");
	}
	REWIND_BDB(&cardresponse);
	if (sendAPDU(hCard, &MOCGETMATCHERID, dryrun, &cardresponse, &sw1, &sw2)
	    != 0)
		ERR_OUT("Could not get matcher ID");
	if (dryrun == 0) {
		CHECKSTATUS("GET MATCHER ID", sw1, sw2);
		REWIND_BDB(&cardresponse);
		if (getIDinresponse(matcherID, &cardresponse) != 0)
			ERR_OUT("Could not get matcher ID");
	}
	/* The output file name is a combination of card ID and date. */
	if (dryrun)
		STRGENTESTFN(outfn, "dryrun", "dryrun");
	else
		STRGENTESTFN(outfn, cardID, matcherID);
	if (stat(outfn, &sb) == 0)
		ERR_EXIT("File %s exists", outfn);
	if ((outfp = fopen(outfn, "w")) == NULL)
		OPEN_ERR_EXIT(outfn);
	fprintf(outfp, "# Card ID: 0x%s, Matcher ID: 0x%s\n",
	    cardID, matcherID);
	fprintf(outfp, "# There are %u BITs in the BIT group:\n", bit_count);
	fprintf(outfp, "# BIT 1 Info: CBEFF: 0x%04X:%04X :: Minutiae min/max/"
	    "order: %u/%u/0x%02X\n", bit[0]->bit_format_owner,
	    bit[0]->bit_format_type, bit[0]->bit_minutia_min,
	    bit[0]->bit_minutia_max, bit[0]->bit_minutia_order);
	if (bit_count != 1)
		fprintf(outfp, "# BIT 2 Info: CBEFF: 0x%04X:%04X :: "
		    "Minutiae min/max/order: %u/%u/0x%02X\n",
		    bit[1]->bit_format_owner, bit[1]->bit_format_type,
		    bit[1]->bit_minutia_min, bit[1]->bit_minutia_max,
		    bit[1]->bit_minutia_order);
	thetime = time(NULL);
	fprintf(outfp, "# Local Time: %s", ctime(&thetime));
	if (getcwd(outfn, MAXPATHLEN) == NULL)
		ERR_EXIT("Could not get current working directory");
	fprintf(outfp, "# Current working directory is %s\n#\n", outfn);

	enrollapdu = MOCSTORETEMPLATE;
	verifyapdu = MOCVERIFY;
	mtdobuf[V] = malloc(RESPONSEBUFSIZE);
	if (mtdobuf[V] == NULL)
		ALLOC_ERR_EXIT("MTDO[V] BDB buffer");
	mtdobuf[E] = malloc(RESPONSEBUFSIZE);
	if (mtdobuf[E] == NULL)
		ALLOC_ERR_EXIT("MTDO[E] BDB buffer");

	resetctr = MOC_RESET_RETRY_MAX + 1;	/* Force a reset at the start */
	iteration = 1;
	while (1) {
		if (fscanf(infp, "%s %s", &fmrfn[V], &fmrfn[E]) != 2)
			if (feof(infp))
				break;
			else
				ERR_OUT("Reading input file");

		printf("\rIteration: %u", iteration); fflush(stdout);
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
		fprintf(outfp, "%s", fmrfn[V]);
		minutiae_count =
		    get_fmd_count(TAILQ_FIRST(&infmr[V]->finger_views));
		fprintf(outfp, " %d", minutiae_count);
		new_fmr(FMR_STD_ISO_COMPACT_CARD, &ccfmr[V]);
		if (prune_convert_sort_fmr(infmr[V], ccfmr[V],
		    bit[1]->bit_minutia_min, bit[1]->bit_minutia_max,
		    bit[1]->bit_minutia_order, cx[V], cy[V], usecm[V]) != 0)
			ERR_OUT("Pruning/sorting first FMR failed.");
		/* create_mtdo() inits the mtdo BDB blocks... */
		create_mtdo(ccfmr[V], &mtdo[V], mtdobuf[V], &mtdolen[V]);
		minutiae_count =
		    get_fmd_count(TAILQ_FIRST(&ccfmr[V]->finger_views));
		fprintf(outfp, " %d", minutiae_count);

		fprintf(outfp, " %s", fmrfn[E]);
		minutiae_count =
		    get_fmd_count(TAILQ_FIRST(&infmr[E]->finger_views));
		fprintf(outfp, " %d", minutiae_count);
		new_fmr(FMR_STD_ISO_COMPACT_CARD, &ccfmr[E]);
		if (prune_convert_sort_fmr(infmr[E], ccfmr[E],
		    bit[0]->bit_minutia_min, bit[0]->bit_minutia_max,
		    bit[0]->bit_minutia_order, cx[E], cy[E], usecm[E]) != 0)
			ERR_OUT("Pruning/sorting second FMR failed.");
		create_mtdo(ccfmr[E], &mtdo[E], mtdobuf[E], &mtdolen[E]);
		minutiae_count =
		    get_fmd_count(TAILQ_FIRST(&ccfmr[E]->finger_views));
		fprintf(outfp, " %d", minutiae_count);

		/*
		 * Build the APDUs to be sent to the card by adding the
		 * minutiae template data object to the pre-defined APDU's
		 * command data field.
		 */
		add_data_to_apdu((uint8_t *)mtdo[E].bdb_start, mtdolen[E],
		    &enrollapdu);
		REWIND_BDB(&cardresponse);
		gettimeofday(&starttm, 0);
		if (sendAPDU(hCard, &enrollapdu, dryrun, &cardresponse,
		    &sw1, &sw2) != 0)
			ERR_OUT("Could not enroll");
		gettimeofday(&finishtm, 0);
		delta_t = (double)(TIMEINTERVAL(starttm, finishtm)) / 1000000;
		fprintf(outfp, " %f", delta_t);
		if (dryrun == 0)
			CHECKSTATUSWITHRETRY("ENROLL", sw1, sw2, 1, outfp,
			    goto nextone);

		/*
		 * Every so often, perform a guaranteed match to reset the
		 * retry counter on the card. We do this by sending in the
		 * enrolled template for verification.
		 */
		if (resetctr > MOC_RESET_RETRY_MAX) {
			add_data_to_apdu((uint8_t *)mtdo[E].bdb_start,
			    mtdolen[E], &verifyapdu);
			REWIND_BDB(&cardresponse);
			if (sendAPDU(hCard, &verifyapdu, dryrun, &cardresponse,
			    &sw1, &sw2) != 0)
				ERR_OUT("Could not verify");
			if (dryrun == 0)
				CHECKSTATUSWITHRETRY("PERFECT VERIFY",
				    sw1, sw2, 1, outfp, goto nextone);
			resetctr = 0;
		}
		resetctr++;

		add_data_to_apdu((uint8_t *)mtdo[V].bdb_start, mtdolen[V],
		    &verifyapdu);
		REWIND_BDB(&cardresponse);
		/*
		 * Write the compact card records to separate files,
		 * if the user asked for it.
		 */
		if (dumpcc) {
			sprintf(rawccfn, "probe_%d.CC", iteration-1);
			rawccfp = fopen(rawccfn, "wb");
			write_fmr(rawccfp, ccfmr[V]);
			fclose(rawccfp);
			sprintf(rawccfn, "gallery_%d.CC", iteration-1);
			rawccfp = fopen(rawccfn, "wb");
			write_fmr(rawccfp, ccfmr[E]);
			fclose(rawccfp);
		}
		gettimeofday(&starttm, 0);
		if (sendAPDU(hCard, &verifyapdu, dryrun, &cardresponse,
		    &sw1, &sw2) != 0)
			ERR_OUT("Could not verify");
		gettimeofday(&finishtm, 0);
		delta_t = (double)(TIMEINTERVAL(starttm, finishtm)) / 1000000;
		fprintf(outfp, " %f", delta_t);
		if (dryrun == 0)
			CHECKSTATUSWITHRETRY("VERIFY", sw1, sw2, 1, outfp,
			    goto nextone);

		/* Write a 0 to represent the exit status from the 
		 * match_templates() call made in the SDK test.
		 */
		fprintf(outfp, " 0");

		/* Set the true/false match indicator */
		if (sw1 == APDU_NORMAL_COMPLETE)
			fprintf(outfp, " T");
		else
			fprintf(outfp, " F");

		/* Execute GET DATA APDU for similarity score */
		REWIND_BDB(&cardresponse);
		gettimeofday(&starttm, 0);
		if (sendAPDU(hCard, &MOCGETSCORE, 0, &cardresponse, &sw1, &sw2)
		    != 0)
			ERR_OUT("Could not get score");
		gettimeofday(&finishtm, 0);
		delta_t = (double)(TIMEINTERVAL(starttm, finishtm)) / 1000000;
		fprintf(outfp, " %f", delta_t);
		CHECKSTATUS("GET SCORE", sw1, sw2);
		if (( ((uint8_t *)cardresponse.bdb_start)[0] != SCORETAG ) ||
		    ( ((uint8_t *)cardresponse.bdb_start)[1] != SCORESIZE ))
			ERR_OUT("Invalid score tag or length");
		score = ntohs(*(uint16_t *)(cardresponse.bdb_start + 2));
		fprintf(outfp, " %d\n", score);

nextone:
		free_fmr(ccfmr[V]);
		free_fmr(ccfmr[E]);
		free_fmr(infmr[V]);
		free_fmr(infmr[E]);
		iteration++;

	} /* while not EOF */
	printf("\n");
	exitcode = EXIT_SUCCESS;

err_out:
	if (respbuf != NULL)
		free(respbuf);
	if (mtdobuf[V] != NULL)
		free(mtdobuf[V]);
	if (mtdobuf[E] != NULL)
		free(mtdobuf[E]);
	if (bit[0] != NULL)
		free(bit[0]);
	if ((bit_count != 1) && (bit[1] != NULL))
		free(bit[1]);
	if (infp != NULL)
		fclose(infp);
	if (outfp != NULL)
		fclose(outfp);

	exit (exitcode);
}
