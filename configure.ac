dnl Process this file with autoconf to produce a configure script.

AC_INIT(dballe, 0.7, [enrico@enricozini.org])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_CONFIG_HEADER(config.h)

AC_ARG_ENABLE(mysql4,
		[  --enable-mysql4     Use features from MySQL 4],
		[case "${enableval}" in
			yes) mysql4=true ;;
			no)  mysql4=false ;;
			*) AC_MSG_ERROR(bad value ${enableval} for --enable-mysql4) ;;
		esac],[mysql4=true])
if test x$mysql4 = xtrue
then
	AC_DEFINE(USE_MYSQL4, 1, "Use features from MySQL 4")
fi

AC_ARG_ENABLE(fortran,
		[  --enable-fortran    Turn on generation of fortran bindings using CNF],
		[case "${enableval}" in
			yes) fortran=true ;;
			no)  fortran=false ;;
			*) AC_MSG_ERROR(bad value ${enableval} for --enable-fortran) ;;
		esac],[fortran=true])
AM_CONDITIONAL(DO_FORTRAN, test x$fortran = xtrue)

AC_ARG_ENABLE(docs,
		[  --enable-docs    Turn on generation of documentation],
		[case "${enableval}" in
			yes) docs=true ;;
			no)  docs=false ;;
			*) AC_MSG_ERROR(bad value ${enableval} for --enable-docs) ;;
		esac],[docs=true])
AM_CONDITIONAL(DO_DOCS, test x$docs = xtrue)

AC_ARG_ENABLE(swig,
		[  --enable-swig    Turn on generation of swig bindings],
		[case "${enableval}" in
			yes) swig=true ;;
			no)  swig=false ;;
			*) AC_MSG_ERROR(bad value ${enableval} for --enable-swig) ;;
		esac],[swig=false])
AM_CONDITIONAL(DO_SWIG, test x$swig = xtrue)

dnl To use subdirs
AC_PROG_MAKE_SET

AC_LANG([C])

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CXX
AC_PROG_F77
AM_PROG_CC_STDC
AC_HEADER_STDC

dnl Needed by subdir-objects
AM_PROG_CC_C_O

dnl Use libtool
dnl AM_ENABLE_STATIC
dnl AM_DISABLE_SHARED
AM_PROG_LIBTOOL

if test x$swig = x"true"
then
	AM_PATH_PYTHON
fi

AC_CHECK_HEADER(sql.h, AC_DEFINE(HAVE_ODBC_H, 1, sql.h (part of unixodbc) has been found),
       AC_MSG_ERROR([
*** sql.h not found. Check 'config.log' for more details.]))

AC_CHECK_LIB(odbc, SQLAllocHandle, LIBS="$LIBS -lodbc",
       AC_MSG_ERROR([
*** libodbc not found. Check 'config.log' for more details.]))

LIBS="$LIBS -lm"

confdir='$(sysconfdir)'"/$PACKAGE"
AC_SUBST(confdir)

tabledir='$(pkgdatadir)'
AC_SUBST(tabledir)
dnl def_tabledir=`echo "${sharedstatedir}/lib/$PACKAGE/tables"`
dnl AC_DEFINE_UNQUOTED(TABLE_DIR, "$def_tabledir", "Default location for table files")

LIBDBALLE_VERSION_INFO=`echo $VERSION | awk -F. '{ printf "%d:%d:%d", $1+$2, $3, $2 }'`
AC_SUBST(LIBDBALLE_VERSION_INFO)

CFLAGS="-Wall $CFLAGS"

AC_CONFIG_FILES([
Makefile
dballe/Makefile
dballe/err/Makefile
dballe/conv/Makefile
dballe/core/Makefile
dballe/io/Makefile
dballe/msg/Makefile
dballe/bufrex/Makefile
dballe/aof/Makefile
dballe/db/Makefile
fortran/Makefile
tools/Makefile
tables/Makefile
swig/Makefile
doc/Makefile
doc/libdballe.dox
tests/Makefile
libdballe.pc
])
AC_OUTPUT
#examples/Makefile

dnl tests/Makefile
dnl doc/Makefile

