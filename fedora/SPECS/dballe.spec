Summary: DB-ALLe is a database for punctual metereological data  (Command line tools)
Name: dballe
Version: 7.24
Release: 1
License: GPL
Group: Applications/Meteo
URL: https://github.com/ARPA-SIMC/dballe
#Source0: %{name}-%{version}.tar.gz
Source0: https://github.com/arpa-simc/%{name}/archive/v%{version}-%{release}.tar.gz#/%{name}-%{version}-%{release}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires: gperf, doxygen, python-docutils, lua-devel, libwreport-devel >= 3.2 , python-devel, python3-devel, popt-devel, postgresql-devel, mariadb-devel, sqlite-devel, help2man, libwreport-doc, python-wreport3, python3-wreport3
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}, python-dballe
Obsoletes: provami <= 7.6


%description
 Database for punctual meteorological data (Command line tools)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This framework allows to manage large amounts of data using its simple
 Application Program Interface, and provides tools to visualise, import
 and export in the standard formats BUFR, AOF and CREX.
 .
 The main characteristics of DB-ALL.e are:
 .
  * Fortran, C, C++ and Python APIs are provided.
  * To make computation easier, data is stored as physical quantities,
    that is, as measures of a variable in a specific point of space and
    time, rather than as a sequence of report.
  * Internal representation is similar to BUFR and CREX WMO standard
    (table code driven) and utility for import and export are included
    (generic and ECMWF template).
  * Representation is in 7 dimensions: latitude and longitude geographic
    coordinates, table driven vertical coordinate, reference time,
    table driven observation and forecast specification, table driven
    data type.
  * It allows to store extra information linked to the data, such as
    confidence intervals for quality control.
  * It allows to store extra information linked to the stations.
  * Variables can be represented as real, integer and characters, with
    appropriate precision for the type of measured value.
  * It is based on physical principles, that is, the data it contains are
    defined in terms of homogeneous and consistent physical data. For
    example, it is impossible for two incompatible values to exist in the
    same point in space and time.
  * It can manage fixed stations and moving stations such as airplanes or
    ships.
  * It can manage both observational and forecast data.
  * It can manage data along all three dimensions in space, such as data
    from soundings and airplanes.
  * Report information is preserved. It can work based on physical
    parameters or on report types.

%package  -n libdballe-devel
Summary:  DB-ALL.e core C development library
Group:    Applications/Meteo
Requires: lib%{name}6 = %{?epoch:%epoch:}%{version}-%{release}, lua-devel, postgresql-devel, mariadb-devel, sqlite-devel, popt-devel
Obsoletes: libdballepp-devel 

%description -n libdballe-devel
 DB-ALL.e core C development library
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.

The Fedora packaging of DB-All.e includes all the features of the libraries,
 but any subset can be used without interference from other subsets.  It is
 also possible to rebuild the library to include only those features that are
 needed.
 .
 Features provided:
 .
  * Unit conversion
  * Handling of physical variables
  * Encoding and decoding of BUFR and CREX reports from:
     * fixed land and sea stations, like synops and buoys
     * mobile stations: ships, airplanes
     * soundings: temp, pilot
     * METAR reports
     * Satellite strides (decode only)
  * Decoding of AOF reports
  * Interpretation of weather reports as physical data precisely located in
    space and time, and encoding of physical data into weather reports.
  * Smart on-disk database for observed and forecast weather data based on
    physical principles, built to support operations such as quality control,
    data thinning, correlation of data from mixed sources

%package -n libdballe-doc
Summary:   DB-ALL.e core C development library (documentation)
Group: Applications/Meteo
%description  -n libdballe-doc
 DB-ALL.e core C development library (documentation)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the documentation for the core DB_All.e development library.


%package  -n libdballe6
Summary:   DB-ALL.e core shared library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}
Obsoletes: libdballe5, libdballe4, libdballepp4 

%description -n libdballe6
DB-ALL.e C shared library
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 
 This is the shared library for C programs.


%package -n libdballef-devel

Summary:  DB-All.e Fortran development library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}, lua-devel

%description -n libdballef-devel
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the DB-All.e Fortran API, designed to make it easy to use the DB-All.e
 database as a smart working area for meteorological software.


%package -n libdballef4

Summary:  DB-ALL.e Fortran shared library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballef4
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the shared library for Fortran programs.


%package common

Summary:  Common data files for all DB-All.e modules
Group:    Applications/Meteo

%description common
Common data files for all DB-All.e modules
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This package contains common DB-All.e data files, including variable metadata,
 BUFR and CREX decoding tables, report metadata, level and time range
 descriptions.

%package -n python-dballe
Summary:  DB-ALL.e Python library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}, %{?fedora:rpy}, numpy, python-wreport3

%description -n python-dballe
 DB-ALL.e Python library for weather research
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.

 These are the python bindings.

%package -n python3-dballe
Summary:  DB-ALL.e Python library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}, %{?fedora:rpy}, python3-numpy, python3-wreport3

%description -n python3-dballe
 DB-ALL.e Python library for weather research
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.

 These are the python bindings.

%prep
%setup -q -n %{name}-%{version}-%{release}

rm -rf %{py3dir}
cp -a . %{py3dir}

%build

autoreconf -ifv

%configure FC=gfortran F90=gfortan F77=gfortran --enable-dballef --enable-dballe-python --enable-docs
make

pushd %{py3dir}
autoreconf -ifv
%configure PYTHON=%{__python3} FC=gfortran F90=gfortan F77=gfortran --enable-dballef --enable-dballe-python --enable-docs
popd
make

%install
[ "%{buildroot}" != / ] && rm -rf "%{buildroot}"

pushd %{py3dir}
make install DESTDIR="%{buildroot}" STRIP=/bin/true
mkdir -p $RPM_BUILD_ROOT%{_fmoddir}
mv $RPM_BUILD_ROOT%{_includedir}/*.mod $RPM_BUILD_ROOT%{_fmoddir}
popd

make install DESTDIR="%{buildroot}" STRIP=/bin/true
mkdir -p $RPM_BUILD_ROOT%{_fmoddir}
mv $RPM_BUILD_ROOT%{_includedir}/*.mod $RPM_BUILD_ROOT%{_fmoddir}


%clean
[ "%{buildroot}" != / ] && rm -rf "%{buildroot}"



%files
%defattr(-,root,root,-)
%{_bindir}/dbadb
%{_bindir}/dbamsg
%{_bindir}/dbatbl
%{_bindir}/dbaexport
%doc %{_mandir}/man1/dbadb*
%doc %{_mandir}/man1/dbamsg*
%doc %{_mandir}/man1/dbatbl*
%doc %{_mandir}/man1/dbaexport*

%files common
%defattr(-,root,root,-)
%{_datadir}/wreport/[BD]*
%{_datadir}/wreport/dballe.txt
%{_datadir}/wreport/repinfo.csv

%files -n libdballe6
%defattr(-,root,root,-)
%{_libdir}/libdballe.so.*

%files -n libdballe-devel
%defattr(-,root,root,-)
%{_includedir}/dballe/*.h
%{_includedir}/dballe/core/*
%{_includedir}/dballe/msg/*
%{_includedir}/dballe/sql/*
%{_includedir}/dballe/db/*
%{_includedir}/dballe/cmdline/*
%{_includedir}/dballe/simple/*

%{_libdir}/libdballe.a
%{_libdir}/libdballe.la
%{_libdir}/libdballe.so
%{_libdir}/pkgconfig/libdballe.pc
%{_datadir}/aclocal/libdballe.m4


%files -n libdballef-devel
%defattr(-,root,root,-)
%{_includedir}/dballe/dballef.h
%{_includedir}/dballe/dballeff.h
%{_libdir}/libdballef*.a
%{_libdir}/pkgconfig/libdballef*
%{_libdir}/libdballef*.la
%{_libdir}/libdballef*.so
%{_datadir}/aclocal/libdballef*.m4
%{_fmoddir}/*.mod



%files -n libdballef4
%defattr(-,root,root,-)
%{_libdir}/libdballef*.so.*


%files -n python-dballe
%defattr(-,root,root,-)
%dir %{python2_sitelib}/dballe
%{python2_sitelib}/dballe/*
%dir %{python2_sitearch}
%{python2_sitearch}/*.a
%{python2_sitearch}/*.la
%{python2_sitearch}/*.so*
%{_bindir}/dbatbl_makeb

%files -n libdballe-doc
%defattr(-,root,root,-)
%doc %{_docdir}/dballe/*


%files -n python3-dballe
%defattr(-,root,root,-)
%dir %{python3_sitelib}/dballe
%{python3_sitelib}/dballe/*
%dir %{python3_sitearch}
%{python3_sitearch}/*.a
%{python3_sitearch}/*.la
%{python3_sitearch}/*.so*
%{_bindir}/dbatbl_makeb

%post
/sbin/ldconfig

%postun
/sbin/ldconfig


%changelog
* Fri Sep 1 2017 Daniele Branchini <dbranchini@arpae.it> - 7.24-1
- closed #82, #87, #91, #93, #97, #98
- added new variable B13211

* Fri Apr 21 2017 Daniele Branchini <dbranchini@arpae.it> - 7.23-1
- closed #85
- added new pollen variables

* Mon Dec 19 2016 Daniele Branchini <dbranchini@arpae.it> - 7.22-1
- closed #76, #77

* Thu Nov 10 2016 Daniele Branchini <dbranchini@arpae.it> - 7.21-1
- closed #30

* Thu Oct 27 2016 Daniele Branchini <dbranchini@arpae.it> - 7.20-1
- closed #73

* Wed Oct 12 2016 Daniele Branchini <dbranchini@arpae.it> - 7.19-1
- closed #41, #47, #48, #49, #50, #51, #58, #65, #72

* Mon Jul 18 2016 Daniele Branchini <dbranchini@arpae.it> - 7.16-1
- closed #63 (removed memdb)
- closed #62 (dbadb export missing date)
- removed warnings for Fedora 24 compiling

* Mon Jul 11 2016 Daniele Branchini <dbranchini@arpae.it> - 7.15-1
- closed #61 (removed cnf)

* Tue May 3 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.14-1
- removed ODBC support
- make the fortran trace file line-buffered (#53)

* Thu Apr 21 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.13-1
- closed #52

* Thu Apr 21 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.12-1
- packaged missing headers
- closed #45

* Tue Apr 19 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.11-1
- Implemented experimental V7 database format for MySQL
- Implemented dbadb info
- Added documentation (see #18, #19)
- Adapted tests to fit the #44 attribute behaviour in V7 databases

* Thu Apr 14 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.10-1
- Added new experimental V7 database format
- closed #45, #46

* Tue Mar 15 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.9-1
- dbadb import tuning
- closed #35, #37

* Thu Feb 25 2016 Davide Cesari <dcesari@arpa.emr.it> - 7.8-1
- closed #33

* Tue Nov 24 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 7.7-2
- managing provami messy legacy

* Tue Nov 24 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 7.7-1
- virtualenv/pip support
- closed #13, #22

* Thu Nov 12 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 7.6-2
- Fix dballe and dballe-python dependencies, excluded old provami files

* Wed Sep 16 2015 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 7.6-1
- Fix JSON import from stdin (issue #11)

* Wed Sep 16 2015 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 7.5-1
- Fixed tests

* Tue Sep 15 2015 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 7.4-1
- JSON import/export (issue #5)
- Fix empty report in import (issue #8)
- Fix opening stdin and stdout from Fortran bindings (issue #3)
- Stable CSV header (issue #1)
- Ported to wreport-3.2
- Removed wibble dependency

* Fri Sep  4 2015 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 7.3-2
- Fixed test

* Fri Sep  4 2015 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 7.3-1
- Encoding parser ignore case

* Mon Aug  3 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 7.2-1
- Requires libwreport v3.0
- Switching to git upstream

* Wed Apr 29 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 7.1-4715%{dist}
- using spostgresql-devel and mariadb-devel

* Wed Feb  4 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 6.8-4479%{dist}
- using sqlite-devel instead of unixodbc

* Wed Feb  5 2014 Daniele Branchini <dbranchini@arpa.emr.it> - 6.6-4233%{dist}
- fixed conversion B07007 (M) <-> B07193 (mm).

* Wed Nov 13 2013 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 6.6-4105%{dist}
- Requires libwreport v2.10

* Wed Sep  4 2013 Paolo Patruno <ppatruno@pigna.metarpa> - 6.3-3991%{dist}
-  * New upstream release
     - refactored cursor+querybuilder for v6 databases
     - do not leave SQL_TIMESTAMP_STRUCT.fraction uninitialised, which caused
       duplicate imports in some cases
     - fortran message api: deal gracefully with uninterpretable messaes
     - fixed data overwrite on V6 databases
     - provami: documented the possibility of passing initial key=val filters
       on command line


* Fri Aug 23 2013 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 6.2-3967%{dist}
- Record::set_from_string: hyphen ("-") as MISSING_INT
- Fixes for PostgreSQL (trim char(n) and sequence support)
- B22049 - Sea-surface temperature
- B15236 - [SIM] C6H6 Concentration

* Tue May 28 2013 Paolo Patruno <ppatruno@pigna.metarpa> - 6.1-3884%{dist}
- dballe 6.1
