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
 * Revision 1.2  1999/03/27 19:50:44  mueller
 * Renamed class 'AudioData' to 'TrackData'.
 *
 * Revision 1.1  1998/11/15 12:17:06  mueller
 * Initial revision
 *
 */


#ifndef __TRACKDATALIST_H__
#define __TRACKDATALIST_H__

class TrackData;

class TrackDataList {
public:
  TrackDataList();
  ~TrackDataList() { clear(); }
  
  void append(TrackData *);
  void clear();
  
  long count() const { return count_; }

  unsigned long length() const;

  const TrackData *first() const;
  const TrackData *next() const;

private:
  struct Entry {
    TrackData *data;
    Entry *next;
  };

  Entry *list_;
  Entry *last_;
  Entry *iterator_;

  long count_;
};


#endif
