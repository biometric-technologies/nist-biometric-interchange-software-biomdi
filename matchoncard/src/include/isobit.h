/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#ifndef _ISOBIT_H
#define _ISOBIT_H

#define BERTLVTAG_BITGROUP		0x7F61
#define BERTLVTAG_BIT			0x7F60
#define BERTLVTAG_BHT			0xA1
#define BERTLVTAG_ALGOPARAM		0xB1

#define SIMPLETLVTAG_NUMBITS		0x02
#define SIMPLETLVTAG_BIOMETRIC_TYPE	0x81
#define SIMPLETLVTAG_BIOMETRIC_SUBTYPE	0x82
#define SIMPLETLVTAG_FORMATOWNER	0x87
#define SIMPLETLVTAG_FORMATTYPE		0x88
#define SIMPLETLVTAG_MINMAXMINUTIAE	0x81
#define SIMPLETLVTAG_MINUTIAEORDER	0x82
#define SIMPLETLVTAG_FEATUREHANDLING	0x83

#define BIOMETRIC_TYPE_FINGERPRINT	0x08

#define FEATURE_HANDLING_NONE		0x00

#define MIN_MINUTIAE_MASK		0xFF00
#define MIN_MINUTIAE_SHIFT		16
#define MAX_MINUTIAE_MASK		0x00FF

/*
 * The values of the minutia order setting, as given in ISO/IEC 19794-4.
 */
#define MINUTIA_ORDER_NONE		0x00
#define MINUTIA_ORDER_ASCENDING		0x01
#define MINUTIA_ORDER_DESCENDING	0x02
#define MINUTIA_ORDER_DIRECTION_MASK	0x03
#define MINUTIA_ORDER_XY		0x04
#define MINUTIA_ORDER_YX		0x08
#define MINUTIA_ORDER_ANGLE		0x0C
#define MINUTIA_ORDER_POLAR		0x10
#define MINUTIA_ORDER_METHOD_MASK	0x1C

struct biometric_info_template {
	uint8_t		bit_biometric_type;
	uint8_t		bit_biometric_subtype;
	uint16_t	bit_format_owner;
	uint16_t	bit_format_type;
	uint8_t		bit_minutia_min;
	uint8_t		bit_minutia_max;
	uint8_t		bit_minutia_order;
	uint8_t		bit_feature_handling;
	int		bit_biometric_type_present;
	int		bit_biometric_subtype_present;
	int		bit_feature_handling_present;
};
typedef struct biometric_info_template BIT;

/******************************************************************************/
/* Read a Biometric Information Template from a file, or buffer, filling in   */
/* the fields of the header record, including all of the Finger Views.        */
/* This function does not do any validation of the data being read.           */
/* Fields within the FILE and BDB structs are modified by these functions.    */
/*                                                                            */
/* Parameters:                                                                */
/*   fp     The open file pointer.                                            */
/*   bdb    Pointer to the biometric data block containing the BIT.           */
/*   fmr    Pointer to the BIT.                                               */
/*                                                                            */
/* Returns:                                                                   */
/*        READ_OK     Success                                                 */
/*        READ_EOF    End of file encountered                                 */
/*        READ_ERROR  Failure                                                 */
/******************************************************************************/
int
read_bit(FILE *fp, BIT *bit);

int
scan_bit(BDB *bdb, BIT *bit);

int
write_bit(FILE *fp, BIT *bit);

/*
 * Create a complete BER-TLV Biometric Information Template from the
 * internal representation.
 */
int
push_bit(BDB *bdb, BIT *bit);

#endif /* _ISOBIT_H */
