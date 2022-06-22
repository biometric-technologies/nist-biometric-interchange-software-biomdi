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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <biomdi.h>
#include <biomdimacro.h>
#include <fir.h>
#include <fmr.h>
#include <piv.h>

#define IOERR_EXIT 	-1
#define RECERR_EXIT	-2
#define OTHERR_EXIT	-3

static int
piv_verify_fmr(struct finger_minutiae_record *fmr)
{
	int ret = VALIDATE_OK;
	struct finger_view_minutiae_record *fvmr;
	struct finger_minutiae_data *fmd;

	// Check the header bits
	if (fmr->record_length_type != FMR_ANSI_SMALL_HEADER_TYPE) {
		ERRP("FMR header length is incorrect.");
		ret = VALIDATE_ERROR;
	}
	NCSR(fmr->record_length, 0, "Record Length");
	NCSR(fmr->product_identifier_type, 0, "CBEFF Product Identifier Type");
	CSR(fmr->compliance, 8, "Capture Equipment Compliance");
	NCSR(fmr->scanner_id, 0, "Capture Equipment ID");
	CSR(fmr->x_resolution, 197, "X-Resolution");
	CSR(fmr->y_resolution, 197, "Y-Resolution");
	CSR(fmr->num_views, 2, "Number of Finger Views");
	CSR(fmr->reserved, 0, "Header Reserved Field");

	// Check the Finger View record
	fvmr = TAILQ_FIRST(&fmr->finger_views);
	if (fvmr == NULL) {
		ERRP("There are no finger views.");
		return (VALIDATE_ERROR);
	}

	// PIV states there is a single view
	// The 02/01/06 version of 800-76 states view number shall be 1,
	// but this conflicts with 378-2004 which states numbers start
	// at 0. Use 0 here.
	// Errata for 800-76 corrects the view number requirement to be 0.
	CSR(fvmr->view_number, 0, "Finger view number");

	if ((fvmr->impression_type != LIVE_SCAN_PLAIN) &&
	    (fvmr->impression_type != NONLIVE_SCAN_PLAIN)) {
		ERRP("Impression type is invalid.");
		ret = VALIDATE_ERROR;
	}
	if ((fvmr->finger_quality != 20) && (fvmr->finger_quality != 40) &&
	    (fvmr->finger_quality != 60) && (fvmr->finger_quality != 80) &&
	    (fvmr->finger_quality != 100)) {
		ERRP("Finger quality is invalid.");
		ret = VALIDATE_ERROR;
	}
	if (fvmr->number_of_minutiae > 128) {
		ERRP("Number of minutiae is invalid.");
		ret = VALIDATE_ERROR;
	}

	// Check each minutia record
	TAILQ_FOREACH(fmd, &fvmr->minutiae_data, list) {
		// XXX Check position within image
		// XXX Check angle?
		// Check of type is done in libfmr
	}

	// Check the extended data attached to the FVMR
	if (fvmr->extended != NULL) {
		ERRP("There is extended data on the FVMR.");
		ret = VALIDATE_ERROR;
	}

	return (ret);
}

static int
process_fmr(FILE *infp, int pflag, int vflag)
{
	struct finger_minutiae_record *fmr;
	int ret;

	if (new_fmr(FMR_STD_ANSI, &fmr) < 0) {
		fprintf(stderr, "Could not allocate FMR\n");
		return (-1);
	}

	if (read_fmr(infp, fmr) != READ_OK) {
		fprintf(stderr, "Could not read FMR.\n");
		if (pflag) {
			printf("Partial record:\n---------------\n");
			print_fmr(stdout, fmr);
		}
		free_fmr(fmr);
		return (-1);
	}

	if (pflag)
		print_fmr(stdout, fmr);

	if (vflag) {
		fprintf(stdout, "Finger Minutiae Record validation:\n");
		if (validate_fmr(fmr) == VALIDATE_OK)
			fprintf(stdout, "\tPasses INCITS-378 criteria.\n");
		else
			fprintf(stdout, "\tDoes NOT pass 378 criteria.\n");

		if (piv_verify_fmr(fmr) == VALIDATE_OK)
			fprintf(stdout, "\tPasses PIV criteria.\n");
		else
			fprintf(stdout, "\tDoes NOT pass PIV criteria.\n");
	}
	free_fmr(fmr);
	ret = fmr->record_length;
	return (ret);
}

static int
piv_verify_fir(struct finger_image_record *fir)
{
	int ret = VALIDATE_OK;
	struct finger_image_view_record *fivr;
	struct finger_image_view_record **fivrs;
	int count, i;

	NCSR(fir->record_length, 0, "Record Length");
	NCSR(fir->scanner_id, 0, "Capture Device ID");
	CSR(fir->image_acquisition_level, 31, "Image Acquisition Level");
	CSR(fir->scale_units, FIR_SCALE_UNITS_CM, "Scale Units");
	CSR(fir->x_scan_resolution, 197, "X Scan Resolution");
	CSR(fir->y_scan_resolution, 197, "Y Scan Resolution");
	CSR(fir->x_image_resolution, 197, "X Image Resolution");
	CSR(fir->y_image_resolution, 197, "Y Image Resolution");
	CSR(fir->pixel_depth, 8, "Pixel Depth");
	if ((fir->image_compression_algorithm !=
	    COMPRESSION_ALGORITHM_UNCOMPRESSED_NO_BIT_PACKED) &&
	    (fir->image_compression_algorithm !=
		COMPRESSION_ALGORITHM_COMPRESSED_WSQ)) {
		ERRP("Image Compression Algorithm is invalid.");
		ret = VALIDATE_ERROR;
	}

	// Check the Finger Image View records
	count = get_fivr_count(fir);
	if (count == 0) {
		ERRP("Count of finger images is 0.");
		return (VALIDATE_ERROR);
	}
	fivrs = (struct finger_image_view_record **) malloc(
	    count * sizeof(struct finger_image_view_record *));
	if (fivrs == NULL) {
		ret = VALIDATE_ERROR;
		ALLOC_ERR_OUT("array of FIVRs");
	}
	if (get_fivrs(fir, fivrs) < 0) {
		ret = VALIDATE_ERROR;
		ERR_OUT("Could not get FIVRs from FIR.");
	}
	for (i = 0; i < count; i++) {
		fivr = fivrs[i];
		if ((fivr->quality != 20) && (fivr->quality != 40) &&
		    (fivr->quality != 60) && (fivr->quality != 80) &&
		    (fivr->quality != 100)) {
			ERRP("Finger quality is invalid");
			ret = VALIDATE_ERROR;
		}
		if ((fivr->impression_type != LIVE_SCAN_PLAIN) &&
		    (fivr->impression_type != NONLIVE_SCAN_PLAIN)) {
			ERRP("Impression type is invalid");
			ret = VALIDATE_ERROR;
		}
	}

err_out:
	return (ret);
}

static int
process_fir(FILE *infp, int pflag, int vflag)
{
	struct finger_image_record *fir;
	int ret;

	if (new_fir(FIR_STD_ANSI, &fir) < 0) {
		fprintf(stderr, "Could not allocate FIR\n");
		return (-1);
	}

	if (read_fir(infp, fir) != READ_OK) {
		fprintf(stderr, "Could not read FIR.\n");
		if (pflag) {
			printf("Partial record:\n---------------\n");
			print_fir(stdout, fir);
		}
		free_fir(fir);
		return (-1);
	}

	if (pflag)
		print_fir(stdout, fir);

	if (vflag) {
		fprintf(stdout, "Finger Image Record validation:\n");
		if (validate_fir(fir) == VALIDATE_OK)
			fprintf(stdout, "\tPasses INCITS-381 criteria.\n");
		else
			fprintf(stdout, "\tDoes NOT pass INCITS-381 criteria.\n");

		if (piv_verify_fir(fir) == VALIDATE_OK)
			fprintf(stdout, "\tPasses PIV criteria.\n");
		else
			fprintf(stdout,
			    "\tDoes NOT pass PIV criteria.\n");
	}
	ret = fir->record_length;
	free_fir(fir);
	return (ret);
}

int
main(int argc, char *argv[])
{
	char *usage = "usage: pivv [-p] [-c] [-b] <datafile>\n"
	    "\t -p Print the record\n"
	    "\t -c Validate the CBEFF header\n"
	    "\t -b Validate the Biometric Data Block\n";
	FILE *fp;
	struct piv_cbeff_record pcr;
	int ch;
	int exit_code = EXIT_SUCCESS;
	int pflag, cflag, bflag;
	int ret;
	int len;
	int type;
	int i;

	if ((argc != 2) && (argc != 3)) {
		fprintf(stderr, "%s\n", usage);
		exit (EXIT_FAILURE);
	}

	pflag = cflag = bflag = 0;
	while ((ch = getopt(argc, argv, "pcb")) != -1) {
		switch (ch) {
			case 'p' :
				pflag = 1;
				break;
			case 'c' :
				cflag = 1;
				break;
			case 'b' :
				bflag = 1;
				break;
			default :
				printf("%s\n", usage);
				exit (EXIT_FAILURE);
		break;
		}
	}

	if (argv[optind] == NULL) {
		printf("%s\n", usage);
		exit (EXIT_FAILURE);
	}

	if (!cflag && !bflag)
		cflag = bflag = 1;

	fp = fopen(argv[optind], "rb");
	if (fp == NULL) {
		ERRP("Open of %s failed: %s.", argv[1], strerror(errno));
		exit (EXIT_FAILURE);
	}

	i = 0;
	for (;;) {
		i++;
		ret = piv_read_pcr(fp, &pcr);
		if (ret == READ_EOF)
			break;
		fprintf(stdout,
		    "== Biometric Entry %03d ======================================================\n", i);
		if (ret == READ_ERROR) {
			fprintf(stderr, "Could not read CBEFF header\n");
			exit (EXIT_FAILURE);
		}
		if (pflag)
			piv_print_pcr(stdout, &pcr);

		switch (pcr.bdb_format_type) {
			case PIV_FORMAT_TYPE_FINGER_MINUTIAE:
				len = process_fmr(fp, pflag, bflag);
				type = PIVFMR;
				break;
			case PIV_FORMAT_TYPE_FINGER_IMAGE:
				len = process_fir(fp, pflag, bflag);
				type = PIVFIR;
				break;
			case PIV_FORMAT_TYPE_FACE_IMAGE:
				INFOP("CBEFF indicates Face; skipping...");
				len = pcr.bdb_length;
				/* Skip over the facial image block */
				fseek(fp, len, SEEK_CUR);
				type = PIVFRF;
				break;
			default:
				ERRP("Unsupported Biometric Data Block Format Type");
				break;
		}
		if (len < 0)
			ERRP("Could not read Biometric Data Block");
		if (cflag) {
			fprintf(stdout,
			    "CBEFF Header validation:\n");
			if (piv_verify_pcr(&pcr, len, type) == VALIDATE_OK) {
				fprintf(stdout, "\tPasses PIV criteria.\n");
			} else {
				fprintf(stdout,
				    "\tDoes NOT pass PIV criteria.\n");
				exit_code = EXIT_FAILURE;
			}
		}
		fprintf(stdout, "\n");
		/* Skip over the signature block */
		fseek(fp, pcr.sb_length, SEEK_CUR);
	}

eof_out:
err_out:
	exit(exit_code);
}
