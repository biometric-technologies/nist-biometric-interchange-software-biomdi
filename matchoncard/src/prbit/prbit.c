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
#include <isobit.h>
#include <fmr.h>
#include <tlv.h>
#include <moc.h>

/*
 * This program reads a Biometric Information Template (BIT) group
 * stored as a TLV structure in the file who name is passed is as an argument.
 * The structure of the BIT group is defined in the MINEX II test document.
 */ 
static void
usage()
{
	fprintf(stderr, "Usage: prbit <filename>\n");
	exit (EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	FILE *fp;
	TLV *parent;
	TLV *child;
	BDB *bdb;
	BIT *bit;
	void *buf;
	struct stat sb;
	uint8_t count;
	int i;

	if (argc != 2)
		usage();

	fp = fopen(argv[optind], "rb");
	if (fp == NULL)
		ERR_EXIT("open of %s failed: %s", argv[optind],
		    strerror(errno));
	if (fstat(fileno(fp), &sb) < 0)
		ERR_EXIT("Could not get stats on input file");

	if (new_tlv(&parent, 0, 0) != 0)
		ALLOC_ERR_EXIT("TLV structure");

	bdb = (BDB *)malloc(sizeof(BDB));
	if (bdb == NULL)
		ALLOC_ERR_EXIT("BDB structure");
	buf = malloc(sb.st_size);
	if (buf == NULL)
		ALLOC_ERR_EXIT("BDB structure");
	if (fread(buf, 1, sb.st_size, fp) != sb.st_size)
		ERR_EXIT("Could not read input file");
	INIT_BDB(bdb, buf, sb.st_size);

	if (scan_tlv(bdb, parent) != READ_OK) {
		printf("Partial read of TLV:\n");
		print_tlv(stdout, parent);
		ERR_EXIT("Could not scan TLV");
	}
	print_tlv(stdout, parent);

	bit = (BIT *)malloc(sizeof(BIT));
	if (bit == NULL)
		ALLOC_ERR_EXIT("BIT structure");

	/* The first child TLV in the group is the BIT count */
	child = TAILQ_FIRST(&parent->tlv_value.tlv_children);
	if (child == NULL)
		ERR_EXIT("Could not get first TLV in group");
	count = *child->tlv_value.tlv_primitive;
	printf("-------------------------------\n");
	printf("%u BITs in the group:\n", count);

	for (i = 1; i <= count; i++) {
		child = TAILQ_NEXT(child, tlv_list);
		if (child == NULL)
			ERR_EXIT("Could not get TLV %d in group", i);
		if (get_bit_from_tlv(bit, child) != 0)
			ERR_EXIT("Could not convert BIT %d from TLV", i);
		printf("BIT %d:\n", i);
		print_bit(stdout, bit);
	}

	free_tlv(parent);
	exit (0);
}
