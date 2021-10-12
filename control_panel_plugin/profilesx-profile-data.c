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

#include "profilesx-profile-data.h"
#include <libprofile.h>
#include <keys_nokia.h>
#include <gconf/gconf-client.h>
#include <gio/gio.h>

#define PROFILESX_SETTINGS_FILE "/.profilesx"
#define GC_ROOT "/apps/maemo/profilesx"

gchar*
load_icon_name(const gchar* profile_name)
{
  GKeyFile* key_file;
  gchar* file_name;
  gchar* icon_name = NULL;

  key_file = g_key_file_new();
  file_name = g_strconcat(g_get_home_dir(), PROFILESX_SETTINGS_FILE, NULL);
  if(g_key_file_load_from_file(key_file, file_name, G_KEY_FILE_NONE, NULL))
  {
    if(g_key_file_has_key(key_file, profile_name, "icon", NULL))
    {
      icon_name = g_key_file_get_string(key_file, profile_name, "icon", NULL);
    }
  }
  g_free(file_name);
  g_key_file_free(key_file);
  return icon_name;
}

void
store_icon_name(const gchar* profile_name, const gchar* icon_name)
{
  GKeyFile* key_file;
  gchar* file_name;
  key_file = g_key_file_new();
  file_name = g_strconcat(g_get_home_dir(), PROFILESX_SETTINGS_FILE, NULL);
  g_key_file_load_from_file(key_file, file_name, G_KEY_FILE_NONE, NULL);
  if(icon_name!=NULL)
  {
    gchar* path = g_path_get_basename(icon_name);
    if(g_strcmp0(path, icon_name) == 0)
    {
      g_key_file_set_string(key_file, profile_name, "icon", icon_name);
    }
    else
    {
      gchar* escaped_profile_name = gconf_escape_key(profile_name, -1);
      gchar* old_icon_name = g_strdup_printf("%s.png", escaped_profile_name);
      gchar* new_icon_path = g_build_filename("/home/user/.profiled/", old_icon_name, NULL);
      if(g_file_test(new_icon_path, G_FILE_TEST_EXISTS))
      {
	g_unlink(new_icon_path);
      }
      GFile* source = g_file_new_for_path(icon_name);
      GFile* dest = g_file_new_for_path(new_icon_path);
      
      g_file_copy(source, dest,
		  G_FILE_COPY_NONE, NULL, NULL, NULL, NULL);
      
      g_object_unref(source);
      g_object_unref(dest);
      g_key_file_set_string(key_file, profile_name, "icon", new_icon_path);
      g_free(new_icon_path);
      g_free(old_icon_name);
      g_free(escaped_profile_name);
    }
  }
  else
  {
    g_key_file_remove_key(key_file, profile_name, "icon", NULL);
    gchar* escaped_profile_name = gconf_escape_key(profile_name, -1);
    gchar* old_icon_name = g_strdup_printf("%s.png", escaped_profile_name);
    gchar* new_icon_path = g_build_filename("/home/user/.profiled/", old_icon_name, NULL);
    if(g_file_test(new_icon_path, G_FILE_TEST_EXISTS))
      g_unlink(new_icon_path);

    g_free(new_icon_path);
    g_free(old_icon_name);
    g_free(escaped_profile_name);
  }
  gsize size;
  gchar* file_data = g_key_file_to_data(key_file, &size, NULL);
  g_file_set_contents(file_name, file_data, size, NULL);
  g_free(file_data);
  g_free(file_name);
  g_key_file_free(key_file);
}

static
void 
store_gconf_settings(const gchar* profile_name,
		    profilesx_profile_data* profile_data)
{
  GConfClient* client = gconf_client_get_default();
  g_assert(GCONF_IS_CLIENT(client));
  gchar* profile_name_key = gconf_escape_key(profile_name, -1);
  if(!profile_data->autoanswer &&
     !profile_data->turn_speaker_on &&
     !profile_data->disable_proximity_check &&
     profile_data->auto_answer_delay == 1)
  {
    gchar* profile_conf_key = g_strdup_printf("%s/%s", GC_ROOT, profile_name_key);
    gconf_client_recursive_unset(client, profile_conf_key, GCONF_UNSET_INCLUDING_SCHEMA_NAMES, NULL);
    g_free(profile_conf_key);
  }
  else
  {
    gchar* autoanswer_conf_key = g_strdup_printf("%s/%s/autoAnswer", GC_ROOT, profile_name_key);
    gchar* disable_proximity_check_conf_key = g_strdup_printf("%s/%s/disableProximityCheck", GC_ROOT, profile_name_key);
    gchar* turnspeakeron_conf_key = g_strdup_printf("%s/%s/turnSpeakerOn", GC_ROOT, profile_name_key);
    gchar* auto_answer_delay_conf_key = g_strdup_printf("%s/%s/autoanswerdelay", GC_ROOT, profile_name_key);
    gconf_client_set_bool(client, autoanswer_conf_key, profile_data->autoanswer, NULL);
    gconf_client_set_bool(client, disable_proximity_check_conf_key, profile_data->disable_proximity_check, NULL);
    gconf_client_set_bool(client, turnspeakeron_conf_key, profile_data->turn_speaker_on, NULL);
    gconf_client_set_int(client, auto_answer_delay_conf_key, profile_data->auto_answer_delay, NULL);
    g_free(turnspeakeron_conf_key);
    g_free(autoanswer_conf_key);
    g_free(disable_proximity_check_conf_key);
    g_free(auto_answer_delay_conf_key);
  }
  g_free(profile_name_key);
  g_object_unref(client);
}

void
profilesx_store_profile_data(const gchar* profile_name,
			    profilesx_profile_data* profile_data)
{
  profile_set_value_as_bool(profile_name,
			    PROFILEKEY_VIBRATING_ALERT_ENABLED,
			    profile_data->vibrating_enabled);
  // ringtone
  profile_set_value(profile_name,
		    PROFILEKEY_RINGING_ALERT_TONE,
		    profile_data->ringing_tone);
  profile_set_value_as_int(profile_name,
			   PROFILEKEY_RINGING_ALERT_VOLUME,
			   profile_data->ringing_volume);
  
  // sms
  profile_set_value(profile_name,
		    PROFILEKEY_SMS_ALERT_TONE,
		    profile_data->sms_tone);
  profile_set_value_as_int(profile_name,
			   PROFILEKEY_SMS_ALERT_VOLUME,
			   profile_data->sms_volume);
  
  // IM
  profile_set_value(profile_name,
		    PROFILEKEY_IM_ALERT_TONE,
		    profile_data->im_tone);
  profile_set_value_as_int(profile_name,
			   PROFILEKEY_IM_ALERT_VOLUME,
			   profile_data->im_volume);
  
  
  // email
  profile_set_value(profile_name,
		    PROFILEKEY_EMAIL_ALERT_TONE,
		    profile_data->email_tone);
  profile_set_value_as_int(profile_name,
			   PROFILEKEY_EMAIL_ALERT_VOLUME,
			   profile_data->email_volume);
  
  // sound_level
  profile_set_value_as_int(profile_name,
			   PROFILEKEY_KEYPAD_SOUND_LEVEL,
			   profile_data->keypad_sound_level);
  profile_set_value_as_int(profile_name,
			   PROFILEKEY_TOUCHSCREEN_SOUND_LEVEL,
			   profile_data->touchscreen_sound_level);
  profile_set_value_as_int(profile_name,
			   PROFILEKEY_SYSTEM_SOUND_LEVEL,
			   profile_data->system_sound_level);

  profile_set_value_as_bool(profile_name,
			   PROFILEKEY_TOUCHSCREEN_VIBRATION,
			   profile_data->touch_vibrating_enabled);

  store_icon_name(profile_name,
		  profile_data->status_bar_icon_name);
  store_gconf_settings(profile_name,
		       profile_data);
}

static
void 
load_gconf_settings(const gchar* profile_name,
		    profilesx_profile_data* profile_data)
{
  GConfClient* client = gconf_client_get_default();
  
  g_assert(GCONF_IS_CLIENT(client));

  gchar* profile_name_key = gconf_escape_key(profile_name, -1);
  gchar* autoanswer_conf_key = g_strdup_printf("%s/%s/autoAnswer", GC_ROOT, profile_name_key);
  gchar* disable_proximity_check_conf_key = g_strdup_printf("%s/%s/disableProximityCheck", GC_ROOT, profile_name_key);
  gchar* turnspeakeron_conf_key = g_strdup_printf("%s/%s/turnSpeakerOn", GC_ROOT, profile_name_key);
  gchar* auto_answer_delay_conf_key = g_strdup_printf("%s/%s/autoanswerdelay", GC_ROOT, profile_name_key);
  profile_data->autoanswer = gconf_client_get_bool(client, autoanswer_conf_key, NULL);
  profile_data->disable_proximity_check = gconf_client_get_bool(client, disable_proximity_check_conf_key, NULL);
  profile_data->turn_speaker_on = gconf_client_get_bool(client, turnspeakeron_conf_key, NULL);
  profile_data->auto_answer_delay = gconf_client_get_int(client, auto_answer_delay_conf_key, NULL);
  if(profile_data->auto_answer_delay == 0)
    profile_data->auto_answer_delay = 1;
  g_free(turnspeakeron_conf_key);
  g_free(autoanswer_conf_key);
  g_free(disable_proximity_check_conf_key);
  g_free(profile_name_key);
  g_object_unref(client);
}
			      
void
profilesx_load_profile_data(const gchar* profile_name,
			    profilesx_profile_data* profile_data)
{
  profile_data->vibrating_enabled = profile_get_value_as_bool(profile_name,
							      PROFILEKEY_VIBRATING_ALERT_ENABLED);
  // ringtone
  profile_data->ringing_tone = profile_get_value(profile_name,
						 PROFILEKEY_RINGING_ALERT_TONE);
  profile_data->ringing_volume = profile_get_value_as_int(profile_name,
							  PROFILEKEY_RINGING_ALERT_VOLUME);
  
  // sms
  profile_data->sms_tone = profile_get_value(profile_name,
					     PROFILEKEY_SMS_ALERT_TONE);
  profile_data->sms_volume = profile_get_value_as_int(profile_name,
						      PROFILEKEY_SMS_ALERT_VOLUME);
  
  // IM
  profile_data->im_tone = profile_get_value(profile_name,
					    PROFILEKEY_IM_ALERT_TONE);
  profile_data->im_volume = profile_get_value_as_int(profile_name,
						     PROFILEKEY_IM_ALERT_VOLUME);
  
  
  // email
  profile_data->email_tone = profile_get_value(profile_name,
					       PROFILEKEY_EMAIL_ALERT_TONE);
  profile_data->email_volume = profile_get_value_as_int(profile_name,
							PROFILEKEY_EMAIL_ALERT_VOLUME);
  
  // sound_level
  
  profile_data->keypad_sound_level = profile_get_value_as_int(profile_name,
							      PROFILEKEY_KEYPAD_SOUND_LEVEL);
  
  profile_data->touchscreen_sound_level = profile_get_value_as_int(profile_name,
								   PROFILEKEY_TOUCHSCREEN_SOUND_LEVEL);

  profile_data->touch_vibrating_enabled = profile_get_value_as_bool(profile_name,
								   PROFILEKEY_TOUCHSCREEN_VIBRATION);
  
  profile_data->system_sound_level = profile_get_value_as_int(profile_name,
							      PROFILEKEY_SYSTEM_SOUND_LEVEL);
  
  profile_data->status_bar_icon_name = load_icon_name(profile_name);
  load_gconf_settings(profile_name, profile_data);
}

void
profilesx_free_profile_data(profilesx_profile_data* profile_data)
{
  g_free(profile_data->ringing_tone);
  g_free(profile_data->sms_tone);
  g_free(profile_data->im_tone);
  g_free(profile_data->email_tone);
  g_free(profile_data->status_bar_icon_name);
  g_free(profile_data);
  profile_data = NULL;
}

