# LIBDBALLE_CORE_DEFS([LIBDBALLE_CORE_REQS=libdballe-core])
# ---------------------------------------
AC_DEFUN([LIBDBALLE_CORE_DEFS],
[
	dnl Import libdballe-core data
	PKG_CHECK_MODULES(LIBDBALLE_CORE,m4_default([$1], libdballe-core))
	AC_SUBST(LIBDBALLE_CORE_CFLAGS)
	AC_SUBST(LIBDBALLE_CORE_LIBS)
])
