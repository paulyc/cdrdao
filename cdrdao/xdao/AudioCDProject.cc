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
#include <gtkmm.h>
#include <libgnome/gnome-i18n.h>

#include "Toc.h"
#include "SoundIF.h"
#include "AudioCDProject.h"
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
#include "MessageBox.h"

AudioCDProject::AudioCDProject(int number, const char *name, TocEdit *tocEdit,
    Gtk::Window *parent)
{
  parent_ = parent;
  pack_start(hbox_);

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


  audioCDView_ = new AudioCDView(this);
  hbox_.pack_start(*audioCDView_, TRUE, TRUE);
  audioCDView_->tocEditView()->sampleViewFull();

  guiUpdate(UPD_ALL);
  show_all();
}

void AudioCDProject::add_menus(Glib::RefPtr<Gtk::UIManager> m_refUIManager)
{
  m_refActionGroup = Gtk::ActionGroup::create("AudioCDProject");

  m_refActionGroup->add( Gtk::Action::create("Save", Gtk::Stock::SAVE),
                         sigc::mem_fun(*this, &Project::saveProject) );

  m_refActionGroup->add( Gtk::Action::create("SaveAs", Gtk::Stock::SAVE_AS),
                         sigc::mem_fun(*this, &Project::saveAsProject) );

  m_refActionGroup->add( Gtk::Action::create("ProjectInfo", Gtk::Stock::PROPERTIES,
                         _("Project Info..."),
                         _("Edit global project data")),
                         sigc::mem_fun(*this, &AudioCDProject::projectInfo) );

  m_refActionGroup->add( Gtk::Action::create("CDTEXT", Gtk::Stock::PROPERTIES,
                         _("CD-TEXT..."),
                         _("Edit CD-TEXT data")),
                         sigc::mem_fun(*this, &AudioCDProject::cdTextDialog) );

  m_refActionGroup->add( Gtk::Action::create("Record", Icons::RECORD,
                         _("_Record"),
                         _("Record")),
                         sigc::mem_fun(*this, &AudioCDProject::recordToc2CD) );

  m_refActionGroup->add( Gtk::Action::create("Play", Icons::PLAY,
                         _("Play"),
                         _("Play")),
                         sigc::mem_fun(*this, &AudioCDProject::on_play_clicked) );

  m_refActionGroup->add( Gtk::Action::create("Stop", Icons::STOP,
                         _("Stop"),
                         _("Stop")),
                         sigc::mem_fun(*this, &AudioCDProject::on_stop_clicked) );

  m_refActionGroup->add( Gtk::Action::create("Pause", Icons::PAUSE,
                         _("Pause"),
                         _("Pause")),
                         sigc::mem_fun(*this, &AudioCDProject::on_pause_clicked) );

  //Add Toggle Actions:
  Gtk::RadioAction::Group group_colors;
  m_refActionGroup->add( Gtk::RadioAction::create(group_colors, "Select",
                         Gtk::Stock::JUMP_TO,
                         _("Select"),
                         _("Select Mode")),
                         sigc::mem_fun(*this, &AudioCDProject::on_select_clicked));
  m_refActionGroup->add( Gtk::RadioAction::create(group_colors, "Zoom",
                         Gtk::Stock::ZOOM_FIT,
                         _("Zoom"),
                         _("Zoom Mode")),
                         sigc::mem_fun(*this, &AudioCDProject::on_zoom_clicked));

  m_refActionGroup->add( Gtk::Action::create("ZoomIn", Gtk::Stock::ZOOM_IN,
                         _("Zoom In"),
                         _("Zoom In")),
                         sigc::mem_fun(*this, &AudioCDProject::on_zoom_in_clicked) );

  m_refActionGroup->add( Gtk::Action::create("ZoomOut", Gtk::Stock::ZOOM_OUT,
                         _("Zoom Out"),
                         _("Zoom Out")),
                         sigc::mem_fun(*this, &AudioCDProject::on_zoom_out_clicked) );

  m_refActionGroup->add( Gtk::Action::create("ZoomFit", Gtk::Stock::ZOOM_FIT,
                         _("Zoom Fit"),
                         _("Zoom Fit")),
                         sigc::mem_fun(*this, &AudioCDProject::on_zoom_fit_clicked) );

  m_refUIManager->insert_action_group(m_refActionGroup);

  // Merge menuitems
  try
  {
    Glib::ustring ui_info =
        "<ui>"
        "  <menubar name='MenuBar'>"
        "    <menu action='FileMenu'>"
        "      <placeholder name='FileSaveHolder'>"
        "      <menuitem action='Save'/>"
        "      <menuitem action='SaveAs'/>"
        "      </placeholder>"
        "    </menu>"
        "    <menu action='EditMenu'>"
        "      <menuitem action='ProjectInfo'/>"
        "      <menuitem action='CDTEXT'/>"
        "    </menu>"
        "    <menu action='ActionsMenu'>"
        "      <placeholder name='ActionsRecordHolder'>"
        "        <menuitem action='Record'/>"
        "      </placeholder>"
        "      <menuitem action='Play'/>"
        "      <menuitem action='Stop'/>"
        "      <menuitem action='Pause'/>"
        "      <separator/>"
        "      <menuitem action='Select'/>"
        "      <menuitem action='Zoom'/>"
        "      <separator/>"
        "      <menuitem action='ZoomIn'/>"
        "      <menuitem action='ZoomOut'/>"
        "      <menuitem action='ZoomFit'/>"
        "    </menu>"
        "  </menubar>"
        "  <toolbar name='ToolBar'>"
        "    <toolitem action='Save'/>"
        "    <toolitem action='Record'/>"
        "    <separator/>"
        "    <toolitem action='Play'/>"
        "    <toolitem action='Stop'/>"
        "    <toolitem action='Pause'/>"
        "    <separator/>"
        "    <toolitem action='Select'/>"
        "    <toolitem action='Zoom'/>"
        "    <separator/>"
        "    <toolitem action='ZoomIn'/>"
        "    <toolitem action='ZoomOut'/>"
        "    <toolitem action='ZoomFit'/>"
        "    <separator/>"
        "  </toolbar>"
        "</ui>";

    m_refUIManager->add_ui_from_string(ui_info);
  }
  catch(const Glib::Error& ex)
  {
    std::cerr << "merging menus failed: " <<  ex.what();
  }

  Glib::RefPtr<Gtk::Action> action;
  action = m_refActionGroup->get_action ("Play");
  action->set_sensitive(true);
  action = m_refActionGroup->get_action ("Pause");
  action->set_sensitive(false);
  action = m_refActionGroup->get_action ("Stop");
  action->set_sensitive(false);

  audioCDView_->add_menus (m_refUIManager);
  audioCDView_->signal_tocModified.connect(sigc::mem_fun(*this, &AudioCDProject::update));
}

void AudioCDProject::configureAppBar (Gnome::UI::AppBar *s, Gtk::ProgressBar* p,
                                      Gtk::Button *b)
{
  statusbar_ = s;
  progressbar_ = p;
  progressButton_ = b;
  audioCDView_->tocEditView()->tocEdit()->sampleManager()->
      setProgressBar(p);
  audioCDView_->tocEditView()->tocEdit()->sampleManager()->
      setAbortButton(progressButton_);
};

bool AudioCDProject::closeProject()
{
  if (!tocEdit_->editable()) {
    tocBlockedMsg(_("Close Project"));
    return false;
  }

  if (tocEdit_->tocDirty()) {
    gchar *message;
    
    message = g_strdup_printf(_("Project %s not saved."),
                              tocEdit_->filename());

    Ask2Box msg(getParentWindow (), _("Close"), 0, 2, message, "",
                _("Continue?"), NULL);
    g_free(message);

    if (msg.run() != 1)
      return false;
  }

  if (audioCDView_) {
    delete audioCDView_;
    audioCDView_ = NULL;
  }

  if (tocInfoDialog_)   delete tocInfoDialog_;
  if (cdTextDialog_)    delete cdTextDialog_;
  if (recordTocDialog_) delete recordTocDialog_;

  return true;
}

void AudioCDProject::recordToc2CD()
{
  if (recordTocDialog_ == NULL)
    recordTocDialog_ = new RecordTocDialog(tocEdit_);

  recordTocDialog_->start (parent_);
}

void AudioCDProject::projectInfo()
{
  if (!tocInfoDialog_)
    tocInfoDialog_ = new TocInfoDialog(parent_);

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

  audioCDView_->update(level);

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

    Glib::RefPtr<Gtk::Action> action;
    action = m_refActionGroup->get_action ("Play");
    action->set_sensitive(sensitivity[playStatus_][0]);
    action = m_refActionGroup->get_action ("Pause");
    action->set_sensitive(sensitivity[playStatus_][1]);
    action = m_refActionGroup->get_action ("Stop");
    action->set_sensitive(sensitivity[playStatus_][2]);
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
      Glib::signal_idle().connect(sigc::mem_fun(*this,&AudioCDProject::playCallback));
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
      statusMessage(_("WARNING: Cannot open \"/dev/dsp\""));
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

  Glib::signal_idle().connect(sigc::mem_fun(*this,
                                            &AudioCDProject::playCallback));
}

void AudioCDProject::playPause()
{
  if (playStatus_ == PAUSED) {
    playStatus_ = PLAYING;
    Glib::signal_idle().connect(sigc::mem_fun(*this,
                                              &AudioCDProject::playCallback));
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

bool AudioCDProject::appendTracks(std::list<std::string>& files)
{
  int ret = tocEdit()->appendTracks(files);
  const char* singlefn = (files.size() > 1 ? 0 : (*(files.begin())).c_str());

  switch(ret) {
  case 0:
    audioCDView_->fullView();
    if (!singlefn)
      statusMessage(_("Appended %d tracks."), files.size());
    else {
      if (TrackData::audioFileType(singlefn) == TrackData::WAVE) {
        statusMessage(_("Appended track with WAV audio from \"%s\"."),
                      singlefn);
      } else {
        statusMessage(_("Appended track with raw audio samples from \"%s\"."),
                      singlefn);
      }
    }
    break;
  case 1:
    if (singlefn)
      statusMessage(_("Cannot open audio file \"%s\"."), singlefn);
    else
      statusMessage(_("Cannot open audio file."));
    break;
  case 2:
    if (singlefn)
      statusMessage(_("Audio file \"%s\" has wrong format."), singlefn);
    else
      statusMessage(_("Audio file has wrong format."));
    break;
  }

  guiUpdate();
  return (ret == 0);
}

bool AudioCDProject::appendFiles(std::list<std::string>& files)
{
  int ret = tocEdit()->appendFiles(files);
  const char* singlefn = (files.size() > 1 ? 0 : (*(files.begin())).c_str());

  switch (ret) {
  case 0:
    audioCDView_->fullView();
    if (singlefn)
      statusMessage(_("Appended audio data from \"%s\"."), singlefn);
    else
      statusMessage(_("Appended audio data from %d files."), files.size());
    break;
  case 1:
    if (singlefn)
      statusMessage(_("Cannot open audio file \"%s\"."), singlefn);
    else
      statusMessage(_("Cannot open audio file."));
    break;
  case 2:
    if (singlefn)
      statusMessage(_("Audio file \"%s\" has wrong format."), singlefn);
    else
      statusMessage(_("Audio file has wrong format."));
    break;
  }
  guiUpdate();
  return (ret == 0);
}

bool AudioCDProject::insertFiles(std::list<std::string>& files)
{
  const char* singlefn = (files.size() > 1 ? 0 : (*(files.begin())).c_str());
  TocEditView* view = audioCDView_->tocEditView();
  if (!view) return false;

  unsigned long pos, len;
  view->sampleMarker(&pos);
  int ret = tocEdit()->insertFiles(files, pos, &len);

  switch(ret) {
  case 0:
    view->sampleSelection(pos, pos+len-1);
    if (singlefn)
      statusMessage(_("Inserted audio data from \"%s\"."), singlefn);
    else
      statusMessage(_("Inserted audio data from %d files."), files.size());
    break;
  case 1:
    if (singlefn)
      statusMessage(_("Cannot open audio file \"%s\"."), singlefn);
    else
      statusMessage(_("Cannot open audio file."));
    break;
  case 2:
    if (singlefn)
      statusMessage(_("Audio file \"%s\" has wrong format."), singlefn);
    else
      statusMessage(_("Audio file has wrong format."));
    break;
  }
  guiUpdate();
  return (ret == 0);
}
