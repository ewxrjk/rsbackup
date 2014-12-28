# Copyright © 2011 Richard Kettlewell.
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
AC_DEFUN([RJK_GCOV],[
  GCOV=${GCOV:-true}
  AC_ARG_WITH([gcov],
              [AS_HELP_STRING([--with-gcov],
                              [Enable coverage testing])],
              [[if test $withval = yes; then
                 CXXFLAGS="${CXXFLAGS} -O0 -fprofile-arcs -ftest-coverage"
                 GCOV=`echo $CXX | sed s'/[cg]++/gcov/;s/ .*$//'`;
               fi]])
  AC_SUBST([GCOV],[$GCOV])
])
