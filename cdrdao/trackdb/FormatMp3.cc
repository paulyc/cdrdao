/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2004  Denis Leroy <denis@poolshark.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

//  A lot of this code is taken from mpg321 :
//  mpg321 - a fully free clone of mpg123.
//  Copyright (C) 2001 Joe Drew <drew@debian.org>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "util.h"
#include "FormatMp3.h"


FormatMp3::FormatMp3()
{
    memset(&dither_, 0, sizeof(dither_));
}

int FormatMp3::convert(const char* from, const char* to)
{
  struct stat st;

  if (stat(from, &st) != 0 || st.st_size == 0) {
    message(-2, "Could not stat input file \"%s\": %s", from,
            strerror(errno));
    return 1;
  }

  int fd = open(from, O_RDONLY);
  if (!fd) {
    message(-2, "Could not open input file \"%s\": %s", from,
            strerror(errno));
    return 1;
  }

  void* fdm = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (fdm == MAP_FAILED) {
    message(-2, "Could not map file \"%s\" into memory: %s", from,
            strerror(errno));
    return 1;
  }

  ao_sample_format out_format;
  out_format.bits = 16;
  out_format.rate = 44100;
  out_format.channels = 2;
  out_format.byte_format = AO_FMT_NATIVE;

  out_ = ao_open_file(ao_driver_id("wav"), to, 1, &out_format,
                      NULL);
  if (!out_) {
    message(-2, "Could not create output file \"%s\": %s", to,
            strerror(errno));
    return 2;
  }

  // Decoding loop
  start_ = (unsigned char const*)fdm;
  length_ = st.st_size;

  mad_decoder_init(&decoder_, this, FormatMp3::madinput, 0, 0,
                   FormatMp3::madoutput, FormatMp3::maderror, 0);

  int result = mad_decoder_run(&decoder_, MAD_DECODER_MODE_SYNC);

  mad_decoder_finish(&decoder_);

  // Done, close things
  munmap(fdm, st.st_size);
  close(fd);

  ao_close(out_);

  if (result != MAD_ERROR_NONE)
    return 3;

  return 0;
}

unsigned long FormatMp3::prng(unsigned long state)
{
  return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

signed long FormatMp3::audio_linear_dither(unsigned int bits,
                                           mad_fixed_t sample,
                                           struct audio_dither *dither)
{
  unsigned int scalebits;
  mad_fixed_t output, mask, random;

  enum {
    MIN = -MAD_F_ONE,
    MAX =  MAD_F_ONE - 1
  };

  /* noise shape */
  sample += dither->error[0] - dither->error[1] + dither->error[2];

  dither->error[2] = dither->error[1];
  dither->error[1] = dither->error[0] / 2;

  /* bias */
  output = sample + (1L << (MAD_F_FRACBITS + 1 - bits - 1));

  scalebits = MAD_F_FRACBITS + 1 - bits;
  mask = (1L << scalebits) - 1;

  /* dither */
  random  = prng(dither->random);
  output += (random & mask) - (dither->random & mask);

  dither->random = random;

  /* clip */
  if (output > MAX) {
    output = MAX;

    if (sample > MAX)
      sample = MAX;
  }
  else if (output < MIN) {
    output = MIN;

    if (sample < MIN)
      sample = MIN;
  }

  /* quantize */
  output &= ~mask;

  /* error feedback */
  dither->error[0] = sample - output;

  /* scale */
  return output >> scalebits;
}

enum mad_flow FormatMp3::madoutput(void* data, struct mad_header const* header,
                                   struct mad_pcm* pcm)
{
  FormatMp3* fmp3 = (FormatMp3*)data;
  ao_device* playdevice = fmp3->out_;
  register int nsamples = pcm->length;
  mad_fixed_t const *left_ch = pcm->samples[0], *right_ch = pcm->samples[1];
    
  register char* ptr = fmp3->stream_;
  register signed int sample;
  register mad_fixed_t tempsample;

  if (pcm->channels == 2) {
    while (nsamples--) {
      tempsample = (mad_fixed_t)(*left_ch++);
      sample = (signed int) audio_linear_dither(16, tempsample,
                                                &fmp3->dither_);

#ifndef WORDS_BIGENDIAN
      *ptr++ = (unsigned char) (sample >> 0);
      *ptr++ = (unsigned char) (sample >> 8);
#else
      *ptr++ = (unsigned char) (sample >> 8);
      *ptr++ = (unsigned char) (sample >> 0);
#endif
            
      tempsample = (mad_fixed_t)(*right_ch++);
      sample = (signed int) audio_linear_dither(16, tempsample,
                                                &fmp3->dither_);
#ifndef WORDS_BIGENDIAN
      *ptr++ = (unsigned char) (sample >> 0);
      *ptr++ = (unsigned char) (sample >> 8);
#else
      *ptr++ = (unsigned char) (sample >> 8);
      *ptr++ = (unsigned char) (sample >> 0);
#endif
    }

    ao_play(playdevice, fmp3->stream_, pcm->length * 4);

  } else {
    while (nsamples--) {
      tempsample = (mad_fixed_t)((*left_ch++)/MAD_F_ONE);
      sample = (signed int) audio_linear_dither(16, tempsample,
                                                &fmp3->dither_);
            
      /* Just duplicate the sample across both channels. */
#ifndef WORDS_BIGENDIAN
      *ptr++ = (unsigned char) (sample >> 0);
      *ptr++ = (unsigned char) (sample >> 8);
      *ptr++ = (unsigned char) (sample >> 0);
      *ptr++ = (unsigned char) (sample >> 8);
#else
      *ptr++ = (unsigned char) (sample >> 8);
      *ptr++ = (unsigned char) (sample >> 0);
      *ptr++ = (unsigned char) (sample >> 8);
      *ptr++ = (unsigned char) (sample >> 0);
#endif
    }

    ao_play(playdevice, fmp3->stream_, pcm->length * 4);
  }

  return MAD_FLOW_CONTINUE;        
}

enum mad_flow FormatMp3::madinput(void* data, struct mad_stream* stream)
{
  FormatMp3* fmp3 = (FormatMp3*)data;

  if (!fmp3->length_)
    return MAD_FLOW_STOP;

  mad_stream_buffer(stream, fmp3->start_, fmp3->length_);
  fmp3->length_ = 0;

  return MAD_FLOW_CONTINUE;
}

enum mad_flow FormatMp3::maderror(void* data, struct mad_stream* stream,
                                  struct mad_frame* frame)
{
  FormatMp3* fmp3 = (FormatMp3*)data;

  // Don't print out loss of synchronizations
  if (stream->error != MAD_ERROR_LOSTSYNC) {
      message(-1, "Decoding error 0x%04x (%s) at byte offset %u",
              stream->error, mad_stream_errorstr(stream),
              stream->this_frame - fmp3->start_);
      return MAD_FLOW_BREAK;
  }

  return MAD_FLOW_CONTINUE;
}

// ----------------------------------------------------------------
//
// Manager class
//
//

FormatSupport* FormatMp3Manager::newConverter(const char* extension)
{
  if (strcmp(extension, "mp3") == 0)
    return new FormatMp3;

  return NULL;
}

int FormatMp3Manager::supportedExtensions(std::list<std::string>& list)
{
  list.push_front("mp3");
  return 1;
}

