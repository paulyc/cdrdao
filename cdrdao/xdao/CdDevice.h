/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2001  Andreas Mueller <mueller@daneb.ping.de>
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

#ifndef __CD_DEVICE_H__
#define __CD_DEVICE_H__

#include <sigc++/object.h>
#include <gdk/gdk.h>

#include "Project.h"

class TocEdit;
class Process;
class ScsiIf;

class CdDevice : public SigC::Object
{
public:
  enum Status { DEV_READY, DEV_RECORDING, DEV_READING, DEV_WAITING, DEV_BUSY,
		DEV_NO_DISK, DEV_BLANKING, DEV_FAULT, DEV_UNKNOWN };
  enum DeviceType { CD_R, CD_RW, CD_ROM };

  enum Action { A_RECORD, A_READ, A_DUPLICATE, A_BLANK, A_NONE };

  CdDevice(int bus, int id, int lun, const char *vendor,
	   const char *product, bool is_atapi = false);
  ~CdDevice();

  char *settingString() const;

  int bus() const                    { return bus_; }
  int id() const                     { return id_; }
  int lun() const                    { return lun_; }
  const char *vendor() const         { return vendor_; }
  const char *product() const        { return product_; }
  bool is_atapi() const              { return is_atapi_; }

  Status status() const              { return status_; }
  Process *process() const           { return process_; }

  int exitStatus() const;

  void status(Status);
  int updateStatus();

  Action action() const              { return action_; }

  bool updateProgress(Glib::IOCondition, int fd);

  int autoSelectDriver();

  int driverId() const                 { return driverId_; }
  void driverId(int);

  DeviceType deviceType() const;
  void deviceType(DeviceType);

  unsigned long driverOptions() const;
  void driverOptions(unsigned long);

  const char *specialDevice() const    { return specialDevice_; }
  void specialDevice(const char *);

  int manuallyConfigured() const       { return manuallyConfigured_; }
  void manuallyConfigured(int);

  bool recordDao(Gtk::Window& parent, TocEdit *, int simulate,
                 int multiSession, int speed, int eject, int reload,
                 int buffer, int overburn);
  void abortDaoRecording();

  int extractDao(Project& parent, const char *tocFileName, int correction,
                 int readSubChanMode);
  void abortDaoReading();

  int duplicateDao(Project& parent, int simulate, int multiSession, int speed,
		   int eject, int reload, int buffer, int onthefly,
		   int correction, int readSubChanMode, CdDevice *readdev);
  void abortDaoDuplication();

  int blank(Project* parent, int fast, int speed, int eject, int reload);
  void abortBlank();
    
  int progressStatusChanged();
  void progress(int *status, int *totalTracks, int *track,
		      int *trackProgress, int *totalProgress,
		      int *bufferFill) const;
  
  static int maxDriverId();
  static const char *driverName(int id);
  static int driverName2Id(const char *);

  static const char *status2string(Status);
  static const char *deviceType2string(DeviceType);

  static void importSettings();
  static void exportSettings();

  static CdDevice *add(int bus, int id, int lun, const char *vendor,
		       const char *product, bool is_atapi = false);

  static CdDevice *add(const char *setting);

  static CdDevice *find(int bus, int id, int lun);
  
  static void scan();

  static void remove(int bus, int id, int lun);

  static void clear();

  static CdDevice *first();
  static CdDevice *next(const CdDevice *);

  static int updateDeviceStatus();

  /* not used anymore since Gtk::Main::input signal will call
   * CdDevice::updateProgress directly.

     static int updateDeviceProgress();
  */

  static int count();

private:
  int bus_; // SCSI bus
  int id_;  // SCSI id
  int lun_; // SCSI logical unit
  bool is_atapi_;
  char *vendor_;
  char *product_;

  DeviceType deviceType_;

  int driverId_;
  unsigned long options_;
  char *specialDevice_;

  int manuallyConfigured_;

  ScsiIf *scsiIf_;
  int scsiIfInitFailed_;
  Status status_;

  Action action_;

  int exitStatus_;

  int progressStatusChanged_;
  int progressStatus_;
  int progressTotalTracks_;
  int progressTrack_;
  int progressTotal_;
  int progressTrackRelative_;
  int progressBufferFill_;

  Process *process_;

  CdDevice *next_;
  CdDevice *slaveDevice_; // slave device (used when copying etc.)

  void createScsiIf();

  static char *DRIVER_NAMES_[];
  static CdDevice *DEVICE_LIST_;
};

#endif
