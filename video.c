
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
  *
  *
  * This module implements all hardware-aware and hardware-dependent
  * management of the video system.
  *
  * It is currently (and intentionally) tied tightly to the Video 4 Linux 2
  * (V4L2) API, and it insulates other code as much as possible (entirely?)
  * from the details of the video subsystem.
  *
  * Conversely, to the greatest extent possible everything else, not
  * directly related to V4L2 management of a video stream, is kept OUT of
  * this module.
  *
  * It also provides the implementation of the "replay" mode for offline
  * analysis of images. That is, it can read in a series of PGM images
  * and deliver them to sm.c as if they were dequeued video frame.
  *
  * Finally, the unit test for this module constitutes just about the
  * simplest possible code to stream video to an X11 window...useful in
  * its own right.
  */

/**
  * Snippets of documentation are included from:
  *
  * http://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-\
  * single/v4l2.html  * Video for Linux Two API Specification
  *
  * Revision 2.6.32
  *
  * Michael H Schimek
  *
  *     <mschimek@gmx.at>
  *
  * Bill Dirks
  *
  * Original author of the V4L2 API and documentation.
  * Hans Verkuil
  *
  * Designed and documented the VIDIOC_LOG_STATUS ioctl, the extended
  * control ioctls and major parts of the sliced VBI API.
  *
  *     <hverkuil@xs4all.nl>
  *
  * Martin Rubli
  *
  * Designed and documented the VIDIOC_ENUM_FRAMESIZES and
  * VIDIOC_ENUM_FRAMEINTERVALS ioctls.
  * Andy Walls
  *
  * Documented the fielded V4L2_MPEG_STREAM_VBI_FMT_IVTV MPEG stream
  * embedded, sliced VBI data format in this specification.
  *
  *     <awalls@radix.net>
  *
  * Mauro Carvalho Chehab
  *
  * Documented libv4l, designed and added v4l2grab example, Remote
  * Controller chapter
  *
  *     <mchehab@redhat.com>
  *
  * Copyright (c) 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
  * 2008, 2009 Bill Dirks, Michael H. Schimek, Hans Verkuil, Martin Rubli,
  * Andy Walls, Mauro Carvalho Chehab
  *
  * This document is copyrighted © 1999-2009 by Bill Dirks, Michael H.
  * Schimek, Hans Verkuil, Martin Rubli, Andy Walls and Mauro Carvalho
  * Chehab.
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <fcntl.h>     // for O_RDWR | O_NONBLOCK
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <err.h>
#include <assert.h>

#include <linux/videodev2.h> // for VIDEO_MAX_FRAME, struct v4l2_buffer, etc.

#include "video.h"
#include "vidfrm.h"
#include "vidfmt.h"
#include "fourcc.h"

#define USE_SELECT (1)

/**
  * externs
  */


/**
  * The minimal information returned by VIDIOC_QUERYBUF necessary
  * to support buffer (un)mapping via mmap and munmap.
  */
struct frame_buffer {
	int    index;
	void  *address;
	size_t length;
};

#define MAXLEN_DEVPATH (63)

struct video_state {

	struct video_capture interface;

	/**
	  * The device name and file descriptor
	  */
	char name[ MAXLEN_DEVPATH+1 ];

	/**
	  * The file descriptor for driver access.
	  */
	int fd;

	/**
	  * Configured video.
	  */
	struct video_format format;

	long dequeue_timeout;

	/**
	  * bit flags indicating which of frames are queued
	  * (owned by kernel)
	  */
	uint32_t queued;

	/**
	  * The items in .frame correspond (in index) to items in a similar
	  * array maintained in the V4L2 kernel driver. We use the indices
	  * when communicating (de)queue intentions to the kernel.
	  */
	int frame_count;

	struct frame_buffer frame[ VIDEO_MAX_FRAME ];
};
typedef struct video_state video_state_t;
typedef const video_state_t VIDEO_STATE_T;

/***************************************************************************
  * Private helpers
  */

#ifdef _DEBUG
static void _dump( struct video_state *vs, FILE *fp) {

	fprintf( fp,
		"video_state {\n"
		"   name: %s\n"
		"     fd: %d\n"
		"\tvideo_format {\n"
		"\t   width: %d\n"
		"\t  height: %d\n"
		"\t  format: %s\n"
		"\t}\n"
		"deq.timeout: %ld\n"
		"     queued: %08X\n"
		"frame_count: %d\n",
		vs->name,
		vs->fd,
		vs->format.width,
		vs->format.height,
		vs->format.pixel_format,
		vs->dequeue_timeout,
		vs->queued,
		vs->frame_count );
	for(int i = 0; i < vs->frame_count; i++ ) {
		fprintf( fp, "frames[%d].length: %ld\n", i, vs->frame[i].length );
	}
	fprintf( fp, "}\n" );
}
#endif


/**
  * Interruptible ioctl.
  */
static inline int iioctl( int fh, int request, void *arg ) {
	int r;
	do {
		r = ioctl( fh, request, arg );
	} while( r < 0 && EINTR == errno );
	return r;
}


static int _unmap_frames( int n, struct frame_buffer *arr ) {
	int err = 0;
	while( n-- > 0 ) {
		void *address
			= arr[n].address;
		off_t length
			= arr[n].length;
		if( munmap( address, length ) < 0 ) {
			warn( "munmap( %p, %ld )", 
				address, length ); // ...but keep going.
			err = -1;
		}
	}
	return err;
}


/**
  * This requests V4L2 to create some number of kernel buffers
  * and then maps those buffers into user space for copy-free
  * access to video frames.
  */
static int _map_frames( int fd, int count, struct frame_buffer *frame ) {

	int n = 0;

	struct v4l2_requestbuffers req = {
		.count  = count,
		.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.memory = V4L2_MEMORY_MMAP,
		.reserved = {
			0,
			0
		}
	};

	if( iioctl( fd, VIDIOC_REQBUFS, &req) < 0 ) {
		warn( "VIDIOC_REQBUFS(%d)\n", count );
		return -1;
	}

	for(n = 0; n < req.count; ++n ) {

		struct v4l2_buffer buf = {
			.index = n,
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			//.bytesused = 0,
			//.flags = 0,
			//.field = 0,
			//.timestamp, // struct timeval
			//.timecode,  // struct v4l2_timecode	
			//.sequence = 0,
			.memory = V4L2_MEMORY_MMAP,
			//.m.offset = 0,
			//.length   = 0,
			//.input    = 0,
			//.reserved = 0
		};
		/*memset( &buf, 0, sizeof(buf) );
		buf.index = n;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;*/

		if( iioctl( fd, VIDIOC_QUERYBUF, &buf ) < 0 ) {
			warn("VIDIOC_QUERYBUF");
			goto failure;
		}

		//assert( (buf.flags & V4L2_BUF_FLAG_MAPPED ) != 0 );
		assert( (buf.flags & V4L2_BUF_FLAG_QUEUED ) == 0 );
		assert( (buf.flags & V4L2_BUF_FLAG_DONE   ) == 0 );

		frame[ n ].address
			= mmap(NULL,                // start anywhere (in userspace)
				buf.length,             // dictated by driver
				PROT_READ | PROT_WRITE, // required
				MAP_SHARED,             // recommended
				fd, 
				buf.m.offset );         // a "handle" provided by driver

		if( MAP_FAILED == frame[ n ].address ) {
			warn( "mmap( NULL, length=%d,..., fd=%d, offset=%d )", 
				buf.length, fd, buf.m.offset );
			goto failure;
		}
		frame[ n ].length = buf.length;
	}

	return n;

failure:

	_unmap_frames( n, frame ); // ...whatever has been mapped.

	return -1;
}


static inline bool _is_queued( VIDEO_STATE_T *vs, int i ) {
	assert( 0 <= i && i < VIDEO_MAX_FRAME );
	return ( vs->queued & (1<<i) ) != 0;
}

static inline void _set_unqueued( video_state_t *vs, int i ) {
	assert( 0 <= i && i < VIDEO_MAX_FRAME );
	vs->queued &= (~(1 << i));
}

/***************************************************************************
  * Public interface
  */

const struct video_format * _format( struct video_capture *vci ) {
	struct video_state *vs
		= (struct video_state*)vci;
	return & vs->format;
}


static int _config( struct video_capture *vci, struct video_format *pref, int n ) {

	struct video_state *vs
		= (struct video_state*)vci;
	int i, selection = -1;
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	for(i = 0; i < n; i++ ) {

		const struct video_format *vf
			= pref + i;
		const uint32_t FOURCC_CODE
			= fourcc_integer( vf->pixel_format );

		fmt.fmt.pix.width	    = vf->width;
		fmt.fmt.pix.height	    = vf->height;
		fmt.fmt.pix.pixelformat = FOURCC_CODE;
		fmt.fmt.pix.field	    = V4L2_FIELD_NONE;
		//fmt.fmt.pix.bytesperline u32
		//fmt.fmt.pix.sizeimage    u32
		//fmt.fmt.pix.colorspace   enum
		//fmt.fmt.pix.priv         u32

		if( iioctl( vs->fd, VIDIOC_S_FMT, &fmt) < 0 ) {
			warn("setting video format: %dx%d,%s", 
				vf->width, 
				vf->height, 
				vf->pixel_format );
			continue;
		}

		/**
		  * We seem to have succeeded but...according to docs
		  * VIDIOC_S_FMT may change width and height. This isn't
		  * considered an error, but it's an error to us!
		  * If any of the returned values don't match what we sent
		  * in, move on to the next format and try again.
		  * If all match we found a winner and can quit.
		  */

		static const char *REQRCV
			= "requested %s: 0x%08x, received 0x%08x";

		if( true /* exact */ ) {
			if( vf->width != fmt.fmt.pix.width ) {
				warnx( REQRCV, "width", vf->width, fmt.fmt.pix.width );
				continue;
			}
			if( vf->height != fmt.fmt.pix.height ) {
				warnx( REQRCV, "height", vf->height, fmt.fmt.pix.height );
				continue;
			}
			if( FOURCC_CODE != fmt.fmt.pix.pixelformat ) {
				warnx( REQRCV, "pixel format", vf->pixel_format, fmt.fmt.pix.pixelformat );
				continue;
			}
		}

		// TODO: Revisit: Validate following struct v4l2_format members:
		// fmt.fmt.pix.bytesperline
		// fmt.fmt.pix.sizeimage
		// fmt.fmt.pix.colorspace

		selection = i;
		break;
	}

	if( 0 <= selection ) {
		// Set the video format
		vs->format.width  = fmt.fmt.pix.width;
		vs->format.height = fmt.fmt.pix.height;
		strcpy(
			vs->format.pixel_format,
			fourcc_string( fmt.fmt.pix.pixelformat ) );
	}

#if 0
	// Preserve original settings as, for example, might have
	// been set by v4l2-ctl.
	if( iioctl( fd, VIDIOC_G_FMT, &fmt ) < 0 ) {
		warn("getting video format");
		goto unwind0;
	}
#endif

	vs->frame_count
		= _map_frames( vs->fd, VIDEO_MAX_FRAME, vs->frame );

	if( vs->frame_count <= 0 )
		return -2;
#ifdef _DEBUG
	_dump( vs, stdout );
#endif

	return selection;
}


/**
  * The VIDIOC_STREAMON and VIDIOC_STREAMOFF ioctl start and stop the
  * capture or output process during streaming (memory mapping or user
  * pointer) I/O.
  *
  * Specifically the capture hardware is disabled and no input buffers are
  * filled (if there are any empty buffers in the incoming queue) until
  * VIDIOC_STREAMON has been called. Accordingly the output hardware is
  * disabled, no video signal is produced until VIDIOC_STREAMON has been
  * called. The ioctl will succeed only when at least one output buffer is
  * in the incoming queue.
  *
  * Both ioctls take a pointer to an integer, the desired buffer or stream
  * type. This is the same as struct v4l2_requestbuffers type.
  *
  * Note applications can be preempted for unknown periods right before or
  * after the VIDIOC_STREAMON or VIDIOC_STREAMOFF calls, there is no notion
  * of starting or stopping "now". Buffer timestamps can be used to
  * synchronize with other events.
  */
static int _start( struct video_capture *vci ) {

	struct video_state *vs
		= ( struct video_state*)vci;

	int argv = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if( iioctl( vs->fd, VIDIOC_STREAMON, &argv ) < 0 ) {
		warn( "VIDIOC_STREAMON" );
		return -1;
	}
	return 0; 
}


/**
  * The VIDIOC_STREAMOFF ioctl, apart of aborting or finishing any DMA in
  * progress, unlocks any user pointer buffers locked in physical memory,
  * and it removes all buffers from the incoming and outgoing queues. That
  * means all images captured but not dequeued yet will be lost, likewise
  * all images enqueued for output but not transmitted yet. I/O returns to
  * the same state as after calling VIDIOC_REQBUFS and can be restarted
  * accordingly.
  */
static int _stop( struct video_capture *vci ) {

	struct video_state *vs
		= ( struct video_state*)vci;

	int argv = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if( iioctl( vs->fd, VIDIOC_STREAMOFF, &argv ) < 0 ) {
		warn( "VIDIOC_STREAMOFF" );
		return -1;
	}
	vs->queued = 0;
	return 0; 
}


/**
  * Applications call the VIDIOC_QBUF ioctl to enqueue an empty (capturing)
  * or filled (output) buffer in the driver's incoming queue. The semantics
  * depend on the selected I/O method.
  *
  * To enqueue a memory mapped buffer applications set the type field of a
  * struct v4l2_buffer to the same buffer type as previously struct
  * v4l2_format type and struct v4l2_requestbuffers type, the memory field
  * to V4L2_MEMORY_MMAP and the index field. Valid index numbers range from
  * zero to the number of buffers allocated with VIDIOC_REQBUFS (struct
  * v4l2_requestbuffers count) minus one. The contents of the struct
  * v4l2_buffer returned by a VIDIOC_QUERYBUF ioctl will do as well. When
  * the buffer is intended for output (type is V4L2_BUF_TYPE_VIDEO_OUTPUT
  * or V4L2_BUF_TYPE_VBI_OUTPUT) applications must also initialize the
  * bytesused, field and timestamp fields. See Section 3.5, “Buffers”
  * for details. When VIDIOC_QBUF is called with a pointer to this
  * structure the driver sets the V4L2_BUF_FLAG_MAPPED and
  * V4L2_BUF_FLAG_QUEUED flags and clears the V4L2_BUF_FLAG_DONE flag in
  * the flags field, or it returns an EINVAL error code.
  *
  * To enqueue a user pointer buffer applications set the type field of a
  * struct v4l2_buffer to the same buffer type as previously struct
  * v4l2_format type and struct v4l2_requestbuffers type, the memory field
  * to V4L2_MEMORY_USERPTR and the m.userptr field to the address of the
  * buffer and length to its size. When the buffer is intended for output
  * additional fields must be set as above. When VIDIOC_QBUF is called with
  * a pointer to this structure the driver sets the V4L2_BUF_FLAG_QUEUED
  * flag and clears the V4L2_BUF_FLAG_MAPPED and V4L2_BUF_FLAG_DONE flags
  * in the flags field, or it returns an error code. This ioctl locks the
  * memory pages of the buffer in physical memory, they cannot be swapped
  * out to disk. Buffers remain locked until dequeued, until the
  * VIDIOC_STREAMOFF or VIDIOC_REQBUFS ioctl are called, or until the
  * device is closed.
  */
static int _enqueue1( struct video_capture *vci, int buffer_id ) {

	struct video_state *vs
		= ( struct video_state*)vci;

	const unsigned int BIT
		= 1 << buffer_id;

	struct v4l2_buffer buf = {
		.index = 0,
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		//.bytesused = 0,
		//.flags = 0,
		//.field = 0,
		//.timestamp, // struct timeval
		//.timecode,  // struct v4l2_timecode	
		//.sequence = 0,
		.memory = V4L2_MEMORY_MMAP,
		//.m.offset = 0,
		//.length   = 0,
		//.input    = 0,
		//.reserved = 0
	};

	assert( 0 <= buffer_id && buffer_id < VIDEO_MAX_FRAME );

	if( (BIT & vs->queued ) == 0 ) {
		buf.index = buffer_id;
		if( iioctl( vs->fd, VIDIOC_QBUF, &buf ) < 0 )
			warn( "%s:%d: enqueueing buffer %d", __FILE__, __LINE__, buf.index );
		else 
			vs->queued |= BIT;
	}

#if 0
	assert( ( buf.flags & V4L2_BUF_FLAG_MAPPED ) != 0 );
	assert( ( buf.flags & V4L2_BUF_FLAG_QUEUED ) != 0 );
	assert( ( buf.flags & V4L2_BUF_FLAG_DONE   ) == 0 );
#endif
	return 0;
}


static int _enqueue( struct video_capture *vci, unsigned int flags ) {

	struct video_state *vs
		= ( struct video_state*)vci;

	struct v4l2_buffer buf = {
		.index = 0,
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		//.bytesused = 0,
		//.flags = 0,
		//.field = 0,
		//.timestamp, // struct timeval
		//.timecode,  // struct v4l2_timecode	
		//.sequence = 0,
		.memory = V4L2_MEMORY_MMAP,
		//.m.offset = 0,
		//.length   = 0,
		//.input    = 0,
		//.reserved = 0
	};

	flags &= (~vs->queued);

	while( flags ) {
		buf.index = __builtin_ctz( flags );
		if( iioctl( vs->fd, VIDIOC_QBUF, &buf ) < 0 ) {
			warn( "%s:%d: enqueueing buffer %d", __FILE__, __LINE__, buf.index );
			break;
		} else {
			unsigned int BIT
				= (1<<buf.index);
			vs->queued |= BIT;
			flags      ^= BIT;
		}
	}

#if 0
	assert( ( buf.flags & V4L2_BUF_FLAG_MAPPED ) != 0 );
	assert( ( buf.flags & V4L2_BUF_FLAG_QUEUED ) != 0 );
	assert( ( buf.flags & V4L2_BUF_FLAG_DONE   ) == 0 );
#endif
	return 0;
}


/**
  * Applications call the VIDIOC_DQBUF ioctl to dequeue a filled
  * (capturing) or displayed (output) buffer from the driver's outgoing
  * queue. They just set the type and memory fields of a struct v4l2_buffer
  * as above, when VIDIOC_DQBUF is called with a pointer to this structure
  * the driver fills the remaining fields or returns an error code.
  *
  * By default VIDIOC_DQBUF blocks when no buffer is in the outgoing queue.
  * When the O_NONBLOCK flag was given to the open() function, VIDIOC_DQBUF
  * returns immediately with an EAGAIN error code when no buffer is
  * available.
  */
static int _dequeue( struct video_capture *vci, 
		int timeout, struct video_frame *fr ) {

	struct video_state *vs
		= (struct video_state*)vci;
	struct v4l2_buffer *buf
		= (struct v4l2_buffer *)fr;

	// Make no assumptions about callers, in particular thread structure.
	// If the device does not appear to have any frames queued, don't even
	// wait for select unless caller specified "no timeout" since, in that
	// case, it may be that another thread is going to queue a frame.

	if( vs->queued == 0 && timeout != VIDEO_DEQ_TIMEOUT_NONE )
		return -1;

	if( timeout == VIDEO_DEQ_TIMEOUT_DEFAULT )
		timeout = vs->dequeue_timeout;

	/*
	   Just for references...
		struct v4l2_buffer buf = {
			.index    = index,
			.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.bytesused = 0,
			.flags    = 0,
			.field    = 0,
			.timestamp, // struct timeval
			.timecode,  // struct v4l2_timecode	
			.sequence = 0,
			.memory = v4l2_memory_mmap,
			.m.offset = 0,
			.length   = 0,
			.input    = 0,
			.reserved = 0
		};
	*/

	buf->type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf->memory = V4L2_MEMORY_MMAP;

	if( timeout > 0 ) {

		int nfd = 0;
		fd_set rfds,efds;
		struct timeval tv = { timeout, 0 };
		FD_ZERO( &rfds );
		FD_ZERO( &efds );
		FD_SET( vs->fd, &rfds );
		FD_SET( vs->fd, &efds );

		// Wait for the V4L2 driver to signal buffer/frame availability.

		nfd = select( vs->fd + 1, &rfds, NULL, &efds, &tv );

		// Negative returns imply error; 0 implies timeout.

		if( nfd < 0 ) {
			if( EINTR == errno ) { // should only happen for Ctrl-C
				warn( "monitor_loop interrupted" );
				return __LINE__;
			} else
				err( 0, "waiting on input" );
		}

		if( 0 == nfd ) {
			warn( "monitor_loop timeout (%lds)", vs->dequeue_timeout );
			return __LINE__; // timed out
		}
	}

	if( iioctl( vs->fd, VIDIOC_DQBUF, buf ) < 0 ) {
		warn( "%s:%d: VIDIOC_DQBUF", __FILE__, __LINE__ );
		return __LINE__;
	}

	// At least dequeueing was successful...

	_set_unqueued( vs, buf->index );

	// ...but whether or not the buffer's content is valid is a separate
	// issue...

	if( buf->flags & V4L2_BUF_FLAG_ERROR ) {
		warnx( "V4L2_BUF_FLAG_ERROR in buffer %d", buf->index );
		return __LINE__;
	}

	fr->mem = vs->frame[ buf->index ].address;

	return 0;
}


/**
  * This function uses whatever configuration has been previously applied
  * to the camera and grabs exactly one frame, returning (by copying) the 
  * framebuffer into the provided buffer if there is space.
  *
  * This is mutually exclusive with streaming; the video_loop must NOT
  * be running. If it were, there would be no point in this.
  */
static int _snap( struct video_capture *vci, size_t *len, uint8_t **ubuf ) {

	struct video_state *vs
		= ( struct video_state*)vci;
	//struct video_frame dequeued;

	int econd = 0;

	struct v4l2_buffer buf = {
		//.index = index,
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		//.bytesused = 0,
		//.flags = 0,
		//.field = 0,
		//.timestamp, // struct timeval
		//.timecode,  // struct v4l2_timecode	
		//.sequence = 0,
		.memory = V4L2_MEMORY_MMAP,
		//.m.offset = 0,
		//.length   = 0,
		//.input    = 0,
		//.reserved = 0
	};

	// Enqueue all available buffers

	if( _enqueue1( vci, 0 ) )
		err( -1, "queueing failure (%d)", __LINE__ );

	if( _start( vci ) < 0 ) {
		warn("starting streaming");
		return -1;
	}

	// It seems that the 1st frame dequeued when using cameras with the 
	// "gspca" driver are always bad. No idea whether this is a driver
	// or hardware problem. In any event, queueing several buffers and
	// keeping the last seems to *usually* fix it. This is why even in
	// snapshot mode n may be >1.

	while( vs->queued ) {

		if( _dequeue( vci, 2 /* seconds */, (struct video_frame*)&buf ) ) {
			if( errno == EAGAIN )
				continue;
			if( errno == EIO )
				continue; // TODO: Revisit this.
			// Any other error is fatal.
			warn("dequeueing a buffer(%d)", __LINE__ );
			econd = -1;
		}

		break;
	}

	if( _stop( vci ) < 0 ) econd = -1;

	/**
	  * Both pointers must be valid to proceed. Assuming that...
	  * Allocate a buffer on caller's behalf if and only if the provided
	  * buffer is not large enough.
	  */

	if( (NULL != ubuf) && (NULL != len) ) {
		if( *len < buf.bytesused ) {
#ifdef _DEBUG
			fprintf( stdout, "realloc( %p, %ld => %d )\n",
				*ubuf, *len, buf.bytesused );
#endif
			void *p = realloc( *ubuf, buf.bytesused );
			if( p )
				*ubuf = p;
			else
				return -1;
		}
		if( *ubuf ) {
			memcpy(
				*ubuf, 
				vs->frame[ buf.index ].address, 
				buf.bytesused );
			*len  = buf.bytesused;
		} else {
			*len = 0;
			econd = -1;
		}
	}

	return econd;
}


static void _destroy( struct video_capture *vci ) {
	
	struct video_state *vs
		= ( struct video_state*)vci;
	_unmap_frames( vs->frame_count, vs->frame );
	close( vs->fd );

#ifndef HAVE_SINGLETON_DEVICE
	free( vs );
#endif
}


#ifdef HAVE_SINGLETON_DEVICE
static struct video_state _singleton_state = {
	.interface = {
		.format  = _format,
		.config  = _config,
		.snap    = _snap,
		.start   = _start,
		.enqueue1 = _enqueue1,
		.enqueue = _enqueue,
		.dequeue = _dequeue,
		.stop    = _stop,
		.destroy = _destroy,
	},
	.dequeue_timeout = 2 /* seconds */,
/*	.name[ MAXLEN_DEVPATH+1 ],
	.fd,
	.format = {},

	.queued,
	.frame_count,
	.frame[ VIDEO_MAX_FRAME ]
*/
};
#ifdef _DEBUG
const uint32_t *queued = &_singleton_state.queued;
#endif
#endif

/**
  * Either attempt to foist a specific resolution and format
  * onto the driver, or (better) just query what it's default
  * is.
  */
struct video_capture *video_open( const char *devpath ) {

	struct v4l2_capability cap;
	struct video_state *vs = NULL;
	struct stat st;
	int fd = -1;

	if( VIDEO_MAX_FRAME != 32 )
		return NULL; // too much depends on this now.

	/**
	  * Do all the most likely to fail stuff first.
	  */

	if( stat( devpath, &st ) < 0 ) {
		warn( "stat'ing device %s", devpath );
		return NULL;
	}

	if( ! S_ISCHR(st.st_mode) ) {
		warn( "%s is not a character device", devpath );
		return NULL;
	}

	fd = open( devpath, O_RDWR /* required */ | O_NONBLOCK, 0 );

	if( -1 == fd ) {
		warn( "opening device %s", devpath );
		return NULL;
	}

	if( iioctl( fd, VIDIOC_QUERYCAP, &cap ) < 0 ) {
		if( EINVAL == errno ) {
			warn( "maybe not a V4L2 device(?)" );
		} else {
			warn( "querying device capabilities" );
		}
		goto unwind0;
	}

	if( ! (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ) {
		warnx( "not video capture device\n");
		goto unwind0;
	}
	if( ! (cap.capabilities & V4L2_CAP_STREAMING) ) {
		warnx( "streaming not support\n");
		goto unwind0;
	}

#ifdef HAVE_SINGLETON_DEVICE
	vs = &_singleton_state;
#else
	vs = calloc( 1, sizeof(struct video_state) );
	if( NULL == vs )
		goto unwind0;
#endif

	/**
	  * TODO: Revisit: Just enumerate supported formats here and cache
	  * them in user space for subsequent inspection by client.
	  */

	// Move everything to the struct...
	strncpy( vs->name, devpath, MAXLEN_DEVPATH+1 );
	vs->fd = fd;

#ifdef _DEBUG
	_dump( vs, stdout );
#endif

	return & vs->interface;

unwind0:
	close( fd );

	return NULL;
}
/*
	if( __builtin_popcount( vs->queued ) < 2 ) {
		warnx( "too few frames (%d) queued",
			__builtin_popcount( vs->queued ) );
		return (void*)(-1);
	}
*/

/**
  * Everything below here builds a simple executable to stream video to
  * an X11 window.
  */

#ifdef UNIT_TEST_VIDEO

#include <getopt.h>
#include <stddef.h>

#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#endif

/**
  * The video interface, testing of which is really the only point of all
  * the following X11 code...
  */
struct video_capture *_vci  = NULL;

#ifdef HAVE_EXTRAS
static int _verbosity = 0;
#endif

/**
  * Data and state.
  */

static struct video_format _fmt = {
	.width = 160,
	.height = 120,
	.pixel_format = {'Y','U','Y','V','\0'}
};

#ifdef HAVE_X11

const long ALLEVENTS 
	= 0 // NoEventMask
	| KeyPressMask
	| KeyReleaseMask
	| ButtonPressMask
	| ButtonReleaseMask
	| EnterWindowMask
	| LeaveWindowMask
	| PointerMotionMask
	| PointerMotionHintMask
	| Button1MotionMask
	| Button2MotionMask
	| Button3MotionMask
	| Button4MotionMask
	| Button5MotionMask
	| ButtonMotionMask
	| KeymapStateMask
	| ExposureMask
	| VisibilityChangeMask
	| StructureNotifyMask
	| ResizeRedirectMask
	| SubstructureNotifyMask
	| SubstructureRedirectMask
	| FocusChangeMask
	| PropertyChangeMask
	| ColormapChangeMask
	| OwnerGrabButtonMask;


typedef struct _context {

	Display *display;
	int      screen;
	Window   win;
	GC       gc;

} context_t;

static context_t _cx;

/**
  * Following buffers are what are sent to X11 for rendering
  * via XPutImage.
  */
static XImage *_img         = NULL;
static unsigned char *_data = NULL;

#ifdef HAVE_EXTRAS

static int _afterFxn( Display *d ) {
	fprintf( stderr, "_afterFxn( %p )\n", d );
	return 0;
}

static int _errorHandler( Display *d, XErrorEvent *err ) {
	static char buf[1024];
	switch( err->error_code ) {
	case BadAccess:
	case BadAlloc:
	case BadAtom:
	case BadColor:
	case BadCursor:
	case BadDrawable:
	case BadFont:
	case BadGC:
	case BadIDChoice:
	case BadImplementation:
	case BadLength:
	case BadMatch :
	case BadName:
	case BadPixmap:
	case BadRequest:
	case BadValue:
	case BadWindow:
	default:;
	}
	XGetErrorText( d, err->error_code, buf, 1024 );
	fprintf( stderr, "%s (#%lu %u.%u)\n",
		buf,
		//err->type
		err->serial,
		err->request_code,
		err->minor_code);
	return 0;
}


static int _ioErrorHandler( Display *d ) {
	fprintf( stderr, "Fatal I/O or syscall error\n" );
	exit(-1);
}
#endif


static int _createImage( Display *d, int W, int H ) {

	XVisualInfo info = {
		.visual = NULL,
		.visualid = 0,
		.screen = _cx.screen,
		.depth = 1,
		.class = StaticGray,
		.red_mask = 0,
		.green_mask = 0,
		.blue_mask = 0,
		.colormap_size = 0,
		.bits_per_rgb = 0
	};
	int nmatched;

	memset( &info, 0, sizeof(info));

	XVisualInfo *matching
		= XGetVisualInfo( d, VisualNoMask, &info, &nmatched );

#ifdef HAVE_EXTRAS
	fprintf( stdout, "matched %d XVisualInfo\n", nmatched );
	if( _verbosity > 1 ) {
		for(i = 0; i < nmatched; i++ ) {
			XVisualInfo *vi = matching + i;
			fprintf( stdout,
				"%02d-----------------\n"
				"        .depth = %08X\n"
				"        .class = %d\n"
				"     .red_mask = %08X\n"
				"   .green_mask = %08X\n"
				"    .blue_mask = %08X\n"
				".colormap_size = %d\n"
				" .bits_per_rgb = %d\n",i,
				vi->depth,
				vi->class,
				vi->red_mask,
				vi->green_mask,
				vi->blue_mask,
				vi->colormap_size,
				vi->bits_per_rgb );
		}
	}
#endif

	if( matching ) {

		const size_t S
			= W * H * sizeof(unsigned int);
		_data = malloc( S );

		if( _data == NULL ) abort();

		/**
		  * Following actually ignores what XGetVisualInfo returned
		  * above and sets up the default that is prevalent on all
		  * modern hardware.
		  */
		_img = XCreateImage( d, 
				matching[1].visual,
				24,     // depth
				ZPixmap,// format
				0,      // offset
				(char*)_data,
				W, H,
				32,  // bitmap_pad 8, 16, or 32
				0 ); // bytes per line, inferred
		if( _img == NULL ) {
			fprintf( stderr, "XCreateImage failed. Aborting...\n" );
			free( _data );
			_data = NULL;
		}

		XFree( matching );
	}

	return _img == NULL ? -1 : 0;
}


static Bool _procEvent( XEvent *e ) {

	Bool proceed = True;

	switch( e->type ) {

	case KeyPress:
		switch( XLookupKeysym(&(e->xkey), 0) ) {
		case XK_Shift_L:
		case XK_Shift_R:
			break;
		case XK_Escape:
			proceed = False;
			break;
		}
		break;

	case KeyRelease:
		switch( XLookupKeysym(&(e->xkey), 0) ) {
		case XK_Shift_L:
		case XK_Shift_R:
			break;
		}
		break;

#ifdef HAVE_EXTRAS
	case ButtonPress:
	case ButtonRelease:
	case MotionNotify:
		break;

	case ConfigureNotify:
		fprintf( stdout, "ConfigureNotify( %d, %d )\n",
			((XConfigureEvent*)e)->width,
			((XConfigureEvent*)e)->height );
		break;

	case ClientMessage: 
		if(*XGetAtomName( _cx.display, e->xclient.message_type) == *"WM_PROTOCOLS") {
			proceed = False;
		}
		break;

	case EnterNotify:
	case LeaveNotify:
	case FocusIn:
	case FocusOut:
	case KeymapNotify:
	case Expose:
	case GraphicsExpose:
	case NoExpose:
	case VisibilityNotify:
	case CreateNotify:
	case DestroyNotify:
	case UnmapNotify:
	case MapNotify:
	case ReparentNotify:
	case GravityNotify:
	case CirculateNotify:
	case PropertyNotify:
	case SelectionClear:
	case SelectionNotify:
	case ColormapNotify:
	case MappingNotify:

	////////////////////////////////////////////////////////////////////////
	// Requests which a simple app should rarely handle itself, if even
	// receive assuming event masks have been setup properly...

	case MapRequest:
	case ConfigureRequest:
	case ResizeRequest:
	case CirculateRequest:
	case SelectionRequest:
		break;
	default:
		fprintf( stderr, "unexpected event type:%08X\n", e->type );
#endif
	}
#ifdef _ECHO_EVENTS_TO_STDOUT
	fprintf( stdout, "EV\t%s\n", X11_EVENT_NAME[e->type] );
#endif
	return proceed;
}


static inline uint8_t clampf( float v ) {
	if( v < 0.0 )
		return 0;
	else
	if( v > 255.0 )
		return 255;
	else
		return (uint8_t)v;
}

static inline uint8_t clampi( int v ) {
	if( v < 0 )
		return 0;
	else
	if( v > 255 )
		return 255;
	else
		return (uint8_t)v;
}

static inline void YUV2BGR( int y, int u, int v, uint8_t *bgr ) {
#if 1
	const int C = y -  16;
	const int D = u - 128;
	const int E = v - 128;
	bgr[2] = clampi( (298*C         + 409*E + 128) >> 8 );
	bgr[1] = clampi( (298*C - 100*D - 208*E + 128) >> 8 );
	bgr[0] = clampi( (298*C + 516*D         + 128) >> 8 );
#else
	bgr[2] = clampf( y +                 1.402*(u-128) );
	bgr[1] = clampf( y - 0.344*(v-128) - 0.714*(u-128) );
	bgr[0] = clampf( y + 1.772*(v-128)                 );
#endif
}

static void _render_video_frame( void *vbuf ) {

	const uint16_t *yuyv = vbuf;
	int err, r, c;

	/**
	  * Copy the video frame into our local buffer converting
	  * it from YUYV to...
	  */

	for(r = 0; r < _fmt.height; r++ ) {
#if 0
		/**
		  * ...grayscale rendered as 24-bit color in the process.
		  */
		for(c = 0; c < _fmt.width; c++ ) {
			const uint8_t GRAY
				= (uint8_t)(0x00FF & yuyv[ r*_fmt.width + c ]);
			_data[ 4*_fmt.width*r + 4*c+2 ] = GRAY; // red
			_data[ 4*_fmt.width*r + 4*c+1 ] = GRAY; // green
			_data[ 4*_fmt.width*r + 4*c+0 ] = GRAY; // blue
		}
#else
		/**
		  * ...24-bit color in the process.
		  */
		const uint8_t *iline = (const uint8_t*)(
			yuyv  + r*_fmt.width );
		      uint8_t *oline = 
			_data + r*_fmt.width*4;
		for(c = 0; c < 2*_fmt.width; c+=4 ) {
			const int Y0 = iline[c+0];
			const int U  = iline[c+1];
			const int Y1 = iline[c+2];
			const int V  = iline[c+3];
			YUV2BGR( Y0, U, V, oline + 4*(c/2+0) );
			YUV2BGR( Y1, U, V, oline + 4*(c/2+1) );
		}
#endif
	}

	err = XPutImage( _cx.display, _cx.win, _cx.gc, _img, 
			0, 0,
			0, 0,
			_fmt.width, _fmt.height );

	if( Success != err ) {
		static char buf[512];
		XGetErrorText( _cx.display, err, buf, sizeof(buf) );
		puts( buf );
	}
}


static void _exec_gui( const char *devname, int W, int H ) {

	Display *d = _cx.display;
	Window topwin;

	XEvent e;

#ifdef _DEBUG
#ifdef HAVE_EXTRAS
	XSetAfterFunction( d, _afterFxn );
#endif
	XSynchronize( d, 1 );
#endif

	printf("default depth = %d\n", DefaultDepth(d, _cx.screen ));

	_cx.win = XCreateSimpleWindow( d,
		DefaultRootWindow(d),
		0, 0,
		W, H,
		0, // no border
		BlackPixel( d, _cx.screen ), // border
		0xcccccc  );// background
	if( _cx.win == 0 ) return;

	topwin = _cx.win;

	_cx.gc = XCreateGC( d, topwin,
		0,      // mask of values
		NULL ); // array of values

	XStoreName( d, topwin, devname );

	XSelectInput( d, topwin,
		  KeyPressMask
		| KeyReleaseMask
		| ButtonPressMask
		| ButtonReleaseMask
		| EnterWindowMask
		| LeaveWindowMask
		| PointerMotionMask
		//| PointerMotionHintMask this will limit MotionNotifyEvents
		| Button1MotionMask
		| Button2MotionMask
		| Button3MotionMask
		| Button4MotionMask
		| Button5MotionMask
		| ButtonMotionMask
		| KeymapStateMask
		| ExposureMask
		| VisibilityChangeMask
		| StructureNotifyMask
		//| ResizeRedirectMask //exclusion precludes ResizeRequest events.
		//| SubstructureNotifyMask
		| SubstructureRedirectMask
		| FocusChangeMask
		| PropertyChangeMask
		| ColormapChangeMask
		| OwnerGrabButtonMask
		);

	XMapWindow( d, topwin ); // ...wait to for map confirmation
	XMapSubwindows( d, topwin );

	do {
		XNextEvent( d, &e );   // calls XFlush
	} while( e.type != MapNotify );

	do {

		if( ! XCheckMaskEvent( d, ALLEVENTS, &e ) ) {

			struct video_frame fr;

			/**
			  * Update the video when there are no events to process,
			  * and immediately requeue the frame.
			  */

			if( _vci->dequeue( _vci, 1 /* second */, &fr ) == 0 ) {
				_render_video_frame( fr.mem );
				_vci->enqueue( _vci, 1 << fr.buffer_id );
			}
		}

	} while( _procEvent( &e ) );

	XDestroySubwindows( d, topwin );
	XDestroyWindow( d, topwin );
}

#endif // HAVE_X11


int main( int argc, char *argv[] ) {

	static char video_device[ 64 ];
	static const char *USAGE
		= "%s -w <width> -h <height> -f <FOURCC pixel type> [ <device path> ]\n";
#ifndef HAVE_X11
	size_t   snapsize = 0;
	uint8_t *snapshot = NULL;
#endif
#if 0
	printf( "offsetof( struct v4l2_buffer, timestamp) = %ld\n",
			offsetof( struct v4l2_buffer, timestamp) );
	printf( "offsetof( struct video_frame, timestamp) = %ld\n",
			offsetof( struct video_frame, timestamp) );
	printf( "offsetof( struct v4l2_buffer, m) = %ld\n",
			offsetof( struct v4l2_buffer, m) );
	printf( "offsetof( struct video_frame, mem) = %ld\n",
			offsetof( struct video_frame, mem) );
	printf( "sizeof(struct v4l2_buffer) = %ld\n",
			sizeof(struct v4l2_buffer) );
	printf( "sizeof(struct video_frame) = %ld\n",
			sizeof(struct video_frame) );
#endif

	/**
	  * Process options
	  */

	do {
		static const char *OPTIONS = "w:h:f:v:";
		const char c = getopt( argc, argv, OPTIONS );
		if( c < 0 ) break;

		switch(c) {

		case 'w':
			_fmt.width = atoi( optarg );
			break;

		case 'h':
			_fmt.height = atoi( optarg );
			break;

		case 'f':
			strncpy( _fmt.pixel_format, optarg, 5 );
			break;

		case 'v':
#ifdef HAVE_EXTRAS
			_verbosity = atoi( optarg );
#endif
			break;
		default:
			printf ("error: unknown option: %c\n", c );
			exit(-1);
		}
	} while(1);

	/**
	  * Validate required arguments.
	  */

	if( optind < argc ) {
		strncpy( video_device, argv[ optind++ ], sizeof(video_device) );
	} else
	if( first_video_dev( video_device, sizeof(video_device) ) ) {
		fprintf( stdout, "no video devices found\n" );
		fprintf( stdout, USAGE, argv[0] );
		exit(-1);
	}

	_vci = video_open( video_device );
	if( _vci == NULL ) {
		fprintf( stderr, "error: opening \"%s\"\n", video_device );
		abort();
	}
	if( _vci->config( _vci, &_fmt, 1 ) != 0 )
		abort();

#ifndef HAVE_X11

	/**
	  * Without X, just emit a snapshot to a tmp file in CWD.
	  */

	if( _vci->snap( _vci, &snapsize, &snapshot ) == 0 ) {
		char filename[ 10 ];
		int fd;
		strcpy( filename, "imgXXXXXX" );
		if( (fd = mkstemp( filename )) >= 0 ) {
			write( fd, snapshot, snapsize );
			close( fd );
			fprintf( stdout, "%dW x %dH %s in %s\n", _fmt.width, _fmt.height, _fmt.pixel_format, filename );
		} else
			fprintf( stderr, "failed capturing\n" );
	}

#else

	_vci->start( _vci );
	_vci->enqueue( _vci, ALL_AVAILABLE_BUFFERS );

	/**
	  * Set up shared context.
	  */

	memset( &_cx, 0, sizeof(_cx) );

	_cx.display  = XOpenDisplay( NULL );

	if( _cx.display ) {

		if( _createImage( _cx.display, _fmt.width, _fmt.height ) == 0 ) {

#ifdef HAVE_EXTRAS
			if( _verbosity > 0 )
				fprintf( stdout, 
					"Server vendor: %s\n"
					"   X Protocol: %d.%d Release 0x%08X\n",
					XServerVendor(_cx.display),
					XProtocolVersion(_cx.display),
					XProtocolRevision(_cx.display),
					XVendorRelease(_cx.display) );
#endif
			_cx.screen = DefaultScreen( _cx.display );

#ifdef HAVE_EXTRAS
			XSetErrorHandler( _errorHandler );
			XSetIOErrorHandler( _ioErrorHandler );
#endif
			_exec_gui( video_device, _fmt.width, _fmt.height ); // runs an event loop

			if( _img )
				XDestroyImage( _img );
		}
		XCloseDisplay( _cx.display );
	}
	_vci->stop( _vci );

#endif // HAVE_X11

	_vci->destroy( _vci );

	return 0;
}

#endif

