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
 * Revision 1.3  2000/05/01 18:15:00  andreasm
 * Switch to gnome-config settings.
 * Adapted Message Box to Gnome look, unfortunately the Gnome::MessageBox is
 * not implemented in gnome--, yet.
 *
 * Revision 1.2  2000/02/20 23:34:54  llanero
 * fixed scsilib directory (files mising ?-()
 * ported xdao to 1.1.8 / gnome (MDI) app
 *
 * Revision 1.1.1.1  2000/02/05 01:38:46  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 */

#ifndef __MESSAGE_BOX_H__
#define __MESSAGE_BOX_H__

class MessageBoxBase : public Gtk::Dialog {
public:
  MessageBoxBase(Gtk::Window *);
  virtual ~MessageBoxBase();

  void showAskDontShow(bool ask);
  bool dontShowAgain() const;

protected:
  Gtk::VBox contents;
  Gtk::HBox hbox;
  Gtk::VBox vbox;
  Gtk::Image image;
  Gtk::Label primary_text;
  Gtk::Label secondary_text;
  Gtk::CheckButton *dontShowAgain_;
};

class ErrorMessageBox : public MessageBoxBase {
public:
  ErrorMessageBox(Gtk::Window *, const char *title, bool askDontShow,
    const string primary_text, const string secondary_text = 0);
};


class MessageBox : public MessageBoxBase {
public:
  MessageBox(Gtk::Window *, const char *title, bool askDontShow,
    const string primary_text, const string secondary_text = 0);
};

class Ask2Box : public MessageBoxBase {
public:
  Ask2Box(Gtk::Window *, const char *title, bool askDontShow,
    const string primary_text, const string secondary_text = 0);
};

class Ask3Box : public MessageBoxBase {
public:
  Ask3Box(Gtk::Window *, const char *title, bool askDontShow,
    const string primary_text, const string secondary_text = 0);
};

#endif
