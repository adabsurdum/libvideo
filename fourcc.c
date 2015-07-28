
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
#include "fourcc.h"

const char *fourcc_string( uint32_t fcc ) {
	static char buf[5];
	buf[0] = (fcc >>  0) & 0xff;
	buf[1] = (fcc >>  8) & 0xff;
	buf[2] = (fcc >> 16) & 0xff;
	buf[3] = (fcc >> 24) & 0xff;
	buf[4] = '\0';
	return buf;
}

uint32_t fourcc_integer( const char *s ) {
	uint32_t part, val = s[0];
	part = s[1];
	val |= (0x0000FF00 & (part <<  8) );
	part = s[2];
	val |= (0x00FF0000 & (part << 16) );
	part = s[3];
	val |= (0xFF000000 & (part << 24) );
	return val;
}

