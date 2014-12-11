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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include "video.h"
#include "huffman_tables.h"
#include "common.h"

#define DEBUG 0
#define MSGPREFIX "[v4l2] "

static void print_capability(struct video_data *vd)
{
	fprintf(stderr, MSGPREFIX"Capabilities:\n");
	fprintf(stderr, MSGPREFIX"  Driver:       %s\n", vd->cap.driver);
	fprintf(stderr, MSGPREFIX"  Card:         %s\n", vd->cap.card);
	fprintf(stderr, MSGPREFIX"  Bus info:     %s\n", vd->cap.bus_info);
	fprintf(stderr, MSGPREFIX"  Version:      0x%08x\n", vd->cap.version);
	fprintf(stderr, MSGPREFIX"  Capabilities: 0x%08x", vd->cap.capabilities);
#define DECODE_FIELD(field, value) if (field & value) fprintf(stderr, " " #value);
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_VIDEO_CAPTURE)
/*	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_VIDEO_CAPTURE_MPLANE) */
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_VIDEO_OUTPUT)
/*	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_VIDEO_OUTPUT_MPLANE) */
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_VIDEO_OVERLAY)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_VBI_CAPTURE)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_VBI_OUTPUT)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_SLICED_VBI_CAPTURE)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_SLICED_VBI_OUTPUT)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_RDS_CAPTURE)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_VIDEO_OUTPUT_OVERLAY)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_HW_FREQ_SEEK)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_RDS_OUTPUT)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_TUNER)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_AUDIO)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_RADIO)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_MODULATOR)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_READWRITE)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_ASYNCIO)
	DECODE_FIELD(vd->cap.capabilities, V4L2_CAP_STREAMING)
	fprintf(stderr, "\n");
}

static void print_input(struct video_data *vd)
{
	fprintf(stderr, MSGPREFIX"Input %u:\n", vd->input.index);
	fprintf(stderr, MSGPREFIX"  Name:         %s\n", vd->input.name);
	fprintf(stderr, MSGPREFIX"  Type:         0x%08x", vd->input.type);
	if (vd->input.type & V4L2_INPUT_TYPE_TUNER)
		fprintf(stderr, " V4L2_INPUT_TYPE_TUNER\n");
	if (vd->input.type & V4L2_INPUT_TYPE_CAMERA)
		fprintf(stderr, " V4L2_INPUT_TYPE_CAMERA\n");
	fprintf(stderr, MSGPREFIX"  Audioset:     0x%08x\n", vd->input.audioset);
	fprintf(stderr, MSGPREFIX"  Tuner:        0x%08x\n", vd->input.tuner);
	fprintf(stderr, MSGPREFIX"  Std:          0x%016llx\n", vd->input.std);
	fprintf(stderr, MSGPREFIX"  Status:       0x%08x\n", vd->input.status);
/*	fprintf(stderr, MSGPREFIX"  Capabilities: 0x%08x\n", vd->input.capabilities); */
}

static void print_format(struct video_data *vd)
{
	fprintf(stderr, MSGPREFIX"Image format %u:\n", vd->fmtdesc.index);
	fprintf(stderr, MSGPREFIX"  Flags:        0x%08x", vd->fmtdesc.flags);
	DECODE_FIELD(vd->fmtdesc.flags, V4L2_FMT_FLAG_COMPRESSED)
	DECODE_FIELD(vd->fmtdesc.flags, V4L2_FMT_FLAG_EMULATED)
	fprintf(stderr, "\n");
	fprintf(stderr, MSGPREFIX"  Description:  %s\n", vd->fmtdesc.description);
	fprintf(stderr, MSGPREFIX"  Pixelformat:  0x%08x\n", vd->fmtdesc.pixelformat);
}

static void print_capture_parameters(struct video_data *vd)
{
	fprintf(stderr, MSGPREFIX"  Capture parameters:\n");
	fprintf(stderr, MSGPREFIX"    Capability:   0x%08x", vd->streamparm.parm.capture.capability);
	DECODE_FIELD(vd->streamparm.parm.capture.capability, V4L2_CAP_TIMEPERFRAME)
	fprintf(stderr, "\n");
	fprintf(stderr, MSGPREFIX"    Capturemode:  0x%08x", vd->streamparm.parm.capture.capturemode);
	DECODE_FIELD(vd->streamparm.parm.capture.capturemode, V4L2_MODE_HIGHQUALITY)
	fprintf(stderr, "\n");
	fprintf(stderr, MSGPREFIX"    Timeperframe: %u/%u\n", vd->streamparm.parm.capture.timeperframe.numerator,
									vd->streamparm.parm.capture.timeperframe.denominator);
}

static int open_device(struct video_data *vd)
{
	int ret;

	/* Open device */
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"Opening video device '%s'...\n", vd->device);
	vd->fd = open(vd->device, O_RDWR);
	if (vd->fd == -1) {
		perror(MSGPREFIX"open()");
		return -1;
	}

	/* Query device capabilities */
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"Querying capabilities, inputs and formats...\n");
	ret = ioctl(vd->fd, VIDIOC_QUERYCAP, &vd->cap);
	if (ret == -1) {
		perror(MSGPREFIX"VIDIOC_QUERYCAP");
		return -1;
	}
	if (vd->verbose)
		print_capability(vd);

	/* Enumerate video inputs */
	vd->input.index = 0;
	while (1) {
		ret = ioctl(vd->fd, VIDIOC_ENUMINPUT, &vd->input);
		if (ret == -1) {
			if (errno == EINVAL)
				break;
			perror(MSGPREFIX"VIDIOC_ENUMINPUT");
			return -1;
		}

		if (vd->verbose)
			print_input(vd);

		/* Autodetect correct input */
		if (vd->ninput == -1 && vd->input.type & V4L2_INPUT_TYPE_CAMERA)
			vd->ninput = vd->input.index;

		vd->input.index++;
	}

	/* Enumerate image formats */
	vd->fmtdesc.index = 0;
	while (1) {
		vd->fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = ioctl(vd->fd, VIDIOC_ENUM_FMT, &vd->fmtdesc);
		if (ret == -1) {
			if (errno == EINVAL)
				break;
			perror(MSGPREFIX"VIDIOC_ENUM_FMT");
			return -1;
		}

		if (vd->verbose)
			print_format(vd);

		vd->fmtdesc.index++;
	}

	/* Make sure we have capture capability */
	if (!(vd->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, MSGPREFIX"Error: this device doesn't support capture\n");
		return -1;
	}
	/* Make sure we have streaming capability */
	if (!(vd->cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf(stderr, MSGPREFIX"Error: this device doesn't support streaming I/O\n");
		return -1;
	}

	return 0;
}

static int select_input(struct video_data *vd)
{
	int ret;

	/* Select video input */
	if (vd->ninput == -1) {
		fprintf(stderr, MSGPREFIX"Error: camera input not found\n");
		return -1;
	}
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"Selecting video input %d...\n", vd->ninput);
	ret = ioctl(vd->fd, VIDIOC_S_INPUT, &vd->ninput);
	if (ret == -1) {
		perror(MSGPREFIX"VIDIOC_S_INPUT");
		return -1;
	}

	/* Enumerate selected input */
	vd->input.index = vd->ninput;
	ret = ioctl(vd->fd, VIDIOC_ENUMINPUT, &vd->input);
	if (ret == -1) {
		perror(MSGPREFIX"VIDIOC_ENUMINPUT");
		return -1;
	}

	if (vd->input.std) {
		/* Enumerate standards */
		vd->standard.index = 0;
		while (1) {
			ret = ioctl(vd->fd, VIDIOC_ENUMSTD, &vd->standard);
			if (ret == -1) {
				if (errno == EINVAL)
					break;
				perror(MSGPREFIX"VIDIOC_ENUMSTD");
				return -1;
			}

			if (vd->verbose)
				fprintf(stderr, MSGPREFIX"  Std: %2d, 0x%016llx, %s\n",
				        vd->standard.index, vd->standard.id, vd->standard.name);

			vd->standard.index++;
		}

		/* Get current standard */
		ret = ioctl(vd->fd, VIDIOC_G_STD, &vd->std_id);
		if (ret == -1) {
			perror(MSGPREFIX"VIDIOC_G_STD");
			return -1;
		}
		if (vd->verbose)
			fprintf(stderr, MSGPREFIX"  Current std: 0x%016llx\n", vd->std_id);

		/* Match current standard with one of the enumerated standards */
		vd->standard.index = 0;
		while (1) {
			ret = ioctl(vd->fd, VIDIOC_ENUMSTD, &vd->standard);
			if (ret == -1) {
				perror(MSGPREFIX"VIDIOC_ENUMSTD");
				return -1;
			}
			if (vd->standard.id == vd->std_id) {
				if (vd->verbose)
					fprintf(stderr, MSGPREFIX"  Current std: %s, frameperiod: %d/%d\n",
					        vd->standard.name, vd->standard.frameperiod.numerator, vd->standard.frameperiod.denominator);
				break;
			}

			vd->standard.index++;
		}
	}

	return 0;
}

static int get_format_and_streamparm(struct video_data *vd)
{
	int ret;

	/* Get the data format */
	vd->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"Getting data format...\n");
	ret = ioctl(vd->fd, VIDIOC_G_FMT, &vd->format);
	if (ret == -1) {
		perror(MSGPREFIX"VIDIOC_G_FMT");
		return -1;
	}
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"  Data format: %ux%u, pixelformat=0x%08x, field=%d, byteperline=%u, sizeimage=%u, colorspace=%d\n",
		        vd->format.fmt.pix.width, vd->format.fmt.pix.height, vd->format.fmt.pix.pixelformat,
		        vd->format.fmt.pix.field, vd->format.fmt.pix.bytesperline, vd->format.fmt.pix.sizeimage,
		        vd->format.fmt.pix.colorspace);

	/* Get streaming parameters */
	memset(&vd->streamparm, 0, sizeof(vd->streamparm));
	vd->streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(vd->fd, VIDIOC_G_PARM, &vd->streamparm);
	if (ret == -1) {
		if (errno == EINVAL) {
			fprintf(stderr, MSGPREFIX"Warning: VIDIOC_G_PARM is not implemented\n");
		} else {
			perror(MSGPREFIX"VIDIOC_G_PARM");
			return -1;
		}
	} else {
		if (vd->verbose)
			print_capture_parameters(vd);
	}

	return 0;
}

static int set_format_and_streamparm(struct video_data *vd)
{
	int ret;

	/* Set the data format */
	vd->format.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vd->format.fmt.pix.width       = vd->width;
	vd->format.fmt.pix.height      = vd->height;
	vd->format.fmt.pix.pixelformat = vd->pixelformat;
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"Setting data format %ux%u (0x%08x)...\n", vd->format.fmt.pix.width,
		        vd->format.fmt.pix.height, vd->format.fmt.pix.pixelformat);
	ret = ioctl(vd->fd, VIDIOC_S_FMT, &vd->format);
	if (ret == -1) {
		perror(MSGPREFIX"VIDIOC_S_FMT");
		return -1;
	}
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"  Data format: %ux%u, pixelformat=0x%08x, field=%d, byteperline=%u, sizeimage=%u, colorspace=%d\n",
		        vd->format.fmt.pix.width, vd->format.fmt.pix.height, vd->format.fmt.pix.pixelformat,
		        vd->format.fmt.pix.field, vd->format.fmt.pix.bytesperline, vd->format.fmt.pix.sizeimage,
		        vd->format.fmt.pix.colorspace);
	if (vd->width != vd->format.fmt.pix.width)
		vd->width = vd->format.fmt.pix.width;
	if (vd->height != vd->format.fmt.pix.height)
		vd->height = vd->format.fmt.pix.height;
	if (vd->format.fmt.pix.pixelformat != vd->pixelformat) {
		fprintf(stderr, MSGPREFIX"Error: (0x%08x) pixelformat not supported\n", vd->pixelformat);
		return -1;
	}

	/* Set streaming parameters */
	if (vd->streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
		vd->streamparm.parm.capture.timeperframe.numerator   = vd->tpfnum;
		vd->streamparm.parm.capture.timeperframe.denominator = vd->tpfden;
		if (vd->verbose)
			fprintf(stderr, MSGPREFIX"Setting streaming parameters %u/%u...\n",
			        vd->streamparm.parm.capture.timeperframe.numerator,
			        vd->streamparm.parm.capture.timeperframe.denominator);
		ret = ioctl(vd->fd, VIDIOC_S_PARM, &vd->streamparm);
		if (ret == -1) {
			if (errno == EINVAL) {
				fprintf(stderr, MSGPREFIX"Warning: VIDIOC_S_PARM is not implemented\n");
			} else {
				perror(MSGPREFIX"VIDIOC_S_PARM");
				return -1;
			}
		} else {
			if (vd->verbose)
				print_capture_parameters(vd);
		}

		if (vd->tpfnum != vd->streamparm.parm.capture.timeperframe.numerator)
			vd->tpfnum = vd->streamparm.parm.capture.timeperframe.numerator;
		if (vd->tpfden != vd->streamparm.parm.capture.timeperframe.denominator)
			vd->tpfden = vd->streamparm.parm.capture.timeperframe.denominator;
	} else if (vd->input.std) { /* set time per frame according to standard */
		vd->tpfnum = vd->standard.frameperiod.numerator;
		vd->tpfden = vd->standard.frameperiod.denominator;
	}

	return 0;
}

static int init_mmap(struct video_data *vd)
{
	int ret;
	unsigned int i;

	/* Initiate Memory Mapping I/O */
	memset(&vd->reqbuf, 0, sizeof(vd->reqbuf));
	vd->reqbuf.count  = 20;
	vd->reqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vd->reqbuf.memory = V4L2_MEMORY_MMAP;
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"Requesting %u buffers...\n", vd->reqbuf.count);
	ret = ioctl(vd->fd, VIDIOC_REQBUFS, &vd->reqbuf);
	if (ret == -1) {
		if (errno == EINVAL)
			fprintf(stderr, MSGPREFIX"Error: video capturing or mmap-streaming is not supported\n");
		else
			perror(MSGPREFIX"VIDIOC_REQBUFS");
		return -1;
	}
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"  Buffer obtained: %u\n", vd->reqbuf.count);

	/* We want at least five buffers. */
	if (vd->reqbuf.count < 5) {
		/* You may need to free the buffers here. */
		fprintf(stderr, MSGPREFIX"Error: we want at least 5 buffers\n");
		return -1;
	}

	vd->buffers = calloc(vd->reqbuf.count, sizeof(*vd->buffers));
	if (vd->buffers == NULL) {
		fprintf(stderr, MSGPREFIX"Error: calloc() failed\n");
		return -1;
	}

	/* Map buffers into our address space */
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"Mapping buffers...\n");
	for (i = 0; i < vd->reqbuf.count; i++) {
		memset(&vd->buffer, 0, sizeof(vd->buffer));
		vd->buffer.type   = vd->reqbuf.type;
/*		vd->buffer.memory = vd->reqbuf.memory; */
		vd->buffer.index  = i;
		ret = ioctl (vd->fd, VIDIOC_QUERYBUF, &vd->buffer);
		if (ret == -1) {
			perror(MSGPREFIX"VIDIOC_QUERYBUF");
			return -1;
		}
#if DEBUG
		fprintf(stderr, MSGPREFIX"  Buffer: index=%u, type=%d, bytesused=%u, flags=0x%08x, sequence=%u, memory=%u, m.offset=%u, length=%u\n",
		        vd->buffer.index, vd->buffer.type, vd->buffer.bytesused, vd->buffer.flags, vd->buffer.sequence,
		        vd->buffer.memory, vd->buffer.m.offset, vd->buffer.length);
#endif
		vd->buffers[i].length = vd->buffer.length; /* remember for munmap() */
		vd->buffers[i].start  = mmap(NULL, vd->buffer.length,
		                        PROT_READ | PROT_WRITE, /* recommended */
		                        MAP_SHARED,             /* recommended */
		                        vd->fd, vd->buffer.m.offset);

		if (MAP_FAILED == vd->buffers[i].start) {
			/* If you do not exit here you should unmap() and free()
			the buffers mapped so far. */
			perror(MSGPREFIX"mmap()");
			return -1;
		}
	}

	/* Allocate space for jpeg and yuyv buffers */
	if (vd->pixelformat == V4L2_PIX_FMT_MJPEG || vd->pixelformat == V4L2_PIX_FMT_JPEG) {
		if (vd->pixelformat == V4L2_PIX_FMT_MJPEG) {
			vd->jpegbuflen = vd->buffers[0].length + DHT_SIZE;
			vd->jpegbuf    = calloc(1, vd->jpegbuflen);

			if (vd->jpegbuf == NULL) {
				fprintf(stderr, MSGPREFIX"Error: calloc() failed\n");
				return -1;
			}
		} else if (vd->pixelformat == V4L2_PIX_FMT_JPEG) {
			vd->jpegbuflen = vd->buffers[0].length;
			vd->jpegbuf    = NULL;
		}

		/* yuyv422 format */
		vd->yuvbuflen = 2 * vd->width * vd->height;
		/* On x86_32 malloc returns memory that is 8-byte aligned, while on
		   x86_64 it returns 16-byte aligned memory. To be sure we get a 16-byte
		   aligned buffer (required for the sse2 routines) we explicitely request it */
		ret = posix_memalign((void **)&vd->yuvbuf, 16, vd->yuvbuflen);

		if (ret || vd->yuvbuf == NULL) {
			fprintf(stderr, MSGPREFIX"Error: posix_memalign() failed\n");
			return -1;
		}
	}
	
	return 0;
}

static int enqueue_all_buffers(struct video_data *vd)
{
	int ret;
	unsigned int i;

	/* Enqueue all buffers */
	if (vd->verbose)
		fprintf(stderr, MSGPREFIX"Enqueueing buffers...\n");
	for (i = 0; i < vd->reqbuf.count; i++) {
		memset(&vd->buffer, 0, sizeof(vd->buffer));
		vd->buffer.type   = vd->reqbuf.type;
		vd->buffer.memory = vd->reqbuf.memory;
		vd->buffer.index  = i;

		ret = ioctl(vd->fd, VIDIOC_QBUF, &vd->buffer);
		if (ret == -1) {
			perror(MSGPREFIX"VIDIOC_QBUF");
			return -1;
		}
	}

	return 0;
}

int video_init(struct video_data *vd, char *device, int ninput,
               unsigned int width, unsigned int height,
               int fps, unsigned int pixelformat, int verbose)
{
	int ret;

	vd->device      = device;
	vd->ninput      = ninput;
	vd->width       = width;
	vd->height      = height;
	vd->pixelformat = pixelformat;
	vd->tpfnum      = 1;
	vd->tpfden      = fps;
	vd->isstreaming = 0;
	vd->yuvbuf      = NULL;
	vd->yuvbuflen   = 0;
	vd->jpegbuf     = NULL;
	vd->jpegbuflen  = 0;
	vd->jpegbufused = 0;
	vd->verbose     = verbose;

	ret = open_device(vd);
	if (ret == -1)
		return -1;

	ret = select_input(vd);
	if (ret == -1)
		return -1;

	ret = get_format_and_streamparm(vd);
	if (ret == -1)
		return -1;

	ret = set_format_and_streamparm(vd);
	if (ret == -1)
		return -1;

	ret = init_mmap(vd);
	if (ret == -1)
		return -1;

	return 0;
}

int video_start_streaming(struct video_data *vd)
{
	int ret, buf_type;

	/* Check if we're streaming */
	if (vd->isstreaming)
		return 0;

	ret = enqueue_all_buffers(vd);
	if (ret == -1)
		return -1;

	/* Start streaming */
	if (vd->verbose)
		print_msg_nl(MSGPREFIX"Start streaming...\n");
	buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(vd->fd, VIDIOC_STREAMON, &buf_type);
	if (ret == -1) {
		snprintf(msgbuf, MBS, MSGPREFIX"VIDIOC_STREAMON: %s\n", strerror(errno));
		print_msg_nl(msgbuf);
		return -1;
	}

	vd->isstreaming = 1;

	return 0;
}

int video_frame_get(struct video_data *vd)
{
	int ret;

	/* Dequeue a buffer */
	memset(&vd->buffer, 0, sizeof(vd->buffer));
	vd->buffer.type   = vd->reqbuf.type;
	vd->buffer.memory = vd->reqbuf.memory;

	ret = ioctl(vd->fd, VIDIOC_DQBUF, &vd->buffer);
	if (ret == -1) {
		snprintf(msgbuf, MBS, MSGPREFIX"VIDIOC_DQBUF: %s\n", strerror(errno));
		print_msg_nl(msgbuf);
		return -1;
	}

#if DEBUG
	fprintf(stderr, MSGPREFIX"  Buffer: index=%u, type=%d, bytesused=%u, flags=0x%08x, sequence=%u, memory=%u, m.offset=%u, length=%u\n",
	        vd->buffer.index, vd->buffer.type, vd->buffer.bytesused, vd->buffer.flags, vd->buffer.sequence,
	        vd->buffer.memory, vd->buffer.m.offset, vd->buffer.length);
#endif

	if (vd->pixelformat == V4L2_PIX_FMT_YUYV || vd->pixelformat == V4L2_PIX_FMT_YUV420) {
		/* Copy buffer pointer and length */
		vd->yuvbuf    = vd->buffers[vd->buffer.index].start;
		vd->yuvbuflen = vd->buffers[vd->buffer.index].length;
	} else if (vd->pixelformat == V4L2_PIX_FMT_MJPEG) {
		/* Insert Huffman tables in jpeg picture */
		memcpy(vd->jpegbuf, vd->buffers[vd->buffer.index].start, HEADERFRAME1);
		memcpy(vd->jpegbuf + HEADERFRAME1, dht_data, DHT_SIZE);
		memcpy(vd->jpegbuf + HEADERFRAME1 + DHT_SIZE,
		       vd->buffers[vd->buffer.index].start + HEADERFRAME1,
		       vd->buffer.bytesused - HEADERFRAME1);
		vd->jpegbufused = vd->buffer.bytesused + DHT_SIZE;
	} else if (vd->pixelformat == V4L2_PIX_FMT_JPEG) {
		/* Copy buffer pointer and length */
		vd->jpegbuf     = vd->buffers[vd->buffer.index].start;
		vd->jpegbufused = vd->buffer.bytesused;
	}

	return 0;
}

int video_frame_done(struct video_data *vd)
{
	int ret;

	/* Enqueue a buffer */
	ret = ioctl(vd->fd, VIDIOC_QBUF, &vd->buffer);
	if (ret == -1) {
		snprintf(msgbuf, MBS, MSGPREFIX"VIDIOC_QBUF: %s\n", strerror(errno));
		print_msg_nl(msgbuf);
		return -1;
	}

	return 0;
}

int video_stop_streaming(struct video_data *vd)
{
	int ret, buf_type;

	/* Check if we're streaming */
	if (vd->isstreaming == 0)
		return 0;

	/* Stop streaming */
	if (vd->verbose)
		print_msg_nl(MSGPREFIX"Stop streaming...\n");
	buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(vd->fd, VIDIOC_STREAMOFF, &buf_type);
	if (ret == -1) {
		snprintf(msgbuf, MBS, MSGPREFIX"VIDIOC_STREAMOFF: %s\n", strerror(errno));
		print_msg_nl(msgbuf);
		return -1;
	}

	vd->isstreaming = 0;

	return 0;
}

int video_close(struct video_data *vd)
{
	int ret;
	unsigned int i;

	/* Stop streaming */
	if (vd->isstreaming) {
		ret = video_stop_streaming(vd);
		if (ret == -1)
			return -1;
	}

	/* Cleanup. */
	if (vd->verbose)
		print_msg_nl(MSGPREFIX"Cleaning up...\n");
	for (i = 0; i < vd->reqbuf.count; i++)
		munmap(vd->buffers[i].start, vd->buffers[i].length);
	free(vd->buffers);
	if (vd->pixelformat == V4L2_PIX_FMT_MJPEG || vd->pixelformat == V4L2_PIX_FMT_JPEG) {
		if (vd->pixelformat == V4L2_PIX_FMT_MJPEG)
			free(vd->jpegbuf);
		free(vd->yuvbuf);
	}

	/* Close device */
	if (vd->verbose)
		print_msg_nl(MSGPREFIX"Closing video device...\n");
	ret = close(vd->fd);
	if (ret == -1) {
		snprintf(msgbuf, MBS, MSGPREFIX"close(): %s\n", strerror(errno));
		print_msg_nl(msgbuf);
		return -1;
	}

	return 0;
}
