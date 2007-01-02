# LIBDBALLEPP_DEFS([LIBDBALLEPP_REQS=libdballepp])
# ---------------------------------------
AC_DEFUN([LIBDBALLEPP_DEFS],
[
	dnl Import libdballepp data
	PKG_CHECK_MODULES(LIBDBALLEPP,m4_default([$1], libdballepp))
	AC_SUBST(LIBDBALLEPP_CFLAGS)
	AC_SUBST(LIBDBALLEPP_LIBS)
])
