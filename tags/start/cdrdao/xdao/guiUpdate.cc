/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998, 1999 Andreas Mueller <mueller@daneb.ping.de>
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
 * Revision 1.1  1999/08/19 20:28:40  mueller
 * Initial revision
 *
 */

static char rcsid[] = "$Id: guiUpdate.cc,v 1.1.1.1 2000-02-05 01:40:28 llanero Exp $";

#include "guiUpdate.h"

#include "xcdrdao.h"
#include "TocEdit.h"
#include "MainWindow.h"
#include "TrackInfoDialog.h"
#include "TocInfoDialog.h"
#include "AddSilenceDialog.h"
#include "AddFileDialog.h"
#include "DeviceConfDialog.h"
#include "RecordDialog.h"
#include "RecordProgressDialog.h"
#include "ProcessMonitor.h"
#include "CdDevice.h"

#include "util.h"

void guiUpdate(unsigned long level)
{
  if (MAIN_WINDOW == NULL)
    return;

  TocEdit *tocEdit = MAIN_WINDOW->tocEdit();

  level |= tocEdit->updateLevel();

  MAIN_WINDOW->update(level);

  if (TRACK_INFO_DIALOG != NULL)
    TRACK_INFO_DIALOG->update(level, tocEdit);

  if (TOC_INFO_DIALOG != NULL)
    TOC_INFO_DIALOG->update(level, tocEdit);

  if (ADD_SILENCE_DIALOG != NULL)
    ADD_SILENCE_DIALOG->update(level, tocEdit);

  if (ADD_FILE_DIALOG != NULL)
    ADD_FILE_DIALOG->update(level, tocEdit);

  if (DEVICE_CONF_DIALOG != NULL)
    DEVICE_CONF_DIALOG->update(level, tocEdit);

  if (RECORD_DIALOG != NULL)
    RECORD_DIALOG->update(level, tocEdit);

  if (RECORD_PROGRESS_POOL != NULL)
    RECORD_PROGRESS_POOL->update(level, tocEdit);
}

int guiUpdatePeriodic()
{
  if (CdDevice::updateDeviceStatus())
    guiUpdate(UPD_CD_DEVICE_STATUS);

  if (CdDevice::updateDeviceProgress())
    guiUpdate(UPD_PROGRESS_STATUS);

  return 1;
}
