/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998, 1999  Andreas Mueller <mueller@daneb.ping.de>
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

#ifndef __DEVICE_LIST_H
#define __DEVICE_LIST_H

#include <gtkmm.h>
#include <gtk/gtk.h>

class TocEdit;
#include "CdDevice.h"

class DeviceList : public Gtk::Frame
{
 public:
  DeviceList(CdDevice::DeviceType filterType);
  ~DeviceList();

  struct DeviceData {
    int bus, id, lun;
  };

  DeviceData* selection();
  void selectOne();
  void selectOneBut(DeviceData* targetData);
  void appendTableEntry(CdDevice *);
  void import();
  void importStatus();

private:
  TocEdit* tocEdit_;

  int speed_;
  CdDevice::DeviceType filterType_;

  class ListColumns : public Gtk::TreeModel::ColumnRecord {
  public:
    ListColumns()
      { add(bus); add(id); add(lun); add(vendor); add(model); add(status);
      add(data); };

    Gtk::TreeModelColumn<int>         bus;
    Gtk::TreeModelColumn<int>         id;
    Gtk::TreeModelColumn<int>         lun;
    Gtk::TreeModelColumn<std::string> vendor;
    Gtk::TreeModelColumn<std::string> model;
    Gtk::TreeModelColumn<std::string> status;
    Gtk::TreeModelColumn<DeviceData*> data;
  };

  Gtk::TreeView list_;
  Glib::RefPtr<Gtk::ListStore> listModel_;
  ListColumns listColumns_;
};

#endif
