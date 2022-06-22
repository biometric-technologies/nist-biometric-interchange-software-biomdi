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
/* This program uses the Iris Image Biometric Data Block library to           */
/* manipulate the contents of a file containing iris image records.           */
/******************************************************************************/
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <biomdi.h>
#include <biomdimacro.h>
#include <iid_ext.h>
#include <iid.h>
#include <irex.h>

/*
 * Global variables, set in option processing.
 */
#define CREATE_WITH_DEFAULTS	1
#define CREATE_WITH_INPUT	2
static int G_operation;
static int G_eyepos;
static uint16_t sensor;
static FILE *G_imgfp;
static FILE *G_outfp;

static void
usage(char * str)
{
	printf("Usage:\n");
	printf("\t%s -i <pgmfile>  -o <outfile>  -e[L | R]  -s <sensorID>\n", str);
}

/******************************************************************************/
/* Close all open files.                                                      */
/******************************************************************************/
static void
close_files()
{
	if (G_imgfp != NULL)
		(void)fclose(G_imgfp);
	if (G_outfp != NULL)
		(void)fclose(G_outfp);
}

/******************************************************************************/
/* Process the command line options, and set the global option indicators     */
/* based on those options.  This function will force an exit of the program   */
/* on error.                                                                  */
/******************************************************************************/
static void
get_options(int argc, char *argv[])
{
	int ch;
	int i_opt, o_opt, e_opt, s_opt;
	char *out_file;
	char pm;
	struct stat sb;
	char *argstr = "i:o:e:s:";

	i_opt = o_opt = e_opt = s_opt = 0;
	while ((ch = getopt(argc, argv, argstr)) != -1) {
		switch (ch) {
		case 'i':
       			if ((G_imgfp = fopen(optarg, "r")) == NULL)
			       OPEN_ERR_EXIT(optarg);
       			i_opt++;
			break;
		case 'o':
			if (stat(optarg, &sb) == 0) {
				ERR_OUT("File '%s' exists, remove it first.",
				    optarg);
			}
			if ((G_outfp = fopen(optarg, "wb")) == NULL)
				OPEN_ERR_EXIT(optarg);
			out_file = optarg;
			o_opt++;
			break;
		case 's':
			if (1 != sscanf(optarg, "%hx", &sensor))
				ERR_OUT("Could not scan a hex integer from: '%s'",
				    optarg);
			if ((sensor != 0x2A16) && (sensor != 0x2A1E) &&
			    (sensor != 0x2A26) && (sensor != 0x1A03) && (sensor != 0x0000))
			{
				ERR_OUT("Sensor should be 0x2A16 0x2A1E 0x2A26 0x1A03 or 0x0000 not: '%s'",
				    optarg);
			}
			s_opt++;
			break;
			
		case 'e':
			pm = *(char *)optarg;
				switch (pm) {
					case 'L':
						G_eyepos = IID_EYE_LEFT;
						break;
					case 'R':
						G_eyepos = IID_EYE_RIGHT;
						break;
					default:
						goto err_usage_out;
						break;  /* not reached */
				}
			e_opt++;
			break;
		default:
			goto err_usage_out;
			break;	/* not reached */
		}
	}

	/* Enforce the option choices */
	/* Only one instance of each option */
	if ((i_opt != 1) || (o_opt != 1) || (e_opt != 1) || (s_opt != 1))
		goto err_usage_out;
	G_operation = CREATE_WITH_DEFAULTS;
	return;

err_usage_out:
	usage(argv[0]);
err_out:
	/* If we created the output file, remove it. */
	if (o_opt == 1)
		(void)unlink(out_file);
	close_files();
	exit(EXIT_FAILURE);
}

static int
read_pgm_hdr(FILE *fp, int *hdrlen, int *width, int *height, int *intensity)
{
//XXX this needs to be more robust, and handle comments
	if(fscanf(fp, "P5 %d %d %d %n", width, height, intensity, hdrlen) < 0)
		return (-1);
	*intensity = (int)(log((double)(*intensity + 1)) / log((double)2));
	return (0);
}

static int
create_outfile_defaults(FILE *outfp, FILE *imgfp)
{
	struct stat sb;
	IIBDB *iibdb;
	IIH *iih;
	IBSH *ibsh;
	int hdrlen, width, height, intensity;

	if (fstat(fileno(G_imgfp), &sb) < 0) {
		ERRP("Could not get stats on image file");
		return (-1);
	}
	if (read_pgm_hdr(imgfp, &hdrlen, &width, &height, &intensity) < 0) {
		ERRP("Could not read PGM header");
		return (-1);
	}

	if (new_iibdb(&iibdb) < 0)
		ALLOC_ERR_RETURN("Iris Image Biometric Data Block");
	if (new_ibsh(&ibsh) < 0)
		ALLOC_ERR_RETURN("Iris Biometric Subtype Header");
	if (new_iih(&iih) < 0)
		ALLOC_ERR_RETURN("Iris Image Header");

	strcpy(iibdb->record_header.format_id, IID_FORMAT_ID);
	strcpy(iibdb->record_header.format_version, IREX_FORMAT_VERSION_ID);
	iibdb->record_header.kind_of_imagery =
	    IID_IMAGE_KIND_RECTLINEAR_NO_ROI_NO_CROPPING;
	iibdb->record_header.capture_device_id = sensor; /* IREX_SENSOR_UNKNOWN; */
	iibdb->record_header.num_eyes = 1;
	iibdb->record_header.record_header_length = IREX_RECORD_HEADER_LENGTH;
	iibdb->record_header.horizontal_orientation = IID_ORIENTATION_BASE;
	iibdb->record_header.vertical_orientation = IID_ORIENTATION_BASE;
	iibdb->record_header.scan_type = IID_SCAN_TYPE_PROGRESSIVE;
	iibdb->record_header.iris_occlusions = 0;
	iibdb->record_header.occlusion_filling = 0;
	iibdb->record_header.diameter = 0;
	iibdb->record_header.image_format = IID_IMAGEFORMAT_MONO_RAW;
	iibdb->record_header.image_transformation = IID_TRANS_UNDEF;
	strcpy(iibdb->record_header.device_unique_id, IID_DEVICE_UNIQUE_ID_NONE);
	iibdb->record_header.image_width = width;
	iibdb->record_header.image_height = height;
	iibdb->record_header.intensity_depth = intensity;

	ibsh->eye_position = G_eyepos;
	ibsh->num_images = 1;

	/* new_iih() zero's the entire block, so no need to set some fields */
	iih->image_length = sb.st_size - hdrlen;
	iih->image_data = (uint8_t *)malloc(iih->image_length);
	if (iih->image_data == NULL)
		ALLOC_ERR_RETURN("Buffer for image data");
	if (fread(iih->image_data, 1, iih->image_length, imgfp) !=
	    iih->image_length) {
		ERRP("Failed to read image data");
		free_iibdb(iibdb);
		return (-1);
	}
	iih->image_number = 1;
	iih->image_quality = IID_IMAGE_QUALITY_NOT_COMPUTED;
	iih->rotation_angle = IID_ROT_ANGLE_UNDEF;
	iih->rotation_uncertainty = IID_ROT_UNCERTAIN_UNDEF;
	iih->image_ancillary.pupil_center_of_ellipse_x =
	    IID_EXT_COORD_NOT_COMPUTED;
	iih->image_ancillary.iris_center_of_ellipse_x =
	    IID_EXT_COORD_NOT_COMPUTED;

	iibdb->record_header.record_length = IID_RECORD_HEADER_LENGTH +
	    IID_IBSH_LENGTH + IID_IIH_LENGTH + IID_ROIMASK_LENGTH +
	    IID_UNSEG_POLAR_LENTH +
	    IID_IMAGE_ANCILLARY_LENGTH(&iih->image_ancillary) +
	    iih->image_length;

	add_iih_to_ibsh(iih, ibsh);
	iibdb->biometric_subtype_headers[0] = ibsh;
	if (write_iibdb(outfp, iibdb) != WRITE_OK) {
		ERRP("Failed to write Iris image data block");
		free_iibdb(iibdb);
		return (-1);
	}
	free_iibdb(iibdb);
	return (0);
}

int
main(int argc, char *argv[])
{
	int ret, status;

	get_options(argc, argv);

	status = EXIT_SUCCESS;
	if (G_operation == CREATE_WITH_DEFAULTS)
		ret = create_outfile_defaults(G_outfp, G_imgfp);
	if (ret != 0)
		ERR_EXIT("Could not create output file");
	close_files();
	exit (status);
}
