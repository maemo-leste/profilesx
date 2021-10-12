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

#include "profilesx-profile-waitdialog.h"
#include "profilesx-control-panel-plugin.h"
#include <gtk/gtk.h>
#include <libprofile.h>

#define MAX_CHECKS 20

struct _pdata
{
  GtkWidget* dialog;
  gchar* profile_name;
  user_data_t* data;
  gboolean for_new;
  guint timeoutcounter;
};

static  gboolean
look_for_new_profile(gpointer data)
{
  pdata* pata = (pdata*)data;
  if(pata->for_new == profile_has_profile(pata->profile_name) ||
      pata->timeoutcounter > MAX_CHECKS)
  {
    gdk_threads_enter();
    hildon_gtk_window_set_progress_indicator(pata->dialog, 0);
    gtk_widget_set_sensitive(pata->dialog, TRUE);
    fill_profiles_list_store(pata->data);
    gdk_threads_leave();
    g_free(pata->profile_name);
    g_free(pata);
    ++pata->timeoutcounter;
    return FALSE;
  }
  return TRUE;
}

void show_progress_bar(const gchar* profile_name, 
		       user_data_t* data, 
		       gboolean for_new,
		       GtkWidget* dialog)
{
  pdata* pata = g_new0(pdata, 1);
  pata->dialog = dialog;
  pata->for_new = for_new;
  pata->data = data;
  pata->profile_name = g_strdup(profile_name);
  pata->timeoutcounter = 0;
  g_timeout_add(200, look_for_new_profile, pata);
  hildon_gtk_window_set_progress_indicator(dialog,
					   1);
  
  gtk_widget_set_sensitive(dialog, FALSE);
}
