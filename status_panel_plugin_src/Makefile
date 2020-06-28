SRCDIR = src
OBJS=profilesx-status-panel-plugin.o profile_changed_marshal.o
SOURCES = profilesx-status-panel-plugin.c
MARSHAL_LIST=profile_changed_marshal.h profile_changed_marshal.c
BUILDDIR = ../build
LIB=$(BUILDDIR)/profilesx-sp-plugin.so
PKG_FLAGS=$(shell pkg-config gconf-2.0 hildon-1 libhildondesktop-1 profile --libs --cflags)
CCFLAGS=-shared -fPIC
CC=gcc

all: $(MARSHAL_LIST) $(LIB)


$(LIB):$(OBJS)
	$(CC) $(CCFLAGS) $(PKG_FLAGS) $(OBJS) -o $(LIB)

.c.o:
	$(CC) $(CCFLAGS) $(PKG_FLAGS) -c $< -o $@

profile_changed_marshal.c: profile_changed_marshal.list
	glib-genmarshal --body --prefix=_profile_changed_marshal profile_changed_marshal.list  > profile_changed_marshal.c


profile_changed_marshal.h: profile_changed_marshal.list
	glib-genmarshal --header --prefix=_profile_changed_marshal profile_changed_marshal.list > profile_changed_marshal.h


.PHONY: clean all
clean:
	rm $(OBJS) $(MARSHAL_LIST)
