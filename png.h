
#ifndef _png_h_
#define _png_h_

int png_write( FILE *fp, const uint8_t *buf, int W, int H, const char *comment, int spp );
int png_save( const char *name, const uint8_t *buf, int W, int H, const char *comment, int spp );

#endif

