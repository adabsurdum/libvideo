
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const static char *VIDEO_DEVICE_PATH_TEMPLATE = "/dev/video%d";
const int MAX_VIDEO_DEVICE_COUNT = 8;

int first_video_dev( char *path, int maxlen ) {
	for(int i = 0; i < MAX_VIDEO_DEVICE_COUNT; i++ ) {
		struct stat info;
		snprintf( path, maxlen, VIDEO_DEVICE_PATH_TEMPLATE, i );
		if( stat( path, &info ) == 0 && S_ISCHR( info.st_mode ) )
			return 0;
		else
			path[0] = '\0';
	}
	return -1;
}

