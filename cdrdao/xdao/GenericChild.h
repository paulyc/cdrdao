/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2000 Andreas Mueller <mueller@daneb.ping.de>
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


// Abstract Class.
// All Childs need to implement this basic funcs!

#ifndef __GENERIC_CHILD_H__
#define __GENERIC_CHILD_H__

#include <gnome--.h>

class TocEdit;

class GenericChild : public Gnome::MDIChild
{
public:
  GenericChild();
  ~GenericChild();

  virtual TocEdit *tocEdit() const;

  virtual void update(unsigned long level) = 0;
  virtual void saveProject() = 0;
  virtual void saveAsProject() = 0;
  virtual bool closeProject() = 0;
  virtual void record_to_cd() = 0;

protected:
  TocEdit *tocEdit_;
  
};

#endif
