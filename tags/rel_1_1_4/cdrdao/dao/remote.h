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
 * Revision 1.2  1999/12/15 20:31:46  mueller
 * Added remote messages for 'read-cd' progress used by a GUI.
 *
 * Revision 1.1  1999/11/07 09:17:08  mueller
 * Initial revision
 *
 */

#ifndef __REMOTE_H__
#define __REMOTE_H__

struct DaoWritingProgress {
  int status; // 1: writing lead-in, 2: writing data, 3: writing lead-out
  int track; // actually written track
  int totalProgress; // total writing progress 0..1000
  int bufferFillRate; // buffer fill rate 0..100
};

struct ReadCdProgress {
  int status; // 1: analyzing, 2: extracting
  int track; // actually processed track
  // int totalTracks; // total number of tracks
  int trackProgress; // extraction progress for track 0..1000
};

#endif
