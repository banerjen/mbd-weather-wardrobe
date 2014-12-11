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

#define MSGPREFIX "[x86 ] "

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "x86.h"
#include "common.h"

union vendor_id {
	unsigned int v[3];
	char vendor_string[12];
};

union brand_id {
	unsigned int v[12];
	char brand_string[48];
};

#define CPUID(func, eax, ebx, ecx, edx)              \
	__asm__ __volatile__ (                           \
	"cpuid"                                          \
	: "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) \
	: "0" (func)                                     \
	)                                                \

int x86_get_cpu_flags(int verbose)
{
	int caps = 0;
	unsigned int max_func, func;
	unsigned int eax, ebx, ecx, edx;
	union vendor_id vid;
	union brand_id  bid;

	/* Standard Functions */
	func = 0;
	CPUID(func, eax, ebx, ecx, edx);

	max_func = eax;
	vid.v[0] = ebx;
	vid.v[1] = edx;
	vid.v[2] = ecx;
	if (verbose)
		fprintf(stderr, MSGPREFIX"Processor Vendor: '%.12s'\n", vid.vendor_string);

	if (max_func >= 1) {
		func = 1;
		CPUID(func, eax, ebx, ecx, edx);

		if (edx & (1U << 23))
			caps |= CPU_FLAG_MMX;
		if (edx & (1U << 25))
			caps |= CPU_FLAG_SSE;
		if (edx & (1U << 26))
			caps |= CPU_FLAG_SSE2;
	}

	/* Extended Functions */
	func = 0x80000000;
	CPUID(func, eax, ebx, ecx, edx);

	max_func = eax;
	vid.v[0] = ebx;
	vid.v[1] = edx;
	vid.v[2] = ecx;

	for (func = func + 1; func <= max_func; func++) {
		CPUID(func, eax, ebx, ecx, edx);

		if (func == 0x80000001 && !strncmp("AuthenticAMD", vid.vendor_string, 12)) {
			if (edx & (1U << 23))
				caps |= CPU_FLAG_MMX;
			if (edx & (1U << 22))
				caps |= CPU_FLAG_MMXEXT;
		}

		if (func == 0x80000002) {
			bid.v[0] = eax;
			bid.v[1] = ebx;
			bid.v[2] = ecx;
			bid.v[3] = edx;
		}
		if (func == 0x80000003) {
			bid.v[4] = eax;
			bid.v[5] = ebx;
			bid.v[6] = ecx;
			bid.v[7] = edx;
		}
		if (func == 0x80000004) {
			bid.v[ 8] = eax;
			bid.v[ 9] = ebx;
			bid.v[10] = ecx;
			bid.v[11] = edx;
		}
	}

	if (verbose) {
		if (max_func >= 0x80000004)
			fprintf(stderr, MSGPREFIX"Processor Brand:  '%.48s'\n", bid.brand_string);
		fprintf(stderr, MSGPREFIX"Caps:%s%s%s%s\n",
			(caps & CPU_FLAG_MMX)    ? " MMX"    : "",
			(caps & CPU_FLAG_MMXEXT) ? " MMXEXT" : "",
			(caps & CPU_FLAG_SSE)    ? " SSE"    : "",
			(caps & CPU_FLAG_SSE2)   ? " SSE2"   : "");
	}

	return caps;
}

void yuyv422_to_y8_mmxext(const unsigned char *src, th_ycbcr_buffer ycbcrbuf)
{
#ifdef ARCH_X86_32
	uint32_t i;
#else
	uint64_t i;
#endif
	const uint8_t *s;
	uint8_t *dy;

	s   = src;
	dy  = ycbcrbuf[0].data;
	i = (2 * ycbcrbuf[0].stride * ycbcrbuf[0].height) / 16;

	/* Make sure image width is multiple of 8 and src is 8-byte aligned
	   The alignment is not strictly necessary for mmx code, but improves speed */
	if (ycbcrbuf[0].stride & 7) {
		snprintf(msgbuf, MBS, MSGPREFIX"Error: image width not multiple of 8\n");
		die(msgbuf, 1);
	}
	if (((long)src) & 7L) {
		snprintf(msgbuf, MBS, MSGPREFIX"Error: source buffer not properly aligned\n");
		die(msgbuf, 1);
	}

	/* pavgw, pshufw, prefetchnta, movntq and sfence are mmxext/see instructions */
	__asm__ __volatile__ (
	"   pxor        %%mm6, %%mm6   \n\t" /* 00 00 00 00 00 00 00 00 */
	"   pcmpeqw     %%mm7, %%mm7   \n\t" /* FF FF FF FF FF FF FF FF */
	"   punpcklbw   %%mm6, %%mm7   \n\t" /* 00 FF 00 FF 00 FF 00 FF */
	"1:                            \n\t"
	"   prefetchnta 256(%[s])      \n\t"
	"   movq       (%[s]), %%mm0   \n\t" /* V1 Y3 U1 Y2 V0 Y1 U0 Y0 */
	"   movq      8(%[s]), %%mm1   \n\t" /* V3 Y7 U3 Y6 V2 Y5 U2 Y4 */
	"   pand        %%mm7, %%mm0   \n\t" /* 00 Y3 00 Y2 00 Y1 00 Y0 */
	"   pand        %%mm7, %%mm1   \n\t" /* 00 Y7 00 Y6 00 Y5 00 Y4 */
	"   packuswb    %%mm1, %%mm0   \n\t" /* Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 */
	"   movntq      %%mm0, (%[dy]) \n\t"
	"   add           $16, %[s]    \n\t"
	"   add            $8, %[dy]   \n\t"
	"   dec          %[i]          \n\t"
	"   jnz            1b          \n\t"
	"   sfence                     \n\t"
	"   emms                       \n\t"
	: [i] "+r" (i), [s] "+r" (s), [dy] "+r" (dy)
	:
	: "memory"
	);
}

void yuyv422_to_y8_sse2(const unsigned char *src, th_ycbcr_buffer ycbcrbuf)
{
#ifdef ARCH_X86_32
	uint32_t i;
#else
	uint64_t i;
#endif
	const uint8_t *s;
	uint8_t *dy;

	s   = src;
	dy  = ycbcrbuf[0].data;
	i = (2 * ycbcrbuf[0].stride * ycbcrbuf[0].height) / 16;

	/* Make sure image width is multiple of 8 and src is 16-byte aligned
	   The alignment is strictly necessary since we're using movdqa, otherwise we get a segfault */
	if (ycbcrbuf[0].stride & 7) {
		snprintf(msgbuf, MBS, MSGPREFIX"Error: image width not multiple of 8\n");
		die(msgbuf, 1);
	}
	if (((long)src) & 15L) {
		snprintf(msgbuf, MBS, MSGPREFIX"Error: source buffer not properly aligned\n");
		die(msgbuf, 1);
	}

	__asm__ __volatile__ (
	"   pxor       %%xmm6, %%xmm6   \n\t" /* 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 */
	"   pcmpeqw    %%xmm7, %%xmm7   \n\t" /* FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF */
	"   punpcklbw  %%xmm6, %%xmm7   \n\t" /* 00 FF 00 FF 00 FF 00 FF 00 FF 00 FF 00 FF 00 FF */
	"1:                             \n\t"
	"   prefetchnta 256(%[s])       \n\t"
	"   movdqa     (%[s]), %%xmm0   \n\t" /* V3 Y7 U3 Y6 V2 Y5 U2 Y4 V1 Y3 U1 Y2 V0 Y1 U0 Y0 */
	"   pand       %%xmm7, %%xmm0   \n\t" /* 00 Y7 00 Y6 00 Y5 00 Y4 00 Y3 00 Y2 00 Y1 00 Y0 */
	"   packuswb   %%xmm6, %%xmm0   \n\t" /* 00 00 00 00 00 00 00 00 Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 */
	"   movdq2q    %%xmm0, %%mm0    \n\t" /* copy to mmreg so we can use nt move */ 
	"   movntq      %%mm0, (%[dy])  \n\t"
	"   add           $16, %[s]     \n\t"
	"   add            $8, %[dy]    \n\t"
	"   dec          %[i]           \n\t"
	"   jnz            1b           \n\t"
	: [i] "+r" (i), [s] "+r" (s), [dy] "+r" (dy)
	:
	: "memory"
	);
}

void yuyv422_to_yuv420p_mmxext(const unsigned char *src, th_ycbcr_buffer ycbcrbuf)
{
#ifdef ARCH_X86_32
	uint32_t i, j, stride;
#else
	uint64_t i, j, stride;
#endif
	const uint8_t *s;
	uint8_t *dy, *dcb, *dcr;

	s   = src;
	dy  = ycbcrbuf[0].data;
	dcb = ycbcrbuf[1].data;
	dcr = ycbcrbuf[2].data;
	stride = ycbcrbuf[0].stride;
	i = ycbcrbuf[0].height;
	j = (2 * stride) / 16;

	/* Make sure image width is multiple of 8, image height is even and src is 8-byte aligned
	   The alignment is not strictly necessary for mmx code, but improves speed */
	if ((ycbcrbuf[0].stride & 7) || (ycbcrbuf[0].height & 1)) {
		snprintf(msgbuf, MBS, MSGPREFIX"Error: image width/height not multiple of 8/2\n");
		die(msgbuf, 1);
	}
	if (((long)src) & 7L) {
		snprintf(msgbuf, MBS, MSGPREFIX"Error: source buffer not properly aligned\n");
		die(msgbuf, 1);
	}

	/* pavgw, pshufw, prefetchnta, movntq and sfence are mmxext/see instructions */
	__asm__ __volatile__ (
	"   pxor            %%mm6, %%mm6     \n\t" /* 00 00 00 00 00 00 00 00 */
	"   pcmpeqw         %%mm7, %%mm7     \n\t" /* FF FF FF FF FF FF FF FF */
	"   punpcklbw       %%mm6, %%mm7     \n\t" /* 00 FF 00 FF 00 FF 00 FF */
	"1: mov             %[st], %[j]      \n\t"
	"   shr                $3, %[j]      \n\t" /* j = (2*stride)/16 */
	"2:                                  \n\t"
	"   prefetchnta 256(%[s])            \n\t"
	"   prefetchnta 256(%[s],%[st],2)    \n\t"
	"   movq           (%[s]), %%mm0     \n\t" /* V1 Y3 U1 Y2 V0 Y1 U0 Y0 */
	"   movq          8(%[s]), %%mm1     \n\t" /* V3 Y7 U3 Y6 V2 Y5 U2 Y4 */
	"   movq   (%[s],%[st],2), %%mm2     \n\t" /* v1 y3 u1 y2 v0 y1 u0 y0 (next row) */
	"   movq  8(%[s],%[st],2), %%mm3     \n\t" /* v3 y7 u3 y6 v2 y5 u2 y4 (next row) */

	"   movq            %%mm0, %%mm4     \n\t" /* V1 Y3 U1 Y2 V0 Y1 U0 Y0 */
	"   movq            %%mm2, %%mm5     \n\t" /* v1 y3 u1 y2 v0 y1 u0 y0 */
	"   psrlw              $8, %%mm4     \n\t" /* 00 V1 00 U1 00 V0 00 U0 */
	"   psrlw              $8, %%mm5     \n\t" /* 00 v1 00 u1 00 v0 00 u0 */
	"   pavgw           %%mm5, %%mm4     \n\t" /* 00 V1 00 U1 00 V0 00 U0 (avg) */
	"   pshufw   $0xd8, %%mm4, %%mm6     \n\t" /* 00 V1 00 V0 00 U1 00 U0 */

	"   movq            %%mm1, %%mm4     \n\t" /* V3 Y7 U3 Y6 V2 Y5 U2 Y4 */
	"   movq            %%mm3, %%mm5     \n\t" /* v3 y7 u3 y6 v2 y5 u2 y4 */
	"   psrlw              $8, %%mm4     \n\t" /* 00 V3 00 U3 00 V2 00 U2 */
	"   psrlw              $8, %%mm5     \n\t" /* 00 v3 00 u3 00 v2 00 u2 */
	"   pavgw           %%mm5, %%mm4     \n\t" /* 00 V3 00 U3 00 V2 00 U2 (avg) */
	"   pshufw   $0xd8, %%mm4, %%mm5     \n\t" /* 00 V3 00 V2 00 U3 00 U2 */

	"   packuswb        %%mm5, %%mm6     \n\t" /* V3 V2 U3 U2 V1 V0 U1 U0 */
	"   pshufw   $0xd8, %%mm6, %%mm6     \n\t" /* V3 V2 V1 V0 U3 U2 U1 U0 */
	"   movd            %%mm6, (%[dcb])  \n\t"
	"   psrlq             $32, %%mm6     \n\t" /* 00 00 00 00 V3 V2 V1 V0 */
	"   movd            %%mm6, (%[dcr])  \n\t"

	"   pand            %%mm7, %%mm0     \n\t" /* 00 Y3 00 Y2 00 Y1 00 Y0 */
	"   pand            %%mm7, %%mm1     \n\t" /* 00 Y7 00 Y6 00 Y5 00 Y4 */
	"   pand            %%mm7, %%mm2     \n\t"
	"   pand            %%mm7, %%mm3     \n\t"
	"   packuswb        %%mm1, %%mm0     \n\t" /* Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 */
	"   packuswb        %%mm3, %%mm2     \n\t"
	"   movntq          %%mm0, (%[dy])   \n\t"
	"   movntq          %%mm2, (%[dy],%[st]) \n\t"

	"   add               $16, %[s]      \n\t" /* next 16 bytes of src */
	"   add                $8, %[dy]     \n\t"
	"   add                $4, %[dcb]    \n\t"
	"   add                $4, %[dcr]    \n\t"
	"   dec              %[j]            \n\t" /* j-- */
	"   jnz                2b            \n\t"

	"   add             %[st], %[s]      \n\t" /* src += 2*stride */
	"   add             %[st], %[s]      \n\t"
	"   add             %[st], %[dy]     \n\t" /* dy += stride */
	"   sub                $2, %[i]      \n\t" /* i-=2 */
	"   jnz                1b            \n\t"

	"   sfence                           \n\t"
	"   emms                             \n\t"
#ifdef ARCH_X86_32
	: [i] "+m" (i), [j] "+r" (j),
#else
	: [i] "+r" (i), [j] "+r" (j),
#endif
	  [s] "+r" (s), [dy] "+r" (dy), [dcb] "+r" (dcb), [dcr] "+r" (dcr)
	: [st] "r" (stride)
	: "memory"
	);
}

void yuyv422_to_yuv420p_sse2(const unsigned char *src, th_ycbcr_buffer ycbcrbuf)
{
#ifdef ARCH_X86_32
	uint32_t i, j, stride;
#else
	uint64_t i, j, stride;
#endif
	const uint8_t *s;
	uint8_t *dy, *dcb, *dcr;

	s   = src;
	dy  = ycbcrbuf[0].data;
	dcb = ycbcrbuf[1].data;
	dcr = ycbcrbuf[2].data;
	stride = ycbcrbuf[0].stride;
	i = ycbcrbuf[0].height;
	j = (2 * stride) / 16;

	/* Make sure image width is multiple of 8, image height is even and src is 16-byte aligned
	   The alignment is strictly necessary since we're using movdqa, otherwise we get a segfault */
	if ((ycbcrbuf[0].stride & 7) || (ycbcrbuf[0].height & 1)) {
		snprintf(msgbuf, MBS, MSGPREFIX"Error: image width/height not multiple of 8/2\n");
		die(msgbuf, 1);
	}
	if (((long)src) & 15L) {
		snprintf(msgbuf, MBS, MSGPREFIX"Error: source buffer not properly aligned\n");
		die(msgbuf, 1);
	}

	__asm__ __volatile__ (
	"   pxor            %%xmm6, %%xmm6   \n\t" /* 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 */
	"   pcmpeqw         %%xmm7, %%xmm7   \n\t" /* FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF */
	"   punpcklbw       %%xmm6, %%xmm7   \n\t" /* 00 FF 00 FF 00 FF 00 FF 00 FF 00 FF 00 FF 00 FF */
	"1: mov              %[st], %[j]     \n\t"
	"   shr                 $3, %[j]     \n\t" /* j = (2*stride)/16 */
	"2:                                  \n\t"
	"   prefetchnta  256(%[s])           \n\t"
	"   prefetchnta  256(%[s],%[st],2)   \n\t"
	"   movdqa          (%[s]), %%xmm0   \n\t" /* V3 Y7 U3 Y6 V2 Y5 U2 Y4 V1 Y3 U1 Y2 V0 Y1 U0 Y0 */
	"   movdqa  (%[s],%[st],2), %%xmm1   \n\t" /* v3 y7 u3 y6 v2 y5 u2 y4 v1 y3 u1 y2 v0 y1 u0 y0 (next row) */

	"   movdqa          %%xmm0, %%xmm2   \n\t" /* V3 Y7 U3 Y6 V2 Y5 U2 Y4 V1 Y3 U1 Y2 V0 Y1 U0 Y0 */
	"   movdqa          %%xmm1, %%xmm3   \n\t" /* v3 y7 u3 y6 v2 y5 u2 y4 v1 y3 u1 y2 v0 y1 u0 y0 */
	"   psrlw               $8, %%xmm2   \n\t" /* 00 V3 00 U3 00 V2 00 U2 00 V1 00 U1 00 V0 00 U0 */
	"   psrlw               $8, %%xmm3   \n\t" /* 00 v3 00 u3 00 v2 00 u2 00 v1 00 u1 00 v0 00 u0 */
	"   pavgw           %%xmm3, %%xmm2   \n\t" /* 00 V3 00 U3 00 V2 00 U2 00 V1 00 U1 00 V0 00 U0 (avg) */
	"   pshuflw  $0xd8, %%xmm2, %%xmm2   \n\t" /* 00 V3 00 U3 00 V2 00 U2 00 V1 00 V0 00 U1 00 U0 */
	"   pshufhw  $0xd8, %%xmm2, %%xmm2   \n\t" /* 00 V3 00 V2 00 U3 00 U2 00 V1 00 V0 00 U1 00 U0 */
	"   packuswb        %%xmm6, %%xmm2   \n\t" /* 00 00 00 00 00 00 00 00 V3 V2 U3 U2 V1 V0 U1 U0 */
	"   pshuflw  $0xd8, %%xmm2, %%xmm2   \n\t" /* 00 00 00 00 00 00 00 00 V3 V2 V1 V0 U3 U2 U1 U0 */
	"   movd            %%xmm2, (%[dcb]) \n\t"
	"   psrlq              $32, %%xmm2   \n\t" /* 00 00 00 00 00 00 00 00 00 00 00 00 V3 V2 V1 V0 */
	"   movd            %%xmm2, (%[dcr]) \n\t"

	"   pand            %%xmm7, %%xmm0   \n\t" /* 00 Y7 00 Y6 00 Y5 00 Y4 00 Y3 00 Y2 00 Y1 00 Y0 */
	"   pand            %%xmm7, %%xmm1   \n\t"
	"   packuswb        %%xmm6, %%xmm0   \n\t" /* 00 00 00 00 00 00 00 00 Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0 */
	"   packuswb        %%xmm6, %%xmm1   \n\t"
	"   movdq2q         %%xmm0, %%mm0    \n\t" /* copy to mmreg so we can use nt move */
	"   movdq2q         %%xmm1, %%mm1    \n\t"
	"   movntq           %%mm0, (%[dy])  \n\t"
	"   movntq           %%mm1, (%[dy],%[st]) \n\t"

	"   add                $16, %[s]     \n\t" /* next 16 bytes of src */
	"   add                 $8, %[dy]    \n\t"
	"   add                 $4, %[dcb]   \n\t"
	"   add                 $4, %[dcr]   \n\t"
	"   dec               %[j]           \n\t" /* j-- */
	"   jnz                 2b           \n\t"

	"   add              %[st], %[s]     \n\t" /* src += 2*stride */
	"   add              %[st], %[s]     \n\t"
	"   add              %[st], %[dy]    \n\t" /* dy += stride */
	"   sub                 $2, %[i]     \n\t" /* i-=2 */
	"   jnz                 1b           \n\t"
#ifdef ARCH_X86_32
	: [i] "+m" (i), [j] "+r" (j),
#else
	: [i] "+r" (i), [j] "+r" (j),
#endif
	  [s] "+r" (s), [dy] "+r" (dy), [dcb] "+r" (dcb), [dcr] "+r" (dcr)
	: [st] "r" (stride)
	: "memory"
	);
}
