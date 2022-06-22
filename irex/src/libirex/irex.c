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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <biomdi.h>
#include <biomdimacro.h>
#include <iid_ext.h>
#include <iid.h>
#include <irex.h>
#ifdef USE_IMGLIBS
#include <jasper/jasper.h>
#endif

static char *
buffer_pgm_image(uint16_t width, uint16_t height, uint8_t intensity,
    uint8_t *image_data, uint32_t length, unsigned int *totsize)
{
	char temp[256];
	char *buf;
	unsigned int size;
	unsigned int idx;

	bzero((void *)temp, 256);
	sprintf(temp, "P5 %u %u %u\n", width, height, (1<<intensity) - 1);
	size = strlen(temp) + length;

	buf = (char *)malloc(size);
	if (buf == NULL)
		return (NULL);

	idx = strlen(temp);
	bcopy(temp, buf, idx);
	bcopy((char *)image_data, buf + idx, length);
	*totsize = size;
	return (buf);
}

#ifdef USE_IMGLIBS
int
irex_degrade_image(IIH *iih, double samplerate)
{
	jas_stream_t *pgms, *jpgs;
	jas_image_t *pgmi, *jpgi;
	char *pgmb, *jpgb;
	uint8_t *raster;
	unsigned int totsize, idx;
	int ret, status;
	char opt[32];

	status = -1;

	/* Get the memory buffer for the PGM input, created from image data */
	pgmb = buffer_pgm_image(
	    iih->ibsh->iibdb->record_header.image_width,
	    iih->ibsh->iibdb->record_header.image_height,
	    iih->ibsh->iibdb->record_header.intensity_depth,
	    iih->image_data,
	    iih->image_length,
	    &totsize);
	if (pgmb == NULL)
		ALLOC_ERR_RETURN("Temporary image buffer");

	/* Get the memory for the JPG output */
	// XXX What is the correct size here?
	jpgb = (char *)malloc(totsize);
	if (jpgb == NULL)
		ALLOC_ERR_OUT("Temporary image buffer");
	bzero(jpgb, totsize);

	if (jas_init())
		ERR_OUT("Could not initialize Jasper library");

	/*
	 * Create an image stream in memory to hold the existing IIH image,
	 * and create a Jasper image object to represent the image.
	 */
	pgms = jas_stream_memopen(pgmb, totsize);
	if (pgms == NULL)
		ERR_OUT("Could not crete PGM stream");
	pgmi = jas_image_decode(pgms, jas_image_strtofmt("pnm"), NULL);
	if (pgmi == NULL)
		ERR_OUT("Could not decode PGM input");

	/*
	 * Create an image stream and Jasper object to represent the degraded
	 * JPEG image, then convert from PGM to JPG with the rate option.
	 */
	snprintf(opt, 32, "rate=%f", samplerate);
	jpgs = jas_stream_memopen(jpgb, totsize);
	if (jpgs == NULL)
		ERR_OUT("Could not create JPG stream");
	ret = jas_image_encode(pgmi, jpgs, jas_image_strtofmt("jp2"), opt);
	if (ret != 0)
		ERR_OUT("Could not encode PGM to JPG");
	jas_stream_rewind(jpgs);
	jpgi = jas_image_decode(jpgs, jas_image_strtofmt("jp2"), NULL);
	if (jpgi == NULL)
		ERR_OUT("Could not transform JPG image");

	/*
	 * Convert the degraded JPEG image to a PGM image.
	 */
	jas_stream_close(pgms);
	jas_image_destroy(pgmi);
	bzero(pgmb, totsize);
	pgms = jas_stream_memopen(pgmb, totsize);
	ret = jas_image_encode(jpgi, pgms, jas_image_strtofmt("pnm"), NULL);
	if (ret != 0)
		ERR_OUT("Could not encode JPEG to PGM");
	jas_stream_rewind(pgms);
	pgmi = jas_image_decode(pgms, jas_image_strtofmt("pnm"), NULL);
	if (pgmi == NULL)
		ERR_OUT("Could not decode PGM converted image");

	/*
	 * Skip over the PGM header info, which consists of four ASCII
	 * strings followed by the image data, all separated by whitespace.
	 * idx is the index of the next character to be read, the start
	 * of the raw image data.
	 */
	(void)sscanf((char *)pgmb, "%*s %*s %*s %*s %n", &idx);

	/* Replace the existing image data with the new data */
	raster = (uint8_t *)malloc(totsize - idx);
	if (raster == NULL)
		ALLOC_ERR_OUT("Could not allocate buffer for degraded image");
	free(iih->image_data);
	iih->image_data = raster;
	bcopy(pgmb+idx, iih->image_data, totsize-idx);

	/* Fix up the IIH length and record header length */
	/* (Note that the degraded image size should be the same as input) */
	iih->ibsh->iibdb->record_header.record_length -= iih->image_length;
	iih->image_length = totsize - idx;
	iih->ibsh->iibdb->record_header.record_length += iih->image_length;
	status = 0;

err_out:
	jas_stream_close(jpgs);
	jas_image_destroy(jpgi);
	jas_stream_close(pgms);
	jas_image_destroy(pgmi);
	if (pgmb != NULL)
		free (pgmb);
	if (jpgb != NULL)
		free (jpgb);
	return (status);
}
#endif


int
irex_polar_to_rectilinear(IIH *iih, int method)
{
	int ret;
	uint8_t *rect_raster;
	unsigned int width, height, len;

	if (iih->ibsh->iibdb->record_header.kind_of_imagery !=
	    IID_IMAGE_KIND_UNSEGMENTED_POLAR) {
		ERRP("Invalid image kind");
		return (-1);
	}
	/* Compute the output image length: width is 2 * outer radius, and
	 * height is 2 * outer radius.
	 */
	width  = iih->ibsh->iibdb->record_header.image_width;
	height = iih->ibsh->iibdb->record_header.image_height;
	len = width * height;
	rect_raster = (uint8_t *)malloc(len);
	if (rect_raster == NULL)
		ALLOC_ERR_RETURN("buffer for converted image");
	switch (method) {
		case IREX_BILINEAR:
		    ret = bilinear_polar_to_rectilinear(iih->image_data,
			iih->unsegmented_polar.num_samples_circumferentially,
			iih->unsegmented_polar.num_samples_radially,
			rect_raster,
		        width,
		        height,
			iih->unsegmented_polar.inner_outer_circle_x,
			iih->unsegmented_polar.inner_outer_circle_y,
		        iih->unsegmented_polar.inner_circle_radius,
		        iih->unsegmented_polar.outer_circle_radius);
			break;
		case IREX_BICUBIC:
		    ret = bicubic_polar_to_rectilinear(iih->image_data,
			iih->unsegmented_polar.num_samples_circumferentially,
			iih->unsegmented_polar.num_samples_radially,
			rect_raster,
		        width,
		        height,
			iih->unsegmented_polar.inner_outer_circle_x,
			iih->unsegmented_polar.inner_outer_circle_y,
		        iih->unsegmented_polar.inner_circle_radius,
		        iih->unsegmented_polar.outer_circle_radius);
			break;
		default:
			ERRP("Invalid polar to rectilinear conversion method");
			return (-1);
			break;			/* not reached */
	}
	/* Fix up the length in the IIH, and length and size in the parent
	 * record header.
	 */
	if (ret == 0) {
		free(iih->image_data);
		iih->image_data = rect_raster;
		iih->ibsh->iibdb->record_header.record_length -= iih->image_length;
		iih->ibsh->iibdb->record_header.record_length += len;
		iih->image_length = len;
		iih->ibsh->iibdb->record_header.image_width = width;
		iih->ibsh->iibdb->record_header.image_height = height;
		iih->ibsh->iibdb->record_header.kind_of_imagery =
		    IID_IMAGE_KIND_RECTILINEAR_UNSEGMENTED_POLAR;
		/* The elliptical and freeman chain anciilary information was always
		 * in the coordinate system of the original rectilinear parent image.
		 * This is correct for the reconstructed image, so no action needed here.
		 */
	}
	return (ret);
}
