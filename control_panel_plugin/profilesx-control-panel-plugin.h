/*
 *  profilesx
 *  Copyright (C) 2010 Nicolai Hess
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef PROFILESX_CONTROL_PANEL_PLUGIN_H
#define PROFILESX_CONTROL_PANEL_PLUGIN_H
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gdk/gdkx.h>

typedef struct _user_data_t user_data_t;

struct _user_data_t
{
  GtkWidget* main_dialog;
  GtkWidget* profiles_selector;
  GtkListStore* profiles_list_store;
  GtkWidget* profile_delete_button;
  GstElement* sound_player;
};

void fill_profiles_list_store(user_data_t* data);

void
profilesx_dialog_size_changed(GdkScreen* screen,
			      gpointer user_data);

#endif
