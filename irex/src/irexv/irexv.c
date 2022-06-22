/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility  whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */
/******************************************************************************/
/* This program uses the Iris Image Biometric Data Block library to validate  */
/* the contents of a file containing iris image records according to the      */
/* ISO/IEC 19794-6:2005 standard with extensions described in the NIST IREX   */
/* test specification.                                                        */
/******************************************************************************/
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <biomdi.h>
#include <biomdimacro.h>
#include <iid_ext.h>
#include <iid.h>

#include <irex.h>

static int
internal_validate_roimask(ROIMASK *roimask)
{
	int status = VALIDATE_OK;

	if (roimask->upper_eyelid_mask != 0)
		status = VALIDATE_ERROR;
	if (roimask->lower_eyelid_mask != 0)
		status = VALIDATE_ERROR;
	if (roimask->sclera_mask != 0)
		status = VALIDATE_ERROR;
	return (status);
}

static int
internal_validate_unsegpolar(UNSEGPOLAR *unsegpolar)
{
	int status = VALIDATE_OK;

	if (unsegpolar->num_samples_radially != 0)
		status = VALIDATE_ERROR;
	if (unsegpolar->num_samples_circumferentially != 0)
		status = VALIDATE_ERROR;
	if (unsegpolar->inner_outer_circle_x != 0)
		status = VALIDATE_ERROR;
	if (unsegpolar->inner_outer_circle_y != 0)
		status = VALIDATE_ERROR;
	if (unsegpolar->inner_circle_radius != 0)
		status = VALIDATE_ERROR;
	if (unsegpolar->outer_circle_radius != 0)
		status = VALIDATE_ERROR;
	return (status);
}

static int
internal_validate_iih(IIH *iih, uint8_t kind_of_imagery)
{
	int ret;
	int status = VALIDATE_OK;

	if (iih->image_number != 1) {
		ERRP("Image Header image number is not 1");
		status = VALIDATE_ERROR;
	}
	if (iih->image_quality > IID_MAX_IMAGE_QUALITY) {
		if ((iih->image_quality != IID_IMAGE_QUALITY_NOT_COMPUTED) &&
		    (iih->image_quality != IID_IMAGE_QUALITY_NOT_AVAILABLE)) {
			ERRP("Image Header quality value is not valid");
			status = VALIDATE_ERROR;
		}
	}
	if (kind_of_imagery == IID_IMAGE_KIND_RECTLINEAR_MASKING_CROPPING) {
		ret = validate_roimask(&iih->roi_mask);
		if (ret != VALIDATE_OK) {
			ERRP("Image Header ROI Mask is not ISO valid");
			status = VALIDATE_ERROR;
		}
	} else {
		ret = internal_validate_roimask(&iih->roi_mask);
		if (ret != VALIDATE_OK) {
			ERRP("Image Header ROI Mask must be zero'd");
			status = VALIDATE_ERROR;
		}
	}
	if ((kind_of_imagery == IID_IMAGE_KIND_UNSEGMENTED_POLAR) ||
	    (kind_of_imagery == IID_IMAGE_KIND_RECTILINEAR_UNSEGMENTED_POLAR))
		ret = validate_unsegpolar(&iih->unsegmented_polar);
	else
		ret = internal_validate_unsegpolar(&iih->unsegmented_polar);
	if (ret != VALIDATE_OK) {
		ERRP("Image Header UNSEG Polar fields not valid");
		status = VALIDATE_ERROR;
	}
	return (status);
}

static int
internal_validate_ibsh(IBSH *ibsh)
{
	int status = VALIDATE_OK;

	if ((ibsh->eye_position != IID_EYE_RIGHT) &&
	    (ibsh->eye_position != IID_EYE_LEFT)) {
		ERRP("Invalid eye position: %s", 
		    iid_code_to_str(IID_CODE_CATEGORY_EYE_POSITION,
                                ibsh->eye_position));
		status = VALIDATE_ERROR;
	}
	if (ibsh->num_images != 1) {
		ERRP("Invalid number of iris images: %u", ibsh->num_images);
		status = VALIDATE_ERROR;
	}
	return (status);
}

static int
internal_validate_irh(IRH *irh)
{
	int status = VALIDATE_OK;

	if (strncmp(irh->format_version, IREX_FORMAT_VERSION_ID,
	    IID_FORMAT_VERSION_LEN) != 0) {
		ERRP("Format version is not valid: \"%s\"",
		    irh->format_version);
		status = VALIDATE_ERROR;
	}

	switch (irh->capture_device_id) {
	case IREX_SENSOR_UNKNOWN:
	case IREX_SENSOR_ID_LG2200:
	case IREX_SENSOR_ID_LG3000:
	case IREX_SENSOR_ID_LG4000:
	case IREX_SENSOR_ID_PIER:
		break;
	default:
		ERRP("Invalid capture device ID 0x%04x",
		    irh->capture_device_id);
		status = VALIDATE_ERROR;
		break;
	}
	if ((irh->num_eyes != 0) && (irh->num_eyes != 1)) {
		ERRP("Invalid number of eyes: %u", irh->num_eyes);
		status = VALIDATE_ERROR;
	}
	if (irh->record_header_length != IREX_RECORD_HEADER_LENGTH) {
		ERRP("Invalid record header length %u",
		    irh->record_header_length);
		status = VALIDATE_ERROR;
	}
	if (irh->horizontal_orientation != IID_ORIENTATION_BASE) {
		ERRP("Invalid horizontal orientation: %u",
		    irh->horizontal_orientation);
		status = VALIDATE_ERROR;
	}
	if (irh->vertical_orientation != IID_ORIENTATION_BASE) {
		ERRP("Invalid vertical orientation: %u",
		    irh->vertical_orientation);
		status = VALIDATE_ERROR;
	}
	if ((irh->scan_type != IID_SCAN_TYPE_PROGRESSIVE) &&
	    (irh->scan_type != IID_SCAN_TYPE_INTERLACE_FRAME)) {
		ERRP("Invalid scan type: %u", irh->scan_type);
		status = VALIDATE_ERROR;
	}
	if (irh->image_format != IID_IMAGEFORMAT_MONO_RAW) {
		ERRP("Invalid image format %u", irh->image_format);
		status = VALIDATE_ERROR;
	}
	if (irh->kind_of_imagery == IID_IMAGE_KIND_UNSEGMENTED_POLAR) {
		if (irh->image_transformation != IID_TRANS_STD) {
			ERRP("Invalid image transformation %u",
			    irh->image_transformation);
			status = VALIDATE_ERROR;
		}
	} else {
		if (irh->image_transformation != 0) {
			ERRP("Image transformation must be zero");
			status = VALIDATE_ERROR;
		}
	}
	if (strncmp(irh->device_unique_id, IID_DEVICE_UNIQUE_ID_NONE,
	    IID_DEVICE_UNIQUE_ID_LEN) != 0) {
		ERRP("Device unique ID is not valid: \"%s\"",
		    irh->device_unique_id);
		status = VALIDATE_ERROR;
	}
	return (status);
}

int
main(int argc, char *argv[])
{
	char *usage = "usage: irexv <datafile>\n";
	FILE *fp;
	struct stat sb;
	IIBDB *iibdb;
	int ret, iih_cond;
	int status;
	int i;
	IRH *irh;
	IIH *iih;
	IBSH *ibsh;

	if (argc != 2) {
		printf("%s\n", usage);
		exit (EXIT_FAILURE);
	}
				
	fp = fopen(argv[1], "rb");
	if (fp == NULL)
		ERR_EXIT("Open of %s failed: %s\n", argv[1],
		    strerror(errno));

	if (new_iibdb(&iibdb) < 0)
		ALLOC_ERR_EXIT("Iris Image Biometric Data Block");

	if (fstat(fileno(fp), &sb) < 0)
		ERR_EXIT("Could not get stats on input file");

	status = EXIT_SUCCESS;
	irh = &iibdb->record_header;

	ret = read_iibdb(fp, iibdb);
	if (ret != READ_OK)
		exit (EXIT_FAILURE);

	/*
	 * For IREX, only one record is allowed in the file, so check
	 * that the file size matches the length in the record header.
	 */
	if (irh->record_length != sb.st_size) {
		ERRP("Record header length doesn't match file size");
		status = EXIT_FAILURE;
	}

	printf("Checking entire record against ISO standard: ");
	ret = validate_iibdb(iibdb);
	if (ret != VALIDATE_OK) {
		printf("Is NOT valid.\n");
		status = EXIT_FAILURE;
	} else {
		printf("Is valid.\n");
	}

	/* Check the IREX constraints from this point forward */
	ret = internal_validate_irh(irh);
	if (ret != VALIDATE_OK)
		status = EXIT_FAILURE;

	/*
	 * Even though IREX says only one eye, we'll check all that may
	 * be present.
	 */
	iih_cond = VALIDATE_OK;
	for (i = 0; i < irh->num_eyes; i++) {
		printf("Eye number %d:\n", i+1);
		ibsh = iibdb->biometric_subtype_headers[i];
		ret = internal_validate_ibsh(ibsh);
		printf("\tSubtype Header is ");
		if (ret != VALIDATE_OK) {
			printf("not valid.\n");
			status = EXIT_FAILURE;
		} else {
			printf("valid.\n");
		}
		TAILQ_FOREACH(iih, &ibsh->image_headers, list) {
			ret = internal_validate_iih(iih, irh->kind_of_imagery);
			if (ret != VALIDATE_OK) {
				iih_cond = VALIDATE_ERROR;
				status = EXIT_FAILURE;
			}
		}
		printf("\tImage Header(s) are ");
		if (iih_cond != VALIDATE_OK)
			printf("not valid.\n");
		else
			printf("valid.\n");
	}
	free_iibdb(iibdb);
	exit (status);
}
