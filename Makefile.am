
CLEANFILES = fg_config.h
EXTRA_DIST = autogen.sh \
	COPYING \
	ChangeLog \
	INSTALL \
	NEWS \
	README \
	VERSION \
	pidgin-festival.spec

SUBDIRS=src po

BUILT_SOURCES = fg_config.h

$(OBJECTS): $(BUILT_SOURCES)

fg_config.h: pre_config.h
	$(sedpath) 's/#define PACKAGE/#define FG_PACKAGE/g' pre_config.h > $@


