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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <biomdimacro.h>
#include <piv.h>
#include <pivdata.h>
#include <fmr.h>	/* From BIOMDI, INCITS-378 Finger Minutiae */
#include <frf.h>	/* From BIOMDI, INCITS-385 Face Recognition */

/*
 */
int
pivValidatePIN(unsigned char pin[PIV_PIN_LENGTH])
{
	int i;

	/* A proper PIV PIN is either a digit, or 0xFF. All characters
	 * after the first 0xFF must also be 0xFF.
	 */
	for (i = 0; i < PIV_PIN_LENGTH; i++) {
		if (pin[i] == 0xff)
			break;
		if (!isdigit(pin[i]))
			return (PIV_PINERR);
	} 

	for (; i < PIV_PIN_LENGTH; i++)
		if (pin[i] != 0xff)
			return (PIV_PINERR);
	return (0);
}

/*
 */
int
pivGetFaceImage(uint8_t *rec, unsigned int reclen, uint8_t *buffer,
    unsigned int *bufsz)
{
	int status;
	int ret;
	FB *fb;
	FDB *fdb;
	BDB bdb;

	fb = NULL;
	ret = new_fb(&fb);
	if (ret != 0) 
		return (PIV_MEMERR);

	INIT_BDB(&bdb, rec, reclen);
	ret = scan_fb(&bdb, fb);
	if (ret != READ_OK) {
		status = PIV_DATAERR;
		goto err_out;
	}

	/* Pull the first (and only) facial data block from the parent */
	fdb = TAILQ_FIRST(&fb->facial_data);
	if (fdb == NULL) {
		status = PIV_DATAERR;
		goto err_out;
	}

	/* Check the input buffer for room */
	if (*bufsz < fdb->image_len)
		return (PIV_BUFSZ);

	memcpy(buffer, fdb->image_data, fdb->image_len);
	*bufsz = fdb->image_len;
	status = 0;
err_out:
	if (fb != NULL)
		free_fb(fb);
	return (status);
}

/*
 * Helper function that either subsets an FMR and returns a new FMR with the
 * single view, or retrieves the minutiae from a single view and returns
 * them in a buffer.
 */
static int
subsetFMR(unsigned int view, uint8_t *rec, unsigned int reclen,
    uint8_t *buffer, unsigned int *bufsz, unsigned int *count, FMR **newfmr)
{
	int status;
	int ret;
	FMR *fmr, *lnewfmr;
	FVMR **fvmrs, *newfvmr;
	FMD **fmds, *newfmd;
	BDB bdb;
	struct piv_fmd *pfmd;
	int vcount, mcount;
	int i;

	if ((view != 1) && (view != 2))
		return (PIV_PARMERR);

	fmr = NULL;
	lnewfmr = NULL;
	fvmrs = NULL;
	fmds = NULL;
	ret = new_fmr(FMR_STD_ANSI, &fmr);
	if (ret != 0) 
		return (PIV_MEMERR);

	INIT_BDB(&bdb, rec, reclen);
	ret = scan_fmr(&bdb, fmr);
	if (ret != READ_OK) {
		status = PIV_DATAERR;
		goto err_out;
	}
	vcount = get_fvmr_count(fmr);
	if (vcount == 0) {
		status = PIV_DATAERR;
		goto err_out;
	}
	fvmrs = (FVMR **)malloc(vcount * sizeof(FVMR *));
	if (fvmrs == NULL) {
		status = PIV_MEMERR;
		goto err_out;
	}
	ret = get_fvmrs(fmr, fvmrs);
	if (ret != vcount) {
		status = PIV_DATAERR;
		goto err_out;
	}
	view = view - 1;

	mcount = get_fmd_count(fvmrs[view]);
	fmds = (FMD **)malloc(mcount * sizeof(FMD **));
	if (fmds == NULL) {
		status = PIV_MEMERR;
		goto err_out;
	}
	ret = get_fmds(fvmrs[view], fmds);
	if (ret != mcount) {
		status = PIV_DATAERR;
		goto err_out;
	}
	if (buffer != NULL) {
		*count = mcount;
		if (*bufsz < (mcount * sizeof(struct piv_fmd))) {
			status = PIV_BUFSZ;
			goto err_out;
		}
		*bufsz = mcount * sizeof(struct piv_fmd);
		pfmd = (struct piv_fmd *)buffer;
		for (i = 0; i < mcount; i++) {
			pfmd->type = fmds[i]->type;
			pfmd->x_coord = fmds[i]->x_coord;
			pfmd->y_coord = fmds[i]->y_coord;
			pfmd->angle = fmds[i]->angle;
			pfmd->quality = fmds[i]->quality;
			pfmd++;
		}
	}

	if (newfmr != NULL) {
		ret = new_fmr(FMR_STD_ANSI, &lnewfmr);
		if (ret != 0) {
			status = PIV_MEMERR;
			goto err_out;
		}
		COPY_FMR(fmr, lnewfmr);
		lnewfmr->record_length = FMR_ANSI_SMALL_HEADER_LENGTH +
		    FEDB_HEADER_LENGTH;
		lnewfmr->num_views = 1;
		ret = new_fvmr(FMR_STD_ANSI, &newfvmr);
		if (ret != 0) {
			status = PIV_MEMERR;
			goto err_out;
		}
		COPY_FVMR(fvmrs[view], newfvmr);
		for (i = 0; i < mcount; i++) {
			ret = new_fmd(FMR_STD_ANSI, &newfmd, i);
			if (ret != 0) {
				status = PIV_MEMERR;
				goto err_out;
			}
			COPY_FMD(fmds[i], newfmd);
			add_fmd_to_fvmr(newfmd, newfvmr);
			lnewfmr->record_length += FMD_DATA_LENGTH;
		}
		add_fvmr_to_fmr(newfvmr, lnewfmr);
		lnewfmr->record_length += FVMR_HEADER_LENGTH;
		*newfmr = lnewfmr;
	}
	status = 0;
err_out:
	if (fmr != NULL)
		free_fmr(fmr);
	if (fvmrs != NULL)
		free(fvmrs);
	if (fmds != NULL)
		free(fmds);
	if (status != 0)
		if (lnewfmr != NULL)
			free_fmr(lnewfmr);
	return (status);
}

/*
 */
int
pivGetFingerMinutiae(unsigned int view, uint8_t *rec, unsigned int reclen,
    uint8_t *buffer, unsigned int *bufsz, unsigned int *count)
{
	return (subsetFMR(view, rec, reclen, buffer, bufsz, count, NULL));
}

/*
 */
int
pivSubsetFingerMinutiaeRec(unsigned int view, uint8_t *rec, unsigned int reclen,
    uint8_t *newrec, unsigned int *newrecsz)
{
	int status, ret;
	FMR *newfmr;
	BDB bdb;

	ret = subsetFMR(view, rec, reclen, NULL, NULL, NULL, &newfmr);
	if (ret != 0)
		return (ret);
	if (*newrecsz < newfmr->record_length) {
		status = PIV_BUFSZ;
		goto err_out;
	}
	INIT_BDB(&bdb, newrec, *newrecsz);
	ret = push_fmr(&bdb, newfmr);
	if (ret != WRITE_OK) {
		status = PIV_DATAERR;
		goto err_out;
	}
	*newrecsz = newfmr->record_length;

	status = 0;
err_out:
	free_fmr(newfmr);
	return (status);
}

/*
 */
int pivGetFingerPosition(unsigned int view, uint8_t *rec, unsigned int reclen,
    int *pos)
{
	int status, ret, vcount;
	FMR *fmr;
	FVMR **fvmrs;
	BDB bdb;

	if ((view != 1) && (view != 2))
		return(PIV_PARMERR);

	fmr = NULL;
	fvmrs = NULL;
	ret = new_fmr(FMR_STD_ANSI, &fmr);
	if(ret != 0)
		return (PIV_MEMERR);

	INIT_BDB(&bdb, rec, reclen);
	ret = scan_fmr(&bdb, fmr);
	if (ret != READ_OK) {
		status = PIV_DATAERR;
		goto err_out;
	}
	vcount = get_fvmr_count(fmr);
	if (vcount == 0) {
		status = PIV_DATAERR;
		goto err_out;
	}
	fvmrs = (FVMR **)malloc(vcount * sizeof(FVMR *));
	if (fvmrs == NULL) {
		status = PIV_MEMERR;
		goto err_out;
	}
	ret = get_fvmrs(fmr, fvmrs);
	if (ret != vcount) {
		status = PIV_DATAERR;
		goto err_out;
	}
	view = view - 1;
	*pos = fvmrs[view]->finger_number;

	status = 0;
err_out:
	if (fmr != NULL)
		free_fmr(fmr);
	if (fvmrs != NULL)
		free(fvmrs);
	return (status);
}
