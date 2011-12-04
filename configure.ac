#
# Most of this configure script was ripped from pidgin or the
# pidgin-encryption configure script, thanks guys ;)
#

AC_INIT(src/festival.c)
AC_PREREQ([2.50])

FG_VERSION="2.4"
AM_INIT_AUTOMAKE([pidgin-festival], $FG_VERSION, 'no-define')
AC_DEFINE_UNQUOTED(FG_VERSION, "$FG_VERSION", [Pidgin-Festival Version])

AC_PATH_PROG(sedpath, sed)
AC_DISABLE_STATIC


#
# Setup libtool
#
AM_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"

PKG_CONFIG_PATH="$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig:$HOME/lib/pkgconfig"
export PKG_CONFIG_PATH

#
# Our header
#

AH_TOP([ /* our header */
#ifndef _FG_CONFIG_H
#define _FG_CONFIG_H
])
AH_BOTTOM([
#endif /* _FG_CONFIG_H */
])

#
# Look for the C compiler
#
CFLAGS_save="$CFLAGS"
AC_PROG_CC
CFLAGS="$CFLAGS_save"

AC_ARG_ENABLE(debug,   [  --enable-debug          compile with debugging support],,enable_debug=no)
AC_ARG_ENABLE(deprecated,	[ --disable-deprecated	compile without deprecated API usage],,enable_deprecated=yes)

if test "x$enable_debug" = "xyes" ; then
	AC_DEFINE(DEBUG, 1, [Define if debugging is enabled.])
fi

if test "x$enable_deprecated" = "xno"; then
	DEBUG_CFLAGS="$DEBUG_CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
fi
AC_SUBST(DEBUG_CFLAGS)

if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall -g3"
fi
AC_SUBST(CFLAGS)

#
# Check for libpurple
#

PKG_CHECK_MODULES(LIBPURPLE, purple,
[
	AC_DEFINE(HAVE_LIBPURPLE, 1, [Define if we've found libpurple.])
])

AC_SUBST(LIBPURPLE_CFLAGS)
LIBPURPLE_LIBDIR=`pkg-config --variable=libdir purple`
LIBPURPLE_DATADIR=`pkg-config --variable=datadir purple`
AC_SUBST(LIBPURPLE_LIBDIR)
AC_SUBST(LIBPURPLE_DATADIR)

#
# Check for pidgin
#

PKG_CHECK_MODULES(PIDGIN, pidgin,
[
  AC_DEFINE(HAVE_PIDGIN, 1, [Define if we've found pidgin.])
])

AC_SUBST(PIDGIN_CFLAGS)
PIDGIN_LIBDIR=`pkg-config --variable=libdir pidgin`
PIDGIN_DATADIR=`pkg-config --variable=datadir pidgin`
AC_SUBST(PIDGIN_LIBDIR)
AC_SUBST(PIDGIN_DATADIR)

#
# check for gtk and other dependencies
#

AM_PATH_GLIB_2_0(2.0.0,,AC_MSG_ERROR([
*** GLib 2.0 is required to build Pidgin-Festival; please make sure you have
*** the GLib development headers installed. The latest version of GLib is 
*** always available at http://www.gtk.org/.]))
AC_SUBST(GLIB_CFLAGS)

AM_PATH_GTK_2_0(2.0.0,,AC_MSG_ERROR([
*** GTK+ 2.0 is required to build Pidgin-Festival; please make sure you have
*** the GTK+ development headers installed. The latest version of GTK+ is
*** always available at http://www.gtk.org/.]))
AC_SUBST(GTK_CFLAGS)

#AM_PATH_FESTIVAL(1.0.0, , AC_MSG_ERROR([
#*** Festival >= 1.0 is required to build Pidgin-Festival; please make sure you
#*** have the Festival development headers installed.  The latest version of Festival
#*** is always available at http://www.cstr.ed.ac.uk/projects/festival/.]))

#
# Check if plugins are enabled
#
want_plugins=yes
AM_CONDITIONAL(PLUGINS, test "$want_plugins" = "yes")

#
# Setup gettext
#
ALL_LINGUAS="`cat ./po/LINGUAS`"
AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE="pidgin-festival"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [gettext domain])
                                                                                                                           
#
# Finish up
#
AM_CONFIG_HEADER(pre_config.h)
AC_CONFIG_FILES([Makefile
		   src/Makefile
		   pidgin-festival.spec
		   po/Makefile.in
		  ])
AC_OUTPUT

echo;
echo Configuration complete
echo;
echo Using Pidgin source from.........: $with_pidgin
echo;
echo Debugging enabled..............: $enable_debug
echo Deprecated API enabled.........: $enable_deprecated
echo;
echo Type make to compile
echo;
