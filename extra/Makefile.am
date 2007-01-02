noinst_INCLUDES = \
	cmdline.h \
	conversion.h \
	processor.h \
	test-utils-core.h \
	test-utils-bufrex.h \
	test-utils-msg.h \
	test-utils.h \
	tut.h \
	tut_reporter.h \
	tut_restartable.h

noinst_LTLIBRARIES = libcmdline.la libextra.la libtests.la libtests-main.la
libcmdline_la_SOURCES = \
	cmdline.c

libextra_la_SOURCES = \
	cmdline.c \
	conversion.c \
	processor.c

libtests_la_SOURCES = \
	test-utils-core.cc \
	test-utils-bufrex.cc \
	test-utils-msg.cc \
	test-utils-db.cc

libtests_main_la_SOURCES = \
	test-utils-core.cc \
	test-utils-bufrex.cc \
	test-utils-msg.cc \
	test-utils-db.cc \
	tut-main.cc

EXTRA_DIST = \
	runtest \
	test-bufr-table.txt \
	test-crex-d-table.txt \
	test-crex-table.txt
