
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/types.h>
#include <linux/videodev2.h>
#include "vidfrm.h"

/**
struct v4l2_buffer {
	__u32			index;
	enum v4l2_buf_type      type;
	__u32			bytesused;
	__u32			flags;
	enum v4l2_field		field;
	struct timeval		timestamp;
	struct v4l2_timecode	timecode;
	__u32			sequence;

	enum v4l2_memory        memory;
	union {
		__u32           offset;
		unsigned long   userptr;
		struct v4l2_plane *planes;
	} m;
	__u32			length;
	__u32			input;
	__u32			reserved;
};

  */
int main( int argc, char *argv[] ) {

	printf(
		"    index: %ld\n"
		"     type: %ld\n"
		"bytesused: %ld\n"
		"    flags: %ld\n"
		"    field: %ld\n"
		"timestamp: %ld\n"
		" timecode: %ld\n"
		" sequence: %ld\n"
		"   memory: %ld\n"
		"        m: %ld\n"
		"   length: %ld\n"
		"    input: %ld\n"
		" reserved: %ld\n",
		offsetof( struct v4l2_buffer, index),
		offsetof( struct v4l2_buffer, type),
		offsetof( struct v4l2_buffer, bytesused),
		offsetof( struct v4l2_buffer, flags),
		offsetof( struct v4l2_buffer, field),
		offsetof( struct v4l2_buffer, timestamp),
		offsetof( struct v4l2_buffer, timecode),
		offsetof( struct v4l2_buffer, sequence),
		offsetof( struct v4l2_buffer, memory),
		offsetof( struct v4l2_buffer, m),
		offsetof( struct v4l2_buffer, length),
		offsetof( struct v4l2_buffer, input),
		offsetof( struct v4l2_buffer, reserved) );
	puts("---");
	printf(
		"buffer_id: %ld\n"
		"     pad0: %ld\n"
		"timestamp: %ld\n"
		"     pad1: %ld\n"
		"      mem: %ld\n"
		"     pad2: %ld\n",
		offsetof( struct video_frame, buffer_id),
		offsetof( struct video_frame, pad0),
		offsetof( struct video_frame, timestamp),
		offsetof( struct video_frame, pad1),
		offsetof( struct video_frame, mem),
		offsetof( struct video_frame, pad2));
	if( sizeof(struct video_frame) != sizeof(struct v4l2_buffer) )
		printf( "%ld != %ld\n",
			sizeof(struct video_frame), sizeof(struct v4l2_buffer)  );
	return 0;
}

