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

#include <iostream.h>

#include <libgnomeuimm.h>

#include "xcdrdao.h"
#include "guiUpdate.h"
#include "MessageBox.h"
#include "SampleDisplay.h"
#include "TocEdit.h"
#include "TocEditView.h"
#include "util.h"

#include "AudioCDProject.h"
#include "AudioCDChild.h"
#include "AudioCDView.h"
#include "Project.h"
#include "TrackInfoDialog.h"
#include "AddFileDialog.h"
#include "AddSilenceDialog.h"

AudioCDView::AudioCDView(AudioCDChild *child, AudioCDProject *project) 
{
  char buf[20];
  gint viewNumber = project->getViewNumber();
  cdchild = child;
  project_ = project;
  tocEditView_ = new TocEditView(project->tocEdit());

  widgetList = new std::list<Gtk::Widget *>;
  widgetList->push_back(this);

// These are not created until first needed, for faster startup
// and less memory usage.
  trackInfoDialog_ = 0;
  addFileDialog_ = 0;
  addSilenceDialog_ = 0;

  Gtk::VBox *vbox = this;
  
  static const GtkTargetEntry drop_types [] =
  {
    { "text/uri-list", 0, TARGET_URI_LIST }
  };

  static gint n_drop_types = sizeof (drop_types) / sizeof(drop_types[0]);

//GTKMM2  drag_dest_set(static_cast <GtkDestDefaults> (GTK_DEST_DEFAULT_MOTION
//GTKMM2    | GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP),
//GTKMM2    &drop_types[0], n_drop_types, GDK_ACTION_COPY);
  
//GTKMM2  signal_drag_data_received().connect(slot(*this, &AudioCDView::drag_data_received_cb));

  sampleDisplay_ = new SampleDisplay;
  sampleDisplay_->setTocEdit(child->tocEdit());

//GTKMM2  sampleDisplay_->set_default_size(200,200);
  
  vbox->pack_start(*sampleDisplay_, TRUE, TRUE);
  sampleDisplay_->show();

  Gtk::HScrollbar *scrollBar =
    new Gtk::HScrollbar(*(sampleDisplay_->getAdjustment()));
  vbox->pack_start(*scrollBar, FALSE, FALSE);
  scrollBar->show();
  
  Gtk::Label *label;
  Gtk::HBox *selectionInfoBox = new Gtk::HBox;

//FIXME: Calculate entry width for the current font.
  gint entry_width = 90;

  markerPos_ = new Gtk::Entry;
  markerPos_->set_editable(true);
//GTKMM2  markerPos_->set_default_size(entry_width, 0);
  markerPos_->signal_activate().connect(slot(*this, &AudioCDView::markerSet));

  cursorPos_ = new Gtk::Entry;
//GTKMM2  cursorPos_->set_default_size(entry_width, 0);
  cursorPos_->set_editable(false);

  selectionStartPos_ = new Gtk::Entry;
  selectionStartPos_->set_editable(true);
//GTKMM2  selectionStartPos_->set_default_size(entry_width, 0);
  selectionStartPos_->signal_activate().connect(slot(*this, &AudioCDView::selectionSet));

  selectionEndPos_ = new Gtk::Entry;
  selectionEndPos_->set_editable(true);
//GTKMM2  selectionEndPos_->set_default_size(entry_width, 0);
  selectionEndPos_->signal_activate().connect(slot(*this, &AudioCDView::selectionSet));

  label = new Gtk::Label("Cursor: ");
  selectionInfoBox->pack_start(*label, FALSE, FALSE);
  selectionInfoBox->pack_start(*cursorPos_);
  label->show();
  cursorPos_->show();

  label = new Gtk::Label("Marker: ");
  selectionInfoBox->pack_start(*label, FALSE, FALSE);
  selectionInfoBox->pack_start(*markerPos_);
  label->show();
  markerPos_->show();

  label = new Gtk::Label("Selection: ");
  selectionInfoBox->pack_start(*label, FALSE, FALSE);
  selectionInfoBox->pack_start(*selectionStartPos_);
  label->show();
  selectionStartPos_->show();

  label = new Gtk::Label(" - ");
  selectionInfoBox->pack_start(*label, FALSE, FALSE);
  selectionInfoBox->pack_start(*selectionEndPos_);
  label->show();
  selectionEndPos_->show();
  
  selectionInfoBox->set_border_width(2);
  sprintf(buf, "selectionBox-%i", viewNumber);
  project->add_docked(*selectionInfoBox, buf, (BonoboDockItemBehavior)8,
  		(BonoboDockPlacement)2, 1, 1, 0);
//GTKMM2  widgetList->push_back(project->get_dock_item_by_name(buf));
  selectionInfoBox->show();

  Gtk::Toolbar *playToolbar = project->getPlayToolbar();
  Gtk::Image *pixmap;
  Gtk::RadioButton_Helpers::Group playGroup;
  Gtk::Widget* tool;

  pixmap = manage(new Gtk::Image(PIXMAPS_DIR "/stock_media-play.png"));
  playToolbar->tools().push_back(Gtk::Toolbar_Helpers::RadioElem(playGroup, "Play", *pixmap,
  		slot(*this, &AudioCDView::playStart), "Play", ""));
  playStartButton_ = static_cast <Gtk::RadioButton *>(playToolbar->tools().back().get_widget());
  playStartButton_->hide();
  widgetList->push_back(playStartButton_);

  pixmap = manage(new Gtk::Image(PIXMAPS_DIR "/stock_media-pause.png"));
  playToolbar->tools().push_back(Gtk::Toolbar_Helpers::RadioElem(playGroup, "Pause", *pixmap,
  		slot(*this, &AudioCDView::playPause), "Pause", ""));
  playPauseButton_ = static_cast <Gtk::RadioButton *>(playToolbar->tools().back().get_widget());
  playPauseButton_->hide();
  widgetList->push_back(playPauseButton_);

  pixmap = manage(new Gtk::Image(PIXMAPS_DIR "/stock_media-stop.png"));
  playToolbar->tools().push_back(Gtk::Toolbar_Helpers::RadioElem(playGroup, "Stop", *pixmap,
  	    slot(*this, &AudioCDView::playStop), "Stop", ""));
  playStopButton_ = static_cast <Gtk::RadioButton *>(playToolbar->tools().back().get_widget());
  playStopButton_->set_active(true);
  // We need to connect the signal after we set it, so no signal is emitted.
//GTKMM2  playStopButton_->signal_toggled().connect(slot(*this, &AudioCDView::playStop));
  playStopButton_->hide();
  widgetList->push_back(playStopButton_);

  setMode(SELECT);

  Gtk::Toolbar *zoomToolbar = child->getZoomToolbar();
  Gtk::RadioButton_Helpers::Group toolGroup;

  widgetList->push_back(zoomToolbar);

//GTKMM2  pixmap = manage(new Gnome::Pixmap(Gnome::Pixmap::find_file("gcdmaster/pixmap_cursor-tool.xpm")));
  pixmap = manage(new Gtk::Image(PIXMAPS_DIR "/pixmap_cursor-tool.xpm"));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::RadioElem(toolGroup, "Select", *pixmap,
  		bind(slot(*this, &AudioCDView::setMode), SELECT), "Selection tool", ""));
  tool = zoomToolbar->tools().back().get_widget();
  tool->hide();
  widgetList->push_back(tool);

//GTKMM2  pixmap = manage(new Gnome::Pixmap(Gnome::Pixmap::find_file("gcdmaster/pixmap_zoom-tool.xpm")));
  pixmap = manage(new Gtk::Image(PIXMAPS_DIR "/pixmap_zoom-tool.xpm"));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::RadioElem(toolGroup, "Zoom", *pixmap,
  		bind(slot(*this, &AudioCDView::setMode), ZOOM), "Zoom tool", ""));
  tool = zoomToolbar->tools().back().get_widget();
  tool->hide();
  widgetList->push_back(tool);

  pixmap = manage(new Gtk::Image(Gtk::StockID(Gtk::Stock::ZOOM_IN), Gtk::ICON_SIZE_LARGE_TOOLBAR));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Zoom +", *pixmap,
  		slot(*this, &AudioCDView::zoomx2), "Zoom In", ""));
  tool = zoomToolbar->tools().back().get_widget();
  tool->hide();
  widgetList->push_back(tool);

  pixmap = manage(new Gtk::Image(Gtk::StockID(Gtk::Stock::ZOOM_OUT), Gtk::ICON_SIZE_LARGE_TOOLBAR));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Zoom -", *pixmap,
  		slot(*this, &AudioCDView::zoomOut), "Zoom Out", ""));
  tool = zoomToolbar->tools().back().get_widget();
  tool->hide();
  widgetList->push_back(tool);

  pixmap = manage(new Gtk::Image(Gtk::StockID(Gtk::Stock::ZOOM_FIT), Gtk::ICON_SIZE_LARGE_TOOLBAR));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Fit", *pixmap,
  		slot(*this, &AudioCDView::fullView), "Full View", ""));
  tool = zoomToolbar->tools().back().get_widget();
  tool->hide();
  widgetList->push_back(tool);

  pixmap = manage(new Gtk::Image(Gtk::StockID(Gtk::Stock::ZOOM_100), Gtk::ICON_SIZE_LARGE_TOOLBAR));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Fit Sel", *pixmap,
  		slot(*this, &AudioCDView::zoomIn), "Zoom to fit the selection", ""));
  tool = zoomToolbar->tools().back().get_widget();
  tool->hide();
  widgetList->push_back(tool);


  sampleDisplay_->markerSet.connect(slot(*this,
  			&AudioCDView::markerSetCallback));
  sampleDisplay_->selectionSet.connect(slot(*this,
  			&AudioCDView::selectionSetCallback));
  sampleDisplay_->cursorMoved.connect(slot(*this,
  			&AudioCDView::cursorMovedCallback));
  sampleDisplay_->trackMarkSelected.connect(slot(*this,
  			&AudioCDView::trackMarkSelectedCallback));
  sampleDisplay_->trackMarkMoved.connect(slot(*this,
  			&AudioCDView::trackMarkMovedCallback));
  sampleDisplay_->viewModified.connect(slot(*this,
		        &AudioCDView::viewModifiedCallback));

  tocEditView_->sampleViewFull();
  
  // Menu Stuff
  {
    using namespace Gnome::UI::Items;
    vector<Info> menus;
	Info info;
    int i;
    
    menus.push_back(Item(Icon(Gtk::Stock::PROPERTIES.id),
    				 N_("Track Info..."),
  			      slot(*this, &AudioCDView::trackInfo),
  			      N_("Edit track data")));
  
    menus.push_back(Separator());
  
	info = Item(Icon(Gtk::Stock::CUT.id),
  			      N_("Cut"),
  			      slot(*this, &AudioCDView::cutTrackData),
  			      N_("Cut out selected samples"));

    info.set_accel(Gtk::Menu_Helpers::AccelKey("<control>X"));
    menus.push_back(info);

    info = Item(Icon(Gtk::Stock::PASTE.id),
  			      N_("Paste"),
  			      slot(*this, &AudioCDView::pasteTrackData),
  			      N_("Paste previously cut samples"));
	info.set_accel(Gtk::Menu_Helpers::AccelKey("<control>V"));
    menus.push_back(info);

    menus.push_back(Separator());

    info = Item(N_("Add Track Mark"),
			      slot(*this, &AudioCDView::addTrackMark),
			      N_("Add track marker at current marker position"));
	info.set_accel(Gtk::Menu_Helpers::AccelKey("T"));
    menus.push_back(info);

    info = Item(N_("Add Index Mark"),
			      slot(*this, &AudioCDView::addIndexMark),
			      N_("Add index marker at current marker position"));
	info.set_accel(Gtk::Menu_Helpers::AccelKey("I"));
    menus.push_back(info);

    info = Item(N_("Add Pre-Gap"),
			      slot(*this, &AudioCDView::addPregap),
			      N_("Add pre-gap at current marker position"));
	info.set_accel(Gtk::Menu_Helpers::AccelKey("P"));
    menus.push_back(info);

    info = Item(N_("Remove Track Mark"),
			      slot(*this, &AudioCDView::removeTrackMark),
			      N_("Remove selected track/index marker or pre-gap"));
	info.set_accel(Gtk::Menu_Helpers::AccelKey("<control>D"));
    menus.push_back(info);
 
    menus.push_back(Separator());

    info = Item(N_("Append Track"),
			      slot(*this, &AudioCDView::appendTrack),
			      N_("Append track with data from audio file"));
	info.set_accel(Gtk::Menu_Helpers::AccelKey("<control>T"));
    menus.push_back(info);

    info = Item(N_("Append File"),
			      slot(*this, &AudioCDView::appendFile),
			      N_("Append data from audio file to last track"));
	info.set_accel(Gtk::Menu_Helpers::AccelKey("<control>F"));
    menus.push_back(info);
  
    info = Item(N_("Insert File"),
			      slot(*this, &AudioCDView::insertFile),
			      N_("Insert data from audio file at current marker position"));
	info.set_accel(Gtk::Menu_Helpers::AccelKey("<control>I"));
    menus.push_back(info);

    menus.push_back(Separator());

    menus.push_back(Item(N_("Append Silence"),
			      slot(*this, &AudioCDView::appendSilence),
			      N_("Append silence to last track")));

    menus.push_back(Item(N_("Insert Silence"),
			      slot(*this, &AudioCDView::insertSilence),
			      N_("Insert silence at current marker position")));

    Array<Info>& arrayInfo = project->insert_menus("Edit/CD-TEXT...", menus);
    for (i = 0; i < menus.size(); i ++)
    {
      Gtk::Widget *menuitem = arrayInfo[i].get_widget();
      menuitem->hide();
      widgetList->push_back(menuitem);
    }
  }
}

AudioCDView::~AudioCDView()
{
  if (trackInfoDialog_)
    delete trackInfoDialog_;

  if (addFileDialog_)
    delete addFileDialog_;

  if (addSilenceDialog_)
    delete addSilenceDialog_;
}

void AudioCDView::update(unsigned long level)
{
  if (level & (UPD_TOC_DIRTY | UPD_TOC_DATA)) {
    cursorPos_->set_text("");
  }

  if (level & UPD_TRACK_MARK_SEL) {
    int trackNr, indexNr;

    if (tocEditView_->trackSelection(&trackNr) && 
	tocEditView_->indexSelection(&indexNr)) {
      sampleDisplay_->setSelectedTrackMarker(trackNr, indexNr);
    }
    else {
      sampleDisplay_->setSelectedTrackMarker(0, 0);
    }
  }

  if (level & UPD_SAMPLES) {
    unsigned long smin, smax;

    tocEditView_->sampleView(&smin, &smax);
    sampleDisplay_->updateToc(smin, smax);
  }
  else if (level & (UPD_TRACK_DATA | UPD_TRACK_MARK_SEL)) {
    sampleDisplay_->updateTrackMarks();
  }

  if (level & UPD_SAMPLE_MARKER) {
    unsigned long marker;

    if (tocEditView_->sampleMarker(&marker)) {
      markerPos_->set_text(cdchild->sample2string(marker));
      sampleDisplay_->setMarker(marker);
    }
    else {
      markerPos_->set_text("");
      sampleDisplay_->clearMarker();
    }
  }

  if (level & UPD_SAMPLE_SEL) {
    unsigned long start, end;

    if (tocEditView_->sampleSelection(&start, &end)) {
      selectionStartPos_->set_text(cdchild->sample2string(start));
      selectionEndPos_->set_text(cdchild->sample2string(end));
      sampleDisplay_->setRegion(start, end);
    }
    else {
      selectionStartPos_->set_text("");
      selectionEndPos_->set_text("");
      sampleDisplay_->setRegion(1, 0);
    }
  }

  if (level & UPD_PLAY_STATUS) {
    switch (project_->getPlayStatus()) {
      case AudioCDProject::PLAYING:
        playStartButton_->set_active(true);
        sampleDisplay_->setCursor(1, project_->playPosition() - project_->getDelay());
// FIXME: What about using a separate cursor for playing?
        cursorPos_->set_text(cdchild->sample2string(project_->playPosition() - project_->getDelay()));
        break;
      case AudioCDProject::PAUSED:
        playPauseButton_->set_active(true);
        sampleDisplay_->setCursor(1, project_->playPosition() - project_->getDelay());
// FIXME: What about using a separate cursor for playing?
        cursorPos_->set_text(cdchild->sample2string(project_->playPosition() - project_->getDelay()));
        break;
      case AudioCDProject::STOPPED:
        playStopButton_->set_active(true);
        sampleDisplay_->setCursor(0, 0);
        break;
      default:
        std::cerr << "invalid play status" << std::endl;
    }
  }

  if (trackInfoDialog_ != 0)
    trackInfoDialog_->update(level, tocEditView_);

  if (addFileDialog_ != 0)
    addFileDialog_->update(level, tocEditView_);

  if (addSilenceDialog_ != 0)
    addSilenceDialog_->update(level, tocEditView_);

}

void AudioCDView::zoomIn()
{
  unsigned long start, end;

  if (tocEditView_->sampleSelection(&start, &end)) {
    tocEditView_->sampleView(start, end);
    guiUpdate();
  }
}

void AudioCDView::zoomx2()
{
  unsigned long start, end, len, center;

  tocEditView_->sampleView(&start, &end);

  len = end - start + 1;
  center = start + len / 2;

  start = center - len / 4;
  end = center + len / 4;

  tocEditView_->sampleView(start, end);
  guiUpdate();
}

void AudioCDView::zoomOut()
{
  unsigned long start, end, len, center;

  tocEditView_->sampleView(&start, &end);

  len = end - start + 1;
  center = start + len / 2;

  if (center > len)
    start = center - len;
  else 
    start = 0;

  end = center + len;
  if (end >= tocEditView_->tocEdit()->toc()->length().samples())
    end = tocEditView_->tocEdit()->toc()->length().samples() - 1;

  tocEditView_->sampleView(start, end);
  guiUpdate();
}

void AudioCDView::fullView()
{
  tocEditView_->sampleViewFull();

  guiUpdate();
}

void AudioCDView::playStart()
{
  unsigned long start, end;

  if (!playStartButton_->get_active())
    return;

  if (project_->getPlayStatus() == AudioCDProject::PLAYING)
    return;

  if (!(project_->getPlayStatus() == AudioCDProject::PAUSED))
  {
    if (!tocEditView_->sampleSelection(&start, &end))
      tocEditView_->sampleView(&start, &end);
  }

  project_->playStart(start, end);
}

void AudioCDView::playPause()
{
  if (!playPauseButton_->get_active())
    return;

  if (project_->getPlayStatus() == AudioCDProject::PAUSED)
    return;

  if (project_->getPlayStatus() == AudioCDProject::STOPPED)
  {
    playStopButton_->set_active(true);
    return;
  }

  project_->playPause();
}

void AudioCDView::playStop()
{
  unsigned long start, end;

  if (!playStopButton_->get_active())
    return;

  if (project_->getPlayStatus() == AudioCDProject::STOPPED)
    return;

  project_->playStop();
}

int AudioCDView::getMarker(unsigned long *sample)
{
  if (tocEditView_->tocEdit()->lengthSample() == 0)
    return 0;

  if (sampleDisplay_->getMarker(sample) == 0) {
    project_->statusMessage("Please set marker.");
    return 0;
  }

  return 1;
}

void AudioCDView::trackMarkSelectedCallback(const Track *, int trackNr,
					   int indexNr)
{
  tocEditView_->trackSelection(trackNr);
  tocEditView_->indexSelection(indexNr);
  guiUpdate();
}

void AudioCDView::markerSetCallback(unsigned long sample)
{
  tocEditView_->sampleMarker(sample);
  guiUpdate();
}

void AudioCDView::selectionSetCallback(unsigned long start,
				      unsigned long end)
{
  if (mode_ == ZOOM) {
    tocEditView_->sampleView(start, end);
  }
  else {
    tocEditView_->sampleSelection(start, end);
  }

  //cout << "selectionSetCallback called" << endl;

  guiUpdate();
}

void AudioCDView::cursorMovedCallback(unsigned long pos)
{
  cursorPos_->set_text(cdchild->sample2string(pos));
}

void AudioCDView::viewModifiedCallback(unsigned long start, unsigned long end)
{
  tocEditView_->sampleView(start, end);
  guiUpdate();
}

void AudioCDView::setMode(Mode m)
{
  mode_ = m;
}

void AudioCDView::markerSet()
{
  unsigned long s = cdchild->string2sample(markerPos_->get_text().c_str());

  tocEditView_->sampleMarker(s);
  guiUpdate();
}

void AudioCDView::selectionSet()
{
  unsigned long s1 = cdchild->string2sample(selectionStartPos_->get_text().c_str());
  unsigned long s2 = cdchild->string2sample(selectionEndPos_->get_text().c_str());

  tocEditView_->sampleSelection(s1, s2);
  guiUpdate();
}

void AudioCDView::drag_data_received_cb(GdkDragContext *context,
  gint x, gint y, GtkSelectionData *selection_data, guint info, guint time)
{
/*//GTKMM2
  GList *names;
  
  switch (info) {
    case TARGET_URI_LIST:
      names = (GList *)gnome_uri_list_extract_filenames \
			((char *)selection_data->data);  

//  	tocEdit_->blockEdit();
//FIXME      while (names->data) {
      if (names->data) {
          std::string str = g_strdup(static_cast <char *>(names->data));
          const char *file = stripCwd(str.c_str());

        switch (tocEditView_->tocEdit()->appendTrack(file)) {
        case 0:
	      guiUpdate();
	      project_->statusMessage("Appended track with audio data from \"%s\".", file);
	      break;
        case 1:
	      project_->statusMessage("Cannot open audio file \"%s\".", file);
	      break;
        case 2:
	      project_->statusMessage("Audio file \"%s\" has wrong format.", file);
	      break;
	    }
	    names = g_list_remove(names, names->data);
	    if (names == NULL)
	      break;
      }
//	tocEdit_->unblockEdit();
    break;
  }
*/
}

void AudioCDView::trackInfo()
{
  int track;
  
  if (tocEditView_->trackSelection(&track))
  {
    if (trackInfoDialog_ == 0)
      trackInfoDialog_ = new TrackInfoDialog();
  
    trackInfoDialog_->start(tocEditView_);
  }
  else
  {
      Gtk::MessageDialog(*project_, "Please select a track first", Gtk::MESSAGE_INFO);
  }

}

void AudioCDView::cutTrackData()
{
  if (!project_->tocEdit()->editable()) {
    project_->tocBlockedMsg("Cut");
    return;
  }

  switch (project_->tocEdit()->removeTrackData(tocEditView_)) {
  case 0:
    project_->statusMessage("Removed selected samples.");
    guiUpdate();
    break;
  case 1:
    project_->statusMessage("Please select samples.");
    break;
  case 2:
    project_->statusMessage("Selected sample range crosses track boundaries.");
    break;
  }
}

void AudioCDView::pasteTrackData()
{
  if (!project_->tocEdit()->editable()) {
    project_->tocBlockedMsg("Paste");
    return;
  }

  switch (project_->tocEdit()->insertTrackData(tocEditView_)) {
  case 0:
    project_->statusMessage("Pasted samples.");
    guiUpdate();
    break;
  case 1:
    project_->statusMessage("No samples in scrap.");
    break;
  }
}

void AudioCDView::addTrackMark()
{
  unsigned long sample;

  if (!project_->tocEdit()->editable()) {
    project_->tocBlockedMsg("Add Track Mark");
    return;
  }

  if (getMarker(&sample)) {
    long lba;
    int snapped = snapSampleToBlock(sample, &lba);

    switch (project_->tocEdit()->addTrackMarker(lba)) {
    case 0:
      project_->statusMessage("Added track mark at %s%s.", Msf(lba).str(),
				snapped ? " (snapped to next block)" : "");
      guiUpdate();
      break;

    case 2:
      project_->statusMessage("Cannot add track at this point.");
      break;

    case 3:
    case 4:
      project_->statusMessage("Resulting track would be shorter than 4 seconds.");
      break;

    case 5:
      project_->statusMessage("Cannot modify a data track.");
      break;

    default:
      project_->statusMessage("Internal error in addTrackMark(), please report.");
      break;
    }
  }
}

void AudioCDView::addIndexMark()
{
  unsigned long sample;

  if (!project_->tocEdit()->editable()) {
    project_->tocBlockedMsg("Add Index Mark");
    return;
  }

  if (getMarker(&sample)) {
    long lba;
    int snapped = snapSampleToBlock(sample, &lba);

    switch (project_->tocEdit()->addIndexMarker(lba)) {
    case 0:
      project_->statusMessage("Added index mark at %s%s.", Msf(lba).str(),
				snapped ? " (snapped to next block)" : "");
      guiUpdate();
      break;

    case 2:
      project_->statusMessage("Cannot add index at this point.");
      break;

    case 3:
      project_->statusMessage("Track has already 98 index marks.");
      break;

    default:
      project_->statusMessage("Internal error in addIndexMark(), please report.");
      break;
    }
  }
}

void AudioCDView::addPregap()
{
  unsigned long sample;

  if (!project_->tocEdit()->editable()) {
    project_->tocBlockedMsg("Add Pre-Gap");
    return;
  }

  if (getMarker(&sample)) {
    long lba;
    int snapped = snapSampleToBlock(sample, &lba);

    switch (project_->tocEdit()->addPregap(lba)) {
    case 0:
      project_->statusMessage("Added pre-gap mark at %s%s.", Msf(lba).str(),
				snapped ? " (snapped to next block)" : "");
      guiUpdate();
      break;

    case 2:
      project_->statusMessage("Cannot add pre-gap at this point.");
      break;

    case 3:
      project_->statusMessage("Track would be shorter than 4 seconds.");
      break;

    case 4:
      project_->statusMessage("Cannot modify a data track.");
      break;

    default:
      project_->statusMessage("Internal error in addPregap(), please report.");
      break;
    }
  }
}

void AudioCDView::removeTrackMark()
{
  int trackNr;
  int indexNr;

  if (!project_->tocEdit()->editable()) {
    project_->tocBlockedMsg("Remove Track Mark");
    return;
  }

  if (tocEditView_->trackSelection(&trackNr) &&
      tocEditView_->indexSelection(&indexNr)) {
    switch (project_->tocEdit()->removeTrackMarker(trackNr, indexNr)) {
    case 0:
      project_->statusMessage("Removed track/index marker.");
      guiUpdate();
      break;
    case 1:
      project_->statusMessage("Cannot remove first track.");
      break;
    case 3:
      project_->statusMessage("Cannot modify a data track.");
      break;
    default:
      project_->statusMessage("Internal error in removeTrackMark(), please report.");
      break; 
    }
  }
  else {
    project_->statusMessage("Please select a track/index mark.");
  }
}

int AudioCDView::snapSampleToBlock(unsigned long sample, long *block)
{
  unsigned long rest = sample % SAMPLES_PER_BLOCK;

  *block = sample / SAMPLES_PER_BLOCK;

  if (rest == 0) 
    return 0;

  if (rest > SAMPLES_PER_BLOCK / 2)
    *block += 1;

  return 1;
}

void AudioCDView::trackMarkMovedCallback(const Track *, int trackNr,
					int indexNr, unsigned long sample)
{
  if (!project_->tocEdit()->editable()) {
    project_->tocBlockedMsg("Move Track Marker");
    return;
  }

  long lba;
  int snapped = snapSampleToBlock(sample, &lba);

  switch (project_->tocEdit()->moveTrackMarker(trackNr, indexNr, lba)) {
  case 0:
    project_->statusMessage("Moved track marker to %s%s.", Msf(lba).str(),
			      snapped ? " (snapped to next block)" : "");
    break;

  case 6:
    project_->statusMessage("Cannot modify a data track.");
    break;
  default:
    project_->statusMessage("Illegal track marker position.");
    break;
  }

  tocEditView_->trackSelection(trackNr);
  tocEditView_->indexSelection(indexNr);

  guiUpdate();
}

void AudioCDView::appendTrack()
{
  if (addFileDialog_ == 0)
    addFileDialog_ = new AddFileDialog(project_);

  addFileDialog_->mode(AddFileDialog::M_APPEND_TRACK);
  addFileDialog_->start(tocEditView_);
}

void AudioCDView::appendFile()
{
  if (addFileDialog_ == 0)
    addFileDialog_ = new AddFileDialog(project_);

  addFileDialog_->mode(AddFileDialog::M_APPEND_FILE);
  addFileDialog_->start(tocEditView_);
}

void AudioCDView::insertFile()
{
  if (addFileDialog_ == 0)
    addFileDialog_ = new AddFileDialog(project_);

  addFileDialog_->mode(AddFileDialog::M_INSERT_FILE);
  addFileDialog_->start(tocEditView_);
}

void AudioCDView::appendSilence()
{
  if (addSilenceDialog_ == 0)
    addSilenceDialog_ = new AddSilenceDialog();

  addSilenceDialog_->mode(AddSilenceDialog::M_APPEND);
  addSilenceDialog_->start(tocEditView_);
}

void AudioCDView::insertSilence()
{
  if (addSilenceDialog_ == 0)
    addSilenceDialog_ = new AddSilenceDialog();

  addSilenceDialog_->mode(AddSilenceDialog::M_INSERT);
  addSilenceDialog_->start(tocEditView_);
}
