AC_INIT([Oakfoam], [0.0.8], [flash.slash@gmail.com], [oakfoam])
AM_INIT_AUTOMAKE([-Wall subdir-objects])

AM_CONFIG_HEADER([config.h])
CXXFLAGS="-Wall -g -O3 -Wno-strict-overflow"

AC_PROG_CXX
AC_PROG_CXX_C_O
AC_PROG_INSTALL

m4_include([m4/ax_boost_base.m4])
m4_include([m4/ax_boost_thread.m4])

AX_BOOST_BASE([1.33.1])
AX_BOOST_THREAD

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
