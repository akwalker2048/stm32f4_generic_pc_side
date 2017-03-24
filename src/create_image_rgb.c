#include <stdint.h>
#include <stdio.h>

#include "create_image_rgb.h"
#include "palettes.h"


int create_image_rgb(uint16_t *pixels, uint16_t num_cols, uint16_t num_rows, uint8_t image_num)
{

   char fname[2048];
   int cc, rr;
   FILE *fp;
   uint8_t color[3];

   uint16_t pixel_value;
   uint16_t max_value, min_value;
   float scale;
   float diff;
   int index;

   snprintf(fname, 2047, "./output/image_%03u.ppm", image_num);
   fp = fopen(fname, "wb"); /* b - binary mode */
   fprintf(fp, "P6\n%d %d\n255\n", num_cols, num_rows);



   min_value = 0xFFFF;
   max_value = 0;
   for (rr=0; rr<num_rows; rr++)
   {
      for (cc=0; cc<num_cols; cc++)
      {
         index = get_index(cc, rr);
         if(pixels[index] > max_value)
         {
            max_value = pixels[index];
         }

         if(pixels[index]< min_value)
         {
            min_value = pixels[index];
         }
      }
   }


   diff = (float)max_value - (float)min_value;
   scale = 255/diff;

   printf("max = %u, min = %u\n", max_value, min_value);
   printf("diff = %f, scale = %f\n", diff, scale);


   for (rr = 0; rr < num_rows; rr++)
   {
      for (cc = 0; cc < num_cols; cc++)
      {
         index = get_index(cc, rr);
         pixel_value = (uint16_t)((pixels[index]-min_value)*scale);
         if(pixel_value >= 255)
         {
            pixel_value = 255;
         }
         color[0] = colormap_ironblack[pixel_value*3];  /* red */
         color[1] = colormap_ironblack[pixel_value*3+1];  /* green */
         color[2] = colormap_ironblack[pixel_value*3+2];  /* blue */
         (void) fwrite(color, 1, 3, fp);
      }
   }
   (void) fclose(fp);

   return 0;
}
