# LIBDBALLE_MSG_DEFS([LIBDBALLE_MSG_REQS=libdballe-msg])
# ---------------------------------------
AC_DEFUN([LIBDBALLE_MSG_DEFS],
[
	dnl Import libdballe-msg data
	PKG_CHECK_MODULES(LIBDBALLE_MSG,m4_default([$1], libdballe-msg))
	AC_SUBST(LIBDBALLE_MSG_CFLAGS)
	AC_SUBST(LIBDBALLE_MSG_LIBS)
])
