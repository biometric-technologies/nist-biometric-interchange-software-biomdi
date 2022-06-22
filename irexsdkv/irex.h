/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility  whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */

#ifndef _IREX_H
#define _IREX_H

#define IREX_FORMAT_VERSION_ID		"015"
#define IREX_RECORD_HEADER_LENGTH	46

/* The sensor manufacturer IDs */
#define IREX_SENSOR_UNKNOWN		0x0000
#define IREX_SENSOR_ID_LG2200		0x2A16
#define IREX_SENSOR_ID_LG3000		0x2A1E
#define IREX_SENSOR_ID_LG4000		0x2A26
#define IREX_SENSOR_ID_PIER		0x1A03

/* Polar->rectilinear conversion methods */
#define IREX_BILINEAR			1
#define IREX_BICUBIC			2

/*
 * Degrade an image by transforming the raw data to JPEG, using the JPEG
 * rate to specify the quality, then convert back to raw data. The image
 * data contained in the input image header is replaced with the degraded
 * image data.
 * The record header is modified, setting the image length to match the
 * degraded image length (which should be the same as the input image).
 * Parameters:
 *    iih          Pointer to the Iris Image Header
 *    samplerate   The JPEG sampling bit-rate
 * Returns:
 *    0 on Success
 *   -1 on error (memory allocation, etc.)
 */
#ifdef USE_IMGLIBS
int
irex_degrade_image(IIH *iih, double samplerate);
#endif

/*
 * Convert an iris image kind polar to kind rectilinear, which involves
 * converting the raster data and fixing up the image length field. The
 * existing image data buffer will be freed.
 * The record header is modified, setting the image length to match the
 * converted image length, and setting the width and height to those of
 * the rectilinear image.
 * Parameters:
 *    iih     Pointer to the iris image header structure
 *    method  The method to be used: IREX_BILINEAR or IREX_BICUBIC
 * Returns:
 *    0 on Success
 *   -1 on error (memory allocation, etc.)
 */
int
irex_polar_to_rectilinear(IIH *iih, int method);

/*
 * Convert rectilinear image raster data to polar format.
 * Returns 0 on success, -1 otherwise.
 */
int
bilinear_rectilinear_to_polar(const uint8_t *rect_raster,
    const unsigned int width, const unsigned int height,
    uint8_t *polar_raster,
    const unsigned int num_circum_samples,
    const unsigned int num_radial_samples,
    const unsigned int polar_center_x, const unsigned int polar_center_y,
    const unsigned int inner_radius, const unsigned int outer_radius);

int
bicubic_rectilinear_to_polar(const uint8_t *rect_raster,
    const unsigned int width, const unsigned int height,
    uint8_t *polar_raster,
    const unsigned int num_circum_samples,
    const unsigned int num_radial_samples,
    const unsigned int polar_center_x, const unsigned int polar_center_y,
    const unsigned int inner_radius, const unsigned int outer_radius);

/*
 * Convert polar image raster data to rectilinear format.
 * Assumes space for the rectilinear image has already been allocated, and
 * that the width and height are both 2 * outer radius distance.
 * Returns 0 on success, -1 otherwise.
 */
int
bilinear_polar_to_rectilinear(const uint8_t *polar_raster,
    const unsigned int num_circum_samples,
    const unsigned int num_radial_samples,
    uint8_t *rect_raster,
    const unsigned int width, const unsigned int height,
    const unsigned int polar_center_x, const unsigned int polar_center_y,
    const unsigned int inner_radius, const unsigned int outer_radius);

int
bicubic_polar_to_rectilinear(const uint8_t *polar_raster,
    const unsigned int num_circum_samples,
    const unsigned int num_radial_samples,
    uint8_t *rect_raster,
    const unsigned int width, const unsigned int height,
    const unsigned int polar_center_x, const unsigned int polar_center_y,
    const unsigned int inner_radius, const unsigned int outer_radius);

/*
 * Convert a binary stream of encoded Freeman Chain Code values.
 * For example, FCC values 010 100 111 convert to 2 4 7.
 * 
 * Parameters:
 *    enc      Pointer to the binary FCC stream
 *    nelem    The number of FCC elements in the stream (the number expected
 *             to be decoded)
 * Returns an array of decoded integer values.
 */
uint8_t *
fcc_binary_decode(const uint8_t *enc, const unsigned int nelem);

#endif	/*  _IREX_H */
