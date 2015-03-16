
#ifndef _video_h_
#define _video_h_

struct video_frame;
struct video_format;

/**
  * All supported formats' pixel sizes should be defined below.
  */
#define SIZEOF_PIXEL_YUYV (2)
#define VIDEO_DEQ_TIMEOUT_DEFAULT (0)
#define VIDEO_DEQ_TIMEOUT_NONE    (-1)

struct video_capture {

	const struct video_format *(*format)( struct video_capture * );

	int   (*config)( struct video_capture *, struct video_format *, int );

	int   (*snap)( struct video_capture *, int *len, unsigned char **frame );

	int   (*start)( struct video_capture * );

	/**
	  * Integer value identifies a *single* buffer to be enqueued.
	  */
	int   (*enqueue1)( struct video_capture *, int buffer_id );

	/**
	  * Bits in flags correspond to buffers 0..31.
	  * Multiple buffers can be enqueued in one enqueue call.
	  */
	int   (*enqueue)( struct video_capture *, unsigned int buffer_flags );

	/**
	  * Dequeues exactly one of enqueued buffers, if any are available
	  * to be dequeued.
	  */
	int   (*dequeue)( struct video_capture *, int timeout, 
			struct video_frame * /* out */ );

	int   (*stop)(    struct video_capture * );

	void  (*destroy)( struct video_capture * );
};

struct video_capture *video_open( const char *devpath );
void yuyv_to_gray( const uint16_t *yuyv, int w, int h, uint8_t *o );
int first_video_dev( char *path, int maxlen );


#endif

