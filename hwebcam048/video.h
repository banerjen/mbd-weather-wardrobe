/*
 * Copyright (C) 2011 Giorgio Vazzana
 *
 * This file is part of hwebcam.
 *
 * hwebcam is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * hwebcam is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VIDEO_H
#define VIDEO_H

#include <linux/videodev2.h>

struct video_data {
	int fd;
	struct v4l2_capability     cap;
	struct v4l2_input          input;
	struct v4l2_fmtdesc        fmtdesc;
	struct v4l2_format         format;
	struct v4l2_standard       standard;
	v4l2_std_id                std_id;
	struct v4l2_streamparm     streamparm;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer         buffer;
	struct {
		void   *start;
		size_t length;
	} *buffers;

	char          *device;
	int            ninput;
	unsigned int   width;
	unsigned int   height;
	unsigned int   pixelformat;
	unsigned int   tpfnum; /* time per frame */
	unsigned int   tpfden;
	int            isstreaming;

	unsigned char *yuvbuf;
	size_t         yuvbuflen;

	unsigned char *jpegbuf;
	size_t         jpegbuflen;
	size_t         jpegbufused;

	int            verbose;
};

int video_init(struct video_data *vd, char *device, int ninput,
               unsigned int width, unsigned int height,
               int fps, unsigned int pixelformat, int verbose);
int video_start_streaming(struct video_data *vd);
int video_frame_get(struct video_data *vd);
int video_frame_done(struct video_data *vd);
int video_stop_streaming(struct video_data *vd);
int video_close(struct video_data *vd);

#endif
