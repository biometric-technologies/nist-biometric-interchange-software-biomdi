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
 * Test some of the functions in libtlv.
 */

#define _XOPEN_SOURCE   1

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <termios.h>

#include <piv.h>
#include <pivcard.h>
#include <pivdata.h>

static void
usage()
{
	fprintf(stderr, "Usage: testpivcard\n");
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
	int ret;
	int i;

	if (argc != 1)
		usage();

	ret = pivCardInserted();
	printf("pivCardInserted() returns %d.\n", ret);
	if (ret != 0) {
		printf("Exiting....\n");
		exit (EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
