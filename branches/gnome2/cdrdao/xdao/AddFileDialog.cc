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


#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "AddFileDialog.h"

#include "guiUpdate.h"
#include "TocEdit.h"
#include "Sample.h"
#include "util.h"
#include "AudioCDProject.h"
#include "xcdrdao.h"

AddFileDialog::AddFileDialog(AudioCDProject *project)
    : Gtk::FileSelection("")
{
  active_ = 0;
  project_ = project;

  set_filename("*.wav");
  show_fileop_buttons();
  set_select_multiple(false);
  set_transient_for(*project);
  mode(M_APPEND_TRACK);

  Gtk::Button* cancel = get_cancel_button();
  cancel->set_label(Gtk::Stock::CLOSE.id);
  cancel->set_use_stock(true);

  Gtk::Button* ok = get_ok_button();
  ok->set_label(Gtk::Stock::ADD.id);
  ok->set_use_stock(true);

  ok->signal_clicked().connect(SigC::slot(*this,&AddFileDialog::applyAction));
  cancel->signal_clicked().connect(SigC::slot(*this,
                                              &AddFileDialog::closeAction));
}

AddFileDialog::~AddFileDialog()
{
}

void AddFileDialog::mode(Mode m)
{
  mode_ = m;

  switch (mode_) {
  case M_APPEND_TRACK:
    set_title("Append Track");
    break;
  case M_APPEND_FILE:
    set_title("Append File");
    break;
  case M_INSERT_FILE:
    set_title("Insert File");
    break;
  }
}

void AddFileDialog::start()
{
  if (active_) {
    get_window()->raise();
    return;
  }

  active_ = true;
  set_filename("*.wav");
  show();
}

void AddFileDialog::stop()
{
  if (active_) {
    hide();
    active_ = false;
  }
}

void AddFileDialog::update(unsigned long level)
{
  if (level & UPD_EDITABLE_STATE) {
    if (project_->tocEdit()) {
      get_ok_button()->set_sensitive(project_->tocEdit()->editable());
    }
  }
}

bool AddFileDialog::on_delete_event(GdkEventAny*)
{
  stop();
  return 1;
}

void AddFileDialog::closeAction()
{
  stop();
}

void AddFileDialog::applyAction()
{
  if (!project_->tocEdit() ||
      !project_->tocEdit()->editable()) {
    return;
  }

  std::string sel = get_filename();
  const char *s = stripCwd(sel.c_str());
  
  if (s != NULL && *s != 0 && s[strlen(s) - 1] != '/') {
    unsigned long pos;

    switch (mode_) {
    case M_APPEND_TRACK:
      project_->appendTrack(s);
      break;

    case M_APPEND_FILE:
      project_->appendFile(s);
      break;

    case M_INSERT_FILE:
      project_->insertFile(s);
      break;
    }
  }

  set_filename("*.wav");
}
