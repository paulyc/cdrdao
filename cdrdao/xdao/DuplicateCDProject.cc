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
#include "DuplicateCDProject.h"
#include "RecordCDSource.h"
#include "RecordCDTarget.h"
#include "DeviceList.h"
#include "MessageBox.h"
#include "Settings.h"

DuplicateCDProject::DuplicateCDProject()
{
  set_title("Duplicate CD");

  Gtk::VBox *vbox = new Gtk::VBox;
  vbox->set_border_width(10);
  vbox->set_spacing(10);
  vbox->show();
  Gtk::HBox *hbox = new Gtk::HBox;
  hbox->set_spacing(20);
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
  CDSource->show();
  CDTarget = new RecordCDTarget(this);
  CDTarget->show();

  hbox->pack_start(*CDSource);
  CDSource->show();
  hbox->pack_start(*CDTarget);
  CDTarget->show();

  hbox = new Gtk::HBox;
  hbox->set_spacing(10);
  hbox->show();

  Gtk::VBox *frameBox = new Gtk::VBox;
  frameBox->show();
  Gtk::RadioButton_Helpers::Group group;
  simulate_rb = new Gtk::RadioButton(group, "Simulate", 0);
  simulateBurn_rb = new Gtk::RadioButton(group, "Simulate and Burn", 0);
  burn_rb = new Gtk::RadioButton(group, "Burn", 0);

  frameBox->pack_start(*simulate_rb);
  simulate_rb->show();
  frameBox->pack_start(*simulateBurn_rb);
//  simulateBurn_rb->show();
  frameBox->pack_start(*burn_rb);
  burn_rb->show();

  hbox->pack_start(*frameBox, true, false);

  Gtk::Image *pixmap = new Gtk::Image(PIXMAPS_DIR "/gcdmaster.png");
  Gtk::Label *startLabel = manage(new Gtk::Label("      Start      "));
  Gtk::VBox *startBox = manage(new Gtk::VBox);
  Gtk::Button *button = manage(new Gtk::Button());
  startBox->pack_start(*pixmap, false, false);
  startBox->pack_start(*startLabel, false, false);

  button->add(*startBox);
  button->signal_clicked().connect(slot(*this, &DuplicateCDProject::start));
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

  CdDevice::signal_statusChanged.connect(slot(*this, &DuplicateCDProject::devicesStatusChanged));
}

DuplicateCDProject::~DuplicateCDProject()
{
  delete CDSource;
  delete CDTarget;
}

void DuplicateCDProject::start()
{
  DeviceList *sourceList = CDSource->getDeviceList();
  DeviceList *targetList = CDTarget->getDeviceList();

  if (sourceList->selectionEmpty()) {
    Gtk::MessageDialog(*this, "Please select one reader device", Gtk::MESSAGE_INFO).run();
    return;
  }

  if (targetList->selectionEmpty()) {
    Gtk::MessageDialog(*this, "Please select at least one recorder device", Gtk::MESSAGE_INFO).run();
    return;
  }

  // We can only have one source device selected
  CdDevice *sourceDev = sourceList->getFirstSelected();

  //Read options
  int onTheFly = CDSource->getOnTheFly();
  if (onTheFly)
  {
    // We can't make on the fly copy with the same device, check that

    if (targetList->isSelected(sourceDev))
    {
      // If the user selects the same device for reading and writing
      // we can't do on the fly copying. More complex situations with
      // multiple target devices are not handled
      if (gnome_config_get_bool(SET_DUPLICATE_ONTHEFLY_WARNING)) {
        Ask2Box msg(this, "Request", 1, 2, 
  		  "To duplicate a CD using the same device for reading and writing",
  		  "you need to copy the CD to disk before burning", "",
  	  	"Proceed and copy to disk before burning?", NULL);

        switch (msg.run()) {
        case 1: // proceed without on the fly
          CDSource->setOnTheFly(false);
          onTheFly = 0;
          if (msg.dontShowAgain())
          {
        	gnome_config_set_bool(SET_DUPLICATE_ONTHEFLY_WARNING, FALSE);
      	    gnome_config_sync();
          }
          break;
        default: // do not proceed
          return;
          break;
        }
      }
      else
      {
        CDSource->setOnTheFly(false);
        onTheFly = 0;
      }
    }
  }

  int correction = CDSource->getCorrection();

  //Record options
  int simulate;
  if (simulate_rb->get_active())
    simulate = 1;
  else if (simulateBurn_rb->get_active())
    simulate = 2;
  else
    simulate = 0;

  int multiSession = CDTarget->getMultisession();
  int burnSpeed = CDTarget->getSpeed();
  int eject = CDTarget->checkEjectWarning(this);
  if (eject == -1)
    return;

  int reload = CDTarget->checkReloadWarning(this);
  if (reload == -1)
    return;

  int buffer = CDTarget->getBuffer();

  std::list<CdDevice *> devices = targetList->getAllSelected();

  for (std::list<CdDevice *>::iterator i = devices.begin();
         i != devices.end(); i++)
  {
    CdDevice *writeDevice = *i;

    if (writeDevice->duplicateDao(simulate, multiSession, burnSpeed,
        eject, reload, buffer, onTheFly, correction, sourceDev) != 0)
      Gtk::MessageDialog(*this, "Cannot start disk-at-once duplication", Gtk::MESSAGE_ERROR).run();
  }
}

bool DuplicateCDProject::closeProject()
{
  return true;  // Close the project
}

void DuplicateCDProject::devicesStatusChanged()
{
  DeviceList *sourceList = CDSource->getDeviceList();
  DeviceList *targetList = CDTarget->getDeviceList();

  targetList->selectOne();

  CdDevice *targetDev = targetList->getFirstSelected();

  if (targetDev == 0)
    sourceList->selectOne();
  else
    sourceList->selectOneBut(targetDev);
}
