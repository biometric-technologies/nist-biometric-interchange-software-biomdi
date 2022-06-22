/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/
#include <sys/queue.h>
#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <biomdimacro.h>
#include <fmr.h>
#include <fmrsort.h>
#include <isobit.h>
#include <tlv.h>
#include <moc.h>

#include "genutils.h"

/*
 * Compare two minutiae records by the minutiae index.
 */
static int
compare_by_index(const void *m1, const void *m2)
{
	FMD **lmpp;
	FMD *lm1, *lm2;

	lmpp = (FMD **)m1;
	lm1 = (FMD *)*lmpp;
	lmpp = (FMD **)m2;
	lm2 = (FMD *)*lmpp;

	/* Equality should never happen as index is unique */
	if (lm1->index == lm2->index)
		return (0);
	else
		if (lm1->index < lm2->index)
			return (-1);
		else
			return (1);
}
/*
 * Prune a set of minutiae by quality set, which means we find the set
 * of minutiae with lowest equivalent quality that must be included in
 * a final set of mcount size.
 */
static inline int prune_minutiae_into_quality_set(FMD **fmds, int mcount,
    int max, FMD ***ofmds, uint16_t x, uint16_t y, int usecm)
{
	int remain, excess;
	int lidx, cidx;
	int m, lcount;
	FMD **qfmds, **lfmds;

	/* Minutiae are sorted ascending */
	sort_fmd_by_quality(fmds, mcount);

	/* First, we find the first quality subset that must be included. */
	lidx = 0;
	remain = mcount - max;
	while (remain > 0) {
		cidx = lidx;
		while (fmds[lidx]->quality == fmds[lidx + 1]->quality) {
			lidx++;
			if (lidx >= (mcount - 1)) /* If we've reached the end,*/
				break;      /* the entire set is included. */
		}
		/* At this point, cidx is the start of the quality subset,
		 * and lidx is the end. */
		lidx++;
		lcount = lidx - cidx;	/* size of the subset */
		excess = remain;
		remain = remain - lcount;
	}
	/* Now we polar prune the quality subset if it is still too large,
	 * meaning that it contains more members than the difference between
	 * the max and what we've already dropped.
	 */
	qfmds = (FMD **)malloc(lcount * sizeof(FMD *));
	if (qfmds == NULL)
		ALLOC_ERR_RETURN("FMD array");
	for (m = 0; m < lcount; m++)
		qfmds[m] = fmds[cidx + m];
	sort_fmd_by_polar(qfmds, lcount, x, y, usecm);

	lfmds = (FMD **)malloc(max * sizeof(FMD *));
		if (lfmds == NULL)
			ALLOC_ERR_RETURN("FMD array");

	/* qfmds is the polar-sorted quality subset, in ascending order,
	 * which means the minutiae closest to the center are at the
	 * beginning of the array. excess is the number of minutiae 
	 * that are not needed in the quality subset; they are the
	 * farthest distance from the center.
	 */
	remain = lcount - excess;

	/* Copy the polar selected minutiae from the quality subset */
	for (m = 0; m < remain; m++)
		lfmds[m] = qfmds[m];
	free(qfmds);

	/* Copy the remaining higher quality minutiae from the input set. */
	for (; m < max; m++)
		lfmds[m] = fmds[lidx++];

	/* Now we have to sort the pruned subset by index, which
	 * tracks the minutiae order in the input FMR.
	 */
	qsort(lfmds, max, sizeof(FMD *), compare_by_index);

	*ofmds = lfmds;

	return (0);
}

#define DUP_FMD								\
	do {								\
		if (new_fmd(FMR_STD_ISO_COMPACT_CARD, &ofmd, m) < 0)	\
			ALLOC_ERR_EXIT("Output FMD");			\
		COPY_FMD(ofmds[m], ofmd);				\
		ofmd->index = ofmds[m]->index;				\
		add_fmd_to_fvmr(ofmd, ofvmr);				\
		ofvmr->number_of_minutiae++;				\
	} while (0)							\

int
prune_convert_sort_fmr(FMR *infmr, FMR *outfmr, uint8_t min, uint8_t max,
    uint8_t order, uint16_t x, uint16_t y, int usecm)
{
	int v, vcount, m, mcount;
	unsigned int fmr_len, fvmr_len;
	int rc, retval;
	FVMR *ofvmr, *lfvmr;
	FVMR **ifvmrs = NULL;
	FMD **fmds, **ofmds, *ofmd;
	int lmax;

	COPY_FMR(infmr, outfmr);/* We don't care about fixing up the rest
				 * of the FMR header because the output FMR
				 * isn't going anywhere, just its minutiae. */

	retval = -1;			/* Assume failure, for now */

	/* Get all of the finger view records */
	vcount = get_fvmr_count(infmr);
	if (vcount == 0)
		ERR_OUT("There are no FVMRs in the input FMR");
	if (vcount < 0)
		ERR_OUT("Could not retrieve FVMRs from input FMR");

	ifvmrs = (FVMR **) malloc(vcount * sizeof(FVMR *));
	if (ifvmrs == NULL)
		ALLOC_ERR_OUT("FVMR Array");
	if (get_fvmrs(infmr, ifvmrs) != vcount)
		ERR_OUT("Getting FVMRs from FMR");

	for (v = 0; v < vcount; v++) {

		/* Allocate a temporary FVMR to hold the minutiae that
		 * are converted from ANSI to ISO-CC. We convert all
		 * minutiae, then sort, then prune.
		 */
		if (new_fvmr(FMR_STD_ISO_COMPACT_CARD, &lfvmr) < 0)
                        ALLOC_ERR_RETURN("Temp FVMR");

		/* The ofvmr record will contain the pruned set of
		 * convert minutiae, sorted as requested.
		 */
		if (new_fvmr(FMR_STD_ISO_COMPACT_CARD, &ofvmr) < 0)
                        ALLOC_ERR_RETURN("Output FVMR");

		/* Convert to compact card: The FMR library does the
		 * conversion, but maintains 16-bit coordinate sizes,
		 * so we're fine with just converting then using the 
		 * converted values for comparison.
		 */
		rc = ansi2isocc_fvmr(ifvmrs[v], lfvmr, &fvmr_len,
		    infmr->x_resolution, infmr->y_resolution);
		if (rc != 0)
			ERR_OUT("Modifying FVMR");

		mcount = get_fmd_count(lfvmr);
		if (mcount != 0) {
        
			fmds = (FMD **)malloc(mcount * sizeof(FMD *));
			if (fmds == NULL)
				ALLOC_ERR_RETURN("FMD array");
			if (get_fmds(lfvmr, fmds) != mcount)
				ERR_OUT("getting FMDs from FVMR");

			if (mcount > max) {
				if (prune_minutiae_into_quality_set(
				   fmds, mcount, max, &ofmds, x, y, usecm) != 0)
					ERR_OUT("Could not prune by quality");
				lmax = max;
			} else {
				ofmds = fmds;
				lmax = mcount;
			}
			/* prune/sort the fmds */
			switch (order & MINUTIA_ORDER_METHOD_MASK) {
				case MINUTIA_ORDER_NONE:
					/* If no ordering criteria is given,
					 * protect against sorting the input
					 * inadvertently.
					 */
					order = 0;
					break;
				case MINUTIA_ORDER_XY:
					sort_fmd_by_xy(ofmds, lmax);
					break;
				case MINUTIA_ORDER_YX:
					sort_fmd_by_yx(ofmds, lmax);
					break;
				case MINUTIA_ORDER_POLAR:
					sort_fmd_by_polar(ofmds, lmax, x, y,
					    usecm);
					break;
				default:
					ERR_OUT("Invalid sort order");
					break;		/* not reached */
			}
			if (order & MINUTIA_ORDER_DESCENDING)
				for (m = lmax - 1; m >= 0; m--)
					DUP_FMD;
			else
				for (m = 0; m < lmax; m++)
					DUP_FMD;

			free_fvmr(lfvmr);
			free(fmds);

			/* Free the array created by 
			 * prune_minutiae_into_quality_set() */
			if (mcount > max)
				free(ofmds);
		}
		fmr_len += fvmr_len;
		ofvmr->extended = NULL;
		add_fvmr_to_fmr(ofvmr, outfmr);
		fmr_len += FEDB_HEADER_LENGTH;
	}

	retval = 0;
err_out:
	if (ifvmrs != NULL)
		free (ifvmrs);
	return (retval);
}
