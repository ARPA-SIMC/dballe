dnl Process this file with autoconf to produce a configure script.

AC_INIT(dballe, [7.7], [enrico@enricozini.org])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

dnl Use libtool
dnl AM_ENABLE_STATIC
dnl AM_DISABLE_SHARED
LT_INIT()
LT_LANG([C++])
AM_PROG_LIBTOOL

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

dnl To use subdirs
AC_PROG_MAKE_SET

AC_LANG([C++])

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CXX
AC_PROG_FC([gfortran pgf90 ifort g95 xlf90 f90])
F77=$FC

dnl Use c++11
AX_CXX_COMPILE_STDCXX_11

AM_PROG_CC_STDC

dnl Needed by subdir-objects
AM_PROG_CC_C_O

dnl We want to invoke pkg-config directly
PKG_PROG_PKG_CONFIG

if test x$enable_docs = xyes
then
	dnl Check for doxygen
	AC_CHECK_PROG(have_doxygen, doxygen, true)
	if test x$have_doxygen != xtrue
	then
		AC_MSG_WARN([doxygen was not found: documentation will not be built])
		enable_docs=no
	fi
fi

dnl Check for gperf
AC_CHECK_PROG(have_gperf, gperf, true)
if test x$have_gperf != xtrue
then
	AC_MSG_WARN([gperf was not found: relying on prebuilt sources])
	has_build_gperf_sources=yes
	for f in dballe/core/record_keyword.cc dballe/core/aliases.cc dballe/msg/vars.cc
	do
		if ! test -f $f
		then
			has_build_gperf_sources=no
			AC_MSG_WARN([please extract $f from the distribution tarball])
		fi
	done
	if test $has_build_gperf_sources = no
	then
		AC_MSG_ERROR([some gperf-generated source files are missing and gperf is not available to rebuild them])
	fi
fi

dnl Check for missing functions in libc
AC_CHECK_FUNC(vasprintf, , [
	AC_DEFINE([USE_OWN_VASPRINTF], 1, [we need to use our own vasprintf])
])
AC_CHECK_FUNC(bswap_32, , [
	AC_DEFINE([USE_OWN_BSWAP], 1, [we need to use our own bswap_* functions])
])
AC_CHECK_FUNC(strcasestr, [
	AC_DEFINE([HAVE_STRCASESTR], 1, [we can use strcasestr])
])

if test x$docdir = x
then
	docdir='${datadir}/doc/${PACKAGE_TARNAME}'
	AC_SUBST(docdir)
fi

dnl Check for python
AM_PATH_PYTHON([], [], [
	enable_dballe_python=no
])
if test x$enable_dballe_python = xyes
then
    PYTHON_CFLAGS=`unset CFLAGS; $PYTHON-config --cflags`
    AC_SUBST(PYTHON_CFLAGS)
fi


dnl Check for wreport
PKG_CHECK_MODULES(WREPORT,libwreport >= 3.0)

dnl Check for sqlite3
PKG_CHECK_MODULES(SQLITE3, sqlite3)


dnl Check for libpq
PKG_CHECK_EXISTS(libpq, [have_libpq=yes], [have_libpq=no])
if test x$have_libpq = xyes
then
    PKG_CHECK_MODULES(LIBPQ, libpq)
    AC_DEFINE([HAVE_LIBPQ], 1, [PostgreSQL is available])
fi
AM_CONDITIONAL([HAVE_LIBPQ], [test x"$have_libpq" = x"yes"])


dnl Check for libmysqlclient
AC_PATH_PROG(mysql_config, [mysql_config])
if test x$mysql_config = x
then
    have_mysql=no
else
    have_mysql=yes
    MYSQL_CFLAGS=`$mysql_config --cflags`
    MYSQL_LIBS=`$mysql_config --libs_r`
    AC_SUBST(MYSQL_CFLAGS)
    AC_SUBST(MYSQL_LIBS)
    AC_DEFINE([HAVE_MYSQL], 1, [MySQL is available])
fi
AM_CONDITIONAL([HAVE_MYSQL], [test x"$have_mysql" = x"yes"])


dnl Check for unixodbc
have_odbc=yes
AC_CHECK_HEADER(sql.h, AC_DEFINE(HAVE_ODBC_H, 1, sql.h (part of unixodbc) has been found),
        AC_MSG_WARN([*** sql.h not found. Check 'config.log' for more details.])
        have_odbc=no
)
AC_CHECK_LIB(odbc, SQLAllocHandle, LIBS="$LIBS -lodbc",
        AC_MSG_WARN([*** libodbc not found. Check 'config.log' for more details.])
        have_odbc=no
)


dnl Check for lua
PKG_CHECK_EXISTS([lua5.2], [have_lua=yes], [have_lua=no])
if test x$have_lua = xyes
then
        PKG_CHECK_MODULES(LUA,lua5.2,,[have_lua=no])
else
	PKG_CHECK_EXISTS([lua5.1], [have_lua=yes], [have_lua=no])
	if test x$have_lua = xyes
	then
		PKG_CHECK_MODULES(LUA,lua5.1,,[have_lua=no])
	else
		PKG_CHECK_EXISTS([lua], [have_lua=yes], [have_lua=no])
		if test x$have_lua = xyes
		then
			PKG_CHECK_MODULES(LUA,lua,,[have_lua=no])
		else
			dnl We don't always have the luxury of .pc files for lua, it seems
			have_lua=yes
			AC_CHECK_LIB([lua], [lua_gettop], [true], [have_lua=no])
			AC_CHECK_HEADER([lua.h], [true], [have_lua=no])
			if test x$have_lua = xyes
			then
				LUA_CFLAGS=
				LUA_LIBS="-llua"
			fi
		fi
	fi
fi
if test x$have_lua = xyes
then
        AC_DEFINE([HAVE_LUA], 1, [lua is available])
fi
AM_CONDITIONAL([LUA], [test x"$have_lua" = x"yes"])

dnl Check for popt
AC_CHECK_HEADER(popt.h, AC_DEFINE(HAVE_POPT_H, 1, popt.h has been found),
        AC_MSG_ERROR([*** popt.h not found. Check 'config.log' for more details.])
)
AC_CHECK_LIB(popt, poptGetContext, LIBS="$LIBS -lpopt",
        AC_MSG_ERROR([*** libpopt not found. Check 'config.log' for more details.])
)

LIBS="$LIBS -lm"

confdir='${sysconfdir}'"/$PACKAGE"
AC_SUBST(confdir)

# tabledir="`$PKG_CONFIG --variable=tabledir libwreport`"
tabledir='${datadir}/wreport'
AC_SUBST(tabledir)
dnl def_tabledir=`echo "${sharedstatedir}/lib/$PACKAGE/tables"`
dnl AC_DEFINE_UNQUOTED(TABLE_DIR, "$def_tabledir", "Default location for table files")

dnl From libtool's documentation:
dnl
dnl This flag accepts an argument of the form `CURRENT[:REVISION[:AGE]]'.
dnl
dnl CURRENT
dnl      The most recent interface number that this library implements.
dnl 
dnl REVISION
dnl      The implementation number of the CURRENT interface.
dnl 
dnl AGE
dnl      The difference between the newest and oldest interfaces that this
dnl      library implements.  In other words, the library implements all the
dnl      interface numbers in the range from number `CURRENT - AGE' to
dnl      `CURRENT'.
dnl
dnl If two libraries have identical CURRENT and AGE numbers, then the
dnl dynamic linker chooses the library with the greater REVISION number.
dnl
dnl  3. If the library source code has changed at all since the last
dnl     update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
dnl
dnl  4. If any interfaces have been added, removed, or changed since the
dnl     last update, increment CURRENT, and set REVISION to 0.
dnl
dnl  5. If any interfaces have been added since the last public release,
dnl     then increment AGE.
dnl
dnl  6. If any interfaces have been removed since the last public release,
dnl     then set AGE to 0.

LIBDBALLE_VERSION_INFO="7:3:0"
LIBDBALLEF_VERSION_INFO="4:20:0"
AC_SUBST(LIBDBALLE_VERSION_INFO)
AC_SUBST(LIBDBALLEF_VERSION_INFO)

dnl Enable extra compiler warnings
AX_CXXFLAGS_WARN_ALL
dnl warnings="-Wall -Wextra -Werror=return-type -Werror=empty-body"
dnl CFLAGS="$warnings $CFLAGS"
dnl CXXFLAGS="$warnings $CXXFLAGS"

# Enforce dependencies
if test x"$have_odbc" = x"yes"; then
	AC_DEFINE(HAVE_ODBC,[],[Have DB module])
fi
if test x"$enable_dballef" = x"yes"; then
	AC_DEFINE(HAVE_DBALLEF,[],[Have Fortran bindings])
fi
if test x"$enable_dballe_python" = x"yes"; then
	AC_DEFINE(HAVE_DBALLE_PYTHON,[],[Have Python bindings])
fi

AM_CONDITIONAL([HAVE_ODBC], [test x"$have_odbc" = x"yes"])
AM_CONDITIONAL([DO_DBALLEF], [test x"$enable_dballef" = x"yes"])
AM_CONDITIONAL([DO_DBALLE_PYTHON], [test x"$enable_dballe_python" = x"yes"])
AM_CONDITIONAL([DO_DOCS], [test x"$enable_docs" = x"yes"])

AM_CONDITIONAL([HAVE_LATEX2HTML], [test x$have_latex2html = xyes])
AM_CONDITIONAL([HAVE_RST2HTML], [test x$have_rst2html = xyes])

AC_CONFIG_FILES([
Makefile
dballe/Makefile
fortran/Makefile
src/Makefile
tables/Makefile
extra/Makefile
python/Makefile
doc/Makefile
doc/libdballe.dox
doc/libdballef.dox
libdballe.pc
libdballef.pc
])
AC_OUTPUT

AC_MSG_NOTICE([
===================================================
$PACKAGE_NAME-$PACKAGE_VERSION configuration:
AS_HELP_STRING([ODBC:], [$have_odbc])
AS_HELP_STRING([PostgreSQL:], [$have_libpq])
AS_HELP_STRING([MySQL:], [$have_mysql])
AS_HELP_STRING([Fortran:], [$enable_dballef])
AS_HELP_STRING([Python:], [$enable_dballe_python])
AS_HELP_STRING([documentation:], [$enable_docs])
AS_HELP_STRING([latex2html:], [$have_latex2html])
AS_HELP_STRING([rst2html:], [$have_rst2html])
===================================================])
