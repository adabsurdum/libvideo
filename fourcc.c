
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

