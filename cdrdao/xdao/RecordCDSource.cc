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
#include <unistd.h>

#include <libgnomeuimm.h>

#include "RecordCDSource.h"
#include "MessageBox.h"
#include "xcdrdao.h"
#include "Settings.h"

#include "CdDevice.h"
#include "guiUpdate.h"

#include "util.h"

#include "DeviceList.h"

#define MAX_CORRECTION_ID 3

static RecordCDSource::CorrectionTable CORRECTION_TABLE[MAX_CORRECTION_ID + 1] = {
  { 3, "Jitter + scratch" },
  { 2, "Jitter + checks" },
  { 1, "Jitter correction" },
  { 0, "No checking" }
};


RecordCDSource::RecordCDSource(Gtk::Window *parent)
{
  parent_ = parent;
  correction_ = 0;
  speed_ = 1;
  moreSourceOptions_ = 0;
  set_spacing(10);

  DEVICES = new DeviceList(CdDevice::CD_ROM);
  pack_start(*DEVICES, false, false);

  // device settings
  Gtk::VBox *vbox = new Gtk::VBox;
  vbox->set_border_width(5);
  vbox->set_spacing(5);

  optionsLabel_ = new Gtk::Label("<b><big>Read options</big></b>");
  optionsLabel_->set_alignment(0, 1);
  optionsLabel_->set_use_markup(true);
  vbox->pack_start(*optionsLabel_, false, false);
  optionsLabel_->show();

  onTheFlyButton_ = new Gtk::CheckButton("Copy to disk before burning", 0);
  onTheFlyButton_->set_active(true);
  onTheFlyButton_->show();
  vbox->pack_start(*onTheFlyButton_);

  Gtk::HBox *hbox = new Gtk::HBox;
//  hbox->show();
  Gtk::Label *label = new Gtk::Label("Speed: ", 0);
  label->show();
  hbox->pack_start(*label, false, false);

  Gtk::Adjustment *adjustment = new Gtk::Adjustment(1, 1, 50);
  speedSpinButton_ = new Gtk::SpinButton(*adjustment);
  speedSpinButton_->set_digits(0);
  speedSpinButton_->show();
  speedSpinButton_->set_sensitive(false);
  adjustment->signal_value_changed().connect(SigC::slot(*this, &RecordCDSource::speedChanged));
  hbox->pack_start(*speedSpinButton_, false, false, 10);

  speedButton_ = new Gtk::CheckButton("Use max.", 0);
  speedButton_->set_active(true);
  speedButton_->show();
  speedButton_->signal_toggled().connect(SigC::slot(*this, &RecordCDSource::speedButtonChanged));
  hbox->pack_start(*speedButton_, true, true);
  vbox->pack_start(*hbox);

  pack_start(*vbox, false, false);
  vbox->show();
}

RecordCDSource::~RecordCDSource()
{
  if (moreSourceOptions_)
    delete moreSourceOptions_;
}

Gtk::VBox *RecordCDSource::moreOptions()
{
  if (!moreSourceOptions_)
  {
    Gtk::HBox *hbox;
    Gtk::Label *label;

    moreSourceOptions_ = new Gtk::VBox();

    label = new Gtk::Label("<b><big>Read options</big></b>");
    label->set_alignment(0, 1);
    label->set_use_markup(true);
    moreSourceOptions_->pack_start(*label, false, false);
    label->show();
    moreSourceOptions_->set_border_width(10);
    moreSourceOptions_->set_spacing(5);

    continueOnErrorButton_ = new Gtk::CheckButton("Continue if errors found", 0);
    continueOnErrorButton_->set_active(false);
//    continueOnErrorButton_->show();
    moreSourceOptions_->pack_start(*continueOnErrorButton_);

    ignoreIncorrectTOCButton_ = new Gtk::CheckButton("Ignore incorrect TOC", 0);
    ignoreIncorrectTOCButton_->set_active(false);
//    ignoreIncorrectTOCButton_->show();
    moreSourceOptions_->pack_start(*ignoreIncorrectTOCButton_);

    Gtk::Menu *menuCorrection = manage(new Gtk::Menu);
    Gtk::MenuItem *miCorr;
	
    for (int i = 0; i <= MAX_CORRECTION_ID; i++) {
      miCorr = manage(new Gtk::MenuItem(CORRECTION_TABLE[i].name));
      miCorr->signal_activate().connect(bind(slot(*this, &RecordCDSource::setCorrection), i));
      miCorr->show();
      menuCorrection->append(*miCorr);
    }
  
    correctionMenu_ = new Gtk::OptionMenu;
    correctionMenu_->set_menu(*menuCorrection);
  
    correctionMenu_->set_history(correction_);
  
    hbox = new Gtk::HBox;
    label = new Gtk::Label("Correction Method: ");
    hbox->pack_start(*label, FALSE);
    label->show();
    hbox->pack_start(*correctionMenu_, FALSE);
    correctionMenu_->show();
    hbox->show();
    moreSourceOptions_->pack_start(*hbox, false, false);
  }

  moreSourceOptions_->show();

  return moreSourceOptions_; 
}

void RecordCDSource::setCorrection(int s)
{
  if (s >= 0 && s <= MAX_CORRECTION_ID)
    correction_ = s;
}

bool RecordCDSource::getOnTheFly()
{
  return onTheFlyButton_->get_active() ? 0 : 1;
}

void RecordCDSource::setOnTheFly(bool active)
{
  onTheFlyButton_->set_active(!active);
}

int RecordCDSource::getCorrection()
{
  return CORRECTION_TABLE[correction_].correction;
}

void RecordCDSource::speedButtonChanged()
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

void RecordCDSource::speedChanged()
{
  //Do some validating here. speed_ <= MAX read speed of CD.
  speed_ = speedSpinButton_->get_value_as_int();
}

void RecordCDSource::onTheFlyOption(bool visible)
{
  if (visible)
  {
    optionsLabel_->show();
    onTheFlyButton_->show();
  }
  else
  {
    optionsLabel_->hide();
    onTheFlyButton_->hide();
  }
}

