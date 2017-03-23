#define VOSPI_ROWS 60
#define VOSPI_COLS 80

int create_image(uint16_t *pixels, uint16_t num_cols, uint16_t num_rows, uint8_t image_num);
int get_index(int col, int row);

/* Useful command line stuff. */
/* convert -delay 6 -quality 95 image_*ppm movie.mpg */
/* vlc movie.mpg */
