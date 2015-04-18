
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <err.h>
#include "pnm.h"

extern void yuyv2rgb( const uint16_t *yuyv, int w, int h, uint8_t *o );

int main( int argc, char *argv[] ) {

	FILE *ifp = NULL, *ofp = NULL;
	const char *iname = NULL, *oname = NULL;
	int w = 0, h = 0;
	uint16_t *yuyv = NULL;
	uint8_t  *xrgb = NULL;

	do {
		const char c = getopt( argc, argv, "w:h:" );
		if( c < 0 ) break;
		switch(c) {
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
		printf( "%s -w <width> -h <height> <input file> <output file>\n", argv[0] );
		exit(-1);
	}

	yuyv = calloc( w*h,  sizeof(uint16_t) );
	xrgb = calloc( w*h*3, sizeof(uint8_t) );

	if( yuyv != NULL && xrgb != NULL ) {

		ifp = fopen( iname, "r" );
		ofp = fopen( oname, "wb" );

		if( ifp != NULL && ofp != NULL ) {

			const int NPX = w*h;
			if( fread( yuyv, sizeof(uint16_t), NPX, ifp ) != NPX )
				err( -1, "reading %s", iname );
			fclose( ifp );

			yuyv2rgb( yuyv, w, h, xrgb );

			pnm_write( ofp, xrgb, w, h, "converted by yuyv2pnm" );
			fclose( ofp );

		} else
			err( -1, "opening one or both of %s and %s", iname, oname );
	}

	if( xrgb )
		free( xrgb );

	if( yuyv )
		free( yuyv );

	return EXIT_SUCCESS;
}

