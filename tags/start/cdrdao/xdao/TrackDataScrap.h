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
 * Revision 1.1  1999/08/27 18:39:38  mueller
 * Initial revision
 *
 */

#ifndef __TRACK_DATA_SCRAP_H__
#define __TRACK_DATA_SCRAP_H__

class TrackDataList;

class TrackDataScrap {
public:
  TrackDataScrap(TrackDataList *);
  ~TrackDataScrap();

  const TrackDataList *trackDataList() const;

  void setPeaks(long blocks,
		short *leftNegSamples, short *leftPosSamples,
		short *rightNegSamples, short *rightPosSamples);
  void getPeaks(long blocks,
		short *leftNegSamples, short *leftPosSamples,
		short *rightNegSamples, short *rightPosSamples) const;

private:
  TrackDataList *list_;
  long blocks_;

  short *leftNegSamples_;
  short *leftPosSamples_;
  short *rightNegSamples_;
  short *rightPosSamples_;
};

#endif
