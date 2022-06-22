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
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <biomdimacro.h>
#include <fmr.h>
#include <tlv.h>

static void
print_raw_tlv(TLV *tlv)
{
	printf("tlv_tag_field\t\t\t= 0x%08X\n", tlv->tlv_tag_field);
	printf("tlv_length_field\t\t= %u\n", tlv->tlv_length_field);
	printf("tlv_tagclass\t\t\t= 0x%02X\n", tlv->tlv_tagclass);
	printf("tlv_data_encoding\t\t= 0x%02X\n", tlv->tlv_data_encoding);
	printf("tlv_tagnum\t\t\t= 0x%08X\n", tlv->tlv_tagnum);
	printf("tlv_tag_field_length\t\t= %u\n", tlv->tlv_tag_field_length);
	printf("tlv_length\t\t\t= %u\n", tlv->tlv_length);
	printf("tlv_length_field_length\t\t= %u\n", tlv->tlv_length_field_length);
	printf("------------------------------------\n");
}

int main(int argc, char *argv[])
{
	TLV *grandparent, *parent, *child;
	uint8_t *buf;

	if (new_tlv(&grandparent, 0xAC7F61, 3) != 0)
		ALLOC_ERR_EXIT("Grandarent TLV");
	print_raw_tlv(grandparent);
	grandparent->tlv_length_field_length = 1;

	if (new_tlv(&parent, 0x7F60, 2) != 0)
		ALLOC_ERR_EXIT("Child TLV");
	print_raw_tlv(parent);

	if (new_tlv(&child, 0x81, 1) != 0)
		ALLOC_ERR_EXIT("Child TLV");
	print_raw_tlv(child);

	parent->tlv_length_field_length = 1;

	child->tlv_length_field_length = 1;
	buf = (uint8_t *)malloc(4);
	*(uint32_t *)buf = 0xDEADBEEF;
	add_primitive_to_tlv(buf, child, 4);
	if (add_tlv_to_tlv(child, parent) != 0)
		ERR_EXIT("Failed to add child TLV to parent");

	new_tlv(&child, 0x82, 1);
	child->tlv_length_field_length = 1;
	buf = (uint8_t *)malloc(2);
	*(uint16_t *)buf = 0xFACE;
	add_primitive_to_tlv(buf, child, 2);
	add_tlv_to_tlv(child, parent);
	add_tlv_to_tlv(parent, grandparent);

	print_tlv(stdout, grandparent);

	free_tlv(parent);

	exit (0);
}
