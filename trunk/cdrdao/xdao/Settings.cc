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
 * Revision 1.1.1.1  2000/02/05 01:39:57  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 */

static char rcsid[] = "$Id: Settings.cc,v 1.2 2000-05-01 18:15:00 andreasm Exp $";

#include "Settings.h"

const char *SET_CDRDAO_PATH = "/xcdrdao/cdrdao/path=cdrdao";
const char *SET_RECORD_EJECT_WARNING = "/xcdrdao/record/ejectWarning=true";
const char *SET_RECORD_RELOAD_WARNING = "/xcdrdao/record/reloadWarning=true";

const char *SET_SECTION_DEVICES = "/xcdrdao/devices/";
const char *SET_DEVICES_NUM = "/xcdrdao/devices/count=0";