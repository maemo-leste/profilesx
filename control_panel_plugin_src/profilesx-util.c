#include <stdio.h>
#include <glib.h>

#define KEYFILE "/etc/profiled/80.profilesx.ini"

void handle_error(GError* error)
{
  #if 0
  printf("Error: %s\n", error->message);
  #endif
  g_error_free(error);
  error = NULL;
}

GKeyFile* load_key_file()
{
  GKeyFile* keyFile = g_key_file_new();
  GError* error = NULL;
  g_key_file_load_from_file(keyFile,
			    KEYFILE,
			    G_KEY_FILE_KEEP_COMMENTS,
			    &error);
  if(error)
  {
    handle_error(error);
  }
  return keyFile;
}

int save_key_file(GKeyFile* keyFile, GError** error)
{
  gchar* data = NULL;
  gsize size;
  data = g_key_file_to_data(keyFile, 
			    &size,
			    error);
  if((*error))
  {
    handle_error(*error);
    g_free(data);
    return 1;
  }
  int ret = g_file_set_contents(KEYFILE, data, size, error);
  g_free(data);
  if((*error))
  {
    handle_error(*error);
    return 1;
  }
  return ret;
}

int remove_profile(gchar* profile)
{
  GKeyFile* keyFile = load_key_file();
  GError* error = NULL;
  int ret = 0;
  if(g_key_file_has_group(keyFile, profile))
  {
    g_key_file_remove_group(keyFile, profile, NULL);
    ret = save_key_file(keyFile, &error);
  }
  g_key_file_free(keyFile);
  if(error)
  {
    handle_error(error);
    return 1;
  }
  return ret;
}

int add_profile(gchar* profile)
{
  GKeyFile* keyFile = load_key_file();
  GError* error = NULL;
  int ret = 0;
  if(!g_key_file_has_group(keyFile, profile))
  {
    g_key_file_set_string(keyFile, 
			  profile,
			  "key",
			  "no_comment");
    
    g_key_file_remove_key(keyFile,
			  profile,
			  "key",
			  NULL);

    ret = save_key_file(keyFile, &error);
  }
  g_key_file_free(keyFile);
  if(error)
  {
    handle_error(error);
    return 1;
  }
  return ret;
}

int main(int argc, char** argv)
{
  if(argc!=3)
  {
    return 1;
  }

  if(g_strcmp0(argv[1], "-r")==0)
  {
    return remove_profile(argv[2]);
  }
  else if(g_strcmp0(argv[1], "-a")==0)
  {
    return add_profile(argv[2]);
  }
  else
  {
    return 1;
  }
}
