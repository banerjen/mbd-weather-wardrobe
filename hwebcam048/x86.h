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

#ifndef X86_H
#define X86_H

#include <theora/theoraenc.h>

#define CPU_FLAG_MMX    0x01
#define CPU_FLAG_MMXEXT 0x02
#define CPU_FLAG_SSE    0x04
#define CPU_FLAG_SSE2   0x08

int x86_get_cpu_flags(int verbose);

void yuyv422_to_y8_mmxext(const unsigned char *src, th_ycbcr_buffer ycbcrbuf);
void yuyv422_to_y8_sse2(const unsigned char *src, th_ycbcr_buffer ycbcrbuf);

void yuyv422_to_yuv420p_mmxext(const unsigned char *src, th_ycbcr_buffer ycbcrbuf);
void yuyv422_to_yuv420p_sse2(const unsigned char *src, th_ycbcr_buffer ycbcrbuf);

#endif
