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
/*
 * $Log: not supported by cvs2svn $
 * Revision 1.6  2002/01/20 20:43:37  andreasm
 * Added support for sub-channel reading and writing.
 * Adapted to autoconf-2.52.
 * Adapted to gcc-3.0.
 *
 * Revision 1.5  2000/10/01 16:39:10  llanero
 * applied Jason Lunz patch: "Close" instead of "Cancel" where appropiate.
 *
 * Revision 1.4  2000/09/21 02:07:06  llanero
 * MDI support:
 * Splitted AudioCDChild into same and AudioCDView
 * Move Selections from TocEdit to AudioCDView to allow
 *   multiple selections.
 * Cursor animation in all the views.
 * Can load more than one from from command line
 * Track info, Toc info, Append/Insert Silence, Append/Insert Track,
 *   they all are built for every child when needed.
 * ...
 *
 * Revision 1.3  2000/04/23 09:07:08  andreasm
 * * Fixed most problems marked with '//llanero'.
 * * Added audio CD edit menus to MDIWindow.
 * * Moved central storage of TocEdit object to MDIWindow.
 * * AudioCdChild is now handled like an ordinary non modal dialog, i.e.
 *   it has a normal 'update' member function now.
 * * Added CdTextTable modal dialog.
 * * Old functionality of xcdrdao is now available again.
 *
 * Revision 1.2  2000/02/20 23:34:53  llanero
 * fixed scsilib directory (files mising ?-()
 * ported xdao to 1.1.8 / gnome (MDI) app
 *
 * Revision 1.1.1.1  2000/02/05 01:38:46  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 * Revision 1.1  1999/09/06 09:09:37  mueller
 * Initial revision
 *
 */

#ifndef __DEVICE_CONF_DIALOG_H
#define __DEVICE_CONF_DIALOG_H

class TocEdit;
class CdDevice;
class DeviceList;

class DeviceConfDialog : public Gtk::Dialog {
public:
  DeviceConfDialog();
  ~DeviceConfDialog();

  gint delete_event_impl(GdkEventAny*);

private:
  CdDevice *selectedDevice_;

  struct DeviceData {
    int bus, id, lun;
    int driverId;
    int deviceType;
    unsigned long options;
    std::string specialDevice;
  };

  DeviceList *devices_;

  Gtk::OptionMenu *driverMenu_;

  Gtk::OptionMenu *devtypeMenu_;

  Gtk::Dialog *addDeviceDialog_;
  Gtk::SpinButton *busEntry_, *idEntry_, *lunEntry_;
  Gtk::Entry *vendorEntry_, *productEntry_;
  Gtk::Entry *specialDeviceEntry_;
  Gtk::Entry *driverOptionsEntry_;

  const char *checkString(const std::string &str);

  void setDriverId(int);
  void setDeviceType(int);

  void selectRow();
  void unselectRow(gint, gint, GdkEvent *);

  void closeAction();
  void applyAction();
  void addDevice();
  void addDeviceAction();
  void deleteDeviceAction();
  void rescanAction();

  void importConfiguration(CdDevice *);

  void exportConfiguration(CdDevice *);
};

#endif
