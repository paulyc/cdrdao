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

#include <libgnomeuimm.h>

#include "DeviceList.h"
#include "MessageBox.h"
#include "xcdrdao.h"
#include "Settings.h"

#include "CdDevice.h"
#include "TocEdit.h"

#include "util.h"

DeviceList::DeviceList(CdDevice::DeviceType filterType)
{
  Gtk::Label *infoLabel = new Gtk::Label();
  infoLabel->set_alignment(0, 1);
  infoLabel->set_use_markup(true);
  pack_start(*infoLabel, false, false);
  infoLabel->show();

  filterType_ = filterType;

  // create the model
  listStore_ = Gtk::ListStore::create(modelColumns_);

  // create tree view
  treeView_.set_model(listStore_);
  treeView_.set_rules_hint();

  // device name
  {
    Gtk::CellRendererText* pRenderer = Gtk::manage( new Gtk::CellRendererText() );

    int cols_count = treeView_.append_column("Device", *pRenderer);
    Gtk::TreeViewColumn* pColumn = treeView_.get_column(cols_count - 1);

    pColumn->add_attribute(pRenderer->property_text(), modelColumns_.name);
  }

  // device status
  {
    Gtk::CellRendererText* pRenderer = Gtk::manage( new Gtk::CellRendererText() );

    int cols_count = treeView_.append_column("Status", *pRenderer);
    Gtk::TreeViewColumn* pColumn = treeView_.get_column(cols_count - 1);

    pColumn->add_attribute(pRenderer->property_text(), modelColumns_.status);
  }

  treeView_.set_headers_visible(true);
  treeView_.set_headers_clickable(false);
  treeView_.set_size_request(-1, 80);
  treeView_.show();

  Gtk::ScrolledWindow *scrollWindow = new Gtk::ScrolledWindow();
  pack_start(*scrollWindow, TRUE, TRUE);
  scrollWindow->add(treeView_);
  scrollWindow->set_shadow_type(Gtk::SHADOW_IN);
  scrollWindow->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  scrollWindow->show();

  switch (filterType_)
  {
    case CdDevice::CD_ROM:
                 infoLabel->set_label("<b><big>Available Reader Devices</big></b>");
                 treeView_.get_selection()->set_mode(Gtk::SELECTION_SINGLE);
                 break;
    case CdDevice::CD_R:
                 infoLabel->set_label("<b><big>Available Recorder Devices</big></b>");
                 treeView_.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
                 break;
    case CdDevice::CD_RW:
                 infoLabel->set_label("<b><big>Available Recorder (RW) Devices</big></b>");
                 treeView_.get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
                 break;
  }

  set_spacing(5);

  import();
  CdDevice::signal_statusChanged.connect(slot(*this, &DeviceList::importStatus));
  show();
}

DeviceList::~DeviceList()
{
  Gtk::TreeIter iter;

  while (iter = listStore_->get_iter("0")) {
    listStore_->erase(iter);
  }
}

Glib::RefPtr<Gtk::TreeSelection> DeviceList::get_selection()
{
  return treeView_.get_selection();
}

void DeviceList::appendTableEntry(CdDevice *dev)
{
  Gtk::TreeRow row = *(listStore_->append());
  row[modelColumns_.name] = string(dev->vendor()) + " " + string(dev->product()) + "  ";
  row[modelColumns_.status] = CdDevice::status2string(dev->status());
  row[modelColumns_.device] = dev;
}

void DeviceList::import()
{
  CdDevice *drun;
  unsigned int i;

  Gtk::TreeIter iter;

  while (iter = listStore_->get_iter("0")) {
    listStore_->erase(iter);
  }

  for (drun = CdDevice::first(); drun != NULL; drun = CdDevice::next(drun)) {
    switch (filterType_)
    {
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

  selectOne();
}

void DeviceList::importStatus()
{
  CdDevice *dev;
  Gtk::TreeIter iter;
  Gtk::TreeRow row;

  for (iter = listStore_->get_iter("0"); iter != 0; iter++) {
    row = *(iter);
    static_cast<void *>(dev) = row[modelColumns_.device];
    (*iter)[modelColumns_.status] = CdDevice::status2string(dev->status());
  }
}

void DeviceList::selectOne()
{
  CdDevice *dev;
  Gtk::TreeIter iter;
  Gtk::TreeRow row;

  if (selectionEmpty()) {
    for (iter = listStore_->get_iter("0"); iter != 0; iter++) {
      row = *(iter);
      static_cast<void *>(dev) = row[modelColumns_.device];
      if (dev->status() == CdDevice::DEV_READY) {
        treeView_.get_selection()->select(iter);
        break;
      }
    }
  }
}

void DeviceList::selectOneBut(CdDevice *target)
{
  CdDevice *dev;
  Gtk::TreeIter iter;
  Gtk::TreeRow row;


  if (selectionEmpty()) {
    for (iter = listStore_->get_iter("0"); iter != 0; iter++) {
      if (treeView_.get_selection()->is_selected(Gtk::TreePath(iter))) {
        row = *(iter);
        static_cast<void *>(dev) = row[modelColumns_.device];
        if ((dev->status() == CdDevice::DEV_READY)
         && (dev != target)) {
           treeView_.get_selection()->select(iter);
        break;
        }
      }
    }
  }

  // Try selectOne in case we don't selected any device.
  selectOne();
}

CdDevice *DeviceList::getFirstSelected()
{
  CdDevice *dev = 0;
  Gtk::TreeIter iter;
  Gtk::TreeRow row;

  for (iter = listStore_->get_iter("0"); iter != 0; iter++) {
    if (treeView_.get_selection()->is_selected(Gtk::TreePath(iter)))
    {
      row = *(iter);
      static_cast<void *>(dev) = row[modelColumns_.device];
	  break;
    }
  }

  return dev;
}

bool DeviceList::isSelected(CdDevice *device)
{
  CdDevice *dev = 0;
  Gtk::TreeIter iter;
  Gtk::TreeRow row;

  for (iter = listStore_->get_iter("0"); iter != 0; iter++) {
    if (treeView_.get_selection()->is_selected(Gtk::TreePath(iter)))
    {
      row = *(iter);
      static_cast<void *>(dev) = row[modelColumns_.device];
      if (dev == device)
        return true;
    }
  }

  return false;
}

bool DeviceList::selectionEmpty()
{
  CdDevice *dev;
  Gtk::TreeIter iter;
  Gtk::TreeRow row;

  for (iter = listStore_->get_iter("0"); iter != 0; iter++) {
    if (treeView_.get_selection()->is_selected(Gtk::TreePath(iter)))
      return false;
  }

  return true;
}

std::list<CdDevice *> DeviceList::getAllSelected()
{
  CdDevice *dev = 0;
  Gtk::TreeIter iter;
  Gtk::TreeRow row;
  std::list<CdDevice *> devices;

  for (iter = listStore_->get_iter("0"); iter != 0; iter++) {
    if (treeView_.get_selection()->is_selected(Gtk::TreePath(iter)))
    {
      row = *(iter);
      static_cast<void *>(dev) = row[modelColumns_.device];
      devices.push_back(dev);
    }
  }

  return devices;
}
