/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998-2001 Andreas Mueller <mueller@daneb.ping.de>
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

#include <libgnomeuimm.h>

#include "xcdrdao.h"
#include "ProgressDialog.h"
#include "ProcessMonitor.h"
#include "TocEdit.h"
#include "guiUpdate.h"

#include "util.h"

void guiUpdate(unsigned long level)
{
//GTKMM2  if (gcdmaster == 0)
//GTKMM2    return;

//GTKMM2  gcdmaster->update(level);

  if (PROGRESS_POOL != NULL)
    PROGRESS_POOL->update(level);
}
