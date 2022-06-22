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
/* This library will manipulate the non-biometric components of the PIV       */
/* record according to the constraints of the NIST SP 800-76 PIV              */
/* specification.                                                             */
/*                                                                            */
/******************************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <biomdimacro.h>
#include <piv.h>

static void
print_encoded_date(FILE *fp, uint8_t date[])
{
	fprintf(fp, "%02d%02d/%02d/%02d %02d:%02d:%02d(%c)",
	    date[0],
	    date[1],
	    date[2],
	    date[3],
	    date[4],
	    date[5],
	    date[6],
	    date[7]);
}

/*
 * 
 */
static int
internal_piv_read_pcr(FILE *fp, BDB *bdb, struct piv_cbeff_record *pcr)
{
	uint32_t val;

	CGET(&pcr->patron_header_version, fp, bdb);
	CGET(&pcr->sbh_security_options, fp, bdb);
	LGET(&pcr->bdb_length, fp, bdb);
	SGET(&pcr->sb_length, fp, bdb);
	SGET(&pcr->bdb_format_owner, fp, bdb);
	SGET(&pcr->bdb_format_type, fp, bdb);
	OGET(pcr->biometric_creation_date, 1, BIOMETRIC_CREATION_DATE_LEN,
	    fp, bdb);
	OGET(pcr->validity_period, 1, VALIDITY_PERIOD_LEN, fp, bdb);
	OGET(&val, 1, BIOMETRIC_TYPE_LEN, fp, bdb);
	pcr->biometric_type = ntohl(val) >> 8;
	CGET(&pcr->biometric_data_type, fp, bdb);
	CGET(&pcr->biometric_data_quality, fp, bdb);
	OGET(pcr->creator, 1, CREATOR_LEN, fp, bdb);
	OGET(pcr->fascn, 1, FASCN_LEN, fp, bdb);
	OGET(pcr->reserved, 1, RESERVED_LEN, fp, bdb);
	return (0);

eof_out:
	return (READ_EOF);
err_out:
	return (READ_ERROR);
}

int
piv_read_pcr(FILE *fp, struct piv_cbeff_record *pcr)
{
	return (internal_piv_read_pcr(fp, NULL, pcr));
}
int
piv_scan_pcr(BDB *pcrdb, struct piv_cbeff_record *pcr)
{
	return (internal_piv_read_pcr(NULL, pcrdb, pcr));
}

static int
internal_piv_write_pcr(FILE *fp, BDB *bdb, struct piv_cbeff_record *pcr)
{
	uint32_t val;
	char *cptr;

	CPUT(pcr->patron_header_version, fp, bdb);
	CPUT(pcr->sbh_security_options, fp, bdb);
	LPUT(pcr->bdb_length, fp, bdb);
	SPUT(pcr->sb_length, fp, bdb);
	SPUT(pcr->bdb_format_owner, fp, bdb);
	SPUT(pcr->bdb_format_type, fp, bdb);
	OPUT(pcr->biometric_creation_date, 1, BIOMETRIC_CREATION_DATE_LEN,
	    fp, bdb);
	OPUT(pcr->validity_period, 1, VALIDITY_PERIOD_LEN, fp, bdb);
	val = htonl(pcr->biometric_type);
	cptr = ((char *)(&val)) + 1;
	OPUT(cptr, 1, BIOMETRIC_TYPE_LEN, fp, bdb);
	CPUT(pcr->biometric_data_type, fp, bdb);
	CPUT(pcr->biometric_data_quality, fp, bdb);
	OPUT(pcr->creator, 1, CREATOR_LEN, fp, bdb);
	OPUT(pcr->fascn, 1, FASCN_LEN, fp, bdb);
	OPUT(pcr->reserved, 1, RESERVED_LEN, fp, bdb);
	return (WRITE_OK);

err_out:
	return (WRITE_ERROR);
}

int
piv_write_pcr(FILE *fp, struct piv_cbeff_record *pcr)
{
	return (internal_piv_write_pcr(fp, NULL, pcr));
}
int
piv_push_pcr(BDB *pcrdb, struct piv_cbeff_record *pcr)
{
	return (internal_piv_write_pcr(NULL, pcrdb, pcr));
}

/*
 *
 */
int
piv_print_pcr(FILE *fp, struct piv_cbeff_record *pcr)
{
	int i;

	fprintf(fp, "Patron Header Version\t: 0x%02x\n",
	    pcr->patron_header_version);

	fprintf(fp, "SBH Security Options\t: 0x%02x ",
	    pcr->sbh_security_options);
	if (pcr->sbh_security_options == PIV_SECURITY_ENCRYPTED)
		fprintf(fp, "(Signed and Encrypted)\n");
	else if (pcr->sbh_security_options == PIV_SECURITY_NON_ENCRYPTED)
		fprintf(fp, "(Signed but not Encrypted)\n");
	else
		fprintf(fp, "(Invalid)\n");

	fprintf(fp, "BDB Length\t\t: %u\n", pcr->bdb_length);
	fprintf(fp, "SB Length\t\t: %hu\n", pcr->sb_length);
	fprintf(fp, "BDB Format Owner\t: 0x%04x\n", pcr->bdb_format_owner);

	fprintf(fp, "BDB Format Type\t\t: 0x%04x ", pcr->bdb_format_type);
	if (pcr->bdb_format_type == PIV_FORMAT_TYPE_FINGER_MINUTIAE)
		fprintf(fp, "(Fingerprint Minutiae Template)\n");
	else if (pcr->bdb_format_type == PIV_FORMAT_TYPE_FINGER_IMAGE)
		fprintf(fp, "(Fingerprint Image)\n");
	else if (pcr->bdb_format_type == PIV_FORMAT_TYPE_FACE_IMAGE)
		fprintf(fp, "(Face Image)\n");
	else
		fprintf(fp, "(Invalid)\n");

	fprintf(fp, "Creation Date\t\t: ");
	print_encoded_date(fp, pcr->biometric_creation_date);
	fprintf(fp, "\n");

	fprintf(fp, "Validity Period\t\t: ");
	print_encoded_date(fp, pcr->validity_period);
	fprintf(fp, " - ");
	print_encoded_date(fp, pcr->validity_period + PIV_ENCODED_DATA_LEN);
	fprintf(fp, "\n");

	fprintf(fp, "Biometric Type\t\t: 0x%06x ", pcr->biometric_type);
	if (pcr->biometric_type == PIV_BIO_TYPE_FACE)
		fprintf(fp, "(Face)\n");
	if (pcr->biometric_type == PIV_BIO_TYPE_FINGER)
		fprintf(fp, "(Finger)\n");
	else
		fprintf(fp, "(Other)\n");

	fprintf(fp, "Biometric Data Type\t: 0x%02hhx ",
	    pcr->biometric_data_type);
	if ((pcr->biometric_data_type & PIV_BIO_DATA_TYPE_MASK) ==
	    PIV_BIO_DATA_TYPE_RAW)
		fprintf(fp, "(Raw)\n");
	else if ((pcr->biometric_data_type & PIV_BIO_DATA_TYPE_MASK) ==
	    PIV_BIO_DATA_TYPE_INTERMEDIATE)
		fprintf(fp, "(Intermediate)\n");
	else if ((pcr->biometric_data_type & PIV_BIO_DATA_TYPE_MASK) ==
	    PIV_BIO_DATA_TYPE_PROCESSED)
		fprintf(fp, "(Processed)\n");
	else
		fprintf(fp, "(Invalid)\n");

	fprintf(fp, "Biometric Data Quality\t: %hhd\n",
	    pcr->biometric_data_quality);
	fprintf(fp, "Creator\t\t\t: %s\n", pcr->creator);

	fprintf(fp, "FASC-N\t\t\t: 0x");
	for (i = 0; i < FASCN_LEN; i++)
		fprintf(fp, "%02x", pcr->fascn[i]);
	fprintf(fp, "\n");

	fprintf(fp, "Reserved\t\t: 0x%08x\n", *(uint32_t *)pcr->reserved);
	fprintf(fp, "===========================================\n");
	return (0);
}

/*
 * 
 */
int
piv_verify_pcr(struct piv_cbeff_record *pcr, int bdb_len, int type)
{
	int ret = VALIDATE_OK;
	uint32_t res;
	struct piv_cbeff_record lpcr;

	switch (type) {
		case PIVFMR:
			lpcr.bdb_format_type = PIV_FORMAT_TYPE_FINGER_MINUTIAE;
			lpcr.biometric_type = PIV_BIO_TYPE_FINGER;
			lpcr.biometric_data_type = PIV_BIO_DATA_TYPE_PROCESSED;
			break;
		case PIVFIR:
			lpcr.bdb_format_type = PIV_FORMAT_TYPE_FINGER_IMAGE;
			lpcr.biometric_type = PIV_BIO_TYPE_FINGER;
			lpcr.biometric_data_type = PIV_BIO_DATA_TYPE_RAW;
			break;
		case PIVFRF:
			lpcr.bdb_format_type = PIV_FORMAT_TYPE_FACE_IMAGE;
			lpcr.biometric_type = PIV_BIO_TYPE_FACE;
			lpcr.biometric_data_type = PIV_BIO_DATA_TYPE_RAW;
			break;
		default:
			ERRP("Invalid type passed to %s", __FUNCTION__);
			return (VALIDATE_ERROR);
			break;
	}

	CSR(pcr->patron_header_version, 0x03, "Patron Header Version");
	if (type == PIVFMR) {
		CSR(pcr->sbh_security_options, PIV_SECURITY_NON_ENCRYPTED,
		    "SBH Security Options");
	} else {
		if ((pcr->sbh_security_options != PIV_SECURITY_NON_ENCRYPTED) &&
		    (pcr->sbh_security_options != PIV_SECURITY_ENCRYPTED)) {
			ERRP("SBH Security Options invalid value %d",
			    pcr->sbh_security_options);
			ret = VALIDATE_ERROR;
		}
	}

	CSR(pcr->bdb_length, bdb_len, "BDB Length");
	NCSR(pcr->sb_length, 0, "SB Length");
	CSR(pcr->bdb_format_owner, PIV_BDB_FORMAT_OWNER, "BDB Format Owner");

	CSR(pcr->bdb_format_type, lpcr.bdb_format_type, "BDB Format Type");
	CSR(pcr->biometric_type, lpcr.biometric_type, "Biometric Type");
	CSR((pcr->biometric_data_type & PIV_BIO_DATA_TYPE_MASK),
	    lpcr.biometric_data_type, "Biometric Data Type");

	if (type == PIVFRF) {
		if ((pcr->biometric_data_quality < -2) ||
		    (pcr->biometric_data_quality > 100)) {
			ERRP("Biometric quality is invalid.");
			ret = VALIDATE_ERROR;
		}
	} else {
		if ((pcr->biometric_data_quality != -2) &&
		    (pcr->biometric_data_quality != -1) &&
		    (pcr->biometric_data_quality != 20) &&
		    (pcr->biometric_data_quality != 40) &&
		    (pcr->biometric_data_quality != 60) &&
		    (pcr->biometric_data_quality != 80) &&
		    (pcr->biometric_data_quality != 100)) {
			ERRP("Biometric quality is invalid.");
			ret = VALIDATE_ERROR;
		}
	}
	res = *(uint32_t *)pcr->reserved;
	CSR(res, 0, "Reserved field");
	return (ret);
}
