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

#include "guiUpdate.h"
#include "Project.h"
#include "DumpCDProject.h"
#include "RecordCDSource.h"
#include "RecordHDTarget.h"
#include "DeviceList.h"
#include "MessageBox.h"
#include "Settings.h"
#include "util.h"

DumpCDProject::DumpCDProject()
{
  set_title("Dump CD to disk");

  Gtk::VBox *vbox = new Gtk::VBox;
  vbox->set_border_width(10);
  vbox->set_spacing(10);
  vbox->show();
  Gtk::HBox *hbox = new Gtk::HBox;
  hbox->set_spacing(10);
  hbox->show();
  vbox->pack_start(*hbox, false, false);
  set_contents(*vbox);

  // menu stuff
  miSave_->set_sensitive(false);
  miSaveAs_->set_sensitive(false);
  miEditTree_->hide();
  miRecord_->set_sensitive(false);
  tiSave_->set_sensitive(false);
  tiRecord_->hide();

  CDSource = new RecordCDSource(this);
  CDSource->onTheFlyOption(false);
  CDSource->show();
  HDTarget = new RecordHDTarget();
  HDTarget->show();

  hbox->pack_start(*CDSource);
  CDSource->show();
  hbox->pack_start(*HDTarget);
  HDTarget->show();

  hbox = new Gtk::HBox;
  hbox->set_spacing(10);
  hbox->show();

  Gtk::VBox *frameBox = new Gtk::VBox;
  frameBox->show();
  hbox->pack_start(*frameBox, true, false);

  Gtk::Image *pixmap = new Gtk::Image(PIXMAPS_DIR "/pixmap_dumpcd.png");
  Gtk::Label *startLabel = manage(new Gtk::Label("      Start      "));
  Gtk::VBox *startBox = manage(new Gtk::VBox);
  Gtk::Button *button = manage(new Gtk::Button());
  startBox->pack_start(*pixmap, false, false);
  startBox->pack_start(*startLabel, false, false);

  button->add(*startBox);
  button->signal_clicked().connect(slot(*this, &DumpCDProject::start));
  pixmap->show();
  startLabel->show();
  startBox->show();
  button->show();

  hbox->pack_start(*button, true, false);

  Gtk::HBox *hbox2 = new Gtk::HBox;
  hbox2->show();
  hbox2->pack_start(*hbox, true, false);
  vbox->pack_start(*hbox2, true, false);

  install_menu_hints();

  CdDevice::signal_statusChanged.connect(slot(*this, &DumpCDProject::devicesStatusChanged));
}

DumpCDProject::~DumpCDProject()
{
  delete CDSource;
  delete HDTarget;
}

void DumpCDProject::start()
{
  DeviceList *sourceList = CDSource->getDeviceList();

  if (sourceList->selectionEmpty()) {
    Gtk::MessageDialog(*this, "Please select one reader device", Gtk::MESSAGE_INFO).run();
    return;
  }

  //Read options
  int correction = CDSource->getCorrection();

  Glib::ustring imageName = HDTarget->getFilename();

  if (imageName == "")
  {
//FIXME: Allow for a temporary file?
    Gtk::MessageDialog(*this, "Please specify a name for the image", Gtk::MESSAGE_INFO).run();
    return;
  }

  char *tmp, *p;
  tmp = strdupCC(imageName.c_str());

  if ((p = strrchr(tmp, '.')) != NULL && strcmp(p, ".toc") == 0)
    *p = 0;

  if (*tmp == 0 || strcmp(tmp, ".") == 0 || strcmp(tmp, "..") == 0) {
    Gtk::MessageDialog(*this, "The specified image name is invalid", Gtk::MESSAGE_ERROR).run();
    delete[] tmp;
    return;
  }

  imageName = tmp;
  delete[] tmp;

  Glib::ustring imagePath;
  Glib::ustring binPath;
  Glib::ustring tocPath;
  {  
    Glib::ustring path = HDTarget->getPath();
    const char *s = path.c_str();
    long len = strlen(s);

    if (len == 0) {
      imagePath = imageName;
    }
    else {
      imagePath = path;

      if (s[len - 1] != '/')
	imagePath += "/";

      imagePath += imageName;
    }
  }
  
  binPath = imagePath;
  binPath += ".bin";

  tocPath = imagePath;
  tocPath += ".toc";

  if (access(binPath.c_str(), R_OK) == 0) {
    Glib::ustring s = "The image file \"";
    s += binPath;
    s += "\" already exists.";

    Ask2Box msg(this, "Dump CD", 0, 1, s.c_str(),
    		"Do you want to overwrite it?", "", NULL);

    if (msg.run() != 1) 
      return;
  }

  if (access(tocPath.c_str(), R_OK) == 0) 
  {
    Glib::ustring s = "The toc-file \"";
    s += tocPath;
    s += "\" already exists.";

    Ask2Box msg(this, "Dump CD", 0, 1, s.c_str(),
    		"Do you want to overwrite it?", "", NULL);

    switch (msg.run()) {
    case 1: // remove the file an continue
      if (unlink(tocPath.c_str()) != -0)
      {
        MessageBox msg(this, "Dump CD", 0,
		       "Cannot delete toc-file", tocPath.c_str(), NULL);
        msg.run();
        return;
      }
      break;
    default: // cancel
      return;
      break;
    }
  }

  // We can only have one source device selected
  CdDevice *sourceDev = sourceList->getFirstSelected();

  if (sourceDev->extractDao(imagePath.c_str(), correction) != 0)
    Gtk::MessageDialog(*this, "Cannot start reading", Gtk::MESSAGE_ERROR).run();
}

bool DumpCDProject::closeProject()
{
  return true;  // Close the project
}

void DumpCDProject::devicesStatusChanged()
{
  CDSource->getDeviceList()->selectOne();
}
