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

#ifndef __GCDMASTER_H__
#define __GCDMASTER_H__

#include <gtkmm.h>
#include <gtk/gtk.h>
#include <libgnomeuimm.h>

#include <list>

class ProjectChooser;
class BlankCDDialog;
#include "Project.h"

class GCDMaster : public Gnome::UI::App
{
private:
  static std::list<Project *> projects;
  static std::list<ProjectChooser *> choosers;

  Project * project_;
  ProjectChooser * projectChooser_;
  gint project_number;

  BlankCDDialog *blankCDDialog_;

  Gtk::Notebook notebook_;

  Glib::RefPtr<Gtk::UIManager> m_refUIManager;
  Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;

  Gnome::UI::AppBar* statusbar_;  
  Gtk::ProgressBar* progressbar_;  
  Gtk::Button* progressButton_;  
  Gnome::UI::About* about_;

  void add(Project *);
  void add(ProjectChooser *);

  Gtk::FileSelection readFileSelector_;
  void readFileSelectorOKCB();
  void readFileSelectorCancelCB();
  void createMenus();
  void createStatusbar();
  void aboutDialog();

public:
  GCDMaster();

  static std::list<GCDMaster *> apps;

  void appClose();
  void closeProject();
  void closeChooser();
  bool on_delete_event(GdkEventAny* e);
  bool openNewProject(const char*);
  void openProject();
  void newChooserWindow();
  void newAudioCDProject2();
  void newAudioCDProject(const char *name, TocEdit *tocEdit);
  void newDuplicateCDProject();
  void newDumpCDProject();

  void update(unsigned long level);

  void configureDevices();
  void blankCDRW();

  void registerStockIcons();
};
#endif
