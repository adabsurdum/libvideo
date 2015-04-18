
#include <stdio.h>
//#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
//#include <err.h>
//#include <string.h>

int pnm_write( FILE *fp, const uint8_t *rgb, int w, int h, const char *comment ) {

	int wr;

	if( comment )
		wr = fprintf( fp, "P3\n%d %d\n# %s\n255\n", w, h, comment  );
	else
		wr = fprintf( fp, "P3\n%d %d\n255\n", w, h  );

	if( wr < 0 )
		return -1;

	for(int r = 0; r < h; r++ ) {
		const char *sep = "";
		for(int c = 0; c < w; c++ ) {
			int red = *rgb++;
			int grn = *rgb++;
			int blu = *rgb++;
			fprintf( fp, "%s%d %d %d", sep, red, grn, blu );
			sep = " ";
		}
		fputc( '\n', fp );
	}
	return 0;
}


int pnm_save( const char *name, const uint8_t *buf, int w, int h, const char *comment ) {

	FILE *fp = fopen( name, "w" );
	if( fp ) {
		pnm_write( fp, buf, w, h, comment );
		fclose( fp );
		return 0;
	}
	return -1;
}

