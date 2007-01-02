# LIBDBALLE_BUFREX_DEFS([LIBDBALLE_BUFREX_REQS=libdballe-bufrex])
# ---------------------------------------
AC_DEFUN([LIBDBALLE_BUFREX_DEFS],
[
	dnl Import libdballe-bufrex data
	PKG_CHECK_MODULES(LIBDBALLE_BUFREX,m4_default([$1], libdballe-bufrex))
	AC_SUBST(LIBDBALLE_BUFREX_CFLAGS)
	AC_SUBST(LIBDBALLE_BUFREX_LIBS)
])
