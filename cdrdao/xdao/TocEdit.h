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

#ifndef __TOC_EDIT_H__
#define __TOC_EDIT_H__

#include <string>
#include <list>

#include "Toc.h"
#include "CdTextItem.h"

class Toc;
class TrackData;
class TrackDataScrap;
class SampleManager;
class TocEditView;
class Converter;

class TocEdit {
public:
  TocEdit(Toc *, const char *);
  ~TocEdit();

  void toc(Toc *, const char *);
  Toc *toc() const;

  SampleManager *sampleManager();

  unsigned long lengthSample() const;

  void tocDirty(int);
  int tocDirty() const;

  void blockEdit();
  void unblockEdit();
  bool editable() const;

  // returns and resets update level
  unsigned long updateLevel();

  void filename(const char *);
  const char *filename() const;

  int readToc(const char *);
  int saveToc();
  int saveAsToc(const char *);
  
  int moveTrackMarker(int trackNr, int indexNr, long lba);
  int addTrackMarker(long lba);
  int removeTrackMarker(int trackNr, int indexNr);
  int addIndexMarker(long lba);
  int addPregap(long lba);

  int appendTrack(const char* filename);
  int appendTracks(std::list<std::string>& tracks);
  int appendFile(const char* filename);
  int appendFiles(std::list<std::string>& tracks);
  int insertFile(const char *fname, unsigned long pos, unsigned long *len);
  int insertFiles(std::list<std::string>& tracks, unsigned long pos,
                  unsigned long *len);
  int appendSilence(unsigned long);
  int insertSilence(unsigned long length, unsigned long pos);

  int removeTrackData(TocEditView *);
  int insertTrackData(TocEditView *);
  
  void setTrackCopyFlag(int trackNr, int flag);
  void setTrackPreEmphasisFlag(int trackNr, int flag);
  void setTrackAudioType(int trackNr, int flag);
  void setTrackIsrcCode(int trackNr, const char *);

  void setCdTextItem(int trackNr, CdTextItem::PackType, int blockNr,
		     const char *);
  void setCdTextGenreItem(int blockNr, int code1, int code2,
			  const char *description);
  void setCdTextLanguage(int blockNr, int langCode);

  void setCatalogNumber(const char *);
  void setTocType(Toc::TocType);
  
private:
  Toc *toc_;
  SampleManager *sampleManager_;
  Converter* converter_;

  char *filename_;

  TrackDataScrap *trackDataScrap_;

  int tocDirty_;
  int editBlocked_;

  unsigned long updateLevel_;

  int createAudioData(const char *filename, TrackData **);
  int modifyAllowed() const;

  friend class TocEditView;
};

#endif
