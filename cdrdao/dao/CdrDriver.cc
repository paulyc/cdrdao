/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2000 Andreas Mueller <mueller@daneb.ping.de>
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
 * Revision 1.2  2000/04/23 16:29:49  andreasm
 * Updated to state of my private development environment.
 *
 * Revision 1.17  1999/12/15 20:31:46  mueller
 * Added remote messages for 'read-cd' progress used by a GUI.
 *
 * Revision 1.16  1999/12/12 16:23:37  mueller
 * Added density code argument to 'setBlockSize'.
 * Updated driver selection table.
 *
 * Revision 1.15  1999/12/12 14:15:37  mueller
 * Updated driver selection table.
 *
 * Revision 1.14  1999/11/07 09:14:59  mueller
 * Release 1.1.3
 *
 * Revision 1.13  1999/04/05 18:47:11  mueller
 * Added CD-TEXT support.
 *
 * Revision 1.12  1999/03/27 20:52:17  mueller
 * Added data track support for writing and toc analysis.
 *
 * Revision 1.11  1999/02/06 20:41:21  mueller
 * Added virtual member function 'checkToc()'.
 *
 * Revision 1.10  1998/10/24 14:28:59  mueller
 * Changed prototype of 'readDiskToc()'. It now accepts the name of the
 * audio file that should be placed into the generated toc-file.
 * Added virtual function 'readDisk()' that for reading disk toc and
 * ripping audio data at the same time.
 *
 * Revision 1.9  1998/10/03 15:10:38  mueller
 * Added function 'writeZeros()'.
 * Updated function 'writeData()'.
 *
 * Revision 1.8  1998/09/27 19:19:18  mueller
 * Added retrieval of control nibbles for track with 'analyzeTrack()'.
 * Added multi session mode.
 *
 * Revision 1.7  1998/09/22 19:15:13  mueller
 * Removed memory allocations during write process.
 *
 * Revision 1.6  1998/09/07 15:20:20  mueller
 * Reorganized read-toc related code.
 *
 * Revision 1.5  1998/09/06 13:34:22  mueller
 * Use 'message()' for printing messages.
 *
 * Revision 1.4  1998/08/30 19:12:19  mueller
 * Added supressing of error messages for some commands.
 * Added structure 'DriveInfo'.
 *
 * Revision 1.3  1998/08/25 19:24:07  mueller
 * Moved basic index extraction algorithm for read-toc to this class.
 * Added vendor codes.
 *
 */

static char rcsid[] = "$Id: CdrDriver.cc,v 1.3 2000-06-06 22:26:13 andreasm Exp $";

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include "CdrDriver.h"
#include "PWSubChannel96.h"
#include "Toc.h"
#include "util.h"
#include "CdTextItem.h"
#include "remote.h"

// all drivers
#include "CDD2600.h"
#include "PlextorReader.h"
#include "PlextorReaderScan.h"
#include "GenericMMC.h"
#include "GenericMMCraw.h"
#include "RicohMP6200.h"
#include "TaiyoYuden.h"
#include "YamahaCDR10x.h"
#include "TeacCdr55.h"
#include "SonyCDU920.h"
#include "SonyCDU948.h"
#include "ToshibaReader.h"

// Paranoia DAE related
#include "cdda_interface.h"
#include "../paranoia/cdda_paranoia.h"


typedef CdrDriver *(*CdrDriverConstructor)(ScsiIf *, unsigned long);

struct DriverSelectTable {
  char *driverId;
  char *vendor;
  char *model;
  unsigned long options;
};

struct DriverTable {
  char *driverId;
  CdrDriverConstructor constructor;
};

static DriverSelectTable READ_DRIVER_TABLE[] = {
{ "generic-mmc", "ASUS", "CD-S340", 0 },
{ "generic-mmc", "ASUS", "CD-S400", 0 },
{ "generic-mmc", "E-IDE", "CD-ROM 36X/AKU", 0 },
{ "generic-mmc", "HITACHI", "CDR-8435", OPT_MMC_NO_SUBCHAN },
{ "generic-mmc", "KENWOOD", "CD-ROM UCR-415", OPT_DRV_GET_TOC_GENERIC },
{ "generic-mmc", "LG", "CD-ROM CRD-8480C", OPT_MMC_NO_SUBCHAN },
{ "generic-mmc", "LITEON", "CD-ROM", OPT_DRV_GET_TOC_GENERIC },
{ "generic-mmc", "MATSHITA", "CD-ROM CR-588", 0 },
{ "generic-mmc", "MEMOREX", "CD-233E", 0 },
{ "generic-mmc", "PHILIPS", "CD-ROM PCCD052", 0 },
{ "generic-mmc", "PHILIPS", "E-IDE CD-ROM 36X", 0 },
{ "generic-mmc", "PIONEER", "DVD-103", OPT_MMC_USE_PQ|OPT_MMC_PQ_BCD },
{ "generic-mmc", "SONY", "CD-ROM CDU31A-02", 0 },
{ "generic-mmc", "TEAC", "CD-524E", OPT_DRV_GET_TOC_GENERIC },
{ "generic-mmc", "TEAC", "CD-532E", OPT_MMC_USE_PQ|OPT_MMC_PQ_BCD },
{ "generic-mmc", "TOSHIBA", "CD-ROM XM-3206B", 0 },
{ "generic-mmc", "TOSHIBA", "CD-ROM XM-6302B", 0 },
{ "plextor", "HITACHI", "DVD-ROM GD-2500", 0 },
{ "plextor", "MATSHITA", "CD-ROM CR-506", OPT_PLEX_DAE_D4_12 },
{ "plextor", "NAKAMICH", "MJ-5.16S", 0 },
{ "plextor", "PIONEER", "CD-ROM DR-U03", OPT_DRV_GET_TOC_GENERIC },
{ "plextor", "PIONEER", "CD-ROM DR-U06", OPT_DRV_GET_TOC_GENERIC },
{ "plextor", "PIONEER", "CD-ROM DR-U10", OPT_DRV_GET_TOC_GENERIC },
{ "plextor", "PIONEER", "CD-ROM DR-U12", OPT_DRV_GET_TOC_GENERIC },
{ "plextor", "PIONEER", "CD-ROM DR-U16", OPT_DRV_GET_TOC_GENERIC },
{ "plextor", "PIONEER", "DVD-303", 0 },
{ "plextor", "SAF", "CD-R2006PLUS", 0 },
{ "plextor", "SONY", "CD-ROM", 0 },
{ "plextor", "SONY", "CD-ROM CDU-76", 0 },
{ "plextor-scan", "PLEXTOR", "CD-ROM", 0 },
{ "plextor-scan", "TEAC", "CD-ROM CD-532S", OPT_PLEX_USE_PQ|OPT_PLEX_PQ_BCD },
{ "toshiba", "TOSHIBA", "CD-ROM XM-5701TA", 0 },
{ "toshiba", "TOSHIBA", "CD-ROM XM-6201TA", 0 },
{ "toshiba", "TOSHIBA", "CD-ROM XM-6401TA", 0 },
{ NULL, NULL, NULL, 0 }};

static DriverSelectTable WRITE_DRIVER_TABLE[] = {
{ "cdd2600", "HP", "CD-Writer 4020", 0 },
{ "cdd2600", "HP", "CD-Writer 6020", 0 },
{ "cdd2600", "IMS", "CDD2000", 0 },
{ "cdd2600", "KODAK", "PCD-225", 0 },
{ "cdd2600", "PHILIPS", "CDD2000", 0 },
{ "cdd2600", "PHILIPS", "CDD2600", 0 },
{ "cdd2600", "PHILIPS", "CDD522", 0 },
{ "generic-mmc", "AOPEN", "CRW9624", 0 },
{ "generic-mmc", "GENERIC", "CRD-RW2", 0 },
{ "generic-mmc", "HP", "CD-Writer+ 7570", OPT_MMC_CD_TEXT },
{ "generic-mmc", "HP", "CD-Writer+ 8100", OPT_MMC_CD_TEXT },
{ "generic-mmc", "HP", "CD-Writer+ 8200", OPT_MMC_CD_TEXT },
{ "generic-mmc", "HP", "CD-Writer+ 9100", OPT_MMC_CD_TEXT },
{ "generic-mmc", "HP", "CD-Writer+ 9200", OPT_MMC_CD_TEXT },
{ "generic-mmc", "MATSHITA", "CD-R   CW-7502", 0 },
{ "generic-mmc", "MATSHITA", "CD-R   CW-7503", 0 },
{ "generic-mmc", "MATSHITA", "CD-R   CW-7582", 0 },
{ "generic-mmc", "MATSHITA", "CDRRW01", 0 },
{ "generic-mmc", "MEMOREX", "CD-RW4224", 0 },
{ "generic-mmc", "MITSUMI", "CR-4801", 0 },
{ "generic-mmc", "PANASONIC", "CD-R   CW-7582", 0 },
{ "generic-mmc", "PHILIPS", "PCA460RW", 0 },
{ "generic-mmc", "PLEXTOR", "CD-R   PX-R412", OPT_MMC_USE_PQ|OPT_MMC_READ_ISRC },
{ "generic-mmc", "PLEXTOR", "CD-R   PX-R820", 0 },
{ "generic-mmc", "PLEXTOR", "CD-R   PX-W124", 0 },
{ "generic-mmc", "PLEXTOR", "CD-R   PX-W4220", OPT_MMC_CD_TEXT },
{ "generic-mmc", "PLEXTOR", "CD-R   PX-W8220", OPT_MMC_CD_TEXT },
{ "generic-mmc", "PLEXTOR", "CD-R   PX-W8432", OPT_MMC_CD_TEXT },
{ "generic-mmc", "RICOH", "CD-R/RW MP7060", OPT_MMC_CD_TEXT },
{ "generic-mmc", "SAF", "CD-R 8020", 0 },
{ "generic-mmc", "SAF", "CD-RW6424", 0 },
{ "generic-mmc", "SONY", "CRX100", OPT_MMC_CD_TEXT },
{ "generic-mmc", "SONY", "CRX120", OPT_MMC_CD_TEXT },
{ "generic-mmc", "SONY", "CRX140", OPT_MMC_CD_TEXT },
{ "generic-mmc", "TEAC", "CD-R56", OPT_MMC_USE_PQ|OPT_MMC_CD_TEXT },
{ "generic-mmc", "TEAC", "CD-R58", OPT_MMC_USE_PQ|OPT_MMC_CD_TEXT },
{ "generic-mmc", "TEAC", "CD-W54E", 0 },
{ "generic-mmc", "TRAXDATA", "CDRW4260", 0 },
{ "generic-mmc", "WAITEC", "WT624", 0 },
{ "generic-mmc", "YAMAHA", "CDR200", 0 },
{ "generic-mmc", "YAMAHA", "CDR400", 0 },
{ "generic-mmc", "YAMAHA", "CRW2260", 0 },
{ "generic-mmc", "YAMAHA", "CRW4001", 0 },
{ "generic-mmc", "YAMAHA", "CRW4260", 0 },
{ "generic-mmc", "YAMAHA", "CRW4416", 0 },
{ "generic-mmc", "YAMAHA", "CRW6416", 0 },
{ "generic-mmc", "YAMAHA", "CRW8424", 0 },
{ "generic-mmc-raw", "ATAPI", "CD-R/RW 4X4X32", 0 },
{ "generic-mmc-raw", "ATAPI", "CD-R/RW CRW6206A", 0 },
{ "generic-mmc-raw", "BTC", "BCE621E", 0 },
{ "generic-mmc-raw", "HP", "CD-Writer+ 7100", 0 },
{ "generic-mmc-raw", "HP", "CD-Writer+ 7200", 0 },
{ "generic-mmc-raw", "IDE-CD", "R/RW 2x2x24", 0 },
{ "generic-mmc-raw", "IOMEGA", "ZIPCD 4x650", 0 },
{ "generic-mmc-raw", "MEMOREX", "CDRW-2216", 0 },
{ "generic-mmc-raw", "MEMOREX", "CR-622", 0 },
{ "generic-mmc-raw", "MEMOREX", "CRW-1662", 0 },
{ "generic-mmc-raw", "MITSUMI", "CR-4802", 0 },
{ "generic-mmc-raw", "PHILIPS", "CDD3600", 0 },
{ "generic-mmc-raw", "PHILIPS", "CDD3610", 0 },
{ "generic-mmc-raw", "PHILIPS", "PCRW404", 0 },
{ "generic-mmc-raw", "TRAXDATA", "CDRW2260+", 0 },
{ "generic-mmc-raw", "TRAXDATA", "CRW2260 PRO", 0 },
{ "generic-mmc-raw", "WAITEC", "WT4424", 0 },
{ "ricoh-mp6200", "PHILIPS", "OMNIWRITER26", 0 },
{ "ricoh-mp6200", "RICOH", "MP6200", 0 },
{ "ricoh-mp6200", "RICOH", "MP6201", 0 },
{ "sony-cdu920", "SONY", "CD-R   CDU920", 0 },
{ "sony-cdu920", "SONY", "CD-R   CDU924", 0 },
{ "sony-cdu948", "SONY", "CD-R   CDU948", 0 },
{ "taiyo-yuden", "T.YUDEN", "CD-WO EW-50", 0 },
{ "teac-cdr55", "JVC", "R2626", 0 },
{ "teac-cdr55", "SAF", "CD-R2006PLUS", 0 },
{ "teac-cdr55", "SAF", "CD-R4012", 0 },
{ "teac-cdr55", "SAF", "CD-RW 226", 0 },
{ "teac-cdr55", "TEAC", "CD-R50", 0 },
{ "teac-cdr55", "TEAC", "CD-R55", 0 },
{ "teac-cdr55", "TRAXDATA", "CDR4120", 0 },
{ "yamaha-cdr10x", "YAMAHA", "CDR100", 0 },
{ "yamaha-cdr10x", "YAMAHA", "CDR102", 0 },
{ NULL, NULL, NULL, 0 }};

static DriverTable DRIVERS[] = {
{ "cdd2600", &CDD2600::instance },
{ "generic-mmc", &GenericMMC::instance },
{ "generic-mmc-raw", &GenericMMCraw::instance },
{ "plextor", &PlextorReader::instance },
{ "plextor-scan", &PlextorReaderScan::instance },
{ "ricoh-mp6200", &RicohMP6200::instance },
{ "sony-cdu920", &SonyCDU920::instance },
{ "sony-cdu948", &SonyCDU948::instance },
{ "taiyo-yuden", &TaiyoYuden::instance },
{ "teac-cdr55", &TeacCdr55::instance },
{ "toshiba", &ToshibaReader::instance },
{ "yamaha-cdr10x", &YamahaCDR10x::instance },
{ NULL, NULL }};


struct CDRVendorTable {
  char m1, s1, f1; // 1st vendor code
  char m2, s2, f2; // 2nd vendor code
  const char *id;  // vendor ID
};

static CDRVendorTable VENDOR_TABLE[] = {
  // permanent codes
  { 97,28,30,  97,46,50, "Auvistar Industry Co.,Ltd." },
  { 97,26,60,  97,46,60, "CMC Magnetics Corporation" },
  { 97,23,10,  0,0,0,    "Doremi Media Co., Ltd." },
  { 97,26,00,  97,45,00, "FORNET INTERNATIONAL PTE LTD." },
  { 97,46,40,  97,46,40, "FUJI Photo Film Co., Ltd." },
  { 97,26,40,  0,0,0,    "FUJI Photo Film Co., Ltd." },
  { 97,28,10,  97,49,10, "GIGASTORAGE CORPORATION" },
  { 97,25,20,  97,47,10, "Hitachi Maxell, Ltd." },
  { 97,27,40,  97,48,10, "Kodak Japan Limited" },
  { 97,26,50,  97,48,60, "Lead Data Inc." },
  { 97,27,50,  97,48,50, "Mitsui Chemicals, Inc." },
  { 97,34,20,  97,50,20, "Mitsubishi Chemical Corporation" },
  { 97,28,20,  97,46,20, "Multi Media Masters & Machinary SA" },
  { 97,21,40,  0,0,0,    "Optical Disc Manufacturing Equipment" },
  { 97,27,30,  97,48,30, "Pioneer Video Corporation" },
  { 97,27,10,  97,48,20, "Plasmon Data systems Ltd." },
  { 97,26,10,  97,47,40, "POSTECH Corporation" },
  { 97,27,20,  97,47,20, "Princo Corporation" },
  { 97,32,10,  0,0,0,    "Prodisc Technology Inc." },
  { 97,27,60,  97,48,00, "Ricoh Company Limited" },
  { 97,31,00,  97,47,50, "Ritek Co." },
  { 97,26,20,  0,0,0,    "SKC Co., Ltd." },
  { 97,24,10,  0,0,0,    "SONY Corporation" },
  { 97,24,00,  97,46,00, "Taiyo Yuden Company Limited" },
  { 97,32,00,  97,49,00, "TDK Corporation" },
  { 97,25,60,  97,45,60, "Xcitek Inc." },

  // tentative codes
  { 97,22,60,  97,45,20, "Acer Media Technology, Inc" },
  { 97,25,50,  0,0,0,    "AMS Technology Inc." },
  { 97,23,30,  0,0,0,    "AUDIO DISTRIBUTORS CO., LTD." },
  { 97,30,10,  97,50,30, "CDA Datentraeger Albrechts GmbH" },
  { 97,22,40,  97,45,40, "CIS Technology Inc." },
  { 97,24,20,  97,46,30, "Computer Support Italy s.r.l." },
  { 97,23,60,  0,0,0,    "Customer Pressing Oosterhout" },
  { 97,27,00,  97,48,40, "DIGITAL STORAGE TECHNOLOGY CO.,LTD" },
  { 97,31,30,  97,51,10, "Grand Advance Technology Ltd." },
  { 97,24,50,  97,45,50, "Guann Yinn Co.,Ltd." },
  { 97,25,30,  97,51,20, "INFODISC Technology Co., Ltd." },
  { 97,28,40,  97,49,20, "King Pro Mediatek Inc." },
  { 97,23,00,  97,49,60, "Matsushita Electric Industrial Co., Ltd." },
  { 97,15,20,  0,0,0,    "Mitsubishi Chemical Corporation" },
  { 97,23,20,  0,0,0,    "Nacar Media sr" },
  { 97,26,30,  0,0,0,    "OPTICAL DISC CORPRATION" },
  { 97,28,00,  97,49,30, "Opti.Me.S. S.p.A." },
  { 97,23,50,  0,0,0,    "OPTROM.INC." },
  { 97,47,60,  0,0,0,    "Prodisc Technology Inc." },
  { 97,15,10,  0,0,0,    "Ritek Co." },
  { 97,29,00,  0,0,0,    "Taeil Media Co.,Ltd." },
  { 97,15,00,  0,0,0,    "TDK Corporation." },
  { 97,24,30,  97,45,10, "UNITECH JAPAN INC." },
  { 97,29,10,  97,50,10, "Vanguard Disc Inc." },
  { 97,22,00,  0,0,0,    "Woongjin Media corp." },

  { 0, 0, 0,  0, 0, 0,  NULL}
};

unsigned char CdrDriver::syncPattern[12] = {
  0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0
};

char CdrDriver::REMOTE_MSG_SYNC_[4] = { 0xff, 0x00, 0xff, 0x00 };

const char *CdrDriver::selectDriver(int readWrite, const char *vendor,
				    const char *model, unsigned long *options)
{
  DriverSelectTable *run;
  DriverSelectTable *match = NULL;
  unsigned int matchLen = 0;

  run = (readWrite == 0) ? READ_DRIVER_TABLE : WRITE_DRIVER_TABLE;

  while (run->driverId != NULL) {
    if (strcmp(run->vendor, vendor) == 0 && 
	strstr(model, run->model) != NULL) {
      if (match == NULL || strlen(run->model) > matchLen) {
	match = run;
	matchLen = strlen(run->model);
      }
    }
    run++;
  }

  if (match != NULL) {
    *options = match->options;
    return match->driverId;
  }

  return NULL;
}


CdrDriver *CdrDriver::createDriver(const char *driverId, unsigned long options,
				   ScsiIf *scsiIf)
{
  DriverTable *run = DRIVERS;

  while (run->driverId != NULL) {
    if (strcmp(run->driverId, driverId) == 0) 
      return run->constructor(scsiIf, options);

    run++;
  }

  return NULL;
}

void CdrDriver::printDriverIds()
{
  DriverTable *run = DRIVERS;

  while (run->driverId != NULL) {
    message(0, "%s", run->driverId);
    run++;
  }
}

CdrDriver::CdrDriver(ScsiIf *scsiIf, unsigned long options) 
{
  size16 byteOrderTest = 1;
  char *byteOrderTestP = (char*)&byteOrderTest;

  options_ = options;
  scsiIf_ = scsiIf;
  toc_ = NULL;

  if (*byteOrderTestP == 1)
    hostByteOrder_ = 0; // little endian
  else
    hostByteOrder_ = 1; // big endian

  audioDataByteOrder_ = 0; // default to little endian

  fastTocReading_ = 0;
  rawDataReading_ = 0;
  padFirstPregap_ = 1;
  onTheFly_ = 0;
  onTheFlyFd_ = -1;
  multiSession_ = 0;
  encodingMode_ = 0;
  remote_ = 0;

  blockLength_ = 0;
  blocksPerWrite_ = 0;
  zeroBuffer_ = NULL;

  transferBuffer_ = new (unsigned char)[scsiIf_->maxDataLen()];

  maxScannedSubChannels_ = scsiIf_->maxDataLen() / (AUDIO_BLOCK_LEN + 96);
  scannedSubChannels_ = new (SubChannel*)[maxScannedSubChannels_];

  paranoia_ = NULL;
  paranoiaDrive_ = NULL;
  paranoiaMode(3); // full paranoia but allow skip
}

CdrDriver::~CdrDriver()
{
  toc_ = NULL;

  delete[] zeroBuffer_;
  zeroBuffer_ = NULL;

  delete[] transferBuffer_;
  transferBuffer_ = NULL;

  delete [] scannedSubChannels_;
  scannedSubChannels_ = NULL;
}

// Sets multi session mode. 0: close session, 1: open next session
// Return: 0: OK
//         1: multi session not supported by driver
int CdrDriver::multiSession(int m)
{
  multiSession_ = m != 0 ? 1 : 0;

  return 0;
}


void CdrDriver::onTheFly(int fd)
{
  if (fd >= 0) {
    onTheFly_ = 1;
    onTheFlyFd_ = fd;
  }
  else {
    onTheFly_ = 0;
    onTheFlyFd_ = -1;
  }
}

void CdrDriver::remote(int f)
{
  remote_ = (f != 0 ? 1 : 0);

  if (remote_) {
    int flags;
    int fd = 3;

    // switch FD 3 to non blocking IO mode
    if ((flags = fcntl(fd, F_GETFL)) == -1) {
      message(-1, "Cannot get flags of remote stream: %s", strerror(errno));
      return;
    }

    flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) < 0) {
      message(-1, "Cannot set flags of remote stream: %s", strerror(errno));
    }
  }
}

int CdrDriver::cdrVendor(Msf &code, const char **vendorId, 
			 const char **mediumType)
{
  CDRVendorTable *run = VENDOR_TABLE;

  *vendorId = NULL;

  char m = code.min();
  char s = code.sec();
  char f = code.frac();

  char type = f % 10;
  f -= type;

  while (run->id != NULL) {
    if ((run->m1 == m && run->s1 == s && run->f1 == f) ||
	(run->m2 == m && run->s2 == s && run->f2 == f)) {
      *vendorId = run->id;
      break;
    }
    run++;
  }

  if (*vendorId != NULL) {
    if (type < 5) {
      *mediumType = "Long Strategy Type, e.g. Cyanine";
    }
    else {
      *mediumType = "Short Strategy Type, e.g. Phthalocyanine";
    }

    return 1;
  }

  return 0;
}

// Sends SCSI command via 'scsiIf_'.
// return: see 'ScsiIf::sendCmd()'
int CdrDriver::sendCmd(const unsigned char *cmd, int cmdLen,
		       const unsigned char *dataOut, int dataOutLen,
		       unsigned char *dataIn, int dataInLen,
		       int showErrorMsg) const
{
  return scsiIf_->sendCmd(cmd, cmdLen, dataOut, dataOutLen, dataIn,
			  dataInLen, showErrorMsg);
}

// checks if unit is ready
// return: 0: OK
//         1: scsi command failed
//         2: not ready
//         3: not ready, no disk in drive
//         4: not ready, tray out

int CdrDriver::testUnitReady(int ignoreUnitAttention) const
{
  unsigned char cmd[6];
  const unsigned char *sense;
  int senseLen;

  memset(cmd, 0, 6);

  switch (scsiIf_->sendCmd(cmd, 6, NULL, 0, NULL, 0, 0)) {
  case 1:
    return 1;

  case 2:
    sense = scsiIf_->getSense(senseLen);
    
    int code = sense[2] & 0x0f;

    if (code == 0x02) {
      // not ready
      return 2;
    }
    else if (code != 0x06) {
      scsiIf_->printError();
      return 1;
    }
    else {
      return 0;
    }
  }

  return 0;
}

int CdrDriver::speed2Mult(int speed)
{
  return speed / 176;
}

int CdrDriver::mult2Speed(int mult)
{
  return mult * 177;
}

// start unit ('startStop' == 1) or stop unit ('startStop' == 0)
// return: 0: OK
//         1: scsi command failed

int CdrDriver::startStopUnit(int startStop) const
{
  unsigned char cmd[6];

  memset(cmd, 0, 6);

  cmd[0] = 0x1b;
  
  if (startStop != 0) {
    cmd[4] |= 0x01;
  }

  if (sendCmd(cmd, 6, NULL, 0, NULL, 0) != 0) {
    message(-2, "Cannot start/stop unit.");
    return 1;
  }

  return 0;
}


// blocks or unblocks tray
// return: 0: OK
//         1: scsi command failed

int CdrDriver::preventMediumRemoval(int block) const
{
  unsigned char cmd[6];

  memset(cmd, 0, 6);

  cmd[0] = 0x1e;
  
  if (block != 0) {
    cmd[4] |= 0x01;
  }

  if (sendCmd(cmd, 6, NULL, 0, NULL, 0) != 0) {
    message(-2, "Cannot prevent/allow medium removal.");
    return 1;
  }

  return 0;
}

// reset device to initial state
// return: 0: OK
//         1: scsi command failed

int CdrDriver::rezeroUnit(int showMessage) const
{
  unsigned char cmd[6];

  memset(cmd, 0, 6);

  cmd[0] = 0x01;
  
  if (sendCmd(cmd, 6, NULL, 0, NULL, 0, showMessage) != 0) {
    if (showMessage)
      message(-2, "Cannot rezero unit.");
    return 1;
  }

  return 0;
}

// Flushs cache of drive which inidcates end of write action. Errors resulting
// from this command are ignored because everything is already done and
// at most the last part of the lead-out track may be affected.
// return: 0: OK
int CdrDriver::flushCache() const
{
  unsigned char cmd[10];

  memset(cmd, 0, 10);

  cmd[0] = 0x35; // FLUSH CACHE
  
  // Print no message if the flush cache command fails because some drives
  // report errors even if all went OK.
  sendCmd(cmd, 10, NULL, 0, NULL, 0, 0);

  return 0;
}

// Reads the cd-rom capacity and stores the total number of available blocks
// in 'length'.
// return: 0: OK
//         1: SCSI command failed
int CdrDriver::readCapacity(long *length, int showMessage)
{
  unsigned char cmd[10];
  unsigned char data[8];

  memset(cmd, 0, 10);
  memset(data, 0, 8);

  cmd[0] = 0x25; // READ CD-ROM CAPACITY

  if (sendCmd(cmd, 10, NULL, 0, data, 8, showMessage) != 0) {
    if (showMessage)
      message(-2, "Cannot read capacity.");
    return 1;
  }
  
  *length = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
  // *length += 1;

  return 0;
}

int CdrDriver::blankDisk()
{
  message(-2, "Blanking is not supported by this driver.");
  return 1;
}

// Writes data to target, the block length depends on the actual writing mode
// 'mode'. 'len' is number of blocks to write.
// 'lba' specifies the next logical block address for writing and is updated
// by this function.
// return: 0: OK
//         1: scsi command failed
int CdrDriver::writeData(TrackData::Mode mode, long &lba, const char *buf,
			 long len)
{
  assert(blocksPerWrite_ > 0);
  int writeLen = 0;
  unsigned char cmd[10];
  long blockLength = blockSize(mode);
  
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

    if (sendCmd(cmd, 10, (unsigned char *)buf, writeLen * blockLength,
		NULL, 0) != 0) {
      message(-2, "Write data failed.");
      return 1;
    }

    buf += writeLen * blockLength;

    lba += writeLen;
    len -= writeLen;
  }
      
  return 0;
}

// Writes 'count' blocks with zero data with 'writeData'.
// m: mode for encoding zero data
// lba: logical block address for the write command, will be updated
// encLba: logical block address used by the LE-C encoder for the
//         sector headers
// count: number of zero blocks to write
// Return: 0: OK
//         1: SCSI error occured
int CdrDriver::writeZeros(TrackData::Mode m, long &lba, long encLba,
			  long count)
{
  assert(blocksPerWrite_ > 0);
  assert(zeroBuffer_ != NULL);

  int n, i;
  long cnt = 0;
  long total = count * AUDIO_BLOCK_LEN;
  long cntMb;
  long lastMb = 0;
  long blockLen;

  if (encodingMode_ == 0)
    blockLen = AUDIO_BLOCK_LEN;
  else 
    blockLen = blockSize(m);

  //FILE *fp = fopen("zeros.out", "w");

  while (count > 0) {
    n = (count > blocksPerWrite_ ? blocksPerWrite_ : count);

    for (i = 0; i < n; i++) {
      Track::encodeZeroData(encodingMode_, m, encLba++,
			    zeroBuffer_ + i * blockLen);
    }

    if (encodingMode_ == 0 && bigEndianSamples() == 0) {
      // swap encoded data blocks
      swapSamples((Sample *)zeroBuffer_, n * SAMPLES_PER_BLOCK);
    }

    //fwrite(zeroBuffer_, blockLen, n, fp);

    if (writeData(encodingMode_ == 0 ? TrackData::AUDIO : m, lba, zeroBuffer_,
		  n) != 0) {
      return 1;
    }
    
    cnt += n * AUDIO_BLOCK_LEN;

    cntMb = cnt >> 20;

    if (cntMb > lastMb) {
      message(1, "Wrote %ld of %ld MB.\r", cntMb, total >> 20);
      fflush(stdout);
      lastMb = cntMb;
    }

    count -= n;
  }

  //fclose(fp);

  return 0;
}

// Requests mode page 'pageCode' from device and places it into given
// buffer of maximum length 'bufLen'.
// modePageHeader: if != NULL filled with mode page header (8 bytes)
// blockDesc     : if != NULL filled with block descriptor (8 bytes),
//                 buffer is zeroed if no block descriptor is received
// return: 0: OK
//         1: scsi command failed
//         2: buffer too small for requested mode page
int CdrDriver::getModePage(int pageCode, unsigned char *buf, long bufLen,
			   unsigned char *modePageHeader,
			   unsigned char *blockDesc,
			   int showErrorMsg)
{
  unsigned char cmd[10];
  long dataLen = bufLen + 8/*mode parameter header*/ + 
                          100/*spare for block descriptors*/;
  unsigned char *data = new (unsigned char)[dataLen];

  memset(cmd, 0, 10);
  memset(data, 0, dataLen);
  memset(buf, 0, bufLen);

  cmd[0] = 0x5a; // MODE SENSE
  cmd[2] = pageCode & 0x3f;
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if (sendCmd(cmd, 10, NULL, 0, data, dataLen,  showErrorMsg) != 0) {
    delete[] data;
    return 1;
  }

  long modeDataLen = (data[0] << 8) | data[1];
  long blockDescLen = (data[6] << 8) | data[7];

  if (modePageHeader != NULL)
    memcpy(modePageHeader, data, 8);

  if (blockDesc != NULL) {
    if (blockDescLen >= 8)
      memcpy(blockDesc, data + 8, 8);
    else 
      memset(blockDesc, 0, 8);
  }

  if (modeDataLen > blockDescLen + 6) {
    unsigned char *modePage = data + blockDescLen + 8;
    long modePageLen = modePage[1] + 2;

    if (modePageLen > bufLen)
      modePageLen = bufLen;

    memcpy(buf, modePage, modePageLen);
    delete[] data;
    return 0;
  }
  else {
    message(-2, "No mode page data received.");
    delete[] data;
    return 1;
  }
}

// Sets mode page in device specified in buffer 'modePage'
// modePageHeader: if != NULL used as mode page header (8 bytes)
// blockDesc     : if != NULL used as block descriptor (8 bytes),
// Return: 0: OK
//         1: SCSI command failed
int CdrDriver::setModePage(const unsigned char *modePage,
			   const unsigned char *modePageHeader,
			   const unsigned char *blockDesc,
			   int showErrorMsg)
{
  long pageLen = modePage[1] + 2;
  unsigned char cmd[10];
  long dataLen = pageLen + 8/*mode parameter header*/;

  if (blockDesc != NULL)
    dataLen += 8;

  unsigned char *data = new (unsigned char)[dataLen];

  memset(cmd, 0, 10);
  memset(data, 0, dataLen);

  if (modePageHeader != NULL)
    memcpy(data, modePageHeader, 8);

  data[0] = 0;
  data[1] = 0;
  data[4] = 0;
  data[5] = 0;


  if (blockDesc != NULL) {
    memcpy(data + 8, blockDesc, 8);
    memcpy(data + 16, modePage, pageLen);
    data[6] = 0;
    data[7] = 8;
  }
  else {
    memcpy(data + 8, modePage, pageLen);
    data[6] = 0;
    data[7] = 0;
  }

  cmd[0] = 0x55; // MODE SELECT
  cmd[1] = 1 << 4;

  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;

  if (sendCmd(cmd, 10, data, dataLen, NULL, 0, showErrorMsg) != 0) {
    delete[] data;
    return 1;
  }

  delete[] data;
  return 0;
}

// As above, but implemented with six byte mode commands

// Requests mode page 'pageCode' from device and places it into given
// buffer of maximum length 'bufLen'.
// modePageHeader: if != NULL filled with mode page header (4 bytes)
// blockDesc     : if != NULL filled with block descriptor (8 bytes),
//                 buffer is zeroed if no block descriptor is received
// return: 0: OK
//         1: scsi command failed
//         2: buffer too small for requested mode page
int CdrDriver::getModePage6(int pageCode, unsigned char *buf, long bufLen,
			    unsigned char *modePageHeader,
			    unsigned char *blockDesc,
			    int showErrorMsg)
{
  unsigned char cmd[6];
  long dataLen = bufLen + 4/*mode parameter header*/ + 
                          100/*spare for block descriptors*/;
  unsigned char *data = new (unsigned char)[dataLen];

  memset(cmd, 0, 6);
  memset(data, 0, dataLen);
  memset(buf, 0, bufLen);

  cmd[0] = 0x1a; // MODE SENSE(6)
  cmd[2] = pageCode & 0x3f;
  cmd[4] = (dataLen > 255) ? 0 : dataLen;

  if (sendCmd(cmd, 6, NULL, 0, data, dataLen,  showErrorMsg) != 0) {
    delete[] data;
    return 1;
  }

  long modeDataLen = data[0];
  long blockDescLen = data[3];

  if (modePageHeader != NULL)
    memcpy(modePageHeader, data, 4);

  if (blockDesc != NULL) {
    if (blockDescLen >= 8)
      memcpy(blockDesc, data + 4, 8);
    else 
      memset(blockDesc, 0, 8);
  }

  if (modeDataLen > blockDescLen + 4) {
    unsigned char *modePage = data + blockDescLen + 4;
    long modePageLen = modePage[1] + 2;

    if (modePageLen > bufLen)
      modePageLen = bufLen;

    memcpy(buf, modePage, modePageLen);
    delete[] data;
    return 0;
  }
  else {
    message(-2, "No mode page data received.");
    delete[] data;
    return 1;
  }
}
 
// Sets mode page in device specified in buffer 'modePage'
// modePageHeader: if != NULL used as mode page header (4 bytes)
// blockDesc     : if != NULL used as block descriptor (8 bytes),
// Return: 0: OK
//         1: SCSI command failed
int CdrDriver::setModePage6(const unsigned char *modePage,
			    const unsigned char *modePageHeader,
			    const unsigned char *blockDesc,
			    int showErrorMsg)
{
  long pageLen = modePage[1] + 2;
  unsigned char cmd[6];
  long dataLen = pageLen + 4/*mode parameter header*/;

  if (blockDesc != NULL)
    dataLen += 8;

  unsigned char *data = new (unsigned char)[dataLen];

  memset(cmd, 0, 6);
  memset(data, 0, dataLen);

  if (modePageHeader != NULL)
    memcpy(data, modePageHeader, 4);

  data[0] = 0;

  if (blockDesc != NULL) {
    memcpy(data + 4, blockDesc, 8);
    memcpy(data + 12, modePage, pageLen);
    data[3] = 8;
  }
  else {
    memcpy(data + 4, modePage, pageLen);
    data[3] = 0;
  }

  cmd[0] = 0x15; // MODE SELECT(6)
  cmd[1] = 1 << 4;

  cmd[4] = dataLen;

  if (sendCmd(cmd, 6, data, dataLen, NULL, 0, showErrorMsg) != 0) {
    delete[] data;
    return 1;
  }

  delete[] data;
  return 0;
}


// Retrieves TOC data of inserted CD. It won't distinguish between different
// sessions.
// The track information is returend either for all sessions or for the
// first session depending on the drive. 'cdTocLen' is filled with number
// of entries including the lead-out track.
// Return: 'NULL' on error, else array of 'CdToc' structures with '*cdTocLen'
// entries 
CdToc *CdrDriver::getTocGeneric(int *cdTocLen)
{
  unsigned char cmd[10];
  unsigned short dataLen;
  unsigned char *data = NULL;;
  unsigned char reqData[4]; // buffer for requestion the actual length
  unsigned char *p = NULL;
  int i;
  CdToc *toc;
  int nTracks;

  // read disk toc length
  memset(cmd, 0, 10);
  cmd[0] = 0x43; // READ TOC
  cmd[6] = 0; // return info for all tracks
  cmd[8] = 4;
  
  if (sendCmd(cmd, 10, NULL, 0, reqData, 4) != 0) {
    message(-2, "Cannot read disk toc.");
    return NULL;
  }

  dataLen = (reqData[0] << 8) | reqData[1];
  dataLen += 2;

  message(3, "getTocGeneric: data len %d", dataLen);

  if (dataLen < 12) {
    dataLen = (100 * 8) + 4;
  }

  data = new (unsigned char)[dataLen];
  memset(data, 0, dataLen);

  // read disk toc
  cmd[7] = dataLen >> 8;
  cmd[8] = dataLen;
  
  if (sendCmd(cmd, 10, NULL, 0, data, dataLen) != 0) {
    message(-2, "Cannot read disk toc.");
    delete[] data;
    return NULL;
  }

  nTracks = data[3] - data[2] + 1;
  if (nTracks > 99) {
    message(-2, "Got illegal toc data.");
    delete[] data;
    return NULL;
  }

  toc = new CdToc[nTracks + 1];
  
  for (i = 0, p = data + 4; i <= nTracks; i++, p += 8) {
    toc[i].track = p[2];
    toc[i].adrCtl = p[1];
    toc[i].start = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];
  }

  *cdTocLen = nTracks + 1;

  delete[] data;

  return toc;
}

// Retrieves TOC data of inserted CD. The track information is returend for
// specified session number only. The lead-out start is taken from the
// correct session so that it can be used to calculate the length of the
// last track.
// 'cdTocLen' is filled with number of entries including the lead-out track.
// Return: 'NULL' on error, else array of 'CdToc' structures with '*cdTocLen'
// entries 

#define IS_BCD(v) (((v) & 0xf0) <= 0x90 && ((v) & 0x0f) <= 0x09)
CdToc *CdrDriver::getToc(int sessionNr, int *cdTocLen)
{
  int rawTocLen;
  int completeTocLen;
  CdToc *completeToc; // toc retrieved with generic method to verify with raw
                      // toc data
  CdToc *cdToc;
  CdRawToc *rawToc;
  int i, j, tocEnt;
  int nTracks = 0;
  int trackNr;
  long trackStart;
  int isBcd = -1;
  int lastTrack;
  int min, sec, frame;

  if ((completeToc = getTocGeneric(&completeTocLen)) == NULL)
    return NULL;

  if (options_ & OPT_DRV_GET_TOC_GENERIC) {
    *cdTocLen = completeTocLen;
    return completeToc;
  }

  if ((rawToc = getRawToc(1, &rawTocLen)) == NULL) {
    *cdTocLen = completeTocLen;
    return completeToc;
  }

  // Try to determine if raw toc data contains BCD or HEX numbers.
  for (i = 0; i < rawTocLen; i++) {
    if ((rawToc[i].adrCtl & 0xf0) == 0x10) { // only process QMODE1 entries
      if (rawToc[i].point < 0xa0 && !IS_BCD(rawToc[i].point)) {
	isBcd = 0;
      }

      if (rawToc[i].point < 0xa0 || rawToc[i].point == 0xa2) {
	if (!IS_BCD(rawToc[i].pmin) || !IS_BCD(rawToc[i].psec) ||
	    !IS_BCD(rawToc[i].pframe)) {
	  isBcd = 0;
	  break;
	}
      }
    }
  }

  if (isBcd == -1) {
    // We still don't know if the values are BCD or HEX but we've ensured
    // so far that all values are valid BCD numbers.

    // Assume that we have BCD numbers and compare with the generic toc data.
    isBcd = 1;
    for (i = 0; i < rawTocLen && isBcd == 1; i++) {
      if ((rawToc[i].adrCtl & 0xf0) == 0x10 && // only process QMODE1 entries
	  rawToc[i].point < 0xa0) { 
	trackNr = SubChannel::bcd2int(rawToc[i].point);

	for (j = 0; j < completeTocLen; j++) {
	  if (completeToc[j].track == trackNr) {
	    break;
	  }
	}

	if (j < completeTocLen) {
	  min = SubChannel::bcd2int(rawToc[i].pmin);
	  sec = SubChannel::bcd2int(rawToc[i].psec);
	  frame = SubChannel::bcd2int(rawToc[i].pframe);

	  if (min <= 99 && sec < 60 && frame < 75) {
	    trackStart = Msf(min, sec, frame).lba() - 150;
	    if (completeToc[j].start != trackStart) {
	      // start does not match -> values are not BCD
	      isBcd = 0;
	    }
	  }
	  else {
	    // bogus time code -> values are not BCD
	    isBcd = 0;
	  }
	}
	else {
	  // track not found -> values are not BCD
	  isBcd = 0;
	}
      }
    }

    if (isBcd == 1) {
      // verify last lead-out pointer
      trackStart = 0; // start of lead-out
      for (i = rawTocLen - 1; i >= 0; i--) {
	if ((rawToc[i].adrCtl & 0xf0) == 0x10 && // QMODE1 entry
	    rawToc[i].point == 0xa2) {
	  min = SubChannel::bcd2int(rawToc[i].pmin);
	  sec = SubChannel::bcd2int(rawToc[i].psec);
	  frame = SubChannel::bcd2int(rawToc[i].pframe);

	  if (min <= 99 && sec < 60 && frame < 75)
	    trackStart = Msf(min, sec, frame).lba() - 150;
	  break;
	}
      }

      if (i < 0) {
	message(-3, "Found bogus toc data (no lead-out entry in raw data). Please report!");
	delete[] rawToc;
	delete[] completeToc;
	return NULL;
      }
	
      for (j = 0; j < completeTocLen; j++) {
	if (completeToc[j].track == 0xaa) {
	  break;
	}
      }
      
      if (j < completeTocLen) {
	if (trackStart != completeToc[j].start) {
	  // lead-out start does not match -> values are not BCD
	    isBcd = 0;
	}
      }
      else {
	message(-3,
		"Found bogus toc data (no lead-out entry). Please report!");
	delete[] rawToc;
	delete[] completeToc;
	return NULL;
      }
    }

  }

  if (isBcd == 0) {
    // verify that the decision is really correct.
    for (i = 0; i < rawTocLen && isBcd == 0; i++) {
      if ((rawToc[i].adrCtl & 0xf0) == 0x10 && // only process QMODE1 entries
	  rawToc[i].point < 0xa0) {
	
	trackNr = rawToc[i].point;
	
	for (j = 0; j < completeTocLen; j++) {
	  if (completeToc[j].track == trackNr) {
	    break;
	  }
	}

	if (j < completeTocLen) {
	  min = rawToc[i].pmin;
	  sec = rawToc[i].psec;
	  frame = rawToc[i].pframe;

	  if (min <= 99 && sec < 60 && frame < 75) {
	    trackStart = Msf(min, sec, frame).lba() - 150;
	    if (completeToc[j].start != trackStart) {
	      // start does not match -> values are not HEX
	      isBcd = -1;
	    }
	  }
	  else {
	    // bogus time code -> values are not HEX
	    isBcd = -1;
	  }
	}
	else {
	  // track not found -> values are not BCD
	  isBcd = -1;
	}
      }
    }

    // verify last lead-out pointer
    trackStart = 0; // start of lead-out
    for (i = rawTocLen - 1; i >= 0; i--) {
      if ((rawToc[i].adrCtl & 0xf0) == 0x10 && // QMODE1 entry
	  rawToc[i].point == 0xa2) {
	min = rawToc[i].pmin;
	sec = rawToc[i].psec;
	frame = rawToc[i].pframe;

	if (min <= 99 && sec < 60 && frame < 75)
	  trackStart = Msf(min, sec, frame).lba() - 150;
	break;
      }
    }

    if (i < 0) {
      message(-3, "Found bogus toc data (no lead-out entry in raw data). Please report!");
      delete[] rawToc;
      delete[] completeToc;
      return NULL;
    }
    
    for (j = 0; j < completeTocLen; j++) {
      if (completeToc[j].track == 0xaa) {
	break;
      }
    }
    
    if (j < completeTocLen) {
      if (trackStart != completeToc[j].start) {
	// lead-out start does not match -> values are not BCD
	isBcd = -1;
      }
    }
    else {
      message(-3,
	      "Found bogus toc data (no lead-out entry). Please report!");
      delete[] rawToc;
      delete[] completeToc;
      return NULL;
    }
  }
  
  if (isBcd == -1) {
    message(-1, "Could not determine if raw toc data is BCD or HEX. Please report!");
    message(-1, "Using TOC data retrieved with generic method (no multi session support).");
    delete[] rawToc;

    *cdTocLen = completeTocLen;
    return completeToc;
  }

  message(3, "Raw toc contains %s values.", isBcd == 0 ? "HEX" : "BCD");

  for (i = 0; i < rawTocLen; i++) {
    if (rawToc[i].sessionNr == sessionNr &&
	(rawToc[i].adrCtl & 0xf0) == 0x10 && /* QMODE1 entry */
	rawToc[i].point < 0xa0) {
      nTracks++;
    }
  }

  if (nTracks == 0 || nTracks > 99) {
    message(-3, "Found bogus toc data (0 or > 99 tracks). Please report!");
    delete[] rawToc;
    delete[] completeToc;
    return NULL;
  }

  cdToc = new CdToc[nTracks + 1];
  tocEnt = 0;
  lastTrack = -1;

  for (i = 0; i < rawTocLen; i++) {
    if (rawToc[i].sessionNr == sessionNr &&
	(rawToc[i].adrCtl & 0xf0) == 0x10 &&  // QMODE1 entry
	rawToc[i].point < 0xa0) {

      if (isBcd) {
	trackNr = SubChannel::bcd2int(rawToc[i].point);
	trackStart = Msf(SubChannel::bcd2int(rawToc[i].pmin),
			 SubChannel::bcd2int(rawToc[i].psec),
			 SubChannel::bcd2int(rawToc[i].pframe)).lba();
      }
      else {
	trackNr = rawToc[i].point;
	trackStart =
	  Msf(rawToc[i].pmin, rawToc[i].psec, rawToc[i].pframe).lba();
      }

      if (lastTrack != -1 && trackNr != lastTrack + 1) {
	message(-3, "Found bogus toc data (track number sequence). Please report!");
	delete[] rawToc;
	delete[] completeToc;
	delete[] cdToc;
	return NULL;
      }

      lastTrack = trackNr;

      cdToc[tocEnt].adrCtl = rawToc[i].adrCtl;
      cdToc[tocEnt].track = trackNr;
      cdToc[tocEnt].start = trackStart - 150;
      tocEnt++;
    }
  }

  // find lead-out pointer
  for (i = 0; i < rawTocLen; i++) {
    if (rawToc[i].sessionNr == sessionNr &&
	(rawToc[i].adrCtl & 0xf0) == 0x10 &&  // QMODE1 entry
	rawToc[i].point == 0xa2 /* Lead-out pointer */) {

      if (isBcd) {
	trackStart = Msf(SubChannel::bcd2int(rawToc[i].pmin),
			 SubChannel::bcd2int(rawToc[i].psec),
			 SubChannel::bcd2int(rawToc[i].pframe)).lba();
      }
      else {
	trackStart =
	  Msf(rawToc[i].pmin, rawToc[i].psec, rawToc[i].pframe).lba();
      }
      
      cdToc[tocEnt].adrCtl = rawToc[i].adrCtl;
      cdToc[tocEnt].track = 0xaa;
      cdToc[tocEnt].start = trackStart - 150;
      tocEnt++;

      break;
    }
  }
  
  if (tocEnt != nTracks + 1) {
    message(-3, "Found bogus toc data (no lead-out pointer for session). Please report!");
    delete[] rawToc;
    delete[] completeToc;
    delete[] cdToc;
    return NULL;
  }
  

  delete[] rawToc;
  delete[] completeToc;

  *cdTocLen = nTracks + 1;
  return cdToc;
}

#if 0
static void adjustPreGaps(TrackInfo *trackInfos, long nofTrackInfos)
{
  long i;

  for (i = 0; i < nofTrackInfos - 1; i++) {
    if (i == 0) {
      trackInfos[i].pregap = trackInfos[i].start;
    }
    else {
      long prevTrackLen = trackInfos[i].start - trackInfos[i - 1].start;

      if (prevTrackLen >= 6 * 75) {
	if (trackInfos[i].mode == TrackData::AUDIO ||
	    trackInfos[i].mode != trackInfos[i - 1].mode) {
	  trackInfos[i].pregap = 2 * 75;
	}
      }
    }
  }
}
#endif

static char *buildDataFileName(int trackNr, CdToc *toc, int nofTracks, 
			       const char *basename, const char *extension)
{
  char buf[30];
  int start, end;
  int run;
  int onlyOneAudioRange = 1;

  // don't modify the STDIN filename
  if (strcmp(basename, "-") == 0)
    return strdupCC(basename);


  if ((toc[trackNr].adrCtl & 0x04) != 0) {
    // data track
    sprintf(buf, "_%d", trackNr + 1);
    return strdup3CC(basename, buf, NULL);
  }

  // audio track, find continues range of audio tracks
  start = trackNr;
  while (start > 0 && (toc[start - 1].adrCtl & 0x04) == 0)
    start--;

  if (start > 0) {
    run = start - 1;
    while (run >= 0) {
      if ((toc[run].adrCtl & 0x04) == 0) {
	onlyOneAudioRange = 0;
	break;
      }
      run--;
    }
  }

  end = trackNr;
  while (end < nofTracks - 1 && (toc[end + 1].adrCtl & 0x04) == 0)
    end++;

  if (onlyOneAudioRange && end < nofTracks - 1) {
    run = end + 1;
    while (run < nofTracks) {
      if ((toc[run].adrCtl & 0x04) == 0) {
	onlyOneAudioRange = 0;
	break;
      }
      run++;
    }
  }

  if (onlyOneAudioRange) {
    return strdup3CC(basename, extension, NULL);
  }
  else {
    sprintf(buf, "_%d-%d", start + 1, end + 1);
    return strdup3CC(basename, buf, extension);
  }
}

// Creates 'Toc' object for inserted CD.
// session: session that should be analyzed
// audioFilename: name of audio file that is placed into TOC
// Return: newly allocated 'Toc' object or 'NULL' on error
Toc *CdrDriver::readDiskToc(int session, const char *dataFilename)
{
  int nofTracks = 0;
  int i, j;
  CdToc *cdToc = getToc(session, &nofTracks);
  Msf indexIncrements[98];
  int indexIncrementCnt = 0;
  char isrcCode[13];
  unsigned char trackCtl; // control nibbles of track
  int ctlCheckOk;
  char *fname;
  char *extension = NULL;
  char *p;
  TrackInfo *trackInfos;

  if (cdToc == NULL) {
    return NULL;
  }

  if (nofTracks <= 1) {
    message(-1, "No tracks on disk.");
    delete[] cdToc;
    return NULL;
  }

  message(1, "");
  printCdToc(cdToc, nofTracks);
  message(1, "");
  //return NULL;

  nofTracks -= 1; // do not count lead-out

  fname = strdupCC(dataFilename);
  if ((p = strrchr(fname, '.')) != NULL) {
    extension = strdupCC(p);
    *p = 0;
  }

  trackInfos = new TrackInfo[nofTracks + 1];
  memset(trackInfos, 0, (nofTracks + 1) * sizeof(TrackInfo));

  for (i = 0; i < nofTracks; i++) {
    TrackData::Mode trackMode;

    if ((cdToc[i].adrCtl & 0x04) != 0) {
      if ((trackMode = getTrackMode(i + 1, cdToc[i].start)) ==
	  TrackData::MODE0) {
	message(-1, "Cannot determine mode of data track %d - asuming MODE1.",
		i + 1);
	trackMode = TrackData::MODE1;
      }

      if (rawDataReading_) {
	if (trackMode == TrackData::MODE1) {
	  trackMode = TrackData::MODE1_RAW;
	}
	else if (trackMode == TrackData::MODE2) {
	  trackMode = TrackData::MODE2_RAW;
	}
	else if (trackMode == TrackData::MODE2_FORM1 ||
		 trackMode == TrackData::MODE2_FORM2 ||
		 trackMode == TrackData::MODE2_FORM_MIX) {
	  trackMode = TrackData::MODE2_RAW;
	}
      }
      else {
	if (trackMode == TrackData::MODE2_FORM1 ||
	    trackMode == TrackData::MODE2_FORM2) {
	  trackMode = TrackData::MODE2_FORM_MIX;
	}
      }
    }
    else {
      trackMode = TrackData::AUDIO;
    }

    trackInfos[i].trackNr = cdToc[i].track;
    trackInfos[i].ctl = cdToc[i].adrCtl & 0x0f;
    trackInfos[i].mode = trackMode;
    trackInfos[i].start = cdToc[i].start;
    trackInfos[i].pregap = 0;
    trackInfos[i].fill = 0;
    trackInfos[i].indexCnt = 0;
    trackInfos[i].isrcCode[0] = 0;
    trackInfos[i].filename = buildDataFileName(i, cdToc, nofTracks, fname,
					       extension);
    trackInfos[i].bytesWritten = 0;
  }

  // lead-out entry
  trackInfos[nofTracks].trackNr = 0xaa;
  trackInfos[nofTracks].ctl = 0;
  trackInfos[nofTracks].mode = trackInfos[nofTracks - 1].mode;
  trackInfos[nofTracks].start = cdToc[nofTracks].start;
  trackInfos[nofTracks].pregap = 0;
  trackInfos[nofTracks].fill = 0;
  trackInfos[nofTracks].indexCnt = 0;
  trackInfos[nofTracks].isrcCode[0] = 0;
  trackInfos[nofTracks].filename = NULL;
  trackInfos[nofTracks].bytesWritten = 0;
  
  long pregap = 0;
  long slba, elba;

  if (session == 1) {
    pregap = cdToc[0].start; // pre-gap of first track
  }

  for (i = 0; i < nofTracks; i++) {
    trackInfos[i].pregap = pregap;

    slba = trackInfos[i].start;
    elba = trackInfos[i + 1].start;

    if (trackInfos[i].mode != trackInfos[i + 1].mode)
      elba -= 150;

    Msf trackLength(elba - slba);

    message(1, "Analyzing track %d (%s): start %s, ", i + 1, 
	    TrackData::mode2String(trackInfos[i].mode),
	    Msf(cdToc[i].start).str());
    message(1, "length %s...", trackLength.str());

    if (pregap > 0) {
      message(1, "Found pre-gap: %s", Msf(pregap).str());
    }

    isrcCode[0] = 0;
    indexIncrementCnt = 0;
    pregap = 0;
    trackCtl = 0;

    if (!fastTocReading_) {
      // Find index increments and pre-gap of next track
      if (trackInfos[i].mode == TrackData::AUDIO) {
	analyzeTrack(TrackData::AUDIO, i + 1, slba, elba,
		     indexIncrements, &indexIncrementCnt, 
		     i < nofTracks - 1 ? &pregap : 0, isrcCode, &trackCtl);

	if (trackInfos[i].mode != trackInfos[i + 1].mode)
	  pregap = 150;
      }
      else {
	if (trackInfos[i].mode != trackInfos[i + 1].mode)
	  pregap = 150;
      }
    }
    else {
      if (trackInfos[i].mode == TrackData::AUDIO) {
	if (readIsrc(i + 1, isrcCode) != 0) {
	  isrcCode[0] = 0;
	}

	if (trackInfos[i].mode != trackInfos[i + 1].mode)
	  pregap = 150;
      }
      else {
	if (trackInfos[i].mode != trackInfos[i + 1].mode)
	  pregap = 150;
      }	
    }

    if (isrcCode[0] != 0) {
      message(1, "Found ISRC code.");
      memcpy(trackInfos[i].isrcCode, isrcCode, 13);
    }

    for (j = 0; j < indexIncrementCnt; j++)
      trackInfos[i].index[j] = indexIncrements[j].lba();
    
    trackInfos[i].indexCnt = indexIncrementCnt;
    
    if ((trackCtl & 0x80) != 0) {
      // Check track against TOC control nibbles
      ctlCheckOk = 1;
      if ((trackCtl & 0x01) !=  (cdToc[i].adrCtl & 0x01)) {
	message(-1, "Pre-emphasis flag of track differs from TOC - toc file contains TOC setting.");
	ctlCheckOk = 0;
      }
      if ((trackCtl & 0x08) != (cdToc[i].adrCtl & 0x08)) {
	message(-1, "2-/4-channel-audio  flag of track differs from TOC - toc file contains TOC setting.");
	ctlCheckOk = 0;
      }

      if (ctlCheckOk) {
	message(1, "Control nibbles of track match CD-TOC settings.");
      }
    }
  }

  /*
  if (pregap != 0)
    trackInfos[nofTracks - 1].fill += pregap;

  if (fastTocReading_) {
    adjustPreGaps(trackInfos, nofTracks + 1);
  }
  */

  int padFirstPregap;

  if (onTheFly_) {
    if (session == 1 && (options_ & OPT_DRV_NO_PREGAP_READ) == 0)
      padFirstPregap = 0;
    else
      padFirstPregap = 1;
  }
  else {
    padFirstPregap = (session != 1) || padFirstPregap_;
  }


  Toc *toc = buildToc(trackInfos, nofTracks + 1, padFirstPregap);

  if (toc != NULL) {
    readCdTextData(toc);

    if (readCatalog(toc, trackInfos[0].start, trackInfos[nofTracks].start)) {
      message(1, "Found disk catalogue number.");
    }
  }

  delete[] cdToc;
  delete[] trackInfos;

  delete[] fname;
  if (extension != NULL)
    delete[] extension;

  return toc;
}

// Implementation is based on binary search over all sectors of actual
// track. ISRC codes are not extracted here.
int CdrDriver::analyzeTrackSearch(TrackData::Mode, int trackNr, long startLba,
				  long endLba, Msf *index, int *indexCnt,
				  long *pregap, char *isrcCode,
				  unsigned char *ctl)
{
  isrcCode[0] = 0;
  *ctl = 0;

  if (pregap != NULL) {
    *pregap = findIndex(trackNr + 1, 0, startLba, endLba - 1);
    if (*pregap >= endLba) {
	*pregap = 0;
    }
    else if (*pregap > 0) {
      *pregap = endLba - *pregap;
    }
  }

  // check for index increments
  int ind = 2;
  long indexLba = startLba;
  *indexCnt = 0;

  do {
    if ((indexLba = findIndex(trackNr, ind, indexLba, endLba - 1)) > 0) {
      message(1, "Found index %d at %s", ind, Msf(indexLba).str());
      if (*indexCnt < 98 && indexLba > startLba) {
	index[*indexCnt] =  Msf(indexLba - startLba);
	*indexCnt += 1;
      }
      ind++;
    }
  } while (indexLba > 0 && indexLba < endLba);


  // Retrieve control nibbles of track, add 75 to track start so we 
  // surely get a block of current track.
  int dummy, track;
  if (getTrackIndex(startLba + 75, &track, &dummy, ctl) == 0 &&
      track == trackNr) {
    *ctl |= 0x80;
  }

  return 0;
}

int CdrDriver::getTrackIndex(long lba, int *trackNr, int *indexNr, 
			     unsigned char *ctl)
{
  return 1;
}

// Scan from lba 'trackStart' to 'trackEnd' for the position at which the
// index switchs to 'index' of track number 'track'.
// return: lba of index start postion or 0 if index was not found
long CdrDriver::findIndex(int track, int index, long trackStart,
			  long trackEnd)
{
  int actTrack;
  int actIndex;
  long start = trackStart;
  long end = trackEnd;
  long mid;

  //message(0, "findIndex: %ld - %ld", trackStart, trackEnd);

  while (start < end) {
    mid = start + ((end - start) / 2);
    
    //message(0, "Checking block %ld...", mid);
    if (getTrackIndex(mid, &actTrack, &actIndex, NULL) != 0) {
      return 0;
    }
    //message(0, "Found track %d, index %d", actTrack, actIndex);
    if ((actTrack < track || actIndex < index) && mid + 1 < trackEnd) {
      //message(0, "  Checking block %ld...", mid + 1);
      if (getTrackIndex(mid + 1, &actTrack, &actIndex, NULL) != 0) {
	return 0;
      }
      //message(0, "  Found track %d, index %d", actTrack, actIndex);
      if (actTrack == track && actIndex == index) {
	//message(0, "Found pregap at %ld", mid + 1);
	return mid;
      }
      else {
	start = mid + 1;
      }
      
    }
    else {
      end = mid;
    }
  }

  return 0;
}

int CdrDriver::analyzeTrackScan(TrackData::Mode, int trackNr, long startLba,
				long endLba,
				Msf *index, int *indexCnt, long *pregap,
				char *isrcCode, unsigned char *ctl)
{
  SubChannel **subChannels;
  int n, i;
  int actIndex = 1;
  long length;
  long crcErrCnt = 0;
  long timeCnt = 0;
  int ctlSet = 0;
  int isrcCodeFound = 0;
  long trackStartLba = startLba;

  *isrcCode = 0;

  if (pregap != NULL)
    *pregap = 0;

  *indexCnt = 0;
  *ctl = 0;

  //startLba -= 75;
  if (startLba < 0) {
    startLba = 0;
  }
  length = endLba - startLba;

  while (length > 0) {
    n = (length > maxScannedSubChannels_) ? maxScannedSubChannels_ : length;

    if (readSubChannels(startLba, n, &subChannels, NULL) != 0 ||
	subChannels == NULL) {
      return 1;
    }

    for (i = 0; i < n; i++) {
      SubChannel *chan = subChannels[i];
      //chan->print();

      if (chan->checkCrc() && chan->checkConsistency()) {
	if (chan->type() == SubChannel::QMODE1DATA) {
	  int t = chan->trackNr();
	  Msf time(chan->min(), chan->sec(), chan->frame()); // track rel time

	  if (timeCnt > 74) {
	    message(1, "%s\r", time.str());
	    timeCnt = 0;
	  }

	  if (t == trackNr && !ctlSet) {
	    *ctl = chan->ctl();
	    *ctl |= 0x80;
	    ctlSet = 1;
	  }
	  if (t == trackNr && chan->indexNr() == actIndex + 1) {
	    actIndex = chan->indexNr();
	    message(1, "Found index %d at: %s", actIndex, time.str());
	    if ((*indexCnt) < 98) {
	      index[*indexCnt] = time;
	      *indexCnt += 1;
	    }
	  }
	  else if (t == trackNr + 1) {
	    if (chan->indexNr() == 0) {
	      if (pregap != NULL)
		*pregap = time.lba();
	      if (crcErrCnt != 0)
		message(1, "Found %ld Q sub-channels with CRC errors.",
			crcErrCnt);
	    
	      return 0;
	    }
	  }
	}
	else if (chan->type() == SubChannel::QMODE3) {
	  if (!isrcCodeFound && startLba > trackStartLba) {
	    strcpy(isrcCode, chan->isrc());
	    isrcCodeFound = 1;
	  }
	}
      }
      else {
	crcErrCnt++;
#if 0
	if (chan->type() == SubChannel::QMODE1DATA) {
	  message(1, "Q sub-channel data at %02d:%02d:%02d failed CRC check - ignored",
		 chan->min(), chan->sec(), chan->frame());
	}
	else {
	  message(1, "Q sub-channel data failed CRC check - ignored.");
	}
	chan->print();
#endif
      }

      timeCnt++;
    }

    length -= n;
    startLba += n;
  }

  if (crcErrCnt != 0)
    message(1, "Found %ld Q sub-channels with CRC errors.", crcErrCnt);

  return 0;
}

int CdrDriver::readSubChannels(long, long, SubChannel ***, Sample *)
{
  return 1;
}

// Checks if toc is suitable for writing. Usually all tocs are OK so
// return just 0 here.
// Return: 0: OK
//         1: toc may not be  suitable
//         2: toc is not suitable
int CdrDriver::checkToc(const Toc *toc)
{
  if (multiSession_ && toc->tocType() != Toc::CD_ROM_XA) {
    message(-1, "The toc type should be set to CD_ROM_XA if a multi session");
    message(-1, "CD is recorded.");
    return 1;
  }

  return 0;
}

// Returns block size for given mode and actual 'encodingMode_' that must
// be used to send data to the recorder.
long CdrDriver::blockSize(TrackData::Mode m) const
{
  long bsize = 0;

  if (encodingMode_ == 0) {
    // only audio blocks are written
    bsize = AUDIO_BLOCK_LEN;
  }
  else if (encodingMode_ == 1) {
    // encoding for SCSI-3/mmc drives in session-at-once mode
    switch (m) {
    case TrackData::AUDIO:
      bsize = AUDIO_BLOCK_LEN;
      break;
    case TrackData::MODE1:
    case TrackData::MODE1_RAW:
      bsize = MODE1_BLOCK_LEN;
      break;
    case TrackData::MODE2:
    case TrackData::MODE2_RAW:
    case TrackData::MODE2_FORM1:
    case TrackData::MODE2_FORM2:
    case TrackData::MODE2_FORM_MIX:
      bsize = MODE2_BLOCK_LEN;
      break;
    case TrackData::MODE0:
      message(-3, "Illegal mode in 'CdrDriver::blockSize()'.");
      break;
    }
  }
  else {
    message(-3, "Illegal encoding mode in 'CdrDriver::blockSize()'.");
  }

  return bsize;
}

void CdrDriver::printCdToc(CdToc *toc, int tocLen)
{
  int t;
  long len;

  message(1, "Track   Mode    Flags  Start                Length");
  message(1, "------------------------------------------------------------");

  for (t = 0; t < tocLen; t++) {
    if (t == tocLen - 1) {
      message(1, "Leadout %s   %x      %s(%6ld)",
	      (toc[t].adrCtl & 0x04) != 0 ? "DATA " : "AUDIO",
	      toc[t].adrCtl & 0x0f,
	      Msf(toc[t].start).str(), toc[t].start);
    }
    else {
      len = toc[t + 1].start - toc[t].start;
      message(1, "%2d      %s   %x      %s(%6ld) ", toc[t].track,
	      (toc[t].adrCtl & 0x04) != 0 ? "DATA " : "AUDIO",
	      toc[t].adrCtl & 0x0f,
	      Msf(toc[t].start).str(), toc[t].start);
      message(1, "    %s(%6ld)", Msf(len).str(), len);
    }
  }
}


TrackData::Mode CdrDriver::getTrackMode(int, long trackStartLba)
{
  unsigned char cmd[10];
  unsigned char data[2340];
  int blockLength = 2340;
  TrackData::Mode mode;

  if (setBlockSize(blockLength) != 0) {
    return TrackData::MODE0;
  }

  memset(cmd, 0, 10);

  cmd[0] = 0x28; // READ10
  cmd[2] = trackStartLba >> 24;
  cmd[3] = trackStartLba >> 16;
  cmd[4] = trackStartLba >> 8;
  cmd[5] = trackStartLba;
  cmd[8] = 1;

  if (sendCmd(cmd, 10, NULL, 0, data, blockLength) != 0) {
    setBlockSize(MODE1_BLOCK_LEN);
    return TrackData::MODE0;
  }
  
  setBlockSize(MODE1_BLOCK_LEN);

  mode = determineSectorMode(data);

  if (mode == TrackData::MODE0) {
    message(-2, "Found illegal mode in sector %ld.", trackStartLba);
  }

  return mode;
}

TrackData::Mode CdrDriver::determineSectorMode(unsigned char *buf)
{
  switch (buf[3]) {
  case 1:
    return TrackData::MODE1;
    break;

  case 2:
    return analyzeSubHeader(buf + 4);
    break;
  }

  // illegal mode found
  return TrackData::MODE0;
}

// Analyzes given 8 byte sub head and tries to determine if it belongs
// to a form 1, form 2 or a plain mode 2 sector.
TrackData::Mode CdrDriver::analyzeSubHeader(unsigned char *sh)
{
  if (sh[0] == sh[4] && sh[1] == sh[5] && sh[2] == sh[6] && sh[3] == sh[7]) {
    // check first copy
    //if (sh[0] < 8 && sh[1] < 8 && sh[2] != 0) {
    if (sh[2] & 0x20 != 0)
      return TrackData::MODE2_FORM2;
    else
      return TrackData::MODE2_FORM1;
    //}

#if 0
    // check second copy
    if (sh[4] < 8 && sh[5] < 8 && sh[6] != 0) {
      if (sh[6] & 0x20 != 0)
	return TrackData::MODE2_FORM2;
      else
	return TrackData::MODE2_FORM1;
    }
#endif
  }
  else {
    // no valid sub-header data, sector is a plain MODE2 sector
    return TrackData::MODE2;
  }
}


// Sets block size for read/write operation to given value.
// blocksize: block size in bytes
// density: (optional, default: 0) density code
// Return: 0: OK
//         1: SCSI command failed
int CdrDriver::setBlockSize(long blocksize, unsigned char density)
{
  unsigned char cmd[10];
  unsigned char ms[16];

  if (blockLength_ == blocksize)
    return 0;

  memset(ms, 0, 16);

  ms[3] = 8;
  ms[4] = density;
  ms[10] = blocksize >> 8;
  ms[11] = blocksize;

  memset(cmd, 0, 10);

  cmd[0] = 0x15; // MODE SELECT6
  cmd[4] = 12;

  if (sendCmd(cmd, 6, ms, 12, NULL, 0) != 0) {
    message(-2, "Cannot set block size.");
    return 1;
  }

  blockLength_ = blocksize;

  return 0;
}


// Returns control flags for given track.		  
unsigned char CdrDriver::trackCtl(const Track *track)
{
  unsigned char ctl = 0;

  if (track->copyPermitted()) {
    ctl |= 0x20;
  }

  if (track->type() == TrackData::AUDIO) {
    // audio track
    if (track->preEmphasis()) {
      ctl |= 0x10;
    }
    if (track->audioType() == 1) {
      ctl |= 0x80;
    }
  }
  else {
    // data track
    ctl |= 0x40;
  }
  
  return ctl;
}

// Returns session format for point A0 toc entry depending on Toc type.
unsigned char CdrDriver::sessionFormat()
{
  unsigned char ret = 0;

  switch (toc_->tocType()) {
  case Toc::CD_DA:
  case Toc::CD_ROM:
    ret = 0x00;
    break;

  case Toc::CD_I:
    ret = 0x10;
    break;

  case Toc::CD_ROM_XA:
    ret = 0x20;
    break;
  }

  message(2, "Session format: %x", ret);

  return ret;
}

// Generic method to read CD-TEXT packs according to the MMC-2 specification.
// nofPacks: filled with number of found CD-TEXT packs
// return: array of CD-TEXT packs or 'NULL' if no packs where retrieved
CdTextPack *CdrDriver::readCdTextPacks(long *nofPacks)
{
  unsigned char cmd[10];
  unsigned char *data;
  unsigned char reqData[4];

  memset(cmd, 0, 10);

  cmd[0] = 0x43; // READ TOC/PMA/ATIP
  cmd[2] = 5;    // CD-TEXT
  cmd[8] = 4;

  if (sendCmd(cmd, 10, NULL, 0, reqData, 4, 0) != 0) {
    message(1, "Cannot read CD-TEXT data - maybe not supported by drive.");
    return NULL;
  }

  long len = ((reqData[0] << 8 ) | reqData[1]) + 2;

  message(3, "CD-TEXT data len: %ld", len);

  if (len <= 4)
    return NULL;

  if (len > scsiIf_->maxDataLen()) {
    message(-2, "CD-TEXT data too big for maximum SCSI transfer length.");
    return NULL;
  }

  data = new (unsigned char)[len];

  cmd[7] = len >> 8;
  cmd[8] = len;

  if (sendCmd(cmd, 10, NULL, 0, data, len, 1) != 0) {
    message(-2, "Reading of CD-TEXT data failed.");
    delete[] data;
    return NULL;
  }
  
  *nofPacks = (len - 4) / sizeof(CdTextPack);

  CdTextPack *packs = new CdTextPack[*nofPacks];

  memcpy(packs, data + 4, *nofPacks * sizeof(CdTextPack));

  delete[] data;

  return packs;
}

// Analyzes CD-TEXT packs and stores read data in given 'Toc' object.
// Return: 0: OK
//         1: error occured
int CdrDriver::readCdTextData(Toc *toc)
{
  long i, j;
  long nofPacks;
  CdTextPack *packs = readCdTextPacks(&nofPacks);
  unsigned char buf[256 * 12];
  unsigned char lastType;
  int lastBlockNumber;
  int blockNumber;
  int pos;
  int actTrack;
  CdTextItem::PackType packType;
  CdTextItem *sizeInfoItem = NULL;
  CdTextItem *item;

  if (packs == NULL)
    return 1;

  message(1, "Found CD-TEXT data.");

  pos = 0;
  lastType = packs[0].packType;
  lastBlockNumber = (packs[0].blockCharacter >> 4) & 0x07;
  actTrack = 0;

  for (i = 0; i < nofPacks; i++) {
    CdTextPack &p = packs[i];

#if 1
    message(3, "%02x %02x %02x %02x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x  CRC: %02x %02x", p.packType, p.trackNumber,
	    p.sequenceNumber, p.blockCharacter, p.data[0], p.data[1], 
	    p.data[2], p.data[3], p.data[4], p.data[5], p.data[6], p.data[7], 
	    p.data[8], p.data[9], p.data[10], p.data[11],
	    p.crc0, p.crc1);
#endif

    blockNumber = (p.blockCharacter >> 4) & 0x07;

    if (lastType != p.packType || lastBlockNumber != blockNumber) {
      if (lastType >= 0x80 && lastType <= 0x8f) {
	packType = CdTextItem::int2PackType(lastType);

	if (CdTextItem::isBinaryPack(packType)) {
	  // finish binary data

	  if (packType == CdTextItem::CDTEXT_GENRE) {
	    // The two genre codes may be followed by a string. Adjust 'pos'
	    // so that all extra 0 bytes at the end of the data are stripped
	    // off.
	    for (j = 2; j < pos && buf[j] != 0; j++) ;
	    if (j < pos)
	      pos = j + 1;
	  }

	  item = new CdTextItem(packType, lastBlockNumber, buf, pos);

	  if (packType == CdTextItem::CDTEXT_SIZE_INFO)
	    sizeInfoItem = item;

	  toc->addCdTextItem(0, item);
	}
      }
      else {
	message(-2, "CD-TEXT: Found invalid pack type: %02x", lastType);
	delete[] packs;
	return 1;
      }

      lastType = p.packType;
      lastBlockNumber = blockNumber;
      pos = 0;
      actTrack = 0;
    }

    if (p.packType >= 0x80 && p.packType <= 0x8f) {
      packType = CdTextItem::int2PackType(p.packType);

      if (CdTextItem::isBinaryPack(packType)) {
	memcpy(buf + pos, p.data, 12);
	pos += 12;
      }
      else {
	// pack contains text -> read all string from it
	j = 0;

	while (j < 12) {
	  for (; j < 12 && p.data[j] != 0; j++)
	    buf[pos++] = p.data[j];

	  if (j < 12) {
	    // string is finished
	    buf[pos] = 0;

#if 0	
	    message(0, "%02x %02x: %s", p.packType, p.trackNumber, buf);
#endif

	    toc->addCdTextItem(actTrack,
			       new CdTextItem(packType, blockNumber,
					      (char*)buf));
	    actTrack++;
	    pos = 0;

	    // skip zero data
	    while (j < 12 && p.data[j] == 0)
	      j++;
	  }
	}
      }
    }
    else {
      message(-2, "CD-TEXT: Found invalid pack type: %02x", p.packType);
      delete[] packs;
      return 1;
    }
  }

  if (pos != 0 && lastType >= 0x80 && lastType <= 0x8f) {
    packType = CdTextItem::int2PackType(lastType);

    if (CdTextItem::isBinaryPack(packType)) {
      // finish binary data

      if (packType == CdTextItem::CDTEXT_GENRE) {
	// The two genre codes may be followed by a string. Adjust 'pos'
	// so that all extra 0 bytes at the end of the data are stripped
	// off.
	for (j = 2; j < pos && buf[j] != 0; j++) ;
	if (j < pos)
	  pos = j + 1;
      }

      item = new CdTextItem(packType, lastBlockNumber, buf, pos);
      toc->addCdTextItem(0, item);

      if (packType == CdTextItem::CDTEXT_SIZE_INFO)
	sizeInfoItem = item;
    }
  }

  delete[] packs;

  // update language mapping from SIZE INFO pack data
  if (sizeInfoItem != NULL && sizeInfoItem->dataLen() >= 36) {
    const unsigned char *data = sizeInfoItem->data();
    for (i = 0; i < 8; i++) {
      if (data[28 + i] > 0)
	toc->cdTextLanguage(i, data[28 + i]);
      else
	toc->cdTextLanguage(i, -1);
    }
  }
  else {
    message(-1, "Cannot determine language mapping from CD-TEXT data.");
    message(-1, "Using default mapping.");
  }

  return 0;
}

long CdrDriver::readTrackData(TrackData::Mode mode, long lba, long len,
			      unsigned char *buf)
{
  return -1;
}

int CdrDriver::analyzeDataTrack(TrackData::Mode mode, int trackNr,
				long startLba, long endLba, long *pregap)
{
  long maxLen = scsiIf_->maxDataLen() / AUDIO_BLOCK_LEN;
  long lba = startLba;
  long len = endLba - startLba;
  long actLen, n;

  *pregap = 0;

  while (len > 0) {
    n = len > maxLen ? maxLen : len;

    if ((actLen = readTrackData(mode, lba, n, transferBuffer_)) < 0) {
      message(-2, "Analyzing of track %d failed.", trackNr);
      return 1;
    }

    message(1, "%s\r", Msf(lba).str());

    if (actLen != n) {
      //message(0, "Data track pre-gap: %ld", len - actLen);
      *pregap = len - actLen;

      if (*pregap > 300) {
	message(-1,
		"The pre-gap of the following track appears to have length %s.",
		Msf(*pregap).str());
	message(-1, "This value is probably bogus and may be caused by unexpected");
	message(-1, "behavior of the drive. Try to verify with other tools how");
	message(-1, "much data can be read from the current track and compare it");
	message(-1, "to the value stored in the toc-file. Usually, the pre-gap");
	message(-1, "should have length 00:02:00.");
      }

      return 0;
    }

    len -= n;
    lba += n;
  }

  return 0;
}

// Reads toc and audio data from CD. Must be overloaded by derived class.
Toc *CdrDriver::readDisk(int session, const char *dataFilename)
{
  int padFirstPregap = 1;
  int nofTracks = 0;
  int i;
  CdToc *cdToc = getToc(session, &nofTracks);
  //unsigned char trackCtl; // control nibbles of track
  //int ctlCheckOk;
  TrackInfo *trackInfos;
  TrackData::Mode trackMode;
  int fp;
  char *fname = strdupCC(dataFilename);
  Toc *toc = NULL;

  if (cdToc == NULL) {
    return NULL;
  }

  if (nofTracks <= 1) {
    message(-1, "No tracks on disk.");
    delete[] cdToc;
    return NULL;
  }

  message(1, "");
  printCdToc(cdToc, nofTracks);
  message(1, "");
  //return NULL;

  if (onTheFly_) {
    fp = onTheFlyFd_;
  }
  else {
#ifdef _WIN32
    if ((fp = open(dataFilename, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,
		   0666)) < 0)
#else
    if ((fp = open(dataFilename, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)
#endif
    {
      message(-2, "Cannot open \"%s\" for writing: %s", dataFilename,
	      strerror(errno));
      delete[] cdToc;
      return NULL;
    }
  }

  nofTracks -= 1; // do not count lead-out

  trackInfos = new TrackInfo[nofTracks + 1];
  
  for (i = 0; i < nofTracks; i++) {
    if ((cdToc[i].adrCtl & 0x04) != 0) {
      if ((trackMode = getTrackMode(i + 1, cdToc[i].start)) ==
	  TrackData::MODE0) {
	message(-1, "Cannot determine mode of data track %d - asuming MODE1.",
		i + 1);
	trackMode = TrackData::MODE1;
      }

      if (rawDataReading_) {
	if (trackMode == TrackData::MODE1) {
	  trackMode = TrackData::MODE1_RAW;
	}
	else if (trackMode == TrackData::MODE2_FORM1 ||
		 trackMode == TrackData::MODE2_FORM2 ||
		 trackMode == TrackData::MODE2_FORM_MIX) {
	  trackMode = TrackData::MODE2_RAW;
	}
      }
      else {
	if (trackMode == TrackData::MODE2_FORM1 ||
	    trackMode == TrackData::MODE2_FORM2 ||
	    trackMode == TrackData::MODE2_FORM_MIX) {
	  trackMode = TrackData::MODE2_FORM_MIX;
	}
      }
    }
    else {
      trackMode = TrackData::AUDIO;
    }

    trackInfos[i].trackNr = cdToc[i].track;
    trackInfos[i].ctl = cdToc[i].adrCtl & 0x0f;
    trackInfos[i].mode = trackMode;
    trackInfos[i].start = cdToc[i].start;
    trackInfos[i].pregap = 0;
    trackInfos[i].fill = 0;
    trackInfos[i].indexCnt = 0;
    trackInfos[i].isrcCode[0] = 0;
    trackInfos[i].filename = fname;
    trackInfos[i].bytesWritten = 0;
  }

  // lead-out entry
  trackInfos[nofTracks].trackNr = 0xaa;
  trackInfos[nofTracks].ctl = 0;
  trackInfos[nofTracks].mode = trackInfos[nofTracks - 1].mode;
  trackInfos[nofTracks].start = cdToc[nofTracks].start;
  trackInfos[nofTracks].pregap = 0;
  trackInfos[nofTracks].indexCnt = 0;
  trackInfos[nofTracks].isrcCode[0] = 0;
  trackInfos[nofTracks].filename = NULL;
  trackInfos[nofTracks].bytesWritten = 0;

  int trs = 0;
  int tre = 0;
  long slba, elba;

  while (trs < nofTracks) {
    if (trackInfos[trs].mode != TrackData::AUDIO) {
      if (trs == 0) {
	if (session == 1)
	  trackInfos[trs].pregap = trackInfos[trs].start;
      }
      else {
	if (trackInfos[trs].mode != trackInfos[trs - 1].mode)
	  trackInfos[trs].pregap = 150;
      }

      slba = trackInfos[trs].start;
      elba = trackInfos[trs + 1].start;

      if (trackInfos[trs].mode != trackInfos[trs + 1].mode) {
	elba -= 150;
      }

      message(1, "Copying data track %d (%s): start %s, ", trs + 1, 
	      TrackData::mode2String(trackInfos[trs].mode),
	      Msf(cdToc[trs].start).str());
      message(1, "length %s to \"%s\"...", Msf(elba - slba).str(),
	      trackInfos[trs].filename);
      
      if (readDataTrack(fp, slba, elba, &trackInfos[trs]) != 0)
	goto fail;

      trs++;
    }
    else {
      // find continuous range of audio tracks
      tre = trs;
      while (tre < nofTracks && trackInfos[tre].mode == TrackData::AUDIO)
	tre++;

      if (trs == 0) {
	if (session == 1)
	  trackInfos[trs].pregap = trackInfos[trs].start;
      }
      else {
	if (trackInfos[trs].mode != trackInfos[trs - 1].mode)
	  trackInfos[trs].pregap = 150;
      }

      slba = cdToc[trs].start;
      elba = cdToc[tre].start;

      // If we have the first track of the first session we can start ripping
      // from lba 0 to extract the pre-gap data.
      // But only if the drive supports it as indicated by 'options_'.
      if (session == 1 && trs == 0 &&
	  (options_ & OPT_DRV_NO_PREGAP_READ) == 0) {
	slba = 0;
      }

      // Assume that the pre-gap length conforms to the standard if the track
      // mode changes. 
      if (trackInfos[tre - 1].mode != trackInfos[tre].mode) {
	elba -= 150;
      }

      message(1, "Copying audio tracks %d-%d: start %s, ", trs + 1, tre,
	      Msf(slba).str());
      message(1, "length %s to \"%s\"...", Msf(elba - slba).str(),
	      trackInfos[trs].filename);

      if (readAudioRange(fp, slba, elba, trs, tre - 1, trackInfos) != 0)
	goto fail;

      trs = tre;
    }
  }

  // if the drive allows to read audio data from the first track's
  // pre-gap the data will be written to the output file and 
  // 'buildToc()' must not create zero data for the pre-gap
  padFirstPregap = 1;

  if (session == 1 && (options_ & OPT_DRV_NO_PREGAP_READ) == 0)
    padFirstPregap = 0;

  toc = buildToc(trackInfos, nofTracks + 1, padFirstPregap);

  if (!onTheFly_ && toc != NULL) {
    readCdTextData(toc);

    if (readCatalog(toc, trackInfos[0].start, trackInfos[nofTracks].start)) {
      message(1, "Found disk catalogue number.");
    }
  }

fail:
  delete[] cdToc;
  delete[] trackInfos;

  delete[] fname;

  if (!onTheFly_) {
    if (close(fp) != 0) {
      message(-2, "Writing to \"%s\" failed: %s", dataFilename,
	      strerror(errno));
      delete toc;
      return NULL;
    }
  }

  return toc;
}


Toc *CdrDriver::buildToc(TrackInfo *trackInfos, long nofTrackInfos,
			 int padFirstPregap)
{
  long i, j;
  long nofTracks = nofTrackInfos - 1;
  int foundDataTrack = 0;
  int foundAudioTrack = 0;
  int foundXATrack = 0;
  long modeStartLba = 0; // start LBA for current mode
  unsigned long dataLen;
  TrackData::Mode trackMode;
  TrackData::Mode lastMode = TrackData::MODE0; // illegal in this context
  int newMode;
  long byteOffset = 0;

  if (nofTrackInfos < 2)
    return NULL;

  Toc *toc = new Toc;

  // build the Toc

  for (i = 0; i < nofTracks; i++) {
    TrackInfo &ati = trackInfos[i]; // actual track info
    TrackInfo &nti = trackInfos[i + 1]; // next track info

    newMode = 0;
    trackMode = ati.mode;

    switch (trackMode) {
    case TrackData::AUDIO:
      foundAudioTrack = 1;
      break;
    case TrackData::MODE1:
    case TrackData::MODE1_RAW:
    case TrackData::MODE2:
      foundDataTrack = 1;
      break;
    case TrackData::MODE2_RAW:
    case TrackData::MODE2_FORM1:
    case TrackData::MODE2_FORM2:
    case TrackData::MODE2_FORM_MIX:
      foundXATrack = 1;
      break;
    case TrackData::MODE0:
      // should not happen
      break;
    }

    if (trackMode != lastMode) {
      newMode = 1;
      if (i == 0 && !padFirstPregap)
	modeStartLba = 0;
      else
	modeStartLba = ati.start;
      lastMode = trackMode;
    }
    
    Track t(trackMode);

    t.preEmphasis(ati.ctl & 0x01);
    t.copyPermitted(ati.ctl & 0x02);
    t.audioType(ati.ctl & 0x08);

    if (ati.isrcCode[0] != 0)
      t.isrc(ati.isrcCode);

    if (trackMode == TrackData::AUDIO) {
      if (newMode && (i > 0 || padFirstPregap)) {
	Msf trackLength(nti.start - ati.start - nti.pregap);
	if (ati.pregap > 0) {
	  t.append(SubTrack(SubTrack::DATA,
			    TrackData(trackMode, Msf(ati.pregap).samples())));
	}
	t.append(SubTrack(SubTrack::DATA,
			  TrackData(ati.filename, byteOffset,
				    0, trackLength.samples())));
      }
      else {
	Msf trackLength(nti.start - ati.start - nti.pregap + ati.pregap);
	t.append(SubTrack(SubTrack::DATA,
			  TrackData(ati.filename, byteOffset,
				    Msf(ati.start - modeStartLba
					- ati.pregap).samples(), 
				    trackLength.samples())));
      }
	
      t.start(Msf(ati.pregap));
    }
    else {
      long trackLength = nti.start - ati.start - nti.pregap - ati.fill;

      if (ati.pregap != 0) {
	// add zero data for pre-gap
	dataLen = ati.pregap * TrackData::dataBlockSize(trackMode);
	t.append(SubTrack(SubTrack::DATA, TrackData(trackMode, dataLen)));
      }
      
      dataLen = trackLength * TrackData::dataBlockSize(trackMode);
      t.append(SubTrack(SubTrack::DATA,
			TrackData(trackMode, ati.filename, byteOffset,
				  dataLen)));

      if (ati.fill > 0) {
	dataLen =  ati.fill * TrackData::dataBlockSize(trackMode);
	t.append(SubTrack(SubTrack::DATA, TrackData(trackMode, dataLen)));
      }

      t.start(Msf(ati.pregap));
    }

    for (j = 0; j < ati.indexCnt; j++)
      t.appendIndex(Msf(ati.index[j]));

    toc->append(&t);

    byteOffset += ati.bytesWritten;
  }

  if (foundXATrack)
    toc->tocType(Toc::CD_ROM_XA);
  else if (foundDataTrack)
    toc->tocType(Toc::CD_ROM);
  else
    toc->tocType(Toc::CD_DA);

  return toc;
}

// Reads a complete data track.
// start: start of data track from TOC
// end: start of next track from TOC
// trackInfo: info about current track, updated by this function
// Return: 0: OK
//         1: error occured
int CdrDriver::readDataTrack(int fd, long start, long end,
			     TrackInfo *trackInfo)
{
  long len = end - start;
  long totalLen = len;
  long lba;
  long lastLba;
  long blockLen;
  long blocking;
  long burst;
  long iterationsWithoutError = 0;
  long n, ret;
  long act;
  int foundLECError;
  unsigned char *buf;
  TrackData::Mode mode;

  switch (trackInfo->mode) {
  case TrackData::MODE1:
    mode = TrackData::MODE1;
    blockLen = MODE1_BLOCK_LEN;
    break;
  case TrackData::MODE1_RAW:
    mode = TrackData::MODE1_RAW;
    blockLen = AUDIO_BLOCK_LEN;
    break;
  case TrackData::MODE2:
    mode = TrackData::MODE2;
    blockLen = MODE2_BLOCK_LEN;
    break;
  case TrackData::MODE2_RAW:
    mode = TrackData::MODE2_RAW;
    blockLen = AUDIO_BLOCK_LEN;
    break;
  case TrackData::MODE2_FORM1:
  case TrackData::MODE2_FORM2:
  case TrackData::MODE2_FORM_MIX:
    mode = TrackData::MODE2_FORM_MIX;
    blockLen = MODE2_BLOCK_LEN;
    break;
  case TrackData::MODE0:
  case TrackData::AUDIO:
    message(-3, "CdrDriver::readDataTrack: Illegal mode.");
    return 1;
    break;
  }

  // adjust mode in 'trackInfo'
  trackInfo->mode = mode;

  trackInfo->bytesWritten = 0;

  blocking = scsiIf_->maxDataLen() / AUDIO_BLOCK_LEN;;
  assert(blocking > 0);

  buf = new (unsigned char)[blocking * blockLen];

  lba = lastLba = start;
  burst = blocking;

  while (len > 0) {
    if (burst != blocking && iterationsWithoutError > 2 * blocking)
      burst = blocking;

    n = (len > burst) ? burst : len;

    foundLECError = 0;

    if ((act = readTrackData(mode, lba, n, buf)) == -1) {
      message(-2, "Read error while copying data from track.");
      delete[] buf;
      return 1;
    }

    if (act == -2) {
      // L-EC error encountered
      if (trackInfo->mode == TrackData::MODE1_RAW ||
	  trackInfo->mode == TrackData::MODE2_RAW) {

	if (n > 1) {
	  // switch to single step mode
	  iterationsWithoutError = 0;
	  burst = 1;
	  continue;
	}
	else {
	  foundLECError = 1;
	  act = n = 0;
	}
      }
      else {
	message(-2, "L-EC error around sector %ld while copying data from track.", lba);
	message(-2, "Use option '--read-raw' to ignore L-EC errors.");
	delete[] buf;
	return 1;
      }
    }

    if (foundLECError) {
      iterationsWithoutError = 0;

      message(-1, "Found L-EC error at sector %ld - ignored.", lba);

      // create a dummy sector for the sector with L-EC errors
      Msf m(lba + 150);

      memcpy(buf, syncPattern, 12);
      buf[12] = SubChannel::bcd(m.min());
      buf[13] = SubChannel::bcd(m.sec());
      buf[14] = SubChannel::bcd(m.frac());
      if (trackInfo->mode == TrackData::MODE1_RAW)
	buf[15] = 1;
      else
	buf[15] = 2;

      memset(buf + 16, 0, AUDIO_BLOCK_LEN - 16);

      if ((ret = fullWrite(fd, buf, AUDIO_BLOCK_LEN)) != AUDIO_BLOCK_LEN) {
	if (ret < 0)
	  message(-2, "Writing of data failed: %s", strerror(errno));
	else
	  message(-2, "Writing of data failed: Disk full");
	  
	delete[] buf;
	return 1;
      }

      trackInfo->bytesWritten += AUDIO_BLOCK_LEN;

      lba += 1;
      len -= 1;
    }
    else {
      iterationsWithoutError++;

      if (act > 0) {
	if ((ret = fullWrite(fd, buf, blockLen * act)) != blockLen * act) {
	  if (ret < 0)
	    message(-2, "Writing of data failed: %s", strerror(errno));
	  else
	    message(-2, "Writing of data failed: Disk full");
	  
	  delete[] buf;
	  return 1;
	}
      }

      trackInfo->bytesWritten += blockLen * act;

      if (lba > lastLba + 75) {
	Msf lbatime(lba);
	message(1, "%02d:%02d:00\r", lbatime.min(), lbatime.sec());
	lastLba = lba;

	if (remote_) {
	  long progress = (totalLen - len) * 1000;
	  progress /= totalLen;

	  sendReadCdProgressMsg(RCD_EXTRACTING, trackInfo->trackNr, progress);
	}
      }

      lba += act;
      len -= act;

      if (act != n)
	break;
    }
  }

  // pad remaining blocks with zero data, e.g. for disks written in TAO mode

  if (len > 0) {
    message(-1, "Padding with %ld zero sectors.", len);

    if (mode == TrackData::MODE1_RAW || mode == TrackData::MODE2_RAW) {
      memcpy(buf, syncPattern, 12);

      if (mode == TrackData::MODE1_RAW)
	buf[15] = 1;
      else
	buf[15] = 2;

      memset(buf + 16, 0, AUDIO_BLOCK_LEN - 16);
    }
    else {
      memset(buf, 0, blockLen);
    }
    
    while (len > 0) {
      if (mode == TrackData::MODE1_RAW || mode == TrackData::MODE2_RAW) {
	Msf m(lba + 150);

	buf[12] = SubChannel::bcd(m.min());
	buf[13] = SubChannel::bcd(m.sec());
	buf[14] = SubChannel::bcd(m.frac());
      }

      if ((ret = fullWrite(fd, buf, blockLen)) != blockLen) {
	if (ret < 0)
	  message(-2, "Writing of data failed: %s", strerror(errno));
	else
	  message(-2, "Writing of data failed: Disk full");
	  
	delete[] buf;
	return 1;
      }

      trackInfo->bytesWritten += blockLen;
      
      len--;
      lba++;
    }
  }

  delete[] buf;
  
  return 0;
}

// Tries to read the catalog number from the sub-channels starting at LBA 0.
// If a catalog number is found it will be placed into the provided 14 byte
// buffer 'mcnCode'. Otherwise 'mcnCode[0]' is set to 0.
// Return: 0: OK
//         1: SCSI error occured
#define N_ELEM 11
#define MCN_LEN 13
#define MAX_MCN_SCAN_LENGTH 5000

static int cmpMcn(const void *p1, const void *p2)
{
  const char *s1 = (const char *)p1;
  const char *s2 = (const char *)p2;
  
  return strcmp(s1, s2);
}

int CdrDriver::readCatalogScan(char *mcnCode, long startLba, long endLba)
{

  SubChannel **subChannels;
  int n, i;
  long length;
  int mcnCodeFound = 0;
  char mcn[N_ELEM][MCN_LEN+1];

  *mcnCode = 0;

  length = endLba - startLba;

  if (length > MAX_MCN_SCAN_LENGTH)
    length = MAX_MCN_SCAN_LENGTH;


  while ((length > 0) && (mcnCodeFound < N_ELEM)) {
    n = (length > maxScannedSubChannels_ ? maxScannedSubChannels_ : length);

    if (readSubChannels(startLba, n, &subChannels, NULL) != 0 ||
	subChannels == NULL) {
      return 1;
    }

    for (i = 0; i < n; i++) {
      SubChannel *chan = subChannels[i];
      //chan->print();

      if (chan->checkCrc() && chan->checkConsistency()) {
        if (chan->type() == SubChannel::QMODE2) {
          if (mcnCodeFound < N_ELEM) {
            strcpy(mcn[mcnCodeFound++], chan->catalog());
          }
        }
      }
    }

    length -= n;
    startLba += n;
  }

  if(mcnCodeFound > 0) {
    qsort(mcn, mcnCodeFound, MCN_LEN + 1, cmpMcn);
    strcpy(mcnCode, mcn[(mcnCodeFound >> 1)]);
  }

  return 0;
}

#undef N_ELEM
#undef MCN_LEN
#undef MAX_MCN_SCAN_LENGTH


// Sends a read cd progress message without blocking the actual process.
void CdrDriver::sendReadCdProgressMsg(ReadCdProgressType type,
				      int track, int trackProgress)
{
  if (remote_) {
    int fd = 3;
    ReadCdProgress p;

    p.status = type;
    p.track = track;
    //p.totalTracks = totalTracks;
    p.trackProgress = trackProgress;

    if (write(fd, REMOTE_MSG_SYNC_, sizeof(REMOTE_MSG_SYNC_)) != sizeof(REMOTE_MSG_SYNC_) ||
	write(fd, (const char*)&p, sizeof(p)) != sizeof(p)) {
      message(-1, "Failed to send read CD remote progress message.");
    }
  }
}


// read cdda paranoia related:

void CdrDriver::paranoiaMode(int mode)
{
  paranoiaMode_ = PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP;

  switch (mode) {
  case 0:
    paranoiaMode_ = PARANOIA_MODE_DISABLE;
    break;

  case 1:
    paranoiaMode_ &= ~PARANOIA_MODE_VERIFY;
    break;

  case 2:
    paranoiaMode_ &= ~(PARANOIA_MODE_SCRATCH|PARANOIA_MODE_REPAIR);
    break;
  }
}

int CdrDriver::readAudioRangeParanoia(int fd, long start, long end,
				      int startTrack, int endTrack, 
				      TrackInfo *trackInfo)
{
  long startLba = start;
  long endLba = end - 1;
  long len, ret;
  size16 *buf;

  if (paranoia_ == NULL) {
    // first time -> allocate paranoia structure 
    paranoiaDrive_ = new cdrom_drive;
    paranoiaDrive_->cdr = this;
    paranoiaDrive_->nsectors = maxScannedSubChannels_;
    paranoia_ = paranoia_init(paranoiaDrive_);
  }

  paranoia_set_range(paranoia_, startLba, endLba);
  paranoia_modeset(paranoia_, paranoiaMode_);

  paranoiaTrackInfo_ = trackInfo;
  paranoiaStartTrack_ = startTrack;
  paranoiaEndTrack_ = endTrack;
  paranoiaActLba_ = startLba + 149;
  paranoiaActTrack_ = startTrack;
  paranoiaActIndex_ = 1;
  paranoiaCrcCount_ = 0;
  paranoiaError_ = 0;
  paranoiaProgress_ = 0;

  len = endLba - startLba + 1;

  message(1, "Track %d...", startTrack + 1);

  trackInfo[endTrack].bytesWritten = 0;

  while (len > 0) {
    buf = paranoia_read(paranoia_, &CdrDriver::paranoiaCallback);

    // The returned samples are always in host byte order. We want to
    // output in big endian byte order so swap if we are a little
    // endian host.
    if (hostByteOrder_ == 0)
      swapSamples((Sample*)buf, SAMPLES_PER_BLOCK);

    if ((ret = fullWrite(fd, buf, AUDIO_BLOCK_LEN)) != AUDIO_BLOCK_LEN) {
      if (ret < 0)
	message(-2, "Writing of data failed: %s", strerror(errno));
      else
	message(-2, "Writing of data failed: Disk full");

      return 1;
    }

    trackInfo[endTrack].bytesWritten += AUDIO_BLOCK_LEN;

    len--;
  }


  if (paranoiaCrcCount_ != 0)
    message(1, "Found %ld Q sub-channels with CRC errors.", paranoiaCrcCount_);

  return 0;
}

long CdrDriver::paranoiaRead(Sample *buffer, long startLba, long len)
{
  SubChannel **chans;
  int i;
  int swap;

  if (readSubChannels(startLba, len, &chans, buffer) != 0) {
    memset(buffer, 0, len * AUDIO_BLOCK_LEN);
    paranoiaError_ = 1;
    return len;
  }

  swap = (audioDataByteOrder_ == hostByteOrder_) ? 0 : 1;

  if (options_ & OPT_DRV_SWAP_READ_SAMPLES)
    swap = !swap;

  if (swap)
    swapSamples(buffer, len * SAMPLES_PER_BLOCK);

  if (remote_) {
    long totalTrackLen = paranoiaTrackInfo_[paranoiaActTrack_ + 1].start -
                         paranoiaTrackInfo_[paranoiaActTrack_ ].start;
    long progress = startLba - paranoiaTrackInfo_[paranoiaActTrack_ ].start;

    if (progress > 0) {
      progress *= 1000;
      progress /= totalTrackLen;
    }
    else {
      progress = 0;
    }

    sendReadCdProgressMsg(RCD_EXTRACTING, paranoiaActTrack_ + 1, progress);
  }

  if (chans == NULL) {
    // drive does not provide sub channel data so that's all we could do here:

    if (startLba > paranoiaTrackInfo_[paranoiaActTrack_ + 1].start) {
      paranoiaActTrack_++;
      message(1, "Track %d...", paranoiaActTrack_ + 1);
    }

    if (startLba - paranoiaProgress_ > 75) {
      paranoiaProgress_ = startLba;
      Msf m(paranoiaProgress_);
      message(1, "%02d:%02d:00\r", m.min(), m.sec());
    }
    
    return len;      
  }

  // analyze sub-channels to find pre-gaps, index marks and ISRC codes
  for (i = 0; i < len; i++) {
    SubChannel *chan = chans[i];
    //chan->print();
    
    if (chan->checkCrc() && chan->checkConsistency()) {
      if (chan->type() == SubChannel::QMODE1DATA) {
	int t = chan->trackNr() - 1;
	Msf atime = Msf(chan->amin(), chan->asec(), chan->aframe());

	//message(0, "LastLba: %ld, ActLba: %ld", paranoiaActLba_, atime.lba());

	if (t >= paranoiaStartTrack_ && t <= paranoiaEndTrack_ &&
	    atime.lba() > paranoiaActLba_ && 
	    atime.lba() - 150 < paranoiaTrackInfo_[t + 1].start) {
	  Msf time(chan->min(), chan->sec(), chan->frame()); // track rel time

	  paranoiaActLba_ = atime.lba();

	  if (paranoiaActLba_ - paranoiaProgress_ > 75) {
	    paranoiaProgress_ = paranoiaActLba_;
	    Msf m(paranoiaProgress_ - 150);
	    message(1, "%02d:%02d:00\r", m.min(), m.sec());
	  }

	  if (t == paranoiaActTrack_ &&
	      chan->indexNr() == paranoiaActIndex_ + 1) {
	  
	    if (chan->indexNr() > 1) {
	      message(1, "Found index %d at: %s", chan->indexNr(),
		      time.str());
	  
	      if (paranoiaTrackInfo_[t].indexCnt < 98) {
		paranoiaTrackInfo_[t].index[paranoiaTrackInfo_[t].indexCnt] = time.lba();
		paranoiaTrackInfo_[t].indexCnt += 1;
	      }
	    }
	  }
	  else if (t == paranoiaActTrack_ + 1) {
	    message(1, "Track %d...", t + 1);
	    //chan->print();
	    if (chan->indexNr() == 0) {
	      paranoiaTrackInfo_[t].pregap = time.lba();
	      message(1, "Found pre-gap: %s", time.str());
	    }
	  }

	  paranoiaActIndex_ = chan->indexNr();
	  paranoiaActTrack_ = t;
	}
      }
      else if (chan->type() == SubChannel::QMODE3) {
	if (paranoiaTrackInfo_[paranoiaActTrack_].isrcCode[0] == 0) {
	  message(1, "Found ISRC code.");
	  strcpy(paranoiaTrackInfo_[paranoiaActTrack_].isrcCode,
		 chan->isrc());
	}
      }
    }
    else {
      paranoiaCrcCount_++;
    }
  }

  return len;
}

long cdda_read(cdrom_drive *d, void *buffer, long beginsector, long sectors)
{
  CdrDriver *cdr = (CdrDriver*)d->cdr;

  return cdr->paranoiaRead((Sample*)buffer, beginsector, sectors);
}

void CdrDriver::paranoiaCallback(long, int)
{
}
