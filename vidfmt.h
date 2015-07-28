
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

#ifndef _vidfmt_h_
#define _vidfmt_h_

/**
  * This is a subset of the parameters supported by V4L2.
  * Since I only intend to support one or two camera models, this is need
  * not be nearly as general-purpose as might otherwise be required.
  * Other parameters can be compile-time constants.
  *
  * The format actually used at runtime is potentially a compromise
  * between a user-requested format and hardware capabilities.
  */
struct video_format {
	unsigned /*short*/ width;
	unsigned /*short*/ height;
	char pixel_format[ 4 + 1 /* allow for NUL term */ ];
};

#endif

