#CFLAGS = -Wall -Werror -Wextra -std=gnu99 -O2 -ggdb -g

bin_PROGRAMS = profilesx_util

profilesx_util_SOURCES = profilesx-util.c
profilesx_util_CFLAGS = $(GLIB_CFLAGS)
profilesx_util_LDADD = $(GLIB_LIBS)

hildoncontrolpanelpluginlib_LTLIBRARIES = libprofilesx-cp-plugin.la

libprofilesx_cp_plugin_la_SOURCES = profilesx-control-panel-plugin.c \
									profilesx-profile-data.c \
									profilesx-profile-editor-dialog.c \
									profilesx-profile-waitdialog.c
libprofilesx_cp_plugin_la_CFLAGS = $(GSTBASE_CFLAGS) $(HILDONFM_CFLAGS) $(PROFILE_CFLAGS) $(HILDONCONTROLPANEL_CFLAGS) $(OSSO_CFLAGS) $(PULSE_CFLAGS)
libprofilesx_cp_plugin_la_LIBADD = $(GSTBASE_LIBS) $(HILDONFM_LIBS) $(PROFILE_LIBS) $(HILDONCONTROLPANEL_LIBS) $(OSSO_LIBS) $(PULSE_LIBS)
libprofilesx_cp_plugin_la_LDFLAGS = -Wl,--as-needed -module -avoid-version

install-data-hook:
	@(cd $(DESTDIR)$(hildoncontrolpanelpluginlibdir) && $(RM) \
		$(hildoncontrolpanelpluginlib_LTLIBRARIES))

