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

#ifndef __MDI_WINDOW_H__
#define __MDI_WINDOW_H__

#include <gtk--.h>
#include <gtk/gtk.h>

#include <gnome--.h>

GtkWidget* example_creator(GnomeMDIChild *child, gpointer data);

class MDIWindow : public Gnome::MDI
{
public:
  MDIWindow();
  void example_child();
//  GtkWidget* example_creator(GnomeMDIChild *child, gpointer data);

protected:
  void install_menus_and_toolbar();
  void app_close();
  virtual int delete_event_impl(GdkEventAny *event);
  void nothing_cb();  

private:
//llanero  void quit();
//llanero  void configureDevices();

//llanero  void extract();
//llanero  void record();
//llanero perhaps record must be a per child menu? and extract??

public:
//  void update(unsigned long level);
//  gint delete_event_impl(GdkEventAny*);

};
#endif
