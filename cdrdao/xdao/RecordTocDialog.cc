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

#include "RecordTocDialog.h"
#include "RecordTocSource.h"
#include "RecordCDSource.h"
#include "RecordCDTarget.h"
#include "RecordHDTarget.h"
#include "guiUpdate.h"
#include "DeviceList.h"
#include "MessageBox.h"
#include "TocEdit.h"

RecordTocDialog::RecordTocDialog(TocEdit *tocEdit)
{
  tocEdit_ = tocEdit;

  Gtk::VBox *vbox = new Gtk::VBox;
  vbox->set_border_width(10);
  vbox->set_spacing(10);
  vbox->show();
  Gtk::HBox *hbox = new Gtk::HBox;
  hbox->set_spacing(10);
  hbox->show();
  vbox->pack_start(*hbox, false, false);
  add(*vbox);

  active_ = 0;

  TocSource = new RecordTocSource(tocEdit_);
  TocSource->show();
  CDTarget = new RecordCDTarget(this);
  CDTarget->show();

  hbox->pack_start(*TocSource);
  TocSource->show();
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
  button->signal_clicked().connect(slot(*this, &RecordTocDialog::startAction));
  pixmap->show();
  startLabel->show();
  startBox->show();
  button->show();

  hbox->pack_start(*button, true, false);

  Gtk::HBox *hbox2 = new Gtk::HBox;
  hbox2->show();
  hbox2->pack_start(*hbox, true, false);
  vbox->pack_start(*hbox2, true, false);

  CdDevice::signal_statusChanged.connect(slot(*this, &RecordTocDialog::devicesStatusChanged));
}

RecordTocDialog::~RecordTocDialog()
{
  delete TocSource;
  delete CDTarget;
}

void RecordTocDialog::start(Gtk::Window *parent)
{
  active_ = 1;

  TocSource->show();
  CDTarget->show();

  update(UPD_ALL);

  set_transient_for(*parent);

  show();
}

void RecordTocDialog::stop()
{
  if (active_) {
    hide();
    active_ = 0;
  }
}

void RecordTocDialog::update(unsigned long level)
{
  if (!active_)
    return;

  std::string title;
  title += "Record project ";
  title += tocEdit_->filename();
  title += " to CD";
  set_title(title);

  TocSource->update(level);
}

void RecordTocDialog::devicesStatusChanged()
{
  CDTarget->getDeviceList()->selectOne();
}

gint RecordTocDialog::delete_event_impl(GdkEventAny*)
{
  stop();
  return 1;
}

void RecordTocDialog::startAction()
{
  if (tocEdit_ == NULL)
    return;

  DeviceList *targetList = CDTarget->getDeviceList();
 
  if (targetList->selectionEmpty()) {
    Gtk::MessageDialog(*this, "Please select at least one recorder device", Gtk::MESSAGE_INFO).run();
    return;
  }

  Toc *toc = tocEdit_->toc();

  if (toc->nofTracks() == 0 || toc->length().lba() < 300) {
    Gtk::MessageDialog(*this, "Current toc contains no tracks or length of at least one track is < 4 seconds", Gtk::MESSAGE_ERROR).run();
    return;
  }

  switch (toc->checkCdTextData()) {
    case 0: // OK
      break;
    case 1: // warning
      {
        Glib::ustring s = "Inconsistencies were detected in the defined CD-TEXT data ";
        s += "which may produce undefined results. See the console ";
        s += "output for more details.";

        Ask2Box msg(this, "CD-TEXT Inconsistency", false,
		    "Ignore CD-TEXT inconsistency?", s);

	    switch (msg.run())
        {
          case Gtk::RESPONSE_YES:
            break;
          case Gtk::RESPONSE_NO:
            return;
            break;
          default:
            cout << "invalid response" << endl;
            return;
            break;
        }
      }
      break;
    default: // error
      {
        ErrorMessageBox msg(this, "CD-TEXT Error", false, 
	  	     "The defined CD-TEXT data is erroneous or incomplete.",
		       "See the console output for more details.");
        msg.run();
        return;
      }
      break;
  }

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

  std::list<CdDevice *> devices = targetList->getAll(true);

  for (std::list<CdDevice *>::iterator i = devices.begin();
         i != devices.end(); i++)
  {
    CdDevice *writeDevice = *i;

    if (writeDevice->recordDao(tocEdit_, simulate, multiSession,
			     burnSpeed, eject, reload, buffer) != 0)
      Gtk::MessageDialog(*this, "Cannot start disk-at-once recording", Gtk::MESSAGE_ERROR).run();
  }

  stop();
}

