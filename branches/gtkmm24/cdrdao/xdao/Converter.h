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

#ifndef __CONVERTER_H__
#define __CONVERTER_H__

#include <list>
#include <string>
#include <sigc++/signal.h>

#include "FormatConverter.h"

// Class managing list of files to be converted into WAV files.

class Converter
{
public:
  Converter() : idle_(true) {};

  // Add work to the queue
  void start(const char* srcfile);
  void start(std::list<std::string>& list);

  // Is converter currently idle ?
  bool idle() { return idle_; }

  // Abort current conversion and clear queue.
  void abort();

  // Return number of outstanding files to be converted, excluding the
  // current one.
  int  queueSize();

  // Signals
  sigc::signal0<void> sigAllDone;
  sigc::signal2<void, const char*, const char*> sigStart;
  sigc::signal3<void, const char*, const char*, int> sigEnd;

 protected:

  bool idleWorker();
  bool idle_;
  bool abort_;
  std::list<std::string> list_;
  std::string cur_src_;
  std::string cur_dst_;
  FormatSupport* cur_cvrt_;
};

#endif
