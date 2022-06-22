/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#include <stdint.h>

#include <nistapdu.h>

/* Select the MOC application.
 */
APDU MOCSELECTAPP = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xA4,
	.apdu_p1		= 0x04,
	.apdu_p2		= 0x0C,
	.apdu_lc		= 0x10,
	.apdu_nc		= { 0xF0, 0x4E, 0x49, 0x53, 0x54, 0x20, 0x4D, 0x4F, 0x43, 0x20, 0x54, 0x53, 0x54, 0x20, 0x50, 0x31 },
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_descr		= "Select MOC Application"
};
APDU ALTMOCSELECTAPP = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xA4,
	.apdu_p1		= 0x04,
	.apdu_p2		= 0x00,
	.apdu_lc		= 0x10,
	.apdu_nc		= { 0xF0, 0x4E, 0x49, 0x53, 0x54, 0x20, 0x4D, 0x4F, 0x43, 0x20, 0x54, 0x53, 0x54, 0x20, 0x50, 0x31 },
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_descr		= "Alternative Select MOC Application"
};
/* MOC: Store an enrollment template on the card. The Lc and Nc fields
 * must be set in the application.
 */
APDU MOCSTORETEMPLATE = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xDB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x00,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_descr		= "Store Enrollment Template"
};
/* MOC: Retrieve the BIT group.
 */
APDU MOCREADBIT = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x04,
	.apdu_nc		= { 0x5C, 0x02, 0x7F, 0x61 },
	.apdu_le		= 0x00,
	.apdu_field_mask	= APDU_FIELD_LC | APDU_FIELD_LE,
	.apdu_descr		= "Read BIT"
};
/* MOC: Verify templates. The Lc and Nc fields must be set in the
 * application.
 */
APDU MOCVERIFY = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0x21,
	.apdu_p1		= 0x00,
	.apdu_p2		= 0x00,
	.apdu_lc		= 0x00,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_descr		= "MOC Verify"
};
/* MOC: Retrieve the similarity score.
 */
APDU MOCGETSCORE = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x03,
	.apdu_nc		= { 0x5C, 0x01, 0xC0 },
	.apdu_le		= 0x04,
	.apdu_field_mask	= APDU_FIELD_LC | APDU_FIELD_LE,
	.apdu_descr		= "MOC Get Score"
};
/* MOC: Retrieve the card ID.
 */
APDU MOCGETCARDID = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x03,
	.apdu_nc		= { 0x5C, 0x01, 0x66 },
	.apdu_le		= 0x00,
	.apdu_field_mask	= APDU_FIELD_LC | APDU_FIELD_LE,
	.apdu_descr		= "MOC Get Card ID"
};
/* MOC: Retrieve the matcher ID.
 */
APDU MOCGETMATCHERID = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x03,
	.apdu_nc		= { 0x5C, 0x01, 0x6E },
	.apdu_le		= 0x00,
	.apdu_field_mask	= APDU_FIELD_LC | APDU_FIELD_LE,
	.apdu_descr		= "MOC Get Matcher ID"
};
/* Get a response from the card; should work on all cards */
APDU GETRESPONSE = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xC0,
	.apdu_p1		= 0x00,
	.apdu_p2		= 0x00,
	.apdu_lc		= 0x00,
	.apdu_field_mask	= 0x00,
	.apdu_descr		= "Get Response"
};
