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

class TocEdit;
#include "CdDevice.h"

class DeviceList : public Gtk::VBox {
public:
  DeviceList(CdDevice::DeviceType filterType);
  ~DeviceList();

  Glib::RefPtr<Gtk::TreeSelection> get_selection();

  void selectOne();
  void selectOneBut(CdDevice *target);

  CdDevice *getFirstSelected();
  bool isSelected(CdDevice *);

  std::list<CdDevice *> getAllSelected();

private:
  TocEdit *tocEdit_;

  int speed_;
  CdDevice::DeviceType filterType_;

public:

  void appendTableEntry(CdDevice *);
  void import();
  void importStatus();

private:
  Gtk::TreeView treeView_;
  Glib::RefPtr<Gtk::ListStore> listStore_;

  struct ModelColumns : public Gtk::TreeModelColumnRecord
  {
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> status;
    Gtk::TreeModelColumn<void *> device;

    ModelColumns() { add(name); add(status); add(device); }
  };

  const ModelColumns modelColumns_;
};

#endif
