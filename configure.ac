AC_INIT([ProfilesX Extended Profiles Manager],
        [1.7.0],
        [merlijn@wizzup.org], [profilesx])

AM_INIT_AUTOMAKE
LT_INIT([shared disable-static])
AC_CONFIG_FILES([Makefile control_panel_plugin/Makefile status_panel_plugin/Makefile])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(LIBHILDON, hildon-1)
AC_SUBST(LIBHILDON_CFLAGS)
AC_SUBST(LIBHILDON_LIBS)

PKG_CHECK_VAR([GLIB_GENMARSHAL], [glib-2.0], [glib_genmarshal])

PKG_CHECK_MODULES(GLIB, glib-2.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(PROFILE, profile)
AC_SUBST(PROFILE_CFLAGS)
AC_SUBST(PROFILE_LIBS)

PKG_CHECK_MODULES(LIBHILDONDESKTOP, libhildondesktop-1)
AC_SUBST(LIBHILDONDESKTOP_CFLAGS)
AC_SUBST(LIBHILDONDESKTOP_LIBS)

PKG_CHECK_MODULES(GCONF, gconf-2.0)
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)

PKG_CHECK_MODULES(GSTBASE, gstreamer-base-1.0)
AC_SUBST(GSTBASE_CFLAGS)
AC_SUBST(GSTBASE_LIBS)

PKG_CHECK_MODULES(HILDONFM, hildon-fm-2)
AC_SUBST(HILDONFM_CFLAGS)
AC_SUBST(HILDONFM_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-2.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(HILDONCONTROLPANEL, hildon-control-panel)
AC_SUBST(HILDONCONTROLPANEL_CFLAGS)
AC_SUBST(HILDONCONTROLPANEL_LIBS)

PKG_CHECK_MODULES(OSSO, libosso)
AC_SUBST(OSSO_CFLAGS)
AC_SUBST(OSSO_LIBS)

PKG_CHECK_MODULES(PULSE, libpulse)
AC_SUBST(PULSE_CFLAGS)
AC_SUBST(PULSE_LIBS)


hildoncontrolpaneldesktopentrydir="`$PKG_CONFIG --variable=plugindesktopentrydir hildon-control-panel`"
AC_SUBST(hildoncontrolpaneldesktopentrydir)

hildoncontrolpanelpluginlibdir="`$PKG_CONFIG --variable=pluginlibdir hildon-control-panel`"
AC_SUBST(hildoncontrolpanelpluginlibdir)

hildondesktoplibdir="`$PKG_CONFIG --variable=hildondesktoplibdir libhildondesktop-1`"
AC_SUBST(hildondesktoplibdir)
hildondesktopdatadir="`$PKG_CONFIG --variable=hildonstatusmenudesktopentrydir libhildondesktop-1`"
AC_SUBST(hildondesktopdatadir)

AC_OUTPUT
