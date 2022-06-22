/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#ifndef _CARD_ACCESS_H
#define _CARD_ACCESS_H
#include <PCSC/winscard.h>
#include <stdint.h>

/*
 * Later versions of the PCSC-lite API don't define the extended buffer
 * size explicitly. Rather, the API consumer must define
 * PCSCLITE_ENHANCED_MESSAGING to have MAX_BUFFER_SIZE defined as the
 * larger size. We want to support older PCSC APIs, so define the max
 * buffer size here, if not already defined.
 */
#ifndef MAX_BUFFER_SIZE_EXTENDED
#define MAX_BUFFER_SIZE_EXTENDED	(1<<15)
#endif

/******************************************************************************/
/* Get a list of attached PCSC readers.                                       */
/*                                                                            */
/* Parameters:                                                                */
/*   context   The smartcard context object.                                  */
/*   readerPtr Address of a pointer to an array of strings that will be       */
/*             allocated and initialized to the reader names. An array of     */
/*             (of size count) char pointers will be allocated, and each of   */
/*             those pointers will point to allocated storage containing the  */
/*             a reader name. It is up to the caller to free these pointers   */
/*             an the array itself.                                           */
/*   count     Address of an integer that will contain the number of          */
/*             readers that were found.                                       */
/*                                                                            */
/* Returns:                                                                   */
/*    0     Success                                                           */
/*   -1     Failure                                                           */
/******************************************************************************/
int
getReaders(SCARDCONTEXT context, char ***readerPtr, int *count);

/******************************************************************************/
/* Add data to to an APDU. The data will become the Nc field of the APDU,     */
/* and the Lc field will be set.                                              */
/*                                                                            */
/* Parameters:                                                                */
/*   data   Pointer to data block.                                            */
/*   len    The length of the input data.                                     */
/*   apdu   Pointer to the input APDU structure.                              */
/******************************************************************************/
void
add_data_to_apdu(uint8_t *data, uint16_t len, APDU *apdu);

/******************************************************************************/
/* Send an APDU to a card.  This function handles all command and response    */
/* chaining, therefore the response buffer must contain enough room for the   */
/* entire card's response, not just the response from a single APDU           */
/* transmission.                                                              */
/*                                                                            */
/* Parameters:                                                                */
/*   hCard     The smartcard context object.                                  */
/*   apdu      The APDU structure containing the APDU hex string and          */
/*             description.                                                   */
/*   dryrun    If 1, don't actually send the APDU, but dump what would be     */
/*             sent to stdout.                                                */
/*   response  Pointer to biometric data block that will contain the response */
/*             from the card on success. This can be NULL, meaning the caller */
/*             is not interested in the card's response. The response data    */
/*             will not include the status words.                             */
/*   sw1       Pointer to the first status word returned from the card.       */
/*   sw2       Pointer to the second status word returned from the card.      */
/*                                                                            */
/* Returns:                                                                   */
/*    0     Success                                                           */
/*   -1     Failure                                                           */
/******************************************************************************/
int
sendAPDU(SCARDHANDLE hCard, APDU *apdu, int dryrun, BDB *response, uint8_t *sw1,
    uint8_t *sw2);

#endif /* _CARD_ACCESS_H */
