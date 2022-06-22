/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility  whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
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
#ifdef IREX08_UNSEGPOLAR
#include <biomdi.h>
#include <biomdimacro.h>
#include <iid_ext.h>
#include <iid.h>
#include <irex.h>
#endif
#include <irex_sdk.h>


void bail(const char *message, const char *offender, const int exitcode)
  { fprintf(stderr, "Error: %s w.r.t %s\n", message, offender);  exit(exitcode); }

BYTE *readpgm(const char *, UINT16 *, UINT16 *);
BYTE readeye(const char *);

// make a template from a t6 record, and write it to disk
// add a four byte size header

#ifdef IREX08_TEMPLATE_OPTION_B
void make_asymmetric_templates(const char *fn, const BYTE *t6)
{
  char s[96];
  UINT16 tsize;
  BYTE *tplate = (BYTE *)malloc(65536);
  if (tplate == NULL)
     bail("write_template: could not malloc space for template", fn, 5);

  // from this image make an enrollment template
  {
     const INT32 ret = convert_image_to_enrollment_template(t6, &tsize, tplate);
     if (!ret)
     {
        sprintf(s, "%s.enrol", fn);
        const UINT32 sz = (UINT32)tsize;  // always write four bytes
        FILE *fp = fopen(s, "wb");
        if (!fp)
           bail("could not write fopen", s, 6);
        if (1 != fwrite(&sz, sizeof(UINT32), 1, fp))
           bail("write_template: could not fwrite template size", s, 7);
        if (tsize != fwrite(tplate, sizeof(BYTE), tsize, fp))
           bail("write_template: could not fwrite template", s, 8);
        fclose(fp);
     }
     else
     {
        sprintf(s, "%s.enrol.errcode", fn);
        {
           FILE *fp = fopen(s, "w");
           if (!fp)
              bail("could not write fopen", s, 6);
           fprintf(fp, "call to make enrol template for file %s returned %d\n", fn, ret); 
           fclose(fp);
        }
     }
  }
  
  // and also make the verification template
  {
     const INT32 ret = convert_image_to_verification_template(t6, &tsize, tplate);
     if (!ret)
     {
        sprintf(s, "%s.verif", fn);
        const UINT32 sz = (UINT32)tsize;  // always write four bytes
        FILE *fp = fopen(s, "wb");
        if (!fp)
           bail("could not write fopen", s, 6);
        if (1 != fwrite(&sz, sizeof(UINT32), 1, fp))
           bail("write_template: could not fwrite template size", s, 7);
        if (tsize != fwrite(tplate, sizeof(BYTE), tsize, fp))
           bail("write_template: could not fwrite template", s, 8);
        fclose(fp);
     }
     else
     {
        sprintf(s, "%s.verif.errcode", fn);
        {
           FILE *fp = fopen(s, "w");
           if (!fp)
              bail("could not write fopen", s, 6);
           fprintf(fp, "call to make verif template for file %s returned %d\n", fn, ret); 
           fclose(fp);
        }
     }
  }
  free(tplate);
}
#else
void make_template(const char *fn, const BYTE *t6)
{
  INT32 ret;
  UINT16 tsize;
  BYTE *tplate = (BYTE *)malloc(65536);
  if (tplate == NULL)
     bail("write_template: could not malloc space for template", fn, 5);
  ret = convert_image_to_template(t6, &tsize, tplate);
  if (!ret)
  {
     const UINT32 sz = (UINT32)tsize;  // always write four bytes
     FILE *fp = fopen(fn, "wb");
     if (!fp)
        bail("could not write fopen", fn, 6);
     if (1 != fwrite(&sz, sizeof(UINT32), 1, fp))
        bail("write_template: could not fwrite template size", fn, 7);
     if (tsize != fwrite(tplate, sizeof(BYTE), tsize, fp))
        bail("write_template: could not fwrite template", fn, 8);
     fclose(fp);
  }
  else
  {
     char s[96];
     sprintf(s, "%s.errcode", fn);
     {
        FILE *fp = fopen(s, "w");
        if (!fp)
           bail("could not write fopen", s, 6);
        fprintf(fp, "call to make template for file %s returned %d\n", fn, ret); 
        fclose(fp);
     }
  }
  free(tplate);
}
#endif


BYTE *read_template(const char *fn, UINT32 *tsize)
{
  FILE *fp = fopen(fn, "rb");
  if (!fp)
     bail("read_template: could not read fopen", fn, 6);
  if (1 != fread(tsize, sizeof(UINT32), 1, fp))
     bail("read_template: could not fread template size", fn, 7);
  {
     BYTE *tplate = (BYTE *)malloc(*tsize * sizeof(BYTE));
     if (tplate == NULL)
        bail("read_template: could not malloc space for template", fn, 8);
     if (*tsize != fread(tplate, sizeof(BYTE), *tsize, fp))
        bail("read_template: could not fread template", fn, 8);
     fclose(fp);
     return(tplate);
  }
}


UINT32 get_record_length(const BYTE *t6)
{
  /* input is a Table 6 record;  return the value of the fourth field "Record Length" */
  UINT32 rl;
  memcpy(&rl, &t6[9], 4);
  rl = ntohl(rl);
  return rl; 
}

void write_table6_instance(const char *fn, const BYTE *t6)
{
  const UINT32 recordlength = get_record_length(t6);
  FILE *fp = fopen(fn, "wb");
  if (!fp)
     bail("could not write fopen", fn, 3);
  if (recordlength != fwrite(t6, 1, recordlength, fp))
  bail("could not fwrite the Table 6 record", fn, 4);
  fclose(fp);
}

// read and match two templates
INT32 compare_two_templates(const char *fn1, const char *fn2, UINT32 *t1size, UINT32 *t2size, double *d)
{
   BYTE *t1, *t2;
#ifdef IREX08_TEMPLATE_OPTION_B
   char s[96];
   sprintf(s, "%s.verif", fn1); t1 = read_template(s, t1size);
   sprintf(s, "%s.enrol", fn2); t2 = read_template(s, t2size);
#else
   t1 = read_template(fn1, t1size);
   t2 = read_template(fn2, t2size);
#endif
   return match_templates(t1, (UINT16)*t1size, t2, (UINT16)*t2size, d);
}

int main(int argc, char **argv)
{
  unsigned int i, j;
  const unsigned int numfiles = 11;

  //
  // Exercise the SDK in two stages.
  // A.  Generation of Table 6 records and templates, and B.  Matching
  // 
  for ( i = 0 ; i < numfiles; i++ )
  {
     char fn1[64], fn2[64];
     sprintf(fn1, "images/%02d.eye", i);
     sprintf(fn2, "images/%02d.pgm", i);
     INT32 ret;
     {
        char fn[64];
        UINT16 w, h;
        const BYTE rightleft = readeye(fn1);
        BYTE *raster = readpgm(fn2, &w, &h);
        const UINT32 allocated_bytes = w * h;

        // 1. make an cropped rectilinear instance
        {
           INT16 bbox_x, bbox_y;
           BYTE *stdrecord = (BYTE *)calloc(allocated_bytes, sizeof(BYTE));
           if (NULL == stdrecord)
              bail("calloc failed on the cropped raster for", fn2, 1);

           ret = convert_raster_to_cropped_rectilinear(raster, w, h, 
              (BYTE)1,  /* base horz orientation */
              (BYTE)1,  /* base vert orientation */
              (BYTE)2,  /* scan type interlace frame - these images appear to be interlaced */
              rightleft,
              (BYTE)2, (BYTE)8, /* vanilla greyscale in eight bits */
              (UINT16)0x2A16,   /* these are LG2200 */
              &bbox_x, &bbox_y,
              allocated_bytes, stdrecord);

              if (ret == SUCCESS) {
                 /* write the crop only Table 6 result */
                 sprintf(fn, "output/t6/croponly/%02d.t6", i);
                 write_table6_instance(fn, stdrecord);

               /* now make a template from this cropped iris */
                 sprintf(fn, "output/templates/croponly/%02d.template", i);
#ifdef IREX08_TEMPLATE_OPTION_B
                 make_asymmetric_templates(fn, stdrecord);
#else
                 make_template(fn, stdrecord);
#endif
              } else {
                 printf("convert_raster_to_cropped_rectilinear ret %d\n", ret);
              }
           free(stdrecord);
        }

#ifdef IREX08_CROPMASK
        // 2. make an cropped+masked rectilinear instance - #ifdef this away if you don't support this
        {
           BYTE *stdrecord = (BYTE *)calloc(allocated_bytes, sizeof(BYTE));
           if (NULL == stdrecord)
              bail("calloc failed on the cropped and masked raster", fn2, 1);

           ret = convert_raster_to_cropped_and_masked_rectilinear(raster, w, h, 
              (BYTE)1,  /* base horz orientation */
              (BYTE)1,  /* base vert orientation */
              (BYTE)2,  /* scan type interlace frame - these images appear to be interlaced */
              rightleft,
              (BYTE)2, (BYTE)8, /* vanilla greyscale in eight bits */
              (UINT16)0x2A16,   /* these are LG2200 */
              allocated_bytes, stdrecord);

              if (ret == SUCCESS) {
                 /* write the cropped and masked Table 6 result */
                 sprintf(fn, "output/t6/cropmask/%02d.t6", i);
                 write_table6_instance(fn, stdrecord);

                 /* now make a template from this cropped iris */
                 sprintf(fn, "output/templates/cropmask/%02d.template", i);
#ifdef IREX08_TEMPLATE_OPTION_B
                 make_asymmetric_templates(fn, stdrecord);
#else
                 make_template(fn, stdrecord);
#endif
              } else {
                 printf("convert_raster_to_cropped_and_masked_rectilinear ret %d\n", ret);
              }
           free(stdrecord);
        }
#endif

        // 3. make an unsegmented polar instance
#ifdef IREX08_UNSEGPOLAR
        {
           BYTE *stdrecord = (BYTE *)calloc(allocated_bytes, sizeof(BYTE));
           if (NULL == stdrecord)
              bail("calloc failed on the undeg polar", fn2, 1);

           ret = convert_raster_to_unsegmented_polar(raster, w, h, 
              (BYTE)1,  /* base horz orientation */
              (BYTE)1,  /* base vert orientation */
              (BYTE)2,  /* scan type interlace frame - these images appear to be interlaced */
              rightleft,
              (BYTE)2, (BYTE)8, /* vanilla greyscale in eight bits */
              (UINT16)0x2A16,   /* these are LG2200 */
              0, 0,  /* you choose the radial and circumferential sampling rates */
              allocated_bytes, stdrecord);

              if (ret == SUCCESS)
              {
                 /* write the unseg polar Table 6 result */
                 sprintf(fn, "output/t6/unsegpolar/%02d.t6.k16", i);
                 write_table6_instance(fn, stdrecord);

                 {
                    BDB x;  IIH *iih; IRH *irh;
                    INIT_BDB(&x, stdrecord, allocated_bytes);
                    IIBDB *iibdb;
                    new_iibdb(&iibdb);
                    scan_iibdb(&x, iibdb);
                    irh = &iibdb->record_header;
                    for ( j = 0 ; j < irh->num_eyes ; j++ )
                    {
                       IBSH *ibsh = iibdb->biometric_subtype_headers[j];
                       TAILQ_FOREACH(iih, &ibsh->image_headers, list)
                       {
                          // In IREX only NIST executes revsere polar transformation
                          // so we need to convert this Kind=16 record to a Kind 48 record,
                          ret = irex_polar_to_rectilinear(iih, IREX_BICUBIC);
                          if (ret != SUCCESS)
                             bail("conversion of kind 16 to kind 48 failed", fn2, ret);
                       }
                    }
                    // write the unseg polar Table 6 result
                    // and rightaway read it back into a flat byte array
                    sprintf(fn, "output/t6/unsegpolar/%02d.t6.k48", i);
                    {
                       FILE *fp = fopen(fn, "wb");
                       write_iibdb(fp, iibdb);
                       fclose(fp);
                    }{
                       FILE *fp = fopen(fn, "rb");
                       const size_t sz = (size_t)irh->record_length;
                       if ((stdrecord = (BYTE *)realloc(stdrecord, sz)) == NULL)
                          bail("read_template: could not realloc space for the kind48 image", fn, 8);
                       if (sz != fread(stdrecord, sizeof(BYTE), sz, fp))
                          bail("reverse polar: could not fread the kind48 image", fn, 8);
                       fclose(fp);
                    }
                    free_iibdb(iibdb);
                 }


                 /* now make a template from this cropped iris */
                 sprintf(fn, "output/templates/unsegpolar/%02d.template", i);
#ifdef IREX08_TEMPLATE_OPTION_B
                 make_asymmetric_templates(fn, stdrecord);
#else
                 make_template(fn, stdrecord);
#endif
              } else {
                 printf("convert_raster_to_unsegmented_polar ret %d\n", ret);
              }
           free(stdrecord);
        }
#endif
        free(raster);
     }
  }

  //
  // Exercise the SDK in two stages.  This is the second.
  // B.  Matching of the templates generated in Stage A.
  // 
  {
     char fn[64], fn1[64], fn2[64];
     double dissimilarity;
     INT32 ret; UINT32 t1size, t2size;
     FILE *fp;

     {  // 1. match croponly vs. croponly templates
        sprintf(fn, "output/matching/croponly/fullcross.txt");
        fp = fopen(fn, "w");
        if (fp == NULL)
           bail("could not write fopen", fn, 10);

        for ( i = 0 ; i < numfiles; i++ )
        {
           sprintf(fn1, "output/templates/croponly/%02u.template", i);
           for ( j = 0 ; j < numfiles; j++ )
           {
              sprintf(fn2, "output/templates/croponly/%02u.template", j);
              ret = compare_two_templates(fn1, fn2, &t1size, &t2size, &dissimilarity);
              fprintf(fp, "%02u %02u %d %u %u %lf\n", i, j, (int)ret, t1size, t2size, dissimilarity);
           }
        }
        fclose(fp);
     }

#ifdef IREX08_CROPMASK
     {  // 2. match cropmask vs. cropmask templates
        sprintf(fn, "output/matching/cropmask/fullcross.txt");
        fp = fopen(fn, "w");
        if (fp == NULL)
           bail("could not write fopen", fn, 10);

        for ( i = 0 ; i < numfiles; i++ )
        {
           sprintf(fn1, "output/templates/cropmask/%02u.template", i);
           for ( j = 0 ; j < numfiles; j++ )
           {
              sprintf(fn2, "output/templates/cropmask/%02u.template", j);
              ret = compare_two_templates(fn1, fn2, &t1size, &t2size, &dissimilarity);
              fprintf(fp, "%02u %02u %d %u %u %lf\n", i, j, (int)ret, t1size, t2size, dissimilarity);
           }
        }
        fclose(fp);
     }
#endif

#ifdef IREX08_UNSEGPOLAR
     {  // 3. match unseg polar vs. unseg polar templates
        sprintf(fn, "output/matching/unsegpolar/fullcross.txt");
        fp = fopen(fn, "w");
        if (fp == NULL)
           bail("could not write fopen", fn, 10);

        for ( i = 0 ; i < numfiles; i++ )
        {
           sprintf(fn1, "output/templates/unsegpolar/%02u.template", i);
           for ( j = 0 ; j < numfiles; j++ )
           {
              sprintf(fn2, "output/templates/unsegpolar/%02u.template", j);
              ret = compare_two_templates(fn1, fn2, &t1size, &t2size, &dissimilarity);
              fprintf(fp, "%02u %02u %d %u %u %lf\n", i, j, (int)ret, t1size, t2size, dissimilarity);
           }
        }
        fclose(fp);
     }
#endif
  }

  return 0;
}



BYTE readeye(const char *filename)
{
  FILE *fp = fopen(filename, "r");
  if (!fp)
     bail("failed to fopen", filename, errno);

  {
     unsigned int eye;
     if (1 != fscanf(fp, "%u", &eye))
        bail("could not fscanf a magic number", filename, 1);
     if ((eye != 1) && (eye != 2))
        bail("illegal eye label value", filename, 2);
     fclose(fp);
     return (BYTE)eye;
  }
}

/* Loads an uncompressed, binary PGM (Pixel Gray Map) file into memory.

  PGM format specification:
     1) A Magic number ("P5")
     2) Whitespace
     3) Image width
     4) Whitespace
     5) Image Height
     6) Whitespace
     7) Maximum Gray Value (should be 255)
     8) Whitespace
     9) Raster Data
*/

BYTE *readpgm(const char *filename, UINT16 *width, UINT16 *height)
{
  FILE *fp = fopen(filename, "rb");
  if (!fp)
     bail("failed to fopen", filename, errno);

  {  /* 1. magic number test */
     char magicNumber[9];
     if (1 != fscanf(fp, "%s ", magicNumber))
        bail("could not fscanf a magic number", filename, 1);
     if (strcmp(magicNumber, "P5"))
        bail("magic number is incorrect, expecting P5", filename, 2);
  }
  {  /* 2. read image dimensions */
     unsigned int w, h;
     if (2 != fscanf(fp, "%u %u", &w, &h))
        bail("failed to fscanf width and height", filename, 3);
     *width  = (UINT16)w;
     *height = (UINT16)h;
  }
  {  /* 3. read max gray value */
     unsigned int maxGrayValue;
     if (1 != fscanf(fp, "%u", &maxGrayValue))
        bail("failed to fscanf maxvalue", filename, 4);
     (void)fgetc(fp); // consume a whitespace character
     if (maxGrayValue != 255)
        bail("cannot process PGMs with other than 8 bits per pixel", filename, 5);
  }
  {  /* 4. read in the raster data */
     const unsigned int num_pixels = *width * *height;
     BYTE *raster = (BYTE *)malloc(num_pixels);
     if (raster == NULL)
        bail("malloc of image raster failed", filename, 6);
     {
        const unsigned int num_bytes_read = fread(raster, 1, num_pixels, fp);
        if (num_bytes_read != num_pixels)
           bail("fread return incorrect number of bytes", filename, 7);
     }
     fclose(fp);
     return raster;
  }
}
