dnl Process this file with autoconf to produce a configure script.

AC_INIT(dballe, [9.3], [enrico@enricozini.org])
AC_CONFIG_SRCDIR([configure.ac])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

dnl Use libtool
dnl AM_ENABLE_STATIC
dnl AM_DISABLE_SHARED
LT_INIT()
LT_LANG([C++])
AC_PROG_LIBTOOL

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

dnl Check for python
AM_PATH_PYTHON([3], [], [
	enable_dballe_python=no
])
if test x$enable_dballe_python = xyes
then
    PYTHON_CFLAGS=`unset CFLAGS; $PYTHON-config --cflags`
    AC_SUBST(PYTHON_CFLAGS)
    AX_PYTHON_MODULE_VERSION(wreport, 3.22)
    AX_PYTHON_MODULE(numpy, yes)
fi

dnl Export information about the current major python version
dnl FIXME: remove when we finally only support python3
PYTHON_MAJOR_VERSION=[`$PYTHON -c 'import sys; print(sys.version_info[0])'`]
AC_SUBST(PYTHON_MAJOR_VERSION)
AM_CONDITIONAL([HAVE_PYTHON2], [test $PYTHON_MAJOR_VERSION = 2])
AM_CONDITIONAL([HAVE_PYTHON3], [test $PYTHON_MAJOR_VERSION = 3])

AC_ARG_VAR([WREPORT_DOXYGEN_DIR], [path of wreport doxygen doc])
if test x$WREPORT_DOXYGEN_DIR = x
then
    WREPORT_DOXYGEN_DIR=/usr/share/doc/wreport/html/doxygen
    test -e "$WREPORT_DOXYGEN_DIR" || WREPORT_DOXYGEN_DIR="/usr/share/doc/libwreport-doc/apidocs/"
    test -e "$WREPORT_DOXYGEN_DIR" || WREPORT_DOXYGEN_DIR="/usr/share/doc/libwreport-doc/html/doxygen/"
    test -e "$WREPORT_DOXYGEN_DIR" || WREPORT_DOXYGEN_DIR="/usr/share/doc/libwreport-dev/html/doxygen/"
fi
AC_ARG_VAR([WREPORT_DOXYGEN_TAGFILE], [path of libwreport.doxytags])
if test x$WREPORT_DOXYGEN_TAGFILE = x
then
    for location in libwreport-doc libwreport-dev wreport
    do
        WREPORT_DOXYGEN_TAGFILE="/usr/share/doc/${location}/libwreport.doxytags"
	test -e "$WREPORT_DOXYGEN_TAGFILE" && break
	WREPORT_DOXYGEN_TAGFILE="/usr/share/doc/${location}/libwreport.doxytags.gz"
	test -e "$WREPORT_DOXYGEN_TAGFILE" && break
    done
fi
if test x$enable_docs = xyes
then
    if test ! -d "$WREPORT_DOXYGEN_DIR"
    then
        wreport_doxygen_dir_result=no
    else
        wreport_doxygen_dir_result=yes
    fi
    if test ! -e "$WREPORT_DOXYGEN_TAGFILE"
    then
        wreport_doxygen_tagfile_result=no
    else
        wreport_doxygen_tagfile_result=yes
    fi
    AC_MSG_CHECKING([for wreport documentation dir $WREPORT_DOXYGEN_DIR])
    AC_MSG_RESULT([$wreport_doxygen_dir_result])
    AC_MSG_CHECKING([for wreport doxytag file $WREPORT_DOXYGEN_TAGFILE])
    AC_MSG_RESULT([$wreport_doxygen_tagfile_result])
    if test x$wreport_doxygen_dir_result = xno
    then
        AC_MSG_WARN([WREPORT_DOXYGEN_DIR not found, documentation will not be built.])
        enable_docs=no
    fi
    if test x$wreport_doxygen_tagfile_result = xno
    then
        AC_MSG_WARN([WREPORT_DOXYGEN_TAGFILE not found, documentation will not be built.])
        enable_docs=no
    fi

    dnl Check for doxygen
    AC_CHECK_PROGS(DOXYGEN, [doxygen], [no])
    if test $DOXYGEN = no
    then
        AC_MSG_WARN([doxygen was not found: documentation will not be built])
        enable_docs=no
    fi

    AC_CHECK_PROGS(SPHINX_BUILD, [sphinx-build sphinx-build-3 sphinx-build-$PYTHON_VERSION], [no])
    if test $SPHINX_BUILD = no
    then
        enable_docs=no
    fi
    AX_PYTHON_MODULE(breathe)
    if test x$HAVE_PYMOD_BREATHE != xyes
    then
        enable_docs=no
    fi
fi

AC_SUBST(DOXYGEN)
AC_SUBST(SPHINX_BUILD)

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

AC_SYS_LARGEFILE

dnl See http://www.gossamer-threads.com/lists/xen/devel/275132
FILE_OFFSET_BITS_64=
if test $ac_cv_sys_file_offset_bits = 64
then
    FILE_OFFSET_BITS_64=yes
fi
AM_CONDITIONAL([FILE_OFFSET_BITS_64], [test x"$FILE_OFFSET_BITS_64" = x"yes"])

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

        DOXYGEN_OUTPUT_DIR="$ac_abs_confdir/doc/"
        AC_SUBST(DOXYGEN_OUTPUT_DIR)
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

PKGCONFIG_LIBS="-lm"
PKGCONFIG_CFLAGS=""
PKGCONFIG_REQUIRES="libwreport >= 3.19 sqlite3"
AC_SUBST(PKGCONFIG_LIBS)
AC_SUBST(PKGCONFIG_CFLAGS)
AC_SUBST(PKGCONFIG_REQUIRES)

dnl Check for wreport
PKG_CHECK_MODULES(WREPORT,libwreport >= 3.22)

dnl Check for sqlite3
PKG_CHECK_MODULES(SQLITE3, sqlite3)


dnl Check for libpq
PKG_CHECK_EXISTS(libpq, [have_libpq=yes], [have_libpq=no])
if test x$have_libpq = xyes
then
    PKG_CHECK_MODULES(LIBPQ, libpq)
    AC_DEFINE([HAVE_LIBPQ], 1, [PostgreSQL is available])
    PKGCONFIG_REQUIRES="$PKGCONFIG_REQUIRES libpq"
else
    dnl libpq without .pc file
    have_libpq=yes
    AC_CHECK_LIB([pq], [PQconnectStart], [true], [have_libpq=no])
    AC_CHECK_HEADER([libpq-fe.h], [true], [have_libpq=no])
    LIBPQ_CFLAGS=
    LIBPQ_LIBS="-lpq"
    PKGCONFIG_LIBS="$PKGCONFIG_LIBS -lpq"
    AC_SUBST(LIBPQ_CFLAGS)
    AC_SUBST(LIBPQ_LIBS)
    AC_DEFINE([HAVE_LIBPQ], 1, [PostgreSQL is available])
fi
AM_CONDITIONAL([HAVE_LIBPQ], [test x"$have_libpq" = x"yes"])


dnl Check for libmysqlclient
PKG_CHECK_EXISTS([mariadb], [have_mysql=yes], [have_mysql=no])
if test x$have_mysql = xyes
then
    PKG_CHECK_MODULES(MYSQL, mariadb)
    AC_DEFINE([HAVE_MYSQL], 1, [MySQL is available])
    PKGCONFIG_REQUIRES="$PKGCONFIG_REQUIRES mariadb"
else
    AC_PATH_PROGS(mysql_config, [mysql_config mariadb_config])
    if test x$mysql_config != x
    then
        have_mysql=yes
        MYSQL_CFLAGS=`$mysql_config --cflags`
        MYSQL_LIBS=`$mysql_config --libs_r`
        PKGCONFIG_CFLAGS="$PKGCONFIG_CFLAGS $MYSQL_CFLAGS"
        PKGCONFIG_LIBS="$PKGCONFIG_LIBS $MYSQL_LIBS"
        AC_SUBST(MYSQL_CFLAGS)
        AC_SUBST(MYSQL_LIBS)
        AC_DEFINE([HAVE_MYSQL], 1, [MySQL is available])
    fi
fi
AM_CONDITIONAL([HAVE_MYSQL], [test x"$have_mysql" = x"yes"])


dnl Check for Xapian
PKG_CHECK_EXISTS(xapian-core, [have_xapian=yes], [have_xapian=no])
if test x$have_xapian = xyes
then
    PKG_CHECK_MODULES(XAPIAN, xapian-core >= 1.4)
    AC_DEFINE([HAVE_XAPIAN], 1, [Xapian is available])
    PKGCONFIG_REQUIRES="$PKGCONFIG_REQUIRES xapian-core"
fi
AM_CONDITIONAL([HAVE_XAPIAN], [test x"$have_xapian" = x"yes"])


dnl Check for popt
PKG_CHECK_EXISTS([popt], [have_popt=yes], [have_popt=no])
if test x$have_popt = xyes
then
    PKG_CHECK_MODULES(POPT,popt,,[have_popt=no])
    PKGCONFIG_REQUIRES="$PKGCONFIG_REQUIRES popt"
else
    AC_CHECK_HEADER(popt.h, [have_popt=yes], [have_popt=no])
    if test x$have_popt = xno
    then
        AC_MSG_ERROR([*** popt.h not found. Check 'config.log' for more details.])
    fi
    AC_CHECK_LIB(popt, poptGetContext, [have_popt=yes], [have_popt=no])
    if test x$have_popt = xno
    then
        AC_MSG_ERROR([*** libpopt not found. Check 'config.log' for more details.])
    fi
    POPT_LIBS="-lpopt"
    PKGCONFIG_LIBS="$PKGCONFIG_LIBS $POPT_LIBS"
    AC_SUBST(POPT_LIBS)
fi
AC_DEFINE(HAVE_POPT_H, 1, popt.h has been found)

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

LIBDBALLE_VERSION_INFO="9:0:0"
LIBDBALLEF_VERSION_INFO="5:0:0"
AC_SUBST(LIBDBALLE_VERSION_INFO)
AC_SUBST(LIBDBALLEF_VERSION_INFO)

dnl Enable extra compiler warnings
AX_CXXFLAGS_WARN_ALL
dnl warnings="-Wall -Wextra -Werror=return-type -Werror=empty-body"
dnl CFLAGS="$warnings $CFLAGS"
dnl CXXFLAGS="$warnings $CXXFLAGS"
AX_APPEND_FLAG([-Wextra -Wno-unused-parameter -Wno-error=cpp])
AX_CHECK_COMPILE_FLAG([-Wredundant-move], [has_redundant_move=yes], [has_redundant_move=no])
if test $has_redundant_move = yes
then
	AX_APPEND_FLAG([-Wno-error=redundant-move])
fi

# Enforce dependencies
if test x"$enable_dballef" = x"yes"; then
	AC_DEFINE(HAVE_DBALLEF,[],[Have Fortran bindings])
fi
if test x"$enable_dballe_python" = x"yes"; then
	AC_DEFINE(HAVE_DBALLE_PYTHON,[],[Have Python bindings])
fi

AM_CONDITIONAL([DO_DBALLEF], [test x"$enable_dballef" = x"yes"])
AM_CONDITIONAL([DO_DBALLE_PYTHON], [test x"$enable_dballe_python" = x"yes"])
AM_CONDITIONAL([DO_DOCS], [test x"$enable_docs" = x"yes"])

AC_CONFIG_FILES([
Makefile
dballe/Makefile
fortran/Makefile
src/Makefile
tables/Makefile
extra/Makefile
python/Makefile
bench/Makefile
doc/Makefile
doc/libdballe.dox
libdballe.pc
libdballef.pc
])
AC_OUTPUT

AC_MSG_NOTICE([
===================================================
$PACKAGE_NAME-$PACKAGE_VERSION configuration:
AS_HELP_STRING([PostgreSQL:], [$have_libpq])
AS_HELP_STRING([MySQL:], [$have_mysql])
AS_HELP_STRING([Xapian:], [$have_xapian])
AS_HELP_STRING([Fortran:], [$enable_dballef])
AS_HELP_STRING([Python:], [$enable_dballe_python])
AS_HELP_STRING([documentation:], [$enable_docs])
===================================================])
