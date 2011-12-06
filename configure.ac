AC_INIT([Oakfoam], [0.1.1-dev], [flash.slash@gmail.com], [oakfoam])
AM_INIT_AUTOMAKE([-Wall subdir-objects])

AM_CONFIG_HEADER([config.h])
CXXFLAGS="-Wall -g -O3 -DTOPSRCDIR=\\\"`pwd`\\\""

AC_PROG_CXX
AC_PROG_CXX_C_O
AC_PROG_INSTALL
AC_LANG(C++)

m4_include([m4/ax_boost_base.m4])
m4_include([m4/ax_boost_thread.m4])
m4_include([m4/ax_mpi.m4])

AX_BOOST_BASE([1.33.1])
AX_BOOST_THREAD
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[[
  #ifndef HAVE_BOOST_THREAD
    error
  #endif
]])],,AC_MSG_ERROR([Boost::Thread library is required!]))

AC_ARG_WITH(mpi, [AS_HELP_STRING([--with-mpi],
   [compile with MPI (cluster parallelization) support. If none is found,
   MPI is not used. Default: no])
],,[with_mpi=no])

if test x"$with_mpi" = xyes; then
  AX_MPI([
    AC_DEFINE(HAVE_MPI,1,[Define if you have the MPI library.])
    MPI_CPPFLAGS=`$MPICXX -showme:compile`
    MPI_LDFLAGS=`$MPICXX -showme:link`
    AC_SUBST(MPI_CPPFLAGS)
    AC_SUBST(MPI_LDFLAGS)
  ],[
    AC_MSG_FAILURE([MPI compiler requested, but couldn't use MPI.])
  ])
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
