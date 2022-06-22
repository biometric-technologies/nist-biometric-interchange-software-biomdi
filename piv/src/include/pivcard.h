/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#include <PCSC/winscard.h>

#ifndef _PIVCARD_H
#define _PIVCARD_H
#include <stdint.h>

/*
 * Structure used to represent a PIV card.
 */
struct pivcard {
	SCARDCONTEXT _pivCardContext;
	SCARDHANDLE _pivCardHandle;
};
typedef struct pivcard PIVCARD;

/*
 * Define the BER-TLV tag values assigned to the PIV card data objects when
 * requesting them with the GET DATA APDU. These tags are different than
 * the tag used on the data item that is returned.
 */
#define PIVCARDAUTHCERTTAG_DO	0x5FC101
#define PIVCHUIDTAG_DO		0x5FC102
#define PIVFINGERPRINTSTAG_DO	0x5FC103
#define PIVPIVAUTHCERTTAG_DO	0x5FC105
#define PIVSECURITYOBJECTTAG_DO	0x5FC106
#define PIVCCCTAG_DO		0x5FC107
#define PIVFACETAG_DO		0x5FC108
#define PIVPRINTEDINFOTAG_DO	0x5FC109
#define PIVDIGITALSIGCERTTAG_DO	0x5FC10A
#define PIVKEYMGMTCERTTAG_DO	0x5FC10B

/*
 * Define the tags used to identify the PIV data elements when they are
 * returned from the card. These 'elements' are contained within the
 * data objects using the request tags given above. This may not be
 * a complete list.
 */
#define PIVCARDINVALIDTAG_ELEM		0x00
#define PIVCARDAUTHCERTTAG_ELEM		0x70
#define PIVCARDAUTHCERTINFOTAG_ELEM	0x71
#define PIVFINGERPRINTSTAG_ELEM		0xBC
#define PIVFACETAG_ELEM			0xBC
#define PIVERRDETECTIONCODE_ELEM	0xBC

/*
 * pivCardConnect() connects to the first PIV card found in a reader.
 * Parameters:
 *   card    - (out) Object representing the card that is connected; set
 *             on success.
 * Returns:
 *   0              on success
 *   PIV_NOCARD otherwise
 */
int
pivCardConnect(PIVCARD *card);

/*
 * pivCardDisconnect() disconnects from the PIV card.
 * Parameters:
 *   card    - (in) The card that is currently connected.
 * Returns:
 *   0           on success
 *   PIV_CARDERR otherwise
 */
int
pivCardDisconnect(PIVCARD card);

/*
 * pivCardSaveContainer() reads a data object from a PIV card and saves the
 * PIV container object contained within the Tag-Length-Value (TLV) object.
 *
 * Parameters:
 *   card     - (in) Object representing the PIV card.
 *   objtag   - (in) The BER-TLV tag of the object to be retrieved.
 *   filename - (in) The name of the file to write to.
 * Returns:
 *   0           on success
 *   PIV_CARDERR otherwise
 */
int
pivCardSaveContainer(PIVCARD card, uint32_t objtag, char *filename);

/*
 * pivCardInserted() detects whether a PIV card is inserted in any smartcard
 * reader attached to the system.
 * 
 * Returns: 
 *   0              on success  
 *   PIV_NOCARD otherwise
 */
int pivCardInserted();

/*
 * pivCardGetFingerMinutiaeRec() returns the finger minutiae recrod from
 * the PIV card. A call to pivCardPINAuth() must be successfully made prior
 * to a call to this function. The data returned is the ANSI/INCITS-378
 * record.
 * 
 * Parameters:
 *   card   - (in) Object representing the PIV card.
 *   buffer - (in) Pointer to the buffer where the data is to be stored.
 *            This buffer must be allocated by the caller.
 *   bufsz  - (in/out) The size of the allocated buffer on input, the size
 *            of the minutiae data block on output.
 *
 * Returns: 
 *   0               - Success.
 *   PIV_NOCARD  - No PIV card is inserted.
 *   PIV_BUFSZ   - Insufficient memory in the caller's buffer.
 *   PIV_MEMERR  - Failed to allocate temporary memory buffer.
 *   PIV_DATAERR - Data from the card could not be processed.
 *   PIV_CARDERR - Some other error occurred when reading the card.
 */
int
pivCardGetFingerMinutiaeRec(PIVCARD card, uint8_t *buffer, unsigned int *bufsz);

/*
 * pivCardGetFaceImageRec() returns the facial data from the PIV card.  
 * A call to pivCardPINAuth() must be successfully made prior to a call to this
 * function. The data returned is the ANSI/INCITS-385 record.
 * 
 * Parameters:
 *   card   - (in) Object representing the PIV card.
 *   buffer - (in) Pointer to the buffer where the data is to be stored.
 *            This buffer must be allocated by the caller.
 *   bufsz  - (in/out) The size of the allocated buffer on input, the size
 *            of the minutiae data block on output.
 *
 * Returns: 
 *   0              - Success.
 *   PIV_NOCARD  - No PIV card is inserted.
 *   PIV_BUFSZ   - Insufficient memory in the caller's buffer.
 *   PIV_MEMERR  - Failed to allocate temporary memory buffer.
 *   PIV_DATAERR - Data from the card could not be processed.
 *   PIV_CARDERR - Some other error occurred when reading the card.
 */
int
pivCardGetFaceImageRec(PIVCARD card, uint8_t *buffer, unsigned int *bufsz);

/*
 * pivCardGetFaceImage() returns the facial image data from the PIV card.  
 * A call to pivCardPINAuth() must be successfully made prior to a call to this
 * function. The data returned is the JPEG data.
 * 
 * Parameters:
 *   card   - (in) Object representing the PIV card.
 *   buffer - (in) Pointer to the buffer where the data is to be stored.
 *            This buffer must be allocated by the caller.
 *   bufsz  - (in/out) The size of the allocated buffer on input, the size
 *            of the face data on output.
 *
 * Returns: 
 *   0              - Success.
 *   PIV_NOCARD  - No PIV card is inserted.
 *   PIV_BUFSZ   - Insufficient memory in the caller's buffer.
 *   PIV_MEMERR  - Failed to allocate temporary memory buffer.
 *   PIV_DATAERR - Data from the card could not be processed.
 *   PIV_CARDERR - Some other error occurred when reading the card.
 */
int
pivCardGetFaceImage(PIVCARD card, uint8_t *buffer, unsigned int *bufsz);

/*
 * pivCardPINAuth() authenticates to the PIV card using the PIN.
 *
 * Parameters:
 *   card   - (in) Object representing the PIV card.
 *   pin    - (in) Array containing the PIN. This array must be formatted
 *            per the PIV standard: 8 characters, padded with 0xFF.
 * Returns:
 *   0                  - PIN is valid.
 *   PIV_PINERR     - PIN is not in valid format.
 *   PIV_MEMERR     - Failed to allocate temporary memory buffer.
 *   PIV_PININVALID - PIN is not correct.
 *   PIV_CARDERR - Some other error occurred when reading the card.
 */
int
pivCardPINAuth(PIVCARD card, unsigned char pin[PIV_PIN_LENGTH]);
 
#endif	/* _PIVCARD_H */
