/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2002  Andreas Mueller <andreas@daneb.ping.de>
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

#include <libgnomeuimm.h>

#include "RecordCDTarget.h"
#include "MessageBox.h"
#include "xcdrdao.h"
#include "Settings.h"

#include "CdDevice.h"
#include "guiUpdate.h"
#include "TocEdit.h"
#include "DeviceList.h"

#include "util.h"

RecordCDTarget::RecordCDTarget(Gtk::Window *parent)
{
  speed_ = 1;
  parent_ = parent;
  moreTargetOptions_ = 0;
  set_spacing(10);

  DEVICES = new DeviceList(CdDevice::CD_R);

  pack_start(*DEVICES, false, false);

  // device settings
  Gtk::VBox *vbox = new Gtk::VBox;
  vbox->set_border_width(5);
  vbox->set_spacing(5);
  vbox->show();

  Gtk::Label *optionsLabel = new Gtk::Label("<b><big>Record options</big></b>");
  optionsLabel->set_alignment(0, 1);
  optionsLabel->set_use_markup(true);
  vbox->pack_start(*optionsLabel, false, false);
  optionsLabel->show();

  Gtk::HBox *hbox = new Gtk::HBox;
  hbox->show();
  Gtk::Label *label = new Gtk::Label("Speed: ", 0);
  label->show();
  hbox->pack_start(*label, false, false);
  
  Gtk::Adjustment *adjustment = new Gtk::Adjustment(1, 1, 20);
  speedSpinButton_ = new Gtk::SpinButton(*adjustment);
  speedSpinButton_->set_digits(0);
  speedSpinButton_->show();
  speedSpinButton_->set_sensitive(false);
  adjustment->signal_value_changed().connect(SigC::slot(*this, &RecordCDTarget::speedChanged));
  hbox->pack_start(*speedSpinButton_, false, false, 10);

  speedButton_ = new Gtk::CheckButton("Use max.", 0);
  speedButton_->set_active(true);
  speedButton_->show();
  speedButton_->signal_toggled().connect(SigC::slot(*this, &RecordCDTarget::speedButtonChanged));
  hbox->pack_start(*speedButton_, true, true);
  vbox->pack_start(*hbox);

  hbox = new Gtk::HBox;
//  hbox->show();
  label = new Gtk::Label(string("Copies: "), 0);
  label->show();
  hbox->pack_start(*label, false, false);

  adjustment = new Gtk::Adjustment(1, 1, 999);
  copiesSpinButton_ = new Gtk::SpinButton(*adjustment);
  copiesSpinButton_->set_digits(0);
  copiesSpinButton_->show();
  hbox->pack_start(*copiesSpinButton_, false, false, 10);
  vbox->pack_start(*hbox);

  pack_start(*vbox, false, false);
  vbox->show();
}

RecordCDTarget::~RecordCDTarget()
{
  if (moreTargetOptions_)
    delete moreTargetOptions_;
}

Gtk::VBox *RecordCDTarget::moreOptions()
{
  if (!moreTargetOptions_)
  {
    Gtk::Label *label;

    moreTargetOptions_ = new Gtk::VBox();

    label = new Gtk::Label("<b><big>Record options</big></b>");
    label->set_alignment(0, 1);
    label->set_use_markup(true);
    moreTargetOptions_->pack_start(*label, false, false);
    label->show();
    moreTargetOptions_->set_border_width(10);
    moreTargetOptions_->set_spacing(5);

    ejectButton_ = new Gtk::CheckButton("Eject the CD after writing", 0);
    ejectButton_->set_active(false);
    ejectButton_->show();
    moreTargetOptions_->pack_start(*ejectButton_);

    reloadButton_ = new Gtk::CheckButton("Reload the CD after writing, if necessary", 0);
    reloadButton_->set_active(false);
    reloadButton_->show();
    moreTargetOptions_->pack_start(*reloadButton_);

    closeSessionButton_ = new Gtk::CheckButton("Close disk - no further writing possible!", 0);
    closeSessionButton_->set_active(true);
    closeSessionButton_->show();
    moreTargetOptions_->pack_start(*closeSessionButton_);

    Gtk::HBox *hbox = new Gtk::HBox;
    hbox->show();
    label = new Gtk::Label("Buffer: ", 0);
    label->show();
    hbox->pack_start(*label, false, false);

    Gtk::Adjustment *adjustment = new Gtk::Adjustment(10, 10, 1000);
    bufferSpinButton_ = new Gtk::SpinButton(*adjustment);
    bufferSpinButton_->show();
    hbox->pack_start(*bufferSpinButton_, false, false, 10);

    label = new Gtk::Label("audio seconds ");
    hbox->pack_start(*label, false, false);
    label->show();
    bufferRAMLabel_ = new Gtk::Label("= 1.72 Mb buffer.", 0);
    hbox->pack_start(*bufferRAMLabel_, true, true);
    bufferRAMLabel_->show();
    adjustment->signal_value_changed().connect(SigC::slot(*this, &RecordCDTarget::updateBufferRAMLabel));

	moreTargetOptions_->pack_start(*hbox);
  }

  moreTargetOptions_->show();

  return moreTargetOptions_;
}

int RecordCDTarget::getMultisession()
{
  if (moreTargetOptions_)
    return closeSessionButton_->get_active() ? 0 : 1;
  else
    return 0; 
}

int RecordCDTarget::getCopies()
{
  return copiesSpinButton_->get_value_as_int();
}

int RecordCDTarget::getSpeed()
{
  if (speedButton_->get_active())
    return 0;
  else
    return speed_;
}

int RecordCDTarget::getBuffer()
{
  if (moreTargetOptions_)
    return bufferSpinButton_->get_value_as_int();
  else
    return 10; 
}

bool RecordCDTarget::getEject()
{
  if (moreTargetOptions_)
    return ejectButton_->get_active() ? 1 : 0;
  else
    return 0; 
}

int RecordCDTarget::checkEjectWarning(Gtk::Window *parent)
{
  // If ejecting the CD after recording is requested issue a warning message
  // because buffer under runs may occur for other devices that are recording.
  if (getEject())
  {
    if (gnome_config_get_bool(SET_RECORD_EJECT_WARNING)) {
      Glib::ustring s = "Ejecting a CD may block the SCSI bus and";
      s += "cause buffer under runs when other devices";
      s += "are still recording.";
      Ask3Box msg(parent, "Request", true,
        "Keep the eject setting?", s);
  
      switch (msg.run()) {
      case Gtk::RESPONSE_YES: // keep eject setting
        if (msg.dontShowAgain())
        {
          gnome_config_set_bool(SET_RECORD_EJECT_WARNING, FALSE);
          gnome_config_sync();
        }
        return 1;
        break;
      case Gtk::RESPONSE_NO: // don't keep eject setting
        ejectButton_->set_active(false);
        return 0;
        break;
      default: // cancel
        return -1;
        break;
      }
    }
    return 1;
  }
  return 0;  
}

bool RecordCDTarget::getReload()
{
  if (moreTargetOptions_)
    return reloadButton_->get_active() ? 1 : 0;
  else
    return 0; 
}

int RecordCDTarget::checkReloadWarning(Gtk::Window *parent)
{
  // The same is true for reloading the disk.
  if (getReload())
  {
    if (gnome_config_get_bool(SET_RECORD_RELOAD_WARNING)) {
      Glib::ustring s = "Reloading a CD may block the SCSI bus and";
      s += "cause buffer under runs when other devices";
      s += "are still recording.";
      Ask3Box msg(parent, "Request", true,
        "Keep the reload setting?", s);

      switch (msg.run()) {
      case Gtk::RESPONSE_YES: // keep reload setting
        if (msg.dontShowAgain())
        {
      	gnome_config_set_bool(SET_RECORD_RELOAD_WARNING, FALSE);
          gnome_config_sync();
        }
        return 1;
        break;
      case Gtk::RESPONSE_NO: // don't keep reload setting
        reloadButton_->set_active(false);
        return 0;
        break;
      default: // cancel
        return -1;
        break;
      }
    }
    return 1;
  }
  return 0;
}

void RecordCDTarget::updateBufferRAMLabel()
{
  char label[20];
  
  sprintf(label, "= %0.2f Mb buffer.", bufferSpinButton_->get_value() * 0.171875);
  bufferRAMLabel_->set_text(label);
}

void RecordCDTarget::speedButtonChanged()
{
  if (speedButton_->get_active())
  {
    speedSpinButton_->set_sensitive(false);
  }
  else
  {
    speedSpinButton_->set_sensitive(true);
  }
}

void RecordCDTarget::speedChanged()
{
//FIXME: get max burn speed from selected burner(s)
  int new_speed;

  new_speed = speedSpinButton_->get_value_as_int();

  if ((new_speed % 2) == 1)
  {
    if (new_speed > 2)
    {
      if (new_speed > speed_)
      {
        new_speed = new_speed + 1;
      }
      else
      {
        new_speed = new_speed - 1;
      }
    }
    speedSpinButton_->set_value(new_speed);
  }
  speed_ = new_speed;
}

