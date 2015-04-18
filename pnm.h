
#ifndef _pnm_h_
#define _pnm_h_

int pnm_write( FILE *fp, const uint8_t *rgb, int w, int h, const char *comment );
int pnm_save( const char *name, const uint8_t *buf, int w, int h, const char *comment );

#endif

