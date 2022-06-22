/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>

#include <biomdimacro.h>
#include <fmr.h>
#include <isobit.h>
#include <tlv.h>
#include <nistapdu.h>
#include <cardaccess.h>
#include <mocapdu.h>
#include <moc.h>

#include "cardutils.h"
#include "genutils.h"

static void
usage()
{
	fprintf(stderr, "Usage: cardinfo [-d]\n"
	    "\t-d indicates a dry run, where APDUs are not performed\n"
	    "\t   but dumped to stdout instead.\n");
	exit (EXIT_FAILURE);
}

static void
print_apdu_response(APDU *apdu, BDB *response) {

	uint8_t *bptr;
	printf("Response to %s:", apdu->apdu_descr);
	for (bptr = (uint8_t *)response->bdb_start;
	    bptr < (uint8_t *)response->bdb_current; bptr++)
		printf(" %02X", *bptr);
	printf("\n");
}

/*
 * This program reads some info from a card that should always be
 * available: The card and matcher IDs, some BIT info. The BIT group
 * is written into an output file with the name based on the card
 * and matcher IDs.
 */ 
int
main(int argc, char *argv[])
{
	TLV *bit_group;
	BIT *bit[2];
	BDB cardresponse;
	void *respbuf = NULL;
	uint8_t sw1, sw2;
	char cardID[MAXIDSTRINGSIZE + 1], matcherID[MAXIDSTRINGSIZE + 1];
	int bit_count;
	int exitcode;
	int dryrun;
	int ret;
	int ch;
	SCARDCONTEXT context;
	SCARDHANDLE hCard;

	char **readers;
	int rdrcount;
	DWORD rdrprot;
	int i, r, sz;
	int idcount;

	char bitfn[MAXPATHLEN];
	FILE *bitfp;
	struct stat sb;

	exitcode = EXIT_FAILURE;
	dryrun = 0;

	if ((argc != 1) && (argc != 2))
		usage();

	while ((ch = getopt(argc, argv, "d")) != -1) {
		switch (ch) {
		case 'd':
			dryrun = 1;
			break;
		default :
			usage();
			break;
		}
	}

	/*
	 * Connect to the reader and card.
	 */
	if (SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &context) !=
	     SCARD_S_SUCCESS)
		ERR_EXIT("Could not establish contact with reader");
	if (getReaders(context, &readers, &rdrcount) != 0)
		ERR_EXIT("Could not get list of readers.");
	if (rdrcount < 1)
		ERR_EXIT("No readers found");

	respbuf = malloc(RESPONSEBUFSIZE);
	if (respbuf == NULL)
		ALLOC_ERR_EXIT("Response BDB buffer");
	INIT_BDB(&cardresponse, respbuf, RESPONSEBUFSIZE);

	for (r = 0; r < rdrcount; r++) {
		printf("\nTrying reader %s\n", readers[r]);
		if (SCardConnect(context, readers[r], SCARD_SHARE_EXCLUSIVE,
		    SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &rdrprot)
		    != 0) {
			INFOP("Could not connect to card or no card in reader");
			continue;
		}

		/* Select the MOC application. */
		ret = sendAPDU(hCard, &MOCSELECTAPP, dryrun, &cardresponse,
		    &sw1, &sw2);
		if ((ret != 0) && (!dryrun))
			ERR_OUT("Could not send '%s' APDU",
			    MOCSELECTAPP.apdu_descr);
		if ((sw1 != APDU_NORMAL_COMPLETE) && (!dryrun)) {

			/* Try using the alternate select APDU. */
			ret = sendAPDU(hCard, &ALTMOCSELECTAPP, dryrun,
			    &cardresponse, &sw1, &sw2);
			if ((ret != 0) && (!dryrun))
				ERR_OUT("Could not send '%s' APDU",
				    ALTMOCSELECTAPP.apdu_descr);
			if ((sw1 != APDU_NORMAL_COMPLETE) && (!dryrun)) {
				INFOP("Card is not MOC: SW1-SW2 0x%02X:%02X",
				    sw1, sw2);
				continue;
			}
		}

		/*
		 * Get the card and matcher IDs from the card.
		 */
		idcount = 0;
		REWIND_BDB(&cardresponse);
		ret = sendAPDU(hCard, &MOCGETCARDID, dryrun, &cardresponse,
		    &sw1, &sw2);
		if (!dryrun) {
			if (ret != 0) {
				ERRP("Could not send APDU to get card ID; "
				    "last response is 0x%02X:%02X", sw1, sw2);
			} else {
				CHECKSTATUS("GET CARD ID", sw1, sw2);
				REWIND_BDB(&cardresponse);
				if (getIDinresponse(cardID, &cardresponse)
				    != 0) {
					ERRP("Could not get card ID");
					print_apdu_response(&MOCGETCARDID,
					    &cardresponse);
				} else {
					printf("Card ID: %s\n", cardID);
					idcount++;
				}
			}
		}

		REWIND_BDB(&cardresponse);
		ret = sendAPDU(hCard, &MOCGETMATCHERID, dryrun, &cardresponse,
		    &sw1, &sw2);
		if (!dryrun) {
			if (ret != 0) {
				ERRP("Could not send APDU to get matcher ID; "
				    "last response is 0x%02X:%02X", sw1, sw2);
			} else {
				CHECKSTATUS("GET MATCHER ID", sw1, sw2);
				REWIND_BDB(&cardresponse);
				if (getIDinresponse(matcherID, &cardresponse)
				    != 0) {
					ERRP("Could not get matcher ID");
					print_apdu_response(&MOCGETMATCHERID,
					    &cardresponse);
				} else {
					printf("Matcher ID: %s\n", matcherID);
					idcount++;
				}
			}
		}

		/*
		 * Create a TLV object for the BIT group, and get the group
		 * from the card.
		 */
		if (new_tlv(&bit_group, 0, 0) != 0)
			ALLOC_ERR_EXIT("TLV structure");
		REWIND_BDB(&cardresponse);
		ret = sendAPDU(hCard, &MOCREADBIT, dryrun, &cardresponse,
		    &sw1, &sw2);
		if (dryrun)
			continue;

		if (ret != 0)
			ERR_OUT("Could not read BIT group; "
			    "last response is 0x%02X:%02X", sw1, sw2);
		CHECKSTATUS("BIT group read", sw1, sw2);

		/* Save the BIT to a file without clobbering; make sure we
		 * have both IDs for the file name.
		 */
		if (idcount == 2) {
			STRGENBITFN(bitfn, cardID, matcherID);
			if (stat(bitfn, &sb) == 0) {
				ERRP("BIT file %s exists; will not clobber",
				    bitfn);
			} else {
				bitfp = fopen(bitfn, "wb+");
				if (bitfp == NULL) {
					ERRP("Could not open BIT file %s.",
					    bitfn);
				} else {
					sz = cardresponse.bdb_current -
					    cardresponse.bdb_start;
					if (fwrite(cardresponse.bdb_start, 1,
					    sz, bitfp) != sz)
						ERRP("Could not write BIT file %s.",
						    bitfn);
					fclose(bitfp);
				}
			}
		} else {
			printf("BIT file not created.\n");
		}
		REWIND_BDB(&cardresponse);
		if (scan_tlv(&cardresponse, bit_group) != READ_OK) {
			ERRP("Scanning BIT group from card");
		} else {
			printf("BIT group TLV:\n");
			print_tlv(stdout, bit_group);
			if (get_bits_from_tlv(bit, bit_group, &bit_count)
			    != READ_OK)
				ERRP("Getting BITs from TLV group");
			else {
				for (i = 0; i < bit_count; i++) {
					printf("BIT %d:\n", i + 1);
					print_bit(stdout, bit[i]);
				}
			}
		}

	}
	exitcode = EXIT_SUCCESS;

err_out:
	if (respbuf != NULL)
		free(respbuf);
	exit (exitcode);
}
