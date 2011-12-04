plugindir = $(LIBPURPLE_LIBDIR)/purple-2

festival_la_LDFLAGS = \
	-module \
	-avoid-version

if PLUGINS

plugin_LTLIBRARIES = festival.la

festival_la_SOURCES = \
	festival.c

endif

AM_CPPFLAGS = \
	-DLIBDIR=\"$(LIBPURPLE_LIBDIR)/purple-2/\" \
	-DDATADIR=\"$(LIBPURPLE_DATADIR)\" \
	-DLOCALEDIR=\"$(LIBPURPLE_DATADIR)/locale\" \
	$(DEBUG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	$(LIBPURPLE_CFLAGS) \
	$(FESTIVAL_CFLAGS)
