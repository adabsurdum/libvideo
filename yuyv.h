
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

#ifndef _yuyv_h_
#define _yuyv_h_

void yuyv2gray( const uint16_t *yuyv, int w, int h, uint8_t *o );
void yuyv2rgb( const uint16_t *yuyv, int w, int h, uint8_t *o );

#endif

