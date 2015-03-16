
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

