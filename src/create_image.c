#include <pgm.h>
#include <stdint.h>

#include "create_image.h"
#include "palettes.h"

/* reduce the image intensity by 1/2 */
int create_image(uint16_t *pixels, uint16_t num_cols, uint16_t num_rows, uint8_t image_num)
{
   uint16_t pixel_value;
   uint16_t max_value, min_value;
   float scale;
   float diff;
   int index;

   FILE *output;
   char fname[2048];

   char *argv[] = {"program_name", "arg1", "arg2", NULL};
   int argc = sizeof(argv) / sizeof(char*) - 1;

    /* image is a pointer to a 2d array that we will point at an array that
     * pgm_readpgm will allocate */
    gray **image;

    /* This will be set to the maximum value of our input image, probably
     * 255 */
    gray max;

    /* These will be set to the number of rows and columns in the input
     * (and output) image. */
    /* int cols, rows; */

    /* Indexes that we will use to access pixels at y,x */
    int y, x;

    /* initialize libpgm. pgm_init wants argv and a pointer to argc, which
     * is why we don't just do int main(void) above. */
    pgm_init(&argc, argv);

    /* read the image from stdin. the data is in image, and the number of
     * cols and rows, and the maximum intensity value are saved in the
     * respective variables. */
    /* image = pgm_readpgm(stdin, &cols, &rows, &max); */

    max = 0xFF;
    image = pgm_allocarray(num_rows, num_cols);

    /* make image half as intense (darker) */
    min_value = 0xFFFF;
    max_value = 0;
    for (y=0; y<num_rows; y++)
    {
        for (x=0; x<num_cols; x++)
        {
           index = get_index(x, y);
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

    for (y=0; y<num_rows; y++)
    {
        for (x=0; x<num_cols; x++)
        {
           index = get_index(x, y);
           pixel_value = (uint16_t)((pixels[index]-min_value)*scale);
           if(pixel_value >= 255)
           {
              pixel_value = 255;
           }
           image[y][x] = colormap_grayscale[pixel_value*3];
        }
    }

    /* write the modified image to stdout */
    snprintf(fname, 2047, "./output/image_%03u.ppm", image_num);
    output = fopen(fname, "w");
    if(output != NULL)
    {
       pgm_writepgm(output, image, num_rows, num_cols, max, 1);
       fclose(output);
    }

    /* cleanup */
    pgm_freearray(image, num_rows);

    /* Success */
    return 0;
}


int get_index(int col, int row)
{
   int index;

   if(col >= VOSPI_COLS)
   {
      col = (VOSPI_COLS-1);
   }

   index = row*VOSPI_ROWS + col;

   return index;
}
