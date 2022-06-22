/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

#ifndef _NISTAPDU_H
#define _NISTAPDU_H
#include <stdint.h>

/* Bit masks to indicate what optional fields to include */
#define	APDU_FIELD_LC	0x00000001	/* Implies Nc present as well */
#define	APDU_FIELD_LE	0x00000002

/* Define the length of the APDU fields */
#define APDU_FLEN_CLA		1
#define APDU_FLEN_INS		1
#define APDU_FLEN_P1		1
#define APDU_FLEN_P2		1
#define APDU_FLEN_LC_SHORT	1
#define APDU_FLEN_LC_EXTENDED	3
#define APDU_FLEN_LE_SHORT	1
#define APDU_FLEN_LE_EXTENDED	3
#define APDU_FLEN_TRAILER	2

#define APDU_FLAG_CLA_NOCHAIN	0x00
#define APDU_FLAG_CLA_CHAIN	0x10

/*
 * The max size of any command data is determined by the max size of the
 * Le field, and that is 0 (absent), 1, or 3 bytes. In the 3-byte case,
 * the first byte is 0x00, and the next two are 0x0001-0xFFFF.
 */
#define APDU_MAX_NC_SIZE	0xFFFF

#define APDU_MAX_SHORT_LC	255
#define APDU_MAX_SHORT_LE	255
#define APDU_HEADER_LEN		(APDU_FLEN_CLA + APDU_FLEN_INS + APDU_FLEN_P1 + APDU_FLEN_P2)

/*
 * Representation of a ISO/IEC 7816-4 APDU. These fields are optional:
 *     Lc - Length of the command data field
 *     Nc - Command data
 *     Le - Length of the expected response
 */
struct apdu {
	/* Data that makes up the actual APDU fields */
	uint8_t		apdu_cla;
	uint8_t		apdu_ins;
	uint8_t		apdu_p1;
	uint8_t		apdu_p2;
	uint16_t	apdu_lc;
	uint8_t		apdu_nc[APDU_MAX_NC_SIZE];
	uint16_t	apdu_le;
	uint8_t		apdu_field_mask;	/* Mask of optional fields */
						/* Use defines from above */
	char		*apdu_descr;		/* Text description */
};
typedef struct apdu APDU;

/*
 * Define some specified tags.
 */
#define DISCRETIONARYDATATAG		0x53
#define PROPRIETARYDATATAG		0x73

/*
 * Define some response codes for SW1.
 */
#define APDU_NORMAL_COMPLETE		0x90
#define APDU_NORMAL_CHAINING		0x61
#define APDU_WARN_NVM_UNCHANGED		0x62
#define APDU_WARN_NVM_CHANGED		0x63
#define APDU_EXEC_ERR_NVM_UNCHANGED	0x64
#define APDU_EXEC_ERR_NVM_CHANGED	0x65
#define APDU_EXEC_ERR_SECURITY		0x66
#define APDU_CHECK_ERR_WRONG_LENGTH	0x67
#define APDU_CHECK_ERR_CLA_FUNCTION	0x68
#define APDU_CHECK_ERR_CMD_NOT_ALLOWED	0x69
#define APDU_CHECK_ERR_WRONG_PARAM_QUAL	0x6A
#define APDU_CHECK_ERR_WRONG_PARAM	0x6B
#define APDU_CHECK_ERR_WRONG_LE		0x6C
#define APDU_CHECK_ERR_INVALID_INS	0x6D
#define APDU_CHECK_ERR_CLA_UNSUPPORTED	0x6E
#define APDU_CHECK_ERR_NO_DIAGNOSIS	0x6F

/*
 * Mask for SW2 retry counter.
 */
#define RETRY_COUNTER_MASK		0x0F
#define RETRY_COUNTER_INDICATOR		0xC0
#define RETRY_COUNTER_INDICATOR_MASK	0xF0
#define RETRY_COUNTER_MAX		15

/******************************************************************************/
/* A macro to pull off the SW1 and SW2 values contained in the response       */
/* from a card after an APDU has been sent.                                   */
/******************************************************************************/
#define UNDEFINEDSWCODE	0xFF
#define GETAPDUSTATUS(bdbptr, sw1, sw2)					\
do {									\
	int len = (bdbptr)->bdb_current - (bdbptr)->bdb_start;		\
	if (len <= 0)							\
		sw1 = sw2 = UNDEFINEDSWCODE;				\
	else if (len == 1) {						\
		sw1 = ((uint8_t *)(bdbptr)->bdb_start)[0];		\
		sw2 = UNDEFINEDSWCODE;					\
	} else {							\
		sw1 = ((uint8_t *)(bdbptr)->bdb_start)[len - 2];	\
		sw2 = ((uint8_t *)(bdbptr)->bdb_start)[len - 1];	\
	}								\
} while (0)

/******************************************************************************/
/* Check the return status for an APDU.                                       */
/******************************************************************************/
#define CHECKSTATUS(str, sw1, sw2)					\
do {									\
	if (sw1 != APDU_NORMAL_COMPLETE)				\
		ERR_OUT("Bad status for %s: 0x%02X%02X", str, sw1, sw2);\
} while (0)

/******************************************************************************/
/* Dump the contents of an APDU to stdout.                                    */
/******************************************************************************/
#define DUMPAPDU(apdu)							\
do {									\
	printf("[%s] \n", apdu.apdu_descr);				\
	printf("CLA=%02hhX, INS=%02hhX, P1=%02hhX, P2=%02hhX, Lc=%u(dec)\n",\
	    apdu.apdu_cla, apdu.apdu_ins, apdu.apdu_p1, apdu.apdu_p2,	\
	    apdu.apdu_lc);						\
	int idx;							\
	printf("Nc:\n");						\
	for (idx = 0; idx < apdu.apdu_lc; idx++) {			\
		printf("%02hhX ", apdu.apdu_nc[idx]);			\
		if (((idx+1) % 16) == 0)				\
			printf("\n");					\
	}								\
	printf("\n");							\
	fflush(stdout);							\
} while (0)

/******************************************************************************/
/* Check the return status for an APDU. If not clean, check the non-volatile  */
/* memory status, for that is allowed to change. Then check the retry         */
/* counter and call for an exit when the counter is dangerously low.          */
/* For some APDUs, a retry counter value of 0 means disabled, so conditionally*/
/* check for that. The presence of the retry counter is indicated in the      */
/* high-order 4 bits of SW2.                                                  */
/* The 'nfaction' parameter is code to execute when a non-fatal status        */
/* encountered.                                                               */
/******************************************************************************/
#define CHECKSTATUSWITHRETRY(str, sw1, sw2, zeroallowed, fp, nfaction)	\
do {									\
/* printf("%s SW1-SW2: 0x%02X-%02X\n", str, sw1, sw2); */		\
	if (sw1 != APDU_NORMAL_COMPLETE) {				\
		if (sw1 == APDU_WARN_NVM_CHANGED) { 			\
			if ((sw2 & RETRY_COUNTER_INDICATOR_MASK) ==	\
			    RETRY_COUNTER_INDICATOR) {			\
				uint8_t __retryval = sw2 & RETRY_COUNTER_MASK;\
				if (__retryval == 0) { 			\
				    if (!zeroallowed) {			\
					FPRINTF(fp,			\
					    "\n# Retry counter for %s is 0.\n",\
					    str);\
					ERR_OUT("Retry counter for %s is 0",\
					    str);			\
				    }					\
				} else {				\
				    if (__retryval < MOC_RETRY_MIN) {\
					FPRINTF(fp,			\
					    " # ERROR: Retry counter for %s is %d.\n",\
					     str, __retryval);		\
					ERR_OUT("Retry counter for %s is %d",\
					    str, __retryval);		\
				    }					\
				}					\
			} else {					\
				;					\
			/* 0x63 is a warning, so we take no action */	\
			/* when the retry counter flag is not set */	\
			}						\
		} else {						\
		    FPRINTF(fp, " # ERROR: Bad status for %s: 0x%02X%02X.\n",	\
			str, sw1, sw2);					\
		    nfaction;						\
		}							\
	} 								\
} while (0)

#endif /* _NISTAPDU_H */
