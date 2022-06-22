/*
    Copyright (C) 2004-2005   Ludovic Rousseau

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
* This software was modified at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. NIST assumes no responsibility whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/
/*
 * This software requires that the PCSC Lite package be installed, or other
 * similar smartcard libraries be present. An open source implementation
 * of this library can be found at http://www.linuxnet.com/
 * This same library is included with Mac OS-X 10.4 and higher, although
 * the header files may need to be installed manually.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>

#include <biomdimacro.h>
#include <nistapdu.h>

#include <cardaccess.h>

int
getReaders(SCARDCONTEXT context, char ***readerPtr, int *count)
{
	LPTSTR mszReaders;
	DWORD dwReaders;
	LONG rc;
	char *ptr;
	char **readers;
	int numReaders;

	/* Retrieve the available readers list */
	rc = SCardListReaders(context, NULL, NULL, &dwReaders);
	if (rc != SCARD_S_SUCCESS)
		ERR_OUT("SCardListReader: %s", pcsc_stringify_error(rc));

	mszReaders = malloc(sizeof(char)*dwReaders);
	if (mszReaders == NULL)
		ALLOC_ERR_OUT("Reader array");

	rc = SCardListReaders(context, NULL, mszReaders, &dwReaders);
	if (rc != SCARD_S_SUCCESS)
		ERR_OUT("SCardListReader: %s", pcsc_stringify_error(rc));

	/* Extract readers from the null separated string and get the total
	 * number of readers */
	numReaders = 0;
	ptr = mszReaders;
	while (*ptr != '\0') {
		ptr += strlen(ptr)+1;
		numReaders++;
	}

	if (numReaders == 0) {
		*count = 0;
		return (0);
	}

	/* allocate the readers table */
	readers = calloc(numReaders, sizeof(char *));
	if (NULL == readers)
		ALLOC_ERR_OUT("Readers array");

	/* fill the readers table */
	numReaders = 0;
	ptr = mszReaders;
	while (*ptr != '\0') {
		readers[numReaders] = (char *)malloc(strlen(ptr));
		strcpy(readers[numReaders], ptr);
		ptr += strlen(ptr)+1;
		numReaders++;
	}
	free(mszReaders);
	*readerPtr = readers;

	*count = numReaders;
	return (0);

err_out:
	return (-1);
}
