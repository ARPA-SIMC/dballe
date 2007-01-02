# LIBDBALLECORE_DEFS([LIBDBALLECORE_REQS=libdballe-core])
# ---------------------------------------
AC_DEFUN([LIBDBALLECORE_DEFS],
[
	dnl Import libdballe-core data
	PKG_CHECK_MODULES(LIBDBALLECORE,m4_default([$1], libdballe-core))
	AC_SUBST(LIBDBALLECORE_CFLAGS)
	AC_SUBST(LIBDBALLECORE_LIBS)
])
