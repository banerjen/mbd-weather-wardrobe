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
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <theora/theoraenc.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include "common.h"
#include "video.h"
#include "audio.h"
#include "jpeg.h"

#ifdef USE_SDL
#include "SDL.h"
#endif

#if defined(ARCH_X86_32) || defined(ARCH_X86_64)
#include "x86.h"
#endif

#define VERSION "0.48"
#define DEBUG   0

struct ppm_st {
	unsigned int   width;
	unsigned int   height;
	unsigned int   max;
	unsigned char *data;
};

enum muxer_page_type {
	AUDIO,
	VIDEO
};

struct muxer_page_st {
	ogg_page              opage;
	struct muxer_page_st *next;
};

struct muxer_st {
	FILE                 *fpvideo;

	th_enc_ctx           *tctx;
	struct muxer_page_st *vq_head;
	struct muxer_page_st *vq_tail;

	int                   haveaudio;
	vorbis_dsp_state     *vds;
	struct muxer_page_st *aq_head;
	struct muxer_page_st *aq_tail;
};

static volatile sig_atomic_t keep_going = 1;

static void signal_handler(int sig)
{
	if (sig == SIGINT)
		keep_going = 0;
}

#define CLIPRGB(x) if (x < 0) x = 0; else if (x > 255) x = 255;
#define COMPUTE_RGB(y, cb, cr) r = (256 * y +          + 359 * cr + 128) >> 8;\
                               g = (256 * y -  88 * cb - 183 * cr + 128) >> 8;\
                               b = (256 * y + 454 * cb +          + 128) >> 8;
static void yuyv422_to_rgb24(const unsigned char *src, struct ppm_st *frame)
{
	unsigned int i, j;
	int y1, cb, y2, cr, i_rows, j3;
	int r, g, b;

	for (i = 0; i < frame->height; i++) {
		i_rows = 3*i*frame->width;
		for (j = 0; j < frame->width; j += 2) {
			y1 = src[0];
			cb = src[1] - 128;
			y2 = src[2];
			cr = src[3] - 128;

			COMPUTE_RGB(y1, cb, cr)
			CLIPRGB(r)
			CLIPRGB(g)
			CLIPRGB(b)

			j3 = 3*j;
			*(frame->data + j3 + i_rows + 0) = r;
			*(frame->data + j3 + i_rows + 1) = g;
			*(frame->data + j3 + i_rows + 2) = b;

			COMPUTE_RGB(y2, cb, cr)
			CLIPRGB(r)
			CLIPRGB(g)
			CLIPRGB(b)

			j3 += 3;
			*(frame->data + j3 + i_rows + 0) = r;
			*(frame->data + j3 + i_rows + 1) = g;
			*(frame->data + j3 + i_rows + 2) = b;

			src += 4;
		}
	}
}

static void yuyv422_to_y8_c(const unsigned char *src, th_ycbcr_buffer ycbcrbuf)
{
	int i, j, i_rows_y;
#if 0
	for (i = 0; i < ycbcrbuf[0].width * ycbcrbuf[0].height; i++)
		ycbcrbuf[0].data[i] = src[2*i];
#endif
	for (i = 0; i < ycbcrbuf[0].height; i++) {
		i_rows_y = i*ycbcrbuf[0].width;
		for (j = 0; j < ycbcrbuf[0].width; j++) {
			*(ycbcrbuf[0].data + i_rows_y + j) = *src;
			src += 2;
		}
	}
}

static void yuyv422_to_yuv420p_c(const unsigned char *src, th_ycbcr_buffer ycbcrbuf)
{
	int i, j, y1, cb, y2, cr;
	int two_rows_y, i_rows_y, i_rows_c;

	two_rows_y = 2 * ycbcrbuf[0].width;
	for (i = 0; i < ycbcrbuf[0].height; i++) {
		i_rows_y =      i*ycbcrbuf[0].width;
		i_rows_c = (i>>1)*ycbcrbuf[1].width;
		for (j = 0; j < ycbcrbuf[0].width; j += 2) {
			y1 = src[0];
			cb = src[1];
			y2 = src[2];
			cr = src[3];

			*(ycbcrbuf[0].data + i_rows_y + j  ) = y1;
			*(ycbcrbuf[0].data + i_rows_y + j+1) = y2;

			if (!(i&1)) {
				cb += src[two_rows_y + 1] + 1;
				*(ycbcrbuf[1].data + i_rows_c + (j>>1)) = cb >> 1;

				cr += src[two_rows_y + 3] + 1;
				*(ycbcrbuf[2].data + i_rows_c + (j>>1)) = cr >> 1;
			}
			src += 4;
		}
	}
}

static void muxer_add_page(struct muxer_st *muxer, ogg_page *opage, enum muxer_page_type type)
{
	struct muxer_page_st **tail, *oldtail;

	if (type == AUDIO)
		tail = &muxer->aq_tail;
	else
		tail = &muxer->vq_tail;

	oldtail = *tail;

	if (*tail == NULL) {
		*tail = malloc(sizeof(**tail));
	} else {
		(*tail)->next = malloc(sizeof(**tail));
		*tail = (*tail)->next;
	}

	if (*tail) {
		(*tail)->opage = *opage;
		(*tail)->next  = NULL;
		(*tail)->opage.header = malloc(opage->header_len);
		(*tail)->opage.body   = malloc(opage->body_len);

		if ((*tail)->opage.header && (*tail)->opage.body) {
			memcpy((*tail)->opage.header, opage->header, opage->header_len);
			memcpy((*tail)->opage.body,   opage->body,   opage->body_len);
		} else {
			free((*tail)->opage.header);
			free((*tail)->opage.body);
			free(*tail);
			*tail = oldtail;
		}
	} else
		*tail = oldtail;

	if (oldtail == NULL) {
		if (type == AUDIO)
			muxer->aq_head = *tail;
		else
			muxer->vq_head = *tail;
	}
}

static void muxer_write(struct muxer_st *muxer, int flush)
{
	ogg_int64_t granulepos_theora;
	ogg_int64_t granulepos_vorbis;
	double      time_theora;
	double      time_vorbis;
	struct muxer_page_st *p;

	/* if we have only a video stream we can just flush pages */
	if (muxer->haveaudio == 0)
		flush = 1;

	while (1) {
		/* if both the queues are empty, there's nothing to do */
		if (muxer->aq_head == NULL && muxer->vq_head == NULL)
			break;

		/* look for pages with granulepos != -1 in audio queue */
		granulepos_vorbis = -1;
		p = muxer->aq_head;
		while (p) {
			granulepos_vorbis = ogg_page_granulepos(&p->opage);
			if (granulepos_vorbis != -1)
				break;
			p = p->next;
		}

		/* look for pages with granulepos != -1 in video queue */
		granulepos_theora = -1;
		p = muxer->vq_head;
		while (p) {
			granulepos_theora = ogg_page_granulepos(&p->opage);
			if (granulepos_theora != -1)
				break;
			p = p->next;
		}

		/* if one of the queues is empty or doesn't have pages with
		   granulepos != -1, then we're done */
		if (!flush && (granulepos_vorbis == -1 || granulepos_theora == -1))
			break;

		/* ok, at this point both granulepos are != -1, we can compute times */
		time_vorbis = vorbis_granule_time(muxer->vds, granulepos_vorbis);
		time_theora = th_granule_time(muxer->tctx, granulepos_theora);

		/* write audio pages */
		if (time_vorbis <= time_theora || flush) {
			while (1) {
				if (muxer->aq_head == NULL)
					break;

				p = muxer->aq_head;
				muxer->aq_head = muxer->aq_head->next;
				if (muxer->aq_head == NULL)
					muxer->aq_tail = NULL;

				granulepos_vorbis = ogg_page_granulepos(&p->opage);
				fwrite(p->opage.header, 1, p->opage.header_len, muxer->fpvideo);
				fwrite(p->opage.body  , 1, p->opage.body_len,   muxer->fpvideo);
				free(p->opage.header);
				free(p->opage.body);
				free(p);

				/* stop when we find a page with granulepos != -1 */
				if (!flush && granulepos_vorbis != -1)
					break;
			}
		}

		/* write video pages */
		if (time_vorbis >= time_theora || flush) {
			while (1) {
				if (muxer->vq_head == NULL)
					break;

				p = muxer->vq_head;
				muxer->vq_head = muxer->vq_head->next;
				if (muxer->vq_head == NULL)
					muxer->vq_tail = NULL;

				granulepos_theora = ogg_page_granulepos(&p->opage);
				fwrite(p->opage.header, 1, p->opage.header_len, muxer->fpvideo);
				fwrite(p->opage.body  , 1, p->opage.body_len,   muxer->fpvideo);
				free(p->opage.header);
				free(p->opage.body);
				free(p);

				if (!flush && granulepos_theora != -1)
					break;
			}
		}
	}
	fflush(muxer->fpvideo);
}

static char *pixelformat_to_str(unsigned int pixelformat)
{
	switch (pixelformat) {
	case V4L2_PIX_FMT_YUYV:
		return "YUYV";
	case V4L2_PIX_FMT_YUV420:
		return "YUV420";
	case V4L2_PIX_FMT_MJPEG:
		return "MJPEG";
	case V4L2_PIX_FMT_JPEG:
		return "JPEG";
	default:
		return "Unknown";
	}
}

static void usage(void)
{
	fprintf(stderr,
"usage: hwebcam [options]\n"
"       -h          print help\n"
"       -v          increment verbosity level\n"
"Video options:\n"
"       -d videodev use video device videodev      (/dev/video0)\n"
"       -I input    choose device input            (autodetected)\n"
"       -s WxH      choose frame size              (640x400)\n"
"       -i fps      choose frame rate              (10)\n"
"       -f fmt      choose pixelformat, supported values:\n"
"                   yuyv, yuv, mjpeg, jpeg         (yuyv)\n"
"       -g          grayscale                      (disabled)\n"
"       -t val      save a frame every val seconds (disabled, 0=every frame)\n"
"       -e          encode captured stream in ogg/theora/vorbis\n"
"       -o ofile    save captured stream in ofile  (capture.ogg, -=stdout)\n"
"       -b bitrate  choose theora bitrate in bit/s (200000)\n"
"       -q quality  choose theora quality          (0, range=[0-63])\n"
#ifdef USE_SDL
"       -w          display stream in a sdl window\n"
#endif
"Audio options:\n"
"       -a          include audio in captured stream\n"
"       -D audiodev use audio device audiodev      (hw:0,0)\n"
"       -c channels choose number of channels      (1)\n"
"       -r rate     choose sampling rate           (32000)\n"
"       -B bitrate  choose vorbis bitrate in bit/s (48000)\n"
"       -Q quality  choose vorbis quality          (0.1, range=[-0.1 - 1.0])\n"
	);
}

int main(int argc, char *argv[])
{
	int ret, opt, i, verbose = 0;
	struct sigaction   act;
	struct pollfd      pfds[4];
	int                nfds, pollret;
	struct timeval     tea0, tea1; /* encode audio */
	struct timeval     tev0, tev1; /* encode video */
	struct timeval     tdp0, tdp1; /* decode picture */
	long int           t_enca, t_encv, t_decp;

	/* video data */
	struct video_data  vd;
	char              *videodevice = "/dev/video0";
	int                ifc, ninput = -1, width = 640, height = 400, gray = 0;
	struct fract {
		int      num;
		int      den;
		double   ratio;
		long int tpf_us;
	}                  fps = {10, 1, 10.0, 100000};
	unsigned int       pixelformat = V4L2_PIX_FMT_YUYV;
	struct timeval     frame_start, frame_stop;
	time_t             timeout_start, timeout_stop;
	long int           frame_interval;
	double             actual_fps;

	/* audio data */
	struct audio_data  ad;
	char              *audiodevice = "hw:0,0";
	unsigned int       rate = 32000, channels = 1, peak_count = 0;
	float              peak_percent = 0.0f;

	/* a/v sync */
	ogg_int64_t        time_audio_us = 0;
	ogg_int64_t        time_video_us = 0;
	int                av_desync = 0;

	/* image data */
	struct ppm_st      ppmimage = {0, 0, 0, NULL};
	int                timeout = -1;
	FILE              *fpimage = NULL;
	char               imagename[32];

	/* ogg data */
	ogg_stream_state   ostream_theora;
	ogg_stream_state   ostream_vorbis;
	ogg_page           opage;
	FILE              *fpvideo = NULL;
	char              *videoname = "capture.ogg";
	struct muxer_st    muxer;

	/* theora data */
	int                encode_theora = 0, height_theora;
	th_info            ti;
	th_comment         tc;
	th_enc_ctx        *tctx = NULL;
	th_ycbcr_buffer    ycbcrbuf;
	ogg_packet         opacket_theora;
	int                bitrate_theora = 200000, quality_theora = 0;
	int                duplicate = 0, keep_going_theora = 0;
	void               (*yuyv422_to_y8)(const unsigned char *, th_ycbcr_buffer);
	void               (*yuyv422_to_yuv420p)(const unsigned char *, th_ycbcr_buffer);

	/* vorbis data */
	int                encode_vorbis = 0;
	vorbis_info        vi;
	vorbis_comment     vc;
	vorbis_dsp_state   vds;
	vorbis_block       vb;
	ogg_packet         opacket_vorbis[3];
	float            **vorbisbuffer;
	int                bitrate_vorbis = 48000, keep_going_vorbis = 0;
	float              quality_vorbis = 0.1f;

#ifdef USE_SDL
	int                sdl_window = 0;
	char               caption_string[32];
	SDL_Surface       *surface = NULL;
	SDL_Overlay       *overlay = NULL;
	SDL_Rect           video_rect;
	struct timeval     tsdl0, tsdl1; /* display picture to sdl */
	long int           t_sdl;
#endif

	fprintf(stderr, "hwebcam, ver. %s\n\n", VERSION);
	newline = 1;

#ifdef USE_SDL
	while ((opt = getopt(argc, argv, "hvd:I:s:i:f:gt:eo:b:q:waD:c:r:B:Q:")) != -1) {
#else
	while ((opt = getopt(argc, argv, "hvd:I:s:i:f:gt:eo:b:q:aD:c:r:B:Q:")) != -1) {
#endif
		switch (opt) {
		case 'h':
			usage();
			exit(0);
			break;
		case 'v':
			verbose++;
			break;
		case 'd':
			videodevice = optarg;
			break;
		case 'I':
			ninput = atoi(optarg);
			break;
		case 's':
			ret = sscanf(optarg, "%dx%d", &width, &height);
			if (ret != 2) {
				fprintf(stderr, "Error: cannot read frame size\n\n");
				usage();
				exit(1);
			}
			break;
		case 'i':
			fps.num   = atoi(optarg);
			fps.den   = 1;
			fps.ratio = (double)fps.num / (double)fps.den;
			fps.tpf_us= 0.5 + 1000000.0 * (double)fps.den / (double)fps.num;
			break;
		case 'f':
			if (!strcmp(optarg, "yuyv"))
				pixelformat = V4L2_PIX_FMT_YUYV;
			else if (!strcmp(optarg, "yuv"))
				pixelformat = V4L2_PIX_FMT_YUV420;
			else if (!strcmp(optarg, "mjpeg"))
				pixelformat = V4L2_PIX_FMT_MJPEG;
			else if (!strcmp(optarg, "jpeg"))
				pixelformat = V4L2_PIX_FMT_JPEG;
			else {
				fprintf(stderr, "Error: unknown pixelformat\n\n");
				usage();
				exit(1);
			}
			break;
		case 'g':
			gray = 1;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 'e':
			encode_theora = 1;
			break;
		case 'o':
			videoname = optarg;
			break;
		case 'b':
			bitrate_theora = atoi(optarg);
			break;
		case 'q':
			quality_theora = atoi(optarg);
			bitrate_theora = 0;
			break;
#ifdef USE_SDL
		case 'w':
			sdl_window = 1;
			break;
#endif
		case 'a':
			encode_vorbis = 1;
			break;
		case 'D':
			audiodevice = optarg;
			break;
		case 'c':
			channels = atoi(optarg);
			break;
		case 'r':
			rate = atoi(optarg);
			break;
		case 'B':
			bitrate_vorbis = atoi(optarg);
			break;
		case 'Q':
			quality_vorbis = atof(optarg);
			bitrate_vorbis = 0;
			break;
		default:
			fprintf(stderr, "\n");
			usage();
			exit(1);
		}
	}

#if DEBUG
	fprintf(stderr,
	        "v=%d.  Video: d=%s, I=%d, s=%dx%d, i=%d, g=%d, t=%d, e=%d, o=%s, b=%d, q=%d, w=%d."
	        "  Audio: a=%d, D=%s, c=%u, r=%u, B=%d, Q=%.2f\n",
	        verbose, videodevice, ninput, width, height, fps.num, gray, timeout, encode_theora,
	        videoname, bitrate_theora, quality_theora, sdl_window, encode_vorbis,
	        audiodevice, channels, rate, bitrate_vorbis, quality_vorbis);
#endif

	if (width<0 || width>1280 || height<0 || height>1024 || fps.num<0 || fps.num>30 ||
	    bitrate_theora<0 || quality_theora<0 || quality_theora>63 ||
	    channels>6 || rate>96000 || bitrate_vorbis<0 || quality_vorbis<-0.1f ||
	    quality_vorbis>1.0f) {
		fprintf(stderr, "Error: parameters out of bounds\n\n");
		usage();
		exit(1);
	}

	/* we can only do: theora, theora+vorbis. we don't do vorbis alone */
	if (encode_vorbis == 1 && encode_theora == 0)
		encode_vorbis = 0;

	/* set up simd optimized conversion functions on x86 */
	yuyv422_to_y8 = yuyv422_to_y8_c;
	yuyv422_to_yuv420p = yuyv422_to_yuv420p_c;
#if defined(ARCH_X86_32) || defined(ARCH_X86_64)
	ret = x86_get_cpu_flags(verbose >= 2 ? 1 : 0);
	if (ret & CPU_FLAG_SSE2) {
		yuyv422_to_y8 = yuyv422_to_y8_sse2;
		yuyv422_to_yuv420p = yuyv422_to_yuv420p_sse2;
	} else if ((ret & CPU_FLAG_MMXEXT) || ((ret & CPU_FLAG_MMX) && (ret & CPU_FLAG_SSE))) {
		yuyv422_to_y8 = yuyv422_to_y8_mmxext;
		yuyv422_to_yuv420p = yuyv422_to_yuv420p_mmxext;
	}
#endif

	/* Install signal handler */
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	ret = sigaction(SIGINT, &act, NULL);
	if (ret == -1)  {
		fprintf(stderr, "Error: sigaction() failed\n");
		exit(1);
	}

	/* Init video */
	ret = video_init(&vd, videodevice, ninput, width, height, fps.num, pixelformat, verbose >= 2 ? 1 : 0);
	if (ret == -1)
		exit(1);
	if ((unsigned int)width != vd.width || (unsigned int)height != vd.height ||
	    (unsigned int)fps.num != vd.tpfden || (unsigned int)fps.den != vd.tpfnum) {
		fprintf(stderr, "Warning: video device do not support %dx%d@%.2f, %dx%d@%.2f chosen instead\n",
		        width, height, fps.ratio, vd.width, vd.height, (double)vd.tpfden/(double)vd.tpfnum);
		width     = vd.width;
		height    = vd.height;
		fps.num   = vd.tpfden;
		fps.den   = vd.tpfnum;
		fps.ratio = (double)fps.num / (double)fps.den;
		fps.tpf_us= 0.5 + 1000000.0 * (double)fps.den / (double)fps.num;
	}
	/* file descriptor for video is always at pfds[0] */
	pfds[0].fd     = vd.fd;
	pfds[0].events = POLLIN;
	nfds = 1;
	/* if we are encoding in theora, width (and height) must be multiple of 16 */
	if (encode_theora && (width % 16)) {
		fprintf(stderr, "Error: image width is not multiple of 16\n");
		exit(1);
	}
	/* make height_theora multiple of 16. the last (height % 16) rows of pixels
	   will be discarded when compressing the stream */
	height_theora = height & 0xfffffff0;
	if (height_theora != height)
		fprintf(stderr, "Warning: discarding the last %d rows of pixels in encoded stream\n", height-height_theora);

	/* Init alsa audio */
	if (encode_vorbis) {
		ret = audio_init(&ad, audiodevice, SND_PCM_STREAM_CAPTURE,
		                 rate, channels, pfds+1, verbose >= 2 ? 1 : 0);
		if (ret == -1)
			exit(1);
		nfds += ad.nfds;
	}

	/* Prepare ppm image */
	if (timeout >= 0) {
		ppmimage.width  = width;
		ppmimage.height = height;
		ppmimage.max    = 255;
		ppmimage.data   = malloc(3 * ppmimage.width * ppmimage.height * sizeof(unsigned char));
		if (ppmimage.data == NULL)  {
			fprintf(stderr, "Error: malloc() failed\n");
			exit(1);
		}
	}

	/* Prepare theora and, if necessary, vorbis encoders */
	if (encode_theora) {
		/* open output file */
		if (!strcmp(videoname, "-"))
			fpvideo = stdout;
		else {
			fpvideo = fopen(videoname, "w");
			if (fpvideo == NULL) {
				fprintf(stderr, "Error: cannot open %s\n", videoname);
				exit(1);
			}
		}

		/* prepare ogg theora stream with random serialno */
		srand(time(NULL));
		ret = ogg_stream_init(&ostream_theora, rand());
		if (ret != 0) {
			fprintf(stderr, "Error: ogg_stream_init() failed\n");
			exit(1);
		}

		/* configure theora encoder */
		th_info_init(&ti);
		th_comment_init(&tc);
		th_comment_add(&tc, "ENCODER=hwebcam "VERSION);

		ti.frame_width            = width;
		ti.frame_height           = height_theora;
		ti.pic_width              = ti.frame_width;
		ti.pic_height             = ti.frame_height;
		ti.pic_x                  = 0;
		ti.pic_y                  = 0;
		ti.colorspace             = TH_CS_UNSPECIFIED;
		ti.pixel_fmt              = TH_PF_420;
		ti.target_bitrate         = bitrate_theora;
		ti.quality                = quality_theora;
		ti.keyframe_granule_shift = 6;
		ti.version_major          = 0;
		ti.version_minor          = 0;
		ti.version_subminor       = 0;
		ti.fps_numerator          = fps.num;
		ti.fps_denominator        = fps.den;
		ti.aspect_numerator       = 1;
		ti.aspect_denominator     = 1;

		/* allocate theora encoder handle */
		tctx = th_encode_alloc(&ti);
		if (tctx == NULL) {
			fprintf(stderr, "Error: th_encode_alloc() failed\n");
			exit(1);
		}

		/* We need to push the packets/pages in this order:
		   1st       theora header packet, in his own page
		   1st       vorbis header packet, in his own page    (if present)
		   remaining vorbis headers, flush the page when done (if present)
		   remaining theora headers, flush the page when done  */

		/* 1st theora header packet */
		if ((ret = th_encode_flushheader(tctx, &tc, &opacket_theora)) > 0) {
			if (ogg_stream_packetin(&ostream_theora, &opacket_theora) == 0) {
				while (ogg_stream_pageout(&ostream_theora, &opage)) {
					fwrite(opage.header, 1,  opage.header_len, fpvideo);
					fwrite(opage.body,   1,  opage.body_len,   fpvideo);
				}
			} else {
				fprintf(stderr, "Error: ogg_stream_packein() failed\n");
				exit(1);
			}
		}
		if (ret == TH_EFAULT) {
			fprintf(stderr, "Error: th_encode_flushheader() failed\n");
			exit(1);
		}


		if (encode_vorbis) {
			/* prepare ogg vorbis stream with random serialno */
			ret = ogg_stream_init(&ostream_vorbis, rand());
			if (ret != 0) {
				fprintf(stderr, "Error: ogg_stream_init() failed\n");
				exit(1);
			}

			/* configure vorbis and initialize encoder */
			vorbis_info_init(&vi);
			if (bitrate_vorbis == 0)
				ret = vorbis_encode_init_vbr(&vi, ad.channels, ad.rate, quality_vorbis);
			else
				ret = vorbis_encode_init(&vi, ad.channels, ad.rate, -1, bitrate_vorbis, -1);
			if (ret != 0) {
				fprintf(stderr, "Error: vorbis_encode_init*() failed\n");
				exit(1);
			}
			vorbis_analysis_init(&vds, &vi);
			vorbis_comment_init(&vc);
			vorbis_comment_add(&vc, "ENCODER=hwebcam "VERSION);

			/* vorbis header packets */
			ret = vorbis_analysis_headerout(&vds, &vc,
			      &opacket_vorbis[0], &opacket_vorbis[1], &opacket_vorbis[2]);
			if (ret != 0) {
				fprintf(stderr, "Error: vorbis_analysis_headerout() failed\n");
				exit(1);
			}
			for (i = 0; i < 3; i++)  {
				if (ogg_stream_packetin(&ostream_vorbis, &opacket_vorbis[i]) == 0) {
					while (ogg_stream_pageout(&ostream_vorbis, &opage)) {
						fwrite(opage.header, 1, opage.header_len, fpvideo);
						fwrite(opage.body  , 1, opage.body_len,   fpvideo);
					}
				} else {
					fprintf(stderr, "Error: ogg_stream_packein() failed\n");
					exit(1);
				}
			}

			/* force remaining vorbis header packets into a page */
			while (ogg_stream_flush(&ostream_vorbis, &opage)) {
				fwrite(opage.header, 1, opage.header_len, fpvideo);
				fwrite(opage.body,   1, opage.body_len,   fpvideo);
			}

			vorbis_block_init(&vds, &vb);

			/* ok, vorbis encoder is ready */
			keep_going_vorbis = 1;
		}

		/* remaining theora header packets */
		while ((ret = th_encode_flushheader(tctx, &tc, &opacket_theora)) > 0) {
			if (ogg_stream_packetin(&ostream_theora, &opacket_theora) == 0) {
				while (ogg_stream_pageout(&ostream_theora, &opage)) {
					fwrite(opage.header, 1,  opage.header_len, fpvideo);
					fwrite(opage.body,   1,  opage.body_len,   fpvideo);
				}
			} else {
				fprintf(stderr, "Error: ogg_stream_packein() failed\n");
				exit(1);
			}
		}
		if (ret == TH_EFAULT) {
			fprintf(stderr, "Error: th_encode_flushheader() failed\n");
			exit(1);
		}
		/* force remaining theora header packets into a page */
		while (ogg_stream_flush(&ostream_theora, &opage)) {
			fwrite(opage.header,  1, opage.header_len, fpvideo);
			fwrite(opage.body,    1, opage.body_len,   fpvideo);
		}

		/* ok, theora encoder is ready */
		keep_going_theora = 1;

		/* now, set up theora's ycbcr buffer */
		memset(ycbcrbuf, 0, sizeof(ycbcrbuf));

		for (i = 0; i < 3; i++) {
			int s;

			s = (i == 0) ? 0 : 1;
			ycbcrbuf[i].width  = width         >> s;
			ycbcrbuf[i].height = height_theora >> s;
			ycbcrbuf[i].stride = ycbcrbuf[i].width;
			ret = posix_memalign((void **)&ycbcrbuf[i].data, 16, ycbcrbuf[i].stride * ycbcrbuf[i].height);

			if (ret || !ycbcrbuf[i].data) {
				fprintf(stderr, "Error: posix_memalign() failed\n");
				exit(1);
			}

			memset(ycbcrbuf[i].data, (i == 0) ? 16 : 128, ycbcrbuf[i].stride * ycbcrbuf[i].height);
		}

		/* prepare muxer */
		muxer.fpvideo   = fpvideo;
		muxer.tctx      = tctx;
		muxer.vq_head   = NULL;
		muxer.vq_tail   = NULL;
		if (encode_vorbis) {
			muxer.haveaudio = 1;
			muxer.vds       = &vds;
		} else {
			muxer.haveaudio = 0;
			muxer.vds       = NULL;
		}
		muxer.aq_head   = NULL;
		muxer.aq_tail   = NULL;
	}

#ifdef USE_SDL
	if (sdl_window) {
		/* Initialize SDL Video subsystem */
		if (verbose)
			fprintf(stderr, "Initializing SDL...\n");
		if (SDL_Init(SDL_INIT_VIDEO) == -1) {
			fprintf(stderr, "Error: could not initialize SDL: %s\n", SDL_GetError());
			exit(1);
		}

		/* Clean up on exit */
		atexit(SDL_Quit);

		/* Initialize the display requesting a software surface */
		surface = SDL_SetVideoMode(width, height, 24, SDL_SWSURFACE|SDL_ANYFORMAT);
		if (surface == NULL) {
			fprintf(stderr, "Error: couldn't set requested video mode: %s\n", SDL_GetError());
			exit(1);
		}
		if (verbose)
			fprintf(stderr, "  Surface: %dx%d at %d bits-per-pixel mode\n",
			        surface->w, surface->h, surface->format->BitsPerPixel);

		/* Create a YUV video overlay  */
		if (pixelformat == V4L2_PIX_FMT_YUV420)
			overlay = SDL_CreateYUVOverlay(width, height, SDL_IYUV_OVERLAY, surface);
		else
			overlay = SDL_CreateYUVOverlay(width, height, SDL_YUY2_OVERLAY, surface);
		if (verbose) {
			fprintf(stderr, "  Overlay:\n");
			fprintf(stderr, "    Dimensions:     %dx%d\n", overlay->w, overlay->h);
			fprintf(stderr, "    Format:         0x%08x\n", overlay->format);
			fprintf(stderr, "    Planes:         %d\n", overlay->planes);
			for (i = 0; i < overlay->planes; i++)
				fprintf(stderr, "      Pitches[%d]:   %d\n", i, overlay->pitches[i]);
			fprintf(stderr, "    Hw accelerated: %d\n", overlay->hw_overlay);
		}

		/* Set the window title and icon name */
		SDL_WM_SetCaption("hwebcam", "hwebcam");
	}
#endif

	/* Print streaming info */
	fprintf(stderr, "Info:\n");
	fprintf(stderr, "  Video device: %s\n", videodevice);
	fprintf(stderr, "  Frame size:   %dx%d\n", width, height);
	fprintf(stderr, "  Frame rate:   %.2f fps\n", fps.ratio);
	fprintf(stderr, "  Frame format: %s\n", pixelformat_to_str(pixelformat));
	if (timeout >= 0)
	fprintf(stderr, "  Save frame:   every %d seconds\n", timeout);
#ifdef USE_SDL
	fprintf(stderr, "  Window:       %s\n", sdl_window ? "SDL" : "none");
#endif
	if (encode_theora) {
	fprintf(stderr, "  Theora stream:\n");
	fprintf(stderr, "    libtheora:  %s\n", th_version_string());
	fprintf(stderr, "    File name:  %s\n", strcmp(videoname, "-") == 0 ? "stdout" : videoname);
	if (bitrate_theora > 0)
	fprintf(stderr, "    Bitrate:    %d bit/s\n", ti.target_bitrate);
	else
	fprintf(stderr, "    Quality:    %d\n", ti.quality);
	}
	if (encode_vorbis) {
	fprintf(stderr, "  Audio device: %s\n", audiodevice);
	fprintf(stderr, "  Vorbis stream:\n");
	fprintf(stderr, "    libvorbis:  %s\n", vorbis_version_string());
	fprintf(stderr, "    Channels:   %d\n", vi.channels);
	fprintf(stderr, "    Rate:       %ld\n", vi.rate);
	if (bitrate_vorbis > 0)
	fprintf(stderr, "    Bitrate:    %d bit/s\n", bitrate_vorbis);
	else
	fprintf(stderr, "    Quality:    %.2f\n", quality_vorbis);
	}

	/* Main loop */
	/* start video capture */
	ret = video_start_streaming(&vd);
	if (ret == -1)
		exit(1);
	/* wait until the first video frame is ready */
	pollret = poll(pfds, 1, -1);

	if (encode_vorbis) {
		/* start audio capture */
		ret = audio_start(&ad);
		if (ret == -1)
			exit(1);
	}

	if (verbose)
		fprintf(stderr, "\n");
	frame_start.tv_sec  = 0;	/* silence warning */
	frame_start.tv_usec = 0;
	timeout_start = time(NULL);
	t_decp = 0;
	t_enca = 0;
	t_encv = 0;
#ifdef USE_SDL
	t_sdl  = 0;
#endif
	ifc = 0; /* frame counter */
	while (keep_going || keep_going_theora || keep_going_vorbis) {
		/* poll: is audio or video ready? */
		pollret = poll(pfds, nfds, -1);

		/* poll can fail and return -1 when a signal arrives. errno will be EINTR */
		if (pollret == -1) {
			if (errno == EINTR) {
				continue;
			} else {
				snprintf(msgbuf, MBS, "poll(): %s\n", strerror(errno));
				die(msgbuf, 1);
			}
		}
#if 0
		/* timeout should never happen really... */
		if (pollret == 0) {
			print_msg_nl("Poll timeout\n");
			continue;
		}
#endif
		/** AUDIO **/
		if (encode_vorbis) {
			audio_poll_descriptors_revents(&ad);

			/* this will happen in case of overrun (POLLIN will also be set) */
			if (ad.revents & POLLERR) {
				if (!(ad.revents & POLLIN))
					die("Audio POLLERR\n", 1);
			}

			/* process audio if ready */
			if (ad.revents & POLLIN && keep_going_vorbis) {
				unsigned int c;
				int s;

				/* read available frames */
				ret = audio_avail(&ad, 1);

				/* read ad.period_size frames of audio */
				ret = audio_read(&ad, 1);
				if (ret < 0) {
					print_msg_nl("Buffer overrun\n");
					/* overrun, let's handle it and restart the stream. we will lose
				 	   the entire ring buffer (which was full) so the a/v sync
					   will be screwed up from now on, but anyway... */
					ret = audio_recover(&ad, ret, 1);
					if (ret == 0) {
						ret = audio_start(&ad);
						if (ret == -1)
							die(NULL, 1);
						ret = audio_avail(&ad, 0);
						if (ret < 0)
							die(NULL, 1);
						ret = audio_read(&ad, 0);
						if (ret < 0)
							die(NULL, 1);
					} else {
						die(NULL, 1);
					}
				}

				/* update audio time counter */
				time_audio_us += 10000;

				/* encoding */
				gettimeofday(&tea0, NULL);

				if (peak_count >= 100) {
					peak_count   = 0;
					peak_percent = 0.0f;
				}
				audio_find_peak(&ad);
				if (peak_percent < ad.peak_percent)
					peak_percent = ad.peak_percent;
				peak_count++;

				vorbisbuffer = vorbis_analysis_buffer(&vds, ad.frames);
				for (c = 0; c < ad.channels; c++) {
					for (s = 0; s < ad.frames; s++) {
						vorbisbuffer[c][s] = (float)ad.alsabuffer[s * ad.channels + c] * (1.0f/32768.0f);
					}
				}
				if (keep_going == 0)
					keep_going_vorbis = 0;

				ret = vorbis_analysis_wrote(&vds, keep_going_vorbis ? ad.frames : 0);
				if (ret != 0)
					die("Error: vorbis_analysis_wrote() failed\n", 1);

				while ((ret = vorbis_analysis_blockout(&vds, &vb)) > 0) {
					ret = vorbis_analysis(&vb, NULL);
					if (ret != 0)
						die("Error: vorbis_analysis() failed\n", 1);

					ret = vorbis_bitrate_addblock(&vb);
					if (ret != 0)
						die("Error: vorbis_bitrate_addblock() failed\n", 1);

					while ((ret = vorbis_bitrate_flushpacket(&vds, &opacket_vorbis[0])) > 0) {
						if (ogg_stream_packetin(&ostream_vorbis, &opacket_vorbis[0]) == 0) {
							while (ogg_stream_pageout(&ostream_vorbis, &opage))
								muxer_add_page(&muxer, &opage, AUDIO);
						} else {
							die("Error: ogg_stream_packein() failed\n", 1);
						}
					}
					if (ret < 0)
						die("Error: vorbis_bitrate_flushpacket() failed\n", 1);
				}
				if (ret < 0)
					die("Error: vorbis_analysis_blockout() failed\n", 1);

				muxer_write(&muxer, 0);
				gettimeofday(&tea1, NULL);
				t_enca = (tea1.tv_sec - tea0.tv_sec) * 1000000L + tea1.tv_usec - tea0.tv_usec;
			}
		}

		/** VIDEO **/
		if (pfds[0].revents & POLLERR)
			die("Video POLLERR\n", 1);

		/* process video if ready */
		if (pfds[0].revents & POLLIN) {
			/* get a video frame */
			ret = video_frame_get(&vd);
			if (ret == -1)
				die(NULL, 1);

			/* compute frame time and actual fps */
			frame_stop     = vd.buffer.timestamp;
			if (ifc == 0)
				frame_start= frame_stop;
			frame_interval = (frame_stop.tv_sec - frame_start.tv_sec) * 1000000L +
			                  frame_stop.tv_usec - frame_start.tv_usec;
			actual_fps     = (frame_interval == 0) ? fps.ratio : (1000000.0 / (double)frame_interval);
			frame_start    = vd.buffer.timestamp;

#if 0
			/* Dump raw data */
			sprintf(imagename, "%08d.raw", ifc);
			fpimage = fopen(imagename, "w");
			if (fpimage == NULL)  {
				snprintf(msgbuf, MBS, "Error: cannot open %s\n", imagename);
				die(msgbuf, 1);
			}
			fwrite(vd.buffers[vd.buffer.index].start, 1, vd.buffer.bytesused, fpimage);
			fclose(fpimage);
#endif

			/* decompress jpeg image only if we need it */
			if ((pixelformat == V4L2_PIX_FMT_MJPEG || pixelformat == V4L2_PIX_FMT_JPEG) && (encode_theora
#ifdef USE_SDL
			|| sdl_window
#endif
			)) {
				gettimeofday(&tdp0, NULL);
				ret = jpeg_decompress(vd.jpegbuf, vd.jpegbufused, vd.yuvbuf);
				gettimeofday(&tdp1, NULL);
				t_decp = (tdp1.tv_sec - tdp0.tv_sec) * 1000000L + tdp1.tv_usec - tdp0.tv_usec;
				if (ret == -1)
					die(NULL, 1);
			}

#ifdef USE_SDL
			if (sdl_window) {
				gettimeofday(&tsdl0, NULL);
				/* Fill in pixel data */
				SDL_LockYUVOverlay(overlay);
				if (pixelformat == V4L2_PIX_FMT_YUV420) {
					/* chroma planes start address */
					unsigned char *p = vd.yuvbuf + width * height;
					/* copy Y plane */
					for (i = 0; i < height; i++)
						memcpy(overlay->pixels[0]+i*overlay->pitches[0],
						       vd.yuvbuf+i*overlay->pitches[0],
						       (size_t) overlay->pitches[0]);
					/* copy Cb and Cr planes */
					for (i = 0; i < height/2; i++)
						memcpy(overlay->pixels[1]+i*overlay->pitches[1],
						       p+i*overlay->pitches[1],
						       (size_t) overlay->pitches[1]);
					p += (width * height) / 4;
					for (i = 0; i < height/2; i++)
						memcpy(overlay->pixels[2]+i*overlay->pitches[2],
						       p+i*overlay->pitches[2],
						       (size_t) overlay->pitches[2]);

				} else {
					for (i = 0; i < height; i++)
						memcpy(overlay->pixels[0]+i*overlay->pitches[0],
							   vd.yuvbuf+i*overlay->pitches[0],
							   (size_t) overlay->pitches[0]);
				}
				SDL_UnlockYUVOverlay(overlay);
				/* Display frame */
				video_rect.x = 0;
				video_rect.y = 0;
				video_rect.w = width;
				video_rect.h = height;
				SDL_DisplayYUVOverlay(overlay, &video_rect);
				/* Print fps in window's title, every 1 second */
				if (actual_fps > 0.5 && (ifc % (int)(actual_fps+0.5)) == 0) {
					sprintf(caption_string, "hwebcam - %5.2f fps", actual_fps);
					SDL_WM_SetCaption(caption_string, "hwebcam");
				}
				gettimeofday(&tsdl1, NULL);
				t_sdl = (tsdl1.tv_sec - tsdl0.tv_sec) * 1000000L + tsdl1.tv_usec - tsdl0.tv_usec;
			}
#endif

			if (encode_theora && keep_going_theora) {
				gettimeofday(&tev0, NULL);
				/* prepare the frame for the encoder */
				if (pixelformat == V4L2_PIX_FMT_YUV420) {
					memcpy(ycbcrbuf[0].data, vd.yuvbuf, (size_t) (ycbcrbuf[0].width * ycbcrbuf[0].height));
					if (!gray) {
						memcpy(ycbcrbuf[1].data, vd.yuvbuf+width*height,
						      (size_t) (ycbcrbuf[1].width * ycbcrbuf[1].height));
						memcpy(ycbcrbuf[2].data, vd.yuvbuf+(5*width*height)/4,
						      (size_t) (ycbcrbuf[2].width * ycbcrbuf[2].height));
					}
				} else {
					if (gray)
						yuyv422_to_y8(vd.yuvbuf, ycbcrbuf);
					else
						yuyv422_to_yuv420p(vd.yuvbuf, ycbcrbuf);
				}

				/* if actual_fps==fps/2, push 2 frames into the encoder. if fps/4, push 4 frames */
				duplicate = 0;
				if (0.40 * fps.ratio < actual_fps && actual_fps < 0.60 * fps.ratio)
					duplicate = 1;
				else if (0.20 * fps.ratio < actual_fps && actual_fps < 0.30 * fps.ratio)
					duplicate = 3;

				/* update video time counter */
				time_video_us += fps.tpf_us * (duplicate + 1);

				/* maintain A/V sync by dropping/duplicating frames
				   we will skip this frame if duplicate = -1 */
				if (encode_vorbis) {
					long int diff = time_audio_us - time_video_us;

					if (labs(diff) > fps.tpf_us)
						av_desync++;
					else
						av_desync = 0;

					/* correct A/V sync only once a second */
					if (av_desync >= (long int)(fps.ratio+0.5)) {
						if (diff < (-fps.tpf_us)) {
							duplicate--;
							time_video_us -= fps.tpf_us;
							print_msg_nl("Dropping one frame to maintain A/V sync\n");
						} else if (diff > fps.tpf_us) {
							duplicate++;
							time_video_us += fps.tpf_us;
							print_msg_nl("Inserting one frame to maintain A/V sync\n");
						}
						av_desync = 0;
					}
				}

				if (keep_going == 0)
					keep_going_theora = 0;

				if (duplicate >= 0) {
					/* set the number of duplicates of the next frame to produce */
					if (duplicate > 0) {
						ret = th_encode_ctl(tctx, TH_ENCCTL_SET_DUP_COUNT, &duplicate, sizeof(duplicate));
						if (ret != 0)
							die("Error: th_encode_ctl() failed\n", 1);
					}

					/* pass the frame to the encoder */
					if (th_encode_ycbcr_in(tctx, ycbcrbuf) != 0)
						die("Error: th_encode_ycbcr_in() failed\n", 1);

					/* get the packet out and pass it to the ogg stream */
					while ((ret = th_encode_packetout(tctx, !keep_going_theora, &opacket_theora)) > 0) {
						if (ogg_stream_packetin(&ostream_theora, &opacket_theora) == 0) {
							while (ogg_stream_pageout(&ostream_theora, &opage))
								muxer_add_page(&muxer, &opage, VIDEO);
						} else {
							die("Error: ogg_stream_packein() failed\n", 1);
						}
					}
					if (ret == TH_EFAULT)
						die("Error: th_encode_packetout() failed\n", 1);

					muxer_write(&muxer, 0);
				}
				gettimeofday(&tev1, NULL);
				t_encv = (tev1.tv_sec - tev0.tv_sec) * 1000000L + tev1.tv_usec - tev0.tv_usec;
			}

			if (timeout >= 0) {
				timeout_stop = time(NULL);
				if (difftime(timeout_stop, timeout_start) >= timeout) {
					if (pixelformat == V4L2_PIX_FMT_MJPEG || pixelformat == V4L2_PIX_FMT_JPEG) {
						/* Dump jpeg image */
						sprintf(imagename, "image.jpg");
						fpimage = fopen(imagename, "w");
						if (fpimage == NULL)  {
							snprintf(msgbuf, MBS, "Error: cannot open %s\n", imagename);
							die(msgbuf, 1);
						}
						fwrite(vd.jpegbuf, 1, vd.jpegbufused, fpimage);
						fclose(fpimage);
					} else if (pixelformat == V4L2_PIX_FMT_YUYV){ /* TODO YUV420 */
						/* Dump ppm image */
						sprintf(imagename, "image.ppm");
						fpimage = fopen(imagename, "w");
						if (fpimage == NULL)  {
							snprintf(msgbuf, MBS, "Error: cannot open %s\n", imagename);
							die(msgbuf, 1);
						}
						yuyv422_to_rgb24(vd.yuvbuf, &ppmimage);
						fprintf(fpimage, "P6\n%u %u\n%u\n", ppmimage.width, ppmimage.height, ppmimage.max);
						fwrite(ppmimage.data, 3, ppmimage.width * ppmimage.height, fpimage);
						fclose(fpimage);
					}
					if (verbose)
						print_msg_nl("Image saved\n");
					timeout_start = time(NULL);
				}
			}

			/* print stats in verbose mode every frame if actual_fps<=10, or every 2 frames */
			if (verbose && ((int)(actual_fps+0.5)<=10 || !(ifc%2))) {
				if (verbose == 1) {
					fprintf(stderr, "\rf:%5d  fps:%5.2f", ifc, actual_fps);
					if (encode_vorbis)
					fprintf(stderr, "  ap:%5.1f%%  A-V:%4lldms",
					        peak_percent, (time_audio_us-time_video_us+500LL)/1000LL);

				} else {
					fprintf(stderr, "\rf:%5d  tf:%6ldus  fps:%5.2f", ifc, frame_interval, actual_fps);
					if (pixelformat == V4L2_PIX_FMT_MJPEG || pixelformat == V4L2_PIX_FMT_JPEG)
					fprintf(stderr, "  tdp:%3ldms", (t_decp+500)/1000);
#ifdef USE_SDL
					if (sdl_window)
					fprintf(stderr, "  tsdl:%3ldms", (t_sdl+500)/1000);
#endif
					if (encode_theora)
					fprintf(stderr, "  tev:%3ldms", (t_encv+500)/1000);
					if (encode_vorbis)
					fprintf(stderr, "  tea:%2ldms  ap:%5.1f%%  rb:%3.0f%%  A-V:%4lldms",
					       (t_enca+500)/1000, peak_percent, 100.0f * ad.avail/(float)ad.buffer_size,
					       (time_audio_us-time_video_us+500LL)/1000LL);
				}
				fflush(stderr);
				newline = 0;
			}

			/* ok, we're done with this frame */
			ret = video_frame_done(&vd);
			if (ret == -1)
				die(NULL, 1);

			/* increment frame counter */
			ifc++;
		}
	}
	fprintf(stderr, "\n");

	ret = video_close(&vd);

	/* clean up */
	if (encode_theora) {

		muxer_write(&muxer, 1);

		th_encode_free(tctx);
		th_info_clear(&ti);
		th_comment_clear(&tc);
		ogg_stream_clear(&ostream_theora);

		if (encode_vorbis) {
			ret = audio_stop(&ad);
			ret = audio_close(&ad);

			vorbis_info_clear(&vi);
			vorbis_comment_clear(&vc);
			vorbis_block_clear(&vb);
			vorbis_dsp_clear(&vds);

			ogg_stream_clear(&ostream_vorbis);
		}

		if (strcmp(videoname, "-"))
			fclose(fpvideo);
	}

	exit(0);
}
