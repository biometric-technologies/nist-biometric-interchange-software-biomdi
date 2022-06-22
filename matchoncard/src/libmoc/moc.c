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
#include <fmr.h>
#include <isobit.h>
#include <nistapdu.h>
#include <tlv.h>
#include <moc.h>

#define FINDDO(__tgt, __src, __tag, __found)				\
do {									\
	__found = FALSE;						\
	TAILQ_FOREACH(__tgt, &__src->tlv_value.tlv_children, tlv_list)	\
		if (__tgt->tlv_tag_field == __tag) {			\
			__found = TRUE;					\
			break;						\
		}							\
} while (0)

int
get_bit_from_tlv(BIT *bit, TLV *tlv)
{
	/* TLV objects for the Biometric Header Template, the
	 * Biometric Matching Algorithm Parameters, and children
	 * of either.
	 */
	TLV *bht, *bmap, *child;

	BDB bdb;
	int found;

	/*
	 * The BIT structure:
	 *
	 * TAG 0x7F60, Length 28, Value 
	 *     TAG 0xA1, Length 26, Value		(The BHT)
	 *         TAG 0x81, Length 1, Value ...	(biometric type)
	 *         TAG 0x82, Length 1, Value ...	(biometric subtype)
	 *         TAG 0x87, Length 2, Value ...	(format owner)
	 *         TAG 0x88, Length 2, Value ...	(format type)
	 *         TAG 0xB1, Length 10, Value 
	 *             TAG 0x81, Length 2, Value ...	(min/max minutiae)
	 *             TAG 0x82, Length 1, Value ...	(minutiae order)
	 *             TAG 0x83, Length 1, Value ...	(feature handling)
	 */
	/* Feature handling is optional, and the objects need not be in the
	 * order given above. Other objects in the TLV are ignored.
	 */

	/* The BHT is always a child of the BIT. */
	FINDDO(bht, tlv, BERTLVTAG_BHT, found);		
	if (found == FALSE)
		return (-1);

	/*
	 * Find the required data objects.
	 */
	FINDDO(child, bht, SIMPLETLVTAG_BIOMETRIC_TYPE, found);
	if (found == FALSE) {
		bit->bit_biometric_type_present = FALSE;
	} else {
		bit->bit_biometric_type =
		    *(uint8_t *)child->tlv_value.tlv_primitive;
		bit->bit_biometric_type_present = TRUE;
	}

	FINDDO(child, bht, SIMPLETLVTAG_BIOMETRIC_SUBTYPE, found);
	if (found == FALSE) {
		bit->bit_biometric_subtype_present = FALSE;
	} else {
		bit->bit_biometric_subtype =
		    *(uint8_t *)child->tlv_value.tlv_primitive;
		bit->bit_biometric_subtype_present = TRUE;
	}

	FINDDO(child, bht, SIMPLETLVTAG_FORMATOWNER, found);
	if (found == FALSE)
		return (-1);
	INIT_BDB(&bdb, child->tlv_value.tlv_primitive, 2);
	SSCAN(&bit->bit_format_owner, &bdb);

	FINDDO(child, bht, SIMPLETLVTAG_FORMATTYPE, found);
	if (found == FALSE)
		return (-1);
	INIT_BDB(&bdb, child->tlv_value.tlv_primitive, 2);
	SSCAN(&bit->bit_format_type, &bdb);

	/* Find the Biometric Matching Algorithm Parameters object inside
	 * the BHT, then get its children.
	 */
	FINDDO(bmap, bht, BERTLVTAG_ALGOPARAM, found);
	if (found == FALSE)
		return (-1);
	FINDDO(child, bmap, SIMPLETLVTAG_MINMAXMINUTIAE, found);
	if (found == FALSE)
		return (-1);
	bit->bit_minutia_min = *(uint8_t *)child->tlv_value.tlv_primitive;
	bit->bit_minutia_max = *(uint8_t *)(child->tlv_value.tlv_primitive +1);

	FINDDO(child, bmap, SIMPLETLVTAG_MINUTIAEORDER, found);
	if (found == FALSE)
		return (-1);
	bit->bit_minutia_order = *(uint8_t *)child->tlv_value.tlv_primitive;

	/*
	 * Optional fields.
	 */
	child = TAILQ_NEXT(child, tlv_list);
	FINDDO(child, bmap, SIMPLETLVTAG_FEATUREHANDLING, found);
	if (found == TRUE) {
		bit->bit_feature_handling =
		    *(uint8_t *)child->tlv_value.tlv_primitive;
		bit->bit_feature_handling_present = TRUE;
	} else {
		bit->bit_feature_handling_present = FALSE;
	}

	return (0);
eof_out:
	return (-1);
}

int
get_tlv_from_bit(TLV *tlv, BIT *bit)
{
	return (0);
}

int
get_bits_from_tlv(BIT **bits, TLV *bit_group, int *bit_count)
{
	int i;
	uint8_t count;
	TLV *child;

	/* The first child TLV in the group is the BIT count */
	child = TAILQ_FIRST(&bit_group->tlv_value.tlv_children);
	if (child == NULL)
		ERR_OUT("Could not get first TLV in group");
	count = *child->tlv_value.tlv_primitive;

	if ((count != 1) && (count != 2))
		ERR_OUT("Invalid number of BITs");
	for (i = 0; i < count; i++) {
		bits[i] = (BIT *)malloc(sizeof(BIT));
		if (bits[i] == NULL)
			ALLOC_ERR_RETURN("BIT structure");

		child = TAILQ_NEXT(child, tlv_list);
		if (child == NULL)
			ERR_OUT("Could not get TLV %d in group", i+1);
		if (get_bit_from_tlv(bits[i], child) != 0)
			ERR_OUT("Could not convert BIT %d from TLV", i+1);
	}

	*bit_count = count;
	return (READ_OK);
err_out:
	return (READ_ERROR);
}


int
print_bit(FILE *fp, BIT *bit)
{
	FPRINTF(fp, "\tBiometric Type: ");
	if (bit->bit_biometric_type_present == TRUE)
		FPRINTF(fp, "%d\n", bit->bit_biometric_type);
	else
		FPRINTF(fp, "Not present\n");
	FPRINTF(fp, "\tBiometric Subtype: ");
	if (bit->bit_biometric_subtype_present == TRUE)
		FPRINTF(fp, "%d\n", bit->bit_biometric_subtype);
	else
		FPRINTF(fp, "Not present\n");
	FPRINTF(fp, "\tCBEFF: 0x%04X:%04X\n", bit->bit_format_owner,
	    bit->bit_format_type);
	FPRINTF(fp, "\tMinutiae min/max: %u:%u\n", bit->bit_minutia_min,
	    bit->bit_minutia_max);
	FPRINTF(fp, "\tMinutiae Order: 0x%02X\n", bit->bit_minutia_order);
	FPRINTF(fp, "\tFeature Handling: ");
	if (bit->bit_feature_handling_present == TRUE)
		FPRINTF(fp, "0x%02X\n", bit->bit_feature_handling);
	else
		FPRINTF(fp, "Not present\n");
	return (PRINT_OK);

err_out:
	return (PRINT_ERROR);
}

int
fvmr_to_mtdo(FVMR *fvmr, BDB *mtdo)
{
	uint8_t val8;
	uint16_t val16;
	uint32_t val32;
	int l2size, msize;
	FMD *fmd;

	if (fvmr->format_std != FMR_STD_ISO_COMPACT_CARD)
		return (WRITE_ERROR);

	msize = get_fmd_count(fvmr) * 3;
	if (msize <= BERTLV_SB_MAX_VALUE)
		l2size = 1;
	else if (msize <= BERTLV_MB_2_MAX_VALUE)
		l2size = 2;
	else if (msize <= BERTLV_MB_3_MAX_VALUE)
		l2size = 3;

	SPUSH(MTDOTAG_BIOMETRIC_DATA_TEMPLATE, mtdo);

	/* Calculate L1: Size data tag + size of L2 + 3 * minutiae-count */
	val32 = MTDOTAGSIZE_FINGER_MINUTIAE_DATA + l2size + msize;
	if (val32 <= BERTLV_SB_MAX_VALUE) {
		val8 = (uint8_t)val32;
		CPUSH(val8, mtdo);
	} else if (val32 <= BERTLV_MB_2_MAX_VALUE) {
		CPUSH(BERTLV_SB_MB_LENGTH_MB_2, mtdo);
		val8 = (uint8_t)val32;
		CPUSH(val8, mtdo);
	} else if (val32 <= BERTLV_MB_3_MAX_VALUE) {
		CPUSH(BERTLV_SB_MB_LENGTH_MB_3, mtdo);
		val16 = (uint16_t)val32;
		SPUSH(val16, mtdo);
	} else {
		return (WRITE_ERROR);  /* We don't support larger lengths yet */
	}

	CPUSH(MTDOTAG_FINGER_MINUTIAE_DATA, mtdo);

	/* L2 is 3 * minutiae count, already calcuated */
	if (msize <= BERTLV_SB_MAX_VALUE) {
		val8 = (uint8_t)msize;
		CPUSH(val8, mtdo);
	} else if (msize < BERTLV_MB_2_MAX_VALUE) {
		CPUSH(BERTLV_SB_MB_LENGTH_MB_2, mtdo);
		val8 = (uint8_t)msize;
		CPUSH(val8, mtdo);
	} else if (msize < BERTLV_MB_3_MAX_VALUE) {
		CPUSH(BERTLV_SB_MB_LENGTH_MB_3, mtdo);
		val16 = (uint16_t)msize;
		SPUSH(val16, mtdo);
	} else {
		return (WRITE_ERROR);  /* We don't support larger lengths yet */
	}

	/* Push the minutiae data records, purposly not using
	  * libfmr::push_fmd() to save some time.
	  */
	TAILQ_FOREACH(fmd, &fvmr->minutiae_data, list) {
		val8 = (uint8_t)(fmd->x_coord);
		CPUSH(val8, mtdo);
		val8 = (uint8_t)(fmd->y_coord);
		CPUSH(val8, mtdo);
		val8 = fmd->type << FMD_ISO_COMPACT_MINUTIA_TYPE_SHIFT;
		val8 = val8 | (fmd->angle & FMD_ISO_COMPACT_MINUTIA_ANGLE_MASK);
		CPUSH(val8, mtdo);
	}

	return (WRITE_OK);
err_out:
	return (WRITE_ERROR);
}
