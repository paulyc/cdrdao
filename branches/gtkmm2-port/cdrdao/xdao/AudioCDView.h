/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 2000  Andreas Mueller <mueller@daneb.ping.de>
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

#ifndef __AUDIO_CD_VIEW_H__
#define __AUDIO_CD_VIEW_H__

#include "GenericView.h"
#include <list>

class SampleDisplay;
class Project;
class TrackInfoDialog;
class AddFileDialog;
class AddSilenceDialog;
class AudioCDChild;
class AudioCDProject;

enum {
  TARGET_URI_LIST,
};

class AudioCDView : public GenericView
{
public:
  AudioCDView(AudioCDChild *child, AudioCDProject *project);
  ~AudioCDView();
  SigC::Signal0<void> add_view;

  enum Mode { ZOOM, SELECT };

  void setMode(Mode m);
  void updatePlayPos(bool managed, unsigned long pos);
  void getSelection(unsigned long &start, unsigned long &end);

  void update(unsigned long level);

private:
  friend class AudioCDChild;
  AudioCDProject *project_;

  TrackInfoDialog *trackInfoDialog_;
  AddFileDialog *addFileDialog_;
  AddSilenceDialog *addSilenceDialog_;

  AudioCDChild *cdchild;

  SampleDisplay *sampleDisplay_;

  Gtk::Entry *markerPos_;
  Gtk::Entry *cursorPos_;
  Gtk::Entry *selectionStartPos_;
  Gtk::Entry *selectionEndPos_;

  Mode mode_;

  void markerSetCallback(unsigned long);
  void markerSetUpdate();
  void cursorMovedCallback(unsigned long);
  void selectionSetCallback(unsigned long, unsigned long);
  void selectionSetUpdate();
  void trackMarkSelectedCallback(const Track *, int trackNr, int indexNr);
  void trackMarkSelectedUpdate();
  void trackMarkMovedCallback(const Track *, int trackNr, int indexNr,
			      unsigned long sample);
  void viewModifiedCallback(unsigned long, unsigned long);
  int snapSampleToBlock(unsigned long sample, long *block);

  void trackInfo();
  void cutTrackData();
  void pasteTrackData();

  void addTrackMark();
  void addIndexMark();
  void addPregap();
  void removeTrackMark();

  void appendSilence();
  void insertSilence();

  void appendTrack();
  void appendFile();
  void insertFile();

  void samplesUpdate();
  int getMarker(unsigned long *sample);
  void markerSet();

  void selectionSet();

  void drag_data_received_cb(GdkDragContext *context, gint x, gint y,
         GtkSelectionData *selection_data, guint info, guint time);

};

#endif

