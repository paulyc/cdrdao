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

#include <libgnomeuimm.h>

#include "guiUpdate.h"
#include "DeviceList.h"
#include "MessageBox.h"
#include "BlankCDDialog.h"
#include "Settings.h"


BlankCDDialog::BlankCDDialog()
{
  Gtk::VBox *vbox = new Gtk::VBox;
  vbox->set_border_width(10);
  vbox->set_spacing(10);
  add(*vbox);

  moreOptionsDialog_ = 0;
  speed_ = 1;

  Devices = new DeviceList(CdDevice::CD_RW);
  vbox->pack_start(*Devices, true, true);

  // device settings
  Gtk::VBox *vbox2 = new Gtk::VBox;
  vbox2->set_border_width(5);
  vbox2->set_spacing(5);

  Gtk::Label *optionsLabel = new Gtk::Label("<b><big>Blank options</big></b>");
  optionsLabel->set_alignment(0, 1);
  optionsLabel->set_use_markup(true);
  vbox2->pack_start(*optionsLabel, false, false);
  optionsLabel->show();

  Gtk::RadioButton_Helpers::Group group;
  fastBlank_rb = new Gtk::RadioButton(group, "Fast Blank - does not erase contents", 0);
  fullBlank_rb = new Gtk::RadioButton(group, "Full Blank - erases contents, slower", 0);

  vbox2->pack_start(*fastBlank_rb);
  fastBlank_rb->show();
  vbox2->pack_start(*fullBlank_rb);
  fullBlank_rb->show();

  Gtk::Image *moreOptionsPixmap = manage(new Gtk::Image(Gtk::StockID(Gtk::Stock::PROPERTIES), Gtk::ICON_SIZE_SMALL_TOOLBAR));
  Gtk::Label *moreOptionsLabel = manage(new Gtk::Label("More Options"));
  Gtk::HBox *moreOptionsBox = manage(new Gtk::HBox);
  moreOptionsBox->set_border_width(2);
  Gtk::Button *moreOptionsButton = manage(new Gtk::Button());
  moreOptionsBox->pack_start(*moreOptionsPixmap, false, false, 3);
  moreOptionsBox->pack_start(*moreOptionsLabel, false, false, 4);
  moreOptionsButton->add(*moreOptionsBox);
  moreOptionsButton->signal_clicked().connect(slot(*this, &BlankCDDialog::moreOptions));
  moreOptionsPixmap->show();
  moreOptionsLabel->show();
  moreOptionsBox->show();
  moreOptionsButton->show();
  moreOptionsBox = manage(new Gtk::HBox);
  moreOptionsBox->show();
  vbox2->pack_start(*moreOptionsBox, false, false);
  moreOptionsBox->pack_start(*moreOptionsButton, false, false);
 
  Gtk::Image *pixmap = new Gtk::Image(PIXMAPS_DIR "/gcdmaster.png");
  Gtk::Label *startLabel = manage(new Gtk::Label("      Start      "));
  Gtk::VBox *startBox = manage(new Gtk::VBox);
  Gtk::Button *button = manage(new Gtk::Button());
  startBox->pack_start(*pixmap, false, false);
  startBox->pack_start(*startLabel, false, false);

  button->add(*startBox);
  button->signal_clicked().connect(slot(*this, &BlankCDDialog::startAction));
  pixmap->show();
  startLabel->show();
  startBox->show();
  button->show();

  Gtk::HBox *hbox2 = new Gtk::HBox;
  hbox2->show();
  hbox2->pack_start(*button, true, false);

  vbox->pack_start(*vbox2, false, false);
  vbox->pack_start(*hbox2, true, false);
  vbox2->show();
  vbox->show();

  CdDevice::signal_statusChanged.connect(slot(*this, &BlankCDDialog::devicesStatusChanged));

  set_title("Blank Rewritable CD");
}

void BlankCDDialog::moreOptions()
{
  if (!moreOptionsDialog_)
  {
    Gtk::Button *button;
    std::vector <std::string> buttons;
    moreOptionsDialog_ = new Gtk::Dialog("Blank options",
							  *this, false, false);

    button = moreOptionsDialog_->add_button(Gtk::StockID(Gtk::Stock::CLOSE), Gtk::RESPONSE_CLOSE);
    moreOptionsDialog_->set_transient_for(*this);

    button->signal_clicked().connect(slot(*moreOptionsDialog_, &Gtk::Dialog::hide));

    Gtk::VBox *vbox = moreOptionsDialog_->get_vbox();
    Gtk::Frame *frame = new Gtk::Frame("More Blank Options");
    vbox->pack_start(*frame);
    vbox = new Gtk::VBox;
    vbox->set_border_width(10);
    vbox->set_spacing(5);
    vbox->show();
    frame->add(*vbox);
    frame->show();

    ejectButton_ = new Gtk::CheckButton("Eject the CD after blanking", 0);
    ejectButton_->set_active(false);
    ejectButton_->show();
    vbox->pack_start(*ejectButton_);

    reloadButton_ = new Gtk::CheckButton("Reload the CD after writing, if necessary", 0);
    reloadButton_->set_active(false);
    reloadButton_->show();
    vbox->pack_start(*reloadButton_);

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
    adjustment->signal_value_changed().connect(SigC::slot(*this, &BlankCDDialog::speedChanged));
    hbox->pack_start(*speedSpinButton_, false, false, 10);
  
    speedButton_ = new Gtk::CheckButton("Use max.", 0);
    speedButton_->set_active(true);
    speedButton_->show();
    speedButton_->signal_toggled().connect(SigC::slot(*this, &BlankCDDialog::speedButtonChanged));
    hbox->pack_start(*speedButton_, true, true);
    vbox->pack_start(*hbox);

    signal_hide().connect(slot(*moreOptionsDialog_, &Gtk::Dialog::hide));
  }

  moreOptionsDialog_->present();
}

void BlankCDDialog::devicesStatusChanged()
{
  Devices->selectOne();
}

void BlankCDDialog::startAction()
{
  if (Devices->selectionEmpty()) {
    Gtk::MessageDialog(*this, "Please select at least one recorder device", Gtk::MESSAGE_INFO).run();
    return;
  }

  int fast;
  if (fastBlank_rb->get_active())
    fast = 1;
  else
    fast = 0;

  int eject = checkEjectWarning(this);
  int reload = checkReloadWarning(this);
  int burnSpeed = getSpeed();

  std::list<CdDevice *> devices = Devices->getAll(true);

  for (std::list<CdDevice *>::iterator i = devices.begin();
         i != devices.end(); i++)
  {
    CdDevice *writeDevice = *i;

    if (writeDevice->blank(fast, burnSpeed, eject, reload) != 0)
      Gtk::MessageDialog(*this, "Cannot start blanking", Gtk::MESSAGE_ERROR).run();
  }

  hide();
}

void BlankCDDialog::speedButtonChanged()
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

void BlankCDDialog::speedChanged()
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

bool BlankCDDialog::getEject()
{
  if (moreOptionsDialog_)
    return ejectButton_->get_active() ? 1 : 0;
  else
    return 0; 
}

int BlankCDDialog::checkEjectWarning(Gtk::Window *parent)
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

bool BlankCDDialog::getReload()
{
  if (moreOptionsDialog_)
    return reloadButton_->get_active() ? 1 : 0;
  else
    return 0; 
}

int BlankCDDialog::checkReloadWarning(Gtk::Window *parent)
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
      case GTK_RESPONSE_NO: // don't keep reload setting
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

int BlankCDDialog::getSpeed()
{
  if (moreOptionsDialog_)
  {
   if (speedButton_->get_active())
     return 0;
   else
     return speed_;
  }
  return 0;
}
