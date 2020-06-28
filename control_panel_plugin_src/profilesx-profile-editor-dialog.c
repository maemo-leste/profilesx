#include "profilesx-profile-editor-dialog.h"
#include "profilesx-control-panel-plugin.h"
#include <gtk/gtk.h>
#include <libprofile.h>
#include <keys_nokia.h>
#include <libintl.h>
#include <locale.h>
#include <string.h>
#include <hildon/hildon.h>
#include <hildon/hildon-file-chooser-dialog.h>

#define IM_SECTION "[IM]"
#define SMS_SECTION "[SMS]"
#define EMAIL_SECTION "[EMAIL]"
#define RINGING_SECTION "[RINGTONE]" // [sic!]

#define PROFILESX_PLUGIN_SETTINGS_FILE "/.profilesx"
#define VOLUME_SCALE 0.001

struct _editor_data_t
{
  GtkWidget* iconButton;
  GtkWidget* ringtoneSelector;
  GtkWidget* ringtoneButton;
  GtkWidget* emailtoneSelector;
  GtkWidget* emailtoneButton;
  GtkWidget* imtoneSelector;
  GtkWidget* imtoneButton;
  GtkWidget* smstoneSelector;
  GtkWidget* smstoneButton;
  GtkWidget* ringtoneVolumeSlider;
  GtkWidget* smsVolumeSlider;
  GtkWidget* imVolumeSlider;
  GtkWidget* emailVolumeSlider;

  GtkWidget* keypadsoundLevelButton;
  GtkWidget* touchscreensoundLevelButton;
  GtkWidget* systemsoundLevelButton;

  GtkListStore* keypadsoundLevel;
  GtkListStore* touchscreensoundLevel;
  GtkListStore* systemsoundLevel;

  GtkWidget* autoAnswerButton;
  GtkWidget* disableProximityCheck;
  GtkWidget* autoAnswerDelaySlider;
  GtkWidget* turnSpeakerOnButton;
  GtkWidget* main_dialog;
  GtkWidget* vibratingCheckbutton;

  GtkListStore* ringingTone;
  GtkListStore* smsTone;
  GtkListStore* imTone;
  GtkListStore* emailTone;  
  GstElement* sound_player;
  profilesx_profile_data* profile_data;
};

void play_sound(const gchar* file, editor_data_t* data)
{
  int ret = gst_element_set_state(data->sound_player, GST_STATE_NULL);
  GstState state;
  GstState pending;
  int stret = gst_element_get_state(data->sound_player, &state, &pending, GST_CLOCK_TIME_NONE);
  g_object_set(data->sound_player, "uri", g_filename_to_uri(file, NULL, NULL), NULL);
  int ret2 = gst_element_set_state(data->sound_player, GST_STATE_PLAYING);
 
}

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

static double
_rescale_volume(gdouble volume)
{
  gdouble v = volume / 100.0;
  return v * v * v;
}

static void 
read_predefined_ringtones(editor_data_t* data)
{
  GIOChannel* channel;
  GRegex* regex;
  GError* error = NULL;

  channel = g_io_channel_new_file("/etc/ringtones", "r", &error);
  regex = g_regex_new("^\"([^\"]+)\"[ \t]+(.*)$",
		      0,
		      0,
		      NULL);
  
  gchar* line;
  gchar* line_without_linebreak;
  int length;
  gsize line_terminator;
  GtkListStore* current_list_store = NULL;

  while(g_io_channel_read_line(channel,
			       &line,
			       &length,
			       &line_terminator,
			       &error)== G_IO_STATUS_NORMAL)
  {
    if(length>2)
    {
      line_without_linebreak = g_strndup(line,
					 line_terminator);
      g_free(line);
      line = line_without_linebreak;
      if(g_strcmp0(line, SMS_SECTION)==0)
      {
	current_list_store = data->smsTone;
      }
      else if(g_strcmp0(line, IM_SECTION)==0)
      {
	current_list_store = data->imTone;
      }
      else if(g_strcmp0(line, EMAIL_SECTION)==0)
      {
	current_list_store = data->emailTone;
      }
      else if(g_strcmp0(line, RINGING_SECTION)==0)
      {
	current_list_store = data->ringingTone;
      }
      else
      {
	if(current_list_store!=NULL)
	{
	  GtkTreeIter iter;
	  GMatchInfo* match_info;
	  g_regex_match(regex, line, 0, &match_info);
	  gchar* key = g_match_info_fetch(match_info, 1);
	  gchar* value = g_match_info_fetch(match_info, 2);
	  gtk_list_store_append(current_list_store, &iter);
	  gtk_list_store_set(current_list_store, &iter, 0, key, 1, value, -1);
	  g_match_info_free(match_info);
	}
      }
    }
    g_free(line);
  }
  g_io_channel_unref(channel);
}

static gchar*
get_display_value_from_file_name(const gchar* value, GtkListStore* list_store, GtkTreeIter* iter)
{
  GFile* file;
  gchar* basename;
  file = g_file_new_for_path(value);
  basename = g_file_get_basename(file);
  g_object_unref(file);
  gtk_list_store_prepend(list_store, iter);
  gtk_list_store_set(list_store, iter, 0, basename, 1, value, -1);
  return basename;
}

static gchar*
get_display_value(const gchar* value, GtkListStore* list_store, GtkTreeIter* iter)
{
  gboolean valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(list_store), iter);
  while (valid)
  {
    gchar* key;
    gchar* list_value;
    gtk_tree_model_get (GTK_TREE_MODEL(list_store), iter,
			0, &key,
			1, &list_value,
			-1);
    if(g_strcmp0(list_value, value)==0)
    {
      g_free (list_value);
      return key;
    }
    g_free (list_value);
    g_free (key);
    valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(list_store), iter);
  }
  return get_display_value_from_file_name(value, list_store, iter);
}

void 
set_profile_tone_value(const gchar* tone, GtkWidget* button, GtkListStore* list_store)
{
  GtkTreeIter iter;
  gchar* display_value = get_display_value(tone, list_store, &iter);

  if(strlen(display_value) > 27)
  {
    gchar* short_display_value = g_strndup(display_value, 27);
    short_display_value[26] = '.';
    short_display_value[25] = '.';
    short_display_value[24] = '.';
    hildon_button_set_value(HILDON_BUTTON(button), short_display_value);
    g_free(short_display_value);
  }
  else
  {
    hildon_button_set_value(HILDON_BUTTON(button), display_value);
  }
  g_free(display_value);
}

void
set_profile_sound_level_value(const int value, GtkWidget* button)
{
  GtkTreeIter iter;
  hildon_picker_button_set_active(HILDON_PICKER_BUTTON(button), value);
  HildonTouchSelector* selector = 
    hildon_picker_button_get_selector(HILDON_PICKER_BUTTON(button));
  
  GtkListStore* list_store = 
    
    GTK_LIST_STORE(hildon_touch_selector_get_model(hildon_picker_button_get_selector(HILDON_PICKER_BUTTON(button)), 0));

  gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);

  while(valid)
  {
    gchar* list_key;
    int list_value;
    gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, 
		       0, &list_key, 1, &list_value, -1);
    if(list_value==value)
    {
      hildon_button_set_value(HILDON_BUTTON(button), list_key);
      g_free(list_key);
      return;
    }
    g_free(list_key);
    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
  }
}

void set_profile_status_icon_value(editor_data_t* data)
{
  gchar* icon_name = data->profile_data->status_bar_icon_name;
  if(icon_name!=NULL)
  {
    GtkWidget* image;
    GdkPixbuf* pixbuf = NULL;
    pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
				      icon_name,
				      18, GTK_ICON_LOOKUP_NO_SVG, NULL);
    if(!pixbuf)
    {
      gchar* icon_path = g_build_filename("/home/user/.profiled/", icon_name, NULL);
      pixbuf = gdk_pixbuf_new_from_file_at_size(icon_path,
						18, 18, NULL);
      g_free(icon_path);
    }
    image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    gtk_button_set_image(GTK_BUTTON(data->iconButton), image);
  }
}

void
set_profile_values(const gchar* profile_name, editor_data_t* data)
{
  set_profile_tone_value(data->profile_data->ringing_tone, data->ringtoneButton, data->ringingTone);
  set_profile_tone_value(data->profile_data->sms_tone, data->smstoneButton, data->smsTone);
  set_profile_tone_value(data->profile_data->im_tone, data->imtoneButton, data->imTone);
  set_profile_tone_value(data->profile_data->email_tone, data->emailtoneButton, data->emailTone);
  set_profile_sound_level_value(data->profile_data->keypad_sound_level, data->keypadsoundLevelButton);
  set_profile_sound_level_value(data->profile_data->touchscreen_sound_level, data->touchscreensoundLevelButton);
  set_profile_sound_level_value(data->profile_data->system_sound_level, data->systemsoundLevelButton);

  gtk_range_set_value(GTK_RANGE(data->ringtoneVolumeSlider), data->profile_data->ringing_volume);
  gtk_range_set_value(GTK_RANGE(data->smsVolumeSlider), data->profile_data->sms_volume);
  gtk_range_set_value(GTK_RANGE(data->imVolumeSlider), data->profile_data->im_volume);
  gtk_range_set_value(GTK_RANGE(data->emailVolumeSlider), data->profile_data->email_volume);

  if(data->profile_data->vibrating_enabled)
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->vibratingCheckbutton), TRUE);
  else
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->vibratingCheckbutton), FALSE);

  if(data->profile_data->autoanswer)
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->autoAnswerButton), TRUE);
  else
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->autoAnswerButton), FALSE);
  if(data->profile_data->disable_proximity_check)
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->disableProximityCheck), TRUE);
  else
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->disableProximityCheck), FALSE);
  if(data->profile_data->turn_speaker_on)
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->turnSpeakerOnButton), TRUE);
  else
    hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->turnSpeakerOnButton), FALSE);
  gtk_range_set_value(GTK_RANGE(data->autoAnswerDelaySlider), data->profile_data->auto_answer_delay);
  set_profile_status_icon_value(data);
}

void
get_iter_for_value(GtkListStore* list_store, const gchar* value, GtkTreeIter* iter)
{
  gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), iter);
  while(valid)
  {
    gchar* list_key;
    gchar* list_value;
    gtk_tree_model_get(GTK_TREE_MODEL(list_store), iter,
		       0, &list_key,
		       1, &list_value,
		       -1);
    if(g_strcmp0(list_key, value)==0)
    {
      g_free(list_value);
      g_free(list_key);
      return;
    }
    g_free(list_value);
    g_free(list_key);
    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), iter);
  }
}

void 
save_profile_tone_value(gchar** tone, GtkWidget* button, GtkListStore* list_store)
{
  GtkTreeIter iter;
  const gchar* value = hildon_button_get_value(HILDON_BUTTON(button));
  get_iter_for_value(list_store, value, &iter);
  if(gtk_list_store_iter_is_valid(list_store, &iter))
    gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter,
		       1, tone,
		       -1);
}

void
save_profile_values(const gchar* profile_name, editor_data_t* data)
{
  g_free(data->profile_data->ringing_tone);
  g_free(data->profile_data->sms_tone);
  g_free(data->profile_data->im_tone);
  g_free(data->profile_data->email_tone);
  save_profile_tone_value(&(data->profile_data->ringing_tone), data->ringtoneButton, data->ringingTone);
  save_profile_tone_value(&(data->profile_data->sms_tone), data->smstoneButton, data->smsTone);
  save_profile_tone_value(&(data->profile_data->im_tone), data->imtoneButton, data->imTone);
  save_profile_tone_value(&(data->profile_data->email_tone), data->emailtoneButton, data->emailTone);

  data->profile_data->ringing_volume = (int)gtk_range_get_value(GTK_RANGE(data->ringtoneVolumeSlider));
  data->profile_data->sms_volume = (int)gtk_range_get_value(GTK_RANGE(data->smsVolumeSlider));
  data->profile_data->im_volume = (int)gtk_range_get_value(GTK_RANGE(data->imVolumeSlider));
  data->profile_data->email_volume = (int)gtk_range_get_value(GTK_RANGE(data->emailVolumeSlider));
  data->profile_data->keypad_sound_level = 
    hildon_touch_selector_get_active(hildon_picker_button_get_selector(HILDON_PICKER_BUTTON(data->keypadsoundLevelButton)),
				     0);
  data->profile_data->touchscreen_sound_level = 
    hildon_touch_selector_get_active(hildon_picker_button_get_selector(HILDON_PICKER_BUTTON(data->touchscreensoundLevelButton)),
									    0);
  data->profile_data->system_sound_level = 
    hildon_touch_selector_get_active(hildon_picker_button_get_selector(HILDON_PICKER_BUTTON(data->systemsoundLevelButton)),
									    0);

  data->profile_data->vibrating_enabled = hildon_check_button_get_active(HILDON_CHECK_BUTTON(data->vibratingCheckbutton));
  data->profile_data->autoanswer = hildon_check_button_get_active(HILDON_CHECK_BUTTON(data->autoAnswerButton));
  data->profile_data->disable_proximity_check = hildon_check_button_get_active(HILDON_CHECK_BUTTON(data->disableProximityCheck));
  data->profile_data->turn_speaker_on = hildon_check_button_get_active(HILDON_CHECK_BUTTON(data->turnSpeakerOnButton));
  data->profile_data->auto_answer_delay = gtk_range_get_value(GTK_RANGE(data->autoAnswerDelaySlider));
  profilesx_store_profile_data(profile_name, data->profile_data);
}

void add_tone_to_list_store(GtkButton* button, GtkWidget* selector)
{
  GtkWidget* dialog;
  GtkWidget* parent = gtk_widget_get_toplevel(GTK_WIDGET(button));
  GtkListStore* list_store = GTK_LIST_STORE(hildon_touch_selector_get_model(HILDON_TOUCH_SELECTOR(selector), 0));
										    
  //  parent = gtk_widget_get_parent(parent);
  dialog = hildon_file_chooser_dialog_new(GTK_WINDOW(parent),
					  GTK_FILE_CHOOSER_ACTION_OPEN);
  g_object_set(dialog, "empty-text", dgettext("osso-profiles", "profi_va_select_object_no_sound_clips"), NULL);
  gtk_window_set_title(GTK_WINDOW(dialog), dgettext("osso-profiles", "profi_ti_open_sound_clip"));

  GtkFileFilter* fileFilter = gtk_file_filter_new();
  gtk_file_filter_add_mime_type(fileFilter, "audio/*");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), fileFilter);
  gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), fileFilter);

  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), "/home/user/MyDocs/.sounds");
  
  int ret = gtk_dialog_run(GTK_DIALOG(dialog));
  if(ret == GTK_RESPONSE_OK)
  {
    gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    GtkTreeIter iter;
    gchar* display_value = get_display_value(filename, list_store, &iter);
    hildon_touch_selector_select_iter(HILDON_TOUCH_SELECTOR(selector),
				      0, &iter, TRUE);
    g_free(filename);
    g_free(display_value);
  }
  gtk_widget_destroy(dialog);
}

void
play_sound_sample(HildonTouchSelector* selector, gint column, editor_data_t* user_data)
{
  GtkTreeIter iter;
  hildon_touch_selector_get_selected(selector,
				     0,
				     &iter);
  gchar* list_value;
  gtk_tree_model_get(hildon_touch_selector_get_model(selector, 0), &iter,
		     1, &list_value,
		     -1);
  play_sound(list_value, user_data);
}

static const gchar*
get_tone_selector_title(editor_data_t* data, GtkWidget* button)
{
  if(data)
  {
    if(button == data->ringtoneButton)
    {
      return dgettext("osso-profiles", "profi_ti_ringing_tone");
    }
    else if(button == data->smstoneButton)
    {
      return dgettext("osso-profiles", "profi_ti_sub_text_message_tone");
    }
    else if(button == data->imtoneButton)
    {
      return dgettext("osso-profiles", "profi_ti_instant_messaging_tone");
    }
    else if(button == data->emailtoneButton)
    {
      return dgettext("osso-profiles", "profi_ti_email_alert_tone");
    }  
  }
  g_return_val_if_reached(dgettext("osso-profiles", "profi_ti_ringing_tone"));
}

void
show_tone_selector(GtkListStore* list_store, GtkWidget* button, GtkWidget* selector, editor_data_t* data)
{
  GtkTreeIter iter;
  GtkWidget* more_button;

  get_iter_for_value(list_store, hildon_button_get_value(HILDON_BUTTON(button)), &iter);
  
  hildon_touch_selector_select_iter(HILDON_TOUCH_SELECTOR(selector),
				    0,
				    &iter, TRUE);


  int sound_handler_id = g_signal_connect(selector, "changed", G_CALLBACK(play_sound_sample), data);
  GtkWidget* dialog = gtk_dialog_new_with_buttons(get_tone_selector_title(data, button),
						  GTK_WINDOW(data->main_dialog),
						  GTK_DIALOG_MODAL,
						  NULL);
  profilesx_dialog_size_changed(gdk_display_get_default_screen(gdk_display_get_default()),
				dialog);
  g_signal_connect(gdk_display_get_default_screen(gdk_display_get_default()), 
		   "size-changed", 
		   G_CALLBACK(profilesx_dialog_size_changed), dialog); 

  more_button = hildon_gtk_button_new(HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT);
  gtk_button_set_label(GTK_BUTTON(more_button), dgettext("hildon-libs", "wdgt_bd_add"));
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_action_area(GTK_DIALOG(dialog))), more_button);
  g_signal_connect(more_button, "clicked", G_CALLBACK(add_tone_to_list_store), selector);
  (void) gtk_dialog_add_button(GTK_DIALOG(dialog), dgettext("hildon-libs", "wdgt_bd_done"), GTK_RESPONSE_OK);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), selector);

  gtk_window_set_default_size(GTK_WINDOW(dialog), -1, 300);  
  gtk_widget_show_all(dialog);
  int ret = gtk_dialog_run(GTK_DIALOG(dialog));
  if(ret == GTK_RESPONSE_OK)
  {
    GtkTreeIter iter;
    hildon_touch_selector_get_selected(HILDON_TOUCH_SELECTOR(selector), 0, &iter);
    gchar* v;
    gtk_tree_model_get(GTK_TREE_MODEL(list_store),
		       &iter,0, &v, -1);
    hildon_button_set_value(HILDON_BUTTON(button), v);
    g_free(v);
  }
  //  g_signal_handler_disconnect(selector, sound_handler_id);
  ret = gst_element_set_state(data->sound_player, GST_STATE_NULL);
  GstState state; 
  GstState pending; 
  int stret = gst_element_get_state(data->sound_player, &state, &pending, GST_CLOCK_TIME_NONE); 
  gtk_widget_destroy(dialog);
}

static GtkWidget* 
create_tone_selector_widget(GtkListStore* list_store)
{
  GtkWidget* widget = hildon_touch_selector_new();
  GtkCellRenderer* renderer;
  HildonTouchSelectorColumn* column;

  renderer = gtk_cell_renderer_text_new(); 
  column = hildon_touch_selector_append_column(HILDON_TOUCH_SELECTOR(widget), 
 					       GTK_TREE_MODEL(list_store), 
 					       renderer, 
 					       "text", 0, NULL); 

  g_object_set (G_OBJECT(renderer), "width", 1, "xalign", 0.5f, NULL);
  g_object_set(G_OBJECT(column), 
 	       "text-column", 0, 
 	       NULL); 
  return widget; 
}

static
void
show_ringtone_selector(GtkWidget* button, editor_data_t *data)
{
  gdouble v = _rescale_volume(gtk_range_get_value(GTK_RANGE(data->ringtoneVolumeSlider)));
  g_object_set(data->sound_player, "volume", v , NULL);
  data->ringtoneSelector=create_tone_selector_widget(data->ringingTone);
  show_tone_selector(data->ringingTone, data->ringtoneButton, data->ringtoneSelector, data);
}

static
void
show_smstone_selector(GtkWidget* button, editor_data_t *data)
{
  gdouble v = _rescale_volume(gtk_range_get_value(GTK_RANGE(data->smsVolumeSlider)));
  g_object_set(data->sound_player, "volume", v , NULL);
  data->smstoneSelector=create_tone_selector_widget(data->smsTone);
  show_tone_selector(data->smsTone, data->smstoneButton, data->smstoneSelector, data);
}

static
void
show_imtone_selector(GtkWidget* button, editor_data_t *data)
{
  gdouble v = _rescale_volume(gtk_range_get_value(GTK_RANGE(data->imVolumeSlider)));
  g_object_set(data->sound_player, "volume", v, NULL);
  data->imtoneSelector=create_tone_selector_widget(data->imTone);
  show_tone_selector(data->imTone, data->imtoneButton, data->imtoneSelector, data);
}

static
void
show_emailtone_selector(GtkWidget* button, editor_data_t *data)
{
  gdouble v = _rescale_volume(gtk_range_get_value(GTK_RANGE(data->emailVolumeSlider)));
  g_object_set(data->sound_player, "volume", v, NULL);
  data->emailtoneSelector=create_tone_selector_widget(data->emailTone);
  show_tone_selector(data->emailTone, data->emailtoneButton, data->emailtoneSelector, data);
}

static void 
init_editor_tone_selector_widgets(editor_data_t* app_data)
{
  app_data->imTone = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  app_data->emailTone = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  app_data->ringingTone = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  app_data->smsTone = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  read_predefined_ringtones(app_data);
  app_data->ringtoneButton = hildon_button_new(HILDON_SIZE_FINGER_HEIGHT,
					       HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  app_data->smstoneButton = hildon_button_new(HILDON_SIZE_FINGER_HEIGHT,
					      HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  app_data->imtoneButton = hildon_button_new(HILDON_SIZE_FINGER_HEIGHT,
					     HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  app_data->emailtoneButton = hildon_button_new(HILDON_SIZE_FINGER_HEIGHT,
						HILDON_BUTTON_ARRANGEMENT_VERTICAL);

  hildon_button_set_title(HILDON_BUTTON(app_data->ringtoneButton),
			  dgettext("osso-profiles", "profi_fi_ringingtone_tone"));
  hildon_button_set_title(HILDON_BUTTON(app_data->emailtoneButton),
			  dgettext("osso-profiles", "profi_ti_email_alert_tone"));
  hildon_button_set_title(HILDON_BUTTON(app_data->smstoneButton),
			  dgettext("osso-profiles", "profi_ti_sub_text_message_tone"));
  hildon_button_set_title(HILDON_BUTTON(app_data->imtoneButton),
			  dgettext("osso-profiles", "profi_fi_instant_messaging_alert_tone"));    
  hildon_button_set_alignment(HILDON_BUTTON(app_data->ringtoneButton),
			      0, 0,
			      1, 1);
  hildon_button_set_alignment(HILDON_BUTTON(app_data->emailtoneButton),
			      0, 0,
			      1, 1);
  hildon_button_set_alignment(HILDON_BUTTON(app_data->smstoneButton),
			      0, 0,
			      1, 1);
  hildon_button_set_alignment(HILDON_BUTTON(app_data->imtoneButton),
			      0, 0,
			      1, 1);
  hildon_button_set_style(HILDON_BUTTON (app_data->ringtoneButton), HILDON_BUTTON_STYLE_PICKER);
  hildon_button_set_style(HILDON_BUTTON (app_data->emailtoneButton), HILDON_BUTTON_STYLE_PICKER);
  hildon_button_set_style(HILDON_BUTTON (app_data->smstoneButton), HILDON_BUTTON_STYLE_PICKER);
  hildon_button_set_style(HILDON_BUTTON (app_data->imtoneButton), HILDON_BUTTON_STYLE_PICKER);

  g_signal_connect(app_data->ringtoneButton, "clicked", G_CALLBACK(show_ringtone_selector), app_data);
  g_signal_connect(app_data->smstoneButton, "clicked", G_CALLBACK(show_smstone_selector), app_data);
  g_signal_connect(app_data->imtoneButton, "clicked", G_CALLBACK(show_imtone_selector), app_data);
  g_signal_connect(app_data->emailtoneButton, "clicked", G_CALLBACK(show_emailtone_selector), app_data);
  
}

static void 
init_editor_tone_volume_widgets(editor_data_t* app_data)
{
  app_data->ringtoneVolumeSlider = hildon_gtk_hscale_new();
  gtk_scale_set_digits(GTK_SCALE(app_data->ringtoneVolumeSlider), 0);
  gtk_adjustment_configure(gtk_range_get_adjustment(GTK_RANGE(app_data->ringtoneVolumeSlider)),
			   0, 0, 100, 1, 0, 0);
			   
  app_data->smsVolumeSlider = hildon_gtk_hscale_new();
  gtk_scale_set_digits(GTK_SCALE(app_data->smsVolumeSlider), 0);
  gtk_adjustment_configure(gtk_range_get_adjustment(GTK_RANGE(app_data->smsVolumeSlider)),
			   0, 0, 100, 1, 0, 0);
			   
  app_data->imVolumeSlider = hildon_gtk_hscale_new();
  gtk_scale_set_digits(GTK_SCALE(app_data->imVolumeSlider), 0);
  gtk_adjustment_configure(gtk_range_get_adjustment(GTK_RANGE(app_data->imVolumeSlider)),
			   0, 0, 100, 1, 0, 0);
			   
  app_data->emailVolumeSlider = hildon_gtk_hscale_new();
  gtk_scale_set_digits(GTK_SCALE(app_data->emailVolumeSlider), 0);
  gtk_adjustment_configure(gtk_range_get_adjustment(GTK_RANGE(app_data->emailVolumeSlider)),
			   0, 0, 100, 1, 0, 0);
			   
}

static GtkWidget* 
create_sound_level_widget(GtkListStore* list_store, gchar* title)
{
  GtkWidget* selector;
  GtkCellRenderer* renderer;
  HildonTouchSelectorColumn* column;
  GtkWidget* widget = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT,
					       HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  hildon_button_set_alignment(HILDON_BUTTON(widget),
			      0, 0,
			      1, 1);

  renderer = gtk_cell_renderer_text_new();
  selector = hildon_touch_selector_new();
  column = hildon_touch_selector_append_column(HILDON_TOUCH_SELECTOR(selector),
					       GTK_TREE_MODEL(list_store), renderer,
					       "text", 0, NULL);
  g_object_set(G_OBJECT(column), "text-column", 0, NULL);
 
  g_object_set (G_OBJECT(renderer), "width", 1, "xalign", 0.5f, NULL);  
  hildon_button_set_title(HILDON_BUTTON(widget),
			  title);
  hildon_picker_button_set_selector(HILDON_PICKER_BUTTON(widget), 
				    HILDON_TOUCH_SELECTOR(selector));
  return widget;
}

static void 
load_custom_icon(GtkButton* button, gchar** icon_path)
{
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
  {
    GtkWidget* dialog;
    GtkWidget* parent = gtk_widget_get_toplevel(GTK_WIDGET(button));
    dialog = hildon_file_chooser_dialog_new(GTK_WINDOW(parent),
					    GTK_FILE_CHOOSER_ACTION_OPEN);
    GtkFileFilter* fileFilter = gtk_file_filter_new();
    gtk_file_filter_add_pixbuf_formats(fileFilter);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), fileFilter);

    gtk_widget_show_all(GTK_WIDGET(dialog));
    int ret = gtk_dialog_run(GTK_DIALOG(dialog));
    if(ret == GTK_RESPONSE_OK)
    {
      gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      if(*icon_path)
	g_free(*icon_path);
      (*icon_path) = filename;
      GtkWidget* image = gtk_image_new_from_file(filename);
      gtk_button_set_image(button, image);
    }
    gtk_widget_destroy(dialog);
  }
}

static void 
icon_selection_dialog(GtkButton* button, editor_data_t* app_data)
{
  const gchar* icon_name_silent = "statusarea_silent";
  const gchar* icon_name_home = "statusarea_profilesx_home";
  const gchar* icon_name_im = "statusarea_profilesx_im";
  const gchar* icon_name_outside = "statusarea_profilesx_outside";
  const gchar* icon_name_loud = "statusarea_profilesx_loud";

  GtkWidget* dialog = gtk_dialog_new_with_buttons("Statusbar icon",
						  GTK_WINDOW(app_data->main_dialog),
						  GTK_DIALOG_MODAL,
						  dgettext("hildon-libs", "wdgt_bd_save"),
						  GTK_RESPONSE_OK,
						  NULL);
  GtkWidget* icon_box = gtk_hbox_new(TRUE, 0);

  GtkWidget* icon_button_none = hildon_gtk_radio_button_new_from_widget(HILDON_SIZE_FINGER_HEIGHT,
									NULL);
  GtkWidget* icon_button_silent = hildon_gtk_radio_button_new_from_widget(HILDON_SIZE_FINGER_HEIGHT,
									  GTK_RADIO_BUTTON(icon_button_none));
  GtkWidget* icon_button_home = hildon_gtk_radio_button_new_from_widget(HILDON_SIZE_FINGER_HEIGHT,
									GTK_RADIO_BUTTON(icon_button_silent));
  GtkWidget* icon_button_im = hildon_gtk_radio_button_new_from_widget(HILDON_SIZE_FINGER_HEIGHT,
								      GTK_RADIO_BUTTON(icon_button_home));
  GtkWidget* icon_button_outside = hildon_gtk_radio_button_new_from_widget(HILDON_SIZE_FINGER_HEIGHT,
									   GTK_RADIO_BUTTON(icon_button_im));
  GtkWidget* icon_button_loud = hildon_gtk_radio_button_new_from_widget(HILDON_SIZE_FINGER_HEIGHT,
									GTK_RADIO_BUTTON(icon_button_outside));
  GtkWidget* icon_button_custom = hildon_gtk_radio_button_new_from_widget(HILDON_SIZE_FINGER_HEIGHT,
									  GTK_RADIO_BUTTON(icon_button_loud));
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(icon_button_none), FALSE);
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(icon_button_silent), FALSE);
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(icon_button_home), FALSE);
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(icon_button_im), FALSE);
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(icon_button_outside), FALSE);
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(icon_button_loud), FALSE);
  gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(icon_button_custom), FALSE);

  GdkPixbuf* pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
					       icon_name_silent,
					       18, GTK_ICON_LOOKUP_NO_SVG, NULL);
  
  gtk_button_set_image(GTK_BUTTON(icon_button_silent), gtk_image_new_from_pixbuf(pixbuf));
  g_object_unref(pixbuf);
  pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
				    icon_name_home,
				    18, GTK_ICON_LOOKUP_NO_SVG, NULL);
  gtk_button_set_image(GTK_BUTTON(icon_button_home), gtk_image_new_from_pixbuf(pixbuf));
  g_object_unref(pixbuf);
  pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
				    icon_name_im,
				    18, GTK_ICON_LOOKUP_NO_SVG, NULL);
  gtk_button_set_image(GTK_BUTTON(icon_button_im), gtk_image_new_from_pixbuf(pixbuf));
  g_object_unref(pixbuf);
  pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
				    icon_name_outside,
				    18, GTK_ICON_LOOKUP_NO_SVG, NULL);
  gtk_button_set_image(GTK_BUTTON(icon_button_outside), gtk_image_new_from_pixbuf(pixbuf));
  g_object_unref(pixbuf);
  pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
				    icon_name_loud,
				    18, GTK_ICON_LOOKUP_NO_SVG, NULL);
  gtk_button_set_image(GTK_BUTTON(icon_button_loud), gtk_image_new_from_pixbuf(pixbuf));
  g_object_unref(pixbuf);
  pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
				    "general_folder",
				    18, GTK_ICON_LOOKUP_NO_SVG, NULL);
  gtk_button_set_image(GTK_BUTTON(icon_button_custom), gtk_image_new_from_pixbuf(pixbuf));
  g_object_unref(pixbuf);

  gtk_box_pack_start(GTK_BOX(icon_box), icon_button_none, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(icon_box), icon_button_silent, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(icon_box), icon_button_home, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(icon_box), icon_button_im, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(icon_box), icon_button_outside, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(icon_box), icon_button_loud, TRUE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(icon_box), icon_button_custom, TRUE, TRUE, 3);
  gchar* custom_icon_name = NULL;
  g_signal_connect(icon_button_custom, "clicked", G_CALLBACK(load_custom_icon), &custom_icon_name);

  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), icon_box);
  
  gtk_widget_show_all(dialog);
  int ret = gtk_dialog_run(GTK_DIALOG(dialog));
  if(ret == GTK_RESPONSE_OK)
  {
    g_free(app_data->profile_data->status_bar_icon_name);
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(icon_button_custom)))
    {
      app_data->profile_data->status_bar_icon_name = custom_icon_name;
      gtk_button_set_image(button, gtk_button_get_image(GTK_BUTTON(icon_button_custom)));
    }
    else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(icon_button_none)))
    {
      app_data->profile_data->status_bar_icon_name = NULL;
      gtk_button_set_image(button, gtk_button_get_image(GTK_BUTTON(icon_button_none)));
    }
    else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(icon_button_silent)))
    {
      app_data->profile_data->status_bar_icon_name = g_strdup(icon_name_silent);
      gtk_button_set_image(button, gtk_button_get_image(GTK_BUTTON(icon_button_silent)));
    }
    else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(icon_button_home)))
    {
      app_data->profile_data->status_bar_icon_name = g_strdup(icon_name_home);
      gtk_button_set_image(button, gtk_button_get_image(GTK_BUTTON(icon_button_home)));
    }
    else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(icon_button_im)))
    {
      app_data->profile_data->status_bar_icon_name = g_strdup(icon_name_im);
      gtk_button_set_image(button, gtk_button_get_image(GTK_BUTTON(icon_button_im)));
    }
    else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(icon_button_outside)))
    {
      app_data->profile_data->status_bar_icon_name = g_strdup(icon_name_outside);
      gtk_button_set_image(button, gtk_button_get_image(GTK_BUTTON(icon_button_outside)));
    }
    else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(icon_button_loud)))
    {
      app_data->profile_data->status_bar_icon_name = g_strdup(icon_name_loud);
      gtk_button_set_image(button, gtk_button_get_image(GTK_BUTTON(icon_button_loud)));
    }
  }
  gtk_widget_destroy(dialog);
}

static
void 
init_sound_level_list_store(GtkListStore* list_store)
{
  GtkTreeIter iter;
  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set(list_store, &iter, 0, dgettext("osso-profiles", "profi_va_key_sounds_off"), 1, 0, -1);
  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set(list_store, &iter, 0, dgettext("osso-profiles", "profi_va_key_sounds_level1"), 1, 1, -1);
  gtk_list_store_append(list_store, &iter);
  gtk_list_store_set(list_store, &iter, 0, dgettext("osso-profiles", "profi_va_key_sounds_level2"), 1, 2, -1);
}

static void
_enable_disable_proximity_check(HildonCheckButton* button, gpointer user_data)
{
  gtk_widget_set_sensitive(GTK_WIDGET(user_data), hildon_check_button_get_active(button));
}

static void 
layout_dialog_widgets(const gchar* profile_name, editor_data_t* data, GtkWidget* dialog)
{
  GtkWidget* content_area;
  GtkWidget* pannable_area;
  GtkWidget* iconBox = gtk_hbox_new(TRUE, 0);
  GtkWidget* ringtoneBox = gtk_hbox_new(TRUE, 0);
  GtkWidget* smsBox = gtk_hbox_new(TRUE, 0);
  GtkWidget* imBox = gtk_hbox_new(TRUE, 0);
  GtkWidget* emailBox = gtk_hbox_new(TRUE, 0);
  GtkWidget* icon_label = gtk_label_new("Statusbar icon");
  GtkWidget* auto_answer_box = gtk_vbox_new(TRUE, 0);

  gtk_box_pack_start(GTK_BOX(iconBox), icon_label, TRUE, TRUE, 0); 
  gtk_box_pack_end(GTK_BOX(iconBox), data->iconButton, TRUE, TRUE, 0); 
  gtk_box_pack_start(GTK_BOX(ringtoneBox), data->ringtoneButton, TRUE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(ringtoneBox), data->ringtoneVolumeSlider, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(smsBox), data->smstoneButton, TRUE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(smsBox), data->smsVolumeSlider, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(imBox), data->imtoneButton, TRUE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(imBox), data->imVolumeSlider, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(emailBox), data->emailtoneButton, TRUE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(emailBox), data->emailVolumeSlider, TRUE, TRUE, 0);

  if(g_strcmp0(profile_name, "silent")!=0)
  {
    content_area = gtk_vbox_new(FALSE, 3);  
    gtk_box_pack_start(GTK_BOX(content_area), iconBox, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), data->vibratingCheckbutton, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), ringtoneBox, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), smsBox, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), imBox, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), emailBox, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), data->systemsoundLevelButton, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), data->keypadsoundLevelButton, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), data->touchscreensoundLevelButton, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), data->autoAnswerButton, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(auto_answer_box), data->autoAnswerDelaySlider, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(auto_answer_box), data->disableProximityCheck, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_area), auto_answer_box, TRUE, TRUE, 0);
    g_signal_connect(data->autoAnswerButton, "toggled", G_CALLBACK(_enable_disable_proximity_check), auto_answer_box);
    if(!hildon_check_button_get_active(HILDON_CHECK_BUTTON(data->autoAnswerButton)))
      gtk_widget_set_sensitive(auto_answer_box, FALSE);
    gtk_box_pack_start(GTK_BOX(content_area), data->turnSpeakerOnButton, TRUE, FALSE, 0);
    pannable_area = hildon_pannable_area_new();
    hildon_pannable_area_add_with_viewport(HILDON_PANNABLE_AREA(pannable_area), content_area);
    g_object_set(pannable_area, "hscrollbar-policy", GTK_POLICY_NEVER, NULL);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		      pannable_area);
  }
  else
  {
    content_area = gtk_vbox_new(FALSE, 3);  
    gtk_box_pack_start(GTK_BOX(content_area), iconBox, TRUE, TRUE, 3);
    gtk_box_pack_end(GTK_BOX(content_area), data->vibratingCheckbutton, TRUE, TRUE, 3);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		       content_area,
		       FALSE, FALSE, 0);
  }
}

gchar*
_format_autoanswerdelay_value(GtkScale* scale,
			      double value,
			      gpointer user_data)
{
  return g_strdup_printf("%0.*g seconds", gtk_scale_get_digits(scale), value);
}

static void 
init_editor_dialog_widgets(editor_data_t* data)
{
  data->iconButton = hildon_button_new(HILDON_SIZE_FINGER_HEIGHT,
					   HILDON_BUTTON_ARRANGEMENT_VERTICAL);
  g_signal_connect(data->iconButton, "clicked", G_CALLBACK(icon_selection_dialog), data);

  init_editor_tone_selector_widgets(data);
  init_editor_tone_volume_widgets(data);
  
  data->keypadsoundLevel = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  data->touchscreensoundLevel = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  data->systemsoundLevel = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  init_sound_level_list_store(data->keypadsoundLevel);
  init_sound_level_list_store(data->systemsoundLevel);
  init_sound_level_list_store(data->touchscreensoundLevel);

  data->keypadsoundLevelButton = 
    create_sound_level_widget(data->keypadsoundLevel,
			      dgettext("osso-profiles", "profi_fi_key_sounds"));
  data->touchscreensoundLevelButton = 
    create_sound_level_widget(data->touchscreensoundLevel,
			      dgettext("osso-profiles", "profi_fi_touch_screen_sounds"));

  data->systemsoundLevelButton = 
    create_sound_level_widget(data->systemsoundLevel,
			      dgettext("osso-profiles", "profi_fi_system_sounds"));

  data->vibratingCheckbutton = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
  gtk_button_set_label(GTK_BUTTON(data->vibratingCheckbutton),
		       dgettext("osso-profiles", "profi_fi_general_vibrate"));
  hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->vibratingCheckbutton), TRUE);

  data->autoAnswerButton = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
  gtk_button_set_label(GTK_BUTTON(data->autoAnswerButton), "Autoanswer");
  hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->autoAnswerButton), FALSE);
  data->disableProximityCheck = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
  gtk_button_set_label(GTK_BUTTON(data->disableProximityCheck), "Disable Proximity Check");
  hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->disableProximityCheck), FALSE);
  data->turnSpeakerOnButton = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
  data->autoAnswerDelaySlider = hildon_gtk_hscale_new();
  gtk_range_set_range(GTK_RANGE(data->autoAnswerDelaySlider), 1, 6);
  gtk_scale_set_digits(GTK_SCALE(data->autoAnswerDelaySlider), 0);
  gtk_scale_set_draw_value(GTK_SCALE(data->autoAnswerDelaySlider), TRUE);
  g_signal_connect(data->autoAnswerDelaySlider, "format-value", G_CALLBACK(_format_autoanswerdelay_value), NULL);
  gtk_button_set_label(GTK_BUTTON(data->turnSpeakerOnButton), 
		       dgettext("rtcom-call-ui", "call_fi_call_handling_speaker"));
  hildon_check_button_set_active(HILDON_CHECK_BUTTON(data->turnSpeakerOnButton), FALSE);
}

void 
show_editor_dialog(const gchar* profile_name, GtkWidget* main_dialog, GstElement* sound_player)
{
  GtkWidget* dialog;
  editor_data_t* data = g_new0(editor_data_t, 1);
  data->profile_data = g_new0(profilesx_profile_data, 1);
  data->main_dialog = main_dialog;
  profilesx_load_profile_data(profile_name,
			      data->profile_data);
  dialog = gtk_dialog_new_with_buttons(get_profile_display_name(profile_name),
				       GTK_WINDOW(main_dialog),
				       GTK_DIALOG_MODAL,
				       dgettext("hildon-libs", "wdgt_bd_save"), GTK_RESPONSE_OK,
				       NULL);
  profilesx_dialog_size_changed(gdk_display_get_default_screen(gdk_display_get_default()),
				dialog);
  g_signal_connect(gdk_display_get_default_screen(gdk_display_get_default()), 
		   "size-changed", 
		   G_CALLBACK(profilesx_dialog_size_changed), dialog); 

  init_editor_dialog_widgets(data);
  set_profile_values(profile_name, data);
  data->sound_player = sound_player;

  layout_dialog_widgets(profile_name, data, dialog);
  gtk_widget_show_all(dialog);

  int ret = gtk_dialog_run(GTK_DIALOG(dialog));
  if(ret == GTK_RESPONSE_OK)
  {
    save_profile_values(profile_name, data);
    gchar* active_profile = profile_get_profile();
    if(g_strcmp0(active_profile, profile_name)==0)
    {
      profile_set_profile(profile_name);
    }
    g_free(active_profile);
  }
  gtk_widget_destroy(dialog);
  gtk_list_store_clear(data->imTone);
  gtk_list_store_clear(data->smsTone);
  gtk_list_store_clear(data->ringingTone);
  gtk_list_store_clear(data->emailTone);
  gtk_list_store_clear(data->keypadsoundLevel);
  gtk_list_store_clear(data->touchscreensoundLevel);
  gtk_list_store_clear(data->systemsoundLevel);
  g_object_unref(data->imTone);
  g_object_unref(data->smsTone);
  g_object_unref(data->ringingTone);
  g_object_unref(data->emailTone);
  g_object_unref(data->keypadsoundLevel);
  g_object_unref(data->touchscreensoundLevel);
  g_object_unref(data->systemsoundLevel);
  profilesx_free_profile_data(data->profile_data);
  g_free(data);
}
