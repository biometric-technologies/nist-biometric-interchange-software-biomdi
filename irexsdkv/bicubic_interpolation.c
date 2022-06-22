/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*/

/*
Version		0.5.2
Last modified:	August 27, 2008
*/

#include <sys/queue.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include <biomdi.h>
#include <biomdimacro.h>
#include <iid_ext.h>
#include <iid.h>
#include <irex.h>

/*
 * Perform bicubic Hermite spline interpolation at (x, y).
 * Returns the interpolated value.
 */
static double
bicubic_interpolation(const uint8_t *image, const unsigned int width, const unsigned int height,
    const double x, const double y)
{
   /* Bicubic Hermite spline interpolation uses a weighted sum of the 16 neighbouring pixels */

   /* Weights for cubic Hermite spline */
   const static double weights[4][4] = {{  0,    1,    0,   0},
                                        {-.5,    0,   .5,   0},
                                        {  1, -2.5,    2, -.5},
                                        {-.5,  1.5, -1.5,  .5} };

   double x_weights[4] = { 0., 0., 0., 0. },
          y_weights[4] = { 0., 0., 0., 0. };

   /* nested iterators */
   unsigned int i, j;

   /* x and y fractional remainders */
   double xdiff = x - (int)x,
          ydiff = y - (int)y;

   /* Will eventually hold the interpolated pixel intensity */
   double p = 0;

   /* determine weights to be applied to each of the neighbouring pixels */
   for (i = 0; i < 4; i++)
   {
      double xdiff_power = pow(xdiff, i),
             ydiff_power = pow(ydiff, i);

      for (j = 0; j < 4; j++)
      {
         x_weights[j] += weights[i][j] * xdiff_power;
         y_weights[j] += weights[i][j] * ydiff_power;
      }
   }

   /* Weights computed; Next step is to perform convolution */

   /* set src_offset to point at the lower-left-most pixel */
   const uint8_t *src_offset = image + (int)x - 1 + ((int)y - 1) * width;

   /* iterate over y-coordinates of neighbouring pixels */
   for (i = 0; i < 4; i++)
   {
      if (x < 1)
      {
         /* Handle special case:
          * If we are interpolating a polar image and x (i.e. the circumferential index)
          * is less than 1, then the left-most neighbour is actually the last element
          * of the current row (since it loops around).
          */
         src_offset += width;
      }

      /* iterate over x-coordinates of neighbouring pixels */
      for (j = 0; j < 4; j++)
      {
         /* add weighted contribution of current neighbouring pixel */
         p += y_weights[i] * x_weights[j] * *src_offset++;

         /* Handle special situation for polar images where the circumferential index
          * increments past the last index of the current row and should loop back
          * to the first element of the row.
          */
         if ( (x < 1 && j == 0) || (int)x + j == width)
            src_offset -= width;
      }

      /* move to next row */
      src_offset += width - 4;

      if (x + 2 >= width)
      {
         /* if at some point we encountered the special situation above,
          * jump back to the left side of the zero-degree line.
          */
         src_offset += width;
      }

   }

   /* check bounds of result and return */
   return  (p > 255.0) ? (uint8_t)255 :
          ((p <   0.0) ? (uint8_t)0   : (uint8_t)(p + .5));
}

/*
 * Convert a rectilinear image to polar format.
*/
int
bicubic_rectilinear_to_polar(const uint8_t *rect_raster,
    const unsigned int width, const unsigned int height,
    uint8_t *polar_raster,
    const unsigned int num_circum_samples,
    const unsigned int num_radial_samples,
    const unsigned int polar_center_x, const unsigned int polar_center_y,
    const unsigned int inner_radius, const unsigned int outer_radius)
{

   /* nested iterators */
   double radial_index, angle_index;

   /* Each radial unit spans this many pixels in the rectilinear image */
   const double radial_resolution = (outer_radius - inner_radius) / (double)num_radial_samples;

   /* Each circumferal unit spans this many radians */
   const double circum_resolution = (M_PI + M_PI) / (double)num_circum_samples;

   /* do parameter checking... */

   if (inner_radius >= outer_radius) {
      ERRP("Inner radius larger than or equal to outer radius");
      return (-1);
   }

   /* foreach circumferal unit (rotate around counter-clockwise) */
   for (angle_index = 0; angle_index < num_circum_samples; angle_index++)
   {
      /* Add pi/2 to the angle so that zero degrees is at 6 o'clock rather than 3 o'clock */
      const double angle = M_PI_2 - angle_index * circum_resolution;

      const double COS = cos(angle),
                   SIN = sin(angle);

      /* foreach radial unit (moving out from the center) */
      for (radial_index = 0; radial_index < num_radial_samples; radial_index++)
      {
         const double radius = inner_radius + radial_index * radial_resolution;

         const double x = polar_center_x + COS * radius,
                      y = polar_center_y + SIN * radius;

         if (x < 1 || y < 1 || x >= width - 2 || y >= height - 2)
         {
            /* Coordinates are not within the rectilinear boundary, so pad with zero.
               Even if coordinates are within boundary, pad with zero if interpolation
               cannot be done. */
          
            *polar_raster = 0;

         } else
         {
            *polar_raster = bicubic_interpolation(rect_raster, width, height, x, y);
         }

         polar_raster += num_circum_samples;

      } /* end radial units loop */

      polar_raster -= (num_radial_samples * num_circum_samples) - 1;

   } /* end circumferal units loop */
   return (0);

}


/*
 * Convert polar image to rectilinear format.
 */
int
bicubic_polar_to_rectilinear(const uint8_t *polar_raster,
    const unsigned int num_circum_samples,
    const unsigned int num_radial_samples,
    uint8_t *rect_raster,
    const unsigned int width,	/* dimensions of the output	*/
    const unsigned int height,	/* reconstructed raster		*/
    const unsigned int polar_center_x,
    const unsigned int polar_center_y,
    const unsigned int inner_radius,
    const unsigned int outer_radius)
{
   /* nested iterators */
   unsigned int y, x;

   /* Each radial unit spans this many pixels in the rectilinear image */
   const double radial_resolution = (outer_radius - inner_radius) / (double)num_radial_samples;

   /* Each circumferal unit spans this many radians */
   const double circum_resolution = (M_PI + M_PI) / (double)num_circum_samples;

   /* do parameter checking... */

   if (inner_radius >= outer_radius) {
      ERRP("Inner radius larger than or equal to outer radius");
      return (-1);
   }


   for (y = 0; y < height; y++)
   {
      const int ydiff = (int)y - (int)polar_center_y;

      for (x = 0; x < width; x++)
      {
         const int xdiff = (int)x - (int)polar_center_x;
         const double radius = pow(ydiff * ydiff + xdiff * xdiff, .5);
         const double radial_index = (radius - inner_radius) / radial_resolution;

         if (radial_index < 1 || radial_index >= num_radial_samples - 2)
         {
            /* coordinates are not within the polar image, so pad with zero */
            *rect_raster = 0;

         } else {

            /* compute angle for (x,y), and put it into the range [0, 2*pi) */
            const double angle = fmod(M_PI + M_PI + M_PI_2 - atan2(ydiff, xdiff), M_PI + M_PI);

            const double circum_index = angle / circum_resolution;

            /* interpolate at the point (circum_index, radial_index) */
            *rect_raster = bicubic_interpolation(polar_raster, num_circum_samples, num_radial_samples, circum_index, radial_index);

         } /* end if */

         rect_raster++;

      } /* end foreach x */

   } /* end foreach y */

	return (0);
}

