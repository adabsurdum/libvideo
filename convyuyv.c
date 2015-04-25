
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <err.h>

#include "yuyv.h"
#include "pnm.h"
#include "png.h"

int main( int argc, char *argv[] ) {

	FILE *ifp = NULL, *ofp = NULL;
	const char *iname = NULL, *oname = NULL;
	int w = 0, h = 0;
	uint16_t *yuyv = NULL;
	uint8_t  *obuf = NULL;

	int  spp = 3;     // ...assuming RGB format
	bool png = true;  // ...assuming PNG target

	do {
		const char c = getopt( argc, argv, "gw:h:" );
		if( c < 0 ) break;
		switch(c) {
		case 'g': spp = 1;            break;
		case 'w': w = atoi( optarg ); break;
		case 'h': h = atoi( optarg ); break;
		default:
			printf ("error: unknown option: %c\n", c );
			exit(-1);
		}
	} while(1);

	if( optind + 2 == argc ) {
		iname = argv[ optind++ ];
		oname = argv[ optind++ ];
	} else {
		printf( "%s [ -g ] -w <width> -h <height> <input file> <output file>\n", argv[0] );
		exit(-1);
	}

	if( strncasecmp( oname + strlen(oname) - 3, "png", 3 ) ) {
		png = false;
		// ...in which case we do PNM or PGM according to whether it's
		// RGB or gray...
	}

	yuyv = calloc( w*h,     sizeof(uint16_t) );
	obuf = calloc( w*h*spp, sizeof(uint8_t) );

	if( yuyv != NULL && obuf != NULL ) {

		ifp = fopen( iname, "r" );
		ofp = fopen( oname, "wb" );

		if( ifp != NULL && ofp != NULL ) {

			static char comment[256];

			const int NPX = w*h;

			if( fread( yuyv, sizeof(uint16_t), NPX, ifp ) != NPX )
				err( -1, "reading %s", iname );
			fclose( ifp );

			if( spp == 3 )
				yuyv2rgb(  yuyv, w, h, obuf );
			else
				yuyv2gray( yuyv, w, h, obuf );

			sprintf( comment, "converted by %s", argv[0] );
			if( png ) {
				png_write( ofp, obuf, w, h, comment, spp );
			} else
			if( spp == 1 ) {
				//pgm_write( ofp, obuf, w, h, comment );
			} else
			if( spp == 3 ) {
				pnm_write( ofp, obuf, w, h, comment );
			} else
				err( -1, "what?" );

			fclose( ofp );

		} else
			err( -1, "opening one or both of %s and %s", iname, oname );
	}

	if( obuf )
		free( obuf );

	if( yuyv )
		free( yuyv );

	return EXIT_SUCCESS;
}

