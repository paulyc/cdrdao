/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2000  Andreas Mueller <mueller@daneb.ping.de>
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
 * Revision 1.6  2000/06/19 20:17:37  andreasm
 * Added CDDB reading to add CD-TEXT information to toc-files.
 * Fixed bug in reading ATIP data in 'GenericMMC::diskInfo()'.
 * Attention: CdrDriver.cc is currently configured to read TAO disks.
 *
 * Revision 1.5  2000/06/10 14:48:05  andreasm
 * Tracks that are shorter than 4 seconds can be recorded now if the user confirms
 * it.
 * The driver table is now read from an external file (.../share/cdrdao/drivers
 * and $HOME/.cdrdao-drivers).
 * Fixed bug the might prevented writing pure data CDs with some recorders.
 *
 * Revision 1.4  2000/06/06 22:26:13  andreasm
 * Updated list of supported drives.
 * Added saving of some command line settings to $HOME/.cdrdao.
 * Added test for multi session support in raw writing mode to GenericMMC.cc.
 * Updated manual page.
 *
 * Revision 1.3  2000/05/01 18:13:18  andreasm
 * Fixed too small mode page buffer.
 *
 * Revision 1.2  2000/04/23 16:29:50  andreasm
 * Updated to state of my private development environment.
 *
 * Revision 1.14  1999/12/15 20:31:46  mueller
 * Added remote messages for 'read-cd' progress used by a GUI.
 *
 * Revision 1.13  1999/11/07 09:14:59  mueller
 * Release 1.1.3
 *
 * Revision 1.12  1999/04/05 18:47:40  mueller
 * Added driver options.
 * Added option to read Q sub-channel data instead raw PW sub-channel data
 * for 'read-toc'.
 *
 * Revision 1.11  1999/03/27 20:51:26  mueller
 * Added data track support.
 *
 * Revision 1.10  1998/10/25 14:38:28  mueller
 * Fixed comment.
 *
 * Revision 1.9  1998/10/24 14:48:55  mueller
 * Added retrieval of next writable address. The Panasonic CW-7502 needs it.
 *
 * Revision 1.8  1998/10/03 15:08:44  mueller
 * Moved 'writeZeros()' to base class 'CdrDriver'.
 * Takes 'writeData()' from base class now.
 *
 * Revision 1.7  1998/09/27 19:18:48  mueller
 * Added retrieval of control nibbles for track with 'analyzeTrack()'.
 * Added multi session mode.
 *
 * Revision 1.6  1998/09/22 19:15:13  mueller
 * Removed memory allocations during write process.
 *
 * Revision 1.5  1998/09/08 11:54:22  mueller
 * Extended disk info structure because CDD2000 does not support the
 * 'READ DISK INFO' command.
 *
 * Revision 1.4  1998/09/07 15:20:20  mueller
 * Reorganized read-toc related code.
 *
 * Revision 1.3  1998/09/06 13:34:22  mueller
 * Use 'message()' for printing messages.
 *
 * Revision 1.2  1998/08/30 19:17:56  mueller
 * Fixed cue sheet generation and first writable address after testing
 * with a Yamaha CDR400t.
 *
 * Revision 1.1  1998/08/15 20:44:58  mueller
 * Initial revision
 *
 */

static char rcsid[] = "$Id: GenericMMC.cc,v 1.7 2000-06-22 12:19:28 andreasm Exp $";

#include <config.h>

#include <string.h>
#include <assert.h>

#include "GenericMMC.h"

#include "port.h"
#include "Toc.h"
#include "util.h"
#include "PQSubChannel16.h"
#include "PWSubChannel96.h"
#include "CdTextEncoder.h"

GenericMMC::GenericMMC(ScsiIf *scsiIf, unsigned long options)
  : CdrDriver(scsiIf, options)
{
  int i;
  driverName_ = "Generic SCSI-3/MMC - Version 1.0 (data)";
  
  speed_ = 0;
  simulate_ = 1;
  encodingMode_ = 1;

  scsiTimeout_ = 0;
  
  cdTextEncoder_ = NULL;

  memset(&diskInfo_, 0, sizeof(DiskInfo));

  for (i = 0; i < maxScannedSubChannels_; i++) {
    if (options_ & OPT_MMC_USE_PQ) 
      scannedSubChannels_[i] = new PQSubChannel16;
    else
      scannedSubChannels_[i] = new PWSubChannel96;
  }

  // MMC drives usually return little endian samples
  audioDataByteOrder_ = 0;
}

GenericMMC::~GenericMMC()
{
  int i;

  for (i = 0; i < maxScannedSubChannels_; i++) {
    delete scannedSubChannels_[i];
    scannedSubChannels_[i] = NULL;
  }

  delete cdTextEncoder_;
  cdTextEncoder_ = NULL;
}

// static constructor
CdrDriver *GenericMMC::instance(ScsiIf *scsiIf, unsigned long options)
{
  return new GenericMMC(scsiIf, options);
}


int GenericMMC::checkToc(const Toc *toc)
{
  int err = CdrDriver::checkToc(toc);
  int e;

  if (options_ & OPT_MMC_CD_TEXT) {
    if ((e = toc->checkCdTextData()) > err)
      err = e;
  }

  return err;
}

// sets speed
// return: 0: OK
//         1: illegal speed
int GenericMMC::speed(int s)
{
  speed_ = s;

  if (selectSpeed(0) != 0)
    return 1;
  
  return 0;
}

int GenericMMC::speed()
{
  DriveInfo di;

  if (driveInfo(&di, 1) != 0) {
    return 0;
  }

  return speed2Mult(di.currentWriteSpeed);

}

// loads ('unload' == 0) or ejects ('unload' == 1) tray
// return: 0: OK
//         1: scsi command failed
int GenericMMC::loadUnload(int unload) const
{
  unsigned char cmd[6];

  memset(cmd, 0, 6);

  cmd[0] = 0x1b; // START/STOP UNIT
  if (unload) {
    cmd[4] = 0x02; // LoUnlo=1, Start=0
  }
  else {
    cmd[4] = 0x03; // LoUnlo=1, Start=1
  }
  
  if (sendCmd(cmd, 6, NULL, 0, NULL, 0) != 0) {
    message(-2, "Cannot load/unload medium.");
    return 1;
  }

  return 0;
}

// Performs complete blanking of a CD-RW.
// return: 0: OK
//         1: scsi command failed
int GenericMMC::blankDisk()
{
  unsigned char cmd[12];

  memset(cmd, 0, 12);

  cmd[0] = 0xa1; // BLANK
  cmd[1] = 1 << 4; // erase complete disk, immediate return

  if (sendCmd(cmd, 12, NULL, 0, NULL, 0, 1) != 0) {
    message(-2, "Cannot erase CD-RW.");
    return 1;
  }

  return 0;
}

// sets read/write speed and simulation mode
// return: 0: OK
//         1: scsi command failed
int GenericMMC::selectSpeed(int readSpeed)
{
  unsigned char cmd[12];
  int spd;

  memset(cmd, 0, 12);

  cmd[0] = 0xbb; // SET CD SPEED

  // select maximum read speed
  if (readSpeed == 0) {
    cmd[2] = 0xff;
    cmd[3] = 0xff;
  }
  else {
    spd = mult2Speed(readSpeed);
    cmd[2] = spd >> 8;
    cmd[3] = spd;
  }

  // select maximum write speed
  if (speed_ == 0) {
    cmd[4] = 0xff;
    cmd[5] = 0xff;
  }
  else {
    spd = mult2Speed(speed_);
    cmd[4] = spd >> 8;
    cmd[5] = spd;
  }

  if (sendCmd(cmd, 12, NULL, 0, NULL, 0) != 0) {
    message(-2, "Cannot set cd speed.");
    return 1;
  }

  return 0;
}

// Determins start and length of lead-in and length of lead-out.
// return: 0: OK
//         1: SCSI command failed
int GenericMMC::getSessionInfo()
{
  unsigned char cmd[10];
  unsigned long dataLen = 34;
  unsigned char data[34];

  memset(cmd, 0, 10);
  memset(data, 0, dataLen);

  cmd[0] = 0x51; // READ DISK INFORMATION
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if (sendCmd(cmd, 10, NULL, 0, data, dataLen) != 0) {
    message(-2, "Cannot retrieve disk information.");
    return 1;
  }

  leadInStart_ = Msf(data[17], data[18], data[19]);

  if (leadInStart_.lba() >= Msf(80, 0, 0).lba()) {
    leadInLen_ = 450000 - leadInStart_.lba();
    leadOutLen_ = Msf(1, 30, 0).lba(); // 90 seconds lead-out
  }
  else {
    leadInLen_ = Msf(1, 0, 0).lba();
    leadOutLen_ = Msf(0, 30, 0).lba();
  }


  message(3, "Lead-in start: %s length: %ld", leadInStart_.str(),
	  leadInLen_);
  message(3, "Lead-out length: %ld", leadOutLen_);

  return 0;
}

int GenericMMC::readBufferCapacity(long *capacity)
{
  unsigned char cmd[10];
  unsigned char data[12];

  memset(cmd, 0, 10);
  memset(data, 0, 12);

  cmd[0] = 0x5c; // READ BUFFER CAPACITY
  cmd[8] = 12;

  if (sendCmd(cmd, 10, NULL, 0, data, 12) != 0) {
    message(-2, "Read buffer capacity failed.");
    return 1;
  }

  *capacity = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];

  return 0;
}

int GenericMMC::performPowerCalibration()
{
  unsigned char cmd[10];

  memset(cmd, 0, 10);

  cmd[0] = 0x54; // SEND OPC INFORMATION
  cmd[1] = 1;

  message(1, "Executing power calibration...");

  if (sendCmd(cmd, 10, NULL, 0, NULL, 0) != 0) {
    message(-2, "Power calibration failed.");
    return 1;
  }
  
  message(1, "Power calibration successful.");

  return 0;
}

// Sets write parameters via mode page 0x05.
// return: 0: OK
//         1: scsi command failed
int GenericMMC::setWriteParameters()
{
  unsigned char mp[0x38];
  unsigned char mpHeader[8];
  unsigned char blockDesc[8];

  if (getModePage(5/*write parameters mode page*/, mp, 0x38,
		  mpHeader, blockDesc, 1) != 0) {
    message(-2, "Cannot retrieve write parameters mode page.");
    return 1;
  }

  mp[0] &= 0x7f; // clear PS flag

  mp[2] &= 0xe0;
  mp[2] |= 0x02; // write type: Session-at-once
  if (simulate_) {
    mp[2] |= 1 << 4; // test write
  }

  mp[3] &= 0x3f; // Multi-session: No B0 pointer, next session not allowed

  if (multiSession_ != 0)
    mp[3] |= 0x03 << 6; // open next session
  else if (!diskInfo_.empty)
    mp[3] |= 0x01 << 6; // use B0=FF:FF:FF when closing last session of a
                        // multi session CD-R

  message(3, "Multi session mode: %d", mp[3] >> 6);

  mp[4] &= 0xf0; // Data Block Type: raw data, block size: 2352 (I think not
                 // used for session at once writing)

  mp[8] = sessionFormat();

  if (!diskInfo_.empty) {
    // use the toc type of the already recorded sessions
    switch (diskInfo_.diskTocType) {
    case 0x00:
    case 0x10:
    case 0x20:
      mp[8] = diskInfo_.diskTocType;
      break;
    }
  }

  message(3, "Toc type: 0x%x", mp[8]);

  if (setModePage(mp, mpHeader, NULL, 1) != 0) {
    message(-2, "Cannot set write parameters mode page.");
    return 1;
  }

  return 0;
}

int GenericMMC::getNWA(long *nwa)
{
  unsigned char cmd[10];
  int infoblocklen = 16;
  unsigned char info[16];
  long lba = 0;

  cmd[0] = 0x52; // READ TRACK INFORMATION
  cmd[1] = 0x01; // track instead of lba designation
  cmd[2] = 0x00;
  cmd[3] = 0x00;
  cmd[4] = 0x00;
  cmd[5] = 0xff; // invisible track
  cmd[6] = 0x00; // reserved
  cmd[7] = infoblocklen << 8;
  cmd[8] = infoblocklen; // alloc length
  cmd[9] = 0x00; // Control Byte

  if (sendCmd(cmd, 10, NULL, 0, info, infoblocklen) != 0) {
    message(-2, "Cannot get Track Information Block.");
    return 1;
  }

#if 0
  message(2,"Track Information Block");
  for (int i=0;i<infoblocklen;i++) message(2,"byte %02x : %02x",i,info[i]);
#endif

  if ((info[6] & 0x40) && (info[7] & 0x01) && !(info[6] & 0xb0))
  {
      message(3,"Track is Blank, Next Writable Address is valid");
      lba |= info[12] << 24; // MSB of LBA
      lba |= info[13] << 16;
      lba |= info[14] << 8;
      lba |= info[15];       // LSB of LBA
  }

  message(3, "NWA: %ld", lba);

  if (nwa != NULL) 
    *nwa = lba;

  return 0;
}

// Determines first writable address of data area of current empty session. 
// lba: set to start of data area
// return: 0: OK
//         1: error occured
int GenericMMC::getStartOfSession(long *lba)
{
  unsigned char mp[0x38];
  unsigned char mpHeader[8];

  // first set the writing mode because it influences which address is
  // returned with 'READ TRACK INFORMATION'

  if (getModePage(5/*write parameters mode page*/, mp, 0x38,
		  mpHeader, NULL, 0) != 0) {
    return 1;
  }

  mp[0] &= 0x7f; // clear PS flag

  mp[2] &= 0xe0;
  mp[2] |= 0x02; // write type: Session-at-once

  if (setModePage(mp, mpHeader, NULL, 1) != 0) {
    message(-2, "Cannot set write parameters mode page.");
    return 1;
  }

  return getNWA(lba);
}

static unsigned char leadInOutDataMode(TrackData::Mode mode)
{
  unsigned char ret;

  switch (mode) {
  case TrackData::AUDIO:
    ret = 0x01;
    break;

  case TrackData::MODE0: // should not happen
  case TrackData::MODE1:
  case TrackData::MODE1_RAW:
    ret = 0x14;
    break;

  case TrackData::MODE2_FORM1:
  case TrackData::MODE2_FORM2:
  case TrackData::MODE2_FORM_MIX:
  case TrackData::MODE2_RAW: // assume it contains XA sectors
    ret = 0x24;
    break;

  case TrackData::MODE2:
    ret = 0x34;
    break;
  }

  return ret;
}

		  

// Creates cue sheet for current toc.
// cueSheetLen: filled with length of cue sheet in bytes
// return: newly allocated cue sheet buffer or 'NULL' on error
unsigned char *GenericMMC::createCueSheet(long *cueSheetLen)
{
  const Track *t;
  int trackNr;
  Msf start, end, index;
  unsigned char *cueSheet;
  long len = 3; // entries for lead-in, gap, lead-out
  long n; // index into cue sheet
  unsigned char ctl; // control nibbles of cue sheet entry CTL/ADR
  long i;
  unsigned char dataMode;
  int trackOffset = 0;
  long lbaOffset = 0;

  if (!diskInfo_.empty && diskInfo_.append) {
    trackOffset = diskInfo_.lastTrackNr;
    lbaOffset = diskInfo_.thisSessionLba;
  }

  TrackIterator itr(toc_);

  if (itr.first(start, end) == NULL) {
    return NULL;
  }

  if (toc_->catalogValid()) {
    len += 2;
  }

  for (t = itr.first(start, end), trackNr = 1;
       t != NULL; t = itr.next(start, end), trackNr++) {
    len += 1; // entry for track
    if (t->start().lba() != 0 && trackNr > 1) {
      len += 1; // entry for pre-gap
    }
    if (t->type() == TrackData::AUDIO && t->isrcValid()) {
      len += 2; // entries for ISRC code
    }
    len += t->nofIndices(); // entry for each index increment
  }

  cueSheet = new (unsigned char)[len * 8];
  n = 0;

  if (toc_->leadInMode() == TrackData::AUDIO) {
    ctl = 0;
  }
  else {
    ctl = 0x40;
  }

  if (toc_->catalogValid()) {
    // fill catalog number entry
    cueSheet[n*8] = 0x02 | ctl;
    cueSheet[(n+1)*8] = 0x02 | ctl;
    for (i = 1; i <= 13; i++) {
      if (i < 8) {
	cueSheet[n*8 + i] = toc_->catalog(i-1) + '0';
      }
      else {
	cueSheet[(n+1)*8 + i - 7] = toc_->catalog(i-1) + '0';
      }
    }
    cueSheet[(n+1)*8+7] = 0;
    n += 2;
  }

  // entry for lead-in
  cueSheet[n*8] = 0x01 | ctl; // CTL/ADR
  cueSheet[n*8+1] = 0;    // Track number
  cueSheet[n*8+2] = 0;    // Index

  if (cdTextEncoder_ != NULL) {
    cueSheet[n*8+3] = 0x41; // Data Form: CD-DA with P-W sub-channels,
                            // main channel data generated by device
  }
  else {
    cueSheet[n*8+3] = leadInOutDataMode(toc_->leadInMode());
  }

  cueSheet[n*8+4] = 0;    // Serial Copy Management System
  cueSheet[n*8+5] = 0;    // MIN
  cueSheet[n*8+6] = 0;    // SEC
  cueSheet[n*8+7] = 0;    // FRAME
  n++;

  int firstTrack = 1;

  for (t = itr.first(start, end), trackNr = trackOffset + 1;
       t != NULL;
       t = itr.next(start, end), trackNr++) {
    if (encodingMode_ == 0) {
      // just used for some experiments with raw writing
      dataMode = 0;
    }
    else {
      switch (t->type()) {
      case TrackData::AUDIO:
	dataMode = 0;
	break;
      case TrackData::MODE1:
      case TrackData::MODE1_RAW:
	dataMode = 0x10;
	break;
      case TrackData::MODE2:
	dataMode = 0x30;
	break;
      case TrackData::MODE2_RAW: // assume it contains XA sectors
      case TrackData::MODE2_FORM1:
      case TrackData::MODE2_FORM2:
      case TrackData::MODE2_FORM_MIX:
	dataMode = 0x20;
	break;
      default:
	dataMode = 0;
	break;
      }
    }

    ctl = 0;
    if (t->copyPermitted()) {
      ctl |= 0x20;
    }

    if (t->type() == TrackData::AUDIO) {
      // audio track
      if (t->preEmphasis()) {
	ctl |= 0x10;
      }
      if (t->audioType() == 1) {
	ctl |= 0x80;
      }

      if (t->isrcValid()) {
	cueSheet[n*8] = ctl | 0x03;
	cueSheet[n*8+1] = trackNr;
	cueSheet[n*8+2] = t->isrcCountry(0);
	cueSheet[n*8+3] = t->isrcCountry(1);
	cueSheet[n*8+4] = t->isrcOwner(0);  
	cueSheet[n*8+5] = t->isrcOwner(1);   
	cueSheet[n*8+6] = t->isrcOwner(2);  
	cueSheet[n*8+7] = t->isrcYear(0) + '0';
	n++;

	cueSheet[n*8] = ctl | 0x03;
	cueSheet[n*8+1] = trackNr;
	cueSheet[n*8+2] = t->isrcYear(1) + '0';
	cueSheet[n*8+3] = t->isrcSerial(0) + '0';
	cueSheet[n*8+4] = t->isrcSerial(1) + '0';
	cueSheet[n*8+5] = t->isrcSerial(2) + '0';
	cueSheet[n*8+6] = t->isrcSerial(3) + '0';
	cueSheet[n*8+7] = t->isrcSerial(4) + '0';
	n++;
      }
    }
    else {
      // data track
      ctl |= 0x40;
    }
	 

    if (firstTrack) {
      Msf sessionStart(lbaOffset);

      // entry for gap before first track
      cueSheet[n*8]   = ctl | 0x01;
      cueSheet[n*8+1] = trackNr;
      cueSheet[n*8+2] = 0;    // Index 0
      cueSheet[n*8+3] = dataMode;    // Data Form
      cueSheet[n*8+4] = 0;    // Serial Copy Management System
      cueSheet[n*8+5] = sessionStart.min();
      cueSheet[n*8+6] = sessionStart.sec();
      cueSheet[n*8+7] = sessionStart.frac();
      n++;
    }
    else if (t->start().lba() != 0) {
      // entry for pre-gap
      Msf pstart(lbaOffset + start.lba() - t->start().lba() + 150);

      cueSheet[n*8]   = ctl | 0x01;
      cueSheet[n*8+1] = trackNr;
      cueSheet[n*8+2] = 0; // Index 0 indicates pre-gap
      cueSheet[n*8+3] = dataMode; // Data Form
      cueSheet[n*8+4] = 0; // no alternate copy bit
      cueSheet[n*8+5] = pstart.min();
      cueSheet[n*8+6] = pstart.sec();
      cueSheet[n*8+7] = pstart.frac();
      n++;
    }

    Msf tstart(lbaOffset + start.lba() + 150);
    
    cueSheet[n*8]   = ctl | 0x01;
    cueSheet[n*8+1] = trackNr;
    cueSheet[n*8+2] = 1; // Index 1
    cueSheet[n*8+3] = dataMode; // Data Form
    cueSheet[n*8+4] = 0; // no alternate copy bit
    cueSheet[n*8+5] = tstart.min();
    cueSheet[n*8+6] = tstart.sec();
    cueSheet[n*8+7] = tstart.frac();
    n++;

    for (i = 0; i < t->nofIndices(); i++) {
      index = tstart + t->getIndex(i);
      cueSheet[n*8]   = ctl | 0x01;
      cueSheet[n*8+1] = trackNr;
      cueSheet[n*8+2] = i+2; // Index
      cueSheet[n*8+3] = dataMode; // Data Form
      cueSheet[n*8+4] = 0; // no alternate copy bit
      cueSheet[n*8+5] = index.min();
      cueSheet[n*8+6] = index.sec();
      cueSheet[n*8+7] = index.frac();
      n++;
    }

    firstTrack = 0;
  }

  assert(n == len - 1);

  // entry for lead out
  Msf lostart(lbaOffset + toc_->length().lba() + 150);
  ctl = toc_->leadOutMode() == TrackData::AUDIO ? 0 : 0x40;
    
  cueSheet[n*8]   = ctl | 0x01;
  cueSheet[n*8+1] = 0xaa;
  cueSheet[n*8+2] = 1; // Index 1
  cueSheet[n*8+3] = leadInOutDataMode(toc_->leadOutMode());
  cueSheet[n*8+4] = 0; // no alternate copy bit
  cueSheet[n*8+5] = lostart.min();
  cueSheet[n*8+6] = lostart.sec();
  cueSheet[n*8+7] = lostart.frac();

  message(2, "\nCue Sheet:");
  message(2, "CTL/  TNO  INDEX  DATA  SCMS  MIN  SEC  FRAME");
  message(2, "ADR               FORM");
  for (n = 0; n < len; n++) {
    message(2, "%02x    %02x    %02x     %02x    %02x   %02d   %02d   %02d",
	   cueSheet[n*8],
	   cueSheet[n*8+1], cueSheet[n*8+2], cueSheet[n*8+3], cueSheet[n*8+4],
	   cueSheet[n*8+5], cueSheet[n*8+6], cueSheet[n*8+7]);
  }

  *cueSheetLen = len * 8;
  return cueSheet;
}

int GenericMMC::sendCueSheet()
{
  unsigned char cmd[10];
  long cueSheetLen;
  unsigned char *cueSheet = createCueSheet(&cueSheetLen);

  if (cueSheet == NULL) {
    return 1;
  }

  memset(cmd, 0, 10);

  cmd[0] = 0x5d; // SEND CUE SHEET

  cmd[6] = cueSheetLen >> 16;
  cmd[7] = cueSheetLen >> 8;
  cmd[8] = cueSheetLen;

  if (sendCmd(cmd, 10, cueSheet, cueSheetLen, NULL, 0) != 0) {
    message(-2, "Cannot send cue sheet.");
    delete[] cueSheet;
    return 1;
  }

  delete[] cueSheet;
  return 0;
}

int GenericMMC::initDao(const Toc *toc)
{
  long n;
  blockLength_ = AUDIO_BLOCK_LEN;
  blocksPerWrite_ = scsiIf_->maxDataLen() / blockLength_;

  assert(blocksPerWrite_ > 0);

  toc_ = toc;

  if (options_ & OPT_MMC_CD_TEXT) {
    delete cdTextEncoder_;
    cdTextEncoder_ = new CdTextEncoder(toc_);
    if (cdTextEncoder_->encode() != 0) {
      message(-2, "CD-TEXT encoding failed.");
      return 1;
    }

    if (cdTextEncoder_->getSubChannels(&n) == NULL || n == 0) {
      delete cdTextEncoder_;
      cdTextEncoder_ = NULL;
    }
    
    //return 1;
  }

  diskInfo();

  if (!diskInfo_.valid.empty || !diskInfo_.valid.append) {
    message(-2, "Cannot determine status of inserted medium.");
    return 1;
  }

  if (!diskInfo_.append) {
    message(-2, "Inserted medium is not appendable.");
    return 1;
  }

  if (selectSpeed(0) != 0 ||
      getSessionInfo() != 0) {
    return 1;
  }


  // allocate buffer for writing zeros
  n = blocksPerWrite_ * blockLength_;
  delete[] zeroBuffer_;
  zeroBuffer_ = new char[n];
  memset(zeroBuffer_, 0, n);

  return 0;
}

int GenericMMC::startDao()
{
  scsiTimeout_ = scsiIf_->timeout(3 * 60);

  if (setWriteParameters() != 0)
    return 1;

  if (!simulate_) {
    if (performPowerCalibration() != 0) {
      if (!force()) {
	message(-2, "Use option --force to ignore this error.");
	return 1;
      }
      else {
	message(-2, "Ignored because of option --force.");
      }
    }
  }

  // It does not hurt if the following command fails.
  // The Panasonic CW-7502 needs it, unfortunately it returns the wrong
  // data so we ignore the returned data and start writing always with
  // LBA -150.
  getNWA(NULL);

  if (sendCueSheet() != 0)
    return 1;

  //message(1, "Writing lead-in and gap...");

  if (writeCdTextLeadIn() != 0) {
    return 1;
  }

  long lba = diskInfo_.thisSessionLba - 150;

  if (writeZeros(toc_->leadInMode(), lba, lba + 150, 150) != 0) {
    return 1;
  }
  
  return 0;
}

int GenericMMC::finishDao()
{
  int ret;

  //message(1, "Writing lead-out...");

  while ((ret = testUnitReady(0)) == 2)
    mSleep(2000);

  if (ret != 0)
    message(-1, "TEST UNIT READY failed after recording.");
  
  message(1, "Flushing cache...");
  
  if (flushCache() != 0) {
    return 1;
  }

  scsiIf_->timeout(scsiTimeout_);

  delete cdTextEncoder_;
  cdTextEncoder_ = NULL;

  delete[] zeroBuffer_, zeroBuffer_ = NULL;

  return 0;
}

void GenericMMC::abortDao()
{
  flushCache();

  delete cdTextEncoder_;
  cdTextEncoder_ = NULL;
}

// Writes data to target, the block length depends on the actual writing
// 'mode'. 'len' is number of blocks to write.
// 'lba' specifies the next logical block address for writing and is updated
// by this function.
// return: 0: OK
//         1: scsi command failed
int GenericMMC::writeData(TrackData::Mode mode, long &lba, const char *buf,
			  long len)
{
  assert(blocksPerWrite_ > 0);
  int writeLen = 0;
  unsigned char cmd[10];
  long blockLength = blockSize(mode);
  int retry;
  int ret;

#if 0
  long bufferCapacity;
  int waitForBuffer;
  int speedFrac;

  if (speed_ > 0)
    speedFrac = 75 * speed_;
  else
    speedFrac = 75 * 10; // adjust this value when the first >10x burner is out
#endif

#if 0
  long sum, i;

  sum = 0;

  for (i = 0; i < len * blockLength; i++) {
    sum += buf[i];
  }

  message(0, "W: %ld: %ld, %ld, %ld", lba, blockLength, len, sum);
#endif

  memset(cmd, 0, 10);
  cmd[0] = 0x2a; // WRITE1
  
  while (len > 0) {
    writeLen = (len > blocksPerWrite_ ? blocksPerWrite_ : len);

    cmd[2] = lba >> 24;
    cmd[3] = lba >> 16;
    cmd[4] = lba >> 8;
    cmd[5] = lba;

    cmd[7] = writeLen >> 8;
    cmd[8] = writeLen & 0xff;

#if 0
    do {
      waitForBuffer = 0;

      if (readBufferCapacity(&bufferCapacity) == 0) {
	//message(0, "Buffer Capacity: %ld", bufferCapacity);
	if (bufferCapacity < writeLen * blockLength) {
	  long t = 1000 * writeLen;
	  t /= speedFrac;
	  if (t <= 0)
	    t = 1;

	  message(0, "Waiting for %ld msec at lba %ld", t, lba);

	  mSleep(t);
	  waitForBuffer = 1;
	}
      }
    } while (waitForBuffer);
#endif

    do {
      retry = 0;

      ret = sendCmd(cmd, 10, (unsigned char *)buf, writeLen * blockLength,
		    NULL, 0, 0);

      if(ret == 2) {
        const unsigned char *sense;
        int senseLen;

        sense = scsiIf_->getSense(senseLen);

	// check if drive rejected the command because the internal buffer
	// is filled
	if(senseLen >= 14 && (sense[2] & 0x0f) == 0x2 && sense[7] >= 6 &&
	   sense[12] == 0x4 && sense[13] == 0x8) {
	  // Not Ready, long write in progress
	  mSleep(40);
          retry = 1;
        }
	else {
	  scsiIf_->printError();
	}
      }
    } while (retry);

    if (ret != 0) {
      message(-2, "Write data failed.");
      return 1;
    }

    buf += writeLen * blockLength;

    lba += writeLen;
    len -= writeLen;
  }
      
  return 0;
}

int GenericMMC::writeCdTextLeadIn()
{
  unsigned char cmd[10];
  const PWSubChannel96 **cdTextSubChannels;
  long cdTextSubChannelCount;
  long channelsPerCmd = scsiIf_->maxDataLen() / 96;
  long scp = 0;
  long lba = -150 - leadInLen_;
  long len = leadInLen_;
  long n;
  long i;
  unsigned char *p;

  if (cdTextEncoder_ == NULL)
    return 0;

  cdTextSubChannels = cdTextEncoder_->getSubChannels(&cdTextSubChannelCount);

  assert(channelsPerCmd > 0);
  assert(cdTextSubChannels != NULL);
  assert(cdTextSubChannelCount > 0);

  message(1, "Writing CD-TEXT lead-in...");

  message(2, "Start LBA: %ld, length: %ld", lba, len);

  memset(cmd, 0, 10);
  cmd[0] = 0x2a; // WRITE1

  while (len > 0) {
    n = (len > channelsPerCmd) ? channelsPerCmd : len;

    cmd[2] = lba >> 24;
    cmd[3] = lba >> 16;
    cmd[4] = lba >> 8;
    cmd[5] = lba;

    cmd[7] = n >> 8;
    cmd[8] = n;

    p = transferBuffer_;
    
    for (i = 0; i < n; i++) {
      memcpy(p, cdTextSubChannels[scp]->data(), 96);
      p += 96;

      scp++;
      if (scp >= cdTextSubChannelCount)
	scp = 0;
    }

    message(4, "Writing %ld CD-TEXT sub-channels at LBA %ld.", n, lba);

    if (sendCmd(cmd, 10, transferBuffer_, n * 96, NULL, 0) != 0) {
      message(-2, "Writing of CD-TEXT data failed.");
      return 1;
    }


    len -= n;
    lba += n;
  }
    
  return 0;
}

DiskInfo *GenericMMC::diskInfo()
{
  unsigned char cmd[10];
  unsigned long dataLen = 34;
  unsigned char data[34];
  char spd;

  DriveInfo info;

  driveInfo(&info, 1);

  memset(&diskInfo_, 0, sizeof(DiskInfo));

  // perform READ DISK INFORMATION
  memset(cmd, 0, 10);
  memset(data, 0, dataLen);

  cmd[0] = 0x51; // READ DISK INFORMATION
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if (sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) == 0) {
    diskInfo_.cdrw = (data[2] & 0x10) ? 1 : 0;
    diskInfo_.valid.cdrw = 1;

    switch (data[2] & 0x03) {
    case 0:
      // disc is empty
      diskInfo_.empty = 1;
      diskInfo_.append = 1;

      diskInfo_.manufacturerId = Msf(data[17], data[18], data[19]);
      diskInfo_.valid.manufacturerId = 1;
      break;

    case 1:
      // disc is not empty but appendable
      diskInfo_.sessionCnt = data[4];
      diskInfo_.lastTrackNr = data[6];

      diskInfo_.diskTocType = data[8];

      switch ((data[2] >> 2) & 0x03) {
      case 0:
	// last session is empty
	diskInfo_.append = 1;

	// don't count the empty session and invisible track
	diskInfo_.sessionCnt -= 1;
	diskInfo_.lastTrackNr -= 1;

	if (getStartOfSession(&(diskInfo_.thisSessionLba)) == 0) {
	  // reserve space for pre-gap after lead-in
	  diskInfo_.thisSessionLba += 150;
	}
	else {
	  // try to guess start of data area from start of lead-in
	  // reserve space for 4500 lead-in and 150 pre-gap sectors
	  diskInfo_.thisSessionLba = Msf(data[17], data[18],
					 data[19]).lba() - 150 + 4650;
	}
	break;

      case 1:
	// last session is incomplete (not fixated)
	// we cannot append in DAO mode, just update the statistic data
	
	diskInfo_.diskTocType = data[8];

	// don't count the invisible track
	diskInfo_.lastTrackNr -= 1;
	break;
      }
      break;

    case 2:
      // disk is complete
      diskInfo_.sessionCnt = data[4];
      diskInfo_.lastTrackNr = data[6];
      diskInfo_.diskTocType = data[8];
      break;
    }

    diskInfo_.valid.empty = 1;
    diskInfo_.valid.append = 1;

    if (data[21] != 0xff || data[22] != 0xff || data[23] != 0xff) {
      diskInfo_.valid.capacity = 1;
      diskInfo_.capacity = Msf(data[21], data[22], data[23]).lba() - 150;
    }
  }

  // perform READ TOC to get session info
  memset(cmd, 0, 10);
  dataLen = 12;
  memset(data, 0, dataLen);

  cmd[0] = 0x43; // READ TOC
  cmd[2] = 1; // get session info
  cmd[8] = dataLen; // allocation length

  if (sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) == 0) {
    diskInfo_.lastSessionLba = (data[8] << 24) | (data[9] << 16) |
                               (data[10] << 8) | data[11];

    if (!diskInfo_.valid.empty) {
      diskInfo_.valid.empty = 1;
      diskInfo_.empty = (data[3] == 0) ? 1 : 0;

      diskInfo_.sessionCnt = data[3];
    }
  }

  // read ATIP data
  dataLen = 28;
  memset(cmd, 0, 10);
  memset(data, 0, dataLen);

  cmd[0] = 0x43; // READ TOC/PMA/ATIP
  cmd[1] = 0x00;
  cmd[2] = 4; // get ATIP
  cmd[7] = 0;
  cmd[8] = dataLen; // data length
  
  if (sendCmd(cmd, 10, NULL, 0, data, dataLen, 0) == 0) {
    if (data[6] & 0x04) {
      diskInfo_.valid.recSpeed = 1;
      spd = (data[16] >> 4) & 0x07;
      diskInfo_.recSpeedLow = spd == 1 ? 2 : 0;
      
      spd = (data[16] & 0x0f);
      diskInfo_.recSpeedHigh = spd >= 1 && spd <= 4 ? spd * 2 : 0;
    }

    if (data[8] >= 80 && data[8] <= 99) {
      diskInfo_.manufacturerId = Msf(data[8], data[9], data[10]);
      diskInfo_.valid.manufacturerId = 1;
    }
  }

  return &diskInfo_;
}


// tries to read catalog number from disk and adds it to 'toc'
// return: 1 if valid catalog number was found, else 0
int GenericMMC::readCatalog(Toc *toc, long startLba, long endLba)
{
  unsigned char cmd[10];
  unsigned char data[24];
  char catalog[14];
  int i;

  if (options_ & OPT_MMC_SCAN_MCN) {
    if (readCatalogScan(catalog, startLba, endLba) == 0) {
      if (catalog[0] != 0) {
	if (toc->catalog(catalog) == 0)
	  return 1;
	else
	  message(-1, "Found illegal MCN data: %s", catalog);
      }
    }
  }
  else {
    memset(cmd, 0, 10);
    memset(data, 0, 24);

    cmd[0] = 0x42; // READ SUB-CHANNEL
    cmd[2] = 1 << 6;
    cmd[3] = 0x02; // get media catalog number
    cmd[8] = 24;   // transfer length
    
    if (sendCmd(cmd, 10, NULL, 0, data, 24) != 0) {
      message(-2, "Cannot get catalog number.");
      return 0;
    }
    
    if (data[8] & 0x80) {
      for (i = 0; i < 13; i++) {
	catalog[i] = data[0x09 + i];
      }
      catalog[13] = 0;
      
      if (toc->catalog(catalog) == 0) {
	return 1;
      }
    }
  }

  return 0;
}

int GenericMMC::readIsrc(int trackNr, char *buf)
{
  unsigned char cmd[10];
  unsigned char data[24];
  int i;

  buf[0] = 0;

  memset(cmd, 0, 10);
  memset(data, 0, 24);

  cmd[0] = 0x42; // READ SUB-CHANNEL
  cmd[2] = 1 << 6;
  cmd[3] = 0x03; // get media catalog number
  cmd[6] = trackNr;
  cmd[8] = 24;   // transfer length

  if (sendCmd(cmd, 10, NULL, 0, data, 24) != 0) {
    message(-2, "Cannot get ISRC code.");
    return 0;
  }

  if (data[8] & 0x80) {
    for (i = 0; i < 12; i++) {
      buf[i] = data[0x09 + i];
    }
    buf[12] = 0;
  }

  return 0;
}

int GenericMMC::analyzeTrack(TrackData::Mode mode, int trackNr, long startLba,
			     long endLba, Msf *indexIncrements,
			     int *indexIncrementCnt, long *pregap,
			     char *isrcCode, unsigned char *ctl)
{
  int ret;

  selectSpeed(0);

  if (options_ & OPT_MMC_NO_SUBCHAN)
    ret = analyzeTrackSearch(mode, trackNr, startLba, endLba, indexIncrements,
			     indexIncrementCnt, pregap, isrcCode, ctl);
  else
    ret = analyzeTrackScan(mode, trackNr, startLba, endLba,
			   indexIncrements, indexIncrementCnt, pregap,
			   isrcCode, ctl);

  if ((options_ & OPT_MMC_NO_SUBCHAN) ||
      (options_ & OPT_MMC_READ_ISRC) ||
      ((options_ & OPT_MMC_USE_PQ) && !(options_ & OPT_MMC_PQ_BCD))) {
    // The ISRC code is usually not usable if the PQ channel data is
    // converted to hex numbers by the drive. Read them with the
    // appropriate command in this case

    *isrcCode = 0;
    if (mode == TrackData::AUDIO)
      readIsrc(trackNr, isrcCode);
  }

  return ret;
}

int GenericMMC::readSubChannels(long lba, long len, SubChannel ***chans,
				Sample *audioData)
{
  int retries = 5;
  unsigned char cmd[12];
  int i;
  long blockLen;

  if (options_ & OPT_MMC_NO_SUBCHAN) {
    blockLen = AUDIO_BLOCK_LEN;
  }
  else {
    if (options_ & OPT_MMC_USE_PQ) 
      blockLen = AUDIO_BLOCK_LEN + 16;
    else
      blockLen = AUDIO_BLOCK_LEN + 96;
  }

  cmd[0] = 0xbe;  // READ CD
  cmd[1] = 0;
  cmd[2] = lba >> 24;
  cmd[3] = lba >> 16;
  cmd[4] = lba >> 8;
  cmd[5] = lba;
  cmd[6] = len >> 16;
  cmd[7] = len >> 8;
  cmd[8] = len;
  cmd[9] = 0xf8;

  if (options_ & OPT_MMC_NO_SUBCHAN) {
    cmd[10] = 0;
  }
  else {
    if (options_ & OPT_MMC_USE_PQ) 
      cmd[10] = 0x02;  // PQ sub-channel data
    else
      cmd[10] = 0x01;  // raw PW sub-channel data
  }

  cmd[11] = 0;

  while (1) {
    if (sendCmd(cmd, 12, NULL, 0,
		transferBuffer_, len * blockLen, retries == 0 ? 1 : 0) != 0) {
      if (retries == 0) 
	return 1;
    }
    else {
      break;
    }

    retries--;
  }

#if 0
  if (lba > 5000) {
    char fname[200];
    sprintf(fname, "testout_%ld", lba);
    FILE *fp = fopen(fname, "w");
    fwrite(transferBuffer_, blockLen, len, fp);
    fclose(fp);
  }
#endif

  if ((options_ & OPT_MMC_NO_SUBCHAN) == 0) {
    unsigned char *buf = transferBuffer_ + AUDIO_BLOCK_LEN;

    for (i = 0; i < len; i++) {
      if (options_ & OPT_MMC_USE_PQ) {
	if (!(options_ & OPT_MMC_PQ_BCD)) {
	  // All numbers in sub-channel data are hex conforming to the
	  // MMC standard. We have to convert them back to BCD for the
	  // 'SubChannel' class.
	  buf[1] = SubChannel::bcd(buf[1]);
	  buf[2] = SubChannel::bcd(buf[2]);
	  buf[3] = SubChannel::bcd(buf[3]);
	  buf[4] = SubChannel::bcd(buf[4]);
	  buf[5] = SubChannel::bcd(buf[5]);
	  buf[6] = SubChannel::bcd(buf[6]);
	  buf[7] = SubChannel::bcd(buf[7]);
	  buf[8] = SubChannel::bcd(buf[8]);
	  buf[9] = SubChannel::bcd(buf[9]);
	}
	
	((PQSubChannel16*)scannedSubChannels_[i])->init(buf);
	if (scannedSubChannels_[i]->type() != SubChannel::QMODE_ILLEGAL) {
	  // the CRC of the sub-channel data is usually invalid -> mark the
	  // sub-channel object that it should not try to verify the CRC
	  scannedSubChannels_[i]->crcInvalid();
	}
      }
      else {
	((PWSubChannel96*)scannedSubChannels_[i])->init(buf);
      }
      
      buf += blockLen;
    }
  }
  
  if (audioData != NULL) {
    unsigned char *p = transferBuffer_;

    for (i = 0; i < len; i++) {
      memcpy(audioData, p, AUDIO_BLOCK_LEN);

      p += blockLen;
      audioData += SAMPLES_PER_BLOCK;
    }
  }

  if (options_ & OPT_MMC_NO_SUBCHAN)
    *chans = NULL;
  else
    *chans = scannedSubChannels_;

  return 0;
}


// Tries to retrieve configuration feature 'feature' and fills data to
// provided buffer 'buf' with maximum length 'bufLen'.
// Return: 0: OK
//         1: feature not available
//         2: SCSI error
int GenericMMC::getFeature(unsigned int feature, unsigned char *buf,
			   unsigned long bufLen, int showMsg)
{
  unsigned char header[8];
  unsigned char *data;
  unsigned char cmd[10];
  unsigned long len;

  memset(cmd, 0, 10);
  memset(header, 0, 8);

  cmd[0] = 0x46; // GET CONFIGURATION
  cmd[1] = 0x02; // return single feature descriptor
  cmd[2] = feature >> 8;
  cmd[3] = feature;
  cmd[8] = 8; // allocation length

  if (sendCmd(cmd, 10, NULL, 0, header, 8, showMsg) != 0) {
    if (showMsg)
      message(-2, "Cannot get feature 0x%x.", feature);
    return 2;
  }

  len = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];

  message(3, "getFeature: data len: %lu", len);

  if (len < 8)
    return 1; // feature not defined

  if (bufLen == 0)
    return 0;

  len -= 4; 

  if (len > bufLen)
    len = bufLen;

  data = new (unsigned char)[len + 8];

  cmd[7] = (len + 8) >> 8;
  cmd[8] = (len + 8);

  if (sendCmd(cmd, 10, NULL, 0, data, len + 8, showMsg) != 0) {
    if (showMsg)
      message(-2, "Cannot get data for feature 0x%x.", feature);
    
    delete[] data;
    return 2;
  }
  
  len = (header[0] << 24) | (header[1] << 16) | (header[2] << 8) | header[3];
  
  message(3, "getFeature: data len: %lu", len);

  if (len < 8) {
    delete[] data;
    return 1; // feature not defined
  }

  len -= 4;

  if (len > bufLen)
    len = bufLen;

  memcpy(buf, data + 8, len);

  delete[] data;
  
  return 0;
}

int GenericMMC::driveInfo(DriveInfo *info, int showErrorMsg)
{
  unsigned char mp[32];

  if (getModePage(0x2a, mp, 32, NULL, NULL, showErrorMsg) != 0) {
    if (showErrorMsg) {
      message(-2, "Cannot retrieve CD capabilities mode page.");
    }
    return 1;
  }

  info->accurateAudioStream = mp[5] & 0x02 ? 1 : 0;

  info->maxReadSpeed = (mp[8] << 8) | mp[9];
  info->currentReadSpeed = (mp[14] << 8) | mp[15];

  info->maxWriteSpeed = (mp[18] << 8) | mp[19];
  info->currentWriteSpeed = (mp[20] << 8) | mp[21];

#if 0
  unsigned char cdMasteringFeature[8];
  if (getFeature(0x2e, cdMasteringFeature, 8, 1) == 0) {
    message(0, "Feature: %x %x %x %x %x %x %x %x", cdMasteringFeature[0],
	    cdMasteringFeature[1], cdMasteringFeature[2],
	    cdMasteringFeature[3], cdMasteringFeature[4],
	    cdMasteringFeature[5], cdMasteringFeature[6],
	    cdMasteringFeature[7]);
  }
#endif

  return 0;
}

TrackData::Mode GenericMMC::getTrackMode(int, long trackStartLba)
{
  unsigned char cmd[12];
  unsigned char data[AUDIO_BLOCK_LEN];

  memset(cmd, 0, 12);
  cmd[0] = 0xbe;  // READ CD

  cmd[2] = trackStartLba >> 24;
  cmd[3] = trackStartLba >> 16;
  cmd[4] = trackStartLba >> 8;
  cmd[5] = trackStartLba;

  cmd[8] = 1;

  cmd[9] = 0xf8;

  if (sendCmd(cmd, 12, NULL, 0, data, AUDIO_BLOCK_LEN) != 0) {
    message(-2, "Cannot read sector of track.");
    return TrackData::MODE0;
  }

  if (memcmp(CdrDriver::syncPattern, data, 12) != 0) {
    // cannot be a data sector
    return TrackData::MODE0;
  }

  TrackData::Mode mode = determineSectorMode(data + 12);

  if (mode == TrackData::MODE0) {
    // illegal
    message(-2, "Found illegal mode in sector %ld.", trackStartLba);
  }

  return mode;
}

CdRawToc *GenericMMC::getRawToc(int sessionNr, int *len)
{
  unsigned char cmd[10];
  unsigned short dataLen;
  unsigned char *data = NULL;;
  unsigned char reqData[4]; // buffer for requestion the actual length
  unsigned char *p;
  int i, entries;
  CdRawToc *rawToc;

  assert(sessionNr >= 1);

  // read disk toc length
  memset(cmd, 0, 10);
  cmd[0] = 0x43; // READ TOC
  cmd[2] = 2;
  cmd[6] = sessionNr;
  cmd[8] = 4;

  if (sendCmd(cmd, 10, NULL, 0, reqData, 4) != 0) {
    message(-2, "Cannot read disk toc.");
    return NULL;
  }

  dataLen = ((reqData[0] << 8) | reqData[1]) + 2;
  
  message(3, "Raw toc data len: %d", dataLen);

  data = new (unsigned char)[dataLen];
  
  // read disk toc
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if (sendCmd(cmd, 10, NULL, 0, data, dataLen) != 0) {
    message(-2, "Cannot read disk toc.");
    delete[] data;
    return NULL;
  }

  entries = (((data[0] << 8) | data[1]) - 2) / 11;

  rawToc = new CdRawToc[entries];

  for (i = 0, p = data + 4; i < entries; i++, p += 11 ) {
#if 0
    message(0, "%d %02x %02d %2x %02d:%02d:%02d %02d %02d:%02d:%02d",
	    p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10]);
#endif
    rawToc[i].sessionNr = p[0];
    rawToc[i].adrCtl = p[1];
    rawToc[i].point = p[3];
    rawToc[i].pmin = p[8];
    rawToc[i].psec = p[9];
    rawToc[i].pframe = p[10];
  }

  delete[] data;

  *len = entries;

  return rawToc;
}

long GenericMMC::readTrackData(TrackData::Mode mode, long lba, long len,
			       unsigned char *buf)
{
  long i;
  long inBlockLen = AUDIO_BLOCK_LEN;
  unsigned char cmd[12];
  TrackData::Mode actMode;
  int ok = 0;
  const unsigned char *sense;
  int senseLen;
  int softError;

  memset(cmd, 0, 12);

  cmd[0] = 0xbe; // READ CD
  cmd[1] = 0;
  cmd[2] = lba >> 24;
  cmd[3] = lba >> 16;
  cmd[4] = lba >> 8;
  cmd[5] = lba;
  cmd[9] = 0xf8;

  while (len > 0 && !ok) {
    cmd[6] = len >> 16;
    cmd[7] = len >> 8;
    cmd[8] = len;

    memset(transferBuffer_, 0, len * inBlockLen);

    switch (sendCmd(cmd, 12, NULL, 0, transferBuffer_, len * inBlockLen, 0)) {
    case 0:
      ok = 1;
      break;

    case 2:
      softError = 0;
      sense = scsiIf_->getSense(senseLen);

      if (senseLen > 0x0c) {
	if ((sense[2] & 0x0f) == 5) { // Illegal request
	  switch (sense[12]) {
	  case 0x63: // End of user area encountered on this track
	  case 0x64: // Illegal mode for this track
	    softError = 1;
	    break;
	  }
	}
	else if ((sense[2] & 0x0f) == 3) { // Medium error
	  switch (sense[12]) {
	  case 0x02: // No seek complete, sector not found
	  case 0x11: // L-EC error
	    return -2;
	    break;
	  }
	}
      }

      if (!softError) {
	scsiIf_->printError();
	return -1;
      }
      break;

    default:
      message(-2, "Read error at LBA %ld, len %ld", lba, len);
      return -1;
      break;
    }

    if (!ok) {
      len--;
    }
  }

  unsigned char *sector = transferBuffer_;
  for (i = 0; i < len; i++) {
     

    if (memcmp(CdrDriver::syncPattern, sector, 12) != 0) {
      // can't be a data block
      message(3, "Stopped because no sync pattern found.");
      message(3, "%d %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", i, 
	      sector[0], sector[1], sector[2], sector[3], sector[4], sector[5],
	      sector[6], sector[7], sector[8], sector[9], sector[10],
	      sector[11]);
      return i;
    }

    actMode = determineSectorMode(sector + 12);

    if (!(actMode == mode ||
	  (mode == TrackData::MODE2_FORM_MIX &&
	   (actMode == TrackData::MODE2_FORM1 ||
	    actMode == TrackData::MODE2_FORM2)) ||

	  (mode == TrackData::MODE1_RAW && actMode == TrackData::MODE1) ||

	  (mode == TrackData::MODE2_RAW &&
	   (actMode == TrackData::MODE2 ||
	    actMode == TrackData::MODE2_FORM1 ||
	    actMode == TrackData::MODE2_FORM2)))) {
      message(3, "Sector with not matching mode %s found.",
	      TrackData::mode2String(actMode));
      message(3, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
	      sector[12], sector[13], sector[14], sector[15], sector[16],
	      sector[17], sector[18], sector[19], sector[20], sector[21],
	      sector[22], sector[23]);
      return i;
    }

    if (buf != NULL) {
      switch (mode) {
      case TrackData::MODE1:
	memcpy(buf, sector + 16, MODE1_BLOCK_LEN);
	buf += MODE1_BLOCK_LEN;
	break;
      case TrackData::MODE1_RAW:
	memcpy(buf, sector, AUDIO_BLOCK_LEN);
	buf += AUDIO_BLOCK_LEN;
	break;
      case TrackData::MODE2:
      case TrackData::MODE2_FORM_MIX:
	memcpy(buf, sector + 16, MODE2_BLOCK_LEN);
	buf += MODE2_BLOCK_LEN;
	break;
      case TrackData::MODE2_FORM1:
	memcpy(buf, sector + 24, MODE2_FORM1_DATA_LEN);
	buf += MODE2_FORM1_DATA_LEN;
	break;
      case TrackData::MODE2_FORM2:
	memcpy(buf, sector + 24, MODE2_FORM2_DATA_LEN);
	buf += MODE2_FORM2_DATA_LEN;
	break;
      case TrackData::MODE2_RAW:
	memcpy(buf, sector, AUDIO_BLOCK_LEN);
	buf += AUDIO_BLOCK_LEN;
	break;
      case TrackData::MODE0:
      case TrackData::AUDIO:
	message(-3, "GenericMMC::readTrackData: Illegal mode.");
	return 0;
	break;
      }
    }

    sector += inBlockLen;
  }

  return len;
}

int GenericMMC::readAudioRange(int fd, long start, long end, int startTrack,
			       int endTrack, TrackInfo *info)
{
  if (!onTheFly_) {
    if ((options_ & OPT_MMC_NO_SUBCHAN) ||
	(options_ & OPT_MMC_READ_ISRC) ||
	((options_ & OPT_MMC_USE_PQ) && !(options_ & OPT_MMC_PQ_BCD))) {
      int t;
      long pregap = 0;

      // The ISRC code is usually not usable if the PQ channel data is
      // converted to hex numbers by the drive. Read them with the
      // appropriate command in this case

      message(1, "Analyzing...");


      for (t = startTrack; t <= endTrack; t++) {
	message(1, "Track %d...", t + 1);

	sendReadCdProgressMsg(RCD_ANALYZING, t + 1, 0);

	if (options_ & OPT_MMC_NO_SUBCHAN) {
	  // we have to use the binary search method to find pre-gap and
	  // index marks if the drive cannot read sub-channel data
	  if (!fastTocReading_) {
	    long slba, elba;
	    int i, indexCnt;
	    Msf index[98];
	    unsigned char ctl;

	    if (pregap > 0)
	      message(1, "Found pre-gap: %s", Msf(pregap).str());

	    slba = info[t].start;
	    if (info[t].mode == info[t + 1].mode)
	      elba = info[t + 1].start;
	    else
	      elba = info[t + 1].start - 150;
	    
	    pregap = 0;
	    if (analyzeTrackSearch(TrackData::AUDIO, t + 1, slba, elba,
				   index, &indexCnt, &pregap, info[t].isrcCode,
				   &ctl) != 0)
	      return 1;
	  
	    for (i = 0; i < indexCnt; i++)
	      info[t].index[i] = index[i].lba();
	    
	    info[t].indexCnt = indexCnt;
	    
	    if (t < endTrack)
	      info[t + 1].pregap = pregap;
	  }
	  else {
	    info[t].indexCnt = 0;
	    info[t + 1].pregap = 0;
	  }
	}


	info[t].isrcCode[0] = 0;
	readIsrc(t + 1, info[t].isrcCode);
	if (info[t].isrcCode[0] != 0)
	  message(1, "Found ISRC code.");

	sendReadCdProgressMsg(RCD_ANALYZING, t + 1, 1000);
      }

      message(1, "Reading...");
    }
  }

  return CdrDriver::readAudioRangeParanoia(fd, start, end, startTrack,
					   endTrack, info);
}

int GenericMMC::getTrackIndex(long lba, int *trackNr, int *indexNr,
			      unsigned char *ctl)
{
  unsigned char cmd[12];
  unsigned short dataLen = 0x30;
  unsigned char data[0x30];
  int waitLoops = 10;
  int waitFailed = 0;

  // play one audio block
  memset(cmd, 0, 10);
  cmd[0] = 0x45; // PLAY AUDIO
  cmd[2] = lba >> 24;
  cmd[3] = lba >> 16;
  cmd[4] = lba >> 8;
  cmd[5] = lba;
  cmd[7] = 0;
  cmd[8] = 1;

  if (sendCmd(cmd, 10, NULL, 0, NULL, 0) != 0) {
    message(-2, "Cannot play audio block.");
    return 1;
  }

  // wait until the play command finished
  memset(cmd, 0, 12);
  cmd[0] = 0xbd; // MECHANISM STATUS
  cmd[9] = 8;

  while (waitLoops > 0) {
    if (sendCmd(cmd, 12, NULL, 0, data, 8, 0) == 0) {
      //message(0, "%d, %x", waitLoops, data[1]);
      if ((data[1] >> 5) == 1) // still playing?
	waitLoops--;
      else
	waitLoops = 0;
    }
    else {
      waitFailed = 1;
      waitLoops = 0;
    }
  }

  if (waitFailed) {
    // The play operation immediately returns success status and the waiting
    // loop above failed. Wait here for a while until the desired block is
    // played. It takes ~13 msecs to play a block but access time is in the
    // order of several 100 msecs
    mSleep(300);
  }

  // read sub channel information
  memset(cmd, 0, 10);
  cmd[0] = 0x42; // READ SUB CHANNEL
  cmd[2] = 0x40; // get sub channel data
  cmd[3] = 0x01; // get sub Q channel data
  cmd[6] = 0;
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if (sendCmd(cmd, 10, NULL, 0, data, dataLen) != 0) {
    message(-2, "Cannot read sub Q channel data.");
    return 1;
  }

  *trackNr = data[6];
  *indexNr = data[7];
  if (ctl != NULL) {
    *ctl = data[5] & 0x0f;
  }

  //message(0, "%d %d", *trackNr, *indexNr);

  return 0;
}
