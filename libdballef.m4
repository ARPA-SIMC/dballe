# LIBDBALLEF_DEFS([LIBDBALLEF_REQS=libdballef])
# ---------------------------------------
AC_DEFUN([LIBDBALLEF_DEFS],
[
	dnl Import libdballef data
	PKG_CHECK_MODULES(LIBDBALLEF,m4_default([$1], libdballef))
	AC_SUBST(LIBDBALLEF_CFLAGS)
	AC_SUBST(LIBDBALLEF_LIBS)
])
