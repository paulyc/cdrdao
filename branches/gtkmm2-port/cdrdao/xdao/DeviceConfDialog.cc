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
#include <ctype.h>

#include <libgnomeuimm.h>

#include "util.h"

#include "CdDevice.h"
#include "DeviceList.h"
#include "guiUpdate.h"

#include "DeviceConfDialog.h"

#define MAX_DEVICE_TYPE_ID 2

static CdDevice::DeviceType ID2DEVICE_TYPE[MAX_DEVICE_TYPE_ID + 1] = {
  CdDevice::CD_ROM,
  CdDevice::CD_R,
  CdDevice::CD_RW
};
    

DeviceConfDialog::DeviceConfDialog()
{
  int i;
  Gtk::Label *label;
  Gtk::Table *table;
  Gtk::HBox *hbox;
  Gtk::VBox *vbox;
  Gtk::Button *button;

  selectedDevice_ = 0;
  addDeviceDialog_ = 0;

  set_title("Configure Devices");
  set_default_size(0, 400);

  devices_ = new DeviceList(CdDevice::CD_ALL);

  devices_->signal_changed.connect(SigC::slot(*this,&DeviceConfDialog::selectRow));

  Gtk::Menu *dmenu = manage(new Gtk::Menu);
  Gtk::MenuItem *mi;

  for (i = 0; i <= CdDevice::maxDriverId(); i++) {
    mi = manage(new Gtk::MenuItem(CdDevice::driverName(i)));
    mi->signal_activate().connect(bind(slot(*this, &DeviceConfDialog::setDriverId), i));
    mi->show();
    dmenu->append(*mi);
  }

  driverMenu_ = new Gtk::OptionMenu;
  driverMenu_->set_menu(*dmenu);

  Gtk::Menu *tmenu = manage(new Gtk::Menu);

  for (i = 0; i <= MAX_DEVICE_TYPE_ID; i++) {
    mi = manage(new Gtk::MenuItem(CdDevice::deviceType2string(ID2DEVICE_TYPE[i])));
    mi->signal_activate().connect(bind(slot(*this, &DeviceConfDialog::setDeviceType), i));
    mi->show();
    tmenu->append(*mi);
  }

  devtypeMenu_ = new Gtk::OptionMenu;
  devtypeMenu_->set_menu(*tmenu);

  specialDeviceEntry_ = new Gtk::Entry;
  specialDeviceEntry_->signal_activate().connect(SigC::slot(*this,&DeviceConfDialog::applyAction));
  driverOptionsEntry_ = new Gtk::Entry;

  Gtk::VBox *contents = new Gtk::VBox;
  contents->set_spacing(20);

  // device list
  Gtk::VBox *listBox = new Gtk::VBox;
  listBox->set_spacing(5);

  devices_->show();

  listBox->pack_start(*devices_, TRUE, TRUE);

  Gtk::ButtonBox *bbox = new Gtk::HButtonBox(Gtk::BUTTONBOX_SPREAD);

  button = new Gtk::Button(Gtk::Stock::REFRESH);
  bbox->pack_start(*button);
  button->show();
  button->signal_clicked().connect(SigC::slot(*this,&DeviceConfDialog::rescanAction));

  button = new Gtk::Button(Gtk::Stock::ADD);
  bbox->pack_start(*button);
  button->show();
  button->signal_clicked().connect(slot(*this, &DeviceConfDialog::addDevice));

  button = new Gtk::Button(Gtk::Stock::REMOVE);
  bbox->pack_start(*button);
  button->show();
  button->signal_clicked().connect(SigC::slot(*this,&DeviceConfDialog::deleteDeviceAction));

  listBox->pack_start(*bbox, FALSE);
  bbox->show();

  listBox->show();
  contents->pack_start(*listBox, TRUE, TRUE);

  // device settings
  vbox = new Gtk::VBox;

  Gtk::Label *infoLabel = new Gtk::Label("<b><big>Device settings</big></b>");
  infoLabel->set_alignment(0, 1);
  infoLabel->set_use_markup(true);
  vbox->pack_start(*infoLabel, false, false);
  infoLabel->show();

  table = new Gtk::Table(2, 4, FALSE);
  table->set_row_spacings(5);
  table->set_col_spacings(5);
  vbox->pack_start(*table, FALSE, FALSE, 5);
  vbox->show();
  table->show();
  
  label = new Gtk::Label("Device Type:");
  hbox = new Gtk::HBox;
  hbox->pack_start(*label, FALSE, FALSE);
  table->attach(*hbox, 0, 1, 0, 1);
  label->show();
  hbox->show();
  hbox = new Gtk::HBox;
  hbox->pack_start(*devtypeMenu_, true, true);
  table->attach(*hbox, 1, 2, 0, 1);
  devtypeMenu_->show();
  hbox->show();

  label = new Gtk::Label("Driver:");
  hbox = new Gtk::HBox;
  hbox->pack_start(*label, FALSE, FALSE);
  table->attach(*hbox, 0, 1, 1, 2);
  label->show();
  hbox->show();
  hbox = new Gtk::HBox;
  hbox->pack_start(*driverMenu_, true, true);
  table->attach(*hbox, 1, 2, 1, 2);
  driverMenu_->show();
  hbox->show();

  label = new Gtk::Label("Driver Options:");
  hbox = new Gtk::HBox;
  hbox->pack_start(*label, FALSE, FALSE);
  table->attach(*hbox, 0, 1, 2, 3);
  label->show();
  hbox->show();
  hbox = new Gtk::HBox;
  hbox->pack_start(*driverOptionsEntry_, true, true);
  table->attach(*hbox, 1, 2, 2, 3);
  driverOptionsEntry_->show();
  hbox->show();

  label = new Gtk::Label("Device Node:");
  hbox = new Gtk::HBox;
  hbox->pack_start(*label, FALSE, FALSE);
  table->attach(*hbox, 0, 1, 3, 4);
  label->show();
  hbox->show();
  hbox = new Gtk::HBox;
  hbox->pack_start(*specialDeviceEntry_, true, true);
  table->attach(*hbox, 1, 2, 3, 4);
  specialDeviceEntry_->show();
  hbox->show();

//FIXME: Add a "Restore defaults for this device" button here.

  contents->pack_start(*vbox, FALSE, FALSE);

  Gtk::HBox *contentsHBox = new Gtk::HBox;

  contentsHBox->pack_start(*contents, TRUE, TRUE, 10);
  contents->show();

  get_vbox()->pack_start(*contentsHBox, TRUE, TRUE, 10);
  contentsHBox->show();

  get_vbox()->show();

  bbox = new Gtk::HButtonBox(Gtk::BUTTONBOX_SPREAD);

  Gtk::Button *cancelButton = new Gtk::Button(Gtk::Stock::CLOSE);
  bbox->pack_start(*cancelButton);
  cancelButton->show();
  cancelButton->signal_clicked().connect(SigC::slot(*this,&DeviceConfDialog::hide));

  signal_hide().connect(SigC::slot(*this,&DeviceConfDialog::applyAction));

  get_action_area()->pack_start(*bbox);
  bbox->show();
  get_action_area()->show();
}

DeviceConfDialog::~DeviceConfDialog()
{
  delete devices_;
  devices_ = NULL;
}

void DeviceConfDialog::addDevice()
{
  if (addDeviceDialog_ == 0)
  {
    addDeviceDialog_ = new Gtk::Dialog("Add device", *this, false, false);

    Gtk::Button *button = addDeviceDialog_->add_button(Gtk::StockID(Gtk::Stock::ADD), Gtk::RESPONSE_OK);
    button->signal_clicked().connect(slot(*this, &DeviceConfDialog::addDeviceAction));
    button = addDeviceDialog_->add_button(Gtk::StockID(Gtk::Stock::CLOSE), Gtk::RESPONSE_CLOSE);
    button->signal_clicked().connect(slot(*addDeviceDialog_, &Gtk::Widget::hide));

    Gtk::VBox *vbox = new Gtk::VBox;
    vbox->set_border_width(10);
    vbox->set_spacing(5);

    Gtk::Adjustment *adjust = new Gtk::Adjustment(0.0, 0.0, 16.0);
    busEntry_ = new Gtk::SpinButton(*adjust, 1.0, 0);
    busEntry_->set_numeric(true);

    adjust = new Gtk::Adjustment(0.0, 0.0, 15.0);
    idEntry_ = new Gtk::SpinButton(*adjust, 1.0, 0);
    idEntry_->set_numeric(true);

    adjust = new Gtk::Adjustment(0.0, 0.0, 8.0);
    lunEntry_ = new Gtk::SpinButton(*adjust, 1.0, 0);
    lunEntry_->set_numeric(true);

    vendorEntry_ = new Gtk::Entry();
    vendorEntry_->set_max_length(8);
    productEntry_ = new Gtk::Entry();
    productEntry_->set_max_length(16);

    // add device
    Gtk::Label *infoLabel = new Gtk::Label("<b><big>Add device</big></b>");
    infoLabel->set_alignment(0, 1);
    infoLabel->set_use_markup(true);
    vbox->pack_start(*infoLabel, false, false);
    infoLabel->show();

    Gtk::VBox *addDeviceBox = new Gtk::VBox;
    addDeviceBox->set_spacing(5);

    Gtk::HBox *hbox = new Gtk::HBox;
    hbox->set_spacing(5);

    Gtk::Label *label = new Gtk::Label("Bus:");
    hbox->pack_start(*label, FALSE);
    hbox->pack_start(*busEntry_, FALSE);
    label->show();
    busEntry_->show();

    label = new Gtk::Label("Id:");
    hbox->pack_start(*label, FALSE);
    hbox->pack_start(*idEntry_, FALSE);
    label->show();
    idEntry_->show();

    label = new Gtk::Label("Lun:");
    hbox->pack_start(*label, FALSE);
    hbox->pack_start(*lunEntry_, FALSE);
    label->show();
    lunEntry_->show();

    addDeviceBox->pack_start(*hbox, FALSE);
    hbox->show();

    Gtk::Table *table = new Gtk::Table(2, 2, FALSE);
    table->set_row_spacings(5);
    table->set_col_spacings(5);
    addDeviceBox->pack_start(*table, FALSE);
    table->show();

    label = new Gtk::Label("Vendor:");
    hbox = new Gtk::HBox;
    hbox->pack_start(*label, FALSE, FALSE);
    table->attach(*hbox, 0, 1, 0, 1);
    label->show();
    hbox->show();
    hbox = new Gtk::HBox;
    hbox->pack_start(*vendorEntry_, TRUE, TRUE);
    table->attach(*hbox, 1, 2, 0, 1);
    vendorEntry_->show();
    hbox->show();

    label = new Gtk::Label("Product:");
    hbox = new Gtk::HBox;
    hbox->pack_start(*label, FALSE, FALSE);
    table->attach(*hbox, 0, 1, 1, 2);
    label->show();
    hbox->show();
    hbox = new Gtk::HBox;
    hbox->pack_start(*productEntry_, TRUE, TRUE);
    table->attach(*hbox, 1, 2, 1, 2);
    productEntry_->show();
    hbox->show();

    hbox = new Gtk::HBox;
    hbox->pack_start(*addDeviceBox, FALSE, FALSE, 5);
    addDeviceBox->show();
    vbox->pack_start(*hbox, FALSE, FALSE, 5);
    vbox->show();
    hbox->show();
    addDeviceDialog_->get_vbox()->pack_start(*vbox, false, false);
  }

  addDeviceDialog_->run();
}

void DeviceConfDialog::applyAction()
{
  if (selectedDevice_)
    exportConfiguration(selectedDevice_);
}

void DeviceConfDialog::addDeviceAction()
{
  const char *s;

  std::string vendor;
  std::string product;
  int bus = busEntry_->get_value_as_int();
  int id = idEntry_->get_value_as_int();
  int lun = lunEntry_->get_value_as_int();
  CdDevice *dev;

  if ((s = checkString(vendorEntry_->get_text())) == NULL)
    return;
  vendor = s;

  if ((s = checkString(productEntry_->get_text())) == NULL)
    return;
  product = s;

  dev = CdDevice::find(bus, id, lun);
  if (dev != NULL)
  {
    string info = string(dev->vendor()) + " " + string(dev->product()) + " is already configured at ";
	info += busEntry_->get_text() + "," + idEntry_->get_text() + "," + lunEntry_->get_text();
    Gtk::MessageDialog(*this, info, Gtk::MESSAGE_INFO).run();
    return;
  }

  dev = CdDevice::add(bus, id, lun, vendor.c_str(), product.c_str());

  if (dev)
    dev->manuallyConfigured(1);

  addDeviceDialog_->hide();
}

void DeviceConfDialog::deleteDeviceAction()
{
  CdDevice *dev;

  if (selectedDevice_) {
    dev = selectedDevice_;
    if (dev->status() == CdDevice::DEV_RECORDING ||
	    dev->status() == CdDevice::DEV_BLANKING) {
      // don't remove device that is currently busy
      Gtk::MessageDialog(*this, "This device is in use", Gtk::MESSAGE_ERROR).run();
      return;
    }

    CdDevice::remove(dev->bus(), dev->id(), dev->lun());
    selectedDevice_ = 0;
  }
}

void DeviceConfDialog::rescanAction()
{
  CdDevice::scan();
}

void DeviceConfDialog::importConfiguration(CdDevice *data)
{
  char buf[50];

  if (data != 0) {
    driverMenu_->set_sensitive(true);
    driverMenu_->set_history(data->driverId());

    devtypeMenu_->set_sensitive(true);
    devtypeMenu_->set_history(data->deviceType());

    driverOptionsEntry_->set_sensitive(true);
    sprintf(buf, "0x%lx", data->driverOptions());
    driverOptionsEntry_->set_text(buf);

    specialDeviceEntry_->set_sensitive(true);
    if (data->specialDevice())
      specialDeviceEntry_->set_text(data->specialDevice());
  }
  else {
    driverMenu_->set_history(0);
    driverMenu_->set_sensitive(false);

    devtypeMenu_->set_history(0);
    devtypeMenu_->set_sensitive(false);

    driverOptionsEntry_->set_text("");
    driverOptionsEntry_->set_sensitive(false);

    specialDeviceEntry_->set_text("");
    specialDeviceEntry_->set_sensitive(false);
  }
}

void DeviceConfDialog::exportConfiguration(CdDevice *device)
{
  const char *s, *t;

  if (device) {
	if (strtoul(driverOptionsEntry_->get_text().c_str(), NULL, 0) != device->driverOptions())
    {
      device->driverOptions(strtoul(driverOptionsEntry_->get_text().c_str(), NULL, 0));
      device->manuallyConfigured(1);
    }

    s = checkString(specialDeviceEntry_->get_text());
    t = device->specialDevice();

    if (t == NULL)
    {
      if (s != NULL)
      {
        device->specialDevice(s);
        device->manuallyConfigured(1);
      }
    }
	else
    {
	  if (s == NULL)
        device->specialDevice("");
      else
	    device->specialDevice(s);

      device->manuallyConfigured(1);
    }
  }
}

void DeviceConfDialog::setDriverId(int id)
{
  if (selectedDevice_ && id >= 0 && id <= CdDevice::maxDriverId())
  {
    selectedDevice_->driverId(id);
    selectedDevice_->manuallyConfigured(1);
  }
}

void DeviceConfDialog::setDeviceType(int id)
{
  if (selectedDevice_ && id >= 0 && id <= CdDevice::maxDriverId())
  {
    selectedDevice_->deviceType(ID2DEVICE_TYPE[id]);
    selectedDevice_->manuallyConfigured(1);
  }
}

void DeviceConfDialog::selectRow()
{
  selectedDevice_ = devices_->getFirstSelected();
  importConfiguration(selectedDevice_);
}

void DeviceConfDialog::unselectRow(gint row, gint column, GdkEvent *event)
{
  if (selectedDevice_)
    exportConfiguration(selectedDevice_);

  selectedDevice_ = 0;

  importConfiguration(NULL);
}

const char *DeviceConfDialog::checkString(const std::string &str)
{
  static char *buf = NULL;
  static long bufLen = 0;
  char *p, *s;
  long len = strlen(str.c_str());

  if (len == 0)
    return NULL;

  if (buf == NULL || len + 1 > bufLen) {
    delete[] buf;
    bufLen = len + 1;
    buf = new char[bufLen];
  }

  strcpy(buf, str.c_str());

  s = buf;
  p = buf + len - 1;

  while (*s != 0 && isspace(*s))
    s++;

  if (*s == 0)
    return NULL;

  while (p > s && isspace(*p)) {
    *p = 0;
    p--;
  }
  
  return s;
}
