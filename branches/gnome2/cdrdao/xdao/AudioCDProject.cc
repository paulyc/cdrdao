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

#include <assert.h>
#include <glib-object.h>

#include "Toc.h"
#include "SoundIF.h"
#include "AudioCDProject.h"
#include "AudioCDChild.h"
#include "AudioCDView.h"
#include "TocEdit.h"
#include "TocEditView.h"
#include "TocInfoDialog.h"
#include "CdTextDialog.h"
#include "guiUpdate.h"
#include "util.h"
#include "gcdmaster.h"
#include "xcdrdao.h"
#include "RecordTocDialog.h"
#include "SampleManager.h"
#include "Icons.h"

void describeChildren(Gtk::Widget* w, int indent)
{
  for (int i = indent; i >= 0; i--)
    printf(" ");
  int width, height;
  w->get_size_request(width, height);
  printf("0x%08x is a %s (%d x %d)\n", (unsigned)w,
         g_type_name(G_OBJECT(w->gobj())->g_type_instance.g_class->g_type),
         width, height);

  if (GTK_IS_CONTAINER(w->gobj())) {
    Gtk::Container* c = dynamic_cast<Gtk::Container*>(w);

    Glib::ListHandle<Gtk::Widget*> children = c->get_children();
    for (Glib::ListHandle<Gtk::Widget*>::iterator i = children.begin();
         i != children.end(); i++) {
      describeChildren(*i, indent + 2);
    }
  }
}

AudioCDProject::AudioCDProject(int number, const char *name, TocEdit *tocEdit)
{
  frame_.add(hbox_);

  projectNumber_ = number;

  tocInfoDialog_ = NULL;
  cdTextDialog_ = NULL;
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
    std::vector<Info> menus, viewMenuTree;

    menus.push_back(Item(Icon(Gtk::StockID(Gtk::Stock::PROPERTIES)),
                         "CD-TEXT...",
                         slot(*this, &AudioCDProject::cdTextDialog),
                         "Edit CD-TEXT data"));
    insert_menus("Edit/Project Info...", menus);
  }
  
  install_menu_hints();
  createToolbar();

  audioCDChild_ = new AudioCDChild(this);

  audioCDView_ = audioCDChild_->view();
  hbox_.pack_start(*audioCDView_, TRUE, TRUE);
  audioCDView_->tocEditView()->sampleViewFull();
  audioCDView_->tocEditView()->tocEdit()->sampleManager()->
      setProgressBar(progressbar_);
  audioCDView_->tocEditView()->tocEdit()->sampleManager()->
      setAbortButton(progressButton_);

  guiUpdate(UPD_ALL);
  show_all();
}

bool AudioCDProject::closeProject()
{
  if (audioCDChild_ && audioCDChild_->closeProject()) {
    delete audioCDChild_;
    audioCDChild_ = NULL;

    if (tocInfoDialog_)   delete tocInfoDialog_;
    if (cdTextDialog_)    delete cdTextDialog_;
    if (recordTocDialog_) delete recordTocDialog_;

    return true;
  }
  return false;  // Do not close the project
}

void AudioCDProject::recordToc2CD()
{
  if (recordTocDialog_ == NULL)
    recordTocDialog_ = new RecordTocDialog(tocEdit_);

  recordTocDialog_->start(this);
}

void AudioCDProject::projectInfo()
{
  if (!tocInfoDialog_)
    tocInfoDialog_ = new TocInfoDialog(this);

  tocInfoDialog_->start(tocEdit_);
}

void AudioCDProject::cdTextDialog()
{
  if (cdTextDialog_ == 0)
    cdTextDialog_ = new CdTextDialog();

  cdTextDialog_->start(tocEdit_);
}

void AudioCDProject::update(unsigned long level)
{
  //FIXME: Here we should update the menus and the icons
  //       this is, enabled/disabled.

  level |= tocEdit_->updateLevel();

  if (level & (UPD_TOC_DIRTY | UPD_TOC_DATA))
    updateWindowTitle();

  audioCDChild_->update(level);

  if (tocInfoDialog_)
    tocInfoDialog_->update(level, tocEdit_);

  if (cdTextDialog_ != 0)
    cdTextDialog_->update(level, tocEdit_);
  if (recordTocDialog_ != 0)
    recordTocDialog_->update(level);

  if (level & UPD_PLAY_STATUS) {
    bool sensitivity[3][3] = {
      //PLAY  PAUSE STOP 
      { false, true, true }, // Playing
      { true, true, true },  // Paused
      { true, false, false } // Stopped
    };

    buttonPlay_->set_sensitive(sensitivity[playStatus_][0]);
    buttonPause_->set_sensitive(sensitivity[playStatus_][1]);
    buttonStop_->set_sensitive(sensitivity[playStatus_][2]);
  }
}

void AudioCDProject::playStart()
{
  unsigned long start, end;

  if (audioCDView_ && audioCDView_->tocEditView()) {
    if (!audioCDView_->tocEditView()->sampleSelection(&start, &end))
      audioCDView_->tocEditView()->sampleView(&start, &end);

    playStart(start, end);
  }
}

void AudioCDProject::playStart(unsigned long start, unsigned long end)
{
  unsigned long level = 0;

  if (playStatus_ == PLAYING)
    return;

  if (playStatus_ == PAUSED)
    {
      playStatus_ = PLAYING;
      Glib::signal_idle().connect(slot(*this, &AudioCDProject::playCallback));
      return;
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
  if (playStatus_ == PAUSED) {
    playStatus_ = PLAYING;
    Glib::signal_idle().connect(slot(*this, &AudioCDProject::playCallback));
  } else if (playStatus_ == PLAYING) {
    playStatus_ = PAUSED;
  }
}

void AudioCDProject::playStop()
{
  if (getPlayStatus() == PAUSED)
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
      return false; // remove idle handler
    }

  if (tocReader.readSamples(playBuffer_, len) != len ||
      soundInterface_->play(playBuffer_, len) != 0) {
    soundInterface_->end();
    tocReader.init(NULL);
    playStatus_ = STOPPED;
    level |= UPD_PLAY_STATUS;
    tocEdit_->unblockEdit();
    guiUpdate(level);
    return false; // remove idle handler
  }

  playLength_ -= len;
  playPosition_ += len;

  unsigned long delay = soundInterface_->getDelay();

  if (delay <= playPosition_)
    level |= UPD_PLAY_STATUS;

  if (len == 0 || playAbort_ != 0) {
    soundInterface_->end();
    tocReader.init(NULL);
    playStatus_ = STOPPED;
    level |= UPD_PLAY_STATUS;
    tocEdit_->unblockEdit();
    guiUpdate(level);
    return false; // remove idle handler
  }
  else {
    guiUpdate(level);
    return true; // keep idle handler
  }
}

enum AudioCDProject::PlayStatus AudioCDProject::getPlayStatus()
{
  return playStatus_;
}

unsigned long AudioCDProject::playPosition()
{
  return playPosition_;
}

unsigned long AudioCDProject::getDelay()
{
  return soundInterface_->getDelay();
}

void AudioCDProject::createToolbar()
{
  using namespace Gnome::UI::Items;

  std::vector<Info> toollist;
  toollist.push_back(Item(Icon(Gtk::Stock::NEW), "New",
                          slot(*gcdmaster, &GCDMaster::newChooserWindow),
                          "New Project"));
  toollist.push_back(Item(Icon(Gtk::Stock::OPEN), "Open",
                          bind(slot(*gcdmaster,&GCDMaster::openProject),
                               (ProjectChooser *)0),
                          "Open a project"));
  toollist.push_back(Item(Icon(Gtk::Stock::CLOSE), "Close",
                          bind(slot(*gcdmaster, &GCDMaster::closeProject),
                               this),
                          "Close current project"));

  toollist.push_back(Item(Icon(Gtk::Stock::SAVE), "Save",
                          slot(*this, &Project::saveProject),
                          "Save current project"));

  toollist.push_back(Item(Icon(Gtk::Stock::CDROM), "Record",
                          slot(*this, &Project::recordToc2CD),
                          "Record to CD"));

  toollist.push_back(Separator());
  toollist.push_back(Item(Icon(Icons::PLAY), "Play",
                          slot(*this, &AudioCDProject::on_play_clicked),
                          "Play"));
  toollist.push_back(Item(Icon(Icons::STOP), "Stop",
                          slot(*this, &AudioCDProject::on_stop_clicked),
                          "Stop"));
  toollist.push_back(Item(Icon(Icons::PAUSE), "Pause",
                          slot(*this, &AudioCDProject::on_pause_clicked),
                          "Pause"));

  toollist.push_back(Separator());

  std::vector<Info> radiotree;
  radiotree.push_back(Item(Icon(Gtk::Stock::JUMP_TO), "Select",
                           slot(*this, &AudioCDProject::on_select_clicked),
                           "Select Mode"));
  radiotree.push_back(Item(Icon(Gtk::Stock::ZOOM_FIT), "Zoom",
                           slot(*this, &AudioCDProject::on_zoom_clicked),
                           "Zoom Mode"));
  toollist.push_back(RadioTree(radiotree));
  toollist.push_back(Separator());

  toollist.push_back(Item(Icon(Gtk::Stock::ZOOM_IN), "In",
                          slot(*this, &AudioCDProject::on_zoom_in_clicked),
                          "Zoom In"));
  toollist.push_back(Item(Icon(Gtk::Stock::ZOOM_OUT), "Out",
                          slot(*this, &AudioCDProject::on_zoom_out_clicked),
                          "Zoom Out"));
  toollist.push_back(Item(Icon(Gtk::Stock::ZOOM_FIT), "Fit",
                          slot(*this, &AudioCDProject::on_zoom_fit_clicked),
                          "Zoom Fit"));

  Array<Info>& realtb = create_toolbar(toollist);

  buttonPlay_  = (Gtk::Button*)realtb[6].get_widget();
  buttonStop_  = (Gtk::Button*)realtb[7].get_widget();
  buttonPause_ = (Gtk::Button*)realtb[8].get_widget();
  assert(GTK_IS_BUTTON(buttonPlay_->gobj()));
  assert(GTK_IS_BUTTON(buttonStop_->gobj()));
  assert(GTK_IS_BUTTON(buttonPause_->gobj()));
  buttonPlay_->set_sensitive(true);
  buttonStop_->set_sensitive(false);
  buttonPause_->set_sensitive(false);

  // describeChildren((Gtk::Widget*)buttonPlay_->get_parent());
}


void AudioCDProject::createToolbar2()
{
  using namespace Gnome::UI::Items;
  using namespace Gtk::Toolbar_Helpers;

  Gtk::Toolbar* toolbar = manage(new Gtk::Toolbar);
  toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);
  toolbar->tools().
    push_back(StockElem(Gtk::Stock::NEW,
                        slot(*gcdmaster, &GCDMaster::newChooserWindow),
                        "New Project"));
  toolbar->tools().
    push_back(StockElem(Gtk::Stock::OPEN,
                        bind(slot(*gcdmaster,&GCDMaster::openProject),
                             (ProjectChooser *)0),
                        "Open a project"));
  toolbar->tools().
    push_back(StockElem(Gtk::Stock::CLOSE,
                        bind(slot(*gcdmaster, &GCDMaster::closeProject), this),
                        "Close current project"));

  toolbar->tools().
    push_back(StockElem(Gtk::Stock::SAVE,
                        slot(*this, &Project::saveProject),
                        "Save current project"));

  toolbar->tools().
    push_back(StockElem(Gtk::Stock::CDROM,
                        slot(*this, &Project::recordToc2CD),
                        "Record to CD"));
  toolbar->tools().
    push_back(StockElem(Gtk::Stock::ZOOM_IN,
                        slot(*this, &AudioCDProject::on_zoom_in_clicked),
                        "Zoom In"));
  toolbar->tools().
    push_back(StockElem(Gtk::Stock::ZOOM_OUT,
                        slot(*this, &AudioCDProject::on_zoom_out_clicked),
                        "Zoom Out"));
  toolbar->tools().
    push_back(StockElem(Gtk::Stock::ZOOM_FIT,
                        slot(*this, &AudioCDProject::on_zoom_fit_clicked),
                        "Zoom Fit"));

  toolbar->show_all();
  add_docked(*toolbar, "some name", BONOBO_DOCK_ITEM_BEH_NORMAL,
             BONOBO_DOCK_TOP, 0, 0, 0);
  toolbar->set_toolbar_style(Gtk::TOOLBAR_BOTH);
  toolbar->set_icon_size(Gtk::ICON_SIZE_SMALL_TOOLBAR);
}

void AudioCDProject::on_play_clicked()
{
  playStart();
}

void AudioCDProject::on_stop_clicked()
{
  playStop();
}

void AudioCDProject::on_pause_clicked()
{
  playPause();
}

void AudioCDProject::on_zoom_in_clicked()
{
  if (audioCDView_)
    audioCDView_->zoomx2();
}

void AudioCDProject::on_zoom_out_clicked()
{
  if (audioCDView_)
    audioCDView_->zoomOut();
}

void AudioCDProject::on_zoom_fit_clicked()
{
  if (audioCDView_)
    audioCDView_->fullView();
}

void AudioCDProject::on_zoom_clicked()
{
  if (audioCDView_)
    audioCDView_->setMode(AudioCDView::ZOOM);
}

void AudioCDProject::on_select_clicked()
{
  if (audioCDView_)
    audioCDView_->setMode(AudioCDView::SELECT);
}

bool AudioCDProject::appendTrack(const char* filename)
{
  int ret = tocEdit()->appendTrack(filename);

  switch(ret) {
  case 0:
    audioCDView_->fullView();
    statusMessage("Appended track with audio data from \"%s\".", filename);
    break;
  case 1:
    statusMessage("Cannot open audio file \"%s\".", filename);
    break;
  case 2:
    statusMessage("Audio file \"%s\" has wrong format.", filename);
    break;
  }
  guiUpdate();
}

bool AudioCDProject::appendFile(const char* filename)
{
  int ret = tocEdit()->appendFile(filename);

  switch (ret) {
  case 0:
    audioCDView_->fullView();
    statusMessage("Appended audio data from \"%s\".", filename);
    break;
  case 1:
    statusMessage("Cannot open audio file \"%s\".", filename);
    break;
  case 2:
    statusMessage("Audio file \"%s\" has wrong format.", filename);
    break;
  }
  guiUpdate();
}

bool AudioCDProject::insertFile(const char* filename)
{
  TocEditView* view = audioCDView_->tocEditView();
  if (!view) return false;

  unsigned long pos, len;
  view->sampleMarker(&pos);
  int ret = tocEdit()->insertFile(filename, pos, &len);

  switch(ret) {
  case 0:
    view->sampleSelection(pos, pos+len-1);
    audioCDView_->fullView();
    statusMessage("Inserted audio data from \"%s\".", filename);
    break;
  case 1:
    statusMessage("Cannot open audio file \"%s\".", filename);
    break;
  case 2:
    statusMessage("Audio file \"%s\" has wrong format.", filename);
    break;
  }
  guiUpdate();
}
