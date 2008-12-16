dnl Process this file with autoconf to produce a configure script.

AC_INIT(dballe, 4.0.10, [enrico@enricozini.org])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_CONFIG_HEADER(config.h)

AC_ARG_ENABLE([dballe-bufrex],
    [AS_HELP_STRING(
        [--enable-dballe-bufrex],
        [build BUFR and CREX support])],
    [],
    [enable_dballe_bufrex="yes"])

AC_ARG_ENABLE([dballe-msg],
    [AS_HELP_STRING(
        [--enable-dballe-msg],
        [build AOF, BUFR and CREX sensemaking support])],
    [],
    [enable_dballe_msg="yes"])

AC_ARG_ENABLE([dballe-db],
    [AS_HELP_STRING(
        [--enable-dballe-db],
        [build DB-All.e database support])],
    [],
    [enable_dballe_db="yes"])

AC_ARG_ENABLE([dballepp],
    [AS_HELP_STRING(
        [--enable-dballepp],
        [build DB-All.e C++ bindings])],
    [],
    [enable_dballepp="yes"])

AC_ARG_ENABLE([dballef],
    [AS_HELP_STRING(
        [--enable-dballef],
        [build DB-All.e Fortran bindings])],
    [],
    [enable_dballef="yes"])

AC_ARG_ENABLE([dballe-python],
    [AS_HELP_STRING(
        [--enable-dballe-python],
        [build DB-All.e Python bindings])],
    [],
    [enable_dballe_python="yes"])

AC_ARG_ENABLE([docs],
    [AS_HELP_STRING(
        [--enable-docs],
        [enable building of the documentation])],
    [],
    [enable_docs="yes"])

AC_CHECK_PROG(have_latex2html, latex2html, yes, no)
AC_CHECK_PROG(have_rst2html, rst2html, yes, no)
AC_CHECK_PROG(have_swig, swig, yes, no)

dnl To use subdirs
AC_PROG_MAKE_SET

AC_LANG([C])

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CXX
AC_PROG_FC
dnl F77=$FC

AM_PROG_CC_STDC
AC_HEADER_STDC

dnl Needed by subdir-objects
AM_PROG_CC_C_O

dnl Use libtool
dnl AM_ENABLE_STATIC
dnl AM_DISABLE_SHARED
AM_PROG_LIBTOOL

AC_CHECK_PROG(have_gperf, gperf, true)
if test x$have_gperf != xtrue
then
	AC_MSG_ERROR([gperf was not found])
fi

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

if test x$have_swig = x"yes"
then
	AM_PATH_PYTHON
else
	enable_dballe_python=no
fi

AC_CHECK_HEADER(sql.h, AC_DEFINE(HAVE_ODBC_H, 1, sql.h (part of unixodbc) has been found),
        AC_MSG_WARN([*** sql.h not found. Check 'config.log' for more details.])
	enable_dballe_db=false
)

AC_CHECK_LIB(odbc, SQLAllocHandle, LIBS="$LIBS -lodbc",
        AC_MSG_WARN([*** libodbc not found. Check 'config.log' for more details.])
	enable_dballe_db=false
)

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

# Enforce dependencies
if ! test x"$enable_dballepp"=x"yes"
then
	enable_dballe_python=no
fi

if test x"$enable_dballe_bufrex"=x"yes"; then
	AC_DEFINE(HAVE_DBALLE_BUFREX,[],[Have BUFREX module])
fi
if test x"$enable_dballe_msg"=x"yes"; then
	AC_DEFINE(HAVE_DBALLE_MSG,[],[Have MSG module])
fi
if test x"$enable_dballe_db"=x"yes"; then
	AC_DEFINE(HAVE_DBALLE_DB,[],[Have DB module])
fi
if test x"$enable_dballepp"=x"yes"; then
	AC_DEFINE(HAVE_DBALLEPP,[],[Have C++ bindings])
fi
if test x"$enable_dballef"=x"yes"; then
	AC_DEFINE(HAVE_DBALLEF,[],[Have Fortran bindings])
fi
if test x"$enable_dballe_python"=x"yes"; then
	AC_DEFINE(HAVE_DBALLE_PYTHON,[],[Have Python bindings])
fi

AM_CONDITIONAL([DO_DBALLE_BUFREX], [test x"$enable_dballe_bufrex" = x"yes"])
AM_CONDITIONAL([DO_DBALLE_MSG], [test x"$enable_dballe_msg" = x"yes"])
AM_CONDITIONAL([DO_DBALLE_DB], [test x"$enable_dballe_db" = x"yes"])
AM_CONDITIONAL([DO_DBALLEPP], [test x"$enable_dballepp" = x"yes"])
AM_CONDITIONAL([DO_DBALLEF], [test x"$enable_dballef" = x"yes"])
AM_CONDITIONAL([DO_DBALLE_PYTHON], [test x"$enable_dballe_python" = x"yes"])
AM_CONDITIONAL([DO_DOCS], [test x"$enable_docs" = x"yes"])

AM_CONDITIONAL([HAVE_LATEX2HTML], [test x$have_latex2html = xyes])
AM_CONDITIONAL([HAVE_RST2HTML], [test x$have_rst2html = xyes])

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
doc/libdballe.dox
doc/libdballepp.dox
provami/Makefile
libdballe.pc
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
AS_HELP_STRING([bufrex:], [$enable_dballe_bufrex])
AS_HELP_STRING([msg:], [$enable_dballe_msg])
AS_HELP_STRING([db:], [$enable_dballe_db])
AS_HELP_STRING([C++:], [$enable_dballepp])
AS_HELP_STRING([Fortran:], [$enable_dballef])
AS_HELP_STRING([Python:], [$enable_dballe_python])
AS_HELP_STRING([documentation:], [$enable_docs])
AS_HELP_STRING([latex2html:], [$have_latex2html])
AS_HELP_STRING([rst2html:], [$have_rst2html])
===================================================])
