OBJS=profilesx-control-panel-plugin.o profilesx-profile-waitdialog.o profilesx-profile-editor-dialog.o profilesx-profile-data.o
SOURCES = profilesx-control-panel-plugin.c profilesx-profile-waitdialog.c profilesx-profile-editor-dialog.c profilesx-profile-data.c
BUILDDIR = ../build
LIB=$(BUILDDIR)/profilesx-cp-plugin.so
PKG_FLAGS=$(shell pkg-config gstreamer-base-1.0 hildon-fm-2 profile gtk+-2.0 hildon-control-panel libosso libpulse --libs --cflags)
UTIL-BIN=profilesx-util
UTIL-OBJ=profilesx-util.o
CCFLAGS=-shared -fPIC
CC=gcc

all: $(LIB) $(UTIL-BIN)

$(UTIL-BIN): $(UTIL-OBJ)
	$(CC) $(CCFLAGS) $(shell pkg-config glib-2.0 --cflags --libs) $< -o $(BUILDDIR)/$@

$(LIB):$(OBJS)
	$(CC) $(CCFLAGS) $(PKG_FLAGS) $(OBJS) -o $(LIB)


.c.o:
	$(CC) $(CCFLAGS) $(PKG_FLAGS) -c $< -o $@


.PHONY: clean all

clean:
	rm $(OBJS) $(UTIL-OBJ)
