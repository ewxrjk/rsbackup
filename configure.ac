# Copyright © 2011-17 Richard Kettlewell.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
AC_PREREQ([2.61])
AC_INIT([rsbackup], [4.0], [rjk@greenend.org.uk])
AC_CONFIG_AUX_DIR([config.aux])
AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_SRCDIR([src/Conf.cc])
AM_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST
AC_LANG([C++])
AC_PROG_CXX
AM_PROG_AR
AC_PROG_RANLIB
AC_SET_MAKE
AC_C_BIGENDIAN
case "$host_os" in
*bsd* )
  CPPFLAGS="${CPPFLAGS} -isystem /usr/local/include"
  LDFLAGS="${LDFLAGS} -L/usr/local/lib"
  ;;
linux-gnu )
  # For backtrace to work properly
  LDFLAGS="${LDFLAGS} -rdynamic"
  ;;
esac
AC_CHECK_HEADERS([paths.h execinfo.h])
case "$host" in
  *apple-darwin* )
    # Use system sqlite3
    AC_SUBST([SQLITE3_CFLAGS],[])
    AC_SUBST([SQLITE3_LIBS],[-lsqlite3])
    ;;
  * )
    PKG_CHECK_MODULES([SQLITE3],[sqlite3])
    ;;
esac
boost="-lboost_filesystem -lboost_system"
AC_SUBST([BOOST_LIBS],[${boost}])
AC_CHECK_LIB([iconv],[iconv_open])
AC_CHECK_LIB([rt],[clock_gettime])
AC_CHECK_LIB([execinfo],[backtrace])
AC_CACHE_CHECK([type of second argument to iconv()], [rjk_cv_iconv_inptr],[
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([#include <iconv.h>],[
      iconv_t cd = 0;
      char *in, *out;
      size_t n;
      iconv(cd, &in, &n, &out, &n);
    ])],
    [rjk_cv_iconv_inptr="char **"],
    [AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([#include <iconv.h>],[
        iconv_t cd = 0;
        const char *in;
        char *out;
        size_t n;
        iconv(cd, &in, &n, &out, &n);
      ])],
      [rjk_cv_iconv_inptr="const char **"],
      [rjk_cv_iconv_inptr="unknown"])])
])
case "$rjk_cv_iconv_inptr" in
unknown )
  AC_MSG_WARN([cannot determine argument type, winging it])
  AC_DEFINE([ICONV_FIXUP],[],[fixup string for second argument to iconv()])
  ;;
"char **" )
  AC_DEFINE([ICONV_FIXUP],[],[fixup string for second argument to iconv()])
  ;;
"const char **" )
  AC_DEFINE_UNQUOTED([ICONV_FIXUP],[(${rjk_cv_iconv_inptr})],[fixup string for second argument to iconv()])
  ;;
esac
AC_CHECK_LIB([pthread], [pthread_create],
             [AC_SUBST(LIBPTHREAD,[-lpthread])],
	     [missing_libraries="$missing_libraries libpthread"])
AC_CACHE_CHECK([for Cairomm CFLAGS],[rjk_cv_cairomm_cflags],[
  rjk_cv_cairomm_cflags=`pkg-config --silence-errors --cflags cairomm-1.0`
])
AC_CACHE_CHECK([for Cairomm LIBS],[rjk_cv_cairomm_libs],[
  rjk_cv_cairomm_libs=`pkg-config --silence-errors --libs cairomm-1.0`
])
AC_SUBST([CAIROMM_CFLAGS],[$rjk_cv_cairomm_cflags])
AC_SUBST([CAIROMM_LIBS],[$rjk_cv_cairomm_libs])
AC_CACHE_CHECK([for Pangomm CFLAGS],[rjk_cv_pangomm_cflags],[
  rjk_cv_pangomm_cflags=`pkg-config --silence-errors --cflags pangomm-1.4`
])
AC_CACHE_CHECK([for Pangomm LIBS],[rjk_cv_pangomm_libs],[
  rjk_cv_pangomm_libs=`pkg-config --silence-errors --libs pangomm-1.4`
])
AC_SUBST([PANGOMM_CFLAGS],[$rjk_cv_pangomm_cflags])
AC_SUBST([PANGOMM_LIBS],[$rjk_cv_pangomm_libs])
AC_DEFINE([_GNU_SOURCE], [1], [use GNU extensions])
RJK_GCOV
AC_ARG_WITH([lyx],
            [AS_HELP_STRING([--with-lyx],
                            [Use LyX to compile PDF])],
            [],
            [with_lyx=check])
AS_IF([test "x$with_lyx" = xcheck],
  [AS_IF([type lyx >/dev/null 2>&1],[with_lyx=yes])])
AS_IF([test "x$with_lyx" != xno],[LYX_TARGETS=pdf])
AC_SUBST([LYX_TARGETS])

if test "x$GXX" = xyes; then
  CXXFLAGS="-std=gnu++11 -Wall -W -Werror -Wpointer-arith -Wwrite-strings $CXXFLAGS"
  CXXFLAGS="${CXXFLAGS} -Wno-c++14-extensions"
fi
CXXFLAGS="${CXXFLAGS} ${CXXFLAGS_EXTRA}"

AH_BOTTOM([#ifdef __GNUC__
# define attribute(x) __attribute__(x)
#else
# define attribute(x)
#endif])

AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile doc/Makefile tools/Makefile])
AC_OUTPUT
