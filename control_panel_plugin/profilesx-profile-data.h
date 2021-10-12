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
#include <glib.h>
typedef struct _profilesx_profile_data_t profilesx_profile_data;

struct _profilesx_profile_data_t
{
  gboolean vibrating_enabled;
  gchar* ringing_tone;
  gint ringing_volume;
  gchar* sms_tone;
  gint sms_volume;
  gchar* im_tone;
  gint im_volume;
  gchar* email_tone;
  gint email_volume;
  gint keypad_sound_level;
  gint touchscreen_sound_level;
  gboolean touch_vibrating_enabled;
  gint system_sound_level;
  gchar* status_bar_icon_name;
  gboolean autoanswer;
  gint auto_answer_delay;
  gboolean disable_proximity_check;
  gboolean turn_speaker_on;
};

void
profilesx_load_profile_data(const gchar* profile_name,
			    profilesx_profile_data* profile_data);
void
profilesx_store_profile_data(const gchar* profile_name,
			    profilesx_profile_data* profile_data);
void
profilesx_free_profile_data(profilesx_profile_data* profile_data);

