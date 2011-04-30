
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
