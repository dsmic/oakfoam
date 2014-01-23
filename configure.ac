AC_INIT([Oakfoam], [0.2.0], [oakfoam@gmail.com], [oakfoam])
AM_INIT_AUTOMAKE([-Wall subdir-objects])

AM_CONFIG_HEADER([config.h])
CXXFLAGS="-Wall -g -O3 -DTOPSRCDIR=\\\"`pwd`\\\" -fno-strict-aliasing"

AC_PROG_CXX
AC_PROG_CXX_C_O
AC_PROG_INSTALL
AC_LANG(C++)

m4_include([m4/ax_boost_base.m4])
m4_include([m4/ax_boost_system.m4])
m4_include([m4/ax_boost_thread.m4])
m4_include([m4/ax_boost_filesystem.m4])
m4_include([m4/ax_mpi.m4])

AC_ARG_WITH(hoard, [AS_HELP_STRING([--with-hoard=<srcdir>],
   [Compile using the Hoard memory allocator. Default: no])
],,[with_hoard=no])

AC_ARG_WITH(web, [AS_HELP_STRING([--with-web],
   [Compile with web interface support. Default: no])
],,[with_web=no])

if test x"$with_hoard" != xno; then
  LDFLAGS_BAK="$LDFLAGS"
  LIBS_BAK="$LIBS"
  if test x"$with_hoard" = xyes; then
    HOARD_LDFLAGS=""
  else
    HOARD_LDFLAGS="-L$with_hoard"
  fi
  HOARD_LIB="-lhoard -ldl"
  LDFLAGS="$LDFLAGS $HOARD_LDFLAGS"
  LIBS="$LIBS $HOARD_LIB"
  AC_MSG_CHECKING(whether Hoard memory allocator is available)
  AC_LINK_IFELSE([AC_LANG_PROGRAM()],
    AC_MSG_RESULT(yes)
    AC_SUBST(HOARD_LDFLAGS)
    AC_SUBST(HOARD_LIB)
  ,
    AC_MSG_RESULT(no)
    AC_MSG_FAILURE([Hoard couldn't be found!]) #'
  )
  LDFLAGS="$LDFLAGS_BAK"
  LIBS="$LIBS_BAK"
fi

if test x"$with_web" = xyes; then
  AX_BOOST_BASE([1.35.0])
  AC_DEFINE(HAVE_WEB,1,[Define if you have web interface support.])
else
  AX_BOOST_BASE([1.33.1])
fi
AX_BOOST_SYSTEM
AX_BOOST_THREAD
AX_BOOST_FILESYSTEM
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[[
  #ifndef HAVE_BOOST_THREAD
    error
  #endif
]])],,AC_MSG_FAILURE([Boost::Thread library is required!]))

AC_ARG_WITH(mpi, [AS_HELP_STRING([--with-mpi],
   [Compile with MPI (cluster parallelization) support. If none is found,
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
