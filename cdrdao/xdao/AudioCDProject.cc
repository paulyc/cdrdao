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

#include <libgnomeuimm.h>

#include "Toc.h"
#include "SoundIF.h"
#include "AudioCDChild.h"
#include "AudioCDView.h"
#include "TocEdit.h"
#include "TocEditView.h"
#include "TocInfoDialog.h"
#include "CdTextDialog.h"
#include "guiUpdate.h"
#include "util.h"
#include "RecordTocDialog.h"
#include "AudioCDProject.h"

AudioCDProject::AudioCDProject(int number, const char *name, TocEdit *tocEdit)
{
  hbox = new Gtk::HBox;
  hbox->show();
  set_contents(*hbox);

  projectNumber_ = number;

  tocInfoDialog_ = 0;
  cdTextDialog_ = 0;
  playStatus_ = STOPPED;
  playBurst_ = 588 * 10;
  playBuffer_ = new Sample[playBurst_];
  soundInterface_ = NULL;

  if (tocEdit == NULL)
    tocEdit_ = new TocEdit(NULL, NULL);
  else
    tocEdit_ = tocEdit;

  if (strlen(name) == 0)
  {
    char buf[20];
    if (projectNumber_ == 0)
      sprintf(buf, "unnamed.toc", projectNumber_);
    else
      sprintf(buf, "unnamed-%i.toc", projectNumber_);
    tocEdit_->filename(buf);
    new_ = true;
  }
  else
    new_ = false; // The project file already exists
  
  updateWindowTitle();

  // Menu Stuff
  {
    using namespace Gnome::UI::Items;
    vector<Info> menus, viewMenuTree;

    menus.push_back(Item(Icon(Gtk::Stock::PROPERTIES),
    				 N_("CD-TEXT..."),
			      slot(*this, &AudioCDProject::cdTextDialog),
			      N_("Edit CD-TEXT data")));
    insert_menus("Edit/Project Info...", menus);
  }
  
  install_menu_hints();

  Gtk::Toolbar *playToolbar = toolbar_;
  Gtk::Image *pixmap;
  Gtk::RadioButton_Helpers::Group playGroup;

  playToolbar->tools().push_back(Gtk::Toolbar_Helpers::Space());

  pixmap = manage(new Gtk::Image(PIXMAPS_DIR "/stock_media-play.png"));
  playToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Play", *pixmap,
  		slot(*this, &AudioCDProject::playStart), "Play"));
  playStartButton_ = playToolbar->tools().back().get_widget();

  pixmap = manage(new Gtk::Image(PIXMAPS_DIR "/stock_media-pause.png"));
  playToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Pause", *pixmap,
  		slot(*this, &AudioCDProject::playPause), "Pause", ""));
  playPauseButton_ = playToolbar->tools().back().get_widget();

  pixmap = manage(new Gtk::Image(PIXMAPS_DIR "/stock_media-stop.png"));
  playToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Stop", *pixmap,
  	    slot(*this, &AudioCDProject::playStop), "Stop", ""));
  playStopButton_ = playToolbar->tools().back().get_widget();

  Gtk::Toolbar *zoomToolbar = toolbar_;
  Gtk::RadioButton_Helpers::Group toolGroup;

  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::Space());

  pixmap = manage(new Gtk::Image(PIXMAPS_DIR "/pixmap_cursor-tool.xpm"));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::RadioElem(toolGroup, "Select", *pixmap,
  		bind(slot(*this, &AudioCDProject::setMode), AudioCDView::SELECT), "Selection tool", ""));
  selectButton_ = zoomToolbar->tools().back().get_widget();

  pixmap = manage(new Gtk::Image(PIXMAPS_DIR "/pixmap_zoom-tool.xpm"));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::RadioElem(toolGroup, "Zoom", *pixmap,
  		bind(slot(*this, &AudioCDProject::setMode), AudioCDView::ZOOM), "Zoom tool", ""));
  zoomButton_ = zoomToolbar->tools().back().get_widget();

  pixmap = manage(new Gtk::Image(Gtk::StockID(Gtk::Stock::ZOOM_IN), Gtk::ICON_SIZE_LARGE_TOOLBAR));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Zoom +", *pixmap,
  		slot(*this, &AudioCDProject::zoomx2), "Zoom In", ""));
  zoomInButton_ = zoomToolbar->tools().back().get_widget();

  pixmap = manage(new Gtk::Image(Gtk::StockID(Gtk::Stock::ZOOM_OUT), Gtk::ICON_SIZE_LARGE_TOOLBAR));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Zoom -", *pixmap,
  		slot(*this, &AudioCDProject::zoomOut), "Zoom Out", ""));
  zoomOutButton_ = zoomToolbar->tools().back().get_widget();

  pixmap = manage(new Gtk::Image(Gtk::StockID(Gtk::Stock::ZOOM_FIT), Gtk::ICON_SIZE_LARGE_TOOLBAR));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Fit", *pixmap,
  		slot(*this, &AudioCDProject::fullView), "Full View", ""));
  zoomFullButton_ = zoomToolbar->tools().back().get_widget();

  pixmap = manage(new Gtk::Image(Gtk::StockID(Gtk::Stock::ZOOM_100), Gtk::ICON_SIZE_LARGE_TOOLBAR));
  zoomToolbar->tools().push_back(Gtk::Toolbar_Helpers::ButtonElem("Fit Sel", *pixmap,
  		slot(*this, &AudioCDProject::zoomIn), "Zoom to fit the selection", ""));
  zoomSelectionButton_ = zoomToolbar->tools().back().get_widget();


  audioCDChild_ = new AudioCDChild(this);

  audioCDView_ = audioCDChild_->newView();
  hbox->pack_start(*audioCDView_, TRUE, TRUE);
  setMode(AudioCDView::SELECT);
  tocEditView_ = audioCDView_->tocEditView();
  tocEditView_->sampleViewFull();

  show_all();
}

bool AudioCDProject::closeProject()
{
  if (audioCDChild_->closeProject())
  {
    delete audioCDChild_;
    audioCDChild_ = 0;

    if (tocInfoDialog_)
      delete tocInfoDialog_;

    if (cdTextDialog_)
      delete cdTextDialog_;

    if (recordTocDialog_)
      delete recordTocDialog_;

    return true;
  }
  return false;  // Do not close the project
}

void AudioCDProject::setMode(AudioCDView::Mode m)
{
  audioCDView_->setMode(m);
}

void AudioCDProject::recordToc2CD()
{
  if (recordTocDialog_ == 0)
    recordTocDialog_ = new RecordTocDialog(tocEdit_);

  recordTocDialog_->start(this);
}

void AudioCDProject::projectInfo()
{
  if (tocInfoDialog_ == 0)
    tocInfoDialog_ = new TocInfoDialog();

  tocInfoDialog_->setView(tocEdit_);
  tocInfoDialog_->present();
}

void AudioCDProject::cdTextDialog()
{
  if (cdTextDialog_ == 0)
    cdTextDialog_ = new CdTextDialog();

  cdTextDialog_->setTocEdit(tocEdit_);
  cdTextDialog_->present();
}

void AudioCDProject::update(unsigned long level)
{
//FIXME: Here we should update the menus and the icons
//       this is, enabled/disabled.

  level |= tocEdit_->updateLevel();

  if (level & (UPD_TOC_DIRTY | UPD_TOC_DATA))
    updateWindowTitle();

  if (audioCDChild_ != 0)
    audioCDChild_->update(level);

  if (tocInfoDialog_ != 0)
    tocInfoDialog_->update(level, tocEdit_);

  if (cdTextDialog_ != 0)
    cdTextDialog_->update(level, tocEdit_);
  if (recordTocDialog_ != 0)
    recordTocDialog_->update(level);
}

void AudioCDProject::playStart()
{
  unsigned long start, end;
  unsigned long level = 0;

  if (playStatus_ == PLAYING)
  {
    audioCDView_->getSelection(start, end);
    playPosition_ = start;
    tocReader.seekSample(start);
    return;
  }

  if (playStatus_ == PAUSED)
  {
    playStatus_ = PLAYING;
    Glib::signal_idle().connect(slot(*this, &AudioCDProject::playCallback));
    return;
  }
  else
  {
    audioCDView_->getSelection(start, end);
  }

  if (tocEdit_->lengthSample() == 0)
  {
    guiUpdate(UPD_PLAY_STATUS);
    return;
  }

  if (soundInterface_ == NULL) {
    soundInterface_ = new SoundIF;
    if (soundInterface_->init() != 0) {
      delete soundInterface_;
      soundInterface_ = NULL;
      guiUpdate(UPD_PLAY_STATUS);
	  statusMessage("WARNING: Cannot open \"/dev/dsp\"");
      return;
    }
  }

  if (soundInterface_->start() != 0)
  {
    guiUpdate(UPD_PLAY_STATUS);
    return;
  }

  tocReader.init(tocEdit_->toc());
  if (tocReader.openData() != 0) {
    tocReader.init(NULL);
    soundInterface_->end();
    guiUpdate(UPD_PLAY_STATUS);
    return;
    }

  if (tocReader.seekSample(start) != 0) {
    tocReader.init(NULL);
    soundInterface_->end();
    guiUpdate(UPD_PLAY_STATUS);
    return;
  }

  playLength_ = end - start + 1;
  playPosition_ = start;
  playStatus_ = PLAYING;
  playAbort_ = 0;

  level |= UPD_PLAY_STATUS;

//FIXME: Selection / Zooming does not depend
//       on the Child, but the View.
//       we should have different blocks!
  tocEdit_->blockEdit();

  guiUpdate(level);

  Glib::signal_idle().connect(slot(*this, &AudioCDProject::playCallback));
}

void AudioCDProject::playPause()
{
  if (playStatus_ == STOPPED)
    return;

  if (playStatus_ == PLAYING)
    playStatus_ = PAUSED;
  else
  {
    playStatus_ = PLAYING;
    Glib::signal_idle().connect(slot(*this, &AudioCDProject::playCallback));
  }
}

void AudioCDProject::playStop()
{
  unsigned long start, end;

  if (playStatus_ == STOPPED)
    return;

  if (playStatus_ == PAUSED)
  {
    soundInterface_->end();
    tocReader.init(NULL);
    playStatus_ = STOPPED;
    tocEdit_->unblockEdit();
    playStatus_ = STOPPED;
    guiUpdate(UPD_PLAY_STATUS);
  }
  else
  {
    playAbort_ = 1;
  }
}

bool AudioCDProject::playCallback()
{
  unsigned long level = 0;

  long len = playLength_ > playBurst_ ? playBurst_ : playLength_;

  if (playStatus_ == PAUSED)
  {
    level |= UPD_PLAY_STATUS;
    guiUpdate(level);
    return 0; // remove idle handler
  }

  if (tocReader.readSamples(playBuffer_, len) != len ||
      soundInterface_->play(playBuffer_, len) != 0) {
    soundInterface_->end();
    tocReader.init(NULL);
    playStatus_ = STOPPED;
    level |= UPD_PLAY_STATUS;
    tocEdit_->unblockEdit();
    guiUpdate(level);
    return 0; // remove idle handler
  }

  playLength_ -= len;
  playPosition_ += len;

  unsigned long delay = soundInterface_->getDelay();

  if (delay <= playPosition_)
//    level |= UPD_PLAY_STATUS;
    audioCDView_->updatePlayPos(playPosition() - getDelay());

  if (len == 0 || playAbort_ != 0) {
    soundInterface_->end();
    tocReader.init(NULL);
    playStatus_ = STOPPED;
    level |= UPD_PLAY_STATUS;
    tocEdit_->unblockEdit();
    audioCDView_->updatePlayPos(0);
    guiUpdate(level);
    return 0; // remove idle handler
  }
  else {
    guiUpdate(level);
    return 1; // keep idle handler
  }
}

unsigned long AudioCDProject::playPosition()
{
  return playPosition_;
}

unsigned long AudioCDProject::getDelay()
{
  return soundInterface_->getDelay();
}

void AudioCDProject::zoomx2()
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

void AudioCDProject::zoomOut()
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

  audioCDView_->tocEditView()->sampleView(start, end);
  guiUpdate();
}

void AudioCDProject::fullView()
{
  tocEditView_->sampleViewFull();

  guiUpdate();
}

void AudioCDProject::zoomIn()
{
  unsigned long start, end;

  if (tocEditView_->sampleSelection(&start, &end)) {
    tocEditView_->sampleView(start, end);
    guiUpdate();
  }
}
