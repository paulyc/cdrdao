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
/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  2000/02/05 01:38:46  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 * Revision 1.1  1999/09/07 11:16:16  mueller
 * Initial revision
 *
 */

#ifndef __RECORD_DIALOG_H
#define __RECORD_DIALOG_H

#include <gtk--.h>
#include <gtk/gtk.h>

class TocEdit;
class CdDevice;

class RecordDialog : public Gtk::Dialog {
public:
  RecordDialog();
  ~RecordDialog();

  void start(TocEdit *);
  void stop();

  void update(unsigned long level, TocEdit *);

  gint delete_event_impl(GdkEventAny*);

private:
  TocEdit *tocEdit_;
  int active_;

  int speed_;

  struct SpeedTable {
    int speed;
    const char *name;
  };

  struct DeviceData {
    int bus, id, lun;
  };

  Gtk::CList *list_;
  Gtk::Button *startButton_;

  Gtk::RadioButton *writeButton_;
  Gtk::RadioButton *simulateButton_;
  
  Gtk::CheckButton *closeSessionButton_;
  Gtk::CheckButton *ejectButton_;
  Gtk::CheckButton *reloadButton_;

//llanero  Gtk::ItemFactory::Menu *speedMenuFactory_;
  Gtk::OptionMenu *speedMenu_;

  void cancelAction();
  void startAction();

  void setSpeed(int);

  void appendTableEntry(CdDevice *);
  void import();
  void importStatus();

};

#endif
