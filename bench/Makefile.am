## Process this file with automake to produce Makefile.in

DBALLELIBS =  ../dballe/libdballe.la

AM_CPPFLAGS = -I$(top_srcdir) $(WREPORT_CFLAGS) $(LUA_CFLAGS) -Werror
if FILE_OFFSET_BITS_64
AM_CPPFLAGS += -D_FILE_OFFSET_BITS=64
endif

noinst_PROGRAMS = import query

import_SOURCES = import.cc
import_LDFLAGS = $(DBALLELIBS)
import_DEPENDENCIES = $(DBALLELIBS)

query_SOURCES = query.cc
query_LDFLAGS = $(DBALLELIBS)
query_DEPENDENCIES = $(DBALLELIBS)
