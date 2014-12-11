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
#include <setjmp.h>
#include <jpeglib.h>
#include "jpeg.h"
#include "common.h"

#define DEBUG 0
#define MSGPREFIX "[jpeg] "

struct private_data {
	const unsigned char *jpegbuf;
	size_t               jpegbufused;
	jmp_buf              env;
	int                  valid;
};

static void my_output_message(j_common_ptr cinfo)
{
	struct private_data *pd = cinfo->client_data;

	/* create the message */
	strcpy(msgbuf, MSGPREFIX);
	(*cinfo->err->format_message) (cinfo, msgbuf+strlen(msgbuf));
	strcat(msgbuf, "\n");

	/* print error message */
	print_msg_nl(msgbuf);

	/* something happened during decoding, image is not valid */
	pd->valid = 0;
}

static void my_error_exit(j_common_ptr cinfo)
{
	struct private_data *pd = cinfo->client_data;

	/* always display the error message */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(pd->env, 1);
}

static void init_source(j_decompress_ptr cinfo)
{
	struct private_data *pd = cinfo->client_data;

	cinfo->src->next_input_byte = pd->jpegbuf;
	cinfo->src->bytes_in_buffer = pd->jpegbufused;
}

static boolean fill_input_buffer(j_decompress_ptr cinfo)
{
	struct private_data *pd = cinfo->client_data;

	print_msg_nl(MSGPREFIX"fill_input_buffer() called, aborting\n");

	/* Return control to the setjmp point */
	longjmp(pd->env, 1);
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
/*	fprintf(stderr, MSGPREFIX"skip_input_data: num_bytes=%ld, bytes_in_buffer=%zd\n",
	        num_bytes, cinfo->src->bytes_in_buffer); */
	cinfo->src->next_input_byte += num_bytes;
	cinfo->src->bytes_in_buffer -= num_bytes;
/*	fprintf(stderr, MSGPREFIX"skip_input_data: bytes_in_buffer=%zd\n", cinfo->src->bytes_in_buffer); */
}

static void term_source(j_decompress_ptr cinfo)
{
	(void) cinfo;
}

static void yuv444_to_yuyv422(const unsigned char *src, unsigned int width, unsigned int height, unsigned char *yuyvbuf)
{
	unsigned int i, j;
	unsigned int y1, y2, cb, cr, i_rows;
	const unsigned char *p;

	p = src;
	for (i = 0; i < height; i++) {
		i_rows = i*2*width;
		for (j = 0; j < 2*width; j += 4) {
			y1 = p[0];
			cb = (p[1] + p[4] + 1) >> 1;
			y2 = p[3];
			cr = (p[2] + p[5] + 1) >> 1;
			*(yuyvbuf + i_rows + j  ) = y1;
			*(yuyvbuf + i_rows + j+1) = cb;
			*(yuyvbuf + i_rows + j+2) = y2;
			*(yuyvbuf + i_rows + j+3) = cr;
			p += 6;
		}
	}
}

int jpeg_decompress(const unsigned char *jpegbuf, size_t jpegbufused, unsigned char *yuyvbuf)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr         jerr;
	struct jpeg_source_mgr        jsm;
	struct private_data           pd;
	JSAMPROW row_pointer[1];
	unsigned int width, height;
	unsigned char *dimage = NULL;

	/* prepare private data */
	pd.jpegbuf        = jpegbuf;
	pd.jpegbufused    = jpegbufused;
	pd.valid          = 1;
	cinfo.client_data = &pd;

	/* error manager */
	cinfo.err           = jpeg_std_error(&jerr);
	jerr.error_exit     = my_error_exit;
	jerr.output_message = my_output_message;

	/* establish the setjmp return context for my_error_exit to use. */
	if (setjmp(pd.env)) {
		jpeg_destroy_decompress(&cinfo);
		free(dimage);
		return 0;
	}

	/* initialize decompression object */
	jpeg_create_decompress(&cinfo);

	/* source manager */
	jsm.next_input_byte   = NULL;
	jsm.bytes_in_buffer   = 0;
	jsm.init_source       = init_source;
	jsm.fill_input_buffer = fill_input_buffer;
	jsm.skip_input_data   = skip_input_data;
	jsm.resync_to_restart = jpeg_resync_to_restart;
	jsm.term_source       = term_source;
	cinfo.src             = &jsm;

	/* read header */
	jpeg_read_header(&cinfo, TRUE);

#if DEBUG
	fprintf(stderr, MSGPREFIX"image_width      = %d\n", cinfo.image_width);
	fprintf(stderr, MSGPREFIX"image_height     = %d\n", cinfo.image_height);
	fprintf(stderr, MSGPREFIX"num_components   = %d\n", cinfo.num_components);
	fprintf(stderr, MSGPREFIX"jpeg_color_space = %d\n", cinfo.jpeg_color_space);
	fprintf(stderr, MSGPREFIX"\n");
#endif

	/* select output color space */
	cinfo.out_color_space = JCS_YCbCr;

	/* start decompress */
	jpeg_start_decompress(&cinfo);
	width  = cinfo.output_width;
	height = cinfo.output_height;

#if DEBUG
	fprintf(stderr, MSGPREFIX"out_color_space      = %d\n", cinfo.out_color_space);
	fprintf(stderr, MSGPREFIX"output_width         = %d\n", cinfo.output_width);
	fprintf(stderr, MSGPREFIX"output_height        = %d\n", cinfo.output_height);
	fprintf(stderr, MSGPREFIX"out_color_components = %d\n", cinfo.out_color_components);
	fprintf(stderr, MSGPREFIX"output_components    = %d\n", cinfo.output_components);
	fprintf(stderr, MSGPREFIX"\n");
#endif

	/* allocate temporary buffer for decompressed image */
	dimage = malloc((cinfo.output_width * cinfo.output_components) * cinfo.image_height);
	if (dimage == NULL) {
		print_msg_nl(MSGPREFIX"Error: calloc() failed\n");
		return -1;
	}

	/* decompression loop */
	while (cinfo.output_scanline < cinfo.output_height) {
		row_pointer[0] = dimage + (cinfo.output_width * cinfo.output_components) * cinfo.output_scanline;
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
	}

	/* finish decompress */
	jpeg_finish_decompress(&cinfo);

	/* release decompression object */
	jpeg_destroy_decompress(&cinfo);


	/* convert packed yuv444 to packed yuyv422 */
	if (pd.valid)
		yuv444_to_yuyv422(dimage, width, height, yuyvbuf);

	/* free temporary buffer */
	free(dimage);

	return 0;
}
