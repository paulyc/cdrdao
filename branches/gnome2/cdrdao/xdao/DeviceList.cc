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

#include <gtkmm.h>
#include <gnome.h>

#include "DeviceList.h"
#include "MessageBox.h"
#include "xcdrdao.h"
#include "Settings.h"

#include "CdDevice.h"
#include "guiUpdate.h"
#include "TocEdit.h"

#include "util.h"

DeviceList::DeviceList(CdDevice::DeviceType filterType)
{
  Gtk::HBox *hbox;

  filterType_ = filterType;

  listModel_ = Gtk::ListStore::create(listColumns_);
  list_.set_model(listModel_);
  list_.append_column("Vendor", listColumns_.vendor);
  list_.append_column("Model", listColumns_.model);
  list_.append_column("Status", listColumns_.status);

  Gtk::VBox *contents = new Gtk::VBox;
  contents->set_spacing(10);

  // available device list
  Gtk::HBox *listHBox = new Gtk::HBox;
  Gtk::VBox *listVBox = new Gtk::VBox;

  hbox = new Gtk::HBox;
  hbox->pack_start(list_, TRUE, TRUE);

  Gtk::Adjustment *adjust = new Gtk::Adjustment(0.0, 0.0, 0.0);

  Gtk::VScrollbar *scrollBar = new Gtk::VScrollbar(*adjust);
  hbox->pack_start(*scrollBar, FALSE, FALSE);

  list_.set_vadjustment(*adjust);

  listHBox->pack_start(*hbox, TRUE, TRUE, 5);
  listVBox->pack_start(*listHBox, TRUE, TRUE, 5);

  switch (filterType_) {
  case CdDevice::CD_ROM:
    set_label(" Available Reader Devices ");
    break;
  case CdDevice::CD_R:
    set_label(" Available Recorder Devices ");
    break;
  case CdDevice::CD_RW:
    set_label(" Available Recorder (RW) Devices ");
    break;
  }

  add(*listVBox);
}

DeviceList::~DeviceList()
{
  DeviceData *data;

  Gtk::TreeNodeChildren ch = listModel_->children();
  for (int i = 0; i < ch.size(); i++) {
    Gtk::TreeRow row = ch[i];
    data = row[listColumns_.data];
    if (data)
      delete data;
  }
}

DeviceList::DeviceData* DeviceList::selection()
{
  Gtk::TreeIter i = list_.get_selection()->get_selected();

  if (i)
    return (*i)[listColumns_.data];
  else
    return NULL;
}

void DeviceList::appendTableEntry(CdDevice *dev)
{
  DeviceData *data;

  data = new DeviceData;
  data->bus = dev->bus();
  data->id = dev->id();
  data->lun = dev->lun();

  Gtk::TreeIter newiter = listModel_->append();
  Gtk::TreeModel::Row row = *newiter;
  row[listColumns_.bus] = data->bus;
  row[listColumns_.id] = data->id;
  row[listColumns_.lun] = data->lun;
  row[listColumns_.vendor] = dev->vendor();
  row[listColumns_.model] = dev->product();
  row[listColumns_.status] = CdDevice::status2string(dev->status());
  row[listColumns_.data] = data;

  if (dev->status() == CdDevice::DEV_READY)
    list_.get_selection()->select(newiter);
}

void DeviceList::import()
{
  CdDevice *drun;
  DeviceData *data;
  unsigned int i;

  Gtk::TreeNodeChildren ch = listModel_->children();
  for (int i = 0; i < ch.size(); i++) {
    Gtk::TreeRow row = ch[i];
    data = row[listColumns_.data];
    if (data)
      delete data;
  }

  listModel_->clear();

  for (drun = CdDevice::first(); drun != NULL; drun = CdDevice::next(drun)) {
    switch (filterType_) {
    case CdDevice::CD_ROM:
      if (drun->driverId() > 0 &&
          (drun->deviceType() == CdDevice::CD_ROM ||
           drun->deviceType() == CdDevice::CD_R ||
           drun->deviceType() == CdDevice::CD_RW)) {
        appendTableEntry(drun);
      }
      break;
    case CdDevice::CD_R:
      if (drun->driverId() > 0 &&
          (drun->deviceType() == CdDevice::CD_R ||
           drun->deviceType() == CdDevice::CD_RW)) {
        appendTableEntry(drun);
      }
      break;
    case CdDevice::CD_RW:
      if (drun->driverId() > 0 &&
          (drun->deviceType() == CdDevice::CD_RW)) {
        appendTableEntry(drun);
      }
      break;
    }
  }

  if (listModel_->children().size() > 0) {
    list_.columns_autosize();
    list_.get_selection()->select(Gtk::TreeModel::Path((unsigned)1));
  }
}

void DeviceList::importStatus()
{
  DeviceData *data;
  CdDevice *dev;

  Gtk::TreeNodeChildren ch = listModel_->children();
  for (int i = 0; i < ch.size(); i++) {
    Gtk::TreeRow row = ch[i];
    data = row[listColumns_.data];

    if (data && (dev = CdDevice::find(data->bus, data->id, data->lun))) {
      if (dev->status() == CdDevice::DEV_READY)
        list_.get_column(i)->set_clickable(true);
      else
        list_.get_column(i)->set_clickable(false);

      row[listColumns_.status] = CdDevice::status2string(dev->status());
    }
  }

  list_.columns_autosize();
}

void DeviceList::selectOne()
{
  for (int i = 0; i < listModel_->children().size(); i++) {
    list_.get_selection()->select(Gtk::TreePath((unsigned)1, i));
    if (list_.get_selection()->count_selected_rows() > 0)
      break;
  }
}

void DeviceList::selectOneBut(DeviceList::DeviceData* targetData)
{
  if (list_.get_selection()->count_selected_rows() == 0) {

    Gtk::TreeNodeChildren ch = listModel_->children();

    for (int i = 0; i < ch.size(); i++) {

      DeviceList::DeviceData *sourceData = (ch[i])[listColumns_.data];

      if ((targetData->bus == sourceData->bus)
          && (targetData->id == sourceData->id)
          && (targetData->lun == sourceData->lun)) {

        list_.get_selection()->select(ch[i]);
        break;
      }
    }

    if (list_.get_selection()->count_selected_rows() == 0) {
      selectOne();
    }
  }
}
