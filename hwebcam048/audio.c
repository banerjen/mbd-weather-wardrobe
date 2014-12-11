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
#include <errno.h>
#include "audio.h"
#include "common.h"

#define MSGPREFIX "[alsa] "

int audio_init(struct audio_data *ad, char *device, snd_pcm_stream_t streamtype,
				unsigned int rate, unsigned int channels, struct pollfd *pfds, int verbose)
{
	int                 err, dir, i;
/*	int                 sival;
	snd_pcm_uframes_t   ufval; */

	ad->device          = device;
	ad->stream          = streamtype;
	ad->access          = SND_PCM_ACCESS_RW_INTERLEAVED;
	ad->format          = SND_PCM_FORMAT_S16_LE;
	ad->pcm_handle      = NULL;
	ad->hw_params       = NULL;
	ad->rate            = rate;
	ad->channels        = channels;
/*	ad->periods         = */
/*	ad->period_size     = */
	ad->period_time     = 10000;
/*	ad->buffer_size     = */
/*	ad->buffer_time     = */
	ad->sw_params       = NULL;
/*	ad->avail_min       = period_size; */
/*	ad->start_threshold = */
/*	ad->stop_threshold  = */
	ad->pfds            = pfds;
	ad->nfds            = 0;
	ad->verbose         = verbose;

	/* open pcm device */
	if (ad->verbose)
		fprintf(stderr, MSGPREFIX"Opening pcm audio device '%s' for %s...\n",
		        ad->device, ad->stream == SND_PCM_STREAM_CAPTURE ? "capture" : "playback");
	if ((err = snd_pcm_open(&ad->pcm_handle, ad->device, ad->stream, 0)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot open audio device '%s' (%s)\n", ad->device, snd_strerror(err));
		return -1;
	}

	/* hardware parameters */
	if (ad->verbose)
		fprintf(stderr, MSGPREFIX"Setting hardware parameters...\n");
	if ((err = snd_pcm_hw_params_malloc(&ad->hw_params)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_any(ad->pcm_handle, ad->hw_params)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
		return -1;
	}
	/* set hardware parameters */
	if ((err = snd_pcm_hw_params_set_access(ad->pcm_handle, ad->hw_params, ad->access)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set access type (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_set_format(ad->pcm_handle, ad->hw_params, ad->format)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set sample format (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_set_rate(ad->pcm_handle, ad->hw_params, ad->rate, 0)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set sample rate (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_set_channels(ad->pcm_handle, ad->hw_params, ad->channels)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set channel count (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_set_period_time(ad->pcm_handle, ad->hw_params, ad->period_time, 0)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set period time (%s)\n", snd_strerror(err));
		return -1;
	}
#if 0
	if ((err = snd_pcm_hw_params_set_periods(ad->pcm_handle, ad->hw_params, ad->periods, 0)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set periods (%s)\n", snd_strerror(err));
		return -1;
	}
#endif
	/* apply hardware parameters to pcm device and prepare device */
	if (ad->verbose)
		fprintf(stderr, MSGPREFIX"Applying hardware parameters...\n");
	if ((err = snd_pcm_hw_params(ad->pcm_handle, ad->hw_params)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set parameters (%s)\n", snd_strerror(err));
		return -1;
	}
/*	if ((err = snd_pcm_prepare(ad->pcm_handle)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot prepare audio interface for use (%s)\n", snd_strerror(err));
		return -1;
	}*/
	/* read hardware parameters */
	if ((err = snd_pcm_hw_params_get_rate(ad->hw_params, &ad->rate, &dir)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get rate (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_get_channels(ad->hw_params, &ad->channels)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get channel count (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_get_periods(ad->hw_params, &ad->periods, &dir)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get periods (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_get_period_size(ad->hw_params, &ad->period_size, &dir)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get period size (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_get_period_time(ad->hw_params, &ad->period_time, &dir)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get period time (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_get_buffer_size(ad->hw_params, &ad->buffer_size)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get buffer size (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_hw_params_get_buffer_time(ad->hw_params, &ad->buffer_time, &dir)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get buffer time (%s)\n", snd_strerror(err));
		return -1;
	}
	/* print hardware parameters */
	if (ad->verbose) {
		fprintf(stderr, MSGPREFIX"Hardware parameters chosen:\n");
		fprintf(stderr, MSGPREFIX"  rate        = %u\n", ad->rate);
		fprintf(stderr, MSGPREFIX"  channels    = %u\n", ad->channels);
		fprintf(stderr, MSGPREFIX"  periods     = %u\n", ad->periods);
		fprintf(stderr, MSGPREFIX"  period_size = %lu\n", ad->period_size);
		fprintf(stderr, MSGPREFIX"  period_time = %u us\n", ad->period_time);
		fprintf(stderr, MSGPREFIX"  buffer_size = %lu\n", ad->buffer_size);
		fprintf(stderr, MSGPREFIX"  buffer_time = %u us\n", ad->buffer_time);
	}
	/* free hardware parameters */
	snd_pcm_hw_params_free(ad->hw_params);



	/* software parameters */
	if ((err = snd_pcm_sw_params_malloc(&ad->sw_params)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot allocate software parameter structure (%s)\n", snd_strerror(err));
		return -1;
	}
	if ((err = snd_pcm_sw_params_current(ad->pcm_handle, ad->sw_params)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot initialize software parameters structure (%s)\n", snd_strerror(err));
		return -1;
	}
#if 0
	/* get */
	fprintf(stderr, MSGPREFIX"Software parameters:\n");
	if ((err = snd_pcm_sw_params_get_avail_min(ad->sw_params, &ufval)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get minimum available count (%s)\n", snd_strerror(err));
		return -1;
	}
	fprintf(stderr, MSGPREFIX"  avail_min = %lu\n", ufval);
	if ((err = snd_pcm_sw_params_get_start_threshold(ad->sw_params, &ufval)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get start threshold (%s)\n", snd_strerror(err));
		return -1;
	}
	fprintf(stderr, MSGPREFIX"  start_threshold = %lu\n", ufval);
	if ((err = snd_pcm_sw_params_get_stop_threshold(ad->sw_params, &ufval)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get stop threshold (%s)\n", snd_strerror(err));
		return -1;
	}
	fprintf(stderr, MSGPREFIX"  stop_threshold = %lu\n", ufval);
	if ((err = snd_pcm_sw_params_get_boundary(ad->sw_params, &ufval)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get stop boundary (%s)\n", snd_strerror(err));
		return -1;
	}
	fprintf(stderr, MSGPREFIX"  boundary = %lu\n", ufval);
	if ((err = snd_pcm_sw_params_get_period_event(ad->sw_params, &sival)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot get period event (%s)\n", snd_strerror(err));
		return -1;
	}
	fprintf(stderr, MSGPREFIX"  period_event = %d\n", sival);
#endif
	/* set software parameters */
	if (ad->verbose)
		fprintf(stderr, MSGPREFIX"Setting software parameters...\n");
#if 0
	/* playback: if the samples in ring buffer are >= start_threshold, and the stream is not running,
	             it will be started
	   capture:  if we try to read a number of frames >= start_threshold, then the stream will be started */
	ad->start_threshold = ad->period_size;
	if ((err = snd_pcm_sw_params_set_start_threshold(ad->pcm_handle, ad->sw_params, ad->start_threshold)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set start threshold (%s)\n", snd_strerror(err));
		return -1;
	}
#endif
#if 0
	/* playback: if the available (empty) samples in ring buffer are >= stop_threshold, then the stream
	             will be stopped. stop_threshold can be > buffer_size to delay underrun
	   capture:  if the available (filled) samples in ring buffer are >= stop_threshold, then the stream
	             will be stopped. stop_threshold can be > buffer_size to delay overrun */
	ad->stop_threshold = ad->buffer_size;
	if ((err = snd_pcm_sw_params_set_stop_threshold(ad->pcm_handle, ad->sw_params, ad->stop_threshold)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set stop threshold (%s)\n", snd_strerror(err));
		return -1;
	}
#endif
#if 0
	ad->avail_min = ad->period_size;
	if ((err = snd_pcm_sw_params_set_avail_min(ad->pcm_handle, ad->sw_params, ad->avail_min)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set minimum available count (%s)\n", snd_strerror(err));
		return -1;
	}
#endif
	/* apply software parameters */
	if (ad->verbose)
		fprintf(stderr, MSGPREFIX"Applying software parameters...\n");
	if ((err = snd_pcm_sw_params(ad->pcm_handle, ad->sw_params)) < 0) {
		fprintf(stderr, MSGPREFIX"Cannot set software parameters (%s)\n", snd_strerror(err));
		return -1;
	}
	/* print software parameters */
	if (ad->verbose) {
		fprintf(stderr, MSGPREFIX"Software parameters chosen:\n");
		if ((err = snd_pcm_sw_params_current(ad->pcm_handle, ad->sw_params)) < 0) {
			fprintf(stderr, MSGPREFIX"Cannot initialize software parameters structure (%s)\n", snd_strerror(err));
			return -1;
		}
		if ((err = snd_pcm_sw_params_get_avail_min(ad->sw_params, &ad->avail_min)) < 0) {
			fprintf(stderr, MSGPREFIX"Cannot get minimum available count (%s)\n", snd_strerror(err));
			return -1;
		}
		fprintf(stderr, MSGPREFIX"  avail_min       = %lu\n", ad->avail_min);
		if ((err = snd_pcm_sw_params_get_start_threshold(ad->sw_params, &ad->start_threshold)) < 0) {
			fprintf(stderr, MSGPREFIX"Cannot get start threshold (%s)\n", snd_strerror(err));
			return -1;
		}
		fprintf(stderr, MSGPREFIX"  start_threshold = %lu\n", ad->start_threshold);
		if ((err = snd_pcm_sw_params_get_stop_threshold(ad->sw_params, &ad->stop_threshold)) < 0) {
			fprintf(stderr, MSGPREFIX"Cannot get stop threshold (%s)\n", snd_strerror(err));
			return -1;
		}
		fprintf(stderr, MSGPREFIX"  stop_threshold  = %lu\n", ad->stop_threshold);
	}
	/* free software parameters */
	snd_pcm_sw_params_free(ad->sw_params);



	/* poll */
	if (ad->pfds) {
		ad->nfds = snd_pcm_poll_descriptors_count(ad->pcm_handle);
		err = snd_pcm_poll_descriptors(ad->pcm_handle, ad->pfds, ad->nfds);
		if (ad->verbose) {
			fprintf(stderr, MSGPREFIX"Poll descriptors = %d\n", ad->nfds);
			for (i = 0; i < ad->nfds; i++) {
				fprintf(stderr, MSGPREFIX"  %d: fd = %d,", i, ad->pfds[i].fd);
				fprintf(stderr, " %s", ad->pfds[i].events & POLLIN  ? "POLLIN"  : "");
				fprintf(stderr, " %s", ad->pfds[i].events & POLLOUT ? "POLLOUT" : "");
				fprintf(stderr, "\n");
			}
		}
	}

	/* prepare alsa buffer */
	ad->alsabuffersize = ad->period_size;
	ad->alsabuffer     = calloc(ad->channels * sizeof(*ad->alsabuffer), ad->alsabuffersize);
	if (ad->alsabuffer == NULL) {
		fprintf(stderr, MSGPREFIX"Error: calloc() failed\n");
		return -1;
	}
	ad->frames = 0;
	ad->avail  = 0;
	ad->peak_percent = 0.0f;

	return 0;
}

int audio_start(struct audio_data *ad)
{
	int err;

	if (ad->verbose)
		print_msg_nl(MSGPREFIX"Starting pcm audio device...\n");

	if ((err = snd_pcm_start(ad->pcm_handle)) < 0) {
		snprintf(msgbuf, MBS, MSGPREFIX"Cannot start pcm stream (%s)\n", snd_strerror(err));
		print_msg_nl(msgbuf);
		return -1;
	}

	return 0;
}

int audio_stop(struct audio_data *ad)
{
	int err;

	if (ad->verbose)
		print_msg_nl(MSGPREFIX"Stopping pcm audio device...\n");

	if ((err = snd_pcm_drop(ad->pcm_handle)) < 0) {
		snprintf(msgbuf, MBS, MSGPREFIX"Cannot stop pcm stream (%s)\n", snd_strerror(err));
		print_msg_nl(msgbuf);
		return -1;
	}

	return 0;
}

void audio_poll_descriptors_revents(struct audio_data *ad)
{
	snd_pcm_poll_descriptors_revents(ad->pcm_handle, ad->pfds, ad->nfds, &ad->revents);
}

snd_pcm_sframes_t audio_avail(struct audio_data *ad, int silent)
{
	if ((ad->avail = snd_pcm_avail(ad->pcm_handle)) < 0) {
		if (ad->verbose || !silent) {
			snprintf(msgbuf, MBS, MSGPREFIX"Cannot get pcm avail (%s)\n", snd_strerror(ad->avail));
			print_msg_nl(msgbuf);
		}
	}

	return ad->avail;
}

snd_pcm_sframes_t audio_read(struct audio_data *ad, int silent)
{
	if ((ad->frames = snd_pcm_readi(ad->pcm_handle, ad->alsabuffer, ad->alsabuffersize)) < 0) {
		if (ad->verbose || !silent) {
			snprintf(msgbuf, MBS, MSGPREFIX"Cannot read from pcm (%s)\n", snd_strerror(ad->frames));
			print_msg_nl(msgbuf);
		}
	}

	return ad->frames;
}

snd_pcm_sframes_t audio_write(struct audio_data *ad, int16_t *buffer, snd_pcm_uframes_t buffersize, int silent)
{
	if ((ad->frames = snd_pcm_writei(ad->pcm_handle, buffer, buffersize)) < 0) {
		if (ad->verbose || !silent) {
			snprintf(msgbuf, MBS, MSGPREFIX"Cannot write to pcm (%s)\n", snd_strerror(ad->frames));
			print_msg_nl(msgbuf);
		}
	}

	return ad->frames;
}

int audio_recover(struct audio_data *ad, int err, int silent)
{
	return snd_pcm_recover(ad->pcm_handle, err, silent);
}

void audio_find_peak(struct audio_data *ad)
{
	unsigned int s;
	int v, peak = 0;

	for (s = 0; s < ad->frames * ad->channels; s++) {
		v = abs(ad->alsabuffer[s]);
		if (v > peak)
			peak = v;
	}

	ad->peak_percent = 100.0f * (float)peak / (float)(INT16_MAX+1);
}

int audio_close(struct audio_data *ad)
{
	int err;

	if (ad->verbose)
		print_msg_nl(MSGPREFIX"Closing audio device...\n");

	if ((err = snd_pcm_close(ad->pcm_handle)) < 0) {
		snprintf(msgbuf, MBS, MSGPREFIX"Cannot close pcm stream (%s)\n", snd_strerror(err));
		print_msg_nl(msgbuf);
		return -1;
	}

	free(ad->alsabuffer);

	return 0;
}
