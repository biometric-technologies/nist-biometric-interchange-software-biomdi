/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#ifndef _MOC_H
#define _MOC_H

/*
 * Define the minutiae template data object TLV tags for MINEX-II MOC testing
 */

#define MTDOTAG_BIOMETRIC_DATA_TEMPLATE		0x7F2E
#define MTDOTAGSIZE_BIOMETRIC_DATA_TEMPLATE	2
#define MTDOTAG_FINGER_MINUTIAE_DATA		0x81
#define MTDOTAGSIZE_FINGER_MINUTIAE_DATA	1

#define SCORETAG				0xC0
#define SCORESIZE				2

/* Define the data object IDs for the card and matcher IDs. */
#define CARDIDDOID				0x66
#define MATCHERIDDOID				0x6E
#define CARDIDTAG				0x88
#define MATCHERIDTAG				0x99
#define MAXIDSIZE				8
#define MAXIDSTRINGSIZE				16

/******************************************************************************/
/* Convert a Tag-Length-Value record to a Biometric Information Template.     */
/* The data values on the leaf nodes of the TLV are not checked for           */
/* validity. However, if the correct amount of TLV child and leaf nodes       */
/* is not present, this function will return an error.                        */
/*                                                                            */
/* Parameters:                                                                */
/*   tlv    Pointer to the input tlv structure.                               */
/*   bit    Pointer to the output bit structure.                              */
/*                                                                            */
/* Returns:                                                                   */
/*        0     Success                                                       */
/*        -1    Failure                                                       */
/******************************************************************************/
int
get_bit_from_tlv(BIT *bit, TLV *tlv);

/* XXX Implement only when needed... */
int
get_tlv_from_bit(TLV *tlv, BIT *bit);

/******************************************************************************/
/* Get the BITs from a BIT group that is represented as a TLV.                */
/*                                                                            */
/* Parameters:                                                                */
/*   bits        Pointer to the an array of BIT structures; each BIT will be  */
/*               allocated by this routine and must be free'd by the caller.  */
/*   bit_group   Pointer to the TLV-format BIT group.                         */
/*   bit_count   Pointer to an integer that will be set to the number of      */
/*               BITs found in the BIT group. Valid values are 1 and 2.       */
/*                                                                            */
/* Returns:                                                                   */
/*        0     Success                                                       */
/*       -1     Failure                                                       */
/******************************************************************************/
int
get_bits_from_group(BIT **bits, TLV *bit_group, int *bit_count);

/******************************************************************************/
/* Print a Biometric Information Template, in human-readable form, to the     */
/* given output file.                                                         */
/*                                                                            */
/* Parameters:                                                                */
/*   fp     Pointer to the output file.                                       */
/*   bit    Pointer to the input bit structure.                               */
/*                                                                            */
/* Returns:                                                                   */
/*   PRINT_OK    Success                                                      */
/*   PRINT_ERROR Failure                                                      */
/******************************************************************************/
int
print_bit(FILE *fp, BIT *bit);

/******************************************************************************/
/* Convert the minutiae contained within a finger view minutiae record (FVMR) */
/* to the hex representation of the MINEX-II minutiae template data object.   */
/* This function is a short-cut around a conversion of an FMR to a TLV, then  */
/* conversion of the TLV to the data object, and is specific to the needs of  */
/* the MINEX-II match-on-card testing framework. The data object will be in   */
/* big-endian format, ready for transmittal to a card.                        */
/*                                                                            */
/* Parameters:                                                                */
/*   fvmr   Pointer to a finger view minutiae record in ISO compact card      */
/*          format.                                                           */
/*   mtdo   The data object (DO), wrapped as a biometric data block.          */
/*                                                                            */
/* Returns:                                                                   */
/*   WRITE_OK    Success                                                      */
/*   WRITE_ERROR Failure, the BDB is not large enough.                        */
/******************************************************************************/
int
fvmr_to_mtdo(FVMR *fvmr, BDB *mtdo);

#endif /* _MOC_H */
