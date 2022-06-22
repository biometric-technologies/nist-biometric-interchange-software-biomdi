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
/* This program will combine several Finger Minutiae Records or a set of
/* Finger Image Records into a PIV compliant record, stored in a file.        */
/*                                                                            */
/* XXX It would be desirable to create the signature block.                   */
/******************************************************************************/

/* Needed by the GNU C libraries for Posix and other extensions */
#define _XOPEN_SOURCE	1

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <biomdimacro.h>
#include <fir.h>
#include <fmr.h>
#include <piv.h>

struct fmrelem {
	struct finger_minutiae_record		*fmr;
	TAILQ_ENTRY(fmrelem)			list;
};

static void
usage(char *name)
{
	printf("usage: %s -h <hdrfile> -f <listfile> -o <outfile>"
	    " [-s <sigfile>] -t [FMR | FIR]\n", name);
	exit (EXIT_FAILURE);
}

/*
 * Convert a string starting with '0x' or '0X' to an array of unsigned ints.
 */
static int
atox(char *s, uint8_t b[], int len)
{
	int i;
	char buf[5];

	if (s[0] != '0' || (toupper(s[1]) != 'X'))
		return (-1);
	for (i = 0; i < len; i++) {
		sprintf(buf, "0x%c%c", s[(i + 1)*2], s[((i + 1)*2) + 1]);
		sscanf(buf, "%hhx", &b[i]);
	}
	
}

/*
 * Read the PIV CBEFF header.
 */
static int
readin_pcr(FILE *infp, struct piv_cbeff_record *pcr)
{
	int count, rval, i;
	char buf[255];

	bzero((void *)pcr, sizeof(struct piv_cbeff_record));
	rval = fscanf(infp, "%hhx %hhx %u %hu %hx %hx",
	    &pcr->patron_header_version,
	    &pcr->sbh_security_options,
	    &pcr->bdb_length,
	    &pcr->sb_length,
	    &pcr->bdb_format_owner,
 	    &pcr->bdb_format_type);
	count = rval;
	if (count != 6)
		READ_ERR_RETURN("PCR header (read %d of 14 items)", count);
	   
	rval = fscanf(infp, "%s", buf);
	if (rval != 1)
		READ_ERR_RETURN("PCR header (read %d of 14 items)", count);
	if (atox(buf, pcr->biometric_creation_date,
	    BIOMETRIC_CREATION_DATE_LEN) < 0)
		READ_ERR_RETURN("PCR header (read %d of 14 items)", count);
	count += rval;

	rval = fscanf(infp, "%s", buf);
	if (rval != 1)
		READ_ERR_RETURN("PCR header (read %d of 14 items)", count);
	if (atox(buf, pcr->validity_period, VALIDITY_PERIOD_LEN) < 0)
		READ_ERR_RETURN("PCR header (read %d of 14 items)", count);
	count += rval;

	rval = fscanf(infp, "%x %hhx %hhd %17s",
	    &pcr->biometric_type,
	    &pcr->biometric_data_type,
	    &pcr->biometric_data_quality,
	    pcr->creator);
	if (rval != 4)
		READ_ERR_RETURN("PCR header (read %d of 14 items)", count);
	count += rval;

	rval = fscanf(infp, "%s", buf);
	if (rval != 1)
		READ_ERR_RETURN("PCR header (read %d of 14 items)", count);
	if (atox(buf, pcr->fascn, FASCN_LEN) < 0)
		READ_ERR_RETURN("PCR header (read %d of 14 items)", count);
	count += rval;

	rval = fscanf(infp, "%x", pcr->reserved);
	if (rval != 1)
		READ_ERR_RETURN("PCR header (read %d of 14 items)", count);

	return (READ_OK);
}

static int
readin_fmrs(FILE *infp, FILE *outfp, struct finger_minutiae_record **fmrp)
{
	FILE *fmrfp = NULL;
	struct fmrelem *fmrelem;
	TAILQ_HEAD(, fmrelem) fmrlist;
	char fmr_filename[MAXPATHLEN + 1];
	struct finger_minutiae_record *fmr;
	struct finger_view_minutiae_record *fvmr;
	int ret;

	TAILQ_INIT(&fmrlist);
	// Read in all the finger minutiae records from their respective
	// files, and place them in a list.
	for (;;) {
		ret = fscanf(infp, "%s", fmr_filename);
		if (ret == EOF)
			break;
		if (ret != 1)
			READ_ERR_OUT(fmr_filename);
		if (new_fmr(FMR_STD_ANSI, &fmr) < 0)
			ALLOC_ERR_OUT("finger minutiae record");

		fmrfp = fopen(fmr_filename, "rb");
		if (fmrfp == NULL)
			ERR_OUT("Could not open %s", fmr_filename);
		if (read_fmr(fmrfp, fmr) != READ_OK)
			READ_ERR_OUT(fmr_filename);
		fclose(fmrfp);

		fmrelem = (struct fmrelem *)malloc(sizeof(struct fmrelem));
		if (fmrelem == NULL)
			ALLOC_ERR_OUT("finger minutiae record list");
		fmrelem->fmr = fmr;
		TAILQ_INSERT_TAIL(&fmrlist, fmrelem, list);
	}
	// Use the first FMR as the master, and add all the finger view
	// minutiae records to it.
	fmrelem = TAILQ_FIRST(&fmrlist);
	TAILQ_REMOVE(&fmrlist, fmrelem, list);
	fmr = fmrelem->fmr;
	free(fmrelem);
	while (!TAILQ_EMPTY(&fmrlist)) {
		fmrelem = TAILQ_FIRST(&fmrlist);
		TAILQ_REMOVE(&fmrlist, fmrelem, list);
		// Fix up the fmr record length
		fmr->record_length += fmrelem->fmr->record_length -
			fmrelem->fmr->record_length_type;

		// The current 378 spec says that the image size in the
		// header is to be the largest of the views. For this
		// program, we want to find the largest given in the headers.
		if (fmrelem->fmr->x_image_size > fmr->x_image_size)
			fmr->x_image_size = fmrelem->fmr->x_image_size;
		if (fmrelem->fmr->y_image_size > fmr->y_image_size)
			fmr->y_image_size = fmrelem->fmr->y_image_size;

		while (!TAILQ_EMPTY(&fmrelem->fmr->finger_views)) {
			fvmr = TAILQ_FIRST(&fmrelem->fmr->finger_views);
			TAILQ_REMOVE(&fmrelem->fmr->finger_views, fvmr, list);
			add_fvmr_to_fmr(fvmr, fmr);
			fmr->num_views++;
		}
		free_fmr(fmrelem->fmr);
	}
	*fmrp = fmr;
	return (READ_OK);

err_out:
	free_fmr(fmr);
	if (fmrfp != NULL)
		fclose(fmrfp);
	return (READ_ERROR);
}

static int
readin_firs(FILE *infp, FILE *outfp, struct finger_image_record **firp)
{
	FILE *firfp = NULL;
	char fir_filename[MAXPATHLEN + 1];
	struct finger_image_record *headfir = NULL;
	struct finger_image_record *fir = NULL;
	struct finger_image_view_record **fivrs;
	struct finger_image_view_record *fivr;
	int ret;
	int rcount;
	int i;

	// Read in the first finger image record 
	ret = fscanf(infp, "%s", fir_filename);
	if (ret != 1)
		READ_ERR_OUT("FIR list file");
	if (new_fir(FMR_STD_ANSI, &headfir) < 0)
		ALLOC_ERR_OUT("finger image record");
	firfp = fopen(fir_filename, "rb");
	if (firfp == NULL)
		ERR_OUT("Could not open %s", fir_filename);
	if (read_fir(firfp, headfir) != READ_OK)
		READ_ERR_OUT(fir_filename);
	fclose(firfp);

	// Read in the remaining FIRs and add their view records to the 
	// first FIR
	for (;;) {
		ret = fscanf(infp, "%s", fir_filename);
		if (ret == EOF)
			break;
		if (ret != 1)
			READ_ERR_OUT(fir_filename);
		if (new_fir(FMR_STD_ANSI, &fir) < 0)
			ALLOC_ERR_OUT("finger image record");

		firfp = fopen(fir_filename, "rb");
		if (firfp == NULL)
			ERR_OUT("Could not open %s", fir_filename);
		if (read_fir(firfp, fir) != READ_OK)
			READ_ERR_OUT(fir_filename);
		fclose(firfp);

		// Pull off the FIVRs
		rcount = get_fivr_count(fir);
		fivrs = (struct finger_image_view_record **) malloc(
		    rcount * sizeof(struct finger_image_view_record *));
		if (fivrs == NULL)
			ALLOC_ERR_EXIT("FIVR Array");
		if (get_fivrs(fir, fivrs) != rcount)
			ERR_OUT("getting FIVRs from FIR");

		for (i = 0; i < rcount; i++) {
			if (new_fivr(&fivr) < 0)
				ALLOC_ERR_OUT("finger image view record copy");
			copy_fivr(fivrs[i], fivr);
			add_fivr_to_fir(fivr, headfir);
		}
		free_fir(fir);
		fir = NULL;
	}

	*firp = headfir;
	return (READ_OK);

err_out:
	free_fir(headfir);
	if (fir != NULL)
		free_fir(fir);
	if (firfp != NULL)
		fclose(firfp);
	return (READ_ERROR);
}

int
main(int argc, char *argv[])
{
	struct piv_cbeff_record pcr;
	struct finger_minutiae_record *fmr = NULL;
	struct finger_image_record *fir = NULL;
	int ch;
	int exit_code = EXIT_FAILURE;
	int hflag, fflag, oflag, sflag, tflag;
	FILE *h_fp, *f_fp, *o_fp, *s_fp;
	struct stat sb;
	char f_opt_filename[MAXPATHLEN + 1];
	int type;
	int s_size;
	char *s_buf;

	hflag = oflag = fflag = tflag = sflag = 0;
	s_buf = NULL;
	while ((ch = getopt(argc, argv, "h:f:o:s:t:")) != -1) {
		switch (ch) {
			case 'h' :
				if ((h_fp = fopen(optarg, "r")) == NULL)
					OPEN_ERR_EXIT(optarg);
				hflag = 1;
				break;
			case 'f' :
				if ((f_fp = fopen(optarg, "r")) == NULL)
					OPEN_ERR_EXIT(optarg);
				strncpy(f_opt_filename, optarg, MAXPATHLEN);
				fflag = 1;
				break;
			case 'o' :
				if (stat(optarg, &sb) == 0)
					ERR_EXIT("File '%s' exists, remove it first", optarg);
				if ((o_fp = fopen(optarg, "wb")) == NULL)
					OPEN_ERR_EXIT(optarg);
				oflag = 1;
				break;
			case 's' :
				if (stat(optarg, &sb) != 0)
					ERR_EXIT("Could not find %s", optarg);
				if ((s_fp = fopen(optarg, "r")) == NULL)
					OPEN_ERR_EXIT(optarg);
				s_size = sb.st_size;
				s_buf = (char *)malloc(s_size);
				if (s_buf == NULL)
					ALLOC_ERR_EXIT("Signature buffer");
				if (fread(s_buf, 1, s_size, s_fp) != s_size)
					ERR_EXIT("Could not read %s", optarg);
				fclose(s_fp);
				sflag = 1;
				break;
			case 't' :
				if (strncasecmp(optarg, FMR_FORMAT_ID,
				    strlen(FMR_FORMAT_ID)) == 0)
					type = PIVFMR;
				else if (strncasecmp(optarg, FIR_FORMAT_ID,
				    strlen(FIR_FORMAT_ID)) == 0)
					type = PIVFIR;
				else
					usage(argv[0]);
				tflag = 1;
				break;
			default :
				usage(argv[0]);
				break;	// not reached
		}
	}

	if (!hflag || !fflag || !oflag || !tflag)
		usage(argv[0]);

	// Create the PIV CBEFF header
	if (readin_pcr(h_fp, &pcr) != READ_OK)
		READ_ERR_OUT("PIV CBEFF header");

	switch (type) {
		case PIVFMR:
			if (readin_fmrs(f_fp, o_fp, &fmr) != READ_OK)
				READ_ERR_OUT("FMRs");
			pcr.bdb_length = fmr->record_length;
			break;
		case PIVFIR:
			if (readin_firs(f_fp, o_fp, &fir) != READ_OK)
				READ_ERR_OUT("FIRs");
			pcr.bdb_length = fir->record_length;
			break;
	}
		
	if (piv_write_pcr(o_fp, &pcr) != WRITE_OK)
		WRITE_ERR_OUT("CBEFF Header into PIV file");

	switch (type) {
		case PIVFMR:
			if (write_fmr(o_fp, fmr) != WRITE_OK)
				WRITE_ERR_OUT("FMR into PIV file");
			break;
		case PIVFIR:
			if (write_fir(o_fp, fir) != WRITE_OK)
				WRITE_ERR_OUT("FIR into PIV file");
			break;
	}

	/* Write out the signature, if specified */
	if (sflag)
		if (fwrite(s_buf, 1, s_size, o_fp) != s_size)
			WRITE_ERR_OUT("Signature block");

	exit_code = EXIT_SUCCESS;

err_out:
	if (fmr != NULL)
		free_fmr(fmr);
	if (fir != NULL)
		free_fir(fir);
	if (s_buf != NULL)
		free(s_buf);
	fclose(h_fp);
	fclose(f_fp);
	fclose(o_fp);
	
	exit(exit_code);
}
