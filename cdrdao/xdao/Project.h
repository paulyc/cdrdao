/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 2000  Andreas Mueller <mueller@daneb.ping.de>
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

#ifndef __PROJECT_H__
#define __PROJECT_H__


class RecordTocDialog;
class TocEdit;
#include "ViewSwitcher.h"

class Project : public Gnome::UI::App
{
protected:
  Gtk::Toolbar *toolbar;
  Gtk::HBox *hbox;
  Gnome::UI::AppBar *statusbar_;  
  Gtk::ProgressBar *progressbar_;  
  Gtk::Button *progressButton_;  
  ViewSwitcher *viewSwitcher_;
  
  int projectNumber_;
  gint viewNumber;
  bool new_; // If it is a new project (not saved)

  TocEdit *tocEdit_;
  RecordTocDialog *recordTocDialog_;

  Gtk::Widget *miSave_;
  Gtk::Widget *miSaveAs_;
  Gtk::Widget *miEditTree_;
  Gtk::Widget *miRecord_;
  Gtk::Widget *tiSave_;
  Gtk::Widget *tiRecord_;

  void createMenus();
  void createToolbar();
  void createStatusbar();
  void updateWindowTitle();
  void saveProject();
  void saveAsProject();
  Gtk::FileSelection *saveFileSelector_;
  void saveFileSelectorOKCB();
  void saveFileSelectorCancelCB();

  virtual void recordToc2CD() = 0;
  virtual void projectInfo() = 0;

public:
  Project();

  void readToc(char *name);
  void statusMessage(const char *fmt, ...);
  void tocBlockedMsg(const char *);
  virtual bool closeProject() = 0;
  int projectNumber();
  TocEdit *tocEdit();
  gint getViewNumber();

  SigC::Signal0<void> signal_newProject;
  SigC::Signal0<void> signal_openProject;
  SigC::Signal0<void> signal_newAudioProject;
  SigC::Signal0<void> signal_newDuplicateProject;
  SigC::Signal0<void> signal_newDumpProject;
  SigC::Signal0<void> signal_closeProject;
  SigC::Signal0<void> signal_exitApp;
  SigC::Signal0<void> signal_blankCD;
  SigC::Signal0<void> signal_configureDevices;
  SigC::Signal0<void> signal_about;

  virtual void update(unsigned long level) = 0;
};
#endif

