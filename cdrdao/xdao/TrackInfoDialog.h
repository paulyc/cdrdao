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
 * Revision 1.3  2000/09/21 02:07:07  llanero
 * MDI support:
 * Splitted AudioCDChild into same and AudioCDView
 * Move Selections from TocEdit to AudioCDView to allow
 *   multiple selections.
 * Cursor animation in all the views.
 * Can load more than one from from command line
 * Track info, Toc info, Append/Insert Silence, Append/Insert Track,
 *   they all are built for every child when needed.
 * ...
 *
 * Revision 1.2  2000/02/20 23:34:54  llanero
 * fixed scsilib directory (files mising ?-()
 * ported xdao to 1.1.8 / gnome (MDI) app
 *
 * Revision 1.1.1.1  2000/02/05 01:38:55  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 * Revision 1.1  1999/08/19 20:27:16  mueller
 * Initial revision
 *
 */

#ifndef __TRACK_INFO_DIALOG_H__
#define __TRACK_INFO_DIALOG_H__

#include <gtk--.h>
#include <gtk/gtk.h>

class Toc;
class TocEdit;
class TextEdit;
class AudioCDChild;

class TrackInfoDialog : public Gtk::Dialog {
public:
  TrackInfoDialog(AudioCDChild *child);
  ~TrackInfoDialog();

  gint delete_event_impl(GdkEventAny*);

  void update(unsigned long, TocEdit *);

  void start(TocEdit *);
  void stop();

private:
  TocEdit *tocEdit_;
  int active_;

  int selectedTrack_;

  AudioCDChild *cdchild;

  Gtk::Button *applyButton_;

  Gtk::Label *trackNr_;
  Gtk::Label *pregapLen_;
  Gtk::Label *trackStart_;
  Gtk::Label *trackEnd_;
  Gtk::Label *trackLen_;
  Gtk::Label *indexMarks_;

  Gtk::CheckButton *copyFlag_;
  Gtk::CheckButton *preEmphasisFlag_;

  Gtk::RadioButton *twoChannelAudio_;
  Gtk::RadioButton *fourChannelAudio_;

  TextEdit *isrcCodeCountry_;
  TextEdit *isrcCodeOwner_;
  TextEdit *isrcCodeYear_;
  TextEdit *isrcCodeSerial_;

  struct CdTextPage {
    Gtk::Label *label;
    Gtk::Entry *title;
    Gtk::Entry *performer;
    Gtk::Entry *songwriter;
    Gtk::Entry *composer;
    Gtk::Entry *arranger;
    Gtk::Entry *message;
    Gtk::Entry *isrc;
  };

  CdTextPage cdTextPages_[8];
  
  void closeAction();
  void applyAction();

  Gtk::VBox *createCdTextPage(int);

  void clear();
  void clearCdText();

  const char *checkString(const string &);
  void importCdText(const Toc *, int);
  void importData(const Toc *, int);
  void exportCdText(TocEdit *, int);
  void exportData(TocEdit *, int);
};

#endif
