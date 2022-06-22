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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef USE_IMGLIBS
#include <gd.h>
#endif

#include <biomdi.h>
#include <biomdimacro.h>
#include <iid_ext.h>
#include <iid.h>
#include <irex.h>

/* The operations done by this tool */
#define EXTRACT_IMG		0
#define DEGRADE_IMG		1
#define CONVERT_POLAR_IMG	2

/* The type of images that can be saved */
#define OUTIMG_PGM 	0
#define OUTIMG_RAW 	1
#define OUTIMG_PNG 	2

char *imgext[3] = {"pgm", "raw", "png"};

/*
 * Global variables, set in option processing.
 */
static int G_imgtype;
static int G_plotfcc;
static FILE *G_infp;
static FILE *G_outfp;
static char G_imgprefix[PATH_MAX];
#ifdef USE_IMGLIBS
static double G_samplerate;
#endif
static int G_operation;

static void
usage(char * str)
{
	printf("Usage:\n\t%s -i <infile> -x{p | r} -p <imgfile_prefix>\n", str);
#ifdef USE_IMGLIBS
	printf("\t%s -i <infile> -r <rate> -o <outfile>\n", str);
	printf("\t%s -i <infile> -xn [-f] -p <imgfile_prefix>\n", str);
#endif
	printf("\t%s -i <infile> -cp -o <outfile>\n", str);
}

/******************************************************************************/
/* Close all open files.                                                      */
/******************************************************************************/
static void
close_files()
{
	if (G_infp != NULL)
		(void)fclose(G_infp);
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
	int c_opt, i_opt, o_opt, p_opt, x_opt, r_opt, f_opt;
	char pm, *out_file;
	struct stat sb;
#ifdef USE_IMGLIBS
	char *endptr;
	char *argstr = "c:i:o:p:x:r:f";
#else
	char *argstr = "c:i:o:p:x:";
#endif

	c_opt = i_opt = o_opt = p_opt = x_opt = r_opt = f_opt = 0;
	G_plotfcc = 0;
	while ((ch = getopt(argc, argv, argstr)) != -1) {
		switch (ch) {
		case 'c':
			pm = *(char *)optarg;
			switch (pm) {
			case 'p':
				G_operation = CONVERT_POLAR_IMG;
				break;
			default:
				goto err_usage_out;
				break;	/* not reached */
			}
			c_opt++;
			break;
		case 'i':
       			if ((G_infp = fopen(optarg, "r")) == NULL)
			       OPEN_ERR_EXIT(optarg);
       			i_opt = 1;
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
		case 'p':
			strncpy(G_imgprefix, optarg, PATH_MAX);
			p_opt++;
			break;
#ifdef USE_IMGLIBS
		case 'r':
			G_samplerate = strtod(optarg, &endptr);
			if ((G_samplerate == 0.0) && (endptr == optarg))
				ERR_OUT("JPEG rate must be numeric");
			if (G_samplerate < 0.0)
				ERR_OUT("JPEG rate must be greater than 0");
			r_opt++;
			break;
		case 'f':
			G_plotfcc = 1;
			f_opt++;
			break;
#endif
		case 'x':
			pm = *(char *)optarg;
			switch (pm) {
#ifdef USE_IMGLIBS
			case 'n':
				G_imgtype = OUTIMG_PNG;
				break;
#endif
			case 'p':
				G_imgtype = OUTIMG_PGM;
				break;
			case 'r':
				G_imgtype = OUTIMG_RAW;
				break;
			default:
				goto err_usage_out;
				break;	/* not reached */
			}
			x_opt++;
			break;
		default:
			goto err_usage_out;
			break;	/* not reached */
		}
	}

	/* Enforce the option choices */
	/* Only one instance of each option */
	if ((c_opt > 1) || (i_opt > 1) || (p_opt > 1) || (r_opt > 1) ||
	    (x_opt > 1) || (f_opt > 1))
		goto err_usage_out;
	if (i_opt != 1)
		goto err_usage_out;
	/* Must give one of these options */
#ifdef USE_IMGLIBS
	if ((x_opt == 0) && (r_opt == 0) && (c_opt == 0))
#else
	if ((x_opt == 0) && (c_opt == 0))
#endif
		goto err_usage_out;
	if (x_opt == 1) {
		if ((p_opt != 1) || (o_opt != 0) || (r_opt != 0))
			goto err_usage_out;
		G_operation = EXTRACT_IMG;
	}
#ifdef USE_IMGLIBS
	if (r_opt == 1) {
		if (o_opt != 1)
			goto err_usage_out;
		G_operation = DEGRADE_IMG;
	}
#endif
	if (c_opt == 1) {
		if (o_opt != 1)
			goto err_usage_out;
	}
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
write_pgm_hdr(FILE *fp, uint16_t width, uint16_t height, uint8_t intensity)
{
	if (fprintf(fp, "P5\n%u %u %u\n",
	    width, height, (1<<intensity) - 1) < 0)
		return (-1);
	return (0);
}

#ifdef USE_IMGLIBS
static int
plot_fcc(FCCB *fccb, gdImagePtr img, int color)
{
	int i;
	uint8_t *decoded;
	int32_t x, y;

	decoded = fcc_binary_decode(fccb->fcc, fccb->num_codes);
	if (decoded == NULL)
		return (-1);

	x = fccb->start_x;
	y = fccb->start_y;
	gdImageSetPixel(img, x, y, color);
	for (i = 0; i < fccb->num_codes; i++) {
		switch (decoded[i]) {
			/* Assumes Y increases down the image */
			case 0: x += 1; break;
			case 1: x += 1; y -= 1; break;
			case 2: y -= 1; break;
			case 3: x -= 1; y -= 1; break;
			case 4: x -= 1; break;
			case 5: x -= 1; y += 1; break;
			case 6: y += 1; break;
			case 7: x += 1; y += 1; break;
			default: break;		/* should never happen */
		}
		if ((x >= 0) && (y >= 0))
			gdImageSetPixel(img, x, y, color);
	}
	return (0);
}

static int
create_imgpng(IIH *iih, FILE *fp)
{
	int idx, x, y;
	uint16_t width, height;
	gdImagePtr img;
	int color;
	IRH *irh;
	IMAGEANCILLARY *ia;
	int ret;
#ifdef USE_IMGLIBS
	int RED = gdTrueColor(255, 0, 0);
	int YELLOW = gdTrueColor(255, 255, 0);
#endif

	irh = &iih->ibsh->iibdb->record_header;	
	ia = &iih->image_ancillary;
	if (irh->kind_of_imagery == IID_IMAGE_KIND_UNSEGMENTED_POLAR) {
		width = iih->unsegmented_polar.num_samples_circumferentially;
		height = iih->unsegmented_polar.num_samples_radially;
	} else {
		width = irh->image_width;
		height = irh->image_height;
	}
	img = gdImageCreateTrueColor(width, height);
	idx = 0;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			color = gdTrueColor(iih->image_data[idx+x],
			    iih->image_data[idx+x], iih->image_data[idx+x]);
			gdImageSetPixel(img, x, y, color);
		}
		idx += width;
	}
	if (G_plotfcc == 1) {
		if (ia->pupil_iris_boundary_freeman_code_length != 0) {
			ret = plot_fcc(
			    &ia->pupil_iris_boundary_freeman_code_data,
			    img, RED);
			if (ret != 0) {
				gdImageDestroy(img);
				return (-1);
			}
		}
		if (ia->sclera_iris_boundary_freeman_code_length != 0) {
			ret = plot_fcc(
			    &ia->sclera_iris_boundary_freeman_code_data,
			    img, YELLOW);
			if (ret != 0) {
				gdImageDestroy(img);
				return (-1);
			}
		}
	}
	gdImagePngEx(img, fp, 0);
	gdImageDestroy(img);
	return (0);
}
#endif

static void
extract_image(IIH *iih, int num)
{
	int ret;
	char outimg_fn[PATH_MAX];
	FILE *outimg_fp;
	IRH *irh;
	uint16_t width, height;

	irh = &iih->ibsh->iibdb->record_header;

	snprintf(outimg_fn, PATH_MAX, "%s-%d_%d.%s", G_imgprefix,
	    iih->image_number, num, imgext[G_imgtype]);
	if ((outimg_fp = fopen(outimg_fn, "wb")) == NULL)
		OPEN_ERR_EXIT(outimg_fn);
	switch (G_imgtype) {
	case OUTIMG_PGM:
		if (irh->kind_of_imagery == IID_IMAGE_KIND_UNSEGMENTED_POLAR) {
			width = iih->unsegmented_polar.num_samples_circumferentially;
			height = iih->unsegmented_polar.num_samples_radially;
		} else {
			width = irh->image_width;
			height = irh->image_height;
		}
		ret = write_pgm_hdr(outimg_fp, width, height,
		    irh->intensity_depth);
		if (ret != 0)
			ERR_OUT("Could not write PGM header");
		/* fall through */
	case OUTIMG_RAW:
		ret = fwrite(iih->image_data, sizeof(uint8_t),
		    iih->image_length, outimg_fp);
		if (ret < 0)
			ERR_OUT("Could not write image data");
		break;
#ifdef USE_IMGLIBS
	case OUTIMG_PNG:
		ret = create_imgpng(iih, outimg_fp);
		if (ret != 0)
			ERR_OUT("Could not create PNG image");
		break;
#endif
	}
	(void)fclose(outimg_fp);
	return;
err_out:
	close_files();
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	IIBDB *iibdb;
	IRH *irh;
	IIH *iih;
	IBSH *ibsh;
	int ret, status;
	int i;
	unsigned int len;
	struct stat sb;

	get_options(argc, argv);

	if (new_iibdb(&iibdb) < 0)
		ALLOC_ERR_EXIT("Iris Image Biometric Data Block");

	if (fstat(fileno(G_infp), &sb) < 0)
		ERR_EXIT("Could not get stats on input file");

	status = EXIT_SUCCESS;
	ret = read_iibdb(G_infp, iibdb);
	if (ret != READ_OK)
		ERR_EXIT("Could not read input file");
	irh = &iibdb->record_header;

	/*
	 * For IREX, only one record is allowed in the file, so check
	 * that the file size matches the length in the record header.
	 */
	if (irh->record_length != sb.st_size)
		INFOP("Record header length doesn't match file size");

	/*
	 * Even though IREX says only one eye, we'll check all that may
	 * be present.
	 */
	for (i = 0; i < irh->num_eyes; i++) {
		ibsh = iibdb->biometric_subtype_headers[i];
		TAILQ_FOREACH(iih, &ibsh->image_headers, list) {
			len = iih->image_length;
			switch (G_operation) {
				case EXTRACT_IMG:
					extract_image(iih, i);
					break;
#ifdef USE_IMGLIBS
				case DEGRADE_IMG:
					ret = irex_degrade_image(iih,
					    G_samplerate);
					if (ret != 0) {
						ERRP("Failed to degrade image");
						status = EXIT_FAILURE;
					} else {
						write_iibdb(G_outfp, iibdb);
					}
					break;
#endif
				case CONVERT_POLAR_IMG:
					ret = irex_polar_to_rectilinear(iih,
					    IREX_BICUBIC);
					if (ret != 0) {
						ERRP("Failed to convert image");
						status = EXIT_FAILURE;
					} else {
						write_iibdb(G_outfp, iibdb);
					}
					break;
			}
		}
	}
	free_iibdb(iibdb);
	close_files();
	exit (status);
}
