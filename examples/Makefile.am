export DBA_REPINFO=../tables/repinfo.csv

noinst_PROGRAMS=dump_ana

dump_ana_SOURCES=\
	dump_ana.f
dump_ana_LDADD = ../fortran/libdballef.la ../dballe/libdballe.la -lcnf @CHECK_LIBS@
dump_ana_FFLAGS = -g -fno-second-underscore

../tables/dballe.idx: ../tables/dballe.txt ../tools/dballetool
	$(MAKE) -C ../tables indexes

INCLUDES = -I..
