/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998, 1999  Andreas Mueller <mueller@daneb.ping.de>
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

#ifndef __RECORD_TOC_SOURCE_H
#define __RECORD_TOC_SOURCE_H

#include <gtk--.h>
#include <gtk/gtk.h>

class TocEdit;
//class CdDevice;

class RecordTocSource : public Gtk::VBox {
public:
  RecordTocSource();
  ~RecordTocSource();

  Gtk::Window *parent; // the dialog where the vbox is placed
  
  void start(TocEdit *);
  void stop();

  void update(unsigned long level);

private:
  TocEdit *tocEdit_;
  int active_;

  Gtk::Label *projectLabel_;
  Gtk::Label *tocTypeLabel_;
  Gtk::Label *nofTracksLabel_;
  Gtk::Label *tocLengthLabel_;

};

#endif
