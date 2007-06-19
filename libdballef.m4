# LIBDBALLEF_DEFS([LIBDBALLEF_REQS=libdballef])
# ---------------------------------------
AC_DEFUN([LIBDBALLEF_DEFS],
[
	dnl Import libdballef data
	PKG_CHECK_MODULES(LIBDBALLEF,m4_default([$1], libdballef))

	dnl Check for the existance of cnf and add its compile and link flags
	AC_LANG_PUSH([C])
	AC_CHECK_LIB([cnf], [cnfImpn],
		[LIBDBALLEF_LIBS="-lcnf $LIBDBALLEF_LIBS"],
		[AC_MSG_ERROR([libcnf library not found])])
	AC_LANG_POP([C])

	AC_SUBST(LIBDBALLEF_CFLAGS)
	AC_SUBST(LIBDBALLEF_LIBS)
])
