#include "Icons.h"

Gtk::StockID Icons::PLAY("gcdmaster-play");
Gtk::StockID Icons::STOP("gcdmaster-stop");
Gtk::StockID Icons::PAUSE("gcdmaster-pause");
Gtk::StockID Icons::GCDMASTER("gcdmaster-gcdmaster");
Gtk::StockID Icons::OPEN("gcdmaster-open");
Gtk::StockID Icons::AUDIOCD("gcdmaster-audiocd");
Gtk::StockID Icons::COPYCD("gcdmaster-copycd");
Gtk::StockID Icons::DUMPCD("gcdmaster-dumpcd");

struct Icons::IconEntry Icons::iconList[] = {
  { Icons::PLAY,      GNOMEICONDIR"/gcdmaster/play.png" },
  { Icons::STOP,      GNOMEICONDIR"/gcdmaster/stop.png" },
  { Icons::PAUSE,     GNOMEICONDIR"/gcdmaster/pause.png" },
  { Icons::GCDMASTER, GNOMEICONDIR"/gcdmaster/gcdmaster.png" },
  { Icons::OPEN,      GNOMEICONDIR"/gcdmaster/open.png" },
  { Icons::AUDIOCD,   GNOMEICONDIR"/gcdmaster/audiocd.png" },
  { Icons::COPYCD,    GNOMEICONDIR"/gcdmaster/copycd.png" },
  { Icons::DUMPCD,    GNOMEICONDIR"/gcdmaster/dumpcd.png" },
};

void Icons::registerStockIcons()
{
  Glib::RefPtr<Gtk::IconFactory> factory = Gtk::IconFactory::create();
  factory->add_default();

  for (int i = 0; i < G_N_ELEMENTS(iconList); i++) {
    Gtk::IconSource* source = new Gtk::IconSource;
    source->set_filename(iconList[i].filename);
    Gtk::IconSet* set = new Gtk::IconSet;
    set->add_source(*source);
    factory->add(iconList[i].name, *set);
  }
}
