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

#include <gdkmm.h>
#include <sigc++/object.h>

#include "Converter.h"
#include "TempFileManager.h"

void Converter::start(const char* srcfile)
{
  std::list<std::string> flist;
  flist.push_front(srcfile);
  start(flist);
}

void Converter::start(std::list<std::string>& list)
{
  std::list<std::string>::iterator i = list.begin();

  for (;i != list.end(); i++) {
    list_.push_front(*i);
  }

  if (idle_) {
    idle_ = false;
    abort_ = false;
    cur_src_.clear();
    cur_dst_.clear();
    Glib::signal_idle().connect(sigc::mem_fun(*this, &Converter::idleWorker));
  }
}

void Converter::abort()
{
  // Will be caught by idleWorker.
  abort_ = true;
}

bool Converter::idleWorker()
{
  if (abort_) {
    if (cur_cvrt_) {
      cur_cvrt_->convertAbort();
      delete cur_cvrt_;
      cur_cvrt_ = NULL;
    }
    cur_src_.clear();
    list_.clear();
    idle_ = true;
    abort_ = false;
    return 0;
  }

  if (cur_src_.empty()) {

    // List empty ? We're done.
    if (list_.empty()) {
      sigAllDone();
      idle_ = true;
      return 0;
    }
    cur_src_ = list_.front();
    list_.pop_front();

    // Create a format converter for this file.
    cur_cvrt_ = formatConverter.newConverter(cur_src_.c_str());

    // If we can't convert it, it's probably already a WAV or RAW
    // file. Just pass it along as a success file.
    if (!cur_cvrt_) {
      sigEnd(cur_src_.c_str(), cur_src_.c_str(), 0);
      cur_src_.clear();
      return 1;
    }

    const char* extension;
    extension = ((cur_cvrt_->format() == TrackData::WAVE) ? "wav" : "raw");
    bool exists = tempFileManager.getTempFile(cur_dst_, cur_src_.c_str(),
                                              extension);

    if (exists) {
      sigEnd(cur_src_.c_str(), cur_dst_.c_str(), 0);
      cur_src_.clear();
      return 1;
    }

    sigStart(cur_src_.c_str(), cur_dst_.c_str());
    cur_cvrt_->convertStart(cur_src_.c_str(), cur_dst_.c_str());
  }

  // Perform the incremental conversion/decoding
  FormatSupport::Status st = cur_cvrt_->convertContinue();

  // Is this one done ? Prepare for next one.
  if (st != FormatSupport::FS_IN_PROGRESS) {
    sigEnd(cur_src_.c_str(), cur_dst_.c_str(),
           (st != FormatSupport::FS_SUCCESS));
    delete cur_cvrt_;
    cur_cvrt_ = NULL;
    cur_src_.clear();
  }

  return 1;
}

int Converter::queueSize()
{
  return list_.size();
}
