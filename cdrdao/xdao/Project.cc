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

#include <errno.h>

#include <libgnomeuimm.h>

#include "Project.h"
#include "util.h"
#include "xcdrdao.h"
#include "guiUpdate.h"
#include "MessageBox.h"
#include "AudioCDChild.h"
#include "AudioCDView.h"
#include "TocEdit.h"
#include "TocEditView.h"
#include "RecordTocDialog.h"

Project::Project() : Gnome::UI::App("gcdmaster", APP_NAME)
{
  new_ = true;
  saveFileSelector_ = 0;  
  viewNumber = 0;
  recordTocDialog_ = 0;
  enable_layout_config(true);
  set_default_size(630, 440);
  set_wmclass("GCDMaster", "GCDMaster");

  createMenus();
  createToolbar();
  createStatusbar();
}

void Project::createMenus()
{
  std::vector<Gnome::UI::Items::SubTree> menus;
  std::vector<Gnome::UI::Items::Info> fileMenuTree, newMenuTree, editMenuTree, actionsMenuTree;
  std::vector<Gnome::UI::Items::Info> settingsMenuTree, helpMenuTree, windowsMenuTree;

  {
    using namespace Gnome::UI::Items;
    fileMenuTree.push_back(Item(Icon(Gtk::Stock::NEW),
						N_("New..."),
						signal_newProject.slot(),
						N_("New Project")));

    // File->New menu
    newMenuTree.push_back(Item(Icon(Gtk::Stock::NEW),
						N_("_Audio CD"),
						signal_newAudioProject.slot(),
						N_("New Audio CD")));

    newMenuTree.push_back(Item(Icon(Gtk::Stock::NEW),
						N_("_Duplicate CD"),
						signal_newDuplicateProject.slot(),
						N_("Make a copy of a CD")));

    newMenuTree.push_back(Item(Icon(Gtk::Stock::NEW),
						N_("_Copy CD to disk"),
						signal_newDumpProject.slot(),
						N_("Dump CD to disk")));

    // File menu
    fileMenuTree.push_back(SubTree(Icon(Gtk::Stock::NEW),
					    N_("New"),
					    newMenuTree,
					    "Create a new project"));
  }

  guint posFileSave;
  guint posFileSaveAs;
  {
    using namespace Gnome::UI::MenuItems;
    fileMenuTree.push_back(Open(signal_openProject.slot()));
    fileMenuTree.push_back(Save(slot(*this, &Project::saveProject)));
    posFileSave = fileMenuTree.size() - 1;
    fileMenuTree.push_back(SaveAs(slot(*this, &Project::saveAsProject)));
    posFileSaveAs = fileMenuTree.size() - 1;

    fileMenuTree.push_back(Gnome::UI::Items::Separator());

//    fileMenuTree.push_back(PrintSetup(slot(this, &Project::nothing_cb)));
//
//    fileMenuTree.push_back(Gnome::UI::Item(Gnome::UI::Icon(GNOME_STOCK_MENU_PRINT),
//								 N_("Print Cover..."),
//								 slot(this, &Project::nothing_cb),
//								 N_("Print Cover")));
//
//    fileMenuTree.push_back(Gnome::UI::Separator());

    // Close the current child (project);
    fileMenuTree.push_back(Close(signal_closeProject.slot()));
    fileMenuTree.push_back(Exit(signal_exitApp.slot()));
  }

  guint posActionsRecord;
  {
    using namespace Gnome::UI::Items;
    // Edit menu
    editMenuTree.push_back(Item(Icon(Gtk::Stock::PROPERTIES),
    				 N_("Project Info..."),
			      slot(*this, &Project::projectInfo),
			      N_("Edit global project data")));

    // Actions menu
    actionsMenuTree.push_back(Item(Icon(Gtk::Stock::CDROM),
								N_("_Record"),
								slot(*this, &Project::recordToc2CD),
								N_("Record")));
    posActionsRecord = actionsMenuTree.size() - 1;

    actionsMenuTree.push_back(Item(Icon(Gtk::Stock::CDROM),
								N_("Blank CD-RW"),
								signal_blankCD.slot(),
								N_("Erase a CD-RW")));

//    actionsMenuTree.push_back(Gnome::UI::Item(N_("Fixate CD"),
//					    slot(this, &Project::nothing_cb)));
//    actionsMenuTree.push_back(Gnome::UI::Item(N_("Get Info"),
//					    slot(this, &Project::nothing_cb)));

    // Settings menu
    settingsMenuTree.push_back(Item(Icon(Gtk::Stock::PREFERENCES),
								N_("Configure Devices..."),
								signal_configureDevices.slot()));
  }

//    settingsMenuTree.push_back(Gnome::MenuItems::Preferences
//  				(slot(this, &Project::nothing_cb)));


  // Help menu
  //helpMenuTree.push_back(Gnome::UI::Help("Quick Start"));

  helpMenuTree.push_back(Gnome::UI::MenuItems::About
  				(signal_about.slot()));

  {
    using namespace Gnome::UI::Menus;
    menus.push_back(File(fileMenuTree));
    menus.push_back(Edit(editMenuTree));
    menus.push_back(Gnome::UI::Items::Menu(N_("_Actions"), actionsMenuTree));
    menus.push_back(Settings(settingsMenuTree));
//    menus.push_back(Windows(windowsMenuTree));
    menus.push_back(Help(helpMenuTree));
  }

  {
    using namespace Gnome::UI::Items;
	
    Array<SubTree>& arrayInfo = create_menus(menus);
    SubTree& subtreeFile = arrayInfo[0];
    SubTree& subtreeEdit = arrayInfo[1];
    SubTree& subtreeAction = arrayInfo[2];
    Array<Info>& arrayInfoFile = subtreeFile.get_uitree();
    Array<Info>& arrayInfoEdit = subtreeEdit.get_uitree();
    Array<Info>& arrayInfoAction = subtreeAction.get_uitree();

    // Get widget of created menuitems
    miSave_ = arrayInfoFile[posFileSave].get_widget();
    miSaveAs_ = arrayInfoFile[posFileSaveAs].get_widget();
    miEditTree_ = subtreeEdit.get_widget();
    miRecord_ = arrayInfoAction[posActionsRecord].get_widget();
  }
}

void Project::createToolbar()
{
  toolbar_ = new Gtk::Toolbar;
  Glib::RefPtr<Gtk::AccelGroup> accel_group = Gtk::AccelGroup::create();

  Gtk::Image *pixmap = manage (new Gtk::Image(Gtk::StockID(Gtk::Stock::NEW),
                              Gtk::ICON_SIZE_LARGE_TOOLBAR));
  toolbar_->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem(N_("New"), *pixmap,
  		signal_newProject.slot(), N_("New project"), ""));

  pixmap = manage (new Gtk::Image(Gtk::StockID(Gtk::Stock::OPEN),
                              Gtk::ICON_SIZE_LARGE_TOOLBAR));
  toolbar_->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem(N_("Open"), *pixmap,
  		signal_openProject.slot(), N_("Open a project"), ""));

  pixmap = manage (new Gtk::Image(Gtk::StockID(Gtk::Stock::SAVE),
                              Gtk::ICON_SIZE_LARGE_TOOLBAR));
  toolbar_->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem(N_("Save"), *pixmap,
  		slot(*this, &Project::saveProject), N_("Save current project"), ""));
  tiSave_ = toolbar_->tools().back().get_widget();

  pixmap = manage(new Gtk::Image(Gtk::StockID(Gtk::Stock::CDROM),
                              Gtk::ICON_SIZE_LARGE_TOOLBAR));
  toolbar_->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem(N_("Record"), *pixmap,
  		slot(*this, &Project::recordToc2CD), N_("Record to CD"), ""));
  tiRecord_ = toolbar_->tools().back().get_widget();

  set_toolbar(*toolbar_);
}

void Project::createStatusbar()
{
  Gtk::HBox *container = new Gtk::HBox;
  statusbar_ = new Gnome::UI::AppBar(FALSE, TRUE, Gnome::UI::PREFERENCES_NEVER);
  progressbar_ = new Gtk::ProgressBar;
  progressButton_ = new Gtk::Button("Cancel");
  progressButton_->set_sensitive(false);

  progressbar_->set_size_request(150, 0);
  container->pack_start(*statusbar_, TRUE, TRUE); 
  container->pack_start(*progressbar_, FALSE, FALSE); 
  container->pack_start(*progressButton_, FALSE, FALSE); 
  set_statusbar_custom(*container, *statusbar_);
  container->set_spacing(2);
  container->set_border_width(2);
  container->show_all();
}

void Project::updateWindowTitle()
{
  std::string s(tocEdit_->filename());
  s += " - ";
  s += APP_NAME;
  if (tocEdit_->tocDirty())
    s += "(*)";
  set_title(s);
}

void Project::saveProject()
{
  if (new_)
  {
    saveAsProject();
    return;
  }
  if (tocEdit_->saveToc() == 0)
  {
    statusMessage("Project saved to ", tocEdit_->filename());
//FIXME    guiUpdate();
  }
  else {
    std::string s("Cannot save toc to \"");
    s += tocEdit_->filename();
    s+= "\":";
    
    MessageBox msg(this, "Save Project", 0, s.c_str(), strerror(errno), NULL);
    msg.run();
  }
}

void Project::saveAsProject()
{
  if (saveFileSelector_)
  {
    Glib::RefPtr<Gdk::Window> selector_win = saveFileSelector_->get_window();
    selector_win->show();
    selector_win->raise();
  }
  else
  {
    saveFileSelector_ = new Gtk::FileSelection("Save Project");
    saveFileSelector_->get_ok_button()->signal_clicked().connect(
				slot(*this, &Project::saveFileSelectorOKCB));
    saveFileSelector_->get_cancel_button()->signal_clicked().connect(
				slot(*this, &Project::saveFileSelectorCancelCB));
    saveFileSelector_->set_transient_for(*this);
  }
  saveFileSelector_->show();
}

void Project::saveFileSelectorCancelCB()
{
  saveFileSelector_->hide();
  delete saveFileSelector_;
  saveFileSelector_ = 0;
}

void Project::saveFileSelectorOKCB()
{
  char *s = g_strdup(saveFileSelector_->get_filename().c_str());

  if (s != NULL && *s != 0 && s[strlen(s) - 1] != '/') {
    if (tocEdit_->saveAsToc(stripCwd(s)) == 0) {
      statusMessage("Project saved to \"%s\".", tocEdit_->filename());
//FIXME  	guiUpdate();

      new_ = false; // The project is now saved
      //cout << tocEdit_->filename() << endl;
      updateWindowTitle();
      saveFileSelectorCancelCB();
    }
    else {
  	std::string m("Cannot save toc to \"");
  	m += tocEdit_->filename();
  	m += "\":";
    
  	MessageBox msg(saveFileSelector_, "Save Project", 0, m.c_str(), strerror(errno), NULL);
  	msg.run();
    }
    g_free(s);
  }
}

gint Project::getViewNumber()
{
  return(viewNumber++);
}

void Project::statusMessage(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  char *s = g_strdup_vprintf(fmt, args);

//GTKMM2  flash(s);

  free(s);

  va_end(args);
}

int Project::projectNumber()
{
  return projectNumber_;
}

TocEdit *Project::tocEdit()
{
  return tocEdit_;
}

void Project::tocBlockedMsg(const char *op)
{
  MessageBox msg(this, op, 0,
		 "Cannot perform requested operation because", 
		 "project is in read-only state.", NULL);
  msg.run();
}

