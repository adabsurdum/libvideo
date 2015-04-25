
#ifndef _yuyv_h_
#define _yuyv_h_

void yuyv2gray( const uint16_t *yuyv, int w, int h, uint8_t *o );
void yuyv2rgb( const uint16_t *yuyv, int w, int h, uint8_t *o );

#endif

