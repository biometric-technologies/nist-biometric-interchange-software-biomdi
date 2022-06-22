/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#define RESPONSEBUFSIZE		1024	/* Should be sufficient for 128 minutiae
					 * and any response from a card. */

/*
 * Get the BIT group from a card.
 * Parameters:
 *	hCard        Handle to the opened card.
 *      bitgroup_tlv TLV representing the BIT group on return.
 * Returns:
 *	 READ_OK    Success
 *	 READ_ERROR Failure
 */
int get_bitgroup_from_card(SCARDHANDLE hCard, TLV *bitgroup_tlv);

/*
 * Get the identifier present in the response from a card, represented as
 * a string. This is useful for the MOC matcher and card IDs.
 * Parameters:
 *	id       Pointer the the ID char array, filled on return.
 *	response Pointer to the data block containing the card's response to
 *               a get XXX ID APDU.
 * Returns:
 *	 0 Success
 *	-1 Failure, usually due to the response TLV being invalid.
 */
int getIDinresponse(char *id, BDB *response);
