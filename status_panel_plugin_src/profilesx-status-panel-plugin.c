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

#include <gtk/gtk.h>
#include <hildon/hildon.h>
#include "profilesx-status-panel-plugin.h"
#include <libprofile.h>
#include <keys_nokia.h>
#include <libintl.h>
#include <locale.h>
#include "profile_changed_marshal.h"
#include <gconf/gconf-client.h>

#define PROFILESX_STATUS_PLUGIN_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
									      TYPE_PROFILESX_STATUS_PLUGIN, ProfilesxStatusPluginPrivate))

#define PROFILESX_PLUGIN_SETTINGS_FILE "/.profilesx"

#define 	PROFILED_SERVICE   "com.nokia.profiled"
#define 	PROFILED_PATH   "/com/nokia/profiled"
#define 	PROFILED_INTERFACE   "com.nokia.profiled"
#define 	PROFILED_CHANGED   "profile_changed"

#define 	MCE_SERVICE   "com.nokia.mce"
#define 	MCE_PATH   "/com/nokia/mce/signal"
#define 	MCE_INTERFACE   "com.nokia.mce.signal"
#define 	MCE_STATE   "sig_call_state_ind"

#define         CALL_SERVICE "com.nokia.csd.Call"
#define         CALL_PATH "/com/nokia/csd/call/1"
#define         CALL_INTERFACE "com.nokia.csd.Call.Instance"
#define         CALL_ANSWER "Answer"

#define         PLAYBACK_SERVICE "org.maemo.Playback.Manager"
#define         PLAYBACK_PATH "/org/maemo/Playback/Manager"
#define         PLAYBACK_INTERFACE "org.maemo.Playback.Manager"
#define         PLAYBACK_REQUEST_PRIVACY_OVERRIDE "RequestPrivacyOverride"

#define GC_ROOT "/apps/maemo/profilesx"

struct _ProfilesxStatusPluginPrivate
{
  GtkWidget* button;
  DBusGConnection *dbus_conn;
  DBusGConnection *dbus_system_conn;
  DBusGProxy *dbus_proxy;
  DBusGProxy *dbus_system_proxy;
  guint gconf_notify_handler;
  gboolean autoanswer;
  gint auto_answer_delay;
  gboolean turn_speaker_on;
  gboolean disable_proximity_check;
};

HD_DEFINE_PLUGIN_MODULE(ProfilesxStatusPlugin, profilesx_status_plugin, HD_TYPE_STATUS_MENU_ITEM);


const gchar* get_profile_display_name(const gchar* profile_name)
{
  if(g_strcmp0(profile_name, "silent")==0)
  {
    return dgettext("osso-profiles", "profi_bd_silent");
  }
  else if(g_strcmp0(profile_name, "general")==0)
  {
    return dgettext("osso-profiles", "profi_bd_general");
  }
  else
  {
    return profile_name;
  }
}

void 
set_active_profile_from_radio_button(GtkRadioButton* button, gchar** profileList)
{
  GSList* list = gtk_radio_button_get_group(button);
  gboolean found = FALSE;
  gint index = g_strv_length(profileList);

  while(list && !found)
  {
    found = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(list->data));
    if(found)
    {
      profile_set_profile(profileList[index-1]);
    }
    list = list->next;
    index--;
  }
}

static void
profilesx_status_plugin_class_finalize(ProfilesxStatusPluginClass* klass)
{
}

static void
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

static void
show_profile_selection_dlg(GtkButton* button, ProfilesxStatusPlugin* plugin)
{
  gtk_widget_hide(gtk_widget_get_toplevel(GTK_WIDGET(button)));

  GtkWidget* pannable_area;
  GtkWidget* content_area;
  GtkWidget* dialog = gtk_dialog_new_with_buttons(dgettext("osso-profiles", "profi_ti_select_profiles"),
						  NULL,
						  GTK_DIALOG_MODAL,
						  dgettext("hildon-libs", "wdgt_bd_done"), GTK_RESPONSE_OK, NULL);
  gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 300);  

  content_area = gtk_hbox_new(FALSE, 3);
  pannable_area = hildon_pannable_area_new();
  gchar** profileList = profile_get_profiles();
  GtkWidget** vibs;
  GtkWidget* previousRadioButton = NULL;
  GtkWidget* left_box = gtk_vbox_new(FALSE, 0);
  GtkWidget* right_box = gtk_vbox_new(FALSE, 0);
  if(profileList)
  {
    gint profile_list_length = g_strv_length(profileList);
    gint idx = 0;
    vibs = g_new0(GtkWidget*, profile_list_length);

    gchar** profilePtr;
    gchar* activeProfile = profile_get_profile();

    for(profilePtr = profileList;
	*profilePtr;
	profilePtr++)
    {
      GtkWidget* button = hildon_gtk_radio_button_new_from_widget(HILDON_SIZE_FINGER_HEIGHT,
								  GTK_RADIO_BUTTON(previousRadioButton));
      gtk_button_set_label(GTK_BUTTON(button),
			   get_profile_display_name(*profilePtr));

      gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);

      if(g_strcmp0(*profilePtr, activeProfile)==0)
      {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);  
      }
      previousRadioButton = button;
      GtkWidget* vib_button = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
      hildon_check_button_set_active(HILDON_CHECK_BUTTON(vib_button), profile_get_value_as_bool(*profilePtr,
												PROFILEKEY_VIBRATING_ALERT_ENABLED));
      vibs[idx] = vib_button;
      idx++;
      gtk_button_set_label(GTK_BUTTON(vib_button),
			   dgettext("osso-profiles", "profi_fi_general_vibrate"));

      gtk_box_pack_start(GTK_BOX(left_box), button, FALSE, FALSE, 3);
      gtk_box_pack_start(GTK_BOX(right_box), vib_button, FALSE, FALSE, 3);
    }
  }

  gtk_box_pack_start(GTK_BOX(content_area), left_box, TRUE, TRUE, 3);
  gtk_box_pack_start(GTK_BOX(content_area), right_box, FALSE, FALSE, 3);

  hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(pannable_area), content_area); 
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), pannable_area);

  profilesx_dialog_size_changed(gdk_display_get_default_screen(gdk_display_get_default()),
				dialog);

  g_signal_connect(gdk_display_get_default_screen(gdk_display_get_default()), 
		   "size-changed", 
		   G_CALLBACK(profilesx_dialog_size_changed), dialog); 

  gtk_widget_show_all(dialog);

  int ret = gtk_dialog_run(GTK_DIALOG(dialog));

  if(ret == GTK_RESPONSE_OK)
  {
    set_active_profile_from_radio_button(GTK_RADIO_BUTTON(previousRadioButton), profileList);

    if(profileList)
    {
      gint profile_list_length = g_strv_length(profileList);
      for(;profile_list_length>0;--profile_list_length)
      {
	if(hildon_check_button_get_active(HILDON_CHECK_BUTTON(vibs[profile_list_length-1])))
	{
	  profile_set_value_as_bool(profileList[profile_list_length-1], PROFILEKEY_VIBRATING_ALERT_ENABLED, TRUE);
	}
	else
	{
	  profile_set_value_as_bool(profileList[profile_list_length-1], PROFILEKEY_VIBRATING_ALERT_ENABLED, FALSE);
	}
      }
    }
  }
  profile_free_profiles(profileList);
  g_free(vibs);
  gtk_widget_destroy(dialog);
}

static gchar* 
get_profile_icon_name(const gchar* profile_name)
{
  GKeyFile *keyFile;
  FILE *iniFile;
  gchar *filename;
  gchar* icon_name = NULL;
  keyFile = g_key_file_new();
  filename = g_strconcat (g_get_home_dir(), PROFILESX_PLUGIN_SETTINGS_FILE, NULL);
  g_key_file_load_from_file(keyFile, filename, G_KEY_FILE_KEEP_COMMENTS, NULL);
  if(g_key_file_load_from_file(keyFile, filename, G_KEY_FILE_NONE, NULL))
  {
    if(g_key_file_has_key(keyFile, profile_name, "icon", NULL))
    {
      icon_name = g_key_file_get_string(keyFile, profile_name, "icon", NULL);
    }
  }
  g_key_file_free (keyFile); 
  g_free (filename); 
  return icon_name;
}

static void
turn_speaker_on(ProfilesxStatusPlugin* plugin)
{
  if(plugin->priv->dbus_conn)
  {
    DBusGProxy* proxy = 
      dbus_g_proxy_new_for_name(plugin->priv->dbus_conn, 
				PLAYBACK_SERVICE,
				PLAYBACK_PATH, 
				PLAYBACK_INTERFACE); 
    dbus_g_proxy_call_no_reply(proxy,
			       PLAYBACK_REQUEST_PRIVACY_OVERRIDE,  
			       G_TYPE_BOOLEAN, TRUE, 
			       G_TYPE_INVALID, G_TYPE_INVALID);
    g_object_unref(proxy);
  }
}

static gboolean
is_proximity_covered()
{
  char state[7];
  FILE* fd = fopen("/sys/devices/platform/gpio-switch/proximity/state", "r");
  gboolean rc = TRUE;
  if(fd != NULL)
  {
    fread(&state, sizeof(char), 7, fd);
    state[6] = '\0';
    if(!g_str_has_prefix(state, "closed"))
    {
      rc = FALSE;
    }
  }
  fclose(fd);
  return rc;
}

static gboolean
answer_call(gpointer user_data)
{
  ProfilesxStatusPlugin* plugin = PROFILESX_STATUS_PLUGIN(user_data);
  if(plugin->priv->disable_proximity_check || !is_proximity_covered())
  {
    if(plugin->priv->dbus_system_conn)
    {
      DBusGProxy* proxy = 
	dbus_g_proxy_new_for_name(plugin->priv->dbus_system_conn, 
				  CALL_SERVICE,
				  CALL_PATH, 
				  CALL_INTERFACE); 
      dbus_g_proxy_call_no_reply(proxy,
				 CALL_ANSWER,  
				 G_TYPE_INVALID, G_TYPE_INVALID);
      g_object_unref(proxy);
    }

  }
  return FALSE;
}

static void
handle_incoming_call(DBusGProxy *object,
		     const char* state, 
		     const char* emstate, 
		     gpointer user_data)
{
  ProfilesxStatusPlugin* plugin = PROFILESX_STATUS_PLUGIN(user_data);
  if(g_strcmp0(state, "ringing") == 0 && plugin->priv->autoanswer)
  {
    g_timeout_add_seconds(plugin->priv->auto_answer_delay+1, answer_call, plugin);
  }
  if(g_strcmp0(state, "active") == 0 && plugin->priv->turn_speaker_on)
  {
    turn_speaker_on(plugin);
  }
}

static void
unregister_incoming_call_handler(ProfilesxStatusPlugin* plugin)
{
  if(plugin->priv->dbus_system_conn)
  {
    if(plugin->priv->dbus_system_proxy)
    {
      dbus_g_proxy_disconnect_signal(plugin->priv->dbus_system_proxy, 
				     MCE_STATE, 
				     G_CALLBACK(handle_incoming_call), 
				     plugin);
      g_object_unref(plugin->priv->dbus_system_proxy);
      plugin->priv->dbus_system_proxy = NULL;
    }
  }
}

static void
register_incoming_call_handler(ProfilesxStatusPlugin* plugin)
{
  if(plugin->priv->dbus_system_conn == NULL)
    plugin->priv->dbus_system_conn = 
      hd_status_plugin_item_get_dbus_g_connection(HD_STATUS_PLUGIN_ITEM(&plugin->parent),
						  DBUS_BUS_SYSTEM, 
						  NULL);
  if(plugin->priv->dbus_system_conn)
  {
    plugin->priv->dbus_system_proxy = dbus_g_proxy_new_for_name(plugin->priv->dbus_system_conn,
								MCE_SERVICE,
								MCE_PATH,
								MCE_INTERFACE);
    dbus_g_object_register_marshaller(_profile_changed_marshal_VOID__STRING_STRING,
				      G_TYPE_NONE,
				      G_TYPE_STRING,
				      G_TYPE_STRING,
				      G_TYPE_INVALID);   
    dbus_g_proxy_add_signal(plugin->priv->dbus_system_proxy,    
			    MCE_STATE,
			    G_TYPE_STRING,
			    G_TYPE_STRING,
			    G_TYPE_INVALID);    
    dbus_g_proxy_connect_signal(plugin->priv->dbus_system_proxy,
				MCE_STATE,
				G_CALLBACK(handle_incoming_call), plugin, NULL);
  }
}

static void
register_unregister_on_call(ProfilesxStatusPlugin* plugin)
{
  if(plugin->priv->autoanswer || plugin->priv->turn_speaker_on)
  {
    if(plugin->priv->dbus_system_proxy == NULL)
    {
      register_incoming_call_handler(plugin);
    }
  }
  else
  {
    if(plugin->priv->dbus_system_proxy != NULL)
    {
      unregister_incoming_call_handler(plugin);
    }
  }
}

static void
read_profile_gconf_options(ProfilesxStatusPlugin* plugin, const gchar* profile)
{
  GConfClient* client = gconf_client_get_default();
  g_assert(GCONF_IS_CLIENT(client));
  gchar* profile_name_key = gconf_escape_key(profile, -1);
  gchar* autoanswer_conf_key = g_strdup_printf("%s/%s/autoAnswer", GC_ROOT, profile_name_key);
  gchar* disable_proximity_check_conf_key = g_strdup_printf("%s/%s/disableProximityCheck", GC_ROOT, profile_name_key);
  gchar* turnspeakeron_conf_key = g_strdup_printf("%s/%s/turnSpeakerOn", GC_ROOT, profile_name_key);
  gchar* autoanswerdelay_conf_key = g_strdup_printf("%s/%s/autoanswerdelay", GC_ROOT, profile_name_key);

  plugin->priv->autoanswer = gconf_client_get_bool(client, autoanswer_conf_key, NULL);
  plugin->priv->disable_proximity_check = gconf_client_get_bool(client, disable_proximity_check_conf_key, NULL);
  plugin->priv->turn_speaker_on = gconf_client_get_bool(client, turnspeakeron_conf_key, NULL);
  plugin->priv->auto_answer_delay = gconf_client_get_int(client, autoanswerdelay_conf_key, NULL);

  register_unregister_on_call(plugin);
  g_free(turnspeakeron_conf_key);
  g_free(autoanswer_conf_key);
  g_free(disable_proximity_check_conf_key);
  g_free(profile_name_key);
  g_free(autoanswerdelay_conf_key);
  g_object_unref(client);
}

static void
set_profile_status(ProfilesxStatusPlugin* plugin, const gchar* profile)
{
  gchar* icon_name = get_profile_icon_name(profile);
  if(icon_name != NULL)
  {
    GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), icon_name, 18, GTK_ICON_LOOKUP_NO_SVG, NULL);
    if(pixbuf==NULL)
    {
      pixbuf = gdk_pixbuf_new_from_file(icon_name,NULL);
    }
    if(pixbuf==NULL)
    {
      gchar* full_file_icon_name = g_strdup_printf("/usr/share/icons/hicolor/18x18/hildon/%s.png", icon_name);
      pixbuf = gdk_pixbuf_new_from_file(full_file_icon_name,NULL);
      g_free(full_file_icon_name);
    }
    if(pixbuf==NULL)
    {
      hd_status_plugin_item_set_status_area_icon (HD_STATUS_PLUGIN_ITEM (plugin), NULL);
    }
    else
    {
      hd_status_plugin_item_set_status_area_icon (HD_STATUS_PLUGIN_ITEM (plugin), pixbuf);
      g_object_unref(pixbuf);
    }
    g_free(icon_name);
  }
  else
  {
    hd_status_plugin_item_set_status_area_icon (HD_STATUS_PLUGIN_ITEM (plugin), NULL);
  }
  hildon_button_set_value(HILDON_BUTTON(plugin->priv->button), 
			  get_profile_display_name(profile));
}

static void
handle_profile_changed(DBusGProxy *object,
		      gboolean changed, 
		      gboolean active, 
		      const char* profile, 
		      GPtrArray* values, 
		      ProfilesxStatusPlugin* plugin)
{
  gchar* new_profile = profile_get_profile();
  set_profile_status(plugin, new_profile);
  read_profile_gconf_options(plugin, new_profile);
  g_free(new_profile);
}

static void
profilesx_status_plugin_finalize(GObject* object)
{
  ProfilesxStatusPlugin* plugin = PROFILESX_STATUS_PLUGIN(object);
  unregister_incoming_call_handler(plugin);
  
  if(plugin->priv->dbus_proxy)
  {
    dbus_g_proxy_disconnect_signal(plugin->priv->dbus_proxy, 
				   PROFILED_CHANGED, 
				   G_CALLBACK(handle_profile_changed), 
				   plugin);
    g_object_unref(plugin->priv->dbus_proxy);
    plugin->priv->dbus_proxy = NULL;
  }

  if(plugin->priv->gconf_notify_handler!=0)
  {
    GConfClient* client = gconf_client_get_default();
    gconf_client_notify_remove(client,
			       plugin->priv->gconf_notify_handler);    
  }
}

static void
register_dbus_signal_on_profile_change(ProfilesxStatusPlugin* plugin)
{
  plugin->priv->dbus_conn = NULL;
  plugin->priv->dbus_proxy = NULL;
  plugin->priv->dbus_conn = hd_status_plugin_item_get_dbus_g_connection(HD_STATUS_PLUGIN_ITEM(&plugin->parent),
									DBUS_BUS_SESSION, 
									NULL);

  if(plugin->priv->dbus_conn)
  {
    plugin->priv->dbus_proxy = dbus_g_proxy_new_for_name(plugin->priv->dbus_conn,
							 PROFILED_SERVICE,
							 PROFILED_PATH,
							 PROFILED_INTERFACE);
    // this took me
    GType ARRAY_STRING_STRING_STRING_TYPE = 
      dbus_g_type_get_collection("GPtrArray",    
				 dbus_g_type_get_struct("GValueArray",    
							G_TYPE_STRING,    
							G_TYPE_STRING,    
							G_TYPE_STRING,   
							G_TYPE_INVALID));
    // hours to figure out
    dbus_g_object_register_marshaller(_profile_changed_marshal_VOID__BOOLEAN_BOOLEAN_STRING_BOXED,
				      G_TYPE_NONE,
				      G_TYPE_BOOLEAN,
				      G_TYPE_BOOLEAN,
				      G_TYPE_STRING,
				      ARRAY_STRING_STRING_STRING_TYPE, 
				      G_TYPE_INVALID);   
    // how to do
    dbus_g_proxy_add_signal(plugin->priv->dbus_proxy,    
   			    PROFILED_CHANGED,    
   			    G_TYPE_BOOLEAN,    
   			    G_TYPE_BOOLEAN,    
   			    G_TYPE_STRING,    
   			    ARRAY_STRING_STRING_STRING_TYPE,
   			    G_TYPE_INVALID );    
    // this!
    dbus_g_proxy_connect_signal(plugin->priv->dbus_proxy,
				PROFILED_CHANGED,
				G_CALLBACK(handle_profile_changed), plugin, NULL);
  }
}

static void
profilesx_status_plugin_class_init(ProfilesxStatusPluginClass* klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = (GObjectFinalizeFunc)profilesx_status_plugin_finalize;
  g_type_class_add_private(klass, sizeof (ProfilesxStatusPluginPrivate));
}

static void
profilesx_hide_profile_button(GtkWidget* widget, gpointer data)
{
  if(g_strcmp0(G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(G_OBJECT(widget))),
	       "ProfilesStatusMenuItem")==0)
  {
    gtk_widget_hide(widget);
  }
}

static void
profilesx_show_profile_button(GtkWidget* widget, gpointer data)
{
  if(g_strcmp0(G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(G_OBJECT(widget))),
	       "ProfilesStatusMenuItem")==0)
  {
    gtk_widget_show(widget);
  }
}

static GtkWidget*
profilesx_find_HDStatusMenuBox(GtkWidget* widget)
{
  
  const gchar* class_name = G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(G_OBJECT(widget)));
  GtkWidget* w = widget;
  while(w != NULL && g_strcmp0(class_name, "HDStatusMenuBox")!=0)
  {
    w = gtk_widget_get_parent(w);
    class_name = G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(G_OBJECT(w)));
  }
  return w;
}

static void
hide_original_profile_button(ProfilesxStatusPlugin* plugin, gpointer data)
{
   GtkWidget* container = profilesx_find_HDStatusMenuBox(plugin->priv->button);

   if(container != NULL)  
   {  
     gtk_container_foreach(GTK_CONTAINER(container),  
			   (GtkCallback)(profilesx_hide_profile_button),  
			   NULL);  
   }  
}

static void
show_original_profile_button(ProfilesxStatusPlugin* plugin, gpointer data)
{
  GtkWidget* container = profilesx_find_HDStatusMenuBox(plugin->priv->button);
  
  if(container != NULL)  
  {  
    gtk_container_foreach(GTK_CONTAINER(container),  
			  (GtkCallback)(profilesx_show_profile_button),  
			  NULL);  
  }  
}

static void
_on_gconf_notify(GConfClient *client,
		 guint cnxn_id,
		 GConfEntry *entry,
		 gpointer user_data)
{
  gchar* profile = profile_get_profile();
  if(profile)
  {
    read_profile_gconf_options(PROFILESX_STATUS_PLUGIN(user_data), profile);
  }
  g_free(profile);
}

static void
_init_gconf_client_notify(ProfilesxStatusPlugin* plugin)
{
  GConfClient* client = gconf_client_get_default();
  g_assert(GCONF_IS_CLIENT(client));
  gconf_client_add_dir(client,
		       GC_ROOT, GCONF_CLIENT_PRELOAD_NONE, NULL);

  plugin->priv->gconf_notify_handler = gconf_client_notify_add(client,
							       GC_ROOT,
							       _on_gconf_notify, plugin,
							       NULL, NULL);
  g_object_unref(client);
}

static void
profilesx_status_plugin_init(ProfilesxStatusPlugin* plugin)
{
  plugin->priv = PROFILESX_STATUS_PLUGIN_GET_PRIVATE(plugin);
  plugin->priv->autoanswer = FALSE;
  plugin->priv->disable_proximity_check = FALSE;
  plugin->priv->turn_speaker_on = FALSE;
  plugin->priv->auto_answer_delay = 1;
  plugin->priv->gconf_notify_handler = 0;
  plugin->priv->dbus_system_conn = NULL;
  plugin->priv->dbus_system_proxy = NULL;

  _init_gconf_client_notify(plugin);
  register_dbus_signal_on_profile_change(plugin);

  plugin->priv->button = hildon_button_new(HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT,
					   HILDON_BUTTON_ARRANGEMENT_VERTICAL);

  hildon_button_set_title(HILDON_BUTTON(plugin->priv->button), dgettext("osso-profiles", "profi_ti_menu_plugin"));
  g_signal_connect(plugin->priv->button, "clicked", G_CALLBACK(show_profile_selection_dlg), plugin);
  g_signal_connect(plugin, "unrealize", G_CALLBACK(show_original_profile_button), NULL); 
  GtkWidget *image;
  image = gtk_image_new_from_icon_name("general_profile", HILDON_ICON_SIZE_FINGER);
  hildon_button_set_image(HILDON_BUTTON (plugin->priv->button), image);
  hildon_button_set_style(HILDON_BUTTON (plugin->priv->button), HILDON_BUTTON_STYLE_PICKER);
  hildon_button_set_alignment(HILDON_BUTTON(plugin->priv->button), 
			      0, 0,
			      1, 1);
  gchar* profile = profile_get_profile();
  if(profile)
  {
    set_profile_status(plugin, profile);
    read_profile_gconf_options(plugin, profile);
    g_free(profile);
  }
  gtk_container_add(GTK_CONTAINER(plugin), plugin->priv->button);
  g_signal_connect(plugin, "realize", G_CALLBACK(hide_original_profile_button), NULL); 
  gtk_widget_show_all(plugin->priv->button);
  gtk_widget_show(GTK_WIDGET(plugin));
}
