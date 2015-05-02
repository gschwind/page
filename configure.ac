
AC_INIT([page], [1.5.3])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/main.cxx])
AC_CONFIG_HEADERS([src/config.hxx])

# define custom macro to setup dir from datadir

# Check for programs
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

# check for C++11
AX_CXX_COMPILE_STDCXX_11

# Check for libs
PKG_CHECK_MODULES(X11, x11 >= 1.3.2)
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

# Check for libs
PKG_CHECK_MODULES(XCB, xcb >= 1.10)
AC_SUBST(XCB_CFLAGS)
AC_SUBST(XCB_LIBS)

PKG_CHECK_MODULES(X11XCB, x11-xcb >= 1.6)
AC_SUBST(X11XCB_CFLAGS)
AC_SUBST(X11XCB_LIBS)

PKG_CHECK_MODULES(XCBUTIL, xcb-util >= 0.3.0)
AC_SUBST(XCBUTIL_CFLAGS)
AC_SUBST(XCBUTIL_LIBS)

PKG_CHECK_MODULES(XCBFIXES, xcb-xfixes >= 1.0.0)
AC_SUBST(XCBFIXES_CFLAGS)
AC_SUBST(XCBFIXES_LIBS)

PKG_CHECK_MODULES(XCBDMG, xcb-damage >= 1.0.0)
AC_SUBST(XCBDMG_CFLAGS)
AC_SUBST(XCBDMG_LIBS)

PKG_CHECK_MODULES(XCBRR, xcb-randr >= 1.0.0)
AC_SUBST(XCBRR_CFLAGS)
AC_SUBST(XCBRR_LIBS)

PKG_CHECK_MODULES(XCBDMG, xcb-shape >= 1.0.0)
AC_SUBST(XCBSHP_CFLAGS)
AC_SUBST(XCBSHP_LIBS)

PKG_CHECK_MODULES(XCBCOMP, xcb-composite >= 1.0.0)
AC_SUBST(XCBCOMP_CFLAGS)
AC_SUBST(XCBCOMP_LIBS)

PKG_CHECK_MODULES(CAIRO, cairo >= 1.0.0)
AC_SUBST(CAIRO_CFLAGS)
AC_SUBST(CAIRO_LIBS)

PKG_CHECK_MODULES(XDAMAGE, xdamage >= 0.26)
AC_SUBST(XDAMAGE_CFLAGS)
AC_SUBST(XDAMAGE_LIBS)

PKG_CHECK_MODULES(XRENDER, xrender >= 0.9.6)
AC_SUBST(XRENDER_CFLAGS)
AC_SUBST(XRENDER_LIBS)

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

PKG_CHECK_MODULES(PANGO, pangocairo >= 0.28)
AC_SUBST(PANGO_CFLAGS)
AC_SUBST(PANGO_LIBS)


# This adds the option of compiling without using the ctemplate library,
# which has proved troublesome for compilation on some platforms
AC_ARG_ENABLE(pango,
  [ --disable-pango   Disable pango code path],
  [case "${enableval}" in
     yes | no ) WITH_PANGO="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-pango) ;;
   esac],
  [WITH_PANGO="yes"]
)

dnl Make sure we register this option with Automake, so we know whether to
dnl descend into ctemplate for more configuration or not
AM_CONDITIONAL([WITH_PANGO], [test "x$WITH_PANGO" = "xyes"])

# Define CTEMPLATE in config.h if we're going to compile against it
if test "x$WITH_PANGO" = "xyes"; then
    AC_DEFINE([WITH_PANGO], [], ["build using pango library"])
    AC_MSG_NOTICE([pango code path will be enabled])
else
    AC_MSG_NOTICE([pango code path will be enabled])
fi

# This adds the option of compiling without using the ctemplate library,
# which has proved troublesome for compilation on some platforms
AC_ARG_ENABLE(compositor,
  [ --disable-compositor   Disable compositor code path],
  [case "${enableval}" in
     yes | no ) WITH_COMPOSITOR="${enableval}" ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --disable-compositor) ;;
   esac],
  [WITH_COMPOSITOR="yes"]
)

dnl Make sure we register this option with Automake, so we know whether to
dnl descend into compositor for more configuration or not
AM_CONDITIONAL([WITH_COMPOSITOR], [test "x$WITH_COMPOSITOR" = "xyes"])

if test "x$WITH_COMPOSITOR" = "xyes"; then

	PKG_CHECK_MODULES(XCOMPOSITE, xcomposite >= 0.4.0)
	AC_SUBST(XCOMPOSITE_CFLAGS)
	AC_SUBST(XCOMPOSITE_LIBS)

    AC_DEFINE([WITH_COMPOSITOR], [], ["build compositor"])
    AC_MSG_NOTICE([compositor code path will be enabled])
else
    AC_MSG_NOTICE([compositor code path will be disabled])
fi

AC_CHECK_LIB(rt, clock_gettime,
    [RT_LIBS="-lrt"
     AC_DEFINE(HAVE_CLOCK_GETTIME, 1,
	       [Define to 1 if you have the `clock_gettime` function.])])
AC_SUBST(RT_LIBS)

AC_DEFINE_DIR([DATA_DIR], [datadir], [Data directory (/usr/share)])

AC_CONFIG_FILES([
  page.conf
  page.desktop
  Makefile
  src/Makefile
  ])
AC_OUTPUT
