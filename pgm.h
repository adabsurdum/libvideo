
#ifndef _pgm_h_
#define _pgm_h_

int pgm_write( FILE *fp, const uint8_t *buf, int w, int h, const char *comment, int stride );
int pgm_save( const char *name, const uint8_t *buf, int w, int h, const char *comment, int stride );
int pgm_load( const char *name, int *w, int *h, uint8_t **img );

#endif

