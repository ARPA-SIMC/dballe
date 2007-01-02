## Process this file with automake to produce Makefile.in

SUBDIRS = . aof bufrex

messageincludedir = $(includedir)/dballe/msg

# Include the .cc files that contain template definitions
messageinclude_HEADERS = \
		datum.h \
		level.h \
		vars.h \
		msg.h \
		msgs.h \
		readers.h \
		writers.h \
		marshal.h \
		file.h

noinst_LTLIBRARIES = libmsg.la
libmsg_la_SOURCES = \
		vars.c \
		datum.c \
		level.c \
		msg.c \
		msgs.c \
		readers.c \
		writers.c \
		marshal.c \
		file.c

vars.h vars.c: vars.csv mkvars
	./mkvars < $<

EXTRA_DIST = msg.dox mkvars vars.csv vars.h vars.c
