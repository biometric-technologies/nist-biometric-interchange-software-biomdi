/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

/*
 * Prune a set of minutiae down, then convert from ANSI 378 to ISO
 * compact card, then sort based on the requested order.
 * Parameters:
 *	infmr  Pointer to the finger minutiae record to be converted,
 *	       in ANSI 378 format.
 *	outfmr Pointer to the output FMR
 *	min    Minimum number of minutiae to include in the output.
 *	max    Maximum number of minutiae to include in the output.
 *	order  The minutiae sorting order, one of: X-Y, Y-X, POLAR, NONE.
 *      x, y   The coenter-of-interest for pruning. If the usecm parameter
 *             is FALSE, this coordinate is used for the center when doing
 *             the polar prune.
 *      usecm  If TRUE, use the center of mass to prune.
 * Returns:
 *	 0     Success
 *	-1     Failure
 */
int prune_convert_sort_fmr(FMR *infmr, FMR *outfmr, uint8_t min, uint8_t max,
    uint8_t order, uint16_t x, uint16_t y, int usecm);

/*
 * Macros to create the test output file names, test results and BIT.
 * Two versions: One that takes the IDs as integers, and one that takes
 * them as strings.
 */

#define GENTESTFN(__fn, __softID, __matcherID)				\
do {									\
	sprintf(__fn, "%08X_%08X.results", __softID, __matcherID);	\
} while (0)

#define GENBITFN(__fn, __softID, __matcherID)				\
do {									\
/*	sprintf(__fn, "%08X_%08X.bit", __softID, __matcherID); */	\
	sprintf(__fn, "%08X.bit", __matcherID);				\
} while (0)

#define STRGENTESTFN(__fn, __softID, __matcherID)			\
do {									\
	sprintf(__fn, "%s_%s.results", __softID, __matcherID);	\
} while (0)

#define STRGENBITFN(__fn, __softID, __matcherID)			\
do {									\
	sprintf(__fn, "%s.bit", __matcherID);				\
} while (0)


/*
 *  Set __cx and __cy to the x,y "center" coordinates parsed from a template
 *  filename IF PRESENT.  Otherwise, set them to 0,0.  If x,y = 0,0 or if
 *  __cx  >= __fmr->x_image_size then set __usecm equal to TRUE, otherwise
 *  set __usecm equal to FALSE.
 *
 *  Format of filenames containing "x,y center" coordinates:
 *
 *     {SDK_id_code}-{original_image_filename}-X_Y.MIN_2
 *
 *   where
 *        {}=any alphanumeric string not containing "-" characters
 *        X = positive integer in decimal format
 *        Y = positive integer in decimal format
 *
 */
#define CHOOSEPRUNECENTER(__fmrfn, __fmr, __cx, __cy, __usecm)		\
do {									\
        int i,x,y,dashes=0;                                             \
	__cx = 0;							\
	__cy = 0;							\
	__usecm = TRUE;							\
        for (i=0;i<strlen(__fmrfn);i++) {                               \
          if (__fmrfn[i] == '-') {                                      \
	    dashes++;                                                   \
	    if (dashes > 1) break;                                      \
	  }                                                             \
        }                                                               \
        if (dashes == 2) {                                              \
           if (sscanf(&__fmrfn[i],"-%d_%d.",&x,&y) == 2) {              \
             __cx=x;                                                    \
             __cy=y;                                                    \
           }                                                            \
           if (__cx < __fmr->x_image_size)                              \
             __usecm=FALSE;                                             \
        }                                                               \
} while (0)

/*
/*
 * Calculate a time interval in microseconds, based on the start and
 * finish times represented as struct timeval.
 */
#define TIMEINTERVAL(__s, __f)						\
	(__f.tv_sec - __s.tv_sec)*1000000+(__f.tv_usec - __s.tv_usec)





