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
#ifndef PROFILESX_PROFILE_EDITOR_DIALOG_H
#define PROFILESX_PROFILE_EDITOR_DIALOG_H

#include <gtk/gtk.h>
#include <gst/gst.h>
#include "profilesx-profile-data.h"

typedef struct _editor_data_t editor_data_t;
const gchar* get_profile_display_name(const gchar* profile_name);
void show_editor_dialog(const gchar* profile_name, GtkWidget* main_dialog, GstElement* sound_player);

#endif
