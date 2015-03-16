
#ifndef _vidfrm_h_
#define _vidfrm_h_

#ifndef VIDEO_MAX_FRAME
// This ought to be same as in <linux/videodev2.h>
#define VIDEO_MAX_FRAME (32)
#endif
#define ALL_AVAILABLE_BUFFERS (0xFFFFFFFF)

/**
  * This struct is intended to *overlay* a struct v4l2_buffer to:
  * 1) minimize mem copying required when dequeueing frames, and
  * 2) insulate code external to video.c--in particular, the state machine
  *    implementation--from the V4L2 internals.
  *
  * AUDIT: The index and timeval members must be in exactly the same
  * position in this struct and in struct v4l2_buffer.
  */
struct video_frame {

	/**
	  * This is the <index> member in the struct v4l2_buffer.
	  */
	int buffer_id;

	/**
	  * This padding insures that the timestamp is in the same position
	  * in video_frame as it is in v4l2_buffer.
	  */
	char pad0[16];

	/**
	  * This is the actual kernel (or device driver/V4L2)-provided
	  * timestamp.
	  */
	struct timeval timestamp;

	/**
	  * This padding insures that the two buffers are the same total size
	  * This is essential since monitor.c will "offer" video_frame pointers
	  * to video_capture.dequeue which will use them internally as if they
	  * were v4l2_buffer's to receive dequeued frame information.
	  */
	char pad1[24];

	/**
	  * A pointer to the actual (memory mapped) buffer of the frame
	  * indicated by <index>.
	  * This field clobbers the <type> field of struct v4l2_buffer.
	  * video_capture.dequeue overwrites v4l2_buffer.type with this pointer
	  * before returning.
	  */
	void *mem;

	char pad2[12];
};

#endif

