/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

/*
 * This application will probe a PIV card, retrieving many of the data
 * objects on the first PIV card found in a reader.
 */

/* Needed by the GNU C libraries for Posix and other extensions */
#define _XOPEN_SOURCE	1

#include <sys/queue.h> 
#include <signal.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h> 
#include <unistd.h>
#include <PCSC/wintypes.h>
#include <nistapdu.h>
#include <biomdimacro.h>
#include <cardaccess.h>
#include <piv.h>
#include <tlv.h>
#include <pivcard.h>
#include <pivdata.h>

static void
usage()
{
	fprintf(stderr, "Usage: pivprobe\n");
	exit (EXIT_FAILURE);
}

/*
 * getPin() is adapted from the comp.unix.programmer FAQ, which
 * adapted from Stevens' Advanced Programming In The Unix Environment.
 */
static int
getPin(char *pin)
{ 
	int i, j;
	sigset_t sig, sigsave; 
	struct termios term, termsave; 
	FILE *fp; 
	int c; 
	if((fp=fopen(ctermid(NULL),"r+")) == NULL) 
		return (-1);
	setbuf(fp, NULL); 
	sigemptyset(&sig);    /* block SIGINT & SIGTSTP, save signal mask */ 
	sigaddset(&sig, SIGINT); 
	sigaddset(&sig, SIGTSTP); 
	sigprocmask(SIG_BLOCK, &sig, &sigsave); 
	tcgetattr(fileno(fp), &termsave); 
	term = termsave; 
	term.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL); 
	tcsetattr(fileno(fp), TCSAFLUSH, &term); 
	fputs("PIN: ", fp); 
	for (i = 0; i < PIV_PIN_LENGTH; i++) {
		c = getc(fp);
		if ((c != EOF) && (c != '\n'))
			pin[i] = c;
		else
			break;
	} 
	tcsetattr(fileno(fp), TCSAFLUSH, &termsave); 
	sigprocmask(SIG_SETMASK, &sigsave, NULL); 
	fclose(fp); 

	/* Fill out the remainder of the PIN with the required values */
	for (; i < PIV_PIN_LENGTH; i++)
		pin[i] = 0xff;
	return (0); 
}

int
main(int argc, char *argv[])
{
	PIVCARD card;
	LONG ret;
	FILE *fp;
	BDB cardresponse;
	uint8_t *cardbuf;
	uint8_t *databuf;
	unsigned int databufsz, cardbufsz;
	unsigned int mcount;
	uint8_t sw1, sw2;
	int exitcode;
	uint8_t pin[PIV_PIN_LENGTH];
	char ch;
	struct piv_fmd *pfmd;
	int i, m;

	if (argc != 1)
		usage();

	exitcode = EXIT_FAILURE;	/* always the pessimist */
	if (pivCardConnect(&card) != 0)
		ERR_EXIT("Could not attach to PIV card");

	cardbuf = malloc(PIV_MAX_OBJECT_SIZE);
	if (cardbuf == NULL)
		ALLOC_ERR_EXIT("Response BDB buffer");
	INIT_BDB(&cardresponse, cardbuf, PIV_MAX_OBJECT_SIZE);
	
	databuf = malloc(PIV_MAX_OBJECT_SIZE);
	if (databuf == NULL)
		ALLOC_ERR_EXIT("Data buffer");

	/*
	 * Get the mandatory data objects that are always readable,
	 * without a PIN
	 */
	pivCardSaveContainer(card, PIVCCCTAG_DO, "ccc.raw");
	pivCardSaveContainer(card, PIVCHUIDTAG_DO, "chuid.raw");
	pivCardSaveContainer(card, PIVPIVAUTHCERTTAG_DO, "pivauthcert.raw");
	pivCardSaveContainer(card, PIVSECURITYOBJECTTAG_DO, "securityobj.raw");

	/*
	 * Get the optional data objects that are always readable,
	 * without a PIN. Note that if they are not present, the file
	 * will not be created by the library.
	 */
	pivCardSaveContainer(card, PIVDIGITALSIGCERTTAG_DO, "digitalsigcert.raw");
	pivCardSaveContainer(card, PIVKEYMGMTCERTTAG_DO, "keymgmtcert.raw");
	pivCardSaveContainer(card, PIVCARDAUTHCERTTAG_DO, "cardauthcert.raw");

	/*
	 * Ask for the PIN, and send it to the card.
	 * If VERIFY fails, retry counter is decremented,
	 * so be judicious.
	 */
	if (getPin(pin) != 0) {
		printf("Error getting PIN.\n");
		goto err_out;
	}
	printf("\n");
	if (pivValidatePIN(pin) != 0) {
		printf("PIN in not in a valid format.\n");
		goto err_out;
	}

	REWIND_BDB(&cardresponse);
	ret = pivCardPINAuth(card, pin);
	if (ret != 0)
		ERR_OUT("Invalid PIN");

	/*
	 * Get the mandatory data objects that are readable with a PIN
	 */
	cardbufsz = PIV_MAX_OBJECT_SIZE;
	ret = pivCardGetFingerMinutiaeRec(card, cardbuf, &cardbufsz);
	if (ret != 0) {
		ERRP("Error getting finger minutiae INCITS record: %u.\n", ret);
	} else {
		fp = fopen("fmr.378", "wb+");
		fwrite(cardbuf, 1, cardbufsz, fp);
		fclose(fp);
	}
	/*
	 * Get the minutiae from the record.
	 */
#if 0
	for (i=1; i<=2; i++) {
		databufsz = PIV_MAX_OBJECT_SIZE;
		ret = pivGetFingerMinutiae(i, cardbuf, PIV_MAX_OBJECT_SIZE,
		    databuf, &databufsz, &mcount);
		printf("View %d has %u minutiae.\n", i, mcount);
		pfmd = (struct piv_fmd *)databuf;
		for (m=0; m<mcount; m++) {
			printf("%d: %u %u %u %u %u\n", m, pfmd->type,
			    pfmd->x_coord, pfmd->y_coord, pfmd->angle,
			    pfmd->quality);
			pfmd++;
		}
	}
#endif

	cardbufsz = PIV_MAX_OBJECT_SIZE;
	ret = pivCardGetFaceImageRec(card, cardbuf, &cardbufsz);
	if (ret != 0) {
		ERRP("Error getting facial image INCITS record: %u.\n", ret);
	} else {
		fp = fopen("face.385", "wb+");
		fwrite(cardbuf, 1, cardbufsz, fp);
		fclose(fp);
	}
	/* Get the face image as well. */
	databufsz = PIV_MAX_OBJECT_SIZE;
	ret = pivGetFaceImage(cardbuf, PIV_MAX_OBJECT_SIZE, databuf, &databufsz);
	if (ret != 0) {
		ERRP("Error getting facial image from buffer: %u.\n", ret);
	} else {
		fp = fopen("facefrombuffer.jpg", "wb+");
		fwrite(databuf, 1, databufsz, fp);
		fclose(fp);
	}

	cardbufsz = PIV_MAX_OBJECT_SIZE;
	ret = pivCardGetFaceImage(card, cardbuf, &cardbufsz);
	if (ret != 0) {
		ERRP("Error getting facial image from card: %u.\n", ret);
	} else {
		fp = fopen("facefromcard.jpg", "wb+");
		fwrite(cardbuf, 1, cardbufsz, fp);
		fclose(fp);
	}

	pivCardSaveContainer(card, PIVFINGERPRINTSTAG_DO, "fingerminutiae.raw");

	/*
	 * Get the optional data objects that are readable with a PIN
	 */
	pivCardSaveContainer(card, PIVPRINTEDINFOTAG_DO, "printedinfo.raw");
	pivCardSaveContainer(card, PIVFACETAG_DO, "facialimage.raw");

	ret = pivCardDisconnect(card);
	if (ret != 0)
		ERRP("Could not disconnect from card");

	exitcode = EXIT_SUCCESS;

err_out:

	exit(exitcode);
}
