Summary: DB-ALLe is a database for punctual metereological data
Name: dballe
Version: 0.4
Release: 2
License: GPL
Group: Applications/Meteo
URL: http://www.arpa.emr.it/sim
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires: unixODBC-devel, gperf, starmet, tetex, tetex-latex, doxygen
Requires: MyODBC

 
%description
DB-ALLe is a database for punctual metereological data
         intended to complement the existing tools to manage grid-based data.

 These are the main characteristics of DB-ALLe:

    * it is temporary, to be used for a limited time and then be deleted.
    * does not need backup, since it only contains replicated or derived data.
    * write access is enabled for its users.
    * it is fast for both read and for write access.
    * it is based on physical principles, that is, the data it contains are defined in terms of 
      omogeneous and consistent physical data. For example, it is impossible for two incompatible 
      values to exist in the same point in space and time.
    * it can manage fixed station and moving stations such as airplanes or ships.
    * it can manage both observational and forecast data.
    * it can manage data along all three dimensions in space, such as data from soundings and airplanes.
    * it can work based on physical parameters or on report types.
%prep
%setup -q
%configure -disable-unittest

%build
make

%install
#rm -rf $RPM_BUILD_ROOT
%makeinstall

%clean
#rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)


%dir /usr/bin
%dir /usr/include/dballe
%dir /usr/lib
%dir /usr/share/dballe
%dir /usr/share/aclocal
%dir /var/cache/dballe
%dir /etc/dballe

/usr/bin/
/usr/include/dballe/
/usr/lib/
/usr/share/dballe/
/usr/share/aclocal/
/var/cache/dballe/
/etc/dballe/

#   /usr/bin/dbadb
#   /usr/bin/dbamsg
#   /usr/bin/dbatbl
#   /usr/include/dballe/aof/aof_decoder.h
#   /usr/include/dballe/bufrex/bufr_decoder.h
#   /usr/include/dballe/bufrex/bufr_encoder.h
#   /usr/include/dballe/bufrex/bufrex_conv.h
#   /usr/include/dballe/bufrex/bufrex_dtable.h
#   /usr/include/dballe/bufrex/bufrex_opcode.h
#   /usr/include/dballe/bufrex/bufrex_raw.h
#   /usr/include/dballe/bufrex/crex_decoder.h
#   /usr/include/dballe/bufrex/crex_encoder.h
#   /usr/include/dballe/conv/dba_conv.h
#   /usr/include/dballe/core/dba_array.h
#   /usr/include/dballe/core/dba_csv.h
#   /usr/include/dballe/core/dba_record.h
#   /usr/include/dballe/core/dba_var.h
#   /usr/include/dballe/core/dba_varconv.h
#   /usr/include/dballe/dba_export.h
#   /usr/include/dballe/dba_file.h
#   /usr/include/dballe/dba_import.h
#   /usr/include/dballe/dballe.h
#   /usr/include/dballe/err/dba_error.h
#   /usr/include/dballe/io/dba_file_readers.h
#   /usr/include/dballe/io/dba_file_writers.h
#   /usr/include/dballe/io/dba_rawfile.h
#   /usr/include/dballe/io/dba_rawmsg.h
#   /usr/include/dballe/msg/dba_msg.h
#   /usr/include/dballe/msg/dba_msg_buoy.h
#   /usr/include/dballe/msg/dba_msg_synop.h
#   /usr/lib/libdballe.a
#   /usr/lib/libdballe.la
#   /usr/lib/libdballef.a
#   /usr/lib/libdballef.la
#   /usr/lib/pkgconfig/libdballe.pc
#   /usr/share/aclocal/libdballe.m4
#   /usr/share/dballe/B000000000980000.txt
#   /usr/share/dballe/B000000000980201.txt
#   /usr/share/dballe/B000000000980600.txt
#   /usr/share/dballe/B000000000980601.txt
#   /usr/share/dballe/B000000000981101.txt
#   /usr/share/dballe/B000000000981200.txt
#   /usr/share/dballe/B000000000981201.txt
#   /usr/share/dballe/B000000002551200.txt
#   /usr/share/dballe/B000101.txt
#   /usr/share/dballe/B000103.txt
#   /usr/share/dballe/B000203.txt
#   /usr/share/dballe/D000000000980000.txt
#   /usr/share/dballe/D000000000980201.txt
#   /usr/share/dballe/D000000000980600.txt
#   /usr/share/dballe/D000000000980601.txt
#   /usr/share/dballe/D000000000981101.txt
#   /usr/share/dballe/D000000000981200.txt
#   /usr/share/dballe/D000000000981201.txt
#   /usr/share/dballe/D000000002551200.txt
#   /usr/share/dballe/D000101.txt
#   /usr/share/dballe/D000103.txt
#   /usr/share/dballe/D000203.txt
#   /usr/share/dballe/dballe.txt
#   /var/cache/dballe/B000000000980000.idx
#   /var/cache/dballe/B000000000980201.idx
#   /var/cache/dballe/B000000000980600.idx
#   /var/cache/dballe/B000000000980601.idx
#   /var/cache/dballe/B000000000981101.idx
#   /var/cache/dballe/B000000000981200.idx
#   /var/cache/dballe/B000000000981201.idx
#   /var/cache/dballe/B000000002551200.idx
#   /var/cache/dballe/B000101.idx
#   /var/cache/dballe/B000103.idx
#   /var/cache/dballe/B000203.idx
#   /var/cache/dballe/D000000000980000.idx
#   /var/cache/dballe/D000000000980201.idx
#   /var/cache/dballe/D000000000980600.idx
#   /var/cache/dballe/D000000000980601.idx
#   /var/cache/dballe/D000000000981101.idx
#   /var/cache/dballe/D000000000981200.idx
#   /var/cache/dballe/D000000000981201.idx
#   /var/cache/dballe/D000000002551200.idx
#   /var/cache/dballe/D000101.idx
#   /var/cache/dballe/D000103.idx
#   /var/cache/dballe/D000203.idx
#   /var/cache/dballe/dballe.idx

%doc /usr/share/man/

#   /usr/share/man/man1/dbadb.1.gz
#   /usr/share/man/man1/dbamsg.1.gz
#   /usr/share/man/man1/dbatbl.1.gz

%postun
/sbin/ldconfig


%changelog
* Tue Sep 13 2005 root <root@strip.metarpa> 
- Initial build.


