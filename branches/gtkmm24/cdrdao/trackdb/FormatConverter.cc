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

#include <stdlib.h>
#include <ao/ao.h>

#include "config.h"
#include "util.h"
#include "FormatConverter.h"
#include "TempFileManager.h"

#ifdef HAVE_MP3_SUPPORT
#include "FormatMp3.h"
#endif
#ifdef HAVE_OGG_SUPPORT
#include "FormatOgg.h"
#endif

FormatConverter::FormatConverter()
{
#if defined(HAVE_MP3_SUPPORT) || defined(HAVE_OGG_SUPPORT)
  ao_initialize();
#endif
#ifdef HAVE_MP3_SUPPORT
  managers_.push_front(new FormatMp3Manager);
#endif
#ifdef HAVE_OGG_SUPPORT
  managers_.push_front(new FormatOggManager);
#endif
}

FormatConverter::~FormatConverter()
{
  std::list<FormatSupportManager*>::iterator i = managers_.begin();
  while (i != managers_.end()) {
    delete *i++;
  }
#if defined(HAVE_MP3_SUPPORT) || defined(HAVE_OGG_SUPPORT)
    ao_shutdown();
#endif
}

FormatSupport* FormatConverter::newConverter(const char* fn)
{
  const char* ext = strrchr(fn, '.');
  if (!ext)
    return NULL;
  ext++;

  FormatSupport* candidate = NULL;
  std::list<FormatSupportManager*>::iterator i = managers_.begin();
  while (i != managers_.end()) {
    FormatSupportManager* mgr = *i++;

    candidate = mgr->newConverter(ext);
    if (candidate)
      break;
  }

  return candidate;
}

bool FormatConverter::canConvert(const char* fn)
{
  FormatSupport* c = newConverter(fn);

  if (!c)
    return false;

  delete c;
  return true;
}

const char* FormatConverter::convert(const char* fn)
{
  FormatSupport* c = newConverter(fn);

  if (!c)
    return NULL;

  std::string* file = new std::string;
  const char* extension;
  if (c->format() == TrackData::WAVE)
      extension = "wav";
  else
      extension = "raw";

  bool exists = tempFileManager.getTempFile(*file, fn, extension);

  if (!exists) {
    message(2, "Decoding file \"%s\"", fn);
    int result = c->convert(fn, file->c_str());

    if (result != 0)
      return NULL;
    
    tempFiles_.push_front(file);
  }

  return file->c_str();
}

int FormatConverter::supportedExtensions(std::list<std::string>& list)
{
    int num = 0;

    std::list<FormatSupportManager*>::iterator i = managers_.begin();
    for (;i != managers_.end(); i++) {
        num += (*i)->supportedExtensions(list);
    }
    return num;
}

FormatConverter formatConverter;
