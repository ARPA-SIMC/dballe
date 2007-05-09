dnl Process this file with autoconf to produce a configure script.

AC_INIT(dballe, 3.4, [enrico@enricozini.org])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_CONFIG_HEADER(config.h)

AC_ARG_ENABLE([db-tests],
    [AS_HELP_STRING(
        [--enable-db-tests],
        [enable the dba_db tests that need an ODBC DSN previously set up])],
    [],
    [enable_db_tests="no"])

AC_ARG_ENABLE(fortran,
		[  --enable-fortran    Turn on generation of fortran bindings using CNF],
		[case "${enableval}" in
			yes) fortran=true ;;
			no)  fortran=false ;;
			*) AC_MSG_ERROR(bad value ${enableval} for --enable-fortran) ;;
		esac],[fortran=true])

AC_ARG_ENABLE(docs,
		[  --enable-docs    Turn on generation of documentation],
		[case "${enableval}" in
			yes) docs=true ;;
			no)  docs=false ;;
			*) AC_MSG_ERROR(bad value ${enableval} for --enable-docs) ;;
		esac],[docs=true])

AC_ARG_ENABLE(swig,
		[  --enable-swig    Turn on generation of swig bindings using CNF],
		[case "${enableval}" in
			yes) swig=true ;;
			no)  swig=false ;;
			*) AC_MSG_ERROR(bad value ${enableval} for --enable-swig) ;;
		esac],[swig=true])
AM_CONDITIONAL(DO_SWIG, test x$swig = xtrue)

AC_CHECK_PROG(have_latex2html, latex2html, yes, no)

dnl To use subdirs
AC_PROG_MAKE_SET

AC_LANG([C])

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CXX
AC_PROG_FC
F77=$FC

AM_PROG_CC_STDC
AC_HEADER_STDC

dnl Needed by subdir-objects
AM_PROG_CC_C_O

dnl Use libtool
dnl AM_ENABLE_STATIC
dnl AM_DISABLE_SHARED
AM_PROG_LIBTOOL

dnl Check if std::vector has a std::vector::data method
AC_CACHE_CHECK(whether the compiler has std::vector::data (),
	ac_cv_cxx_have_vector_data,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <vector>
using namespace std;
],[vector<int> v; v.data(); return 0;],
 ac_cv_cxx_have_vector_data=yes, ac_cv_cxx_have_vector_data=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_have_vector_data" = yes; then
 AC_DEFINE(HAVE_VECTOR_DATA,,[define if the compiler has the method
std::vector::data ()])
fi


if test x$docdir = x
then
	docdir='${datadir}/doc/${PACKAGE_TARNAME}'
	AC_SUBST(docdir)
fi

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

AM_CONDITIONAL([DO_FORTRAN], [test x$fortran = xtrue])
AM_CONDITIONAL([DO_DOCS], [test x$docs = xtrue])
AM_CONDITIONAL([HAVE_LATEX2HTML], [test x$have_latex2html = xyes])
AM_CONDITIONAL([DB_TESTS], [test x"$enable_db_tests" = x"yes"])

AC_CONFIG_FILES([
Makefile
dballe/Makefile
dballe/core/Makefile
dballe/bufrex/Makefile
dballe/msg/Makefile
dballe/db/Makefile
dballe/bench/Makefile
dballe++/Makefile
fortran/Makefile
tools/Makefile
tables/Makefile
extra/Makefile
swig/Makefile
doc/Makefile
doc/libdballe-core.dox
doc/libdballe-bufrex.dox
doc/libdballe-msg.dox
doc/libdballe-db.dox
doc/libdballepp.dox
provami/Makefile
libdballe-core.pc
libdballe-bufrex.pc
libdballe-msg.pc
libdballe-db.pc
libdballef.pc
libdballepp.pc
])
AC_OUTPUT
#examples/Makefile

dnl tests/Makefile
dnl doc/Makefile

AC_MSG_NOTICE([
===================================================
$PACKAGE_NAME-$PACKAGE_VERSION configuration:
AS_HELP_STRING([documentation:], [$docs])
AS_HELP_STRING([latex2html:], [$have_latex2html])
AS_HELP_STRING([db_tests:], [$enable_db_tests])
===================================================])

