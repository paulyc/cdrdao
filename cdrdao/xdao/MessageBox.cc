/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2002  Andreas Mueller <andreas@daneb.de>
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

#include <stddef.h>
#include <stdarg.h>

#include <libgnomeuimm.h>

#include "MessageBox.h"

MessageBoxBase::MessageBoxBase(Gtk::Window * win)
 : contents(false, 24), hbox(false, 12), vbox(false, 12)
{
  dontShowAgain_ = NULL;

  set_has_separator(false);

  contents.set_border_width(12);

  image.set_alignment(0.5, 0.0);
  primary_text.set_line_wrap(true);
  primary_text.set_selectable(true);
  primary_text.set_alignment(0.0, 0.0);
  secondary_text.set_line_wrap(true);
  secondary_text.set_selectable(true);
  secondary_text.set_alignment(0.0, 0.0);

  hbox.pack_start(image, false, false, 0);
  hbox.pack_start(vbox, false, false, 0);
  vbox.pack_start(primary_text, true, true, 0);
  vbox.pack_start(secondary_text, true, true, 0);

  contents.pack_start(hbox, true, true, 0);

  get_vbox()->pack_start(contents);
  contents.show_all();

  if (win != NULL)
    set_transient_for(*win);
  else
    set_position(Gtk::WIN_POS_CENTER);
}

MessageBoxBase::~MessageBoxBase()
{
  delete dontShowAgain_;
  dontShowAgain_ = NULL;
}

void MessageBoxBase::showAskDontShow(bool ask)
{
  int i;
  const char *s;

  if (ask) {
    dontShowAgain_ = new Gtk::CheckButton("Don't show this message again");
    dontShowAgain_->set_active(FALSE);
    dontShowAgain_->show();

    vbox.pack_start(*dontShowAgain_, true, true, 0);
  }
}

bool MessageBoxBase::dontShowAgain() const
{
  if (dontShowAgain_ != NULL)
    return dontShowAgain_->get_active() ? 1 : 0;
  else
    return 0;
}

ErrorMessageBox::ErrorMessageBox(Gtk::Window *win, const char *title, bool askDontShow,
  const string primary_text, const string secondary_text = 0)
  : MessageBoxBase(win)
{
  set_title(title);
  image.set(Gtk::Stock::DIALOG_ERROR, Gtk::ICON_SIZE_DIALOG);
  add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
  this->primary_text.set_markup("<b><big>" + primary_text + "</big></b>");
  this->secondary_text.set_text(secondary_text);
  showAskDontShow(askDontShow);
}

MessageBox::MessageBox(Gtk::Window *win, const char *title, bool askDontShow,
  const string primary_text, const string secondary_text = 0)
  : MessageBoxBase(win)
{
  set_title(title);
  image.set(Gtk::Stock::DIALOG_INFO, Gtk::ICON_SIZE_DIALOG);
  add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
  this->primary_text.set_markup("<b><big>" + primary_text + "</big></b>");
  this->secondary_text.set_text(secondary_text);
  showAskDontShow(askDontShow);
}

Ask2Box::Ask2Box(Gtk::Window *win, const char *title, bool askDontShow,
  const string primary_text, const string secondary_text = 0)
  : MessageBoxBase(win)

{
  set_title(title);
  image.set(Gtk::Stock::DIALOG_QUESTION, Gtk::ICON_SIZE_DIALOG);
  add_button(Gtk::Stock::YES, Gtk::RESPONSE_YES);
  add_button(Gtk::Stock::NO, Gtk::RESPONSE_NO);
  this->primary_text.set_markup("<b><big>" + primary_text + "</big></b>");
  this->secondary_text.set_text(secondary_text);
  showAskDontShow(askDontShow);
}

Ask3Box::Ask3Box(Gtk::Window *win, const char *title, bool askDontShow,
  const string primary_text, const string secondary_text = 0)
  : MessageBoxBase(win)
{
  set_title(title);
  image.set(Gtk::Stock::DIALOG_QUESTION, Gtk::ICON_SIZE_DIALOG);
  add_button(Gtk::Stock::NO, Gtk::RESPONSE_NO);
  add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  add_button(Gtk::Stock::YES, Gtk::RESPONSE_YES);
  this->primary_text.set_markup("<b><big>" + primary_text + "</big></b>");
  this->secondary_text.set_text(secondary_text);
  showAskDontShow(askDontShow);
}
