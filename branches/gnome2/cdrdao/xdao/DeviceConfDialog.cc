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

#include "DeviceConfDialog.h"

#include "CdDevice.h"
#include "guiUpdate.h"

#include "util.h"

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

  active_ = false;

  set_title("Configure Devices");

  // TreeView initialization
  listModel_ = Gtk::ListStore::create(listColumns_);
  list_.set_model(listModel_);
  list_.append_column("Bus", listColumns_.bus);
  list_.append_column("Id", listColumns_.id);
  list_.append_column("Lun", listColumns_.lun);
  list_.append_column("Vendor", listColumns_.vendor);
  list_.append_column("Model", listColumns_.model);
  list_.append_column("Status", listColumns_.status);

  selectedRow_ = list_.get_selection()->get_selected();
  list_.get_selection()->signal_changed().
      connect(slot(*this, &DeviceConfDialog::selectionChanged));

  Gtk::Menu *dmenu = manage(new Gtk::Menu);
  Gtk::MenuItem *mi;

  for (i = 0; i <= CdDevice::maxDriverId(); i++) {
    mi = manage(new Gtk::MenuItem(CdDevice::driverName(i)));
    mi->signal_activate().connect(bind(slot(*this,
                                            &DeviceConfDialog::setDriverId),
                                       i));
    mi->show();
    dmenu->append(*mi);
  }

  driverMenu_ = manage(new Gtk::OptionMenu);
  driverMenu_->set_menu(*dmenu);

  Gtk::Menu *tmenu = manage(new Gtk::Menu);

  for (i = 0; i <= MAX_DEVICE_TYPE_ID; i++) {
    mi = manage(new
                Gtk::MenuItem(CdDevice::deviceType2string(ID2DEVICE_TYPE[i])));
    mi->signal_activate().connect(bind(slot(*this,
                                            &DeviceConfDialog::setDeviceType),
                                       i));
    mi->show();
    tmenu->append(*mi);
  }

  devtypeMenu_ = manage(new Gtk::OptionMenu);
  devtypeMenu_->set_menu(*tmenu);

  Gtk::Adjustment *adjust = manage(new Gtk::Adjustment(0.0, 0.0, 16.0));
  busEntry_ = manage(new Gtk::SpinButton(*adjust, 1.0, 1));
  busEntry_->set_numeric(true);
  busEntry_->set_digits(0);

  adjust = manage(new Gtk::Adjustment(0.0, 0.0, 15.0));
  idEntry_ = manage(new Gtk::SpinButton(*adjust, 1.0, 1));
  idEntry_->set_numeric(true);
  idEntry_->set_digits(0);

  adjust = manage(new Gtk::Adjustment(0.0, 0.0, 8.0));
  lunEntry_ = manage(new Gtk::SpinButton(*adjust, 1.0, 1));
  lunEntry_->set_numeric(true);
  lunEntry_->set_digits(0);

  vendorEntry_.set_max_length(8);
  productEntry_.set_max_length(16);

  Gtk::VBox *contents = manage(new Gtk::VBox);
  contents->set_spacing(5);
  contents->set_border_width(7);

  // ---------------------------- Device list
  Gtk::VBox *listBox = manage(new Gtk::VBox);
  listBox->set_spacing(5);
  listBox->set_border_width(10);

  hbox = manage(new Gtk::HBox);

  hbox->pack_start(list_, Gtk::PACK_EXPAND_WIDGET);

  adjust = manage(new Gtk::Adjustment(0.0, 0.0, 0.0));

  Gtk::VScrollbar *scrollBar = manage(new Gtk::VScrollbar(*adjust));
  hbox->pack_start(*scrollBar, Gtk::PACK_SHRINK);

  list_.set_vadjustment(*adjust);

  listBox->pack_start(*hbox, Gtk::PACK_EXPAND_WIDGET);

  Gtk::ButtonBox *bbox = manage(new Gtk::HButtonBox(Gtk::BUTTONBOX_SPREAD));

  button = manage(new Gtk::Button("Rescan"));
  bbox->pack_start(*button);
  button->signal_clicked().connect(SigC::slot(*this,&DeviceConfDialog::rescanAction));

  button = manage(new Gtk::Button(Gtk::StockID(Gtk::Stock::DELETE)));
  bbox->pack_start(*button);
  button->signal_clicked().connect(SigC::slot(*this,&DeviceConfDialog::deleteDeviceAction));

  listBox->pack_start(*bbox, Gtk::PACK_SHRINK);

  listFrame_.set_label(" Device List ");
  listFrame_.add(*listBox);
  contents->pack_start(listFrame_, Gtk::PACK_EXPAND_WIDGET);

  // ---------------------------- Device settings

  settingFrame_.set_label(" Device Settings ");
  table = manage(new Gtk::Table(2, 4, FALSE));
  table->set_row_spacings(5);
  table->set_col_spacings(5);
  table->set_border_width(10);
  settingFrame_.add(*table);
  
  label = manage(new Gtk::Label("Device Type:"));
  table->attach(*label, 0, 1, 0, 1);
  table->attach(*devtypeMenu_, 1, 2, 0, 1);

  label = manage(new Gtk::Label("Driver:"));
  table->attach(*label, 0, 1, 1, 2);
  table->attach(*driverMenu_, 1, 2, 1, 2);

  label = manage(new Gtk::Label("Driver Options:"));
  table->attach(*label, 0, 1, 2, 3);
  table->attach(driverOptionsEntry_, 1, 2, 2, 3);

  label = manage(new Gtk::Label("Device Node:"));
  table->attach(*label, 0, 1, 3, 4);
  table->attach(specialDeviceEntry_, 1, 2, 3, 4);

  contents->pack_start(settingFrame_, Gtk::PACK_SHRINK);

  // -------------- Add device

  addDeviceFrame_.set_label(" Add Device ");
  Gtk::VBox *addDeviceBox = manage(new Gtk::VBox);
  addDeviceBox->set_spacing(5);
  addDeviceBox->set_border_width(5);

  hbox = manage(new Gtk::HBox);
  hbox->set_spacing(5);

  label = manage(new Gtk::Label("Bus:"));
  hbox->pack_start(*label, Gtk::PACK_SHRINK);
  hbox->pack_start(*busEntry_);

  label = manage(new Gtk::Label("Id:"));
  hbox->pack_start(*label, Gtk::PACK_SHRINK);
  hbox->pack_start(*idEntry_);

  label = manage(new Gtk::Label("Lun:"));
  hbox->pack_start(*label, Gtk::PACK_SHRINK);
  hbox->pack_start(*lunEntry_);

  addDeviceBox->pack_start(*hbox, Gtk::PACK_EXPAND_WIDGET);

  table = manage(new Gtk::Table(2, 2, FALSE));
  table->set_row_spacings(5);
  table->set_col_spacings(5);
  addDeviceBox->pack_start(*table, Gtk::PACK_EXPAND_WIDGET);

  label = manage(new Gtk::Label("Vendor:"));
  table->attach(*label, 0, 1, 0, 1);
  table->attach(vendorEntry_, 1, 2, 0, 1);

  label = manage(new Gtk::Label("Product:"));
  table->attach(*label, 0, 1, 1, 2);
  table->attach(productEntry_, 1, 2, 1, 2);

  bbox = manage(new Gtk::HButtonBox(Gtk::BUTTONBOX_SPREAD));
  bbox->set_spacing(5);
  Gtk::Button* addButton =
      manage(new Gtk::Button(Gtk::StockID(Gtk::Stock::ADD)));
  bbox->pack_start(*addButton);
  addButton->signal_clicked().
      connect(slot(*this, &DeviceConfDialog::addDeviceAction));
  addDeviceBox->pack_start(*bbox);

  addDeviceFrame_.add(*addDeviceBox);
  contents->pack_start(addDeviceFrame_, Gtk::PACK_SHRINK);

  // 3 buttons at bottom of window.

  bbox = manage(new Gtk::HButtonBox(Gtk::BUTTONBOX_SPREAD));
  bbox->set_spacing(5);
  hbox->set_border_width(10);
  Gtk::Button* applyButton =
    manage(new Gtk::Button(Gtk::StockID(Gtk::Stock::APPLY)));
  bbox->pack_start(*applyButton);
  applyButton->signal_clicked().connect(slot(*this,
                                             &DeviceConfDialog::applyAction));
  
  Gtk::Button *resetButton = manage(new Gtk::Button("Reset"));
  bbox->pack_start(*resetButton);
  resetButton->signal_clicked().connect(slot(*this,
                                             &DeviceConfDialog::resetAction));

  Gtk::Button *cancelButton =
    manage(new Gtk::Button(Gtk::StockID(Gtk::Stock::CLOSE)));
  bbox->pack_start(*cancelButton);
  cancelButton->signal_clicked().connect(slot(*this,
                                              &DeviceConfDialog::closeAction));

  contents->pack_start(*bbox, Gtk::PACK_SHRINK);

  add(*contents);
}

DeviceConfDialog::~DeviceConfDialog()
{
}


void DeviceConfDialog::start()
{
  if (active_) {
    present();
    return;
  }

  active_ = true;
  update(UPD_CD_DEVICES);
  show_all();
}

void DeviceConfDialog::stop()
{
  hide();
  active_ = false;
}

void DeviceConfDialog::update(unsigned long level)
{
  if (!active_)
    return;

  if (level & UPD_CD_DEVICES)
    import();
  else if (level & UPD_CD_DEVICE_STATUS)
    importStatus();
}

void DeviceConfDialog::closeAction()
{
  stop();
}

void DeviceConfDialog::resetAction()
{
  import();
}


void DeviceConfDialog::applyAction()
{
  if (selectedRow_)
    exportConfiguration(selectedRow_);
  exportData();
  guiUpdate(UPD_CD_DEVICES);
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

  if ((s = checkString(vendorEntry_.get_text())) == NULL)
    return;
  vendor = s;

  if ((s = checkString(productEntry_.get_text())) == NULL)
    return;
  product = s;

  if (CdDevice::find(bus, id, lun) != NULL) 
    return;

  dev = CdDevice::add(bus, id, lun, vendor.c_str(), product.c_str());

  Gtk::TreeIter new_entry = appendTableEntry(dev);
  list_.get_selection()->select(new_entry);

  guiUpdate(UPD_CD_DEVICES);
}

void DeviceConfDialog::deleteDeviceAction()
{
  DeviceData *data;
  CdDevice *dev;

  if (selectedRow_) {

    data = (*selectedRow_)[listColumns_.data];

    dev = CdDevice::find(data->bus, data->id, data->lun);
    if (dev == NULL || dev->status() == CdDevice::DEV_RECORDING ||
        dev->status() == CdDevice::DEV_BLANKING) {
      // don't remove device that is currently busy
      return;
    }

    CdDevice::remove(data->bus, data->id, data->lun);
    listModel_->erase(selectedRow_);
    list_.get_selection()->unselect_all();
    selectedRow_ = list_.get_selection()->get_selected();
    delete data;

    guiUpdate(UPD_CD_DEVICES);
  }
}

void DeviceConfDialog::rescanAction()
{
  CdDevice::scan();
  guiUpdate(UPD_CD_DEVICES);
}

Gtk::TreeIter DeviceConfDialog::appendTableEntry(CdDevice *dev)
{
  DeviceData *data;
  char buf[50];
  std::string idStr;
  std::string busStr;
  std::string lunStr;
  const gchar *rowStr[6];

  data = new DeviceData;
  data->bus = dev->bus();
  data->id = dev->id();
  data->lun = dev->lun();
  data->driverId = dev->driverId();
  data->options = dev->driverOptions();
  if (dev->specialDevice() != NULL)
    data->specialDevice = dev->specialDevice();

  switch (dev->deviceType()) {
  case CdDevice::CD_ROM:
    data->deviceType = 0;
    break;
  case CdDevice::CD_R:
    data->deviceType = 1;
    break;
  case CdDevice::CD_RW:
    data->deviceType = 2;
    break;
  }

  Gtk::TreeIter newiter = listModel_->append();
  Gtk::TreeModel::Row row = *newiter;
  row[listColumns_.bus] = data->bus;
  row[listColumns_.id] = data->id;
  row[listColumns_.lun] = data->lun;
  row[listColumns_.vendor] = dev->vendor();
  row[listColumns_.model] = dev->product();
  row[listColumns_.status] = CdDevice::status2string(dev->status());
  row[listColumns_.data] = data;

  return newiter;
}

void DeviceConfDialog::import()
{
  CdDevice *drun;
  DeviceData *data;

  list_.get_selection()->unselect_all();
  selectedRow_ = list_.get_selection()->get_selected();

  listModel_->clear();

  for (drun = CdDevice::first(); drun != NULL; drun = CdDevice::next(drun)) {
    appendTableEntry(drun);
  }

  if (listModel_->children().size() > 0) {
    list_.columns_autosize();
    list_.get_selection()->select(Gtk::TreeModel::Path((unsigned)1));
  }
}

void DeviceConfDialog::importConfiguration(Gtk::TreeIter row)
{
  char buf[50];
  DeviceData *data;

  if (selectedRow_) {

    data = (*selectedRow_)[listColumns_.data];
    driverMenu_->set_sensitive(true);
    driverMenu_->set_history(data->driverId);
    devtypeMenu_->set_sensitive(true);
    devtypeMenu_->set_history(data->deviceType);
    driverOptionsEntry_.set_sensitive(true);
    sprintf(buf, "0x%lx", data->options);
    driverOptionsEntry_.set_text(buf);
    specialDeviceEntry_.set_sensitive(true);
    specialDeviceEntry_.set_text(data->specialDevice);

  } else {

    driverMenu_->set_history(0);
    driverMenu_->set_sensitive(false);
    devtypeMenu_->set_history(0);
    devtypeMenu_->set_sensitive(false);
    driverOptionsEntry_.set_text("");
    driverOptionsEntry_.set_sensitive(false);
    specialDeviceEntry_.set_text("");
    specialDeviceEntry_.set_sensitive(false);
  }
}

void DeviceConfDialog::importStatus()
{
  DeviceData *data;
  CdDevice *dev;

  Gtk::TreeNodeChildren ch = listModel_->children();
  for (int i = 0; i < ch.size(); i++) {
    Gtk::TreeRow row = ch[i];
    data = row[listColumns_.data];
    if (data && (dev = CdDevice::find(data->bus, data->id, data->lun))) {
      row[listColumns_.status] = CdDevice::status2string(dev->status());
    }
  }

  list_.columns_autosize();
}

void DeviceConfDialog::exportConfiguration(Gtk::TreeIter row)
{
  DeviceData *data;
  const char *s;

  if (row) {
    data = (*row)[listColumns_.data];

    data->options = strtoul(driverOptionsEntry_.get_text().c_str(), NULL, 0);
    s = checkString(specialDeviceEntry_.get_text());

    if (s == NULL)
      data->specialDevice = "";
    else
      data->specialDevice = s;
  }
}

void DeviceConfDialog::exportData()
{
  DeviceData *data;
  CdDevice *dev;
  std::string s;

  Gtk::TreeNodeChildren ch = listModel_->children();
  for (int i = 0; i < ch.size(); i++) {
    Gtk::TreeRow row = ch[i];
    data = row[listColumns_.data];
    if (data && (dev = CdDevice::find(data->bus, data->id, data->lun))) {

      if (dev->driverId() != data->driverId) {
        dev->driverId(data->driverId);
        dev->manuallyConfigured(1);
      }

      if (dev->deviceType() != ID2DEVICE_TYPE[data->deviceType]) {
        dev->deviceType(ID2DEVICE_TYPE[data->deviceType]);
        dev->manuallyConfigured(1);
      }

      if (dev->driverOptions() != data->options) {
        dev->driverOptions(data->options);
        dev->manuallyConfigured(1);
      }

      if ((dev->specialDevice() == NULL &&
           data->specialDevice.c_str()[0] != 0) ||
          (dev->specialDevice() != NULL &&
           strcmp(dev->specialDevice(), data->specialDevice.c_str()) != 0)) {
        dev->specialDevice(data->specialDevice.c_str());
        dev->manuallyConfigured(1);
      }
    }
  }
}



void DeviceConfDialog::setDriverId(int id)
{
  DeviceData *data;

  if (selectedRow_ && id >= 0 && id <= CdDevice::maxDriverId()) {
    data = (*selectedRow_)[listColumns_.data];
    if (data)
      data->driverId = id;
  }
}

void DeviceConfDialog::setDeviceType(int id)
{
  DeviceData *data;

  if (selectedRow_ && id >= 0 && id <= CdDevice::maxDriverId()) {
    data = (*selectedRow_)[listColumns_.data];
    if (data)
      data->deviceType = id;
  }
}

void DeviceConfDialog::selectionChanged()
{
  Gtk::TreeIter new_sel = list_.get_selection()->get_selected();

  if ((bool)selectedRow_ != (bool)new_sel || selectedRow_ != new_sel) {

    if (selectedRow_)
      exportConfiguration(selectedRow_);

    selectedRow_ = new_sel;
    importConfiguration(selectedRow_);
  }
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
