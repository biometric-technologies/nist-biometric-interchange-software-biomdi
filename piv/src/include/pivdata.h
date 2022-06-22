/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#ifndef _PIVDATA_H
#define _PIVDATA_H

#include <piv.h>

/*
 * The representation of a finger minutiae data point for PIV purposes.
 * The minutiae are in M1 format, where the angle is 1/2 the true theta
 * value.
 */
struct piv_fmd {
	unsigned short		type;
	unsigned short		x_coord;
	unsigned short		y_coord;
	unsigned short		angle;
	unsigned short		quality;
};

/*
 * pivValidatePIN() checks the given character array for validity
 * with respect to the PIV PIN format.
 *
 * Parameters:
 *   pin    - (in) Character array containing the PIN.
 * Returns:
 *   0          - PIN is in the valid format.
 *   PIV_PINERR - PIN is not in valid format.
 */
int
pivValidatePIN(unsigned char pin[PIV_PIN_LENGTH]);
 
/*
 * pivGetFaceImage() returns the facial image data from a buffer containing
 * and INCITS-385 face recognition record. Presumably this record was read
 * from a PIV card (using pivCardGetFaceImageRec()), so only the first image
 * in the record is returned (and there should only be one image).
 * 
 * Parameters:
 *   rec    - (in) Buffer containing the INCITS-385 data record.
 *   reclen - (in) Length of the rec buffer.
 *   buffer - (out) Output buffer that will contain the image.
 *            This buffer must be allocated by the caller. It is safe to
 *            allocate a buffer of size PIV_MAX_OBJECT_SIZE.
 *   bufsz  - (in/out) The size of the allocated buffer on input, the size
 *            of the face data on output.
 *
 * Returns: 
 *   0           - Success.
 *   PIV_BUFSZ   - Insufficient memory in the caller's buffer.
 *   PIV_DATAERR - Data from the record could not be processed.
 */
int
pivGetFaceImage(uint8_t *rec, unsigned int reclen, uint8_t *buffer,
    unsigned int *bufsz);

/*
 * pivGetFingerMinutiae() returns the minutiae data from a buffer containing
 * an INCITS-378 finger minutiae record. Presumably this record was read
 * from a PIV card (using pivCardGetFingerMinutiaeRec()), so only the minutiae
 * from the first or second finger view are returned.
 * 
 * Applications can use this function to retrieve minutiae from a 378 record
 * in a M1 format, so the angle value is 1/2 the real theta.
 * Example:
 *     rec = malloc(PIV_MAX_OBJECT_SIZE);
 *     reclen = PIV_MAX_OBJECT_SIZE;
 *     ret = pivCardGetFingerMinutiaeRec(card, rec, &reclen);
 *     databuf = malloc(PIV_MAX_OBJECT_SIZE);
 *     databuf_len = PIV_MAX_OBJECT_SIZE;
 *     ret = pivGetFingerMinutiae(1, rec, PIV_MAX_OBJECT_SIZE, databuf,
 *               &databuf_len, &count);
 *     pfmd = (struct piv_fmd *)databuf;
 *     for (i = 0; i < count; i++) {
 *         // do something with pfmd->x_coord, pfmd->y_coord, etc.
 *         pfmd++;
 *     }
 * 
 * Parameters:
 *   view   - (in) The finger view number, 1 or 2.
 *   rec    - (in) Buffer containing the INCITS-378 data record.
 *   reclen - (in) Length of the rec buffer.
 *   buffer - (out) Output buffer that will contain the minutiae.
 *            This buffer must be allocated by the caller. It is safe to
 *            allocate a buffer of size PIV_MAX_OBJECT_SIZE.
 *   bufsz  - (in/out) The size of the allocated buffer on input, the size
 *            of the minutiae data on output.
 *   count    (out) The number of minutiae.
 *
 * Returns: 
 *   0           - Success.
 *   PIV_BUFSZ   - Insufficient memory in the caller's buffer.
 *   PIV_PARMERR - Incorrect parameter, most likely incorrect view.
 *   PIV_DATAERR - Data from the record could not be processed.
 */
int
pivGetFingerMinutiae(unsigned int view, uint8_t *rec, unsigned int reclen,
    uint8_t *buffer, unsigned int *bufsz, unsigned int *count);

/*
 * pivSubsetFingerMinutiaeRec() returns a new FMR record containing the single
 * finger view specifed by the view parameter. Both the original and new
 * finger minutiae records are INCITS-378 format, and the input record was
 * presumably read from a PIV card using pivCardGetFingerMinutiaeRec().
 * This function is useful creating a 378 record from the PIV card's record
 * and passing that new record to a matcher that expects a single finger view.
 * 
 * Parameters:
 *   view      - (in) The finger view number, 1 or 2.
 *   rec       - (in) Buffer containing the INCITS-378 data record.
 *   reclen    - (in) Length of the rec buffer.
 *   newrec    - (out) Output buffer that will contain the new record.
 *               This buffer must be allocated by the caller. It is safe to
 *               allocate a buffer of size PIV_MAX_OBJECT_SIZE.
 *   newrecsz  - (in/out) The size of the new record buffer on input, the size
 *            of the minutiae record data on output.
 *
 * Returns: 
 *   0           - Success.
 *   PIV_BUFSZ   - Insufficient memory in the caller's buffer.
 *   PIV_PARMERR - Incorrect parameter, most likely incorrect view.
 *   PIV_DATAERR - Data from the record could not be processed.
 */
int
pivSubsetFingerMinutiaeRec(unsigned int view, uint8_t *rec, unsigned int reclen,
    uint8_t *newrec, unsigned int *newrecsz);

/*
 * pivGetFingerPosition() returns the finger number of a finger view.
 *
 * Parameters:
 *   view      - (in) The finger view number, 1 or 2.
 *   rec       - (in) Buffer containing the INCITS-378 data record.
 *   reclen    - (in) Length of the rec buffer.
 *   pos       - (out) The finger number of the view.
 *
 * Returns: 
 *   0           - Success.
 *   PIV_MEMERR  - Internal memory allocation error.
 *   PIV_PARMERR - Incorrect parameter, most likely incorrect view.
 *   PIV_DATAERR - Data from the record could not be processed.
 */
int
pivGetFingerPosition(unsigned int view, uint8_t *rec, unsigned int reclen,
    int *pos);

#endif	/* _PIVDATA_H */
