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
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>

#include <biomdimacro.h>
#include <isobit.h>
#include <fmr.h>
#include <tlv.h>
#include <nistapdu.h>
#include <cardaccess.h>
#include <moc.h>

#define RESPONSE_BUF_LEN	2048

/* Select the PIV application; this is just an example, not part of the
 * match-on-card tests.
 */
APDU PIVSELECTAPP = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xA4,
	.apdu_p1		= 0x04,
	.apdu_p2		= 0x00,
	.apdu_lc		= 0x0B,
	.apdu_nc		= { 0xA0, 0x00, 0x00, 0x03, 0x08, 0x00, 0x00, 0x10, 0x00, 0x01, 0x00 },
	.apdu_le		= 0x00,
	.apdu_field_mask	= APDU_FIELD_LC | APDU_FIELD_LE,
	.apdu_descr		= "Select PIV Application"
};
/* Verify with PIN to a PIV card */
APDU PIVVERIFYPIN = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0x20,
	.apdu_p1		= 0x00,
	.apdu_p2		= 0x80,
	.apdu_lc		= 0x08,
	.apdu_nc		= { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0xff, 0xff },
	.apdu_field_mask	= APDU_FIELD_LC,
	.apdu_descr		= "PIV Verify PIN"
};
/* Get the fingerprints from a PIV card */
APDU PIVGETFINGERPRINTS = {
	.apdu_cla		= 0x00,
	.apdu_ins		= 0xCB,
	.apdu_p1		= 0x3F,
	.apdu_p2		= 0xFF,
	.apdu_lc		= 0x05,
	.apdu_nc		= { 0x5C, 0x03, 0x5F, 0xC1, 0x03 },
	.apdu_le		= 0x00,
	.apdu_field_mask	= APDU_FIELD_LC | APDU_FIELD_LE,
	.apdu_descr		= "Get Fingerprints"
};

static void
print_apdu_response(APDU *apdu, BDB *response) {

	uint8_t *bptr;

	printf("Response for %s:\n", apdu->apdu_descr);
	for (bptr = (uint8_t *)response->bdb_start;
	    bptr < (uint8_t *)response->bdb_current; bptr++)
		printf(" %02X", *bptr);
        printf("\n");
}

static void
internal_send_apdu(SCARDHANDLE card, APDU *apdu, uint8_t *sw1, uint8_t *sw2)
{
	BDB response;
	uint8_t buf[RESPONSE_BUF_LEN];
	LONG rc;

	INIT_BDB(&response, buf, RESPONSE_BUF_LEN);
	rc = sendAPDU(card, apdu, 0, &response, sw1, sw2);
	if (rc != 0)
		ERR_EXIT("Could not send '%s' APDU", apdu->apdu_descr);
	print_apdu_response(apdu, &response);
	printf("SW1-SW2: 0x%02x%02x\n", *sw1, *sw2);

}

int main(int argc, char *argv[])
{
	LONG rc;
	DWORD protocol;
	SCARDCONTEXT context;
	SCARDHANDLE card;
	int count, i;
	uint8_t sw1, sw2;
	char **readers;

	rc = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context);
	if (rc != SCARD_S_SUCCESS)
		ERR_EXIT("Could not establish contact with reader");
	printf("SCardEstablishContext successful.\n");

	rc = getReaders(context, &readers, &count);
	if (rc != 0)
		ERR_EXIT("Unsuccessful call to getReaders()");
	if (count < 1)
		ERR_EXIT("No readers found");
	printf("Found %d readers.\n", count);

	for (i = 0; i < count; i++)
		printf("Reader %s found.\n", readers[i]);

	/* Find the PIV card */
	for (i = 0; i < count; i++) {
		printf("\nTrying reader %s\n", readers[i]);
		rc = SCardConnect(context, readers[i], SCARD_SHARE_EXCLUSIVE,
		    SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &card, &protocol);
		if (rc != 0) {
			INFOP("Could not connect to card or no card in reader");
			continue;
		}
		internal_send_apdu(card, &PIVSELECTAPP, &sw1, &sw2);
		if (sw1 == APDU_NORMAL_COMPLETE)
			break;
		INFOP("Card in reader is not PIV");
		SCardDisconnect(card, SCARD_RESET_CARD);
	}
	if (sw1 != APDU_NORMAL_COMPLETE)
		ERR_EXIT("Could not connect to card");

	printf("\nPIV card found in %s\n", readers[i]);
	internal_send_apdu(card, &PIVVERIFYPIN, &sw1, &sw2);
	internal_send_apdu(card, &PIVGETFINGERPRINTS, &sw1, &sw2);

	for (i = 0; i < count; i++)
		free(readers[i]);
	free(readers);

	exit (0);
}
