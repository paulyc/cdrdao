/*  cdrdao - write audio CD-Rs in disc-at-once mode
 *
 *  Copyright (C) 1998  Andreas Mueller <mueller@daneb.ping.de>
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
 * Revision 1.5  2001/01/21 13:46:11  andreasm
 * 'update()' functions of all dialogs require a 'TocEditView' object now.
 * CD TEXT table entry is now a non modal dialog on its own.
 *
 * Revision 1.4  2000/04/23 09:07:08  andreasm
 * * Fixed most problems marked with '//llanero'.
 * * Added audio CD edit menus to MDIWindow.
 * * Moved central storage of TocEdit object to MDIWindow.
 * * AudioCdChild is now handled like an ordinary non modal dialog, i.e.
 *   it has a normal 'update' member function now.
 * * Added CdTextTable modal dialog.
 * * Old functionality of xcdrdao is now available again.
 *
 * Revision 1.3  2000/03/04 01:28:52  llanero
 * SampleDisplay.{cc,h} are fixed now = gtk 1.1.8 compliant.
 *
 * Revision 1.2  2000/02/20 23:34:54  llanero
 * fixed scsilib directory (files mising ?-()
 * ported xdao to 1.1.8 / gnome (MDI) app
 *
 * Revision 1.1.1.1  2000/02/05 01:38:48  llanero
 * Uploaded cdrdao 1.1.3 with pre10 patch applied.
 *
 * Revision 1.2  1999/01/30 15:11:13  mueller
 * First released version. Compiles with Gtk-- 0.9.14.
 *
 * Revision 1.1  1998/11/20 18:55:22  mueller
 * Initial revision
 *
 */

#ifndef __SAMPLE_DISPLAY_H
#define __SAMPLE_DISPLAY_H

#include "TrackManager.h"

class Toc;
class Sample;
class TocEdit;

class SampleDisplay : public Gtk::DrawingArea {
private:
  enum DragMode { DRAG_NONE, DRAG_SAMPLE_MARKER, DRAG_TRACK_MARKER };

  Gtk::Adjustment *adjustment_;

  Glib::RefPtr<Gdk::Pixmap> pixmap_;
  Glib::RefPtr<Gdk::Pixmap> trackMarkerPixmap_;
  Glib::RefPtr<Gdk::Pixmap> indexMarkerPixmap_;
  Glib::RefPtr<Gdk::Pixmap> trackMarkerSelectedPixmap_;
  Glib::RefPtr<Gdk::Pixmap> indexMarkerSelectedPixmap_;
  Glib::RefPtr<Gdk::Pixmap> trackExtendPixmap_;
  Glib::RefPtr<Gdk::Pixmap> indexExtendPixmap_;

  Glib::RefPtr<Gdk::GC> drawGc_;
  Gdk::Color sampleColor_;
  Gdk::Color middleLineColor_;
  Gdk::Color cursorColor_;
  Gdk::Color markerColor_;
  Gdk::Color selectionBackgroundColor_;
  Gdk::Color whiteColor_;
  Gdk::Color blackColor_;

  Pango::FontDescription timeTickFont_;

  gint width_;
  gint height_;
  gint timeLineHeight_;
  gint timeLineY_;
  gint timeTickWidth_;
  gint timeTickSep_;
  gint sampleStartX_;
  gint sampleEndX_;
  gint sampleWidthX_;

  gint trackLineHeight_;
  gint trackLineY_;
  gint trackMarkerWidth_;
  Pango::FontDescription trackMarkerFont_;
  const TrackManager::Entry *pickedTrackMarker_;

  gint chanSep_;
  gint chanHeight_;
  gint lcenter_;
  gint rcenter_;

  TrackManager *trackManager_;

  TocEdit *tocEdit_;
  unsigned long minSample_;
  unsigned long maxSample_;
  unsigned long resolution_;

  int cursorDrawn_;
  gint cursorX_;
  int cursorControlExtern_;

  int markerSet_;
  gint markerX_;
  unsigned long markerSample_;

  int selectionSet_;
  unsigned long selectionStartSample_;
  unsigned long selectionEndSample_;
  gint selectionStart_;
  gint selectionEnd_;

  int regionSet_;
  unsigned long regionStartSample_;
  unsigned long regionEndSample_;

  int selectedTrack_;
  int selectedIndex_;

  DragMode dragMode_;
  gint dragStart_, dragEnd_;
  gint dragStopMin_, dragStopMax_;
  gint dragLastX_;

  void scrollTo();
  void redraw(gint x, gint y, gint width, gint height, int);
  void readSamples(long startBlock, long endBlock);
  void updateSamples();
  void drawCursor(gint);
  void undrawCursor();
  void getColor(const char *, Gdk::Color *);
  unsigned long pixel2sample(gint x);
  gint sample2pixel(unsigned long);
  void drawMarker();
  void removeMarker();
  void drawTimeTick(gint x, gint y, unsigned long sample);
  gint timeTickWidth();
  void drawTimeLine();
  gint trackMarkerWidth();
  void drawTrackMarker(int mode, gint x, int trackNr, int indexNr,
		       int selected, int extend);
  void drawTrackLine();

public:
  SampleDisplay();

  void setTocEdit(TocEdit *);
  int getSelection(unsigned long *start, unsigned long *end);
  void setSelectedTrackMarker(int trackNr, int indexNr);
  void setMarker(unsigned long sample);
  void clearMarker();
  int getMarker(unsigned long *);
  void setView(unsigned long start, unsigned long end);
  void getView(unsigned long *start, unsigned long *end);
  void setRegion(unsigned long start, unsigned long end);
  int getRegion(unsigned long *start, unsigned long *end);
  Gtk::Adjustment *getAdjustment() { return adjustment_; }
  void updateTrackMarks();
  void setCursor(int, unsigned long);

  void updateToc(unsigned long, unsigned long);

  SigC::Signal1<void, unsigned long> markerSet;
  SigC::Signal1<void, unsigned long> cursorMoved;
  SigC::Signal2<void, unsigned long, unsigned long> selectionSet;
  SigC::Signal3<void, const Track *, int, int> trackMarkSelected;
  SigC::Signal4<void, const Track *, int, int, unsigned long> trackMarkMoved;
  SigC::Signal2<void, unsigned long, unsigned long> viewModified;
  
protected:
  bool handle_configure_event (GdkEventConfigure *);
  bool handle_expose_event (GdkEventExpose *event);
  bool handle_motion_notify_event (GdkEventMotion *event);
  bool handleButtonPressEvent(GdkEventButton*);
  bool handleButtonReleaseEvent(GdkEventButton*);
  bool handleEnterEvent(GdkEventCrossing*);
  bool handleLeaveEvent(GdkEventCrossing*);
};

#endif
