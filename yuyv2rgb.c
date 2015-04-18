
#include <stdint.h>

#ifdef HAVE_FLOAT_CONVERSION
static inline uint8_t clampf( float v ) {
	if( v < 0.0 )
		return 0;
	else
	if( v > 255.0 )
		return 255;
	else
		return (uint8_t)v;
}
#else
static inline uint8_t clampi( int v ) {
	if( v < 0 )
		return 0;
	else
	if( v > 255 )
		return 255;
	else
		return (uint8_t)v;
}
#endif

static inline void YUV2RGB( int y, int u, int v, uint8_t *rgb ) {
#ifdef HAVE_FLOAT_CONVERSION
	rgb[0] = clampf( y +                 1.402*(u-128) );
	rgb[1] = clampf( y - 0.344*(v-128) - 0.714*(u-128) );
	rgb[2] = clampf( y + 1.772*(v-128)                 );
#else
	const int C = y -  16;
	const int D = u - 128;
	const int E = v - 128;
	rgb[0] = clampi( (298*C         + 409*E + 128) >> 8 );
	rgb[1] = clampi( (298*C - 100*D - 208*E + 128) >> 8 );
	rgb[2] = clampi( (298*C + 516*D         + 128) >> 8 );
#endif
}

void yuyv2rgb( const uint16_t *yuyv, int w, int h, uint8_t *o ) {

	for(int r = 0; r < h; r++ ) {
#if 0
		/**
		  * ...grayscale rendered as 24-bit color in the process.
		  */
		for(c = 0; c < w; c++ ) {
			const uint8_t GRAY
				= (uint8_t)(0x00FF & yuyv[ r*w + c ]);
			o[ 4*w*r + 4*c+2 ] = GRAY; // red
			o[ 4*w*r + 4*c+1 ] = GRAY; // green
			o[ 4*w*r + 4*c+0 ] = GRAY; // blue
		}
#else
		/**
		  * ...24-bit color in the process.
		  */
		const uint8_t *iline
			= (const uint8_t*)(yuyv  + r*w );
		uint8_t *oline = o + r*w*3;
		for(int c = 0; c < 2*w; c+=4 ) {
			const int Y0 = iline[c+0];
			const int U  = iline[c+1];
			const int Y1 = iline[c+2];
			const int V  = iline[c+3];
			YUV2RGB( Y0, U, V, oline + 3*(c/2+0) );
			YUV2RGB( Y1, U, V, oline + 3*(c/2+1) );
		}
#endif
	}
}

