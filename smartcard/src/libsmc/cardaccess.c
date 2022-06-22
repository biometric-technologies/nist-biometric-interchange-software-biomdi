/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/
/*
 * This software requires that the PCSC Lite package be installed, or other
 * similar smartcard libraries be present. An open source implementation
 * of this library can be found at http://www.linuxnet.com/
 * This same library is included with Mac OS-X 10.4 and higher, although
 * the header files may need to be installed manually.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>

#include <biomdimacro.h>
#include <nistapdu.h>

#include <cardaccess.h>

void
add_data_to_apdu(uint8_t *data, uint16_t len, APDU *apdu)
{
	memcpy((void *)apdu->apdu_nc, (void *)data, len);
	apdu->apdu_lc = len;
}

static inline LONG
internal_get_response(SCARDHANDLE hCard,
    SCARD_IO_REQUEST pioSendPci, 
    uint8_t *recvBuf, DWORD recvLen, BDB *respBDB, uint8_t *sw1, uint8_t *sw2)
{
	LONG rc;
	uint8_t lsw1, lsw2;
	uint8_t bGetRes[5] = {0x00, 0xC0, 0x00, 0x00, 0x00};
	DWORD lRecvLen;

	/* Get the response status words and check whether chaining was done */
	lsw1 = recvBuf[recvLen - 2];
	lsw2 = recvBuf[recvLen - 1];

	/* Handle response chaining */
	lRecvLen = recvLen;
	while (lsw1 == APDU_NORMAL_CHAINING) {
		if (respBDB != NULL)
			OPUSH(recvBuf, lRecvLen - 2, respBDB);
		bGetRes[4] = lsw2;	/* The Le field */
		lRecvLen = (0 == lsw2) ? 256 : lsw2;
		lRecvLen += 2;	/* Account for the SW */
		rc = SCardTransmit(hCard, &pioSendPci, bGetRes, 5, NULL,
		    recvBuf, &lRecvLen);

		if (rc != SCARD_S_SUCCESS)
			ERR_OUT("Transmit of GET RESPONSE: %s",
			    pcsc_stringify_error(rc));

		lsw1 = recvBuf[lRecvLen - 2];
		lsw2 = recvBuf[lRecvLen - 1];
	}
	*sw1 = lsw1;
	*sw2 = lsw2;
	if (respBDB != NULL)
		OPUSH(recvBuf, lRecvLen - 2, respBDB);
	return (SCARD_S_SUCCESS);
err_out:
	return (rc);
}

#define HEXDUMPBUF(desc, buf, len)					\
do {									\
	int idx;							\
	printf("%s\n", desc);						\
	for (idx = 0; idx < len; idx++) {				\
		printf("%02hhX ", buf[idx]);				\
		if (((idx+1) % 16) == 0)				\
			printf("\n");					\
	}								\
	printf("\n");							\
} while (0)

/*
 * Send an APDU to a card using command chaining.
 */
static inline LONG
internal_send_chained(SCARDHANDLE hCard, SCARD_IO_REQUEST pioSendPci,
    APDU *apdu, int dryrun, BDB *respBDB, uint8_t *sw1, uint8_t *sw2)
{
	LONG rc;
	int LcLen;
	int maxLcLen;
	int ncIndex;
	uint8_t bSendBuffer[MAX_BUFFER_SIZE];
	uint8_t bRecvBuffer[MAX_BUFFER_SIZE];
	DWORD sendIndex;
	DWORD recvLength;

	recvLength = sizeof(bRecvBuffer);

	bSendBuffer[0] = apdu->apdu_cla;
	bSendBuffer[1] = apdu->apdu_ins;
	bSendBuffer[2] = apdu->apdu_p1;
	bSendBuffer[3] = apdu->apdu_p2;

	LcLen = apdu->apdu_lc;
	maxLcLen = APDU_MAX_SHORT_LC - APDU_HEADER_LEN - APDU_FLEN_TRAILER;
	if (apdu->apdu_field_mask & APDU_FIELD_LC)
		maxLcLen -= APDU_FLEN_LC_SHORT;

	if (apdu->apdu_field_mask & APDU_FIELD_LE) {
		if (apdu->apdu_le > APDU_MAX_SHORT_LE) {
			rc = SCARD_F_INTERNAL_ERROR;
			ERR_OUT("Invalid Le value: %d\n", apdu->apdu_le);
		}
		maxLcLen -= APDU_FLEN_LE_SHORT;
	}
	ncIndex = 0;

	while (LcLen > 0) {
		sendIndex = 4;
		if (apdu->apdu_field_mask & APDU_FIELD_LC) {
			if (LcLen > maxLcLen) {
				bSendBuffer[0] |= APDU_FLAG_CLA_CHAIN;
				bSendBuffer[sendIndex] = (uint8_t)maxLcLen;
				sendIndex += 1;
				memcpy(&bSendBuffer[sendIndex],
				    &apdu->apdu_nc[ncIndex], maxLcLen);
				ncIndex += maxLcLen;
				sendIndex += maxLcLen;
				LcLen -= maxLcLen;
			} else { /* Note that LcLen cannot be 0 at this point */
				bSendBuffer[0] &= ~APDU_FLAG_CLA_CHAIN;
				bSendBuffer[sendIndex] = (uint8_t)LcLen;
				sendIndex += 1;
				memcpy(&bSendBuffer[sendIndex],
				    &apdu->apdu_nc[ncIndex], LcLen);
				ncIndex += LcLen;
				sendIndex += LcLen;
				LcLen = 0;
			}
		}
		if (apdu->apdu_field_mask & APDU_FIELD_LE) {
			bSendBuffer[sendIndex] = (uint8_t)apdu->apdu_le;
			sendIndex += 1;
		}

		/* At this point, sendIndex is the location of where the next
		 * byte would be written into the send buffer, and that happens
		 * to be the current length of the send buffer as well.
		 */
		if (dryrun == 0) {
#ifdef DEBUG_OUTPUT
			HEXDUMPBUF(apdu->apdu_descr, bSendBuffer, sendIndex);
#endif
			rc = SCardTransmit(hCard, &pioSendPci, bSendBuffer,
			    sendIndex, NULL, bRecvBuffer, &recvLength);
			if (rc != SCARD_S_SUCCESS)
				ERR_OUT("Transmit of %s: %s", apdu->apdu_descr,
				    pcsc_stringify_error(rc));
			rc = internal_get_response(hCard, pioSendPci,
			    bRecvBuffer, recvLength, respBDB,
			    sw1, sw2);
			if (rc != SCARD_S_SUCCESS)
				ERR_OUT("Getting response for %s: %s",
				    apdu->apdu_descr, pcsc_stringify_error(rc));
#ifdef DEBUG_OUTPUT
			printf("Send chained response for %s: 0x%02X%02X\n",
			    apdu->apdu_descr, *sw1, *sw2);
			printf("Response buffer for %s:\n", apdu->apdu_descr);
			DUMP_BDB(respBDB); printf("\n");
#endif
			if (*sw1 == APDU_CHECK_ERR_CLA_FUNCTION)
				ERR_OUT("Send chained response for %s: 0x%02X%02X",
				    apdu->apdu_descr, *sw1, *sw2);
		} else {
			HEXDUMPBUF(apdu->apdu_descr, bSendBuffer, sendIndex);
			printf("----\n");
			*sw1 = APDU_NORMAL_COMPLETE;
			*sw2 = 0;
		}
	}
	return (SCARD_S_SUCCESS);
err_out:
	return (rc);
}

/*
 * Send an APDU to a card using extended Le and Lc fields.
 */
static inline LONG
internal_send_extended(SCARDHANDLE hCard, SCARD_IO_REQUEST pioSendPci,
    APDU *apdu, int dryrun, BDB *respBDB, uint8_t *sw1, uint8_t *sw2)
{
	LONG rc;
	int lcle_extended;
	uint8_t bSendBuffer[MAX_BUFFER_SIZE_EXTENDED];
	uint8_t bRecvBuffer[MAX_BUFFER_SIZE_EXTENDED];
	DWORD sendIndex;
	DWORD recvLength;

	recvLength = sizeof(bRecvBuffer);

	bSendBuffer[0] = apdu->apdu_cla;
	bSendBuffer[1] = apdu->apdu_ins;
	bSendBuffer[2] = apdu->apdu_p1;
	bSendBuffer[3] = apdu->apdu_p2;
	sendIndex = 4;
	/*
	 * The Lc and Le fields are 0, 1, or 3 bytes. If the length is present
	 * and will fit in one byte, store it; else, store a '00' byte, then
	 * the value. Also, if either field is extended, then both must be.
	 * (Note that we are assuming the card can accept extended fields;
	 * we really should check the card capabilities third table, if that's
	 * even present. Of course, it may not be, but that is also where the
	 * command chaining indicator is located; 7816-4 is just wonderful.
	 * You're probably better off just shoving data to the card and
	 * hoping its tiny little head doesn't explode; that can be detected.)
	 */
	if ((apdu->apdu_lc > APDU_MAX_SHORT_LC) ||
	    (apdu->apdu_le > APDU_MAX_SHORT_LE))
		lcle_extended = 1;
	else
		lcle_extended = 0;
	if (apdu->apdu_field_mask & APDU_FIELD_LC) {
		if (lcle_extended) {
			bSendBuffer[sendIndex] = 0;
			uint16_t val = (uint16_t)htons(apdu->apdu_lc);
			(void)memcpy(bSendBuffer + sendIndex+1, &val, 2);
			sendIndex += 3;
		} else {
			bSendBuffer[sendIndex] = (uint8_t)apdu->apdu_lc;
			sendIndex += 1;
		}
		memcpy(bSendBuffer + sendIndex, apdu->apdu_nc, apdu->apdu_lc);
		sendIndex += apdu->apdu_lc;
	}
	if (apdu->apdu_field_mask & APDU_FIELD_LE) {
		if (lcle_extended) {
			bSendBuffer[sendIndex] = 0;
			uint16_t val = (uint16_t)htons(apdu->apdu_le);
			(void)memcpy(bSendBuffer + sendIndex+1, &val, 2);
			sendIndex += 3;
		} else {
			bSendBuffer[sendIndex] = (uint8_t)apdu->apdu_le;
			sendIndex += 1;
		}
	}
	/* At this point, sendIndex is the location of where the next
	 * byte would be written into the send buffer, and that happens
	 * to be the current length of the send buffer as well.
	 */
	if (dryrun == 0) {
#ifdef DEBUG_OUTPUT
		HEXDUMPBUF(apdu->apdu_descr, bSendBuffer, sendIndex);
#endif
		rc = SCardTransmit(hCard, &pioSendPci, bSendBuffer, sendIndex,
			NULL, bRecvBuffer, &recvLength);
		if (rc != SCARD_S_SUCCESS)
			ERR_OUT("Transmit of %s: %s", apdu->apdu_descr,
			    pcsc_stringify_error(rc));
		rc = internal_get_response(hCard, pioSendPci,
		    bRecvBuffer, recvLength, respBDB, sw1, sw2);
		if (rc != SCARD_S_SUCCESS)
			ERR_OUT("Getting response for %s: %s", apdu->apdu_descr,
			    pcsc_stringify_error(rc));

#ifdef DEBUG_OUTPUT
		printf("Send extended response for %s: 0x%02X%02X\n",
		    apdu->apdu_descr, *sw1, *sw2);
		printf("Response buffer for %s:\n", apdu->apdu_descr);
		DUMP_BDB(respBDB); printf("\n");
#endif
	} else {
		HEXDUMPBUF(apdu->apdu_descr, bSendBuffer, sendIndex);
		*sw1 = APDU_NORMAL_COMPLETE;
		*sw2 = 0;
	}
	return (SCARD_S_SUCCESS);
err_out:
	return (rc);
}

int
sendAPDU(SCARDHANDLE hCard, APDU *apdu, int dryrun, BDB *response, uint8_t *sw1,
    uint8_t *sw2)
{
	LONG rc;
 	SCARD_IO_REQUEST pioSendPci;
	DWORD dwActiveProtocol;
	int endtransaction;
	int status;

	status = -1;
	endtransaction = 0;

	/* connect to a reader (even without a card) */
	dwActiveProtocol = -1;
	rc = SCardReconnect(hCard, SCARD_SHARE_EXCLUSIVE,
		SCARD_PROTOCOL_T0|SCARD_PROTOCOL_T1, SCARD_LEAVE_CARD,
		&dwActiveProtocol);
	if (rc != SCARD_S_SUCCESS)
		ERR_OUT("SCardReconnect: %s", pcsc_stringify_error(rc));

	switch(dwActiveProtocol) {
		case SCARD_PROTOCOL_T0:
			pioSendPci = *SCARD_PCI_T0;
			break;
		case SCARD_PROTOCOL_T1:
			pioSendPci = *SCARD_PCI_T1;
			break;
		default:
			printf("Unknown protocol\n");
			return (-1);
	}

	if (dryrun == 0) {
		rc = SCardBeginTransaction(hCard);
		if (rc != SCARD_S_SUCCESS) {
			(void)SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
			ERR_OUT("SCardBeginTransaction %s",
			    pcsc_stringify_error(rc));
		}
		endtransaction = 1;
	}
	if (dwActiveProtocol == SCARD_PROTOCOL_T0)
		rc = internal_send_chained(hCard, pioSendPci, apdu, dryrun,
		    response, sw1, sw2);
	else
		rc = internal_send_extended(hCard, pioSendPci, apdu, dryrun,
		    response, sw1, sw2);
	if (rc != SCARD_S_SUCCESS)
		ERR_OUT("Send of APDU failed");

	status = 0;

err_out:
	if ((dryrun == 0) && (endtransaction == 1)) {
		rc = SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
		if (rc != SCARD_S_SUCCESS) {
			status = -1;
			ERRP("End Transaction: %s",
			    pcsc_stringify_error(rc));
		}
	}
	return (status);
}
