AC_INIT([page], [0.9.5])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/main.cxx])
#AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_ARG_ENABLE([trace], [AS_HELP_STRING([--enable-trace],[enable trace feature with gcc])], [CXXFLAGS="-g -finstrument-functions -finstrument-functions-exclude-file-list=x86_64-pc-linux-gnu,X11 -DENABLE_TRACE $CXXFLAGS"], [])

# Check for programs
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

AM_PROG_CC_C_O

# Check for libs
PKG_CHECK_MODULES(X11, x11 >= 1.3.2)
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

PKG_CHECK_MODULES(CAIRO, cairo >= 1.0.0)
AC_SUBST(CAIRO_CFLAGS)
AC_SUBST(CAIRO_LIBS)

PKG_CHECK_MODULES(XCOMPOSITE, xcomposite >= 0.4.0)
AC_SUBST(XCOMPOSITE_CFLAGS)
AC_SUBST(XCOMPOSITE_LIBS)

PKG_CHECK_MODULES(XDAMAGE, xdamage >= 0.26)
AC_SUBST(XDAMAGE_CFLAGS)
AC_SUBST(XDAMAGE_LIBS)

PKG_CHECK_MODULES(XRENDER, xrender >= 0.9.6)
AC_SUBST(XRENDER_CFLAGS)
AC_SUBST(XRENDER_LIBS)

PKG_CHECK_MODULES(XINERAMA, xinerama >= 1.1.1)
AC_SUBST(XINERAMA_CFLAGS)
AC_SUBST(XINERAMA_LIBS)

PKG_CHECK_MODULES(XRANDR, xrandr >= 1.2.0)
AC_SUBST(XRANDR_CFLAGS)
AC_SUBST(XRANDR_LIBS)

PKG_CHECK_MODULES(XEXT, xext >= 0.27)
AC_SUBST(XEXT_CFLAGS)
AC_SUBST(XEXT_LIBS)

PKG_CHECK_MODULES(FT2, freetype2 >= 2.0.0)
AC_SUBST(FT2_CFLAGS)
AC_SUBST(FT2_LIBS)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.0.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

AC_CHECK_HEADER([bfd.h],
                [AC_DEFINE([HAVE_BFD_H], [1], [BFD Headers])],
                [AC_MSG_ERROR([Header "bfd.h" from the binutils not found!])])

AC_SEARCH_LIBS([bfd_openr],  [bfd],
               [AC_MSG_RESULT([OK, found libbfd])],
               [AC_MSG_RESULT([no found libbfd])])

AC_CHECK_LIB(rt, clock_gettime,
    [RT_LIBS="-lrt"
     AC_DEFINE(HAVE_CLOCK_GETTIME, 1,
	       [Define to 1 if you have the `clock_gettime` function.])])
AC_SUBST(RT_LIBS)

AC_CONFIG_FILES([
  page.conf
  Makefile
  src/Makefile
  ])
AC_OUTPUT
