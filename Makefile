SUBDIRS = control_panel_plugin_src status_panel_plugin_src

HILDON_CONTROL_PANEL_LIB_DIR=$(shell pkg-config hildon-control-panel --variable=pluginlibdir)
HILDON_STATUS_PANEL_LIB_DIR=$(shell pkg-config libhildondesktop-1 --variable=hildondesktoplibdir)
HILDON_CONTROL_PANEL_DATA_DIR=$(shell pkg-config hildon-control-panel --variable=plugindesktopentrydir)
HILDON_STATUS_PANEL_DATA_DIR=$(shell pkg-config libhildondesktop-1 --variable=hildonstatusmenudesktopentrydir)

#HILDON_CONTROL_PANEL_LIB_DIR=/usr/lib/hildon-control-panel
#HILDON_STATUS_PANEL_LIB_DIR=/usr/lib/hildon-desktop
#HILDON_CONTROL_PANEL_DATA_DIR=/usr/share/applications/hildon-control-panel
#HILDON_STATUS_PANEL_DATA_DIR=/usr/share/applications/hildon-status-menu
BUILDDIR=build
CP_LIB=profilesx-cp-plugin.so
SP_LIB=profilesx-sp-plugin.so
UTIL_BIN=profilesx-util
UTIL_PATH=/usr/bin
DATA_FILE_CP=profilesx-cp-plugin.desktop
DATA_FILE_SP=profilesx-sp-plugin.desktop
#ETC_PROFILE_FILE=80.profilesx.ini
#ETC_PROFILE_PATH=/etc/profiled
IMAGE_PATH=/usr/share/icons/hicolor/18x18/hildon/
IMAGE_48_PATH=/usr/share/icons/hicolor/48x48/hildon/
SUDOERS_PATH=/etc/sudoers.d
all: create_builddir subdirs

install: all
	install -d $(DESTDIR)/$(HILDON_CONTROL_PANEL_LIB_DIR)
	install -m 644 $(BUILDDIR)/$(CP_LIB) $(DESTDIR)/$(HILDON_CONTROL_PANEL_LIB_DIR)
	install -d $(DESTDIR)/$(HILDON_STATUS_PANEL_LIB_DIR)
	install -m 644 $(BUILDDIR)/$(SP_LIB) $(DESTDIR)/$(HILDON_STATUS_PANEL_LIB_DIR)
	install -d $(DESTDIR)/$(HILDON_CONTROL_PANEL_DATA_DIR)
	install -m 644 data/$(DATA_FILE_CP) $(DESTDIR)/$(HILDON_CONTROL_PANEL_DATA_DIR)
	install -d $(DESTDIR)/$(HILDON_STATUS_PANEL_DATA_DIR)
	install -m 644 data/$(DATA_FILE_SP) $(DESTDIR)/$(HILDON_STATUS_PANEL_DATA_DIR)
#	install -d $(DESTDIR)/$(ETC_PROFILE_PATH)
#	install -m 644 data/$(ETC_PROFILE_FILE) $(DESTDIR)/$(ETC_PROFILE_PATH)
	install -d $(DESTDIR)/$(SUDOERS_PATH)
	install -m 644 data/profilesx.sudoers $(DESTDIR)/$(SUDOERS_PATH)
	install -d $(DESTDIR)/$(IMAGE_PATH)
	install -m 644 data/statusarea_profilesx_im.png $(DESTDIR)/$(IMAGE_PATH)
	install -m 644 data/statusarea_profilesx_loud.png $(DESTDIR)/$(IMAGE_PATH)
	install -m 644 data/statusarea_profilesx_outside.png $(DESTDIR)/$(IMAGE_PATH)
	install -m 644 data/statusarea_profilesx_home.png $(DESTDIR)/$(IMAGE_PATH)
	install -d $(DESTDIR)/$(IMAGE_48_PATH)
	install -m 644 data/profilesx_cp_setup.png $(DESTDIR)/$(IMAGE_48_PATH)
	install -m 755 $(BUILDDIR)/$(UTIL_BIN) $(DESTDIR)/$(UTIL_PATH)



subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

create_builddir:
	mkdir -p build

.PHONY: all clean install $(SUBDIRS)

clean:  
	rm -rf build
	for d in $(SUBDIRS); do (cd $$d; $(MAKE) clean);done
