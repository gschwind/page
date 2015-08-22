
AC_INIT([page], [1.9.3])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/main.cxx])
AC_CONFIG_HEADERS([src/config.hxx])

dnl define custom macro to setup dir from datadir

dnl Check for required programs
AC_PROG_CXX
AC_PROG_LIBTOOL

# check for C++11
AX_CXX_COMPILE_STDCXX_11

dnl Check for libs
PKG_CHECK_MODULES(X11, x11 >= 1.3.2)
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

dnl Check for libs
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

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.0.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(PANGO, pangocairo >= 0.28)
AC_SUBST(PANGO_CFLAGS)
AC_SUBST(PANGO_LIBS)


dnl This adds the option of compiling without using the ctemplate library,
dnl which has proved troublesome for compilation on some platforms
dnl AC_ARG_ENABLE(pango,
dnl   [ --disable-pango   Disable pango code path],
dnl   [case "${enableval}" in
dnl      yes | no ) WITH_PANGO="${enableval}" ;;
dnl      *) AC_MSG_ERROR(bad value ${enableval} for --disable-pango) ;;
dnl    esac],
dnl   [WITH_PANGO="yes"]
dnl )
dnl 
dnl Make sure we register this option with Automake, so we know whether to
dnl descend into ctemplate for more configuration or not
dnl AM_CONDITIONAL([WITH_PANGO], [test "x$WITH_PANGO" = "xyes"])
dnl 
dnl # Define CTEMPLATE in config.h if we're going to compile against it
dnl if test "x$WITH_PANGO" = "xyes"; then
dnl     AC_DEFINE([WITH_PANGO], [], ["build using pango library"])
dnl     AC_MSG_NOTICE([pango code path will be enabled])
dnl else
dnl     AC_MSG_NOTICE([pango code path will be enabled])
dnl fi
dnl 
dnl # This adds the option of compiling without using the ctemplate library,
dnl # which has proved troublesome for compilation on some platforms
dnl AC_ARG_ENABLE(compositor,
dnl   [ --disable-compositor   Disable compositor code path],
dnl   [case "${enableval}" in
dnl      yes | no ) WITH_COMPOSITOR="${enableval}" ;;
dnl      *) AC_MSG_ERROR(bad value ${enableval} for --disable-compositor) ;;
dnl    esac],
dnl   [WITH_COMPOSITOR="yes"]
dnl )
dnl 
dnl Make sure we register this option with Automake, so we know whether to
dnl descend into compositor for more configuration or not
dnl AM_CONDITIONAL([WITH_COMPOSITOR], [test "x$WITH_COMPOSITOR" = "xyes"])
dnl 
dnl if test "x$WITH_COMPOSITOR" = "xyes"; then
dnl 
dnl 	PKG_CHECK_MODULES(XCOMPOSITE, xcomposite >= 0.4.0)
dnl 	AC_SUBST(XCOMPOSITE_CFLAGS)
dnl 	AC_SUBST(XCOMPOSITE_LIBS)
dnl 
dnl     AC_DEFINE([WITH_COMPOSITOR], [], ["build compositor"])
dnl     AC_MSG_NOTICE([compositor code path will be enabled])
dnl else
dnl     AC_MSG_NOTICE([compositor code path will be disabled])
dnl fi

AC_CHECK_LIB(rt, clock_gettime,
    [RT_LIBS="-lrt"
     AC_DEFINE(HAVE_CLOCK_GETTIME, 1,
	       [Define to 1 if you have the `clock_gettime` function.])])
AC_SUBST(RT_LIBS)

AC_DEFINE_DIR([DATA_DIR], [datadir], [Data directory (/usr/share)])

safe_CXXFLAGS="${CXXFLAGS}"
safe_LDFLAGS="${LDFLAGS}"
CXXFLAGS="${CXXFLAGS} -flto"
LDFLAGS="${LDFLAGS} -flto"
AC_TRY_LINK([], [int main () { return 0; }],[
dnl keep FLAGS
],[
CXXFLAGS="${safe_CXXFLAGS}"
LDFLAGS="${safe_LDFLAGS}"
])

unset safe_CXXFLAGS
unset safe_LDFLAGS

AC_CONFIG_FILES([
  page.conf
  page.desktop
  Makefile
  src/Makefile
  ])
AC_OUTPUT
