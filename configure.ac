# Copyright © 2011, 2012 Richard Kettlewell.
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
AC_INIT([rsbackup], [0.4.1], [rjk@greenend.org.uk])
AC_CONFIG_AUX_DIR([config.aux])
AM_INIT_AUTOMAKE([1.10])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])
AC_CONFIG_SRCDIR([src/Conf.cc])
AM_CONFIG_HEADER([config.h])
AC_LANG([C++])
AC_PROG_CXX
AC_PROG_RANLIB
AC_SET_MAKE
AC_C_BIGENDIAN
case `uname -s` in
FreeBSD )
  CPPFLAGS="${CPPFLAGS} -isystem /usr/local/include"
  LDFLAGS="${LDFLAGS} -L/usr/local/lib"
  ;;
esac  
AC_CHECK_HEADERS([paths.h])
AC_CHECK_LIB([iconv],[iconv_open])
AC_CHECK_LIB([rt],[clock_gettime])
AC_CACHE_CHECK([type of second argument to iconv()], [rjk_cv_iconv_inptr],[
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([#include <iconv.h>],[
      iconv_t cd;
      char *in, *out;
      size_t n;
      iconv(cd, &in, &n, &out, &n);
    ])],
    [rjk_cv_iconv_inptr="char **"],
    [AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([#include <iconv.h>],[
        iconv_t cd;
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
AC_DEFINE([_GNU_SOURCE], [1], [use GNU extensions])
RJK_GCOV
if test "x$GXX" = xyes; then
  CXX="$CXX -Wall -W -Werror -Wpointer-arith -Wwrite-strings"
fi
AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile doc/Makefile tools/Makefile])
AC_OUTPUT
