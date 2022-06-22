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
/*                                                                            */
/* This file contains the definitions of various constants, types, and        */
/* functions used to process Tag-Length-Value (TLV) records as used by        */
/* the ISO/IEC 7816-4 Identification Card standard.                           */
/*                                                                            */
/******************************************************************************/

/*
 * Definitions used to process Tag-Length_Value (TLV) objects, both
 * Simple-TLV and BER-TLV. See ISO/IEC 7816-4 (2005).
 */
#ifndef _TLV_H
#define _TLV_H

#define BERTLV_TAG_CLASS_MASK			0xC0
#define BERTLV_TAG_CLASS_SHIFT			6
/* Representations of the tag class after shifting out of the tag field */
#define BERTLV_TAG_CLASS_UNIVERSAL		0x00
#define BERTLV_TAG_CLASS_APPLICATION		0x01
#define BERTLV_TAG_CLASS_CONTEXT_SPECIFIC	0x02
#define BERTLV_TAG_CLASS_PRIVATE		0x03

#define BERTLV_TAG_DATA_ENCODING_MASK		0x20
#define BERTLV_TAG_DATA_ENCODING_SHIFT		5
/* Representations of the data encoding after shifting out of the tag field */
#define BERTLV_TAG_DATA_ENCODING_PRIMITIVE	0x00
#define BERTLV_TAG_DATA_ENCODING_CONSTRUCTED	0x01

/* Mask for single/muti-byte tag number */
#define BERTLV_SB_MB_TAGNUM_MASK		0x1F

/* Masks for multi-byte tag numbers */
#define BERTLV_MB_TAGNUM_INDICATOR		0x1F
#define BERTLV_MB_TAGNUM_TERMINATOR_MASK	0x80
#define BERTLV_MB_TAGNUM_MASK			0x7F

/* Indicators for single/multi-byte indicator in first byte of length field */
#define BERTLV_SB_MAX_VALUE			0x7F
#define BERTLV_MB_2_MAX_VALUE			0xFF
#define BERTLV_MB_3_MAX_VALUE			0xFFFF
#define BERTLV_MB_4_MAX_VALUE			0xFFFFFF
#define BERTLV_MB_5_MAX_VALUE			0xFFFFFFFF
#define BERTLV_SB_MB_LENGTH_MB_2		0x81
#define BERTLV_SB_MB_LENGTH_MB_3		0x82
#define BERTLV_SB_MB_LENGTH_MB_4		0x83
#define BERTLV_SB_MB_LENGTH_MB_5		0x84
		
/*
 * A TLV is defined as a set of TLVs. This structure represents the
 * TLV fields after they have been decoded from the TLV stream. So, for
 * example, the tlv_tag field is the actual tag value, not the encoded
 * representation.
 */
struct tag_length_value {
	/* The encoded versions of the TLV fields */
	uint32_t				tlv_tag_field;
	uint64_t				tlv_length_field;

	/* The decoded information, from the above fields */
	uint8_t					tlv_tagclass;
	uint8_t					tlv_data_encoding;
	uint32_t				tlv_tagnum;
	/* Length of the tag field */
	uint8_t					tlv_tag_field_length;
	uint32_t				tlv_length;
	/* Length of the length field */
	uint8_t					tlv_length_field_length;
	union {
		uint8_t *			tlv_primitive;
		TAILQ_HEAD(, tag_length_value)	tlv_children;
	} tlv_value;
	TAILQ_ENTRY(tag_length_value)		tlv_list;
};
typedef struct tag_length_value TLV;

/******************************************************************************/
/* Allocate and initialize storage for a new Tag-Length-Value record.         */
/* The tag field and tag field length fields will be initialized, and the     */
/* tag number, tag class and data encoding fields will be set based on the    */
/* tag value.  All other fields will be set to 'NULL' values, and the list    */
/* of children TLVs will be initialized to empty.                             */
/*                                                                            */
/* Parameters:                                                                */
/*   tlv        Address of the pointer to the TLV that will be allocated.     */
/*   tag        The value of the Tag field.                                   */
/*   taglen     The length of the Tag field.                                  */
/*                                                                            */
/* Returns:                                                                   */
/*   0      Success                                                           */
/*  -1      Failure                                                           */
/*                                                                            */
/******************************************************************************/
int
new_tlv(TLV **tlv, uint32_t tag, uint8_t taglen);

/******************************************************************************/
/* Free the storage for a Tag-Length_Value record.                            */
/* This function does a "deep free", meaning that all storage allocated to    */
/* records on lists associated with this TLV are free'd.                      */
/*                                                                            */
/* Parameters:                                                                */
/*   tlv    Pointer to the TLV structure that will be free'd.                 */
/*                                                                            */
/******************************************************************************/
void
free_tlv(TLV *tlv);

/******************************************************************************/
/* Read a Tag-Length-Value object from a file, or buffer, creating the        */
/* internal representation of the TLV.                                        */
/* This function does not do any validation of the data being read.           */
/* Fields within the FILE and BDB structs are modified by these functions.    */
/*                                                                            */
/* Parameters:                                                                */
/*   fp     The open file pointer.                                            */
/*   bdb    Pointer to the biometric data block containing the raw TLV.       */
/*   tlv    Pointer to the resultant TLV.                                     */
/*                                                                            */
/* Returns:                                                                   */
/*        READ_OK     Success                                                 */
/*        READ_EOF    End of file encountered                                 */
/*        READ_ERROR  Failure                                                 */
/******************************************************************************/
int
read_tlv(FILE *fp, TLV *tlv);

int
scan_tlv(BDB *bdb, TLV *tlv);

/******************************************************************************/
/* Write a Tag-Length-Value object to a file or buffer from the internal      */
/* representation of the TLV.                                                 */
/* Fields within the FILE and BDB structs are modified by these functions.    */
/*                                                                            */
/* Parameters:                                                                */
/*   fp     The open file pointer.                                            */
/*   bdb    Pointer to the biometric data block containing the raw TLV.       */
/*   tlv    Pointer to the resultant TLV.                                     */
/*                                                                            */
/* Returns:                                                                   */
/*        WRITE_OK     Success                                                */
/*        WRITE_EOF    End of file encountered                                */
/*        WRITE_ERROR  Failure                                                */
/******************************************************************************/
int
write_tlv(FILE *fp, TLV *tlv);

int
push_tlv(BDB *bdb, TLV *tlv);

/******************************************************************************/
/* Add a TLV, or primitive data, to another TLV, fixing up the parent's       */
/* length field as appropriate.  Note that the size of the encoded length     */
/* field  can be increased, from single to multiple byte, for example.        */
/*                                                                            */
/* Parameters:                                                                */
/*   child  Pointer to the child TLV.                                         */
/*   parent Pointer to the parent TLV.                                        */
/*   data   Pointer to the primitive data.                                    */
/*   length Length of the primitive data.                                     */
/*                                                                            */
/* Returns:                                                                   */
/*        0            Success                                                */
/*       -1            Child TLV is invalid                                   */
/******************************************************************************/
int
add_tlv_to_tlv(TLV *child, TLV *parent);

int
add_primitive_to_tlv(uint8_t *data, TLV *parent, uint32_t length);

// XXX this function may never be implemented
int
print_tlv(FILE *fp, TLV *tlv);

#endif /* _TLV_H */
