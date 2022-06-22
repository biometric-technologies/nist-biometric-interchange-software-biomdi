/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/
#include <sys/queue.h>
#include <sys/types.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <PCSC/winscard.h>

#include <biomdimacro.h>
#include <fmr.h>
#include <isobit.h>
#include <tlv.h>
#include <nistapdu.h>
#include <cardaccess.h>
#include <mocapdu.h>
#include <moc.h>

#include "cardutils.h"

int
get_bitgroup_from_card(SCARDHANDLE hCard, TLV *bitgroup_tlv)
{
	void *buf;
	BDB cardresponse;
	uint8_t sw1, sw2;

	buf = malloc(RESPONSEBUFSIZE);
	if (buf == NULL)
		ALLOC_ERR_RETURN("BIT group BDB");

	INIT_BDB(&cardresponse, buf, RESPONSEBUFSIZE);
	if (sendAPDU(hCard, &MOCREADBIT, 0, &cardresponse, &sw1, &sw2) != 0)
		ERR_OUT("Could not read BIT group");
	CHECKSTATUS("BIT group read", sw1, sw2);

	REWIND_BDB(&cardresponse);
	if (scan_tlv(&cardresponse, bitgroup_tlv) != READ_OK)
		ERR_OUT("Could not scan BIT group TLV");

	free(buf);
	return (READ_OK);
err_out:
	if (buf != NULL)
		free(buf);
	return (READ_ERROR);
}

int
getIDinresponse(char *id, BDB *response)
{
	TLV *parent, *child;
	int rc;
	int i, j;
	char buf[4];

	rc = -1;

	if (new_tlv(&parent, 0, 0) != 0)
		ALLOC_ERR_OUT("TLV for ID.");
	if (scan_tlv(response, parent) != READ_OK)
		ERR_OUT("Could not read ID TLV.");

	/* The ID is either in the discretionary proprietary TLV directly,
	 * or is in a TLV with a tag set to the requested data object.
	 */
	switch (parent->tlv_tag_field) {
	case PROPRIETARYDATATAG :
		child = TAILQ_FIRST(&parent->tlv_value.tlv_children);
		break;
	case CARDIDDOID :
	case MATCHERIDDOID :
		child = TAILQ_FIRST(&parent->tlv_value.tlv_children);
		if (child == NULL)
			ERR_OUT("ID error: Data object with tag 0x%04X has "
			    " no children", parent->tlv_tag_field);
		if (child->tlv_tag_field != PROPRIETARYDATATAG)
			ERR_OUT("ID: Child in response of requested data "
			    "object is not discretionary: 0x%04X",
			    child->tlv_tag_field);
		child = TAILQ_FIRST(&child->tlv_value.tlv_children);
		break;
	default :
		ERR_OUT("ID TLV top-level tag is incorrect: 0x%04X",
		    parent->tlv_tag_field);
		break;
	}
	if (child == NULL)
		ERR_OUT("ID: Discretionary TLV has no children.");
	if ((child->tlv_tag_field != CARDIDTAG) &&
	    (child->tlv_tag_field != MATCHERIDTAG))
		ERR_OUT("Invalid child ID TLV tag: 0x%04X",
		    child->tlv_tag_field);

	if (child->tlv_length > MAXIDSIZE)
		ERR_OUT("Child TLV has ID length greather than allowed.");

	/* At this point we know the value is a primitive encoding,
	 * because the tag check above is absolute.
	 */
	for (i = 0, j = 0; i < child->tlv_length*2; i+=2, j++) {
		sprintf(buf, "%02hhX", child->tlv_value.tlv_primitive[j]);
		id[i] = buf[0];
		id[i+1] = buf[1];
	}
	id[i] = '\0';
	rc = 0;

err_out:
	if (parent != NULL)
		free_tlv(parent);
	return (rc);
}
