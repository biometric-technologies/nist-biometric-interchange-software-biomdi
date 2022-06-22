/*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
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
Version		0.5.2
Last modified:	August 27, 2008
*/

/*
 * Convert rectilinear image to polar format.
 */
int
bilinear_rectilinear_to_polar(const uint8_t *rect_raster,
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
   const double circumferal_resolution = (M_PI + M_PI) / (double)num_circum_samples;

   /* do parameter checking */

   if (inner_radius >= outer_radius) {
      ERRP("Inner radius larger than or equal to outer radius");
      return (-1);
   }

   /* foreach circumferal unit (rotate around conter-clockwise) */
   for (angle_index = 0; angle_index < num_circum_samples; angle_index++)
   {
      /* add pi/2 to the angle so that zero degrees is at 6 o'clock rather than 3 o'clock */
      const double angle = M_PI_2 - angle_index * circumferal_resolution;

      const double COS = cos(angle),
                   SIN = sin(angle);

      /* foreach radial unit (moving outward from center) */
      for (radial_index = 0; radial_index < num_radial_samples; radial_index++)
      {
         const double radius = inner_radius + radial_index * radial_resolution;

         /* convert polar coordinates to rectilinear */
         const double x = polar_center_x + COS * radius,
                      y = polar_center_y + SIN * radius;

         if (x < 1 || y < 1 || x >= width - 1 || y >= height - 1)
         {
            /* coordinates are not within the rectilinear bounday, so pad with zero */
            *polar_raster = 0;

         } else
         {
            /* prepare for interpolation at (x,y) */

            /* define some variables for easy reference to four neighbouring pixels */
            const unsigned int xl = (int)x,
                               yl = (int)y,
	                       xh = xl + 1,
	                       yh = yl + 1;

            /* get fractional remainder */
            const double delta_x = x - xl,
                         delta_y = y - yl;

            /* do bilinear interpolation */
            *polar_raster = (uint8_t) (.5 + delta_x * delta_y * rect_raster[xh + yh * width] +
                                      (1 - delta_x) * delta_y * rect_raster[xl + yh * width] +
                                      delta_x * (1 - delta_y) * rect_raster[xh + yl * width] +
                                (1 - delta_x) * (1 - delta_y) * rect_raster[xl + yl * width]);
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
bilinear_polar_to_rectilinear(const uint8_t *polar_raster,
    const unsigned int num_circum_samples,
    const unsigned int num_radial_samples,
    uint8_t *rect_raster,
    const unsigned int width,	/* dimensions of the output	*/
    const unsigned int height,	/* rectilinear raster		*/
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

   /* do parameter checking */

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

         if (radial_index < 1 || radial_index > num_radial_samples - 2)
         {
            /* cordinates are not within the polar image, so pad with zero */
            *rect_raster = 0;

         } else {

            /* compute angle for (x, y), and put it into the range [0, 2*pi) */
            const double angle = fmod(M_PI + M_PI + M_PI_2 - atan2(ydiff, xdiff), M_PI + M_PI);

            const double circum_index = angle / circum_resolution;

            /* prepare for interpolation at (circum_index, radial_index) */

            /* define some variables for easy access to four neighbouring pixels */
            const unsigned int rl = (int)radial_index,
		 	       rh = rl + 1,
                               cl = (int)circum_index,
                               ch = (cl == num_circum_samples - 1) ? 0 : cl + 1;

            /* get fractional remainders */
            const double dc = circum_index - cl,
                         dr = radial_index - rl;

            /* do bilinear interpolation */
            const double p =       dc * dr * polar_raster[ch + rh * num_circum_samples] +
                             (1 - dc) * dr * polar_raster[cl + rh * num_circum_samples] +
                             dc * (1 - dr) * polar_raster[ch + rl * num_circum_samples] +
                       (1 - dc) * (1 - dr) * polar_raster[cl + rl * num_circum_samples] ;

            *rect_raster =  (p > 255.0) ? (uint8_t)255 :
                           ((p <   0.0) ? (uint8_t)0   : (uint8_t)(0.5 + p));

         } /* end if */

         rect_raster++;

      } /* end foreach x */

   } /* end foreach y */

	return (0);
}

