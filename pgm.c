
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>
#include <string.h>

#include "pgm.h"

/**
  * The stride element allows an image with 1-byte pixel data to be saved
  * directly from a buffer that may have multi-byte pixels (without making
  * two passes over the buffer).
  */
int pgm_write( FILE *fp, const uint8_t *buf, int w, int h, const char *comment, int stride ) {
	const int N = w*h;
	int wr;
	if( comment )
		wr = fprintf( fp, "P5\n%d %d\n# %s\n255\n", w, h, comment  );
	else
		wr = fprintf( fp, "P5\n%d %d\n255\n", w, h  );
	if( wr < 11 )
		errx( -1, "error: writing PGM header (only %d chars)", wr );

	if( stride == 1 )
		return fwrite( buf, sizeof(uint8_t), N, fp ) == N ? 0 : -1;
	else {
		int n = w*h;
		while( n-- > 0 ) {
			if( fputc( *buf, fp ) == EOF ) return -1;
			buf+=stride;
		}
	}
	return 0;
}

/**
  * Saves in PGM format to the specified file name.
  */
int pgm_save( const char *name, const uint8_t *buf, int w, int h, const char *comment, int stride ) {

	FILE *fp = fopen( name, "w" );
	if( fp ) {
		pgm_write( fp, buf, w, h, comment, stride );
		fclose( fp );
		return 0;
	}
	return -1;
}


static int nextToken( FILE *fp, char *buf, int buflen ) {
	int n = 0;
	--buflen;
	while( n < buflen ) {
		int c = fgetc(fp);
		if( isspace(c) ) {
			if( n > 0 )
				break;
			else
				continue;
		} else
		if( '#' == c ) {
			// Eat the comment (to the end-of-line).
			do {
				c = fgetc(fp);
			} while( c != '\n' && c != '\r' );
			if( n > 0 )
				break;
			else
				continue;
		}
		buf[n++] = (char)c;
	}
	buf[n] = 0;
	return n;
}


int pgm_load( const char *name, int *w, int *h, uint8_t **img ) {
	int n, maxval = -1;
	FILE *fp = fopen( name, "r" );
	if( fp ) {
		char buf[32];
		n = nextToken( fp, buf, 32 );
		if( strcmp( buf, "P5" ) ) {
			errx( 0, "File %s has signature \"%s\"; expected \"P5\"", name, buf );
			return -1;
		}
		nextToken( fp, buf, 32 );
		*w = atoi( buf );
		nextToken( fp, buf, 32 );
		*h = atoi( buf );
		nextToken( fp, buf, 32 );
		maxval = atoi( buf );
		n = (*w)*(*h);
		if( maxval < 256 ) {
			*img = calloc( n, sizeof(uint8_t)  );
			if( fread( *img, sizeof(uint8_t), n, fp ) != n ) {
				free( *img );
				return -1;
			}
		} else {
			*img = calloc( n, sizeof(uint16_t) );
			if( fread( *img, sizeof(uint16_t), n, fp ) != n ) {
				free( *img );
				return -1;
			}
		}
		fclose( fp );
	}
	return maxval;
}

#ifdef UNIT_TEST_PGM
int main( int argc, char *argv[] ) {

	int w, h;
	uint8_t *buf;
	if( pgm_load( argv[1], &w, &h, &buf ) < 0 ) {
		free( buf );
	}
	return 0;
}
#endif

