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
#include <sys/types.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <biomdimacro.h>
#include <tlv.h>

/*
 *
 */
int
new_tlv(TLV **tlv, uint32_t tag, uint8_t taglen)
{
	TLV *ltlv;
	uint8_t tag_byte;

	if (taglen > 3)
		return (-1);

	ltlv = (TLV *)malloc(sizeof(TLV));
	if (ltlv == NULL)
		return (-1);
	memset((void *)ltlv, 0, sizeof(TLV));
	TAILQ_INIT(&ltlv->tlv_value.tlv_children);
	/* Shift to get the first (most-significant) byte of the tag */
	tag_byte = tag >> ((taglen - 1) *8);
	ltlv->tlv_tagclass = ((tag_byte & BERTLV_TAG_CLASS_MASK) >>
	    BERTLV_TAG_CLASS_SHIFT);
	ltlv->tlv_data_encoding = ((tag_byte & BERTLV_TAG_DATA_ENCODING_MASK) >>
	    BERTLV_TAG_DATA_ENCODING_SHIFT);
	ltlv->tlv_tag_field = tag;
	ltlv->tlv_tag_field_length = taglen;
	if (taglen < 3) {
		ltlv->tlv_tagnum = tag & BERTLV_MB_TAGNUM_MASK;
	} else {
		/* Need the second byte of the tag */
		tag_byte = (tag >> 8) & BERTLV_MB_TAGNUM_MASK;
		ltlv->tlv_tagnum = tag_byte;
		ltlv->tlv_tagnum = (ltlv->tlv_tagnum << 8) +
			(tag & BERTLV_MB_TAGNUM_MASK);
	}
	*tlv = ltlv;
	return (0);
}

/*
 * Free a TLV by freeing malloc'd storage for a primitive data object,
 * or recursively freeing the children TLVs.
 */
void
free_tlv(TLV *tlv)
{
	TLV *ltlv;

	if (tlv->tlv_data_encoding == BERTLV_TAG_DATA_ENCODING_PRIMITIVE) {
		if (tlv->tlv_value.tlv_primitive != NULL) {
			free(tlv->tlv_value.tlv_primitive);
			tlv->tlv_value.tlv_primitive = NULL;
		}
	} else {
		TAILQ_FOREACH(ltlv, &tlv->tlv_value.tlv_children, tlv_list) {
			free_tlv(ltlv);
		}
	}
}

/*
 *
 */
static int
internal_read_one_tlv(FILE *fp, BDB *bdb, TLV *tlv)
{
	uint8_t cval;
	uint16_t sval;
	uint32_t lval;

	/* Read first byte, determine if single/multi-byte tag */
	CGET(&cval, fp, bdb);
	tlv->tlv_tag_field = cval;
	tlv->tlv_tagclass = (cval & BERTLV_TAG_CLASS_MASK) >> 
		BERTLV_TAG_CLASS_SHIFT;
	tlv->tlv_data_encoding = (cval & BERTLV_TAG_DATA_ENCODING_MASK) >>
		BERTLV_TAG_DATA_ENCODING_SHIFT;

	/* If all bits of the rest of the tag are 1, the tag value is in
	 * subsequent bytes. Otherwise, the tag value is in this byte.
	 */
	tlv->tlv_tag_field_length = 1;
	if ((cval & BERTLV_SB_MB_TAGNUM_MASK) == BERTLV_SB_MB_TAGNUM_MASK) {
		/* ISO 7816-4 says tag value is in next 1 or 2 bytes */
		CGET(&cval, fp, bdb);
		tlv->tlv_tagnum = cval & BERTLV_MB_TAGNUM_MASK;
		tlv->tlv_tag_field = (tlv->tlv_tag_field << 8) + cval;
		tlv->tlv_tag_field_length++;
		if (cval & BERTLV_MB_TAGNUM_TERMINATOR_MASK) {
			CGET(&cval, fp, bdb);
			tlv->tlv_tagnum = (tlv->tlv_tagnum << 8) |
			    (cval & BERTLV_MB_TAGNUM_MASK);
			tlv->tlv_tag_field = (tlv->tlv_tag_field << 8) + cval;
			tlv->tlv_tag_field_length++;
		}	
	} else {
		tlv->tlv_tagnum = cval & BERTLV_MB_TAGNUM_MASK;
	}

	/* Read first length byte, determine if single/multi-byte length */
	CGET(&cval, fp, bdb);
	tlv->tlv_length_field = cval;
	if (cval <= BERTLV_SB_MAX_VALUE) {
		tlv->tlv_length = cval;
		tlv->tlv_length_field = cval;
		tlv->tlv_length_field_length = 1;
	} else {
		switch (cval) {
			case BERTLV_SB_MB_LENGTH_MB_2 :
				CGET(&cval, fp, bdb);
				tlv->tlv_length = cval;
				tlv->tlv_length_field =
				    (tlv->tlv_length_field << 8) + cval;
				tlv->tlv_length_field_length = 2;
				break;
			case BERTLV_SB_MB_LENGTH_MB_3 :
				SGET(&sval, fp, bdb);
				tlv->tlv_length = sval;
				tlv->tlv_length_field =
				    (tlv->tlv_length_field << 16) + sval;
				tlv->tlv_length_field_length = 3;
				break;
			case BERTLV_SB_MB_LENGTH_MB_4 :
				SGET(&sval, fp, bdb);
				tlv->tlv_length = sval;
				tlv->tlv_length_field =
				    (tlv->tlv_length_field << 16) + sval;
				CGET(&cval, fp, bdb);
				tlv->tlv_length = (tlv->tlv_length << 8) + cval;
				tlv->tlv_length_field =
				    (tlv->tlv_length_field << 8) + cval;
				tlv->tlv_length_field_length = 4;
				break;
			case BERTLV_SB_MB_LENGTH_MB_5 :
				LGET(&lval, fp, bdb);
				tlv->tlv_length = lval;
				tlv->tlv_length_field =
				    (tlv->tlv_length_field << 32) + lval;
				tlv->tlv_length_field_length = 5;
				break;
			default :
				return (READ_ERROR);
				break;			// not reached
		}
	}

	if (tlv->tlv_length == 0)
		return (READ_OK);

	/* Read the value field if primitive type; otherwise, it is up
	 * to the caller to call this function again to read the child TLV.
	 */
	if (tlv->tlv_data_encoding == BERTLV_TAG_DATA_ENCODING_PRIMITIVE) {
		tlv->tlv_value.tlv_primitive = malloc(tlv->tlv_length);
		if (tlv->tlv_value.tlv_primitive == NULL)
			ALLOC_ERR_OUT("TLV value field");
		OGET(tlv->tlv_value.tlv_primitive, 1, tlv->tlv_length, fp, bdb);
	}

	return (READ_OK);

err_out:
	return (READ_ERROR);
eof_out:
	return (READ_EOF);
}

/*
 * Evil recursive version...
 */
static int
internal_read_tlv(FILE *fp, BDB *bdb, TLV *tlv)
{
	int ret;
	TLV *child;
	int64_t length;

	ret  = internal_read_one_tlv(fp, bdb, tlv);
	if (ret != READ_OK)
		return (ret);

	length = tlv->tlv_length;
	child = NULL;
	while (length > 0) {
		if (tlv->tlv_data_encoding ==
		    BERTLV_TAG_DATA_ENCODING_CONSTRUCTED) {
			if (new_tlv(&child, 0, 0) != 0)
				ALLOC_ERR_OUT("TLV structure");
			ret  = internal_read_tlv(fp, bdb, child);
			if (ret != READ_OK)
				return (ret);
			length -= (child->tlv_length +
			    child->tlv_tag_field_length +
			    child->tlv_length_field_length);
			TAILQ_INSERT_TAIL(&tlv->tlv_value.tlv_children,
			    child, tlv_list);
		} else {
			break;
		}
	}
	if (length < 0)
		return (READ_ERROR);
	return (READ_OK);

err_out:
	if (child != NULL)
		free_tlv(child);
	return (READ_ERROR);
}

/*
 *
 */
int
read_tlv(FILE *fp, TLV *tlv)
{
	return (internal_read_tlv(fp, NULL, tlv));
}

/*
 *
 */
int
scan_tlv(BDB *bdb, TLV *tlv)
{
	return (internal_read_tlv(NULL, bdb, tlv));
}

/*
 * XXX implement when needed
 */
int
push_tlv(BDB *bdb, TLV *tlv)
{
	return (WRITE_OK);
}

/*
 *
 */
static int
internal_print_tlv(FILE *fp, TLV *tlv, int indent)
{
	TLV *ltlv;
	int i;

	for (i = 0; i < indent; i++)
		FPRINTF(fp, "    ");
	FPRINTF(fp, "TAG 0x%02X, Length %u, ", tlv->tlv_tag_field,
		tlv->tlv_length);
	FPRINTF(fp, "Value ");
	if (tlv->tlv_data_encoding == BERTLV_TAG_DATA_ENCODING_PRIMITIVE) {
		FPRINTF(fp, "0x");
		for (i = 0; i < tlv->tlv_length; i++)
			FPRINTF(fp, "%02X ", tlv->tlv_value.tlv_primitive[i]);
		FPRINTF(fp, "\n");
	} else {
		FPRINTF(fp, "\n");
		TAILQ_FOREACH(ltlv, &tlv->tlv_value.tlv_children, tlv_list)
			internal_print_tlv(fp, ltlv, indent + 1);
	}
	return (PRINT_OK);
err_out:
	return (PRINT_ERROR);

}

/*
 *
 */
int
print_tlv(FILE *fp, TLV *tlv)
{
	return (internal_print_tlv(fp, tlv, 0));
}

/*
 *
 */
static void
fixup_tlv_encoded_length(TLV *tlv)
{
	if (tlv->tlv_length < BERTLV_SB_MAX_VALUE) {
		tlv->tlv_length_field = tlv->tlv_length;
		return;
	}
	if (tlv->tlv_length < BERTLV_MB_2_MAX_VALUE) {
		tlv->tlv_length_field = BERTLV_SB_MB_LENGTH_MB_2;
		tlv->tlv_length_field = (tlv->tlv_length_field << 8) +
		    tlv->tlv_length;
		return;
	}
	if (tlv->tlv_length < BERTLV_MB_3_MAX_VALUE) {
		tlv->tlv_length_field = BERTLV_SB_MB_LENGTH_MB_3;
		tlv->tlv_length_field = (tlv->tlv_length_field << 16) +
		    tlv->tlv_length;
		return;
	}
	if (tlv->tlv_length < BERTLV_MB_4_MAX_VALUE) {
		tlv->tlv_length_field = BERTLV_SB_MB_LENGTH_MB_4;
		tlv->tlv_length_field = (tlv->tlv_length_field << 24) +
		    tlv->tlv_length;
		return;
	}
	if (tlv->tlv_length < BERTLV_MB_5_MAX_VALUE) {
		tlv->tlv_length_field = BERTLV_SB_MB_LENGTH_MB_5;
		tlv->tlv_length_field = (tlv->tlv_length_field << 32) +
		    tlv->tlv_length;
		return;
	}
}

/*
 *
 */
int
add_tlv_to_tlv(TLV *child, TLV *parent)
{
	TAILQ_INSERT_TAIL(&parent->tlv_value.tlv_children,
	    child, tlv_list);
	parent->tlv_length += child->tlv_length +
	    child->tlv_tag_field_length + child->tlv_length_field_length;
	fixup_tlv_encoded_length(parent);

	return (0);
}

/*
 *
 */
int
add_primitive_to_tlv(uint8_t *data, TLV *parent, uint32_t length)
{
	parent->tlv_value.tlv_primitive = data;
	parent->tlv_length += length;
	fixup_tlv_encoded_length(parent);

	return (0);
}
