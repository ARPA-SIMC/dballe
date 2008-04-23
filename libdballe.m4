# LIBDBALLE_DEFS([LIBDBALLE_REQS=libdballe])
# ---------------------------------------
AC_DEFUN([LIBDBALLE_DEFS],
[
	dnl Import libdballe data
	PKG_CHECK_MODULES(LIBDBALLE,m4_default([$1], libdballe))
	AC_SUBST(LIBDBALLE_CFLAGS)
	AC_SUBST(LIBDBALLE_LIBS)
])
