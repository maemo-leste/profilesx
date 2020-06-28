#CFLAGS = -Wall -Werror -Wextra -std=gnu99 -O2 -ggdb -g

hildondesktoplib_LTLIBRARIES = libprofilesx-sp-plugin.la
libprofilesx_sp_plugin_la_SOURCES = profilesx-status-panel-plugin.c
libprofilesx_sp_plugin_la_CFLAGS = $(GCONF_CFLAGS) $(LIBHILDON_CFLAGS) $(LIBHILDONDESKTOP_CFLAGS) $(PROFILE_CFLAGS)
libprofilesx_sp_plugin_la_LDFLAGS = $(GCONF_LIBS) $(LIBHILDON_LIBS) $(LIBHILDONDESKTOP_LIBS) $(PROFILE_LIBS)

profile_changed_marshal.h: profile_changed_marshal.list
		$(AM_V_GEN)$(GLIB_GENMARSHAL) \
				--prefix=_profile_changed_marshal \
				--header \
				--output=$@ \
				$<

profile_changed_marshal.c: profile_changed_marshal.list profile_changed_marshal.h
		$(AM_V_GEN)$(GLIB_GENMARSHAL) \
				--prefix=_profile_changed_marshal \
				--include-header=profile_changed_marshal.h \
				--body \
				--output=$@ \
				$<

BUILT_SOURCES = profile_changed_marshal.h profile_changed_marshal.c
CLEANFILES = profile_changed_marshal.h profile_changed_marshal.c
EXTRA_DIST = profile_changed_marshal.list

install-data-hook:
	@(cd $(DESTDIR)$(hildondesktoplibdir) && $(RM) \
		$(hildondesktoplib_LTLIBRARIES))

