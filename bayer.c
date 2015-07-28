
/**
  * Simple wrapper API for accessing imaging devices through the V4L2
  * API (Linux only) 
  * Copyright (C) 2015  Roger Kramer
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */

#include <stdint.h>

/**
  * BA81 format according to 
  * http://linuxtv.org/downloads/v4l-dvb-apis/V4L2-PIX-FMT-SBGGR8.html
  * Byte Order. Each cell is one byte.
  * start + 0:	B00	G01	B02	G03
  * start + 4:	G10	R11	G12	R13
  * start + 8:	B20	G21	B22	G23
  * start + 12:	G30	R31	G32	R33
  */

int ba81_to_rgb( const uint8_t *buf, int w, int h, uint8_t *rgb ) {

	int r,c;
	unsigned R,G,B;

	// Currently skipping the edges to avoid the nuisance boundary
	// interpolation conditions.

	buf += w; // skip top raster row
	for(r = 1; r < h-1; r++ ) {
		buf++; // skip first raster column
		if( r & 1 ) {
			//  0 1010
			// [B]GBGB
			// [G]rGRG <=
			// [B]GBGB
			for(c = 1; c < w-1; c++ ) {
				if( c & 1 ) {
					G = ( *(buf-w  ) + *(buf+w  ) + *(buf  -1) + *(buf  +1) ) / 4;
					B = ( *(buf-w-1) + *(buf-w+1) + *(buf+w-1) + *(buf+w+1) ) / 4;
					R = *buf++;
				} else {
					R = ( *(buf-1) + *(buf+1) ) / 2;
					B = ( *(buf-w) + *(buf+w) ) / 2;
					G = *buf++;
				}
				// ready to emit R G B
				rgb[w*r+c] = (uint8_t)( (R+G+B)/3.0 );
			}
		} else {
			//  0 1010
			// [G]RGRG
			// [B]gBGB <=
			// [G]RGRG
			for(c = 1; c < w-1; c++ ) {
				if( c & 1 ) {
					R = ( *(buf-w) + *(buf+w) ) / 2;
					B = ( *(buf-1) + *(buf+1) ) / 2;;
					G = *buf++;
				} else {
					R = ( *(buf-w-1) + *(buf-w+1) + *(buf+w-1) + *(buf+w+1) ) / 4;
					G = ( *(buf-w  ) + *(buf+w  ) + *(buf  -1) + *(buf  +1) ) / 4;
					B = *buf++;
				}
				// ready to emit R G B
				rgb[w*r+c] = (uint8_t)( (R+G+B)/3.0 );
			}
		}
		buf++; // skip last raster column
	}
	// ignore last buf raster row
	return 0;
}

