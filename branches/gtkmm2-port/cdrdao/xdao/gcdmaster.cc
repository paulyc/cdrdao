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

#include <libgnomeuimm.h>

#include "xcdrdao.h"
#include "DeviceConfDialog.h"
#include "ProjectChooser.h"
#include "TocEdit.h"
#include "util.h"
#include "AudioCDView.h"
#include "AudioCDProject.h"
#include "DuplicateCDProject.h"
#include "BlankCDDialog.h"
#include "DumpCDProject.h"
#include "gcdmaster.h"

GCDMaster::GCDMaster()
{
  project_number = 0;
  about_ = 0;
  readFileSelector_ = 0;
  blankCDDialog_ = 0;
}

GCDMaster::~GCDMaster()
{
  if (about_)
    delete about_;
}

void GCDMaster::add(Project *project)
{
  projects.push_back(project);
}

void GCDMaster::add(ProjectChooser *projectChooser)
{
  choosers.push_back(projectChooser);
}

bool GCDMaster::openNewProject(const char* s)
{
  TocEdit *tocEdit = new TocEdit(NULL, NULL);

  if (s != NULL && *s != 0 && s[strlen(s) - 1] != '/')
  {
    if (tocEdit->readToc(stripCwd(s)) == 0)
    {
      //FIXME: We should test what type of project it is
      //       AudioCD, ISO. No problem now.

      newAudioCDProject(stripCwd(s), tocEdit, NULL);
    }
    else
      return false;
  }
  return true;
}

void GCDMaster::openProject(ProjectChooser *projectChooser)
{
  if (readFileSelector_)
  {
    readFileSelector_->present();
  }
  else
  {
    readFileSelector_ = new Gtk::FileSelection("Open project");
    readFileSelector_->get_ok_button()->signal_clicked().connect(
				bind(slot(*this, &GCDMaster::readFileSelectorOKCB), projectChooser));
    readFileSelector_->get_cancel_button()->signal_clicked().connect(
				slot(*this, &GCDMaster::readFileSelectorCancelCB));
  }

  readFileSelector_->show();
}

void GCDMaster::readFileSelectorCancelCB()
{
  readFileSelector_->hide();
  delete readFileSelector_;
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
      newAudioCDProject(stripCwd(s), tocEdit, NULL);
      if (projectChooser)
        closeChooser(projectChooser);
    }
    else
    {
      std::string message("Error loading ");
      message += s;
      Gtk::MessageDialog(message, Gtk::MESSAGE_ERROR).run();
    }
  }
  g_free(s);

  readFileSelectorCancelCB();
}

void GCDMaster::closeProject(Project *project)
{
  if (project->closeProject())
  {
    projects.remove(project);
    delete project;
  }

  if ((projects.size() == 0) && (choosers.size() == 0))
    Gnome::Main::quit(); // Quit if there are not remaining windows
}

void GCDMaster::closeChooser(ProjectChooser *projectChooser)
{
  choosers.remove(projectChooser);
  delete projectChooser;

  if ((projects.size() == 0) && (choosers.size() == 0))
    Gnome::Main::quit(); // Quit if there are not remaining windows
}

void GCDMaster::appClose(Project *project)
{
  if (project->closeProject())
  {
    Project *previous = 0;

    projects.remove(project);
    delete project;

    for (std::list<Project *>::iterator i = projects.begin();
         i != projects.end(); i++)
    {
	  if (previous != 0)
      {
        projects.remove(previous);
        delete (previous);
      }
      if (!((*i)->closeProject()))
        return;
      previous = *i;
    }
    Gnome::Main::quit();
  }
}

void GCDMaster::connectSignals(Project *project)
{
  project->signal_newProject.connect(
    slot(*this, &GCDMaster::newChooserWindow));
  project->signal_newAudioProject.connect(
    bind(slot(*this, &GCDMaster::newAudioCDProject2), (ProjectChooser *)0));
  project->signal_newDuplicateProject.connect(
    bind(slot(*this, &GCDMaster::newDuplicateCDProject), (ProjectChooser *)0));
  project->signal_newDumpProject.connect(
    bind(slot(*this, &GCDMaster::newDumpCDProject), (ProjectChooser *)0));
  project->signal_openProject.connect(
    bind(slot(*this, &GCDMaster::openProject), (ProjectChooser *)0));
  project->signal_closeProject.connect(
    bind(slot(*this, &GCDMaster::closeProject), project));
  project->signal_hide().connect(
    bind(slot(*this, &GCDMaster::closeProject), project));
  project->signal_exitApp.connect(
    bind(slot(*this, &GCDMaster::appClose), project));
  project->signal_blankCD.connect(
    slot(*this, &GCDMaster::blankCDRW));
  project->signal_configureDevices.connect(
    slot(*this, &GCDMaster::configureDevices));
  project->signal_about.connect(
    slot(*this, &GCDMaster::aboutDialog));
}

void GCDMaster::newChooserWindow()
{
  ProjectChooser *projectChooser = new ProjectChooser();
  projectChooser->show();
  add(projectChooser);

  projectChooser->signal_openProject.connect(
    bind(slot(*this, &GCDMaster::openProject), projectChooser));
  projectChooser->signal_newAudioProject.connect(
    bind(slot(*this, &GCDMaster::newAudioCDProject2), projectChooser));
  projectChooser->signal_newDuplicateProject.connect(
    bind(slot(*this, &GCDMaster::newDuplicateCDProject), projectChooser));
  projectChooser->signal_newDumpProject.connect(
    bind(slot(*this, &GCDMaster::newDumpCDProject), projectChooser));
  projectChooser->signal_hide().connect(
    bind(slot(*this, &GCDMaster::closeChooser), projectChooser));
}

ProjectChooser* GCDMaster::newChooserWindow2()
{
  ProjectChooser *projectChooser = new ProjectChooser();
  projectChooser->show();
  add(projectChooser);

  projectChooser->signal_openProject.connect(
    bind(slot(*this, &GCDMaster::openProject), projectChooser));
  projectChooser->signal_newAudioProject.connect(
    bind(slot(*this, &GCDMaster::newAudioCDProject2), projectChooser));
  projectChooser->signal_newDuplicateProject.connect(
    bind(slot(*this, &GCDMaster::newDuplicateCDProject), projectChooser));
  projectChooser->signal_newDumpProject.connect(
    bind(slot(*this, &GCDMaster::newDumpCDProject), projectChooser));
  projectChooser->signal_hide().connect(
    bind(slot(*this, &GCDMaster::closeChooser), projectChooser));

  return projectChooser;
}

void GCDMaster::newAudioCDProject2(ProjectChooser *projectChooser)
{
    newAudioCDProject("", NULL, projectChooser);
}

void GCDMaster::newAudioCDProject(const char *name, TocEdit *tocEdit, ProjectChooser *projectChooser)
{
  AudioCDProject *project = new AudioCDProject(project_number++, name, tocEdit);
  add(project);

  connectSignals(project);

//FIXME: don't use viewSwitcher
// NOTE: We can't show the Gnome::App here, because it also shows all the DockItems
// it contains, and the viewSwitcher will take care of this.
//  project->show();

  if (projectChooser)
    closeChooser(projectChooser);
}

void GCDMaster::newDuplicateCDProject(ProjectChooser *projectChooser)
{
  DuplicateCDProject *project = new DuplicateCDProject();
  add(project);
  project->show();

  connectSignals(project);

  if (projectChooser)
    closeChooser(projectChooser);
}

void GCDMaster::newDumpCDProject(ProjectChooser *projectChooser)
{
  DumpCDProject *project = new DumpCDProject();
  add(project);
  project->show();

  connectSignals(project);

  if (projectChooser)
    closeChooser(projectChooser);
}

void GCDMaster::update(unsigned long level)
{
  for (std::list<Project *>::iterator i = projects.begin();
       i != projects.end(); i++)
  {
//GTKMM2    (*i)->update(level);
  }
}

void GCDMaster::configureDevices()
{
  DEVICE_CONF_DIALOG->start();
}

void GCDMaster::blankCDRW()
{
  if (blankCDDialog_ == 0)
    blankCDDialog_ = new BlankCDDialog;

  blankCDDialog_->present();
}

void GCDMaster::aboutDialog()
{
  if (about_) // "About" dialog hasn't been closed, so just raise it
  {
    about_->present();
  }
  else
  {
    std::vector<std::string> authors;
    std::vector<std::string> documenters;
    Glib::RefPtr<Gdk::Pixbuf> logo;

    authors.push_back("Andreas Mueller <mueller@daneb.ping.de>");
    authors.push_back("Manuel Clos <llanero@jazzfree.com>");

    logo = Gdk::Pixbuf::create_from_file(PIXMAPS_DIR "/gcdmaster.png");

    about_ = new Gnome::UI::About(_("gcdmaster"), VERSION,
                               "(C) Andreas Mueller",
                               authors,
							   documenters,
                               _("A CD Mastering app for Gnome."),
							   _("this is an untranslated version."),
                               logo);
    about_->show();
  }
}
