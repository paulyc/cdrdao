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

#include <stdio.h>

#include "util.h"
#include "FormatOgg.h"


FormatOgg::FormatOgg()
{
}

int FormatOgg::convert(const char* from, const char* to)
{
  FILE* fin = fopen(from, "r");
  if (!fin) {
    message(-2, "Could not open input file \"%s\": %s", from,
            strerror(errno));
    return 1;
  }

  int ovret = ov_open(fin, &vorbisFile_, NULL, 0);
  if (ovret != 0) {
      message(-2, "Could not open Ogg Vorbis file \"%s\"", from);
      return 3;
  }

  outFormat_.bits = 16;
  outFormat_.rate = 44100;
  outFormat_.channels = 2;
  outFormat_.byte_format = AO_FMT_NATIVE;
  aoDev_ = ao_open_file(ao_driver_id("wav"), to, 1, &outFormat_, NULL);
  if (!aoDev_) {
    message(-2, "Could not create output file \"%s\": %s", to,
            strerror(errno));
    return 2;
  }

  // Decoding loop

  while (1) {
    int sec;
    int size = ov_read(&vorbisFile_, buffer, sizeof(buffer), 0, 2, 1, &sec);

    if (!size)
      break;

    ao_play(aoDev_, buffer, size);
  }

  // Done, close things
  ov_clear(&vorbisFile_);
  fclose(fin);
  ao_close(aoDev_);

  return 0;
}

// ----------------------------------------------------------------
//
// Manager class
//
//

FormatSupport* FormatOggManager::newConverter(const char* extension)
{
  if (strcmp(extension, "ogg") == 0)
    return new FormatOgg;

  return NULL;
}

int FormatOggManager::supportedExtensions(std::list<std::string>& list)
{
  list.push_front("ogg");
  return 1;
}

