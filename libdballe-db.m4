# LIBDBALLE_DB_DEFS([LIBDBALLE_DB_REQS=libdballe-db])
# ---------------------------------------
AC_DEFUN([LIBDBALLE_DB_DEFS],
[
	dnl Import libdballe-db data
	PKG_CHECK_MODULES(LIBDBALLE_DB,m4_default([$1], libdballe-db))
	AC_SUBST(LIBDBALLE_DB_CFLAGS)
	AC_SUBST(LIBDBALLE_DB_LIBS)
])
