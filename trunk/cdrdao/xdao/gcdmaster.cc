/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2000  Andreas Mueller <mueller@daneb.ping.de>
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

#include <gnome.h>

#include "xcdrdao.h"
#include "DeviceConfDialog.h"
#include "RecordGenericDialog.h"
#include "ProjectChooser.h"
#include "gcdmaster.h"
#include "TocEdit.h"
#include "util.h"
#include "AudioCDProject.h"

GCDMaster::GCDMaster()
{
  project_number = 0;
  about_ = 0;
  readFileSelector_ = 0;

}

void GCDMaster::add(Project *project)
{
  projects.push_back(project);
cout << "Number of projects = " << projects.size() << endl;
cout << "Number of choosers = " << choosers.size() << endl;
}

void GCDMaster::add(ProjectChooser *projectChooser)
{
  choosers.push_back(projectChooser);
cout << "Number of projects = " << projects.size() << endl;
cout << "Number of choosers = " << choosers.size() << endl;
}

void GCDMaster::openProject(ProjectChooser *projectChooser)
{
  if (readFileSelector_)
  {
    Gdk_Window selector_win = readFileSelector_->get_window();
    selector_win.show();
    selector_win.raise();
  }
  else
  {
    readFileSelector_ = new Gtk::FileSelection("Open project");
    readFileSelector_->get_ok_button()->clicked.connect(
				bind(slot(this, &GCDMaster::readFileSelectorOKCB), projectChooser));
    readFileSelector_->get_cancel_button()->clicked.connect(
				slot(this, &GCDMaster::readFileSelectorCancelCB));
  }

  readFileSelector_->show();
}

void GCDMaster::readFileSelectorCancelCB()
{
  readFileSelector_->hide();
  readFileSelector_->destroy();
  readFileSelector_ = 0;
}

void GCDMaster::readFileSelectorOKCB(ProjectChooser *projectChooser)
{
  TocEdit *tocEdit = new TocEdit(NULL, NULL);
  char *s = g_strdup(readFileSelector_->get_filename().c_str());

  if (s != NULL && *s != 0 && s[strlen(s) - 1] != '/')
  {
    if (tocEdit->readToc(stripCwd(s)) == 0)
    {
//FIXME: We should test what type of project it is
//       AudioCD, ISO. No problem now.
      cout << "Read ok" << endl;
	 newAudioCDProject(stripCwd(s), tocEdit, NULL);
	 if (projectChooser)
	   closeChooser(projectChooser);
    }
    else
    {
      string message("Error loading ");
      message += s;
      Gnome::Dialogs::error(message); 
    }
  }
  g_free(s);

  readFileSelectorCancelCB();
}

void GCDMaster::closeProject(Project *project)
{
  if (project->closeProject())
  {
    delete project;
    projects.remove(project);
  }
cout << "Number of projects = " << projects.size() << endl;
cout << "Number of choosers = " << choosers.size() << endl;
  if ((projects.size() == 0) & (choosers.size() == 0))
    appClose();
}

void GCDMaster::closeChooser(ProjectChooser *projectChooser)
{
  delete projectChooser;
  choosers.remove(projectChooser);
cout << "Number of projects = " << projects.size() << endl;
cout << "Number of choosers = " << choosers.size() << endl;
  if ((projects.size() == 0) & (choosers.size() == 0))
    appClose();
}

void GCDMaster::appClose()
{
  for (list<Project *>::iterator i = projects.begin();
       i != projects.end(); i++)
  {
    if (!((*i)->closeProject()))
      return;
  }
  Gnome::Main::quit();
}

void GCDMaster::newChooserWindow()
{
  ProjectChooser *projectChooser = new ProjectChooser();
  projectChooser->show();
//  Project *project = new Project(project_number);
//  project->newChooserWindow();
//  As it always can be closed, we don't add it.
  add(projectChooser);
}

void GCDMaster::newAudioCDProject(const char *name, TocEdit *tocEdit, ProjectChooser *projectChooser)
{
  AudioCDProject *project = new AudioCDProject(project_number++, name, tocEdit);
  add(project);
  project->show();
  if (projectChooser)
    closeChooser(projectChooser);
}

void GCDMaster::newAudioCDProject2(ProjectChooser *projectChooser)
{
  AudioCDProject *project = new AudioCDProject(project_number++, "", NULL);
  add(project);
  project->show();
  if (projectChooser)
    closeChooser(projectChooser);
}

void GCDMaster::recordCD2CD()
{
  RECORD_GENERIC_DIALOG->cd_to_cd();
}

void GCDMaster::recordCD2HD()
{
  RECORD_GENERIC_DIALOG->cd_to_hd();
}

void GCDMaster::configureDevices()
{
  DEVICE_CONF_DIALOG->start();
}

void GCDMaster::aboutDialog()
{

  if (about_) // "About" dialog hasn't been closed, so just raise it
  {
      Gdk_Window about_win = about_->get_window();
      about_win.show();
      about_win.raise();
  }
  else
  {
    gchar *logo_char;
    string logo;
    vector<string> authors;
    authors.push_back("Andreas Mueller <mueller@daneb.ping.de>");
    authors.push_back("Manuel Clos <llanero@jazzfree.com>");

//FIXME: not yet wrapped
    logo_char = gnome_pixmap_file("gcdmaster/gcdmaster.png");

    if (logo_char != NULL)
      logo = logo_char;

    about_ = new Gnome::About(_("gcdmaster"), VERSION,
                               "(C) Andreas Mueller",
                               authors,
                               _("A CD Mastering app for Gnome."),
                               logo);

//    about_->set_parent(*this);
//    about_->destroy.connect(slot(this, &GCDMaster::aboutDestroy));
    // We attach to the close signal because default behaviour is
    // close_hides = true, as in a normal dialog
    about_->close.connect(slot(this, &GCDMaster::aboutDestroy));
    about_->show();
  }
}

int GCDMaster::aboutDestroy()
{
  cout << "about closed" << endl;
  delete about_;
  about_ = 0;
  return true; // Do not close, as we have already deleted it.
}
