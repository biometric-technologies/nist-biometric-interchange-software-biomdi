/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#include <nistapdu.h>
#include <PCSC/winscard.h>

#ifndef _PIVAPDU_H
#define _PIVAPDU_H
#include <stdint.h>

APDU PIVSETCONTACTMODE = {
	.apdu_cla = 0xC0,
	.apdu_ins = 0x17,
	.apdu_descr = "Set Contact Mode"
};

APDU PIVSELECTAPP = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xA4,
	.apdu_p1		= 0x04,
	.apdu_p2		= 0x00,
	.apdu_lc		= 0x0B,
	.apdu_nc		= { 0xA0, 0x00, 0x00, 0x03, 0x08, 0x00, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00 },
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_descr = "Select PIV Application"
};

/*
 * The PIN is in the NC field and must to be set using add_data_to_apdu().
 * The PIN must be padded with 0xFF values, and therefore is 8 characters
 * long, and the LC total length is always 8.
 */
APDU PIVVERIFYPIN = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0x20,
	.apdu_p1		= 0x00,
	.apdu_p2		= 0x80,
	.apdu_lc		= 0x08,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_descr = "Verify PIN"
};

APDU PIVGETCARDAUTHCERT = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_nc = { 0x5C, 0x03, 0x5F, 0xC1, 0x01 },
	.apdu_descr = "Get PIV Auth Cert"
};

APDU PIVGETCHUID = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_nc = { 0x5C, 0x03, 0x5F, 0xC1, 0x02 },
	.apdu_descr = "Get CHUID"
};

APDU PIVGETFINGERPRINTS = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_nc = { 0x5C, 0x03, 0x5F, 0xC1, 0x03 },
	.apdu_descr = "Get Fingerprints"
};

APDU PIVGETPIVAUTHCERT = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_nc = { 0x5C, 0x03, 0x5F, 0xC1, 0x05 },
	.apdu_descr = "Get PIV Auth Cert"
};

APDU PIVGETSECURITYOBJECT = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_nc = { 0x5C, 0x03, 0x5F, 0xC1, 0x06 },
	.apdu_descr = "Get Security Object"
};

APDU PIVGETCCC = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_nc = { 0x5C, 0x03, 0x5F, 0xC1, 0x07 },
	.apdu_descr = "Get Card Capability Container"
};

APDU PIVGETFACE = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_nc = { 0x5C, 0x03, 0x5F, 0xC1, 0x08 },
	.apdu_descr = "Get Face"
};

APDU PIVGETPRINTEDINFO = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_nc = { 0x5C, 0x03, 0x5F, 0xC1, 0x09 },
	.apdu_descr = "Get Printed Info"
};

APDU PIVGETDIGITALSIGCERT = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_nc = { 0x5C, 0x03, 0x5F, 0xC1, 0x0A },
	.apdu_descr = "Get Digital Signature Cert"
};

APDU PIVGETKEYMGMTCERT = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_nc = { 0x5C, 0x03, 0x5F, 0xC1, 0x0B },
	.apdu_descr = "Get Key Management Cert"
};

/*
 * The apudu_nc must be set using add_data_to_apdu(); for this APDU, the
 * NC field is set to the new PIN, followed by the current PIN.
 * Each PIN must be padded with 0xFF values, and therefore each is
 * 8 characters long, and the LC total length is always 16 (0x10).
 */
APDU PIVRESETPINTRETRYCTR = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0x2C,
	.apdu_p1		= 0x00,
	.apdu_p2		= 0x80,
	.apdu_lc		= 0x10,
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_descr = "Reset PIN Retry Counter"
};

#endif	/* _PIVAPDU_H */
