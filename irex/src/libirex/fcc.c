/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#include <stdint.h>
#include <stdlib.h>

unsigned char f0(const uint8_t *first)
  { return (first[0] & 255) >> 5; }
unsigned char f1(const uint8_t *first)
  { return (first[0] & 127) >> 4; }
unsigned char f2(const uint8_t *first)
  { return (first[0] &  63) >> 3; }
unsigned char f3(const uint8_t *first)
  { return (first[0] &  31) >> 2; }
unsigned char f4(const uint8_t *first)
  { return (first[0] &  15) >> 1; }
unsigned char f5(const uint8_t *first)
  { return (first[0] &   7);      }
unsigned char f6(const uint8_t *first)
  { return ((first[0] &   3) << 1) | ((first[1] & 128) >> 7); }
unsigned char f7(const uint8_t *first)
  { return ((first[0] &   1) << 2) | ((first[1] & 192) >> 6); }

typedef unsigned char(*MASKFUNC)(const uint8_t *);

/*
 * Convert a binary stream of encoded FC codes 010 100 111 to
 * FC values 2 4 7 etc.
 */
uint8_t *
fcc_binary_decode(const uint8_t *enc, const unsigned int nelem)
{
	unsigned char *dec;
	int i;
	MASKFUNC grab[8] = { &f0, &f1, &f2, &f3, &f4, &f5, &f6, &f7 };

	dec = (unsigned char *)calloc(nelem, sizeof(unsigned char));
	if (dec == NULL)
		return (NULL);

	for (i = 0 ; i < nelem ; i++ ) {
		unsigned int insertpos_bits = i + i + i;
		unsigned int insertpos_bit_within_byte = insertpos_bits % 8;
		unsigned int three_byte_index = insertpos_bits / 24;

		/* integer arithmetic, floor */
		unsigned int insertpos_byte_index = (i % 8) / 3;
		unsigned int k = 3 * three_byte_index + insertpos_byte_index;

		MASKFUNC f = grab[insertpos_bit_within_byte];
		dec[i] = (*f)(&enc[k]);
	}
	return (dec);
}

