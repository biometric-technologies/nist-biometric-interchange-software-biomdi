/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#ifndef _PIV_H
#define _PIV_H
#include <biomdimacro.h>
#include <stdint.h>

#define PIV_MAX_OBJECT_SIZE		16386	
					/* Facial image is largest object */
#define PIV_NOCARD		1
#define PIV_CARDERR		2	/* Error accessing the card */
#define PIV_BUFSZ		3	/* user-supplied buffer too small */
#define PIV_MEMERR		4	/* error allocating memory */
#define PIV_PININVALID		5	/* PIN is invalid */
#define PIV_PINERR		6	/* PIN improperly formatted */
#define PIV_DATAERR		7	/* Data item from card is invalid */
#define PIV_PARMERR		8	/* Parameter (card object ID, etc.) */

#define PIV_ENCODED_DATA_LEN		8
#define PIV_SECURITY_ENCRYPTED		0xf
#define PIV_SECURITY_NON_ENCRYPTED	0xd
#define PIV_FORMAT_TYPE_FINGER_MINUTIAE	0x0201
#define PIV_FORMAT_TYPE_FINGER_IMAGE	0x0401
#define PIV_FORMAT_TYPE_FACE_IMAGE	0x0501
#define PIV_BIO_TYPE_FACE		0x000002
#define PIV_BIO_TYPE_FINGER		0x000008
#define PIV_BIO_DATA_TYPE_RAW		0x20	/* must be used with mask */
#define PIV_BIO_DATA_TYPE_INTERMEDIATE	0x40	/* must be used with mask */
#define PIV_BIO_DATA_TYPE_PROCESSED	0x80	/* must be used with mask */
#define PIV_BIO_DATA_TYPE_MASK		0xE0
#define PIV_BDB_FORMAT_OWNER		0x001B
#define PIV_PIN_LENGTH			8

#define BIOMETRIC_CREATION_DATE_LEN	PIV_ENCODED_DATA_LEN
#define VALIDITY_PERIOD_LEN		(PIV_ENCODED_DATA_LEN * 2)
#define BIOMETRIC_TYPE_LEN		3
#define CREATOR_LEN			18
#define FASCN_LEN			25
#define RESERVED_LEN			4
#define CBEFF_HDR_LEN			88
/* Uses C99 types */
struct piv_cbeff_record {
	uint8_t		patron_header_version;
	uint8_t		sbh_security_options;
	uint32_t	bdb_length;
	uint16_t	sb_length;
	uint16_t	bdb_format_owner;
	uint16_t	bdb_format_type;
	uint8_t		biometric_creation_date[BIOMETRIC_CREATION_DATE_LEN];
	uint8_t		validity_period[VALIDITY_PERIOD_LEN];
	uint32_t	biometric_type;
	uint8_t		biometric_data_type;
	int8_t		biometric_data_quality;
	char		creator[CREATOR_LEN];
	uint8_t		fascn[FASCN_LEN];
	uint8_t		reserved[RESERVED_LEN];
};

#define PIVFMR	1
#define PIVFIR	2
#define PIVFRF	3

/*
 * Read/scan the PIV CBEFF record from a file/buffer.
 */
int
piv_read_pcr(FILE *fp, struct piv_cbeff_record *pcr);
int
piv_scan_pcr(BDB *pcrdb, struct piv_cbeff_record *pcr);

/*
 * Write/push the PIV CBEFF record to a file/buffer.
 */
int
piv_write_pcr(FILE *fp, struct piv_cbeff_record *pcr);
int
piv_push_pcr(BDB *pcrdb, struct piv_cbeff_record *pcr);

/*
 * Print the PIV CBEFF record.
 */
int
piv_print_pcr(FILE *fp, struct piv_cbeff_record *pcr);

/*
 * Verify the PIV CBEFF Record according to 800-76 requirements.
 */
int
piv_verify_pcr(struct piv_cbeff_record *pcr, int bdb_len, int type);

#endif	/* _PIV_H */
