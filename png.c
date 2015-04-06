#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int png_write( FILE *fp, const uint8_t *buf, int W, int H, const char *comment, int stride ) {

	png_structp pws = NULL;
	png_infop ppi = NULL;
	png_byte ** row_pointers = NULL;

	pws = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if( pws == NULL) {
		return -1;
	}

	ppi = png_create_info_struct( pws );
	if( ppi == NULL) {
		png_destroy_write_struct( &pws, &ppi );
		return -1;
	}

	if( setjmp( png_jmpbuf(pws) ) ) {
		goto failed_setjmp;
	}

	png_set_IHDR( pws, ppi, W, H, 8,
		PNG_COLOR_TYPE_GRAY,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	row_pointers
		= png_malloc( pws, H * sizeof(png_byte *) );
	for(int y = 0; y < H; ++y ) {
		png_byte *row = 
			png_malloc( pws, sizeof(uint8_t) * W  );
		row_pointers[y] = row;
		for(int x = 0; x < W; ++x) {
			*row++ = *buf;
			buf += stride;
		}
	}

	png_init_io( pws, fp );
	png_set_rows( pws, ppi, row_pointers );
	png_write_png( pws, ppi, PNG_TRANSFORM_IDENTITY, NULL );

	for(int y = 0; y < H; y++ )
		png_free( pws, row_pointers[y] );
	png_free( pws, row_pointers );

failed_setjmp:

	png_destroy_write_struct( &pws, &ppi );

	return 0;
}

int png_save( const char *name, const uint8_t *buf, int W, int H, const char *comment, int stride ) {
	int status = -1;
	FILE *fp = fopen( name, "wb" );
	if( fp ) {
		status =  png_write( fp, buf, W, H, comment, stride );
		fclose( fp );
	}
	return status;
}

