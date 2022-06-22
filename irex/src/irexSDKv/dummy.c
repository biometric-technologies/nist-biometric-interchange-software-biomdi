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
 * This is a stubbed, entirely non functional implementation of the IREX API
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "irex_supplier.h"
#include <biomdi.h>
#include <biomdimacro.h>
#include <iid_ext.h>
#include <iid.h>
#include <irex.h>
#include <irex_sdk.h>

#ifdef IREX08_TEMPLATE_OPTION_B

INT32 convert_image_to_enrollment_template(
   const BYTE *input_record, UINT16 *template_size, BYTE *proprietary_template)
   { return 0; }

INT32 convert_image_to_verification_template(
   const BYTE *input_record, UINT16 *template_size, BYTE *proprietary_template)
   { return 0; }

#else

INT32 convert_image_to_template(
    const BYTE *input_record, UINT16 *template_size, BYTE *proprietary_template)
    { return 0; }

#endif

INT32 convert_raster_to_cropped_rectilinear(
    const BYTE *uncompressed_raster_data,
    const UINT16 image_width,
    const UINT16 image_height,
    const BYTE horz_orientation,
    const BYTE vert_orientation,
    const BYTE scan_type,
    const BYTE which_eye,
    const BYTE image_format,
    const BYTE intensity_depth,
    const UINT16 nist_encoded_device_id,
    INT16 *bbox_topleft_x,
    INT16 *bbox_topleft_y,
    const UINT32 allocated_bytes,
    BYTE * c_rectilinear_image)
    { return 0; }

INT32 convert_raster_to_cropped_and_masked_rectilinear(
    const BYTE *uncompressed_raster_data,
    const UINT16 image_width,
    const UINT16 image_height,
    const BYTE horz_orientation,
    const BYTE vert_orientation,
    const BYTE scan_type,
    const BYTE which_eye,
    const BYTE image_format,
    const BYTE intensity_depth,
    const UINT16 nist_encoded_device_id,
    const UINT32 allocated_bytes,
    BYTE * cm_rectilinear_image)
    { return 0; }

INT32 convert_raster_to_unsegmented_polar(
    const BYTE *uncompressed_raster_data,
    const UINT16 image_width,
    const UINT16 image_height,
    const BYTE horz_orientation,
    const BYTE vert_orientation,
    const BYTE scan_type,
    const BYTE which_eye,
    const BYTE image_format,
    const BYTE intensity_depth,
    const UINT16 nist_encoded_device_id,
    const UINT16 num_samples_radially,
    const UINT16 num_samples_circumferentially,
    const UINT32 allocated_bytes,
    BYTE * unseg_polar_image)
    { return 0; }

INT32 match_templates(
    const BYTE *verification_template,
    const UINT16 verification_template_size,
    const BYTE *enrollment_template,
    const UINT16 enrollment_template_size,
    double *dissimilarity)
    { return 0; }

INT32 get_pid(
    UINT32 *nist_assigned_identifier,
    char *email_address)
    {  return 0; }

