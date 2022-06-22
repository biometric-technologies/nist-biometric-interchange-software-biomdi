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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
#include <nistapdu.h>
#include <biomdimacro.h>
#include <cardaccess.h>
#include <piv.h>
#include <pivapdu.h>
#include <pivcard.h>
#include <pivdata.h>
#include <tlv.h>
#include <frf.h>	/* From BIOMDI, INCITS-385 Face Recognition */

static uint8_t mapElemTagFromObjTag(uint32_t objtag)
{
	switch (objtag) {
		case PIVFINGERPRINTSTAG_DO:
			return (PIVFINGERPRINTSTAG_ELEM);
			break;		/* not reached */
		case PIVFACETAG_DO:
			return (PIVFACETAG_ELEM);
			break;		/* not reached */
		default:
			return (PIVCARDINVALIDTAG_ELEM);
			break;		/* not reached */
	}
}

static APDU *
mapAPDUFromObjTag(uint32_t objtag)
{
	APDU *apdu;

	switch (objtag) {
		case PIVCARDAUTHCERTTAG_DO:
			apdu = &PIVGETCARDAUTHCERT;
			break;
		case PIVCHUIDTAG_DO:
			apdu = &PIVGETCHUID;
			break;
		case PIVFINGERPRINTSTAG_DO:
			apdu = &PIVGETFINGERPRINTS;
			break;
		case PIVPIVAUTHCERTTAG_DO:
			apdu = &PIVGETPIVAUTHCERT;
			break;
		case PIVSECURITYOBJECTTAG_DO:
			apdu = &PIVGETSECURITYOBJECT;
			break;
		case PIVCCCTAG_DO:
			apdu = &PIVGETCCC;
			break;
		case PIVFACETAG_DO:
			apdu = &PIVGETFACE;
			break;
		case PIVPRINTEDINFOTAG_DO:
			apdu = &PIVGETPRINTEDINFO;
			break;
		case PIVDIGITALSIGCERTTAG_DO:
			apdu = &PIVGETDIGITALSIGCERT;
			break;
		case PIVKEYMGMTCERTTAG_DO:
			apdu = &PIVGETKEYMGMTCERT;
			break;
		default:
			apdu = NULL;
			break;
	}
	return (apdu);
}

int
pivCardConnect(PIVCARD *card)
{
	LONG ret;
	uint8_t sw1, sw2;
	char **readers;
	int rdrcount, r;
	DWORD rdrprot;
	BDB cardresponse;
	void *respbuf;
	int status;
	SCARDCONTEXT context;
	SCARDHANDLE handle;

	status = PIV_NOCARD;

	/*
         * Connect to the reader and card.
         */
	readers = NULL;
	respbuf = NULL;
	ret = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
	if (ret != SCARD_S_SUCCESS)
		ERR_OUT("Could not establish contact with reader: %s",
		    pcsc_stringify_error(ret));
	if (getReaders(context, &readers, &rdrcount) != 0)
		ERR_OUT("Could not get list of readers.");
	if (rdrcount < 1)
		ERR_OUT("No readers found");

	respbuf = malloc(PIV_MAX_OBJECT_SIZE);
	if (respbuf == NULL)
		ALLOC_ERR_OUT("Response BDB buffer");
	INIT_BDB(&cardresponse, respbuf, PIV_MAX_OBJECT_SIZE);
	for (r = 0; r < rdrcount; r++) {
		if (SCardConnect(context, readers[r], SCARD_SHARE_EXCLUSIVE,
		    SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &handle, &rdrprot)
		    != 0)
			continue;

		/* Select the PIV application. */
		ret = sendAPDU(handle, &PIVSELECTAPP, 0, &cardresponse,
		    &sw1, &sw2);
		if (ret != 0)
			ERR_OUT("Could not send '%s' APDU",
			    PIVSELECTAPP.apdu_descr);
		if (sw1 == APDU_NORMAL_COMPLETE) {
			card->_pivCardContext = context;
			card->_pivCardHandle = handle;
			status = 0;
			break;
		} else {
			continue;
		}
	}
err_out:
	if (status != 0)
		SCardReleaseContext(context);
	if (readers != NULL)
		free(readers);
	if (respbuf != NULL)
		free(respbuf);
	return (status);
}

int
pivCardDisconnect(PIVCARD card)
{
	PCSC_API LONG ret;

	ret = SCardDisconnect(card._pivCardHandle, SCARD_UNPOWER_CARD);
	if (ret != 0) {
		ERRP("Could not disconnect: %s (0x%lX)\n",
		    pcsc_stringify_error(ret), ret);
		return (PIV_CARDERR);
	}
	ret = SCardReleaseContext(card._pivCardContext);
	if (ret != SCARD_S_SUCCESS) {
		ERRP("Could not release: %s (0x%lX)\n",
		    pcsc_stringify_error(ret), ret);
		return (PIV_CARDERR);
	}
	return (0);
}

/*
 */
static int
pivReadDataObject(PIVCARD card, uint32_t objtag, BDB *cardobject)
{
	APDU *apdu;
	int ret;
	uint8_t sw1, sw2;
	int dryrun = 0;

	apdu = mapAPDUFromObjTag(objtag);
	if (apdu == NULL)
		return (PIV_PARMERR);

	ret = sendAPDU(card._pivCardHandle, apdu, dryrun, cardobject,
	    &sw1, &sw2);
	if (ret != 0)
		ERR_OUT("APDU '%s' failed", apdu->apdu_descr);
	CHECKSTATUS(apdu->apdu_descr, sw1, sw2);
	REWIND_BDB(cardobject);
#if DEBUG_OUTPUT
	printf("[%s] \n", apdu->apdu_descr);
	DUMP_BDB(cardobject);
	REWIND_BDB(cardobject);
	printf("\n");
	fflush(stdout);
#endif

	return (0);
err_out:
	return (PIV_CARDERR);
}

/*
 */
int
pivCardSaveContainer(PIVCARD card, uint32_t objtag, char *filename)
{
	BDB carddata;
	uint8_t *databuf;
	TLV *tlv;
	LONG ret;
	FILE *fp;
	int status;

	status = PIV_CARDERR;
	fp = NULL;
	tlv = NULL;
	
	databuf = malloc(PIV_MAX_OBJECT_SIZE);
	if (databuf == NULL)
		return (PIV_MEMERR);
	INIT_BDB(&carddata, databuf, PIV_MAX_OBJECT_SIZE);
	ret = pivReadDataObject(card, objtag, &carddata);
	if (ret != 0)
		ERR_OUT("Could not read card data");
	new_tlv(&tlv, 0, 0);
	if (scan_tlv(&carddata, tlv) != READ_OK)
		ERR_OUT("Could not scan data object into TLV");

	fp = fopen(filename, "wb");
	if (fp == NULL)
		ERR_OUT("Could not open file '%s'", filename);
	if (fwrite(tlv->tlv_value.tlv_primitive, 1,
	    tlv->tlv_length, fp) != tlv->tlv_length) 
		ERR_OUT("Could not write file '%s'", filename);
	
	status = 0;
err_out:
	if (status != 0) {
		if (fp != NULL) {
			remove(filename);
			fp = NULL;
		}
	}
	if (fp != NULL)
		fclose(fp);
	if (tlv != NULL)
		free_tlv(tlv);
	if (databuf != NULL)
		free(databuf);
	return (status);
}

/*
 */
int
pivCardInserted()
{
	PIVCARD card;
	int ret;

	ret = pivCardConnect(&card);
	if (ret == 0) {
		(void)pivCardDisconnect(card);
		return (0);
	}
	return (PIV_NOCARD);
}

/*
 * Get some card data, extracted from the card object. This function works
 * for data that is contained within a CBEFF wrapper, fingerprint minutiae
 * and facial image, for example.
 */
static int
pivCardGetCardData(PIVCARD card, uint32_t objtag, uint8_t *buffer,
    unsigned int *bufsz)
{
	BDB carddata;
	void *databuf;
	BDB pcrdb;
	struct piv_cbeff_record pcr;
	uint8_t sw1, sw2;
	int ret;
	int status;
	unsigned int datasz;
	TLV *tlv = NULL;
	uint8_t *dptr;
	uint8_t elem_tag;

	elem_tag = mapElemTagFromObjTag(objtag);
	if (elem_tag == PIVCARDINVALIDTAG_ELEM)
		return(PIV_PARMERR);

	databuf = malloc(PIV_MAX_OBJECT_SIZE);
	if (databuf == NULL) {
		status = PIV_MEMERR;
		goto err_out;
	}
	INIT_BDB(&carddata, databuf, PIV_MAX_OBJECT_SIZE);
	ret = pivReadDataObject(card, objtag, &carddata);
	if (ret != 0) {
		status = ret;
		goto err_out;
	}

	/*
	 * The minutiae and face image on a PIV card are contained within a 
	 * CBEFF wrapper, contained within a TLV which is within a TLV. The
	 * top-level TLV is a proper BER_TLV, so we can scan that. However,
	 * the second-level TLV is not a BER-TLV, contrary to claims made in
	 * the PIV spec. (Fingerprints have a tag of 0xBC, which because bit 6
	 * is on, the value field of this TLV should also be a TLV
	 * ('constructed'), but it isn't; we have the length field immediately,
	 * so we really have a primitive encoding of the value field).
	 * We handle the second TLV directly here, and not with the TLV lib.
	 */

	/* Scan the first TLV, whose value field contains the second TLV */
	new_tlv(&tlv, 0, 0);
	REWIND_BDB(&carddata);
	if (scan_tlv(&carddata, tlv) != READ_OK) {
		status = PIV_DATAERR;
		goto err_out;
	}
	/* The second TLV is scanned as primitive data */
	if (tlv->tlv_value.tlv_primitive[0] != elem_tag) {
		status = PIV_DATAERR;
		goto err_out;
	}
	/* PIV treats the length field as if this was a BER-TLV, so check for
	 * the special codes. We only support a 3-byte length field now.
	 */
	if (tlv->tlv_value.tlv_primitive[1] != BERTLV_SB_MB_LENGTH_MB_3) {
		status = PIV_DATAERR;
		goto err_out;
	}
	/* 4 octets: 1 for the element tag, 3 for the length field */
	dptr = tlv->tlv_value.tlv_primitive + 4;

	/* Now, scan off the CBEFF info; we need the biometric data block
	 * length.
	 */
	INIT_BDB(&pcrdb, dptr, tlv->tlv_length);
	if (piv_scan_pcr(&pcrdb, &pcr) != READ_OK) {
		status = PIV_DATAERR;
		goto err_out;
	}
	dptr += CBEFF_HDR_LEN;
	datasz = pcr.bdb_length;
	/* Make sure there's enough room in the output buffer */
	if (*bufsz < datasz) {
		status = PIV_BUFSZ;
		goto err_out;
	}
	memcpy(buffer, dptr, datasz);
	*bufsz = datasz;
	status = 0;
err_out:
	if (tlv != NULL)
		free_tlv(tlv);
	if (databuf != NULL)
		free(databuf);
	return (status);
}

/*
 */
int
pivCardGetFingerMinutiaeRec(PIVCARD card, uint8_t *buffer, unsigned int *bufsz)
{
	return (pivCardGetCardData(card, PIVFINGERPRINTSTAG_DO, buffer, bufsz));
}

/*
 */
int
pivCardGetFaceImageRec(PIVCARD card, uint8_t *buffer, unsigned int *bufsz)
{
	return (pivCardGetCardData(card, PIVFACETAG_DO, buffer, bufsz));
}

/*
 */
int
pivCardGetFaceImage(PIVCARD card, uint8_t *buffer, unsigned int *bufsz)
{
	int ret;
	int status;
	uint8_t *lbuf;
	unsigned int lbufsz;
	FB *fb;
	FDB *fdb;
	BDB bdb;

	lbuf = malloc(PIV_MAX_OBJECT_SIZE);
	if (lbuf == NULL)
		return (PIV_MEMERR);

	status = PIV_CARDERR;
	lbufsz = PIV_MAX_OBJECT_SIZE;
	INIT_BDB(&bdb, lbuf, PIV_MAX_OBJECT_SIZE);
	ret = pivCardGetCardData(card, PIVFACETAG_DO, lbuf, &lbufsz);

	fb = NULL;
	if (new_fb(&fb) < 0) {
		status = PIV_MEMERR;
		goto err_out;
	}
	ret = scan_fb(&bdb, fb);
	if (ret != READ_OK) {
		status = PIV_CARDERR;
		goto err_out;
	}
	/* Pull the first (and only) facial data block from the parent */
	fdb = TAILQ_FIRST(&fb->facial_data);
	if (fdb == NULL) {
		status = PIV_CARDERR;
		goto err_out;
	}
	/* Check the input buffer for room */
	if (*bufsz < fdb->image_len) {
		status = PIV_BUFSZ;
		goto err_out;
	}
	memcpy(buffer, fdb->image_data, fdb->image_len);
	*bufsz = fdb->image_len;

	status = 0;
err_out:
	if (lbuf != NULL)
		free(lbuf);
	return (status);
}

int
pivCardPINAuth(PIVCARD card, unsigned char pin[PIV_PIN_LENGTH])
{
	int ret;
	uint8_t sw1, sw2;
	BDB cardresponse;
	void *respbuf;
	APDU apdu;

	ret = pivValidatePIN(pin);
	if (ret != 0)
		return (PIV_PINERR);

	respbuf = malloc(PIV_MAX_OBJECT_SIZE);
	if (respbuf == NULL)
		return (PIV_MEMERR);

	INIT_BDB(&cardresponse, respbuf, PIV_MAX_OBJECT_SIZE);
	apdu = PIVVERIFYPIN;
	add_data_to_apdu(pin, PIV_PIN_LENGTH, &apdu);
	ret = sendAPDU(card._pivCardHandle, &apdu, 0,
		    &cardresponse, &sw1, &sw2);
	free(respbuf);
	if (ret != 0)
		return (PIV_CARDERR);

	if (sw1 != APDU_NORMAL_COMPLETE)
		return (PIV_PININVALID);

	return (0);
}
