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
#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <hildon/hildon.h>
#include <hildon/hildon-file-chooser-dialog.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <libprofile.h>
#include <keys_nokia.h>
#include <libintl.h>
#include <locale.h>
#include <pulse/pulseaudio.h>
#include "profilesx-control-panel-plugin.h"
#include "profilesx-profile-waitdialog.h"
#include "profilesx-profile-editor-dialog.h"

	
#define PROFILED_INTERFACE "com.nokia.profiled"
#define PROFILED_PATH "/com/nokia/profiled"
#define PROFILED_SERVICE "com.nokia.profiled"

#define RM_PROFILE_CMD "sudo /usr/bin/profilesx-util -r \"%s\""
#define ADD_PROFILE_CMD "sudo /usr/bin/profilesx-util -a \"%s\""
#define CUSTOM_PROFILE_INI "/etc/profiled/80.profilesx.ini"


gboolean is_predefined_profile(const gchar* profile_name)
{
  return 
    g_strcmp0(profile_name, "silent")==0 ||
    g_strcmp0(profile_name, "general")==0;
}

gchar* get_selected_profile_name(user_data_t* data)
{
  GtkTreeIter iter;
  gchar* list_value = NULL;
  if(hildon_touch_selector_get_selected(HILDON_TOUCH_SELECTOR(data->profiles_selector),
					0,
					&iter))
  {
    gtk_tree_model_get(GTK_TREE_MODEL(data->profiles_list_store),
		       &iter,
		       1, &list_value,
		       -1);
  }
  return list_value;
}

void
edit_profile(GtkWidget* button, user_data_t *data)
{
  gchar* profile_name = get_selected_profile_name(data);
  if(profile_name != NULL)
  {
    show_editor_dialog(profile_name, data->main_dialog, data->sound_player);
    g_free(profile_name);
  }
}

gboolean 
is_profile_name_valid(const gchar* profile_name)
{
  gboolean valid = FALSE;
  gchar* name = g_strdup(profile_name);
  g_strstrip(name);
  if(strlen(name) > 0)
    valid = !profile_has_profile(name);
  g_free(name);
  return valid;
}

void
fill_profiles_list_store(user_data_t* data)
{
  hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(data->profiles_selector),0,-1);

  GtkTreeIter iter;
  gchar** profileList = profile_get_profiles();
  gtk_list_store_clear(data->profiles_list_store);
  if(profileList)
  {
    gchar** profilePtr;
    for(profilePtr = profileList;
	*profilePtr;
	profilePtr++)
    {
      gtk_list_store_append(data->profiles_list_store, &iter);
      gtk_list_store_set(data->profiles_list_store, &iter, 
			 0, get_profile_display_name(*profilePtr),
			 1, *profilePtr, 
			 -1);
    }
  }
  profile_free_profiles(profileList);
  hildon_touch_selector_set_active(HILDON_TOUCH_SELECTOR(data->profiles_selector),0,1);
}

gchar*
ask_user_for_profile_name(user_data_t* data)
{
  GtkWidget* entry = hildon_entry_new(HILDON_SIZE_AUTO);

  GtkWidget* dialog = gtk_dialog_new_with_buttons(dgettext("osso-profiles", "profi_ti_menu_plugin"),
						  NULL,
						  GTK_DIALOG_MODAL,
						  dgettext("hildon-libs", "wdgt_bd_done"),
						  GTK_RESPONSE_OK,
						  NULL);

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), entry);
  gtk_widget_show_all(dialog);

  int ret = 0;
  gboolean name_is_valid = FALSE;

  do{
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    if(ret == GTK_RESPONSE_OK)
    {
      name_is_valid = is_profile_name_valid(hildon_entry_get_text(HILDON_ENTRY(entry)));
    }
  }while(ret == GTK_RESPONSE_OK && !name_is_valid);
  gchar* new_profile = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
  gtk_widget_destroy(dialog);
  if(ret == GTK_RESPONSE_OK)
  {
    g_strstrip(new_profile);
    return new_profile;
  }
  else
  {
    return NULL;
  }
}

void
new_profile(GtkButton* button, user_data_t* data)
{
  gchar* new_profile_name = ask_user_for_profile_name(data);
  if(new_profile_name!=NULL)
  {
    gchar* cmd = g_strdup_printf(ADD_PROFILE_CMD, new_profile_name);
    gboolean ret = g_spawn_command_line_sync(cmd,
					     NULL,
					     NULL,
					     NULL,
					     NULL);
    g_free(cmd);
    if(ret)
    {
      show_progress_bar(new_profile_name, 
			data, TRUE, gtk_widget_get_toplevel(GTK_WIDGET(button)));
    }
    g_free(new_profile_name);
  }
}

void
activate_general_profile_on_delete_active(const gchar* deleted_profile)
{
  gchar* active_profile = profile_get_profile();
  if(g_strcmp0(active_profile, deleted_profile)==0)
  {
    profile_set_profile("general");
  }
  g_free(active_profile);
}

void
delete_profile(GtkButton* button, user_data_t* data)
{
  gchar* list_value = get_selected_profile_name(data);
  if(list_value != NULL)
  {
    gchar* cmd = g_strdup_printf(RM_PROFILE_CMD, list_value);
    gboolean ret = g_spawn_command_line_sync(cmd,
					     NULL,
					     NULL,
					     NULL,
					     NULL);
    g_free(cmd);
    if(ret)
    {
      activate_general_profile_on_delete_active(list_value);
      show_progress_bar(list_value, data, FALSE, 
			gtk_widget_get_toplevel(GTK_WIDGET(button)));
    }
    g_free(list_value);
  }
}

void
add_main_dialog_buttons(user_data_t* data)
{
  GtkWidget* content_area;
  GtkWidget* edit_button;
  GtkWidget* new_button;
  data->profile_delete_button = hildon_gtk_button_new(HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT);
  edit_button = hildon_gtk_button_new(HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT);
  new_button = hildon_gtk_button_new(HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT);

  gtk_button_set_label(GTK_BUTTON(data->profile_delete_button), dgettext("hildon-libs", "wdgt_bd_delete"));
  gtk_button_set_label(GTK_BUTTON(edit_button), dgettext("hildon-libs", "wdgt_bd_edit"));
  gtk_button_set_label(GTK_BUTTON(new_button), dgettext("hildon-libs", "wdgt_bd_new"));

  g_signal_connect(edit_button, "clicked", G_CALLBACK(edit_profile), data);
  g_signal_connect(new_button, "clicked", G_CALLBACK(new_profile), data);
  g_signal_connect(data->profile_delete_button, "clicked", G_CALLBACK(delete_profile), data);

  content_area = gtk_dialog_get_action_area(GTK_DIALOG(data->main_dialog));
  gtk_box_pack_start(GTK_BOX(content_area),
		     data->profile_delete_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(content_area),
		     edit_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(content_area),
		     new_button, FALSE, FALSE, 0);
		     
  (void) gtk_dialog_add_button(GTK_DIALOG(data->main_dialog), dgettext("hildon-libs", "wdgt_bd_done"), GTK_RESPONSE_OK);
}

void 
show_hide_delete_button(HildonTouchSelector* selector, gint column, user_data_t* data)
{
  gchar* profile_name = get_selected_profile_name(data);
  if(profile_name!= NULL && is_predefined_profile(profile_name))
  {
    gtk_widget_hide(data->profile_delete_button);
  }
  else
  {
    gtk_widget_show(data->profile_delete_button);
  }
  g_free(profile_name);
}

void
add_main_dialog_profile_list(user_data_t* data)
{
  GtkCellRenderer* renderer;
  HildonTouchSelectorColumn* column;
  data->profiles_list_store = gtk_list_store_new(2,
						 G_TYPE_STRING,
						 G_TYPE_STRING);

  data->profiles_selector = hildon_touch_selector_new_text();
  hildon_touch_selector_set_model(HILDON_TOUCH_SELECTOR(data->profiles_selector),
				  0,
				  GTK_TREE_MODEL(data->profiles_list_store));  
  fill_profiles_list_store(data);
  g_signal_connect(data->profiles_selector, "changed", G_CALLBACK(show_hide_delete_button), data);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(data->main_dialog)->vbox), data->profiles_selector);
}

void create_main_dialog(user_data_t* app_data, GtkWindow* window)
{
  app_data->main_dialog = gtk_dialog_new_with_buttons(dgettext("osso-profiles", "profi_ti_cpa_profiles"),
						      GTK_WINDOW(window),
						      GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
						      NULL);
  add_main_dialog_buttons(app_data);
  add_main_dialog_profile_list(app_data);
}

void init_sound_player(user_data_t* data)
{
  gst_init(NULL, NULL);
  GstElement* pulsesink = gst_element_factory_make("pulsesink", NULL);
  pa_proplist* proplist = pa_proplist_new();
  pa_proplist_sets(proplist,
		  PA_PROP_EVENT_ID,
		  "ringtone-preview");
  pa_proplist_sets(proplist,
		  "module-stream-restore.id",
		  "x-maemo-applet-profiles");
  g_object_set(G_OBJECT(pulsesink), "proplist", proplist, NULL);
  //  pa_proplist_free(proplist);
  data->sound_player = gst_element_factory_make("playbin2", "Media Player");
  g_object_set(data->sound_player, "audio-sink", pulsesink, NULL);
}

void
profilesx_dialog_size_changed(GdkScreen* screen,
			      gpointer user_data)
{
  GdkGeometry geometry;
  if(gdk_screen_get_width(gdk_display_get_default_screen(gdk_display_get_default())) < 800)
  {
    geometry.min_height = 680;
    geometry.min_width = 480; 
  }
  else
  {
    geometry.min_height = 360;
    geometry.min_width = 800; 
  }
  gtk_window_set_geometry_hints(GTK_WINDOW(user_data),
				GTK_WIDGET(user_data),
				&geometry,
				GDK_HINT_MIN_SIZE);
}

osso_return_t execute(osso_context_t *osso,
		      gpointer data,
		      gboolean user_activated)
{
  gint response;
  setlocale (LC_ALL, "");

  user_data_t* app_data = g_new0(user_data_t, 1);
  init_sound_player(app_data);
  create_main_dialog(app_data, GTK_WINDOW(data));
  profilesx_dialog_size_changed(gdk_display_get_default_screen(gdk_display_get_default()),
				app_data->main_dialog);
  g_signal_connect(gdk_display_get_default_screen(gdk_display_get_default()), 
		   "size-changed", 
		   G_CALLBACK(profilesx_dialog_size_changed), app_data->main_dialog); 

  gtk_widget_show_all(app_data->main_dialog);
  show_hide_delete_button(HILDON_TOUCH_SELECTOR(app_data->profiles_selector), 
			  0, 
			  app_data);
  response = gtk_dialog_run(GTK_DIALOG(app_data->main_dialog));
  if(response == GTK_RESPONSE_OK)
  {
    //ignored
  }
  gtk_widget_destroy(GTK_WIDGET(app_data->main_dialog));
  gst_object_unref(app_data->sound_player);
  gst_object_unref(app_data->profiles_list_store);
  g_free(app_data);
  return OSSO_OK;
}

osso_return_t save_state(osso_context_t *osso, gpointer data)
{
  return OSSO_OK;
}

  
